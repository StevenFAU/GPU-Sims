"""Taichi @ti.kernel definitions for mpm-multimaterial.

Adapted from the canonical upstream `mpm3d_ggui.py`
(taichi-dev/taichi: python/taichi/examples/ggui_examples/mpm3d_ggui.py).

Per-material plasticity branching is preserved 1:1 with upstream:
    WATER (0): mu = 0; after SVD, reset F to identity with new_F[0,0] = J
    JELLY (1): h = 0.3 (small Lame multiplier — always soft)
    SNOW  (2): plastic-clamp singular values to [1 - 2.5e-2, 1 + 4.5e-3];
               hardening h = exp(10 * (1 - Jp)); reconstruct F = U @ sig @ V^T

The only structural change from upstream is parameterization: upstream binds
fields as module globals at decoration time; this module passes them via
`ti.template()` arguments so the per-tier SimState re-allocation triggers a
kernel specialization-recompile against the new field shapes.

CUDA-vs-Vulkan portability: `ti.loop_config(block_dim=n_grid)` is a CUDA-only
hint preserved from upstream; it's a no-op on Vulkan. All grid scatter uses
element-wise scalar atomic adds (the `for d in ti.static(range(3))` shape)
which works on both backends; no `ti.atomic_add(vec_field[I], vec_value)`
calls are made.
"""

# NOTE: deliberately NO `from __future__ import annotations`. Taichi 1.7.4's
# @ti.kernel decorator reads argument annotations via isinstance() at
# decoration time; PEP 563 stringifies all annotations, which causes Taichi
# to reject ANY annotated arg (ti.template(), float, int — all of them)
# with `TaichiSyntaxError: Invalid type annotation`. Verified empirically
# against Taichi 1.7.4 with both ti.template() and primitive annotations.
# The constraint applies to every file that defines @ti.kernel functions
# with annotated arguments: this file, examples/hello/main.py, and
# tests/test_kernels.py in both common-py and the sim. Files whose @ti.kernel
# functions take zero args (nested inside test functions, etc.) are
# technically safe today but defensively also drop the future import.

from typing import Final

import taichi as ti

# Material identifiers — referenced by main.py and presets.py.
WATER: Final[int] = 0
JELLY: Final[int] = 1
SNOW:  Final[int] = 2

# Boundary cells reserved on each side of the grid for the no-penetration
# zero-velocity boundary condition. Matches upstream.
BOUND: Final[int] = 3

# Stash sentinel x position used to park "unused" particles far outside the
# unit cube so scene.particles doesn't render them. Matches upstream's value.
_UNUSED_PARK_POS: Final[float] = 533799.0


@ti.kernel
def substep(
    x: ti.template(),
    v: ti.template(),
    C: ti.template(),
    F: ti.template(),
    Jp: ti.template(),
    materials: ti.template(),
    used: ti.template(),
    grid_v: ti.template(),
    grid_m: ti.template(),
    dx: float, p_vol: float, p_mass: float, dt: float,
    n_grid: ti.template(),
    g_x: float, g_y: float, g_z: float,
    e_young: float, nu_poisson: float,
):
    """One MLS-MPM substep: clear grid, P2G, grid update, G2P.

    The four phases are fused into a single @ti.kernel for backend perf —
    matches upstream's structure. Splitting reduces clarity and (on CUDA)
    introduces extra grid-sync overhead between launches.
    """

    # Lamé parameters
    mu_0 = e_young / (2.0 * (1.0 + nu_poisson))
    lambda_0 = e_young * nu_poisson / ((1.0 + nu_poisson) * (1.0 - 2.0 * nu_poisson))

    # ----- Phase 1: clear the grid ---------------------------------------
    for I in ti.grouped(grid_m):
        grid_v[I] = ti.zero(grid_v[I])
        grid_m[I] = 0.0

    # ----- Phase 2: P2G --------------------------------------------------
    ti.loop_config(block_dim=n_grid)  # CUDA hint; no-op on Vulkan
    for p in x:
        if used[p] == 0:
            continue

        Xp = x[p] / dx
        base = int(Xp - 0.5)
        fx = Xp - base.cast(ti.f32)
        w = [
            0.5 * (1.5 - fx) ** 2,
            0.75 - (fx - 1.0) ** 2,
            0.5 * (fx - 0.5) ** 2,
        ]

        # Deformation gradient update
        F[p] = (ti.Matrix.identity(ti.f32, 3) + dt * C[p]) @ F[p]

        # Hardening coefficient: snow gets harder when compressed
        h = ti.exp(10.0 * (1.0 - Jp[p]))
        if materials[p] == JELLY:
            h = 0.3
        mu = mu_0 * h
        la = lambda_0 * h
        if materials[p] == WATER:
            mu = 0.0

        # SVD-based plasticity
        U, sig, V = ti.svd(F[p], ti.f32)
        J = 1.0
        for d in ti.static(range(3)):
            new_sig = sig[d, d]
            if materials[p] == SNOW:
                new_sig = ti.min(ti.max(sig[d, d], 1.0 - 2.5e-2), 1.0 + 4.5e-3)
            Jp[p] *= sig[d, d] / new_sig
            sig[d, d] = new_sig
            J *= new_sig

        if materials[p] == WATER:
            new_F = ti.Matrix.identity(ti.f32, 3)
            new_F[0, 0] = J
            F[p] = new_F
        elif materials[p] == SNOW:
            F[p] = U @ sig @ V.transpose()

        # MLS-MPM stress
        stress = (
            2.0 * mu * (F[p] - U @ V.transpose()) @ F[p].transpose()
            + ti.Matrix.identity(ti.f32, 3) * la * J * (J - 1.0)
        )
        stress = (-dt * p_vol * 4.0) * stress / (dx * dx)
        affine = stress + p_mass * C[p]

        # Scatter to grid (3x3x3 neighborhood)
        for offset in ti.static(ti.grouped(ti.ndrange(3, 3, 3))):
            dpos = (offset.cast(ti.f32) - fx) * dx
            weight = 1.0
            for i in ti.static(range(3)):
                weight *= w[offset[i]][i]
            grid_v[base + offset] += weight * (p_mass * v[p] + affine @ dpos)
            grid_m[base + offset] += weight * p_mass

    # ----- Phase 3: grid update (gravity + boundary) ---------------------
    for I in ti.grouped(grid_m):
        if grid_m[I] > 0.0:
            grid_v[I] /= grid_m[I]
        grid_v[I] += dt * ti.Vector([g_x, g_y, g_z])
        cond = (I < BOUND) & (grid_v[I] < 0.0) | (I > n_grid - BOUND) & (grid_v[I] > 0.0)
        grid_v[I] = ti.select(cond, 0.0, grid_v[I])

    # ----- Phase 4: G2P --------------------------------------------------
    ti.loop_config(block_dim=n_grid)  # CUDA hint; no-op on Vulkan
    for p in x:
        if used[p] == 0:
            continue

        Xp = x[p] / dx
        base = int(Xp - 0.5)
        fx = Xp - base.cast(ti.f32)
        w = [
            0.5 * (1.5 - fx) ** 2,
            0.75 - (fx - 1.0) ** 2,
            0.5 * (fx - 0.5) ** 2,
        ]
        new_v = ti.zero(v[p])
        new_C = ti.zero(C[p])
        for offset in ti.static(ti.grouped(ti.ndrange(3, 3, 3))):
            dpos = (offset.cast(ti.f32) - fx) * dx
            weight = 1.0
            for i in ti.static(range(3)):
                weight *= w[offset[i]][i]
            g_v = grid_v[base + offset]
            new_v += weight * g_v
            new_C += 4.0 * weight * g_v.outer_product(dpos) / (dx * dx)
        v[p] = new_v
        x[p] += dt * v[p]
        C[p] = new_C


@ti.kernel
def init_cube_volume(
    x: ti.template(),
    v: ti.template(),
    C: ti.template(),
    F: ti.template(),
    Jp: ti.template(),
    materials: ti.template(),
    colors: ti.template(),
    used: ti.template(),
    first_par: int, last_par: int,
    x_begin: float, y_begin: float, z_begin: float,
    x_size: float, y_size: float, z_size: float,
    material: int,
):
    """Initialize particles [first_par, last_par) inside a cube volume.

    Position: uniform random.  Velocity: zero.  F: identity.  Jp: 1.0.
    used: 1.  material: as given.
    """
    for i in range(first_par, last_par):
        x[i] = ti.Vector([
            ti.random() * x_size + x_begin,
            ti.random() * y_size + y_begin,
            ti.random() * z_size + z_begin,
        ])
        v[i] = ti.Vector([0.0, 0.0, 0.0])
        C[i] = ti.Matrix.zero(ti.f32, 3, 3)
        F[i] = ti.Matrix.identity(ti.f32, 3)
        Jp[i] = 1.0
        materials[i] = material
        colors[i] = ti.Vector([1.0, 1.0, 1.0, 1.0])
        used[i] = 1


@ti.kernel
def set_all_unused(
    x: ti.template(),
    v: ti.template(),
    C: ti.template(),
    F: ti.template(),
    Jp: ti.template(),
    used: ti.template(),
):
    """Park all particles as 'unused'; reset to identity / zero so a subsequent
    init_cube_volume call doesn't inherit stale state."""
    for p in used:
        used[p] = 0
        x[p] = ti.Vector([_UNUSED_PARK_POS, _UNUSED_PARK_POS, _UNUSED_PARK_POS])
        v[p] = ti.Vector([0.0, 0.0, 0.0])
        C[p] = ti.Matrix.zero(ti.f32, 3, 3)
        F[p] = ti.Matrix.identity(ti.f32, 3)
        Jp[p] = 1.0


@ti.kernel
def set_color_by_material(
    colors: ti.template(),
    materials: ti.template(),
    mat_color: ti.types.ndarray(),
):
    """Apply per-material colors. `mat_color` shape: (3 materials, 3 RGB)."""
    for p in colors:
        m = materials[p]
        colors[p] = ti.Vector([mat_color[m, 0], mat_color[m, 1], mat_color[m, 2], 1.0])
