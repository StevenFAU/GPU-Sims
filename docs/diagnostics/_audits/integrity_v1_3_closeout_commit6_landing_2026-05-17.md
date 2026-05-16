---
title: "Integrity v1.3 Closeout Commit 6 — project-state.md fossil cleanup"
date: 2026-05-17
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_closeout_commit5_landing_2026-05-17.md
  - docs/diagnostics/_audits/integrity_v1_3_closeout_probe_2026-05-17_architect1-via-claude-code.md
---

# Integrity v1.3 Closeout Commit 6 — project-state.md fossil cleanup

## § A. Change summary

<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
Removes three `integrity-allow: cat1.intra-repo;` annotations from
`project-state.md` at probe-time lines 559, 593, 666 per Part-B retro
Decision 6 / probe § E.3 / probe § K.6. These annotations carry the
`other-cat1` reason string but have no backing findings: the
underlying `cat1.intra-repo` findings either resolved (the cited
paths were fixed) or migrated to `cat1.bare-path` after A.3
introduced that check. The probe confirmed exactly zero
`cat1.intra-repo` findings fire on `project-state.md` at any line,
which makes these annotations fossils by every operational
definition.

The adjacent `cat1.bare-path` `other-cat1-bare-path` annotations at
probe-time lines 560 / 594 / 667 are **not** deleted. Probe § G.2
established that those annotations are not actually suppressing the
`cat1.bare-path` findings at the next line (5 HARD_FAILs continue to
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
fire against `project-state.md:561 / 595 / 668`); banking the bug as
a known issue is the closeout-scope answer (lands in commit 7). They
remain in place because removing them could silently make the gate
worse if a future suppression fix activates them.

## § B. File inventory

| Status | Path | Diff |
|---|---|---|
| Modified | `project-state.md` | -3 LOC (three HTML-comment annotation lines deleted) |
| Created | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit6_landing_2026-05-17.md` | this report |

## § C. Verification

Pre-edit anchoring (HEAD `3d25ddc`, after closeout commit 5 landed):

```
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 45 hard-fail, 1339 suppressed
$ grep -n "integrity-allow.*other-cat1\b" project-state.md
559:<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
560:<!-- integrity-allow: cat1.bare-path; bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog); n/a -->
593:<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
594:<!-- integrity-allow: cat1.bare-path; bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog); n/a -->
666:<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
667:<!-- integrity-allow: cat1.bare-path; bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog); n/a -->
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | grep "cat1.intra-repo.*project-state\.md"
(empty)
```

Three `cat1.intra-repo` fossil annotations + three `cat1.bare-path`
load-bearing annotations + zero `cat1.intra-repo` findings firing on
`project-state.md`. Fossil hypothesis confirmed.

Post-edit:

```
$ grep -n "integrity-allow.*other-cat1\b" project-state.md
559:<!-- integrity-allow: cat1.bare-path; bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog); n/a -->
592:<!-- integrity-allow: cat1.bare-path; bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog); n/a -->
664:<!-- integrity-allow: cat1.bare-path; bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog); n/a -->
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 45 hard-fail, 1339 suppressed
```

Hard-fail count unchanged (45 → 45). Removing inert suppressions did
not unsuppress any finding, exactly per the fossil hypothesis.
Suppressed count is unchanged at 1339 (the deleted annotations were
not contributing suppression events). The three remaining
`cat1.bare-path` annotations re-numbered down by one position each
(560 → 559, 594 → 592, 667 → 664) because the surrounding lines
shifted up.

## § D. Behavioral notes

**Deletion scope intentionally narrow.** Per spec § 7.B the spec
explicitly named the three `cat1.intra-repo` annotation lines for
deletion and explicitly preserved the three `cat1.bare-path`
annotation lines on the next line. Probe § B.8 confirmed both: the
intra-repo ones are fossils (no firing finding), the bare-path ones
were intended as load-bearing (but per probe § G.2 are not actually
suppressing — that's a separate known issue, banked in commit 7).

**Line-shift bookkeeping.** Each deletion shifts subsequent line
numbers up by one. With three deletions, the cumulative shift is
-3 by the end of the file. References elsewhere in the audit corpus
to `project-state.md:NNN` for lines past 666 will be off by up to 3
after this commit. Such references are mostly to architectural
sections that are well past the deleted region and the per-line drift
is a one-time cost. The audit-prose-freshness tool from commit 3 will
mechanically surface any that turn into out-of-range citations on
future runs.

**No live-source sweep.** Per Hard Rule 10, no `--sweep-live-source`
or `--force-sweep-category` invocation. The 40-ish live-source
`other-cat1` findings remain P1.8-protected.

## § E. Banked observations

**Fossil hypothesis validated end-to-end.** Probe-time prediction:
deleting the three `cat1.intra-repo` annotations should leave the
gate unchanged because no `cat1.intra-repo` findings fire on
project-state.md. Post-deletion observation: gate unchanged.
Prediction confirmed.

**Adjacent G.2 suppression bug untouched.** The
`cat1.bare-path:561 / 595 / 668` HARD_FAILs continue to fire (now
re-numbered after the line shift). Banking the underlying
suppression-not-firing question in commit 7 as a v2 investigation
item.

**Final fossil count.** With these three deletions, the project-state
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
file's `integrity-allow: cat1.intra-repo` annotation surface is
empty. Any future `cat1.intra-repo` finding on the file would either
need a fresh annotation (placed by the regular sweep) or be a real
defect to fix. The cleanup removes a maintenance hazard: the fossil
annotations could have masked future cat1.intra-repo regressions on
the lines they suppressed had a fresh finding ever wandered into the
suppression window.

## § F. Cross-references

- Spec § 7 (`docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md`)
- Probe § B.8, § E.3, § G.2, § K.6
- Part-B retro Decision 6
- Convention #12 — commit 8 of this batch resolves the
  `3d25ddc` placeholder above
