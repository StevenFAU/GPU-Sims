# Reaction-Diffusion 3D — Load-bearing vs reversible decisions

This file is the per-sim copy of Phase 3's § 2.1 table — the canonical answer
to "what can I tweak by sliding vs what would require rework?" Refer back here
when iterating on the sim.

## Load-bearing (decided in Phase 3, expensive to revisit)

| Decision | Why load-bearing |
|----------|------------------|
| 3D `r32f` storage images, ping-pong pair per field × 2 fields | The compute kernel's binding layout, the host allocation pattern, and the F5 capture format all key off this shape. |
| FP32 fields | FP16 needs `VK_KHR_16bit_storage` plumbing and re-validation that Forward Euler stays stable at FP16 precision. |
| Periodic boundary conditions via `VK_SAMPLER_ADDRESS_MODE_REPEAT` | Switching to Neumann/Dirichlet means shader changes and parameter re-tuning. |
| Volume raymarch as the rendering primitive | Decides the entire render-pipeline shape. Isosurface (marching cubes) is explicitly v1.1. |
| Pearson preset dropdown structure | The presets ARE the demo. Adding more presets is trivial; reshaping the interaction is not. |
| F5 capture schema (camera + meta + raw u/v binary blobs) | Format must remain loadable by future versions. Field additions OK; renames break replay. |
| GpuProfiler scopes structure (`substep`, `raymarch`, `imgui`) | First real consumer of common-cpp's profiler ring buffer. |

## Reversible (turn the knob)

| Decision | Where |
|----------|-------|
| Substep count default (4) | `SUBSTEPS_DEFAULT` constant; runtime slider |
| Raymarch step count, density transfer parameters | Runtime sliders |
| Density transfer function shape | Hardcoded in `raymarch.frag.glsl`; alternative shapes are straightforward edits |
| Default colormap (magma) | `Runtime.colormap` initial value; LUT is a 256×4 texture, swap is free |
| F/k slider ranges around each preset | Slider bounds in `main.cpp` |
| Pearson preset (F, k) approximate values | `PEARSON_PRESETS` table in `main.cpp` |
| Initial-condition seed-block size (16) and noise amplitude (0.05) | Runtime sliders + `SEED_BLOCK_DEF` / `NOISE_AMP_DEFAULT` constants |
| Bloom — currently unused | Reserved uniform; v1.1 candidate to wire HDR ping-pong + half-res bloom |
| Camera orbit speed / radius defaults | `ORBIT_DEFAULT_*` constants |
| Window mode default (windowed 1920×1080) | `gv::Window` constructor args; fullscreen-borderless is a v1.1 polish item once `Window` grows the option |

## Future v1.1 candidates

- GPU isosurface extraction (marching cubes) as an alternative to raymarch
- Phase 2-style HDR ping-pong + half-res bloom + decoupled tonemap pass
- More Pearson presets from Munafo's parameter map
- Stack D (Taichi) port for cross-runtime benchmarking
- VDB export when Phase 5 lands the real OpenVDB writer
- 512³ defaults with measured 6800 XT timings
- Fullscreen-borderless window mode (requires extending `gv::Window`)
