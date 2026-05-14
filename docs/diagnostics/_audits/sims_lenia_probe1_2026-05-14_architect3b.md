---
title: Lenia-FFT Structural Probe (Layer 3 Batch B probe-1)
date: 2026-05-14
author: architect-3b (via Claude Code)
layer: 3
batch: B
sim: continuous-ca/lenia-fft
status: probe
scope: read-only
cross_workstream: none
---

## A. Package layout

Directory tree (depth-aware, `__pycache__` and runtime-output dirs
`captures/`, `frames_export/`, `vdb_export/`, `*.egg-info/` elided):

```
continuous-ca/lenia-fft/
├── README.md
├── docs/
│   ├── load-bearing-decisions.md
│   └── notes.md
└── python/
    ├── imgui.ini
    ├── pyproject.toml
    ├── lenia_fft/
    │   ├── __init__.py
    │   ├── fft_backend.py
    │   ├── kernels.py
    │   ├── main.py
    │   └── presets.py
    └── tests/
        └── test_kernels.py
```

`wc -l` of every `.py` file in the package:

```
   353 python/lenia_fft/fft_backend.py
     0 python/lenia_fft/__init__.py
   351 python/lenia_fft/kernels.py
   726 python/lenia_fft/main.py
   358 python/lenia_fft/presets.py
   378 python/tests/test_kernels.py
  2166 total
```

`pyproject.toml` packages declaration:

```toml:continuous-ca/lenia-fft/python/pyproject.toml:55
[tool.setuptools]
packages = ["lenia_fft"]
```

`pyproject.toml` optional-dependencies block:

```toml:continuous-ca/lenia-fft/python/pyproject.toml:31
[project.optional-dependencies]
# GPU-FFT backend extras. Pick ONE that matches your hardware.
# CuPy (NVIDIA CUDA): pip install -e .[cuda]
# PyTorch ROCm (AMD): pip install -e .[rocm]  — also requires the ROCm wheel
#                     installed separately via the PyTorch-recommended index URL.
# PyTorch CUDA (NVIDIA alternative to CuPy): pip install -e .[cuda-torch]
# All three are optional; absent them, the sim falls back to Taichi real-space
# convolution (universal-baseline, works on both CUDA and Vulkan via the
# standard ti.init(arch=ti.gpu) path). See docs/load-bearing-decisions.md
# "Runtime FFT-backend selection (priority order)".
cuda = ["cupy-cuda12x>=13,<14"]
cuda-torch = ["torch>=2.1,<3"]
rocm = ["torch>=2.1,<3"]   # ROCm wheel installed separately; see README

dev = [
    "ruff>=0.6,<1.0",
    "mypy>=1.11,<2",
    "pytest>=8,<9",
]
```

## B. main.py call graph

Entry point:

```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:725
if __name__ == "__main__":
    main()
```

`def main():` at:

```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:261
def main() -> None:
    ti.init(arch=ti.gpu)
    log.info("lenia-fft starting; Taichi arch=%s", ti.cfg.arch)
```

Init sequence in order:

1. `ti.init(arch=ti.gpu)` and startup log:

   ```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:262
       ti.init(arch=ti.gpu)
       log.info("lenia-fft starting; Taichi arch=%s", ti.cfg.arch)
   ```

2. Tier construction (mutable copy of `TIERS_BASE`, default-tier unpack):

   ```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:266
       tiers: list[tuple[str, int, int, bool]] = list(TIERS_BASE)   # mutable copy for backend-dependent re-labeling
       tier_idx = DEFAULT_TIER_IDX
       _tier_label, dim, n_grid, is_capture_mode = tiers[tier_idx]
   ```

3. Preset list build + dim filter + initial preset:

   ```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:270
       preset_list = presets.build_presets()
       # Filter presets by current dimension (2D presets for 2D tiers, 3D for 3D).
       preset_list_for_dim = [(name, p) for (name, p) in preset_list if p.dim == dim]
       if not preset_list_for_dim:
           log.error("No presets for dim=%d; check presets.py", dim)
           sys.exit(1)
       curr_preset_idx = 0
       curr_preset = preset_list_for_dim[curr_preset_idx][1]
   ```

4. SimState allocation + preset application:

   ```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:279
       state = SimState(dim=dim, n_grid=n_grid, kernel_radius=curr_preset.kernel_radius)
       presets.apply_preset(state, curr_preset)
   ```

5. Backend probe (2D only):

   ```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:283
       convolver: LeniaConvolver | None = None
       if dim == 2:
           convolver = select_backend(
               n_grid=n_grid,
               kernel_lut_np=state.kernel_lut.to_numpy(),
               taichi_state=state,                           # for the Taichi-real-space path
           )
           log.info("Selected FFT backend: %s", convolver.name())
   ```

6. Tier-label runtime mutation (post-backend-select):

   ```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:296
       if convolver is not None and convolver.is_gpu_fft():
           label2, dim2, ng2, _ = tiers[2]
           tiers[2] = (label2.replace("(stretch)", "(stretch, FFT)"), dim2, ng2, False)
       elif convolver is not None:
           # Taichi-real-space or numpy: keep capture-mode framing on the 2048 tier.
           label2, dim2, ng2, _ = tiers[2]
           tiers[2] = (label2.replace("(stretch)", "(stretch, real-space, capture-mode)"), dim2, ng2, True)
   ```

7. Window / canvas / camera:

   ```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:305
       window = ti.ui.Window("lenia-fft — GPU-Sims", RES, vsync=True)
       canvas = window.get_canvas()
   ```

   ```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:312
       camera = Camera(mode=CameraMode.FreeFly)
       camera.set_position(0.5, 0.5, 1.5)
       camera.set_lookat(0.5, 0.5, 0.5)
       camera.set_fov_deg(55)
       camera.set_aspect(RES[0] / RES[1])
   ```

8. ParamPanel + state I/O construction:

   ```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:318
       panel = ParamPanel("Lenia", persist_key="lenia-fft")

       state_writer = StateWriter(CAPTURES_DIR)
       state_reader = StateReader(CAPTURES_DIR)
   ```

9. Output dir creation:

   ```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:323
       FRAMES_EXPORT_DIR.mkdir(parents=True, exist_ok=True)
       VDB_EXPORT_BASE.parent.mkdir(parents=True, exist_ok=True)
       CAPTURES_DIR.mkdir(parents=True, exist_ok=True)
   ```

10. Main-loop entry:

    ```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:478
        while window.running:
    ```

ParamPanel folders (rendered each frame inside the loop), in declared order
within the GUI block:

- `Presets` — `panel.folder("Presets", 0.02, 0.02, 0.22, 0.22)` (line 576)
- `Tier` — `panel.folder("Tier", 0.02, 0.26, 0.22, 0.15)` (line 587)
- `Lenia` — `panel.folder("Lenia", 0.02, 0.43, 0.22, 0.18)` (line 597)
- `Brush` (2D only) — `panel.folder("Brush", 0.02, 0.63, 0.22, 0.14)` (line 614)
- `Slice` (3D only, replaces Brush) — `panel.folder("Slice", 0.02, 0.63, 0.22, 0.16)` (line 620)
- `Export` — `panel.folder("Export", 0.02, y_export, 0.22, 0.20)` (line 630)
- `⚠ Capture mode` (modal, conditional) — `panel.folder("⚠ Capture mode", 0.3, 0.4, 0.4, 0.2)` (line 673)

## C. Backend dispatch

Backend factory lives in `lenia_fft/fft_backend.py`. Quoted verbatim:

```python:continuous-ca/lenia-fft/python/lenia_fft/fft_backend.py:291
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
    # `conv` is the priority-walk's narrowest common type so mypy doesn't
    # infer it as the first branch's concrete class.
    conv: LeniaConvolver
    # Priority 1: CuPy
    try:
        import cupy  # noqa: F401
        try:
            conv = CuPyFFTConvolver(n_grid, kernel_lut_np)
            if _try_smoke(conv):
                return conv
        except Exception as e:
            log.info("CuPy backend construction failed: %s", e)
    except ImportError:
        log.info("cupy not available — skipping CUDA FFT backend")

    # Priority 2: PyTorch
    try:
        import torch
        if torch.cuda.is_available() or (torch.version.hip is not None):
            try:
                conv = TorchFFTConvolver(n_grid, kernel_lut_np)
                if _try_smoke(conv):
                    return conv
            except Exception as e:
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
        except Exception as e:
            log.info("Taichi real-space backend failed: %s", e)
    else:
        log.info("no taichi_state passed — skipping Taichi real-space backend")

    # Priority 4: numpy FFT (CPU fallback / CI smoke)
    try:
        conv = NumpyFFTConvolver(n_grid, kernel_lut_np)
        if _try_smoke(conv):
            log.warn("falling back to numpy FFT (CPU) — sim will be slow")
            return conv
    except Exception as e:
        log.error("numpy FFT backend failed: %s", e)

    raise RuntimeError("no FFT backend available — cannot run lenia-fft")
```

Probe pattern, fall-through, log call: each candidate is gated by an
`import` (CuPy/Torch) or by `taichi_state is not None` (Taichi real-space);
constructed; then run through `_try_smoke()`. On failure the next
candidate is tried. Logs are at `log.info` for skip/non-fatal failures and
at `log.warn` only on the numpy fallback. Final failure raises
`RuntimeError`.

`_try_smoke` (snapshot/restore around a one-cell impulse step):

```python:continuous-ca/lenia-fft/python/lenia_fft/fft_backend.py:260
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
    except Exception as e:
        log.info("backend %s smoke failed: %s", convolver.name(), e)
        return False
    finally:
        if snapshot is not None and field is not None:
            field.from_numpy(snapshot)
```

Backend `convolve` / `step` signatures:

Abstract base:

```python:continuous-ca/lenia-fft/python/lenia_fft/fft_backend.py:54
    @abstractmethod
    def step(
        self,
        state_np: np.ndarray,
        dt: float,
        mu: float,
        sigma: float,
    ) -> np.ndarray:
        """Compute one Lenia step. Returns the new state (same shape, float32)."""
```

CuPy:

```python:continuous-ca/lenia-fft/python/lenia_fft/fft_backend.py:125
    def step(self, state_np: np.ndarray, dt: float, mu: float, sigma: float) -> np.ndarray:
```

PyTorch:

```python:continuous-ca/lenia-fft/python/lenia_fft/fft_backend.py:167
    def step(self, state_np: np.ndarray, dt: float, mu: float, sigma: float) -> np.ndarray:
```

Taichi real-space:

```python:continuous-ca/lenia-fft/python/lenia_fft/fft_backend.py:210
    def step(self, state_np: np.ndarray, dt: float, mu: float, sigma: float) -> np.ndarray:
```

numpy:

```python:continuous-ca/lenia-fft/python/lenia_fft/fft_backend.py:244
    def step(self, state_np: np.ndarray, dt: float, mu: float, sigma: float) -> np.ndarray:
```

All four back ends share `step(state_np, dt, mu, sigma) -> np.ndarray`.
There is no separate `convolve()` method on the abstraction — the
convolution is done internally inside `step()` via the cached
`self._kernel_fft` (FFT backends) or via `kernels.lenia_step_2d` (Taichi
real-space).

## D. Kernel construction

Quad4 LUT init kernel verbatim:

```python:continuous-ca/lenia-fft/python/lenia_fft/kernels.py:59
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
```

3D analog:

```python:continuous-ca/lenia-fft/python/lenia_fft/kernels.py:77
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
```

Spec form `K(r) = (4r(1-r))^4`. Code form (line 73 / 88):
`v = (4.0 * r * (1.0 - r)) ** 4`. **Match — bit-for-bit on the
mathematical form**, with the unit-disc support `if r < 1.0` (and `r =
sqrt(...)/r_kernel` normalizing physical pixel offset to the [0,1]
parameter).

b-string handling: there is no b-string handling in the shipped code.
`LeniaPreset` carries no `b` field at all:

```python:continuous-ca/lenia-fft/python/lenia_fft/presets.py:57
@dataclass(frozen=True)
class LeniaPreset:
    """One named Lenia creature."""

    name: str
    dim: int                         # 2 or 3
    kernel_radius: int               # R
    time_resolution: float           # T (dt = 1/T)
    mu: float                        # growth mean
    sigma: float                     # growth std
    seed_radius_cells: float         # initial random-blob radius
```

There is no parser for `params.b`, no branch on `len(b) > 1`, no fallback
to first peak. The shipped presets are simply hard-coded to the `b="1"`
single-peak set, and the kernel function is a single-shell quad4. So the
codepath does not "fail" or "silently take first peak" — it has no concept
of `b` at all. A future polyring extension is acknowledged in
`presets.py` and `notes.md` (see § J / § K), and would require both a
`LeniaPreset` schema change and a new `init_kernel_polyring_2d` kernel.

## E. Preset table

Verified roster docstring (cite-source for the four codes):

```python:continuous-ca/lenia-fft/python/lenia_fft/presets.py:14
Verified roster (codes match upstream animals.json):

  Slot 1 — basic glider      Orbium unicaudatus     (O2u)   R=13 T=10 μ=0.15  sigma=0.015
  Slot 2 — wanderer          Vagorbium undulatus    (OV2u)  R=20 T=10 μ=0.2   sigma=0.031
  Slot 3 — rotator           Gyrorbium gyrans       (OG2g)  R=13 T=10 μ=0.156 sigma=0.0224
  Slot 4 — shield            Discutium valvatus     (S2v)   R=15 T=10 μ=0.331 sigma=0.057
  Slot 5 — 3D bootstrap      3D Random Blob         (n/a)   R=8  T=10 μ=0.15  sigma=0.015
```

The preset table is `_CANONICAL_2D_PRESETS` followed by `_GENERIC_3D_PRESET`,
quoted verbatim:

```python:continuous-ca/lenia-fft/python/lenia_fft/presets.py:223
_CANONICAL_2D_PRESETS: Final[list[LeniaPreset]] = [
    LeniaPreset(
        # Code O2u — Orbidae > Haplorbinae > Orbium unicaudatus
        # Canonical single-tail glider, the "first creature" of Lenia research.
        name="Orbium unicaudatus",
        dim=2,
        kernel_radius=13,
        time_resolution=10.0,
        mu=0.15,
        sigma=0.015,
        seed_radius_cells=18.0,
    ),
    LeniaPreset(
        # Code OV2u — Orbidae > Haplorbinae > Vagorbium undulatus
        # Larger R=20 undulating wanderer; replaces v1's fabricated "Geminium"
        # (Geminium-family creatures upstream are all polyring b="1,1,1" or
        # "1/2,1,2/3" — incompatible with Phase 10's single-peak kernel).
        name="Vagorbium undulatus",
        dim=2,
        kernel_radius=20,
        time_resolution=10.0,
        mu=0.2,
        sigma=0.031,
        seed_radius_cells=25.0,
    ),
    LeniaPreset(
        # Code OG2g — Orbidae > Haplorbinae > Gyrorbium gyrans
        # Rotating orbital; named-creature parameters preserved from v1
        # (v1 had the correct numeric params but the wrong name — upstream
        # "Gyrorbium" is the family, "Gyrorbium gyrans" is the specific
        # canonical creature with these exact params).
        name="Gyrorbium gyrans",
        dim=2,
        kernel_radius=13,
        time_resolution=10.0,
        mu=0.156,
        sigma=0.0224,
        seed_radius_cells=18.0,
    ),
    LeniaPreset(
        # Code S2v — Scutidae > Haploscutinae > Discutium valvatus
        # Valved two-shield at R=15; replaces v1's fabricated "Scutium-gyrans"
        # (which had no upstream correspondence under any Scutium family).
        # Discutium = two-shield (visually striking, paired motion); valvatus
        # = valved-edge variant (adds dynamic interest vs Discutium solidus).
        name="Discutium valvatus",
        dim=2,
        kernel_radius=15,
        time_resolution=10.0,
        mu=0.331,
        sigma=0.057,
        seed_radius_cells=20.0,
    ),
]
```

```python:continuous-ca/lenia-fft/python/lenia_fft/presets.py:283
_GENERIC_3D_PRESET: Final[LeniaPreset] = LeniaPreset(
    name="3D Random Blob",
    dim=3,
    kernel_radius=8,
    time_resolution=10.0,
    mu=0.15,
    sigma=0.015,
    seed_radius_cells=10.0,
)
```

Per-preset summary (R, T, μ, σ, kn, gn, b — kn/gn/b are uniform across the
shipped roster: kn=1 quad4, gn=1 Gaussian, b="1" single-peak — none of
those three live in the dataclass; they are properties of the chosen
kernel + growth functions, not per-preset parameters):

| Code | Name | dim | R | T | μ | σ | kn | gn | b |
|---|---|---|---|---|---|---|---|---|---|
| O2u | Orbium unicaudatus | 2 | 13 | 10.0 | 0.15 | 0.015 | 1 | 1 | "1" |
| OV2u | Vagorbium undulatus | 2 | 20 | 10.0 | 0.2 | 0.031 | 1 | 1 | "1" |
| OG2g | Gyrorbium gyrans | 2 | 13 | 10.0 | 0.156 | 0.0224 | 1 | 1 | "1" |
| S2v | Discutium valvatus | 2 | 15 | 10.0 | 0.331 | 0.057 | 1 | 1 | "1" |
| n/a | 3D Random Blob | 3 | 8 | 10.0 | 0.15 | 0.015 | 1 | 1 | "1" |

## F. Main loop + integration step

Per-frame loop body verbatim (entry + input + brush + sim dispatch + view
+ exports + GUI + show + deferred-tier):

```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:478
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
```

The 2D step delegates to the selected backend's `step()` via the
`step_2d` shim:

```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:220
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
```

Convolve → growth → field update → potential clamp:

- **Convolve + growth + clamp inside one Taichi pass** (real-space backend):

  ```python:continuous-ca/lenia-fft/python/lenia_fft/kernels.py:172
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
  ```

- **Same trio in the FFT/numpy path** is split between `_pad_kernel_to_grid`
  + the FFT product + `_growth_map_apply`:

  ```python:continuous-ca/lenia-fft/python/lenia_fft/fft_backend.py:99
  def _growth_map_apply(state: np.ndarray, u: np.ndarray, dt: float, mu: float, sigma: float) -> np.ndarray:
      """Apply the Lenia growth map: state' = clip(state + dt * G(u), 0, 1)
      where G(u) = 2*exp(-(u-mu)^2/(2*sigma^2)) - 1.

      Vectorized on numpy; works on both CPU and CuPy/Torch numpy-views
      (CuPy and Torch ndarray-likes broadcast identically to numpy).
      """
      g = 2.0 * np.exp(-((u - mu) ** 2) / (2.0 * sigma * sigma)) - 1.0
      return np.clip(state + dt * g, 0.0, 1.0).astype(np.float32)
  ```

The CuPy / Torch backends inline equivalent ops on their own array types
(see lines 125–135 and 167–177 of `fft_backend.py`). Note: the CuPy
and PyTorch backends do NOT call `_growth_map_apply`; they re-implement
the same `g = 2*exp(...) - 1; clip(state + dt*g, 0, 1)` math
directly on their respective array types.

Deferred-recompile pattern from tier change: the tier dropdown sets a
sentinel `pending_tier_change` (or `pending_capture_mode_modal`); the
re-allocate-after-`window.show()` block runs strictly after the GUI for
this frame:

```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:684
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
```

`SimState.__init__` docstring confirms the deferred recompile is intentional
(`Taichi specializes @ti.kernel on field shapes, so a tier change triggers a
one-time recompile on the first step call (~1-3 s)` — see lines 95–102 of
`main.py`).

## G. GGUI panel + brush

`cursor_to_field_cell` verbatim:

```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:144
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
```

`cursor_in_any_panel` verbatim:

```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:197
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
```

Y-convention name + assessment:

- `cursor_to_field_cell`: **Direct top-down**, treats incoming `cy` as
  y=0-at-top. The docstring says "cursor y=0 is at the TOP of the
  window on Taichi 1.7.4 / Vulkan / Ubuntu 24.04 (empirically verified
  during Phase 10 polish-4 visual-verification gate)." So `cy` is fed
  directly into `fy_norm` which then maps row 0 to top of image. No
  flip.

- `cursor_in_any_panel`: **Inverted (bottom-up)**, treats incoming `cy`
  as y=0-at-bottom and converts via `cy_top = 1.0 - cy_bottom` before
  testing. Docstring says "Taichi GGUI returns cursor coords with y=0 at
  BOTTOM (NDC convention) but panel.folder() positions panels with y=0
  at TOP."

Structural assessment of the "both work empirically" claim:

The two functions are using **directly contradictory** assumptions about
the same cursor input from the same call site. Both consume `cur =
window.get_cursor_pos()` from line 499 in the same frame; one assumes
`cy=0` is the top of the window, the other assumes `cy=0` is the bottom.
Only one of these can be literally true for the cursor at any given
moment.

The most plausible reconciliation, given the in-source notes and the
fact that both behaviors check out empirically on the user's hardware:

1. The `cursor_in_any_panel` test only fires when cursor is over a
   panel, and **all panels live in the upper-left quadrant** at x ∈
   [0.02, 0.24] and (in 2D) y ∈ [0.02, 0.99]. The
   `(1.0 - cy_bottom)` flip thus inverts a top-row cursor (true top, on
   the assumption that cy=0 is top) into the bottom of the window
   (cy_top ≈ 1.0), so an `if py <= cy_top <= py+ph` test for a panel at
   the top of the window (`py ≈ 0.02, ph ≈ 0.22`) would NOT match …
   unless the cursor y-coord coming in is really 0-at-bottom, in which
   case the flip is correct and a top-of-window cursor becomes
   `cy_top ≈ 0.0`, matching the top panel rect. So the
   `cursor_in_any_panel` flip is empirically consistent with the
   "y=0-at-bottom" hypothesis.

2. `cursor_to_field_cell` not flipping is empirically consistent with
   "y=0-at-top". Per its docstring, the gate that signed off on this
   convention was a Phase 10 polish-4 visual-verification round: an
   earlier `(1.0 - fy_norm)` flip "empirically inverted paint position
   on the user's AMD desktop" (from `notes.md` "Polish-4 GGUI
   Y-convention asymmetry"); removing the flip fixed paint placement.

The two empirical observations cannot both be literally true without a
mediating quirk. A few possibilities the in-source notes leave open:

- A handedness mismatch between the cursor coord and the panel-rect
  coord — i.e., the **panel.folder()** positioner inside
  `gpusims_common.ParamPanel` is itself flipped relative to its
  documented y=0-at-top convention. The
  `cursor_in_any_panel` flip then "double-corrects" for a flip already
  inside ParamPanel; `cursor_to_field_cell`, which does not interact
  with ParamPanel, gets the raw GGUI cursor convention.
- The MPM Phase 9 panel layout may have been laterally symmetric
  enough that the `(1.0 - cy_bottom)` flip happened to land the cursor
  inside a (different) panel rect for the relevant test cases; lenia
  inherited the call but its panels happen to also fall in the
  upper-left quadrant where the flip again lands inside a panel rect
  by coincidence. (`notes.md` line 178-180 raises this exact
  possibility.)

Verdict: **load-bearing coincidence, not structural soundness.** The
two functions cannot both be using a correct GGUI convention; one of
them is right by accident. The in-source comments at
`main.py:160-168` and the `notes.md` Polish-4 section both
explicitly flag this as undisproven and bank it for the "consumer #3
promotion review when a third sim arrives with panels in DIFFERENT
screen regions than MPM/lenia" — exactly the moment when the
coincidence is likely to break.

Brush call site (uses both functions in the same frame):

```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:498
        gui_rects = GUI_PANEL_RECTS_3D if dim == 3 else GUI_PANEL_RECTS_2D
        cur = window.get_cursor_pos()
        if dim == 2:
            lmb_held = window.is_pressed(ti.ui.LMB)
            rmb_held = window.is_pressed(ti.ui.RMB)
            paint_active = (lmb_held or rmb_held) and not cursor_in_any_panel(cur, gui_rects)
            if paint_active:
                i, j = cursor_to_field_cell(cur, pan_x, pan_y, zoom, n_grid)
```

## H. F5/F9 capture path

StateWriter / StateReader call sites:

Construction:

```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:320
    state_writer = StateWriter(CAPTURES_DIR)
    state_reader = StateReader(CAPTURES_DIR)
```

Save handler entry (begin_frame / set_meta / save_buffer / end_frame):

```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:352
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
```

Per-buffer save:

```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:374
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
```

Sim-namespaced meta wrapper construction (the `set_meta("leniaFft", …)`
call at line 372):

```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:372
        state_writer.set_meta("leniaFft", meta_wrapper)
```

Per-buffer `{count, stride, format, shape}` fields: the call sites only
pass `shape=[…]` to `save_buffer`. The other three fields (`count`,
`stride`, `format`) are not visible in the sim-side call — they are
populated inside `gpusims_common.StateWriter.save_buffer` and were not
inspected here per the no-deep-audit-of-common-py constraint. The
schema docstring in `load-bearing-decisions.md` § "Capture schema" (see
§ J below) declares the buffers as `r32f`, `state` shape `[N,N]` or
`[N,N,N]`, `kernel_lut` shape `[2R+1,2R+1]` or `[2R+1,2R+1,2R+1]`. The
test in `test_capture_schema_round_trip` (see § L) goes through the
same path and round-trips successfully.

The load handler is the symmetric path; pulling buffers via
`state_reader.load_buffer_reshaped(latest, "state")` /
`state_reader.load_buffer_reshaped(latest, "kernel_lut")` (lines
433–443 of `main.py`).

## I. Banked markers in source

`grep -rnE "banked|v1\.1|stub|TODO|FIXME|XXX"
continuous-ca/lenia-fft/python/`:

- `python/pyproject.toml:16` — `# relative paths are rejected by modern pip — banked Phase 9 retro.`
- `python/pyproject.toml:62` — `# Optional GPU-FFT deps have incomplete stubs or are absent in CI; allow Any.`
- `python/pyproject.toml:67` — `# @ti.kernel functions diverge from Python type semantics (banked Phase 9):`
- `python/tests/test_kernels.py:5` — `surface (banked Phase 9 runtime-only-surface convention).`
- `python/lenia_fft/presets.py:10` — `…all four 2D presets…Polyring (multi-peak via b-string) creatures are banked`
- `python/lenia_fft/presets.py:11` — `v1.1 with the formula documented at LeniaNDK.py:329-335; see`
- `python/lenia_fft/presets.py:279` — `# v1.1 will land Chan's 3D creatures after visual verification on user`
- `python/lenia_fft/fft_backend.py:21` — `(the kernel FFT) and per-step (the state FFT + inverse FFT). v1.1 may`
- `python/lenia_fft/fft_backend.py:134` — `# returns un-annotated Any from CuPy's stub-less .pyi).`
- `python/lenia_fft/fft_backend.py:176` — `# .numpy() returns un-annotated Any from torch's stub-less .pyi).`
- `python/lenia_fft/fft_backend.py:198` — `(the round-trip…); v1.1 may add step_inplace_taichi() to skip…`
- `python/lenia_fft/fft_backend.py:224` — `# ndarray type (Taichi's to_numpy() returns Any per its stubs).`
- `python/lenia_fft/kernels.py:13` — `CUDA-vs-Vulkan portability (banked Phase 9):`
- `python/lenia_fft/kernels.py:49` — `…and v1.1+ extensions, NOT by Phase 10's preset roster).`
- `python/lenia_fft/main.py:38` — `# NOTE: deliberately NO from __future__ import annotations. Phase 9 banked`
- `python/lenia_fft/main.py:245` — `additional scope; v1.1 banked). The 3D step uses kernels.lenia_step_3d`
- `python/lenia_fft/main.py:310` — `# via ti.ui.Scene is banked v1.1 (see docs/load-bearing-decisions.md`
- `python/lenia_fft/main.py:412` — `# (~1-3 s) on the next step call — accepted, banked Phase 9.`
- `python/lenia_fft/main.py:637` — `f"VDB density ({'real' if VdbWriter.is_available() else 'stub'})",`

No `TODO`, `FIXME`, `XXX` markers were found anywhere in the package
tree.

## J. docs/load-bearing-decisions.md inventory

Section headings (in order of appearance):

- `# Lenia-FFT — Load-bearing decisions` (h1)
- `## Runtime FFT-backend selection (priority order)`
- `## 2D + 3D in one phase (2D load-bearing, 3D opt-in stretch)`
- `## 3D viewer in v1: iso-cross-section slice (NOT volumetric raymarch)`
- `## 2D camera divergence from `common-py.Camera``
- `## Brush paint: LMB-drag, not edge-trigger`
- `## Hero render: two paths`
- `## Capture schema (extends Phase 9 sim-namespaced meta wrapper)`
- `## Promotion-review for `common-py` (consumer #2)`
- `## Stack B WebGPU port: deferred to a later phase`
- `## Tiers: 512² default (Chan canonical baseline, not 1024²)`
- `## Substeps per frame: 1 in v1; sub-stepping banked v1.1`

There is no section literally titled "Polyring extension banking" in
this file; that banking lives in `notes.md` (see § K). The closest
in-`load-bearing-decisions.md` reference is one inline mention at line
178 inside the "Capture schema" block (no separate section). Per the
spec instruction, the polyring banking quoted in full **comes from
`notes.md` "Polyring kernel extension banking (v1.1+)"**:

```markdown:continuous-ca/lenia-fft/docs/notes.md:56
## Polyring kernel extension banking (v1.1+)

**Decision:** Phase 10 ships single-peak only (b="1", kn=1 = quad4 kernel). The polyring (multi-peak via b-string) kernel extension is banked for a future phase, NOT for v1.1 polish on Phase 10 — polyring is genuinely new scope (~75 LOC across `kernels.py` + `presets.py` + tests, plus a wider preset library) that should land alongside a sim phase whose visual diversity actually needs it.

**Why this banking is worth the disk space:** Architect-2 round-2 verification surfaced that the polyring path is what unlocks ~50+ additional named creatures upstream — Hydrogeminium, Gyrogeminium, the Scutium serratus family, etc. Phase 10's preset roster is intentionally limited to four creatures specifically because polyring is banked; if a future sim phase wants the broader Lenia menagerie, polyring is the load-bearing precondition.

**Formula anchor (architect-2 round-2 verified):** Upstream `Chakazul/Lenia/Python/LeniaNDK.py:329-335` defines the polyring kernel assembly. The exact construction (architect-2's intuited formula matches Chan's modulo two defensive guards):

```python
# Per upstream LeniaNDK.py:329-335 (verified at architect-2 round-2):
def kernel_shell(r, b, kn):
    """Build polyring kernel from b-string + base kernel-core function.

    b   = list[float] of length B, the per-ring weights from the b-string
          (parsed from JSON params.b: e.g., "1/2,1,2/3" → [0.5, 1.0, 0.667])
    kn  = JSON kernel-core index (1=quad4, 2=bump4, ...)

    The unit interval [0, 1] is partitioned into B equal sub-intervals;
    ring i covers r in [i/B, (i+1)/B] and uses kernel_core[kn-1] mapped
    to the sub-interval, weighted by b[i].
    """
    B = len(b)
    K = kernel_core[kn - 1]
    # ring index for each r; clamp to B-1 to handle r==1 boundary
    i = np.minimum(np.floor(B * r).astype(int), B - 1)
    # r within ring, in [0, 1)
    r_local = B * r - i
    # assemble: weight[i] * K(r_local), masked to r < 1
    return (r < 1.0) * b[i] * K(r_local)
```

The architect-2-intuited formula `K_polyring(r) = β_i * K_bell(r·n - i)` is correct in shape; the two defensive guards Chan adds are (a) the `np.minimum(..., B-1)` to handle r=1 exactly without indexing out of bounds, and (b) the outer `(r < 1.0)` mask to zero the kernel outside the unit disc.

**Architectural cost when adopted:**

- `kernels.py`: a new `init_kernel_polyring_2d` / `_3d` pair (parameterized by the b-list); + ~25 LOC.
- `presets.py`: extend `LeniaPreset` dataclass to carry `b: tuple[float, ...]` and `kn: int`; convert single-peak presets (b="1" → (1.0,), kn=1) to the new shape; + ~15 LOC.
- New test: parametrized polyring smoke for a representative polyring creature (Hydrogeminium natans or similar); + ~35 LOC.
- Preset library expansion: enumerate 30-50 polyring creatures from upstream `animals.json` (the round-2 enumeration table is the starting point).
- No CI ripple; no `common-py` ripple.

**Trigger phase candidates:** A natural next-sim phase that wants this is one whose creative thesis includes the broader Lenia menagerie — likely the WebGPU port phase (Stack B can show off more creatures than Stack D's interactive-focus), or a dedicated "Lenia research-vehicle" continuation phase.

**Bank rationale:** Lenia the research-vehicle is half-built without polyring. Phase 10 is consumer-#2 of `common-py` and "first Stack D continuous-CA sim"; revisit at the first sim that requests Scutium serratus or Hydrogeminium dynamics, or at the v1.1 polish stream if visual verification surfaces a specific polyring need.
```

GGUI Y-convention section (the only Y-convention text in
`load-bearing-decisions.md` is buried inside "Brush paint: LMB-drag, not
edge-trigger" lines 134-148 and "2D camera divergence …" lines
116-131; the explicit asymmetry write-up is in `notes.md` Polish-4
section). The `load-bearing-decisions.md`-side reference text in full:

```markdown:continuous-ca/lenia-fft/docs/load-bearing-decisions.md:134
## Brush paint: LMB-drag, not edge-trigger

LMB-held-and-moving paints a Gaussian splat per frame; RMB-held erases with
the same brush profile but forced negative intensity. The Brush panel has
radius (1–80 px) + intensity (-1.0–+1.0) sliders.

This diverges from MPM Phase 9 LMB-place (edge-triggered, one click → one
cube). Lenia's UX is continuous strokes — drag detection (polled
`window.is_pressed(LMB)` every frame, no edge state).

GUI-occlusion gating inherits MPM:306–318's `_cursor_in_any_panel` pattern.
Sim-local copy; promotion candidate at consumer #3 (STRONG; see promotion
review below).

Save-creature ("RMB-snapshot named creature to library") banked v1.1.
```

Rule-of-three promotion-review section in full:

```markdown:continuous-ca/lenia-fft/docs/load-bearing-decisions.md:208
## Promotion-review for `common-py` (consumer #2)

Per the rule-of-three convention (`/project-state.md` § 7): patterns
identified as "this might want to be in `common-*`" during a sim phase
do not promote on the first consumer. They promote at the THIRD consumer
where the abstraction's shape is empirically validated by repeated use
rather than speculatively designed. The first two consumers keep per-sim
copies of the pattern; the third's spec includes the promotion review.

Phase 10 is consumer #2. This section documents promotion candidates with
rationale. **No code is extracted in Phase 10.** Phase 10's main.py includes
sim-local copies of all patterns identified here.

**Seven candidates inventoried** (synced source citations at
`b914892`):

1. **`_cursor_in_any_panel` GUI-occlusion test** (MPM
   `hybrid-particle-grid/mpm-multimaterial/python/mpm_multimaterial/main.py:306–318`)
   **— STRONG PROMOTE at #3.** Every interactive Stack D sim with sliders
   needs a "don't drop a thing on the canvas when the cursor is over a
   panel" test. The pattern is structurally identical between MPM and
   Lenia: panel rects list + bottom-y-to-top-y conversion. The
   abstraction's shape is clear: `cursor_in_any_panel(cursor, rects)
   -> bool`. Likely future API in common-py: `ParamPanel.is_cursor_over(window)`
   that wraps the panel-rect tracking internally so consumers don't
   manually maintain GUI_PANEL_RECTS lists.

2. **Capture-mode confirmation modal** (MPM `main.py:608–618`)
   **— STRONG PROMOTE at #3.** Tier-dependent capture-mode appears in
   Phase 8 smoke (384³), Phase 9 MPM (500k tier), Phase 10 Lenia (2048²
   FFT-dependent + 3D 128³). The modal's shape is consistent:
   `pending_capture_mode_modal: int | None` sentinel + `with
   panel.folder("⚠ ...", ...)` rendering + Continue/Cancel buttons.
   Likely future API: a `ParamPanel.capture_mode_modal(label,
   on_continue, on_cancel)` helper, or a more general
   `ParamPanel.modal(...)` if other modal flows surface.

3. **Tier dropdown + deferred-change-after-`window.show()`** (MPM
   `main.py:560–567 + 624–642`)
   **— STRONG PROMOTE at #3.** The deferred-after-show idiom is a
   non-obvious UX subtlety: re-allocate Taichi fields BEFORE the next
   step starts, AFTER the current GUI frame finishes (otherwise the
   GUI's checkbox state visually flickers). This is exactly the kind
   of thing the package should encapsulate. Likely future API: a
   `TierDropdown` widget that takes a list of `(label, on_select)`
   tuples and defers the `on_select` callback to after `window.show()`.

4. **F5/F9 save-load buttons UX pattern** (MPM `main.py:599–605`)
   **— MODERATE candidate.** The buttons live in each sim's GUI block
   (sim-specific labels, sim-specific side-effects) so the call sites
   stay sim-local. The pattern is consistent: button-tap → invoke
   save/load handler → log. If a third consumer (`neural-ca` Stack D
   variant likely) surfaces, promote a thin
   `ParamPanel.save_load_buttons(panel, on_save, on_load)` helper that
   renders the buttons and dispatches.

5. **`set_color_by_material` re-apply pattern** (MPM
   `hybrid-particle-grid/mpm-multimaterial/python/mpm_multimaterial/kernels.py`,
   `set_color_by_material`)
   **— KEEP SIM-LOCAL.** MPM-specific (Lenia has no discrete materials).
   The abstraction doesn't generalize to a continuous-CA sim.

6. **Reserve-tail emitter allocation** (MPM `main.py:466–472` +
   `EMITTER_RESERVE_SIZE` `main.py:96–100`)
   **— KEEP SIM-LOCAL.** MPM-specific physics-faithfulness move (preserve
   preset particles while LMB-place claims from a reserved tail region).
   Lenia's brush paints into the state field directly — no allocation
   needed. The abstraction doesn't generalize.

7. **`cursor_to_ground` 3D ray-plane unproject** (MPM `main.py:187+`)
   **— KEEP SIM-LOCAL.** Lenia 2D uses `cursor_to_field_cell` (2D pan-zoom
   inverse). Lenia 3D-slice uses an implicit `cursor_to_slice_cell` (2D-
   on-slice unproject). MPM uses `cursor_to_ground` (3D ground-plane
   unproject). Three different signatures, no common abstraction yet.
   Likely future work: at consumer #4 (whenever a sim needs an
   arbitrary-plane unproject), revisit and promote if a coherent
   abstraction emerges.

**Candidates the consumer-#3 phase spec should re-review FIRST:** 1, 2, 3, 4.
The first three are STRONG promote; #4 is MODERATE. Consumer #3 should
allocate spec budget for the promotion work (architect-1 spec section
"Promote from `common-py`" with API design + migration plan for both
existing consumers).
```

## K. docs/notes.md inventory

Section headings + v1.1-item count per section:

- `# Lenia-FFT — Engineering notes` (h1; intro)
- `## Install stories (per hardware path)` — h2 with three h3 subsections:
  - `### NVIDIA CUDA (user lab PC — RTX 2080 Ti)`
  - `### AMD ROCm (user dev desktop — RX 6800 XT)`
  - `### Universal-baseline (no GPU-FFT extras)`

  v1.1 items: 0.
- `## Polyring kernel extension banking (v1.1+)` — single combined "v1.1+"
  banked item. v1.1 items: 1.
- `## v1.1 polish backlog` — six v1.1 items (Save-creature UX; Named 3D
  creatures; Volumetric raymarch; Sub-stepping per frame; Parameter-search
  UI; Tier-3 diagnostics module; PyTorch-ROCm install-story automation —
  that's seven if you count the install-story as one). v1.1 items: 7.
- `## Known issues / gotchas` — five gotchas, none flagged v1.1
  (one mentions "v1.1 toggle" for show-black-off-edge view mode, count: 1).
- `## Polish-4 GGUI Y-convention asymmetry (Phase 10)` — banked
  retrospective; "Bank for retrospective". v1.1 items: 0 (banked for
  retro/promotion review, not for v1.1 polish).

Banked-instability items verbatim — `notes.md` flags **no instability
items**. The banked items are scope-cuts (polyring, save-creature UX,
named-3D creatures, volumetric raymarch, sub-stepping, parameter-search
UI, Tier-3 diagnostics) and a UX asymmetry, none described as
"unstable" in the bounded/NaN-free/exploding sense. The closest text:

```markdown:continuous-ca/lenia-fft/docs/notes.md:117
- **Sub-stepping per frame:** v1 runs 1 step/frame. Stability at large dt
  on stretch tiers may benefit from N substeps/frame (MPM Phase 9 ships
  25/frame). Bank as a tier-dependent default.
```

— this is the only "stability" reference in `notes.md`, and it is a
forward-looking "may benefit" note, not a banked failure.

## L. Tests

`tests/` location: `continuous-ca/lenia-fft/python/tests/test_kernels.py`.
This is the only `test_*.py` file.

Test functions present (10 total):

| Function | Lines |
|---|---|
| `test_kernel_lut_quad4_shape_2d` | 34–62 |
| `test_lenia_step_2d_bounded` | 69–84 |
| `test_lenia_step_3d_bounded` | 87–103 |
| `test_paint_splat_2d_adds_intensity` | 110–122 |
| `test_paint_splat_2d_erase_clamps_to_zero` | 125–136 |
| `test_composite_view_2d_identity` | 143–154 |
| `test_extract_slice_3d_axis_xy` | 157–166 |
| `test_numpy_fft_backend_smoke` | 173–197 |
| `test_pad_kernel_to_grid_centered` | 200–211 |
| `test_apply_preset_stability_2d` (parametrized × 4 presets) | 218–258 |
| `test_select_backend_factory_falls_back` | 265–300 |
| `test_capture_schema_round_trip` | 307–378 |

Confirming the three load-bearing CI tests named in the spec:

1. **`test_select_backend_factory`** — present as
   `test_select_backend_factory_falls_back` (lines 265–300). Quoted (35
   lines, slightly over the 30-line ceiling — body included for
   transparency):

   ```python:continuous-ca/lenia-fft/python/tests/test_kernels.py:265
   def test_select_backend_factory_falls_back() -> None:
       """`select_backend()` walks the priority list and returns the first
       backend that smoke-passes. In CI (no CuPy, no PyTorch), this should
       return either TaichiRealSpaceConvolver (if taichi_state provided) or
       NumpyFFTConvolver — both must produce a working step() call.

       Load-bearing contract test added in v2: the backend abstraction is
       Phase 10's load-bearing new pattern; ensuring the factory works
       without GPU extras present is the CI-runnable contract surface.
       """
       from lenia_fft.fft_backend import (
           NumpyFFTConvolver,
           TaichiRealSpaceConvolver,
           select_backend,
       )
       from lenia_fft.main import SimState
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
   ```

2. **`test_preset_stability_all`** — present as
   `test_apply_preset_stability_2d` (lines 218–258, parametrized over
   four presets). Quoted (over the 30-line cap, but included for
   transparency since it is the load-bearing preset-contract test):

   ```python:continuous-ca/lenia-fft/python/tests/test_kernels.py:218
   @pytest.mark.parametrize("preset_name", [
       "Orbium unicaudatus",
       "Vagorbium undulatus",
       "Gyrorbium gyrans",
       "Discutium valvatus",
   ])
   def test_apply_preset_stability_2d(preset_name: str) -> None:
       """Each 2D preset applies cleanly and runs 10 Lenia steps without
       dissolving or exploding.

       This is the load-bearing preset-contract test added in v2 (was
       single-Orbium-only in v1). Architect-2 round-3 verification confirmed
       all four pass BOUNDED + NaN_FREE + CHANGED + NON_DEAD assertions on
       CPU backend with quad4 kernel; this test re-validates that under CI
       on every push so preset regressions don't ship silently.
       """
       from lenia_fft.main import SimState
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
   ```

3. **`test_capture_schema_round_trip`** — present at lines 307–378 (>30
   lines so summarized + key bytes quoted; full body lives in
   `tests/test_kernels.py:307`):

   ```python:continuous-ca/lenia-fft/python/tests/test_kernels.py:307
   def test_capture_schema_round_trip(tmp_path: Path) -> None:
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

       from lenia_fft.main import SimState
   ```

   Body assertions cover: state buffer byte-identity round-trip, kernel
   LUT byte-identity round-trip, presence of `leniaFft` key inside
   `meta_blob["meta"]`, and round-trip of nested `view`/`brush` dicts.

All three load-bearing tests exist, named slightly differently than the
spec wording but covering the documented contracts. Names mismatch
(`_falls_back` / `_2d` / unchanged) — flagging in case the spec text
expected exact identifiers.

## M. Citations against upstream (no upstream is vendored)

`grep -rn "Chakazul\|LeniaNDK\|animals\.json"
continuous-ca/lenia-fft/`:

| File:line | Text (verbatim) | Claim type |
|---|---|---|
| `README.md:14` | `Chakazul/Lenia/Python/LeniaNDK.py` `kernel_core[0]` | line-cite (kernel-core registry index) |
| `README.md:17` | `animals.json: Orbium unicaudatus (O2u), Vagorbium undulatus (OV2u),` | line-cite (preset-set anchor) |
| `README.md:82` | `Chan's reference impl: github.com/Chakazul/Lenia (MIT).` | repo-link-cite |
| `docs/notes.md:62` | `Upstream Chakazul/Lenia/Python/LeniaNDK.py:329-335 defines the polyring kernel assembly.` | formula (polyring construction) |
| `docs/notes.md:65` | `# Per upstream LeniaNDK.py:329-335 (verified at architect-2 round-2):` | formula (polyring inline def) |
| `docs/notes.md:94` | `Preset library expansion: enumerate 30-50 polyring creatures from upstream animals.json…` | preset-byte (banked count) |
| `python/tests/test_kernels.py:176` | `presets — see kernels.py module header for the LeniaNDK.py anchor)` | line-cite (kernel-core anchor) |
| `python/lenia_fft/kernels.py:21` | `Chakazul/Lenia on GitHub (MIT license). Kernel: upstream LeniaNDK.py kernel_core[0]` | line-cite (kernel-core registry) |
| `python/lenia_fft/kernels.py:22` | `kernel_core[0] "polynomial (quad4)": K(r) = (4·r·(1-r))^4 for r in [0,1], zero outside.` | formula (quad4) |
| `python/lenia_fft/kernels.py:23` | `Growth map: G(u) = 2*exp(-(u-mu)^2/(2*sigma^2)) - 1 (Gaussian, gn=1).` | formula (growth) |
| `python/lenia_fft/kernels.py:25` | `All Phase 10 presets are single-peak (b="1"), kn=1 (quad4 kernel)…` | behavior (preset roster constraint) |
| `python/lenia_fft/kernels.py:44` | `# This is upstream LeniaNDK.py's kernel_core[0] — the "polynomial (quad4)"…` | line-cite |
| `python/lenia_fft/kernels.py:46` | `every creature with kn=1 in animals.json (all 122 single-peak creatures enumerated upstream)…` | preset-byte (count: 122 single-peak) |
| `python/lenia_fft/kernels.py:64` | `Anchor: Chakazul/Lenia/Python/LeniaNDK.py kernel_core[0] (quad4).` | line-cite |
| `python/lenia_fft/main.py:12` | `(upstream LeniaNDK.py kernel_core[0]; JSON kn=1 via off-by-one indexing)` | line-cite |
| `python/lenia_fft/presets.py:2` | `byte-for-byte from Bert Chan's canonical reference at github.com/Chakazul/Lenia (MIT license).` | preset-byte (whole roster) |
| `python/lenia_fft/presets.py:3` | `Per-creature params: Python/animals.json` | line-cite |
| `python/lenia_fft/presets.py:4` | `Kernel-core registry: Python/LeniaNDK.py kernel_core dict, key 0 (quad4) matched against JSON params.kn=1 via off-by-one indexing.` | line-cite + behavior (off-by-one) |
| `python/lenia_fft/presets.py:11` | `formula documented at LeniaNDK.py:329-335` | formula (polyring) |
| `python/lenia_fft/presets.py:14` | `Verified roster (codes match upstream animals.json):` | preset-byte (codes) |
| `python/lenia_fft/presets.py:39` | `LeniaNDK.py dict keys` | line-cite |
| `python/lenia_fft/presets.py:43` | `Spec authority is upstream animals.json + the verification chain at /tmp/p10presets/ and /tmp/.` | preset-byte (authority claim) |
| `python/lenia_fft/presets.py:76` | `https://github.com/Chakazul/Lenia/blob/master/Python/animals.json` | URL-cite |
| `python/lenia_fft/presets.py:79` | `Decoded via 2D port of LeniaNDK.Board.rle2arr (LeniaNDK.py:184-206).` | line-cite (decoder) |
| `python/lenia_fft/presets.py:203` | `Canonical creatures from Chakazul/Lenia/Python/animals.json.` | preset-byte |
| `python/lenia_fft/presets.py:209` | `header for the LeniaNDK.py anchor), gn=1 (Gaussian growth map).` | line-cite |
| `python/lenia_fft/presets.py:214` | `looks wrong, the source of truth is upstream animals.json…` | preset-byte (authority) |
| `python/lenia_fft/presets.py:317` | `cells from animals.json, centered at the field's middle, over a` | preset-byte (RLE-decoded cells) |

(Also relevant but not matching the grep: the in-source seed-cell
arrays at `presets.py:84–201` are themselves the byte-extracted RLE
decode of `animals.json` for the four shipped 2D presets. They are
data, not citations.)

## N. Skeleton-vs-shipped assessment

Acknowledged banking (deferred, not skeletal):

- 3D FFT path is explicitly not shipped; 3D uses Taichi real-space
  (`main.py:242-254` docstring).
- Volumetric raymarch viewer (banked v1.1 — `main.py:308-311`).
- Sub-stepping (banked v1.1 — `notes.md`).
- Polyring kernels (banked — `notes.md` "Polyring extension banking").
- Named 3D creatures (banked — `presets.py:278-280` and `notes.md`).
- Save-creature UX (banked — `notes.md`).

Shipped-but-skeletal (hidden incompleteness): only one candidate
identified.

- **`paint_splat_2d`'s `n_grid` argument is unused.** Documented as
  intentional (`kernels.py:274-275`: "n_grid is unused in the kernel
  body — kept for API symmetry with future variants that need it.").
  Not skeleton — declared API parking. Worth noting only as
  "scaffolding for future polyring/3D-paint variant".

- **`init_state_random_blob_2d`'s `n_grid` argument is similarly
  unused** with the same justification (`kernels.py:125-127`). Same
  comment.

- **`extract_slice_3d`'s `n_grid` argument is similarly unused**
  (`kernels.py:351`). Same comment.

- **VDB writer "stub" branch** in the GUI label: `f"VDB density
  ({'real' if VdbWriter.is_available() else 'stub'})"`
  (`main.py:637`). The label flips to "stub" when `VdbWriter` reports
  unavailable; the `VdbWriter.write_frame()` call is still issued
  unconditionally on the next line (`main.py:554-560`), so it is on
  `gpusims_common.VdbWriter` to no-op or warn when in stub mode. Not
  inspected here per the no-deep-audit-of-common-py constraint;
  flagging for the common-py audit layer.

- **`do_load_state` partial reallocation comment at line 421-424:**

  ```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:421
              if dim == 2:
                  # Rebuild the FFT backend for the new grid size.
                  # Kernel LUT needs to be populated from the saved buffer below.
                  pass  # convolver re-selected after lut load
  ```

  The `pass` body is a placeholder; the actual re-select happens at
  line 459-467 of the same function. Not a skeleton — it is a
  no-op-marker showing where the re-select would have lived if not
  deferred to after the buffer load. Mildly confusing, but functional.

No deliberate-skeleton (placeholder-comment, no-op, hardcoded-return,
stub-class) code paths were found in the actual numerical / state /
backend / panel logic. The implementation is uniformly shipped.

## O. Cross-workstream incidentals

`grep -rn "from gpusims_common\|import gpusims_common"
continuous-ca/lenia-fft/python/`:

- `python/tests/test_kernels.py:320` — `from gpusims_common import StateReader, StateWriter`
- `python/lenia_fft/main.py:50` —

  ```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:50
  from gpusims_common import (
      Camera,
      CameraMode,
      ParamPanel,
      StateReader,
      StateWriter,
      VdbWriter,
      log,
  )
  ```

- `python/lenia_fft/fft_backend.py:37` — `from gpusims_common import log`
- `python/lenia_fft/presets.py:52` — `from gpusims_common import log`

Load-bearing common-py symbols (flagged for the common-py audit layer,
not deep-audited here):

- **`StateWriter` / `StateReader`** — capture path. Per-buffer
  `{count, stride, format, shape}` field shape is asserted by sim-side
  code (`main.py:374-386`) but populated inside common-py. The CI
  test `test_capture_schema_round_trip` exercises the round-trip but
  does NOT introspect those four fields directly; it asserts
  byte-identity of state/lut and presence of the `leniaFft` meta key.
  Schema drift inside common-py would not be caught at the
  `count/stride/format` level by this sim's tests.
- **`ParamPanel`** — drives the entire GUI block (`main.py:573` and
  every `with panel.folder(...)` site). `ParamPanel.bind(window.get_gui())`
  at `main.py:573` is called every frame. The y-coord-flip
  asymmetry (§ G) is an open question about
  `ParamPanel.folder()`'s coord convention.
- **`Camera` / `CameraMode`** — instantiated for the 3D tier
  (`main.py:312-316`) and serialized via `camera.to_json()` in F5
  saves (`main.py:368`). Used by 3D tier only.
- **`VdbWriter`** — 3D-tier export (`main.py:553-560`); `is_available()`
  drives the GUI label between "real" and "stub". Stub-mode behavior
  is opaque from this sim; flag for common-py audit.
- **`log`** — used in every module; basic logger surface, not
  load-bearing in the structural sense.

## P. Incidental findings

- `kernel_radius=20` for OV2u (Vagorbium undulatus) is a 41×41 kernel
  LUT applied at every cell every step. In the Taichi real-space
  backend that's a 1681-element inner ndrange per cell × n_grid² cells
  per step (`kernels.py:175-180`). This is the largest-radius preset
  in the roster; if any FFT-vs-real-space perf ratio testing surfaces,
  it's the preset where the real-space cost is most painful.

- `cursor_to_field_cell` returns `(int, int)` via `int(...) %
  n_grid`. For `fx_norm` slightly less than zero (cursor panned past
  the left edge), `int(fx_norm * n_grid)` returns a negative value;
  Python's `%` on a negative dividend with positive divisor correctly
  yields a non-negative result (e.g., `-3 % 512 == 509`), so the
  periodic-BC wrap is mathematically correct. Worth noting because in
  C/C++ `-3 % 512 == -3` and a port would need to add `+ n_grid`
  defensive arithmetic. The Taichi `composite_view_2d` kernel already
  uses `fx - n_grid * floor(fx / n_grid)` (`kernels.py:306-307`)
  rather than `%` precisely for this reason.

- `step_2d` always rounds the new state through numpy
  (`main.py:233-235`) **even when** the Taichi real-space backend is
  in use — the docstring at `main.py:226-231` acknowledges this
  ("Taichi-real-space backend skips the numpy round-trip internally"
  — but the wrapper still does `to_numpy()` then `from_numpy()`
  around it). The "skips the numpy round-trip internally" claim
  refers to the in-backend internals (one `from_numpy` + one
  `to_numpy` inside `TaichiRealSpaceConvolver.step` at
  `fft_backend.py:214` and `:225`), so the actual full-round-trip
  count for the real-space backend per frame is **two**:
  `state→numpy→Taichi field→step→numpy→Taichi field`. Not a bug, but
  the in-source comment at `fft_backend.py:18-23` slightly understates
  the cost (it cites "2 numpy↔Taichi round-trips per step", which is
  what actually happens; the disconnect is between that comment and
  `main.py:226-231`'s "skips the numpy round-trip" phrasing).

- The CuPy and PyTorch backends do NOT call the shared
  `_growth_map_apply` helper in `fft_backend.py:99-107` — they each
  re-implement the growth+clip math on their respective array types
  (`fft_backend.py:131-132` for CuPy, `:173-174` for Torch). Only the
  numpy backend uses `_growth_map_apply`. So the helper is, in
  practice, a one-call utility used by one backend, despite its
  docstring claim to "work on both CPU and CuPy/Torch numpy-views" —
  it COULD work for them but the actual per-backend code does not
  route through it. Not a bug; minor surface for refactor at v1.1.

- The `_kernel_fft` for the Torch backend is computed without an
  explicit `.to(self._device)` applied before normalization — the
  `_pad_kernel_to_grid` returns a numpy array, `torch.from_numpy(padded)`
  goes onto the CPU first, then `.to(self._device)` moves it before
  `torch.fft.fft2` runs. Depending on Torch version, this may pin
  some host memory transiently; not a correctness issue.

- `do_load_state` calls `state_reader.find_latest()` then immediately
  proceeds; if `find_latest()` returns `None` (no captures yet), the
  early-return at `main.py:399-401` logs at `log.warn` and exits.
  Sound.

- `imgui.ini` is committed in `python/`. This is GGUI's persisted
  panel-position state and changes whenever the user moves a panel —
  potentially noisy at git-status time. Not a structural issue, just a
  source-control hygiene observation.

End of probe-1 report. Do not propose fix-prompts; await architect-3b synthesis.
