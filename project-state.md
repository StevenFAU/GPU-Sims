# GPU-Sims — Project State

> **Last updated:** end of Phase 1.5 (commit `6b5309a`, 2026-05-08).

> This is the **canonical handoff document** for the GPU-Sims repository. It exists so that:
>
> - A new **repo-architect chat** can take over from a fresh context, pick up where the previous architect left off, and make consistent decisions with what came before.
> - A new **category-architect chat** (responsible for, e.g., volumetric-grid sims) can scope its category-level architecture against the locked cross-stack decisions.
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
| 3 | reaction-diffusion-3d | First Stack C sim. Validates `common-cpp` at simulation scale. 256³ Gray-Scott RD on a periodic 3D grid, volume raymarch visualization, six Pearson 1993 named presets. VDB writer deferred to Phase 5 (eulerian-smoke is the natural sparse-volume consumer). | ✅ Shipped | `<COMMIT-HASH>` |
| 4 | mandelbulb-explorer | Shadertoy → WebGPU port. Establishes the Stack A→B port flow. | ⬜ Not started | — |
| 5 | flagship-cpp-sim | Either `eulerian-smoke` or `sph-water`. Adds OpenVDB or Alembic real impl to common-cpp depending on which. | ⬜ Not started | — |
| 6 | common-py + first-d-sim | `common-py` infrastructure + first Stack D sim (likely `lenia-fft` or `mpm-multimaterial`). | ⬜ Not started | — |
| 7+ | Remaining sims | One phase per remaining sim. Each consumes a settled `common-` package; per-sim phases are smaller than the foundation phases. | ⬜ Not started | — |

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

Module list: same surface as common-cpp, adapted to TypeScript and WebGPU. `ComputePipeline` / `RenderPipeline` instead of `vk::ComputePipeline` / `vk::GraphicsPipeline`. `ParamPanel` (lil-gui) instead of ImGui. `StateWriter`/`Reader` use ZIP via `fflate`. Math via gl-matrix.

Hello-world: `common/common-web/examples/hello/`. Run with `npm run dev:hello-web` from repo root; opens at http://127.0.0.1:5173.

### `common-py` (Stack D, Python/Taichi) — not yet implemented

Lands with the first Stack D sim phase.

---

## 6. Per-category status

| Category | Sims | Stack | Status |
|----------|------|-------|--------|
| `closed-form/strange-attractors/` | strange-attractors | B | Sim-spec stub committed in Phase 0; **implementation in Phase 2 (next)** |
| `closed-form/mandelbulb-explorer/` | mandelbulb-explorer | A → B | Sim-spec stub committed in Phase 0; implementation in Phase 4 |
| `agent-based/physarum/` | physarum | B | Sim-spec stub; implementation TBD |
| `agent-based/boids-3d/` | boids-3d | B | Sim-spec stub; implementation TBD |
| `continuous-ca/lenia-fft/` | lenia-fft | D | Sim-spec stub; tied to Phase 6 (common-py) |
| `continuous-ca/reaction-diffusion-3d/` | reaction-diffusion-3d | C | **Implemented (Phase 3)** |
| `continuous-ca/reaction-diffusion-2d/` | reaction-diffusion-2d | TBD | Sim-spec stub |
| `continuous-ca/neural-ca/` | neural-ca | TBD | Sim-spec stub |
| `volumetric-grid/eulerian-smoke/` | eulerian-smoke | C | Sim-spec stub; flagship sim — likely OpenVDB consumer |
| `volumetric-grid/lattice-boltzmann/` | lattice-boltzmann | C | Sim-spec stub |
| `particle-fluids/sph-water/` | sph-water | C | Sim-spec stub; flagship sim — likely Alembic consumer |
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
├── README.md                 sim-level README
└── docs/                     optional design notes
```

When the first Stack B sim ships, this auto-resolves under root `package.json`'s workspace glob `closed-form/*/web` and gets pulled into `npm install` automatically.

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

### Stack B descriptor construction (TypeScript strict mode)

WebGPU descriptor objects often have optional fields. Under `exactOptionalPropertyTypes: true`, `{label: cond ? 'foo' : undefined}` is rejected because `undefined` isn't assignable to `string`. Use the conditional-assignment pattern instead:

```ts
const desc: GPUBufferDescriptor = { size, usage };
if (label) desc.label = label;
return device.createBuffer(desc);
```

This is the convention every Stack B file in `common-web/` follows; per-sim code should match it.

---

## 8. Things explicitly deferred

These come up periodically in design discussions; the locked answer is "later".

- **Multi-GPU.** Vulkan supports it; Stack C's `Context` opens one device. Multi-GPU happens when a sim genuinely needs it (e.g., training a denoiser concurrently with simulation).
- **Headless render-only mode.** Stack C `Context` has the option to disable swapchain (`enable_swapchain=false`); not used yet. Lands when a sim wants to dump a video without opening a window.
- **Cross-stack capture replay.** Schema is shared; the runtime path that loads a Stack B capture into a Stack C sim isn't implemented. Door is open; not walking through it yet.
- **Toast UI for hot-reload events on Stack B.** Stack C has it (ImGui). Stack B logs to console only — toast HTML overlay is a polish pass.
- **Per-canvas multi-sim composition.** One canvas, one sim. Picture-in-picture or side-by-side comparison is out of scope.
- **Mobile / touch.** Both stacks open windows/canvases at fixed resolutions and assume keyboard + mouse input.

---

## 9. Known issues

Tracked here so future architect chats and per-sim implementers don't waste time re-diagnosing them.

### Stack B (common-web)

- **`GpuProfiler` readback path triggers "Buffer is already mapped".** The current implementation calls `mapAsync` on a readback buffer in the same encoder cycle that targets it via `copyBufferToBuffer`, which WebGPU rejects. State-machine guard mitigates the warning spam but doesn't fix the underlying architectural issue. **Workaround:** `Context.create` does NOT auto-request `timestamp-query` anymore; sims must opt in via `optionalFeatures: ['timestamp-query']` if they want it. The hello-world bypasses by not requesting the feature; profiler degrades to CPU-only timing. **Proper fix (deferred):** rework `GpuProfiler` to use a small pool of unmapped readback buffers with a "pending readback" queue, decoupling readback from `beginFrame`. Owner: whoever first needs precise GPU timings on Stack B (likely the first compute-heavy Stack B sim).

- **Firefox on Linux requires `dom.webgpu.enabled` flag.** Default-off as of May 2026. Chromium/Chrome on Linux works out of the box. Not a code fix — environmental. Document in per-sim READMEs.

- **lil-gui sliders don't auto-update when bound values change externally.** The Camera's `position`/`yawDeg`/`pitchDeg` change during free-fly motion (WASD + RMB) but the displayed numbers stay frozen until the user drags a slider. Cosmetic only — the camera is actually moving. Workaround if it ever matters: call `controller.updateDisplay()` once per frame on the relevant controllers.

### Stack C (common-cpp)

- None currently tracked — Phase 1 shipped clean after the eight defect-fix iterations.

---

## 10. Onboarding prompts

For convenience: prompts to give a fresh chat in each role. The user maintains a Claude project with `project-state.md` and the relevant phase specs as project sources; new chats can search those directly.

### Repo-architect chat (drafts the next phase spec)

```
You are the repo architect for GPU-Sims (github.com/StevenFAU/GPU-Sims).
Read project-state.md from project sources and the most recent shipped
phase spec (e.g., phase1_5_common.md) before answering. The repo's
authoritative state is in the code; project-state.md is the cross-cutting
context.

Your role: draft the next phase's instruction document for Claude Code to
execute. See § 3 of project-state.md for what's next. Make load-bearing
decisions; flag and pause rather than guess. The phase spec output is a
single .md file the user drops into Claude Code.

Begin by confirming you've read the relevant docs and ask any clarifying
questions before drafting.
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

Begin by summarizing the current state and what's next.
```

---

## 11. Quick reference

- Repo: <https://github.com/StevenFAU/GPU-Sims>
- License: MIT
- Latest commit: `6b5309a` (Phase 1.5 — common-web).
- Stack C build: `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPU_SIMS_BUILD_EXAMPLES=ON && cmake --build build`
- Stack C hello-world binary: `./build/bin/gpu_sims_hello`
- Stack B install: `npm install` from repo root (Node 22+ required)
- Stack B hello-world: `npm run dev:hello-web` then http://127.0.0.1:5173 in a WebGPU-enabled browser
- Stack B build: `npm run typecheck && npm run build:web`
- Ubuntu deps for Stack C: see `common/common-cpp/README.md`
