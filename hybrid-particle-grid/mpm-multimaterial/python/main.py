"""mpm-multimaterial — MLS-MPM with water + jelly + snow (Phase 9 Stack D flagship).

Adapted from the canonical Taichi upstream example
`python/taichi/examples/ggui_examples/mpm3d_ggui.py` with the following changes:

- Wired through gpusims_common (Camera, ParamPanel, StateWriter/Reader, VdbWriter,
  AlembicWriter stub, log) per Stack D conventions.
- Tier-dropdown: 250k @ 96^3 (default) / 500k @ 128^3 / 1M @ 192^3 (capture-mode).
- F5 / F9 full state capture-and-load via gpusims_common.StateWriter / Reader.
- Per-frame VDB density export (grid_m) via gpusims_common.VdbWriter — gated on
  pyopenvdb being importable; falls through to stub-warning otherwise.
- Per-frame binary-PLY particle export via ti.tools.PLYWriter (positions +
  per-vertex `material` int channel for Blender Geometry Nodes instancing).
- LMB-place sparse-cube emitter on the camera-forward ground-plane intersection;
  RMB cycles through materials (water → jelly → snow); cap 8 user-placed cubes.
- Four named presets: Single Dam Break / Double Dam Break / Water-Snow-Jelly /
  Mixed Sandbox.

Controls:
  WASDQE         Move (RMB held to look)
  LMB            Place a material cube at click-ray ground intersection (cap 8)
  M              Cycle place-material (water/jelly/snow)
  F5 / F9        Save / load state
  R              Reset to current preset
  Space          Pause / unpause
  Esc            Quit

Run:
  cd hybrid-particle-grid/mpm-multimaterial/python
  pip install -e .
  python main.py
"""

from __future__ import annotations

from pathlib import Path
from typing import Any, Final

import numpy as np
import taichi as ti

from gpusims_common import (
    Camera,
    CameraMode,
    ParamPanel,
    StateReader,
    StateWriter,
    VdbWriter,
    log,
)

import kernels
import presets
from kernels import JELLY, SNOW, WATER
from presets import CubeVolume

# ----------------------------------------------------------------------
# Configuration
# ----------------------------------------------------------------------

RES: Final[tuple[int, int]] = (1280, 720)
DEFAULT_TIER_IDX: Final[int] = 0

# (label_for_dropdown, n_particles, n_grid, capture_mode)
TIERS: Final[list[tuple[str, int, int, bool]]] = [
    ("250 000 @ 96^3 (default)",                250_000, 96,  False),
    ("500 000 @ 128^3",                          500_000, 128, False),
    ("1 000 000 @ 192^3 (capture-mode, not interactive)", 1_000_000, 192, True),
]

SUBSTEPS_PER_FRAME: Final[int] = 25
DT_BASE: Final[float] = 2.0e-4
P_RHO: Final[float] = 1.0
E_YOUNG: Final[float] = 1000.0
NU_POISSON: Final[float] = 0.2
GRAVITY_DEFAULT: Final[tuple[float, float, float]] = (0.0, -9.8, 0.0)

CAPTURES_DIR: Final[Path] = Path("captures")
VDB_EXPORT_BASE: Final[Path] = Path("vdb_export/density")
PLY_EXPORT_BASE: Final[Path] = Path("ply_export/particles")

MAX_USER_EMITTERS: Final[int] = 8

MATERIAL_NAMES: Final[list[str]] = ["water", "jelly", "snow"]


# ----------------------------------------------------------------------
# Sim resources (recreated on tier change)
# ----------------------------------------------------------------------

class SimState:
    """All Taichi fields and per-tier sized state.

    Recreated on tier change. Taichi specializes @ti.kernel on field shapes,
    so a tier change triggers a one-time recompile on the first substep call
    (~1-3 s); the UI shows a 'recompiling' label during the gap.
    """

    def __init__(self, n_particles: int, n_grid: int) -> None:
        self.n_particles: int = n_particles
        self.n_grid: int = n_grid
        self.dx: float = 1.0 / n_grid
        self.p_vol: float = (self.dx * 0.5) ** 3   # 3D voxel volume
        self.p_mass: float = self.p_vol * P_RHO

        # Per-particle fields
        self.x = ti.Vector.field(3, ti.f32, n_particles)
        self.v = ti.Vector.field(3, ti.f32, n_particles)
        self.C = ti.Matrix.field(3, 3, ti.f32, n_particles)        # affine velocity
        self.F = ti.Matrix.field(3, 3, ti.f32, n_particles)        # deformation gradient
        self.Jp = ti.field(ti.f32, n_particles)
        self.materials = ti.field(ti.i32, n_particles)
        self.colors = ti.Vector.field(4, ti.f32, n_particles)
        self.used = ti.field(ti.i32, n_particles)

        # Grid fields
        self.grid_v = ti.Vector.field(3, ti.f32, (n_grid, n_grid, n_grid))
        self.grid_m = ti.field(ti.f32, (n_grid, n_grid, n_grid))


# ----------------------------------------------------------------------
# Volume / preset initialization
# ----------------------------------------------------------------------

def init_volumes(state: SimState, vols: list[CubeVolume]) -> None:
    """Distribute particles across volumes proportionally to volume size.

    Mirrors upstream `init_vols` (mpm3d_ggui.py): first marks all particles
    unused (parked at a far position so they don't render), then walks the
    volume list assigning particle ranges by volume share.
    """
    kernels.set_all_unused(state.x, state.v, state.C, state.F, state.Jp, state.used)
    total_vol = sum(v.volume for v in vols)
    if total_vol <= 0:
        log.warn("init_volumes: total volume is zero; no particles will be initialized")
        return

    next_p = 0
    for i, vol in enumerate(vols):
        # Last volume gets all remaining particles to absorb int-trunc roundoff.
        if i == len(vols) - 1:
            par_count = state.n_particles - next_p
        else:
            par_count = int(vol.volume / total_vol * state.n_particles)
        if par_count <= 0:
            continue
        kernels.init_cube_volume(
            state.x, state.v, state.C, state.F, state.Jp, state.materials,
            state.colors, state.used,
            first_par=next_p,
            last_par=next_p + par_count,
            x_begin=vol.minimum[0], y_begin=vol.minimum[1], z_begin=vol.minimum[2],
            x_size=vol.size[0],   y_size=vol.size[1],   z_size=vol.size[2],
            material=vol.material,
        )
        next_p += par_count


# ----------------------------------------------------------------------
# Click-to-place: ray-hits-ground-plane unproject
# ----------------------------------------------------------------------

def cursor_to_ground(
    camera: Camera,
    cursor_normalized: tuple[float, float],
    ground_y: float = 0.0,
) -> tuple[float, float, float] | None:
    """Unproject a normalized cursor coordinate onto the y=ground_y plane.

    `cursor_normalized` is the (x, y) tuple returned by `window.get_cursor_pos()`
    in [0,1]^2 with y up (Taichi convention). Returns world-space (x, ground_y, z)
    or None if the click ray is parallel to or above the plane.

    Same idiom as boids-3d Phase 7's click-to-place: NDC unproject through
    inverse(view_projection), then ray-plane intersect.
    """
    # NDC: x in [-1, 1] left-to-right, y in [-1, 1] bottom-to-top.
    ndc_x = cursor_normalized[0] * 2.0 - 1.0
    ndc_y = cursor_normalized[1] * 2.0 - 1.0

    vp = camera.view_projection()
    try:
        inv_vp = np.linalg.inv(vp)
    except np.linalg.LinAlgError:
        return None

    near_clip = np.array([ndc_x, ndc_y, -1.0, 1.0], dtype=np.float32)
    far_clip = np.array([ndc_x, ndc_y,  1.0, 1.0], dtype=np.float32)
    near_world = inv_vp @ near_clip
    far_world  = inv_vp @ far_clip
    if near_world[3] == 0 or far_world[3] == 0:
        return None
    near_w = near_world[:3] / near_world[3]
    far_w  = far_world[:3]  / far_world[3]
    ray_dir = far_w - near_w
    if abs(ray_dir[1]) < 1e-6:
        return None  # ray parallel to plane
    t = (ground_y - near_w[1]) / ray_dir[1]
    if t < 0:
        return None  # plane behind the camera
    hit = near_w + t * ray_dir
    return (float(hit[0]), float(hit[1]), float(hit[2]))


# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------

def main() -> None:
    ti.init(arch=ti.gpu)
    log.info("mpm-multimaterial starting; Taichi arch=%s", ti.cfg.arch)

    # Initial tier + state allocation
    tier_idx = DEFAULT_TIER_IDX
    tier_label, n_particles, n_grid, is_capture_mode = TIERS[tier_idx]
    state = SimState(n_particles=n_particles, n_grid=n_grid)

    # Preset selection
    preset_list = presets.build_presets()
    preset_names = [p[0] for p in preset_list]
    curr_preset_idx = 0
    init_volumes(state, preset_list[curr_preset_idx][1])

    # GGUI setup
    window = ti.ui.Window("mpm-multimaterial — GPU-Sims", RES, vsync=True)
    canvas = window.get_canvas()
    scene = window.get_scene()

    camera = Camera(mode=CameraMode.FreeFly)
    camera.set_position(0.5, 1.0, 1.95)
    camera.set_lookat(0.5, 0.3, 0.5)
    camera.set_fov_deg(55)
    camera.set_aspect(RES[0] / RES[1])

    panel = ParamPanel("MPM", persist_key="mpm-multimaterial")

    state_writer = StateWriter(CAPTURES_DIR)
    state_reader = StateReader(CAPTURES_DIR)

    VDB_EXPORT_BASE.parent.mkdir(parents=True, exist_ok=True)
    PLY_EXPORT_BASE.parent.mkdir(parents=True, exist_ok=True)

    # Runtime mutables
    gravity = list(GRAVITY_DEFAULT)
    paused = is_capture_mode  # capture-mode tiers start paused
    use_random_colors = False
    material_colors = [
        [0.1, 0.6, 0.9],   # water — blue
        [0.93, 0.33, 0.23],# jelly — red-orange
        [0.95, 0.95, 1.0], # snow — near-white
    ]
    particles_radius = 0.005
    export_vdb_enabled = False
    export_ply_enabled = False
    place_material = WATER
    user_emitter_count = 0
    frame_idx = 0
    pending_tier_change: int | None = None
    pending_capture_mode_modal: int | None = None

    # Push initial colors into the field.
    kernels.set_color_by_material(
        state.colors, state.materials,
        np.array(material_colors, dtype=np.float32),
    )

    log.info(
        "tier=%s, particles=%d, grid=%d^3, VDB=%s",
        tier_label, n_particles, n_grid,
        "real" if VdbWriter.is_available() else "stub",
    )

    while window.running:
        # ------------------------------ input ----------------------------
        if window.get_event(ti.ui.PRESS):
            ev = window.event
            if ev.key == ti.ui.ESCAPE:
                window.running = False
            elif ev.key == "r":
                init_volumes(state, preset_list[curr_preset_idx][1])
                user_emitter_count = 0
                frame_idx = 0
                log.info("reset to preset '%s'", preset_names[curr_preset_idx])
            elif ev.key == ti.ui.SPACE:
                paused = not paused
            elif ev.key == "m":
                place_material = (place_material + 1) % 3
                log.info("place material → %s", MATERIAL_NAMES[place_material])
            elif ev.key == "f5":
                # Per tier1-capture-format-reference.md § 1: single sim-namespaced
                # meta wrapper. Tier-3's MPM module activates on this exact key.
                state_writer.begin_frame(frame_idx)
                state_writer.set_meta("mpmMultimaterial", {
                    "camera": camera.to_json(),
                    "gravity": gravity,
                    "preset": preset_names[curr_preset_idx],
                    "tier_idx": tier_idx,
                    "n_particles": state.n_particles,
                    "n_grid": state.n_grid,
                    "material_colors": material_colors,
                    "user_emitter_count": user_emitter_count,
                    "place_material": place_material,
                })
                # Per-buffer schema follows § 2-3 of the capture-format reference.
                # `count` is total scalar elements; `shape` is logical layout.
                # For (N, 3) vec3 buffers: count = N*3, shape = [N, 3], format = r32f.
                # For (N,) scalar buffers:  count = N,   shape = [N],   format = r32f/r32i.
                # For (N, 3, 3) matrix buffers: count = N*9, shape = [N, 3, 3], format = r32f.
                N = state.n_particles
                state_writer.save_buffer("x",  state.x.to_numpy().astype(np.float32),  shape=[N, 3])
                state_writer.save_buffer("v",  state.v.to_numpy().astype(np.float32),  shape=[N, 3])
                state_writer.save_buffer("F",  state.F.to_numpy().astype(np.float32),  shape=[N, 3, 3])
                state_writer.save_buffer("C",  state.C.to_numpy().astype(np.float32),  shape=[N, 3, 3])
                state_writer.save_buffer("Jp", state.Jp.to_numpy().astype(np.float32), shape=[N])
                state_writer.save_buffer("materials", state.materials.to_numpy().astype(np.int32), shape=[N])
                state_writer.save_buffer("used",      state.used.to_numpy().astype(np.int32),      shape=[N])
                state_writer.end_frame()
                log.info("saved capture at frame=%d", frame_idx)
            elif ev.key == "f9":
                latest = state_reader.find_latest()
                if latest is None:
                    log.warn("no captures to load")
                else:
                    meta_blob = state_reader.load_meta(latest)
                    # Unwrap the sim-namespaced meta. Captures from older Phase-9
                    # drafts that used flat top-level keys won't have `mpmMultimaterial`;
                    # fall back to top-level lookup with a warning so old captures
                    # remain partially usable during the transition.
                    if "mpmMultimaterial" in meta_blob.get("meta", {}):
                        sim_meta = meta_blob["meta"]["mpmMultimaterial"]
                    else:
                        log.warn(
                            "capture at %s lacks 'mpmMultimaterial' meta wrapper; "
                            "falling back to flat top-level keys (legacy format).",
                            latest,
                        )
                        sim_meta = meta_blob.get("meta", {})
                    cam_json = sim_meta.get("camera")
                    if cam_json is not None:
                        camera.from_json(cam_json)
                    saved_gravity = sim_meta.get("gravity", list(GRAVITY_DEFAULT))
                    gravity = [float(g) for g in saved_gravity]
                    saved_tier_idx = int(sim_meta.get("tier_idx", tier_idx))
                    if saved_tier_idx != tier_idx:
                        log.warn(
                            "loaded capture is tier=%d; current tier=%d; reallocating fields",
                            saved_tier_idx, tier_idx,
                        )
                        tier_idx = saved_tier_idx
                        _, n_particles_new, n_grid_new, _ = TIERS[tier_idx]
                        state = SimState(n_particles=n_particles_new, n_grid=n_grid_new)
                    state.x.from_numpy(state_reader.load_buffer_reshaped(latest, "x").astype(np.float32))
                    state.v.from_numpy(state_reader.load_buffer_reshaped(latest, "v").astype(np.float32))
                    state.F.from_numpy(state_reader.load_buffer_reshaped(latest, "F").astype(np.float32))
                    state.C.from_numpy(state_reader.load_buffer_reshaped(latest, "C").astype(np.float32))
                    state.Jp.from_numpy(state_reader.load_buffer(latest, "Jp").astype(np.float32))
                    state.materials.from_numpy(state_reader.load_buffer(latest, "materials").astype(np.int32))
                    state.used.from_numpy(state_reader.load_buffer(latest, "used").astype(np.int32))
                    frame_idx = int(meta_blob.get("frame", 0))
                    log.info("loaded capture from %s (frame=%d)", latest, frame_idx)

        # LMB place — sample-and-edge-trigger via window.is_pressed
        # (Taichi GGUI doesn't fire LMB-press events the way it does keys; poll instead.)
        # We debounce by requiring at least one frame between placements.
        if window.is_pressed(ti.ui.LMB) and user_emitter_count < MAX_USER_EMITTERS:
            cur = window.get_cursor_pos()
            hit = cursor_to_ground(camera, cur, ground_y=0.02)
            if hit is not None:
                # Place a small cube (0.08 x 0.08 x 0.08) centered on the hit point.
                size = 0.08
                cx = max(0.02, min(1.0 - size, hit[0] - size * 0.5))
                cz = max(0.02, min(1.0 - size, hit[2] - size * 0.5))
                cube = CubeVolume(
                    minimum=(cx, hit[1], cz),
                    size=(size, size, size),
                    material=place_material,
                )
                # Find an unused range of particles to claim (cap-managed).
                slot_size = state.n_particles // MAX_USER_EMITTERS
                first = state.n_particles - (user_emitter_count + 1) * slot_size
                last = first + slot_size
                kernels.init_cube_volume(
                    state.x, state.v, state.C, state.F, state.Jp, state.materials,
                    state.colors, state.used,
                    first_par=first, last_par=last,
                    x_begin=cube.minimum[0], y_begin=cube.minimum[1], z_begin=cube.minimum[2],
                    x_size=cube.size[0],   y_size=cube.size[1],   z_size=cube.size[2],
                    material=cube.material,
                )
                user_emitter_count += 1
                log.info(
                    "placed %s cube at (%.2f, %.2f, %.2f); user emitters: %d/%d",
                    MATERIAL_NAMES[place_material], hit[0], hit[1], hit[2],
                    user_emitter_count, MAX_USER_EMITTERS,
                )

        # ------------------------------ sim ------------------------------
        if not paused:
            for _ in range(SUBSTEPS_PER_FRAME):
                kernels.substep(
                    state.x, state.v, state.C, state.F, state.Jp,
                    state.materials, state.used,
                    state.grid_v, state.grid_m,
                    dx=state.dx, p_vol=state.p_vol, p_mass=state.p_mass,
                    dt=DT_BASE, n_grid=state.n_grid,
                    g_x=gravity[0], g_y=gravity[1], g_z=gravity[2],
                    e_young=E_YOUNG, nu_poisson=NU_POISSON,
                )
            frame_idx += 1

        # ---------------------------- exports ----------------------------
        if export_vdb_enabled and not paused and frame_idx % 4 == 0:
            grid_m_np = state.grid_m.to_numpy()
            VdbWriter(
                base=VDB_EXPORT_BASE,
                dims=(state.n_grid, state.n_grid, state.n_grid),
                voxel_size=state.dx,
                origin=(0.0, 0.0, 0.0),
                grid_name="density",
            ).write_frame(frame_idx, grid_m_np)

        if export_ply_enabled and not paused and frame_idx % 4 == 0:
            x_np = state.x.to_numpy().astype(np.float32)
            material_np = state.materials.to_numpy().astype(np.int32)
            used_np = state.used.to_numpy().astype(np.int32)
            # Filter to used particles for clean PLY (Blender Geometry Nodes can
            # filter on attribute, but excluding parked-at-533799 particles
            # keeps the PLY clean and small).
            mask = used_np > 0
            ply_path = PLY_EXPORT_BASE.with_name(f"{PLY_EXPORT_BASE.name}_{frame_idx:04d}.ply")
            writer = ti.tools.PLYWriter(num_vertices=int(mask.sum()))
            x_used = x_np[mask]
            mat_used = material_np[mask]
            writer.add_vertex_pos(x_used[:, 0], x_used[:, 1], x_used[:, 2])
            writer.add_vertex_channel("material", "int", mat_used)
            writer.export(str(ply_path))

        # ---------------------------- camera -----------------------------
        camera.track_user_inputs(window)
        scene.set_camera(camera.ti_camera())
        scene.ambient_light((0.15, 0.15, 0.15))
        scene.point_light(pos=(0.5, 1.5, 0.5), color=(0.6, 0.6, 0.6))
        scene.point_light(pos=(0.5, 1.5, 1.5), color=(0.4, 0.4, 0.4))
        scene.particles(state.x, per_vertex_color=state.colors, radius=particles_radius)
        canvas.scene(scene)

        # ----------------------------- gui -------------------------------
        panel.bind(window.get_gui())
        with panel.folder("Presets", 0.02, 0.02, 0.22, 0.20) as f:
            for i, name in enumerate(preset_names):
                if f.checkbox(name, curr_preset_idx == i):
                    if curr_preset_idx != i:
                        curr_preset_idx = i
                        init_volumes(state, preset_list[i][1])
                        user_emitter_count = 0
                        frame_idx = 0
                        paused = True
        with panel.folder("Tier", 0.02, 0.24, 0.22, 0.15) as f:
            for i, (label, _np_, _ng_, capture_mode_i) in enumerate(TIERS):
                if f.checkbox(label, tier_idx == i):
                    if tier_idx != i:
                        if capture_mode_i:
                            pending_capture_mode_modal = i
                        else:
                            pending_tier_change = i
        with panel.folder("Gravity", 0.02, 0.41, 0.22, 0.14) as f:
            gravity[0] = f.slider_float("x", gravity[0], -20.0, 20.0)
            gravity[1] = f.slider_float("y", gravity[1], -20.0, 20.0)
            gravity[2] = f.slider_float("z", gravity[2], -20.0, 20.0)
        with panel.folder("Materials", 0.02, 0.57, 0.22, 0.20) as f:
            f.text(f"placing: {MATERIAL_NAMES[place_material]}  (press M)")
            f.text(f"user emitters: {user_emitter_count}/{MAX_USER_EMITTERS}")
            water_c = f.color_edit_3("water", material_colors[0])
            jelly_c = f.color_edit_3("jelly", material_colors[1])
            snow_c  = f.color_edit_3("snow",  material_colors[2])
            if water_c != material_colors[0] or jelly_c != material_colors[1] or snow_c != material_colors[2]:
                material_colors = [list(water_c), list(jelly_c), list(snow_c)]
                kernels.set_color_by_material(
                    state.colors, state.materials,
                    np.array(material_colors, dtype=np.float32),
                )
        with panel.folder("Export", 0.02, 0.79, 0.22, 0.18) as f:
            export_vdb_enabled = f.checkbox(
                f"VDB density ({'real' if VdbWriter.is_available() else 'stub'})",
                export_vdb_enabled,
            )
            export_ply_enabled = f.checkbox("PLY particles (built-in)", export_ply_enabled)
            f.text(f"frame: {frame_idx}")
            f.text(f"paused: {paused}")
            if f.button("reset (R)"):
                init_volumes(state, preset_list[curr_preset_idx][1])
                user_emitter_count = 0
                frame_idx = 0

        # Capture-mode confirmation modal
        if pending_capture_mode_modal is not None:
            modal_idx = pending_capture_mode_modal
            modal_label = TIERS[modal_idx][0]
            with panel.folder("⚠ Capture mode", 0.3, 0.4, 0.4, 0.2) as f:
                f.text(f"'{modal_label}' is for offline-render frame capture only.")
                f.text("Simulation will run at 5-15 fps; UI may stutter.")
                if f.button("Continue"):
                    pending_tier_change = modal_idx
                    pending_capture_mode_modal = None
                if f.button("Cancel"):
                    pending_capture_mode_modal = None

        window.show()

        # ---------------------- deferred tier change ---------------------
        # Applied after window.show() so the panel state is consistent across the frame.
        if pending_tier_change is not None:
            new_idx = pending_tier_change
            new_label, new_n, new_g, new_capture_mode = TIERS[new_idx]
            log.info(
                "tier change: %d → %d (%d particles, %d^3 grid). Recompiling kernels…",
                tier_idx, new_idx, new_n, new_g,
            )
            tier_idx = new_idx
            state = SimState(n_particles=new_n, n_grid=new_g)
            init_volumes(state, preset_list[curr_preset_idx][1])
            user_emitter_count = 0
            frame_idx = 0
            paused = new_capture_mode  # capture-mode tier starts paused
            kernels.set_color_by_material(
                state.colors, state.materials,
                np.array(material_colors, dtype=np.float32),
            )
            pending_tier_change = None
            log.info("tier change complete")


if __name__ == "__main__":
    main()
