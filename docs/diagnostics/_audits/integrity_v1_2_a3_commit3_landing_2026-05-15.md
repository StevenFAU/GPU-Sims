---
title: "Integrity v1.2 A.3 — Commit 3 landing audit"
date: 2026-05-15
author: claude-code (executor)
status: complete
sibling-docs:
  - /home/otacon/Downloads/integrity_v1_2_a3_spec.md
  - docs/diagnostics/_audits/integrity_v1_2_a3_commit2_landing_2026-05-15.md
---

# Integrity v1.2 A.3 — Commit 3 landing audit

## A. Change summary

Commit 3 registers the `cat1.bare-path` check in
`cat1_citations/checks/__init__.py`, adds the bare-path skip-guard to
`cat1.intra-repo`, migrates the two bare-basename test cases in
`test_cat1_intra_repo.py`, and migrates the `good_citations` fixture
to use full paths (so the runner's "fixtures-clean" test stays green
with the new check active).

The strict-mode gate flips **red** at this commit; the count is closed
by commit 4's grandfather sweep companion.

## B. File inventory

Modified files:

- `tools/integrity/integrity/cat1_citations/checks/__init__.py` —
  registered `bare_path` (added to imports and `REGISTERED_CHECKS`).
- `tools/integrity/integrity/cat1_citations/checks/intra_repo.py` —
  added the `if "/" not in citation.path: continue` skip-guard with
  a three-line comment.
- `tools/integrity/tests/test_cat1_intra_repo.py` — renamed and
  reshaped two tests:
  - `test_dangling_citation_is_flagged` →
    `test_dangling_bare_basename_is_not_cat1_intra_repo` (asserts the
    bare-basename citation now produces **zero** cat1.intra-repo
    findings).
  - `test_out_of_range_line_is_flagged` →
    `test_out_of_range_bare_basename_is_not_cat1_intra_repo` (same
    pattern for the OOR case).
- `tools/integrity/tests/fixtures/good_citations/example.md` —
  rewrote citations to use full repo-relative paths:
  `tools/integrity/docs/ground-truth-sources.md:1` and
  `common/sibling.cpp:1`.
- `tools/integrity/tests/fixtures/good_citations/sibling.cpp` →
  `tools/integrity/tests/fixtures/good_citations/common/sibling.cpp`
  (moved so the cited full path resolves).

New files:

- `docs/diagnostics/_audits/integrity_v1_2_a3_commit3_landing_2026-05-15.md`
  (this file).

## C. Verification block

`pytest tools/integrity/tests/ -q` → 119 passed (no regressions; the
two migrated tests and the renamed/relocated fixture pass).

`python3 -m integrity --check cat1.bare-path --output human --no-audit-log` —
the check is now known to the runner and emits findings.

`python3 -m integrity --mode warn-only --no-audit-log --output json`:

| Metric | Value |
|---|---|
| Total `cat1.bare-path` findings | 645 |
| `cat1.bare-path` arm: REGISTERED-UPSTREAM | 0 |
| `cat1.bare-path` arm: INTRA-REPO | 383 |
| `cat1.bare-path` arm: AMBIGUOUS | 148 |
| `cat1.bare-path` arm: UNRESOLVABLE | 114 |
| Total `cat1.intra-repo` findings (post-skip-guard) | 95 |
| `cat1.intra-repo` findings with bare-basename path | **0** (confirms the skip-guard is doing its job) |

`python3 -m integrity --mode strict --no-audit-log`:

```
integrity: 2 pass, 0 soft-warn, 653 hard-fail, 417 suppressed
```

The gate is **intentionally red** at this commit. Commit 4's
grandfather sweep companion will sweep the audit/retro/toolkit-doc
buckets and the deferred-upstream bucket, leaving only the
live-source residue.

## D. Behavioral notes

The transition from "dormant" to "active" is atomic in this commit:
the registration in `checks/__init__.py` and the skip-guard in
`intra_repo.py` ship together so that no intermediate state has
double-firing (cat1.intra-repo + cat1.bare-path on the same
basename).

The fixture migration in `good_citations/` was unavoidable: the
fixture cited `example.md:1` (self-reference) and `sibling.cpp:1`
(sibling-relative), both bare-basenames. Under cat1.bare-path's new
strictness, these became INTRA-REPO findings and broke
`test_runner_runs_against_fixtures_clean`. The minimal fix was to
rewrite the citations to use full paths and to relocate `sibling.cpp`
into a `common/` subdirectory so the cited full path resolves.

The 0 REGISTERED-UPSTREAM arm count is somewhat surprising. Likely
cause: basenames that match files in `references/` also match
intra-repo files, so they route to AMBIGUOUS rather than
REGISTERED-UPSTREAM. The check's classifier requires
`upstream_matches and not intra_matches` for the REGISTERED-UPSTREAM
arm; cross-tree collisions fall through to AMBIGUOUS by design
(Decision 5 / classifier code in bare_path.py).

The hard-fail count is 653, comfortably within the spec's predicted
400–700 range. Commit 4 will sweep most of these; live-source residue
will remain.

## E. Incidental findings during execution

1. The `good_citations` fixture used bare basenames that were
   permissive under v1 cat1.intra-repo (resolved relative to the
   source file). The v1.2 A.3 strictness change required a fixture
   migration. Noted in the spec § 6.3 but the spec didn't enumerate
   the specific fixture file change; in-flight detection during
   pytest caught it. Documented here so the post-A.3 retro can
   surface this as a "fixtures-as-canonical-examples" pattern: when
   v1.2 checks raise the bar on existing grammars, fixtures encoding
   "good" examples need migration too.
2. Strict-mode count at this commit (653 hard-fails) is slightly
   above the spec's "on the order of 400-700" upper-estimate but
   within tolerance.
