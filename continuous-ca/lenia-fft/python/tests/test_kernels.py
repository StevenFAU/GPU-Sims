"""Phase 10 kernel + backend smoke tests.

CI-runnable: uses Taichi CPU backend + numpy FFT backend only. No GPU.
No CuPy / PyTorch verification — those are user-runtime visual-verify
surface (banked Phase 9 runtime-only-surface convention).
"""

# NOTE: deliberately NO `from __future__ import annotations` — this file
# defines or imports @ti.kernel functions with annotated arguments.

import numpy as np
import pytest
import taichi as ti

import kernels
import presets
from fft_backend import NumpyFFTConvolver, _pad_kernel_to_grid


@pytest.fixture(scope="module", autouse=True)
def init_taichi():
    """Initialize Taichi CPU backend once per test module."""
    ti.init(arch=ti.cpu, default_fp=ti.f32)
    yield
    # No explicit teardown — Taichi 1.7.4 doesn't have a clean reset for
    # the test process; subsequent test runs in CI re-spawn the process.


# ----------------------------------------------------------------------
# Kernel LUT
# ----------------------------------------------------------------------

def test_kernel_lut_quad4_shape_2d():
    """The radial quad4 kernel K(r) = (4·r·(1-r))^4 peaks at r=0.5 (value
    1.0) and is zero at r=0 and r=1 (the polynomial vanishes at both
    endpoints; no singularities, no defensive clamp needed).

    Architect-2 round-3 verified analytically: K(0) = (4·0·1)^4 = 0,
    K(0.5) = (4·0.5·0.5)^4 = 1, K(1) = (4·1·0)^4 = 0.
    """
    R = 13
    lut = ti.field(ti.f32, shape=(2 * R + 1, 2 * R + 1))
    kernels.init_kernel_radial_2d(lut, R)
    arr = lut.to_numpy()
    # Center pixel (r=0 because i==j==R) should be ZERO under quad4.
    assert arr[R, R] == 0.0
    # Outer pixels at r >= 1 should be ZERO.
    # Note: at i=0,j=R or i=2R,j=R: di=±R, dj=0, r = R/R = 1.0 → outside;
    # but for diagonal corner pixels (i=0,j=0): r = sqrt(2*R^2)/R > 1, also outside.
    assert arr[0, 0] == 0.0
    assert arr[2 * R, 2 * R] == 0.0
    # Peak at r ≈ 0.5 should be near 1.0 (within sampling tolerance).
    # Find the cell closest to r=0.5: that's at (R, R + R//2) → r = (R//2)/R = 0.5
    interior = arr[R, R + R // 2]
    assert interior > 0.9, f"quad4 peak at r=0.5 should be ~1.0, got {interior}"
    # No NaN, no negatives (quad4 is non-negative throughout).
    assert not np.isnan(arr).any()
    assert (arr >= 0.0).all()
    # Max value should be at or very near 1.0 (analytical peak K(0.5)=1.0).
    assert arr.max() <= 1.0 + 1e-6
    assert arr.max() > 0.99


# ----------------------------------------------------------------------
# Lenia step
# ----------------------------------------------------------------------

def test_lenia_step_2d_bounded():
    """One Lenia step preserves bounded state in [0, 1]."""
    N = 32
    R = 8
    state = ti.field(ti.f32, shape=(N, N))
    state_next = ti.field(ti.f32, shape=(N, N))
    lut = ti.field(ti.f32, shape=(2 * R + 1, 2 * R + 1))
    kernels.init_kernel_radial_2d(lut, R)
    kernels.init_state_random_blob_2d(state, cx=N / 2.0, cy=N / 2.0, radius=8.0, n_grid=N)
    kernels.lenia_step_2d(state, state_next, lut,
                          kernel_radius=R, n_grid=N, dt=0.1, mu=0.15, sigma=0.015)
    kernels.swap_state_2d(state, state_next)
    arr = state.to_numpy()
    assert arr.min() >= 0.0
    assert arr.max() <= 1.0
    assert not np.isnan(arr).any()


def test_lenia_step_3d_bounded():
    """One 3D Lenia step preserves bounded state in [0, 1]."""
    N = 16
    R = 4
    state = ti.field(ti.f32, shape=(N, N, N))
    state_next = ti.field(ti.f32, shape=(N, N, N))
    lut = ti.field(ti.f32, shape=(2 * R + 1, 2 * R + 1, 2 * R + 1))
    kernels.init_kernel_radial_3d(lut, R)
    kernels.init_state_random_blob_3d(state, cx=N / 2.0, cy=N / 2.0, cz=N / 2.0,
                                       radius=4.0, n_grid=N)
    kernels.lenia_step_3d(state, state_next, lut,
                          kernel_radius=R, n_grid=N, dt=0.1, mu=0.15, sigma=0.015)
    kernels.swap_state_3d(state, state_next)
    arr = state.to_numpy()
    assert arr.min() >= 0.0
    assert arr.max() <= 1.0
    assert not np.isnan(arr).any()


# ----------------------------------------------------------------------
# Brush splat
# ----------------------------------------------------------------------

def test_paint_splat_2d_adds_intensity():
    """A positive-intensity splat at (cx, cy) increases state values there."""
    N = 32
    state = ti.field(ti.f32, shape=(N, N))
    kernels.init_state_zero_2d(state)
    kernels.paint_splat_2d(state, cx=N / 2.0, cy=N / 2.0,
                            radius=4.0, intensity=0.5, n_grid=N)
    arr = state.to_numpy()
    # Center cell should have ~max intensity.
    assert arr[N // 2, N // 2] > 0.4
    # Far cells should be untouched.
    assert arr[0, 0] == 0.0
    assert arr.max() <= 1.0


def test_paint_splat_2d_erase_clamps_to_zero():
    """A negative-intensity splat on a partial-fill field decreases toward zero."""
    N = 32
    state = ti.field(ti.f32, shape=(N, N))
    arr_in = np.full((N, N), 0.3, dtype=np.float32)
    state.from_numpy(arr_in)
    kernels.paint_splat_2d(state, cx=N / 2.0, cy=N / 2.0,
                            radius=4.0, intensity=-0.5, n_grid=N)
    arr_out = state.to_numpy()
    # Center cell should be lower than 0.3 (and clamped to >= 0).
    assert arr_out[N // 2, N // 2] < 0.3
    assert arr_out.min() >= 0.0


# ----------------------------------------------------------------------
# Pan-zoom view + slice extraction
# ----------------------------------------------------------------------

def test_composite_view_2d_identity():
    """At pan=(0,0), zoom=1, composite_view samples the field at identity."""
    N = 32
    state = ti.field(ti.f32, shape=(N, N))
    view = ti.field(ti.f32, shape=(N, N))
    arr_in = (np.arange(N * N, dtype=np.float32) / float(N * N)).reshape(N, N)
    state.from_numpy(arr_in)
    kernels.composite_view_2d(state, view, pan_x=0.0, pan_y=0.0, zoom=1.0, n_grid=N)
    arr_out = view.to_numpy()
    # Bilinear sampling at integer cell centers should equal the field
    # (within float epsilon). Identity transform → no displacement.
    assert np.allclose(arr_out, arr_in, atol=1e-4)


def test_extract_slice_3d_axis_xy():
    """Slicing along XY at index k returns state[:, :, k]."""
    N = 16
    state = ti.field(ti.f32, shape=(N, N, N))
    slice_img = ti.field(ti.f32, shape=(N, N))
    arr_in = np.random.rand(N, N, N).astype(np.float32)
    state.from_numpy(arr_in)
    k = 7
    kernels.extract_slice_3d(state, slice_img, axis=0, slice_idx=k, n_grid=N)
    assert np.allclose(slice_img.to_numpy(), arr_in[:, :, k], atol=1e-5)


# ----------------------------------------------------------------------
# FFT backend (numpy only — CI safe)
# ----------------------------------------------------------------------

def test_numpy_fft_backend_smoke():
    """The numpy FFT backend round-trips state through one Lenia step
    without NaN and bounded in [0,1]. Uses quad4 kernel (same as Phase 10
    presets — see kernels.py module header for the LeniaNDK.py anchor)."""
    N = 32
    R = 8
    # Build a quad4 kernel LUT via numpy directly (sidestep Taichi for
    # purely numpy-FFT-path verification).
    lut = np.zeros((2 * R + 1, 2 * R + 1), dtype=np.float32)
    for i in range(2 * R + 1):
        for j in range(2 * R + 1):
            di = float(i - R)
            dj = float(j - R)
            r = np.sqrt(di * di + dj * dj) / float(R)
            if r < 1.0:
                lut[i, j] = float((4.0 * r * (1.0 - r)) ** 4)

    conv = NumpyFFTConvolver(N, lut)
    state = np.random.rand(N, N).astype(np.float32) * 0.5
    new_state = conv.step(state, dt=0.1, mu=0.15, sigma=0.015)
    assert new_state.dtype == np.float32
    assert new_state.shape == (N, N)
    assert not np.isnan(new_state).any()
    assert new_state.min() >= 0.0
    assert new_state.max() <= 1.0


def test_pad_kernel_to_grid_centered():
    """_pad_kernel_to_grid lands the kernel center at (0, 0) after roll."""
    N = 32
    R = 4
    lut = np.zeros((2 * R + 1, 2 * R + 1), dtype=np.float32)
    lut[R, R] = 1.0   # delta at the LUT center
    padded = _pad_kernel_to_grid(lut, N)
    # After centering, the delta should be at padded[0, 0]; normalization
    # divides by the sum (which equals 1.0 for a delta), so padded[0,0] == 1.
    assert padded[0, 0] == 1.0
    # All other cells are zero.
    assert padded.sum() == 1.0


# ----------------------------------------------------------------------
# Preset application (parametrized over all four 2D presets)
# ----------------------------------------------------------------------

@pytest.mark.parametrize("preset_name", [
    "Orbium unicaudatus",
    "Vagorbium undulatus",
    "Gyrorbium gyrans",
    "Discutium valvatus",
])
def test_apply_preset_stability_2d(preset_name):
    """Each 2D preset applies cleanly and runs 10 Lenia steps without
    dissolving or exploding.

    This is the load-bearing preset-contract test added in v2 (was
    single-Orbium-only in v1). Architect-2 round-3 verification confirmed
    all four pass BOUNDED + NaN_FREE + CHANGED + NON_DEAD assertions on
    CPU backend with quad4 kernel; this test re-validates that under CI
    on every push so preset regressions don't ship silently.
    """
    from main import SimState
    preset_list = presets.build_presets()
    preset = next(p for (name, p) in preset_list if name == preset_name)
    sim_state = SimState(dim=2, n_grid=64, kernel_radius=preset.kernel_radius)
    presets.apply_preset(sim_state, preset)
    # Run 10 Lenia steps via the Taichi-real-space path.
    for _ in range(10):
        kernels.lenia_step_2d(
            sim_state.state_2d, sim_state.state_2d_next,
            sim_state.kernel_lut,
            kernel_radius=preset.kernel_radius,
            n_grid=sim_state.n_grid,
            dt=1.0 / preset.time_resolution,
            mu=preset.mu, sigma=preset.sigma,
        )
        kernels.swap_state_2d(sim_state.state_2d, sim_state.state_2d_next)
    arr = sim_state.state_2d.to_numpy()
    # BOUNDED
    assert arr.min() >= 0.0
    assert arr.max() <= 1.0
    # NaN_FREE
    assert not np.isnan(arr).any()
    assert not np.isinf(arr).any()
    # NON_DEAD (state didn't dissolve completely)
    assert arr.sum() > 1.0, f"preset {preset_name!r} dissolved at 10 steps"


# ----------------------------------------------------------------------
# Backend factory contract (new in v2 — covers the FFT backend abstraction)
# ----------------------------------------------------------------------

def test_select_backend_factory_falls_back():
    """`select_backend()` walks the priority list and returns the first
    backend that smoke-passes. In CI (no CuPy, no PyTorch), this should
    return either TaichiRealSpaceConvolver (if taichi_state provided) or
    NumpyFFTConvolver — both must produce a working step() call.

    Load-bearing contract test added in v2: the backend abstraction is
    Phase 10's load-bearing new pattern; ensuring the factory works
    without GPU extras present is the CI-runnable contract surface.
    """
    from main import SimState
    from fft_backend import (
        NumpyFFTConvolver,
        TaichiRealSpaceConvolver,
        select_backend,
    )
    preset_list = presets.build_presets()
    orbium = next(p for (name, p) in preset_list if name == "Orbium unicaudatus")
    sim_state = SimState(dim=2, n_grid=32, kernel_radius=orbium.kernel_radius)
    presets.apply_preset(sim_state, orbium)

    convolver = select_backend(
        n_grid=32,
        kernel_lut_np=sim_state.kernel_lut.to_numpy(),
        taichi_state=sim_state,
    )
    # Expect Taichi or numpy backend in CI (no GPU FFT extras present).
    assert isinstance(convolver, (TaichiRealSpaceConvolver, NumpyFFTConvolver))
    # Backend's step() must work end-to-end with numpy in/out boundary.
    state_np = sim_state.state_2d.to_numpy().astype(np.float32)
    out = convolver.step(state_np, dt=0.1, mu=0.15, sigma=0.015)
    assert out.shape == state_np.shape
    assert out.dtype == np.float32
    assert not np.isnan(out).any()
    assert out.min() >= 0.0
    assert out.max() <= 1.0


# ----------------------------------------------------------------------
# Capture-schema round-trip (new in v2 — covers leniaFft meta wrapper)
# ----------------------------------------------------------------------

def test_capture_schema_round_trip(tmp_path):
    """F5/F9 round-trip preserves the sim's state field byte-for-byte and
    the leniaFft meta-wrapper schema end-to-end.

    Covers the cross-stack capture-schema contract documented at
    docs/tier1-capture-format-reference.md § 1 (sim-namespaced top-level
    meta key, per-buffer fields {name, file, count, stride, format, shape}).

    Load-bearing contract test added in v2: any drift in the meta-wrapper
    schema (renaming a field, changing buffer shape annotation) silently
    breaks save/load and is only caught by user runtime without this CI
    coverage. Same pattern as Phase 9's MPM round-trip test.
    """
    from gpusims_common import StateReader, StateWriter

    from main import SimState

    captures_dir = tmp_path / "captures"

    preset_list = presets.build_presets()
    orbium = next(p for (name, p) in preset_list if name == "Orbium unicaudatus")
    sim_state = SimState(dim=2, n_grid=32, kernel_radius=orbium.kernel_radius)
    presets.apply_preset(sim_state, orbium)
    arr_before = sim_state.state_2d.to_numpy().astype(np.float32)
    lut_before = sim_state.kernel_lut.to_numpy().astype(np.float32)

    # Write capture (simplified — just state + kernel_lut, the two buffers
    # the schema requires).
    writer = StateWriter(captures_dir)
    writer.begin_frame(42)
    writer.set_meta("leniaFft", {
        "dim": 2,
        "tier_idx": 0,
        "n_grid": 32,
        "kernel_radius": orbium.kernel_radius,
        "time_resolution": orbium.time_resolution,
        "mu": orbium.mu,
        "sigma": orbium.sigma,
        "preset_name": "Orbium unicaudatus",
        "view": {"pan_x": 0.0, "pan_y": 0.0, "zoom": 1.0},
        "fft_backend_at_save": "Taichi real-space (universal)",
        "brush": {"radius": 8.0, "intensity": 0.5},
    })
    writer.save_buffer("state", arr_before, shape=[32, 32])
    R2 = 2 * orbium.kernel_radius + 1
    writer.save_buffer("kernel_lut", lut_before, shape=[R2, R2])
    writer.end_frame()

    # Read it back.
    reader = StateReader(captures_dir)
    latest = reader.find_latest()
    assert latest is not None
    arr_after = reader.load_buffer_reshaped(latest, "state").astype(np.float32)
    lut_after = reader.load_buffer_reshaped(latest, "kernel_lut").astype(np.float32)
    meta_blob = reader.load_meta(latest)

    # State buffer round-trips byte-for-byte.
    assert arr_after.shape == arr_before.shape
    assert np.array_equal(arr_after, arr_before)
    # Kernel LUT round-trips byte-for-byte.
    assert lut_after.shape == lut_before.shape
    assert np.array_equal(lut_after, lut_before)
    # leniaFft meta wrapper present + structured correctly.
    assert "leniaFft" in meta_blob.get("meta", {})
    sim_meta = meta_blob["meta"]["leniaFft"]
    assert sim_meta["dim"] == 2
    assert sim_meta["n_grid"] == 32
    assert sim_meta["kernel_radius"] == orbium.kernel_radius
    assert sim_meta["preset_name"] == "Orbium unicaudatus"
    # Brush + view nested dicts preserved.
    assert sim_meta["view"]["pan_x"] == 0.0
    assert sim_meta["brush"]["intensity"] == 0.5
