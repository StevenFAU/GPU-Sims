# Eulerian Smoke — Load-bearing vs reversible decisions

This file is the per-sim copy of Phase 8's § 2.1 table — the canonical answer to "what can I tweak by sliding vs what would require rework?" Refer back here when iterating on the sim.

## Load-bearing (decided in Phase 8, expensive to revisit)

| Decision | Why load-bearing |
|----------|------------------|
| 3D storage images with explicit format per field: `R16G16B16A16_SFLOAT` for velocity + curl, `R32_SFLOAT` for density / temperature / pressure / divergence | Compute kernel binding layouts, host allocation pattern, F5 capture binary format, and VDB readback host buffer size all key off these formats. Switching velocity to rgba32f doubles VRAM and bandwidth for marginal precision gain. |
| Ping-pong on velocity / density / temperature / pressure; single non-ping-ponged scratch images for curl + divergence | Switching any ping-pong field to single-buffered requires re-architecting the corresponding dispatch as in-place-safe. |
| Full Fedkiw-2001 solver stack | MacCormack, vorticity confinement, Jacobi pressure projection, buoyancy from temperature are each discrete dispatches in the per-frame chain. Removing any of them changes preset tuning and visual character. |
| Open-top boundary, no-slip on the five remaining faces | Fully-closed bounds pile smoke at ceiling; fully-periodic loses the rising-plume narrative. |
| 192³ / 256³ / 384³ tier dropdown with 256³ default | The default determines the visual baseline recruiters see. Tier changes require full resource recreation. |
| Sparse volumetric-source emitters (cap 8, multi-channel injection) | Distinct from physarum/boids attractor pattern (see phase8_eulerian_smoke.md § 2.4). The pattern is "volumetric source injection," a new convention rule-of-three candidate. |
| Black-body color ramp for temperature-driven emission | Without this, the live demo looks like grayscale fog. Load-bearing visual feature. |
| Beer-Lambert + single-scattering single-shadow-march raymarch | Simplest render path that produces production-grade looking smoke. Multiple-scattering is v1.1+. |
| Density-only per-frame VDB export via `copyFromDense` | Per-frame velocity export would be untenably slow via the synced Vec3 `setValue` path. |
| Hero render via `render-pipelines/blender/render_smoke.py` with Principled Volume | First real script under the offline-render trajectory; sets the shape for future sims. |
| Six smoke-dynamics presets | Spans the visual dynamics range. Reshaping the preset structure changes the panel UX. |
| Capture/load schema with four binary fields + emitter array + camera + meta under JSON key `'eulerianSmoke'` | Format must remain loadable by future versions. Pressure inclusion makes F9 reload not re-converge from scratch. |
| `GPU_SIMS_USE_OPENVDB=OFF` default; `=ON` opt-in for hero-render-capable builds | Flipping default to ON adds ~3–5 min of CI apt install + significant per-user friction. The opt-in posture matches Phase 1's locked decision #8. |

## Reversible (turn the knob; sliders or constants the next reader can change without restructuring)

| Decision | Where |
|----------|-------|
| Jacobi pressure iterations default (40) | `PRESSURE_ITERS_DEFAULT` constant; runtime slider (10–100) |
| Vorticity confinement strength `ε` | `Runtime.vorticityStrength` + slider; preset table |
| MacCormack clamping epsilon | `Runtime.maccormackEpsilon` + slider |
| Buoyancy coefficients `α` and `β` | `Runtime.buoyancyAlpha`, `Runtime.buoyancyBeta` + sliders; preset table |
| Per-field dissipation rates | `Runtime.{velocity,density,temperature}Dissipation` + sliders |
| Density absorption, emission strength, scattering strength, ambient | All sliders under Rendering panel folder |
| Raymarch / shadow-march step counts | Sliders (16–256, 4–64); defaults 96 / 16 |
| Exposure (Reinhard tonemap input) | Slider (0.1–4.0); default 1.2 |
| Render scale (internal resolution fraction) | Slider (0.5–1.0); default 1.0 |
| Substeps per frame | Slider (1–8); default 2 |
| Simulation dt per substep | Slider (0.01–0.5); default 0.1 |
| Light direction (polar coords) + light color + ambient strength | Sliders under Lighting panel folder |
| Background gradient top/bottom colors | ImGui ColorEdit3 controls |
| Emitter density rate, temperature, velocity bias, falloff radius | Sliders under Emitter panel folder |
| Black-body LUT contents | `colormap::build_blackbody_lut_data()` function in `main.cpp` |
| Auto-orbit speed / radius defaults | `ORBIT_DEFAULT_*` constants |
| Camera FOV, near/far planes | `FOV_DEG_DEFAULT`, `NEAR_PLANE`, `FAR_PLANE` constants |
| Window mode default (windowed 1920×1080) | `gv::Window` constructor args |
| Six preset (vorticity, buoyancy, dissipation, emitter) values | `SMOKE_PRESETS` table in `main.cpp` |
| Default preset (Plume) | `Runtime` initial values reference `SMOKE_PRESETS[0]` |

## Future v1.1 candidates

See [`notes.md`](notes.md) for the prioritized v1.1 polish backlog (obstacles, free-slip walls, MGPCG pressure, animation hero renders, GPU isosurface render alternative, etc.).
