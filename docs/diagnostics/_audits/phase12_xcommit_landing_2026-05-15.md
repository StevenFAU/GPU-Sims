---
title: Phase 12 cross-cutting commit landing
date: 2026-05-15
author: claude-code
phase: 12
status: landed
scope: cross-cutting-commit
substantive_anchor: d41564d
---

# Phase 12 cross-cutting commit landing audit

Lands the § 5 cross-cutting modifications immediately after the
substantive commit `d41564d` per the Phase 11 split-commit posture.
Anchors come from architect-1's reanchor patch (the original spec's
§ 5 anchors were 9-of-10 wrong; reanchored against synced HEAD
`c5955d3`).

## Files modified / added

| Path | Kind | Purpose |
|---|---|---|
| `.github/workflows/build-native.yml` | modify | § 5.A — add `volumetric-grid/**` path trigger to both `push:` and `pull_request:` blocks |
| `CMakeLists.txt` | modify | § 5.B — uncomment + de-gate `add_subdirectory(volumetric-grid/lattice-boltzmann)` |
| `README.md` | modify | § 5.D — gallery row status: "Not started" -> "Implemented (Phase 12)" |
| `CHANGELOG.md` | modify | § 5.E — new `[0.13.0]` block above `[0.12.0]` |
| `project-state.md` | modify | § 5.F.1 ledger row, § 5.F.2 sim-list row, § 5.F.3 "Last updated:" line (eight-firings narrative per corrigendum), § 5.F.4 new H3 entry "Frame-invariant capture meta key" |
| `docs/sim-specs/lattice-boltzmann.md` | modify | § 5.C — replace stub with full sim spec |
| `docs/tier1-capture-format-reference.md` | modify | § 5.G.1 latticeBoltzmann row in § 1 table; § 5.G.2 new precedent block in § 2 (placed after eulerian-smoke since the patch's sph-water anchor was a fabrication; corrigendum § A.2) |
| `docs/phase12_lattice_boltzmann_reanchor_2026-05-15.md` | add | architect-1's reanchor patch landed in repo audit trail |
| `docs/diagnostics/_audits/phase12_lattice_boltzmann_reanchor_2026-05-15_corrigendum.md` | add | corrigendum recording two patch-time fabrications + updated 8-firings narrative |
| `docs/diagnostics/_audits/phase12_reanchor_handoff_2026-05-15.md` | add | Claude Code's pre-patch handoff bundle (synced state of all 9 broken anchors) |
| `docs/diagnostics/_audits/phase12_xcommit_landing_2026-05-15.md` | add | this audit |

## § 5 verification grep results (post-edit)

All from architect-1's reanchor patch verification block:

| Sub-edit | Expected | Actual | Result |
|---|---|---|---|
| 5.A | 4 (corrected by corrigendum to 2) | 2 | PASS (corrigendum § A.1) |
| 5.B uncomment | 1 | 1 | PASS |
| 5.B no commented | 0 | 0 | PASS |
| 5.D no Not started | 0 | 0 | PASS |
| 5.D Implemented | 1 | 1 | PASS |
| 5.E `[0.13.0]` header | 1 | 1 | PASS |
| 5.E `[Algebraic_D3Q19]` ref | >=1 | 2 | PASS |
| 5.F.1 Phase 12 row | 1 | 1 | PASS |
| 5.F.1 placeholder count | >=1 | 2 | PASS |
| 5.F.2 Implemented row | 1 | 1 | PASS |
| 5.F.2 stub gone | 0 | 0 | PASS |
| 5.F.3 Last updated Phase 12 | 1 | 1 | PASS |
| 5.F.3 Phase 11 wording gone | 0 | 0 | PASS |
| 5.F.4 new H3 | 1 | 1 | PASS |
| 5.G.1 row | 1 | 1 | PASS |
| 5.G.2 block | 1 | 1 | PASS |

Corrigendum § D step 2 verification greps (after the 5.F.3 update):
- `eight.*Convention #8 architect-fabrications` -> 1 (PASS)
- `six Convention #8 architect-fabrications` -> 0 (PASS)
- `phase12_lattice_boltzmann_reanchor_2026-05-15_corrigendum` -> 1 (PASS)

## Patch-time fabrications surfaced by verification greps

Per the corrigendum (`docs/diagnostics/_audits/
phase12_lattice_boltzmann_reanchor_2026-05-15_corrigendum.md`), two
Convention #8 architect-fabrications surfaced at patch-apply time
(firings #7 and #8 of Phase 12; #1-#6 caught earlier in the
6-checkpoint protocol):

7. **§ 5.A expected grep count of 4 (actual: 2)**. The patch author
   had a wrong mental model of `build-native.yml` — assumed `paths:`
   blocks live per-job; they actually live at trigger level (one per
   `push:` and `pull_request:`, shared across both jobs). The
   substantive edit applied correctly (both blocks now contain
   `volumetric-grid/**`); only the expected count was wrong.

8. **§ 5.G.2 sph-water anchor not present in synced file**. Patch
   anchored against `### sph-water (Phase 11, Stack C) — **special
   case with packed C-style structs**` in
   `docs/tier1-capture-format-reference.md` § 2; that exact heading
   does not exist. The handoff bundle quoted it (perhaps from a
   different file or a transcription error); the patch consumed the
   quotation without independent re-grep. Resolution: new
   `lattice-boltzmann (Phase 12, Stack C)` precedent block placed at
   end of § 2 after the eulerian-smoke block (whose heading was
   confirmed present). Substantive outcome — Phase 12 precedent in §
   2 — is correct; relative position differs from patch intent.

The corrigendum-not-re-issue pattern was followed: when a patch's
substantive outcome is correct but expected verification differs, the
right discipline is corrigendum-with-record, not patch-re-issue.

The cumulative count for Phase 12 is now nine Convention #8 firings
(eight per the corrigendum + the cat3.d3q19-* unregistered-check-IDs
firing surfaced at § 6.C of the substantive commit), all caught
before reaching production code.

## § 6 verification block re-summary (this commit)

- § 6.A: all 10 grep checks PASS (verified against post-§5 tree).
- § 6.B: Release + Debug rebuild PASS (no regression from § 5 edits).
- § 6.C: cat1.upstream-anchor PASS; cat1.upstream-citation pre-
  existing SPlisHSPlasH HARD_FAILs (Phase 11.5 work, not LBM-caused);
  cat3.d3q19-* check IDs unregistered (banked: promote
  `d3q19_verify.py` to integrity checks in a separate cleanup
  commit).

## Pending follow-ups

- **SHA back-fill commit** (Convention #12). Replaces the two
  `<PHASE_12_SHA>` placeholders in `project-state.md` (§ 3 ledger row
  + "Last updated:" line) with `d41564d`. Substantive commit landed
  with stable SHA — back-fill is a separate follow-up commit,
  NEVER `git commit --amend`.
- **Visual verification** (Checkpoint 6) on the RX 6800 XT. Defects
  surfaced sliced into in-flight-fix / Phase 12.5 polish / v1.1 bank.
- **`docs/tier1-capture-format-reference.md` Phase 9 + Phase 10
  backfill** — banked deferred-backfill from Phase 11 retro item 1.
- **`zou_he_d3q19.md` derivation doc + verification harness** — banked
  v1.1 to upgrade equilibrium-boundary inlet/outlet to true Zou-He.
- **Promote `d3q19_verify.py` to registered integrity checks** —
  banked v1.x cleanup (cat3.d3q19-velocity-set / -weights /
  -equilibrium IDs the spec § 6.C asserted exist).
