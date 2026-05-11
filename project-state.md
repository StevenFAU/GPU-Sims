# GPU-Sims — Project State

> **Last updated:** end of Phase 5 — `reaction-diffusion-2d` shipped at `ed54dd3`, typecheck fix at `e1f0673`, retro at this commit.

> This is the **canonical handoff document** for the GPU-Sims repository. It exists so that:
>
> - A new **repo-architect chat** can take over from a fresh context, pick up where the previous architect left off, and make consistent decisions with what came before.
> - A new **category-architect chat** (responsible for, e.g., volumetric-grid sims) can scope its category-level architecture against the locked cross-stack decisions.
> - A new **reviewer-architect chat** (cross-reviews phase specs before Claude Code execution) can catch drift between draft and synced repo state.
> - A **per-sim implementer chat** can find the conventions it needs without re-deriving them.
> - A **coordinator chat** (the user's project-management role) can quickly see what's done, what's next, and what's blocked.
>
> **Update rule:** every phase ends with at minimum a new row in § 3 (phase ledger). If the phase locked a new architectural decision or changed an existing one, also update § 4. If a stack's `common-` package changed surface area, update § 5. If a category gained a sim or shifted scope, update § 6. If new known issues surfaced, update § 9. Other sections evolve as needed.
>
> **For routine sim work, read this doc plus the relevant phase spec(s) (in project source).** This doc is short on purpose; the repo at github.com/StevenFAU/GPU-Sims is the authoritative source for code-level questions.

---

## 1. Project overview

GPU-Sims is a portfolio repository: a collection of GPU-accelerated physics and emergence simulations, each pushed toward maximum scale and visual quality, all consuming a small set of shared infrastructure libraries. The portfolio's purpose is to demonstrate technical depth across multiple stacks (Shadertoy, WebGPU, native Vulkan, Python/Taichi) — not to maximize traffic or audience size.

- **Owner:** Steven Cohen — `StevenFAU` on GitHub, FAU CS student.
- **Repo:** `git@github.com:StevenFAU/GPU-Sims.git` (public, MIT licensed).
- **Primary dev hardware:** AMD RX 6800 XT (16 GB) on Ubuntu 24.04. Lab access to 4×RTX 2080 Ti and an A100 HPC node for hero renders. One sim eventually hits a D-Wave annealer.
- **Mindset:** "Few amazing sims, not several alright ones." The owner pays for tokens for quality; back-loaded debugging on per-sim phases is OK, but the foundation phases must be right.
- **Authoritative cross-cutting spec:** `docs/overarching-spec.md` (in repo). Reasoning + rejected alternatives: `docs/root-context-distilled.md` (in repo).

---

## 2. Architectural shape

Four stacks. Every sim is implemented in exactly one (or sometimes two — Shadertoy → WebGPU port).

| Stack | Tech | Shared infra | Status |
|-------|------|--------------|--------|
| **A** | Shadertoy / single-file GLSL | None — single-file demos | Used as starting point for some sims; no `common-` package |
| **B** | WebGPU + TypeScript + Vite 7 | `common/common-web/` (`@gpusims/common-web`) | **Implemented (Phase 1.5)** |
| **C** | Native C++20 + Vulkan 1.3 | `common/common-cpp/` (`gpusims::common_cpp`) | **Implemented (Phase 1)** |
| **D** | Python + Taichi (+ optional CuPy) | `common/common-py/` | Not yet drafted; lands with first Stack D sim |

Seven categories. Each lives in its own top-level folder.

| Category | Folder | Sims (planned) |
|----------|--------|---------------|
| Closed-form  | `closed-form/`         | strange-attractors, mandelbulb-explorer |
| Agent-based  | `agent-based/`         | physarum, boids-3d |
| Continuous CA | `continuous-ca/`       | lenia-fft, reaction-diffusion-3d, neural-ca, reaction-diffusion-2d |
| Volumetric grid | `volumetric-grid/`  | eulerian-smoke, lattice-boltzmann |
| Particle fluids | `particle-fluids/`  | sph-water, pic-flip |
| Hybrid particle-grid | `hybrid-particle-grid/` | mpm-multimaterial |
| Quantum | `quantum/`             | ising-dwave |

(Sim-to-category assignments evolve; treat the table as a snapshot, not a contract. The authoritative current list is the gallery row in `README.md`.)

---

## 3. Phase ledger

| # | Phase | Scope | Status | Shipped at |
|---|-------|-------|--------|------------|
| 0 | Skeleton | Repo bootstrap: directory structure, gitignore, MIT license, per-category READMEs, sim-spec stubs, gallery row | ✅ Shipped | `706c7cb` |
| 1 | common-cpp | Vulkan 1.3 shared infrastructure: `Context`, `Window`, `Renderer`, `Camera`, `HotReloader`, `GpuProfiler`, `StateWriter`/`Reader`, ImGui glue, VDB/Alembic stubs. CI build-native job. Hello-world example exercises everything. **Eight spec defects caught + fixed during first build** (see commit body). | ✅ Shipped | `3a64055` |
| 1.5 | common-web | WebGPU + TypeScript shared infrastructure: same surface as common-cpp adapted for the browser. Vite WGSL plugin for hot-reload. CI build-web job. lil-gui parameter panel. ZIP-based state capture. Hello-world example. Gallery placeholder index.html. **Eight spec defects caught + fixed** (TS strict-mode dominant; see commit body). | ✅ Shipped | `6b5309a` |
| 2 | strange-attractors | First Stack B sim. Validates `common-web` against a real consumer. Adds GitHub Pages deploy automation. Adopts canvas-DPR convention for all Stack B portfolio sims. | ✅ Shipped | `7a4f3f5` |
| 3 | reaction-diffusion-3d | First Stack C sim. Validates `common-cpp` at simulation scale. 256³ Gray-Scott RD on a periodic 3D grid, volume raymarch visualization, six Pearson 1993 named presets. VDB writer deferred to whichever phase ships `eulerian-smoke` (the natural sparse-volume consumer). | ✅ Shipped | `d517f02` |
| 3.5 | hardening pass | README gallery row fixes, `structure.yml` stale-entry drop, CHANGELOG backfill (0.2.0 / 0.2.1 / 0.3.0 / 0.4.0), dead `windowFullscreen` capture-schema field removal, markdown-lint + lychee config to bring all three CI workflows green simultaneously for the first time since Phase 0. | ✅ Shipped | `3de7cc5` |
| 4 | mandelbulb-explorer | Shadertoy → WebGPU port. **First Stack A→B port** (Steven-original Shadertoy GLSL preserved at `closed-form/mandelbulb-explorer/shadertoy/`, real WebGPU port at `closed-form/mandelbulb-explorer/web/`). DE raymarcher with cone-traced soft shadows, three orbit-trap coloring presets, n-power morph (toggle default OFF). **First sim using `common-web` in a render-only pipeline** (no compute, just two render passes — single-uniform-buffer raymarch into HDR `rgba16float` offscreen RT, then Reinhard tonemap to canvas). `renderScale` slider trades resolution for cost. **First multi-architect cross-review chain** (3 review rounds caught Camera.lookAt drift, WGSL UV inversion bug, full StateWriter/Reader API mismatch, README gallery-row omission, plus two polish flags). Live at <https://stevenfau.github.io/GPU-Sims/mandelbulb-explorer/>. | ✅ Shipped | `8d8334f` |
| 5 | reaction-diffusion-2d | Second Stack A → B port. **First Stack B sim with compute ping-pong on persistent 2D state** (Phase 2's strange-attractors uses compute for particle integration but does not ping-pong general 2D grid state; Phase 4's mandelbulb-explorer is render-only). Six Pearson 1993 named presets matching the Stack C `reaction-diffusion-3d` sim's preset names exactly for cross-stack vocabulary parity. Mouse-paint brush (LMB-drag splats `v` material) via separate compute kernel. Full-state capture (deinterleaved `u.bin` + `v.bin` matching Phase 3's per-field shape) under JSON meta key `'reactionDiffusion2d'`. **First multi-file Stack A artifact in the repo** (`shadertoy/BufA.glsl` + `shadertoy/Image.glsl` + `shadertoy/README.md`) — extends Phase 4's single-file convention to sims with persistent state. Two in-flight common-web additions (both spec-authorized): `Texture.readback2D` and `ParamPanel.refreshDisplays` (see § 5). One in-flight cross-cutting fix: mandelbulb-explorer's 3 HMR path constants corrected to per-sim `web/`-relative shape so hot-reload actually fires. **Typecheck-fix follow-up at `e1f0673`** (function declaration → arrow expression for TS strict-mode closure narrowing; see § 7). Live at <https://stevenfau.github.io/GPU-Sims/reaction-diffusion-2d/>. | ✅ Shipped | `ed54dd3` |
| 6 | physarum | First agent-system Stack B sim and **first user of `atomic<u32>` storage buffers** in the repo. Multi-species Jones 2010 model: discrete agents on a continuous 2D periodic domain, three species with mutual repulsion (RGB-per-species visualization). Discrete agent-count tier dropdown (256k / 1M / 4M / 10M, default 4M); first sim with raised `requiredLimits` (`maxStorageBufferBindingSize: 200_000_000`) for the 10M-tier 160 MB agent buffer; first sim using `@workgroup_size(256, 1, 1)` on 1D-dispatch kernels (4M tier exceeds baseline `maxComputeWorkgroupsPerDimension` at 64-wide). Persistent food-source pins as the headline interactive moment (LMB places, RMB removes, cap 32; new `pin_deposit.compute.wgsl` pass). Capture/load: trail map + RNG seed + parameters; agents reseeded from seed on load (literal positions not preserved by design). **First sim to ship without a Stack A counterpart** — the `agent-based/physarum/` folder contains `web/`, `docs/`, sim-level `README.md` only; no `shadertoy/`. Establishes the no-port pattern for future Stack B-originated sims. Live at <https://stevenfau.github.io/GPU-Sims/physarum/>. | ✅ Shipped | `1250971` |
| 7 | boids-3d | First 3D Stack B sim and **first user of spatial-hash counting-sort** (counting + multi-block prefix scan + scatter). Multi-species 3D Reynolds flocking with **persistent leader attractors** (LMB-place, Shift+LMB-remove; cap 32) AND **dynamic predators** (auto-spawned, three runtime-switchable hunting modes — nearest-prey, stochastic-prey, flock-center). Discrete tier dropdown (5k / 10k / 25k / 50k / 75k / 100k boids; default 10k after polish pass at `cda37d3`). First sim with: free-fly 3D camera driving rasterization (vs. mandelbulb's raymarcher); manual render-pass construction with depth attachment (mandelbulb-pattern, bypassing `Renderer.beginRendering` which doesn't support depth); instanced low-poly rendering with velocity-derived orientation + Gram-Schmidt singularity fallback; click-to-place ground-plane unproject; ping-pong on a unified entity buffer with explicit read-old-write-new invariant; 3-mode predator kernel with state-buffer union semantics + capture-restoring dropdown mode. Bit-exact-within-one-integration-step capture/load (~640 KB ZIP at default tier). **Second consumer of physarum's agent-buffer + sparse-source pattern; promotion review of these patterns scheduled for the third consumer (likely eulerian-smoke / sph-water / lattice-boltzmann) per rule-of-three.** Box size scaled 32u→80u edge in polish pass after visual verification showed the original cramped 32u box prevented visible flock structure at the original 50k default (volumetric density math missing from the original spec; see § 7). Live at <https://stevenfau.github.io/GPU-Sims/boids-3d/>. | ✅ Shipped | `38d9ab0` + polish `cda37d3` |
| 8 | eulerian-smoke | First Tier-2 flagship Stack C; first real OpenVDB consumer; first Blender Cycles offline-render exercise. Stam stable-fluids w/ MacCormack + vorticity confinement + Jacobi pressure. 6 presets, sparse emitter UX, optional per-frame VDB density export. Hero render via render-pipelines/blender/render_smoke.py. | ✅ Shipped | ``867ea39`` |
| 9 | common-py + first-d-sim | `common-py` infrastructure + first Stack D sim (likely `lenia-fft` or `mpm-multimaterial`). | ⬜ Not started | — |
| 10+ | Remaining sims | One phase per remaining sim. Each consumes a settled `common-` package; per-sim phases are smaller than the foundation phases. | ⬜ Not started | — |

(Phase numbering is for the architect's reference; commit messages don't need to use it.)

---

## 4. Locked architectural decisions

These are settled. Don't re-litigate without strong cause; if a future phase wants to overturn one, that's an architecture-level discussion, not a per-sim choice.

| # | Decision | Rationale (one line) | Locked in |
|---|----------|---------------------|-----------|
| 1 | **Stack C uses Vulkan 1.3, not OpenGL** | Sync-heavy flagship sims hit OpenGL's `glMemoryBarrier` ceiling; Vulkan validation layers are a real productivity tool; Khronos has put OpenGL in maintenance mode. | Phase 1 |
| 2 | **Stack B uses WebGPU only — no WebGL2 fallback** | Doubles the code path for ~zero benefit to target audience (recruiters/reviewers on current browsers). "Requires current browser" is a legitimate stance. | Phase 1.5 |
| 3 | **Modern Vulkan extensions: dynamic rendering, sync2, descriptor indexing, buffer device address, scalar block layout** | Keeps verbosity manageable. All are core or near-core in 1.3. | Phase 1 |
| 4 | **Hot-reload at frame boundary, not mid-frame** | Both stacks: pipeline reload happens between frames, with old GPU resources deferred or GC'd. Matches the natural flush points. | Phase 1 / 1.5 |
| 5 | **GPU profiler uses ring-buffered timestamp queries with delayed read** | Avoids pipeline stalls. Lag of ~`MAX_FRAMES_IN_FLIGHT` frames is imperceptible at 60 fps. (Stack B implementation has a known issue — see § 9.) | Phase 1 / 1.5 |
| 6 | **VMA for all Vulkan memory allocations** | Hand-rolling Vulkan memory management is uniformly a bad idea. | Phase 1 |
| 7 | **`MAX_FRAMES_IN_FLIGHT = 2` globally** | Both stacks. Defined in `gpu_profiler.hpp` (Stack C) and `index.ts` (Stack B). Profiler ring-buffer count must match renderer's. | Phase 1 / 1.5 |
| 8 | **OpenVDB / Alembic are stubs in Phase 1; real impls land with first consumer sim** | Phase 1 ships API headers + write-stubs; the real impl lands with `eulerian-smoke` (VDB) or `sph-water` (Alembic). | Phase 1 |
| 9 | **GLFW X11 backend on Linux even on Wayland sessions** | Wayland + Vulkan surface protocol round-trips stall `glfwPollEvents` 5–20 ms per call when cursor is over the window. `glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11)` resolves it. | Phase 1 |
| 10 | **`vulkan-validationlayers` (no `-dev`) on Ubuntu 24.04+** | The `-dev` package was removed; runtime layers ship in the bare package. | Phase 1 |
| 11 | **Per-sim deployable static sites for Stack B** | Each sim under `<category>/<sim>/web/` is independently deployable to GitHub Pages as a subpath. Gallery `index.html` at repo root links them. | Phase 1.5 |
| 12 | **Vite 7 for Stack B builds, npm workspaces at repo root** | Modern de facto for WebGPU + TS. Workspace lets per-sim packages depend on `@gpusims/common-web` via plain semver. Node 22 LTS minimum (Vite 7's hard requirement). | Phase 1.5 |
| 13 | **Workspace deps use plain semver (`^0.1.0`), not `workspace:*`** | The `workspace:` URL protocol is pnpm/yarn convention; npm only added support in npm 11. Node 22 LTS ships npm 10, which expects plain semver and resolves locally automatically. | Phase 1.5 |
| 14 | **WGSL hot-reload via custom Vite plugin** | Vite's HMR system pushes change events; runtime `HotReloader` subscribes and dispatches per-shader callbacks. Conceptually identical to common-cpp's hot-reload, mechanically push (Vite) instead of poll (filesystem). | Phase 1.5 |
| 15 | **lil-gui for in-browser parameter UI** | The de facto debug-UI library for WebGPU/Three.js demos. ParamPanel wraps it with localStorage persistence per sim. | Phase 1.5 |
| 16 | **State capture: JSON + binary, zipped (web) / loose dir (native)** | Same JSON schema across stacks. Web side packs into ZIP for one-click download/upload; native side writes a loose directory. Cross-stack replay is technically possible (same schema), not used today but kept open. | Phase 1 / 1.5 |
| 17 | **TypeScript strict mode includes `exactOptionalPropertyTypes: true`** | Catches real defects at compile time (Phase 1.5 found 10 sites). Construction pattern: build descriptor as a local, then `if (label) desc.label = label;` — not `{label: cond ? '...' : undefined}`. | Phase 1.5 |
| 18 | **Stack A artifacts preserved alongside Stack B ports** | When a sim is ported A→B, the A artifact lives at `<category>/<sim>/shadertoy/<sim>.glsl` with a port-mapping README; the B port lives at `<category>/<sim>/web/`. Both halves in the repo make the port pattern a documented, reusable convention (mandelbulb-explorer is the canonical example; reaction-diffusion-2d will reuse it). | Phase 4 |

---

## 5. Per-stack package surface area

### `common-cpp` (Stack C, Vulkan 1.3)

Public namespace `gpusims::` (generic) + `gpusims::vk::` (Vulkan-specific). Per-sim consumes via:

```cmake
target_link_libraries(my_sim PRIVATE gpusims::common_cpp)
```

```cpp
#include <gpusims/camera.hpp>
#include <gpusims/vk/context.hpp>
```

Module list: `Camera`, `HotReloader`, `GpuProfiler`, `StateWriter`/`StateReader`, `ImGui` glue, `vdb_writer.hpp` / `alembic_writer.hpp` (stubs), `vk::Context`, `vk::Window`, `vk::Renderer`, `vk::Frame`, `vk::Buffer`, `vk::Image`, `vk::ShaderCompiler`, `vk::ComputePipeline`, `vk::GraphicsPipeline`, `vk::debug`.

> **First-exercised in Phase 8.** Prior to Phase 8 the synced `vdb_writer.cpp` impl was stub-shipped but never invoked; the `writeFloatGrid` (used via `writeFloatFrame`) and `frameSequencePath` paths are exercised for the first time in eulerian-smoke's optional per-frame VDB export. Hard-rule-8 in `phase8_eulerian_smoke.md § 0` documents the in-flight-defect-fix posture if anything surfaces.

Hello-world: `common/common-cpp/examples/hello/`. Built with `-DGPU_SIMS_BUILD_EXAMPLES=ON`. Binary at `build/bin/gpu_sims_hello`.

### `common-web` (Stack B, WebGPU)

npm package `@gpusims/common-web` (workspace-only; never published to public npm). Per-sim consumes via:

```json
{ "dependencies": { "@gpusims/common-web": "^0.1.0" } }
```

```ts
import { Context, Renderer, Camera, ParamPanel } from '@gpusims/common-web';
```

```ts
// In sim's vite.config.ts
import { wgslPlugin } from '@gpusims/common-web/vite-plugin';
export default defineConfig({ plugins: [wgslPlugin()] });
```

Module list: same surface as common-cpp, adapted to TypeScript and WebGPU. `ComputePipeline` / `RenderPipeline` instead of `vk::ComputePipeline` / `vk::GraphicsPipeline`. `ParamPanel` (lil-gui) instead of ImGui. `StateWriter`/`Reader` use ZIP via `fflate`. Math via gl-matrix. `Camera.lookAt(x, y, z)` was added in Phase 2 — pre-existing in synced source by Phase 4 even though the Phase 1.5 spec didn't mention it.

Hello-world: `common/common-web/examples/hello/`. Run with `npm run dev:hello-web` from repo root; opens at http://127.0.0.1:5173.

**Phase 5 additions** (in-flight, both authorized by the Phase 5 spec):

- `Texture.readback2D(bytesPerPixel: number): Promise<Uint8Array>` — async readback of a 2D texture's mip-0 contents into a `Uint8Array`. Mirrors `Buffer.readback`. Throws if `width × bytesPerPixel` is not a multiple of 256 (WebGPU `bytesPerRow` alignment requirement). Used by `reaction-diffusion-2d`'s F5 capture path; future Stack B sims that capture texture state (physarum, neural-CA, lenia-fft web variants) consume it.
- `ParamPanel.refreshDisplays(): void` (plus matching `ParamFolder.refreshDisplays()` interface method + `FolderImpl` implementation) — walks every controller under the panel via lil-gui's `controllersRecursive()` and calls `updateDisplay()` on each. Workaround for the lil-gui slider-freeze on externally-mutated bound state (see § 9). Fail-loud posture: logs the first failure per call via `log.warn` rather than silent swallow. Future Stack B sims with presets or captures (physarum, boids-3d, neural-CA, eulerian-smoke when ported) inherit it.

Per-sim Vite dev ports (assigned in numerical order as sims ship): hello-web 5173, strange-attractors 5174, mandelbulb-explorer 5175, reaction-diffusion-2d 5176, physarum 5177, boids-3d 5178. Next sim takes 5179.

### `common-py` (Stack D, Python/Taichi) — not yet implemented

Lands with the first Stack D sim phase.

---

## 6. Per-category status

| Category | Sims | Stack | Status |
|----------|------|-------|--------|
| `closed-form/strange-attractors/` | strange-attractors | B | **Implemented (Phase 2)**; live at <https://stevenfau.github.io/GPU-Sims/strange-attractors/> |
| `closed-form/mandelbulb-explorer/` | mandelbulb-explorer | A → B | **Implemented (Phase 4)**; live at <https://stevenfau.github.io/GPU-Sims/mandelbulb-explorer/> |
| `agent-based/physarum/` | physarum | B (no Stack A) | **Implemented (Phase 6)**; live at <https://stevenfau.github.io/GPU-Sims/physarum/>. First sim shipping without a `shadertoy/` counterpart. |
| `agent-based/boids-3d/` | boids-3d | B (no Stack A) | **Implemented (Phase 7)**; live at <https://stevenfau.github.io/GPU-Sims/boids-3d/>. Second consumer of the physarum agent-buffer + sparse-source pattern (rule-of-three promotion review scheduled for consumer #3). |
| `continuous-ca/lenia-fft/` | lenia-fft | D | Sim-spec stub; tied to Phase 7 (common-py) |
| `continuous-ca/reaction-diffusion-3d/` | reaction-diffusion-3d | C | **Implemented (Phase 3)** |
| `continuous-ca/reaction-diffusion-2d/` | reaction-diffusion-2d | A → B | **Implemented (Phase 5)**; live at <https://stevenfau.github.io/GPU-Sims/reaction-diffusion-2d/> |
| `continuous-ca/neural-ca/` | neural-ca | TBD | Sim-spec stub |
| `volumetric-grid/eulerian-smoke/` | eulerian-smoke | C | **Implemented (Phase 8)** — first Tier-2 flagship Stack C sim; first real OpenVDB consumer. Stam stable-fluids w/ MacCormack + vorticity confinement + Jacobi pressure. Hero render via `render-pipelines/blender/render_smoke.py`. |
| `volumetric-grid/lattice-boltzmann/` | lattice-boltzmann | C | Sim-spec stub |
| `particle-fluids/sph-water/` | sph-water | C | Sim-spec stub; flagship sim — likely Alembic consumer; **Phase 5 candidate** |
| `particle-fluids/pic-flip/` | pic-flip | C | Sim-spec stub |
| `hybrid-particle-grid/mpm-multimaterial/` | mpm-multimaterial | C or D | Sim-spec stub |
| `quantum/ising-dwave/` | ising-dwave | special | Eventually D-Wave-backed; details deferred |

---

## 7. Conventions

### File layout per Stack B sim (when added)

```
<category>/<sim>/
├── web/
│   ├── package.json          name: @gpusims/<sim>
│   ├── tsconfig.json
│   ├── vite.config.ts        uses wgslPlugin from @gpusims/common-web
│   ├── index.html
│   ├── src/
│   │   └── main.ts
│   └── shaders/*.wgsl
├── shadertoy/                only when sim has a Stack A artifact (A → B port)
│   ├── <sim>.glsl            paste-runnable on shadertoy.com
│   └── README.md             port-mapping notes (Stack A ↔ Stack B)
├── README.md                 sim-level README
└── docs/                     optional design notes
```

When the first Stack B sim ships, this auto-resolves under root `package.json`'s workspace glob `closed-form/*/web` and gets pulled into `npm install` automatically. The `shadertoy/` subfolder is only present for A→B ports (Phase 4's mandelbulb-explorer is the canonical example; Phase 5's reaction-diffusion-2d reuses this layout).

**Stack-B-only sims** (origin Stack B research with no clean Shadertoy expression) ship with `web/`, `docs/`, and a sim-level `README.md` only — no `shadertoy/` subdirectory. First instance: physarum (Phase 6 — Shadertoy can't host the 10M-agent compute pipeline). Future Stack B-originated sims (boids-3d, neural-CA web variant, lenia-fft web variant) inherit this shape.

### Canvas + DPR (Stack B portfolio sims)

Stack B sim canvases (every sim except the hello-world infrastructure example) fill the browser viewport (`width: 100vw; height: 100vh`) and render at `devicePixelRatio` scaling, capped at 2.0. The internal render-target dimensions are `clamp(devicePixelRatio, 1, 2) × cssWidth/cssHeight`. The cap exists because pixel density beyond DPR 2 is invisible at typical viewing distance, while accumulation/bloom textures grow quadratically with DPR.

Hello-world (`common/common-web/examples/hello/`) is exempt: it stays at fixed 1280×720 because its job is infrastructure verification, not presentation.

### File layout per Stack C sim (when added)

```
<category>/<sim>/
├── CMakeLists.txt            target name: <sim>
├── src/
│   └── main.cpp
├── shaders/*.glsl
├── README.md
└── docs/
```

Per-sim `CMakeLists.txt` does `target_link_libraries(<sim> PRIVATE gpusims::common_cpp)`. The top-level `CMakeLists.txt` has commented `add_subdirectory` stubs at the agreed insertion point; uncomment as sims are added.

### Shader & pipeline interfaces

Both stacks follow the same conceptual lifecycle:

1. Create `Context` (one per sim, lifetime-bound to the canvas/window).
2. Create resources (buffers, textures/images).
3. Create pipelines via `ComputePipeline.create(...)` / `GraphicsPipeline.create(...)` (`common-cpp`) / `RenderPipeline.create(...)` (`common-web`). Pipelines own their bind-group / descriptor-set layout.
4. Wire hot-reload: `hotReloader.watch(shader_path, callback)` where the callback invokes `pipeline.reload(...)`.
5. Per frame: `renderer.beginFrame()` → record commands → `renderer.endFrame(...)`.

### State capture schema

JSON `state.json` written by both stacks has the same shape:

```json
{
  "frame": 42,
  "meta": { "camera": {...}, "frame_time_ms": 16.7 },
  "buffers": [
    { "name": "particles", "file": "particles.bin", "bytes": 12345678, "count": 1000000, "stride": 16 }
  ]
}
```

Native side: `capture_NNNN/state.json` plus per-buffer `<name>.bin` files in the same directory.
Web side: ZIP archive named `capture_NNNN.zip` containing `capture_NNNN/state.json` + `<name>.bin` files.

The web `StateWriter`'s root parameter (`new StateWriter('captures')`) controls the in-archive directory name but **not** the download filename — the downloaded ZIP is always named `capture_NNNN.zip`. Per-sim READMEs should state this accurately.

### Stack B descriptor construction (TypeScript strict mode)

WebGPU descriptor objects often have optional fields. Under `exactOptionalPropertyTypes: true`, `{label: cond ? 'foo' : undefined}` is rejected because `undefined` isn't assignable to `string`. Use the conditional-assignment pattern instead:

```ts
const desc: GPUBufferDescriptor = { size, usage };
if (label) desc.label = label;
return device.createBuffer(desc);
```

This is the convention every Stack B file in `common-web/` follows; per-sim code should match it.

### Stack B closure narrowing (TypeScript strict mode)

After the WebGPU support guard `if (!canvas) throw`, the type of `canvas` is narrowed from `HTMLCanvasElement | null` to `HTMLCanvasElement`. The narrowing is preserved into closures **only if the closure is declared as a `const` arrow expression**:

```ts
const onPointerMove = (e: PointerEvent): void => {
    const rect = canvas.getBoundingClientRect();  // narrowed: ok
};
```

A `function` declaration loses the narrowing — TS treats the body as reachable from the top of the enclosing scope (declarations hoist; expressions don't):

```ts
function onPointerMove(e: PointerEvent): void {
    const rect = canvas.getBoundingClientRect();  // error TS18047: 'canvas' is possibly 'null'
}
```

Caught by `npm run typecheck --workspaces` post-Phase-5 ship; fixed at `e1f0673`. Mandelbulb (Phase 4) didn't surface it — no nested-function closures referencing `canvas`. RD-2D (Phase 5) was the first sim to add user-driven event handlers dispatching into a shared compute kernel via a closure capturing the narrowed `canvas`. Future Stack B sims with multiple event handlers (physarum, boids-3d, neural-CA, lenia-fft) should declare all such handlers as `const handler = (...) => { ... }` by default.

### Stack B WGSL UV/NDC convention

Stack B's shared `fullscreen.vert.wgsl` uses UVs `(0,1), (2,1), (0,-1)` so `uv = (0, 0)` is at canvas TOP — matches WebGPU's framebuffer-y origin and texture-sample convention. Render-then-textureSample round-trips preserve orientation without an explicit flip. Strange-attractors (Phase 2) and mandelbulb-explorer (Phase 4) both use this layout. Hello-world (Phase 1.5) uses the opposite convention `(0,0), (2,0), (0,2)` because it's a standalone infrastructure example and the convention question doesn't bite there.

For sims that treat `uv` as a screen coord (origin-bottom, graphics-math convention) when constructing ray directions or NDC math, the fragment shader must explicitly invert: `ndc.y = 1.0 - uv.y * 2.0`. Mandelbulb-explorer's raymarch frag does this; the tonemap frag (a pure texture-blit) doesn't need to.

### Stack B workspace name `-web` suffix

Every Stack B workspace's `package.json` `name` field uses the `@gpusims/<sim-name>-web` shape: `@gpusims/hello-web`, `@gpusims/mandelbulb-explorer-web`, `@gpusims/strange-attractors-web`, `@gpusims/physarum-web`, `@gpusims/reaction-diffusion-2d-web`, `@gpusims/boids-3d-web`. The `-web` suffix distinguishes the Stack B (browser) workspace from a future Stack C native binary or Stack D Python module of the same sim name.

Phase 7 originally shipped boids-3d's workspace as `@gpusims/boids-3d` (without suffix); the post-execution typecheck-fix patch renamed to `@gpusims/boids-3d-web`. Future sim specs should specify the workspace name with `-web` suffix everywhere it appears: `package.json` `name`, `--workspace=` references in build/dev scripts, README build instructions, deploy workflow workspace references.

### Stack B `tsconfig.json` shape parity

All Stack B per-sim workspaces should use a `tsconfig.json` that matches the `compilerOptions` shape of the most recently shipped sim's tsconfig (currently `agent-based/physarum/web/tsconfig.json`). Strict-mode flags should be the same across workspaces — divergence on flags like `noUncheckedIndexedAccess`, `exactOptionalPropertyTypes`, etc. produces spurious "failures" in newer sims when they enable flags older sims didn't, surfacing pre-existing common-web defects as if they were new-sim issues.

Phase 7's boids-3d shipped with `noUncheckedIndexedAccess: true` (and several other strict flags physarum doesn't have), which exposed 42 pre-existing strict-mode gaps in `common-web/src/gpuProfiler.ts` and 1 in `common-web/src/stateReader.ts`. The Phase 7 patch aligned boids-3d's tsconfig to physarum's shape rather than fixing common-web — fixing common-web is the right move for a dedicated hardening phase (see § 9).

Future Stack B sim specs should either inline the full `tsconfig.json` content or direct Claude Code to "copy `agent-based/physarum/web/tsconfig.json` byte-for-byte" rather than relying on Claude Code's default tsconfig generation, which produces stricter-than-codebase defaults. The longer-term right move is a shared `tsconfig.shared.json` at the repo root that every Stack B workspace `extends:` — banked as part of the common-web hardening pass in § 9.

---

### Architects read the actual hello-world source before drafting

Sim-phase specs must reference the *implemented* `common-` API surface, not an idealized one. Concretely, before drafting any per-sim phase spec the architect reads:

- For Stack B sim phases: `common/common-web/examples/hello/src/main.ts` and the modules it imports from `@gpusims/common-web`. Also read the most recently shipped Stack B sim's `main.ts` (currently `closed-form/mandelbulb-explorer/web/src/main.ts`) as a closer template — its `doSave` / `triggerFileLoad` / `applyCapture` shape is canonical for capture/load.
- For Stack C sim phases: `common/common-cpp/examples/hello/main.cpp` and the corresponding headers under `common/common-cpp/include/gpusims/`.
- For Stack D sim phases (eventually): the analogous Python hello example.
- **CI surfaces also count.** When the spec modifies `.github/workflows/*.yml`, the architect fetches the actual file contents and anchors the Python edit on verified strings, not a paraphrase from memory of prior phases. Phase 4's first execution caught two stale anchors in `deploy-pages.yml` (per-step `name:`/`run:` block shape and `cp dist/*` glob form) — Claude Code adapted on the fly, but the lesson banks here. Same drift class as the API mismatches.

This is load-bearing: Phase 3's spec was drafted against an idealized `common-cpp` API and Claude Code adapted ~5–10 sites at execution time (Window constructor signature, `gv::WindowOptions` that doesn't exist, descriptor-set construction style — fluent `ComputeBindings` vs raw `vkUpdateDescriptorSets` — profiler-scope idiom, ImGui glue function names, Camera method names, `beginRendering` vs `beginSwapchainPass`). The adaptations all landed correctly, but they cost real tokens and produced a spec → code gap that's hard to audit afterward. Phase 4's first draft drifted on three API surfaces (`Camera.lookAt` non-existence claim, `StateWriter`/`StateReader` shape, raymarch UV/NDC orientation) before architect cross-review caught them; the fix is upstream — write the spec against the real API.

The architect's pre-execution checks for any sim phase should explicitly include a `view` on the relevant hello-world's `main.{ts,cpp}` and a scan of the `common/<stack>/include/` (or `src/`) directory for the actual exported surface.

### Architect-1 cross-check against synced source between draft sections

For high-stakes phase specs (new sims, common-* changes, first-of-pattern work), architect-1 should pause between major sections of the draft to cross-check the just-written section against the actual synced source — NOT wait until the full draft is complete and route to architect-2.

The cross-check is a focused re-read pass that asks: do the API calls in this section actually match the real signatures? Did I miss a field on a descriptor? Are bind-group binding orders consistent with the corresponding shader's `@binding(N)` declarations? Are constants consistent across files? Did I just describe a struct's byte layout — does the WGSL std-uniform alignment math actually work out to that size?

Phase 7's chain caught 11 self-defects mid-draft this way that would otherwise have been architect-2 round-trips: camera-driving-compute walkback, render-pass-ordering drift, integrate-pipeline read_write binding (WebGPU validation hazard), `RenderPipeline.handle` getter usage (replaced a type-cast through `unknown`), frame-loop `dt` arithmetic, leader mesh vertex-count mismatch (24 vs 36), cell-walk last-cell fallback (stale-data read past `entityCount`), Params struct padding (160 not 144 bytes), CPU-side sentinel-write approach (replaced a botched in-shader computation), plus three architect-2-mandated reversals applied cleanly. Each catch saved an architect-2 round-trip.

The pattern is worth keeping as standard architect-1 practice. Cost: ~5 minutes of pause per section. Savings: one architect-2 round-trip per defect caught (~20–30 minutes each, including the user's bridging messages).

Specifically worth re-checking after writing each section:

- After API-binding-heavy sections (bind groups, pipeline creation): re-read every shader's `@binding(N)` block in BOTH stages where applicable. Bind-group `visibility` flags must cover every stage where the binding is referenced. Phase 7's render-pipeline-validation bug shipped to runtime because the camera uniform was marked `VERTEX` only when fragment shaders also read it.
- After uniform-struct definitions: walk the WGSL std-uniform alignment math (vec3 → 16-byte align, total struct size rounds up to outermost alignment). Compare with CPU-side `PARAMS_BYTES` constant.
- After ping-pong / read-write-discipline kernels: trace the bind-group selection logic against the documented invariant in a comment. Phase 7's mental walk-through of the ping-pong flip caught one wrong selection in `doSave`.
- After file-manifest writes: grep for any reference to a file by path and confirm the spec creates that file at that path with the expected shape.

### Architect-2 deep-review checklist additions

When reviewing a render or compute pipeline's `bindings:` array, check the actual `@binding(N)` declarations and their uses in BOTH stages of the corresponding shader file — not just the primary stage. Common drift: a uniform packed with vertex-stage values (e.g., `viewProj`) plus fragment-stage values (e.g., `lightDir`, `ambient`) is marked `visibility: GPUShaderStage.VERTEX` in the bind-group layout because the mental model treats it as a vertex resource. The pipeline-creation validator rejects this — the fragment shader's `@group(0) @binding(0)` reference fails layout validation.

Phase 7 surfaced this with the boid + leader render pipelines: camera uniform's binding-0 was marked `VERTEX` only, but the fragment shaders read `camera.lightDir` and `camera.ambient` for Lambert lighting. The pipeline failed validation; the render pass produced silent black canvas. Caught only at runtime by checking the browser WebGPU validation log. Architect-2 review should treat bind-group visibility as a checklist item: for each binding, grep for its declared use in vertex + fragment shaders (or all compute entry points) and confirm visibility flag covers every site.

### Volumetric-density-budget alongside neighborhood-budget (crowd-dynamics sims)

For sims with crowd dynamics (Reynolds boids, flock formation, particle aggregation), the neighborhood-budget calculation (radii ≤ cell size for the 27-cell walk to be correct) is necessary but not sufficient for a visually compelling demo. A second calculation must also pass:

**Inter-entity distance at rest > separation radius.** Where inter-entity distance ≈ `(volume / entity_count) ^ (1/3)` for 3D, or `√(area / entity_count)` for 2D. If this fails, every entity's neighborhood overlaps every other neighborhood, and Reynolds-style rules cannot produce visible structure — the scene reads as uniform noise rather than discrete flocks.

Phase 7's boids-3d originally specified `BOX_HALF_EXTENT = 16` (32u edge) with 50k boids default tier. Volumetric density: 1.5 boids/u³. Inter-boid distance: ~0.88u. Separation radius: 0.5u. Ratio: 1.7×. Failed the density budget — flocks couldn't form. Polish pass at `cda37d3` scaled box to 40u (80u edge) and dropped default tier to 10k: density 0.02 boids/u³, inter-boid distance ~3.7u, separation radius ~1.0u. Ratio: 3.7×. Visible flock structure.

Future crowd-dynamics sim specs should include both budgets in their dimensional-analysis block. This is most relevant for: future boids variants, particle-swarm sims, sph-water (if discrete particles visible), lattice-boltzmann (if discrete-particle visualized rather than density-field rendered).

### Pearson's-law-of-three for common-* promotion

Patterns identified as "this might want to be in common-*" during a sim phase do not promote on the first consumer. They promote at the THIRD consumer where the abstraction's shape is empirically validated by repeated use rather than speculatively designed. The first two consumers keep per-sim copies of the pattern; the third's spec includes the promotion review.

Currently tracked at this stage:

- **Point-emitter / sparse-source pattern.** Consumers: physarum food pins (#1), boids-3d leader attractors (#2). **Note: eulerian-smoke (Phase 8) was evaluated and explicitly did NOT count as consumer #3** — its emitter pattern is volumetric source injection, a distinct shape (see phase8_eulerian_smoke.md § 2.4). Promotion review scheduled at the next actual massless-attractor consumer.

- **Volumetric source-injection emitter pattern.** Consumer #1: eulerian-smoke (Phase 8) — multi-channel sparse user-placed sources (LMB-place, cap 8) injecting density + temperature + velocity bias into a 3D Eulerian grid. Likely consumers #2 and #3: sph-water, mpm-multimaterial. Promotion review at consumer #3.

- **Tier-1 / Tier-2 framing.** "Tier-2 flagship" = sims in the overarching-spec's "A100 hero" catalog category (eulerian-smoke, sph-water, mpm-multimaterial, LBM, retroactively reaction-diffusion-3d). "Tier-1 warm-up" = the Stack B WebGPU sims (closed-form, agent-based, 2D continuous CA). This framing is banked from Phase 8's coordinator-Q6 answer; future architects can reference it without re-deriving.
- **3D depth-attachment in `Renderer.beginRendering`.** Consumers: mandelbulb-explorer (#1, raymarcher with depth-disabled), boids-3d (#2, rasterization with depth). Promotion review at consumer #3 (likely eulerian-smoke 3D smoke render, or sph-water particle render).
- **Click-to-place ground-plane unproject (`unprojectToGroundPlane`).** Consumer #1: boids-3d. Wait for consumers #2 and #3 before promoting.
- **xorshift32 RNG helpers.** Per-sim copies in physarum, boids-3d. Promotion considered at consumer #3.
- **Spatial-hash counting-sort (cell_count + multi-block prefix scan + scatter).** Consumer #1: boids-3d. Likely future consumers: sph-water for SPH neighbor queries, lattice-boltzmann if discrete-particle visualized, future flocking variants. Promotion review at consumer #3.
- **3D entity-buffer ping-pong with read-old-write-new invariant.** Consumer #1: boids-3d. Likely consumer #2: sph-water particle update. Consumer #3 TBD.

The pattern is informal currently; this entry formalizes it. Per-sim phases should call out which existing patterns they reuse (and which, if any, they promote at consumer #3).

### Multi-architect cross-review for high-stakes phase specs

For phases that touch wide common-* API surface area, lock load-bearing decisions, or establish first-of-pattern conventions, the spec benefits from a cross-review pass before going to Claude Code. The pattern, banked from Phase 4 (canonical example — first Stack A→B port, established the convention):

1. **Architect-1 (drafter)** reads project-state.md, the most recent phase spec, and the actual `common-*` source for any API the new sim consumes. Drafts the spec to a single `.md` file. Flags load-bearing decisions explicitly. Notes anything they're uncertain about.
2. **Architect-2 (reviewer, fresh chat)** reads the draft + the actual `common-*` source independently — does NOT take the spec's word for any API surface. Looks for: API drift (do the spec's `main.{ts,cpp}` calls match real signatures?), shader correctness (sign/orientation bugs, dimensional inconsistencies), schema drift (state.json field names consistent across phases), CI-surface drift (do the workflow YAML anchors match the synced file?), and root-surface drift (README gallery row, CHANGELOG entry). Reports findings as: blocking issues (must fix), minor flagged items (architect-1's choice to defer or fold in), and approved items (sign-off on architect-1's flagged decisions).
3. **Architect-1 (revisions)** applies the fixes. Sends the revised spec back for a second pass.
4. **Architect-2 (approval)** signs off or flags one more round.
5. Spec goes to Claude Code. Claude Code's completion report flags any execution-time spec defects (e.g., stale CI YAML anchors that even the reviewer missed) for future banking.

Phase 4's chain caught three blocking issues in round 1 (Camera.lookAt drift, WGSL UV/NDC inversion bug, full StateWriter/Reader API mismatch) plus two architect-3 self-catches during revision (GpuProfiler API drift, JsonValue/JsonObject import gaps), plus one real omission in round 2 (root README.md gallery row missing), plus two non-blocking polish flags (auto-morph continuity math, trap-coloring dimensional inconsistency). Each round-1 finding was a real ship-stopper that single-architect drafting missed. End-to-end overhead: ~3-4 conversational turns per architect, ~8-12 turns total including the user's bridging messages.

When it's worth the overhead:

- New Stack B/C/D sim phases (touch wide API surface area).
- Phases that introduce or modify common-* APIs.
- First-of-pattern phases (the first port, the first compute-light render-only sim, etc.).

When single-architect is fine:

- Hardening passes (Phase 3.5 was one architect, no review).
- Small additions to existing infrastructure.
- Documentation-only commits.
- The ledger-pointer chore commit at the end of every sim phase.

Within new-sim phases, first-of-its-kind vs copy-the-template:

The "is it worth the overhead" question has a finer-grained answer once you're inside a new-sim phase. For first-of-its-kind sim phases — new solver family, new render pattern, new common-* surface area, first-of-pattern decisions to lock — the full architect-1 → architect-2 → coordinator chain consistently catches blocking issues that a single architect-2 round misses. For copy-the-template sim phases — second instance of an established pattern, sim that inherits cleanly from an existing one with the same Stack and common-* shape — a single architect-2 round suffices and the additional chain overhead doesn't pay back. Phases 7 (boids-3d, first crowd-dynamics Stack B sim) and 8 (eulerian-smoke, first Tier-2 flagship Stack C) both demonstrated the value of the full chain — each surfaced multiple round-1 blocking issues that one architect alone missed. Future Stack C sims that inherit the eulerian-smoke template (sph-water, mpm-multimaterial, LBM) likely fall on the copy-the-template side of the heuristic.

The reviewer should be a fresh chat (not a continuation of the drafter's session) to avoid context bleed; each architect in the chain having loaded only the file artifacts and project-state.md is what makes them catch each other's blind spots.

### Phase ledger row positioning

The § 3 ledger has speculative future rows below the most-recent shipped row (e.g., a TBD row, `common-py + first-d-sim`, `Remaining sims`). When a phase ships, its row must INSERT immediately after the previous shipped row, and any speculative rows below shift their numbers by one. A literal append at the table's bottom produces a duplicate row number — the Phase 5 spec anchored on `| 4 | mandelbulb-explorer |` and instructed "append after," which would have produced two rows numbered 5; Claude Code adapted by inserting + renumbering speculative rows to 6 / 7 / 8+.

Phase specs writing § 3 ledger edits should make positioning explicit: anchor on the previous shipped row, instruct INSERT (not append), and call out the renumbering of any speculative rows below the insertion point. Where a spec is ambiguous on this, treat row position as intent-preserving rather than literal byte-edit. Banked so future specs include explicit positioning instructions.

---

### Periodic hardening pass

Every 3–4 sim phases, accumulated drift in the visible surface earns its own small commit before becoming embarrassing. Recurring drift sites:

- Root `README.md` gallery row: status column ("Not started"), broken path links to relocated sims, missing live URLs for shipped sims. (Phase 4 caught this preemptively in cross-review and added the row update to the phase itself; future phases should follow that pattern, treating README updates as part of the ship rather than deferring to a later hardening pass.)
- `.github/workflows/structure.yml`: `required_dirs` entries for directories that have moved or been deleted.
- `CHANGELOG.md`: shipped phases not yet entered (Keep-A-Changelog format). Phase 4 also added the CHANGELOG entry inline rather than deferring; same pattern.
- Per-sim docs: dead schema fields, "to be measured post-build" placeholders that have real numbers, stale path references after a sim's canonical home shifts.

The hardening pass is not a phase-ledger row — it's a `chore:` commit between sim phases. Phase 3.5 (commit family `d8ab610..3de7cc5`) is the canonical example: README gallery fix, `structure.yml` stale entry, CHANGELOG backfill (4 entries), dead `windowFullscreen` capture-schema field, plus a small markdown-lint + lychee config follow-up to bring all three CI workflows green simultaneously for the first time since Phase 0.

Don't pre-schedule these; trigger them when drift becomes visible. A short repo-architect chat is enough — they don't need cross-review.

---

### Anchor-grep verification for cross-cutting modifications

When a spec § 5.x section calls for an anchor-based Python edit (find old block, replace with new block), the spec author AND the reviewing architect must `grep` the synced source for the literal anchor text and verify byte-for-byte that the anchor matches. The recurring failure mode is the anchor being paraphrased from memory and drifting from the on-disk reality — Phase 4 surfaced 2 anchor-mismatch defects in cross-cutting mods, Phase 8 surfaced 3 more (CMakeLists block shape, README gallery row cell count, phase ledger column count). The convention is: anchor strings are grep-verified against the synced file before the spec is locked, not after Claude Code reports the mismatch.

### Count-consistency self-check across spec sections

When a fact appears in multiple spec sections — file counts, pipeline counts, shader counts, dispatch counts, watch counts, ledger column counts — pick one occurrence as canonical and re-derive the others, or `grep` all occurrences after each edit and reconcile. Phase 8 surfaced 5+ off-by-one drift items between § 0 preamble, § 3 manifest intro, § 6 verification block comments and echoes, completion-report file counts, and inline comments inside `main.cpp`. Count drift is recurring and silent — none of these were caught by readability review because every individual section reads cleanly in isolation. The discipline is to treat counts as cross-sectional invariants and verify them as a final pass.

### Verification-block grep patterns need anchor discipline

The shell `grep` invocations in spec § 6 verification blocks should use the same anchor patterns as Python edit anchors — i.e., line-start anchors (`^if(...)`), not substring matches. Phase 8 surfaced an unanchored `grep "if(GPU_SIMS_BUILD_VOLUMETRIC_GRID)"` that matched the commented-out lattice-boltzmann line (which the spec itself preserved as commented), causing the verification step to falsely report success. Fix forward: every verification grep pattern is anchored at line start, and where the pattern could match a commented-out line, the negation `grep -v '^#'` (or equivalent) is appended.

### "Identity for clarity" comments are red flags, not calming signals

Round-1 architect-2 review of Phase 8 caught a `project_p` parity bug where the spec contained a comment claiming "identity; explicit for clarity" — the reassuring comment masked an actual inverted formula and was the reason architect-1's self-review skipped past it. The convention is: when reviewing, treat self-justifying comments ("trivially", "by definition", "for clarity", "obviously", "always") with extra scrutiny rather than as skip-signals. They are statistically more likely to be wrong than the same code with no comment, because writers reach for them when their own confidence is shaky.

### Single-source-of-truth for preset / config / parameter tables

When a parameter set appears in multiple places — narrative description table + struct initializer + sim-specs file + completion report — pick one as canonical and have the others reference it; never duplicate the values across sources. Phase 8's Chimney-Down catch (round-1 architect-2 Flag B) was caused by three diverging sources for the same six preset values: each looked internally consistent and only cross-section diff exposed the drift. Same drift class as count-consistency, but specific to per-row tabular data where the columns are easy to misalign during copy-paste. Convention banked: pick one source, reference from others, never duplicate.

### API-grep convention for first-of-its-kind sim phases

For any spec section that calls into common-cpp APIs (`gpusims::` or `gv::`), the spec author must include the synced API signature as a literal quote with file:line reference rather than paraphrasing from memory. Reviewer verification then includes a `grep` of each quoted API against the synced header to confirm the signature matches. Phase 8 surfaced 12 API-drift defects at Claude Code build time because the spec asserted API signatures from memory rather than from grep — every one of them should have been caught at architect-1 self-review with a one-minute grep. This is the single highest-value retro item from Phase 8 and extends the existing "Architects read the actual hello-world source before drafting" convention with concrete per-API verification discipline.

### Architect-1 fabrication pattern (general)

When drafting spec content, prompts to Claude Code, or any architect-2 review request, architect-1 must never assert specific numeric values, struct field names, function signatures, file:line references, or process patterns from memory. Either quote the file with a verifiable reference, or `grep` before asserting. The pattern surfaced four times in Phase 8: (a) 12 API signatures fabricated in the `main.cpp` draft, surfaced at build time; (b) 3 fabricated spec assertions in the Chimney-Down investigation prompt (densityRate=2.0 vs actual 4.0, T_ambient=0.5 vs actual 0.0, shader formula form), surfaced by Claude Code reading the actual files; (c) a misremembered git `--amend` process pattern in the commit prompt that would have shipped a broken SHA pointer if Claude Code hadn't paused for confirmation; (d) confident assertions about present-mode being the heat source without measurement, retracted after `radeontop` data. The convention is the meta-rule: if I would say it in a tone of confident recall, I should grep first. This subsumes several Phase 7/8 conventions and is the single root-cause to internalize.

### Per-preset behavior cross-grep when spec has a parameter table

When a spec defines a per-preset parameter table AND a per-preset behavior elsewhere (emitter placement, initial conditions, camera angle, color ramp selection, etc.), the reviewer must cross-grep the preset name against ALL spec sections to find every preset-conditional behavior site. Phase 8's round-1 review caught the parameter-table values for Chimney-Down but missed that the emitter-placement code did not branch on preset, leaving Chimney-Down's downward buoyancy misapplied at the floor — a defect that survived two architect reviews and was only caught at user-driven visual verification post-ship. Banked as a reviewer-side convention: enumerate every preset's name, then grep each against the full spec for behavior-site mentions, and verify each site either applies uniformly or branches on the preset index correctly.

### Shader-copy destination must namespace by sim

Per-sim `CMakeLists.txt` files that copy `shaders/*.glsl` to `${CMAKE_BINARY_DIR}/bin/` must use a sim-namespaced destination (e.g., `bin/eulerian-smoke/shaders/`) to avoid Ninja "multiple rules generate" collisions across sims with shared shader filenames. The collision is silent until two sims with a common shader name (`fullscreen.vert.glsl` is the canonical case) are built in the same tree. RD-3D's `CMakeLists.txt` used the un-namespaced form and shipped fine because it was the only Stack C consumer at the time; eulerian-smoke (Phase 8) surfaced the collision at build configure time. Rule-of-three candidate: at the next Stack C sim, promote the shader-copy boilerplate to a `common-cpp` CMake helper that namespaces automatically rather than relying on per-sim discipline.

### Commit-SHA back-fill must use a separate follow-up commit, not `git commit --amend`

The back-fill writes the previous commit's SHA into `project-state.md`'s § 3 ledger row and the corresponding date into `CHANGELOG.md`. If the back-fill is folded into the original commit via `git commit --amend`, the commit's contents change, the SHA changes, and the back-filled SHA reference becomes an orphan pointing at a commit that no longer exists in history. Phase 7 handled this correctly with a separate follow-up commit (`5f8fb19 Phase 7 retro: project-state.md commit-SHA backfill`); Phase 8's commit prompt reinvented the broken `--amend` pattern from memory, and Claude Code caught the structural flaw mid-sequence and refused to push. Convention banked: the back-fill is always a follow-up commit referencing the previous commit's stable SHA, never an amend. Same architect-1 fabrication failure mode as the general convention above — process patterns should be grep'd from `git log` precedent, not asserted from memory.

---

## 8. Things explicitly deferred

These come up periodically in design discussions; the locked answer is "later".

- **Multi-GPU.** Vulkan supports it; Stack C's `Context` opens one device. Multi-GPU happens when a sim genuinely needs it (e.g., training a denoiser concurrently with simulation).
- **Headless render-only mode.** Stack C `Context` has the option to disable swapchain (`enable_swapchain=false`); not used yet. Lands when a sim wants to dump a video without opening a window.
- **Cross-stack capture replay.** Schema is shared; the runtime path that loads a Stack B capture into a Stack C sim isn't implemented. Door is open; not walking through it yet.
- **Toast UI for hot-reload events on Stack B.** Stack C has it (ImGui). Stack B logs to console only — toast HTML overlay is a polish pass.
- **Per-canvas multi-sim composition.** One canvas, one sim. Picture-in-picture or side-by-side comparison is out of scope.
- **Mobile / touch.** Both stacks open windows/canvases at fixed resolutions and assume keyboard + mouse input.
- **Mandelbulb-explorer v1.1 polish.** Higher default `iterCap` (≥12 at n=8) for richer per-pixel orbit-trap variegation; smoothstep ramp with adjustable `trapLo`/`trapHi` bounds in place of the linear `clamp(0, 1)`; perceptual gamma on the trap ramp. Notes captured in `closed-form/mandelbulb-explorer/docs/notes.md`. Triggered when v1.1 polish becomes worth it; not before.

---

## 9. Known issues

Tracked here so future architect chats and per-sim implementers don't waste time re-diagnosing them.

### CI baseline (positive)

As of commit `3de7cc5` (post-Phase-3.5 + markdown-lint config), all three repo-level CI workflows are green simultaneously on `main` for the first time since Phase 0:

- `Markdown` → `Lint markdown` (markdownlint-cli2, configured by `.markdownlint.json` to relax style rules that fight with the portfolio's prose conventions).
- `Markdown` → `Check internal links` (lychee, configured by `lychee.toml` to skip `docs/sim-specs/_template.md` which contains placeholder URLs).
- `Structure` (required-directories check; `volumetric-grid/reaction-diffusion-3d` was dropped from `required_dirs` in Phase 3.5 since the sim's canonical home moved to `continuous-ca/`).

A red badge on `main` after this commit is a real regression, not pre-existing breakage. The `Build (native)` and `Build (web)` workflows run only on path triggers (touching `common/common-cpp/**`, `common/common-web/**`, root `CMakeLists.txt`, or `package.json`), so they don't fire on every push — to verify them after a non-build-touching change, dispatch them manually via the Actions tab.

`Pages build and deployment` runs on every push to `main` (no `paths:` filter); each sim phase ship re-runs it (~37 seconds, idempotent). Phase 4's ship deployed cleanly; the ledger-pointer chore re-triggered it as a no-op.

### Stack B (common-web)

- **`GpuProfiler` readback path triggers "Buffer is already mapped".** The current implementation calls `mapAsync` on a readback buffer in the same encoder cycle that targets it via `copyBufferToBuffer`, which WebGPU rejects. State-machine guard mitigates the warning spam but doesn't fix the underlying architectural issue. **Workaround:** `Context.create` does NOT auto-request `timestamp-query` anymore; sims must opt in via `optionalFeatures: ['timestamp-query']` if they want it. The hello-world bypasses by not requesting the feature; profiler degrades to CPU-only timing. Phase 4's mandelbulb-explorer goes one step further and doesn't instantiate a `GpuProfiler` at all (matches strange-attractors precedent — neither imports it). **Proper fix (deferred):** rework `GpuProfiler` to use a small pool of unmapped readback buffers with a "pending readback" queue, decoupling readback from `beginFrame`. Owner: whoever first needs precise GPU timings on Stack B (likely the first compute-heavy Stack B sim).

- **Firefox on Linux requires `dom.webgpu.enabled` flag.** Default-off as of May 2026. Chromium/Chrome on Linux works out of the box. Not a code fix — environmental. Document in per-sim READMEs.

- **lil-gui sliders don't auto-update when bound values change externally.** The Camera's `position`/`yawDeg`/`pitchDeg` change during free-fly motion (WASD + RMB) but the displayed numbers stay frozen until the user drags a slider. Same shape applies to any controller bound to runtime state mutated by preset selection, capture load, or any non-slider input. Cosmetic only — the underlying state is actually changing. **Resolution shipped Phase 5:** call `panel.refreshDisplays()` after externally mutating bound state. The method walks every controller under the panel via lil-gui's `controllersRecursive()` and calls `updateDisplay()` on each. Logs first failure per call via `log.warn` rather than silent swallow. Phase 4 sidestepped this for the n-power slider by defaulting `autoMorphEnabled` to OFF (so the slider stays the source of truth).

- **`common-web` `Camera.projection()` Y-flip — resolved Phase 7.** The `out[5] *= -1` line at `common/common-web/src/camera.ts:163` was a Vulkan-idiom Y-flip mistakenly applied to a WebGPU pipeline (Vulkan clip-space Y points down, WebGPU clip-space Y points up — matching OpenGL and gl-matrix's `mat4.perspectiveZO` default). The flip silently inverted world-Y in every Stack B render since common-web shipped. Strange-attractors and mandelbulb masked the artifact (Lorenz/Rössler attractor silhouettes have no canonical lobes-up vs lobes-down prior; n=8 mandelbulb has radial symmetry around the origin). Phase 7's boids-3d would have been the first sim whose user-facing scene has a clear "up" via gravity-intuition + ground-plane leader placement — would have visibly exhibited the bug on active camera exploration. Caught during architect-2 cross-review of the Phase 7 spec. Fix: deleted the flip line, rewrote the misleading doc comment (which had claimed "Vulkan and WebGPU share clip-space convention" — wrong twice, since they don't share it and the flip introduces the problem rather than fixing it). Verified post-fix that strange-attractors and mandelbulb still render correctly — they pass because neither has canonical orientation. Banked principle: bugs surfaced during sim phases get fixed in the sim phase that surfaces them, because there is no later phase where the fix is a better fit.

- **`common-web` strict-mode gaps — banked Phase 7.** `common-web/src/gpuProfiler.ts` (42 sites) and `common-web/src/stateReader.ts` (1 site) fail typecheck under `noUncheckedIndexedAccess: true` and related strict flags. Boids-3d's spec inadvertently enabled stricter flags than the rest of the codebase via a default-generated `tsconfig.json`, surfacing these. The Phase 7 polish workaround aligned boids-3d's tsconfig to physarum's less-strict shape rather than fixing common-web. **Proper fix (deferred):** a dedicated common-web hardening phase that (a) fixes the 43 sites, (b) enables `noUncheckedIndexedAccess: true` across all Stack B workspaces via a shared `tsconfig.shared.json` referenced via `extends:` from each workspace, (c) formalizes the shared tsconfig in § 7 (replaces the current "copy physarum's shape" workaround). Worth doing after Stack C/D bootstrap so the hardening pass also covers any common-web changes those phases introduce.

- **lil-gui `persistKey` + preset-dropdown interaction (Stack B).** `ParamPanel({ persistKey: '<sim-name>' })` saves all slider values to localStorage on change and restores them on page load. Combined with the convention that "preset dropdown setValue snaps multiple sliders at once," this produces a subtle UX bug: page-load restores the user's last slider values BEFORE the preset dropdown's setValue callback can apply defaults. If the user previously dragged sliders to non-preset values, the preset name in the dropdown reads (e.g.) "Cohesive Flock" but the sliders show the user's old values. Visible in Phase 7's boids-3d: max-speed reading 0.5 with leader-strength 0 despite "Cohesive Flock" selected, producing visually-frozen demo on reload. Workaround in user-facing docs: README instructs to clear localStorage if the demo looks wrong on first load. **Proper fix (deferred):** in `ParamPanel` constructor, after loading from localStorage, re-run the preset's setValue callback (or equivalent "initial state") to ensure defaults always win on load. Owner: whoever next touches `common-web/src/paramPanel.ts`. Banked as v1.1 polish for affected sims rather than blocking.

### Stack C (common-cpp)

- **`choosePresentMode()` hardcoded to MAILBOX-preferred — surfaced Phase 8.** `choosePresentMode()` in `common/common-cpp/src/vk/window.cpp` selects `VK_PRESENT_MODE_MAILBOX_KHR` > `IMMEDIATE` > `FIFO` unconditionally, with no caller control. The effect is that any Stack C sim runs uncapped and pegs the GPU at 100% utilization. Eulerian-smoke (Phase 8) is the first Stack C sim with a heavy enough per-frame workload for this to surface as audible thermal load on the dev hardware (RX 6800 XT fans pegged under sustained compute). RD-3D (Phase 3) was light enough that the issue wasn't noticed. **Workaround:** launch with `vblank_mode=3 ./build/bin/<sim>` to force FIFO at the Mesa driver layer. The eulerian-smoke README already documents this for the user. **Proper fix (deferred):** common-cpp API amendment exposing `VkPresentModeKHR` at `Window` construction (and a `recreateSwapchain(VkPresentModeKHR)` overload for runtime VSync toggling), with FIFO as the new default. Owner: a candidate Phase 8.5 mini-phase ("common-cpp: VSync support"), or folded into a broader common-cpp hardening phase that also covers OpenVDB CI enablement and the shader-copy CMake helper banked in § 7.

---

## 10. Onboarding prompts

For convenience: prompts to give a fresh chat in each role. The user maintains a Claude project with `project-state.md` and the relevant phase specs as project sources; new chats can search those directly.

### Repo-architect chat (drafts the next phase spec)

```
You are the repo architect for GPU-Sims (github.com/StevenFAU/GPU-Sims).
Read project-state.md from project sources and the most recent shipped
phase spec (e.g., phase4_mandelbulb_explorer.md) before answering. The
repo's authoritative state is in the code; project-state.md is the
cross-cutting context.

Your role: draft the next phase's instruction document for Claude Code to
execute. See § 3 of project-state.md for what's next. Make load-bearing
decisions; flag and pause rather than guess. The phase spec output is a
single .md file the user drops into Claude Code.

Critical: read the actual common-* source for any API the new sim
consumes (per § 7's read-actual-source rule). The spec is not authoritative;
the synced source is. Phase 4's first draft drifted on three API surfaces
before architect cross-review caught them — don't repeat that.

**Project sources (in this Claude project) contain phase specs and meta docs
(project-state.md, overarching-spec.md, root-context-distilled.md) but NOT
the synced repo source. Clone <https://github.com/StevenFAU/GPU-Sims.git>
directly via the bash tool to read the actual common-* surface — the
read-actual-source rule (§ 7) is load-bearing. Banked in Phase 7's review
chain after every architect-1 chat had to rediscover this.**

For high-stakes phases, cross-check against synced source BETWEEN major
sections of the draft, not just at the end. Phase 7's chain caught 11
mid-draft defects this way that would otherwise have been architect-2
round-trips. See § 7 "Architect-1 cross-check against synced source
between draft sections" for the specific check-types per section type.

For high-stakes phases (new sims, common-* changes, first-of-pattern
work), expect the user will route the spec through a reviewer-architect
chat before sending to Claude Code; draft accordingly.

Begin by confirming you've read the relevant docs and ask any clarifying
questions before drafting.
```

### Reviewer-architect chat (cross-reviews a phase spec)

```
You are reviewing a phase spec for GPU-Sims (github.com/StevenFAU/GPU-Sims).
Another architect has drafted the spec; your job is to catch drift between
the draft and the actual repo state before it goes to Claude Code.

Read project-state.md (especially § 4 locked decisions, § 7 conventions —
including the read-actual-source rule and the multi-architect cross-review
pattern, and § 9 known issues). Then independently read the actual common-*
source the spec touches — do NOT take the spec's word for any API surface.

For Stack B specs:
- common/common-web/src/index.ts (barrel) and the modules it imports.
- The most recently shipped Stack B sim's main.ts as a closer template
  (currently closed-form/mandelbulb-explorer/web/src/main.ts).

For Stack C specs:
- common/common-cpp/examples/hello/main.cpp and the corresponding headers
  under common/common-cpp/include/gpusims/.
- The most recently shipped Stack C sim's main.cpp as a closer template
  (currently continuous-ca/reaction-diffusion-3d/src/main.cpp).

Look for, in priority order:
- API drift: spec calls methods that don't exist or have wrong signatures.
- Shader correctness: sign/orientation bugs (especially WGSL UV/NDC math
  per § 7), dimensional inconsistencies in coloring or DE math.
- Schema drift: state.json field names consistent across phases.
- CI surface drift: deploy-pages.yml / structure.yml anchors must match
  the synced file shape, not paraphrases. (Phase 4 caught two stale
  anchors here.)
- Root-surface drift: README.md gallery row update, CHANGELOG.md entry
  prepend — must be in the modified-files list, not deferred.

Report findings as: blocking issues (must fix before Claude Code), minor
flagged items (architect-1's choice to defer or fold in), and approved
items (sign-off on architect-1's flagged decisions). Be specific about
which file-line of synced source contradicts which line of the draft.

After architect-1 revises, do a second pass focused on the rewritten
sections. The other fixes are typically surgical and don't need much
follow-up review.

Begin by confirming the surfaces you're checking against and ask any
clarifying questions before reading the draft.
```

### Category-architect chat (owns one category branch)

```
You are the category architect for the <category> branch of GPU-Sims
(github.com/StevenFAU/GPU-Sims). Read project-state.md, then read the
relevant per-sim README and docs/sim-specs/<sim>.md for the category.

Your role: own the category-level architecture and review per-sim
implementation plans within this category. The cross-stack decisions in
project-state.md are settled — work within them.

Begin by confirming the category's locked architectural decisions from
project-state.md and ask any category-scoped clarifying questions before
proceeding.
```

### Per-sim implementer chat (builds one sim)

```
You are implementing the <sim> simulation for GPU-Sims
(github.com/StevenFAU/GPU-Sims). Read project-state.md (especially § 4
locked decisions, § 7 conventions, § 9 known issues), the README at
<category>/<sim>/, and the spec at docs/sim-specs/<sim>.md.

Your role: produce a phase-spec instruction document for Claude Code to
materialize this sim. Consume the relevant common- package as a black box;
don't redesign cross-stack abstractions.

Confirm you've read the docs and ask any sim-specific clarifying questions
before drafting.
```

### Coordinator chat (project management)

```
You are the project coordinator for GPU-Sims
(github.com/StevenFAU/GPU-Sims). Read project-state.md.

Your role: track which phase is current, what's blocked, what's next. You
don't write phase specs (that's the architect's job). You decide what
phase to work on next, hand briefs to architects, receive their phase specs,
and pass them to Claude Code for execution. Update project-state.md's § 3
phase ledger when phases ship.

For high-stakes phases (new sims, common-* changes, first-of-pattern
work), route the architect-1 draft through a reviewer-architect chat
before sending to Claude Code (see § 7's multi-architect cross-review
pattern). For hardening passes and small additions, single-architect
drafting is fine.

Begin by summarizing the current state and what's next.
```

---

## 11. Quick reference

- Repo: <https://github.com/StevenFAU/GPU-Sims>
- License: MIT
- Latest commit: `c36c731` — Phase 6 retro: tier-normalized deposit + project-state fill (on top of `1250971`, Phase 6 main).
- Live sims:
  - <https://stevenfau.github.io/GPU-Sims/strange-attractors/> (Phase 2)
  - <https://stevenfau.github.io/GPU-Sims/mandelbulb-explorer/> (Phase 4)
  - <https://stevenfau.github.io/GPU-Sims/reaction-diffusion-2d/> (Phase 5)
  - <https://stevenfau.github.io/GPU-Sims/physarum/> (Phase 6)
- Stack C build: `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPU_SIMS_BUILD_EXAMPLES=ON && cmake --build build`
- Stack C hello-world binary: `./build/bin/gpu_sims_hello`
- Stack B install: `npm install` from repo root (Node 22+ required)
- Stack B hello-world: `npm run dev:hello-web` then http://127.0.0.1:5173 in a WebGPU-enabled browser
- Stack B build: `npm run typecheck && npm run build:web`
- Per-sim Vite dev ports: hello-web 5173, strange-attractors 5174, mandelbulb-explorer 5175, reaction-diffusion-2d 5176, physarum 5177, next sim 5178
- Ubuntu deps for Stack C: see `common/common-cpp/README.md`
