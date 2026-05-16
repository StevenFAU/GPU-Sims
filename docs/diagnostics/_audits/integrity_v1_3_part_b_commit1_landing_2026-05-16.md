---
title: "Integrity v1.3 Part-B Commit 1 — T1.2 FALLTHROUGH_CATEGORIES + helper"
date: 2026-05-16
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_part_b_spec_2026-05-16_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_t1_1_2_probe_2026-05-16_architect1.md
  - docs/retro/integrity-toolkit-v1.2-bolt-ons.md
  - docs/retro/integrity-toolkit-v1.3-candidates.md
---

# Integrity v1.3 Part-B Commit 1 — T1.2 FALLTHROUGH_CATEGORIES + helper

## § A. Change summary

T1.2 lands the Convention H structural follow-through per roadmap § 4 T1.2
and probe § K.1 decision (d). Module-level `FALLTHROUGH_CATEGORIES:
frozenset[str]` and `is_fallthrough_category()` helper are added to
`tools/integrity/integrity/grandfather.py` near the existing
`SWEEPABLE_PATH_PREFIXES` constants. The literal-string tuple match in
`apply_annotations`'s LIVE-SOURCE filter is refactored to use the helper.

Convention H (v1.2 bolt-ons retro § 4.2 — fallthrough discriminator) gets
a code anchor: the constant + helper docstring name the convention so the
retro text and code can be navigated from either direction. Forward-
compatible for future fallthrough buckets (e.g., a hypothetical
`other-cat3`): extension is a single frozenset entry plus a pinning-test
update.

Zero behavior change. Set-membership semantics are identical to the prior
tuple containment check. The strict-mode gate's hard-fail count is
unchanged. Six new tests pin the v1.3 fallthrough set contents and the
helper's boolean contract.

## § B. File inventory

| Status | Path | Diff |
|---|---|---|
| Modified | `tools/integrity/integrity/grandfather.py` | +27 LOC (constant + helper + docstring) / -1 LOC / +1 LOC (filter refactor) |
| Modified | `tools/integrity/tests/test_grandfather_sweep.py` | +47 LOC (6 tests + section banner) |
| Created | `docs/diagnostics/_audits/integrity_v1_3_part_b_spec_2026-05-16_architect1.md` | spec sibling-doc (staged with this commit; see § A of spec) |
| Created | `docs/diagnostics/_audits/integrity_v1_3_part_b_commit1_landing_2026-05-16.md` | this report |

No other paths touched.

## § C. Verification

Pre-edit anchoring:

```
$ git rev-parse HEAD
1f7785fd6567599f948c8eee68a7641032d3ff4a
$ grep -n "FALLTHROUGH" tools/integrity/integrity/grandfather.py
(empty before edit)
$ grep -n "other-cat1" tools/integrity/integrity/grandfather.py
(returns the literal-string tuple at apply_annotations and the
 fall-through return in classify(); confirms probe § C.2 anchor)
```

Post-edit:

```
$ grep -n "is_fallthrough_category" tools/integrity/integrity/grandfather.py
82:def is_fallthrough_category(category: str) -> bool:
431:            if is_fallthrough_category(cat) and is_live_source_path(f.file):
```

Helper importable:

```
$ python3 -c "from integrity.grandfather import is_fallthrough_category, \
    FALLTHROUGH_CATEGORIES; print(FALLTHROUGH_CATEGORIES); \
    print(is_fallthrough_category('other-cat1'))"
frozenset({'other-cat1-bare-path', 'other-cat1'})
True
```

Test suite (target file + full suite):

```
$ python3 -m pytest tools/integrity/tests/test_grandfather_sweep.py -v
============================== 33 passed in 0.04s ==============================
```

Full suite: 176 passed (170 pre-commit + 6 new); see § E check 1 for
the dual count baseline.

Gate state (pre-commit-1 vs post-commit-1; refactor zero-behavior):

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 5 pass, 0 soft-warn, 44 hard-fail, 1262 suppressed
```

Pre and post hard-fail counts both 44; the LIVE-SOURCE filter
semantics are unchanged.

## § D. Design decisions applied (per spec § 1.2)

| Decision | Spec § | Implementation |
|---|---|---|
| 1 — T1.2 design (d) FALLTHROUGH_CATEGORIES + helper | K.1 | Constant + helper added at module level in `grandfather.py` mirroring `SWEEPABLE_PATH_PREFIXES` idiom |
| 2 — Refactor scope: exactly one site | K.2 | Only the LIVE-SOURCE filter in `apply_annotations` was refactored. `snapshot.py`, `grandfather_sweep.py`, and tests that assert on literal category names are intentionally left alone |
| 3 — Commit ordering T1.2 → T1.1 → back-fill | K.3 | This is commit 1 (T1.2) |
| 9 — Convention H wording cross-link | K.9 | Helper docstring names "Convention H (v1.2 bolt-ons retro § 4.2)"; the module-level comment above the frozenset names it too |

## § E. Self-review checks

| Check | Result |
|---|---|
| 1 — Decision 7 sweep-diff exactness | N/A this commit (commit-2 concern) |
| 2 — Decision 8 `_KNOWN_CATEGORIES` extension | N/A this commit (commit-2 concern) |
| 3 — Decision 4 catalog-count consistency | N/A this commit (commit-2 concern) |
| 4 — Refactor zero-behavior-change | **PASS.** Hard-fail count 44 → 44; gate output identical |
| 5 — SHA citations resolve | Deferred to commit 3 (this report cites only the probe SHA `1f7785f`, which resolves) |

## § F. Banked observations

1. **Existing tests use module-level functions, not pytest classes.** The
   spec § 3.4 example test code wrapped its 6 tests in a `TestFallthrough
   CategoryHelper` class. The existing `test_grandfather_sweep.py` file is
   organized as bare `def test_*` functions with section banners (e.g.,
   "P1.8 -- live-source path-bucket tests"). Tests landed in the file-
   local idiom to keep the module visually consistent. Behavior-identical.

2. **`Finding` lives in `integrity.grandfather`, not `integrity.common.results`.**
   Spec § 3.4 / § 4.5 example code imported `Finding, FailureMode` from
   `integrity.common.results`. The actual `Finding` dataclass is at
   `tools/integrity/integrity/grandfather.py:24` and has no `mode` field
   (just `check_id`, `file`, `line`, `message`). Per spec hard rule #2
   (synced repo state is authoritative), the new tests use the existing
   `_f()` test helper and the `integrity.grandfather.Finding` import that
   the rest of the test file uses. The spec's import path was a probe-
   time assumption that didn't survive grep-verification.

3. **Module-level placement of the new constant.** Inserted immediately
   after `SWEEPABLE_EXACT_PATHS` and before `is_live_source_path`. This
   keeps all the LIVE-SOURCE / sweep-discriminator constants grouped at
   the top of the module, before `classify()` and `apply_annotations()`.

## § G. Cross-references

- Spec § 3 — T1.2 commit definition.
- Probe § C.2 — Pre-edit shape of the literal-string match (re-anchored
  at commit time; the filter site is now at grandfather.py:431, not
  probe-time line 406, reflecting the inserted constant block).
- Probe § K.1, K.2, K.3, K.9 — Decisions applied.
- v1.2 bolt-ons retro § 4.2 — Convention H source.
- v1.3 roadmap § 4 T1.2 — Original scope statement.

## § H. Next commit

Commit 2 — T1.1 three classifier rules + catalog sections + sweep
companion. SHA cross-reference for this commit will be filled in by
commit 3 (SHA back-fill): `239d7a2`.
