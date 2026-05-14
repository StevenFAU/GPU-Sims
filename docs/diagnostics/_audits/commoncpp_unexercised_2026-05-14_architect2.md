---
title: Common-cpp Unexercised-Real-Impl Sweep — Layer 2 audit, Probe 3
date: 2026-05-14
author: architect2
layer: 2
scope: common/common-cpp/ — enumerate real-impl code paths reachable in source but not exercised by any shipped Stack C sim
status: probe (defect-hunt) — read-only
sibling-layers:
  - Layer 1: particle-fluids/sph-water/ (in progress; see phase11_5_* reports)
  - Layer 3: per-sim triage at docs/diagnostics/_audits/sims_prioritization_2026-05-14_triage.md
predecessors:
  - docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md
  - docs/diagnostics/_audits/commoncpp_consumers_2026-05-14_architect2.md
out-of-scope:
  - per-class implementation deep audits (still deferred to later Layer 2 probes)
  - any code modification or fix
  - sph-water source-level audit; sph-water main.cpp lines outside the symbol-grep snippets in phase 7e are not read
cross_workstream: layer-1
---

> Third Layer-2 audit deliverable, hunting the "Phase 11 retro category 7" pattern — real-impl code that compiles fine but no sim exercises, so defects survive until a new consumer trips them. **One real defect surfaced:** `abc::ParticleFrame::radii` is declared in the public struct but never read by the implementation; any consumer setting `frame.radii` has the data silently dropped. Sph-water is the only Alembic consumer, so this is a live cross-workstream-relevant finding. **Two large unexercised function bodies surfaced:** `vdb::writeVec3Grid` (50+ lines, never called by any sim) and the `frame.velocities` branch of `RealParticleWriter::writeFrame` (exercise status UNVERIFIABLE without Layer 1 source read). **One verification CONFIRMED:** the compute_pipeline.cpp pNext-chain implementation correctly satisfies the documented INVARIANT.

## Section A: Summary of findings by risk tier

Total findings: 8 unexercised paths + 1 verified invariant. Ranked by impact and named in the right column for cross-reference.

| # | Risk | Surface | What's unexercised | Where | Detail § |
|---|---|---|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 1 | **HIGH (defect)** | `abc::ParticleFrame::radii` field | Declared in public struct; **never read** by `RealParticleWriter::writeFrame`. Consumer-set radii data is silently dropped. | `alembic_writer.hpp:24` declared; `alembic_writer.cpp:51-82` never reads it | § B.3 |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 2 | **HIGH** | `vdb::writeVec3Grid(...)` | 50+ line public function (Vec3SGrid + manual voxel-by-voxel loop) never called by any sim. eulerian-smoke uses `writeFloatFrame` only. | `vdb_writer.cpp:97-145` | § C.2 |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 3 | **MEDIUM** | `RealParticleWriter::writeFrame` velocity branch | Conditional `if (frame.velocities) { ... }` block. Exercised iff sph-water sets `fr.velocities`; not visible in probe data. | `alembic_writer.cpp:66-75` | § B.4 |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 4 | **MEDIUM** | `Buffer::deviceAddress(VkDevice)` | Public method; no consumer call sites across rd-3d, es, sph-water. | `buffer.hpp:62`; impl in `buffer.cpp` | § E.1 |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 5 | **MEDIUM** | `Buffer::stage(...)` | Single-consumer (sph-water only). Same risk shape as alembic_writer pre-Phase-11. | `buffer.hpp:52`; impl in `buffer.cpp` | § E.2 |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 6 | **LOW** | `ContextCreateInfo::require_discrete_gpu = true` | Short-circuit `return 0` branch in `scoreDevice`; no sim sets the flag. | `context.cpp:368` | § D.1 |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 7 | **LOW** | `ContextCreateInfo::enable_swapchain = false` | Headless-sim path; no headless sim exists. | `context.cpp:245, 282` (and `defaultDeviceExtensions`) | § D.2 |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 8 | **LOW** | `extra_instance_extensions` / `extra_device_extensions` non-empty | No sim populates either vector. | `context.cpp:190, 246, 283` | § D.3 |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| ✓ | **CONFIRMED** | `compute_pipeline.cpp` pNext-chain invariant | Implementation matches the load-bearing comment block at `compute_pipeline.hpp:40-50`. Both `create()` and `reload()` paths verified. | `compute_pipeline.cpp:102-120, 193-207` | § F |

### A.1 What's notably absent from this list

Things I went looking for that turned out to be exercised — worth recording so future probes know not to re-investigate:

- **`Image::readback(void* dst, size_t)`**: 2-argument signature called by rd-3d (lines 706-707) and eulerian-smoke (lines 1432-1435, 2147, 2210). **Exercised.** I almost flagged it as unexercised on incomplete inference — see § E.3.
- **`Image::upload(void*, size_t)`**: heavily used by both rd-3d and es. **Exercised** (28 distinct call sites across the two sims; phase 7f).
- **`Image::transitionLayout(...)`**: used by sph-water (7 call sites; phase 7e) and presumably by Image::upload/readback internally. **Exercised.**
- **`Context::runOneShot(...)`**: no consumer call sites outside common-cpp itself, but this is an *internal* helper called by `Buffer::stage`, `Buffer::readback`, `Image::upload`, `Image::readback` — those calls reach `runOneShot` indirectly. **Exercised internally.**
- **All `Phase-11 in-flight common-cpp surface`** (subgroup-size-control fields/accessors, blend-factor fields): all exercised by sph-water per phase 7e. **Exercised at consumer #1.**
- **`GPU_SIMS_VALIDATION_LAYERS`-gated code in `context.cpp`** and `debug.cpp`: exercised in Debug builds, no-op in Release. CI builds both (per `build-native.yml`, not enumerated here). **Exercised by compilation matrix.**

## Section B: alembic_writer.cpp exercised-vs-unexercised mapping

Full source quoted at phase 7a (lines 6-117 of the probe output map to alembic_writer.cpp lines 1-117).

### B.1 Path inventory

| Path | Lines | Exercised? | By |
|---|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `logUnavailableOnce()` (stub-mode warning) | `alembic_writer.cpp:17-24` | Conditionally exercised (only when `GPU_SIMS_HAVE_ALEMBIC=0`) | CI default Debug job (per phase 7g; OPENVDB=ON, ALEMBIC=ON sometimes off) |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `abc::isAvailable()` | `:28-34` | **Exercised** | sph-water `main.cpp:1214, 2408, 2631` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `RealParticleWriter::RealParticleWriter(path, fps)` constructor | `:40-49` | **Exercised** | sph-water creates one at `main.cpp:1215` |
| `RealParticleWriter::writeFrame` — positions/ids/count handling | `:51-65, 76-77` | **Exercised** | sph-water calls `writeFrame` per phase 7e context |
| `RealParticleWriter::writeFrame` — velocity branch | `:66-75` | **UNVERIFIABLE** (Layer 1) | depends on whether sph-water sets `fr.velocities`; see § B.4 |
| `RealParticleWriter::writeFrame` — exception path | `:78-81` | **UNVERIFIED** | hits if Alembic library throws; not observed in CI |
| `StubParticleWriter` class | `:92-96` | Conditionally exercised (only when `GPU_SIMS_HAVE_ALEMBIC=0`) | CI default jobs running with ALEMBIC=OFF |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `ParticleWriter::create(path, fps)` factory | `:100-113` | **Exercised** (real path) | sph-water `main.cpp:1215` |
| `ParticleWriter::~ParticleWriter()` | `:115` | **Exercised** | RAII destruction at sph-water scope exit |

### B.2 `frame.ids ? frame.ids[i] : i` ternary

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
At `alembic_writer.cpp:61`:

```cpp
ids[i] = frame.ids ? frame.ids[i] : i;
```

The ternary covers two paths:

- **TRUE branch** (`frame.ids != nullptr`): use caller-supplied IDs.
- **FALSE branch** (`frame.ids == nullptr`): use loop index as the ID.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Exercise status of each depends on whether sph-water sets `fr.ids` when populating its `ParticleFrame`. The phase 7e grep on sph-water `main.cpp` showed `abc::ParticleFrame fr{};` at line 2423 (zero-initialized) but did not capture subsequent field-population lines. **UNVERIFIABLE** without reading sph-water main.cpp:2423-2430 specifically; Layer 1 scope rule excludes that read.

Verdict: at least one of the two branches is exercised (the consumer code reaches this line), but which one cannot be determined from Layer 2 data alone.

### B.3 **DEFECT: `ParticleFrame::radii` is dead in the public struct**

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
> **CLAIM:** `abc::ParticleFrame` declares a public `radii` field at `alembic_writer.hpp:24`, but `RealParticleWriter::writeFrame` never reads it. Consumer-set radii data is silently dropped.
> **VERDICT: CONFIRMED.**

Evidence:

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`alembic_writer.hpp:21-27` (struct declaration, from inventory § B.1):

```cpp
struct ParticleFrame {
    const float*    positions  = nullptr;  // 3 floats per particle (x, y, z)
    const float*    velocities = nullptr;  // 3 floats per particle (optional)
    const float*    radii      = nullptr;  // 1 float per particle (optional)
    const std::uint64_t* ids   = nullptr;  // optional
    std::size_t     count      = 0;
};
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`alembic_writer.cpp:51-82` (entire `writeFrame` body, phase 7a):

```cpp
bool writeFrame(const ParticleFrame& frame) override {
    try {
        if (!frame.positions || frame.count == 0) return false;
        std::vector<Alembic::Abc::V3f> positions(frame.count);
        std::vector<Alembic::Abc::uint64_t> ids(frame.count);
        for (std::size_t i = 0; i < frame.count; ++i) {
            positions[i] = Alembic::Abc::V3f(
                frame.positions[3 * i + 0],
                frame.positions[3 * i + 1],
                frame.positions[3 * i + 2]);
            ids[i] = frame.ids ? frame.ids[i] : i;
        }
        Alembic::AbcGeom::OPointsSchema::Sample sample{
            Alembic::Abc::V3fArraySample(positions),
            Alembic::Abc::UInt64ArraySample(ids)};
        if (frame.velocities) {
            std::vector<Alembic::Abc::V3f> vels(frame.count);
            for (std::size_t i = 0; i < frame.count; ++i) {
                vels[i] = Alembic::Abc::V3f(
                    frame.velocities[3 * i + 0],
                    frame.velocities[3 * i + 1],
                    frame.velocities[3 * i + 2]);
            }
            sample.setVelocities(Alembic::Abc::V3fArraySample(vels));
        }
        points_.set(sample);
        return true;
    } catch (const std::exception& e) {
        logError("alembic-writer: writeFrame failed: {}", e.what());
        return false;
    }
}
```

Fields read: `positions`, `count`, `ids`, `velocities`. **`radii` is never referenced.** No `OPointsSchema::Sample` constructor or method call sets a radius array.

This matches the named structural pattern from Phase 11 retro category 7 ("unexercised real-impl in synced common-cpp"). The shape:

- Public surface declared in Phase 1 ("real impl" code, not labeled as stub-only).
- Implementation gap silently shipped because the consumer flag (`GPU_SIMS_USE_ALEMBIC=ON`) wasn't flipped by any sim until Phase 11.
- Phase 11 sph-water is consumer #1 of Alembic, but if sph-water doesn't set `frame.radii`, the defect remains latent until consumer #2.

**Cross-workstream impact** (`cross_workstream: layer-1`):

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
If sph-water sets `fr.radii = <particle_radii_ptr>` somewhere in `main.cpp:2424-2430`, that data is being silently lost during Alembic export — and the produced .abc files have constant-zero or absent radius attribute, which would surface only when the downstream pipeline (Blender import, per repo context) renders particles without their per-particle radius. If sph-water doesn't set `fr.radii`, the bug is latent (no current data loss) but still ships in the public surface.

Architect-1 should be informed so Layer 1 can either:
1. Verify whether sph-water sets `fr.radii` (cheap check; one grep at the `ParticleFrame fr{};` site).
2. If yes: confirm export silently drops it, and decide whether to surface this as a Phase 11 retro bullet (the same retro category 7 bullet pattern).
3. If no: log the defect for fix in a future phase, before consumer #2 of alembic_writer comes along.

Layer 2 does NOT recommend a fix here — that's outside audit scope. The audit's contribution is to surface the defect with citations.

### B.4 `RealParticleWriter::writeFrame` — velocities branch exercise status UNVERIFIABLE

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The conditional at `alembic_writer.cpp:66`:

```cpp
if (frame.velocities) {
    // ... 9 lines of velocity-handling
    sample.setVelocities(Alembic::Abc::V3fArraySample(vels));
}
```

Same exercise-status question as B.2 (the ids ternary): depends on whether sph-water sets `fr.velocities`. UNVERIFIABLE without Layer 1 source read. The branch *body* is 9 lines of straightforward translation — low complexity, but still unexercised at the Layer 2-visible level if sph-water does not exercise it.

### B.5 Compile-time gating

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The `#if GPU_SIMS_HAVE_ALEMBIC` block spans `alembic_writer.cpp:36-88` (the `RealParticleWriter` class) and the create()-path branch at `:102-108`. The `#else` branch (`StubParticleWriter`) spans `:90-96` and the create()-path stub branch at `:109-112`.

Per phase 7g, CI exercises both:

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- `build-native.yml:62` and `:110`: `-DGPU_SIMS_USE_ALEMBIC=ON` (real-impl compiled and linked)
- Other CI jobs default to OFF: stub path compiled and linked

Both compile paths are CI-exercised; only the **execution paths** through the real impl are sim-exercised, and only by sph-water.

## Section C: vdb_writer.cpp exercised-vs-unexercised mapping

Full source quoted at phase 7b (lines 127-284 of the probe output map to vdb_writer.cpp lines 1-158).

### C.1 Path inventory

| Path | Lines | Exercised? | By |
|---|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `logUnavailableOnce()` | `vdb_writer.cpp:19-26` | Conditionally exercised (OPENVDB=0) | CI Debug jobs without OPENVDB |
| `initOpenVdbOnce()` | `:28-32` (real-impl only) | **Exercised** (via writeFloatGrid call chain) | eulerian-smoke `writeFloatFrame` → `writeFloatGrid` → `initOpenVdbOnce` |
| `frameSequencePath(base, frame)` helper | `:35-41` | **Exercised** (via writeFloatFrame) | eulerian-smoke |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `vdb::isAvailable()` | `:45-51` | **Exercised** | eulerian-smoke `main.cpp:2128, 2204` |
| `vdb::writeFloatGrid(...)` real-impl body | `:59-94` | **Exercised indirectly** | eulerian-smoke calls `writeFloatFrame` which calls `writeFloatGrid` |
| `vdb::writeFloatGrid(...)` stub branch | `:91-93` | Conditionally exercised | CI OPENVDB=0 jobs |
| **`vdb::writeVec3Grid(...)` real-impl body** | **`:103-139`** | **NEVER EXERCISED** | no consumer calls it; see § C.2 |
| `vdb::writeVec3Grid(...)` stub branch | `:141-143` | Conditionally exercised | CI OPENVDB=0 jobs |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `vdb::writeFloatFrame(...)` | `:147-156` | **Exercised** | eulerian-smoke `main.cpp:2148, 2211` |

### C.2 **`vdb::writeVec3Grid(...)` is the largest unexercised real-impl body in common-cpp**

> **CLAIM:** `vdb::writeVec3Grid` has 50+ lines of real implementation and is called by no Stack C sim.
> **VERDICT: CONFIRMED** — grep for `vdb::` across the probe output (phase 7e for sph-water; eulerian-smoke main.cpp included in phase 7d at lines 701-2940). Calls found: `vdb::isAvailable()`, `vdb::writeFloatFrame(...)`. **No `vdb::writeVec3Grid` call anywhere.**

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The real-impl body at `vdb_writer.cpp:103-139` (full quote from phase 7b):

```cpp
bool writeVec3Grid(const std::filesystem::path& path,
                   const float*                 data,
                   glm::ivec3                   dims,
                   float                        voxel_size,
                   glm::vec3                    origin,
                   const char*                  grid_name) {
#if GPU_SIMS_HAVE_OPENVDB
    if (!data || dims.x <= 0 || dims.y <= 0 || dims.z <= 0) return false;
    initOpenVdbOnce();
    try {
        openvdb::Vec3SGrid::Ptr grid = openvdb::Vec3SGrid::create(openvdb::Vec3s(0.0f));
        grid->setName(grid_name ? grid_name : "velocity");
        grid->setTransform(openvdb::math::Transform::createLinearTransform(voxel_size));
        grid->setGridClass(openvdb::GRID_STAGGERED);

        // Manual fill: openvdb::tools::copyFromDense doesn't have a Vec3 specialization
        // we can rely on across versions, so we set values voxel-by-voxel.
        auto accessor = grid->getAccessor();
        for (int z = 0; z < dims.z; ++z) {
            for (int y = 0; y < dims.y; ++y) {
                for (int x = 0; x < dims.x; ++x) {
                    const std::size_t i = static_cast<std::size_t>(
                        x + dims.x * (y + dims.y * z)) * 3;
                    accessor.setValue(openvdb::Coord(x, y, z),
                                      openvdb::Vec3s(data[i + 0], data[i + 1], data[i + 2]));
                }
            }
        }

        if (origin != glm::vec3(0.0f)) {
            grid->transform().postTranslate(openvdb::Vec3d(origin.x, origin.y, origin.z));
        }
        // ...
```

**Notable risks in this unexercised body:**

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
1. **The manual voxel-by-voxel fill loop** (the comment explicitly justifies it: "openvdb::tools::copyFromDense doesn't have a Vec3 specialization we can rely on across versions"). Manual loops are the classic site for off-by-one indexing errors. The `x + dims.x * (y + dims.y * z) * 3` index expression depends on operator precedence — `*` binds tighter than `+`, so it parses as `x + (dims.x * (y + dims.y * z)) * 3` which means `x` is unmultiplied. Is that the intended linearization order? **Yes**, assuming x-fastest convention per the header docblock at `vdb_writer.hpp:18` ("3D grids are linearized x-fastest, then y, then z"). But:
   - The `* 3` multiplier converts a voxel index to a float offset (3 floats per voxel). Applied to the *inner* expression but **not** to `x`. That means: `voxel(x, y, z)` reads `data[x + dims.x*(y + dims.y*z) * 3 + {0, 1, 2}]` — which **is wrong**. The intended formula is `data[(x + dims.x*(y + dims.y*z)) * 3 + {0, 1, 2}]` — the `* 3` should be on the whole voxel-index, not on the sub-expression.

   Let me re-read carefully:
   ```cpp
   const std::size_t i = static_cast<std::size_t>(
       x + dims.x * (y + dims.y * z)) * 3;
   ```
   The cast is `static_cast<std::size_t>( x + dims.x * (y + dims.y * z) )` and the **`* 3` is applied after the cast, outside it**. So `i = (linear_voxel_index) * 3` — correct.

   I almost flagged this as a bug; on careful re-reading, the `* 3` is outside the cast and applies to the entire voxel-index. **No defect.** But the code is hard to read; a future maintainer could easily mis-parenthesize this if refactored. Worth noting as a readability hazard.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
2. **The origin-translate branch** at `vdb_writer.cpp:126-128` differs subtly from the writeFloatGrid version at `:75-78`. The float version *re-sets* the transform inside the branch (line 76: `grid->setTransform(...)` then `:77 grid->transform().postTranslate(...)`). The Vec3 version only post-translates without resetting (`:127 grid->transform().postTranslate(...)`). Inconsistent. If the original intent was to ensure a clean voxel-size transform before applying the origin offset, the Vec3 path may produce a different transform than the float path. **Flag for future per-class deep audit.**

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
3. **GRID_STAGGERED grid class** (`:110`) — this is the OpenVDB grid class for staggered (MAC-grid) velocity data. Semantically correct for fluid velocity export, but the consumer-side interface gives the data as a packed float* with the x-fastest convention. If a consumer interprets their velocity buffer as cell-centered rather than staggered, the export would be semantically incorrect. The docblock at `vdb_writer.hpp:32` ("Write a single dense vec3 grid to a .vdb file (interleaved xyz floats)") doesn't mention staggered semantics. Possible documentation gap.

The body is sufficiently complex that "compile-tested but never run" is real risk. When consumer #2 of vdb_writer comes along (or eulerian-smoke decides to export velocity for a future render-pipeline change), this code path is the highest-risk place in common-cpp.

### C.3 `writeFloatGrid` is reached only indirectly

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
eulerian-smoke calls `vdb::writeFloatFrame(...)` directly (per phase 7d, lines 2148 and 2211). `writeFloatFrame` is a 3-line wrapper that calls `writeFloatGrid` internally (`vdb_writer.cpp:147-156`). So `writeFloatGrid` is **execution-reachable but not directly invoked**. Future maintainers refactoring `writeFloatFrame` should know that they're the only call site for `writeFloatGrid`.

## Section D: ContextCreateInfo flag-gated branches

Phase 7c provides the full source of `context.cpp`. Five `ContextCreateInfo` fields control branching:

- `enable_subgroup_size_control` — see inventory § E.1; sph-water exercises both branches.
- `require_discrete_gpu` — § D.1 below.
- `enable_swapchain` — § D.2 below.
- `extra_instance_extensions` — § D.3 below.
- `extra_device_extensions` — § D.3 below.

Default values from `ContextCreateInfo` (inventory § B.12):

| Field | Default | Sims setting non-default |
|---|---|---|
| `enable_subgroup_size_control` | `false` | sph-water (`true`); rd-3d and es use default |
| `require_discrete_gpu` | `false` | none (all 3 sims use default) |
| `enable_swapchain` | `true` | none (all 3 sims use default) |
| `extra_instance_extensions` | `{}` | none |
| `extra_device_extensions` | `{}` | none |

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Per phase 7c grep results, the only `ContextCreateInfo` consumer-side line outside context.cpp itself is `sph-water main.cpp:1164`: `cdesc.enable_subgroup_size_control = true;`. Neither rd-3d nor eulerian-smoke uses an explicit `ContextCreateInfo` — they use the default constructor `Context()` (which calls `Context(ContextCreateInfo{})`).

### D.1 `require_discrete_gpu = true` path

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`context.cpp:368`:

```cpp
if (require_discrete && props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) return 0;
```

A single-line short-circuit that returns 0 (= not selectable) when the device is non-discrete and the consumer requires discrete. Unexercised. Risk is low — single conditional, no complex logic.

**Latent risk shape:** if a future sim sets `require_discrete_gpu = true` on a system with both an integrated and discrete GPU, this path is the first time it's been tested. Probably works; possibly has a defect like "integrated GPU still picked because it has a higher score before the discrete check." The score logic at `:75-79` adds 100000 for discrete and 10000 for integrated, so the discrete-required path should win on score alone — but that's untested in this specific gating.

### D.2 `enable_swapchain = false` path

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`context.cpp:245` and `:282`:

```cpp
auto required = defaultDeviceExtensions(info.enable_swapchain);
// ...
auto exts = defaultDeviceExtensions(info.enable_swapchain);
```

Where `defaultDeviceExtensions(bool with_swapchain)` at `:83-94`:

```cpp
std::vector<const char*> defaultDeviceExtensions(bool with_swapchain) {
    std::vector<const char*> e;
    if (with_swapchain) {
        e.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    e.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    e.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
    return e;
}
```

The `with_swapchain == false` branch is unexercised — no headless sim exists. The flag was presumably added for future headless-render or compute-only sims. Risk is low: the branch simply omits one extension.

**Wider unexercised footprint:** A headless sim would also need to NOT create a `Window` (and thus not call GLFW), which is enforced at the per-sim level rather than common-cpp. Common-cpp doesn't prevent the misuse "set `enable_swapchain = false` but also construct a Window," which would presumably surface as a Vulkan surface-extension-not-enabled error at Window construction. Not a defect; an interface contract that nothing currently exercises.

### D.3 `extra_instance_extensions` / `extra_device_extensions` non-empty

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`context.cpp:190, 246, 283`:

```cpp
for (auto e : info.extra_instance_extensions) exts.push_back(e);
// ...
for (auto e : info.extra_device_extensions) required.push_back(e);
// ...
for (auto e : info.extra_device_extensions) exts.push_back(e);
```

Three loops, all iterate zero times for current consumers. Loop bodies are single-line `push_back`. **Risk is essentially zero;** flagging only for completeness.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The presence of both fields in `ContextCreateInfo` (header at `context.hpp:29, 32`) suggests they were added speculatively for future sims requesting non-default Vulkan capabilities. No banked decision visible from inventory data for when consumer #1 of these might appear.

## Section E: Other unexercised public APIs

### E.1 `Buffer::deviceAddress(VkDevice)` — unexercised

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Per phase 7f, no consumer in rd-3d or eulerian-smoke calls `.deviceAddress(`. Per phase 7e, no sph-water call either. The method at `buffer.hpp:62` is declared public but exercised by zero sims.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Implementation is presumably a thin wrapper around `vkGetBufferDeviceAddress` (visible in `buffer.cpp` per the inventory line counts; not read in detail this round — per-class deep-audit scope). The dependency exists: VMA is created with `VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT` at `context.cpp:350`, and Vulkan 1.2 `bufferDeviceAddress` feature is enabled at `:323`. So the infrastructure for buffer-device-address is present and active in every Context construction, but no sim exercises the surface that retrieves the address.

Risk: low (thin wrapper presumed). Surface presence suggests planned use, perhaps for ray-tracing-style sims (BLAS/TLAS construction uses `VkBufferDeviceAddress`). No banked-design context visible from inventory; flagged for the per-class deep audit.

### E.2 `Buffer::stage(...)` — single-consumer (sph-water only)

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Per phase 7e, sph-water calls `.stage(ctx, ...)` at `main.cpp:2062-2073` (7 distinct call sites). Per phase 7f, neither rd-3d nor eulerian-smoke calls `.stage(`.

`Buffer::stage` is the host→device counterpart to `Buffer::readback`. rd-3d and es don't use DeviceLocal staging buffers; they use `Image::upload(...)` for texture data and likely host-visible buffers for any uniform/SSBO data, so the absence of `Buffer::stage` calls is structurally expected.

But: this means `Buffer::stage` is currently in the same risk shape as alembic_writer was pre-Phase-11 — single-consumer, defects survive until consumer #2.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Risk: medium. Implementation at `buffer.cpp:108-145` (per inventory line counts; not fully audited this round) is the most complex non-trivial implementation in `buffer.cpp` — stage-and-copy via `runOneShot`. Possible defects: race conditions if multiple stages overlap (unlikely; one-shot is fence-synchronous); incorrect VMA-flag combinations for host-visible-sequential staging buffers. Flagged for per-class deep audit.

### E.3 Inferences corrected during the probe

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Per § A.1, I went looking for unexercised `Image::readback` (2-arg form) but the rd-3d call at `main.cpp:706-707` and the es calls at `:1432-1435, 2147, 2210` are 2-arg signatures — matching `Image::readback(void* dst, size_t bytes)` not `Buffer::readback(Context&, void* dst, size_t, size_t)`. So `Image::readback` is exercised by both rd-3d and eulerian-smoke. **Recording the near-miss** because it almost made it into the report as an unexercised finding; Convention #8 caught it on re-read.

## Section F: pNext-chain invariant verification

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
> **CLAIM:** The `compute_pipeline.cpp` implementation correctly satisfies the load-bearing invariant documented at `compute_pipeline.hpp:40-50`.
> **VERDICT: CONFIRMED** via phase 7h side-by-side comparison.

The documented invariant (inventory § B.11, verbatim):

```
INVARIANT (must be preserved by all future maintainers):
  If required_subgroup_size != 0 OR require_full_subgroups == true,
  compute_pipeline.cpp builds VkPipelineShaderStageRequiredSubgroupSize
  CreateInfo and chains it into VkPipelineShaderStageCreateInfo.pNext
  (and/or sets the REQUIRE_FULL_SUBGROUPS_BIT flag). Otherwise pNext
  stays null and the flag stays zero.
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The actual code at `compute_pipeline.cpp:107-120` (create path, from phase 7h):

```cpp
VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroup_size_ci{};
if (desc.required_subgroup_size != 0) {
    subgroup_size_ci.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
    subgroup_size_ci.requiredSubgroupSize = desc.required_subgroup_size;
    subgroup_size_ci.pNext = const_cast<void*>(ss.pNext);
    ss.pNext = &subgroup_size_ci;
}
if (desc.require_full_subgroups) {
    ss.flags |= VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT;
}
```

Reload path at `:198-211` mirrors this with `desc_.` (member access) instead of `desc.` (parameter).

The "and/or" phrasing in the documented invariant could be misread as "always build CreateInfo when either trigger fires," which would yield different semantics than the code. The code semantics are **disjunctive**: `required_subgroup_size != 0` triggers the CreateInfo + pNext-chain branch; `require_full_subgroups` triggers the flag-bit branch. They are independent. The documented invariant is consistent with this interpretation if "(and/or sets the flag)" is read as describing the second independent branch rather than an alternative form of the first.

**Minor documentation clarity issue:** The natural-English phrasing of the invariant is ambiguous. A future maintainer could plausibly read it the wrong way. Worth a small documentation-clarity edit at some point — but **not a code-or-implementation defect.** Both create() and reload() paths handle the invariant correctly as currently implemented.

### F.1 The pNext chain pattern

`ss.pNext = &subgroup_size_ci` chains the extension struct into the shader stage's pNext slot, and `subgroup_size_ci.pNext = const_cast<void*>(ss.pNext)` preserves any pre-existing pNext (defensive — currently `ss.pNext` is always null at that point, so the const_cast preserves null, but the pattern is forward-compatible with future code that pre-populates `ss.pNext` before reaching this block).

**The `const_cast` is mildly concerning** as a code-smell — `VkPipelineShaderStageCreateInfo::pNext` is declared `const void*` in the Vulkan headers, but assigning into `VkPipelineShaderStageRequiredSubgroupSizeCreateInfo::pNext` requires `void*`. The cast is technically correct for Vulkan use (the chain is only read, not written, by the API), but a const-correctness purist would flag it. **Not a defect**; structural quirk worth noting.

### F.2 Stack-lifetime correctness

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The author's comment at `compute_pipeline.cpp:104-106` (visible in phase 7h):

```cpp
// Phase 11 sph-water: subgroup-size-control extension. See INVARIANT in
// compute_pipeline.hpp. The extension struct is stack-allocated; its
// lifetime must span vkCreateComputePipelines below, so it lives in the
// enclosing scope.
```

Confirms that `subgroup_size_ci` is declared in the enclosing scope (not inside the `if` block) so its lifetime persists until `vkCreateComputePipelines` reads through the pNext chain. This is correct — if the struct were inside the `if`, its lifetime would end at the brace and `ss.pNext` would point at a destroyed object.

**Verified.** Same pattern in the reload path.

## Section G: CI coverage cross-check

Per phase 7g, `.github/workflows/build-native.yml` contains:

| Line | Setting |
|---|---|
| 61 | `-DGPU_SIMS_USE_OPENVDB=ON` |
| 62 | `-DGPU_SIMS_USE_ALEMBIC=ON` |
| 110 | `-DGPU_SIMS_USE_ALEMBIC=ON` |

So CI exercises both real-impl compile paths in `alembic_writer.cpp` and `vdb_writer.cpp`. The post-Phase-11 CI state is **comprehensive at compile time** — the unexercised-real-impl defects this probe finds are about *execution* paths through compiled-in code, not about uncompiled code paths.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The pre-Phase-11 CI state was different: per commit `0243278` ("§ 5 cross-cutting edits for sph-water"), `libimath-dev + USE_ALEMBIC=ON` were added to both jobs at Phase 11. Before Phase 11, ALEMBIC=OFF was the CI default, which is why the most-vexing-parse bug at `alembic_writer.cpp:63-65` survived 10 phases of CI without failing.

**Forward CI risk surface:** CI now compiles every code path inside `#if GPU_SIMS_HAVE_*` blocks. New optional-feature code shipping Phase-12+ inside another `#if`-gated path won't have the same blind spot, *provided* the corresponding `-D...=ON` flag is added to CI when the path lands. The lesson from Phase 11 retro category 7 holds: optional-feature CI coverage must be added in the same commit as the consumer code that motivates the feature.

### G.1 Stub path CI coverage

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The `#else` branches in `alembic_writer.cpp:90-96` (StubParticleWriter) and `vdb_writer.cpp:91-93, :141-143` (return-false stubs) compile when `GPU_SIMS_USE_*=OFF`. Phase 7g shows two CI jobs (per `build-native.yml`): one with both flags ON, the other with `USE_OPENVDB` toggled differently (CI yaml not fully cross-quoted; the eulerian-smoke CMakeLists comment at `:5-9` describes the "Stub mode compiles and runs fine" expectation, implying CI exercises stub mode at least sometimes). **Stub-mode compile coverage is plausible but not authoritatively confirmed in this probe**; flagged as a follow-up question if the answer matters for a future audit.

## Section P: Incidental findings and cross-workstream flags

### P.1 **Cross-workstream: ParticleFrame::radii defect** (`cross_workstream: layer-1`)

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Per § B.3. The defect is in common-cpp's public surface but its current real-world impact depends on sph-water's `ParticleFrame fr{};` field-population behavior at `main.cpp:2424-2430` (not visible in Layer 2 probe scope). Architect-1 should be notified to:

1. Determine whether sph-water sets `fr.radii` (one grep at the relevant line block).
2. If yes: surface the silent-data-drop as a Phase 11 retro category-7 instance and either fix sph-water to not set radii (workaround), or fix alembic_writer.cpp to actually use it (real fix).
3. If no: log the defect for fix in a future phase before consumer #2 of `abc::ParticleFrame` exists.

Layer 2 audit does not recommend a specific fix; just surfacing.

### P.2 vdb_writer's `writeFloatGrid` vs `writeVec3Grid` transform-handling divergence

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Per § C.2 item 2. The float-grid path resets the transform inside the origin-translate branch (`vdb_writer.cpp:75-78`); the Vec3 path only post-translates without resetting (`:126-128`). Inconsistent. Could be a deliberate distinction or a copy-paste oversight. Flagged for per-class deep audit when that probe runs.

### P.3 `setObjectName` continues to be an orphan

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Per consumer-mapping report § E.1, `gpusims/vk/debug.hpp` has zero header-include consumers. This probe confirms by extension that `setObjectName(VkDevice, VkObjectType, uint64_t, const char*)` — declared at `debug.hpp:36` — has zero call sites across all 3 Stack C sims. No new finding here; cross-referencing the prior report's open question.

### P.4 Probe-data utilization discipline (`cross_workstream: layer-1`)

Phase 7e enumerated sph-water's symbol usage by targeted greps rather than by full main.cpp read (Layer 1 scope rule). This gave reliable coverage on the specific symbols queried but does NOT cover field-population patterns adjacent to those symbols (the `fr.radii` and `fr.velocities` questions). If a future Layer 2 probe needs reliable per-field exercise data for sph-water-only-consumer surfaces, the cleanest path is for Architect-1 to add the relevant grep results to a Layer-1-authored cross-workstream snippet, rather than for architect-2 to expand the Layer-1 read scope.

### P.5 No further unexercised-real-impl candidates found this round

Architect-2 inspected the full source of three .cpp files (alembic_writer, vdb_writer, context — totaling 724 lines per inventory § A.3) plus the pNext-chain blocks of compute_pipeline.cpp. No other `#if GPU_SIMS_HAVE_*` blocks exist in common-cpp (verified by inventory § C.3 — only OpenVDB and Alembic are optional features). No other `if (info.enable_*)` or `if (info.require_*)` branches beyond those catalogued in § D. Any remaining unexercised paths inside fully-exercised functions would surface only via line-by-line .cpp reads — that's per-class deep-audit scope.

---

## Summary

| Section | Verdict |
|---|---|
| A. Summary table | **8 unexercised paths + 1 verified invariant.** 1 HIGH-risk (defect), 1 HIGH-risk (large unexercised body), 1 MEDIUM (sph-water-dependent), 2 MEDIUM (low-fanout APIs), 3 LOW (flag-gated branches). |
| B. alembic_writer.cpp | **CATALOGUED.** §§ B.3 surfaces the `ParticleFrame::radii` defect — silent data drop in public surface. §§ B.2, B.4 mark velocity/ids branches UNVERIFIABLE without Layer 1 source read. |
| C. vdb_writer.cpp | **CATALOGUED.** §§ C.2 surfaces `writeVec3Grid` as the largest unexercised body in common-cpp; near-miss on a precedence-driven indexing question resolved to NO DEFECT on re-read. § C.2 item 2 flags the transform-handling divergence from `writeFloatGrid`. |
| D. ContextCreateInfo flag-gated branches | **CATALOGUED.** Three low-risk unexercised paths: `require_discrete_gpu=true`, `enable_swapchain=false`, `extra_*_extensions` non-empty. |
| E. Other unexercised public APIs | **CATALOGUED.** Buffer::deviceAddress (zero consumers), Buffer::stage (single-consumer = sph-water). § E.3 records the corrected Image::readback inference. |
| F. pNext-chain invariant | **CONFIRMED** — code implements invariant correctly in both create() and reload() paths. Minor documentation-clarity issue noted, not a defect. |
| G. CI coverage | **POST-PHASE-11 CI IS COMPREHENSIVE** at compile time. Phase 11 retro category 7 risk shape is structurally addressed for compile coverage; execution coverage is the new failure surface. |
| P. Incidentals | 5 entries, 2 flagged `cross_workstream: layer-1` (ParticleFrame::radii defect; probe-data-utilization for future cross-workstream questions). |

**Recommended next Layer-2 probe** (decision-only, no probe load proposed in this report):

**Per-class implementation audit** (deferred since the inventory report). Layer 2 has now established three baselines:
1. Surface inventory (Probe 1).
2. Consumer dependency matrix (Probe 2).
3. Unexercised-real-impl bucket (Probe 3, this report).

The remaining Layer-2 work is per-class deep audit of `.cpp` implementations, prioritized by:

- **High-fanout, high-complexity classes** (3 consumers, largest .cpp): `Context` (407 lines), `GraphicsPipeline` (349), `ComputePipeline` (271), `Image` (246), `Window` (220), `Camera` (216), `HotReloader` (200), `GpuProfiler` (190), `ShaderCompiler` (177), `Buffer` (164).
- **Low-fanout, high-risk classes**: `RealParticleWriter` (alembic_writer.cpp, single-consumer; the radii defect is its first surfaced defect).

The unexercised paths catalogued in this probe should be reviewed for fix-or-document-as-banked in the per-class audit phase.

End of report.
