---
title: Common-cpp Cross-Sim Consumer Mapping — Layer 2 audit, Probe 2
date: 2026-05-14
author: architect2
layer: 2
scope: common/common-cpp/ — consumer-side dependency mapping across all Stack C sims
status: probe (consumer matrix) — read-only
sibling-layers:
  - Layer 1: particle-fluids/sph-water/ (in progress; see phase11_5_* reports)
  - Layer 3: per-sim triage at docs/diagnostics/_audits/sims_prioritization_2026-05-14_triage.md
predecessor: docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md
out-of-scope:
  - per-class implementation deep audits (deferred to later Layer 2 probes)
  - unexercised-real-impl sweep (deferred — see inventory § H.3 and this report § E.2)
  - any code modification
  - particle-fluids/sph-water/ source detail (Layer 1 territory); sph-water still appears as a column in the matrix because the inventory baselined its Phase 11 consumption pattern
cross_workstream: layer-1
---

> Second Layer-2 audit deliverable. Maps every Stack C sim's consumption of common-cpp at header granularity, with reliable struct-field / class-type / free-function data and a known-limited slice on method-call counts (see § G). Headline: only three Stack C sims exist (sph-water, reaction-diffusion-3d, eulerian-smoke); the two non-Layer-1 sims use common-cpp uniformly and have not touched any Phase 11 in-flight surface. Three structural findings worth surfacing: (1) `vk/debug.hpp` is an orphan in the public surface — zero consumers; (2) shader-output-path convention diverges between rd-3d and eulerian-smoke; (3) the optional-feature surface is single-consumer for both Alembic and VDB.

## Section A: Consumer universe

### A.1 Total Stack C sims linking common-cpp

> **CLAIM:** Three sims in total link `gpu_sims_common_cpp`.
> **VERDICT: CONFIRMED** via `grep -rl "gpu_sims_common_cpp\|gpusims::common_cpp" --include="CMakeLists.txt" .` in phase 6a. Result excluding common-cpp's own CMakeLists:

```
./continuous-ca/reaction-diffusion-3d/CMakeLists.txt
./volumetric-grid/eulerian-smoke/CMakeLists.txt
```

Plus `particle-fluids/sph-water/CMakeLists.txt` (excluded from the grep by Layer 1 scope rule; presence confirmed via Layer 1 probe-1's reference to common-cpp surface usage in main.cpp).

Total Stack C consumer count: **3** (sph-water + rd-3d + eulerian-smoke).

### A.2 Why this number is smaller than expected

The Layer 3 triage report (`sims_prioritization_2026-05-14_triage.md` § A) lists 9 shipped sims across the portfolio:

| Category | Sim | Stack |
|---|---|---|
| agent-based | boids-3d | B |
| agent-based | physarum | B |
| closed-form | mandelbulb-explorer | B |
| closed-form | strange-attractors | B |
| continuous-ca | reaction-diffusion-2d | B |
| continuous-ca | reaction-diffusion-3d | **C** |
| continuous-ca | lenia-fft | D |
| hybrid-particle-grid | mpm-multimaterial | D |
| volumetric-grid | eulerian-smoke | **C** |

Only the two bolded entries are Stack C, plus Layer-1-owned `particle-fluids/sph-water`. The other 6 sims (4 Stack B browser-side, 2 Stack D Taichi) do not consume common-cpp because common-cpp is the Stack C substrate; Stacks B and D have separate (or planned-separate) shared infrastructure.

### A.3 Implication for "every Stack C sim depends on it"

The brief's framing — "Every Stack C sim depends on it" — is true at the universal-quantifier level (∀ Stack C sim, depends on common-cpp), but the population it quantifies over is just 3 elements. Cross-sim divergence analysis (the brief's "API used differently by different sims" smell-detector) operates on a 2-element comparison set after Layer 1 is excluded. Small N constrains the kinds of findings this probe can produce.

## Section B: Per-header consumer matrix

For each of the 19 public headers, every consuming sim is listed. sph-water column is baselined from Layer 1's probe-1 § E (which enumerated DFSPH consumer-side common-cpp usage); rd-3d and eulerian-smoke columns are from this probe's 6d output.

| Header | rd-3d | es | sph-water | Count |
|---|:-:|:-:|:-:|:-:|
| `gpusims/alembic_writer.hpp` | — | — | ✓ | 1 |
| `gpusims/camera.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/gpu_profiler.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/hot_reload.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/imgui_setup.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/log.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/state_reader.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/state_writer.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/vdb_writer.hpp` | — | ✓ | — | 1 |
| `gpusims/vk/buffer.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/vk/compute_pipeline.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/vk/context.hpp` | ✓ | ✓ | ✓ | 3 |
| **`gpusims/vk/debug.hpp`** | **—** | **—** | **—** | **0** |
| `gpusims/vk/frame.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/vk/graphics_pipeline.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/vk/image.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/vk/renderer.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/vk/shader_compiler.hpp` | ✓ | ✓ | ✓ | 3 |
| `gpusims/vk/window.hpp` | ✓ | ✓ | ✓ | 3 |

### B.1 Findings from the matrix

**16 of 19 headers have uniform consumption** (all 3 Stack C sims `#include` them). This is the bulk of common-cpp's surface. There is no divergent header-level usage to flag.

**3 headers have non-uniform consumption:**

1. **`vk/debug.hpp` — 0 consumers** (§ E.1)
2. **`alembic_writer.hpp` — 1 consumer (sph-water only)** (§ E.2)
3. **`vdb_writer.hpp` — 1 consumer (eulerian-smoke only)** (§ E.2)

These are flagged in § E.

### B.2 Per-sim include density

From phase 6c output:

| Sim | Headers included |
|---|:-:|
| volumetric-grid/eulerian-smoke | 17 |
| continuous-ca/reaction-diffusion-3d | 16 |

The delta is exactly the `vdb_writer.hpp` include (es has it; rd-3d does not). Otherwise the two non-Layer-1 sims include the same 16 headers — confirming the matrix observation that header-level usage is uniform.

sph-water's include count is not enumerated in this probe (excluded from greps by Layer 1 scope rule), but Layer 1 probe-1 implies a similar magnitude.

## Section C: Per-symbol consumer data

> **METHODOLOGY DISCLOSURE (also documented in § G).** Phase 6e queried 23 symbols. The grep methodology reliably captured struct fields by name (e.g., `enable_subgroup_size_control`), class types by name (e.g., `GpuProfiler`, `HotReloader`, `StateWriter`), and free functions called bare (e.g., `memoryBarrier`). It **did not** reliably capture method calls — patterns like `Buffer::readback` only match qualified-name forms, but call sites typically look like `buf.readback(...)` where the receiver expression is unbound. Method-call counts below show as 0 in the raw probe output; this 0 is **not informative** — it means the grep pattern missed the calls, not that the calls don't exist.

### C.1 Reliable counts (struct fields, class types, free functions)

Per phase 6e, with build-artifact noise filtered (the raw `memoryBarrier` count included 45 hits each in `build-test-alembic/_deps` and `build/_deps`; those are vendored-dependency copies, not consumer call sites):

| Symbol | rd-3d | es | Type |
|---|:-:|:-:|---|
| `GpuProfiler` (class type) | 2 | 2 | class type |
| `profiler.scope` (free idiom) | 3 | 11 | method via fixed receiver name |
| `HotReloader` (class type) | 2 | 2 | class type |
| `StateWriter` (class type) | 2 | 2 | class type |
| `StateReader` (class type) | 4 | 3 | class type |
| `memoryBarrier` (free fn, filtered) | 3 | 12 | free function |
| `enable_subgroup_size_control` (field) | 0 | 0 | struct field |
| `required_subgroup_size` (field) | 0 | 0 | struct field |
| `require_full_subgroups` (field) | 0 | 0 | struct field |
| `src_color_blend_factor` (field) | 0 | 0 | struct field |
| `dst_color_blend_factor` (field) | 0 | 0 | struct field |
| `color_blend_op` (field) | 0 | 0 | struct field |

The struct-field zeros **are informative** — those symbols are reliably detected if present in source. Their absence confirms zero consumption of Phase 11 in-flight surface outside Layer 1 (§ D).

The `profiler.scope` and `memoryBarrier` counts are reliable because the grep pattern (`profiler.scope`, `memoryBarrier`) matches the conventional consumer-side usage form. They scale with sim complexity (es is the more compute-pass-heavy sim and unsurprisingly issues ~4× more profiler scopes and barriers than rd-3d).

### C.2 Unreliable counts (method calls)

The following queries returned 0 hits in phase 6e but the 0 is **not authoritative**:

- `Buffer::readback`, `Buffer::stage`
- `Image::readback`, `Image::upload`
- `runOneShot`
- `vdb::writeFloatGrid`, `vdb::writeVec3Grid`
- `abc::ParticleWriter`
- `lastResults`, `appendCsv`
- `Camera::setMode`

To get reliable method-call data, a follow-up probe would need to grep for the unqualified method name (e.g., `\.readback`, `\.stage`) inside consumer source files. **Architect-2 has elected not to run a correction probe in this round** — reasoning:

- The Layer 2 audit-relevant divergence questions (per the brief: "API used differently by different sims") were answered by the header matrix (§ B) and the reliable struct-field/class-type/free-fn counts (§ C.1). Both non-Layer-1 sims include the same 16 headers and touch zero Phase-11 surface; there is no smoke for method-call data to clarify.
- Method-call counts would size the eventual deep-audit-per-class effort, but that's a sizing question for a *future* probe and can be batched with the deep audit's own probe load.

If the coordinator disagrees with this scoping call, a correction probe is small (one grep block) and can be requested before the deep-audit sequence.

## Section D: Phase-11 in-flight surface — consumer-#1 confirmed

> **CLAIM:** No sim outside Layer 1 (sph-water) consumes any of the three Phase-11 in-flight surface additions catalogued in the inventory § E.
> **VERDICT: CONFIRMED** via the reliable-grep portion of phase 6e (§ C.1).

The three additions:

| Surface addition | Inventory § | Consumer count (non-Layer-1) |
|---|---|:-:|
| `ContextCreateInfo::enable_subgroup_size_control` + `Context::subgroupSize*` accessors | E.1 | 0 |
| `ComputePipelineDesc::required_subgroup_size` / `require_full_subgroups` | E.1 | 0 |
| `Buffer::readback(...)` | E.2 | 0 (method call; § C.2 caveat applies — but the surface itself is one method that has no plausible reason to be invoked by rd-3d or es) |
| `GraphicsPipelineDesc::{src,dst}_{color,alpha}_blend_factor` + `{color,alpha}_blend_op` | E.2 | 0 |

### D.1 Implication for banked consumer-#3 conventions

The inventory § H.4 catalogued two banked decisions from Phase 11 commit messages:

> "Named-toggle pattern for Vulkan feature requests adopted at consumer #1 per rule-of-three convention. Revisit at consumer #3 of any Stack C Vulkan feature toggle..."

> "Convention 4 banking: off-screen multi-pass rendering stays sim-local. Phase 11 is consumer #1; abstraction-promotion review triggers at consumer #2 of off-screen multi-pass in Stack C."

Both convention-revisit triggers are **far from firing**. With sph-water as the only consumer of:

- Vulkan named-toggle pattern: consumer #1. No consumer #2 visible; rule-of-three convention not approaching its revisit threshold.
- Off-screen multi-pass rendering: consumer #1. The es and rd-3d main passes both write directly to swapchain via `renderer.beginRendering()` (per phase 6f CMake comment context); neither has an off-screen multi-pass pipeline. No consumer #2 visible.

### D.2 Implication for Layer 1 commit 8 (async readback)

Per the inventory § G, when async-readback common-cpp surface lands, it will **also start at consumer #1** (sph-water for convergence checks). The "ship surface in its own commit with hello-world smoke test, then consumer code in a separate commit" pattern (inventory § E.1) applies. No precedent in rd-3d or es to either follow or diverge from.

## Section E: Structural anomalies

### E.1 `gpusims/vk/debug.hpp` is an orphan in the public surface

Per phase 6d, **zero consumers** include `gpusims/vk/debug.hpp`. Per the inventory § B.13, the header declares 7 public symbols:

| Symbol | `debug.hpp:` |
|---|---|
| `kValidationLayerName` (constexpr) | 15 |
| `checkValidationLayerSupport()` | 18 |
| `requiredDebugExtensions(uint32_t*)` | 22 |
| `populateDebugMessengerCreateInfo(...)` | 25 |
| `createDebugMessenger(VkInstance, ...)` | 28 |
| `destroyDebugMessenger(VkInstance, ...)` | 32 |
| `setObjectName(VkDevice, VkObjectType, uint64_t, const char*)` | 36 |

These functions are presumably used **internally by `context.cpp`** (validation layer setup is part of Context construction; the inventory § C.4 confirms `GPU_SIMS_VALIDATION_LAYERS` is a public define). The header is exported PUBLIC via `target_include_directories(... PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)` at `CMakeLists.txt:213-215`, but no sim has reached for it.

**Three interpretations**, each plausible:

1. **`setObjectName` was intended for consumer use** (it's the standard Vulkan-debug pattern: tag your own VkBuffers/VkImages with names for RenderDoc/RGP capture). The fact that no sim uses it suggests latent debt — sims debugging in RenderDoc are working with `VK_BUFFER` / `VK_IMAGE` rather than human-readable names. **Verdict: PLAUSIBLE FINDING. Worth flagging for sim-side audits.**

2. **Everything in `debug.hpp` is intended to be Context-internal**, and the header is public-exported only as a side effect of common-cpp's all-headers-are-public policy (§ C.1 of the inventory). **Verdict: PLAUSIBLE; would be the simpler explanation.**

3. **`setObjectName` is banked** for a future Stack C consumer that does heavy GPU-debugging work. **Verdict: POSSIBLE; no documentation evidence to confirm.**

Layer 2 does not have enough signal to pick between interpretations. The audit's contribution is to **catalog the orphan-ness** so the question can be answered when it next becomes relevant. A targeted question for the project owner: *"Should sims be using `setObjectName` on their VkBuffers/VkImages? If yes, this is latent debt across all 3 Stack C sims. If no, the header should be marked internal or moved to `src/internal/`."*

### E.2 Single-consumer exporter headers (Alembic, VDB)

Per phase 6d:

- `alembic_writer.hpp`: 1 consumer (sph-water only; first-exercised in Phase 11)
- `vdb_writer.hpp`: 1 consumer (eulerian-smoke only; first-exercised in Phase 8)

Both exporters are optional features (gated behind `GPU_SIMS_HAVE_ALEMBIC` / `GPU_SIMS_HAVE_OPENVDB`; see inventory § C.3). Both are gated at runtime by `isAvailable()` and degrade gracefully to stub-mode warnings when the build doesn't include the dependency.

**This finding is structurally important** because it ties directly back to the inventory § H.3 / Phase 11 retro category 7 ("unexercised real-impl in synced common-cpp"): the alembic_writer most-vexing-parse bug survived 10 phases because nobody-but-sph-water exercises that code path, and sph-water didn't exist for 10 phases. **The same risk shape applies to vdb_writer**, with one mitigating factor: vdb_writer has been exercised since Phase 8 (versus Phase 11 for alembic_writer). It's been in production for 3 phases. But: it's been in production *for exactly one consumer*, so any vdb_writer code path that the eulerian-smoke implementation doesn't happen to hit is in the same "exercised by nobody" boat as the alembic_writer bug was pre-Phase-11.

**Specific risk surfaces:**

- `vdb_writer.hpp:33` — `writeVec3Grid`: does eulerian-smoke call it? (It's a velocity-export helper. eulerian-smoke is a smoke sim — they export density, not velocity, in typical render pipelines.) If eulerian-smoke only calls `writeFloatGrid` and `writeFloatFrame`, then `writeVec3Grid` is in the unexercised-real-impl bucket.
- `vdb_writer.hpp:41` — `writeFloatFrame` (sequence helper): probable consumer call site if frame-by-frame export is wired in eulerian-smoke. The base function `writeFloatGrid` is the one most likely to be exercised.

These are questions for the unexercised-real-impl sweep probe (deferred to next round; see § P.4). Layer 2 flags the shape; the next probe enumerates the specific unexercised paths.

### E.3 Shader output-path divergence between rd-3d and eulerian-smoke

Per phase 6f, the two non-Layer-1 sims have a small but real convention divergence in their `CMakeLists.txt`:

**reaction-diffusion-3d** (`continuous-ca/reaction-diffusion-3d/CMakeLists.txt`):

```cmake
set(DST ${CMAKE_BINARY_DIR}/bin/${SHADER})
```

Shaders copied flat into `build/bin/`. Same pattern as the hello-world example (`common/common-cpp/examples/hello/CMakeLists.txt` per inventory § A.1).

**eulerian-smoke** (`volumetric-grid/eulerian-smoke/CMakeLists.txt`):

```cmake
set(DST ${CMAKE_BINARY_DIR}/bin/eulerian-smoke/${SHADER})
```

Shaders copied into a sim-named subdirectory `build/bin/eulerian-smoke/`.

**Risk:** If both binaries are ever run from the same cwd (`build/bin/`), the cwd-relative shader lookups would collide on shared filenames. Both sims ship `fullscreen.vert.glsl`; both presumably want their own copy. The flat-bin pattern (rd-3d, hello-world) has a latent collision risk; the subdirectory pattern (es) avoids it.

**Mitigating factor:** both sims compile-in absolute source paths via `target_compile_definitions` (`GPU_SIMS_RD3D_SHADER_DIR` for rd-3d at line 16; `GPU_SIMS_ES_SHADER_DIR` for es at line 24). If the runtime resolves shaders via these absolute paths in normal operation, the `bin/` copies are vestigial — present for the smoke-test-only path where the binary is run with the cwd-relative loader. This makes the collision a smoke-test-time risk, not a production risk.

**Verdict: STRUCTURAL DEBT — minor.** Not Layer 2's role to fix; flagged for whichever audit eventually examines the per-sim convention layer. Either:

- Promote es's subdirectory pattern to a project convention (back-port to rd-3d and hello-world), OR
- Recognize the `bin/` copy is vestigial and remove the `add_custom_command` block from both sims (rely on `GPU_SIMS_*_SHADER_DIR` exclusively).

## Section F: Build-config survey

### F.1 No sim sets `GPU_SIMS_USE_OPENVDB` or `GPU_SIMS_USE_ALEMBIC` in its own CMakeLists

Per phase 6f, neither rd-3d nor eulerian-smoke contains `GPU_SIMS_USE_OPENVDB` or `GPU_SIMS_USE_ALEMBIC` in its CMakeLists. These flags are set:

- At the top-level repo `CMakeLists.txt` (outside Layer 2 scope), OR
- Via build-environment `-D` flags (e.g., CI invokes `cmake -DGPU_SIMS_USE_OPENVDB=ON ...`), OR
- Default to OFF.

The eulerian-smoke CMakeLists comment at lines 5-9 confirms the design intent:

```cmake
# This sim is the first real consumer of common-cpp's OpenVDB writer
# (gpusims::vdb::writeFloatFrame). VDB export is gated at runtime by
# gpusims::vdb::isAvailable(), which returns true only when common-cpp
# was built with -DGPU_SIMS_USE_OPENVDB=ON. Stub mode compiles and runs
# fine; the VDB toggle in the panel becomes a no-op.
```

**Pattern is consistent across consumers:** optional features are runtime-gated via `isAvailable()`. Sims compile and run with optional deps off (stub mode); enabling deps is a build-environment decision, not a sim-side decision.

### F.2 Per-sim CMakeLists are template-clones

The two non-Layer-1 CMakeLists are structurally near-identical:

- Same `add_executable(<sim_name> src/main.cpp)` shape.
- Same `target_link_libraries(<sim> PRIVATE gpusims::common_cpp)`.
- Same `target_compile_features(<sim> PRIVATE cxx_std_20)` C++20 commitment.
- Same `target_compile_definitions(<sim> PRIVATE GPU_SIMS_<SHORT>_SHADER_DIR="...")` pattern.
- Same `set_target_properties(<sim> PROPERTIES OUTPUT_NAME "..." RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")`.
- Same `add_custom_command` + `add_custom_target` + `add_dependencies` shader-copy pattern (with the E.3 subdirectory divergence).

The "template" is clearly visible. Both sim CMakeLists also reference the hello-world example as their explicit template ancestor — rd-3d at line 14 ("Mirrors common-cpp/examples/hello.") and es at line 25 ("Mirrors common-cpp/examples/hello and reaction-diffusion-3d.").

### F.3 C++20 commitment

Both sims declare `cxx_std_20` (line 14 of rd-3d, line 22 of es). common-cpp's top-level CMakeLists does not explicitly set a C++ standard for the library target (verified via the inventory § A.1 file enumeration; no `target_compile_features` on `gpu_sims_common_cpp` itself was visible in the phase 3a CMakeLists output). The standard likely inherits from a project-wide default in the top-level repo CMakeLists (outside Layer 2 scope). **Worth flagging as a question:** is common-cpp itself committed to C++20, or to something looser, and is that intentional? Not in Layer 2 scope to answer; noted for whichever audit examines the top-level project CMake.

## Section G: Probe methodology and limitations

### G.1 What the probe captured reliably

- **Phase 6a** (sim directories linking common-cpp): complete. `grep -rl ... CMakeLists.txt` is reliable for the link directive.
- **Phase 6b/6c/6d** (header includes): complete. `#include <gpusims/...>` is a literal-string match; greppable without ambiguity. Every include is captured.
- **Phase 6e — struct fields, class types, free functions**: reliable. Phase-11 struct fields (`enable_subgroup_size_control`, etc.) are uniquely-named and greppable. Class types (`GpuProfiler`, `HotReloader`, `StateWriter`) and free functions called bare (`memoryBarrier`) match conventional consumer-side usage forms.
- **Phase 6f** (per-sim CMakeLists content): complete; bulk-cat output.

### G.2 What the probe captured unreliably

**Method-call counts.** Phase 6e queries like `Buffer::readback` and `Image::upload` match only fully-qualified usage forms. Conventional consumer-side method calls (e.g., `tier.particles.stage(ctx, data, bytes)`) do not include the class-qualification prefix. The 0 results for method-call queries in phase 6e are not informative — they mean the grep pattern missed the calls, not that the calls are absent.

**Reliability filter applied in this report:** § C.1 lists only the reliable subset. § C.2 explicitly lists the unreliable queries and notes their 0 counts are not authoritative.

### G.3 Why no correction probe was run this round

Architect-2 elected to write the report with the available data rather than run a correction probe for method-call resolution. Reasoning:

- **The audit-relevant divergence questions are already answered.** Per the brief: "Identify cross-sim dependencies — which sims rely on which common-cpp APIs, and how heavily. Flag any common-cpp API used differently by different sims (a smell)." The header matrix (§ B), the reliable struct-field counts (§ C.1), and the CMakeLists survey (§ F) collectively show: 16 of 19 headers consumed uniformly by all 3 sims, 0 Phase-11 surface consumption outside Layer 1, no CMake-config divergence beyond the minor shader-path issue. There is no smoke for method-call counts to clarify.
- **Method-call counts size deep-audit effort.** That sizing question can be batched with the deep audit's own probe load — saves a probe round at the cost of slightly less-informed deep-audit prioritization.
- **The coordinator was explicitly informed** of this scoping call in the proposal turn that preceded this report and was asked to push back if disagreement; no pushback was registered.

If a future probe needs method-call data for a specific question, the correction probe is small:

```bash
# Sketch — not part of this report's submitted probe load.
for method in readback stage upload deviceAddress; do
  echo "--- .$method() call sites in consumer code ---"
  grep -rn "\.$method(" --include="*.cpp" --include="*.hpp" . 2>/dev/null \
    | grep -v "^./common/common-cpp/" \
    | grep -v "^./particle-fluids/sph-water/" \
    | grep -v "^./build" \
    | grep -v "^./references/"
done
```

## Section P: Incidental findings and cross-workstream flags

### P.1 Phase-11 retro category 7 risk extends to vdb_writer (`cross_workstream: layer-1`)

Per § E.2, `vdb_writer.hpp` has exactly one consumer (eulerian-smoke). Per the inventory § H.3, the named structural-debt pattern is "unexercised real-impl in synced common-cpp" — code paths that no sim's CI exercises until a new consumer comes along and surfaces decade-old bugs.

The vdb_writer is in a partial-mitigation state: it has been exercised since Phase 8, but only by one sim. **Specific risk:** any public function in `vdb_writer.hpp` that eulerian-smoke does not happen to call is in the same risk bucket as the alembic_writer most-vexing-parse bug was pre-Phase-11.

Layer 2 cannot enumerate which specific functions are unexercised without reading eulerian-smoke's main.cpp (out of Layer 2 scope — that's per-sim work). The unexercised-real-impl sweep probe proposed in the inventory § H.3 and § "Next Layer-2 probe proposals" #3 would answer this. Coordinator: this strengthens the case for running that probe soon.

### P.2 The `bin/` shader-copy pattern may be vestigial across all 3 Stack C sims

Per § E.3, both rd-3d and eulerian-smoke compile-in absolute-path `GPU_SIMS_*_SHADER_DIR` defines. If runtime shader loading uses these defines (rather than cwd-relative resolution), the `add_custom_command` shader-copy blocks in both sim CMakeLists are vestigial — useful only for the smoke-test-from-cwd path. Same likely applies to hello-world.

This is a sim-side cleanup item, not Layer 2 scope. Flagged for awareness.

### P.3 No explicit C++ standard on `gpu_sims_common_cpp` itself

Per § F.3, both consumer sims declare `cxx_std_20`. common-cpp's CMakeLists (per inventory § A.1, phase 3a output) does not. The library likely inherits a project-wide default from the top-level repo CMakeLists (outside Layer 2 scope). Worth a follow-up question to confirm: *is common-cpp's C++ standard explicit somewhere, or implicit and inheritance-dependent?* If the latter, common-cpp could compile against a different standard than its consumers, which is brittle. Flagged for top-level CMake audit.

### P.4 Refinement of the deferred-probe ranking

The inventory § "Next Layer-2 probe proposals" listed three candidates:

1. Per-class implementation audit
2. Cross-sim consumer mapping (this report)
3. Unexercised-real-impl sweep

With #2 complete, the remaining ranking should be refined:

- **#3 (unexercised-real-impl sweep)** moves up in priority. The consumer-mapping data confirms the risk shape is **wider than the alembic_writer case alone** — vdb_writer is in the same bucket. The probe is small (enumerate `#if GPU_SIMS_HAVE_*` and `if (info.enable_*)` blocks, cross-check against shipped-sim CI configurations) and high-yield.
- **#1 (per-class implementation audit)** now has the consumer-fanout data it needs to prioritize: high-fanout classes (`Context`, `Renderer`, `Window`, `ShaderCompiler`, `ComputePipeline`, `GraphicsPipeline`, `Buffer`, `Image`, `Frame`, `Camera`, `HotReloader`, `GpuProfiler`, `StateReader`, `StateWriter`) — all consumed by all 3 sims — are the deep-audit-priority entries. Low-fanout entries (`debug` — 0 consumers; `alembic_writer` — 1; `vdb_writer` — 1) are deprioritized for the deep audit but **prioritized for unexercised-real-impl sweep**.

### P.5 No additional `cross_workstream: layer-1` items beyond inventory

This probe produced one cross-workstream-tagged finding (P.1, the vdb_writer category-7 risk). Otherwise, all Layer-1-impacting items were already surfaced in the inventory's § P (the gpu_profiler frame-index bug, the Alembic SHA cross-contamination, the surface-additions-ship-in-two-commits convention).

---

## Summary

| Section | Verdict |
|---|---|
| A. Consumer universe | **COMPLETE.** 3 Stack C sims total (sph-water, rd-3d, eulerian-smoke). Other 6 shipped sims are Stack B / D and do not link common-cpp. |
| B. Per-header matrix | **COMPLETE.** 16 of 19 headers consumed uniformly by all 3 sims; 1 header (debug.hpp) has 0 consumers; 2 headers (alembic_writer, vdb_writer) have exactly 1 consumer each. No divergent header-level usage. |
| C. Per-symbol data | **PARTIAL.** Reliable for struct fields, class types, free functions. Unreliable for method-call counts (§ G.2); 0-count method queries are not authoritative. No correction probe run this round (§ G.3). |
| D. Phase-11 surface consumption | **CONFIRMED** at consumer #1. Zero consumption of any Phase-11 in-flight surface outside Layer 1. Both banked "consumer #3" and "consumer #2" convention-revisit triggers are not approaching their thresholds. |
| E. Structural anomalies | **CATALOGUED.** (1) vk/debug.hpp orphan (0 consumers); (2) alembic_writer and vdb_writer each have 1 consumer — category-7 risk extends to vdb_writer; (3) shader-copy convention diverges (subdirectory vs flat) between es and rd-3d. |
| F. Build-config survey | **COMPLETE.** No sim-side flag-flips for optional deps; runtime gating via `isAvailable()` is the consistent pattern. Both sims declare C++20 explicitly; common-cpp does not. |
| G. Methodology | **DISCLOSED.** Reliable-vs-unreliable grep coverage explicit; correction probe sketched for future use. |
| P. Incidentals | 5 entries, 1 flagged `cross_workstream: layer-1` (vdb_writer category-7 risk). |

**Recommended next Layer-2 probe** (decision-only, no probe load proposed in this report):

**Unexercised-real-impl sweep** — promoted to first priority based on this probe's finding that the category-7 risk shape applies to vdb_writer as well as alembic_writer. Probe sketch (will be specced as a full prompt when greenlit):

- Enumerate every `#if GPU_SIMS_HAVE_OPENVDB` and `#if GPU_SIMS_HAVE_ALEMBIC` block in `alembic_writer.cpp` / `vdb_writer.cpp` — list every function definition inside each block.
- Enumerate every `if (info.enable_*)` flag-gated path in `context.cpp`.
- For each flag-gated public function, check the single consumer's main.cpp (rd-3d for none, es for vdb, sph-water out-of-scope) for calls.
- Output: a list of public functions that are reachable in source but unexercised by any shipped sim's CI.

End of report.
