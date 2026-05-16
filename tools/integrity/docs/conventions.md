# Integrity Toolkit Conventions

Conventions banked across the v1.x cycle for the integrity toolkit
chain. Each convention is a short rule that emerged from a specific
failure or design observation; the `Source` line points to the retro
where it was originated.

This doc is the **canonical home** for these conventions (resolves
T3.2 per Decision D5 in
[`docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md`](../../../docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md)
§ 0.3). Retros may bank new conventions; whenever a convention is
added in a retro, mirror it here on the next batch.

The taxonomy groups by intent-of-use, not by origin date. The letters
(A–K and beyond) are the historical anchors; the taxonomy section
headings are how a reader looking for "which convention applies to my
current task?" should find the right one.

> **Disclosure.** This doc consolidates conventions banked by
> architect-1 (chat agents) and signed off via the same source's
> retros. No architect-2 review pass has been obtained on the
> convention list (the user formally opted out of architect-2 at
> the start of v1.2). The conventions are load-bearing in practice
> and have been validated by repeated firings, but they have not
> been independently reviewed.

## Spec-time discipline

### Convention C — Probe API surfaces before drafting a spec

> **C.** Pre-spec probes that ground path-resolution rules must
> include verbatim probe items enumerating 3-5 representative path
> pairs.

**Source:** v1.1 batch-1 retro § 7.2 C.

### Convention D — Probe call-sites before drafting a spec

> **D.** Pre-spec probes that ground behavioral changes must include
> verbatim probe items enumerating all modules that depend on the
> affected behavior.

**Source:** v1.1 batch-1 retro § 7.2 D.

### Convention K — Anchor-sketch labeling for spec content from inference

> **K.** When a spec section constructs content from probe data plus
> architect-1 inference (rather than from verbatim verified content
> on disk), label the section explicitly as an "anchor sketch —
> verify at execution time" rather than presenting it as canonical.
> Anchor sketches name a likely failure mode upfront; the verification
> block for the same section should include a check that confirms
> the sketch against execution-time disk.
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

**Source:** v1.3 batch-1 part-A retro § 4.2.

## Execution-time discipline

### Convention A — New-files-first decomposition

> **A.** Execution specs default to commit decomposition for any
> commit that touches more than one previously-existing file. The
> new-files-only sub-commit ships first.

**Source:** v1.1 batch-1 retro § 7.2 A.

### Convention F — Audit-prose freshness

> **F.** Audit-prose freshness check. Audit reports drafted at
> direction time and landed later by an executor should verify the
> gate-state claims against current disk immediately before commit.
> Discrepancies become addenda (not paraphrases) to preserve the
> audit trail of when each claim was authored vs landed.

**Source:** v1.1 post-retro landing audit § D.2.1. (Originally banked
as indented prose; normalized to the standard `> **F.**` blockquote
shape here per closeout probe § G.7 / spec § 6.C.1.)

## Batch coordination

### Convention G — Sweep-side protection lands before check-side scope expansion

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

**Source:** v1.2 bolt-ons retro § 4.1.

### Convention I — Cross-batch scope discipline

> **I.** When a sweep CLI run during a small-scope batch's
> verification picks up findings outside the batch's scope, do not
> opportunistically sweep them. Defer to the responsible batch's own
> sweep companion. This keeps audit reports clean (one batch's work
> doesn't co-mingle with another's), preserves the per-batch landing
> audit's leverage as forensic evidence, and prevents scope creep
> that may invalidate the batch's own design assumptions.
>
> In v1.2 bolt-ons, ~647 `cat1.bare-path` findings were available to
> sweep during P1.5–P1.7's inline sweep companions. Deferring to
> A.3 commit 4's planned sweep companion (`908f619`) preserved both
> batches' coherence and gave A.3's forensic record sole ownership
> of the bare-path sweep.

**Source:** v1.2 bolt-ons retro § 4.3.

**Letter-I collision note.** Part-B retro § 4.1 also references
"Convention I" for the rewrite-stale-reasons sweep mode T2 candidate.
That was a numbering collision and not a real convention; the
rewrite-stale-reasons capability landed in v1.3 closeout commit 1 as
a feature, not a convention, and is documented at spec § 2. The
canonical Convention I is the cross-batch scope discipline rule
above.

### Convention J — Sweep companions operate across commit boundaries when multi-file commits land

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

**Source:** v1.3 batch-1 part-A retro § 4.1.

## Design taste

### Convention B — Grandfather-sweep companion

> **B.** Every commit that touches cat1-scannable surface either runs
> the sweep or lands a companion sweep commit in the same PR.

**Source:** v1.1 batch-1 retro § 7.2 B.

### Convention E — Spec-author-self-test review

> **E.** When the spec author writes new test code for own-spec
> items, a second pair of eyes (architect-2 or a mechanical own-
> source scan) reviews the test files for grammar-literal leaks
> before commit.

**Source:** v1.1 batch-1 retro § 7.2 E.

### Convention H — Filter rules query properties, not literals

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

**Source:** v1.2 bolt-ons retro § 4.2.

## Decisions resolved without architect-2 review

This section banks the T3 items that had been "needs architect-2
review" across multiple retros. The user formally opted out of
architect-2 at the start of v1.2; per Part-B retro § 5.4 these items
needed either a decision or formal banking. Each gets an architect-1
decision with explicit disclosure.

### D4 — T3.1 (A.9 audit-citation exclusion): rejected

**The proposal:** Exclude audit-doc paths
(`docs/diagnostics/_audits/`) from `cat1.intra-repo` scans entirely,
rather than scanning and bucketing findings under the `audit-citation`
grandfather category.

**Quantified leverage** (per v1.1 self-review probe): ~67% pool
collapse if adopted.

**Decision: rejected.** Keep-and-bucket preserves the audit trail of
which audit doc had which finding. The grandfather catalog already
segments these into named categories (`audit-citation`,
`audit-bare-path`). Pool size is not the load-bearing metric;
per-finding attribution is.

**Status:** architect-1-decided, no architect-2 review obtained.
Reconsideration is v2 scope.

### D5 — T3.2 (conventions doc home): `tools/integrity/docs/conventions.md`

**The proposal:** Three locations were on the table:
- `docs/CONVENTIONS.md` (repo root)
- `tools/integrity/docs/conventions.md` (toolkit-scoped)
- Per-stack (e.g., `common/common-py/docs/conventions.md`)

**Decision: `tools/integrity/docs/conventions.md`.** This is the file
you are reading. Toolkit-scoped conventions live with the toolkit.
The repo-root location was rejected because conventions outside the
integrity chain (e.g., from `docs/retro/phase11.md`) are not the same
lineage and would be confused with these.

**Status:** architect-1-decided, no architect-2 review obtained.

### D6 — T3.3 (numbering taxonomy): four-bucket

**The proposal:** Re-group A–K into a taxonomy or keep alphabetic.

**Decision: four-bucket taxonomy** per v1.3 batch-1 part-A retro
§ 5.2:
- Spec-time discipline
- Execution-time discipline
- Batch coordination
- Design taste

Letters preserved as historical anchors. Section headings are how
readers navigate.

**Status:** architect-1-decided, no architect-2 review obtained.

### D7 — T3.4 (architect-2 backlog from v1.1 batch-1 retro § 6.4): formally banked unresolved

**The proposal:** Four items have been "needs architect-2 review"
across multiple retros:

1. v1.1 retro § 3.1 / § 3.2 root-cause framing of architect-1
   fabrications
2. v1.1 retro § 5.3 recursive-blind-spot procedural-vs-structural
   framing
3. v1.1 retro § 6.1 priority ordering
4. v1.1 retro § 6.2 enforcement-level pick (overlaps with T2.1, now
   resolved by D2)

**Decision: formally bank as unresolved.** Items 1–3 remain
architect-1-perspective only. The framings the retros offer are the
banked default; if architect-2 ever weighs in (v2 or beyond), the
banked framings should be revisited.

Item 4 is resolved by Decision D2 (medium / CI check) in the v1.3
closeout spec.

**Status:** architect-1-decided to defer items 1–3 indefinitely;
item 4 resolved as part of v1.3 closeout.

## Living document

When new conventions are banked in future retros, mirror them here in
the appropriate taxonomy section. The retro remains the source of
truth for origination context; this doc is the navigation surface.

This doc is the canonical home; references to "the conventions" in
spec drafts, retros, and audit reports should point here from now on.
