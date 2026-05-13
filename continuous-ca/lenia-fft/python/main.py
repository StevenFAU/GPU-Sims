"""lenia-fft — Bert Chan's continuous CA on a 2D (+ opt-in 3D) periodic grid.

Phase 10 Stack D consumer #2 of common-py. First sim with runtime FFT-backend
selection: at init, probe CuPy → PyTorch → Taichi real-space → numpy in
priority order and pick the first available. See docs/load-bearing-decisions.md
"Runtime FFT-backend selection" for the full rationale.

Formulation (canonical Chan Lenia, single-channel, asymptotic-update):
  state[t+1] = clip(state[t] + dt * G(K ⊛ state[t]), 0, 1)
where:
  K(r) = (4 * r * (1-r))**4  for r in [0, 1], 0 elsewhere    -- quad4 polynomial kernel
                                                                (upstream LeniaNDK.py kernel_core[0];
                                                                JSON kn=1 via off-by-one indexing)
  ⊛ = normalized convolution (divides by sum-of-kernel-weights)
  G(u) = 2 * exp(-(u - mu)^2 / (2 * sigma^2)) - 1             -- Gaussian growth map (gn=1)
  dt = 1 / T                                                  -- T = time-resolution constant

Controls:
  LMB-drag       Paint (Gaussian splat at cursor; radius + intensity sliders)
  RMB-drag       Erase (negative-intensity paint, same brush profile)
  WASDQE         Camera move (3D tier only — RMB-held to look)
  F5 / F9        Save / load full simulation state
  R              Reset to current preset
  Space          Pause / unpause
  Esc            Quit

Run:
  cd continuous-ca/lenia-fft/python
  pip install -e .            # baseline; uses Taichi real-space convolution
  # OR for GPU FFT acceleration (pick ONE matching your hardware):
  pip install -e .[cuda]      # NVIDIA via CuPy (lab PC RTX 2080 Ti)
  pip install -e .[rocm]      # AMD via PyTorch ROCm (dev desktop RX 6800 XT;
                              # also: pip install torch --index-url https://download.pytorch.org/whl/rocm6.0)
  pip install -e .[cuda-torch] # NVIDIA via PyTorch (alternative to CuPy)
  python main.py
"""

# NOTE: deliberately NO `from __future__ import annotations`. Phase 9 banked
# constraint (polish-2): files in a Stack D sim that may import or proxy a
# @ti.kernel-defining module should defensively omit the future import even
# if they don't define kernels directly. main.py imports kernels.py + paint
# kernels live in-module context. Banking is in docs/conventions.md.

import sys
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
from fft_backend import LeniaConvolver, select_backend

# ----------------------------------------------------------------------
# Configuration
# ----------------------------------------------------------------------

RES: Final[tuple[int, int]] = (1280, 720)
DEFAULT_TIER_IDX: Final[int] = 0

# (label_for_dropdown, dim, n_grid, capture_mode_default).
# Tier 2 (2048²) capture_mode depends on which FFT backend was selected at
# init — the label and capture_mode flag are mutated after backend selection
# in main(); see "Tier-label runtime mutation" below.
# Tier 3 (128³) is always capture-mode; the 3D path uses Taichi real-space
# convolution only (no 3D FFT backend in v1).
TIERS_BASE: Final[list[tuple[str, int, int, bool]]] = [
    ("512^2 (default)",                 2, 512,  False),
    ("1024^2 (mid)",                    2, 1024, False),
    ("2048^2 (stretch)",                2, 2048, True),   # may flip to False post-backend-select
    ("128^3 (3D stretch, capture-mode)", 3, 128,  True),
]

CAPTURES_DIR: Final[Path] = Path("captures")
FRAMES_EXPORT_DIR: Final[Path] = Path("frames_export")
VDB_EXPORT_BASE: Final[Path] = Path("vdb_export/density")

MAX_BRUSH_RADIUS_PX: Final[float] = 80.0


# ----------------------------------------------------------------------
# Sim resources (recreated on tier change)
# ----------------------------------------------------------------------

class SimState:
    """All Taichi fields + per-tier sized state.

    Recreated on tier change. Taichi specializes @ti.kernel on field shapes,
    so a tier change triggers a one-time recompile on the first step call
    (~1-3 s); the UI shows a 'recompiling' label during the gap. Same idiom
    as Phase 9 MPM.

    2D and 3D dimensions are mutually-exclusive per SimState instance:
    state_2d is present for dim==2, state_3d for dim==3. The unused field
    is None.
    """

    def __init__(self, dim: int, n_grid: int, kernel_radius: int) -> None:
        self.dim: int = dim
        self.n_grid: int = n_grid
        self.kernel_radius: int = kernel_radius

        # Kernel LUT (used by Taichi real-space convolver and as a numpy
        # reference for the FFT convolvers).
        lut_size = 2 * kernel_radius + 1
        if dim == 2:
            self.kernel_lut = ti.field(ti.f32, shape=(lut_size, lut_size))
            self.state_2d = ti.field(ti.f32, shape=(n_grid, n_grid))
            self.state_2d_next = ti.field(ti.f32, shape=(n_grid, n_grid))
            self.state_3d = None
            self.state_3d_next = None
            self.slice_2d = None   # 3D-only
            # Pan-zoom view output image (size = window-cell). The view kernel
            # samples state_2d via inverse pan-zoom transform.
            self.view_img = ti.field(ti.f32, shape=(n_grid, n_grid))
        elif dim == 3:
            self.kernel_lut_3d = ti.field(ti.f32, shape=(lut_size, lut_size, lut_size))
            self.state_3d = ti.field(ti.f32, shape=(n_grid, n_grid, n_grid))
            self.state_3d_next = ti.field(ti.f32, shape=(n_grid, n_grid, n_grid))
            # For 3D, the "view" is a 2D cross-section slice. Output image
            # is always (n_grid, n_grid) regardless of which axis is sliced.
            self.slice_2d = ti.field(ti.f32, shape=(n_grid, n_grid))
            self.state_2d = None
            self.state_2d_next = None
            self.kernel_lut = None
            self.view_img = None
        else:
            raise ValueError(f"SimState: dim must be 2 or 3, got {dim}")


# ----------------------------------------------------------------------
# Cursor → field-cell unproject (2D path)
# ----------------------------------------------------------------------

def cursor_to_field_cell(
    cursor_norm: tuple[float, float],
    pan_x: float, pan_y: float, zoom: float,
    n_grid: int,
) -> tuple[int, int]:
    """Convert a normalized cursor coord (Taichi: x right, y up) to a
    field-cell index pair (i, j) on an n_grid x n_grid 2D field.

    Inverse of the pan-zoom composite_view kernel: window-coord → field-coord.
    Periodic-BC wrap: field indices are taken modulo n_grid so painting near
    the edge wraps cleanly to the opposite side (matches Lenia's torus BC).
    """
    cx, cy = cursor_norm
    # Center the window-coord at (0.5, 0.5), apply inverse zoom + pan.
    fx_norm = (cx - 0.5) / zoom + 0.5 + pan_x / n_grid
    fy_norm = (cy - 0.5) / zoom + 0.5 + pan_y / n_grid
    # Convert to field cell index. GGUI cursor y=0 is at the TOP of the
    # window on Taichi 1.7.4 / Vulkan / Ubuntu 24.04 (empirically verified
    # during Phase 10 polish-4 visual-verification gate). Direct mapping:
    # cursor_y=0 → row 0 (top of image). NOTE: the panel-occlusion test at
    # cursor_in_any_panel below uses a (1.0 - cy_bottom) flip inherited
    # from MPM; that flip is verified working in Phase 9 visual verification
    # but appears to contradict this function's convention. Both work in
    # practice; the discrepancy is documented for future investigation in
    # docs/notes.md "Polish-4 GGUI Y-convention asymmetry."
    i = int(fx_norm * n_grid) % n_grid
    j = int(fy_norm * n_grid) % n_grid
    return i, j


# ----------------------------------------------------------------------
# GUI panel rects (used by _cursor_in_any_panel)
# ----------------------------------------------------------------------

# Window-normalized coords (x, y, w, h) with y=0 at TOP to match the
# panel.folder() argument convention. Update this list if any panel
# dimensions change in main()'s GUI block below.
GUI_PANEL_RECTS_2D: Final[list[tuple[float, float, float, float]]] = [
    (0.02, 0.02, 0.22, 0.22),  # Presets
    (0.02, 0.26, 0.22, 0.15),  # Tier
    (0.02, 0.43, 0.22, 0.18),  # Lenia
    (0.02, 0.63, 0.22, 0.14),  # Brush
    (0.02, 0.79, 0.22, 0.20),  # Export
]
GUI_PANEL_RECTS_3D: Final[list[tuple[float, float, float, float]]] = [
    (0.02, 0.02, 0.22, 0.22),  # Presets
    (0.02, 0.26, 0.22, 0.15),  # Tier
    (0.02, 0.43, 0.22, 0.18),  # Lenia
    (0.02, 0.63, 0.22, 0.16),  # Slice (3D only — replaces Brush+View)
    (0.02, 0.81, 0.22, 0.18),  # Export
]


def cursor_in_any_panel(cur: tuple[float, float], rects: list[tuple[float, float, float, float]]) -> bool:
    """Return True if window-cursor `cur` is inside any GUI panel rect.

    Taichi GGUI returns cursor coords with y=0 at BOTTOM (NDC convention)
    but panel.folder() positions panels with y=0 at TOP (screen-space
    convention). We convert cursor y to top-origin before testing.

    Inherited verbatim from MPM main.py:306-318. Documented in
    docs/load-bearing-decisions.md as a STRONG promotion candidate for
    consumer #3 (see § "Promotion-review for common-py").
    """
    cx, cy_bottom = cur
    cy_top = 1.0 - cy_bottom
    for px, py, pw, ph in rects:
        if px <= cx <= px + pw and py <= cy_top <= py + ph:
            return True
    return False


# ----------------------------------------------------------------------
# Per-frame step dispatch
# ----------------------------------------------------------------------

def step_2d(
    state: SimState, convolver: LeniaConvolver,
    dt: float, mu: float, sigma: float,
) -> None:
    """One Lenia step in 2D.

    Pulls state to numpy, hands to the selected FFT/convolution backend,
    pushes the new state back. The numpy round-trip is the price of
    backend-agnostic dispatch — same boundary used by every FFT consumer.
    Taichi-real-space backend skips the numpy round-trip internally (it
    reads/writes the Taichi field directly), but the public step() interface
    is still numpy-in/numpy-out for API consistency.
    """
    state_np: np.ndarray = state.state_2d.to_numpy()
    new_np = convolver.step(state_np, dt, mu, sigma)
    state.state_2d.from_numpy(new_np.astype(np.float32))


def step_3d(
    state: SimState,
    dt: float, mu: float, sigma: float,
) -> None:
    """One Lenia step in 3D using Taichi real-space convolution only.

    v1 does not ship a 3D FFT path (CuPy/Torch both support 3D FFT, but it's
    additional scope; v1.1 banked). The 3D step uses kernels.lenia_step_3d
    which walks a (2R+1)^3 kernel LUT in real space with periodic BCs.
    """
    kernels.lenia_step_3d(
        state.state_3d, state.state_3d_next, state.kernel_lut_3d,
        kernel_radius=state.kernel_radius,
        n_grid=state.n_grid,
        dt=dt, mu=mu, sigma=sigma,
    )
    kernels.swap_state_3d(state.state_3d, state.state_3d_next)


# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------

def main() -> None:
    ti.init(arch=ti.gpu)
    log.info("lenia-fft starting; Taichi arch=%s", ti.cfg.arch)

    # --------- Tier + state allocation ---------
    tiers: list[tuple[str, int, int, bool]] = list(TIERS_BASE)   # mutable copy for backend-dependent re-labeling
    tier_idx = DEFAULT_TIER_IDX
    _tier_label, dim, n_grid, is_capture_mode = tiers[tier_idx]

    preset_list = presets.build_presets()
    # Filter presets by current dimension (2D presets for 2D tiers, 3D for 3D).
    preset_list_for_dim = [(name, p) for (name, p) in preset_list if p.dim == dim]
    if not preset_list_for_dim:
        log.error("No presets for dim=%d; check presets.py", dim)
        sys.exit(1)
    curr_preset_idx = 0
    curr_preset = preset_list_for_dim[curr_preset_idx][1]

    state = SimState(dim=dim, n_grid=n_grid, kernel_radius=curr_preset.kernel_radius)
    presets.apply_preset(state, curr_preset)

    # --------- FFT backend selection (2D only; 3D uses Taichi real-space) ---------
    convolver: LeniaConvolver | None = None
    if dim == 2:
        convolver = select_backend(
            n_grid=n_grid,
            kernel_lut_np=state.kernel_lut.to_numpy(),
            taichi_state=state,                           # for the Taichi-real-space path
        )
        log.info("Selected FFT backend: %s", convolver.name())

    # --------- Tier-label runtime mutation (post-backend-select) ---------
    # If a GPU-FFT backend was selected (CuPy or Torch), the 2048^2 stretch tier
    # is interactive — flip its capture_mode flag and update its label.
    # See docs/load-bearing-decisions.md "Tier-label runtime mutation".
    if convolver is not None and convolver.is_gpu_fft():
        label2, dim2, ng2, _ = tiers[2]
        tiers[2] = (label2.replace("(stretch)", "(stretch, FFT)"), dim2, ng2, False)
    elif convolver is not None:
        # Taichi-real-space or numpy: keep capture-mode framing on the 2048 tier.
        label2, dim2, ng2, _ = tiers[2]
        tiers[2] = (label2.replace("(stretch)", "(stretch, real-space, capture-mode)"), dim2, ng2, True)

    # --------- Window / canvas / camera ---------
    window = ti.ui.Window("lenia-fft — GPU-Sims", RES, vsync=True)
    canvas = window.get_canvas()
    # No window.get_scene() in v1 — Lenia 2D uses canvas.set_image of the
    # state field directly, and the 3D tier uses canvas.set_image of an
    # extracted cross-section slice (no ti.ui.Scene). Volumetric raymarch
    # via ti.ui.Scene is banked v1.1 (see docs/load-bearing-decisions.md
    # "3D viewer in v1").
    camera = Camera(mode=CameraMode.FreeFly)
    camera.set_position(0.5, 0.5, 1.5)
    camera.set_lookat(0.5, 0.5, 0.5)
    camera.set_fov_deg(55)
    camera.set_aspect(RES[0] / RES[1])

    panel = ParamPanel("Lenia", persist_key="lenia-fft")

    state_writer = StateWriter(CAPTURES_DIR)
    state_reader = StateReader(CAPTURES_DIR)

    FRAMES_EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    VDB_EXPORT_BASE.parent.mkdir(parents=True, exist_ok=True)
    CAPTURES_DIR.mkdir(parents=True, exist_ok=True)

    # --------- Runtime mutables ---------
    paused = is_capture_mode   # capture-mode tiers start paused
    pan_x: float = 0.0
    pan_y: float = 0.0
    zoom: float = 1.0
    brush_radius: float = 8.0
    brush_intensity: float = 0.5
    slice_axis: str = "XY"     # 3D tier only
    slice_idx: int = n_grid // 2

    export_png_enabled = False
    export_vdb_enabled = False
    export_video_active = False
    video_manager: Any = None   # ti.tools.VideoManager when active
    # POLISH-3 (visual-verification gate): log the export path on every
    # off→on transition so the user knows where files are landing without
    # having to grep the filesystem.
    prev_export_png_enabled = False
    prev_export_vdb_enabled = False

    frame_idx = 0
    pending_tier_change: int | None = None
    pending_capture_mode_modal: int | None = None

    # --------- Save / load handlers (closure over above mutables) ---------
    def do_save_state() -> None:
        state_writer.begin_frame(frame_idx)
        meta_wrapper: dict[str, Any] = {
            "dim": dim,
            "tier_idx": tier_idx,
            "n_grid": n_grid,
            "kernel_radius": curr_preset.kernel_radius,
            "time_resolution": curr_preset.time_resolution,
            "mu": curr_preset.mu,
            "sigma": curr_preset.sigma,
            "preset_name": preset_list_for_dim[curr_preset_idx][0],
            "view": {"pan_x": pan_x, "pan_y": pan_y, "zoom": zoom},
            "fft_backend_at_save": (convolver.name() if convolver is not None else "n/a-3d"),
            "brush": {"radius": brush_radius, "intensity": brush_intensity},
        }
        if dim == 3:
            meta_wrapper["camera"] = camera.to_json()
            meta_wrapper["slice_axis"] = slice_axis
            meta_wrapper["slice_idx"] = slice_idx

        state_writer.set_meta("leniaFft", meta_wrapper)

        N = state.n_grid
        if dim == 2:
            arr_2d = state.state_2d.to_numpy().astype(np.float32)
            state_writer.save_buffer("state", arr_2d, shape=[N, N])
            lut = state.kernel_lut.to_numpy().astype(np.float32)
            R2 = 2 * state.kernel_radius + 1
            state_writer.save_buffer("kernel_lut", lut, shape=[R2, R2])
        else:
            arr_3d = state.state_3d.to_numpy().astype(np.float32)   # type: ignore[union-attr]
            state_writer.save_buffer("state", arr_3d, shape=[N, N, N])
            lut3 = state.kernel_lut_3d.to_numpy().astype(np.float32)
            R3 = 2 * state.kernel_radius + 1
            state_writer.save_buffer("kernel_lut", lut3, shape=[R3, R3, R3])
        # POLISH-3 (visual-verification gate): capture the path BEFORE
        # end_frame() — end_frame() clears StateWriter._current_dir to None,
        # so logging current_dir() after end_frame() always prints "→ None".
        saved_dir = state_writer.current_dir()
        state_writer.end_frame()
        log.info("saved capture at frame=%d → %s", frame_idx, saved_dir)

    def do_load_state() -> None:
        nonlocal state, tier_idx, n_grid, dim, curr_preset_idx, curr_preset
        nonlocal pan_x, pan_y, zoom, slice_axis, slice_idx, brush_radius, brush_intensity
        nonlocal frame_idx, convolver, preset_list_for_dim
        latest = state_reader.find_latest()
        if latest is None:
            log.warn("no captures to load")
            return
        meta_blob = state_reader.load_meta(latest)
        if "leniaFft" not in meta_blob.get("meta", {}):
            log.warn("capture at %s lacks 'leniaFft' meta wrapper; ignoring", latest)
            return
        sim_meta = meta_blob["meta"]["leniaFft"]
        saved_dim = int(sim_meta.get("dim", 2))
        saved_tier_idx = int(sim_meta.get("tier_idx", tier_idx))
        saved_n_grid = int(sim_meta.get("n_grid", n_grid))
        saved_kernel_radius = int(sim_meta.get("kernel_radius", curr_preset.kernel_radius))
        # If tier or dim changed, recreate SimState. This triggers a kernel recompile
        # (~1-3 s) on the next step call — accepted, banked Phase 9.
        if saved_dim != dim or saved_n_grid != n_grid or saved_tier_idx != tier_idx:
            log.info("loaded capture differs (dim=%d→%d, n_grid=%d→%d); reallocating",
                     dim, saved_dim, n_grid, saved_n_grid)
            tier_idx = saved_tier_idx
            dim = saved_dim
            n_grid = saved_n_grid
            preset_list_for_dim = [(name, p) for (name, p) in presets.build_presets() if p.dim == dim]
            state = SimState(dim=dim, n_grid=n_grid, kernel_radius=saved_kernel_radius)
            if dim == 2:
                # Rebuild the FFT backend for the new grid size.
                # Kernel LUT needs to be populated from the saved buffer below.
                pass  # convolver re-selected after lut load
        # Restore preset (name lookup, default to current if not found in dim).
        saved_preset_name = sim_meta.get("preset_name", "")
        for i, (name, _p) in enumerate(preset_list_for_dim):
            if name == saved_preset_name:
                curr_preset_idx = i
                curr_preset = preset_list_for_dim[i][1]
                break
        # Restore state buffer.
        state_np_flat = state_reader.load_buffer_reshaped(latest, "state").astype(np.float32)
        if dim == 2:
            state.state_2d.from_numpy(state_np_flat)
        else:
            state.state_3d.from_numpy(state_np_flat)   # type: ignore[union-attr]
        # Restore kernel LUT (so we don't recompute from preset on a customized capture).
        lut_np = state_reader.load_buffer_reshaped(latest, "kernel_lut").astype(np.float32)
        if dim == 2:
            state.kernel_lut.from_numpy(lut_np)
        else:
            state.kernel_lut_3d.from_numpy(lut_np)
        # Restore view / brush / camera.
        view = sim_meta.get("view", {})
        pan_x = float(view.get("pan_x", 0.0))
        pan_y = float(view.get("pan_y", 0.0))
        zoom = float(view.get("zoom", 1.0))
        brush = sim_meta.get("brush", {})
        brush_radius = float(brush.get("radius", 8.0))
        brush_intensity = float(brush.get("intensity", 0.5))
        if dim == 3:
            cam_json = sim_meta.get("camera")
            if cam_json is not None:
                camera.from_json(cam_json)
            slice_axis = str(sim_meta.get("slice_axis", "XY"))
            slice_idx = int(sim_meta.get("slice_idx", n_grid // 2))
        # Re-select FFT backend for the new grid size if 2D.
        if dim == 2:
            convolver = select_backend(
                n_grid=n_grid,
                kernel_lut_np=state.kernel_lut.to_numpy(),
                taichi_state=state,
            )
            log.info("Reselected FFT backend after load: %s", convolver.name())
        else:
            convolver = None
        frame_idx = int(meta_blob.get("frame", 0))
        log.info("loaded capture from %s (frame=%d, preset=%s)", latest, frame_idx, saved_preset_name)

    # --------- LMB-drag state ---------
    # Lenia uses DRAG (not edge-trigger): hold LMB and move cursor → continuous splats.
    # No `lmb_was_pressed` state is needed; we poll is_pressed every frame.

    log.info("Tier: %s | dim=%d | n_grid=%d | preset=%s",
             tiers[tier_idx][0], dim, n_grid, preset_list_for_dim[curr_preset_idx][0])

    while window.running:
        # ------------------------------ input ----------------------------
        if window.get_event(ti.ui.PRESS):
            ev = window.event
            log.info("key event: %r", ev.key)
            if ev.key == ti.ui.ESCAPE:
                window.running = False
            elif ev.key == "r":
                presets.apply_preset(state, curr_preset)
                frame_idx = 0
                log.info("reset to preset '%s'", preset_list_for_dim[curr_preset_idx][0])
            elif ev.key == ti.ui.SPACE:
                paused = not paused
            elif str(ev.key).lower() == "f5":
                do_save_state()
            elif str(ev.key).lower() == "f9":
                do_load_state()

        # ------------------------------ brush ----------------------------
        # LMB-drag (no edge trigger) gated against GUI panels.
        gui_rects = GUI_PANEL_RECTS_3D if dim == 3 else GUI_PANEL_RECTS_2D
        cur = window.get_cursor_pos()
        if dim == 2:
            lmb_held = window.is_pressed(ti.ui.LMB)
            rmb_held = window.is_pressed(ti.ui.RMB)
            paint_active = (lmb_held or rmb_held) and not cursor_in_any_panel(cur, gui_rects)
            if paint_active:
                i, j = cursor_to_field_cell(cur, pan_x, pan_y, zoom, n_grid)
                intensity = brush_intensity if lmb_held else -abs(brush_intensity)
                kernels.paint_splat_2d(
                    state.state_2d,
                    cx=float(i), cy=float(j),
                    radius=brush_radius, intensity=intensity,
                    n_grid=n_grid,
                )

        # ------------------------------ sim ------------------------------
        if not paused:
            if dim == 2:
                step_2d(
                    state, convolver,   # type: ignore[arg-type]
                    dt=1.0 / curr_preset.time_resolution,
                    mu=curr_preset.mu, sigma=curr_preset.sigma,
                )
            else:
                step_3d(
                    state,
                    dt=1.0 / curr_preset.time_resolution,
                    mu=curr_preset.mu, sigma=curr_preset.sigma,
                )
            frame_idx += 1

        # ---------------------------- view -------------------------------
        if dim == 2:
            kernels.composite_view_2d(
                state.state_2d, state.view_img,
                pan_x=pan_x, pan_y=pan_y, zoom=zoom, n_grid=n_grid,
            )
            canvas.set_image(state.view_img)
        else:
            # 3D: extract a cross-section slice via the kernel, display via canvas.
            kernels.extract_slice_3d(
                state.state_3d, state.slice_2d,
                axis=("XY", "XZ", "YZ").index(slice_axis),
                slice_idx=slice_idx, n_grid=n_grid,
            )
            canvas.set_image(state.slice_2d)

        # ---------------------------- exports ----------------------------
        if export_png_enabled and not paused and frame_idx % 4 == 0:
            img_to_save = state.view_img if dim == 2 else state.slice_2d
            path = FRAMES_EXPORT_DIR / f"frame_{frame_idx:04d}.png"
            ti.tools.imwrite(img_to_save, str(path))

        if export_vdb_enabled and not paused and frame_idx % 4 == 0 and dim == 3:
            grid_np = state.state_3d.to_numpy().astype(np.float32)   # type: ignore[union-attr]
            VdbWriter(
                base=VDB_EXPORT_BASE,
                dims=(n_grid, n_grid, n_grid),
                voxel_size=1.0 / n_grid,
                origin=(0.0, 0.0, 0.0),
                grid_name="density",
            ).write_frame(frame_idx, grid_np)

        if export_video_active and not paused and video_manager is not None:
            img_to_record = state.view_img if dim == 2 else state.slice_2d
            # Invariant: SimState.__init__ guarantees view_img is set for
            # dim==2 and slice_2d is set for dim==3, so img_to_record is
            # non-None in both branches. The Union typing on SimState's
            # fields can't carry that narrowing across the conditional —
            # assert here so mypy follows.
            assert img_to_record is not None
            video_manager.write_frame(img_to_record.to_numpy())

        # ----------------------------- gui -------------------------------
        panel.bind(window.get_gui())

        # Presets panel
        with panel.folder("Presets", 0.02, 0.02, 0.22, 0.22) as f:
            for i, (name, _p) in enumerate(preset_list_for_dim):
                if f.checkbox(name, curr_preset_idx == i):
                    if curr_preset_idx != i:
                        curr_preset_idx = i
                        curr_preset = preset_list_for_dim[i][1]
                        presets.apply_preset(state, curr_preset)
                        frame_idx = 0
                        paused = True

        # Tier panel
        with panel.folder("Tier", 0.02, 0.26, 0.22, 0.15) as f:
            for i, (label, _dim_i, _ng_i, capture_mode_i) in enumerate(tiers):
                if f.checkbox(label, tier_idx == i):
                    if tier_idx != i:
                        if capture_mode_i:
                            pending_capture_mode_modal = i
                        else:
                            pending_tier_change = i

        # Lenia params panel
        with panel.folder("Lenia", 0.02, 0.43, 0.22, 0.18) as f:
            new_mu = f.slider_float("mu", curr_preset.mu, 0.0, 0.5)
            new_sigma = f.slider_float("sigma", curr_preset.sigma, 0.001, 0.1)
            new_T = f.slider_float("T (1/dt)", curr_preset.time_resolution, 1.0, 50.0)
            if new_mu != curr_preset.mu or new_sigma != curr_preset.sigma or new_T != curr_preset.time_resolution:
                # Slider-driven param edit creates a new preset shadow; preset
                # dropdown still shows the named preset but params are live-edited.
                curr_preset = curr_preset.with_overrides(mu=new_mu, sigma=new_sigma, time_resolution=new_T)
            f.text(f"frame: {frame_idx}")
            f.text(f"paused: {paused}")
            f.text(f"FFT: {convolver.name() if convolver else 'n/a-3d'}")
            if f.button("reset (R)"):
                presets.apply_preset(state, curr_preset)
                frame_idx = 0

        # Brush panel (2D only)
        if dim == 2:
            with panel.folder("Brush", 0.02, 0.63, 0.22, 0.14) as f:
                brush_radius = f.slider_float("radius (px)", brush_radius, 1.0, MAX_BRUSH_RADIUS_PX)
                brush_intensity = f.slider_float("intensity", brush_intensity, -1.0, 1.0)
                f.text("LMB-drag: paint, RMB-drag: erase")
        else:
            # 3D: Slice panel
            with panel.folder("Slice", 0.02, 0.63, 0.22, 0.16) as f:
                axes = ("XY", "XZ", "YZ")
                for axis in axes:
                    if f.checkbox(f"axis: {axis}", slice_axis == axis):
                        if slice_axis != axis:
                            slice_axis = axis
                slice_idx = int(f.slider_float("slice index", float(slice_idx), 0.0, float(n_grid - 1)))

        # Export panel
        y_export = 0.79 if dim == 2 else 0.81
        with panel.folder("Export", 0.02, y_export, 0.22, 0.20) as f:
            export_png_enabled = f.checkbox("PNG (every 4th frame)", export_png_enabled)
            if export_png_enabled and not prev_export_png_enabled:
                log.info("PNG export → %s", str(FRAMES_EXPORT_DIR.resolve()))
            prev_export_png_enabled = export_png_enabled
            if dim == 3:
                export_vdb_enabled = f.checkbox(
                    f"VDB density ({'real' if VdbWriter.is_available() else 'stub'})",
                    export_vdb_enabled,
                )
                if export_vdb_enabled and not prev_export_vdb_enabled:
                    log.info("VDB export → %s", str(VDB_EXPORT_BASE.parent.resolve()))
                prev_export_vdb_enabled = export_vdb_enabled
            video_now = f.checkbox("Recording video (MP4)", export_video_active)
            if video_now != export_video_active:
                if video_now:
                    # Start recording: create a new VideoManager.
                    video_dir = FRAMES_EXPORT_DIR / f"video_{frame_idx:04d}"
                    video_manager = ti.tools.VideoManager(
                        output_dir=str(video_dir),
                        framerate=30,
                        automatic_build=False,
                    )
                    log.info("MP4 record → %s (encoded on finalize)", str(video_dir.resolve()))
                else:
                    # Stop recording: build the video and clear.
                    if video_manager is not None:
                        video_manager.make_video(gif=False, mp4=True)
                        log.info("video built")
                    video_manager = None
                export_video_active = video_now
            save_clicked = f.button("save state (F5)")
            load_clicked = f.button("load latest (F9)")

        if save_clicked:
            do_save_state()
        if load_clicked:
            do_load_state()

        # Capture-mode confirmation modal
        if pending_capture_mode_modal is not None:
            modal_idx = pending_capture_mode_modal
            modal_label = tiers[modal_idx][0]
            with panel.folder("⚠ Capture mode", 0.3, 0.4, 0.4, 0.2) as f:
                f.text(f"'{modal_label}' may run at 5-15 fps;")
                f.text("recommended for offline-render frame capture only.")
                if f.button("Continue"):
                    pending_tier_change = modal_idx
                    pending_capture_mode_modal = None
                if f.button("Cancel"):
                    pending_capture_mode_modal = None

        window.show()

        # ---------------------- deferred tier change ---------------------
        if pending_tier_change is not None:
            new_idx = pending_tier_change
            new_label, new_dim, new_ng, new_capture_mode = tiers[new_idx]
            log.info("tier change: %d → %d (%s; dim=%d, n_grid=%d). Recompiling…",
                     tier_idx, new_idx, new_label, new_dim, new_ng)
            tier_idx = new_idx
            dim = new_dim
            n_grid = new_ng

            # Re-filter preset list by new dim (if dim changed).
            preset_list_for_dim = [(name, p) for (name, p) in presets.build_presets() if p.dim == dim]
            curr_preset_idx = 0
            curr_preset = preset_list_for_dim[curr_preset_idx][1]

            state = SimState(dim=dim, n_grid=n_grid, kernel_radius=curr_preset.kernel_radius)
            presets.apply_preset(state, curr_preset)

            slice_idx = n_grid // 2

            # Re-select FFT backend for the new grid size (2D only).
            if dim == 2:
                convolver = select_backend(
                    n_grid=n_grid,
                    kernel_lut_np=state.kernel_lut.to_numpy(),
                    taichi_state=state,
                )
            else:
                convolver = None

            frame_idx = 0
            paused = new_capture_mode
            pending_tier_change = None
            log.info("tier change complete")

    # End-of-session: if video is still recording, build it.
    if video_manager is not None:
        log.info("session ending; finalizing in-flight video")
        video_manager.make_video(gif=False, mp4=True)


if __name__ == "__main__":
    main()
