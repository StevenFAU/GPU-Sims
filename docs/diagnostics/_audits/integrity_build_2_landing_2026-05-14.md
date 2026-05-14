# Integrity Toolkit — Commit 2 Landing — 2026-05-14

Second of eight commits building the cross-stack integrity verification
toolkit per `docs/integrity-toolkit-spec.md` § 11. This commit implements
the citation extractor + intra-repo resolver and lands two checks:
`cat1.intra-repo` and `cat1.annotation-form`. The runner's check registry
is now populated.

Companion to:

- Spec: `docs/integrity-toolkit-spec.md` § 6.1–6.4 (Cat 1), § 3.2
  (annotation grammar)
- Prior commit's audit: `integrity_build_1_landing_2026-05-14.md`

---

## A. Change summary

Lands the first two Category 1 checks. `cat1.intra-repo` walks every
tracked non-excluded source/doc file, extracts `file:line[-end]`
citations via a single-class regex (chosen to avoid catastrophic
backtracking), filters by recognized file extension, and reports
HARD_FAIL for any citation that does not resolve (missing path) or
falls outside the cited file's line range. `cat1.annotation-form`
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
finds every `integrity-allow:` invocation and validates the grammar
per spec § 3.2 — check_id form, reason length ≥ 8, issue-ref form,
blanket-`*` rejection. The runner's `discover_checks()` and
`run_checks()` are wired to the cat1 registry; the check modules
gracefully fall back to a recursive directory walk when invoked
against a non-git directory (the test fixtures rely on this).

The CLI surface from commit 1 is unchanged. The runner accepts
`--root`, `--cat`, `--check`, `--mode`, `--output`, `--no-audit-log`
exactly as before. No CI workflow invokes the toolkit yet; that is
commit 4.

---

## B. File inventory

### New files

| File | Lines |
| --- | --- |
| `tools/integrity/integrity/cat1_citations/grammar.py` | 111 |
| `tools/integrity/integrity/cat1_citations/resolver.py` | 103 |
| `tools/integrity/integrity/cat1_citations/checks/intra_repo.py` | 84 |
| `tools/integrity/integrity/cat1_citations/checks/annotation.py` | 119 |
| `tools/integrity/tests/test_cat1_intra_repo.py` | 65 |
| `tools/integrity/tests/test_cat1_annotation.py` | 47 |
| `tools/integrity/tests/fixtures/good_citations/example.md` | 5 |
| `tools/integrity/tests/fixtures/good_citations/sibling.cpp` | 2 |
| `tools/integrity/tests/fixtures/good_citations/good_annotation.py` | 5 |
| `tools/integrity/tests/fixtures/bad_citations/dangling.md` | 3 |
| `tools/integrity/tests/fixtures/bad_citations/out_of_range.md` | 3 |
| `tools/integrity/tests/fixtures/bad_citations/target.cpp` | 2 |
| `tools/integrity/tests/fixtures/bad_citations/bad_annotation.py` | 8 |

### Modified files

| File | Lines (now) | Change |
| --- | --- | --- |
| `tools/integrity/integrity/runner.py` | 164 | `discover_checks`/`run_checks` wired to cat1 registry; `passes` count now per-check |
| `tools/integrity/integrity/cat1_citations/__init__.py` | 1 | Module docstring |
| `tools/integrity/integrity/cat1_citations/checks/__init__.py` | 9 | Registers `(intra_repo, annotation)` |
| `tools/integrity/tests/test_runner.py` | 54 | Replaced commit-1 stub tests with --root/exit-code/warn-only coverage |

Diff totals from `git show --stat 0822f6a`: 17 files changed, 634
insertions, 17 deletions.

---

## C. Verification

### C.1 pytest

```text
$ python3 -m pytest tests/ -v
============================= test session starts =============================
platform linux -- Python 3.12.3, pytest-8.4.2, pluggy-1.6.0
rootdir: /home/otacon/Projects/GPU-Sims/GPU-Sims/tools/integrity
configfile: pyproject.toml
plugins: cov-5.0.0, anyio-4.13.0
collecting ... collected 16 items

tests/test_cat1_annotation.py::test_good_annotations_yield_no_findings PASSED [  6%]
tests/test_cat1_annotation.py::test_bad_annotations_yield_three_findings PASSED [ 12%]
tests/test_cat1_annotation.py::test_validate_check_id_grammar PASSED     [ 18%]
tests/test_cat1_annotation.py::test_validate_reason_length PASSED        [ 25%]
tests/test_cat1_annotation.py::test_validate_issue_ref PASSED            [ 31%]
tests/test_cat1_intra_repo.py::test_good_citations_yield_no_findings PASSED [ 37%]
tests/test_cat1_intra_repo.py::test_dangling_citation_is_flagged PASSED  [ 43%]
tests/test_cat1_intra_repo.py::test_out_of_range_line_is_flagged PASSED  [ 50%]
tests/test_cat1_intra_repo.py::test_template_token_is_not_a_citation PASSED [ 56%]
tests/test_cat1_intra_repo.py::test_time_of_day_is_not_a_citation PASSED [ 62%]
tests/test_cat1_intra_repo.py::test_ipv4_port_is_not_a_citation PASSED   [ 68%]
tests/test_runner.py::test_runner_parses_args_cleanly PASSED             [ 75%]
tests/test_runner.py::test_runner_rejects_bad_cli PASSED                 [ 81%]
tests/test_runner.py::test_runner_runs_against_fixtures_clean PASSED     [ 87%]
tests/test_runner.py::test_runner_runs_against_bad_fixtures_fails PASSED [ 93%]
tests/test_runner.py::test_runner_warn_only_mode_downgrades PASSED       [100%]

============================== 16 passed in 0.02s ==============================
```

### C.2 Smoke run — strict mode against the real repo

```text
$ python3 -m integrity --output human --no-audit-log
integrity: 0 pass, 0 soft-warn, 832 hard-fail, 0 suppressed
[... 832 finding lines elided ...]
Exit code: 1
```

Breakdown by check_id:

| check_id | hard-fails |
| --- | --- |
| `cat1.intra-repo` | 810 |
| `cat1.annotation-form` | 22 |
| **total** | **832** |

Top intra-repo source files (these are where the citations are written,
not the cited targets):

| count | source file |
| --- | --- |
| 210 | `docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md` |
| 162 | `docs/diagnostics/_audits/phase11_5_probe_2026-05-14_architect1.md` |
| 92 | `docs/diagnostics/_audits/phase11_5_probe2_2026-05-14_architect1.md` |
| 89 | `docs/diagnostics/_audits/sims_lenia_probe1_2026-05-14_architect3b.md` |
| 51 | `docs/diagnostics/_audits/commoncpp_unexercised_2026-05-14_architect2.md` |
| 46 | `docs/diagnostics/_audits/integrity_toolkit_probe_2026-05-14_architect1.md` |
| 43 | `docs/diagnostics/_audits/phase11_5_probe3_2026-05-14_architect1.md` |
| 22 | `docs/diagnostics/_audits/phase11_5_setup1_2026-05-14_setup1.md` |
| 13 | `docs/diagnostics/_audits/sims_lenia_chakazul_2026-05-14_architect3b.md` |
| 11 | `docs/diagnostics/_audits/sims_lenia_synthesis_2026-05-14_architect3b.md` |

The vast majority of these are vendor/upstream citations to paths
under `references/` (e.g. `references/SPlisHSPlasH/.../TimeStepDFSPH.cpp:NNN`),
which is itself in the canonical exclusion list — so the resolver
correctly refuses to confirm them as intra-repo. Commit 3's
`cat1.upstream-citation` will handle these paths against the vendor
trees explicitly (re-resolution into `references/`).

Annotation-form sources:

| count | source file |
| --- | --- |
| 17 | `docs/integrity-toolkit-spec.md` |
| 2  | `tools/integrity/integrity/common/annotations.py` |
| 1  | `tools/integrity/integrity/common/exclusions.py` |
| 1  | `docs/diagnostics/_audits/integrity_toolkit_probe_2026-05-14_architect1.md` |
| 1  | `docs/diagnostics/_audits/integrity_build_1_landing_2026-05-14.md` |

The spec-doc hits are illustrative grammar examples (intentional). The
two `common/annotations.py` hits are the docstring and the parser
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
regex itself containing the literal `integrity-allow:` token. The
`common/exclusions.py` hit is the docstring naming the
`cat1.exclusion-list` check. These will be grandfathered or
suppressed in commit 4.

### C.3 Smoke run — warn-only mode against the real repo

```text
$ python3 -m integrity --mode warn-only --no-audit-log
integrity: 0 pass, 0 soft-warn, 832 hard-fail, 0 suppressed
[... 832 finding lines elided ...]
Exit code: 0
```

Findings are still surfaced, but `--mode warn-only` suppresses the
exit-1 gate per spec § 5.2 (the CI-only gating). Confirms the
local-development path works correctly.

---

## D. Behavioral expectations

- **Tests pass: 16/16.** Up from commit 1's 3/3, exercising grammar,
  resolution, annotation validation, and the runner's --root /
  warn-only / exit-code paths against synthetic fixtures.
- **No CI gates affected.** The six pre-existing workflows still gate
  as before. The integrity toolkit is not yet invoked from any
  workflow — that lands in commit 4 after the grandfather sweep.
- **`python -m integrity` is now failing locally.** Running the
  toolkit against the real repo returns exit 1 with 832 HARD_FAIL
  findings. This is the expected pre-grandfather state: the toolkit's
  job is to surface every citation/annotation discrepancy so commit
  4 can catalogue them; nothing is suppressed yet.
- **Developers running the toolkit pre-CI** should use `--mode
  warn-only` until commit 4 lands the grandfather catalog. The README
  documents this.
- **Toolkit performance.** Initial run had a regex with nested
  quantifiers that triggered catastrophic backtracking on the repo's
  larger md files; this was caught during smoke testing and fixed in
  `cat1_citations/grammar.py` before commit. Real-repo runs now
  complete in under 120 seconds.

---

## E. Preview — commit 3 scope

Commit 3 will implement the remaining Category 1 checks per spec
§ 6.5–6.7:

- `cat1.upstream-citation` — parse `<UpstreamName> <version>
  <path>:<line>` form (grammar already sketched in
  `grammar.py`), resolve under `references/<UpstreamName>/`, and
  HARD_FAIL on unresolved paths or out-of-range lines.
- `cat1.upstream-anchor` — for each upstream-version pair cited
  anywhere in the repo, read the canonical anchor file
  (`references/<UpstreamName>/.git/HEAD` or a manifest TOML) and
  HARD_FAIL if the anchor doesn't match the version asserted in the
  citation. Resolves the upstream-anchor drift class identified in
  the probe audit.
- `cat1.unregistered-upstream` — every cited UpstreamName must
  appear in a manifest of registered upstream projects.
- Extend `runner.discover_checks` to register the three new modules.
- Fixture tree: `tests/fixtures/upstream_citations/` with valid +
  invalid forms, plus a stub `references/SyntheticUpstream/`
  for the anchor-resolution tests.

Commit 4 then lands the grandfather sweep (mark the 832 pre-existing
findings as suppressed with reason+issue) and the CI workflow.

---

## F. Incidental findings

1. **Catastrophic regex backtracking caught during commit 2.** The
   initial citation regex `(?:[A-Za-z0-9_./-]+/)*[A-Za-z0-9_.-]+\.[A-Za-z0-9.]+`
   had nested overlapping quantifiers — the `[A-Za-z0-9_./-]+` class
   includes `/`, and the outer `*` repeats over a class ending with
   `/`, allowing exponential partitioning of any path-like substring.
   Two concurrent test runs against the real repo hung at 100% CPU
   for 15+ minutes before being killed. Replaced with a single-class
   pattern `[A-Za-z0-9_./-]+\.[A-Za-z0-9.]+`. Cause-of-bug noted here
   for future check authors: nested quantifiers + slash-permissive
   character classes are an ambush waiting for arbitrary repo content.

2. **Vendor citations dominate the intra-repo finding count.** Of 810
   intra-repo HARD_FAILs, the overwhelming majority cite paths under
   `references/<Upstream>/` — which is in `CANONICAL_EXCLUSIONS` and
   so cannot resolve as an intra-repo target. Commit 3's
   `cat1.upstream-citation` will absorb these by re-resolving into
   the vendor trees. The intra-repo HARD_FAIL count is therefore
   expected to drop sharply after commit 3 lands; the residual
   intra-repo failures should be much smaller and easier to triage.

3. **Annotation-form false-positives in toolkit's own source.** The
   `LOOSE_RE` pattern in `annotation.py` finds any literal
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
   `integrity-allow:` token in any scanned file, including docstrings
   and the regex pattern in `common/annotations.py`. This is the
   intended behavior (the check needs to be able to flag malformed
   annotations), but it does mean the toolkit's own source contributes
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
   noise. Commit 4's grandfather sweep should add `integrity-allow:
   cat1.annotation-form; documentation-only literal of the grammar
   token; n/a` annotations at the docstring sites, or — if the spec
   permits — extend `LOOSE_RE` to skip annotations inside string
   literals (likely defer; the targeted suppressions are simpler).

4. **Audit-report annotation hits look like grandfather noise.** The
   one annotation-form finding in
   `integrity_build_1_landing_2026-05-14.md` is at line 176, which is
   the README example block from commit 1. Same for the spec doc and
   the probe audit. Worth a single sweep for "annotation grammar
   appears in markdown fenced code; do not parse" — commit 4 should
   either grandfather these or extend the parser to skip fenced
   blocks (likely defer to a future commit; the v1 spec does not
   require it).
