"""hello-py — exercises every gpusims_common module.

Bouncing-particles demo: 4096 particles fall under gravity, bounce off floor /
walls, rendered as a colored point cloud with WASDQE + RMB free-fly camera.

Demonstrates:
  - ti.ui.Window + ti.ui.Scene + Canvas + ti.ui.Camera (wrapped by gpusims_common.Camera)
  - gpusims_common.ParamPanel for sliders / buttons
  - gpusims_common.StateWriter / StateReader (F5 save, F9 load)
  - gpusims_common.VdbWriter (real-or-stub depending on pyopenvdb availability)
  - gpusims_common.AlembicWriter (permanent stub; logs once and skips)
  - gpusims_common.log

Controls:
  WASDQE         Move (RMB held to look around)
  F5             Save state to captures/capture_NNNN/
  F9             Load latest capture
  R              Reset particles
  Space          Pause / unpause
  Esc            Quit

Run:
  cd common/common-py/examples/hello
  pip install -e .
  python main.py
"""

# NOTE: deliberately NO `from __future__ import annotations`. Taichi 1.7.4's
# @ti.kernel decorator rejects string-form argument annotations at decoration
# time (TaichiSyntaxError from kernel_impl.py:631 extract_arguments). The
# `step` kernel below has `dt: float, gravity_y: float` args; under PEP 563
# those annotations become strings and Taichi can't introspect them. The
# constraint applies to ALL annotated-arg kernels (not just ti.template()
# ones — verified empirically against Taichi 1.7.4 with primitive annotations
# post-Phase-9). See the same comment in
# hybrid-particle-grid/mpm-multimaterial/python/mpm_multimaterial/kernels.py for the canonical
# banking of this constraint.

from pathlib import Path
from typing import Final

import numpy as np
import taichi as ti

from gpusims_common import (
    AlembicWriter,
    Camera,
    CameraMode,
    ParamPanel,
    ParticleFrame,
    StateReader,
    StateWriter,
    VdbWriter,
    log,
)

# ----------------------------------------------------------------------
# Configuration
# ----------------------------------------------------------------------

N_PARTICLES: Final[int] = 4096
RES: Final[tuple[int, int]] = (1080, 720)
DT: Final[float] = 1.0 / 120.0
GRAVITY_DEFAULT: Final[float] = -9.81
CAPTURES_DIR: Final[Path] = Path("captures")
VDB_EXPORT_BASE: Final[Path] = Path("vdb_export/hello_density")
ABC_EXPORT_PATH: Final[Path] = Path("abc_export/hello_particles.abc")

# ----------------------------------------------------------------------
# Taichi setup
# ----------------------------------------------------------------------

ti.init(arch=ti.gpu)

x = ti.Vector.field(3, ti.f32, shape=N_PARTICLES)
v = ti.Vector.field(3, ti.f32, shape=N_PARTICLES)
colors = ti.Vector.field(4, ti.f32, shape=N_PARTICLES)


@ti.kernel
def initialize_particles():
    for i in range(N_PARTICLES):
        x[i] = ti.Vector([ti.random() * 0.6 + 0.2, ti.random() * 0.3 + 0.6, ti.random() * 0.6 + 0.2])
        v[i] = ti.Vector([0.0, 0.0, 0.0])
        colors[i] = ti.Vector([0.1, 0.6, 0.9, 1.0])


@ti.kernel
def step(dt: float, gravity_y: float):
    for i in range(N_PARTICLES):
        v[i].y += dt * gravity_y
        x[i] += dt * v[i]
        # bounce off floor + walls of [0,1]^3 box
        for d in ti.static(range(3)):
            if x[i][d] < 0.0:
                x[i][d] = 0.0
                v[i][d] = -v[i][d] * 0.7
            if x[i][d] > 1.0:
                x[i][d] = 1.0
                v[i][d] = -v[i][d] * 0.7


# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------

def main() -> None:
    initialize_particles()

    window = ti.ui.Window("hello-py — gpusims_common", RES, vsync=True)
    canvas = window.get_canvas()
    scene = window.get_scene()

    camera = Camera(mode=CameraMode.FreeFly)
    camera.set_position(0.5, 0.8, 1.8)
    camera.set_lookat(0.5, 0.4, 0.5)
    camera.set_fov_deg(55)

    panel = ParamPanel("hello", persist_key="hello-py")

    state_writer = StateWriter(CAPTURES_DIR)
    state_reader = StateReader(CAPTURES_DIR)

    VDB_EXPORT_BASE.parent.mkdir(parents=True, exist_ok=True)
    ABC_EXPORT_PATH.parent.mkdir(parents=True, exist_ok=True)

    vdb_writer = VdbWriter(
        base=VDB_EXPORT_BASE,
        dims=(32, 32, 32),
        voxel_size=1.0 / 32,
        origin=(0.0, 0.0, 0.0),
        grid_name="density",
    )
    abc_writer = AlembicWriter.create(ABC_EXPORT_PATH, fps=24.0)  # always None in Phase 9 (stub)

    gravity_y = GRAVITY_DEFAULT
    paused = False
    export_vdb_enabled = False
    export_abc_enabled = False
    frame_idx = 0

    def do_save_state() -> None:
        state_writer.begin_frame(frame_idx)
        state_writer.set_meta("helloPy", {
            "camera": camera.to_json(),
            "gravity_y": gravity_y,
            "paused": paused,
        })
        state_writer.save_buffer("x", x.to_numpy().astype(np.float32), shape=[N_PARTICLES, 3])
        state_writer.save_buffer("v", v.to_numpy().astype(np.float32), shape=[N_PARTICLES, 3])
        state_writer.end_frame()
        log.info("saved state at frame=%d → %s", frame_idx, state_writer.current_dir())

    def do_load_state() -> None:
        nonlocal gravity_y, frame_idx
        latest = state_reader.find_latest()
        if latest is None:
            log.warn("no captures to load")
            return
        meta = state_reader.load_meta(latest)
        sim_meta = meta.get("meta", {}).get("helloPy", meta.get("meta", {}))
        cam_json = sim_meta.get("camera")
        if cam_json is not None:
            camera.from_json(cam_json)
        gravity_y = float(sim_meta.get("gravity_y", GRAVITY_DEFAULT))
        x_np = state_reader.load_buffer_reshaped(latest, "x").astype(np.float32)
        v_np = state_reader.load_buffer_reshaped(latest, "v").astype(np.float32)
        x.from_numpy(x_np)
        v.from_numpy(v_np)
        frame_idx = int(meta.get("frame", 0))
        log.info("loaded state from %s (frame=%d)", latest, frame_idx)

    log.info(
        "hello-py started. N=%d, VDB available=%s, Alembic available=%s",
        N_PARTICLES,
        VdbWriter.is_available(),
        abc_writer is not None,
    )

    while window.running:
        # ------------------------------ input ----------------------------
        if window.get_event(ti.ui.PRESS):
            ev = window.event
            # Diagnostic log — banks the actual Taichi event-key strings observed
            # at runtime. Remove or demote to debug-level once F-key behavior is
            # confirmed against the user's hardware/backend (see Phase 9 retro
            # banking for the "Taichi GGUI key constants are undocumented" gap).
            log.info("key event: %r", ev.key)
            if ev.key == ti.ui.ESCAPE:
                window.running = False
            elif ev.key == "r":
                initialize_particles()
                frame_idx = 0
                log.info("particles reset")
            elif ev.key == ti.ui.SPACE:
                paused = not paused
                log.info("paused=%s", paused)
            elif str(ev.key).lower() == "f5":
                do_save_state()
            elif str(ev.key).lower() == "f9":
                do_load_state()

        # ------------------------------ sim ------------------------------
        if not paused:
            step(DT, gravity_y)
            frame_idx += 1

        # ---------------------------- exports ----------------------------
        if export_vdb_enabled and not paused and frame_idx % 4 == 0:
            # Toy density: histogram particle x positions into a 32^3 grid
            x_np = x.to_numpy()
            density, _ = np.histogramdd(
                x_np,
                bins=32,
                range=[(0.0, 1.0), (0.0, 1.0), (0.0, 1.0)],
            )
            vdb_writer.write_frame(frame_idx, density.astype(np.float32))

        if export_abc_enabled and not paused and frame_idx % 4 == 0:
            if abc_writer is not None:
                abc_writer.write_frame(ParticleFrame(
                    positions=x.to_numpy().astype(np.float32),
                    count=N_PARTICLES,
                ))

        # ---------------------------- camera -----------------------------
        camera.track_user_inputs(window)
        scene.set_camera(camera.ti_camera())
        scene.ambient_light((0.3, 0.3, 0.3))
        scene.point_light(pos=(0.5, 1.5, 0.5), color=(0.7, 0.7, 0.7))
        scene.particles(x, per_vertex_color=colors, radius=0.005)
        canvas.scene(scene)

        # ----------------------------- gui -------------------------------
        panel.bind(window.get_gui())
        with panel.folder("Sim", 0.05, 0.05, 0.2, 0.18) as f:
            gravity_y = f.slider_float("gravity y", gravity_y, -20.0, 20.0)
            f.text(f"frame: {frame_idx}")
            f.text(f"paused: {paused}")
            if f.button("reset (R)"):
                initialize_particles()
                frame_idx = 0
        with panel.folder("Export", 0.05, 0.24, 0.2, 0.22) as f:
            export_vdb_enabled = f.checkbox(
                f"VDB ({'real' if VdbWriter.is_available() else 'stub'})",
                export_vdb_enabled,
            )
            export_abc_enabled = f.checkbox(
                f"Alembic ({'real' if abc_writer is not None else 'stub'})",
                export_abc_enabled,
            )
            f.text("save / load:")
            save_clicked = f.button("save state")
            load_clicked = f.button("load latest")
            f.text("(F5/F9 keys also try — may not fire on all backends)")

        if save_clicked:
            do_save_state()
        if load_clicked:
            do_load_state()

        window.show()


if __name__ == "__main__":
    main()
