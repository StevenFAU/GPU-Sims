---
title: "Integrity v1.1 Post-Retro Landing"
date: 2026-05-15
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/diagnostics/_audits/integrity_v1_1_post_batch_triage_2026-05-15.md
---

# Integrity v1.1 Post-Retro Landing — 2026-05-15

Records the v1.1 batch-1 retro landing (step 1) and the own-source-fix
landing (step 2) plus the retro's grandfather-sweep companion commit.
Closes the "Owner: integrity v1.1 batch-1 author" line item from the
post-batch triage report § E (rows 29-30).

## § A. Change summary

Two principal commits landed:

- **Step 1 — retro** (`f541557`): bring
  `docs/retro/integrity-toolkit-v1.1-batch1.md` into the repo as a
  595-line accumulated retrospective covering the entire v1.1 batch-1
  work (commits `af248cf..78e18d6` plus post-batch triage commits
  `bcba679..445b5a0`). Self-drafted by architect-1; § 0 discloses the
  bias; § 6.4 recommends an architect-2 review pass before locking
  conclusions.

- **Step 2 — fix** (`a42085a`): annotate two toolkit-own grammar
  literals in `tools/integrity/tests/test_suppression_fence.py`
  (lines 3 and 23 of that file pre-fix). Closes a retro § 9
  recommendation: the two findings are fabrication-adjacent
  (own-source discipline gap), and the v1.1 batch-1 author owned the
  fix.

One supporting commit also landed in this session:

- **Step 1 sweep companion** (`9c057e5`): grandfather-sweep the retro
  document itself. The retro discusses the annotation grammar and
  quotes file:line references in narrative prose — same structural
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
  pattern as audit-doc landings — so 10 inline `integrity-allow:`
  annotations were applied across 5 cat1.annotation-form findings
  (retro-grammar-example category) and 5 cat1.intra-repo findings
  (other-cat1 fallthrough; v1.2 candidate for a named
  retro-doc-snapshot category alongside the toolkit-doc-snapshot and
  project-state-snapshot extensions recommended in triage § C.2 and
  the addendum in § D.3 below).

## § B. File inventory

**Step 1 (`f541557`):**

- **New:** `docs/retro/integrity-toolkit-v1.1-batch1.md` (596 lines,
  drafted by architect-1, copied into the repo from the uploaded
  retro file with no edits).

**Step 1 sweep companion (`9c057e5`):**

- **Modified:** `docs/retro/integrity-toolkit-v1.1-batch1.md` (+10
  inline annotations).

**Step 2 (`a42085a`):**

- **Modified:** `tools/integrity/tests/test_suppression_fence.py`
  (+2 lines: one annotation inserted into the module docstring above
  the literal-bearing line; one trailing Python comment with an
  annotation on the `"```cpp"` list element above the literal-
  bearing string-literal list element).

**Step 3 (this audit, separate commit):**

- **New:**
  `docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md`.

## § C. Verification

### § C.1 Gate state immediately after step 1 (retro landing)

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 2 pass, 0 soft-warn, 16 hard-fail, 985 suppressed
```

The retro itself introduced 9 new audit-doc findings (5
cat1.annotation-form grammar mentions in narrative, 4 cat1.intra-repo
bare-path citations in narrative). Mitigated immediately by the sweep
companion commit `9c057e5`.

### § C.2 Gate state after step 1 sweep companion

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 2 pass, 0 soft-warn, 6 hard-fail, 995 suppressed
```

The 6 hard-fails are the same set as the triage snapshot (§ A of the
post-batch triage report): 3 Phase 12 LBM bare-path findings, 1
sph-water Akinci2012 bare-path finding, 2 own-source grammar
literals in `test_suppression_fence.py`. The retro's own findings
are now grandfather-suppressed.

### § C.3 Gate state immediately after step 2 (own-source fix)

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 2 pass, 0 soft-warn, 4 hard-fail, 997 suppressed
EXIT=1
```

Down from 6 to 4. The 2 own-source findings on
`test_suppression_fence.py` are suppressed by the inline annotations
added in step 2.

### § C.4 cat1.annotation-form findings on `test_suppression_fence.py`

```
$ python3 -m integrity --check cat1.annotation-form --output json --no-audit-log \
    | python3 -c "import json,sys; d=json.load(sys.stdin); \
        print([f for f in d['findings'] \
               if 'test_suppression_fence' in f['file'] \
               and not f.get('suppressed')])"
[]
```

Both findings on `test_suppression_fence.py` are now suppressed.
Specifically the underlying findings (at the post-edit line numbers
4 and 24) carry `suppressed=True`.

### § C.5 Pytest test_suppression_fence still green

```
$ cd tools/integrity && python3 -m pytest tests/test_suppression_fence.py -v
collected 2 items

tests/test_suppression_fence.py::test_fence_internal_annotation_does_not_suppress PASSED [ 50%]
tests/test_suppression_fence.py::test_live_annotation_above_fence_suppresses PASSED [100%]

============================== 2 passed in 0.01s ===============================
```

The annotations are inert (one is inside the module docstring, the
other is a trailing Python comment on a list-element line) — they
do not change test behavior.

## § D. Outstanding work after this landing

### § D.1 — Pre-existing live-source findings

4 live-source hard-fails remain, all attributed in triage § E to
their introducing authors:

- 3 in `docs/phase12_lattice_boltzmann.md` (lines 203, 351, 1276) —
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
  bare paths `chapter13/cpu/LBM.cpp:97`, `chapter13/cpu/LBM.cpp:97`,
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
  `main.cpp:1168-1279`. Owner: Phase 12 LBM author
  (`c5955d3 setup(phase12): land architect-1 spec`).
- 1 in
  `particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl:7`
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
  — bare upstream path `SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75`.
  Owner: sph-water Akinci2012 author
  (`f9f2cb9 feat(sph-water): Akinci2012 boundary handling (commit 3)`).

All four are bare-path discipline-class findings. Per retro § 5.1,
these are the canonical leverage case for A.3 in batch 2:
bare-path-to-upstream-basename would catch all four mechanically
and propose a rewrite to the registered-upstream-citation form.

### § D.2 — Additional live-source findings surfaced between triage and post-retro landing

At post-retro-landing time, the real-repo strict gate showed 11
hard-fails, not the 6 expected from the triage snapshot. Five new
findings appeared in `project-state.md` (lines 559, 592, 660 ×3)
between triage HEAD `a28e1d7` and post-retro HEAD `a42085a`.

Content verification (read-only `view` at retro time): the cited
lines sit inside accumulated cross-phase reflection prose covering:

- Line 559-560: narrative about a Phase 10 polish-4 episode (Taichi
  GGUI cursor-y origin issue in Lenia/MPM), with a bare-path
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
  citation `main.py:306-318` missing the `hybrid-particle-grid/mpm-multimaterial/python/`
  prefix.
- Line 592-593: narrative about a Phase 11 sph-water mid-revision
  probe (StateWriter .bin auto-append convention), with a bare-path
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
  citation `state_writer.cpp:57` missing the
  `common/common-cpp/src/` prefix.
- Line 660 (×3): narrative about a Phase 8.5.1 Stack C episode
  (createDebugMessenger name-collision in the Vulkan Debug-job CI),
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
  with bare-path citations `context.hpp:78`, `context.cpp:116`,
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
  `context.cpp:202` missing the `common/common-cpp/include/gpusims/vk/`
  and `common/common-cpp/src/vk/` prefixes.

Attribution: Steven Cohen, project-state.md accumulated cross-phase
reflection content. Specific authoring sessions not re-identified at
retro time; the file is append-only and entries span multiple phases
and work-streams. Same fabrication-adjacent class as triage § E (bare
intra-repo paths missing their prefix), not the canonical fabrication
class (wrong-version anchors, made-up files).

Per the LIVE-SOURCE bucket rule defined in triage § B (file path NOT
under `docs/diagnostics/_audits/`, `docs/retro/`, `tools/integrity/docs/`,
`docs/integrity-toolkit-spec.md`, or `tools/integrity/README.md`),
these 5 are LIVE-SOURCE by default and NOT swept in this session.

Final gate state after this landing: 9 hard-fails outstanding (3
Phase 12 LBM + 1 sph-water Akinci2012 + 5 project-state.md accumulated
reflection). All 9 are bare-path discipline-class findings; all 9
would be caught mechanically by A.3 (bare-path-to-upstream-basename)
in batch 2.

### § D.2.1 — Addendum: gate state mooted between direction and landing

The verbatim § D.2 above was authored at retro-direction time when
`python3 -m integrity --mode strict --no-audit-log` reported 9
hard-fails (the 5 project-state.md findings plus the 4 pre-existing
phase 12 LBM and sph-water Akinci2012 findings).

Between direction time and landing time, concurrent commit `47104ad`
(`chore(phase12): cross-cutting edits — CI + README + CHANGELOG +
project-state + capture-format`) modified `project-state.md` and
addressed the 5 bare-path findings cited in § D.2 as collateral
cleanup. The actual gate state at this audit's landing time is **4
hard-fails outstanding** (3 Phase 12 LBM + 1 sph-water Akinci2012).
The "Final gate state" sentence at the end of § D.2 is preserved
verbatim as the at-direction-time snapshot; this addendum is the
authoritative landing-time count.

This mooting is itself banked evidence for the v1.1 retro § 7.1
operating-conditions discussion. Concurrent commits don't just race
the gate — they can moot in-flight audit prose between draft and
land. Specific shape of this instance: a Phase-12 cross-cutting
cleanup commit happened to address findings that an unrelated
session's audit was about to attribute and surface. The Phase-12
author was not aware of the v1.1 retro's pending audit; the v1.1
retro author was not aware of the in-flight Phase-12 cleanup.
Neither party did anything wrong — parallel sessions did parallel
work and one's cleanup intersected the other's reporting.

Recommend banking a sixth operating-condition convention alongside
retro § 7.2 A–E:

    F. Audit-prose freshness check. Audit reports drafted at
       direction time and landed later by an executor should
       verify the gate-state claims against current disk
       immediately before commit. Discrepancies become addenda
       (not paraphrases) to preserve the audit trail of when
       each claim was authored vs landed.

This is a v1.2 candidate; not implemented in this session.

### § D.3 — Banking opportunity strengthened: project-state-snapshot category

The 5 new `project-state.md` findings reinforce a structural classifier
gap that the v1.1 retro § 6.1 already flagged as a v1.2 candidate (item
4): the toolkit doc → audit doc → retro doc bucket pattern covers
every state-snapshot document EXCEPT `project-state.md` at the repo
root, which falls through to LIVE-SOURCE by default despite being
structurally identical (append-only narrative, bare-path citations
in prose, may go stale as code moves).

Architect-2 review item for v1.2: add a classifier rule alongside
the recommended `toolkit-doc-snapshot` extension from triage § C.2.
Concrete shape (architect-2 to confirm):

    if cid == "cat1.intra-repo" and f == "project-state.md":
        return Classification(
            category="project-state-snapshot",
            reason="project-state.md cross-phase reflection snapshot (see grandfather-catalog project-state-snapshot)",
            issue_ref="n/a",
        )

Both the `toolkit-doc-snapshot` and `project-state-snapshot` rules
should land together in batch 2 alongside A.3, since A.3 catches the
underlying bare-path defect class and the classifier extensions
separate "structural-snapshot doc" from "live-source defect" cleanly.

Note: implementing either classifier rule in this session is out of
scope — batch 2 work, architect-2 review first.

## § E. Banked for batch 2

Restating the v1.2 priority order from retro § 6.1, refined with the
post-retro evidence:

1. **A.3 (bare-path-to-upstream-basename)** — catches all 4 of the
   remaining live-source hard-fails on this landing. Also caught the
   5 project-state.md findings that concurrent commit `47104ad`
   resolved by hand. Highest-leverage v1.2 item.
2. **A.2 (toolkit self-application)** — catches own-source
   discipline class. The 2 findings annotated in step 2 are the
   canonical evidence: a v1.1 batch-1 commit landed without
   self-application catching it, and the gap was only surfaced
   post-batch by manual triage.
3. **Pre-spec probe template upgrades** (retro § 7.2 C+D).
4. **`toolkit-doc-snapshot`** classifier extension (triage § C.2)
   and **`project-state-snapshot`** classifier extension (§ D.3
   above) and **`retro-doc-snapshot`** classifier extension
   (analogous gap surfaced by this session's step-1 sweep companion
   — retro intra-repo findings fell through to other-cat1, same
   pattern). Bundle all three into one v1.2 classifier-rules commit
   alongside the catalog rationale entries.
5. **Grandfather-sweep companion convention enforcement** (retro
   § 6.2). Architect-2 picks soft/medium/hard. The session-six
   discovery in § D.2.1 (concurrent cleanup mooting audit prose) is
   additional evidence that the convention pays off across
   independent work-streams, not just within a single session.
6. **Audit-prose freshness check** (new v1.2 candidate banked in
   § D.2.1 above). Architect-2 review item.
7. Lower-priority items per retro § 6.1.

Note that the retro is self-drafted by architect-1 and § 6.4
recommends an architect-2 review pass before its conclusions lock.
This audit report does not pre-suppose that review's outcome — it
records the retro as landed in its as-drafted form. Architect-2's
review pass may add, reorder, or strike items in the priority list
above; the list as-written reflects the architect-1-drafted retro
plus the two new candidates surfaced during this landing session
(retro-doc-snapshot in item 4, audit-prose freshness in item 6).

End of post-retro landing audit report.
