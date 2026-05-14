# Lenia (FFT / real-space) — sim spec

Stack D continuous-CA. First sim with runtime FFT-backend selection.

## 1. What it simulates

Bert Chan's Lenia: a continuous-valued cellular automaton on a 2D (or 3D) periodic
grid where each cell holds a scalar in [0, 1]. Per-step update:

    state[t+1] = clip(state[t] + dt · G(K ⊛ state[t]), 0, 1)

with K the upstream "quad4" radial polynomial kernel `K(r) = (4·r·(1-r))^4` for
r ∈ [0, 1] (zero outside), and G the Gaussian growth map
`G(u) = 2·exp(-(u-μ)²/(2σ²)) - 1`.

Kernel anchor: `Chakazul/Lenia/Python/LeniaNDK.py` `kernel_core[0]` (matched against
JSON `params.kn=1` via off-by-one indexing). All 122 single-peak (b="1") creatures
in upstream `animals.json` use this kernel; Phase 10's preset roster is byte-verified
against that source.

## 2. Tiers

| Tier | Dim | Grid | Memory | Real-space | GPU-FFT |
|---|---|---|---|---|---|
| Default | 2 | 512² | ~5 MB | 60+ fps | 60+ fps |
| Mid | 2 | 1024² | ~12 MB | 30–60 fps | 60+ fps |
| Stretch | 2 | 2048² | ~50 MB | 5–15 fps capture | 30–60 fps |
| 3D Stretch | 3 | 128³ | ~32 MB | 5–15 fps capture | N/A v1.1 |

## 3. Backends

Runtime FFT-backend selection at sim init, priority order:

1. CuPy (NVIDIA CUDA)
2. PyTorch FFT (NVIDIA CUDA or AMD ROCm)
3. Taichi real-space convolution (universal — CUDA + Vulkan)
4. numpy FFT (CPU fallback / CI smoke)

CuPy + PyTorch are optional extras (`pip install -e .[cuda]` / `.[rocm]` /
`.[cuda-torch]`); Taichi-real-space is universal baseline.

## 4. Controls

LMB-drag paint, RMB-drag erase, F5 save, F9 load, R reset, Space pause,
Esc quit. 3D tier adds WASDQE camera + slice-axis radio + slice-index slider.

## 5. Presets (verified against Chakazul/Lenia/Python/animals.json)

All 2D presets are single-peak (b="1"), kn=1 (quad4 kernel), gn=1 (Gaussian growth).
Codes match upstream entries.

| Slot | Creature | Code | R | T | μ | σ |
|---|---|---|---|---|---|---|
| 1 — basic glider | Orbium unicaudatus | `O2u` | 13 | 10 | 0.15 | 0.015 |
| 2 — wanderer | Vagorbium undulatus | `OV2u` | 20 | 10 | 0.2 | 0.031 |
| 3 — rotator | Gyrorbium gyrans | `OG2g` | 13 | 10 | 0.156 | 0.0224 |
| 4 — shield | Discutium valvatus | `S2v` | 15 | 10 | 0.331 | 0.057 |
| 5 — 3D bootstrap | 3D Random Blob | — | 8 | 10 | 0.15 | 0.015 |

Reference: `Chakazul/Lenia` on GitHub (MIT). Polyring (multi-peak) creatures
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
banked v1.1 with formula anchor at `LeniaNDK.py:329-335`.

## 6. Export paths

- PNG sequence (`frames_export/`) via `ti.tools.imwrite`
- VDB sequence (`vdb_export/density/`, 3D only) via `gpusims_common.vdb_writer`
- MP4 video (`frames_export/video_<frame>/`) via `ti.tools.VideoManager`
- State capture (`captures/`) via `gpusims_common.state_writer`, sim-namespaced as `leniaFft`

## 7. Hero render

- `render-pipelines/blender/render_lenia_2d.py` — Cycles compositor on PNG
  sequence, polished MP4 output (color grade + saturation + vignette + optional
  temporal blur).
- `render-pipelines/blender/render_lenia_3d.py` — Cycles volume scatter on VDB
  sequence, mirrors `render_smoke.py` template.
- Houdini path banked, not built (license-dependent).

## 8. CI

`build-py.yml` exercises Taichi-CPU + numpy-FFT backend smoke. GPU-FFT paths
(CuPy / PyTorch-ROCm) are user-runtime visual verification only.

## 9. Implementation status

- 2D path: Phase 10 v1 (this spec).
- 3D path: Phase 10 v1 opt-in tier (contingency: scope-cut to Phase 10.5 if needed).
- Stack B WebGPU port: deferred (Phase 11 or 12).
- Save-creature UX: v1.1.
- Volumetric raymarch viewer: v1.1.
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
- Polyring kernel extension: v1.1+ (formula anchor at `LeniaNDK.py:329-335`).

## 10. References

- Chan, B. (2019). [Lenia: Biology of Artificial Life](https://arxiv.org/abs/1812.05433).
- [Chakazul/Lenia](https://github.com/Chakazul/Lenia) — canonical reference impl.

## 11. Phase ledger

- Phase 0 (stub): created the sim directory shell.
- **Phase 10 (this spec):** v1 implementation, 2D + opt-in 3D, runtime FFT-backend
  selection, quad4 polynomial kernel, four upstream-verified 2D presets, hero
  render scripts.
