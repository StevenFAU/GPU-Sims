---
title: "Integrity v1.3 Commit 1 — T1.3 Catalog Auto-Refresh Script"
date: 2026-05-16
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_t1_3_5_probe_2026-05-16_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_t1_3_5_spec_2026-05-16_architect1.md
  - docs/retro/integrity-toolkit-v1.3-candidates.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
---

# Integrity v1.3 Commit 1 — T1.3 Catalog Auto-Refresh Script

## § A. Change summary

T1.3 lands the auto-refresh script per roadmap § 4 T1.3 and probe § B.7's
six design decisions. A new script at
`tools/integrity/scripts/refresh_catalog_counts.py` reads the output of
`python3 -m integrity --grandfather-report --no-history-append` and
rewrites each numeric `(N)` parenthetical in
`tools/integrity/docs/grandfather-catalog.md`'s H3 category headings.
Non-numeric parentheticals (`(?)`, free-prose `(0 swept; 44 live-source
skipped)`) are preserved verbatim. The script is idempotent (probe
§ B.7 (4)).

Driven by v1.1 batch-1 retro § 5.5's quantification of catalog drift at
+6.7% per batch cycle and probe § B.6's empirical drift table
(audit-citation: 597 → 99, audit-bare-path: 635 → 729, etc.). The manual
refresh workflow is replaced by a tool-assisted workflow; the catalog's
`## Updating counts` block is rewritten in this commit to point at the
new script as the canonical entry point.

## § B. File inventory

| Status | Path | Diff |
|---|---|---|
| Created | `tools/integrity/scripts/refresh_catalog_counts.py` | +228 LOC |
| Created | `tools/integrity/tests/test_refresh_catalog_counts.py` | +207 LOC |
| Created | `docs/diagnostics/_audits/integrity_v1_3_t1_3_5_spec_2026-05-16_architect1.md` | +1990 LOC (sibling-doc; lands here per Stage 0) |
| Modified | `tools/integrity/docs/grandfather-catalog.md` | `## Updating counts` block: -12 / +27 LOC (prose-only rewrite) |
| Created | `docs/diagnostics/_audits/integrity_v1_3_commit1_landing_2026-05-16.md` | this report |

No other paths touched.

## § C. Verification

```
$ cd tools/integrity && python3 -m pytest tests/test_refresh_catalog_counts.py -v
============================= test session starts ==============================
collected 17 items

tests/test_refresh_catalog_counts.py::test_parse_canonical_heading PASSED
tests/test_refresh_catalog_counts.py::test_parse_placeholder_heading PASSED
tests/test_refresh_catalog_counts.py::test_parse_prose_heading PASSED
tests/test_refresh_catalog_counts.py::test_parse_multiple_headings_in_document_order PASSED
tests/test_refresh_catalog_counts.py::test_parse_ignores_h2_and_other_levels PASSED
tests/test_refresh_catalog_counts.py::test_report_line_regex_canonical PASSED
tests/test_refresh_catalog_counts.py::test_report_line_regex_rejects_summary_dict PASSED
tests/test_refresh_catalog_counts.py::test_report_line_regex_rejects_header PASSED
tests/test_refresh_catalog_counts.py::test_refresh_updates_numeric_count PASSED
tests/test_refresh_catalog_counts.py::test_refresh_preserves_placeholder_verbatim PASSED
tests/test_refresh_catalog_counts.py::test_refresh_preserves_prose_verbatim PASSED
tests/test_refresh_catalog_counts.py::test_refresh_errors_on_missing_heading PASSED
tests/test_refresh_catalog_counts.py::test_refresh_idempotent_when_already_correct PASSED
tests/test_refresh_catalog_counts.py::test_refresh_idempotent_after_first_refresh PASSED
tests/test_refresh_catalog_counts.py::test_refresh_zero_count_in_catalog_not_in_report_preserved PASSED
tests/test_refresh_catalog_counts.py::test_refresh_handles_mixed_numeric_and_nonnumeric PASSED
tests/test_refresh_catalog_counts.py::test_script_imports_cleanly PASSED

============================== 17 passed in 0.02s ==============================
```

Full suite: 170 passed, 0 failed (135.5s). Pre-commit was 153 tests
(per the +17 delta of this commit against the prior baseline).

Script dry-run against current catalog:

```
$ python3 tools/integrity/scripts/refresh_catalog_counts.py --dry-run
refresh_catalog_counts: no changes needed (17 categories checked)
```

The catalog is already in sync with the report at this commit (A.2
commit 4 / commit 5 landed catalog updates as part of A.2's grandfather
sweep). The 17 categories matched corresponds to the 17 numeric-headed
categories the parser identified across the catalog's 18 total H3
headings (one heading — `toolkit-own-unused` — was a `(?)` placeholder
pre-A.2 but is now numeric `(24)`).

Idempotency confirmation:

```
$ python3 tools/integrity/scripts/refresh_catalog_counts.py
refresh_catalog_counts: no changes needed (17 categories checked)
$ python3 tools/integrity/scripts/refresh_catalog_counts.py
refresh_catalog_counts: no changes needed (17 categories checked)
```

Gate state (pre-commit and post-commit-pre-sweep):

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 5 pass, 0 soft-warn, 45 hard-fail, 1247 suppressed
```

Hard-fail count matches the pre-batch baseline. This commit's own
audit-doc bare-paths in this report and the sibling-doc spec are
swept by the inline grandfather-sweep companion before the next commit.

## § D. Design decisions applied (per probe § B.7)

| Probe § B.7 # | Decision | Implementation |
|---|---|---|
| 1 | Heading-with-no-report preserved | `build_refreshed_text` skips when `report_count is None`. Pinned by `test_refresh_zero_count_in_catalog_not_in_report_preserved`. |
| 2 | Report-with-no-heading errors out | `build_refreshed_text` collects errors before any rewriting; returns input unchanged. Pinned by `test_refresh_errors_on_missing_heading`. |
| 3 | Two-number-prose preserved verbatim | `is_numeric` gate via `NUMERIC_COUNT_RE`. Pinned by `test_refresh_preserves_prose_verbatim`. |
| 4 | Idempotency | Already-correct headings skipped; second invocation produces no diff. Pinned by `test_refresh_idempotent_when_already_correct` and `test_refresh_idempotent_after_first_refresh`. |
| 5 | In-place edit with `--dry-run` | `main()` writes via `Path.write_text` in the default branch, prints unified diff when `--dry-run` is set. |
| 6 | `## Updating counts` block updated in same commit | Done — see § B inventory. |

Subprocess decision (probe § B.3) applied at `fetch_report_counts`:
invokes `python3 -m integrity --grandfather-report --no-history-append`
rather than importing `emit_grandfather_report` from `integrity.snapshot`.

## § E. Banked observations

1. **Test count: spec § 1.2 / § 2 / § 7 quotes "+12 tests" but the spec
   body in § 3.3 defines 17 explicit tests.** This commit lands all 17
   per the body (the body is authoritative; the summary is the
   discrepant claim). The probe's friction-points block in the
   execution prompt did not flag this. Recorded here per Convention F
   (audit-prose freshness) rather than silently editing § 3.3.

2. **Catalog state at execution time: zero drift surfaced.** The dry-run
   reports "no changes needed" because A.2 commit 4 (`9c8979a`) ran
   the grandfather sweep with `--force-sweep-category` flags that
   updated counts to current state. The empirical drift table from
   probe § B.6 (audit-citation -498, audit-bare-path +94) was already
   reconciled by A.2 between probe time and this commit time; T1.3's
   value going forward is preventing future drift rather than fixing
   probe-time drift.

3. **`toolkit-own-unused` heading transitioned from `(?)` to numeric
   `(24)` post-probe.** Probe § B.1 noted the placeholder form as a
   load-bearing test case for non-numeric preservation. The placeholder
   was populated by A.2 commit 4's force-sweep; the test fixture for
   `test_refresh_preserves_placeholder_verbatim` uses a fresh string
   literal, so test coverage of the placeholder form is preserved
   even though live catalog no longer exercises it.

## § F. Cross-references

- Probe § B.1 — Heading shape (`### \`<cat>\` (<N>)`), 18 categories at probe time.
- Probe § B.2 — Report line format (`{cat:>35s}: {n}` after `per-category counts:` label).
- Probe § B.3 — Subprocess-vs-import decision (subprocess wins on stability).
- Probe § B.6 — Empirical drift table (now historical; A.2 reconciled).
- Probe § B.7 — Six design choices (all six applied per § D above).
- Roadmap § 4 T1.3 — Original spec-time scope.
- v1.1 batch-1 retro § 5.5 — Drift quantification (+6.7%/cycle).

## § G. Next commit

Commit 2 — T1.5 cat3 TOML → JSON convergence. SHA cross-reference will
be filled in by commit 4 (SHA back-fill): `72a2d26`.
