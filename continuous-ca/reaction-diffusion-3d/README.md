# Reaction-Diffusion 3D

> **Stack:** C (native Vulkan 1.3) · **Status:** Implemented (Phase 3) · **Live:** N/A — native binary; build locally

The first GPU-Sims Stack C sim. 256³ Gray-Scott reaction-diffusion on a periodic 3D grid, integrated with Forward Euler at fixed substep dt, visualized via volume raymarching with HDR + Reinhard tonemap. Six Pearson 1993 named parameter presets shipped as the headline UX dropdown — pick a preset and watch that pattern type form in seconds.

## What you're seeing

Two scalar fields `u(x, y, z, t)` and `v(x, y, z, t)` evolving according to the Gray-Scott reaction-diffusion equations. `v` is rendered as the volume's density via front-to-back raymarching, colored by a perceptual LUT (default magma). The patterns you see — spots, stripes, moving fronts, self-replicating spots — are emergent from local diffusion + nonlinear reaction; no global control, no scripted choreography. Different points in the (F, k) parameter space produce qualitatively different stable pattern types, named by Pearson 1993.

The grid is periodic — patterns that drift off one face reappear on the opposite face. Topologically the volume is a 3-torus.

## Controls

| Key / mouse | Action |
|-------------|--------|
| Auto-orbit toggle (in panel) | Default on — slow camera orbit around the volume |
| WASDQE | Move (when Auto-orbit is off) |
| RMB-drag | Look around (when Auto-orbit is off) |
| F5 | Save full capture (camera + parameters + raw u/v fields, ~134 MB) |
| F9 | Load most recent capture |

## The ImGui panel

- **Preset:** dropdown of Pearson 1993 named regions (λ/σ/α/β/ξ/τ) + Custom. Picking a preset sets (F, k, Du, Dv) and resets state to the standard initial conditions.
- **Parameters:** F (feed), k (kill), Du / Dv (diffusion coefficients). Touching any flips the dropdown to "Custom".
- **Integration:** substep count (1–32), initial-condition seed, seed-block size, noise amplitude, "Reseed" button.
- **Rendering:** raymarch step count (16–256), density threshold + intensity, exposure.
- **Color:** colormap dropdown (magma / inferno / viridis / hsv).
- **Camera:** auto-orbit toggle, orbit speed, orbit radius.
- **State:** save / load capture buttons (mirror F5/F9).
- **Profiler:** per-substep + raymarch + ImGui pass timings, live.

## Building

From the repo root:

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPU_SIMS_BUILD_EXAMPLES=ON
cmake --build build --target reaction_diffusion_3d
./build/bin/reaction_diffusion_3d
```

Ubuntu deps for Stack C are listed in `common/common-cpp/README.md` (Vulkan SDK ≥ 1.3, GLFW with X11 backend, VMA, glm, nlohmann_json, spdlog, ImGui, shaderc, OpenVDB optional).

## Mathematics

Gray-Scott:

```
∂u/∂t = Du · ∇²u  −  u·v²  +  F · (1 − u)
∂v/∂t = Dv · ∇²v  +  u·v²  −  (F + k) · v
```

Discretized: Forward Euler in time, central differences in space, normalized `dx = 1` and `dt = 1`. See [`docs/sim-specs/reaction-diffusion-3d.md`](../../docs/sim-specs/reaction-diffusion-3d.md) for full design rationale and references.

## Performance

| Tier | Hardware | Grid | Substeps | Frame time target |
|------|----------|------|----------|-------------------|
| Default | RX 6800 XT or comparable | 256³ | 4 | ≤ 16.7 ms (60 fps) |
| Stretch | RX 6800 XT or comparable | 512³ | 4 | "to be measured post-build" |
| HPC | A100 / 1024³ | 1024³ | 4 | Future stretch (out of v1 scope) |

## References

- John E. Pearson. "Complex Patterns in a Simple System." *Science* 261:189–192 (1993).
- Robert Munafo. "Xmorphia / Reaction-Diffusion explorer" — mrob.com/pub/comp/xmorphia. Canonical online catalog of Gray-Scott parameter regions.
- Inigo Quilez, polynomial colormap fits — [https://www.shadertoy.com/view/WlfXRN](https://www.shadertoy.com/view/WlfXRN). Public domain.

## License

MIT, same as the rest of the repo. See `LICENSE` at the repo root.
