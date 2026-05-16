---
title: "Integrity Toolkit v1.2 A.2 — Commit 4 landing audit (grandfather sweep + catalog refresh)"
date: 2026-05-15
author: claude-code
status: draft
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_2_a2_spec_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_a2_commit1_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_2_a2_commit2_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_2_a2_commit3_landing_2026-05-15.md
---

# A.2 commit 4 landing audit — grandfather sweep + catalog count refresh

Companion to: `9c8979a` (SHA back-filled by commit 5 per Convention #12).
Builds on: `e079c7b` (commit 1), `df21312` (commit 2), `926aa30` (commit 3).

## A. Change summary

Runs the grandfather sweep with `--force-sweep-category
toolkit-own-unused` (NOT `--sweep-live-source`) to grandfather the
24 newly-surfaced toolkit-own-unused findings in place. Refreshes the
nine drifted catalog category counts and fills the
`toolkit-own-unused (?)` placeholder added in commit 2.

Also lands a small infrastructure addition to
`tools/integrity/integrity/snapshot.py`: registers
`toolkit-own-unused` in the `_KNOWN_CATEGORIES` tuple used by
`emit_grandfather_report()`. Without this, the new category would
default to `"other"` in the report and Decision 6's catalog refresh
machinery would have to special-case it. Surfaced during the post-
sweep `--grandfather-report` run; resolved inline in this commit.

## B. File inventory

Sweep modifications (15 files, 34 annotations):

```
tools/integrity/integrity/cat1_citations/checks/bare_path.py    (1 annotation)
tools/integrity/integrity/cat1_citations/grammar.py             (1)
tools/integrity/integrity/cat1_citations/resolver.py            (1)
tools/integrity/integrity/cat2_contracts/stack_b.py             (3)
tools/integrity/integrity/cat3_numerical/cubic_kernel.py        (2)
tools/integrity/integrity/cat3_numerical/d3q19_verify.py        (2)
tools/integrity/integrity/cat3_numerical/generate_expected.py   (2)
tools/integrity/integrity/common/annotations.py                 (1)
tools/integrity/integrity/common/audit_log.py                   (2)
tools/integrity/integrity/common/stack_paths.py                 (2)
tools/integrity/integrity/grandfather.py                        (4)
tools/integrity/integrity/runner.py                             (3)
docs/diagnostics/_audits/integrity_v1_2_a2_commit3_landing_2026-05-15.md  (1 audit-report-grammar-example)
docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md  (~7 audit-bare-path + grammar-example)
docs/diagnostics/_audits/integrity_v1_2_a2_spec_2026-05-15_architect1.md   (~3 audit-report-grammar-example)
```

Per-category sweep totals (verbatim from the sweep CLI output):

```
                   toolkit-own-unused: 24
                      audit-bare-path: 6
         audit-report-grammar-example: 3
                       audit-citation: 1
```

LIVE-SOURCE other-cat1 protections preserved: **39 findings skipped**
(per `skipped as live-source (other-cat1): 39`). These are the
pre-existing other-cat1 / other-cat1-bare-path findings on
particle-fluids/, docs/phase12_lattice_boltzmann.md, and similar
live-source paths. None of the toolkit-own-unused findings were
skipped; the `--force-sweep-category` flag correctly opted them in.

Catalog refresh (counts changed; descriptive text unchanged):

| Category | Catalog (pre-A.2) | Live (post-sweep) |
|---|---|---|
| `audit-citation` | 597 | 100 |
| `audit-doc-1810` | 15 | 17 |
| `spec-grammar-example` | 17 | 18 |
| `toolkit-own-source` | 22 | 25 |
| `retro-grammar-example` | 2 | 8 |
| `audit-report-grammar-example` | 19 | 49 |
| `other-cat1` | 66 | 36 |
| `cat2-stack-c-unused` | 111 | 110 |
| `audit-bare-path` | 635 | 735 |
| `retro-bare-path` | 11 | 18 |
| `toolkit-own-unused` | (new — `?` placeholder from commit 2) | 24 |

Infrastructure:

- `tools/integrity/integrity/snapshot.py` — add `toolkit-own-unused`
  to `_KNOWN_CATEGORIES`. One-line addition.

## C. Pre-sweep snapshot (toolkit-own-unused inventory)

The 24 findings the sweep grandfathered (captured pre-sweep via
`python3 -m integrity --check cat2.public-symbol-used-toolkit
--output json --mode warn-only --no-audit-log > /tmp/a2_pre_sweep.json`):

```
tools/integrity/integrity/cat1_citations/checks/bare_path.py:91   BarePathResolution
tools/integrity/integrity/cat1_citations/grammar.py:140            UpstreamCitation
tools/integrity/integrity/cat1_citations/resolver.py:18            ResolutionResult
tools/integrity/integrity/cat2_contracts/stack_b.py:26             PublicSymbolB
tools/integrity/integrity/cat2_contracts/stack_b.py:34             is_node_available
tools/integrity/integrity/cat2_contracts/stack_b.py:38             ensure_helper_built
tools/integrity/integrity/cat3_numerical/cubic_kernel.py:33        TestPoint
tools/integrity/integrity/cat3_numerical/cubic_kernel.py:41        DriverEvaluation
tools/integrity/integrity/cat3_numerical/d3q19_verify.py:187       opposite_index
tools/integrity/integrity/cat3_numerical/d3q19_verify.py:192       assert_close
tools/integrity/integrity/cat3_numerical/generate_expected.py:31   cubic_W
tools/integrity/integrity/cat3_numerical/generate_expected.py:45   cubic_gradW_magnitude
tools/integrity/integrity/common/annotations.py:17                 Annotation
tools/integrity/integrity/common/audit_log.py:26                   audit_log_path
tools/integrity/integrity/common/audit_log.py:34                   append_findings
tools/integrity/integrity/common/stack_paths.py:10                 StackPaths
tools/integrity/integrity/common/stack_paths.py:16                 stack_paths
tools/integrity/integrity/grandfather.py:32                        Classification
tools/integrity/integrity/grandfather.py:247                       comment_form_for_md_inside_fence
tools/integrity/integrity/grandfather.py:328                       collect_findings
tools/integrity/integrity/grandfather.py:356                       group_findings_by_target
tools/integrity/integrity/runner.py:32                             CliArgs
tools/integrity/integrity/runner.py:82                             discover_checks
tools/integrity/integrity/runner.py:102                            run_checks
```

24 findings. After the sweep, all 24 carry an `integrity-allow:
cat2.public-symbol-used-toolkit; ...` comment immediately preceding
the cited line.

## D. P1.8 coordination check

- Invocation: `python3 tools/integrity/scripts/grandfather_sweep.py
  --force-sweep-category toolkit-own-unused`
- `--sweep-live-source` flag NOT passed.
- Sweep CLI summary reports `skipped as live-source (other-cat1): 39`,
  matching the pre-A.2 LIVE-SOURCE other-cat1 baseline. None of those
  39 were touched.
- Of the 15 files modified, 12 are under `tools/integrity/integrity/`
  (the toolkit's own source, the intended target of force-sweep) and
  3 are under `docs/diagnostics/_audits/` (audit reports — already
  sweep-eligible via `SWEEPABLE_PATH_PREFIXES`).
- Zero modifications to `particle-fluids/`,
  `docs/phase12_lattice_boltzmann.md`, or any other sim-source path.
- Convention I (cross-batch scope discipline) honored: the sweep did
  not opportunistically touch the 39 protected LIVE-SOURCE findings.

## E. Final gate state

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 5 pass, 0 soft-warn, 44 hard-fail, 1247 suppressed
```

The gate is back to **44 hard-fail** — exact match with the pre-A.2
LIVE-SOURCE baseline. Suppressed count is 1247 (vs 1213 pre-A.2;
delta +34 matches the sweep CLI's `34 annotations added` summary
exactly).

`python3 -m integrity --check cat2.public-symbol-used-toolkit
--output json --mode warn-only --no-audit-log` now reports 24
findings all with `suppressed: true` — the toolkit-own-unused
baseline is fully grandfathered.

## F. Decision-7 validation: `stack_paths()` annotation

```
$ grep -n "integrity-allow: cat2.public-symbol-used-toolkit" \
    tools/integrity/integrity/common/stack_paths.py
10:# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
17:# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
```

Two annotations on `stack_paths.py`: one for `StackPaths` (the
`@dataclass`) at line 10 and one for `stack_paths()` (the function)
at line 17 (was line 16 pre-annotation; +1 for the inserted comment).
The catalog's `toolkit-own-unused` tracking-notes entry naming
`stack_paths()` as "the first tracked entry" remains accurate.

## G. Snapshot.py extension (Decision-6 follow-through)

Prior to this commit, `tools/integrity/integrity/snapshot.py` only
recognized eighteen of the categories in
`tools/integrity/docs/grandfather-catalog.md`. The
`emit_grandfather_report()` function uses `_KNOWN_CATEGORIES` to
match suppression reasons back to categories. New categories not in
that tuple fell through to the catch-all `_extract_category()` return
of `"other"`.

Without the extension, the first run of `--grandfather-report` after
the sweep reported `other: 24` — 24 toolkit-own-unused findings
miscounted as "other." Adding `toolkit-own-unused` to
`_KNOWN_CATEGORIES` corrects this. Verification:

```
$ python3 -m integrity --grandfather-report --no-history-append --no-audit-log
...
                   toolkit-own-unused: 24
...
```

The fix is one line; idempotent; backward-compatible (the `"other"`
bucket simply shrinks by 24 in favor of the named bucket).

## H. Tests

The sweep does not touch tests. `pytest tools/integrity/tests/ -q`
still reports 153 passed.

The `test_real_repo_finding_count_in_expected_range` slow test in
`test_cat2_public_symbol_used_toolkit.py` still passes against the
real repo. Note that the test's `run(repo_root)` invocation returns
findings BEFORE the runner's suppression layer is applied; so the 24
findings are still produced by the check itself, then suppressed by
the just-landed annotations at runner time. The slow test asserts on
the pre-suppression count; that remains within [3, 30].

## I. Risk and reversal

The grandfather sweep is idempotent and reversible. Reverting this
commit removes all 34 inline annotations and reverts the catalog
counts; running the sweep again would regenerate them. The
snapshot.py change is similarly trivially reversible.

## J. Outstanding for commit 5

SHA back-fill across all four audit reports' `Companion to:
<commit-N-sha>` placeholders. Separate commit per Convention #12;
no `--amend`.

## Addendum A — landing-time gate-state correction (2026-05-15)

Appended at landing per Convention C / audit-prose freshness
convention (v1.3 candidates roadmap Addendum A pattern). The post-A.2
strict-mode gate measured against current disk is **45 hard-fail**, not
the 44 baseline this report's § E claimed.

- **What the +1 is:** one `cat1.annotation-form` finding on this audit
  report itself at line 126, where the prose `24 findings. After the
  sweep, all 24 carry an
  ` literal-mention-of-grammar string was added to the body AFTER the
  commit-4 sweep had already run earlier in the commit. The sweep
  picked up the analogous string in the commit-3 audit (added to the
  body BEFORE commit 4's sweep) and produced an
  audit-report-grammar-example annotation there, but could not see
  this report's body because the body did not exist yet at sweep
  time.
- **Why kept as +1 rather than corrected in-place:** Per the
  audit-prose freshness convention, the body of a landed audit
  report is not silently edited. Adding an inline suppression
  annotation above line 126 is mechanically a sweep operation but
  semantically a body edit on the landed artifact. The next batch's
  sweep companion will absorb this finding as a routine sweep result
  (the same way commit-4's sweep absorbed commit-1/2's audit-body
  grammar literals).
- **Why not run a follow-up sweep now:** Hard Rule #9 specifies
  per-category force-sweep, not blanket. A blanket sweep here would
  technically work for this single finding but would establish a
  precedent of "sweep again to clean up the audit-of-the-just-finished-
  sweep," which is recursive and risks scope creep across batches.
  The freshness convention is the cleaner answer: surface and defer.

The +1 does not affect Decisions 7 or 8 verification; the underlying
LIVE-SOURCE baseline (per the strict-mode dump) is still 44, plus the
+1 audit-doc residue.

If a reader runs `--mode strict --no-audit-log` post-this-addendum
and sees 45 rather than 44, the difference is this single
annotation-form finding on this report's § C prose. The next sweep
will eliminate it.
