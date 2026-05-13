"""Taichi @ti.kernel definitions for lenia-fft.

Kernels implemented:
  init_kernel_radial_2d / init_kernel_radial_3d   -- sample Chan's quad4 K(r)=(4r(1-r))^4 into a LUT
  init_state_random_2d / init_state_random_3d     -- noise-fill the state
  lenia_step_2d                                   -- one real-space Lenia step (Taichi-real-space backend)
  lenia_step_3d                                   -- one 3D real-space Lenia step
  swap_state_2d / swap_state_3d                   -- ping-pong copy after a step
  paint_splat_2d                                  -- Gaussian brush stroke (LMB-drag / RMB-erase)
  composite_view_2d                               -- pan-zoom-transformed view sampler for canvas.set_image
  extract_slice_3d                                -- 2D cross-section extraction for the 3D viewer

CUDA-vs-Vulkan portability (banked Phase 9):
  All kernels use scalar atomic adds and ti.f32 fields. No ti.atomic_add on
  vector fields. ti.loop_config(block_dim=N) hints are NOT used in v1 — the
  kernels are simple enough that default block sizing is fine on both
  backends. Periodic BCs use modulo arithmetic which Taichi lowers to integer
  remainder; behaves identically on CUDA and Vulkan.

Reference:
  - Bert Chan, "Lenia: Biology of Artificial Life" (2019); Chakazul/Lenia
    on GitHub (MIT license). Kernel: upstream `LeniaNDK.py` kernel_core[0]
    "polynomial (quad4)": K(r) = (4·r·(1-r))^4 for r in [0,1], zero outside.
    Growth map: G(u) = 2*exp(-(u-mu)^2/(2*sigma^2)) - 1 (Gaussian, gn=1).
    All Phase 10 presets are single-peak (b="1"), kn=1 (quad4 kernel),
    gn=1 (Gaussian growth) — see presets.py for the verified roster.
"""

# NOTE: deliberately NO `from __future__ import annotations`. Banked Phase 9
# polish-2: Taichi 1.7.4's @ti.kernel reads argument annotations via
# isinstance() at decoration time; PEP 563 stringifies all annotations,
# causing Taichi to reject ANY annotated arg (ti.template(), float, int — all
# of them). The constraint applies to every file that defines @ti.kernel
# functions with annotated arguments, which this file does. See
# docs/conventions.md "No `from __future__ import annotations` x @ti.kernel".

import taichi as ti

# ----------------------------------------------------------------------
# Kernel LUT initialization — Chan's quad4 polynomial kernel
# ----------------------------------------------------------------------
# K(r) = (4·r·(1-r))^4   for r in [0,1], zero outside.
#
# This is upstream LeniaNDK.py's kernel_core[0] — the "polynomial (quad4)"
# entry. JSON params.kn=1 maps to dict-key 0 via off-by-one indexing, so
# every creature with kn=1 in animals.json (all 122 single-peak creatures
# enumerated upstream) uses THIS kernel, not the exp-bump bump4 variant
# (which is kernel_core[1], JSON kn=2 — used by some polyring creatures
# and v1.1+ extensions, NOT by Phase 10's preset roster).
#
# Quad4 has no singularities at r=0 or r=1:
#   K(0) = (4·0·1)^4 = 0     (the (1-r) factor zeroes it)
#   K(1) = (4·1·0)^4 = 0     (the r factor zeroes it)
#   K(0.5) = (4·0.5·0.5)^4 = 1.0   (peak at center of unit interval)
# So no r_safe defensive clamp is needed; the kernel evaluates cleanly.
# Verified empirically at architect-2 round-3 smoke (no NaN/inf in LUT at
# R=13, 15, 20; see /tmp/probe_quad4_smoke_report.md § 1).

@ti.kernel
def init_kernel_radial_2d(kernel_lut: ti.template(), r_kernel: int):
    """Sample the radial polynomial kernel K(r) = (4·r·(1-r))^4 for r in [0,1]
    into a (2R+1) x (2R+1) LUT centered at (R, R). Zero outside r=1.

    Anchor: Chakazul/Lenia/Python/LeniaNDK.py kernel_core[0] (quad4).
    """
    center = r_kernel
    for i, j in kernel_lut:
        di = float(i - center)
        dj = float(j - center)
        r = ti.sqrt(di * di + dj * dj) / float(r_kernel)
        v = 0.0
        if r < 1.0:
            v = (4.0 * r * (1.0 - r)) ** 4
        kernel_lut[i, j] = v


@ti.kernel
def init_kernel_radial_3d(kernel_lut: ti.template(), r_kernel: int):
    """3D analog of init_kernel_radial_2d. Same quad4 formula, 3D radial distance."""
    center = r_kernel
    for i, j, k in kernel_lut:
        di = float(i - center)
        dj = float(j - center)
        dk = float(k - center)
        r = ti.sqrt(di * di + dj * dj + dk * dk) / float(r_kernel)
        v = 0.0
        if r < 1.0:
            v = (4.0 * r * (1.0 - r)) ** 4
        kernel_lut[i, j, k] = v


# ----------------------------------------------------------------------
# State initialization
# ----------------------------------------------------------------------

@ti.kernel
def init_state_zero_2d(state: ti.template()):
    for i, j in state:
        state[i, j] = 0.0


@ti.kernel
def init_state_zero_3d(state: ti.template()):
    for i, j, k in state:
        state[i, j, k] = 0.0


@ti.kernel
def init_state_random_blob_2d(
    state: ti.template(),
    cx: float, cy: float, radius: float, n_grid: int,
):
    """Initialize the field with a random-noise blob centered at (cx, cy) of
    radius `radius`. Outside the blob the field is zero. This is the canonical
    Lenia seed: a small randomized region to grow from."""
    for i, j in state:
        di = float(i) - cx
        dj = float(j) - cy
        d2 = di * di + dj * dj
        r2 = radius * radius
        if d2 < r2:
            state[i, j] = ti.random()
        else:
            state[i, j] = 0.0
    # n_grid arg unused in body but kept for API symmetry; Taichi specializes
    # on field shape, not on int args, so this doesn't trigger a recompile
    # when called with different n_grid values for the same field.


@ti.kernel
def init_state_random_blob_3d(
    state: ti.template(),
    cx: float, cy: float, cz: float, radius: float, n_grid: int,
):
    """3D analog of init_state_random_blob_2d."""
    for i, j, k in state:
        di = float(i) - cx
        dj = float(j) - cy
        dk = float(k) - cz
        d2 = di * di + dj * dj + dk * dk
        r2 = radius * radius
        if d2 < r2:
            state[i, j, k] = ti.random()
        else:
            state[i, j, k] = 0.0


# ----------------------------------------------------------------------
# Real-space Lenia step (Taichi-real-space backend)
# ----------------------------------------------------------------------

@ti.kernel
def lenia_step_2d(
    state: ti.template(),
    state_next: ti.template(),
    kernel_lut: ti.template(),
    kernel_radius: int,
    n_grid: int,
    dt: float,
    mu: float,
    sigma: float,
):
    """One Lenia step via real-space normalized convolution.

    For each cell (i, j):
      u   = normalized neighborhood-weighted-average of state via kernel_lut
      G   = 2 * exp(-(u-mu)^2 / (2*sigma^2)) - 1            -- growth map
      new = clip(state[i,j] + dt * G, 0, 1)

    Periodic BCs via modulo wrap (canonical Lenia torus).
    """
    for i, j in state:
        u = 0.0
        kernel_sum = 0.0
        for di, dj in ti.ndrange(2 * kernel_radius + 1, 2 * kernel_radius + 1):
            ni = (i + di - kernel_radius + n_grid) % n_grid
            nj = (j + dj - kernel_radius + n_grid) % n_grid
            w = kernel_lut[di, dj]
            u += w * state[ni, nj]
            kernel_sum += w
        if kernel_sum > 0.0:
            u /= kernel_sum
        g = 2.0 * ti.exp(-(u - mu) * (u - mu) / (2.0 * sigma * sigma)) - 1.0
        new = state[i, j] + dt * g
        if new < 0.0:
            new = 0.0
        if new > 1.0:
            new = 1.0
        state_next[i, j] = new


@ti.kernel
def lenia_step_3d(
    state: ti.template(),
    state_next: ti.template(),
    kernel_lut: ti.template(),
    kernel_radius: int,
    n_grid: int,
    dt: float,
    mu: float,
    sigma: float,
):
    """3D analog of lenia_step_2d. Walks a (2R+1)^3 kernel LUT."""
    for i, j, k in state:
        u = 0.0
        kernel_sum = 0.0
        for di, dj, dk in ti.ndrange(
            2 * kernel_radius + 1, 2 * kernel_radius + 1, 2 * kernel_radius + 1
        ):
            ni = (i + di - kernel_radius + n_grid) % n_grid
            nj = (j + dj - kernel_radius + n_grid) % n_grid
            nk = (k + dk - kernel_radius + n_grid) % n_grid
            w = kernel_lut[di, dj, dk]
            u += w * state[ni, nj, nk]
            kernel_sum += w
        if kernel_sum > 0.0:
            u /= kernel_sum
        g = 2.0 * ti.exp(-(u - mu) * (u - mu) / (2.0 * sigma * sigma)) - 1.0
        new = state[i, j, k] + dt * g
        if new < 0.0:
            new = 0.0
        if new > 1.0:
            new = 1.0
        state_next[i, j, k] = new


@ti.kernel
def swap_state_2d(state: ti.template(), state_next: ti.template()):
    """Copy state_next → state (ping-pong after a step)."""
    for i, j in state:
        state[i, j] = state_next[i, j]


@ti.kernel
def swap_state_3d(state: ti.template(), state_next: ti.template()):
    """Copy state_next → state (3D ping-pong)."""
    for i, j, k in state:
        state[i, j, k] = state_next[i, j, k]


# ----------------------------------------------------------------------
# Brush splat (LMB-drag paint / RMB-erase)
# ----------------------------------------------------------------------

@ti.kernel
def paint_splat_2d(
    state: ti.template(),
    cx: float, cy: float,
    radius: float, intensity: float,
    n_grid: int,
):
    """Add a Gaussian splat to the state field, centered at (cx, cy) in
    field-cell coords, with falloff radius `radius` and signed `intensity`.

    The splat is `intensity * exp(-d^2 / (r^2 * 0.5))`, clipped to [0, 1].
    `intensity` may be negative (RMB-erase mode). The 0.5 factor in the
    exponent denominator gives a softer tail than a pure Gaussian — matches
    typical Lenia brush UX where the user wants to "paint a creature shape"
    rather than draw a hard disk.
    """
    r2 = radius * radius
    for i, j in state:
        di = float(i) - cx
        dj = float(j) - cy
        d2 = di * di + dj * dj
        if d2 < r2:
            w = ti.exp(-d2 / (r2 * 0.5))
            new = state[i, j] + intensity * w
            if new > 1.0:
                new = 1.0
            if new < 0.0:
                new = 0.0
            state[i, j] = new
    # n_grid is unused in the kernel body — kept for API symmetry with future
    # variants that need it. Taichi specializes on field shape, not int args.


# ----------------------------------------------------------------------
# Pan-zoom view compositor (2D, → canvas.set_image)
# ----------------------------------------------------------------------

@ti.kernel
def composite_view_2d(
    state: ti.template(),
    view_img: ti.template(),
    pan_x: float, pan_y: float, zoom: float, n_grid: int,
):
    """Sample the state field into view_img with pan + zoom + periodic-BC wrap.

    For each output pixel (i, j) in view_img:
      Map (i, j) → field coord (fx, fy) via inverse pan+zoom transform.
      Wrap fx, fy modulo n_grid (Lenia torus BC; lets the user pan past edges).
      Bilinear-sample state at (fx, fy).
    """
    for i, j in view_img:
        # Window-pixel → window-normalized [0,1].
        ux = float(i) / float(n_grid)
        uy = float(j) / float(n_grid)
        # Inverse pan-zoom: window-norm → field-norm.
        fx_norm = (ux - 0.5) / zoom + 0.5 + pan_x / float(n_grid)
        fy_norm = (uy - 0.5) / zoom + 0.5 + pan_y / float(n_grid)
        # Field-norm → field-cell coord (float for bilinear).
        fx = fx_norm * float(n_grid)
        fy = fy_norm * float(n_grid)
        # Periodic-BC wrap. fmod doesn't handle negative; add n_grid first.
        fx = fx - float(n_grid) * ti.floor(fx / float(n_grid))
        fy = fy - float(n_grid) * ti.floor(fy / float(n_grid))
        # Integer + fractional split for bilinear.
        i0 = int(fx)
        j0 = int(fy)
        i1 = (i0 + 1) % n_grid
        j1 = (j0 + 1) % n_grid
        tx = fx - float(i0)
        ty = fy - float(j0)
        # Bilinear interpolation.
        v00 = state[i0, j0]
        v10 = state[i1, j0]
        v01 = state[i0, j1]
        v11 = state[i1, j1]
        view_img[i, j] = ((1.0 - tx) * (1.0 - ty) * v00
                          + tx * (1.0 - ty) * v10
                          + (1.0 - tx) * ty * v01
                          + tx * ty * v11)


# ----------------------------------------------------------------------
# 3D slice extraction (cross-section viewer)
# ----------------------------------------------------------------------

@ti.kernel
def extract_slice_3d(
    state: ti.template(),
    slice_img: ti.template(),
    axis: int,           # 0 = XY, 1 = XZ, 2 = YZ
    slice_idx: int,
    n_grid: int,
):
    """Extract a 2D slice from the 3D state field for canvas display.

    axis = 0 (XY-plane): slice_img[i, j] = state[i, j, slice_idx]
    axis = 1 (XZ-plane): slice_img[i, j] = state[i, slice_idx, j]
    axis = 2 (YZ-plane): slice_img[i, j] = state[slice_idx, i, j]
    """
    for i, j in slice_img:
        if axis == 0:
            slice_img[i, j] = state[i, j, slice_idx]
        elif axis == 1:
            slice_img[i, j] = state[i, slice_idx, j]
        else:
            slice_img[i, j] = state[slice_idx, i, j]
    # n_grid unused but kept for API symmetry.
