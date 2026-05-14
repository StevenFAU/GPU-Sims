---
title: Layer 3 Sim Audit Prioritization Probe — triage ranking
date: 2026-05-14
author: probe (Claude)
audience: parent workstream coordinator
scope: triage rank for Layer 3 deep-audits (every shipped sim except `particle-fluids/sph-water/`)
status: read-only — no source modifications, no builds, no binary runs
sibling-layers:
  - Layer 1: `particle-fluids/sph-water/` (in progress; see `phase11_5_*` reports)
  - Layer 2: `common/common-cpp/` (in progress)
  - Layer 3: this probe — per-sim ranking only, no per-sim deep audit
out-of-scope:
  - particle-fluids/sph-water/ (Layer 1)
  - common/common-cpp/ (Layer 2)
  - unimplemented sims (continuous-ca/neural-ca, particle-fluids/pic-flip, quantum/ising-dwave, render-pipelines/houdini, render-pipelines/optix, volumetric-grid/lattice-boltzmann)
  - render-pipelines/blender/ (consumer of sims, not a sim)
---

# Layer 3 sim-audit prioritization

Nine shipped sims surveyed. The triage table is shallow-and-broad per the probe brief; per-sim deep audits are the workstreams this probe schedules, not produces.

## Section A: Inventory and exclusions

**Shipped sims surveyed (9):**

| Category | Sim | Stack |
|---|---|---|
| agent-based | boids-3d | B |
| agent-based | physarum | B |
| closed-form | mandelbulb-explorer | B (Stack-A artifact preserved) |
| closed-form | strange-attractors | B |
| continuous-ca | reaction-diffusion-2d | B (Stack-A artifact preserved) |
| continuous-ca | reaction-diffusion-3d | C |
| continuous-ca | lenia-fft | D (CuPy / Torch / Taichi / numpy backends) |
| hybrid-particle-grid | mpm-multimaterial | D |
| volumetric-grid | eulerian-smoke | C |

**Excluded as unimplemented (README-only, status="Unimplemented"):** continuous-ca/neural-ca, particle-fluids/pic-flip, quantum/ising-dwave, render-pipelines/houdini, render-pipelines/optix, volumetric-grid/lattice-boltzmann.

**Excluded as not-a-sim:** render-pipelines/blender (consumer Python utility for offline Cycles renders).

**Excluded per probe charter:** particle-fluids/sph-water (Layer 1), common/common-cpp (Layer 2).

## Section B: Triage table

| Sim | Stack | Shipped phase(s) | Entry-point LOC | Shader / kernel LOC | Doc total (lbds + notes) | Polish landings (excl. P0/P8.5/P10.1) | Capture / replay |
|---|---|---|---|---|---|---|---|
| eulerian-smoke | C | 8 (`867ea39`) → 8.5 hardening | 2238 (main.cpp) | 813 (GLSL) | 50 + 92 = 142 | 4 (incl. retro, closeout, hardening) | ✓ F5/F9 |
| boids-3d | B | 7 (`38d9ab0`) → 7 polish (`cda37d3`) → 8.5 hardening | 1614 (4 TS files) | 1332 (WGSL) | (no top-level docs/; `web/docs/notes.md` 83 lines) | 3 | ✓ F5/F9 |
| lenia-fft | D | 10 (`7065d32`) → 10 polish-5 (`fb903b3`) → 10.1 restructure (`a81e0d9`) → md-lint (`41781c2`) | 1788 (5 .py files, package) | n/a (Taichi-kernel in `kernels.py`) | 324 + 190 = 514 | 4 | ✓ F5/F9 |
| strange-attractors | B | 2 (`7a4f3f5`) → 8.5 hardening | 1111 (3 TS files) | 367 (WGSL) | 37 + 11 = 48 | 2 | ✓ F5/F9 |
| reaction-diffusion-3d | C | 3 (`d517f02`) → 3 fix (`c805e2b`) → 3.5 hardening (`d8ab610`) | 1021 (main.cpp) | 214 (GLSL) | 42 + 11 = 53 | 3 (incl. dt-halve fix) | ✓ F5/F9 |
| mpm-multimaterial | D | 9 (`50b8c2d`) → 5 polish commits → 10.1 restructure | 958 (4 .py files) | n/a (Taichi-kernel in `kernels.py`) | 254 + 101 = 355 | 10 | ✓ F5/F9 + buttons |
| physarum | B | 6 (`1250971`) → 6 retro (`c36c731`) → 8.5 hardening | 942 (2 TS files) | 400 (WGSL) | 168 + 149 = 317 | 3 | ✓ F5/F9 |
| reaction-diffusion-2d | B | 5 (`ed54dd3`) → 5 fix (`e1f0673`) → 8.5 hardening (`9c2f900` / `35bc0fa`) | 808 (3 TS files) | 182 (WGSL) | 58 + 43 = 101 | 4 | ✓ F5/F9 |
| mandelbulb-explorer | B | 4 (`8d8334f`) → 8.5 hardening (`9c2f900` / `35bc0fa`) | 627 (main.ts) | 219 (WGSL) | 25 + 91 = 116 | 3 | ✓ F5 |

### Banked items (raw counts and qualitative notes)

| Sim | Banked / v1.1 / stub markers (src) | Documented v1.1 backlog items (notes.md) | Notable banked-followup quotes |
|---|---|---|---|
| eulerian-smoke | 2 (`v1.1`) | 14 prioritized | "Active density+temperature drain at open ceiling" (1.0′) — known boundary-condition workaround tuned via dissipation; "Diagnose actual cause of sustained fan load (was: VSync default — retracted)" (1.12) — explicit retraction of falsified attribution; "Unified VDB recording UX" (1.14) — paired density/temperature recording blocked end-to-end Blender validation in Phase 8 |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| lenia-fft | 7 (`banked`) + 4 (`stub`) + 7 (`v1.1`) | 6 v1.1 polish items + extensive polyring banking | "Polyring kernel extension banking (v1.1+)" — claims `Chakazul/Lenia/Python/LeniaNDK.py:329-335` defines polyring assembly (precise upstream line cite, no vendored references); "GGUI Y-convention asymmetry" — `cursor_to_field_cell` and `cursor_in_any_panel` use different Y conventions, both work empirically, neither understood |
| mpm-multimaterial | 1 (`banked`) + 4 (`stub`) + 1 (`v1.1`) | 9 v1.1 stretch + perf-polish + UX-polish | "1M @ 192³ stretch tier — stability investigation" — particle-NaN explosion on unpause, dt scaling banked; "Substep count tuning vs. upstream" — `SUBSTEPS_PER_FRAME=25` is 2.5× upstream mpm3d_ggui's typical 10, drafted without measuring; "F-key save/load keybinding investigation" — F5/F9 don't fire on AMD desktop + Taichi Vulkan + X11, polish-3 added buttons |
| physarum | 2 (`v1.1`) | 6 priority-1 + 4 priority-2 + 2 priority-3 + 3 promotion candidates | "GPU-side reset compute kernel" (1.1) — 10M-tier reset hitches UI 200–500 ms; "Per-pass timing missing" — no `GpuProfiler`, contingent on common-web profiler rework (project-state.md § 9); "Alignment-change-breaks-silently flag (Agent struct)" — audit-on-field-add note |
| boids-3d | 0 (terms occur only in `web/docs/notes.md`) | 11 priority items (1.0 → 1.11) | "GPU-side reseed kernel" (1.4) — 100k-tier 3.2 MB upload visible as UI hitch; "Conditional sort skipping" (1.3) — measurement-banked 20–30% throughput win; "Reservoir-1 sampling" (1.5) — current modulo-pick biased, banked because stochastic mode loses bit-exact replay determinism; "Smooth tier transitions" (1.10) — 50–100 ms hitch on tier change |
| reaction-diffusion-2d | 0 | 8 v1.1 items + Pearson preset tuning TBDs | "Pearson preset tuning observations — TBD after first run-through" (presets λ/σ/α/β/ξ/τ visual verification all "TBD"); "τ U-skate preset flagged suspect in Phase 3 spec" |
| reaction-diffusion-3d | 0 | 0 (notes.md is "Suggested headings to add later" placeholder; 11 lines) | none documented — notes.md is **empty placeholder** |
| strange-attractors | 0 | 0 (notes.md is "Suggested headings to add later" placeholder; 11 lines) | none documented — notes.md is **empty placeholder** |
| mandelbulb-explorer | 0 | 4 trap-coloring v1.1 candidates | "Spec defects encountered during Phase 4 execution" — two anchor mismatches against `deploy-pages.yml`, adapted on the fly |

### Fabrication smell (quick scan, not deep)

| Sim | Upstream / SHA references | Vendored at `references/`? | Smell |
|---|---|---|---|
| eulerian-smoke | Stam 1999, Fedkiw 2001, Selle/Fedkiw 2008, Bridson 2008, McAdams/Sifakis/Teran 2010 — standard textbook refs | no | **LOW** — standard refs; but "full Fedkiw-2001 solver stack" packs many distinct components into one phase. Priority-1.12 v1.1 entry **explicitly documents a falsified attribution** (present-mode → fan-load) with a retraction trail — high fabrication-discipline maturity in the docs, but the underlying load-pattern remains uncharacterized. |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| lenia-fft | `Chakazul/Lenia/Python/LeniaNDK.py:329-335` (precise upstream line citation); `animals.json` "byte-for-byte verified" claim; "architect-2 round-2 verified" qualifier | **NO** (only `references/SPlisHSPlasH/` is vendored) | **HIGH** — same shape as the sph-water `1.8.10` SHA pattern that Setup-1 found fabricated: precise-line-number citation against a non-vendored upstream, with verification-claim qualifiers. Independently of correctness, the citation is unverifiable from the working tree. |
| mpm-multimaterial | "canonical Taichi MLS-MPM upstream `taichi-dev/taichi: python/taichi/examples/ggui_examples/mpm3d_ggui.py`"; Klar et al. 2016 (Drucker-Prager); `SUBSTEPS_PER_FRAME=25 vs upstream's typical ~10` (acknowledged not measured) | no | **MODERATE** — upstream cite is plausible (Taichi ships the named example); `SUBSTEPS_PER_FRAME=25` self-admits to confident-recall sourcing without measurement. Tier-recal from 250k/96³→128k/64³ already happened post-visual-verification, so a category of fabrication-discipline has been exercised here. |
| reaction-diffusion-3d | Pearson 1993 (`Science` 261:189–192); preset names from Munafo's catalog | no | **LOW** — standard refs; flagged in `notes.md`: "τ (U-skate): TBD — Phase 3's spec flagged this as suspect (canonical U-skate is reportedly nearer F ≈ 0.062). Cross-check Munafo's catalog if patterns don't match expectation." Preset-tuning verification banked. |
| reaction-diffusion-2d | Pearson 1993, Munafo's catalog (same preset table as rd-3d, by spec design) | no | **LOW** — inherits rd-3d's tuning suspicion; `notes.md` lists 6 presets with "TBD" under "Pearson preset tuning observations". |
| physarum | Jones 2010 (`Artificial Life` 16(2)) | no | **VERY LOW** — single, well-known published model. |
| boids-3d | Reynolds (1987, canonical) | no | **VERY LOW** — no specific SHA/version pin. |
| mandelbulb-explorer | "iq's site is mostly CC BY-NC-SA — math implemented from scratch" (load-bearing decision #1) | no | **VERY LOW** — explicit fresh-implementation posture; no upstream pin. |
| strange-attractors | "Phase 2: First Stack B sim" — no specific upstream pin | no | **VERY LOW** — no upstream pin. |

### common-cpp consumption (Stack C only)

| Sim | Distinct `gpusims/...` includes | List |
|---|---|---|
| eulerian-smoke | **17** | `camera`, `gpu_profiler`, `hot_reload`, `imgui_setup`, `log`, `state_reader`, `state_writer`, **`vdb_writer`**, `vk/buffer`, `vk/compute_pipeline`, `vk/context`, `vk/frame`, `vk/graphics_pipeline`, `vk/image`, `vk/renderer`, `vk/shader_compiler`, `vk/window` |
| reaction-diffusion-3d | **16** | same as eulerian-smoke minus `vdb_writer` |

Both Stack C sims consume effectively the entire `common-cpp` public surface. **Layer 2 findings will cascade into both**; schedule the deep audits to follow Layer 2's landing. eulerian-smoke is the first/only `vdb_writer` consumer in the repo (per project-state.md § 5), so it has additional independent surface that Layer 2 won't cover.

### Stack-specific concerns

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- **Stack B sims (WGSL):** zero `TODO`/`FIXME` markers across all five sims' WGSL files. v1.1 references in source occur only in `physarum/web/src/main.ts:139, 445` and `eulerian-smoke/shaders/raymarch.frag.glsl:208` (a "real blue noise" v1.1 swap-candidate comment). No skeletal shader docblocks detected.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- **Stack C sims (GLSL):** zero `TODO`/`FIXME`/`placeholder`/`skeleton` markers. eulerian-smoke has 2 `v1.1` references in source (the noise comment above + a "split into two uniform buffers" follow-up at `main.cpp:1789`).
- **Stack D sims (Taichi `@ti.kernel`):** mpm-multimaterial's `kernels.py` and lenia-fft's `kernels.py` contain zero TODO markers. v1.1 references in source: 1 in mpm, 7 in lenia (mostly in `fft_backend.py` re: round-trip optimizations).

### Capture / replay — universal

Every shipped sim implements F5/F9 capture/replay. mpm-multimaterial additionally ships explicit "save state" / "load latest" panel buttons (Phase 9 polish-3, `49c0559`) because F-key event dispatch didn't fire on the user's AMD desktop + Taichi Vulkan + X11 setup — that's a banked unresolved investigation in `mpm-multimaterial/docs/notes.md`.

## Section C: Priority-ranked audit list

Rationale-driven ranking. Stack C sims are flagged for "schedule after Layer 2 lands" so common-cpp findings can cascade in; their **first-pass priority** in the queue is interleaved with non-Stack-C sims that can be audited immediately.

### 1. `volumetric-grid/eulerian-smoke` (audit AFTER Layer 2 lands)

The flagship Tier-2 Stack C, first OpenVDB consumer, first Blender Cycles offline-render integration. Largest single entry point in the repo at 2238 LOC `main.cpp` (more than 2× rd-3d, also Stack C); 813 LOC of GLSL across the Fedkiw-2001 solver stack (MacCormack advection, vorticity confinement, Jacobi pressure, buoyancy, raymarch). Consumes 17 distinct `common-cpp` headers including the never-elsewhere-exercised `gpusims/vdb_writer.hpp` (first-exercise note at project-state.md § 5). 14 documented v1.1 polish items in `notes.md`, including priority-1.12 which **explicitly documents an architect-1 falsified attribution** (present-mode → fan-load, retracted after `vblank_mode=3` test) and priority-1.0′ which documents a **known boundary-condition workaround** (ceiling outflow missing; mitigated via dissipation tuning). Five named-paper references compressed into one solver stack heightens audit value. **Schedule:** wait for Layer 2; expect Layer 2 findings to cascade into this audit (17/17 includes).

### 2. `continuous-ca/lenia-fft` (audit-now)

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Largest Stack D doc footprint in the repo (`load-bearing-decisions.md` 324 lines + `notes.md` 190 lines = 514 doc lines), reflecting 4-backend FFT-selection complexity (CuPy / Torch-CUDA / Torch-ROCm / Taichi-real-space / numpy — four implementations of the same `LeniaConvolver` interface). Highest "banked" marker density (7 in source + extensive doc banking). **Fabrication smell is the highest-priority concern:** the polyring-extension banking quotes a precise upstream line citation (`Chakazul/Lenia/Python/LeniaNDK.py:329-335`) against a **non-vendored upstream** — the same structural shape as the Phase 11 sph-water `SPlisHSPlasH 1.8.10` SHA that Setup-1 found fabricated, modulo qualifier ("architect-2 round-2 verified"). The "GGUI Y-convention asymmetry" (notes.md § Polish-4) is a banked unexplained-behavior flag of the kind a deep audit should resolve. 1788 LOC across the package. Two-machine deployment (AMD Vulkan + NVIDIA CUDA) doubles user-facing verification surface. Audit can run independently of Layer 2.

### 3. `hybrid-particle-grid/mpm-multimaterial` (audit-now)

Heaviest polish history in the field — **10 polish commits post-ship**, including the polish-5 reserve-tail allocation fix (`8a33211` + `e73b4c8`), which was a correctness-class repair, not cosmetic: the pre-polish-5 LMB-place destructively re-tagged preset particles ("a magic transmuter that no MLS-MPM equation supports"). 254-line `load-bearing-decisions.md` documents three deliberate divergences from upstream spec (water+jelly+snow vs. spec's sand+jelly+water; PLY vs. Alembic for particle export; tier recalibrated from 250k/96³ → 128k/64³ post-visual-verification). **The 1M / 192³ tier remains banked due to particle-NaN explosion on unpause** — a real instability the audit can characterize. `SUBSTEPS_PER_FRAME=25` self-admits to confident-recall sourcing 2.5× upstream's typical 10. F5/F9 keybinding investigation banked unresolved. 958 LOC + Taichi kernels. Audit can run independently of Layer 2.

### 4. `agent-based/boids-3d` (audit-now)

Largest Stack B sim at 2946 LOC combined (1614 TS + 1332 WGSL — more WGSL than any other Stack B sim by 3×). 11 prioritized v1.1 items including non-trivial correctness/UX work: GPU-side reseed kernel (1.4), conditional sort-skipping with cross-frame cell-displacement bookkeeping (1.3), reservoir-1 sampling to fix biased stochastic-prey selection (1.5), bit-exact-from-frame-1 replay (1.8), smooth tier transitions to remove 50–100 ms hitch (1.10). **Anomaly:** the sim has no top-level `docs/` folder (other shipped sims do); v1.1 notes live at `web/docs/notes.md`. No `load-bearing-decisions.md` exists for this sim — the rule-of-three "second sparse-source consumer" pattern decisions (project-state.md row Phase 7) live only in the phase spec. The audit should determine whether load-bearing decisions for this sim are documented anywhere consumable. Audit-now (Stack B).

### 5. `agent-based/physarum` (audit-now)

**Pattern-progenitor** for three load-bearing repo conventions (per its own 168-line lbds): first sim without a Stack-A artifact, first `atomic<u32>` storage-buffer consumer, first sparse-source point-emitter pattern (food-pin), first discrete agent-count tier dropdown. 17 prioritized v1.1 items across four priority tiers including a documented audit-on-field-add flag for the `Agent` struct (lbds § "Alignment-change-breaks-silently"). 942 TS + 400 WGSL. As progenitor, drift here cascades into the second-consumer sims (boids-3d) and the still-banked third-consumer promotion review. Audit-now.

### 6. `continuous-ca/reaction-diffusion-3d` (audit AFTER Layer 2 lands)

Stack C; 16 `common-cpp` includes (everything except `vdb_writer`). First Stack C sim, shipped Phase 3 (May 9 — among the oldest in the field, so more time to accumulate drift). 1021 LOC main.cpp + 214 GLSL. **Empty notes.md** ("Suggested headings to add later" placeholder of 11 lines): either nothing has surfaced, or no one has revisited the sim since Phase 3.5 — the audit can't distinguish without reading source. Two real fix commits in history (`c805e2b` dt-halve for Forward Euler stability; `d8ab610` Phase 3.5 hardening). Pearson preset τ flagged suspect by Phase 3 spec; preset tuning verification still TBD per the rd-2d sister sim's notes.md. **Schedule:** wait for Layer 2; cascade is expected.

### 7. `closed-form/strange-attractors` (audit-now)

Oldest sim in the field (shipped Phase 2, `7a4f3f5`, May 9). 1111 TS + 367 WGSL. **`notes.md` is essentially empty** (11 lines of placeholder headings). 37-line `load-bearing-decisions.md` is the shortest non-mandelbulb doc. Implementation has not been revisited beyond the Phase 8.5 hardening sweep (`9c2f900`). The audit value here is *staleness*: a Phase-2 sim that hasn't surfaced any banked items in 6+ months is either rock-solid or under-documented. The probe can't tell which. Audit-now.

### 8. `continuous-ca/reaction-diffusion-2d` (audit-now)

Stack-A → B port (multi-file Stack A artifact). 808 TS + 182 WGSL. 8 v1.1 items in notes.md, modest scope. The six Pearson presets have **all six visual-verification observations marked TBD** in `notes.md`, including the τ (U-skate) preset that Phase 3's spec flagged as suspect. That's a documented hole; whether the visual verification has happened since ship is unclear from the doc trail. Audit-now.

### 9. `closed-form/mandelbulb-explorer` (audit-last)

Smallest sim in the field: 627 LOC `main.ts` + 219 LOC WGSL, render-only (no compute ping-pong). 4 trap-coloring v1.1 candidates well-scoped. `notes.md` documents **two spec-anchor mismatches during Phase 4 execution** (against `deploy-pages.yml`), already resolved on-the-fly. Math implemented from scratch (load-bearing decision #1), so no third-party-shader fabrication surface. First multi-architect-cross-review chain landed against this sim, so its cross-review residue is already in the spec trail. Lowest expected audit yield. Audit-last.

## Section D: Suggested execution batches

Independent of the 1..9 ranking, work can be batched along these axes:

- **Batch A (Stack C, post-Layer-2):** eulerian-smoke (rank 1), reaction-diffusion-3d (rank 6).
- **Batch B (Stack D, audit-now):** lenia-fft (rank 2), mpm-multimaterial (rank 3). Both depend on `common-py` (which Layer 2 doesn't cover) and share Stack D conventions (`@ti.kernel` AST inspection, runtime backend selection, F-key keybinding question).
- **Batch C (Stack B agent-system, audit-now):** physarum (rank 5), boids-3d (rank 4). Boids-3d is the second consumer of physarum's sparse-source pattern; auditing them together amortizes the agent-system mental model and may unlock the still-banked third-consumer promotion review.
- **Batch D (Stack B closed-form / 2D-grid, audit-now):** strange-attractors (rank 7), reaction-diffusion-2d (rank 8), mandelbulb-explorer (rank 9). Smaller surfaces; lower expected yield; can run as a single sweep when capacity allows.

End of probe.
