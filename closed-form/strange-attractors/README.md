# Strange Attractors

> **Stack:** B (WebGPU) · **Status:** Implemented (Phase 2) · **Live:** [stevenfau.github.io/GPU-Sims/strange-attractors/](https://stevenfau.github.io/GPU-Sims/strange-attractors/)

The first GPU-Sims Stack B sim. 2 million particles integrating one of three classic 3D ODE attractor systems (Lorenz, Aizawa, Thomas) via RK4, additively rendered into an HDR accumulation buffer with optional decay. Choose between the persistent "Manifold" preset (the attractor's invariant set fills in over time as a glowing 3D wireframe of light) and the "Motion" preset (decaying trails — particles read as comets).

## What you're seeing

Each particle is an independent trajectory through the chosen attractor's state space. The integrator advances all 2M trajectories in parallel each frame. Particles never collide, never interact — they're independent samples of the same dynamical system. Over thousands of frames, the ensemble traces out the attractor's invariant manifold in proportion to how often the dynamics visit each region.

Color encodes velocity magnitude: cool colors are slow (regions where trajectories linger near unstable equilibria), hot colors are fast (transit regions). The default colormap is magma (perceptually uniform); inferno, viridis, and HSV are available in the lil-gui Color folder.

## Controls

| Key / mouse | Action |
|-------------|--------|
| Auto-orbit toggle (in panel) | Default on — slow camera orbit around the attractor |
| WASDQE | Move (when Auto-orbit is off) |
| RMB-drag | Look around (when Auto-orbit is off) |
| F5 | Save capture (downloads `strange_attractors_NNNN.zip`) |
| F9 | Load capture (file picker) |

The lil-gui panel exposes:
- **Attractor:** system selector (Lorenz / Aizawa / Thomas) + per-system parameter sliders
- **Integration:** RK4 substep count, simulation `dt`, particle initial-condition seed
- **Rendering:** point size, depth attenuation `k`
- **Trail:** preset (Manifold / Motion / Custom) + α-decay slider
- **Color:** colormap (magma / inferno / viridis / HSV), color speed scale, color exponent
- **Post:** bloom intensity / threshold / soft-knee, exposure
- **Camera:** auto-orbit toggle, orbit speed, orbit radius
- **State:** save/load buttons (mirror F5/F9)

All settings persist to `localStorage` per browser. Refresh restores them.

## Mathematics

Three classic chaotic ODE systems, each integrated by classical RK4 with a fixed substep size. See [`docs/sim-specs/strange-attractors.md`](../../docs/sim-specs/strange-attractors.md) for the exact equations, default parameters, and design rationale.

## How to run locally

From the repo root:

```
npm install                                     # one-time, installs all workspaces
npm run dev --workspace=@gpusims/strange-attractors-web
```

Open http://127.0.0.1:5174 in a WebGPU-enabled browser.

To produce a deployable build:

```
npm run build --workspace=@gpusims/strange-attractors-web
# Output in: closed-form/strange-attractors/web/dist/
```

The deployed version on GitHub Pages is built and pushed by `.github/workflows/deploy-pages.yml` on every push to `main`.

## Performance

| Tier | Hardware | Particle count | Frame time target |
|------|----------|----------------|-------------------|
| Desktop (default) | RX 6800 XT or comparable | 2,000,000 | ≤ 16.7 ms (60 fps) |
| Native stretch | A100 / native Stack C | 10,000,000+ | (Stack C variant deferred) |

Measured numbers populated post-build. See `docs/sim-specs/strange-attractors.md` § 8.

## References

- Edward N. Lorenz, "Deterministic Nonperiodic Flow," Journal of the Atmospheric Sciences, 20(2):130–141, 1963.
- Yoji Aizawa, "Symbolic Dynamics Approach to Intermittent Chaos," Progress of Theoretical Physics, 1984.
- René Thomas, "Deterministic chaos seen in terms of feedback circuits…," International Journal of Bifurcation and Chaos, 9(10):1889–1905, 1999.
- Inigo Quilez, polynomial colormap fits — [https://www.shadertoy.com/view/WlfXRN](https://www.shadertoy.com/view/WlfXRN). License: public domain (Shadertoy default).
- Reference implementations consulted (no code lifted): BrutPitt's [glChAoS.P](https://github.com/BrutPitt/glChAoS.P), Paul Bourke's strange-attractor gallery, [merrypranxter/strange_attractors](https://github.com/merrypranxter/strange_attractors).

## License

MIT, same as the rest of the repo. See `LICENSE` at the repo root.
