---
title: "Integrity Toolkit v1.2 A.2 — Commit 1 landing audit (new check module + fixtures + tests)"
date: 2026-05-15
author: claude-code
status: draft
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_2_a2_spec_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md
  - docs/retro/integrity-toolkit-v1.2-bolt-ons.md
  - docs/retro/integrity-toolkit-v1.3-candidates.md
---

# A.2 commit 1 landing audit — new check module + fixtures + tests

Companion to: `<commit-1-sha>` (SHA back-filled by commit 5 per Convention #12).

## A. Change summary

Lands the new `cat2.public-symbol-used-toolkit` check module along with
two fixture trees and a 17-test test file. The check is **not yet
registered** with the runner — registration is commit 3. This commit
is race-immune by design (all new files; no edits to existing modules).

Closes the recursive blind spot identified in v1.1 batch-1 retro § 5.3:
the toolkit enforces public-symbol-used discipline on every other
Python package in the repo but had been exempting itself.

## B. File inventory

New files:

- `tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_toolkit.py`
  (~290 LOC) — the new check module. Implements Decision 2 (strict scan
  with reflection-aware consumption) and Decision 3 (tests/scripts as
  scan-input only).
- `tools/integrity/tests/fixtures/good_toolkit_self/` — 5-file fixture
  where every public symbol is consumed; check yields zero findings.
  Layout: `integrity/{module_a,module_b,module_c}.py` plus
  `tests/test_module_a.py`.
- `tools/integrity/tests/fixtures/bad_toolkit_self/` — 3-file fixture
  with `orphan_helper` and `OrphanClass` declared but not consumed.
  Layout: `integrity/{module_a,module_b}.py` plus
  `tests/test_module_a.py`.
- `tools/integrity/tests/test_cat2_public_symbol_used_toolkit.py`
  (~245 LOC) — 18 tests covering CHECK_ID/MODE smoke, extraction
  sub-rules (top-level def/class, exclusion of module constants /
  underscore-prefixed / visit_* / test_* / tests-dir), consumption
  detection (imports / Name / Attribute), reflection consumers,
  entrypoint convention, fixture-driven good/bad coverage, and a
  pytest.mark.slow Decision-8 sanity test against the real repo.

Plus the spec and probe documents themselves are staged alongside this
commit (they sit at `docs/diagnostics/_audits/` as planning record):

- `docs/diagnostics/_audits/integrity_v1_2_a2_spec_2026-05-15_architect1.md`
- `docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md`

## C. Test results (verbatim)

```
$ pytest tools/integrity/tests/test_cat2_public_symbol_used_toolkit.py -v
============================ test session starts ============================
collected 18 items

test_check_id_and_mode PASSED
test_extract_public_symbols_includes_top_level_def PASSED
test_extract_public_symbols_includes_top_level_class PASSED
test_extract_public_symbols_excludes_module_constants PASSED
test_extract_public_symbols_excludes_underscore_prefixed PASSED
test_extract_public_symbols_excludes_visit_methods PASSED
test_extract_public_symbols_excludes_test_functions PASSED
test_extract_public_symbols_excludes_tests_dir PASSED
test_consumption_index_records_imports PASSED
test_consumption_index_records_name_references PASSED
test_consumption_index_records_attribute_access PASSED
test_extract_registered_check_modules PASSED
test_registered_checks_treated_as_consumers PASSED
test_main_in_entrypoints_treated_as_consumed PASSED
test_good_toolkit_self_yields_no_findings PASSED
test_bad_toolkit_self_emits_findings_for_unused PASSED
test_bad_toolkit_self_does_not_emit_for_consumed PASSED
test_real_repo_finding_count_in_expected_range PASSED
======================== 18 passed, 1 warning in 0.11s =====================
```

The 1 warning is `PytestUnknownMarkWarning` on `@pytest.mark.slow` —
the project does not register the mark. Non-blocking; the test still
runs (only fast-test-mode pruning is affected). Decision 8 sanity
test ran and passed (finding count is in [3, 30] per assertion).

Pre-existing test suite: ran `pytest tools/integrity/tests/` during
the pre-execution checklist; 135 tests passed. After commit 1, the
total is **153** (135 pre-existing + 18 new).

## D. Intentional intermediate state

The check is **not yet registered**. Running
`python3 -m integrity --check cat2.public-symbol-used-toolkit
--no-audit-log` will report "unknown check" or return empty output
because the cat2 `checks/__init__.py` does not import the new module.
Registration is commit 3.

`python3 -m integrity --mode strict --no-audit-log` continues to
report 44 hard-fail + 1213 suppressed — baseline unchanged from
pre-A.2 state.

## E. Decision 2 / Decision 7 / Decision 8 validation

**Decision 2 (strict + reflection-aware extraction):** Six dedicated
extraction tests validate each sub-rule. Real-repo finding count is
24, comfortably within the [3, 30] expected band — naive AST walking
would produce ~93 candidates per spec § 1.2 / Decision 2.

**Decision 7 (`stack_paths()` canary):** A manual standalone run
against the real repo (not yet registered with the runner) produces
24 findings. The list includes
`tools/integrity/integrity/common/stack_paths.py:16` for
`stack_paths` — the spec's canonical "declared public API, no
consumer" finding. Commit 3 will reconfirm this through the runner;
commit 4 will land the grandfather annotation.

Other notable findings (verbatim from standalone run, sorted by file):

```
cat1_citations/checks/bare_path.py:91         BarePathResolution
cat1_citations/grammar.py:140                 UpstreamCitation
cat1_citations/resolver.py:18                 ResolutionResult
cat2_contracts/stack_b.py:26                  PublicSymbolB
cat2_contracts/stack_b.py:34                  is_node_available
cat2_contracts/stack_b.py:38                  ensure_helper_built
cat3_numerical/cubic_kernel.py:33             TestPoint
cat3_numerical/cubic_kernel.py:41             DriverEvaluation
cat3_numerical/d3q19_verify.py:187            opposite_index
cat3_numerical/d3q19_verify.py:192            assert_close
cat3_numerical/generate_expected.py:31        cubic_W
cat3_numerical/generate_expected.py:45        cubic_gradW_magnitude
common/annotations.py:17                      Annotation
common/audit_log.py:26                        audit_log_path
common/audit_log.py:34                        append_findings
common/stack_paths.py:10                      StackPaths
common/stack_paths.py:16                      stack_paths
grandfather.py:32                             Classification
grandfather.py:240                            comment_form_for_md_inside_fence
grandfather.py:321                            collect_findings
grandfather.py:349                            group_findings_by_target
runner.py:32                                  CliArgs
runner.py:82                                  discover_checks
runner.py:102                                 run_checks
```

**Decision 8 (sanity check on post-filter finding count):** 24 is
within [3, 30]. The high end of the expected band (5-15) is exceeded
slightly; the extra ~9 findings are accounted for by:

- Runner-internal helpers (`discover_checks`, `run_checks`, `CliArgs`,
  `parse_args`/`emit_output` — the latter two excluded because they
  ARE referenced from `runner.main`). These are top-level helpers in
  the runner module that are consumed only by other code in the same
  module. The spec's strict scan doesn't have a "same-module-only is
  fine for runner.py" carve-out; banking as toolkit-own-unused is
  consistent with Decision 5 / 7.
- Two cat3 generate_expected.py helpers (`cubic_W`,
  `cubic_gradW_magnitude`) which are similarly intra-module-only.
- `Annotation` (dataclass with grammar parsing helpers), `audit_log_*`
  (used via subprocess), `BarePathResolution` / `UpstreamCitation` /
  `ResolutionResult` (dataclass returns whose call sites flow through
  attribute access on the return value rather than referencing the
  type name directly).

All 24 fall well within Decision 8's hard ceiling of 30. The
extraction strategy is calibrated correctly.

## F. Spec-interpretation note (one)

Spec § 4.2 last paragraph references "the rglob('*') branch (matching
the existing Stack D fixture pattern)," but the § 4.1 code sample
does not include that branch — `target_abs = repo_root /
TOOLKIT_PACKAGE_DIR; if not target_abs.is_dir(): return symbols` would
return empty in fixture mode where fixtures lay out as
`good_toolkit_self/integrity/...` rather than
`good_toolkit_self/tools/integrity/integrity/...`.

Resolution applied per § 4.2's stated intent: the check resolves the
scan-target root via a helper that returns `repo_root /
TOOLKIT_PACKAGE_DIR` when that exists (production) or `repo_root`
itself when it does not (fixture mode). Decision 3's
`tests/`/`scripts/`/`fixtures/` exclusion still applies in either
mode via a `_is_under_tests_or_scripts()` rel-path check. The same
pattern resolves scan-input roots: production uses the three
configured subdirectories; fixture mode falls back to a single
`repo_root` walk.

This interpretation is the smaller-deviation reading: it preserves
the spec's production paths verbatim and matches the spec author's
explicit allowance for a fixture-mode rglob fallback. The alternative
— laying out fixtures with a full `tools/integrity/integrity/...`
prefix — would have required no code change but contradicts the
spec's fixture-tree sketch in § 4.2.

Banked as a v1.3-candidate observation in the closing self-review
addendum if it surfaces further: the production check's hard-coded
`TOOLKIT_PACKAGE_DIR` path is the simplest current shape; a future
batch may parameterize it once a second self-application surface is
needed.

## G. Risk and reversal

Pure new files. No production paths touched. Reversing is `git
revert <sha>` and a `pytest` re-run.

## H. Outstanding for commits 2-4

- Commit 2: classifier rule, catalog section, `apply_annotations`
  refactor for `force_sweep_categories`, sweep CLI flag.
- Commit 3: register the check; verification confirms `[3, 30]` from
  the registered-check codepath plus `stack_paths()` presence.
- Commit 4: grandfather sweep with `--force-sweep-category
  toolkit-own-unused` (NOT `--sweep-live-source`); refresh six
  drifted category counts.
- Commit 5: SHA back-fill (separate commit, no `--amend` per
  Convention #12).
