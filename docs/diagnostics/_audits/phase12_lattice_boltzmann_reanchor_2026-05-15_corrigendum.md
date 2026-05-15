# Phase 12 reanchor patch — corrigendum 2026-05-15

> **Companion to:** `docs/phase12_lattice_boltzmann_reanchor_2026-05-15.md` (the § 5 reanchor patch).
> **Date:** 2026-05-15 (same day as the patch; surfaced at patch-apply verification).
> **Author:** architect-1.
> **Status:** records two fabrications surfaced by the patch's own verification greps. No re-issue of the patch text; file state matches patch intent in both cases.

---

## § A — What the verification greps surfaced

When Claude Code applied the § 5 reanchor patch, two of the verification greps produced unexpected results. The substantive file state matched patch intent in both cases; the patch text contained two distinct Convention #8 architect-fabrications that the verification greps caught.

### § A.1 — § 5.A expected count was wrong

**Patch claim:** `grep -c "volumetric-grid/\*\*" .github/workflows/build-native.yml` returns 4 ("two jobs × push/pull_request").

**Actual:** returns 2.

**Root cause:** the patch author (architect-1) had a wrong mental model of the workflow file's structure. The actual file has `paths:` blocks at the **trigger level** (one `push:` block, one `pull_request:` block), shared across both `build-ubuntu` and `build-ubuntu-debug` jobs. Not duplicated per-job. So the correct expected count is 2, and the file substantively contains the LBM path trigger in both trigger types — exactly what § 5.A intended.

**Fix:** none required to the file. The patch's verification grep expected count is corrected here: **expected = 2**.

### § A.2 — § 5.G.2 anchor was not present in the synced file

**Patch claim:** anchor `### sph-water (Phase 11, Stack C) — **special case with packed C-style structs**` exists in `docs/tier1-capture-format-reference.md` § 2.

**Actual:** that exact heading text does not appear in the synced file. The handoff bundle from Claude Code at `docs/diagnostics/_audits/phase12_reanchor_handoff_2026-05-15.md` quoted the heading verbatim; the patch consumed the quotation as truth.

**Root cause:** architect-1 consumed an anchor string from the handoff bundle without independent re-verification against the synced file. Either the handoff captured the heading with a typo/wrong rendering, or the file was edited between handoff and patch-apply, or the heading text differs subtly. This is a refinement of Convention #8: "memory of a probe is not a substitute for re-grepping the probed surface at spec-lock time" — and consumed-from-a-handoff is a special case of "memory of a probe." The handoff is Claude Code's output but the patch consuming it is architect-1's responsibility.

**Resolution applied by Claude Code:** the new `### lattice-boltzmann (Phase 12, Stack C)` precedent block was placed at the end of § 2, after the eulerian-smoke block (whose heading was confirmed present), since the sph-water anchor couldn't be located. The substantive outcome — a Phase 12 precedent block in § 2 of `tier1-capture-format-reference.md` — is correct. The relative position is slightly different than the patch specified (after eulerian-smoke rather than after sph-water).

**Banked for the deferred-backfill bank**: the actual heading style of § 2 precedent blocks is now an unknown to architect-1. When Phase 9 and Phase 10 precedent blocks are backfilled in a separate cleanup commit (per Phase 11 retro item 1), the actual heading-style convention should be verified against the eulerian-smoke and sph-water blocks as they actually appear in the file.

---

## § B — Updated Convention #8 firing count

The reanchor patch's § 5.F.3 narrative enumerates six Convention #8 architect-fabrications caught during Phase 12. This corrigendum surfaces two more. The count is **8 firings**, not 6. The full list:

| # | Fabrication | Where caught | Class |
|---:|---|---|---|
| 1 | Krüger D3Q19 scope claim (the GitHub languages bar said "C++ 72%, CUDA 7%"; I asserted D3Q19; actual repo is D2Q9 only) | Probe-2 | Pre-spec |
| 2 | § 5 cross-cutting anchor strings (9 of 10 anchors were wrong) | Checkpoint 5 | Spec-write |
| 3 | common-cpp API surface (`renderer.acquireNextFrame`, `ImguiSetup` class, etc.) | Checkpoint 2 | Spec-write |
| 4 | Zou-He 3D inlet/outlet closure equations (transcription errors in the boundary closure formulas) | Checkpoint 5 deep | Spec-write |
| 5 | `writeVec3Frame` API name (actual: `writeVec3Grid`) | Checkpoint 5 | Spec-write |
| 6 | `docs/phase11_sph_water.md` location (actual: `docs/sim-specs/sph-water.md`) | Prep-3 landing | Prep-write |
| 7 | § 5.A expected grep count of 4 (actual: 2) | Patch-apply verification (this corrigendum) | Patch-write |
| 8 | § 5.G.2 sph-water heading anchor (not present in synced file) | Patch-apply verification (this corrigendum) | Patch-write |

**All 8 caught before reaching production code.** The 6-checkpoint protocol plus the audit-trail discipline plus the verification greps in the patch caught every one. The architecture is working.

**Observation worth banking:** five of the eight were "spec-write" fabrications (writing the spec from memory of a probe rather than re-grepping). Two were "patch-write" fabrications (writing the reanchor patch from a wrong mental model + consuming a handoff bundle without re-verification). One was "pre-spec" (the original Krüger anchor claim).

The pattern is **stable across phases of the work**: pre-spec, spec-write, patch-write are all fabrication-prone in the same way. Memory of a probe is not a substitute for re-grepping at any of those stages. The Convention #8 refinement banked at the prep-2 audit ("memory of a probe is not a substitute for re-grepping the probed surface at spec-lock time") should be extended to: **"...at spec-lock time, patch-write time, or any other moment where the architect is producing a load-bearing artifact that asserts repo-state facts."**

---

## § C — Updated § 5.F.3 narrative

The reanchor patch's § 5.F.3 produced the following "Last updated:" line:

```
> **Last updated:** end of Phase 12 — `lattice-boltzmann` Stack C volumetric-grid sim, D3Q19 BGK around a NACA airfoil at a 256×128×128 desktop default tier. Substantive commit at `<PHASE_12_SHA>`; three prep commits (`8fe355b` Krüger vendoring, `0db9c73` algebraic D3Q19 derivation + verification harness, `c5955d3` architect-1 spec) landed before substantive. Phase 12 is the second Stack C volumetric-grid sim (after Phase 8 eulerian-smoke), the first sim consuming algebraic-derivation ground-truth via `tools/integrity/docs/algebraic/d3q19.md` (the `[Algebraic_D3Q19]` registry pattern, paired with the vendored `[Krueger]` MIT math reference), and the first real consumer of `gpusims::vdb::writeVec3Grid` post-Phase 8's `writeFloatGrid`. The single-architect-plus-checkpoints execution model was validated this phase: six Convention #8 architect-fabrications (Krüger D3Q19 scope at probe-2; § 5 anchor strings, common-cpp API surface, Zou-He boundary closure, `writeVec3Frame` API name, `docs/phase11_sph_water.md` location) were all caught by the 6-checkpoint protocol or by prep-landing audits before reaching production code. Equilibrium-distribution boundary shipped as v1 fallback for Zou-He; Zou-He 3D banked for v1.1 with algebraic derivation doc + verification harness at `tools/integrity/docs/algebraic/zou_he_d3q19.md` (to be created). **Next phase:** TBD per coordinator; remaining sims at § 6.
```

**Replace with** (8 firings, updated narrative):

```
> **Last updated:** end of Phase 12 — `lattice-boltzmann` Stack C volumetric-grid sim, D3Q19 BGK around a NACA airfoil at a 256×128×128 desktop default tier. Substantive commit at `<PHASE_12_SHA>`; three prep commits (`8fe355b` Krüger vendoring, `0db9c73` algebraic D3Q19 derivation + verification harness, `c5955d3` architect-1 spec) landed before substantive. Phase 12 is the second Stack C volumetric-grid sim (after Phase 8 eulerian-smoke), the first sim consuming algebraic-derivation ground-truth via `tools/integrity/docs/algebraic/d3q19.md` (the `[Algebraic_D3Q19]` registry pattern, paired with the vendored `[Krueger]` MIT math reference), and the first real consumer of `gpusims::vdb::writeVec3Grid` post-Phase 8's `writeFloatGrid`. The single-architect-plus-checkpoints execution model was validated this phase: **eight** Convention #8 architect-fabrications were caught before any reached production code — Krüger D3Q19 scope at probe-2; § 5 anchor strings, common-cpp API surface, Zou-He 3D boundary closure, and `writeVec3Frame` API name (all caught at Checkpoints 2 / 5); `docs/phase11_sph_water.md` location (caught at prep-3 landing); § 5.A expected grep count and § 5.G.2 sph-water anchor (both caught at reanchor-patch-apply verification, recorded in `docs/diagnostics/_audits/phase12_lattice_boltzmann_reanchor_2026-05-15_corrigendum.md`). The fabrication pattern is stable across pre-spec / spec-write / patch-write stages, all rooted in "memory of a probe is not a substitute for re-grepping at any artifact-production moment" (Convention #8 refinement). The 6-checkpoint protocol plus verification greps plus audit-trail discipline caught all eight; the architecture is load-bearing. Equilibrium-distribution boundary shipped as v1 fallback for Zou-He; Zou-He 3D banked for v1.1 with algebraic derivation doc + verification harness at `tools/integrity/docs/algebraic/zou_he_d3q19.md` (to be created). **Next phase:** TBD per coordinator; remaining sims at § 6.
```

The difference: six → eight, plus the named additions of the two patch-time fabrications, plus the explicit reference to this corrigendum doc, plus the meta-observation that the fabrication pattern is stable across all artifact-production stages.

---

## § D — How Claude Code applies this corrigendum

1. **Land this file** at `docs/diagnostics/_audits/phase12_lattice_boltzmann_reanchor_2026-05-15_corrigendum.md` in the repo.
2. **Apply the § 5.F.3 update** (replace the existing "Last updated:" paragraph in `project-state.md` with the corrected version in § C above). Verify via grep:
   ```
   grep -c "eight.*Convention #8 architect-fabrications" project-state.md
   # Expected: 1.
   grep -c "six Convention #8 architect-fabrications" project-state.md
   # Expected: 0 (the old six-count wording is replaced).
   grep -c "phase12_lattice_boltzmann_reanchor_2026-05-15_corrigendum" project-state.md
   # Expected: 1 (the new wording references this corrigendum).
   ```
3. **Update the cross-cutting commit's audit report** (`docs/diagnostics/_audits/phase12_xcommit_landing_2026-05-15.md`, to be authored by Claude Code at § 7.B.1 land time) to include a section noting that the § 5.A and § 5.G.2 verification greps each surfaced a patch-time fabrication, with reference to this corrigendum.
4. **Proceed** to the rest of § 6 verification (the 10 grep checks from the original spec) and § 7 commits.

The corrigendum is part of the cross-cutting commit, not a separate commit. It's a small honest record, not a procedural ceremony.

---

## § E — Banked for retro

- **Convention #8 refinement extended**: "memory of a probe is not a substitute for re-grepping at spec-lock time, patch-write time, or any other moment where the architect is producing a load-bearing artifact that asserts repo-state facts."
- **Eight Phase 12 fabrications, all caught.** This is a strong validation signal for the single-architect-plus-checkpoints model AND a strong indictment of the architect's fabrication rate. Both findings are real. Phase 12 retro item 9 should engage with the tension: the protocol works, AND the architect's discipline is the bottleneck.
- **The corrigendum-not-re-issue pattern**: when a patch's substantive outcome is correct but the patch's expected verification result is wrong, the right discipline is corrigendum-with-record, not patch-re-issue. Re-issuing would be precious. Corrigendum is right-sized.
- **Handoff-bundle anchors are not a substitute for grep.** Architect-1 consumed `docs/diagnostics/_audits/phase12_reanchor_handoff_2026-05-15.md` as if its quoted anchors were verified. Two were. Eight were. Hard to know without grep. Future handoff bundles either (a) include the grep command and output that produced each anchor for trivial re-verification by the patch author, or (b) get re-greped by the patch author at patch-write time regardless. Bank as convention candidate.
