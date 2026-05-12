# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]


## [0.9.2] - 2026-05-11

### Fixed

- **`common/common-cpp/src/vk/context.cpp:207` name-collision** (latent since Phase 1, surfaced by Phase 8.5's CI fix-forward). Renamed the private member `Context::createDebugMessenger()` to `Context::initDebugMessenger()` so the line-207 unqualified call resolves to the namespace-scope free function `gpusims::vk::createDebugMessenger(...)` instead of shadowing on the class member. Three sites touched: `context.hpp:78` (declaration), `context.cpp:116` (ctor call), `context.cpp:202` (definition). Build (native) Debug-job now compiles cleanly for the first time since Phase 1.
- **`.github/workflows/markdown.yml` lychee args** (latent since Phase 3.5). The repo's `lychee.toml` had `exclude_path = ["docs/sim-specs/_template.md"]` but `lychee-action`'s explicit-`args:` mode bypassed the config-file read. Added `--exclude-path docs/sim-specs/_template.md` to the workflow args. Markdown `Check internal links` job now passes.

### Changed

- **`project-state.md` § 9 "CI baseline"** rewritten as "CI baseline (post-Phase-8.5.1)" — honestly reflects that this is the first time in the project's history all five repo-level CI workflows are simultaneously green. Prior banking only addressed the three always-on workflows; the two path-triggered workflows had been red on at least one job each.
- **`project-state.md` § 9 Stack C known-issues** added entry banking the line-207 name-collision episode (latent since Phase 1, root cause, resolution at Phase 8.5.1).
- **`project-state.md` § 7 conventions** added two new entries: "Architect-1 fabrication discipline extends to transitive-dependency closures" (banks the Phase 8.5 Boost-dep lesson) and "Watch the actual CI surface, not the assumed CI surface" (banks the Phase 8.5.1 Debug-job-red-for-9-days lesson).
- **`project-state.md` § 11 Quick reference** added a Stack C Debug build command alongside the existing Release command, so local builds catch Debug-only defects before they surface in CI.

### Convention

- **§ 7 "Architect-1 fabrication discipline extends to transitive-dependency closures"** — for any new system-package CI enablement, verify the transitive dep closure against the bare CI runner before locking the spec.
- **§ 7 "Watch the actual CI surface, not the assumed CI surface"** — every Stack C-touching push should be followed by an explicit `gh run list --workflow=build-native.yml` check confirming both Release AND Debug jobs are green; same for Build (web) on Stack B pushes.


## [0.9.1] - 2026-05-11

### Added

- `tsconfig.shared.json` at repo root — single source of truth for Stack B TypeScript compiler flags. Every Stack B workspace tsconfig `extends:` it. Strict superset including `noUncheckedIndexedAccess: true`, formalized in `project-state.md` § 7 "Stack B shared `tsconfig.shared.json` extends-from-root pattern."

### Changed

- **CI: `.github/workflows/build-native.yml` Release job** — added `libopenvdb-dev` to apt-get install list; added `-DGPU_SIMS_USE_OPENVDB=ON` to the CMake configure step. The Release job now exercises eulerian-smoke's OpenVDB-enabled code path on every push. Debug job continues to verify stub-mode compilation.
- **Stack B tsconfigs (seven files)** refactored to extends-from-root pattern: `common/common-web/tsconfig.json`, `common/common-web/examples/hello/tsconfig.json`, `closed-form/strange-attractors/web/tsconfig.json`, `closed-form/mandelbulb-explorer/web/tsconfig.json`, `continuous-ca/reaction-diffusion-2d/web/tsconfig.json`, `agent-based/physarum/web/tsconfig.json`, `agent-based/boids-3d/web/tsconfig.json`.
- **`common-web/src/gpuProfiler.ts` and `common-web/src/stateReader.ts`** — fixed all strict-mode sites flagged by `noUncheckedIndexedAccess: true` (exact site count in the commit body and the Phase 8.5 completion report). Fix pattern: explicit guard + `log.warn` or `throw`, fail-loud; no `!` non-null assertions.
- **Root `README.md` gallery row** — mandelbulb-explorer status flipped from "Not started" (stale since Phase 4 shipped at `8d8334f`) to live link + Phase 4 reference.
- **`project-state.md`** — Phase 8.5 entries: "Last updated" prologue updated; § 7 "Stack B tsconfig.json shape parity" rewritten as "Stack B shared `tsconfig.shared.json` extends-from-root pattern"; § 9 strict-mode-gaps entry marked resolved; § 9 present-mode entry rewritten as a retraction of the Phase 8 attribution (see "Retracted" section below); § 11 Quick reference latest-commit pointer, live-sims list, and Vite port list all refreshed.
- **`volumetric-grid/eulerian-smoke/docs/notes.md` Priority 1.12** — rewritten as "Diagnose actual cause of sustained fan load" (was: "VSync default + panel toggle"). See "Retracted" section.

### Retracted

- **Present-mode-as-heat-source attribution (Phase 8 retro, commit `9ad5120`).** The original Phase 8.5 brief banked a `Window`-ctor `VkPresentModeKHR` API amendment as the proper fix for eulerian-smoke's sustained fan load on the RX 6800 XT. The hypothesis was made without measurement and was falsified by direct test (`vblank_mode=3 ./build/bin/eulerian_smoke` — forces FIFO at the Mesa driver layer, equivalent to the proposed API amendment for the user-facing effect — did not reduce fan load). The `Window` API amendment scope was dropped from Phase 8.5. `project-state.md` § 9 line 519 and `volumetric-grid/eulerian-smoke/docs/notes.md` Priority 1.12 were both amended to retract the framing; § 7 line 457 instance (d) (the architect-1 fabrication-pattern entry that itself surfaced the contradiction) stands as-is. The actual cause of the sustained fan load is uncharacterized and banked as a v1.1 diagnostic item.

### Convention

- **§ 7 "Stack B shared `tsconfig.shared.json` extends-from-root pattern"** added (replaces "Stack B `tsconfig.json` shape parity").


## [0.9.0] - 2026-05-11

### Added

- Eulerian Smoke simulation (Stack C) — 256³ Stam stable-fluids with the full Fedkiw-2001 solver stack. First Tier-2 flagship sim. Path: `volumetric-grid/eulerian-smoke/`.
  - Single-pass MacCormack-corrected semi-Lagrangian advection (Selle/Fedkiw 2008) with reverse-Stam clamping limiter.
  - Vorticity confinement (Fedkiw 2001 eq 14) with zero-guarded unit-gradient normalization.
  - Jacobi-iteration pressure projection (panel slider 10–100, default 40).
  - Boussinesq buoyancy from temperature; six smoke-dynamics presets (Plume, Candle, Cigar, Smokestack, Explosion-Puff, Chimney-Down).
  - Sparse user-placed emitters (LMB-place / RMB-remove, cap 8) — first consumer of the new "volumetric source-injection" pattern, distinct from the physarum/boids massless-attractor pattern.
  - Volume raymarch render: Beer-Lambert absorption + single-scattering single-shadow-march + temperature-driven black-body emission via a 256×4 RGBA8 LUT (four ramps: blackbody/sunset/cold/mono).
  - Three discrete grid-resolution tiers (192³ / 256³ / 384³; default 256³); 384³ marked as the stretch tier with the degradation-warning idiom from boids-3d's 100k tier.
  - F5/F9 capture/load with full mid-frame state restore (velocity + density + temperature + pressure + emitters + camera + parameters).
  - Optional per-frame OpenVDB density export via `gpusims::vdb::writeFloatFrame` — first real consumer of the OpenVDB writer that has shipped as a stub since Phase 1. Opt-in via `-DGPU_SIMS_USE_OPENVDB=ON` at CMake-configure time (default remains OFF; CI continues to verify stub-mode compilation only).
  - One-shot temperature VDB export for hero-render pairing.
- `render-pipelines/blender/render_smoke.py` — first script under the offline-render trajectory. Headless Blender Cycles with the Principled Volume shader, GPU device-selection fallback chain (OptiX → HIP → CUDA → fail loud), supports both single-still and animation modes via `--frame-start` / `--frame-end`. v1 deliverable is a single hero still; animation banked v1.1 once A100 access is available.

### Changed

- `project-state.md`: § 3 phase ledger updated with Phase 8 row; § 5 per-stack package surface area — added note that Phase 8 is the first real exercise of `vdb_writer`; § 6 eulerian-smoke row → Implemented (Phase 8); § 9 known issues — entry added if Phase 8 execution surfaced a defect in the never-exercised `vdb_writer.cpp` path.
- `README.md` (root) — gallery row for eulerian-smoke flipped from "Not started" to "Implemented (Phase 8)".
- `CMakeLists.txt` (root) — eulerian-smoke promoted to unconditional `add_subdirectory(volumetric-grid/eulerian-smoke)`; lattice-boltzmann entry preserved as commented for its future phase.
- `docs/sim-specs/eulerian-smoke.md` — fleshed §§ 1–11 from Phase 0 stubs.

### Convention extensions

- **Volumetric source-injection emitter pattern** added to `project-state.md` § 7. Consumer #1: eulerian-smoke (this phase). Likely consumers #2 + #3: sph-water, mpm-multimaterial. Promotion review at consumer #3.
- **Point-emitter / sparse-source pattern** convention annotated: eulerian-smoke was evaluated and explicitly did NOT count as consumer #3 (its emitter pattern is volumetric source injection, a distinct shape).
- **Tier-1 / Tier-2 framing** added to `project-state.md` § 7. Tier-2 = "A100-hero" Stack C/D flagship sims; Tier-1 = Stack B WebGPU warm-ups. Banked from the overarching-spec's categorization for future architect reference.


## [0.8.0] - 2026-05-10

### Added

- Boids 3D simulation (Stack B) — multi-species 3D Reynolds flocking with leader attractors and dynamic predators. Live at <https://stevenfau.github.io/GPU-Sims/boids-3d/>.
  - Spatial-hash counting-sort (histogram + multi-block prefix scan + scatter) for neighbor queries at up to 100k+1k entities.
  - Three runtime-switchable predator hunting modes (nearest-prey, stochastic-prey, flock-center).
  - Persistent leader attractors (LMB-place, Shift+LMB-remove; cap 32) with cosine-envelope falloff.
  - Six parameter presets (Cohesive Flock, Loose Murmuration, Tight Schooling, Predator Spread, Waypoint Tour, Chaos).
  - Four discrete agent-count tiers (25k / 50k / 75k / 100k) with degradation contract at hero tier.
  - Bit-exact-within-one-step capture/load round-trip across all entity types and panel state.
  - First Stack B sim with: 3D free-fly camera driving rasterization (vs. mandelbulb's raymarcher); manual render-pass construction with depth attachment; spatial-hash compute (counting-sort with multi-block prefix scan); instanced low-poly rendering with velocity-derived orientation + Gram-Schmidt singularity fallback; click-to-place ground-plane unproject.
- Per-sim Vite dev port: 5178.

### Fixed

- `common/common-web/src/camera.ts` Y-flip in `Camera.projection()` removed. The `out[5] *= -1` line was a Vulkan-idiom mistakenly applied to a WebGPU pipeline (Vulkan clip-space Y points down, WebGPU Y points up). The flip had silently inverted world-Y in every Stack B render since common-web shipped; strange-attractors and mandelbulb masked the artifact because their rendered shapes have no canonical orientation. Boids-3d would have been the first sim to visibly exhibit the bug. Caught during architect-2 cross-review. See project-state.md § 9.

### Changed

- `project-state.md`: § 3 phase ledger updated with Phase 7 row; § 5 stale dev-port reference updated; § 6 boids-3d row → Implemented; § 9 known issues — `camera.ts` Y-flip resolution note added; § 10 architect-1 onboarding prompt updated with the clone-the-repo-directly note for accessing actual common-* source.

## [0.7.0] — 2026-05-10

### Added
- **Phase 6:** Fourth Stack B sim — `agent-based/physarum/`. Multi-species Jones 2010 slime-mold transport-network simulation: discrete agents on a continuous 2D periodic domain, three species with mutual repulsion, RGB-per-species trail visualization. Discrete agent-count tier dropdown (256k / 1M / 4M / 10M, default 4M) and grid-size dropdown (512 / 1024 / 2048, default 1024). First agent-system sim in the repo and **first user of `atomic<u32>` storage buffers** (three deposit buffers indexed `cellY * gridSize + cellX`; agent-move uses `atomicAdd`, diffuse-decay reads non-atomically). Live at <https://stevenfau.github.io/GPU-Sims/physarum/>.
- Six named presets covering the parameter space: **Networks** (default), **Snowflake**, **Highways**, **Conflict**, **Cooperation**, **Chaos**. Picking a preset overwrites all seven shape parameters and (by default) reseeds; touching any slider flips the dropdown to "Custom".
- Persistent food-source pins as the headline interactive moment: LMB-click places (cap 32), RMB-click within 8 cells removes, panel "Clear all" button. New `pin_deposit.compute.wgsl` pass (conditional, dispatched only when `pinCount > 0`) scatters per-pin contributions into the deposit buffers each frame; visualize fragment overlays 1-pixel white outline rings at active pins.
- Capture/load (F5/F9): trail map (rgba16float bytes, 8 MB at 1024²) + RNG seed + parameters + pin array. **Agents are reseeded from the captured `initSeed`, not loaded literally** — at the 10M tier the agent buffer is 160 MB, beyond reasonable browser-ZIP territory. Same seed + same agent count + same parameters = bit-identical reproduction; different agent count = visually-similar but not literally-identical.
- First sim to use raised `requiredLimits` on `Context.create` (`maxStorageBufferBindingSize: 200_000_000`, `maxBufferSize: 200_000_000`) — the 10M-tier agent buffer exceeds baseline 128 MiB binding-size limit. v1.1 graceful-fallback path documented in `agent-based/physarum/docs/notes.md` priority 1.6.
- First sim to use `@workgroup_size(256, 1, 1)` on the 1D-dispatch compute kernels (agent-move and clear-deposits) rather than the conventional 64. Load-bearing: at 64-wide the 4M tier needs 65,536 workgroups in X — exceeds baseline `maxComputeWorkgroupsPerDimension` (65,535) by one.

### Changed
- Per-sim Vite dev port for physarum: 5177.
- Gallery card list (`gallery/index.html`) extended with the Physarum entry between rd-2d and the hello demo.
- Root README gallery row for Physarum: status flipped from "Not started" to live.

### Convention extensions
- **No-Stack-A pattern:** physarum is the first sim to ship without a `shadertoy/` counterpart, establishing the convention for future Stack B-originated sims (boids-3d, neural-CA web variant, lenia-fft web variant) that have no clean Shadertoy expression. The `agent-based/physarum/` folder contains `web/`, `docs/`, and a sim-level `README.md` only.
- **Atomic-buffer compute idiom:** load-bearing for any future agent or sparse-source simulation in the repo. Documented in `agent-based/physarum/docs/load-bearing-decisions.md` § 2.
- **Sparse-source point-emitter pattern:** first consumer of the food-pin / fluid-emitter / particle-emitter shape (32-element fixed-size storage buffer, 2D dispatch over the grid that iterates pins per cell). Promotion to common-web fires at the third consumer (boids-3d or eulerian-smoke); intentionally per-sim in v1.
- **Discrete agent-count tiers:** first sim with a tier-dropdown for buffer-size-affecting parameters rather than a continuous slider. Pattern: a `Record<string, number>` of named tiers, `recreateGridResources()` on dropdown change.

## [0.6.0] — 2026-05-09

### Added
- **Phase 5:** Third Stack B sim — `continuous-ca/reaction-diffusion-2d/`. Gray-Scott reaction-diffusion on a 512² periodic 2D grid (range 256–1024 via dropdown), Forward Euler integration with substep slider 1–32 (default 4), compute ping-pong on `rg32float` storage textures, manual bilinear visualization through a 4-LUT colormap (magma default). Six Pearson 1993 named parameter presets (λ, σ, α, β, ξ, τ) matching the Stack C `reaction-diffusion-3d` sim's preset names exactly for cross-stack vocabulary parity.
- Mouse-paint brush: LMB-drag splats `v` material onto the grid via a separate compute kernel. The first Stack B sim with user-driven compute dispatch.
- Second Stack A → B port; first multi-file Stack A artifact (`shadertoy/BufA.glsl` + `shadertoy/Image.glsl` + `shadertoy/README.md`). Multi-buffer Shadertoy idiom — establishes the convention for future ports of stateful sims (physarum, neural-CA).
- `Texture.readback2D(bytesPerPixel)` helper on `common/common-web/src/webgpu/texture.ts` — async readback of a 2D texture's mip-0 contents into a `Uint8Array`. Mirrors `Buffer.readback`. First helper of its kind on common-web; future sims that capture texture state (physarum, neural-CA, lenia-fft web variants) consume it.
- `ParamPanel.refreshDisplays()` (and matching `ParamFolder.refreshDisplays()` interface + `FolderImpl` implementation) on `common/common-web/src/paramPanel.ts`. Walks every controller under the panel via lil-gui's `controllersRecursive()` and calls `updateDisplay()` on each. Workaround for the lil-gui slider-freeze on externally-mutated bound state (project-state.md § 9 known issue 3); applies to every Stack B sim with presets or captures.
- Full-state capture: F5 saves JSON parameter block + deinterleaved `u.bin` + `v.bin` in a ZIP, matching Phase 3's `reaction-diffusion-3d` per-field shape under the JSON meta key `'reactionDiffusion2d'`.
- Live at <https://stevenfau.github.io/GPU-Sims/reaction-diffusion-2d/>.

### Changed
- Per-sim Vite dev port for reaction-diffusion-2d: 5176.

### Fixed
- `closed-form/mandelbulb-explorer/web/src/main.ts`: three HMR path constants corrected from the repo-relative form (`'closed-form/mandelbulb-explorer/web/shaders/...'`) to the per-sim `web/`-relative form (`'shaders/...'`). The previous form did not match what `viteWgslPlugin` emits, so mandelbulb's hot-reload was silently a no-op. No behavior change other than hot-reload starting to fire correctly during `npm run dev`.

## [0.5.0] — 2026-05-09

### Added
- **Phase 4:** Second Stack B sim — `closed-form/mandelbulb-explorer/`. Distance-estimator raymarcher (Daniel White / Paul Nylander mandelbulb formulation, n=8 default, range [2, 12]) with cone-traced soft shadows, three orbit-trap coloring presets, and optional auto-morph of the power exponent. First Stack A → B port in the repo: a Steven-original Shadertoy-idiom GLSL implementation preserved at `closed-form/mandelbulb-explorer/shadertoy/mandelbulb.glsl` alongside the WebGPU port at `closed-form/mandelbulb-explorer/web/`. Live at <https://stevenfau.github.io/GPU-Sims/mandelbulb-explorer/>.
- Render pipeline: single-pass raymarch into an `rgba16float` HDR offscreen RT, then a Reinhard tonemap pass to the swap-chain image. `renderScale` slider (0.5–1.0) trades resolution for cost — actually reduces fragment count, not a faux quality knob.
- First sim to consume `@gpusims/common-web` without compute pipelines: pure render-only pipeline, two render passes, one offscreen texture, two uniform buffers, one linear sampler.

## [0.4.0] — 2026-05-09

### Added
- **Phase 3:** First Stack C sim — `continuous-ca/reaction-diffusion-3d/`. 256³ Gray-Scott reaction-diffusion on a periodic 3D grid, Forward Euler integration with fixed substep dt, volume raymarch visualization with HDR + Reinhard tonemap inline. Six Pearson 1993 named parameter presets (λ, σ, α, β, ξ, τ) shipped as the headline UX dropdown.
- `gpusims::vk::memoryBarrier(cmd, srcStage, srcAccess, dstStage, dstAccess)` helper in `common/common-cpp/` for global `VkMemoryBarrier2` via `vkCmdPipelineBarrier2`. Used at all three barrier sites in reaction-diffusion-3d's substep loop.
- 3D-image support in `gpusims::vk::Image::upload` and `Image::readback` (host-visible staging buffer + queue submit + fence wait, transitioning through `TRANSFER_*_OPTIMAL` and back to `GENERAL`).
- `gpusims::StateReader::findLatest(root)` static method.

### Changed
- `SIM_DT_DEFAULT` for reaction-diffusion-3d set to 0.5 (from 1.0). Forward Euler stability bound in 3D is `Du·dt/dx² ≤ 1/6`; at canonical `Du = 0.16, dx = 1, dt = 1`, the ratio is exactly `0.16` (at the edge, no headroom for the reaction terms). 0.5 brings it to a comfortable 0.08.

### Removed
- `volumetric-grid/reaction-diffusion-3d/` stub directory and stale README. Canonical home is `continuous-ca/reaction-diffusion-3d/` per project-state.md § 6.

## [0.3.0] — 2026-05-09

### Added
- **Phase 2:** First Stack B sim — `closed-form/strange-attractors/`. 2M particles integrating Lorenz / Aizawa / Thomas ODEs via classical RK4. HDR additive accumulation (`rgba16float` ping-pong), bloom (extract → blur → composite), inline Reinhard tonemap. Live at <https://stevenfau.github.io/GPU-Sims/strange-attractors/>.
- GitHub Pages deploy automation via `.github/workflows/deploy-pages.yml` (Node 22, builds common-web + hello-web + strange-attractors-web, uploads `_site/` artifact, deploys via `actions/deploy-pages@v4`).
- Canvas-DPR convention for Stack B portfolio sims (project-state.md § 7): canvases fill viewport, render at `clamp(devicePixelRatio, 1, 2)` scaling.

## [0.2.1] — 2026-05-08

### Added
- **Phase 1.5:** `common/common-web/` — WebGPU + TypeScript shared infrastructure. `Context`, `Renderer`, `Camera`, `HotReloader`, `GpuProfiler`, `StateWriter` / `StateReader` (ZIP via fflate), `ParamPanel` (lil-gui), `viteWgslPlugin` for shader hot-reload, `Buffer` / `Texture` / `ShaderModule` / `ComputePipeline` / `RenderPipeline` wrappers.
- Hello-world example at `common/common-web/examples/hello/`.
- Gallery placeholder at `gallery/index.html`.
- `.github/workflows/build-web.yml` (Node 22 typecheck + build).

## [0.2.0] — 2026-05-08

### Added
- **Phase 1:** `common/common-cpp/` — Vulkan 1.3 shared infrastructure. `vk::Context`, `vk::Window`, `vk::Renderer`, `vk::Frame`, `vk::Buffer`, `vk::Image`, `vk::ShaderCompiler` (wraps shaderc), `vk::ComputePipeline`, `vk::GraphicsPipeline`, `Camera`, `HotReloader`, `GpuProfiler` (timestamp-query ring buffer), `StateWriter` / `StateReader` (JSON + binary), ImGui glue.
- Hello-world example at `common/common-cpp/examples/hello/` exercising every Phase 1 subsystem end-to-end.
- `.github/workflows/build-native.yml` (Ubuntu 24.04 + Vulkan SDK + Ninja release build).
- Top-level CMake build system; per-sim CMakeLists pattern.
- Modern Vulkan extensions adopted: dynamic rendering, sync2, descriptor indexing, buffer device address, scalar block layout.

## [0.1.0] — 2026-05-08

### Added
- Initial repository skeleton (Phase 0).
- Authoritative specification documents: `docs/overarching-spec.md`, `docs/root-context-distilled.md`.
- Per-sim spec sheet template at `docs/sim-specs/_template.md`.
- Stub spec sheets for all 14 simulations in the catalog.
- Sim category and per-sim README stubs.
- `render-pipelines/` skeleton with Blender (default), Houdini, and OptiX subfolders.
- Repository hygiene: `LICENSE` (MIT), `.gitignore`, `.gitattributes`, `.editorconfig`, `.clang-format`.
- GitHub-surface files: `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, `CITATION.cff`, issue and PR templates.
- CI workflows for markdown linting and structure validation.

[Unreleased]: https://github.com/StevenFAU/GPU-Sims/compare/v0.6.0...HEAD
[0.6.0]: https://github.com/StevenFAU/GPU-Sims/compare/v0.5.0...v0.6.0
[0.5.0]: https://github.com/StevenFAU/GPU-Sims/compare/v0.4.0...v0.5.0
[0.4.0]: https://github.com/StevenFAU/GPU-Sims/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/StevenFAU/GPU-Sims/compare/v0.2.1...v0.3.0
[0.2.1]: https://github.com/StevenFAU/GPU-Sims/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/StevenFAU/GPU-Sims/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/StevenFAU/GPU-Sims/releases/tag/v0.1.0
