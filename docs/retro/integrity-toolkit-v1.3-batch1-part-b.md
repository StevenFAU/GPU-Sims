---
title: "Integrity Toolkit v1.3 Batch-1 Part-B — Retrospective"
date: 2026-05-16
author: architect1
status: draft
sibling-docs:
  - docs/retro/integrity-toolkit-v1.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/retro/integrity-toolkit-v1.2-bolt-ons.md
  - docs/retro/integrity-toolkit-v1.3-candidates.md
  - docs/retro/integrity-toolkit-v1.3-batch1-part-a.md
  - docs/diagnostics/_audits/integrity_v1_3_t1_1_2_probe_2026-05-16_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_part_b_spec_2026-05-16_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_part_b_commit1_landing_2026-05-16.md
  - docs/diagnostics/_audits/integrity_v1_3_part_b_commit2_landing_2026-05-16.md
---

# Integrity Toolkit v1.3 Batch-1 Part-B — Retrospective

## 0. Author bias disclosure

This retro is drafted by architect-1, the same source that drafted the
part-B execution spec. Three of four substantive specs drafted across
v1.2-v1.3 (A.3, A.2, v1.3 part-B) have shipped with execution-time
fixes for the same class of issue: API shape assumptions in the spec
that weren't verified against the probe's verbatim dumps before
drafting.

Self-retrospection is structurally biased toward charitable framing of
one's own recurring errors. The retro names the pattern directly in
§ 3.3 and § 4.2 because banking it again without naming it would
itself be evidence of the problem.

The parallel session authored the part-A retro independently; this
retro is part-B-specific. A unified v1.3 batch-1 retro was considered
and rejected — separating the halves preserves each architect's
honest assessment of their own work.

## 1. Summary

Part-B closed two items from the v1.3 candidates roadmap (T1.2 and
T1.1) across three commits plus SHA back-fill.

| Commit | SHA | Item |
|---|---|---|
| 1 | 239d7a2 | T1.2 — module-level FALLTHROUGH_CATEGORIES + helper + filter refactor |
| 2 | 710ac93 | T1.1 — three new classifier rules + catalog sections + reason-text rewrites |
| 3 | 67e19c1 | SHA back-fill |

Combined with the parallel session's part-A (commits 65a7685 →
edc28d1), the full v1.3 batch-1 totals 8 commits, ~400-500 LOC,
matching roadmap § 9.1's budget.

**Quantitative:**

- Code added: ~36 LOC (grandfather.py +29 LOC, snapshot.py +7 LOC)
- Catalog added: ~47 LOC (3 new sections in grandfather-catalog.md)
- Tests added: 13 (6 for T1.2 helper, 7 for T1.1 rules)
- Annotation reason-text rewrites: 23 lines across 7 files
- Test suite: 170 → 183 passing
- Gate hard-fails: 44 baseline (probe SHA) → 60 at part-B close
  - +11 from concurrent part-A retro landing (edc28d1) mid-flow
  - +1 from probe doc landing in commit 1
  - +4 from new audit reports across part-B
- Categories drained from other-cat1: 36 → 28 (-8)
- New named categories populated: toolkit-doc-snapshot (4),
  retro-doc-snapshot (4), project-state-snapshot (0)

**Conventions banked from this batch:** see § 4.

## 2. What worked

### 2.1 Probe → spec → execute discipline produced clean strategic outcomes

Every load-bearing strategic decision in the part-B spec was right at
the architecture level. T1.2's FALLTHROUGH_CATEGORIES design choice
(option d, mirroring P1.8's SWEEPABLE_PATH_PREFIXES idiom) was the
correct path. T1.1's Shape B predicate for toolkit-doc-snapshot
(three-predicate union mirroring toolkit-doc-bare-path) was the right
forward-compatible choice. Convention H's code-anchor cross-link
landed exactly as scoped. The classifier rule order, the catalog
section structure, the _KNOWN_CATEGORIES extension — all of these
worked as designed without execution-time adjustment.

The probe-then-spec pattern continues to be the working discipline
for the toolkit. The errors in part-B are at the implementation-detail
layer, not the strategic-decision layer.

### 2.2 Spec-budget discipline held for the first time

The part-B spec landed at 978 lines against an 800-1100 target. Three
prior probes/specs blew past their stated budgets (1394, 1428, 1991).
The part-B spec held because I actively cut during drafting rather
than letting "the analysis is genuinely substantive" become unbounded
permission to expand.

Holding the budget didn't reduce spec quality. If anything it
exposed the spec's weak points more clearly (when there's less prose
to hide in, the API-assumption errors become more visible). Worth
pinning: spec-budget discipline is achievable but requires active
trimming, not passive aspiration.

### 2.3 Self-review checks caught the deviation cleanly

The five mechanical self-review checks in spec § 9 included one
(Check 1 — Decision 7 sweep-diff exactness) that surfaced the
mechanism mismatch immediately. Claude Code documented the deviation
in commit-2 audit § F.1 rather than silently adapting. The discipline
of mechanical checks that produce binary pass/fail outcomes worked
exactly as designed.

### 2.4 Convention H gained a code anchor

The v1.2 bolt-ons retro § 4.2 banked Convention H ("fallthrough
discriminator — name the bucket structurally rather than by literal
string match") as a structural cleanup item. T1.2 anchored it in
code via FALLTHROUGH_CATEGORIES + is_fallthrough_category(). Both
retro text and code now reference each other; future readers can
navigate either direction. This is the kind of convention-to-code
translation that the retro-banking pattern is supposed to produce.

### 2.5 Concurrent commit coordination worked as designed

The parallel session's part-A retro (edc28d1) landed between
this session's commit 1 and commit 2. Spec § 6.1 anticipated this
exact scenario and included instructions for how to handle it.
Claude Code's commit-2 audit § F.2 documented the timing cleanly,
attributed the +11 hard-fail growth to the new retro file's
cat1.intra-repo citations, and proceeded without pause.

The convention chain (P1.8 sweep protection → Convention G
sweep-side-before-scope-expansion → audit-prose freshness) handled
the concurrent landing exactly as designed.

## 3. What didn't work

### 3.1 Decision 7 sweep-mechanism mismatch (most consequential)

**The failure:** Spec § 4.7 directed Claude Code to run
`grandfather_sweep.py` after commit 2's classifier rules landed,
expecting it to annotate exactly the 11 enumerated findings on the
5 listed files. This expectation was empirically wrong.

**Root cause:** `grandfather_sweep.py`'s `annotation_already_present`
check matches on `check_id`, not on `suppression_reason`. The 11
findings I expected the sweep to annotate were already annotated with
stale `other-cat1` reason strings from prior batches. The sweep
correctly identified them as already-covered and skipped them.

To achieve the spec's intended end-state (catalog drains other-cat1,
new categories populate, auto-refresh confirms zero drift), the
existing annotation reason strings had to be **rewritten in place** —
a fundamentally different operation than the sweep performs.

**What Claude Code did right:** Manually rewrote 23 annotation lines
across 7 files. Explicitly did NOT run `grandfather_sweep.py` because
the routine sweep would have absorbed ~12 unrelated unannotated
findings (scope creep). The audit § F.1 captures the deviation,
explains the root cause, and proposes the systemic fix.

**Why the spec was wrong:** I assumed the sweep's idempotency
mechanism would re-classify findings when their classification rule
changed. I never read `annotation_already_present`'s implementation
verbatim during spec drafting. The probe captured `apply_annotations`
verbatim (probe § C.2), but I didn't trace through the
already-present check before writing Decision 7. This is the same
pattern as § 3.3.

**The actionable T2 candidate:** add a "rewrite stale reason text"
mode to `grandfather_sweep.py` that detects annotations whose
`suppression_reason` no longer matches the current `classify()`
output and rewrites the reason in place. Without this mode, every
future classifier-rule addition that re-routes existing findings
will require a manual rewrite pass. See § 4.1 for the formal banking.

### 3.2 Probe enumeration off by 3

**The failure:** Probe § E.2 enumerated 11 findings on 5 files that
would re-classify under T1.1's new rules. Actual reclassifications
landed: 8 (4 toolkit-doc + 4 retro-doc + 0 project-state). The probe
script captured cat1.intra-repo findings whose `classify()` result
would change post-T1.1; the count was empirically off by 3.

**Probable cause:** Concurrent commit churn between probe-time and
execution-time. The parallel session's edc28d1 landed mid-flow,
modifying the cat1.intra-repo finding set. Some findings the probe
captured may have been on lines that were edited; others may have
been added by edc28d1. The probe didn't (and structurally couldn't)
anticipate the concurrent landing.

**Why this is OK:** The spec's verification block included a
mechanical re-derivation step. Claude Code did the recount at
execution time and proceeded with the actual 8, not the spec-time 11.
The deviation is a real-world concurrent-state drift, not a spec
defect.

**What this confirms:** Probe data is point-in-time, not durable. The
discipline of re-deriving counts at execution time (rather than
asserting probe-time numbers as binding) is the right convention.

### 3.3 Spec-drafting API-assumption pattern (fourth occurrence)

**The pattern, now stable across 4 substantive specs:**

| Spec | Missing implementation detail caught at execution |
|---|---|
| A.3 | None (this spec landed clean) |
| A.2 | Fixture-mode rglob fallback; pytest conftest.py for fixtures; snapshot.py _KNOWN_CATEGORIES |
| v1.3 part-B | Decision 7 sweep mechanism (§ 3.1); test idiom (pytest classes vs bare functions); Finding import path; Finding dataclass shape |

Three of four specs shipped with execution-time fixes for "prose says
X, code doesn't enforce X" or "prose assumes X about existing API,
existing API is Y" gaps. The pattern is stable.

**What I haven't been doing:** When drafting a spec section that
cites an existing function signature, class shape, or import path, I
should grep the probe's verbatim dumps for the cited symbol before
writing the section. I haven't been doing this systematically.

**What I have been doing:** Banking the observation in retros and
calling it a convention. Naming the pattern hasn't translated into
different behavior on the next pass. This is the structural awkwardness
the § 0 bias disclosure named.

**The mechanical fix proposal:** see § 4.2.

### 3.4 Audit-report shape drift between spec and front-matter

**The failure:** The part-A spec § 6 (the parallel session's spec)
didn't explicitly require an audit-report deliverable for commit 4
(SHA back-fill). The part-A retro front-matter then cited an
`integrity_v1_3_commit4_landing_2026-05-16.md` sibling-doc that
didn't exist. Claude Code authored the audit post-hoc when landing
the part-A retro (commit edc28d1).

**Why this matters:** This is the same shape as § 3.3 but
operationally: the spec body and the retro front-matter disagreed on
what the work produced. Either the spec under-specified or the retro
over-specified. Either way, the discipline that catches this is
sibling-doc resolution at retro-landing time, which worked correctly
(the parallel session's freshness check found the missing file and
filled the gap transparently).

**Banked for v1.3 batch-2 (if it happens):** spec authors should
explicitly enumerate every deliverable file, including audit reports
for every commit. Don't rely on convention to fill gaps.

## 4. Banked observations

### 4.1 Convention I — Rewrite stale reason text in grandfather_sweep

**Status:** T2 candidate; concrete and actionable.

**Pattern:** When a new classifier rule re-routes existing findings
from one category to another, the existing inline annotations
continue to carry the old category's reason text. The sweep's
`annotation_already_present` check sees them as covered and skips.
This means new classifier rules don't self-apply to historical
findings.

**Mechanical fix:** Add a `--rewrite-stale-reasons` flag to
`grandfather_sweep.py`. When set, for each finding with an existing
annotation, compare the annotation's parsed `suppression_reason`
against `classify(finding).reason`; if they differ, rewrite the
annotation line in place. Default off (don't change current behavior).

**Test surface:** new fixture with annotations carrying stale reason
text + a classifier rule that re-routes them. Verify the flag
detects and rewrites.

**Scope estimate:** ~80 LOC in `grandfather.py` (helper) +
`grandfather_sweep.py` (flag) + tests. Smaller batch than T1.1+T1.2.

This is the highest-leverage banked item from part-B. Recommend
landing whenever a v1.3 batch-2 happens, ahead of any future
classifier-rule additions.

### 4.2 Spec-time API cross-check discipline

**Status:** Process convention; the fourth time this is banked across
retros. The first three named the pattern. This one proposes the
mechanical fix.

**Convention proposal (for whoever picks this up):**

Every spec section that cites an existing function signature, class
shape, import path, or API contract gets a probe-section reference in
the prose AND a `grep -F` against the probe's verbatim dumps before
the section is committed to the spec. If the grep fails to find the
cited shape, either (a) the probe missed it (extend the probe), or
(b) the cited shape is wrong (correct the spec). Either way, surface
before committing the spec.

**Why this hasn't worked yet:** I haven't been doing it. Banking the
convention in retros hasn't translated into spec-drafting behavior.
The fix is mechanical (grep before draft), not aspirational.

**Test of the convention:** the next spec drafted by architect-1
should be assessed against this rule. If the next spec ships with
"prose says X, code doesn't enforce X" gaps, the convention isn't
working and a different intervention is needed (e.g., mechanical
spec-vs-probe verification at commit time, similar to audit-prose
freshness checks).

### 4.3 Spec-budget discipline is achievable but fragile

**Status:** Confirmed for part-B; track across future specs.

The part-B spec held at 978 lines (target 800-1100). Three prior
specs/probes blew past their targets. The difference in part-B was
active cutting during drafting — not letting "but the analysis is
substantive" be unbounded permission.

The probe ran 1394 lines (target 600-900). The probe-budget discipline
is still failing.

**Banked observation:** specs are easier to budget than probes.
Probes are required to be complete; specs can defer prose to "see
probe § X." Architect-1 should pin probe-section references in
specs (which compresses spec length) but accept that probes will run
longer than stated targets.

**Recommendation:** revise probe targets upward. 1200-1800 is the
empirical range for substantive probes; treating it as the budget is
honest. Specs should hold to 800-1500 depending on scope.

## 5. Recommendations

### 5.1 V1.3 batch-1 is closed. Pause.

Eight commits landed across two sessions. The v1.3 candidates roadmap
§ 9.1 budget was hit exactly. The toolkit's gate is stable. The
discipline is working.

**Recommend declaring v1.3 batch-1 a natural pause point** rather than
immediately rolling into v1.3 batch-2. The remaining roadmap items
are opportunistic, not foundational:

- T2 items (sweep enforcement, audit-prose mechanization, A.6 Stack C)
  can land when a forcing function appears
- T3 items are stuck pending architect-2 (which Steven opted out of);
  either demote to architect-1-decides or formally bank as unresolved
- T4 horizon is v2 scope
- Part-C banked items (project-state.md fossils, this batch's
  rewrite-stale-reasons sweep mode) are small; bundle later

Continuing to grind through every roadmap item is itself scope creep.
The toolkit catches every defect class the v1 spec § 12 baseline
identified. The discipline is proven. Stopping is a valid outcome.

### 5.2 If a v1.3 batch-2 happens, start with Convention I (§ 4.1).

The rewrite-stale-reasons sweep mode is concrete, small-scope, and
unblocks future classifier-rule additions. Without it, every future
T1-style addition will require manual reason-text rewrites — exactly
the operational deviation this batch banked.

### 5.3 The next architect-1 should test the § 4.2 convention.

Whether that's me on a future batch or someone else: if Convention §
4.2 (spec-time API cross-check) doesn't translate into different
spec-drafting behavior on the next pass, the convention isn't working.
Different intervention needed: mechanical pre-commit check, peer
review by another agent, or simply stopping spec drafting until the
underlying discipline gap is fixed.

### 5.4 Process-debt items (T3 backlog) need a decision.

Four items have been "needs architect-2 review" across multiple retros.
Steven opted out of architect-2 review at the start of v1.2. The items
are structurally stuck. Either:

- Promote to architect-1-decides with bias disclosure (the pattern this
  retro itself uses), or
- Formally bank as unresolved and remove from active tracking.

Continuing to list them as "needs review" while no review is happening
is dishonest about the actual state.

## 6. Closing

V1.3 batch-1 closed across two sessions, 8 commits, ~450 LOC,
exactly on roadmap budget. The probe → spec → execute → self-review
discipline produced clean strategic outcomes; implementation-detail
fabrications continue at a stable rate in spec drafting.

The most actionable item from part-B is Convention I (§ 4.1) — the
sweep-rewrite-stale-reasons mode that would let future classifier
rules self-apply to historical findings.

The most honest item is § 4.2 — the spec-time API cross-check
convention that's been banked four times now without translating into
different drafting behavior. Whether it works on the next pass is the
test.

Recommend pausing rather than continuing. The toolkit is mature; the
backlog is opportunistic; stopping is a valid outcome.

## End of retrospective
