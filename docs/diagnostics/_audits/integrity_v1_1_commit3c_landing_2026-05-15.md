# Integrity Toolkit v1.1 — Commit 3c Landing — 2026-05-15

Sub-commit 3c of the integrity-toolkit v1.1 batch-1 sequence. Final
sub-commit of the decomposed commit 3: A.8 (per-category live tallies
in the catalog headings) + 5.B (`python` → `python3` docs sweep) +
a snapshot-classifier refinement that unblocks A.8 accuracy.

Companion to:

- Batch-1 execution spec: `docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md`
- Commit 1 (A.1 stub-label): `af248cf` -- `integrity_v1_1_commit1_landing_2026-05-15.md`
- Commit 2 (A.5 fence-block): `f661ec4` -- `integrity_v1_1_commit2_landing_2026-05-15.md`
- Commit 3a (snapshot module): `dbac051` -- `integrity_v1_1_commit3a_landing_2026-05-15.md`
- Commit 3b (CLI flags): `a71594a` -- `integrity_v1_1_commit3b_landing_2026-05-15.md`
- This commit's SHA: `a28e1d7`

---

## A. Change summary

**A.8 — populate per-category counts.** Every category heading in
`tools/integrity/docs/grandfather-catalog.md` now carries a
parenthetical count populated from a fresh `--grandfather-report` run.
A new "Updating counts" section at the top of the catalog documents the
manual-refresh procedure (auto-refresh from the history file is banked
as a v1.2 candidate).

**Approximate-prose count refs removed.** Per spec § 5.4: the
"~10 entries" mention inside `live-shader-1810` and the "~108 entries"
mention inside `cat2-stack-c-unused` are now redundant with the
heading counts.

**5.B — `python` → `python3` docs sweep.** Replaced all 13 occurrences
of `python -m integrity` with `python3 -m integrity` per the spec's
host-environment fact (no `python` shim on the host). Files touched
exactly match spec § 5.5 / Decision 10:

- `tools/integrity/README.md` (6 sites)
- `docs/integrity-toolkit-spec.md` (6 sites)
- `tools/integrity/integrity/__main__.py:1` (docstring, 1 site)

Audit reports under `docs/diagnostics/_audits/` deliberately untouched
(append-only convention — same as the `audit-citation` grandfather
rationale).

**Snapshot classifier refinement (A.8 prerequisite).** The 3a
`_extract_category` heuristic matched only reason strings that embed
the category name. Inspection of actual reasons on the synced repo
showed 6 categories whose classifier rules in `grandfather.py` produce
descriptive reasons that do not mention the category name. Added a
secondary `_REASON_PATTERNS` table to `integrity/snapshot.py` mapping
canonical reason fragments to their categories. Without this fix, 77
of the 944 suppressed findings would have bucketed as "other" and
been missing from the catalog tallies. New unit test
(`test_extract_category_pattern_matches`) covers all 6 patterns.

## B. File inventory

**Modified:**

- `tools/integrity/docs/grandfather-catalog.md` — counts on all 12
  category headings, new "Updating counts" section, approximate-prose
  counts removed from `live-shader-1810` and `cat2-stack-c-unused`
  bodies.
- `tools/integrity/integrity/snapshot.py` — `_REASON_PATTERNS` table
  added; `_extract_category` falls through to it.
- `tools/integrity/tests/test_snapshot.py` —
  `test_extract_category_pattern_matches` covering all 6 new patterns.
- `tools/integrity/README.md` — 6× `python` → `python3`.
- `docs/integrity-toolkit-spec.md` — 6× `python` → `python3`.
- `tools/integrity/integrity/__main__.py` — 1× docstring update.

**New:**

- This audit report.

## C. Verification

### C.1 Catalog tally cross-check

Compared each category heading's parenthetical against the live
`--grandfather-report` output (commit `c3391f7`, 2026-05-15):

| Category | Heading count | Report count | Match |
|---|---|---|---|
| `audit-citation` | 597 | 597 | ✓ |
| `live-shader-1810` | 3 | 3 | ✓ |
| `audit-doc-1810` | 15 | 15 | ✓ |
| `spec-grammar-example` | 17 | 17 | ✓ |
| `toolkit-own-source` | 22 | 22 | ✓ |
| `retro-grammar-example` | 2 | 2 | ✓ |
| `audit-report-grammar-example` | 19 | 19 | ✓ |
| `other-cat1` | 66 | 66 | ✓ |
| `cat2-stack-d-unused` | 17 | 17 | ✓ |
| `cat2-stack-c-unused` | 111 | 111 | ✓ |
| `cat2-stack-b-unused` | 73 | 73 | ✓ |
| `cat2-stub-label-stale` | 2 | 2 | ✓ |
| **Total** | **944** | **944** | ✓ |

### C.2 Docs sweep verification

```
$ rg -n '\bpython -m integrity\b' \
    tools/integrity/README.md \
    docs/integrity-toolkit-spec.md \
    tools/integrity/integrity/__main__.py
(0 matches)

$ rg -n '\bpython3 -m integrity\b' \
    tools/integrity/README.md \
    docs/integrity-toolkit-spec.md \
    tools/integrity/integrity/__main__.py
(13 matches)
```

### C.3 Full test suite

```
$ cd tools/integrity && python3 -m pytest tests/
======================== 96 passed in 134.58s (0:02:14) ========================
```

96 = 95 from 3b + 1 new (`test_extract_category_pattern_matches`).

### C.4 Snapshot pattern coverage

```
$ pytest tests/test_snapshot.py::test_extract_category_pattern_matches -v
tests/test_snapshot.py::test_extract_category_pattern_matches PASSED
```

All 6 of the previously-uncovered reason patterns now classify
correctly:
- "regex or docstring literal of the annotation grammar" → `toolkit-own-source`
- "audit-doc literal mention of the annotation grammar" → `audit-report-grammar-example`
- "documentation-only literal mention of the annotation grammar" → `spec-grammar-example`
- "retrospective-doc literal mention of the annotation grammar" → `retro-grammar-example`
- "historical 1.8.10 fabrication" → `audit-doc-1810`
- "stale phase-n stub label" → `cat2-stub-label-stale`

### C.5 Integrity still green (relative)

Baseline hard-fails on the moving HEAD remain documented in commit 2's
landing report E.2. 3c does not introduce any new findings.

## D. Behavioral notes

- The catalog counts are a manual snapshot, refreshed on each
  count-affecting commit. Auto-refresh from
  `.grandfather-history.json` is a v1.2 candidate.
- The `_REASON_PATTERNS` table is order-sensitive only if patterns
  overlap; current entries are mutually exclusive substrings.

## E. Incidental findings

### E.1 Snapshot classifier was incomplete in 3a

3a's `_extract_category` would have left ~77 of 944 suppressions
bucketed as "other" — a 12% mis-classification rate. The cause was a
mismatch between the snapshot module's category-extraction heuristic
(substring match on the category name) and the actual classifier-rule
reasons in `grandfather.py`, six of which describe the violation in
prose without naming the category. Discovered while populating the
catalog tallies in 3c. Fixed by adding a small secondary pattern table
(`_REASON_PATTERNS`).

The cleaner long-term fix would be to embed the category name into
every classifier-rule reason string in `grandfather.py`, removing the
need for the secondary pattern table. Banked as a v1.2 candidate
("classifier reasons should self-identify their category"). Doing it
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
now would invalidate the existing `integrity-allow:` annotations
(they hard-code the current reason strings), forcing a sweep
re-application.

### E.2 No new HARD_FAIL introduced

The 29 baseline hard-fails reported by `--grandfather-report`'s summary
field (`hard_fail: 29`) are pre-existing on HEAD per commit 2's
landing report E.2 — they predate this batch's work and accumulate
from new audit docs and shader edits landing on `main` without a
grandfather-sweep companion commit. None of 3a, 3b, or 3c contributes
to that 29.

End of commit 3c audit report.
