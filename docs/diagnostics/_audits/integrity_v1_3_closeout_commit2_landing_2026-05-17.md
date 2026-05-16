---
title: "Integrity v1.3 Closeout Commit 2 — T2.3 Stack C single-parse refactor"
date: 2026-05-17
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_closeout_commit1_landing_2026-05-17.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
---

# Integrity v1.3 Closeout Commit 2 — T2.3 Stack C single-parse refactor

## § A. Change summary

Refactors `tools/integrity/integrity/cat2_contracts/stack_c.py` to parse
each translation unit at most once for the
`cat2.public-symbol-used-c` check. The legacy flow ran
`extract_public_surface(repo_root)` and `find_references(repo_root,
symbols, consumer_sources)` as two independent calls, each
re-instantiating `clang.cindex.Index.create()` and reparsing every
source file that appears in both the representative-TU set and the
consumer-source set (the bulk of `common/common-cpp/src/*.cpp`).

The refactor adds two helpers:
`_parse_translation_units(repo_root, sources)` that returns
`(source, TranslationUnit)` pairs for a given source list, and
`extract_and_find_references(repo_root)` that parses the union of
representative TUs and consumer sources once and dispatches the
extraction and reference-finding passes against the cached parsed
TUs. `public_symbol_used_c.run()` now consumes this single-parse
entry point.

Pure refactor by intent: extraction still operates only on
representative TUs, references still operate only on consumer sources,
USR dedup and cross-TU dedup logic unchanged. The legacy
`extract_public_surface` and `find_references` functions stay exported
because `test_cat2_stack_c.py` calls them directly.

A skip-by-default `INTEGRITY_PERF_ASSERTIONS=1`-gated perf assertion is
added: real-repo Stack C scan must complete under a 120s ceiling
(conservative; target post-refactor walltime is ~50s vs ~95s
pre-refactor baseline noted in roadmap T2.3 / v1 retro § 4). CI
walltime is the load-bearing measurement; the local assertion guards
against pathological regressions only.

## § B. File inventory

| Status | Path | Diff |
|---|---|---|
| Modified | `tools/integrity/integrity/cat2_contracts/stack_c.py` | +113 LOC (two new helpers + integrity-allow annotation on the new public symbol) |
| Modified | `tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_c.py` | +2/-7 LOC (consume single-parse entry point) |
| Modified | `tools/integrity/tests/test_cat2_stack_c.py` | +35 LOC (perf-assertion test + section banner + import) |
| Modified | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit1_landing_2026-05-17.md` | +N inline annotations from sweep companion (Convention J carry-over) |
| Modified | `docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md` | +N inline annotations from sweep companion |
| Modified | `docs/diagnostics/_audits/integrity_v1_3_closeout_probe_2026-05-17_architect1-via-claude-code.md` | +N inline annotations from sweep companion |
| Created | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit2_landing_2026-05-17.md` | this report |

## § C. Verification

Pre-edit anchoring (HEAD `<COMMIT_1_SHA>`, after closeout commit 1 landed):

```
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 83 hard-fail, 1289 suppressed
```

Stack C entry-point call sites confirmed against probe § B.3 (two
`Index.create()` instantiations at `stack_c.py:150` in
`extract_public_surface` and `stack_c.py:353` in `find_references`,
unchanged at landing time):

```
$ grep -n "Index.create()" tools/integrity/integrity/cat2_contracts/stack_c.py
150:    index = clang.cindex.Index.create()
353:    index = clang.cindex.Index.create()
```

Stack C tests:

```
$ python3 -m pytest tools/integrity/tests/test_cat2_stack_c.py -v
test_extract_public_surface_finds_class_and_function PASSED
test_extract_public_surface_enumerates_fields PASSED
test_good_contracts_c_yield_no_findings PASSED
test_bad_contracts_c_flag_unused_radii PASSED
test_bad_contracts_c_flag_unused_function PASSED
test_used_symbols_not_flagged PASSED
test_missing_compile_commands_returns_empty PASSED
test_stack_c_single_parse_walltime_under_ceiling SKIPPED
========================= 7 passed, 1 skipped in 0.10s =========================
```

7 prior tests still pass (refactor preserves behavior); 1 new
skip-by-default perf test added.

Full suite:

```
$ python3 -m pytest tools/integrity/tests/ -q
189 passed, 1 warning in ~130s
```

189 passed (183 baseline + 6 from commit 1; the perf test is skipped
and does not contribute to the count).

Stack C single-check smoke:

```
$ python3 -m integrity --check cat2.public-symbol-used-c --no-audit-log
integrity: 0 pass, 0 soft-warn, 0 hard-fail, 110 suppressed
```

110 findings, all suppressed by the grandfather catalog —
matches pre-refactor behavior. The refactor is finding-equivalent.

Post-refactor + sweep companion gate:

```
$ python3 tools/integrity/scripts/grandfather_sweep.py
grandfather-sweep: modified 4 files; 38 annotations added
  skipped as live-source (other-cat1): 39 (use --sweep-live-source to include)
                      audit-bare-path: 34
         audit-report-grammar-example: 4
                       audit-citation: 1
                   toolkit-own-unused: 1
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 44 hard-fail, 1329 suppressed
```

The 38-annotation sweep companion absorbs:
- one `toolkit-own-unused` finding for the new public
  `extract_and_find_references` symbol on `stack_c.py:619` (it has a
  current consumer in `public_symbol_used_c.py`; the check doesn't
  follow imports across modules, so an annotation is the right
  treatment per the same pattern used for other Stack C entry
  points);
- the bulk (34 audit-bare-path + 4 audit-report-grammar-example +
  1 audit-citation) is Convention J carry-over from commit 1's
  audit-doc files (commit1_landing, spec, probe). Commit 1's own
  sweep companion only covered the cat1-scannable surface at its
  run time; the new probe/spec/audit citations and the
  newly-created closeout audit-doc set carried into this commit's
  pre-commit gate. Convention J explicitly accommodates this
  multi-file-commit carry-over pattern.

Gate at 44 hard-fail matches commit 1's post-sweep state. CI walltime
for the perf-assertion test will be captured in commit 8's audit or in
a follow-up addendum.

## § D. Behavioral notes

**Single-parse strategy.** `extract_and_find_references` builds the
union of `_representative_tus(repo_root)` and
`discover_consumer_sources(repo_root)` (resolved to canonical paths
for dedup), then parses each unique source exactly once. The two
phases iterate the parsed-TU list and skip TUs that don't belong to
their respective scope: extraction skips TUs not in the resolved
representative-TU set; reference-finding skips TUs not in the
resolved consumer-source set. Net effect: files in both sets (the
common `common/common-cpp/src/*.cpp` overlap) get parsed once instead
of twice; files in only one set are parsed once as before. Expected
savings: roughly proportional to the size of the overlap (the bulk of
the work).

**USR / dedup semantics preserved.** Cross-TU symbol dedup uses the
same `(file, line, name, kind)` key as `extract_public_surface`. USR
seen-set discipline within a single walk is unchanged. Reference
dedup remains the same: `_collect_refs` appends without dedup (the
caller iterates each consumer source exactly once, preserving the
non-dedup-but-also-non-double-count invariant from the legacy path);
`_collect_field_token_refs` keeps its `(file, line)` dedup.

**Legacy entry points retained.** `extract_public_surface` and
`find_references` remain exported with unchanged signatures because
`test_cat2_stack_c.py` calls `extract_public_surface` directly to
unit-test symbol extraction independent of reference finding. Keeping
both paths means the test surface doesn't churn. The gate path
exclusively uses the single-parse entry point.

**New public symbol annotation.** The new
`extract_and_find_references` function is a public toolkit symbol
that `cat2.public-symbol-used-toolkit` flags as having no current
consumer (the check operates module-locally and doesn't follow
cross-module imports). Sweep companion absorbs the finding with the
standard `toolkit-own-unused` category annotation, mirroring the
treatment for other internal Stack C helpers.

**Perf assertion is opt-in.** The walltime test is gated on
`INTEGRITY_PERF_ASSERTIONS=1` and additionally skips if
`build/compile_commands.json` is absent (which is the same fallback
the runtime path uses). Default CI does not run it; the load-bearing
walltime measurement is the GitHub Actions workflow step duration.
The assertion's ceiling is 120s (>2x the post-refactor target of
~50s) to avoid flake on slow runners while still catching pathological
regressions.

## § E. Banked observations

**CI walltime unverified at landing.** Per probe § F.1, no successful
recent CI run on `main` exists in the last 20-run window, so no
pre-refactor wall-clock baseline is directly observable. The roadmap
T2.3 / v1 retro § 4 estimate of ~95s pre and ~50s post is the
reference. Post-landing CI on this commit will be the first
post-refactor measurement; commit 8's audit (or a follow-up addendum
on this report) should capture the observed walltime.

**Sweep companion modifies committed-to-disk audit-doc files.**
Commit 1's audit report received new inline annotations in this
commit's sweep companion. This is Convention J carry-over (commit 1's
sweep ran against the cat1-scannable surface at its run time;
new citations in the audit reports created during that commit weren't
all picked up by the same-commit sweep). The annotations are correct
suppressions of pre-existing audit-doc citations; they are not
behavioral changes to commit 1.

**No live-source sweep.** Per Hard Rule 10, this commit does not run
`--sweep-live-source` or `--force-sweep-category`. The 39 live-source
`other-cat1` findings remain P1.8-protected.

## § F. Cross-references

- Spec § 3 (`docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md`)
- Probe § B.3, § B.4, § F.1
- Roadmap T2.3 (`docs/retro/integrity-toolkit-v1.3-candidates.md`)
- v1 retro § 4 (`docs/retro/integrity-toolkit-v1.md`)
- v1.1 batch-1 retro § 6.1 item 8 (`docs/retro/integrity-toolkit-v1.1-batch1.md`)
- Convention #12 (SHA back-fill) — commit 8 of this batch resolves the
  `<COMMIT_1_SHA>` placeholder above
