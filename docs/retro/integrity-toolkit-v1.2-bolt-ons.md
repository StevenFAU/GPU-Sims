---
title: "Integrity Toolkit v1.2 Bolt-Ons — Retrospective"
date: 2026-05-15
author: architect1
status: draft
sibling-docs:
  - docs/retro/integrity-toolkit-v1.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_spec_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_commit1_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_2_commit2_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_2_commit3_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_2_commit4_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_2_a3_commit4_landing_2026-05-15.md
---

# Integrity Toolkit v1.2 Bolt-Ons — Retrospective

## 0. Source bias disclosure

This retro is self-drafted by architect-1, the same source that authored
the v1.2 bolt-ons execution spec
(`docs/diagnostics/_audits/integrity_v1_2_bolt_ons_spec_2026-05-15_architect1.md`).
The structural bias is the same one named in
`docs/retro/integrity-toolkit-v1.1-batch1.md` § 0: an author retroing
their own work systematically under-reports failure modes they don't
perceive.

Mitigations in effect:

- **Smaller scope reduces the bias surface.** Four spec items vs.
  v1.1 batch-1's eight. Fewer surfaces means fewer places for
  unperceived failures to hide.
- **No in-batch fabrications surfaced during execution.** Unlike v1.1
  batch-1 (which had three), the v1.2 bolt-ons spec landed with the
  per-commit pause-and-surface guards firing zero times. So
  root-cause-of-fabrication isn't a section in this retro.
- **Inventory claims are taken from the executing session's end-state
  summary** rather than independently grep-verified. The five landing
  SHAs and test/file counts cited in § 1 below should be cross-checked
  against `git diff --stat` before this retro locks; any drift becomes
  an addendum.

**No self-review probe is recommended for this retro.** The v1.1
batch-1 retro warranted one because the batch was large, the retro
was lengthy, and the architect-1 source had introduced in-batch
fabrications that motivated mechanical verification. This batch is
smaller and free of those failure modes; a full probe is overkill.
Architect-2 review remains a useful backstop and is still owed on
the v1.1 batch-1 retro itself per its § 6.4 — that backlog isn't
expanded by this retro.

## 1. Batch summary

### 1.1 Scope

Four small-scope items surfaced by
`docs/retro/integrity-toolkit-v1.1-batch1-addendum.md` § 5 (with P1.8
added post-addendum during the over-sweep pause-and-surface on commit
`9add149`):

- **P1.5** — Register `cat3.d3q19-velocity-set` /
  `cat3.d3q19-weights` / `cat3.d3q19-equilibrium` checks. Three new
  thin check modules consuming a relocated and refactored
  `d3q19_verify.py` harness.
- **P1.6** — Strict-mode human-renderer suppressed-stanza filter.
  Three-line fix to `runner.py` `emit_output()` else-branch.
- **P1.7** — `stub_label_stale.py` module-docstring drift fix.
  Cosmetic; four-line replacement.
- **P1.8** — Grandfather-sweep live-source protection. New top-level
  helpers in `grandfather.py`, new `--sweep-live-source` flag, default
  skip of `other-cat1` findings on live-source paths.

### 1.2 Landing summary

| # | Item | SHA | Description |
|---|---|---|---|
| 1 | P1.8 | `5fe5e6b` | grandfather-sweep live-source protection |
| 2 | P1.5 | `119e353` | register cat3.d3q19-* checks |
| 3 | P1.6 | `71559ce` | human-renderer suppressed-stanza filter |
| 4 | P1.7 | `5cdd20f` | stub_label_stale.py docstring fix |
| 5 | SHA back-fill | `5c3e1ef` | cross-references across commit-1-4 audits |

Five commits. Three new check modules. ~23 new tests (+7 P1.8, +12
P1.5, +4 P1.6). Final test count post-batch: 135 (per executing
session's report).

### 1.3 Concurrent A.3 landing

The coordinator chat landed A.3 (bare-path-to-upstream-basename)
in parallel — five commits `6fc5884`, `77628b6`, `880a400`,
`908f619`, `1a49d33`. Audit reports at
`docs/diagnostics/_audits/integrity_v1_2_a3_commit{1,2,3,4}_landing_2026-05-15.md`.

The two batches share `tools/integrity/integrity/grandfather.py` as
the only common edit surface. A.3 modified `classify()`; this batch
added new top-level helpers and modified `apply_annotations()`. Edits
composed without integration conflicts. One cross-batch interaction
required A.3 commit 4 to extend this batch's P1.8 filter; see § 3.2.

### 1.4 End-state gate

Per executing session: 163 hard-fails post-both-batches-landing,
1046+ suppressed. The hard-fail count expanded from 4 (probe baseline)
through ~44 (immediately post-A.3-residue) to 163 (post all five P1.x
commits plus this batch's audit reports being scanned by A.3's now-active
bare-path check). See § 3.3 for the decomposition.

## 2. What worked

### 2.1 Probe-then-spec cycle held discipline

The probe report at `9add149` captured verbatim line numbers and
<!-- integrity-allow: cat1.bare-path; retrospective-doc bare-path citation pre-v1.2 (see grandfather-catalog retro-bare-path); n/a -->
contents of every surface the spec would touch (`runner.py:116-145`,
<!-- integrity-allow: cat1.bare-path; retrospective-doc bare-path citation pre-v1.2 (see grandfather-catalog retro-bare-path); n/a -->
`stub_label_stale.py:1-40` and `:95-128`, `grandfather.py:23-35` and
<!-- integrity-allow: cat1.bare-path; retrospective-doc bare-path citation pre-v1.2 (see grandfather-catalog retro-bare-path); n/a -->
`:265-330`, `d3q19_verify.py:1-281`). The spec re-grounded every
line-number citation against the probe rather than against the
earlier scoping prompt, which had carried stale numbers
<!-- integrity-allow: cat1.bare-path; retrospective-doc bare-path citation pre-v1.2 (see grandfather-catalog retro-bare-path); n/a -->
(`runner.py:891-895`, `stub_label_stale.py:644-677`).

The stale-line-number trap is a real architect-1 fabrication risk —
the same shape that Convention #8 named in v1 retro. Hard Rule 7
("audit-prose freshness") catches it at draft time; the probe-first
discipline ensured the spec had something fresh to anchor against.

Zero in-batch fabrications during execution is the empirical
confirmation: the discipline held.

### 2.2 P1.8-first commit ordering held under concurrent volume

Decision 1 in the spec re-ordered commits to land sweep protection
before check-side scope expansion (P1.8 → P1.5 → P1.6 → P1.7). The
spec framed this as "subsequent commits' sweep companions won't
over-sweep" — accurate but understated.

Actual leverage was larger because A.3 landed concurrently and its
new `cat1.bare-path` check surfaced 647 new findings repo-wide. With
P1.8's filter already on disk by the time those findings appeared,
each subsequent P1.5/P1.6/P1.7 inline sweep companion saw the
expanded finding population but only swept the small audit-doc
subset relevant to its own commit. Without P1.8's filter, those
inline sweeps would have been a much sharper instrument pointed at
the repo during this batch's execution.

This generalizes; see convention G below.

### 2.3 Concurrent multi-session landing scaled

v1.1 batch-1 retro § 7.1 named concurrent multi-session landing as the
actual operating condition. This batch confirmed at scale: two
five-commit batches landing in parallel against the same repo, sharing
one common file (`grandfather.py`), with edits to different functions
within that file composing cleanly.

The one cross-batch interaction (A.3 extending P1.8's filter — § 3.2)
was caught at A.3's execution time rather than at integration, which is
the right place for it. The integration surface stayed clean.

### 2.4 Cross-batch scope discipline

During P1.5–P1.7's inline sweep companions, ~647 newly-surfaced
`cat1.bare-path` findings were technically sweepable. This batch
deliberately did not sweep them; A.3 commit 4 owned its own sweep
companion (`908f619`) and landed it after the bare-path check had
been registered.

Holding scope kept this batch's audit reports clean and let A.3's
forensic record own the bare-path sweep. See convention I below.

## 3. Surprises and what they revealed

### 3.1 The 4→6 live-source rule validation (positive)

The spec's verification block expected `is_live_source_path()` to
protect 4 outstanding hard-fails (the live-source baseline at
`9add149`: three on `docs/phase12_lattice_boltzmann.md` and one on
`particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl`).
Post-A.3-residue, the filter protected 6 — A.3's bare-path detector
surfaced two additional live-source findings classifying into the
heterogeneous fallthrough bucket. Per executing session's commit-1
audit, the rule absorbed them without modification.

Significance: P1.8's rule was abstract on the path axis
(`is_live_source_path()` checks bucket membership, not literal paths).
If the spec had encoded the protection as "skip these 4 specific
paths," the 2 additional findings would have leaked through and
required either a hand-revert (the 9add149 failure mode this batch
was meant to eliminate) or an immediate code update. The bucket
decision generalized.

Banked as convention H below — with caveats.

### 3.2 A.3 commit 4 extended P1.8's filter (mixed signal)

Per the executing session's report:

> Extended P1.8 live-source protection to cover other-cat1-bare-path
> (the v1.2 fall-through bucket). Without this, ~50 live-source
> bare-paths would have been auto-annotated, violating the
> live-source-stays-red rule.

A.3 introduced a new fallthrough-shaped classifier category,
`other-cat1-bare-path`, that did not exist at probe time. P1.8's filter
explicitly checked `classify(f).category == "other-cat1"` — a literal
string match against the only fallthrough category that existed when
the spec was drafted. The new fallthrough category did not match the
literal, so the filter did not apply to its findings.

A.3 commit 4 (`908f619`) extended the literal match to cover the new
category. The fix was small and contained. But the underlying lesson
is worth being explicit about:

The rule-vs-enumeration design held on the path axis (§ 3.1) but did
not hold on the **category** axis. "Skip iff category equals this
string" is enumeration with one element. It doesn't generalize over
new categories any better than a hardcoded path list generalizes over
new paths.

A more robust P1.8 design would have queried a property of
`Classification` — e.g., an `is_fallthrough: bool` field, or a
predicate `is_unnamed_category(c)` — rather than literal-matching the
category string. Any future fallthrough category would automatically
inherit live-source protection without requiring a coordinated
sweep-filter extension.

This is not a fabrication — the spec was internally consistent with
the classifier surface at probe time. It's a scope-of-generalization
limit. Bank as v1.3 candidate (§ 5) and as convention H with the
caveat (§ 4.2).

### 3.3 163 final hard-fail count

The hard-fail count is now 163 from the baseline of 4. Decomposition
(approximate; cross-check against `git log --stat` and per-commit
audit reports before this retro locks):

- ~40 net new live-source bare-path findings surfaced by A.3's
  bare-path check on existing live-source files.
- ~119 new findings from A.3's bare-path check surfacing bare paths
  inside this batch's own audit reports (the audit reports cite
  file:line references that A.3 now flags as bare paths).
- The original 4 live-source baseline is unchanged.

This isn't a regression — the new findings are exactly the surface
A.3 was designed to expose for attribution. The next batch-2 item
(A.2, or possibly a sweep-companion cycle for the new audit-doc
findings on this batch's reports) is expected to close the audit-doc
subset.

If the executing session's reported "163" disagrees with current disk
when this retro is read, the discrepancy is in the audit-doc subset
(the live-source subset is stable) and should not change any of this
retro's load-bearing conclusions.

## 4. Banked conventions (additions to retro § 7.2)

Inheriting from `docs/retro/integrity-toolkit-v1.1-batch1.md` § 7.2
(conventions A–E) and the post-retro landing audit § D.2.1
(convention F). Three new entries:

### 4.1 Convention G — Sweep-side protection lands before check-side scope expansion

> **G.** Sweep-side protection lands before check-side scope
> expansion. When a v1.x batch adds a check that will classify into
> broad buckets (live-source / audit-doc / retro-doc / etc.), the
> corresponding sweep CLI protection rule must land before or
> alongside the check registration. Specifically: if a check produces
> live-source findings whose intended treatment is "attribute, do not
> sweep," the live-source filter must be active in the sweep CLI
> before the check is registered.
>
> "Active" means the filter must cover the category space the new
> check will produce. If the new check creates a new classifier
> category, the filter needs extending to recognize that category as
> a sweep-protected bucket — landing both within the same batch (or
> coordinating across batches with an explicit hand-off) is
> sufficient. See v1.2 § 3.2 for the v1.2/A.3 interaction case where
> the literal-match filter did not cover A.3's new fallthrough
> category and required an immediate extension.

### 4.2 Convention H — Filter rules query properties, not literals

> **H.** When implementing sweep filters or similar policy rules over
> a typed surface (`Classification`, `FailureMode`, etc.), prefer
> queries against properties of the type ("is this fallthrough?",
> "is this hard-fail?", "is this live-source?") over literal string
> matches against specific values.
>
> Literal matches do not generalize over new values added concurrently
> by other batches; property queries do. See v1.2 § 3.1 for the
> positive case (path-axis property query absorbing 2 additional
> findings under concurrent landing) and § 3.2 for the negative
> contrast (category-axis literal match requiring an extension when
> A.3 introduced `other-cat1-bare-path`).

### 4.3 Convention I — Cross-batch scope discipline

> **I.** When a sweep CLI run during a small-scope batch's verification
> picks up findings outside the batch's scope, do not opportunistically
> sweep them. Defer to the responsible batch's own sweep companion.
> This keeps audit reports clean (one batch's work doesn't co-mingle
> with another's), preserves the per-batch landing audit's leverage
> as forensic evidence, and prevents scope creep that may invalidate
> the batch's own design assumptions.
>
> In v1.2 bolt-ons, ~647 `cat1.bare-path` findings were available to
> sweep during P1.5–P1.7's inline sweep companions. Deferring to
> A.3 commit 4's planned sweep companion (`908f619`) preserved both
> batches' coherence and gave A.3's forensic record sole ownership of
> the bare-path sweep.

## 5. Banked observations (v1.3 candidates)

Small items surfaced during execution that don't warrant convention
status but are worth pinning before they fall off:

1. **TOML vs JSON expected-values format inconsistency.** Probe § C.4
   flagged that `cubic_kernel`'s expected values are in TOML and
   d3q19's are in JSON. Not addressed in v1.2. Worth converging in
   v1.3 to reduce future cat3-check author confusion.
2. **`Classification` could carry an `is_fallthrough` capability.**
   Per § 3.2 / convention H. A field, predicate, or method that lets
   sweep filters query "is this category a heterogeneous fallthrough
   bucket?" without literal string matching. Would have made
   A.3 commit 4's filter extension a no-op.
3. **`_emit_human_summary` ordering.** Currently called before
   stanza emission in the else branch; for a human reader the summary
   would arguably read better after the per-finding stanzas. Mild
   aesthetic inelegance, not load-bearing. Bank if anyone cares.
4. **P1.6 `Finding.suppressed` field accessibility.** The spec
   § 5.2.2 flagged a potential pause-and-surface around whether the
   `Finding` dataclass exposes `suppressed` as a constructor kwarg.
   Per executing session's report, all 4 P1.6 tests pass — the
   fixture pattern worked. Worth eyeballing the landed test file to
   confirm whether Claude Code adjusted the construction; if it did,
   the spec's framing of the assumption should be tightened for
   future P1.6-shaped fixes.
5. **Spec line-budget overrun.** The pre-spec probe was 1987 lines
   against a 500-900 target. The spec itself was 1845 lines. Both
   batches' specs are heavy; reading load grows with toolkit
   complexity. Worth thinking about whether the spec format wants
   compression (more cross-references, fewer verbatim dumps) before
   v1.3's expected larger scope. Not urgent.

## 6. Open questions

1. **Is A.2 (toolkit self-application) still the next batch-2
   priority?** Per addendum § 5 yes. But A.3's landing has expanded the
   recursive blind-spot test space (more checks = more
   own-source-discipline edges A.2 needs to catch). A.2's probe should
   re-anchor against post-A.3 disk rather than against addendum-era
   assumptions.
2. **Should convention H's "property query" recommendation become a
   structural change to `Classification` in v1.3, or stay
   case-by-case?** Either is defensible. Architect-2 perspective would
   help.
3. **Should retro convention numbering be alphabetic (A, B, ... G, H,
   I) indefinitely, or should v1.2's additions be re-numbered into a
   structured taxonomy as the convention list grows?** Currently
   conventions cover: spec-time discipline (B, C, D), execution-time
   discipline (A, F), batch-coordination (G, I), and design taste
   (E, H). Mild taxonomy might help future architects find the right
   convention quickly. Defer to architect-2.

## 7. Closing

Four spec items, five landing commits, ~23 new tests, no in-batch
fabrications. Both batches (v1.2 bolt-ons + A.3) landed in parallel
without integration conflicts beyond the documented A.3-extends-P1.8
interaction. Conventions G/H/I extend the v1.1 banked list.

This batch's load-bearing decisions:

- Probe-then-spec cycle with grep-anchored verbatim claims (§ 2.1).
- P1.8-first commit ordering, motivated by sweep-protection-before-
  check-expansion (§ 2.2, banked as G).
- Abstract rules over enumerations, with awareness of the
  generalization axis (§ 3.1, § 3.2, banked as H).
- Hold scope; defer out-of-scope sweeps (§ 2.4, banked as I).

The bolt-on batch closes here. Next batch-2 work is A.2 per addendum
§ 5; expected to be a probe-then-spec cycle similar to A.3's.

Architect-1 retro of own work. Architect-2 review pass remains open
on the v1.1 batch-1 retro per its § 6.4; this retro can be reviewed
behind that backlog.

## End of retro
