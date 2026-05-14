# Phase 11 — Deferred ledger-backfill items

**Surfaced during:** Phase 11 sph-water spec-revision pass (2026-05-13) and final-line-citation re-anchor probe (post-09c0d9f).

**Scope of this document:** Pre-existing ledger drift surfaced when Phase 11's § 5 cross-cutting edits hit the actual repo state. None of these defects were caused by Phase 11; all pre-date Phase 11 and would have surfaced eventually when any subsequent phase tried to touch the same files. Documenting now while the evidence is fresh.

**Phase 11's posture per coordinator decision (Option A, 2026-05-13):** Phase 11 § 5 edits add only sph-water-related changes. The backfill items below are explicitly out-of-scope for Phase 11's follow-up commit. They get their own cleanup commits (or get absorbed into a future phase's § 5 if that phase already touches the same files).

---

## 1. `docs/tier1-capture-format-reference.md` — § 1 table is two phases stale

**Probe evidence:** Re-anchor probe (post-09c0d9f) found § 1's sim-namespace table ends at Phase 8 (eulerian-smoke / `eulerianSmoke`). Both Phase 9 (mpm-multimaterial) and Phase 10 (lenia-fft / lenia-fft-v2) shipped without adding their rows.

**What's missing:**

- Row for `mpm-multimaterial` (Phase 9, Stack D Taichi, top-level meta key TBD — read from the Phase 9 substantive commit)
- Row for `lenia-fft` / `lenia-fft-v2` (Phase 10 / Phase 10.1, Stack D Taichi, top-level meta key `lenia`)
- Phase 11 adds the `sph-water` row (sphWater) per § 5.C of the Phase 11 spec — that part lands in Phase 11's § 5 follow-up

**Resolution path:**

A separate `chore: backfill phases 9 + 10 in tier1-capture-format-reference.md` commit that adds the two missing rows. The work requires reading each phase's actual shipped capture-schema (top-level meta key, any per-buffer conventions worth noting in § 2's precedent block).

**Estimated effort:** ~15-30 min including verification against each phase's actual `state.json` output.

**Why this happened:** Process gap. The "every shipped sim adds its row" convention exists implicitly but isn't enforced anywhere (no CI check, no checklist in the phase-completion template). Two phases shipped without firing the convention. Worth banking as a Phase 11 retro item: **add ledger-row-update as an explicit check in the phase-completion template, or write a CI check that diffs the sim-list against the table.**

---

## 2. `CHANGELOG.md` — version-link footer skips nine versions

**Probe evidence:** Re-anchor probe found the version-link footer at the bottom of CHANGELOG.md lists only `[Unreleased]`, `[0.6.0]`, `[0.5.0]`, `[0.4.0]`, `[0.3.0]`, `[0.2.1]`, `[0.2.0]`, `[0.1.0]`. Body sections exist for `[0.7.0]` through `[0.11.0]` but compare-link footer entries do not. `[Unreleased]` link still compares from `v0.6.0...HEAD`.

**What's missing:**

Compare-link entries for nine versions: `[0.7.0]`, `[0.8.0]`, `[0.9.0]`, `[0.9.1]`, `[0.9.2]`, `[0.9.3]`, `[0.10.0]`, `[0.10.1]`, `[0.11.0]`. Plus `[Unreleased]` needs re-anchoring from `v0.6.0...HEAD` to `v0.11.0...HEAD` (or `v0.12.0...HEAD` after Phase 11's § 5 follow-up lands).

**Resolution path:**

A separate `chore: backfill changelog version-link footer` commit. Each entry is one line of the form:

```
[X.Y.Z]: https://github.com/<owner>/GPU-Sims/compare/v<previous>...v<X.Y.Z>
```

Sequence each entry against the actual tag history. If some intermediate versions weren't tagged (e.g., 0.9.1 / 0.9.2 / 0.9.3 patches), check `git tag -l 'v0.*'` against the body-section list and add only the entries that have matching tags.

**Estimated effort:** ~10 min once the tag list is confirmed.

**Why this happened:** Same process gap as #1. CHANGELOG footer maintenance is per-release manual; the convention exists but enforcement doesn't. Phase 11 retro should consider whether `release-please` or a similar tool would fit the project's release cadence (probably no — releases are infrequent and the per-phase rhythm doesn't map cleanly to semantic versioning).

---

## 3. `project-state.md` § 6 — sph-water row is two phases stale

**Probe evidence:** Re-anchor probe found § 6 sph-water row at line 191 still says "Sim-spec stub; flagship sim — likely Alembic consumer; Phase 5 candidate."

**What's wrong:**

- "Phase 5 candidate" — sph-water landed as Phase 11, not Phase 5. The roadmap has shifted at least six phases since this row was written.
- "Sim-spec stub" — the spec exists; "stub" is no longer accurate.
- "likely Alembic consumer" — confirmed first Alembic consumer in Phase 11.

**Resolution path:**

Phase 11's § 5.F follow-up commit can fix this row in passing if it touches § 6 (currently the spec doesn't, since § 5.F was scoped to header / ledger / common-cpp surface / latest-commit pointer). Two options:

- **Bundle into § 5.F follow-up:** add a fifth edit to § 5.F that updates the line-191 row. Low cost, clean scope-addition.
- **Defer as separate sim-list backfill:** if § 6 has other stale rows for sims that shipped (Phase 9 mpm-multimaterial, Phase 10 lenia-fft) that weren't updated post-ship, the cleanup is broader and benefits from its own commit.

**Recommended:** check during § 5.F follow-up writing whether § 6 has only the sph-water row stale or whether other shipped sims also have stale rows. If just sph-water, bundle into § 5.F. If broader, defer.

**Estimated effort:** 5 min if bundled, 15-20 min if broader cleanup.

---

## 4. Process gap — ledger maintenance discipline

**Pattern:** Three separate locations in the repo (`tier1-capture-format-reference.md` § 1, `CHANGELOG.md` footer, `project-state.md` § 6) all drifted from reality across Phases 9 + 10 without anyone noticing. The Phase 11 spec assumed these locations were current; reality is they hadn't been touched.

**Root cause:** Convention-without-enforcement. The "every shipped sim updates the ledgers" convention exists (it fired in Phases 1-8) but doesn't have a checklist, CI gate, or template item. Phases 9 + 10 shipped under time pressure and the ledger updates fell off.

**Recommended retro item:** Add ledger-update verification to the phase-completion template — either as a manual checklist or as a CI check that diffs the shipped sim count against rows in each ledger location.

**Phase 11 specifically:** The probe-before-draft-lock convention being banked at Phase 11 retro will catch *future* spec drift against repo state, but won't fix *existing* drift that's been on disk for two phases. Different problem, different fix. Worth surfacing both at retro as related-but-distinct gaps.

---

## 5. `common-cpp` subgroup-size-control surface design

**Status: DONE.** Landed at `9e0ca2f` (Turn 3 of Phase 11 follow-up sequence). Comprehensive surface probe ran first (saved to `/tmp/phase11_commoncpp_surface_probe.md`); design pass produced `phase11_commoncpp_subgroup_size_surface.md`; Claude Code executed it.

**What shipped:**

- `ContextCreateInfo::enable_subgroup_size_control` named bool (default false)
- `Context::subgroupSizeMin()` / `subgroupSizeMax()` / `requiredSubgroupSizeStages()` / `subgroupSizeControlEnabled()` accessors
- `ComputePipelineDesc::required_subgroup_size` / `require_full_subgroups` fields with sentinel-default backward-compat semantics
- `common-cpp/examples/hello/main.cpp` smoke test (`--test-subgroup-size` flag) — proof-of-life independent of consumer code
- Pre-flight feature-support check inside `createDevice` (moved before `f13.subgroupSizeControl = VK_TRUE` to give descriptive throw rather than letting vkCreateDevice fail with generic message)

**Verified on RX 6800 XT:** min=32, max=64, stages=0xe0; pinned compute pipeline with `required_subgroup_size=32` created cleanly.

**Original notes preserved below for historical context:**

**Probe evidence:** Phase 11 substantive commit at 09c0d9f deferred adding the `Context::enable_subgroup_size_control` field + `subgroupSizeMin()`/`subgroupSizeMax()` accessors + `ComputePipelineDesc::required_subgroup_size`/`require_full_subgroups` fields. Reason cited in completion report: "non-trivial work that warrants its own follow-up commit rather than bundling into the scaffold."

**Status (historical):** Was NOT a process gap; was an authorized in-flight surface change per Phase 11 spec § 0 hard rule 5. Was listed here to keep all deferred-from-Phase-11 work in one tracking doc.

**Scope question to resolve before writing:**

How does `enable_subgroup_size_control` fit common-cpp's existing init-feature pattern? Specifically:

- Does common-cpp use a request struct, a builder pattern, or designated-initializer pattern for feature requests?
- Does `Context::createDevice()` build the device-features chain from a fixed list or from a request?
- Does `ComputePipelineDesc` already accept a `pNext` chain, or do we add one?

The completion report flagged this as a "substantive API addition that propagates through Context::createDevice() and ComputePipeline::create() internals." The Turn 3 design pass should probe common-cpp's actual surface before writing the addition.

**Required pre-design probe:**

```bash
# Context init pattern
grep -n "Context::create\|enable_\|VkPhysicalDeviceFeatures\|VkPhysicalDeviceVulkan13Features" \
    common/common-cpp/include/gpusims/vk/context.hpp \
    common/common-cpp/src/vk/context.cpp

# ComputePipelineDesc shape
grep -n "ComputePipelineDesc\|VkPipelineShaderStageCreateInfo\|VkComputePipelineCreateInfo" \
    common/common-cpp/include/gpusims/vk/compute_pipeline.hpp \
    common/common-cpp/src/vk/compute_pipeline.cpp
```

That probe surfaces the actual API shape; design pass writes against it.

**Estimated effort:** Probe ~5 min, design ~30 min, implementation in common-cpp ~50-100 lines plus tests.

---

## 6. Main.cpp DFSPH dispatch chain + render passes

**Status: DONE.** Landed at `1f02fc1` (Turn 4 of Phase 11 follow-up sequence). main.cpp grew 630 → 2290 lines (+1660 LOC, slightly over the 1500 spec estimate due to uniform-packing helpers + descriptor-write boilerplate).

**What shipped:**

- 17 descriptor-write helpers (covering all 21 shaders; density_solve + divergence_solve share; particle_sprite + thickness share)
- `TierResources` struct + `createTierResources` / `destroyTierResources` functions
- 15 compute pipelines (7 subgroup-size pinned per § 2.3 of the Turn 4 spec; 8 unpinned) + 3 graphics pipelines (depth, thickness with additive blend, composite)
- HotReloader watches on all 21 shaders with main-loop flag-driven pipeline reload
- F5/F9 capture-save/load lambdas — bare buffer names, sphWater meta key, camera state via `toJson(json&)` / `fromJson(const json&)`
- Per-substep DFSPH dispatch chain (apply_emitter → 8-stage counting-sort → density_alpha → divergence-Jacobi inner loop → pressure_apply → integrate_forces → density-Jacobi inner loop → pressure_apply) with `gv::memoryBarrier` between phases (sync2 flags) and `profiler.scope` RAII per pass
- 5-pass screen-space fluid render: depth pass (off-screen direct `vkCmdBeginRendering`) → `bilateral_smooth` × N (compute ping-pong) → thickness pass (off-screen direct) → composite + ImGui to swapchain via `renderer.beginRendering`
- `GpuProfiler::beginFrame` / `endFrame` / `drawImGui` wiring
- Alembic `writeFrame` call site every N substeps, gated on `abc::isAvailable() && !rt.paused`
- Tier-change apply path (waitIdle → destroy + recreate → re-write all descriptors → seed initial_fill)

**In-flight common-cpp additions (authorized per Phase 11 spec § 0 hard rule 6):**

- `Buffer::readback(Context&, void* dst, std::size_t bytes, std::size_t offset = 0)` — symmetric to `stage()`; synchronous staging through host-visible intermediate. Required by F5 save + Alembic export.
- `GraphicsPipelineDesc::{src,dst}_{color,alpha}_blend_factor` + `{color,alpha}_blend_op` — defaults preserve historical alpha-blend; thickness pass opts into ONE/ONE additive.

**Spec adaptations surfaced per § 0 hard rule 1 (banked in retro discussion item 7 below):** seven adaptations to architect-1's spec, three of which were probe-data-non-utilization (the Turn 4 surface probe had the correct surface info; the spec was drafted from memory anyway).

**Original notes preserved below for historical context:**

**Status (historical):** ~1500 LOC of host-side wiring deferred from Phase 11 substantive commit.

**Why deferred (historical):** Synced common-cpp API drift from spec's assumptions. `Context::create(cdesc)` / `Window::create({...})` / `Camera::lookAt` / `Camera::setFov` don't exist on the actual common-cpp surface. The shipped 09c0d9f commit's main.cpp used the real API (`Context()` default ctor, `Window(Context&, w, h, title)`, `Camera::setOrientation`, `Camera::setFovDeg`) and surfaced the drift in the completion report.

---

## Phase 11 follow-up commit sequencing

After all the above is on disk, the Phase 11 follow-up work breaks into commits roughly in this order:

1. **`feat(common-cpp): subgroup-size-control surface for Stack C consumers`** — implements Item 5 above. Lands first because main.cpp wiring depends on it.

2. **`feat(phase11): sph-water DFSPH dispatch chain + screen-space fluid render`** — main.cpp wiring (Item 6). Consumes the common-cpp surface from commit #1. This is the largest commit by LOC.

3. **`feat(phase11): cross-cutting edits for sph-water`** — the re-anchored § 5 edits (CI yaml, optional_deps FetchContent swap, README, CHANGELOG, project-state.md). Lands last so its `<PHASE_11_SHA>` placeholders point at the substantive commits above.

4. **`chore: backfill phases 9 + 10 in tier1-capture-format-reference.md`** — Item 1 cleanup.

5. **`chore: backfill changelog version-link footer`** — Item 2 cleanup.

6. **`chore(project-state): update § 6 sim-list rows for shipped sims`** — Item 3 cleanup (if scoped broader than just sph-water).

Commits 4-6 are independent of each other and of the Phase 11 follow-up; they can land in any order or be bundled into a single `chore: ledger backfill across phases 9 + 10` commit if you prefer.

---

## Retro discussion items

Banked through Phase 11's full execution cycle (substantive scaffold at `09c0d9f` → Turn 3 at `9e0ca2f` → Turn 4 at `1f02fc1` → Turn 2 at `0243278` → SHA-backfill follow-up). The first five items are the original mid-phase retro framing; items 6–10 are end-of-phase additions surfaced during execution. (The fabrication-shape category taxonomy, originally item 6 in this list, has been promoted to its own top-level section below.)

### 1. Probe-before-draft-lock convention promotion

Phase 11 fired multiple probe shapes catching distinct fabrication defects: pre-spec-lock probes (DFSPH formulas, Alembic packaging, `saveBuffer` signature, buffer-naming, pre-execute-handoff line citations), § 5 re-anchor probe (post-09c0d9f), Turn 3 comprehensive common-cpp surface probe, Turn 4 pre-spec probe, Turn 2 anchor re-check probe. Each probe pass caught defects the prior pass missed.

**Promote to standalone § 7 H3 in `project-state.md`.** Already landed in Phase 11's Turn 2 commit (`0243278`) per § 5.G.2 of the Turn 2 spec.

### 2. Iterated-probe pattern

Each probe pass caught defects the prior pass missed. Bank the framing: **partial probes give partial confidence, not full confidence by extrapolation.** Architect-1 should either probe every anchor or explicitly mark unprobed ones as architect-memory-grounded.

**Operational rule:** if a probe verifies N of M anchors and the remaining M-N are extrapolated, flag the extrapolated set explicitly so the next probe pass knows where to look. The Phase 11 § 5 anchor drift surfaced because architect-1 verified 5 anchors with a probe and assumed the remaining 3 were also fine; reality is 7 of 8 had drifted.

### 3. Ledger-maintenance process gap

Items 1, 2, 3 of this document are all symptoms of the same root cause — convention-without-enforcement for ledger maintenance (`tier1-capture-format-reference.md` § 1 table, `CHANGELOG.md` version-link footer, `project-state.md` § 6 sim list). Three locations all drifted across Phases 9 + 10 without anyone noticing.

**Recommended fix:** add ledger-update verification to the phase-completion template (manual checklist) OR write a CI check that diffs shipped sim count against rows in each ledger location. Manual checklist is the lower-cost option and probably enough — the phase-completion rhythm is slow enough that a checklist gets consulted.

### 4. Spec scope vs single-execution capacity

Phase 11's substantive spec (`phase11_sph_water.md`) was 6100 lines — the largest single spec the project has shipped. Most of the work-in-this-commit went into shaders (well-bounded inline code in the spec); main.cpp's by-reference-to-ES sections + the synced-API drift left ~1500 LOC of main.cpp wiring deferred to follow-up commits.

**Banking:** future Phase-N-tier specs should consider splitting into:

- **Architectural framing** (~1500-2000 lines): the design decisions, conventions, hard rules, file manifest, cross-cutting edits. Written at draft-time.
- **Per-subsystem specs** (~500-1000 lines each): the detailed wiring for each subsystem. Written *after probe-grounding* the specific surfaces that subsystem touches. May be written in parallel with execution of earlier subsystems.

Phase 11's de-facto split into substantive scaffold + Turn 3 + Turn 4 + Turn 2 + SHA-backfill is an *ad hoc* version of this pattern. The cleaner version would have planned the split at spec-draft time.

### 5. Architect-1 fabrication shape: aspirational API state

The synced-API drift in main.cpp's by-reference-to-ES sections (`Context::create`, `Window::create`, `Camera::lookAt`, `Camera::setFov`) is the same root cause as § 5 anchor drift. Architect-1 drafted against an imagined common-cpp surface state.

**Convention extension:** "Architects read the actual hello-world source before drafting" → "Architects probe the entire common-cpp surface the spec calls into — not just the hello example, not just the APIs flagged as risky — before drafting wiring code." Landed in Phase 11's Turn 2 commit per § 5.G.2.

---

### 6. Convention 4 (rule-of-three runs in reverse) banking for off-screen rendering

**Banked at Turn 4 execution:** Phase 11 is consumer #1 of off-screen multi-pass rendering in Stack C. `Renderer::beginRendering` was NOT extended to support off-screen / multi-attachment / depth-attached passes. Phase 11 calls `vkCmdBeginRendering` directly in `particle-fluids/sph-water/src/main.cpp`.

**Re-evaluation trigger:** consumer #2 of off-screen multi-pass rendering in Stack C. At that point, the two usage patterns provide enough data to extract a shared abstraction into common-cpp's Renderer surface.

**Candidate consumer-#2 sims:** PIC/FLIP (particle-grid hybrid; needs depth-pass for particle splatting + grid-write pass), Lattice-Boltzmann if 3D visualization adds a screen-space-fluid-style render. Neither is concrete in the roadmap.

**Banked in:** `particle-fluids/sph-water/docs/load-bearing-decisions.md` per Phase 11 spec § 4.J convention. Wording per architect-2's framing 2026-05-13: *"Direct vkCmdBeginRendering usage in main.cpp's screen-space-fluid render is intentional. Renderer::beginRendering not extended for off-screen multi-pass rendering at this consumer; second Stack C consumer of the pattern triggers the abstraction-promotion review per Convention 4."*

### 7. Consumer-#3 re-evaluation trigger for `ContextCreateInfo` named-toggle pattern

**Banked at Turn 3 design:** the named-toggle pattern (`enable_subgroup_size_control = false`) was adopted at consumer #1 per rule-of-three discipline. The forward-thinking element isn't "named toggle vs pNext extensibility"; it's banking the consumer-#3 evaluation explicitly so future architect-1 doesn't repeat the exercise.

**Trigger condition:** consumer #3 of any Stack C Vulkan feature toggle.

**Plausible features that may motivate consumer-#2 / consumer-#3 toggles within 2-3 phases:** `shaderInt64`, `bufferDeviceAddress`, `cooperativeMatrix`, `hostQueryReset`, `shaderSubgroupExtendedTypes` (per architect-2 analysis 2026-05-13).

**Question to answer at trigger:** "Does the named-bool list still scale cleanly, or has the cognitive cost of `ContextCreateInfo` field count exceeded the convenience of named access? At what number of feature toggles does pNext-extensibility (consumer builds the feature chain, common-cpp splices) become the cleaner pattern?"

**Architect-2's load-bearing framing (2026-05-13):** *"bank the question, not just the answer."* Without explicit banking, consumer #3 arrives later, nobody remembers the consumer-#1 decision was deliberate, and the named-toggle pattern accidentally ossifies into convention.

### 8. Consumer-#1-of-new-common-cpp-surface pattern (two-commit shape)

**Banked at Turn 3 design:** every consumer-#1 of a new common-cpp surface ships in two commits:

1. **Surface-addition commit** — common-cpp adds the new API, includes a `hello/main.cpp` smoke test exercising the surface in isolation. Ships green independently.
2. **Consumer commit** — the sim that motivated the addition consumes it. Ships against a known-good surface baseline.

**Why this matters:** without the smoke test in commit #1, the first execution of the new API happens at commit #2's consumer code, conflating "is the surface right?" with "is the consumer code right?" into one debugging surface. With the smoke test, those failure modes are isolated.

**Phase 11 demonstrated this pattern:** Turn 3 (`9e0ca2f`) added the subgroup-size-control surface with a `hello` smoke test (`--test-subgroup-size`). Turn 4 (`1f02fc1`) consumed it. Both commits shipped green independently. The pattern paid off: when Turn 4 surfaced spec adaptations during execution, none of them were "the common-cpp surface doesn't work" — they were all consumer-code-level issues.

### 9. Common-cpp surface-extension idiom (sentinel-default backward-compat)

**Pattern surfaced across Turn 3 + Turn 4:** new fields on existing common-cpp structs use sentinel defaults that preserve existing behavior; conditional construction fires only when non-default values are requested.

**Examples:**

- `ComputePipelineDesc::required_subgroup_size = 0` (0 = unconstrained; existing default-null pNext path preserved). Conditional `pNext`-chain construction only fires when `required_subgroup_size != 0 || require_full_subgroups == true`. **Invariant documented in `compute_pipeline.hpp`** with "future maintainers must preserve this invariant" wording per architect-2's sharpening 2026-05-13.
- `GraphicsPipelineDesc::{src,dst}_{color,alpha}_blend_factor` + `{color,alpha}_blend_op` (defaults preserve historical alpha-blend; thickness pass opts into ONE/ONE additive via non-default values).

**Why this matters:** backward-compatibility by construction. Every existing consumer continues to work without code changes; only consumers that actively opt into the new behavior get the new code path. Eliminates the class of "we added a feature and accidentally broke six existing sims" defects.

**Bank as common-cpp surface-extension convention:** any new field on an existing common-cpp struct that controls behavior-changing construction must use a sentinel default that preserves existing behavior. The conditional that gates the new construction must be documented as a load-bearing invariant.

### 10. Closing-commit anchor-validity re-check pattern

**Banked at Turn 2 anchor-recheck probe (2026-05-13):** the closing commit of a multi-commit phase gets an anchor-validity re-check, not just trust-from-implied-isolation.

**Why this matters:** Turn 2's § 5 anchors were verified at HEAD `09c0d9f` (the substantive scaffold landing). By Turn 2 ship-time, HEAD was `1f02fc1` (two follow-up commits later). The plausible assumption was "Turn 3 + Turn 4 only touched common-cpp + sph-water; § 5 anchor files weren't touched, so anchors still valid." The re-check probe confirmed this — but also surfaced one anchor (line 191 prose) that had been approximate even at the original probe.

**Cost:** ~5 min of probe time. **Benefit:** closes the loop on probe-before-execute discipline at the highest-stakes moment of the phase (the closing commit).

**Operational rule:** when a multi-commit phase has anchors that span files OUTSIDE the active commit chain, re-check anchor validity before the closing commit ships. The re-check is a thin probe — just dump current anchor content at each location and compare against the spec.

---

## Fabrication-shape category taxonomy

End-of-Phase-11 retro framing per the discussion 2026-05-13 (architect-1) and architect-2 confirmation. Distinguishing the categories helps future architects route defects to the right discipline rather than treating all fabrication as one bucket.

**Category 1 — Anchor drift.** Wrong file paths, wrong line numbers, stale anchors. Surfaced examples:
- § 5 anchor drift across 7 of 8 subsections (pre-Turn-2 re-anchor probe)
- SPlisHSPlasH upstream directory `DFSPH/` not `TimeStep/` (pre-execute-handoff probe)
- `optional_deps.cmake` Alembic block at lines 53-63 not 54-72 (pre-execute-handoff probe)
- `project-state.md` § 6 sph-water row at line 191 prose was approximate; actual line was elsewhere (Turn 2 anchor re-check)

**Discipline:** anchor by verbatim content match, not by line number. If a spec must reference a line number for orientation, mark it as approximate.

**Category 2 — API surface drift.** Imagined function signatures. Surfaced examples:
- `Context::create(cdesc)` — reality is `Context()` default ctor + `ContextCreateInfo{}` designated-init
- `Window::create({...})` — reality is `Window(Context&, w, h, title)`
- `Camera::lookAt`, `Camera::setFov` — reality is `setOrientation(yaw_deg, pitch_deg)` + `setFovDeg(deg)`
- `Camera::toJson` returning a json — reality is `void toJson(json&)` mutating in place
- `saveBuffer` 5-arg form — reality is 4-arg `(name, data, bytes, meta={})`

**Discipline:** comprehensive common-cpp surface probe at spec-draft time. Per architect-2's framing 2026-05-13: probe every API the spec calls, not just the ones flagged as risky.

**Category 3 — Schema drift.** Wrong table column counts, wrong ledger schemas. Surfaced examples:
- `tier1-capture-format-reference.md` § 1 table — spec assumed 3 columns; actual is 4
- `project-state.md` § 3 ledger — spec assumed 4 columns; actual is 5
- `README.md` gallery table — spec assumed `Sim | Stack | Category` order; actual is `Sim | Category | Stack | Status`

**Discipline:** for any markdown table being edited, dump the actual header row before writing the replacement row. Don't infer schema from "similar tables."

**Category 4 — Sequencing drift.** Code in the right place but in the wrong order. Surfaced examples:
- Turn 3 pre-flight feature-support check placed AFTER `vkCreateDevice` — but `vkCreateDevice` fails first with a generic error message, swallowing the descriptive throw. Claude Code moved the check before the `f13.subgroupSizeControl = VK_TRUE` line during execution.

**Discipline:** when writing init-sequence code, ask "what's the earliest point at which this check can fire?" If the check is to give a descriptive error, it must fire before any Vulkan call that would produce a generic error for the same condition. Probe coverage cannot catch this category; it's a property of how the new code interacts with surrounding code.

**Category 5 — Convention drift.** Pre-existing process gaps not caused by the current phase but surfaced when the current phase touches the affected file. Surfaced examples:
- `tier1-capture-format-reference.md` § 1 table missing Phase 9 + Phase 10 rows (two phases of ledger drift, surfaced by Phase 11)
- `CHANGELOG.md` version-link footer missing 9 entries (`[0.7.0]` through `[0.11.0]`)
- `project-state.md` § 6 sph-water row still says "Phase 5 candidate" (drift across at least 6 phases)

**Discipline:** add ledger-update verification to phase-completion templates. The drift is a process gap, not a fabrication; the fix is process discipline.

**Category 6 — Probe-data non-utilization.** Probe HAD the correct surface info; spec was drafted from memory anyway. **This is a new category surfaced by Turn 4 execution and worth banking explicitly.** Surfaced examples:
- Turn 4's `ComputePipeline::dispatch(cmd, ds, gx, gy, gz)` combined bind+dispatch — probe Task 3 line 1097 dumped the ES verbatim usage; the Turn 4 spec wrote `pipe.bind(cmd, ds); vkCmdDispatch(...)` anyway
- Turn 4's `gv::memoryBarrier` using sync2 flags — probe Task 5 explicitly said `srcStage2, srcAccess2, dstStage2, dstAccess2`; the Turn 4 spec used legacy `VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT` etc.
- Turn 4's `Context::runOneShot` return type — probe Task 1 listed the helper; the Turn 4 spec assumed it returned a `VkCommandBuffer`
- Turn 2's "line 191 sph-water row" prose — earlier probe gave the line approximately; spec treated it as authoritative

**Distinction from Category 2:** in Category 2, architect-1 didn't have the probe data and worked from memory. In Category 6, architect-1 *had* the probe data and worked from memory anyway. Different mechanism, different fix.

**Discipline:** when drafting a section, keep the probe report open and consult it for the specific API/anchor/pattern being referenced. Architect-1's habit of "I just read the probe, I remember the relevant bits" doesn't survive the gap between probe and section-write. Operationally: paste relevant probe excerpts inline into the spec draft as you go, not just at the start.

**Category 7 — Verification-block self-inconsistency.** Verification grep counts derived from architect-1's intent rather than from the spec's own prescribed replacement text. Surfaced examples (Turn 2 execution):
- § 5.B grep expected `c254caf2…` count = 1; the spec's own replacement text contains it twice (comment + GIT_TAG line). Two is correct.
- § 5.B grep expected `libalembic-dev` count = 0; the spec's own replacement text contains a "libalembic-dev was dropped from Ubuntu 24.04 noble" historical-context comment. One is correct.

**Discipline:** verification grep counts must be derived from the spec's prescribed replacement text, not from architect-memory of "what the new state should look like." Practical rule: write the replacement text first, then derive the expected count by inspection.

**Category 8 — Shared-resource consumer-layout drift.** Multiple consumer shaders (or other clients) read the same buffer with disagreeing field-offset assumptions. Each consumer's UBO/SSBO block looks internally well-formed and validates clean; the bug lives entirely in the cross-consumer disagreement.

- Phase 11 example: five DFSPH compute kernels shared `tier.uniform_dfsph` but each declared an independent std140 `uniform U { ... }` block. Four of the five expected `domainMin_cellSize` at byte 48; the host wrote it at byte 80. Only `integrate_forces` matched the host. The other four read garbage past offset 24, producing velocity corrections that happened to cancel gravity. Fixed by commit 83a01d6.
- Spatial sibling of Category 9 (temporal mid-frame-varying-field drift). Same shared-buffer architecture; different failure axis.

**Discipline:** when multiple shaders read a shared buffer, the field layout is part of the buffer's contract and must be reconciled explicitly. Spec for any shared resource between N consumers needs a field-layout reconciliation step: union the referenced fields, pick a canonical order, mirror that order on the host packing side, and gate with a `static_assert(sizeof(Layout) == EXPECTED_BYTES)`. A grep-based check that all consumers' UBO blocks are character-identical (modulo binding number) is a cheap structural gate. Validation alone does not catch this — each consumer's block is locally valid.

**Category 9 — Shared host-mapped UBO with mid-frame-varying field.** A uniform buffer holds a field that needs to take different values at two different GPU execution points within the same frame, written by host memcpy into a persistently-mapped UBO. The host writes both values; both GPU reads happen after queue submission, so both reads see whichever value the host wrote last. Validation passes — no field is out of range, no descriptor is malformed, no synchronization primitive is misused (because none was present in the first place).

- Phase 11 example: the substep loop called `pack_dfsph_uniform(_, 0.0f)` before the loop and `pack_dfsph_uniform(_, 1.0f)` mid-loop to toggle `integrate_forces` between FORCES and POSITION_ONLY modes. Both GPU dispatches read mode=1; POSITION_ONLY ran twice per substep; gravity never applied; particles never moved. Fixed by commit 7294ee4 (move mode out of UBO into a push constant).
- Temporal sibling of Category 8 (spatial consumer-layout drift). Categories 8 and 9 can stack: in Phase 11, category 8 hid category 9 because the UBO was unreadable until layouts agreed, so the mode-toggle bug couldn't be observed until the canonical layout landed.
- Anti-pattern smell: `pack_*_uniform()` called twice within a substep loop body with different arguments, reusing the same descriptor set between two `dispatch()` calls. Each occurrence should be reviewed.

**Discipline:** for any field that varies per-dispatch, prefer push constants — recorded into the command buffer, so each dispatch carries its own value regardless of host write timing. Precedent in this codebase: `pipe_prefix_sum_block`. For larger varying payloads, use `UNIFORM_BUFFER_DYNAMIC` with per-dispatch offsets. Detection cannot be purely static; the Tier-2 particle-system diagnostic spec should include a capture mode that records UBO contents per-dispatch (not per-frame).

---

## Phase 11 closing summary

**Commit chain (5 commits, all on `main`):**

```
09c0d9f → 9e0ca2f → 1f02fc1 → 0243278 → <SHA-backfill>
substantive   Turn 3      Turn 4       Turn 2       backfill
scaffold +    common-cpp  main.cpp     § 5 cross-   placeholder
21 shaders    subgroup    DFSPH chain  cutting      fill
+ docs        size ctrl   + SSF render edits
```

**LOC totals:**

- `09c0d9f`: +2775 / −11 (substantive scaffold; 21 shaders + docs + Blender hero script + stubbed main.cpp)
- `9e0ca2f`: ~+145 (common-cpp Context + ComputePipelineDesc additions + hello smoke test)
- `1f02fc1`: +1755 / −55 (main.cpp 630 → 2290 lines + Buffer::readback + GraphicsPipelineDesc blend fields)
- `0243278`: § 5 cross-cutting edits across 6 files
- SHA-backfill: 1-line replacement

**Total Phase 11 LOC delta (substantive code, excluding docs):** ~4700 LOC across the five commits.

**Probe-pass count:** 6 distinct probes ran during Phase 11.

- Pre-spec-lock probes (DFSPH formulas, Alembic packaging, etc.)
- § 5 re-anchor probe (post-09c0d9f)
- Turn 3 comprehensive common-cpp surface probe
- Turn 4 pre-spec probe
- Turn 2 anchor re-check probe
- (Smoke tests on RX 6800 XT — `min=32, max=64, stages=0xe0` verification at multiple stages)

**Fabrication-shape category instances surfaced:** at least one instance of each of the first 7 categories above during the substantive commit chain; categories 8 and 9 were surfaced during Phase 11 fix-forward (commits 83a01d6 and 7294ee4) and banked into the taxonomy after the fact. The 9-category taxonomy is now considered stable; future phases can route defects to the right discipline rather than treating fabrication as one bucket.

**Conventions banked during Phase 11 (now in `project-state.md` § 7):**

- Probe-before-draft-lock discipline (sharpening of fabrication-discipline) — Turn 2 § 5.G.2
- Stack C source-builds for VFX/graphics libraries when distro packaging unreliable — Turn 2 § 5.G.1

**Conventions banked here in this document (for future § 7 promotion):**

- Convention 4 trigger for off-screen multi-pass rendering in Stack C (re-evaluation at consumer #2)
- `ContextCreateInfo` named-toggle pattern re-evaluation at consumer #3 of any Stack C Vulkan feature toggle
- Consumer-#1-of-new-common-cpp-surface pattern (two-commit shape with hello smoke test)
- Common-cpp surface-extension idiom (sentinel-default backward-compat with documented invariants)
- Closing-commit anchor-validity re-check pattern
- Verification-grep-count derivation discipline (counts derived from prescribed replacement text)
- Probe-data utilization discipline (paste probe excerpts inline while drafting, not just read-at-probe-completion)

**Still pending (the original deferred ledger items, Items 1-3 of this doc):**

- Phase 9 + Phase 10 row backfill in `tier1-capture-format-reference.md` § 1
- CHANGELOG footer backfill for `[0.7.0]` through `[0.11.0]` compare-links
- Other stale § 6 sim rows in `project-state.md` (mpm-multimaterial, lenia-fft, possibly others)
- One small adjacent staleness surfaced during Turn 2 anchor re-check: `project-state.md` line 77 ("10+ Remaining sims" row) still describes sph-water as "the natural Alembic-real-impl consumer" — post-Phase-11 this is technically backward-looking. Tiny adjacent cleanup; bundle with the broader § 6 cleanup or ignore.

These are all `chore:` commits, low-priority, can land any time. None blocks Phase 12 planning.

**Phase 11 is fully shipped. Ready for retro and Phase 12 planning when you are.**
