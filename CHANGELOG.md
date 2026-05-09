# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]


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

[Unreleased]: https://github.com/StevenFAU/GPU-Sims/compare/v0.4.0...HEAD
[0.4.0]: https://github.com/StevenFAU/GPU-Sims/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/StevenFAU/GPU-Sims/compare/v0.2.1...v0.3.0
[0.2.1]: https://github.com/StevenFAU/GPU-Sims/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/StevenFAU/GPU-Sims/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/StevenFAU/GPU-Sims/releases/tag/v0.1.0
