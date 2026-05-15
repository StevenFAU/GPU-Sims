---
title: "Integrity v1.1 Post-Batch Hard-Fail Triage"
date: 2026-05-15
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_1_commit1_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_commit2_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_commit3a_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_commit3b_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_commit3c_landing_2026-05-15.md
---

# Integrity v1.1 Post-Batch Hard-Fail Triage — 2026-05-15

Closes the post-batch follow-up flagged in commits 2/3a/3b/3c audit
reports (E.2 in each): the strict-mode gate stayed red at HEAD because
concurrent commits 0db9c73..66daf9f (plus follow-ons through `a28e1d7`)
landed new audit docs, toolkit docs, an LBM phase spec, an Akinci2012
shader, and a fence-test fixture without grandfather-sweep companion
commits.

Triage policy (hybrid, per directive 2026-05-15):

- **AUDIT-DOC** — files under `docs/diagnostics/_audits/` or `docs/retro/`.
  Grandfather-sweep these; the existing classifier handles them via the
  permanent `audit-citation` and `audit-report-grammar-example`
  categories. Audit docs are append-only by convention.
- **TOOLKIT-DOC** — files under `tools/integrity/docs/` or
  `docs/integrity-toolkit-spec.md` or `tools/integrity/README.md`.
  Grandfather-sweep these too; the existing classifier handles them
  via fallthrough to `other-cat1`. A named permanent category
  (`toolkit-doc-snapshot`) is recommended in § C for v1.2.
- **LIVE-SOURCE** — everything else (sim code, common-* code, shaders,
  phase specs in `docs/` outside the toolkit-doc set, toolkit's own
  test code). **DO NOT sweep.** Attribute the finding back to the
  introducing commit and let the owner decide whether to fix or
  acknowledge.

---

## § A. Enumerated findings

Verbatim from `python3 -m integrity --mode strict --no-audit-log --output json`,
captured 2026-05-15 ~17:00 UTC against HEAD `a28e1d7`:

**Count delta:** baseline at end of batch-1 was 29 hard-fails. At triage
time HEAD had advanced to `a28e1d7` (my own commit 3c landing report
introduced one additional cat1.annotation-form finding at
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`integrity_v1_1_commit3c_landing_2026-05-15.md:171`), bringing the total
to 30. Same-class-of-finding as the original 29; included in the
audit-doc sweep below.

| # | Check | File | Line | Bucket |
|---|---|---|---|---|
|  1 | cat1.intra-repo | docs/diagnostics/_audits/category_context_quantum_landing_2026-05-15.md | 80 | AUDIT-DOC |
|  2 | cat1.intra-repo | docs/diagnostics/_audits/category_context_quantum_landing_2026-05-15.md | 288 | AUDIT-DOC |
|  3 | cat1.intra-repo | docs/diagnostics/_audits/integrity_v1_1_commit2_landing_2026-05-15.md | 102 | AUDIT-DOC |
|  4 | cat1.intra-repo | docs/diagnostics/_audits/integrity_v1_1_commit2_landing_2026-05-15.md | 102 | AUDIT-DOC |
|  5 | cat1.intra-repo | docs/diagnostics/_audits/phase11_5_commit3_landing_2026-05-15.md | 139 | AUDIT-DOC |
|  6 | cat1.intra-repo | docs/diagnostics/_audits/phase11_5_commit3_landing_2026-05-15.md | 139 | AUDIT-DOC |
|  7 | cat1.intra-repo | docs/diagnostics/_audits/phase11_5_commit3_landing_2026-05-15.md | 141 | AUDIT-DOC |
|  8 | cat1.intra-repo | docs/diagnostics/_audits/phase11_5_commit3_landing_2026-05-15.md | 143 | AUDIT-DOC |
|  9 | cat1.intra-repo | docs/diagnostics/_audits/phase11_5_commit3_landing_2026-05-15.md | 149 | AUDIT-DOC |
| 10 | cat1.intra-repo | docs/diagnostics/_audits/phase11_5_commit3_landing_2026-05-15.md | 202 | AUDIT-DOC |
| 11 | cat1.annotation-form | docs/diagnostics/_audits/category_context_quantum_landing_2026-05-15.md | 139 | AUDIT-DOC |
| 12 | cat1.annotation-form | docs/diagnostics/_audits/category_context_quantum_landing_2026-05-15.md | 289 | AUDIT-DOC |
| 13 | cat1.annotation-form | docs/diagnostics/_audits/integrity_v1_1_commit2_landing_2026-05-15.md | 22 | AUDIT-DOC |
| 14 | cat1.annotation-form | docs/diagnostics/_audits/integrity_v1_1_commit2_landing_2026-05-15.md | 30 | AUDIT-DOC |
| 15 | cat1.annotation-form | docs/diagnostics/_audits/integrity_v1_1_commit2_landing_2026-05-15.md | 132 | AUDIT-DOC |
| 16 | cat1.annotation-form | docs/diagnostics/_audits/integrity_v1_1_commit2_landing_2026-05-15.md | 153 | AUDIT-DOC |
| 17 | cat1.annotation-form | docs/diagnostics/_audits/integrity_v1_1_commit2_landing_2026-05-15.md | 171 | AUDIT-DOC |
| 18 | cat1.annotation-form | docs/diagnostics/_audits/integrity_v1_1_commit2_landing_2026-05-15.md | 218 | AUDIT-DOC |
| 19 | cat1.annotation-form | docs/diagnostics/_audits/integrity_v1_1_commit2_landing_2026-05-15.md | 220 | AUDIT-DOC |
| 20 | cat1.annotation-form | docs/diagnostics/_audits/integrity_v1_1_commit3c_landing_2026-05-15.md | 171 | AUDIT-DOC |
| 21 | cat1.intra-repo | tools/integrity/docs/algebraic/d3q19.md | 174 | TOOLKIT-DOC |
| 22 | cat1.intra-repo | tools/integrity/docs/algebraic/d3q19.md | 175 | TOOLKIT-DOC |
| 23 | cat1.intra-repo | tools/integrity/docs/ground-truth-sources.md | 53 | TOOLKIT-DOC |
| 24 | cat1.intra-repo | tools/integrity/docs/ground-truth-sources.md | 53 | TOOLKIT-DOC |
| 25 | cat1.intra-repo | docs/phase12_lattice_boltzmann.md | 203 | LIVE-SOURCE |
| 26 | cat1.intra-repo | docs/phase12_lattice_boltzmann.md | 351 | LIVE-SOURCE |
| 27 | cat1.intra-repo | docs/phase12_lattice_boltzmann.md | 1276 | LIVE-SOURCE |
| 28 | cat1.intra-repo | particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl | 7 | LIVE-SOURCE |
| 29 | cat1.annotation-form | tools/integrity/tests/test_suppression_fence.py | 3 | LIVE-SOURCE |
| 30 | cat1.annotation-form | tools/integrity/tests/test_suppression_fence.py | 23 | LIVE-SOURCE |

## § B. Bucket categorization

| Bucket | Count | Files | Action |
|---|---|---|---|
| AUDIT-DOC | 20 | 4 distinct audit reports | Sweep |
| TOOLKIT-DOC | 4 | 2 toolkit docs | Sweep (fallthrough category) |
| LIVE-SOURCE | 6 | 3 distinct files | Attribute |
| **Total** | **30** | | |

## § C. Resolution per bucket

### § C.1 AUDIT-DOC — swept and committed

Sweep applied as commit **`bcba679`**
(`grandfather(integrity): sweep audit-doc findings from concurrent
commits 0db9c73..66daf9f`). 18 annotations added across 4 audit reports.
Categories used: `audit-citation` (10 cat1.intra-repo) and
`audit-report-grammar-example` (10 cat1.annotation-form). Both are
existing permanent categories per `tools/integrity/docs/grandfather-catalog.md`.

Idempotent: re-running the sweep produces 0 additional annotations.

### § C.2 TOOLKIT-DOC — swept under fallthrough; named category recommended

Sweep applied as commit **`9f85c7f`**
(`grandfather(integrity): sweep toolkit-doc findings from concurrent
commits 0db9c73..66daf9f`). 3 annotations added across 2 files (one
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
covers two citations on the same line in `ground-truth-sources.md:53`).
All 4 findings classified as **`other-cat1`** by fallthrough.

The classifier in `tools/integrity/integrity/grandfather.py` has no
named rule for `cat1.intra-repo` findings in
`tools/integrity/docs/`-rooted paths. The fallthrough works
correctly — annotations parse, findings suppress, the gate goes
green — but it conflates these toolkit-doc citations with the
heterogeneous "other-cat1" bucket which currently holds 70 entries
of varied provenance.

**Recommended v1.2 classifier extension** (architect-2 review item):

```python
# tools/integrity/integrity/grandfather.py, inside classify(), placed
# before the cat1.annotation-form rules:

if cid == "cat1.intra-repo" and (
    f.startswith("tools/integrity/docs/")
    or f == "docs/integrity-toolkit-spec.md"
    or f == "tools/integrity/README.md"
):
    return Classification(
        category="toolkit-doc-snapshot",
        reason="toolkit-doc snapshot of pre-v1 codebase (see grandfather-catalog toolkit-doc-snapshot)",
        issue_ref="n/a",
    )
```

Plus a new section in `tools/integrity/docs/grandfather-catalog.md`
describing the category, mirroring the `audit-citation` rationale
(toolkit docs cite the vendored Krüger book-companion code which
won't move; the citations are reference material, not source-to-source
intra-repo refs).

Doing this in v1.2 requires re-sweeping (the existing `other-cat1`
annotations would not match the new `toolkit-doc-snapshot` rule).
Banked under v1.2 candidate D in the v1.1 retro.

### § C.3 LIVE-SOURCE — attributed, NOT swept

See § D for the attribution table. The strict-mode gate will continue
to fail on these 6 findings until each is fixed or explicitly
acknowledged by its author. **Sweeping live-source findings would
defeat the gate's purpose** and is out of scope per directive § 6.

## § D. LIVE-SOURCE attribution

| # | Finding | Introducing commit | Author |
|---|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 25 | docs/phase12_lattice_boltzmann.md:203 (`chapter13/cpu/LBM.cpp:97`) | `c5955d3` setup(phase12): land architect-1 spec | Steven Cohen |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 26 | docs/phase12_lattice_boltzmann.md:351 (`chapter13/cpu/LBM.cpp:97`) | `c5955d3` setup(phase12): land architect-1 spec | Steven Cohen |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| 27 | docs/phase12_lattice_boltzmann.md:1276 (`main.cpp:1168-1279`) | `c5955d3` setup(phase12): land architect-1 spec | Steven Cohen |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 28 | particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl:7 (`SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75`) | `f9f2cb9` feat(sph-water): Akinci2012 boundary handling (commit 3) | Steven Cohen |
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
| 29 | tools/integrity/tests/test_suppression_fence.py:3 (docstring literal `integrity-allow:`) | `f661ec4` feat(integrity): A.5 fenced-block awareness (v1.1 batch 1 commit 2) | Steven Cohen |
| 30 | tools/integrity/tests/test_suppression_fence.py:23 (test-fixture inline annotation literal) | `f661ec4` feat(integrity): A.5 fenced-block awareness (v1.1 batch 1 commit 2) | Steven Cohen |

All six attributable to the same human (Steven Cohen). Three different
sessions / commit topics:

- Phase 12 LBM (architect-1 spec landing) — 3 findings.
- Phase 11.5 sph-water (Akinci2012 boundary handling commit 3) — 1 finding.
- v1.1 batch-1 integrity work (this chat's commit 2 — fence A.5) — 2 findings.

The session/topic pattern is the canonical operating-condition
fingerprint: concurrent specialized agents landing distinct work
streams in parallel against the same `main`. None of the six is
the `1.8.10` fabrication class the toolkit was built to catch
(§ G.1 below).

## § E. Outstanding work

After this triage's three commits (`bcba679` audit-doc sweep,
`9f85c7f` toolkit-doc sweep, and this report — SHA back-fill is
landed inline since each commit references prior SHAs only), the
gate remains red on the 6 LIVE-SOURCE findings:

### Owner: Phase 12 LBM author (Steven Cohen via `c5955d3`)

1. `docs/phase12_lattice_boltzmann.md:203` — bare path
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
   `chapter13/cpu/LBM.cpp:97`. Two valid fixes:
   - Promote the bare path to a registered-upstream citation:
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
     `Krueger book-companion-code-2016 chapter13/cpu/LBM.cpp:97`.
     The Krueger upstream is registered at
     `tools/integrity/docs/ground-truth-sources.md`.
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
   - Or, add an inline `integrity-allow: cat1.intra-repo` annotation
     justifying why this bare path is intentional (typically not
     justifiable — registered citation is preferred).
2. `docs/phase12_lattice_boltzmann.md:351` — same shape as #1.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
3. `docs/phase12_lattice_boltzmann.md:1276` — bare `main.cpp:1168-1279`.
   "ES's pattern from main.cpp" is too ambiguous to be a citation
   under spec § 3 grammar. Either disambiguate ("eulerian-smoke's
   `main.cpp` in particle-fluids/eulerian-smoke/src/main.cpp at
   lines 1168-1279") or drop the file:line and write prose.

### Owner: sph-water author (Steven Cohen via `f9f2cb9`)

4. `particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl:7`
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
   — citation `SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75` is a
   bare upstream path (uses the upstream prefix but not the upstream-
   citation grammar). Two valid fixes:
   - Rewrite as a proper upstream citation:
<!-- integrity-allow: cat1.upstream-citation; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
     `SPlisHSPlasH 2.16.1 BoundaryModel_Akinci2012.cpp:48-75`.
     The SPlisHSPlasH 2.16.1 anchor is registered.
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
   - Or, add an inline `integrity-allow: cat1.intra-repo` annotation
     (less preferred — registered citation is the canonical form).

### Owner: integrity v1.1 batch-1 author (Steven Cohen via `f661ec4` — this chat)

5. `tools/integrity/tests/test_suppression_fence.py:3` —
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
   the `integrity-allow:` literal appears inside the module
   docstring. The check fires because the docstring isn't fence-
   guarded (`.py` files don't have markdown fences, and my A.5
   work explicitly excludes non-md files). Two valid fixes:
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
   - Add an inline `# integrity-allow: cat1.annotation-form` annotation
     above line 3 (matching the pattern already used in 8 other
     toolkit-own-source files).
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
   - Or rewrite the docstring to not include the literal `integrity-allow:`
     token (e.g. use a different placeholder).
6. `tools/integrity/tests/test_suppression_fence.py:23` — the
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
   `# integrity-allow:` literal appears inside a Python string
   literal that's part of a `md.write_text(...)` call constructing
   a markdown fixture. Same options as #5.

**Recommendation for the v1.1 batch-1 author:** I should fix #5 and
#6 myself in a follow-up commit since I am the f661ec4 author and
the gate stays red on them. Doing it in a separate "fix" commit
keeps the triage commits scoped purely to the post-batch cleanup.
**This is outside the triage's literal scope (§ 6: "Fixing the
underlying live-source defects ... [is out of scope]. Attribution
only.")** but I'll surface it for the author's decision in a
follow-up exchange.

## § F. Operating-conditions note (for v1.1 retro § operating-conditions)

This triage is direct evidence for the retro item already banked in
commit 3b's audit report E.1: **the v1 spec's commit pattern assumed
serialized landing against `main`. The actual operating condition is
concurrent multi-agent work.**

Of the 30 hard-fails this triage cleared (or attributed), 24 came from
4 commits that landed during the v1.1 batch-1 chat without
grandfather-sweep companions. The pattern is reproducible and will
recur on every multi-stream phase.

**Recommended hard convention for v1.1+ retro:**

> Every commit that touches the cat1-scannable surface (live shaders,
> common-* code, sim code, audit docs under `docs/diagnostics/_audits/`,
> toolkit docs under `tools/integrity/docs/`, phase specs under
> `docs/phase*.md`) either runs `python3 tools/integrity/scripts/grandfather_sweep.py`
> as part of the commit OR lands a companion grandfather-sweep commit
> within the same PR.

Two specific patterns to bank:

1. **New-files-only commits are race-immune.** When the work can be
   decomposed so that a sub-commit only adds new files, ship that
   sub-commit first. The race window is precisely the duration of
   the runner.py / catalog.md / shared-file edit. Demonstrated
   successfully in batch-1 commit 3 decomposition (3a/3b/3c).
2. **Audit docs should be paired with their sweep.** A commit that
   lands a new file under `docs/diagnostics/_audits/` and is
   structurally guaranteed to introduce `cat1.intra-repo` and/or
   `cat1.annotation-form` findings (every nontrivial audit doc
   does) should run the sweep as its second action. Otherwise
   the next person to run `python3 -m integrity` inherits red CI.

Architect-2 review item: should the convention be enforced by a
pre-commit hook (auto-run grandfather sweep if any cat1-scannable
file was modified)? Cost: a slow git-commit on toolchain edits.
Benefit: zero post-batch triage.

## § G. Banked for retro (§ 7 directive items)

### § G.1 Authorship pattern

All 6 LIVE-SOURCE findings + 24 swept findings attributable to the
**same human author** (Steven Cohen) via **3-4 different concurrent
sessions / topics**:

- Phase 12 LBM setup (architect-1 session) — 3 LIVE-SOURCE + ~6
  AUDIT-DOC.
- Phase 11.5 sph-water continued work (multiple commits) — 1
  LIVE-SOURCE + ~14 AUDIT-DOC.
- Quantum category-context landing (concurrent doc work) — 0
  LIVE-SOURCE + 4 AUDIT-DOC.
- v1.1 batch-1 integrity (this chat) — 2 LIVE-SOURCE in my own
  test code + most of the audit-doc grammar examples.

Not a fabrication pattern — all citations resolved to real files at
real upstream versions or real internal paths; the issues are
**grammar/discipline misses** (bare paths instead of registered
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
upstream citations, literal `integrity-allow:` tokens in test code
that need own-source annotations).

### § G.2 Are these the fabrication class the toolkit was built to catch?

**No.** Zero of the 30 findings are the `SPlisHSPlasH 1.8.10` /
`Chakazul/Lenia/Python/LeniaNDK.py` fabrication class. They are:

- **Bare-path citations** (LIVE-SOURCE 25, 26, 27, 28) — the author
  knew the upstream/internal file but didn't write it in the
  registered-citation grammar. **A.3 of the v1.1 spec
  ("bare-path-to-upstream-basename") would catch and auto-correct
  these.** Deferred to a later batch per batch-1 § 1.1; this triage
  is direct evidence that A.3 is high-priority.
- **Audit-doc-internal grammar literals** (AUDIT-DOC entries 11-20)
  — necessary mentions of the annotation grammar in prose. The
  existing `audit-report-grammar-example` category absorbs them
  cleanly.
- **Audit-doc-internal intra-repo citations** (AUDIT-DOC entries
  1-10) — audit reports quote file paths that may not resolve at
  audit time. The existing `audit-citation` category absorbs them
  permanently.
- **Toolkit-doc Krüger citations** (TOOLKIT-DOC entries 21-24) —
  reference material citations. New `toolkit-doc-snapshot` category
  recommended in § C.2.
- **Test-fixture grammar literals in own-source** (LIVE-SOURCE 29,
  30) — toolkit's own test code embeds the grammar literal. Needs
  own-source annotation.

### § G.3 Classifier coverage

The existing classifier rules cleanly covered:

- **AUDIT-DOC**: `audit-citation` (cat1.intra-repo) and
  `audit-report-grammar-example` (cat1.annotation-form) categories
  match perfectly.
- **TOOLKIT-DOC**: fallthrough to `other-cat1` works but recommends
  a named `toolkit-doc-snapshot` category (§ C.2).

The classifier did NOT need any new rules to land this triage. The
recommended `toolkit-doc-snapshot` category is a v1.2 cleanup
candidate, not a v1.1 blocker.

## § H. Verification

### § H.1 Real-repo strict-mode run post-triage

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 2 pass, 0 soft-warn, 6 hard-fail, 967 suppressed
EXIT=1
```

The 6 hard-fails are exactly the LIVE-SOURCE bucket. 967 suppressed =
943 baseline + 24 from the two sweeps in this triage. Per directive
§ 5.2, this is the intended state — the gate stays red on
live-source until each finding is fixed or acknowledged by its
author.

### § H.2 Grandfather report post-triage

```
$ python3 -m integrity --grandfather-report --no-history-append
grandfather report @ 9f85c7f (...)
summary: {'pass': 2, 'soft_warn': 0, 'hard_fail': 6, 'suppressed': 967}
per-category counts:
                       audit-citation: 607      (+10)
                  cat2-stack-c-unused: 110
                  cat2-stack-b-unused: 73
                           other-cat1: 70        (+4 from § C.2)
         audit-report-grammar-example: 29       (+10)
                   toolkit-own-source: 22
                 spec-grammar-example: 17
                  cat2-stack-d-unused: 17
                       audit-doc-1810: 15
                     live-shader-1810: 3
                retro-grammar-example: 2
                cat2-stub-label-stale: 2
```

Per-category deltas match the swept counts: `audit-citation` +10,
`audit-report-grammar-example` +10, `other-cat1` +4.

### § H.3 State-snapshot post-triage

```
$ python3 -m integrity --state-snapshot > /tmp/post-triage.json
$ python3 -c "import json; d=json.load(open('/tmp/post-triage.json')); print(sorted(d.keys()))"
['commit', 'per_category', 'registered_checks', 'registered_upstreams', 'schema_version', 'summary', 'timestamp']
```

Valid JSON. All expected keys present. 10 registered checks
(5 cat1, 4 cat2, 1 cat3), 2 registered upstreams.

End of post-batch triage report.
