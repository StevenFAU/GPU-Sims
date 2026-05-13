# Lenia-FFT — Engineering notes

Banked items, install-stories, v1.1 polish, known issues. Distinct from
`load-bearing-decisions.md` (architecture commitments) — this file is the
loose-leaf scratchpad.

## Install stories (per hardware path)

### NVIDIA CUDA (user lab PC — RTX 2080 Ti)

```bash
# From repo root:
pip install -e common/common-py
pip install -e continuous-ca/lenia-fft/python[cuda]
python continuous-ca/lenia-fft/python/main.py
```

The `[cuda]` extra installs `cupy-cuda12x`. If the lab PC's CUDA is on a
different major version (10.x, 11.x), `pip install -e .[cuda]` will fail
— manually `pip install cupy-cuda11x` (or whichever matches) before the
sim install.

### AMD ROCm (user dev desktop — RX 6800 XT)

```bash
# Install PyTorch ROCm wheel separately (large download, ~2 GB).
pip install torch --index-url https://download.pytorch.org/whl/rocm6.0

# Then install the sim with the rocm extra (which just declares the
# torch>=2.1 dep; pip resolves to the already-installed wheel).
pip install -e common/common-py
pip install -e continuous-ca/lenia-fft/python[rocm]
python continuous-ca/lenia-fft/python/main.py
```

PyTorch-ROCm on AMD desktop is finicky. Known gotchas:
- ROCm version on the system must match the wheel index URL (rocm6.0,
  rocm5.7, etc.).
- `torch.cuda.is_available()` returns True on ROCm too (Torch treats ROCm
  as a CUDA-API-compatible backend); `torch.version.hip` distinguishes.
- If neither CUDA nor ROCm works at runtime, `TorchFFTConvolver` falls
  through and the sim uses Taichi real-space (which works fine on Vulkan).

### Universal-baseline (no GPU-FFT extras)

```bash
pip install -e common/common-py
pip install -e continuous-ca/lenia-fft/python
python continuous-ca/lenia-fft/python/main.py
```

The sim runs on Taichi real-space convolution (universal-baseline). On the
user's hardware (either machine) this is interactive at 512² and 1024²;
2048² is capture-mode.

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

## v1.1 polish backlog

Items deferred from v1 to keep Phase 10 scoped:

- **Save-creature UX:** RMB-snapshot a 256×256 state crop centered on cursor
  + parameter set → `creatures/<name>.json` for later recall. Lenia's
  research workflow is parameter-sweep + stable-creature discovery; this
  is the natural save mechanism beyond F5-state.

- **Named 3D creatures from Chan's research:** v1 ships only a generic 3D
  "Random Blob" preset. Chan's published 3D Lenia animals are real but
  parameter-stability is finicky and visual verification at 128³ needs
  the user's hardware before specific parameters are encoded.

- **Volumetric raymarch interactive viewer:** v1 ships iso-cross-section-
  slice display. Volumetric raymarch (Beer-Lambert through 3D scalar field)
  is a separate shader effort — banked.

- **Sub-stepping per frame:** v1 runs 1 step/frame. Stability at large dt
  on stretch tiers may benefit from N substeps/frame (MPM Phase 9 ships
  25/frame). Bank as a tier-dependent default.

- **Parameter-search UI:** Lenia research is largely parameter-space
  exploration. A "scan mu/sigma over a range and animate" UI would be a
  natural Lenia-specific addition. Bank.

- **Tier-3 diagnostics module (per Phase 9 banked structure):** Lenia
  doesn't have an equivalent of MPM's stress-tensor diagnostics, but a
  "kernel-frequency-response diagnostics" tool would expose the
  Fourier-domain shape of the chosen creature's kernel. Bank as Tier-3
  diagnostic surface when relevant.

- **PyTorch-ROCm install-story automation:** Currently the user installs
  PyTorch-ROCm separately via the `--index-url` flag. The sim's `[rocm]`
  extra just declares the `torch>=2.1` Python dep. A doc page or install
  script that figures out the right ROCm version + wheel index URL
  automatically would smooth this.

## Known issues / gotchas

- **Taichi 1.7.4 has no FFT.** Confirmed at draft-time sandbox probe. The
  runtime backend selection is the workaround. If a future Taichi version
  adds FFT, it can become a fifth backend candidate (priority between
  Taichi-real-space and numpy).

- **CuPy version mismatch with system CUDA:** `cupy-cuda12x` requires CUDA
  12.x. The `[cuda]` extra hard-pins the 12x flavor. Users with CUDA 11.x
  need to manually install `cupy-cuda11x` instead.

- **`np.fft.ifft2` returns complex even for real-FFT-roundtrip.** Caller
  must do `.real.astype(np.float32)`. Otherwise the type-checker won't
  catch it but the downstream consumer (Taichi from_numpy) will reject
  complex64 input. Verified at draft-time sandbox.

- **Taichi cursor coords have y=0 at BOTTOM** (NDC convention) but
  `panel.folder()` positions panels with y=0 at TOP. `_cursor_in_any_panel`
  converts cursor y to top-origin before testing — inherited verbatim from
  MPM Phase 9. Banking is in `/docs/conventions.md` "GUI panel coordinate
  conventions" (Phase 9 banked).

- **Bilinear sampling in `composite_view_2d` uses periodic-BC wrap.** This
  matches Lenia's torus BC and lets the user pan past edges to wrap. If
  someone wants "show black off-edge" view mode (zero-pad), it's a v1.1
  toggle.

## Polish-4 GGUI Y-convention asymmetry (Phase 10)

Phase 10 polish-4 fixed a paint-cursor Y inversion in `cursor_to_field_cell`
(main.py:164) by removing a `(1.0 - fy_norm)` flip that empirically
inverted paint position on the user's AMD desktop (Taichi 1.7.4, Vulkan,
Ubuntu 24.04).

The fix surfaces an asymmetry: `cursor_to_field_cell` (paint) now uses
direct cursor-y → row-y mapping, while `cursor_in_any_panel`
(GUI-occlusion test, main.py:203) keeps the `(1.0 - cy_bottom)` flip
inherited verbatim from MPM main.py:306-318 (Phase 9 visual-verified).

Both functions work empirically. The asymmetry suggests Taichi GGUI's
cursor-y origin differs from its panel-rect coordinate convention, OR
that MPM's panels happen to live in a region where the panel-rect flip
is symmetric enough not to misfire, OR that there's a Phase-9-era bug
that lenia-fft inherited but doesn't manifest because lenia's panels also
happen to sit in the upper-left quadrant.

This is a STRONG candidate for the consumer-#3 `common-py` promotion
review: when a third Stack D sim arrives with a 2D click-paint surface
and panels in DIFFERENT screen regions than MPM/lenia, the true GGUI
convention should be empirically pinned down and the abstraction
extracted into `common-py.cursor_utils` (or similar).

Bank for retrospective. Phase 10's promotion-review section already
flagged GUI-occlusion testing as a STRONG promotion candidate; this
asymmetry sharpens the rationale.
