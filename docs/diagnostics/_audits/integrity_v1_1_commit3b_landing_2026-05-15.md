# Integrity Toolkit v1.1 — Commit 3b Landing — 2026-05-15

Sub-commit 3b of the integrity-toolkit v1.1 batch-1 sequence. Wires the
A.7 CLI flags (`--state-snapshot`, `--grandfather-report`,
`--no-history-append`) into `runner.py`. Builds on the
race-immune 3a sub-commit (`dbac051`) that landed the
`integrity.snapshot` module.

Companion to:

- Batch-1 execution spec: `docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md`
- Commit 1 (A.1 stub-label): `af248cf` -- `integrity_v1_1_commit1_landing_2026-05-15.md`
- Commit 2 (A.5 fence-block): `f661ec4` -- `integrity_v1_1_commit2_landing_2026-05-15.md`
- Commit 3a (snapshot module): `dbac051` -- `integrity_v1_1_commit3a_landing_2026-05-15.md`
- This commit's SHA: `a71594a`
- Commit 3c (catalog + python3 sweep): `a28e1d7` -- `integrity_v1_1_commit3c_landing_2026-05-15.md`

---

## A. Change summary

Extends `runner.py` with the three v1.1 A.7 flags. `--state-snapshot`
emits the JSON state document from `integrity.snapshot.emit_state_snapshot`
and exits. `--grandfather-report` emits the human-readable per-category
table from `integrity.snapshot.emit_grandfather_report`; the optional
`--no-history-append` skips appending to
`tools/integrity/.grandfather-history.json` (read-only mode).

## B. File inventory

**Modified:**

- `tools/integrity/integrity/runner.py` — three new argparse flags
  (`--grandfather-report`, `--no-history-append`, `--state-snapshot`),
  three new fields in `CliArgs`, three new field assignments in
  `parse_args`, and a short-circuit dispatch block at the top of
  `main()` that handles the snapshot/report paths.

**New:**

- This audit report.

## C. Verification

### C.1 `--state-snapshot` smoke

```
$ python3 -m integrity --state-snapshot --no-audit-log > /tmp/snap.json
$ python3 -c "import json; d=json.load(open('/tmp/snap.json')); print(sorted(d.keys()))"
['commit', 'per_category', 'registered_checks', 'registered_upstreams', 'schema_version', 'summary', 'timestamp']
$ python3 -c "import json; d=json.load(open('/tmp/snap.json')); print({k: len(v) for k,v in d['registered_checks'].items()})"
{'cat1': 5, 'cat2': 4, 'cat3': 1}
$ python3 -c "import json; d=json.load(open('/tmp/snap.json')); print(len(d['registered_upstreams']))"
2
```

Schema valid; 10 registered checks; 2 registered upstreams
(SPlisHSPlasH 2.16.1, lbm-principles-practice book-companion-code-2016).

### C.2 `--grandfather-report --no-history-append` smoke

```
$ python3 -m integrity --grandfather-report --no-history-append --no-audit-log
grandfather report @ dbac051 (2026-05-15T16:20:09+00:00)
summary: {'pass': 2, 'soft_warn': 0, 'hard_fail': 29, 'suppressed': 944}
per-category counts:
                       audit-citation: 597
                  cat2-stack-c-unused: 111
                                other: 77
                  cat2-stack-b-unused: 73
                           other-cat1: 66
                  cat2-stack-d-unused: 17
                     live-shader-1810: 3
```

Per-category table emitted to stdout. History file unmodified
(`--no-history-append` honored).

### C.3 `--grandfather-report` history append

```
$ cat tools/integrity/.grandfather-history.json | python3 -c "import json,sys; print(len(json.load(sys.stdin)))"
0
$ python3 -m integrity --grandfather-report --no-audit-log > /dev/null
$ cat tools/integrity/.grandfather-history.json | python3 -c "import json,sys; print(len(json.load(sys.stdin)))"
1
```

History file grew from 0 entries to 1 (one append per `--grandfather-report`
without `--no-history-append`). The test-generated entry was reset to
`[]` before commit so 3b's tree carries a clean seeded history file.

### C.4 Full test suite

```
$ cd tools/integrity && python3 -m pytest tests/
======================== 95 passed in 129.52s (0:02:09) ========================
```

95 = 91 from commit 2 + 4 from 3a's snapshot tests. No new tests in
3b; the snapshot smoke is exercised by the existing 3a tests plus
the manual CLI smokes above.

### C.5 Integrity still green (default mode)

Baseline hard-fails on the moving HEAD are documented in commit 2's
landing report E.2 and remain unchanged. 3b does not introduce new
findings.

## D. Behavioral notes

- The two short-circuit branches in `main()` run **before** the normal
  check-dispatch path. Neither emits the standard audit log or summary
  line; they emit their own output format and exit cleanly.
- `--state-snapshot` ignores `--mode` — it's a state read, not a check
  run. The `summary` field inside the snapshot reflects warn-only
  results (so it includes all findings without HARD_FAIL termination).
- `--grandfather-report` shares the same warn-only subprocess pattern.
  The subprocess re-invocation costs ~2 seconds; documented in spec
  § 6.3.

## E. Incidental findings

### E.1 Concurrent-edit race observed during batch landing

Commit `f23fd22` (`revert(integrity): unintended runner.py changes from
commit 4`) landed on `main` while this batch's original (monolithic)
commit-3 work was in flight, wiping in-progress A.7 edits to
`runner.py`. The revert was independently correct: it removed unrelated
commit-4 work from another concurrent session that had accidentally
included WIP edits to `runner.py`. The timing exposed the original
commit-3 design assumption that the batch lands serialized against
`main`.

**Mitigation applied (per user direction 2026-05-15):** commit 3 was
decomposed into three sub-commits to bound future race exposure:

- **3a** (`dbac051`) — new files only (race-immune). `snapshot.py`,
  `.grandfather-history.json`, `test_snapshot.py`, `repo_root` fixture.
- **3b** (this commit) — runner.py CLI extension. Tight pre-edit cycle:
  `git pull --rebase`, inspect runner.py touch history, confirm the
  revert's scope, re-anchor, apply, commit.
- **3c** (next) — docs sweep (catalog + python → python3 in user docs).
  Low race risk; lands last.

Cost of the realized race: approximately 5 minutes of re-application
work. The pattern is expected to recur given parallel work across
phases 11.5, 12, and other batches. Banked for the v1.1 retro under
**operating conditions**:

> The v1 spec's commit pattern assumed serialized landing. The actual
> operating condition is concurrent multi-agent work. Future execution
> specs should default to commit decomposition for any commit whose work
> touches more than one previously-existing file. New-files-only commits
> are race-immune and should ship first when possible.

### E.2 Pre-edit verification of `f23fd22`'s scope

Before applying the runner.py edits, the revert at `f23fd22` was
inspected to confirm its scope matched its commit message
("unintended commit-4 changes, unrelated to A.7 edits"). The diff
showed exactly the 25 lines reverted are the same 25 lines this
sub-commit re-adds — the revert was correct, and the
re-application restores precisely what was intended. The
intermediary commits between `f23fd22` and this sub-commit
(`66daf9f`, `c5955d3`, `dde5f22`, `a4ceeb9`, `dbac051`) did not
touch `runner.py`, so no rebase conflicts arose.

End of commit 3b audit report.
