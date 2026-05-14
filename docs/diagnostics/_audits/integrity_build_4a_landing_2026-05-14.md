# Integrity Toolkit — Commit 4a Landing — 2026-05-14

First half of commit 4 per `docs/integrity-toolkit-spec.md` § 11. Generates
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
and applies inline `integrity-allow:` suppression annotations for every
pre-v1 HARD_FAIL finding, classified into seven categories. Post-sweep,
`python -m integrity --mode strict` exits 0 against the real repo.

Commit 4b lands the CI workflow that gates on this clean state.

Companion to:

- Spec: `docs/integrity-toolkit-spec.md` § 3.2 (annotation grammar),
  § 11 (commit plan), Appendix A (registry)
- Prior commit's audit: `integrity_build_3_landing_2026-05-14.md`

---

## A. Change summary

Adds three new toolkit modules plus a CLI script and the per-category
rationale doc:

- `integrity/grandfather.py` (logic module) — classifier + annotation
  renderer. Holds the dataclasses `Finding`, `Classification`, the
  seven-rule first-match classifier, comment-form selection by file
  extension, in-fence detection for markdown, idempotence check, and
  the `apply_annotations()` driver that runs the toolkit, groups
  findings by `(file, line)`, and writes annotations in-place.
- `integrity/common/suppression.py` (new) — implements spec § 3.2.
  After checks produce raw findings, this module walks each cited
  line's preceding block of annotation comments and marks the
  matching finding's `suppressed` flag. Wildcard form `cat<N>.*` is
  honored. Walks upward through a contiguous annotation block so
  stacked-annotation groups (mixed-category lines) all suppress.
- `scripts/grandfather_sweep.py` (new) — CLI wrapper. Delegates to
  `integrity.grandfather.apply_annotations()`. Supports `--dry-run`.
  Idempotent across runs.
- `docs/grandfather-catalog.md` (new) — per-category rationale,
  future-treatment plan, and removal procedure.
- `tests/test_grandfather_sweep.py` — 19 unit tests covering the
  classifier (all seven categories + fallthrough), comment-form
  selection, fenced-block detection, idempotence helpers, and
  render-line output for single + stacked-finding cases.

Minimal additions outside the grandfather subsystem to satisfy the
post-sweep clean-exit gate:

- `common/exclusions.py` — adds `tools/integrity/tests/fixtures/`
  to `CANONICAL_EXCLUSIONS`. Per spec § 11: "Fixtures live under
  tools/integrity/tests/fixtures/ and are deliberately isolated from
  the real repo." Commit 1 missed codifying this; without the
  exclusion, the toolkit scans malformed fixture files and produces
  findings that the commit 4a forbidden list prohibits suppressing.
  This is the spec-mandated v1 exclusion.
- `runner.py` — invokes `common/suppression.apply_suppressions()`
  between `run_checks()` and the summary computation. This wires up
  spec § 3.2 ("an annotation suppresses checks for the immediate
  next line or expression"), which had data-model scaffolding from
  commit 1 but no live application.

Sweep result against the real repo: 46 files modified, 772 annotation
lines added.

## B. Scope guardrails honored

In scope and shipped: the grandfather module, the CLI script, the
catalog doc, the unit tests, the real-repo sweep (46 files), the
suppression-application wiring required for strict-mode exit 0.

Out of scope and not touched:

- CI workflow file `.github/workflows/integrity.yml` — that is
  commit 4b.
- No check module logic was modified. The intra-repo, annotation-form,
  upstream-citation, upstream-anchor, and unregistered-upstream
  checks behave identically to commit 3.
- No grammar or resolver changes. `INTRA_REPO_RE`, `UPSTREAM_RE`,
  and the `resolver` module are byte-identical.
- No data-model changes. `Finding`, `RunSummary`, `FailureMode`,
  `Annotation` are unchanged.
- No edits to fixtures under `tools/integrity/tests/fixtures/`.
- No GitHub issues created. Issue-refs in suppressions are `n/a`
  per the prompt; category-level tracking lives in
  `grandfather-catalog.md`.

The two minor additions outside the grandfather subsystem (exclusion
of fixtures, suppression wiring) were unavoidable to satisfy the
post-sweep exit-0 requirement: the prompt forbids suppressing fixture
findings, and the runner didn't apply suppressions, so the v1 spec
intent could not be honored without these. See § F.1.

## C. Verification

### Pytest output (46/46 pass)

```
============================= test session starts ==============================
platform linux -- Python 3.12.3, pytest-8.4.2, pluggy-1.6.0 -- /usr/bin/python3
cachedir: .pytest_cache
rootdir: /home/otacon/Projects/GPU-Sims/GPU-Sims/tools/integrity
configfile: pyproject.toml
plugins: cov-5.0.0, anyio-4.13.0
collecting ... collected 46 items

tests/test_cat1_annotation.py::test_good_annotations_yield_no_findings PASSED
tests/test_cat1_annotation.py::test_bad_annotations_yield_three_findings PASSED
tests/test_cat1_annotation.py::test_validate_check_id_grammar PASSED
tests/test_cat1_annotation.py::test_validate_reason_length PASSED
tests/test_cat1_annotation.py::test_validate_issue_ref PASSED
tests/test_cat1_intra_repo.py::test_good_citations_yield_no_findings PASSED
tests/test_cat1_intra_repo.py::test_dangling_citation_is_flagged PASSED
tests/test_cat1_intra_repo.py::test_out_of_range_line_is_flagged PASSED
tests/test_cat1_intra_repo.py::test_template_token_is_not_a_citation PASSED
tests/test_cat1_intra_repo.py::test_time_of_day_is_not_a_citation PASSED
tests/test_cat1_intra_repo.py::test_ipv4_port_is_not_a_citation PASSED
tests/test_cat1_intra_repo.py::test_references_paths_are_not_flagged_as_intra_repo PASSED
tests/test_cat1_unregistered.py::test_registered_upstream_yields_no_findings PASSED
tests/test_cat1_unregistered.py::test_unregistered_upstream_is_flagged PASSED
tests/test_cat1_unregistered.py::test_unregistered_check_deduplicates_per_file_line PASSED
tests/test_cat1_upstream.py::test_good_upstream_citations_yield_no_findings PASSED
tests/test_cat1_upstream.py::test_wrong_version_is_flagged PASSED
tests/test_cat1_upstream.py::test_dangling_upstream_path_is_flagged PASSED
tests/test_cat1_upstream.py::test_unregistered_upstream_is_not_flagged_by_upstream_check PASSED
tests/test_cat1_upstream_anchor.py::test_anchor_mismatch_is_flagged PASSED
tests/test_cat1_upstream_anchor.py::test_missing_vendor_tree_is_flagged PASSED
tests/test_cat1_upstream_anchor.py::test_empty_registry_yields_no_findings PASSED
tests/test_grandfather_sweep.py::test_audit_citation_classification PASSED
tests/test_grandfather_sweep.py::test_live_shader_1810_classification PASSED
tests/test_grandfather_sweep.py::test_audit_doc_1810_classification PASSED
tests/test_grandfather_sweep.py::test_spec_grammar_example_classification PASSED
tests/test_grandfather_sweep.py::test_toolkit_own_source_classification PASSED
tests/test_grandfather_sweep.py::test_audit_report_grammar_example_classification PASSED
tests/test_grandfather_sweep.py::test_other_cat1_fallthrough PASSED
tests/test_grandfather_sweep.py::test_comment_form_python PASSED
tests/test_grandfather_sweep.py::test_comment_form_cpp PASSED
tests/test_grandfather_sweep.py::test_comment_form_glsl PASSED
tests/test_grandfather_sweep.py::test_comment_form_markdown PASSED
tests/test_grandfather_sweep.py::test_outside_fence PASSED
tests/test_grandfather_sweep.py::test_inside_fence PASSED
tests/test_grandfather_sweep.py::test_at_fence_open_line PASSED
tests/test_grandfather_sweep.py::test_annotation_already_present_exact_match PASSED
tests/test_grandfather_sweep.py::test_annotation_already_present_wildcard PASSED
tests/test_grandfather_sweep.py::test_annotation_not_present_different_category PASSED
tests/test_grandfather_sweep.py::test_render_single_finding_in_markdown PASSED
tests/test_grandfather_sweep.py::test_render_two_same_category_emits_one_specific_annotation PASSED
tests/test_runner.py::test_runner_parses_args_cleanly PASSED
tests/test_runner.py::test_runner_rejects_bad_cli PASSED
tests/test_runner.py::test_runner_runs_against_fixtures_clean PASSED
tests/test_runner.py::test_runner_runs_against_bad_fixtures_fails PASSED
tests/test_runner.py::test_runner_warn_only_mode_downgrades PASSED

============================== 46 passed in 0.07s ==============================
```

### Dry-run sweep output

```
grandfather-sweep: would modify 46 files; 772 annotations added
                       audit-citation: 744
                           other-cat1: 52
                       audit-doc-1810: 19
                 spec-grammar-example: 17
                     live-shader-1810: 9
                   toolkit-own-source: 9
         audit-report-grammar-example: 8
```

### Live sweep output (same numbers)

```
grandfather-sweep: modified 46 files; 772 annotations added
                       audit-citation: 744
                           other-cat1: 52
                       audit-doc-1810: 19
                 spec-grammar-example: 17
                     live-shader-1810: 9
                   toolkit-own-source: 9
         audit-report-grammar-example: 8
```

### Idempotence — second run produces no change

```
grandfather-sweep: modified 0 files; 0 annotations added
```

### Post-sweep strict-mode result

```
integrity: 1 pass, 0 soft-warn, 0 hard-fail, 856 suppressed
Exit: 0
```

856 suppressed (the 858 raw findings minus the 2 that were tail-of-upstream
overlap-suppressed by the intra-repo check before reaching the suppression
pass). The 772 inserted annotation lines cover 856 finding lines because
some lines carry multiple findings collapsed into a single annotation.

## D. Spec compliance

Spec § 3.2 (suppression annotation grammar): implemented at long
last. The runner calls `apply_suppressions()` after check dispatch.
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
The grammar tokens (`integrity-allow:`, `;`-delimited fields,
`cat<N>.*` wildcards, `#NNN`/`n/a` issue-refs) match the spec
verbatim — the annotation parser in `common/annotations.py` is
unchanged.

Spec § 3.4 (canonical exclusions): adds `tools/integrity/tests/fixtures/`.
Spec § 11 already documented the intent ("fixtures are deliberately
isolated from the real repo"). The exclusion entry codifies it.

Spec § 11 commit plan: 4a is the grandfather sweep; 4b will land the
CI workflow. The split is intentional — keeping the workflow file out
of 4a means commit 4a can be reviewed/reverted independently of CI
gating.

Spec Appendix A (registry): unchanged.

## E. Test plan

- 19 new unit tests covering: classification of each of the seven
  categories, fallthrough behavior, comment-form selection by
  extension, fenced-code-block detection, idempotence helpers, and
  render output for single + same-category-multi findings.
- The full 46-test suite passes against the post-sweep state, which
  confirms the toolkit's own behavior (intra-repo, upstream-citation,
  annotation-form, upstream-anchor, unregistered-upstream, runner
  fixtures) hasn't regressed.
- Idempotence test in practice: running the sweep twice produces
  zero diffs on the second run.

## F. Incidentals — surprises and deferred work

### F.1. Suppression wiring was missing from commit 1

The runner didn't read inline annotations or mark findings as
suppressed. Spec § 3.2 ("an annotation suppresses checks for the
immediate next line or expression") had data-model scaffolding
(`Finding.suppressed`, `Finding.suppression_reason`,
`Finding.suppression_issue`) but no live application code. Without
this, the grandfather sweep would have been inert — annotations
written but ignored. Adding `common/suppression.py` and the two-line
call in `runner.py` is the minimal wiring required to satisfy the
commit 4a success criterion (strict-mode exits 0). The wiring counts
as "implementing v1," not "modifying v1" per the prompt's intent.

### F.2. Reason text cannot contain `;`

The annotation grammar uses `;` as the field delimiter
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
(`integrity-allow: <check-id>; <reason>; <issue-ref>`). The initial
classifier produced reasons like `"audit-doc snapshot of pre-v1
codebase; citations were valid at audit time"` — semicolon in the
reason — which the grammar parser silently rejected. Found during
first strict-mode verification (0 suppressed despite 772 annotations
inserted). Fixed by rewriting all seven category reasons to use
parenthetical phrases instead of semicolons. Worth adding to the
spec as an explicit author-guidance note (deferred to commit 4b's
spec touch-ups, if any).

### F.3. Mixed-category lines yield stacked annotations

When a single line has findings classifying to multiple categories
(e.g., one `cat1.intra-repo` from audit-citation plus one
`cat1.upstream-citation` from audit-doc-1810), the renderer emits
one annotation per finding rather than a single wildcard
annotation. The naïve suppression check ("look at line N-1") only
sees the closest annotation. Fix: `apply_suppressions()` walks
upward through the contiguous block of annotation comments
immediately above the cited line, stopping at the first
non-annotation line. This honors the spec's "immediate next line"
language interpreted as "the contiguous annotation block
immediately preceding."

### F.4. `other-cat1` came in at 52 vs predicted "small remainder"

The prompt expected other-cat1 to be a small residual. Actual: 52.
Sources include: `continuous-ca/lenia-fft/docs/` notes citing
`LeniaNDK.py`, `project-state.md` citing files removed in cleanup
passes, `CHANGELOG.md` citing context.hpp/context.cpp from earlier
refactor phases, sph-water shader files citing TimeStepDFSPH.cpp
without a version prefix (so they fall through both the
upstream-citation grammar AND the references-prefix filter).
None are surprising on inspection — they are legitimately
heterogeneous and don't warrant their own category. Per the
catalog's "Future treatment: Per-entry review in v2" guidance,
these stay in other-cat1 until v2 review.

### F.5. Annotations inserted inside docstrings

The script inserts annotations on the line above the cited line
without language-aware AST analysis. When the cited line is inside
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
a Python docstring, the script's `# integrity-allow:` comment lands
inside the docstring's string content. This is syntactically valid
Python (the comment becomes text within the string) and the
suppression parser scans raw text per-line, so the annotation works
as intended for suppression purposes. It is ugly aesthetically.
Affected files include `common/annotations.py`, `cat1_citations/grammar.py`,
and `cat1_citations/checks/intra_repo.py` where the toolkit's own
source includes the grammar string as a docstring example. The
toolkit-own-source category was created precisely for this case.
Aesthetic clean-up would require AST-aware insertion (Python `ast`
module + tokenize) which is a v2 concern.

### F.6. 858 raw → 856 suppressed → 0 hard-fail

The arithmetic: commit 3 reported 848 findings. Post-spec-patch (the
intervening 641dc7a) the spec doc grew slightly, adding new
cat1.upstream-citation references. After adding the fixtures
exclusion, raw count is 858. The intra-repo check's
upstream-tail-overlap filter (commit 3's behavior) silently absorbs
2 of those during the check itself, so 856 findings reach the
suppression pass and are marked suppressed. Net hard-fail count is
0.
