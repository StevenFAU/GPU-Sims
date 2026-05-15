# Integrity Toolkit — Commit 3 Landing — 2026-05-14

Third of eight commits building the cross-stack integrity verification
toolkit per `docs/integrity-toolkit-spec.md` § 11. This commit completes
the Category 1 surface by landing three new checks: `cat1.upstream-citation`,
`cat1.upstream-anchor`, and `cat1.unregistered-upstream`. After this
commit, every citation form specified in § 6.1 has a check.

Companion to:

- Spec: `docs/integrity-toolkit-spec.md` § 6.2 (upstream grammar),
  § 6.3 (resolution rules), Appendix A (ground-truth source registry)
- Prior commit's audit: `integrity_build_2_landing_2026-05-14.md`

---

## A. Change summary

Lands the upstream-citation half of Category 1.

- `cat1_citations/grammar.py` — adds `UPSTREAM_RE`, `UpstreamCitation`,
  and `extract_upstream_citations()` for the
  `<UpstreamName> <version> <path>:<line>[-<end>]` form. The version
  regex requires ≥ 2 dotted components (or `HEAD`, or a hex SHA) so
  that single-integer sentence fragments like "Section 1" don't match.
- `cat1_citations/upstream_anchor.py` (new module) — registry loader.
  Parses the fenced TOML block inside
  `tools/integrity/docs/ground-truth-sources.md` and returns a typed
  mapping. Exposes `vendor_head_sha()` for live SHA queries via
  `git -C <vendor_root> rev-parse HEAD`.
- `cat1_citations/checks/upstream.py` — `cat1.upstream-citation`
  HARD_FAIL. For each citation whose upstream name is in the registry,
  asserts (1) the cited version equals the registry's `anchor_version`
  (or is `HEAD`); (2) `<vendor_root>/<path>` resolves on disk; (3) the
  line range is within the resolved file's line count.
- `cat1_citations/checks/upstream_anchor.py` — `cat1.upstream-anchor`
  HARD_FAIL. For each registry entry that opts in via
  `used_by_checks`, verifies the vendor clone's `HEAD` SHA matches
  `anchor_sha`. This is the one documented opt-out from the
  `references/` exclusion in spec § 3.4.
- `cat1_citations/checks/unregistered_upstream.py` —
  `cat1.unregistered-upstream` HARD_FAIL. Flags every cited
  `<UpstreamName>` that isn't a key in the registry. Dedups identical
  (name, file, line) tuples so a docs sentence quoting the same name
  twice yields one finding.
- `cat1_citations/checks/intra_repo.py` (patched) — adds two skip
  paths so cat1.upstream-citation owns the upstream territory:
  (1) `_is_under_references()` skips paths beginning with
  `references/`; (2) the run loop precomputes the
  `(line, path, start, end)` tuples produced by
  `extract_upstream_citations()` and skips any intra-repo citation
  whose tuple matches — i.e., the path:line is the tail of an
  upstream citation on the same line.
- `cat1_citations/checks/__init__.py` — registers the three new
  checks (registry now contains five Cat 1 checks).
- `tools/integrity/docs/ground-truth-sources.md` (new) — v1 registry
  seeded with `SPlisHSPlasH 2.16.1 @ 6bff55a6eaf14083d34650f22a268ce156b62b54`.
  Documents Chakazul/Lenia (LeniaNDK) as intentionally-unregistered.
- Tests: `test_cat1_upstream.py` (4), `test_cat1_upstream_anchor.py`
  (3), `test_cat1_unregistered.py` (3), plus a new
  references-skip test in `test_cat1_intra_repo.py`. Fixtures
  mirror the registry under
  `tests/fixtures/{good,bad}_citations/tools/integrity/docs/`
  and include a `references/SyntheticUpstream/foo.cpp` vendor tree.

## B. Scope guardrails honored

In scope and shipped: the three checks, the registry doc, the
intra-repo skip patches, the grammar extension, fixtures, tests, the
real-repo smoke run.

Out of scope and not touched:

- No grandfather sweep (commit 4).
- No CI workflow file (commit 4).
- No `cat1.audit-log-recursion` (deferred).
- No `cat1.exclusion-list` (deferred — needs git-history awareness).
- No annotation parser changes (the documentation-only literal
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
  mentions of `integrity-allow:` flagged by `cat1.annotation-form`
  in commit 2 § F.3 will be grandfathered via per-line suppression
  in commit 4).
- `INTRA_REPO_RE` and the existing resolver were not modified; the
  filtering for upstream overlap is implemented at the check site,
  not in the grammar/resolver.

## C. Verification

### Pytest output

```
============================= test session starts ==============================
platform linux -- Python 3.12.3, pytest-8.4.2, pluggy-1.6.0 -- /usr/bin/python3
cachedir: .pytest_cache
rootdir: /home/otacon/Projects/GPU-Sims/GPU-Sims/tools/integrity
configfile: pyproject.toml
plugins: cov-5.0.0, anyio-4.13.0
collecting ... collected 27 items

tests/test_cat1_annotation.py::test_good_annotations_yield_no_findings PASSED [  3%]
tests/test_cat1_annotation.py::test_bad_annotations_yield_three_findings PASSED [  7%]
tests/test_cat1_annotation.py::test_validate_check_id_grammar PASSED     [ 11%]
tests/test_cat1_annotation.py::test_validate_reason_length PASSED        [ 14%]
tests/test_cat1_annotation.py::test_validate_issue_ref PASSED            [ 18%]
tests/test_cat1_intra_repo.py::test_good_citations_yield_no_findings PASSED [ 22%]
tests/test_cat1_intra_repo.py::test_dangling_citation_is_flagged PASSED  [ 25%]
tests/test_cat1_intra_repo.py::test_out_of_range_line_is_flagged PASSED  [ 29%]
tests/test_cat1_intra_repo.py::test_template_token_is_not_a_citation PASSED [ 33%]
tests/test_cat1_intra_repo.py::test_time_of_day_is_not_a_citation PASSED [ 37%]
tests/test_cat1_intra_repo.py::test_ipv4_port_is_not_a_citation PASSED   [ 40%]
tests/test_cat1_intra_repo.py::test_references_paths_are_not_flagged_as_intra_repo PASSED [ 44%]
tests/test_cat1_unregistered.py::test_registered_upstream_yields_no_findings PASSED [ 48%]
tests/test_cat1_unregistered.py::test_unregistered_upstream_is_flagged PASSED [ 51%]
tests/test_cat1_unregistered.py::test_unregistered_check_deduplicates_per_file_line PASSED [ 55%]
tests/test_cat1_upstream.py::test_good_upstream_citations_yield_no_findings PASSED [ 59%]
tests/test_cat1_upstream.py::test_wrong_version_is_flagged PASSED        [ 62%]
tests/test_cat1_upstream.py::test_dangling_upstream_path_is_flagged PASSED [ 66%]
tests/test_cat1_upstream.py::test_unregistered_upstream_is_not_flagged_by_upstream_check PASSED [ 70%]
tests/test_cat1_upstream_anchor.py::test_anchor_mismatch_is_flagged PASSED [ 74%]
tests/test_cat1_upstream_anchor.py::test_missing_vendor_tree_is_flagged PASSED [ 77%]
tests/test_cat1_upstream_anchor.py::test_empty_registry_yields_no_findings PASSED [ 81%]
tests/test_runner.py::test_runner_parses_args_cleanly PASSED             [ 85%]
tests/test_runner.py::test_runner_rejects_bad_cli PASSED                 [ 88%]
tests/test_runner.py::test_runner_runs_against_fixtures_clean PASSED     [ 92%]
tests/test_runner.py::test_runner_runs_against_bad_fixtures_fails PASSED [ 96%]
tests/test_runner.py::test_runner_warn_only_mode_downgrades PASSED       [100%]

============================== 27 passed in 0.06s ==============================
```

All 27 tests pass (16 from commit 2 + 11 new).

### Strict-mode real-repo smoke run

`python3 -m integrity --mode strict --no-audit-log` exits 1 (HARD_FAIL).
`python3 -m integrity --mode warn-only --no-audit-log` exits 0.

### Per-check finding counts (grandfather budget for commit 4)

```
cat1.intra-repo:              785
cat1.annotation-form:          34
cat1.upstream-citation:        29
cat1.upstream-anchor:           0
cat1.unregistered-upstream:     0
                               ---
Total:                        848
```

Compared to commit 2 (`810 + 22 = 832`), the totals shifted as follows:

- `cat1.intra-repo` dropped from 810 → 785 (−25). The drop is much
  smaller than commit 2's audit predicted ("sharply"). See § F.1 for
  the cause: the bulk of the 810 were not `references/`-prefixed
  vendor-tree citations but rather bare-basename common-cpp citations
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  in the architect audit reports (e.g. `alembic_writer.hpp:31`).
- `cat1.annotation-form` rose from 22 → 34 (+12). Commit 2's audit
  used a count derived from a snapshot before the smoke-only audit
  reports landed; the current count reflects all merged docs.
- `cat1.upstream-citation` added 29 net-new findings — 28 wrong-version
  (1.8.10 vs registered 2.16.1) + 1 dangling path. See § C.2.
- `cat1.upstream-anchor` reports 0 because `references/SPlisHSPlasH/.git/HEAD`
  matches the registered `6bff55a6...` SHA exactly.
- `cat1.unregistered-upstream` reports 0 because the Chakazul/Lenia
  citations in the codebase do not use the upstream-form grammar
  (see § F.2).

### C.1. Top files per check

```
-- cat1.intra-repo --
   210 docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md
   162 docs/diagnostics/_audits/phase11_5_probe_2026-05-14_architect1.md
    89 docs/diagnostics/_audits/sims_lenia_probe1_2026-05-14_architect3b.md
    82 docs/diagnostics/_audits/phase11_5_probe2_2026-05-14_architect1.md
    51 docs/diagnostics/_audits/commoncpp_unexercised_2026-05-14_architect2.md
-- cat1.annotation-form --
    17 docs/integrity-toolkit-spec.md
     4 docs/diagnostics/_audits/integrity_build_2_landing_2026-05-14.md
     4 tools/integrity/integrity/cat1_citations/checks/annotation.py
     3 tools/integrity/tests/fixtures/bad_citations/bad_annotation.py
     2 tools/integrity/integrity/common/annotations.py
-- cat1.upstream-citation --
    10 docs/diagnostics/_audits/phase11_5_probe2_2026-05-14_architect1.md
     9 docs/diagnostics/_audits/integrity_toolkit_probe_2026-05-14_architect1.md
     3 particle-fluids/sph-water/shaders/density_alpha.comp.glsl
     1 docs/integrity-toolkit-spec.md
     1 particle-fluids/sph-water/shaders/apply_velocity.comp.glsl
```

### C.2. cat1.upstream-citation breakdown

| Failure class    | Count | Notes                                                                                                |
|------------------|-------|------------------------------------------------------------------------------------------------------|
| wrong-version    | 28    | `SPlisHSPlasH 1.8.10 ...` — the pre-v1 anchor still cited across audit docs and live shader headers. |
| dangling-path    |  1    | A citation whose path doesn't resolve under `references/SPlisHSPlasH/`. Confirmed not a parser bug.  |

All 28 wrong-version findings are correctly flagged: the codebase
historically pinned `1.8.10` (which was non-existent and fabricated;
that's the Phase 11.5 setup-1 incident the registry doc references).
The current vendor tree is `2.16.1`. Either the live citations need
rewriting to `2.16.1` or the audit docs need grandfathering — both
are commit 4's concern.

### C.3. cat1.unregistered-upstream breakdown

Zero findings. The expectation in the build prompt was that
Chakazul/Lenia citations from
`continuous-ca/lenia-fft/python/lenia_fft/presets.py` would surface
here. They don't, because the live citations look like
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`LeniaNDK.py:329-335` (bare path) rather than
`Chakazul/Lenia LeniaNDK ...` (upstream-form). See § F.2.

## D. Spec compliance

Spec § 6.2 (upstream grammar): implemented per the form
`<UpstreamName> <version> <path>:<line>[-<end>]`. Version admits
`v?\d+(?:\.\d+){1,3}(?:-\w+)?`, `HEAD`, and 7–40 char hex SHAs.
False-negative on single-integer versions is intentional and
documented in `grammar.py`.

Spec § 6.3 (resolution rules, upstream half): implemented
verbatim — registry lookup → version match → path-under-vendor-root
→ line-range check.

Spec Appendix A (registry format): TOML block inside a markdown
fence, parsed via `tomllib`. Fields land as `UpstreamRegistration`.
The `used_by_checks` field is honored by `cat1.upstream-anchor` so
registry entries can opt out of anchor verification (used by the
synthetic test fixture, which has no `.git`).

Spec § 3.4 (exclusions): the `references/` tree remains excluded
from the default scan, but `cat1.upstream-anchor` consults
`references/<Upstream>/.git/HEAD` directly — the one documented
opt-out.

## E. Test plan

- Synthetic fixtures (good_citations, bad_citations) cover the four
  expected outcomes: clean, wrong-version, dangling-path,
  unregistered-upstream.
- The upstream-anchor tests use `tmp_path` and a runtime-initialized
  git repo (`subprocess.run(['git', 'init', '-q'], ...)`); the test
  environment had git on PATH and the tests passed cleanly.
- The references-skip test in `test_cat1_intra_repo.py` verifies
  the `_is_under_references()` filter; the upstream-fixture
  citation in `good_citations/upstream_user.md` indirectly
  exercises the second skip (upstream-tail overlap) since
<!-- integrity-allow: cat1.unregistered-upstream; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
  `SyntheticUpstream 1.0.0 foo.cpp:1` would otherwise yield a
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  failing `cat1.intra-repo` finding for the bare `foo.cpp:1`.

## F. Incidentals — surprises and deferred work

### F.1. Commit 2's "sharply" prediction was off

Commit 2's audit predicted `cat1.intra-repo` would "drop sharply"
once `cat1.upstream-citation` claimed the vendor-tree paths. The
actual drop is 810 → 785 (−25). The 785 residual is dominated by
audit-report citations that use the bare common-cpp basename
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
(`alembic_writer.hpp:31`, `camera.hpp:33`, etc.) — not vendor-tree
paths. The misprediction came from sampling top-of-list citations
without checking the long tail. The commit 4 grandfather sweep
needs to budget for ~785 intra-repo suppressions, not ~50.

### F.2. Chakazul/Lenia citations don't use upstream-form grammar

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
The Lenia codebase cites the upstream as `LeniaNDK.py:329-335`
(bare path) rather than `LeniaNDK 1.0.0 file:line` or
`Chakazul/Lenia LeniaNDK ...`. The grammar requires
`<UpstreamName> <version> <path>:<line>` so these don't trigger
`cat1.unregistered-upstream`. They surface in `cat1.intra-repo`
instead (the path doesn't resolve in this repo). This is a
v1-grammar limitation, not a bug: the registry's deferred note
about LeniaNDK remains accurate, but its detection vector under
v1 is intra-repo, not unregistered-upstream. A v2 grammar
extension (or a per-upstream alias list) could add bare-path
detection for known-upstream basenames.

### F.3. Grammar false-positive scan (live findings audit)

Spot-checked the 29 `cat1.upstream-citation` findings for the
expected false-positive class (sentence-starting capitalized words
followed by a number). None of the 29 matched that pattern — all
are real `SPlisHSPlasH <version> <path>:<line>` citations. The
tight-spacing requirement in `UPSTREAM_RE` did its job.

### F.4. `cat1.annotation-form` count grew

22 (commit 2 audit) → 34 (commit 3 smoke). The increase is from
audit docs and the toolkit's own check sources (which contain
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
literal `integrity-allow:` strings as grammar examples). Commit 4
will grandfather these via per-line suppression annotations.

### F.5. Subprocess-based test for anchor verification

`test_anchor_mismatch_is_flagged` spawns `git init` / `git commit`
in a temp directory. This passed locally with no special setup; CI
environments must have `git` installed in PATH (a safe assumption
for any commit running on a developer machine or CI runner that
already clones this repo). The runtime cost is ~50 ms per test.
