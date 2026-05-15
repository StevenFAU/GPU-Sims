---
title: "Integrity v1.2 A.3 — Commit 1 landing audit"
date: 2026-05-15
author: claude-code (executor)
status: complete
landed-as-sha: 6fc5884
sibling-docs:
  - /home/otacon/Downloads/integrity_v1_2_a3_spec.md
  - docs/diagnostics/_audits/integrity_v1_2_a3_probe_2026-05-15_architect1.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
companion-shas:
  - "v1.2 A.3 commit 2 (classifier rules): 77628b6"
  - "v1.2 A.3 commit 3 (registration + skip-guard): 880a400"
  - "v1.2 A.3 commit 4 (sweep companion): 908f619"
---

# Integrity v1.2 A.3 — Commit 1 landing audit

## A. Change summary

Commit 1 lands the `cat1.bare-path` check module, fixture trees, and
test file. The check is **not yet registered** in
`cat1_citations/checks/__init__.py` — that lands in commit 3 along with
the `cat1.intra-repo` skip-guard and test migration.

This commit is race-immune: every file is new.

## B. File inventory

New files:

- `tools/integrity/integrity/cat1_citations/checks/bare_path.py` —
  the cat1.bare-path check module. ~340 LOC. Owns `CHECK_ID`, `MODE`,
  `BarePathClass`, `BarePathResolution`, `_build_basename_indices`,
  `_classify_bare_path`, `_format_message`, `_passes_sanity_check`,
  `_list_scannable_files`, `_resolve_upstream_for_path`, `run`.
- `tools/integrity/tests/fixtures/good_bare_path/` — fixture tree
  with no bare-path citations (control).
- `tools/integrity/tests/fixtures/bad_bare_path_upstream/` — fixture
  with REGISTERED-UPSTREAM-BARE citation + mock references/ tree +
  mock `tools/integrity/docs/ground-truth-sources.md` registry.
- `tools/integrity/tests/fixtures/bad_bare_path_intra/` — fixture
  with INTRA-REPO-BARE citation (both valid line and out-of-range line).
- `tools/integrity/tests/fixtures/bad_bare_path_ambiguous/` — fixture
  with AMBIGUOUS citation across 7 candidate files (exercises the
  5-cap truncation marker).
- `tools/integrity/tests/fixtures/bad_bare_path_unresolvable/` —
  fixture with UNRESOLVABLE citation (basename matches nothing).
- `tools/integrity/tests/test_cat1_bare_path.py` — 16 tests covering
  the 4 classification arms, the sanity-check filter, fence-internal
  skip, dotted-path negative case, and a direct unit test of
  `_classify_bare_path`.

Modified files: none.

## C. Verification block

`pytest tools/integrity/tests/test_cat1_bare_path.py -v` → 16 passed.

`pytest tools/integrity/tests/ -q` → full suite green, count grew from
103 → 119 (16 new tests).

`python3 -m integrity --check cat1.bare-path --output human --no-audit-log` →
"0 pass, 0 soft-warn, 0 hard-fail, 0 suppressed" — the check is not
yet known to the runner (registration is commit 3). This is the
expected intermediate state.

`python3 -m integrity --mode strict --no-audit-log` exits 1 with the
pre-existing baseline (5 unsuppressed HARD_FAILs, 1046 suppressed —
1 finding more than the spec's "4 hard-fail" baseline, attributable
to drift between spec-draft time and execution time; does not
conflict with any cited file). Behavior unchanged by this commit.

## D. Behavioral notes

The module exists but is dormant. The runner has no entry for
`cat1.bare-path`, so calling `--check cat1.bare-path` returns an
empty run. This is by design: commit 3 registers the check and
adds the cat1.intra-repo skip-guard in a single coupled change, so
the gate transition is atomic.

Decisions encoded in the module:

- Decision 1: detection reuses `extract_intra_repo_citations`,
  filtered to `"/" not in citation.path` (no new extractor).
- Decision 5: AMBIGUOUS list capped at 5 candidates with
  `, ... (N more)` truncation marker.
- Decision 6: sanity-check filter rejects empty basenames, basenames
  starting with a digit or non-letter (other than underscore),
  basenames containing newline/CR, and line<1.
- Decision 7: basename index built from `list_tracked_files` for
  production; rglob fallback for fixtures (no `.git/`).

The `_resolve_upstream_for_path` helper converts a vendored absolute
path back to (upstream_name, anchor_version, vendor_relative) for the
REGISTERED-UPSTREAM rewrite suggestion. Verified against the fixture
TOML registry.

## E. Incidental findings during execution

1. The repo's working tree carries uncommitted modifications to
   `tools/integrity/integrity/grandfather.py`,
   `tools/integrity/scripts/grandfather_sweep.py`, and
   `tools/integrity/tests/test_grandfather_sweep.py` from the
   parallel-session P1.8 (grandfather-sweep live-source protection)
   work. Commit 1 stages only the new bare-path files; the P1.8
   changes remain in the working tree for the parallel session to
   commit. Coordination spec § 8.1.
2. Strict-mode baseline at commit time was 5 HARD_FAILs (spec
   predicted 4). The +1 drift is plausibly drift between the
   `9add149` spec-drafting baseline and execution time and does not
   conflict with any file this spec edits. Noted for the post-A.3
   retro.
3. Pytest baseline at commit time was 103 tests (spec predicted 96).
   Confirms the parallel session has already landed earlier bolt-ons
   (e.g., P1.5 d3q19, P1.7 stub_label_stale docstring) before this
   batch began. The +7 drift is additive; commits 1–4 do not depend
   on the parallel work.
