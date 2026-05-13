# Lenia (FFT / real-space)

Bert Chan's continuous CA on a 2D periodic grid (+ opt-in 3D). Runtime
FFT-backend selection (CuPy / PyTorch / Taichi real-space / numpy) for
GPU acceleration on both NVIDIA CUDA and AMD ROCm hardware.

![status](https://img.shields.io/badge/status-Phase%2010-success)

## What it does

`state[t+1] = clip(state[t] + dt · G(K ⊛ state[t]), 0, 1)`

with `K(r) = (4·r·(1-r))^4` (quad4 polynomial kernel; upstream
`Chakazul/Lenia/Python/LeniaNDK.py` `kernel_core[0]`) and
`G(u) = 2·exp(-(u-μ)² / (2σ²)) - 1` (Gaussian growth map). Periodic BCs.
Four canonical 2D creature presets verified byte-for-byte against upstream
`animals.json`: Orbium unicaudatus (`O2u`), Vagorbium undulatus (`OV2u`),
Gyrorbium gyrans (`OG2g`), Discutium valvatus (`S2v`). All four are
single-peak (b="1"), kn=1, gn=1. Opt-in 3D tier with generic random-blob
preset. Polyring (multi-peak) creatures banked v1.1+.

## Quickstart

```bash
# baseline (universal — works on both CUDA and Vulkan):
pip install -e common/common-py
pip install -e continuous-ca/lenia-fft/python
python -m lenia_fft.main

# OR with GPU-FFT extras (pick ONE matching your hardware):
pip install -e continuous-ca/lenia-fft/python[cuda]         # NVIDIA CuPy
pip install -e continuous-ca/lenia-fft/python[rocm]         # AMD ROCm
pip install -e continuous-ca/lenia-fft/python[cuda-torch]   # NVIDIA via PyTorch (alternative to CuPy)
```

See `docs/notes.md` for full install stories per hardware path.

## Controls

| Action | Key/Mouse |
|---|---|
| Paint | LMB-drag |
| Erase | RMB-drag |
| Save state | F5 |
| Load latest state | F9 |
| Reset to preset | R |
| Pause / unpause | Space |
| 3D camera (3D tier only) | WASDQE + RMB-look |
| Quit | Esc |

Sliders in the panels: kernel growth μ, kernel growth σ, time resolution T,
brush radius, brush intensity, slice-position (3D tier).

## Tiers

| Tier | Dim | Grid | Notes |
|---|---|---|---|
| 0 default | 2 | 512² | Real-time on all backends |
| 1 mid | 2 | 1024² | Real-time on FFT backends |
| 2 stretch | 2 | 2048² | Real-time on GPU-FFT; capture-mode on real-space |
| 3 3D stretch | 3 | 128³ | Capture-mode; iso-cross-section-slice viewer |

The 2048² tier label is dynamically annotated at sim start based on which
FFT backend the runtime probe selected.

## Export paths

- **PNG sequence** (`frames_export/`) — every 4th frame, via `ti.tools.imwrite`.
  Consumed by `render-pipelines/blender/render_lenia_2d.py` for polished hero
  video.
- **VDB sequence** (`vdb_export/density/`) — 3D tier only, every 4th frame,
  via `gpusims_common.vdb_writer.write_float_frame`. Consumed by
  `render-pipelines/blender/render_lenia_3d.py` for volumetric hero render.
- **MP4 video** (`frames_export/video_<frame>/...`) — in-sim, via
  `ti.tools.VideoManager`. Fast iteration; raw quality. Toggle in Export panel.
- **State capture** (`captures/`) — F5/F9, full simulation state for
  resume / diagnostics.

## References

- Chan, B. (2019). [Lenia: Biology of Artificial Life](https://arxiv.org/abs/1812.05433).
- Chan's reference impl: [github.com/Chakazul/Lenia](https://github.com/Chakazul/Lenia) (MIT).
