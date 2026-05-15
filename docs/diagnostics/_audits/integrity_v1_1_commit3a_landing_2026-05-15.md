# Integrity Toolkit v1.1 — Commit 3a Landing — 2026-05-15

Commit 3 of the integrity-toolkit v1.1 batch-1 sequence has been
decomposed into three sub-commits (3a, 3b, 3c) per user direction
2026-05-15, to bound exposure to concurrent-edit races observed during
batch landing (see commit 3b's audit E.1 once it lands).

3a is the **race-immune sub-commit**: it adds only new files that did
not exist on `main` prior, so no concurrent revert can stomp them.

Companion to:

- Batch-1 execution spec: `docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md`
- Commit 1 (A.1 stub-label): `af248cf` -- `integrity_v1_1_commit1_landing_2026-05-15.md`
- Commit 2 (A.5 fence-block): `f661ec4` -- `integrity_v1_1_commit2_landing_2026-05-15.md`
- This commit's SHA: `dbac051`
- Commit 3b (CLI flags): `a71594a` -- `integrity_v1_1_commit3b_landing_2026-05-15.md`
- Commit 3c (catalog + python3 sweep): `a28e1d7` -- `integrity_v1_1_commit3c_landing_2026-05-15.md`

---

## A. Change summary

3a lands the snapshot module that A.7's CLI flags (3b) will dispatch
into, the seed history file that `--grandfather-report --history-append`
will append to, and unit tests covering the new module. No runner.py
or other shared-source changes -- those are 3b's scope.

## B. File inventory

**New:**

- `tools/integrity/integrity/snapshot.py` -- `emit_state_snapshot()` and
  `emit_grandfather_report()` entry points plus their private helpers
  (`_collect_state`, `_extract_category`, `_parse_ground_truth_sources`,
  `_append_history`).
- `tools/integrity/.grandfather-history.json` -- seeded with `[]`.
- `tools/integrity/tests/test_snapshot.py` -- 4 unit tests covering
  category extraction, ground-truth-sources parsing, and snapshot smoke.
- This audit report.

**Modified:**

- `tools/integrity/tests/conftest.py` -- added `repo_root` pytest fixture
  resolving via `git rev-parse --show-toplevel` (needed by the snapshot
  smoke tests; per spec § 5.7 verification claim).

## C. Verification

### C.1 Unit tests (new module)

```
$ cd tools/integrity && python3 -m pytest tests/test_snapshot.py -v
tests/test_snapshot.py::test_extract_category_matches_known PASSED
tests/test_snapshot.py::test_extract_category_empty PASSED
tests/test_snapshot.py::test_parse_ground_truth_sources_smoke PASSED
tests/test_snapshot.py::test_emit_state_snapshot_smoke PASSED
======================== 4 passed in 100.91s ========================
```

### C.2 Manual smoke (direct module import)

```
>>> import io
>>> from integrity.snapshot import emit_state_snapshot
>>> from pathlib import Path
>>> out = io.StringIO()
>>> emit_state_snapshot(Path("."), out)
>>> import json; d = json.loads(out.getvalue())
>>> sorted(d.keys())
['commit', 'per_category', 'registered_checks', 'registered_upstreams', 'schema_version', 'summary', 'timestamp']
>>> [len(v) for v in d['registered_checks'].values()]
[5, 4, 1]
>>> len(d['registered_upstreams'])
2
```

Verifies all expected snapshot keys, the 10 registered checks
(5 cat1 + 4 cat2 + 1 cat3), and 2 registered upstreams
(SPlisHSPlasH at 2.16.1, lbm-principles-practice at book-companion-code-2016).

### C.3 Integrity still green

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 2 pass, 0 soft-warn, <baseline> hard-fail, <baseline> suppressed
```

Baseline hard-fails on the moving HEAD are documented in commit 2's
landing report E.2. 3a does not introduce any new findings.

## D. Behavioral notes

- The snapshot module is **untriggered** until 3b lands the CLI flags
  that dispatch to it. 3a is a pure addition; nothing in the existing
  toolkit calls into `integrity.snapshot` yet.
- The history file is **initially empty (`[]`)**. The first real entry
  will land when 3b's verification block runs
  `python3 -m integrity --grandfather-report` for the first time.

## E. Incidental findings

None for 3a. The concurrent-edit race that motivated this
decomposition will be documented in commit 3b's audit E.1.

End of commit 3a audit report.
