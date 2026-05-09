# Strange Attractors — Load-bearing vs reversible decisions

This file is the per-sim copy of Phase 2's § 2.1 table — the canonical answer
to "what can I tweak by sliding vs what would require rework?" Refer back here
when iterating on the sim.

## Load-bearing (decided in Phase 2, expensive to revisit)

| Decision | Why load-bearing |
|----------|------------------|
| HDR accumulation format `rgba16float` | Without HDR, additive blending clips at white after a handful of frames. Switching to `rgba8unorm` later means redesigning tonemap. |
| Ping-pong color attachments | `read_write` storage textures on `rgba16float` are feature-gated. Choreography is wired throughout the renderer. |
| RK4 integration scheme | Switching integrators means re-tuning every attractor's parameters. |
| Storage buffer of `vec4<f32>` per particle, instanced point quads | Different rendering primitive = new bind-group layouts and a new vertex pipeline. |
| F5 capture schema (with `schemaVersion`, `initSeed`) | Existing captures must remain loadable. Field additions OK; renames break replay. |

## Reversible (turn the knob)

| Decision | Where |
|----------|-------|
| Bloom intensity / threshold / soft-knee | lil-gui sliders |
| Point sprite size, AA softness | lil-gui slider (size); softness is a constant in `splat.frag.wgsl` |
| Depth attenuation `k` | lil-gui slider |
| Default colormap | One-line constant in `main.ts` (`COLORMAP_INDEX.magma`) |
| Substep count defaults per attractor | `attractors.ts` per-attractor `defaultSubsteps` |
| α-decay preset values | `main.ts` constants in the trail-folder dropdown's `setValue` |
| Camera orbit speed and radius defaults | `attractors.ts` per-attractor `orbitRadius`, top-level `ORBIT_DEFAULT_DEG_PER_SEC` |
| Particle initial-condition seed default | `DEFAULT_SEED` constant in `main.ts` |
| Color scale + exponent defaults | `attractors.ts` per-attractor `defaultColorSpeedScale`; exponent default in `main.ts` |

## Future v1.1 candidates

- AA line-segment quads as an alternative to point sprites
- More attractors (Halvorsen, Rössler, Chen, Sprott)
- Alternative tonemap operators (ACES, Hejl-Burgess-Dawson)
- Toast UI for hot-reload events (currently console-only)
- Stack C native variant pushing 10M+ particles
