---
title: "Integrity Toolkit v1.2 A.2 — Commit 3 landing audit (register check)"
date: 2026-05-15
author: claude-code
status: draft
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_2_a2_spec_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_a2_commit1_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_2_a2_commit2_landing_2026-05-15.md
---

# A.2 commit 3 landing audit — register cat2.public-symbol-used-toolkit

Companion to: `<commit-3-sha>` (SHA back-filled by commit 5 per Convention #12).
Builds on: `e079c7b` (commit 1), `df21312` (commit 2).

## A. Change summary

Registers the new check in
`tools/integrity/integrity/cat2_contracts/checks/__init__.py`:

- Imports `public_symbol_used_toolkit` alongside the four existing
  cat2 check modules
- Appends `(public_symbol_used_toolkit.CHECK_ID,
  public_symbol_used_toolkit)` to `REGISTERED_CHECKS`

This activates the check in the runner. The gate is intentionally red
between commit 3 and commit 4's sweep — the same intermediate-state
pattern as v1.2 A.3 commits 3/4.

## B. File inventory

Modified:

- `tools/integrity/integrity/cat2_contracts/checks/__init__.py`
  (one import added, one tuple entry appended)

Plus this audit report.

## C. Decision 8 verification — finding count in [3, 30]

```
$ python3 -m integrity --check cat2.public-symbol-used-toolkit \
    --output json --mode warn-only --no-audit-log 2>/dev/null \
  | python3 -c "import json,sys; \
     d=json.load(sys.stdin); \
     print(len(d['findings']))"
24
```

**24 findings, all unsuppressed.** Within the expected band [3, 30]
per Decision 8. Decision 8 satisfied; commit 4 may proceed.

Full finding inventory (verbatim, sorted by file:line):

```
tools/integrity/integrity/cat1_citations/checks/bare_path.py:91  BarePathResolution
tools/integrity/integrity/cat1_citations/grammar.py:140           UpstreamCitation
tools/integrity/integrity/cat1_citations/resolver.py:18           ResolutionResult
tools/integrity/integrity/cat2_contracts/stack_b.py:26            PublicSymbolB
tools/integrity/integrity/cat2_contracts/stack_b.py:34            is_node_available
tools/integrity/integrity/cat2_contracts/stack_b.py:38            ensure_helper_built
tools/integrity/integrity/cat3_numerical/cubic_kernel.py:33       TestPoint
tools/integrity/integrity/cat3_numerical/cubic_kernel.py:41       DriverEvaluation
tools/integrity/integrity/cat3_numerical/d3q19_verify.py:187      opposite_index
tools/integrity/integrity/cat3_numerical/d3q19_verify.py:192      assert_close
tools/integrity/integrity/cat3_numerical/generate_expected.py:31  cubic_W
tools/integrity/integrity/cat3_numerical/generate_expected.py:45  cubic_gradW_magnitude
tools/integrity/integrity/common/annotations.py:17                Annotation
tools/integrity/integrity/common/audit_log.py:26                  audit_log_path
tools/integrity/integrity/common/audit_log.py:34                  append_findings
tools/integrity/integrity/common/stack_paths.py:10                StackPaths
tools/integrity/integrity/common/stack_paths.py:16                stack_paths
tools/integrity/integrity/grandfather.py:32                       Classification
tools/integrity/integrity/grandfather.py:247                      comment_form_for_md_inside_fence
tools/integrity/integrity/grandfather.py:328                      collect_findings
tools/integrity/integrity/grandfather.py:356                      group_findings_by_target
tools/integrity/integrity/runner.py:32                            CliArgs
tools/integrity/integrity/runner.py:82                            discover_checks
tools/integrity/integrity/runner.py:102                           run_checks
```

This is the **toolkit-own-unused baseline** that commit 4's sweep
will grandfather.

## D. Decision 7 verification — stack_paths() canary

```
grep -F "common/stack_paths.py:16" <warn-only output> | wc -l
1
```

`stack_paths()` at `tools/integrity/integrity/common/stack_paths.py:16`
is present in the finding list. Decision 7 satisfied. Commit 4 will
land the per-file annotation that the catalog's
`toolkit-own-unused` section names as the first tracked entry.

## E. Strict-mode gate (intentional red state)

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 5 pass, 0 soft-warn, 77 hard-fail, 1213 suppressed
```

Decomposition of the 77 hard-fails:

- 44 LIVE-SOURCE other-cat1 baseline (pre-A.2, unchanged)
- 9 audit-doc bare-path findings from the new commit-1 audit
  report (introduced in commit 1)
- 24 toolkit-own-unused findings from the newly-registered check
  (introduced in this commit)

All 24 toolkit-own-unused findings classify into the named
`toolkit-own-unused` category (verified by classifier rule landed in
commit 2). They pass through the P1.8 live-source filter — the filter
only short-circuits other-cat1 / other-cat1-bare-path findings, so
named categories like `toolkit-own-unused` are sweep-eligible by
default. Commit 4's `--force-sweep-category toolkit-own-unused`
invocation is therefore explicit-form: it would also be a no-op
without the flag, but the flag documents intent and provides forward-
compatibility per the Decision-4 framing.

Commit 4 closes the gate by grandfather-annotating these 24 findings
in place, plus the 9 audit-doc bare-path findings via the existing
audit-bare-path classifier rule. Expected post-commit-4 gate state:
roughly 44 hard-fail again (pre-A.2 baseline restored).

## F. Tests

```
$ pytest tools/integrity/tests/ -q
153 passed, 1 warning in 133.97s
```

Test count unchanged from commit 2 (this commit is a registry edit, no
new tests). The slow Decision-8 sanity test in
`test_cat2_public_symbol_used_toolkit.py` continues to pass.

## G. Pre-condition checks (Decisions 7, 8) for commit 4

The two gates that commit 4 depends on:

- [x] Finding count in [3, 30]: actual 24
- [x] `stack_paths()` present in findings: actual yes, at line 16

Both pre-conditions for commit 4 are satisfied. Proceeding to the
sweep companion commit.

## H. Risk and reversal

This commit is a 1-line change (plus accompanying import). Reverting
restores the previous registry tuple verbatim.

## I. Outstanding for commit 4

- Run `python3 tools/integrity/scripts/grandfather_sweep.py
  --force-sweep-category toolkit-own-unused`
- Verify: the sweep summary reports `toolkit-own-unused: 24` (or
  whatever the count is at sweep time) and the "skipped as
  live-source" count remains at the pre-A.2 other-cat1 baseline (no
  toolkit-own-unused entries skipped)
- Verify: post-sweep, `stack_paths.py` carries an
  `integrity-allow: cat2.public-symbol-used-toolkit` annotation
- Refresh the `toolkit-own-unused (?)` count in
  `grandfather-catalog.md` and any other drifted category counts
  surfaced by the post-sweep `--grandfather-report --no-history-append`
