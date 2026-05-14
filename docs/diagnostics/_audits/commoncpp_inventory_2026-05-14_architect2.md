---
title: Common-cpp Structural Inventory — Layer 2 audit, Probe 1
date: 2026-05-14
author: architect2
layer: 2
scope: common/common-cpp/ — public-header surface inventory and Phase-11-in-flight attribution
status: probe (inventory) — read-only
sibling-layers:
  - Layer 1: particle-fluids/sph-water/ (in progress; see phase11_5_* reports)
  - Layer 3: per-sim triage at docs/diagnostics/_audits/sims_prioritization_2026-05-14_triage.md
out-of-scope:
  - per-class implementation deep audits (deferred to later Layer 2 probes)
  - cross-sim consumer mapping — which sims use which common-cpp APIs (deferred to later Layer 2 probes)
  - behavioral audit / API doc-vs-actual-impl comparison (deferred to later Layer 2 probes)
  - any code modification
  - anything in particle-fluids/sph-water/ (Layer 1 territory)
cross_workstream: layer-1
---

> First Layer-2 audit catalog. Establishes the inventory baseline for `common/common-cpp/` so that future common-cpp surface changes — particularly the async-readback or sparse-readback helper Layer 1 will need for commit 8 (convergence-check infrastructure) — land against an audited surface rather than an undocumented one. Section G surfaces the async-readback feasibility question. Section P carries two cross-workstream flags for Layer 1 (the gpu_profiler timestamp bug and the subgroup-size-control completion fix).

## Section A: File-tree inventory

The full common-cpp directory layout. Every file is enumerated; no hidden directories.

### A.1 Directory structure

```
common/common-cpp/
├── CMakeLists.txt
├── cmake/
│   ├── deps.cmake
│   ├── imgui.cmake
│   ├── optional_deps.cmake
│   └── vma.cmake
├── examples/
│   └── hello/
│       ├── CMakeLists.txt
│       ├── main.cpp
│       └── shaders/
│           ├── fullscreen.frag.glsl
│           ├── fullscreen.vert.glsl
│           ├── gradient.comp.glsl
│           └── trivial.comp.glsl
├── include/
│   └── gpusims/
│       ├── alembic_writer.hpp
│       ├── camera.hpp
│       ├── gpu_profiler.hpp
│       ├── hot_reload.hpp
│       ├── imgui_setup.hpp
│       ├── log.hpp
│       ├── state_reader.hpp
│       ├── state_writer.hpp
│       ├── vdb_writer.hpp
│       └── vk/
│           ├── buffer.hpp
│           ├── compute_pipeline.hpp
│           ├── context.hpp
│           ├── debug.hpp
│           ├── frame.hpp
│           ├── graphics_pipeline.hpp
│           ├── image.hpp
│           ├── renderer.hpp
│           ├── shader_compiler.hpp
│           └── window.hpp
└── src/
    ├── alembic_writer.cpp
    ├── camera.cpp
    ├── gpu_profiler.cpp
    ├── hot_reload.cpp
    ├── imgui_setup.cpp
    ├── log.cpp
    ├── state_reader.cpp
    ├── state_writer.cpp
    ├── vdb_writer.cpp
    └── vk/
        ├── buffer.cpp
        ├── compute_pipeline.cpp
        ├── context.cpp
        ├── debug.cpp
        ├── frame.cpp
        ├── graphics_pipeline.cpp
        ├── image.cpp
        ├── renderer.cpp
        ├── shader_compiler.cpp
        └── window.cpp
```

### A.2 Line counts (header surface)

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
19 public headers, 1,432 lines total. **`include/gpusims/` is fully PUBLIC** per `CMakeLists.txt:213-215` — there is no public/internal header distinction.

| Header | Lines |
|---|---:|
| `include/gpusims/alembic_writer.hpp` | 47 |
| `include/gpusims/camera.hpp` | 132 |
| `include/gpusims/gpu_profiler.hpp` | 120 |
| `include/gpusims/hot_reload.hpp` | 100 |
| `include/gpusims/imgui_setup.hpp` | 72 |
| `include/gpusims/log.hpp` | 37 |
| `include/gpusims/state_reader.hpp` | 54 |
| `include/gpusims/state_writer.hpp` | 58 |
| `include/gpusims/vdb_writer.hpp` | 53 |
| `include/gpusims/vk/buffer.hpp` | 72 |
| `include/gpusims/vk/compute_pipeline.hpp` | 111 |
| `include/gpusims/vk/context.hpp` | 130 |
| `include/gpusims/vk/debug.hpp` | 38 |
| `include/gpusims/vk/frame.hpp` | 60 |
| `include/gpusims/vk/graphics_pipeline.hpp` | 106 |
| `include/gpusims/vk/image.hpp` | 83 |
| `include/gpusims/vk/renderer.hpp` | 70 |
| `include/gpusims/vk/shader_compiler.hpp` | 66 |
| `include/gpusims/vk/window.hpp` | 83 |
| **Subtotal (headers)** | **1,492** |

### A.3 Line counts (implementation)

19 implementation files, exact 1:1 mirror of the headers (same names, same vk/ subdirectory split). 3,389 lines total.

| Source file | Lines |
|---|---:|
| `src/alembic_writer.cpp` | 117 |
| `src/camera.cpp` | 216 |
| `src/gpu_profiler.cpp` | 190 |
| `src/hot_reload.cpp` | 200 |
| `src/imgui_setup.cpp` | 187 |
| `src/log.cpp` | 31 |
| `src/state_reader.cpp` | 85 |
| `src/state_writer.cpp` | 87 |
| `src/vdb_writer.cpp` | 158 |
| `src/vk/buffer.cpp` | 164 |
| `src/vk/compute_pipeline.cpp` | 271 |
| `src/vk/context.cpp` | 407 |
| `src/vk/debug.cpp` | 117 |
| `src/vk/frame.cpp` | 86 |
| `src/vk/graphics_pipeline.cpp` | 349 |
| `src/vk/image.cpp` | 246 |
| `src/vk/renderer.cpp` | 131 |
| `src/vk/shader_compiler.cpp` | 177 |
| `src/vk/window.cpp` | 220 |
| `examples/hello/main.cpp` | 490 |
| **Subtotal (impl + example)** | **3,929** |
| **Total (whole tree)** | **5,421** |

### A.4 Header / implementation pairing

Every public header has a matching `src/` implementation; every implementation file has a matching header. No orphans in either direction. `frame.hpp` is the only header containing both a `struct` (`Frame`) and three free functions (`initFrame`, `destroyFrame`, `memoryBarrier`) — the others are predominantly class- or struct-oriented.

## Section B: Public symbol catalog

Per-header enumeration. Each entry gives the file location and a one-line description; full signatures are at the cited `file:line`. Listed in the order headers appear in the include tree.

### B.1 `gpusims/alembic_writer.hpp` (47 lines)

Namespace `gpusims::abc`. Optional feature gated on `GPU_SIMS_HAVE_ALEMBIC` at compile time.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct | `ParticleFrame` (5 fields: positions, velocities, radii, ids, count) | `alembic_writer.hpp:21` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `ParticleWriter` (abstract base; `create()` factory, virtual `writeFrame()`) | `alembic_writer.hpp:31` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `bool isAvailable()` | `alembic_writer.hpp:44` |

Header docblock at lines 11-16 still describes this as a Phase-1 stub. **Description is stale**; see Section H.1.

### B.2 `gpusims/camera.hpp` (132 lines)

Namespace `gpusims`. No optional gating.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct | `CameraInputState` (14 fields: keys, mouse buttons, deltas, scroll) | `camera.hpp:12` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| enum | `Camera::Mode { FreeFly, Arcball, Orbit }` | `camera.hpp:35` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `Camera` (mode, transforms, lens params, free-fly/arcball/orbit tuning, ImGui inspector, JSON serialization) | `camera.hpp:33` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void setMode(Mode)` | `camera.hpp:46` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void update(float dt, const CameraInputState&)` | `camera.hpp:50` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `glm::mat4 view() / projection() / viewProjection()` | `camera.hpp:55-57` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `glm::vec3 position() / forward() / right() / up()` | `camera.hpp:59-62` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void setFovDeg / setAspect / setNearFar / fovDeg / aspect` | `camera.hpp:67-71` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void setMoveSpeed / setLookSpeed / setBoostMultiplier` | `camera.hpp:76-78` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void setTarget / setOrbitDistance / setOrbitSpeed / resetArcball` | `camera.hpp:83-86` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void setPosition / setOrientation` | `camera.hpp:91-92` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void drawImGui(const char* label = "Camera")` | `camera.hpp:97` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void toJson(nlohmann::json&) const / fromJson(...)` | `camera.hpp:102-103` |

### B.3 `gpusims/gpu_profiler.hpp` (120 lines)

Namespace `gpusims`. Has uncommitted modification (see Section F.1).

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| constant | `inline constexpr std::uint32_t kMaxFramesInFlight = 2` | `gpu_profiler.hpp:16` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| forward decl | `namespace vk { class Context; }` | `gpu_profiler.hpp:18-20` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| constant | `static constexpr std::uint32_t GpuProfiler::kMaxPasses = 256` (working tree; HEAD has 64 — see F.1) | `gpu_profiler.hpp:47` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `GpuProfiler` (RAII scopes, ring-buffered timestamp queries, ImGui overlay, CSV dump) | `gpu_profiler.hpp:45` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| inner class | `GpuProfiler::Scope` (RAII pass timing) | `gpu_profiler.hpp:60` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct | `GpuProfiler::PassResult` (name, cpu_ms, gpu_ms) | `gpu_profiler.hpp:78` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `beginFrame / endFrame / scope / lastResults / drawImGui / appendCsv` | `gpu_profiler.hpp:56-92` |

### B.4 `gpusims/hot_reload.hpp` (100 lines)

Namespace `gpusims`. No optional gating.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `HotReloader` (file-watcher with include-graph awareness, save-during-write retry) | `hot_reload.hpp:33` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| type alias | `using Callback = std::function<void(const std::filesystem::path&)>` | `hot_reload.hpp:35` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| inner struct | `HotReloader::Event` (path, ok, message, time_point) | `hot_reload.hpp:62` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `watch / unwatch / poll / watchCount / recentEvents / reportSuccess / reportFailure` | `hot_reload.hpp:48-73` |

### B.5 `gpusims/imgui_setup.hpp` (72 lines)

Namespace `gpusims::ui`. Uses forward-declared `GLFWwindow*` to avoid pulling GLFW into the header.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct | `ImGuiInit` (10 fields: GLFW window + Vulkan handles + image counts) | `imgui_setup.hpp:30` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `bool initImGui(const ImGuiInit&)` | `imgui_setup.hpp:47` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `void newImGuiFrame()` | `imgui_setup.hpp:51` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `void renderImGui(VkCommandBuffer)` | `imgui_setup.hpp:55` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `void shutdownImGui()` | `imgui_setup.hpp:58` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `VkDescriptorPool createImGuiDescriptorPool(VkDevice)` | `imgui_setup.hpp:62` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `void pushToast(const char* text, bool success, float lifetime_seconds = 3.0f)` | `imgui_setup.hpp:66` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `void drawToasts()` | `imgui_setup.hpp:69` |

Ownership semantics of `ImGuiInit::descriptor_pool` documented inconsistently across struct-field and function-doc comments — see Section H.2.

### B.6 `gpusims/log.hpp` (37 lines)

Namespace `gpusims`. Thin templated wrapper around spdlog.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `void initLogger()` | `log.hpp:13` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| template fn | `logTrace<Args...>(spdlog::format_string_t<Args...>, Args&&...)` | `log.hpp:17` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| template fn | `logDebug<Args...>(...)` | `log.hpp:21` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| template fn | `logInfo<Args...>(...)` | `log.hpp:25` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| template fn | `logWarn<Args...>(...)` | `log.hpp:29` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| template fn | `logError<Args...>(...)` (does not abort or throw — routes to `spdlog::error` only) | `log.hpp:33` |

### B.7 `gpusims/state_reader.hpp` (54 lines)

Namespace `gpusims`. Load side of the capture format.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `StateReader` (open existing capture, query meta/buffer, find latest by NNNN suffix) | `state_reader.hpp:23` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| static fn | `std::optional<StateReader> open(const path&)` | `state_reader.hpp:25` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| static fn | `std::optional<path> findLatest(const path&)` | `state_reader.hpp:30` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `meta / bufferMeta / buffer / frameIndex / dir` | `state_reader.hpp:34-44` |

### B.8 `gpusims/state_writer.hpp` (58 lines)

Namespace `gpusims`. Write side; pairs with `StateReader`.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `StateWriter` (capture-directory-per-frame; JSON meta + binary blobs) | `state_writer.hpp:24` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `StateWriter(path root_dir)` constructor | `state_writer.hpp:26` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `beginFrame / setMeta / saveBuffer / endFrame / currentDir` | `state_writer.hpp:29-49` |

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The format docblock at `state_writer.hpp:17-23` documents Python-side compatibility — JSON + `.bin` blobs readable via `numpy.fromfile`.

### B.9 `gpusims/vdb_writer.hpp` (53 lines)

Namespace `gpusims::vdb`. Optional feature gated on `GPU_SIMS_HAVE_OPENVDB`.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `bool writeFloatGrid(path, const float*, glm::ivec3 dims, voxel_size, origin, grid_name)` | `vdb_writer.hpp:25` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `bool writeVec3Grid(path, const float*, glm::ivec3 dims, voxel_size, origin, grid_name)` | `vdb_writer.hpp:33` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `bool writeFloatFrame(base, frame_idx, ...)` (sequence helper) | `vdb_writer.hpp:41` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `bool isAvailable()` | `vdb_writer.hpp:50` |

Header docblock at lines 11-15 describes this as a Phase-1 stub. **Description is stale** — VDB writer was first-exercised in Phase 8 (eulerian-smoke); see Section H.1.

### B.10 `gpusims/vk/buffer.hpp` (72 lines)

Namespace `gpusims::vk`. Forward-declares VMA: `typedef struct VmaAllocation_T* VmaAllocation`.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| enum | `MemoryUsage { DeviceLocal, HostVisibleSequential, HostVisibleRandom }` | `buffer.hpp:15` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `Buffer` (RAII; move-only) | `buffer.hpp:21` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| static fn | `Buffer::create(Context&, size, VkBufferUsageFlags, MemoryUsage, debug_name)` | `buffer.hpp:23` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `handle / allocation / sizeBytes / mapped` (accessors) | `buffer.hpp:38-43` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void uploadDirect(const void* src, size, offset)` (host-visible only) | `buffer.hpp:47` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void stage(Context&, const void* src, size, offset)` (synchronous host→device for DeviceLocal) | `buffer.hpp:52` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void readback(Context&, void* dst, size, offset)` (synchronous device→host; **Phase 11 in-flight addition — see Section E.2**) | `buffer.hpp:59` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `VkDeviceAddress deviceAddress(VkDevice)` | `buffer.hpp:62` |

### B.11 `gpusims/vk/compute_pipeline.hpp` (111 lines)

Namespace `gpusims::vk`. Includes the load-bearing subgroup-size-control INVARIANT comment.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct | `DescriptorBinding` (binding, type, count, stages) | `compute_pipeline.hpp:24` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct | `ComputePipelineDesc` (shader_path, bindings, push_constant_size, +Phase-11 fields) | `compute_pipeline.hpp:31` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct field | `ComputePipelineDesc::required_subgroup_size` (default 0 = unconstrained; **Phase 11 in-flight addition**) | `compute_pipeline.hpp:51` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct field | `ComputePipelineDesc::require_full_subgroups` (default false; **Phase 11**) | `compute_pipeline.hpp:52` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `ComputePipeline` (move-only, RAII) | `compute_pipeline.hpp:55` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| static fn | `ComputePipeline::create(Context&, ShaderCompiler&, const ComputePipelineDesc&)` | `compute_pipeline.hpp:57` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `bool reload(Context&, ShaderCompiler&, Frame& current_frame, std::string* out_error)` | `compute_pipeline.hpp:75` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `VkDescriptorSet allocateDescriptorSet()` | `compute_pipeline.hpp:81` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void dispatch(cmd, ds, gx, gy, gz, push_constants, push_size)` (combined bind+dispatch) | `compute_pipeline.hpp:85` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `handle / pipelineLayout / descriptorSetLayout / shaderPath / includes` | `compute_pipeline.hpp:94-98` |

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
**Load-bearing invariant** documented at `compute_pipeline.hpp:40-50`:

```
INVARIANT (must be preserved by all future maintainers):
  If required_subgroup_size != 0 OR require_full_subgroups == true,
  compute_pipeline.cpp builds VkPipelineShaderStageRequiredSubgroupSize
  CreateInfo and chains it into VkPipelineShaderStageCreateInfo.pNext
  (and/or sets the REQUIRE_FULL_SUBGROUPS_BIT flag). Otherwise pNext
  stays null and the flag stays zero.

  Collapsing the conditional (always building the extension struct
  regardless of values) would force every compute pipeline to carry the
  subgroup-size extension, breaking compatibility with drivers/devices
  that don't support VK_EXT_subgroup_size_control.
```

This is structural documentation — refactoring this code without preserving the conditional would silently break consumers running on hardware without `VK_EXT_subgroup_size_control`. Catalogued explicitly because the kind of invariant most likely to be lost in a future refactor is the kind documented only in code comments.

### B.12 `gpusims/vk/context.hpp` (130 lines)

Namespace `gpusims::vk`. Forward-declares VMA allocator. Has uncommitted modification in `.cpp` (Section F.3).

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct | `ContextCreateInfo` (application_name, version, extra extensions, require_discrete_gpu, enable_swapchain, +Phase-11 subgroup field) | `context.hpp:24` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct field | `ContextCreateInfo::enable_subgroup_size_control` (default false; **Phase 11 in-flight addition — see E.1**) | `context.hpp:46` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `Context` (instance + debug messenger + physical device + device + queues + VmaAllocator) | `context.hpp:49` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| constructor | `Context()` (zero-config) | `context.hpp:51` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| constructor | `explicit Context(const ContextCreateInfo&)` | `context.hpp:52` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| accessor | `instance / physicalDevice / device / graphicsQueue / computeQueue / graphicsQueueFamily / computeQueueFamily / allocator` | `context.hpp:61-68` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| accessor | `deviceProperties / memoryProperties` | `context.hpp:72-73` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| accessor | `subgroupSizeMin / subgroupSizeMax / requiredSubgroupSizeStages / subgroupSizeControlEnabled` (**Phase 11**) | `context.hpp:84-87` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void waitIdle() const` | `context.hpp:91` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void runOneShot(const std::function<void(VkCommandBuffer)>&)` (transient command buffer; underpins synchronous helpers) | `context.hpp:95` |

### B.13 `gpusims/vk/debug.hpp` (38 lines)

Namespace `gpusims::vk`. Compile-time-gated by `GPU_SIMS_VALIDATION_LAYERS`; all functions are no-ops when validation is disabled.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| constant | `constexpr const char* kValidationLayerName = "VK_LAYER_KHRONOS_validation"` | `debug.hpp:15` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `bool checkValidationLayerSupport()` | `debug.hpp:18` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `const char* const* requiredDebugExtensions(uint32_t* out_count)` | `debug.hpp:22` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT&)` | `debug.hpp:25` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `VkResult createDebugMessenger(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT&, VkDebugUtilsMessengerEXT*)` | `debug.hpp:28` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `void destroyDebugMessenger(VkInstance, VkDebugUtilsMessengerEXT)` | `debug.hpp:32` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `void setObjectName(VkDevice, VkObjectType, uint64_t handle, const char* name)` | `debug.hpp:36` |

### B.14 `gpusims/vk/frame.hpp` (60 lines)

Namespace `gpusims::vk`. Unusual in that it exposes a `struct` with public data members alongside three free functions.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct | `Frame` (per-in-flight-frame state: indices, fence, semaphores, command pool/buffer, deletion queue) | `frame.hpp:15` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| inline method | `Frame::flushDeletions()` (called by Renderer at frame begin once fence has signaled) | `frame.hpp:35-40` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `void initFrame(Context&, Frame&, uint32_t in_flight_index)` | `frame.hpp:44` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `void destroyFrame(Context&, Frame&)` | `frame.hpp:47` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| free fn | `void memoryBarrier(VkCommandBuffer, VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access, VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access)` | `frame.hpp:54` |

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The `memoryBarrier` free function is the foundation of `cs_barrier` and equivalent helpers across the sim portfolio — single global `VkMemoryBarrier2` via `vkCmdPipelineBarrier2`. Documented at `frame.hpp:49-53` as "Global rather than per-image is correct when all of the application's resources move together; the over-broad scope costs nothing in practice for typical per-sim workloads."

### B.15 `gpusims/vk/graphics_pipeline.hpp` (106 lines)

Namespace `gpusims::vk`. Uses dynamic rendering (`VK_KHR_dynamic_rendering`; core in Vulkan 1.3).

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct | `GraphicsPipelineDesc` (vertex+fragment shader paths, bindings, push constant size, color/depth formats, topology, polygon, cull, front-face, depth test/write, blend) | `graphics_pipeline.hpp:23` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct field | `src_color_blend_factor` (default `VK_BLEND_FACTOR_SRC_ALPHA`; **Phase 11 in-flight addition — see E.2**) | `graphics_pipeline.hpp:48` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct field | `dst_color_blend_factor` (default `VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA`; **Phase 11**) | `graphics_pipeline.hpp:49` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct field | `color_blend_op` (default `VK_BLEND_OP_ADD`; **Phase 11**) | `graphics_pipeline.hpp:50` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct field | `src_alpha_blend_factor` (default `VK_BLEND_FACTOR_ONE`; **Phase 11**) | `graphics_pipeline.hpp:51` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct field | `dst_alpha_blend_factor` (default `VK_BLEND_FACTOR_ZERO`; **Phase 11**) | `graphics_pipeline.hpp:52` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct field | `alpha_blend_op` (default `VK_BLEND_OP_ADD`; **Phase 11**) | `graphics_pipeline.hpp:53` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct field | `vertex_bindings / vertex_attributes` (default empty) | `graphics_pipeline.hpp:57-58` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `GraphicsPipeline` (move-only, RAII) | `graphics_pipeline.hpp:61` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| static fn | `GraphicsPipeline::create(Context&, ShaderCompiler&, const GraphicsPipelineDesc&)` | `graphics_pipeline.hpp:63` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `bool reload(Context&, ShaderCompiler&, Frame&, std::string*)` | `graphics_pipeline.hpp:76` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `VkDescriptorSet allocateDescriptorSet()` | `graphics_pipeline.hpp:81` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void bind(cmd, ds, push_constants, push_size)` (separate from draw) | `graphics_pipeline.hpp:84` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| accessor | `handle / pipelineLayout / descriptorSetLayout / includes` | `graphics_pipeline.hpp:89-92` |

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Comment at `graphics_pipeline.hpp:43-47` documents the additive-blend opt-in path for Phase 11's thickness pass.

### B.16 `gpusims/vk/image.hpp` (83 lines)

Namespace `gpusims::vk`. Forward-declares VMA allocation.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| enum | `ImageType { e2D, e3D }` | `image.hpp:15` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct | `ImageCreateInfo` (type, extent, format, mip_levels, array_layers, samples, usage, initial_layout, debug_name) | `image.hpp:20` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `Image` (RAII, move-only) | `image.hpp:32` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| static fn | `Image::create(Context&, const ImageCreateInfo&)` | `image.hpp:34` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| accessor | `handle / view / allocation / extent / format / type` | `image.hpp:45-50` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| static fn | `Image::transitionLayout(cmd, image, aspect, old_layout, new_layout, mip_levels, array_layers)` (explicit; no automatic layout tracking) | `image.hpp:55` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void upload(const void* src, size)` (synchronous host→device) | `image.hpp:67` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void readback(void* dst, size)` (synchronous device→host; not flagged as Phase-11 addition in commit log — needs blame-check) | `image.hpp:73` |

### B.17 `gpusims/vk/renderer.hpp` (70 lines)

Namespace `gpusims::vk`. Owns the per-in-flight-frame Frame slots.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `Renderer` (frame orchestration: acquire / record / submit / present) | `renderer.hpp:31` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| constructor | `Renderer(Context&, Window&)` | `renderer.hpp:33` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `Frame* beginFrame()` (returns nullptr if swapchain out-of-date) | `renderer.hpp:42` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void beginRendering(Frame&, VkClearColorValue)` (dynamic-rendering pass; defaults to dark blue-grey clear) | `renderer.hpp:46` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void endRendering(Frame&)` | `renderer.hpp:47` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void endFrame(Frame&)` | `renderer.hpp:50` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void waitIdle()` | `renderer.hpp:53` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| accessor | `ctx / window / frame(i) / framesInFlight` | `renderer.hpp:56-61` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| private field | `Frame frames_[kMaxFramesInFlight]` (fixed at 2) | `renderer.hpp:66` |

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Includes `gpu_profiler.hpp` solely for `kMaxFramesInFlight` (`renderer.hpp:8`) — coupling between profiler and renderer headers via the shared constant.

### B.18 `gpusims/vk/shader_compiler.hpp` (66 lines)

Namespace `gpusims::vk`. Uses pImpl idiom to keep shaderc out of public headers (shaderc is a `PRIVATE` CMake link dep — see Section C.2).

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| enum | `ShaderStage { Compute, Vertex, Fragment }` | `shader_compiler.hpp:21` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| struct | `CompileResult` (ok flag, spirv vector, error string, includes for hot-reload graph) | `shader_compiler.hpp:27` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `ShaderCompiler` (pImpl) | `shader_compiler.hpp:35` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| constructor | `explicit ShaderCompiler(Context&)` | `shader_compiler.hpp:37` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void addIncludeDir(path)` | `shader_compiler.hpp:44` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `CompileResult compileFile(path, ShaderStage)` | `shader_compiler.hpp:49` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `CompileResult compileSource(const std::string&, ShaderStage, path nominal)` | `shader_compiler.hpp:53` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| static fn | `VkShaderModule createShaderModule(VkDevice, const std::vector<uint32_t>& spirv)` | `shader_compiler.hpp:58` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| private | `struct Impl; Impl* impl_;` (pImpl) | `shader_compiler.hpp:62-63` |

### B.19 `gpusims/vk/window.hpp` (83 lines)

Namespace `gpusims::vk`. Forward-declares `GLFWwindow*`. Wraps `VkSurfaceKHR` and `VkSwapchainKHR`.

| Kind | Symbol | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| class | `Window` (GLFW window + surface + swapchain; transparent recreation on resize) | `window.hpp:20` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| constructor | `Window(Context&, width, height, title)` | `window.hpp:22` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `bool shouldClose() const` | `window.hpp:30` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void pollEvents() / waitEvents()` | `window.hpp:31-32` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `std::optional<uint32_t> acquireNextImage(VkSemaphore)` (nullopt = swapchain recreated; skip frame) | `window.hpp:38` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `bool present(uint32_t image_index, VkSemaphore wait)` (false = out-of-date) | `window.hpp:43` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| method | `void recreateSwapchain()` | `window.hpp:46` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| accessor | `glfwWindow / surface / swapchain / colorFormat / extent / imageCount / image(i) / imageView(i) / aspect` | `window.hpp:49-62` |

## Section C: Build configuration

### C.1 Library target

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Defined at `CMakeLists.txt:187-210`:

- **Target name:** `gpu_sims_common_cpp` (STATIC)
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- **Alias:** `gpusims::common_cpp` (`CMakeLists.txt:268`)
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- **Public include:** `${CMAKE_CURRENT_SOURCE_DIR}/include` via `BUILD_INTERFACE` generator expression (`CMakeLists.txt:213-215`). **All 19 headers under `include/gpusims/` are public.** No `INSTALL_INTERFACE` is declared — common-cpp can only be consumed as a CMake subdirectory, not installed as a standalone package.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- **Private include:** `${CMAKE_CURRENT_SOURCE_DIR}/src` (`CMakeLists.txt:218-220`) — internal `.cpp` cross-references.

### C.2 Link surface

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Public link deps (consumers inherit) at `CMakeLists.txt:225-234`:

| Target | Source |
|---|---|
| `Vulkan::Vulkan` | system (Vulkan SDK; `find_package` required) |
| `glfw` | FetchContent v3.4 |
| `glm::glm` | FetchContent v1.0.1 |
| `imgui` | custom target via `cmake/imgui.cmake` from imgui v1.91.5-docking |
| `spdlog::spdlog` | FetchContent v1.14.1 |
| `nlohmann_json::nlohmann_json` | FetchContent v3.11.3 |
| `GPUOpen::VulkanMemoryAllocator` | FetchContent vma v3.1.0 (headers only) |
| `Threads::Threads` | system |

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Private link deps at `CMakeLists.txt:237-240`:

| Target | Source |
|---|---|
| `shaderc::shaderc` | FetchContent v2024.3 (kept private via `ShaderCompiler` pImpl — see B.18) |
| `vma_impl` | custom target via `cmake/vma.cmake` (single-TU VMA implementation) |

### C.3 Optional features

| Option | Default | Compile-time define when enabled |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `GPU_SIMS_USE_OPENVDB` | OFF | `GPU_SIMS_HAVE_OPENVDB=1` (PUBLIC; `CMakeLists.txt:246-248`) |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `GPU_SIMS_USE_ALEMBIC` | OFF | `GPU_SIMS_HAVE_ALEMBIC=1` (PUBLIC; `CMakeLists.txt:250-253`) |

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Both gates check for the corresponding CMake target's presence; absent target raises a `FATAL_ERROR` with installation hint (`optional_deps.cmake:23-30` for OpenVDB, `:55-103` for Alembic). When OFF, the corresponding `.cpp` files compile to stubs that log a once-only warning and return `false` from `isAvailable()` (verified at B.1, B.9 and Section H.1).

### C.4 Other compile-time defines

| Define | Value | Set at |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `GPU_SIMS_VALIDATION_LAYERS` | 1 in Debug, 0 in Release | `CMakeLists.txt:256-259` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `GPU_SIMS_VULKAN_API_VERSION` | `VK_API_VERSION_1_3` | `CMakeLists.txt:262-264` |

### C.5 Vendored dependency anchors

All FetchContent declarations in `cmake/deps.cmake` and `cmake/optional_deps.cmake`. Pin specifics:

| Dep | Pin | Location |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| glfw | tag 3.4, GIT_SHALLOW | `deps.cmake:521-527` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| glm | tag 1.0.1, GIT_SHALLOW | `deps.cmake:535-540` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| spdlog | tag v1.14.1, GIT_SHALLOW | `deps.cmake:549-554` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| nlohmann_json | tag v3.11.3, GIT_SHALLOW | `deps.cmake:562-567` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| vma | tag v3.1.0, GIT_SHALLOW | `deps.cmake:572-577` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| shaderc | tag v2024.3, GIT_SHALLOW (special handling: `git-sync-deps` invocation, generator-expression patching of `third_party/CMakeLists.txt`) | `deps.cmake:594-672` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| imgui | tag v1.91.5-docking, GIT_SHALLOW | `deps.cmake:606-611` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| Alembic | SHA `c254caf2705ebf5271408dd37a091aa379258a38` (v1.8.10), GIT_SHALLOW | `optional_deps.cmake:463-467` |

The Alembic SHA `c254caf2...` is real and legitimately attached to Alembic v1.8.10 in this file. **See Section P.2 for a cross-workstream note about how this same SHA appears erroneously attached to a different upstream project (SPlisHSPlasH) elsewhere in the repo.**

## Section D: Vulkan API commitment

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`GPU_SIMS_VULKAN_API_VERSION=VK_API_VERSION_1_3` is set PUBLIC at `CMakeLists.txt:262-264`. The Vulkan 1.3 commitment surfaces in several places consumers should know about:

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- Dynamic rendering used directly (no `-KHR` suffix on `vkCmdBeginRendering`) — `renderer.hpp:46-47` documentation refers to core API.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- Synchronization 2 used directly — `frame.hpp:54-58` `memoryBarrier` takes `VkPipelineStageFlags2` / `VkAccessFlags2`.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- Vulkan 1.3 device feature gating via `VkPhysicalDeviceVulkan13Features` — confirmed in the Phase-11 subgroup-size-control addition at `context.cpp:289+` (see Section E.1).

No `1.4` / future-version code paths visible. Sims requiring features beyond 1.3 would need new surface.

## Section E: Phase-11 in-flight surface additions — commit attribution

The Phase 11 spec § 0 hard rule 6 authorized in-flight common-cpp additions. Per `git log --oneline -10 -- common/common-cpp/`, four Phase-11 commits touched common-cpp; this section attributes each surface addition to its specific commit.

### E.1 Commit `9e0ca2f` — subgroup-size-control surface (Wed May 13 15:13:37 2026)

> **CLAIM:** The subgroup-size-control surface addition landed in commit `9e0ca2f`.
> **VERDICT: CONFIRMED** via `git show 9e0ca2f -- common/common-cpp/` (phase 5b probe output).

Surface added:

| Where | Symbol |
|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `context.hpp:46` | `ContextCreateInfo::enable_subgroup_size_control` (default false) |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `context.hpp:84-87` | `Context::subgroupSizeMin/Max/requiredSubgroupSizeStages/Enabled` accessors |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `context.hpp:124-127` | Backing private fields `subgroup_size_min_`, `_max_`, `_stages_`, `_control_enabled_` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `compute_pipeline.hpp:51` | `ComputePipelineDesc::required_subgroup_size` (default 0) |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `compute_pipeline.hpp:52` | `ComputePipelineDesc::require_full_subgroups` (default false) |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `compute_pipeline.hpp:40-50` | The load-bearing INVARIANT comment (catalogued at B.11) |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `compute_pipeline.cpp:102-120` | pNext-chain assembly in `ComputePipeline::create` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `compute_pipeline.cpp:193-207` | pNext-chain assembly in `ComputePipeline::reload` (paired; both paths) |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `context.cpp:289-315` | Pre-flight feature-support check inside `createDevice` (fail-loud if device lacks `VkPhysicalDeviceVulkan13Features::subgroupSizeControl`) |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `context.cpp:116-138` | `VkPhysicalDeviceSubgroupSizeControlProperties` query and caching in `Context::Context` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `examples/hello/main.cpp:143-228` | `runSubgroupSizeControlSmokeTest()` reachable via `--test-subgroup-size` flag (~88 new lines) |
| `examples/hello/shaders/trivial.comp.glsl` | New 18-line shader for smoke-test pipeline creation |

Pattern observed: surface addition + hello-world smoke test landed together, in a separate commit from consumer code. The commit message explicitly names this as a banked convention: "Consumer-#1-of-new-common-cpp-surface pattern: ship in two commits (surface-addition with hello-example smoke test; consumer code). This commit demonstrates the pattern."

**However, this surface shipped incomplete.** See F.3 — `f13.computeFullSubgroups = VK_TRUE` is missing from the original commit and lives currently in the uncommitted working tree.

### E.2 Commit `1f02fc1` — DFSPH consumer + readback + blend factors (Wed May 13 16:55:47 2026)

> **CLAIM:** `Buffer::readback` and `GraphicsPipelineDesc` blend-factor fields landed in commit `1f02fc1`.
> **VERDICT: CONFIRMED** via `git show 1f02fc1 -- common/common-cpp/`.

Common-cpp surface added (diff confined to 4 files, +46 lines / −6 lines):

| Where | Symbol |
|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `buffer.hpp:54-59` | `Buffer::readback(Context&, void* dst, size_t bytes, size_t offset = 0)` declaration |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `buffer.cpp:135-155` | `Buffer::readback` implementation: allocates `HostVisibleRandom` staging buffer, `vkCmdCopyBuffer` via `ctx.runOneShot`, `vmaInvalidateAllocation`, `std::memcpy` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `graphics_pipeline.hpp:43-53` | Six new `GraphicsPipelineDesc` fields: `src_color_blend_factor`, `dst_color_blend_factor`, `color_blend_op`, `src_alpha_blend_factor`, `dst_alpha_blend_factor`, `alpha_blend_op` (defaults preserve historical hardcoded behavior) |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `graphics_pipeline.cpp:131-141` | The hardcoded blend assignments inside `buildPipeline()` rewritten to read from `desc` |

The commit message explicitly catalogues these as "In-flight common-cpp surface additions (authorized per Phase 11 spec § 0 hard rule 6)." Both follow the **sentinel-default backward-compatibility pattern** banked at `9e0ca2f`'s commit message item 2: new fields with defaults that preserve existing behavior; conditional construction fires only when non-default values are requested.

### E.3 Commit `0243278` — Alembic FetchContent rework (Wed May 13 17:11:11 2026)

> **CLAIM:** No new symbol/API surface added; build-system changes only.
> **VERDICT: CONFIRMED** — diff confined entirely to `common/common-cpp/cmake/optional_deps.cmake` (+56/−9).

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Build-config change: Alembic block reworked from `find_package(Alembic CONFIG)` against a system install to FetchContent against a pinned SHA. Driving factor documented in the commit message and embedded comment at `optional_deps.cmake:435-453`: "`libalembic-dev` was dropped from Ubuntu 24.04 noble after Ubuntu 22.04 jammy."

### E.4 Commit `596550d` — Alembic build hardening (Wed May 13 17:30:32 2026)

> **CLAIM:** Two defects fixed; one is a build-system fix, one is a 2-line `alembic_writer.cpp` source fix.
> **VERDICT: CONFIRMED** — diff confined to `optional_deps.cmake` (+13) and `alembic_writer.cpp` (+2/−2).

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Source fix: `alembic_writer.cpp:63-65` had a most-vexing-parse where `OPointsSchema::Sample sample(T1(...), T2(...))` parsed as a function declaration. Fixed by switching to brace-init. **The commit message attaches a structural-debt diagnosis worth quoting directly:**

> This bug shipped in Phase 1 ("real impl" code) but went undetected for 10 phases because GPU_SIMS_USE_ALEMBIC=OFF was the default and no sim had enabled it. Phase 11 sph-water is the first consumer; the bug surfaces in c478ccd's CI.

This is the named "Phase 11 retro doc category 7 — unexercised real-impl in synced common-cpp." See Section H.1 for the wider structural-debt implications.

## Section F: Uncommitted working-tree state

Per `git status common/common-cpp/` (probe 3d), three files are modified on top of HEAD. All three diffs are small. Two are correctness fixes to surface that landed in earlier Phase-11 commits — meaning that the relevant Phase-11 commits shipped subtly incomplete and the gap was filled in the working tree but not yet committed.

### F.1 `include/gpusims/gpu_profiler.hpp` — `kMaxPasses` 64 → 256

Single-line diff:

```diff
-    static constexpr std::uint32_t kMaxPasses = 64;
+    static constexpr std::uint32_t kMaxPasses = 256;
```

Quadruples the per-frame profiler scope capacity. Cited at probe 1 (Layer 1) Section D — Layer 1 already noticed the file was in the modified set but did not diff it.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
**Consumer impact:** `kMaxPasses` is `static constexpr` and used internally (`gpu_profiler.cpp:14` for `kQueriesPerFrame = 2 * kMaxPasses`; per-frame allocation size). The constant is not part of the consumer-facing API surface — sims do not index against it — but a sim that calls `profiler.scope(...)` more than 64 times in a frame would have hit the overflow guard at `gpu_profiler.cpp:57` (`if (f.pass_count >= kMaxPasses)` → `logWarn` and ignored pass). The bump to 256 raises that ceiling.

### F.2 `src/gpu_profiler.cpp` — `readBackResults` frame-index fix

Single-line diff inside `GpuProfiler::beginFrame`:

```diff
-    readBackResults(current_frame_idx_);
+    readBackResults((current_frame_idx_ + 1) % kMaxFramesInFlight);
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The original called `readBackResults(current_frame_idx_)` immediately before the same frame's query pool was about to be reset (next-line `vkCmdResetQueryPool` at `gpu_profiler.cpp:42-44`), which means it read either uninitialized queries (first invocation) or queries from the same frame that was about to be reset (subsequent invocations) — neither yields valid timestamps. The new code reads the *next* in-flight frame's index modulo `kMaxFramesInFlight=2`, which selects the OLDEST in-flight frame — the one whose GPU work most plausibly completed by now.

**This is a correctness fix to behavior that landed in Phase 1.** `gpu_profiler.cpp` has been emitting wrong timestamps since the initial commit `3a64055`. Any sim that consumed `GpuProfiler::lastResults()` for performance characterization was operating on bad data.

**`cross_workstream: layer-1`** — see Section P.1.

### F.3 `src/vk/context.cpp` — `f13.computeFullSubgroups = VK_TRUE`

Single-line diff inside `Context::createDevice`, immediately after the `f13.subgroupSizeControl = VK_TRUE` line:

```diff
         f13.subgroupSizeControl = VK_TRUE;
+        f13.computeFullSubgroups = VK_TRUE;
     }
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
This completes the subgroup-size-control feature shipped in `9e0ca2f`. Without `computeFullSubgroups` enabled in the Vulkan 1.3 features chain, the `VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT` flag (which `compute_pipeline.cpp` sets when `desc.require_full_subgroups = true` — see `compute_pipeline.cpp:115-117` and `:205-207`) can be rejected by some Vulkan drivers as feature-not-enabled.

**The original `9e0ca2f` commit shipped incomplete.** A consumer requesting `require_full_subgroups=true` would either succeed (if the driver was lenient) or fail with a generic Vulkan error (if strict). With the uncommitted fix landed, the feature pairing is correct.

**`cross_workstream: layer-1`** — see Section P.1.

## Section G: Async-readback feasibility for Layer-1 commit 8

> **Layer 1 commit 8 (convergence-check infrastructure) is documented as needing async or sparse readback.** Layer 2 audit-only output here; no implementation proposed. Question: does common-cpp already support this, and if not, what shape of new surface would fit?

### G.1 What exists today (synchronous readback)

Two parallel synchronous-readback paths are exposed:

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- `Buffer::readback(Context&, void* dst, size_t bytes, size_t offset = 0)` at `buffer.hpp:59` — landed in `1f02fc1` (Section E.2). Implementation at `buffer.cpp:135-155`: allocates a `HostVisibleRandom` staging buffer, does `vkCmdCopyBuffer` inside `ctx.runOneShot`, `vmaInvalidateAllocation`, `std::memcpy` to dst.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- `Image::readback(void* dst, size_t bytes)` at `image.hpp:73` — older; commit-blame not attributed by phase-5b's commit window. Header docblock states image must currently be in `VK_IMAGE_LAYOUT_GENERAL`; uses transient staging buffer; synchronous via implicit waitIdle.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Both rely on `Context::runOneShot` (`context.hpp:95`), which the header documents as "Convenience: allocate a transient command buffer, run callback, submit, **wait**, and free." The synchronous-blocking semantics flow from `runOneShot`'s wait.

### G.2 Existing precedent for async-equivalent readback

`GpuProfiler` already implements asynchronous query readback for timestamp queries. The pattern:

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- Two `VkQueryPool` slots, indexed by `frame_in_flight_idx % kMaxFramesInFlight` (`gpu_profiler.hpp:102`).
- Each frame's `beginFrame` calls `readBackResults` for *a different* in-flight slot — specifically the slot most likely to have completed (after F.2's fix, the `(current + 1) % kMaxFramesInFlight` slot).
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- Results "lag by `kMaxFramesInFlight` frames" (gpu_profiler.hpp:41-43 docblock) — that lag is invisible at 60fps and avoids pipeline stalls.

**This is a proven async-readback template.** Convergence-check infrastructure could mirror the same ring-buffered pattern: per-frame staging buffers (sized for the convergence-scalar reads), per-frame fence checks, results consumed N frames after submission. The GpuProfiler implementation in `gpu_profiler.cpp` (190 lines) is the canonical reference and is well-contained.

### G.3 Async readback for Buffer / Image — feasibility note

What async-readback surface for `Buffer` would look like (Layer 2 sketches the shape only; Layer 2 does not commit to it):

- A per-frame staging buffer pool (analogous to GpuProfiler's `pools_[kMaxFramesInFlight]`).
- A non-blocking `Buffer::readbackAsync(Context&, Frame&, void* dst, ...)` that records the copy into the frame's command buffer (not `runOneShot`), and registers a completion callback into `Frame::deletion_queue` or a parallel "readback queue" that's flushed on next-frame fence signal.
- Or a polling-handle pattern: `auto handle = buffer.readbackAsync(...)`; consumer calls `handle.poll()` each frame until the data is ready.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
These are sketches — not API proposals. The point for the inventory: **the building blocks already exist** in `Frame` (deletion queue for fence-deferred work, `gpu_profiler.hpp:42-43` already documents the lag semantics consumers must accept). Async readback is a natural extension of the existing pattern, not an architectural break.

### G.4 What this implies for the audit-baseline-before-commit-8 goal

Per the brief: "Your target: land your first audit catalog before Layer 1 reaches commit 8." This inventory is that catalog.

When Layer 1 proposes the convergence-check infrastructure's common-cpp surface additions, the diff will be against an audited baseline: this inventory documents what's already there (synchronous readback, GpuProfiler's async-equivalent ring buffer, the `Frame::deletion_queue` deferred-callback mechanism), so the proposal can be expressed as "extend X" or "mirror Y" rather than added in isolation.

## Section H: Documentation drift

### H.1 Two "stub" labels that no longer match implementation

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Both `alembic_writer.hpp:13-16` and `vdb_writer.hpp:11-15` describe themselves as Phase-1 stubs. Per phase-5c probe (head of `.cpp` files + grep for `#if GPU_SIMS_HAVE_*`):

- `alembic_writer.cpp` defines a `RealParticleWriter : public ParticleWriter` class (line 102) behind `#if GPU_SIMS_HAVE_ALEMBIC`. First consumed by Phase 11 sph-water.
- `vdb_writer.cpp` has full OpenVDB integration including `openvdb::initialize()` once-flag (line 28-32) and dense-grid write paths behind `#if GPU_SIMS_HAVE_OPENVDB`. First consumed by Phase 8 eulerian-smoke.

Both implementations have been "real" since they were written. The "stub" labels in the headers are stale.

### H.2 `ImGuiInit::descriptor_pool` ownership inconsistency

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
At `imgui_setup.hpp:37`, the field comment reads:

```
VkDescriptorPool  descriptor_pool;   // common-cpp creates and owns this
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
At `imgui_setup.hpp:44-46`, the `initImGui` function comment reads:

```
// Initialize ImGui with GLFW + Vulkan backends. Allocates a small
// descriptor pool internally if you don't pass one (descriptor_pool == VK_NULL_HANDLE).
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
At `imgui_setup.hpp:60-62`, the `createImGuiDescriptorPool` free function reads:

```
// Convenience: create + own a small descriptor pool sized for ImGui's needs.
// Returned pool must be destroyed by the caller via vkDestroyDescriptorPool.
```

Two ownership paths exist: (A) caller passes `VK_NULL_HANDLE` and common-cpp creates+owns; (B) caller uses `createImGuiDescriptorPool` to obtain one, passes it in, and is responsible for destruction. The field-level comment at line 37 documents only path A; path B is documented only via the free function. Caller of path B is the canonical owner — the line-37 claim "common-cpp creates and owns this" is wrong for that path.

### H.3 The named pattern "Phase 11 retro category 7"

Commit `596550d` names the structural shape:

> "unexercised real-impl in synced common-cpp"

The bug fixed there had shipped in Phase 1 and survived 10 phases. The driver of survival was that `GPU_SIMS_USE_ALEMBIC=OFF` was the project default, so no sim's CI exercised the path until Phase 11 turned it on.

**Open question for a future Layer-2 probe:** what other optional-feature code paths in common-cpp have not been exercised by any sim's CI? Candidate sweep: any `#if GPU_SIMS_HAVE_*`-gated block in `alembic_writer.cpp`, `vdb_writer.cpp`. Any `if (info.enable_*)` block in `context.cpp` whose flag is not flipped by any shipped sim. Probably small surface in absolute terms, but each unexercised path is a fabrication-shipped-shipped-undetected risk.

### H.4 Banked patterns surfaced in Phase 11 commit messages

Two banked decisions, captured here so a future per-class deep audit can revisit them at the documented trigger points:

| Source | Banking |
|---|---|
| `1f02fc1` commit message | "Convention 4 banking: off-screen multi-pass rendering stays sim-local. Phase 11 is consumer #1; abstraction-promotion review triggers at consumer #2 of off-screen multi-pass in Stack C." |
| `9e0ca2f` commit message | "Named-toggle pattern for Vulkan feature requests adopted at consumer #1 per rule-of-three convention. Revisit at consumer #3 of any Stack C Vulkan feature toggle — evaluate whether named-bool list still scales or whether pNext-extensibility hook is the right shape based on actual consumer surface. Plausible consumer-#3 features: shaderInt64, bufferDeviceAddress, cooperativeMatrix, hostQueryReset, shaderSubgroupExtendedTypes. The question is banked, not just the answer." |

## Section P: Incidental findings and cross-workstream flags

### P.1 Cross-workstream: Layer 1 perf data may be wrong (`cross_workstream: layer-1`)

Per F.2: `gpu_profiler.cpp::beginFrame` had a `readBackResults` frame-index bug from Phase 1 through to the current uncommitted working tree. Any Layer-1 perf characterization that consumed `GpuProfiler::lastResults()` or `profiler.scope(...)` GPU timings was reading from the wrong ring-buffer slot — likely showing zero or stale values rather than the previous frame's actual timestamps.

If Layer 1 has based any decisions on profiler-reported GPU times (e.g., whether DFSPH dispatches fit the per-substep budget), those decisions should be re-validated against profiler output once F.2's fix is committed.

### P.2 Cross-workstream: Alembic SHA reuse explained (`cross_workstream: layer-1`)

`phase11_5_setup1_2026-05-14_blocked.md` flagged that the SHA `c254caf2705ebf5271408dd37a091aa379258a38` appeared in `particle-fluids/sph-water/docs/load-bearing-decisions.md:8-9` attached to "SPlisHSPlasH 1.8.10," but no `1.8` tag exists upstream in SPlisHSPlasH (the published 1.x line ended at 1.3.1).

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
That SHA is real and correct **for Alembic v1.8.10** — it's the actual git SHA Alembic uses for that tag, and it appears legitimately in `common/common-cpp/cmake/optional_deps.cmake:465`. The fabrication in `load-bearing-decisions.md` was a cross-contamination: someone copy-pasted Alembic's (tag, SHA) pair into the SPlisHSPlasH citation slot in the sph-water doc.

This does not change Layer 1's setup-1 decision (Setup-1 picked `2.16.1` for SPlisHSPlasH and proceeded), but Layer 1 may want to update `load-bearing-decisions.md` to remove the misattributed pair. Out of Layer 2 scope to perform that edit.

### P.3 Cross-workstream: Layer-1 surface-additions ship in two commits by convention (`cross_workstream: layer-1`)

Per the `9e0ca2f` commit message: "Consumer-#1-of-new-common-cpp-surface pattern: ship in two commits (surface-addition with hello-example smoke test; consumer code). This commit demonstrates the pattern."

When Layer 1's commit 8 (convergence-check infrastructure) needs new common-cpp surface — async readback (see G.3) being the most likely shape — this convention applies. The new common-cpp surface should ship in its own commit with a hello-world smoke test, separate from the sph-water consumer code. Layer 2 does not author that surface (per the no-modification scope rule); Layer 1 does, and would file it as a banked convention-following commit.

### P.4 No `INSTALL_INTERFACE` declared in CMake

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`CMakeLists.txt:213-215` uses only `BUILD_INTERFACE`:

```
target_include_directories(gpu_sims_common_cpp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
```

Means: common-cpp is consumable as a CMake subdirectory (which is how every Stack C sim consumes it today), but cannot be installed as a standalone package. If at some point a Stack C sim were to exist outside this monorepo and want to link common-cpp, that would require new build infrastructure. No banked v1.1+ work on this currently.

### P.5 `gpu_profiler.hpp` couples Renderer to the profiler header

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`renderer.hpp:8` includes `gpusims/gpu_profiler.hpp` solely for `kMaxFramesInFlight`. The `Frame frames_[kMaxFramesInFlight]` array (`renderer.hpp:66`) and the `framesInFlight()` accessor (`renderer.hpp:61`) both depend on this. This is a tight cross-header coupling that could be refactored by hoisting `kMaxFramesInFlight` to a smaller dedicated header — but the convention "Renderer and GpuProfiler must agree on `kMaxFramesInFlight` exactly" (`gpu_profiler.hpp:15-16` comment) is structurally correct, and the present include is the simplest way to enforce it. Worth noting; not flagged as debt.

### P.6 `kMaxFramesInFlight` is fixed at 2 with no path to runtime configuration

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`gpu_profiler.hpp:16`: `inline constexpr std::uint32_t kMaxFramesInFlight = 2;` — not a CMake option, not a template parameter. Any sim that wants 3-deep pipelining would require changing this constant repo-wide. Likely intentional (single-source-of-truth) but worth catalogueing.

### P.7 Hello-world example exercises most of the public surface

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`examples/hello/main.cpp` includes 15 of the 19 public headers (verified at lines 17-31 of the example). Not exercised: `alembic_writer`, `vdb_writer` (export-only, no consumer logic in hello), `vk/debug` (internal to Context), `vk/frame` (transitively included via `vk/renderer.hpp:9`). The hello-world is therefore a non-trivial smoke-test surface — if the public API breaks in a backward-incompatible way, hello-world's build will surface it. The `9e0ca2f` smoke test pattern (hello adds a flag-gated smoke-test mode for the new surface) is well-suited to common-cpp's existing structure.

### P.8 Stack D substrate (common-py) does not exist yet

The brief mentioned Stack D (Taichi / Python) is "planned/partial." No `common/common-py/` directory exists; not surveyed by this probe. Out of Layer 2 scope.

### P.9 No TODO / FIXME / BANKED / XXX comments found in headers

`grep` for these tokens across `include/gpusims/**/*.hpp` returned no hits during phase 4 (the verbatim header reads showed no occurrence). Either the project tracks these centrally rather than inline, or common-cpp is in fact free of inline tech-debt markers at this layer. Either way, no extraction-stage analysis is blocked on lurking inline TODOs.

---

## Summary

| Section | Verdict |
|---|---|
| A. File-tree inventory | **COMPLETE.** 19 public headers, 19 implementations, 1:1 mirror. Public via CMake `BUILD_INTERFACE` only. |
| B. Symbol catalog | **COMPLETE** at signature granularity. Per-class deep audit deferred to next probe. |
| C. Build configuration | **COMPLETE.** Public link surface = 8 targets; private = 2. Two optional features (OpenVDB, Alembic); both gated and stubbed-when-off. |
| D. Vulkan API commitment | **CONFIRMED** at Vulkan 1.3 via CMake define + observed in surface (dynamic rendering, sync2, Vulkan13Features). |
| E. Phase-11 in-flight commit attribution | **CONFIRMED.** Subgroup-size surface → `9e0ca2f`; Buffer::readback + blend factors → `1f02fc1`; Alembic FetchContent → `0243278`; Alembic build hardening + most-vexing-parse fix → `596550d`. |
| F. Uncommitted state | **CATALOGUED.** Three diffs: kMaxPasses 64→256; profiler frame-index correctness fix; computeFullSubgroups feature-pairing fix. Two of the three are bug fixes to surface already committed. |
| G. Async readback feasibility | **CATALOGUED.** Synchronous readback present in both Buffer and Image. GpuProfiler provides a proven async-equivalent ring-buffer template. Frame::deletion_queue exists. Building blocks are in place for Layer 1 commit 8's new surface. |
| H. Documentation drift | **CATALOGUED.** Two stale "stub" labels (alembic, vdb); one inconsistent ownership comment (ImGuiInit::descriptor_pool); category-7 unexercised-real-impl pattern named and worth a follow-up sweep. |
| P. Incidentals | 9 entries, 3 flagged `cross_workstream: layer-1`. |

**Next Layer-2 probe proposals** (decided in conversation after this report lands, not in this section):

1. **Per-class implementation audit** — read the 19 `.cpp` files in the order: `context.cpp` (largest, has uncommitted fix), `graphics_pipeline.cpp`, `compute_pipeline.cpp`, `image.cpp`, `buffer.cpp`, then the rest. Verify completeness, document behavior vs. headers, surface implementation-level debt.
2. **Cross-sim consumer mapping** — grep for `#include <gpusims/...>` across all sim directories outside `particle-fluids/sph-water/` (which is Layer 1). For each public symbol, count consumers. Identify any symbol used differently by different sims (a smell per the brief).
3. **Unexercised-real-impl sweep** — section H.1 / H.3. Enumerate every `#if GPU_SIMS_HAVE_*` block and every `if (info.enable_*)` flag-gated path in common-cpp; cross-check which sims exercise each.

End of report.
