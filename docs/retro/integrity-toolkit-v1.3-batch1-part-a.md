---
title: "Integrity Toolkit v1.3 Batch-1 Part-A — Retrospective"
date: 2026-05-16
author: architect1
status: draft
sibling-docs:
  - docs/retro/integrity-toolkit-v1.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/retro/integrity-toolkit-v1.2-bolt-ons.md
  - docs/retro/integrity-toolkit-v1.3-candidates.md
  - docs/diagnostics/_audits/integrity_v1_3_t1_3_5_probe_2026-05-16_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_t1_3_5_spec_2026-05-16_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_commit1_landing_2026-05-16.md
  - docs/diagnostics/_audits/integrity_v1_3_commit2_landing_2026-05-16.md
  - docs/diagnostics/_audits/integrity_v1_3_commit3_landing_2026-05-16.md
  - docs/diagnostics/_audits/integrity_v1_3_commit4_landing_2026-05-16.md
---

# Integrity Toolkit v1.3 Batch-1 Part-A — Retrospective

## 0. Source bias disclosure (compounding)

This retro is self-drafted by architect-1, the same source that
authored the v1.3 batch-1 part-A execution spec at
`docs/diagnostics/_audits/integrity_v1_3_t1_3_5_spec_2026-05-16_architect1.md`.
The structural bias named in v1.1 batch-1 retro § 0 and v1.2
bolt-ons retro § 0 applies again.

**Compounding bias concern.** This is now the third architect-1
retro of architect-1 work in a row (v1.2 bolt-ons → v1.3 candidates
roadmap → this batch's retro). Same single source has authored every
banking document in the v1.2 → v1.3 cycle. Architect-2 review remains
open across the v1.1 batch-1 retro (per its § 6.4), the v1.2 bolt-ons
retro, and the v1.3 candidates roadmap. The backlog is growing.

Specific compounding-bias surface in this retro:

- § 3.1 surfaces two architect-1 drafting errors in the v1.3 batch-1
  part-A spec. Those errors are being self-reported by the source
  that introduced them. The framing is unavoidably charitable to
  architect-1's drafting workflow ("two failures in one spec is
  data") because the alternative ("architect-1 doesn't re-read what
  they just wrote") is harder to write about honestly without
  defensive framing creeping in. Architect-2 review of this section
  specifically would help.
- § 4's two new conventions (J, K) are banked by the same source that
  banked G, H, I in the bolt-ons retro. The convention-numbering
  pile is growing without architect-2 sign-off on any item past F.

**Mitigations in effect.**

- **Inventory claims grep-verified.** End-state SHAs, test counts,
  and gate state come from the Claude Code execution session's
  final report. The five banked observations from execution are
  cited directly with their audit-report § locations.
- **No self-review probe.** This retro is smaller than v1.2 bolt-ons
  retro (three substantive commits, no in-batch fabrications).
  Per the bolt-ons retro § 0 mitigation argument, a full probe is
  overkill at this scope.
- **The compounding bias itself is named.** This retro flags that
  architect-2 review is owed across multiple documents now, not
  just this one. The accumulating backlog is the load-bearing
  concern; mechanically clearing it is architect-2 scheduling work.

## 1. Batch summary

### 1.1 Scope

Three small-scope items from the v1.3 candidates roadmap § 4 T1, plus
SHA back-fill:

- **T1.3** — Catalog auto-refresh script. New
  `tools/integrity/scripts/refresh_catalog_counts.py` reads
  `--grandfather-report` output and refreshes the numeric
  parentheticals in `grandfather-catalog.md` H3 headings. Preserves
  non-numeric parentheticals (placeholders, free-prose). Idempotent.
- **T1.5** — Cat3 expected-values TOML → JSON convergence. Removed
  `expected_values.toml`; added `expected_values.json` matching the
  d3q19 schema. Updated `cubic_kernel.py`, `generate_expected.py`,
  and the test docstrings. New `tools/integrity/docs/cat3-conventions.md`
  recording the JSON-as-canonical convention.
- **T1.4** — Probe template conventions doc. New
  `tools/integrity/docs/probe-template-conventions.md` records
  Convention C (path-resolution enumeration) and Convention D (call-
  site enumeration) from v1.1 batch-1 retro § 7.2, with six worked
  examples drawn from existing audit/retro reports.

### 1.2 Landing summary

| # | Item | SHA | Description |
|---|---|---|---|
| 1 | T1.3 | `65a7685` | catalog auto-refresh script + 17 tests |
| 2 | T1.5 | `72a2d26` | cat3 TOML → JSON convergence + conventions doc |
| 3 | T1.4 | `9e3afa9` | probe-template conventions doc |
| 4 | SHA back-fill | `1f7785f` | cross-references across commit-1-3 audits |

Four commits. ~390 LOC across new + modified + removed. Test suite
grew from 153 to 170 (+17 tests, all in T1.3). End-state gate: 5
pass / 0 soft-warn / 44 hard-fail / 1262 suppressed. Live-source-
skipped (other-cat1): 39 (unchanged from pre-batch). Net hard-fail
movement: 45 → 44 (one prior audit-doc finding got cleaned up by
commit 1's inline sweep companion as collateral cleanup).

### 1.3 Concurrent A.2 landing

A.2 (toolkit self-application) continued landing commits in parallel
during this batch's execution. By batch end, A.2 had landed at least
through commit 4 (per Observation 5: A.2 commit 4's force-sweep
populated the `toolkit-own-unused (?)` heading placeholder with a
numeric `(24)` count mid-batch). Both batches landed cleanly without
integration conflicts; one cross-batch interaction observed (§ 3.4).

## 2. What worked

### 2.1 Probe-then-spec-then-hand-off discipline held

The cycle established in v1.2 (bolt-ons) and refined in v1.3
(candidates roadmap → spec) ran cleanly:

- The probe captured 1234 lines of verbatim grep-anchored content
  including § B.6's empirical drift table, § B.7's six design
  decisions for T1.3, and § C.5 / § C.6's six worked examples for
  T1.4.
- The spec re-grounded every line-number citation against probe
  output rather than against architect-1 prior assumption. Zero
  spec-time fabrications surfaced at execution.
- The Claude Code hand-off prompt enumerated four anticipated
  friction points with documented resolution paths. All four fired
  during execution and were resolved per the documented paths. Zero
  pause-and-surface events required.

The empirical confirmation: friction-prediction at hand-off time is
high-leverage. The hand-off prompt added perhaps 80 lines beyond what
the spec already contained, and shifted four predictable failure
modes from "executor surprised at runtime" to "executor follows the
documented path." Worth banking as Convention K (§ 4.2).

### 2.2 Convention H validation (rules-over-enumerations, category axis)

Per Observation 5: A.2 commit 4's force-sweep populated the
`toolkit-own-unused (?)` placeholder with `(24)` mid-batch. T1.3's
parser (probe § B.7 (1)) was designed to query the property "is the
parenthetical a non-negative integer?" rather than the literal value
`(?)`. The mid-batch swap from `?` to `24` did not require any T1.3
code or test changes — the property-query naturally absorbed the
transition.

This is positive evidence for Convention H from the v1.2 bolt-ons
retro § 4.2 ("filter rules query properties, not literals"). The
category-axis blind spot identified in bolt-ons retro § 3.2 (where
P1.8's filter literal-matched `other-cat1` and missed A.3's new
`other-cat1-bare-path`) was the failure case for the same principle;
this is the success case.

### 2.3 Cross-batch independence (Convention I validation)

T1.3 reads the catalog; A.2 commit 4 writes to the catalog. They
landed in interleaved order without conflict because:

- T1.3's tests use synthetic fixture strings, not the live catalog.
  Test coverage of the placeholder form (`?`) is preserved even after
  the live catalog moved past it.
- T1.3's runtime parser handles both the placeholder form and the
  numeric form via the same `is_numeric` property check.
- T1.3's script and A.2's sweep both honor Convention B (sweep
  companion within the commit). Neither stomped the other.

Convention I's "cross-batch scope discipline" from bolt-ons retro
§ 4.3 — defer out-of-scope work rather than opportunistically
sweeping it — held cleanly across this entire pair of overlapping
batches.

### 2.4 Stage 0 anchor verification

The hand-off prompt's Stage 0 ("verify HEAD against probe SHA before
any commits, pause-and-surface on disagreement") caught the
expected drift: probe ended at `df21312`; HEAD by execution start
had moved as A.2's later commits landed. The verification passed
because none of A.2's edits touched this batch's surface, but the
mechanism worked as designed — the executor didn't blindly assume
disk matched probe.

## 3. Surprises and what they revealed

### 3.1 Two architect-1 drafting errors in one spec (negative)

Observations 1 and 4 from the Claude Code execution report are both
architect-1 spec-time self-consistency failures. They have the same
root cause and are worth surfacing together.

**Error 1 (test count mismatch).** Spec § 1.2 / § 2 / § 7 claimed
"+12 tests." Spec § 3.3 body defined 17 explicit tests. The +17
landed. Convention F (audit-prose freshness) failure during drafting:
I designed 12 tests initially, then expanded to 17 (added the
`test_report_line_regex_*` trio, two parser-detail tests, and the
integration test) without updating the upstream count references in
the table at § 2 or the summary at § 7.

**Error 2 (validation rule vs. doc body contradiction).** Spec § 5.3
included a verification check `content.count('### Examples') == 0`
intended to guard against orphan example headers in the T1.4 doc.
Spec § 5.2 defined the doc body with `### Examples of Convention C
followed` and `### Examples of Convention D followed` subheadings.
Direct self-contradiction: § 5.3 forbade what § 5.2 required. Caught
at execution time; resolution was to relax the validation (doc body
authoritative). Recorded in commit-3 audit § E.1.

**Root cause (both errors).** Drafting downstream sections without
re-reading upstream sections within the same spec. Convention F as
banked focuses on draft-vs-disk freshness; the analog gap is
draft-vs-self-draft consistency. Two same-class failures in one
spec is data, not coincidence.

**Mitigation (architect-1 workflow).** A pre-handoff pass that greps
the spec for cross-references and verifies they agree internally.
Specifically:
- Numeric counts referenced in summary tables / overview prose / end-
  state-verification should match the count actually defined in the
  spec body.
- Validation/grep/lint rules in verification blocks should be
  cross-checked against the content they validate.
- Any "expected: N matches" style assertion in a verification block
  should grep the spec body for the matching content and confirm N.

This is architect-1 workflow, not a toolkit convention. Banked in
§ 5.1 as a workflow note rather than promoted to Convention status.

### 3.2 Anchor-sketch labeling (negative, recoverable)

Observation 2 from execution: spec § 4.2.1 presented the
`expected_values.json` content with 15-significant-figure literals
and integer-zero values (`0`, not `0.0`). The TOML source had those
values exactly that way. The generator's `json.dumps(cubic_W(0.0,
1.0))` output `0.0` (the natural Python float repr) and ~15-sig-fig
literals at full precision. The commit-2 generator round-trip check
flagged the diff.

The hand-off prompt anticipated this and named the resolution path:
"prefer updating § 4.2.1: change the four `0` literals at q=0 and
q=1 to `0.0`, re-run the generator, and confirm zero diff." Resolved
cleanly per documented path. The executor adopted the generator's
output as authoritative; committed JSON matches generator byte-for-
byte.

**Honest framing.** I knew the integer `0` was risky when I drafted
§ 4.2.1. I wrote it that way to match the TOML byte-for-byte rather
than predict what the generator would emit. That was a deliberate
choice with a known failure path, addressed by bolting on a
pause-and-surface clause to § 4.3. The cleaner move would have been
to label § 4.2.1 as an "anchor sketch — verify against generator
output at execution time" rather than presenting it as canonical
content.

This generalizes. Any spec section that constructs content from
probe data plus architect-1 inference (rather than from verbatim
verified content on disk) should be labeled explicitly. Banked as
Convention K (§ 4.2 below).

### 3.3 Convention B sweep-companion mechanics (neutral, mechanical)

Observation 3 from execution: commit 1 landed multiple new doc
files (the probe report copied into place by Stage 0; the spec file;
the commit-1 audit report). The inline sweep companion in commit 1's
verification block swept the staged set but only added one
annotation. Twelve unswept findings carried into commit 2's pre-
commit gate. Commit 2's sweep picked up the remainder.

The mechanic: `grandfather_sweep.py` operates on findings, which
come from cat1-scannable files on disk. The sweep at commit-1 time
saw the staged-but-not-yet-committed files as having pre-existing
findings (some of which already had annotations from prior batches
and didn't need new annotations). After commit 1 landed, commit 2's
sweep saw the same files now-committed-with-new-content and picked
up the remaining findings.

**Significance.** Not a defect — the sweep mechanic is working as
designed. But the implication for spec authors: when a commit lands
multiple new doc files together (probe + spec + audit report in a
single commit), the per-commit sweep may not catch every finding in
one pass. Subsequent commits' sweep companions inherit the residual.

Banked as Convention J (§ 4.1) — a clarification of Convention B
rather than a new convention. Sweep companions should be expected to
operate across commit boundaries when multi-file commits land
together.

### 3.4 Mid-batch cross-batch interaction (positive)

Observation 5: A.2 commit 4 force-swept the catalog to populate the
`toolkit-own-unused (?)` placeholder with `(24)` mid-batch. Discussed
in § 2.2 as Convention H validation. Worth surfacing here too as a
case where parallel-architect coordination worked without explicit
coordination — both batches' independent designs composed correctly
because each was rule-based on the property axis they cared about.

This is exactly the kind of interaction the v1.2 bolt-ons retro § 4.3
Convention I anticipated: "defer to the responsible batch's own
sweep companion." A.2 owned its own force-sweep; this batch didn't
opportunistically include the catalog refresh of `toolkit-own-unused`.
T1.3 inherited a numeric value cleanly.

## 4. Banked conventions (additions to retro § 7.2)

Inheriting conventions A–I from the v1.1 batch-1 retro and v1.2
bolt-ons retro. Adding J, K.

### 4.1 Convention J — Sweep companions operate across commit boundaries when multi-file commits land

> **J.** A grandfather-sweep companion within a single commit
> operates on the cat1-scannable surface as it exists at sweep-run
> time. When a commit lands multiple new audit-doc files in a single
> push (e.g., probe + spec + audit report), the sweep may not catch
> every finding in one pass; residual findings carry into the next
> commit's pre-commit gate and are swept by that commit's sweep
> companion.
>
> This is not a defect; it's a mechanical consequence of how the
> sweep operates on findings. The implication for spec authors:
> expect a transient gate spike at the boundary between multi-file
> commits, with the spike collapsing under the next sweep companion.
> Convention B's "sweep companion within the commit" requirement
> still holds; this is a clarification of the per-commit sweep's
> scope, not an exception.
>
> See v1.3 batch-1 part-A § 3.3 for the case where commit 1 landed
> probe + spec + audit report and 12 unswept findings carried into
> commit 2.

### 4.2 Convention K — Anchor-sketch labeling for spec content from inference

> **K.** When a spec section constructs content from probe data plus
> architect-1 inference (rather than from verbatim verified content
> on disk), label the section explicitly as an "anchor sketch — verify
> at execution time" rather than presenting it as canonical. Anchor
> sketches name a likely failure mode upfront; the verification block
> for the same section should include a check that confirms the
> sketch against execution-time disk.
>
> The failure mode this prevents: an executor reading canonical-
> looking content trusts it without re-deriving, and disagreement
> between sketch and reality surfaces only at the next verification
> step (or worse, after a commit lands with subtly-wrong content).
>
> See v1.3 batch-1 part-A § 3.2 for the case where spec § 4.2.1
> presented JSON content from inferred TOML translation rather than
> from generator output; the integer-vs-float and precision-digit
> mismatches surfaced at the commit-2 generator round-trip check.
> Labeling the section as an anchor sketch would have flagged this
> at hand-off rather than at execution.

## 5. Banked observations

Small items from execution that don't warrant convention status but
are worth pinning:

### 5.1 Architect-1 spec-time self-consistency check (workflow note)

Per § 3.1: two same-class architect-1 drafting failures in one spec
(test count mismatch in three places, validation rule contradicting
doc body) is data. The mitigation is a pre-handoff pass that greps
the spec for cross-references and verifies internal agreement:

- Numeric counts cited in tables / summaries / verification blocks
  should match the count actually defined in the spec body. Grep
  for `\+\d+ tests` style mentions and reconcile.
- Validation/grep/lint rules in verification blocks should be
  cross-checked against the content they validate. Specifically, any
  `content.count('X') == N` style assertion should grep the spec for
  X and confirm N.
- Any "expected: N matches" or "should report N" assertion in a
  verification block should grep the spec body for the matching
  content and confirm.

This is architect-1 workflow, not a toolkit convention. It's specific
to the failure pattern an architect-1 source produces; another source
might have different drafting failure modes. Worth keeping as a
checklist item for future spec drafts by this source.

Banked as v1.3 candidate (architect-1 workflow refinement). Not
promoted to a numbered convention because it doesn't generalize
beyond the drafting source.

### 5.2 Convention numbering taxonomy still open

We are now at conventions A through K (11 entries). Mix of:

- Spec-time discipline: C, D, K
- Execution-time discipline: A, F
- Batch-coordination: G, I, J
- Design taste: B, E, H

Roadmap T3.3 banked the convention-numbering-taxonomy question. With
two more entries added in this retro, the case for restructuring
into a taxonomy (rather than continuing alphabetic numbering) has
strengthened. Still an architect-2 review item.

### 5.3 Architect-2 review backlog has grown

Three retros now pending architect-2 review:

1. v1.1 batch-1 retro (per its § 6.4) — items: root-cause framing,
   recursive blind spot, priority ordering, enforcement level
2. v1.2 bolt-ons retro — items: Convention H structural follow-
   through scope, convention numbering taxonomy
3. This retro — items: § 3.1 honest self-assessment framing,
   conventions J and K, the compounding-bias concern from § 0

Plus the v1.3 candidates roadmap T3 items (A.9 exclusion,
conventions home, convention numbering taxonomy, the v1.1 § 6.4
backlog itself).

The backlog is real and growing. § 6 question 2 surfaces a possible
mitigation.

### 5.4 Generator-output authoritative for cat3 expected-values

Per Observation 2 / § 3.2: the resolution path was "generator output
authoritative; committed JSON matches generator byte-for-byte." This
is the right invariant going forward for any cat3 expected-values
file. The cat3-conventions doc landed in this batch (T1.5)
implicitly encodes this — the schema includes a `source` field
pointing at the generator script — but does not state the invariant
explicitly. Worth a one-line addition to cat3-conventions.md in a
future cat3 batch: "The committed expected-values file MUST match
the generator's output byte-for-byte; the generator is the source of
truth."

Banked for a future cat3 micro-batch.

## 6. Open questions

1. **Is the v1.3 batch-1 part-B (T1.1 + T1.2) ready to draft after
   A.2 closes?** Roadmap § 9.1 sequenced both T1.1 (three classifier
   rules) and T1.2 (`Classification.is_fallthrough`) as Part-B work
   waiting on A.2 to fully land. A.2's commit-4 landing demonstrated
   it had progressed past the core surface that overlapped with T1.1
   / T1.2. If A.2 is fully closed, Part-B can probably draft. If
   A.2 has further commits planned, defer.

2. **Should the architect-2 review backlog have a "minimum viable
   review" mode given it's growing?** Three retros + the candidates
   roadmap + several T3 items in roadmap § 5 = substantial review
   surface for architect-2. One possible pattern: architect-2 reviews
   the most recent retro plus the open-questions sections of all
   prior retros, rather than full re-read. Cheaper, lower-leverage.
   Alternative: architect-2 reviews only when a v1.x release lands
   (i.e., review the cumulative state once per release rather than
   per-retro). Either pattern is defensible; status quo (full review
   of every retro) doesn't scale at current cadence.

3. **Is Convention F's "audit-prose freshness" framing wide enough?**
   The convention as banked focuses on draft-vs-disk freshness (the
   audit prose drifts from the synced repo state). § 3.1 surfaces a
   variant: draft-vs-self-draft consistency, where two sections of
   the same document disagree because the author didn't re-read
   upstream when drafting downstream. Same root pattern ("write
   without re-checking"), different freshness axis. Possibly worth
   widening Convention F's framing rather than introducing a new
   convention.

## 7. Closing

Three substantive commits, one back-fill, +17 tests, two architect-1
spec-time drafting errors caught at execution time and resolved per
documented paths, zero pause-and-surface events. Both new
conventions (J, K) extend the v1.1 + v1.2 banked list. The
property-query design from Convention H got positive validation in
the cross-batch interaction with A.2 commit 4.

**The compounding self-review bias concern from § 0 is the load-
bearing item.** Architect-1 has now self-drafted four documents in
sequence (v1.2 bolt-ons retro, v1.3 candidates roadmap, v1.3 batch-1
part-A spec, this retro). Architect-2 review remains the structural
backstop against architect-1 drift; the backlog is real. § 6 question
2 proposes a possible compression of the review workload that would
help close the backlog without requiring full re-read of every prior
document.

This batch closes here. Next batch is v1.3 batch-1 part-B (T1.1 +
T1.2) pending A.2 closure confirmation, OR a different prioritization
that an architect-2 review of this retro plus the roadmap could
inform.

Architect-1 retro of own work. Architect-2 review pass remains owed
across the v1.1 batch-1, v1.2 bolt-ons, and this retro; this batch's
items can be reviewed alongside whatever pattern § 6 question 2
settles on.

## End of retro
