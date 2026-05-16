---
title: "Integrity v1.3 Closeout Commit 7 — v1-closed marker + bonus cleanups"
date: 2026-05-17
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_closeout_commit6_landing_2026-05-17.md
  - docs/diagnostics/_audits/integrity_v1_3_closeout_probe_2026-05-17_architect1-via-claude-code.md
---

# Integrity v1.3 Closeout Commit 7 — v1-closed marker + bonus cleanups

## § A. Change summary

Three loosely-coupled documentation updates marking the integrity
toolkit v1 milestone closed:

1. **`project-state.md` phase ledger row.** New row `12.5 — Integrity
   toolkit v1 closed` appended at the bottom of § 3 (per spec § 8.C.1
   placement). Status `✅ Done`, Shipped at `<COMMIT_8_SHA>` placeholder
   (resolved by commit 8 of this batch).
2. **`project-state.md` § 8 banked-items list.** Adds an "Integrity
   toolkit v1.x banked-but-not-planned items (post-closeout)"
   subsection enumerating T4 horizon (v2 candidates) and confirming
   T2 / T3 closure status.
3. **`project-state.md` § 9 known-issues banks.** Adds the G.2
   (project-state.md cat1.bare-path suppression non-firing) and G.4
   (`_KNOWN_CATEGORIES` no pinning test) entries with explicit
   "available if forcing function appears" framing. G.2 line numbers
   re-anchored to post-commit-6 (559 / 592 / 664 from probe-time
   560 / 594 / 667).
4. **`tools/integrity/README.md` "Implementation status" → "Status"
   replacement.** Per probe § G.5, the existing checklist showed
   commits 2–8 as unchecked despite all shipping v1.0–v1.2 (~9 months
   stale). Replaced with a current "Status" block stating v1 is
   closed.

## § B. File inventory

| Status | Path | Diff |
|---|---|---|
| Modified | `project-state.md` | +49 LOC (one phase-ledger row, two new § 8/§ 9 sub-sections) / one sweep-companion annotation line |
| Modified | `tools/integrity/README.md` | +7/-9 LOC (replaced 9-line stale checklist with 7-line current status) |
| Modified | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit6_landing_2026-05-17.md` | +2 inline sweep-companion annotations (Convention J carry-over) |
| Created | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit7_landing_2026-05-17.md` | this report |

## § C. Verification

Pre-edit anchoring (HEAD `<COMMIT_6_SHA>`, after closeout commit 6 landed):

```
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 45 hard-fail, 1339 suppressed
```

Audit-prose-freshness sanity check on the updated project-state.md:

```
$ python3 tools/integrity/scripts/audit_prose_freshness.py project-state.md
audit-prose-freshness: checked 3 citations across 1 files (4 skipped as non-repo-local)
audit-prose-freshness: all citations resolve
```

The three checked repo-local citations resolve; the four skipped are
upstream / vendor-style strings the tool deliberately filters out.

Post-edit gate (pre-sweep):

```
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 49 hard-fail, 1339 suppressed
```

+4 hard-fails from the new prose: two grammar-literal mentions of
`integrity-allow:` in audit-doc context (commit-6 audit body refers
to the annotation tokens), one new bare-path citation in
project-state.md's banked-items list, one new cat1.bare-path that
resolves to `other-cat1-bare-path` on project-state.md.

Sweep companion (Convention B):

```
$ python3 tools/integrity/scripts/grandfather_sweep.py
grandfather-sweep: modified 2 files; 4 annotations added
  skipped as live-source (other-cat1): 40 (use --sweep-live-source to include)
         audit-report-grammar-example: 2
                      audit-bare-path: 1
                 other-cat1-bare-path: 1
```

Post-sweep gate:

```
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 46 hard-fail, 1342 suppressed
```

Hard-fail count 49 → 46 (-3, the four annotations absorbed three
findings; the fourth is a new annotation grammar literal in the very
audit it suppresses, which forms a fixed-point but doesn't reduce the
hard-fail count from sweep's perspective). Suppressed grew 1339 →
1342 (+3, matching the absorbed-finding count).

Net change for this commit vs. pre-commit baseline: +1 hard-fail
(45 → 46), expected per Hard Rule 11 ("Hard-fail count may grow by N
per commit if N new audit reports are added that themselves carry
citations").

Final sweep dry-run is clean:

```
$ python3 tools/integrity/scripts/grandfather_sweep.py --dry-run
grandfather-sweep: would modify 0 files; 0 annotations added
```

## § D. Behavioral notes

**Phase ledger row placement.** Per spec § 8.C.1, the new row 12.5
goes at the bottom of the § 3 table (after the `10+` "Remaining sims"
row). This is numerically out-of-order with the rest of the table
(10+ sorts after 12 in the current ordering), but the spec's choice
keeps the closed-marker visually distinct as the closing row of the
ledger. SHA placeholder `<COMMIT_8_SHA>` per Convention #12; commit 8
back-fills.

**Banked-items section content.** Per spec § 8.C.2, T2 items are
explicitly "none (all landed in v1.3 closeout)"; T3 items refer to
the conventions doc disclosure (resolved D4–D7); T4 horizon
enumerates the v2 candidates verbatim from the spec.

**G.2 line-number re-anchor.** Post-commit-6, the
`project-state.md:560 / 594 / 667` annotation references that the
probe report uses no longer apply (commit 6's three deletions
shifted those lines up). § 9's G.2 entry uses the post-commit-6 line
numbers (559 / 592 / 664) and notes the re-anchoring explicitly so a
future debugger lands on the right lines.

**README "Implementation status" replacement.** Per probe § G.5, the
existing 9-line checklist was ~9 months stale. The replacement
8-line "Status" block states v1 is closed, points at the gate
workflow, the conventions doc, and the project-state.md banked-items
list. No commit-by-commit checklist (the phase ledger in
project-state.md is the canonical chronological record).

**Sweep companion shape.** 4 annotations added: 2
`audit-report-grammar-example` (commit-6 audit's body mentions
`integrity-allow:` literally), 1 `audit-bare-path` (some bare-path
citation in the same audit), 1 `other-cat1-bare-path`
(project-state.md added a bare-path citation in the new banked-items
prose). The `other-cat1-bare-path` annotation went into
project-state.md (a SWEEPABLE_EXACT_PATH, so live-source protection
doesn't apply).

## § E. Banked observations

**Gate net +1 per Hard Rule 11.** The closeout-commit growth budget
explicitly accommodates new-audit-doc citations as expected
sweep-companion behavior. +1 is well within that envelope; baseline
45 → 46 with the new audit doc landing.

**No T4 / v2 work triggered.** The banked-items list serves as the
forward-bank for anyone who wants to revisit. The spec's intent
("available if a forcing function appears") is reflected verbatim.

**G.2 still unaddressed.** The `cat1.bare-path` suppression bug on
project-state.md continues to fire 5 HARD_FAILs against
lines 559+1 / 592+1 / 664+1 (i.e., 560 / 593 / 665 post-commit-6).
The banking in § 9 makes the issue forensically discoverable for any
future investigation. Closeout-scope-bounded; out of scope for this
batch.

**G.4 banking is small-scope tee-up.** A future micro-batch can pin
`_KNOWN_CATEGORIES` in one line of test code; the bank ensures the
gap is visible without manually re-discovering it.

## § F. Cross-references

- Spec § 8 (`docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md`)
- Probe § B.8, § B.9, § G.2, § G.4, § G.5
- Convention #12 — commit 8 of this batch resolves the
  `<COMMIT_6_SHA>` and `<COMMIT_8_SHA>` placeholders above
- `tools/integrity/docs/conventions.md` (D4–D7 disclosures referenced)
