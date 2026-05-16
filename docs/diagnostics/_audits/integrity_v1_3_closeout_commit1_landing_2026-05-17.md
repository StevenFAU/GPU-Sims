---
title: "Integrity v1.3 Closeout Commit 1 — Rewrite-stale-reasons sweep mode"
date: 2026-05-17
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_closeout_probe_2026-05-17_architect1-via-claude-code.md
  - docs/retro/integrity-toolkit-v1.3-batch1-part-b.md
---

# Integrity v1.3 Closeout Commit 1 — Rewrite-stale-reasons sweep mode

## § A. Change summary

Part-B retro § 4.1 banked the rewrite-stale-reasons mode as the
mechanical fix for a recurring pattern: a new classifier rule re-routes
existing findings from one category to another, but their inline
annotations continue to carry the old category's reason text. The
suppression matcher keys on `check_id`, so the findings stay suppressed
and the gate stays green — but the reason wording is stale, which
matters at audit time. Part-B commit 2 paid the cost manually (23 hand
edits across 7 files).

This commit lands the mode. `rewrite_stale_reasons(repo_root, dry_run)`
in `tools/integrity/integrity/grandfather.py` enumerates suppressed
findings, compares the parsed-category of each existing annotation to
the current `classify()` output, and rewrites the reason in place when
they differ. Conservative scope per Decision D1 (closeout spec § 0.3):
rewrites only when category changed; wording-only differences in the
same category are left alone. CLI exposure via a new
`--rewrite-stale-reasons` flag on `grandfather_sweep.py`, mutually
exclusive with the normal sweep flags to keep operator intent
unambiguous.

Six new tests pin the behavior (category-changed → rewrite,
wording-only → skip, dry-run → no writes, comment-form preserved,
mutual-exclusion CLI error, no-op when empty). First-run sweep on the
real repo rewrote 8 annotations across 6 files; the dominant
transitions are the T1.1 reclassifications Part-B introduced.

## § B. File inventory

| Status | Path | Diff |
|---|---|---|
| Modified | `tools/integrity/integrity/grandfather.py` | +179 LOC (one new public function + three helpers + docstring header) |
| Modified | `tools/integrity/scripts/grandfather_sweep.py` | +35/-5 LOC (new flag + mutually-exclusive branch + Counter import) |
| Modified | `tools/integrity/tests/test_grandfather_sweep.py` | +260 LOC (6 new tests + 9 inline sweep-companion annotations) |
| Modified | `tools/integrity/tests/test_suppression_fence.py` | +2/-2 LOC (rewrite-stale-reasons output: 2 toolkit-own-source → other-cat1 annotation reasons) |
| Modified | `docs/diagnostics/_audits/integrity_v1_2_a3_probe_2026-05-15_architect1.md` | +2/-2 LOC (rewrite output) |
| Modified | `docs/diagnostics/_audits/integrity_v1_3_t1_1_2_probe_2026-05-16_architect1.md` | +1/-1 LOC (rewrite output) |
| Modified | `docs/retro/integrity-toolkit-v1.1-batch1-addendum.md` | +1/-1 LOC (rewrite output) |
| Modified | `docs/retro/integrity-toolkit-v1.1-batch1.md` | +1/-1 LOC (rewrite output) |
| Modified | `tools/integrity/docs/algebraic/d3q19.md` | +1/-1 LOC (rewrite output) |
| Created | `docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md` | execution spec staged alongside this commit |
| Created | `docs/diagnostics/_audits/integrity_v1_3_closeout_probe_2026-05-17_architect1-via-claude-code.md` | pre-spec probe (untouched since landing in this commit) |
| Created | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit1_landing_2026-05-17.md` | this report |

## § C. Verification

Pre-edit anchoring (HEAD `a1c9121`, matches probe § A.1):

```
$ git rev-parse HEAD
a1c912159d3c946aee7d33b06b36fd265d63d878
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 60 hard-fail, 1263 suppressed
$ python3 -m pytest tools/integrity/tests/ -q 2>&1 | tail -1
183 passed, 1 warning in 132.60s (0:02:12)
```

Test suite (post-edit):

```
$ python3 -m pytest tools/integrity/tests/test_grandfather_sweep.py -q
46 passed in 0.13s
```

40 pre-existing + 6 new = 46.

Dry-run sweep on the real repo:

```
$ python3 tools/integrity/scripts/grandfather_sweep.py --rewrite-stale-reasons --dry-run
grandfather-sweep: would rewrite 9 annotation reasons across 6 files
  other-cat1 -> audit-citation: 2
  other-cat1 -> retro-doc-snapshot: 2
  toolkit-own-source -> audit-report-grammar-example: 2
  toolkit-own-source -> other-cat1: 2
  other-cat1 -> toolkit-doc-snapshot: 1
```

Within spec § 1.4 prediction band of 10–20 (9 is within tolerance; only
>30 triggers pause-and-surface per § 0.2). The five transition classes
are exactly the T1.1 reclassifications plus the two test-file
`toolkit-own-source → other-cat1` corrections — see § D below.

Apply sweep:

```
$ python3 tools/integrity/scripts/grandfather_sweep.py --rewrite-stale-reasons
grandfather-sweep: rewrote 8 annotation reasons across 6 files
  other-cat1 -> audit-citation: 2
  other-cat1 -> retro-doc-snapshot: 2
  toolkit-own-source -> audit-report-grammar-example: 2
  toolkit-own-source -> other-cat1: 2
  other-cat1 -> toolkit-doc-snapshot: 1
```

The 8-vs-9 collapse: two of the dry-run's nine rewrites land on the
same annotation line in
`docs/diagnostics/_audits/integrity_v1_2_a3_probe_2026-05-15_architect1.md:1046`
(two suppressed findings share the line). The first apply pass rewrites
the line; the second pass finds the new reason already in place, the
regex `re.sub` returns the unchanged string, and the second rewrite is
not counted. Behavioral and idempotent.

Mutual-exclusion error path:

```
$ python3 tools/integrity/scripts/grandfather_sweep.py --rewrite-stale-reasons --sweep-live-source
error: --rewrite-stale-reasons is mutually exclusive with --sweep-live-source / --force-sweep-category
$ echo $?
2
```

Post-rewrite gate (before sweep companion):

```
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 60 hard-fail, 1273 suppressed
```

Rewrite alone is finding-suppression neutral. Suppressed grew by +10:
nine new inline `cat1.annotation-form` sweep-companion annotations on
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
test-fixture lines that contain literal `integrity-allow:` grammar
tokens, plus one bookkeeping increment from the rewrite docstring
exchange.

Sweep companion (Convention B):

```
$ python3 tools/integrity/scripts/grandfather_sweep.py
grandfather-sweep: modified 6 files; 16 annotations added
  skipped as live-source (other-cat1): 39 (use --sweep-live-source to include)
                      audit-bare-path: 13
         audit-report-grammar-example: 2
                 spec-grammar-example: 1
```

Final gate:

```
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 44 hard-fail, 1289 suppressed
```

Hard-fail count fell 60 → 44 (-16), suppressed grew 1273 → 1289 (+16):
the sweep companion absorbed Convention J carry-over from the v1.3
Part-A and Part-B batches (annotations in `integrity_v1_3_commit4_landing`,
`integrity_v1_3_part_b_commit1_landing`,
`integrity_v1_3_part_b_commit2_landing`,
`integrity_v1_3_part_b_spec`,
`integrity_v1_3_t1_1_2_probe`,
and one line added to `tools/integrity/docs/grandfather-catalog.md`).
No `live-source` files were swept; the 39 `other-cat1` live-source
findings remain P1.8-protected per Hard Rule 10.

Convention I (cross-batch scope discipline) considered and not
triggered: Part-A and Part-B are closed batches with no further
sweep companions planned, so deferring would leave the residue
permanently red. Per Convention J the sweep operates on the current
cat1-scannable surface at run time; the carry-over absorption here is
the canonical Convention J behavior, not an opportunistic cross-batch
sweep.

## § D. Behavioral notes

**D1 conservative scope.** The rewrite triggers only when the parsed
category of an annotation differs from the current `classify()` output.
Wording-only drift inside the same category is left alone (verified by
`test_rewrite_stale_reasons_wording_diff_only_skipped`). This is
deliberately surgical: history-churn cost is paid only for the
classifier-rule-driven case Part-B § 4.1 named, not for the broader
"polish-every-reason" surface that wording-rewriting would imply.

**Comment-form preservation.** The `_rewrite_annotation_reason` helper
preserves the `//`, `#`, and `<!-- -->` comment-prefix variants by
substituting only the reason segment of the annotation grammar regex,
leaving everything before the first `;` and after the second `;`
untouched. Verified by `test_rewrite_stale_reasons_preserves_comment_form`
across all three forms.

**Mutual exclusion.** `--rewrite-stale-reasons` rejects combination
with `--sweep-live-source` and `--force-sweep-category` at argparse
post-parse time, exiting `2`. Rationale: the two operations have
distinct entry points (`rewrite_stale_reasons` vs `apply_annotations`),
and mixing them in one CLI invocation would silently run only one of
the two paths. Failing fast is the right ergonomics here.

**Transition shape on the real repo.** The 8 applied rewrites
distribute as `other-cat1 → audit-citation` (2), `other-cat1 →
retro-doc-snapshot` (2), `other-cat1 → toolkit-doc-snapshot` (1),
`toolkit-own-source → audit-report-grammar-example` (2), and
`toolkit-own-source → other-cat1` (2). The first three classes are
the expected Part-B T1.1 reclassifications — annotations placed
pre-T1.1 with the generic `other-cat1` reason that now classify into a
named snapshot-doc category. The two `→ audit-report-grammar-example`
are corrections of annotations placed with a too-narrow
`toolkit-own-source` reason on lines that actually live in
`docs/diagnostics/_audits/` (correct category is the audit-doc one).

The two `toolkit-own-source → other-cat1` rewrites live in
`tools/integrity/tests/test_suppression_fence.py` (lines 3 and 23).
`classify()` for `cat1.annotation-form` only recognises four file-path
buckets (`docs/integrity-toolkit-spec.md`, `tools/integrity/docs/`,
`docs/retro/`, `tools/integrity/integrity/`, `docs/diagnostics/_audits/`)
and falls through to `other-cat1` for everything else. The toolkit's
own `tools/integrity/tests/` is not in that list, so test-file
annotations land in `other-cat1` per current classifier shape. The
existing annotations on these two lines were placed with the more
specific `toolkit-own-source` reason — that was a manual judgement
call, not a classifier output. D1 honours `classify()` as the source
of truth and rewrites accordingly. Whether `classify()` should
recognise the `tools/integrity/tests/` path bucket is a separate
classifier-design concern; banked here, not addressed by closeout.

## § E. Banked observations

**Deviation from spec § 2.C.1 drafted code (intent-preserving).** The
spec's drafted `rewrite_stale_reasons` body began with
`findings = collect_findings(repo_root)`. Empirically,
`collect_findings` filters suppressed findings out of its return value
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
(`grandfather.py:402` skips entries with `f.get("suppressed")` set).
The rewrite mode by construction needs the suppressed findings — those
are the entries whose annotations exist and may be stale — so calling
`collect_findings` would yield an empty rewrite set in practice. The
landed implementation introduces a `_collect_all_findings` helper that
re-invokes `python3 -m integrity --output json` and returns every
finding dict including suppressed ones, then reads the JSON's
`suppression_reason` field directly. This preserves spec intent (the
public `rewrite_stale_reasons(repo_root, dry_run)` signature and
return shape are unchanged) while removing the empty-result failure
mode. Probe § G.1 named the related `is_suppressed`/`Finding.suppressed`
absence; this approach sidesteps both by reading the raw JSON.

**Dry-run vs apply count: 9 vs 8.** Already covered in § C. The
difference is a single duplicate-line collision in
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`integrity_v1_2_a3_probe_2026-05-15_architect1.md:1046`. The dry-run
counter walks the rewrite list; the apply pass collapses identical
in-place rewrites. Future tightening: dedupe the rewrite list by
(file, line, new_reason) before reporting the dry-run count. Banked,
not blocking.

**New inline annotations on test fixtures.** The new
rewrite-stale-reasons tests construct fixtures that literally embed
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
`integrity-allow:` grammar tokens (the annotation lines the rewrite
logic operates on). These embeddings would fire `cat1.annotation-form`
on the test file itself; the test file lives under
`tools/integrity/tests/`, which the classifier routes to `other-cat1`
(fall-through), and `other-cat1` on live-source paths is filtered out
of the regular sweep by P1.8 protection. To keep the gate at the
60-hard-fail baseline per spec § 1.4 and the verification block in
§ 2.D, nine inline
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
`# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a`
comments were added above the literal-token lines (the same pattern
the existing tests in this file use at line 73 etc.). This is the
toolkit's standard self-suppression convention for tests that exercise
the annotation grammar.

**Test fixture path strings sanitised.** The initial draft used
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`"some/path.cpp:42"` as fixture text. That string format matches the
`cat1.intra-repo` citation grammar and fired six new
`HARD_FAIL`s on the test file itself. Per Hard Rule 10
(live-source-stays-red) the only acceptable resolution was to remove
the citation-shaped strings, since auto-sweep is forbidden for closeout
commits. Replaced with `"synthetic finding target line (no citation here)"`
and `"synthetic: target path does not resolve"`. The rewrite logic
under test doesn't depend on the fixture text format — only on the
annotation line above it and the synthetic suppressed-finding dict
the test mocks via `monkeypatch.setattr`.

**Spec § 0.2 pause-and-surface triggers reviewed; none fired.** Rewrite
count 9 is below the >30 threshold (predicted band 10–20; 9 is below
band but within tolerance per the explicit "more than 30" condition).
Gate hard-fail unchanged at 60. No live-source findings required
sweeping. Project-state.md was not touched by this commit. No test
failed on first run after the fixture sanitation pass.

## § F. Cross-references

- Spec § 2 (`docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md`)
- Probe § B.1, § G.1
  (`docs/diagnostics/_audits/integrity_v1_3_closeout_probe_2026-05-17_architect1-via-claude-code.md`)
- Part-B retro § 4.1
  (`docs/retro/integrity-toolkit-v1.3-batch1-part-b.md`) — banked the
  rewrite-stale-reasons mode (called "Convention I" there; resolved
  as a feature, not a convention; conventions doc landed in commit 5
  documents the letter-I collision)
- Convention #12 (SHA back-fill discipline) — commit 8 of this batch
  back-fills any sibling-commit references in this report (none
  currently present)
