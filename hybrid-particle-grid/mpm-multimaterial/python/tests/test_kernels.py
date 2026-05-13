"""mpm-multimaterial: sim-level CPU-backend kernel smoke.

Verifies kernels.py imports, the substep kernel compiles against CPU, and a
single substep round-trip produces non-NaN particle positions. Runs in CI
under build-py.yml. Does NOT exercise GPU backends.

Banked: kernels MUST be in a real file on disk for Taichi's AST inspection.
This file IS that file; do not collapse to a parametrized string-based fixture.
"""

from __future__ import annotations

import numpy as np
import taichi as ti


def test_kernels_imports() -> None:
    """kernels.py + presets.py import cleanly."""
    from mpm_multimaterial import kernels, presets  # noqa: F401
    assert kernels.WATER == 0
    assert kernels.JELLY == 1
    assert kernels.SNOW == 2


def test_presets_build() -> None:
    """presets.build_presets returns the four canonical presets."""
    from mpm_multimaterial.presets import build_presets
    presets_list = build_presets()
    names = [name for name, _ in presets_list]
    assert names == ["Single Dam Break", "Double Dam Break", "Water Snow Jelly", "Mixed Sandbox"]
    for name, vols in presets_list:
        assert len(vols) >= 1, f"preset '{name}' has no volumes"
        for v in vols:
            assert v.material in (0, 1, 2), f"preset '{name}': invalid material {v.material}"


def test_substep_compile_cpu() -> None:
    """The MPM substep kernel compiles and runs on the CPU backend.

    Small particle count (256) + small grid (16^3) for fast CI smoke.
    """
    from mpm_multimaterial import kernels

    ti.init(arch=ti.cpu, default_fp=ti.f32)

    n_particles = 256
    n_grid = 16
    dx = 1.0 / n_grid
    p_vol = (dx * 0.5) ** 3
    p_mass = p_vol * 1.0
    dt = 2.0e-4

    x = ti.Vector.field(3, ti.f32, n_particles)
    v = ti.Vector.field(3, ti.f32, n_particles)
    C = ti.Matrix.field(3, 3, ti.f32, n_particles)
    F = ti.Matrix.field(3, 3, ti.f32, n_particles)
    Jp = ti.field(ti.f32, n_particles)
    materials = ti.field(ti.i32, n_particles)
    colors = ti.Vector.field(4, ti.f32, n_particles)
    used = ti.field(ti.i32, n_particles)
    grid_v = ti.Vector.field(3, ti.f32, (n_grid, n_grid, n_grid))
    grid_m = ti.field(ti.f32, (n_grid, n_grid, n_grid))

    kernels.init_cube_volume(
        x, v, C, F, Jp, materials, colors, used,
        first_par=0, last_par=n_particles,
        x_begin=0.3, y_begin=0.3, z_begin=0.3,
        x_size=0.4, y_size=0.4, z_size=0.4,
        material=kernels.JELLY,
    )

    kernels.substep(
        x, v, C, F, Jp, materials, used,
        grid_v, grid_m,
        dx=dx, p_vol=p_vol, p_mass=p_mass, dt=dt,
        n_grid=n_grid,
        g_x=0.0, g_y=-9.8, g_z=0.0,
        e_young=1000.0, nu_poisson=0.2,
    )

    x_np = x.to_numpy()
    assert not np.any(np.isnan(x_np)), "particle positions contain NaN after one substep"
    assert not np.any(np.isinf(x_np)), "particle positions contain Inf after one substep"


if __name__ == "__main__":
    test_kernels_imports()
    test_presets_build()
    test_substep_compile_cpu()
    print("ok — all mpm-multimaterial smoke tests pass")
