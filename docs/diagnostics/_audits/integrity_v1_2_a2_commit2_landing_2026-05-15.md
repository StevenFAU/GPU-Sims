---
title: "Integrity Toolkit v1.2 A.2 — Commit 2 landing audit (classifier + catalog + apply_annotations refactor)"
date: 2026-05-15
author: claude-code
status: draft
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_2_a2_spec_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_a2_commit1_landing_2026-05-15.md
---

# A.2 commit 2 landing audit — classifier + catalog + apply_annotations refactor

Companion to: `<commit-2-sha>` (SHA back-filled by commit 5 per Convention #12).
Builds on: `e079c7b` (commit 1 — new check module + fixtures + tests).

## A. Change summary

Three coordinated edits land in one commit:

1. **Classifier rule** in `grandfather.py:classify()`: route
   `cat2.public-symbol-used-toolkit` to the new
   `toolkit-own-unused` category per Decision 5.
2. **`apply_annotations()` refactor** in `grandfather.py`: add
   `force_sweep_categories: frozenset[str] = frozenset()` parameter
   per Decision 4. Live-source skip logic extended so categories listed
   in `force_sweep_categories` bypass live-source protection
   per-category, while all other live-source other-cat1 protections
   stay in place.
3. **Catalog section** in `tools/integrity/docs/grandfather-catalog.md`:
   add `toolkit-own-unused` section with `stack_paths()` named as the
   first tracked entry per Decision 7. Count placeholder is `(?)`;
   commit 4 fills it after the sweep.
4. **Sweep CLI flag** in `tools/integrity/scripts/grandfather_sweep.py`:
   add `--force-sweep-category <CATEGORY>` (repeatable). The existing
   `--sweep-live-source` flag is preserved.

The check itself is **not yet registered**; that lands in commit 3.

## B. File inventory

Modified:

- `tools/integrity/integrity/grandfather.py` — new classifier rule
  (inserted after `cat2.stub-label-stale` per spec § 5.1 Edit 1);
  new parameter on `apply_annotations()`; extended live-source skip
  predicate.
- `tools/integrity/scripts/grandfather_sweep.py` — new
  `--force-sweep-category` flag; forwarded as `frozenset` to
  `apply_annotations()`.
- `tools/integrity/docs/grandfather-catalog.md` — new
  `toolkit-own-unused` section with tracking notes.

New:

- `tools/integrity/tests/fixtures/conftest.py` — pytest collection
  guard. The good/bad_toolkit_self fixture trees include
  `tests/test_*.py` files that are scan-input for the new check, not
  pytest test cases; this conftest tells pytest to skip them via
  `collect_ignore_glob`. Surfaced during commit-2 verification when
  the fixture test files were being collected as orphan tests.

Plus this audit report.

## C. Verification results

### C.1 Tests

```
$ pytest tools/integrity/tests/ -q
153 passed, 1 warning in 135.95s
```

All 135 pre-existing tests pass plus the 18 from commit 1. The 1
warning is the same `PytestUnknownMarkWarning` on `@pytest.mark.slow`
from commit 1's test file.

The existing P1.8 tests in `test_grandfather_sweep.py` (27 total)
pass without modification, confirming the
`apply_annotations()` refactor preserves backward compatibility for
the no-`force_sweep_categories` codepath.

### C.2 Classifier rule smoke

```
$ python3 -c "from integrity.grandfather import classify, Finding
f = Finding(check_id='cat2.public-symbol-used-toolkit',
            file='tools/integrity/integrity/foo.py',
            line=10, message='test')
print('category:', classify(f).category)"
category: toolkit-own-unused
```

Decision 5 satisfied: new check ID routes to the new named category.

### C.3 `apply_annotations()` signature

```
$ python3 -c "from integrity.grandfather import apply_annotations
import inspect
sig = inspect.signature(apply_annotations)
print(list(sig.parameters))"
['repo_root', 'dry_run', 'sweep_live_source', 'force_sweep_categories']
```

The new keyword parameter is present with `frozenset()` default
(verified via direct inspection of `grandfather.py:apply_annotations`).

### C.4 CLI flag help

```
$ python3 tools/integrity/scripts/grandfather_sweep.py --help
... --force-sweep-category CATEGORY
    Force-sweep findings classified into the given category,
    regardless of LIVE-SOURCE protection. Repeatable. Example:
    --force-sweep-category toolkit-own-unused. Use sparingly --
    this opts a single named category out of the P1.8 live-source
    attribution-not-sweep policy, leaving all other LIVE-SOURCE
    categories protected.
```

### C.5 Strict-mode gate

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 5 pass, 0 soft-warn, 53 hard-fail, 1213 suppressed
```

The hard-fail count rose from the pre-A.2 baseline of 44 to 53. The
delta (+9) is entirely accounted for by audit-doc bare-path findings
on the new commit-1 audit report at
`docs/diagnostics/_audits/integrity_v1_2_a2_commit1_landing_2026-05-15.md`.
These are sweepable (audit-doc paths are in `SWEEPABLE_PATH_PREFIXES`)
and will be grandfather-annotated by commit 4's sweep.

Note: spec § 5.4 verification item 4 expected "baseline unchanged"
gate state. The +9 audit-doc bare-path delta is not "unchanged" by a
strict reading of the spec but is the expected intermediate state
for any commit that adds an audit report — the v1.2 bolt-ons retro
§ 3.3 documents the same pattern (audit-doc findings accumulate
until the next sweep companion runs). Commit 4 will close the
delta.

## D. Intentional intermediate state

- The check is still not registered. `python3 -m integrity
  --check cat2.public-symbol-used-toolkit ...` returns unknown.
- The classifier rule is dormant until commit 3 registers the check.
  No findings currently bucket to `toolkit-own-unused` because no
  `cat2.public-symbol-used-toolkit` findings flow through
  `collect_findings()` yet.
- The `--force-sweep-category` flag is wired through end-to-end but
  is a no-op in practice today (no findings classify into a category
  the flag would force-sweep).

## E. Decision-4 mechanics note

For named categories like `toolkit-own-unused`, the existing P1.8
live-source filter is already a no-op — the filter only checks
`cat in ("other-cat1", "other-cat1-bare-path")`. So in this commit
the `force_sweep_categories` parameter is effectively forward-
compatibility scaffolding: commit 4 will invoke
`--force-sweep-category toolkit-own-unused` for explicitness and
documentation, even though the live-source skip logic does not
currently apply to the named category in the first place.

This is consistent with the spec author's framing: Decision 4 is
"the most consequential decision" structurally because it adds the
per-category bypass machinery, even though in commit 4's specific
invocation the bypass is technically redundant. The benefit is in
the next-batch surface: if a future check produces fallthrough-
shaped findings on LIVE-SOURCE paths and the toolkit needs to sweep
a single category without disabling LIVE-SOURCE protection broadly,
the machinery is already in place.

## F. Risk and reversal

The classifier rule is append-only (inserted into the contiguous cat2
block, first-match-wins ordering preserved). The
`apply_annotations()` refactor is backward-compatible (default empty
frozenset; existing callers don't change). The catalog section is
append-only. Reverting is `git revert <sha>` plus a quick `pytest`
re-run; no migrations needed.

## G. Outstanding for commits 3-4

- Commit 3: register `public_symbol_used_toolkit` in
  `tools/integrity/integrity/cat2_contracts/checks/__init__.py`.
  Verify: post-register warn-only count in [3, 30]; `stack_paths()`
  present.
- Commit 4: run `python3 tools/integrity/scripts/grandfather_sweep.py
  --force-sweep-category toolkit-own-unused`. Refresh the new
  `toolkit-own-unused (?)` count plus any other catalog drift.
- Commit 5: SHA back-fill (separate commit, no `--amend` per
  Convention #12).
