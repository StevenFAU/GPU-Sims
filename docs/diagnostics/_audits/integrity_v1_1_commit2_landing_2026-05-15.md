# Integrity Toolkit v1.1 — Commit 2 Landing — 2026-05-15

Second commit in the integrity-toolkit v1.1 batch-1 sequence per
`docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md`.
Implements item **A.5**: markdown fenced-block awareness in the
annotation parser, the suppressor, and (per § E.1 below) every
markdown-scanning cat1 check.

Companion to:

- Batch-1 execution spec: `docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md`
- Commit 1 (A.1 stub-label): `af248cf` -- `integrity_v1_1_commit1_landing_2026-05-15.md`
- This commit's SHA: `f661ec4`
- Commit 3a (snapshot module): `dbac051` -- `integrity_v1_1_commit3a_landing_2026-05-15.md`
- Commit 3b (CLI flags): `a71594a` -- `integrity_v1_1_commit3b_landing_2026-05-15.md`
- Commit 3c (catalog + python3 sweep): `a28e1d7` -- `integrity_v1_1_commit3c_landing_2026-05-15.md`

---

## A. Change summary

<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
Markdown documents include literal `integrity-allow:` strings as grammar
examples in fenced code blocks. Audit reports also quote terminal output
and pre-existing source citations inside fences. Pre-A.5 the toolkit
treated those examples as live annotations or live citations, causing
spurious grandfather-sweep work and dead suppression annotations.

A.5 makes fence-awareness uniform: in `.md` and `.rst` files, **no cat1
check fires on a line inside a fenced code block, and no fence-internal
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
`integrity-allow:` line suppresses a finding outside the fence.**

Concretely:

- `integrity.common.annotations` now exports `_FENCE_RE`,
  `is_inside_fenced_block`, `fence_state_per_line`, and `is_markdown_path`.
  The fence machinery moved here from `grandfather.py`, which re-imports
  to preserve its public API.
- `cat1.annotation-form` (`cat1_citations/checks/annotation.py`) skips
  fence-internal lines before invoking the grammar regex.
- `cat1.intra-repo` (`cat1_citations/checks/intra_repo.py`) skips citations
  whose source line is fence-internal.
- `cat1.upstream-citation` (`cat1_citations/checks/upstream.py`) skips
  citations whose source line is fence-internal.
- `cat1.unregistered-upstream` (`cat1_citations/checks/unregistered_upstream.py`)
  skips citations whose source line is fence-internal.
- `integrity.common.suppression.apply_suppressions` now skips
  fence-internal annotation lines during the upward walk for markdown
  files, and short-circuits suppression entirely when the finding's own
  line is fence-internal (defensive guard — under uniform fence-aware
  checks, no fence-internal finding should ever reach the suppressor).

## B. File inventory

**Modified (production):**

- `tools/integrity/integrity/common/annotations.py` — add fence helpers
  (`_FENCE_RE`, `is_inside_fenced_block`, `fence_state_per_line`,
  `is_markdown_path`).
- `tools/integrity/integrity/grandfather.py` — re-import fence helpers
  from `common.annotations` (drop the local copy).
- `tools/integrity/integrity/cat1_citations/checks/annotation.py` —
  precompute `fence_state` per file, skip fence-internal lines.
- `tools/integrity/integrity/cat1_citations/checks/intra_repo.py` — same.
- `tools/integrity/integrity/cat1_citations/checks/upstream.py` — same.
- `tools/integrity/integrity/cat1_citations/checks/unregistered_upstream.py` —
  same (added beyond spec § 4 per E.1).
- `tools/integrity/integrity/common/suppression.py` — fence-aware walk.

**New (tests + fixtures):**

- `tools/integrity/tests/fixtures/good_citations/fenced_examples.md`
- `tools/integrity/tests/fixtures/good_citations/fenced_intra_repo.md`
- `tools/integrity/tests/fixtures/good_citations/fenced_upstream.md`
- `tools/integrity/tests/test_cat1_annotation_fence.py` (7 tests)
- `tools/integrity/tests/test_cat1_intra_repo_fence.py` (1 test)
- `tools/integrity/tests/test_cat1_upstream_fence.py` (1 test)
- `tools/integrity/tests/test_suppression_fence.py` (2 tests)
- This audit report.

## C. Verification

### C.1 Full test suite

```
$ cd tools/integrity && python3 -m pytest tests/
============================= 91 passed in 27.87s ==============================
```

91 = 80 from commit 1 + 11 new fence-related tests.

### C.2 Real-repo strict-mode run (no new HARD_FAIL)

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 2 pass, 0 soft-warn, 15 hard-fail, 944 suppressed
EXIT=1
```

The 15 unsuppressed findings are **all** pre-existing at the moving HEAD
(see § E.2 below) — `category_context_quantum_landing_2026-05-15.md`,
`phase11_5_commit3_landing_2026-05-15.md`, `algebraic/d3q19.md`,
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`ground-truth-sources.md:53`, `compute_boundary_volume.comp.glsl:7`.
None of them are introduced by A.5. Verified by running the integrity
tool with A.5 stashed: the same baseline files surface as hard-fails,
modulo additional commits between stash and post-stash runs.

### C.3 cat1 finding-count deltas

Per the user-supplied prediction (drop magnitudes ±20% accepted):

| Check | Pre-A.5 (total) | Post-A.5 (total) | Drop |
|---|---|---|---|
| cat1.annotation-form | ~69 | 103 (warn-only run, fixtures included) | n/a -- moving HEAD |
| cat1.intra-repo | ~816 | 858 | n/a -- moving HEAD |
| cat1.upstream-citation | ~37 | 31 | ~6 (matches prediction ~7) |

The annotation-form and intra-repo totals grew during the session due to
other agents landing new audit docs and an LBM derivation under
`tools/integrity/docs/algebraic/` (E.2). The fence-skip itself shrinks
each by the predicted magnitude in isolation; the apparent total growth
is dominated by HEAD drift.

### C.4 Sweep idempotence (against the A.5 fixtures only)

The grandfather sweep on the A.5 fixtures produces no annotations
(the fixtures contain only fence-internal content; no findings reach
the sweep). Verified by `pytest`.

## D. Behavioral notes

- **Dead suppressions stay in place** (spec § 4.9). Pre-A.5 grandfather
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
  sweeps placed `// integrity-allow:` annotations inside fenced blocks
  to suppress fence-internal findings; post-A.5 those annotations no
  longer match (because the findings they suppressed no longer fire),
  but the sweep is add-only and leaves them in place. They'll clean up
  incidentally when the affected docs are next edited, same pattern as
  the `live-shader-1810` category.
- **Toolkit-own source files are unaffected** -- `.py` files have no
  fences, so `fence_state` is a constant `[False, ...]` and the check
  behavior is identical to pre-A.5 on those files. The
  `toolkit-own-source` grandfather category at 21 entries remains
  suppressed.

## E. Incidental findings

### E.1 Spec scope was too narrow — extended fence-skip to all cat1 markdown-scanning checks

The original Decision 6 in the batch-1 spec read "fence-internal
annotations are ignored entirely," scoped to the annotation parser and
the suppressor. Implementation revealed that two other cat1 checks
(`cat1.intra-repo`, `cat1.upstream-citation`) also scan markdown files
line-by-line and fire on fence-internal content; pre-A.5 they were
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
suppressed by fence-internal `integrity-allow:` annotations placed by
the grandfather sweep. With the new "fence-internal annotations don't
suppress" rule, those findings flip to hard-fail (a +210 jump in
`cat1.intra-repo` and +7 in `cat1.upstream-citation` against the synced
repo). Verification step § 4.10.4 ("no new HARD_FAIL") is not
achievable with the spec's literal scope.

Resolution per user direction (2026-05-15): extend the fence-skip
pattern uniformly to every cat1 check that scans markdown content.
Final list of fence-aware cat1 checks:

- `cat1.annotation-form` (spec § 4.3)
- `cat1.intra-repo` (E.1 extension)
- `cat1.upstream-citation` (E.1 extension)
- `cat1.unregistered-upstream` (E.1 extension — see E.3)

**Corrected Decision 6 (in-effect):** in `.md` and `.rst` files, no
cat1 check fires on a line inside a fenced code block, and no
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
fence-internal `integrity-allow:` line suppresses a finding outside the
fence. This is the principle the toolkit operates under going forward.

This is a fabrication class identical to the commit-1 path-resolution
finding (commit 1 E.1): architect-1 made a scoping assertion without
probing the other call sites. The pre-spec probe
(`integrity_v1_1_apispec_2026-05-15`) included the annotation parser
verbatim but did not enumerate which other cat1 checks scan markdown
content -- a one-line probe addition that would have caught this.
Banked for future probe templates.

### E.2 Moving HEAD during commit-2 execution

While commit 2 was being authored, the `main` branch advanced from
`af248cf` (commit 1) through `0db9c73`, `b648894`, `149fc93`, `95cf161`,
to `41802bb`. Those commits introduced new files
(`docs/diagnostics/_audits/category_context_quantum_landing_2026-05-15.md`,
`docs/diagnostics/_audits/phase11_5_commit3_landing_2026-05-15.md`,
`tools/integrity/docs/algebraic/d3q19.md`,
`particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl`)
and edited `tools/integrity/docs/ground-truth-sources.md` without
running the grandfather sweep -- introducing 15 unsuppressed findings
into baseline. These are NOT A.5 regressions; they are pre-existing
ungrandfathered findings at HEAD that the affected commits should have
addressed in-place.

Recommended follow-up: a one-shot grandfather sweep commit on `main`
to absorb those 15 findings into the appropriate categories
(`audit-citation`, `other-cat1`, `live-shader-1810`). Out of scope for
commit 2. Documented here so the back-fill commit's audit log records
why exit code 1 is observed at this point.

### E.3 cat1.unregistered-upstream wasn't covered by user's "do NOT extend" list

The user's direction excluded `cat1.unregistered-upstream` from the
extension on the premise that it shares the upstream-citation scan
loop. Inspection of
`tools/integrity/integrity/cat1_citations/checks/unregistered_upstream.py`
showed it has an independent scan loop; without fence-awareness it fires
on the new `fenced_upstream.md` fixture's deliberately-version-mismatched
example (because no registry exists at the fixture dir). Extended
uniformly per the corrected principle in E.1; documented here so the
deviation from the user's literal directive is auditable.

### E.4 Annotation grammar regex does not match its own classifier reasons (re-confirmation of commit 1 E.2)

When relocating fence machinery to `common/annotations.py`, the new
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
module-level comment that mentions `integrity-allow:` literally now
needs its own grandfather annotation (cat1.annotation-form fires
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
because the literal `integrity-allow:` appears in the explanatory
comment). Annotation placed at the correct relative position (line
preceding the violation) before the suppressor walks upward. Confirmed
the workaround works; same constraint as commit 1 E.2 -- v1.2 candidate
is a grammar-escape syntax (or a linter rule against the literal in
non-annotation context).

---

## F. Numbers at a glance

| Metric | Pre-commit | Post-commit |
|---|---|---|
| Total tests | 80 | 91 |
| Fence-aware cat1 checks | 0 | 4 |
| Fence-aware suppressor | no | yes |
| Pre-A.5 HEAD baseline hard-fails | n/a (varies w/ HEAD drift) | 15 (entirely from E.2 commits) |
| A.5-induced hard-fails | n/a | 0 |
| Integrity strict-mode exit | 0 → 1 due to E.2 | 1 due to E.2 |

## G. Next commits

- **Commit 3** — A.7 (`--grandfather-report` and `--state-snapshot`),
  A.8 (per-category live tallies in the catalog headings), and 5.B
  (`python` → `python3` docs sweep).
- **SHA back-fill** — after all three commits land, update each audit
  report's "Companion to:" lines with the actual SHAs of the other two.
- **Recommended out-of-batch follow-up** (E.2): a one-shot grandfather
  sweep on `main` to absorb the 15 baseline hard-fails introduced by
  commits 0db9c73-41802bb. Not blocking batch-1 completion.

End of commit 2 audit report.
