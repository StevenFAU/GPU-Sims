"""Runtime FFT-backend selection for lenia-fft.

The Lenia convolution `K ⊛ state` can be computed via several paths:

  1. CuPy FFT          (NVIDIA CUDA)
  2. PyTorch FFT       (NVIDIA CUDA or AMD ROCm)
  3. Taichi real-space (universal; works on both CUDA and Vulkan via
                        ti.init(arch=ti.gpu))
  4. numpy FFT         (CPU fallback; used in CI smoke + headless dev)

At sim init, `select_backend()` probes each in priority order; the first
that imports and smoke-passes is selected. The selected backend logs its
name; failures are non-fatal and logged at log.info (not log.warn — every
run on AMD without CuPy installed would otherwise produce a warning).

All four backends share the same numpy-in / numpy-out public interface
(LeniaConvolver.step) so the main loop is backend-agnostic. The Taichi-
real-space backend internally copies through a Taichi field, which adds
2 numpy↔Taichi round-trips per step (negligible at 512²: 1 MB transfer,
~10s of microseconds); the FFT backends transfer to GPU once at init
(the kernel FFT) and per-step (the state FFT + inverse FFT). v1.1 may
expose a step_inplace_taichi() variant on the real-space backend to skip
the round-trip if the cost shows up in profiling.

References (Phase 10 anchors at draft time):
  - CuPy 13.x:    cupy.fft.fft2(a, axes=(-2, -1))
  - PyTorch 2.x:  torch.fft.fft2(input, dim=(-2, -1))
  - numpy 1.26+:  numpy.fft.fft2(a, axes=(-2, -1))
"""

from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Any

import numpy as np
from gpusims_common import log


# ----------------------------------------------------------------------
# Abstract base
# ----------------------------------------------------------------------

class LeniaConvolver(ABC):
    """One Lenia step: numpy in, numpy out.

    Subclasses cache backend-specific resources (FFT of the kernel, GPU
    handles, etc.) at __init__; subsequent step() calls reuse them.
    """

    def __init__(self, n_grid: int, kernel_lut_np: np.ndarray) -> None:
        self.n_grid: int = n_grid
        self.kernel_lut_np: np.ndarray = kernel_lut_np.astype(np.float32)

    @abstractmethod
    def step(
        self,
        state_np: np.ndarray,
        dt: float,
        mu: float,
        sigma: float,
    ) -> np.ndarray:
        """Compute one Lenia step. Returns the new state (same shape, float32)."""

    @abstractmethod
    def name(self) -> str:
        """Display name for log / GUI."""

    def is_gpu_fft(self) -> bool:
        """True if this backend uses GPU FFT (CuPy / Torch)."""
        return False


# ----------------------------------------------------------------------
# Kernel padding helper (shared between FFT backends)
# ----------------------------------------------------------------------

def _pad_kernel_to_grid(kernel_lut_np: np.ndarray, n_grid: int) -> np.ndarray:
    """Pad the (2R+1, 2R+1) kernel LUT to (n_grid, n_grid) with the kernel
    centered at (0, 0) for FFT-based convolution.

    The FFT convolution theorem assumes the kernel is centered at origin
    (not at (R, R) as in our LUT). We `np.roll` the centered kernel by
    (-R, -R) to land its center at (0, 0). This is the standard frequency-
    domain convolution-with-real-kernel setup.

    Also normalizes by the kernel sum so that the convolution result is
    the kernel-weighted-mean (matches the real-space kernel_sum division).
    """
    R = (kernel_lut_np.shape[0] - 1) // 2
    padded = np.zeros((n_grid, n_grid), dtype=np.float32)
    padded[: kernel_lut_np.shape[0], : kernel_lut_np.shape[1]] = kernel_lut_np
    padded = np.roll(padded, shift=(-R, -R), axis=(0, 1))
    s = float(padded.sum())
    if s > 0.0:
        padded /= s
    return padded


def _growth_map_apply(state: np.ndarray, u: np.ndarray, dt: float, mu: float, sigma: float) -> np.ndarray:
    """Apply the Lenia growth map: state' = clip(state + dt * G(u), 0, 1)
    where G(u) = 2*exp(-(u-mu)^2/(2*sigma^2)) - 1.

    Vectorized on numpy; works on both CPU and CuPy/Torch numpy-views
    (CuPy and Torch ndarray-likes broadcast identically to numpy).
    """
    g = 2.0 * np.exp(-((u - mu) ** 2) / (2.0 * sigma * sigma)) - 1.0
    return np.clip(state + dt * g, 0.0, 1.0).astype(np.float32)


# ----------------------------------------------------------------------
# Backend 1: CuPy (NVIDIA CUDA)
# ----------------------------------------------------------------------

class CuPyFFTConvolver(LeniaConvolver):
    """CuPy FFT (CUDA). Optional extra: pip install -e .[cuda]."""

    def __init__(self, n_grid: int, kernel_lut_np: np.ndarray) -> None:
        super().__init__(n_grid, kernel_lut_np)
        import cupy as cp                       # type: ignore[import-not-found]

        padded = _pad_kernel_to_grid(kernel_lut_np, n_grid)
        self._cp = cp
        self._kernel_fft = cp.fft.fft2(cp.asarray(padded, dtype=cp.float32))

    def step(self, state_np: np.ndarray, dt: float, mu: float, sigma: float) -> np.ndarray:
        cp = self._cp
        state_cp = cp.asarray(state_np, dtype=cp.float32)
        state_fft = cp.fft.fft2(state_cp)
        u_fft = state_fft * self._kernel_fft
        u_cp = cp.real(cp.fft.ifft2(u_fft)).astype(cp.float32)
        g_cp = 2.0 * cp.exp(-((u_cp - mu) ** 2) / (2.0 * sigma * sigma)) - 1.0
        new_cp = cp.clip(state_cp + dt * g_cp, 0.0, 1.0).astype(cp.float32)
        return cp.asnumpy(new_cp)

    def name(self) -> str:
        return "CuPy FFT (CUDA)"

    def is_gpu_fft(self) -> bool:
        return True


# ----------------------------------------------------------------------
# Backend 2: PyTorch (CUDA or ROCm)
# ----------------------------------------------------------------------

class TorchFFTConvolver(LeniaConvolver):
    """PyTorch FFT (CUDA or ROCm). Optional extra: pip install -e .[cuda-torch] or .[rocm]."""

    def __init__(self, n_grid: int, kernel_lut_np: np.ndarray) -> None:
        super().__init__(n_grid, kernel_lut_np)
        import torch                            # type: ignore[import-not-found]

        # Pick device: CUDA (covers CUDA-native + ROCm-via-CUDA-API).
        if torch.cuda.is_available():
            self._device = torch.device("cuda")
        else:
            raise RuntimeError("TorchFFTConvolver requires CUDA or ROCm; neither available")

        padded = _pad_kernel_to_grid(kernel_lut_np, n_grid)
        self._torch = torch
        self._kernel_fft = torch.fft.fft2(
            torch.from_numpy(padded).to(self._device)
        )

    def step(self, state_np: np.ndarray, dt: float, mu: float, sigma: float) -> np.ndarray:
        t = self._torch
        state_t = t.from_numpy(state_np).to(self._device)
        state_fft = t.fft.fft2(state_t)
        u_fft = state_fft * self._kernel_fft
        u_t = t.real(t.fft.ifft2(u_fft)).float()
        g_t = 2.0 * t.exp(-((u_t - mu) ** 2) / (2.0 * sigma * sigma)) - 1.0
        new_t = t.clamp(state_t + dt * g_t, 0.0, 1.0).float()
        return new_t.cpu().numpy()

    def name(self) -> str:
        t = self._torch
        if t.version.hip is not None:
            return "PyTorch FFT (ROCm)"
        return "PyTorch FFT (CUDA)"

    def is_gpu_fft(self) -> bool:
        return True


# ----------------------------------------------------------------------
# Backend 3: Taichi real-space convolution (universal baseline)
# ----------------------------------------------------------------------

class TaichiRealSpaceConvolver(LeniaConvolver):
    """Taichi real-space convolution. Universal baseline; no extra install.

    Internally calls kernels.lenia_step_2d + swap_state_2d. The public API
    is still numpy-in/numpy-out for consistency with the FFT backends.
    The numpy round-trip per step adds ~10s of microseconds at 512²; v1.1
    may add step_inplace_taichi() to skip it if it matters.
    """

    def __init__(self, n_grid: int, kernel_lut_np: np.ndarray, taichi_state: Any = None) -> None:
        super().__init__(n_grid, kernel_lut_np)
        self._taichi_state = taichi_state
        # The Taichi state has its own state_2d + state_2d_next + kernel_lut
        # fields; we use them as scratch space. apply_preset() already
        # populated them; for an arbitrary-state step() call we from_numpy
        # the input first.

    def step(self, state_np: np.ndarray, dt: float, mu: float, sigma: float) -> np.ndarray:
        import kernels
        st = self._taichi_state
        # Copy input into Taichi state field.
        st.state_2d.from_numpy(state_np.astype(np.float32))
        # Run the kernel step + swap.
        kernels.lenia_step_2d(
            st.state_2d, st.state_2d_next, st.kernel_lut,
            kernel_radius=(self.kernel_lut_np.shape[0] - 1) // 2,
            n_grid=self.n_grid,
            dt=dt, mu=mu, sigma=sigma,
        )
        kernels.swap_state_2d(st.state_2d, st.state_2d_next)
        # Pull out as numpy.
        return st.state_2d.to_numpy().astype(np.float32)

    def name(self) -> str:
        return "Taichi real-space (universal)"


# ----------------------------------------------------------------------
# Backend 4: numpy FFT (CPU fallback / CI smoke)
# ----------------------------------------------------------------------

class NumpyFFTConvolver(LeniaConvolver):
    """numpy FFT. CPU only; used as CI smoke + headless-dev fallback."""

    def __init__(self, n_grid: int, kernel_lut_np: np.ndarray) -> None:
        super().__init__(n_grid, kernel_lut_np)
        padded = _pad_kernel_to_grid(kernel_lut_np, n_grid)
        # np.fft.fft2(float32) -> complex64 (verified at draft-time sandbox).
        self._kernel_fft = np.fft.fft2(padded).astype(np.complex64)

    def step(self, state_np: np.ndarray, dt: float, mu: float, sigma: float) -> np.ndarray:
        state_fft = np.fft.fft2(state_np.astype(np.float32))
        u_fft = state_fft * self._kernel_fft
        # np.fft.ifft2 returns complex; take .real and re-cast to f32.
        # Verified at draft-time: round-trip preserves values to within 1e-5.
        u = np.fft.ifft2(u_fft).real.astype(np.float32)
        return _growth_map_apply(state_np.astype(np.float32), u, dt, mu, sigma)

    def name(self) -> str:
        return "numpy FFT (CPU)"


# ----------------------------------------------------------------------
# Backend selection
# ----------------------------------------------------------------------

def _try_smoke(convolver: LeniaConvolver) -> bool:
    """Run a tiny smoke step to confirm the backend actually works.

    Some backends import cleanly but fail at runtime (e.g., CuPy with no
    CUDA device, PyTorch with neither CUDA nor ROCm). The smoke catches that.

    POLISH-3 (visual-verification gate): TaichiRealSpaceConvolver.step writes
    its input INTO the shared `state.state_2d` field for performance (no
    private scratch). Calling step(impulse, ...) here therefore clobbers
    whatever the caller just loaded/applied. Snapshot the field before the
    probe and restore after, so apply_preset()'s canonical cells (and F9's
    just-loaded buffer) survive backend re-selection.
    """
    snapshot: np.ndarray | None = None
    taichi_state = getattr(convolver, "_taichi_state", None)
    field = getattr(taichi_state, "state_2d", None) if taichi_state is not None else None
    if field is not None:
        snapshot = field.to_numpy()
    try:
        smoke_state = np.zeros((convolver.n_grid, convolver.n_grid), dtype=np.float32)
        smoke_state[convolver.n_grid // 2, convolver.n_grid // 2] = 0.5
        convolver.step(smoke_state, dt=0.1, mu=0.15, sigma=0.015)
        return True
    except Exception as e:                                                   # noqa: BLE001
        log.info("backend %s smoke failed: %s", convolver.name(), e)
        return False
    finally:
        if snapshot is not None and field is not None:
            field.from_numpy(snapshot)


def select_backend(
    n_grid: int,
    kernel_lut_np: np.ndarray,
    taichi_state: Any = None,
) -> LeniaConvolver:
    """Pick the first available FFT backend in priority order.

    Order: CuPy → PyTorch → Taichi real-space → numpy.

    Each candidate is import-guarded and smoke-tested before selection.
    Failures are logged at log.info; the next priority is tried.
    """
    # Priority 1: CuPy
    try:
        import cupy                              # type: ignore[import-not-found]  # noqa: F401
        try:
            conv = CuPyFFTConvolver(n_grid, kernel_lut_np)
            if _try_smoke(conv):
                return conv
        except Exception as e:                                               # noqa: BLE001
            log.info("CuPy backend construction failed: %s", e)
    except ImportError:
        log.info("cupy not available — skipping CUDA FFT backend")

    # Priority 2: PyTorch
    try:
        import torch                             # type: ignore[import-not-found]
        if torch.cuda.is_available() or (torch.version.hip is not None):
            try:
                conv = TorchFFTConvolver(n_grid, kernel_lut_np)
                if _try_smoke(conv):
                    return conv
            except Exception as e:                                           # noqa: BLE001
                log.info("PyTorch backend construction failed: %s", e)
        else:
            log.info("torch present but no CUDA/ROCm device — skipping Torch FFT backend")
    except ImportError:
        log.info("torch not available — skipping Torch FFT backend")

    # Priority 3: Taichi real-space (universal)
    if taichi_state is not None:
        try:
            conv = TaichiRealSpaceConvolver(n_grid, kernel_lut_np, taichi_state=taichi_state)
            if _try_smoke(conv):
                return conv
        except Exception as e:                                               # noqa: BLE001
            log.info("Taichi real-space backend failed: %s", e)
    else:
        log.info("no taichi_state passed — skipping Taichi real-space backend")

    # Priority 4: numpy FFT (CPU fallback / CI smoke)
    try:
        conv = NumpyFFTConvolver(n_grid, kernel_lut_np)
        if _try_smoke(conv):
            log.warn("falling back to numpy FFT (CPU) — sim will be slow")
            return conv
    except Exception as e:                                                   # noqa: BLE001
        log.error("numpy FFT backend failed: %s", e)

    raise RuntimeError("no FFT backend available — cannot run lenia-fft")
