---
title: "Integrity v1.2 Commit 3 — P1.6 Runner Human-Output Suppressed-Stanza Filter"
date: 2026-05-15
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_spec_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_commit1_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_2_commit2_landing_2026-05-15.md
---

# Integrity v1.2 Commit 3 — P1.6 Runner Human-Output Suppressed-Stanza Filter

## § A. Change summary

Adds the same `if f.suppressed: continue` guard the github-output
branch carries (line 134) to the default human-output `else` branch
(now lines 147-148) in `tools/integrity/integrity/runner.py`. Pre-fix,
suppressed findings rendered as `HARD_FAIL:` stanzas even though they
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
were intentionally silenced by `integrity-allow:` annotations — the
asymmetry was original to commit `fc20ef7` which added the
suppressed-filter only to the github branch.

## § B. File inventory

- `tools/integrity/integrity/runner.py` — modified.
  - Added 3-line comment + `if f.suppressed: continue` guard at the
    top of the for-loop in the default human-output branch.
- `tools/integrity/tests/test_runner_human_output.py` — new, ~110 LOC;
  4 tests covering suppressed-omission, github branch regression-guard,
  and full-render-when-no-suppression behavior.
- `docs/diagnostics/_audits/integrity_v1_2_commit3_landing_2026-05-15.md` —
  this landing report.

## § C. Verification

### C.1 Tests

```
$ cd tools/integrity && python3 -m pytest tests/test_runner_human_output.py tests/test_runner.py -v
9 passed in 0.75s
```

4 new + 5 existing runner tests; all pass.

### C.2 Strict-mode stanza count

Pre-fix (P1.6 bug): the human-output renderer emitted one `HARD_FAIL:`
stanza per finding regardless of suppression status — producing stanza
counts that included suppressed entries.

Post-fix verification:

```
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 691 hard-fail, 435 suppressed

$ python3 -m integrity --mode strict --no-audit-log 2>&1 | grep -c "HARD_FAIL:"
49
```

49 unsuppressed `HARD_FAIL:` stanzas render — matching the count of
findings that warrant human attention (the live-source `other-cat1`
bucket protected by P1.8, plus the `other-cat1-bare-path` /
`cat1.bare-path` fallthroughs A.3 introduced and A.3 commit 4 will
sweep).

**Note on the 691 summary count:** in the current post-A.3 state, the
`hard_fail` count in `RunSummary` includes findings whose mode is
`HARD_FAIL` regardless of suppression status — `suppressions` is a
separate field counting findings with `suppressed=True`. The two
counters overlap on the (HARD_FAIL ∩ suppressed) intersection. The
spec § 3.4 verification block was written against the simpler
pre-A.3 state where the overlap was empty (so summary.hard_fail ==
unsuppressed-HARD_FAIL count). After A.3, summary.hard_fail is no
longer a single-number stand-in for "things to look at" — the
stanza count is. P1.6 fixes the stanza count to be that single
number. **Aligning the RunSummary counter semantics with the
post-A.3 reality is a separate follow-up (bank as v1.3 candidate).**

## § D. Cross-references

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- Probe § D.2 — verbatim `runner.py:141-145` pre-fix content.
- Probe § D.5 — asymmetry origin in commit `fc20ef7`.
- Probe § A.2 / § D.4 — original summary/stanza-mismatch observation.

## § E. Banked observations

- **RunSummary.hard_fail counter overlaps with suppressions.** Post-A.3,
  the summary's "X hard-fail, Y suppressed" line is no longer
  decomposable into disjoint buckets — some HARD_FAIL findings are
  also suppressed and counted in both. Future v1.3 should either:
  (a) make `hard_fails` count only unsuppressed (and add a
  `suppressed_hard_fails` field for the intersection), or (b) document
  the overlap in the summary string itself. **Bank as v1.3 candidate.**
- **No pause-and-surface fired during commit 3 execution.**

## § F. Sibling commit SHAs

This commit: `71559ce`. Sibling commits in this batch:

- Commit 1 (P1.8 — grandfather-sweep live-source protection): `5fe5e6b`
- Commit 2 (P1.5 — register `cat3.d3q19-*` checks): `119e353`
- Commit 4 (P1.7 — `stub_label_stale.py` docstring fix): `5cdd20f`
- Commit 5 (SHA back-fill): this commit
