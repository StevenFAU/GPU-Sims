---
title: "Integrity Toolkit v1.3 Candidates — Consolidated Roadmap"
date: 2026-05-15
author: architect1
status: draft
sibling-docs:
  - docs/integrity-toolkit-spec.md
  - docs/retro/integrity-toolkit-v1.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/retro/integrity-toolkit-v1.2-bolt-ons.md
  - docs/diagnostics/_audits/integrity_v1_1_post_batch_triage_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_self_review_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md
---

# Integrity Toolkit v1.3 Candidates — Consolidated Roadmap

## 0. Purpose and bias disclosure

The v1.x banked-candidate list has grown across eight predicate
documents (the v1 retro, the v1.1 batch-1 retro and its addendum, the
post-batch triage, the post-retro landing audit, the self-review probe,
the v1.2 bolt-ons retro, plus the canonical v1 spec § 13 v2 list). Each
banks items in its own framing. Picking the next batch composition
without a consolidated view forces re-deriving the priority calculus
from scratch — expensive, error-prone, and a place where Convention #8
(architect-1 fabrication) can leak back in via priority-from-memory.

This document is the single source of truth for v1.3 candidate inventory
as of `9add149`+v1.2-bolt-ons + concurrent A.3 + (A.2 in flight). Each
entry carries its source citations, scope estimate, dependencies, and
status. The closing sections recommend a v1.3 batch composition
appropriate to current resources.

**Bias disclosure.** This document is self-drafted by architect-1, who
also authored the v1.2 bolt-ons retro and spec (the most recent banking
sources). The same structural bias named in
`docs/retro/integrity-toolkit-v1.1-batch1.md` § 0 and v1.2 bolt-ons
retro § 0 applies: the author of the source documents is the wrong
audience for unbiased prioritization. Mitigations:

- **Inventory completeness over priority confidence.** Every banked
  item is recorded with its source; ranking choices are flagged where
  they reflect architect-1 judgment rather than empirical leverage.
- **Architect-2 review pass remains owed.** Per v1.1 batch-1 retro
  § 6.4 and v1.2 bolt-ons retro § 0, architect-2 has a multi-document
  review queue that this roadmap joins.
- **Mechanical claims are grep-verified.** Source citations and SHA
  references were confirmed against project knowledge during drafting;
  any claim of "landed" vs "deferred" status reflects on-disk evidence.

This roadmap is not normative until architect-2 reviews. Architect-1
chats may use it to pick a next batch in the interim, with the explicit
caveat that the chosen scope can be revised once architect-2's review
lands.

## 1. Source corpus

This roadmap synthesizes banked items from:

| Source | Banked items |
|---|---|
| `docs/integrity-toolkit-spec.md` § 13 | 6 v2 candidates (canonical list at v1-landing time) |
| `docs/retro/integrity-toolkit-v1.md` § 4 | 4 honest gaps (A.6, A.2-shaped, A.4-shaped, A.3-shaped) |
| `docs/retro/integrity-toolkit-v1.1-batch1.md` § 6.1 | 9-item priority-ordered v1.2 candidate list (A.x) |
| `docs/retro/integrity-toolkit-v1.1-batch1.md` § 6.3 | v2-candidate: spec-vs-repo-state reconciliation |
| `docs/retro/integrity-toolkit-v1.1-batch1.md` § 8 | 4 open questions for architect-2 |
| `docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md` § D.2.1 | Convention F (audit-prose freshness) + check mechanization candidate |
| `docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md` § D.3 | `project-state-snapshot` classifier extension (concrete code stub) |
| `docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md` § E | 7-item refined batch-2 priority list with concurrent-landing evidence |
| `docs/retro/integrity-toolkit-v1.1-batch1-addendum.md` § 5 | Re-numbered batch-2 priorities (P1–P7+); P-numbering convention |
| `docs/diagnostics/_audits/integrity_v1_1_post_batch_triage_2026-05-15.md` § C.2 | `toolkit-doc-snapshot` classifier rule (concrete) |
| `docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md` § C.4 / § K | TOML/JSON format inconsistency, J.4 sweep thinness |
| `docs/retro/integrity-toolkit-v1.2-bolt-ons.md` § 4 | Conventions G, H, I |
| `docs/retro/integrity-toolkit-v1.2-bolt-ons.md` § 5 | 5 v1.3 candidates (small-scope) |
| `docs/retro/integrity-toolkit-v1.2-bolt-ons.md` § 6 | 3 open questions |

Plus, anticipated additions from the in-flight A.2 probe at
`docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md`
(banked observations not yet visible at roadmap draft time). The closing
§ 10 of this roadmap will need an addendum once A.2 lands.

## 2. Status legend

- **LANDED** — feature is on `main`; archived for reference only.
- **IN FLIGHT** — actively in probe/spec/execution cycle in another
  session.
- **READY** — scope clear, design decision either settled or trivial;
  can go straight to probe → spec → execution.
- **NEEDS DESIGN** — scope clear but at least one design decision is
  open; architect-1 can pick, but the choice is load-bearing enough to
  warrant explicit framing in the spec.
- **NEEDS REVIEW** — architect-2 review is recommended before tier
  assignment or before spec drafting locks. Either because the design
  decision has multiple defensible answers, or because the originating
  source explicitly flagged the item for architect-2 attention.
- **HORIZON** — v2 candidate per spec § 13 or analogous; not slated for
  the v1.x cycle.

## 3. Landed and in-flight inventory

These are removed from the active candidate pool but recorded so the
reader knows where each prior banking went.

| ID | Item | Source | Status |
|---|---|---|---|
| v1.1 | `cat2.stub-label-stale` check | v1 retro § 4 | LANDED (v1.1 batch-1 commit 1, `~f661ec4`) |
| v1.1 | A.5 fence-skip extension | v1.1 retro § 2.2 | LANDED (v1.1 batch-1) |
| v1.1 | A.7 CLI flags (--state-snapshot / --grandfather-report) | v1.1 retro | LANDED (`a71594a`) |
| v1.1 | Own-source annotations on test_suppression_fence.py | v1.1 retro § 3.4 | LANDED (`a42085a`) |
| v1.2 | P1.5 — register cat3.d3q19-* checks | addendum § 4.3 | LANDED (`119e353`) |
| v1.2 | P1.6 — human-renderer suppressed-stanza filter | addendum § 4.1 | LANDED (`71559ce`) |
| v1.2 | P1.7 — stub_label_stale.py docstring | addendum § 4.4 | LANDED (`5cdd20f`) |
| v1.2 | P1.8 — grandfather-sweep live-source protection | bolt-ons retro § 1.1 | LANDED (`5fe5e6b`) |
| v1.2 | A.3 (bare-path-to-upstream-basename) | v1.1 retro § 6.1 P1 / addendum § 5 P1 | LANDED (A.3 commits `6fc5884`–`1a49d33`) |
| v1.2 | A.3 commit 4 extends P1.8 filter for other-cat1-bare-path | bolt-ons retro § 3.2 | LANDED (`908f619`) |
| v1.2 | A.2 (toolkit self-application) | v1.1 retro § 6.1 P2 / addendum § 5 P2 | IN FLIGHT (probe being executed in parallel session) |

The remaining v1.x scope after A.2 lands is the subject of § 4–§ 8 below.

## 4. T1 — Ready to spec

These items have settled scope and trivial-or-settled design decisions.
A.2 is the headline in-flight item; this tier defines what's queued
behind it.

### T1.1 — Classifier rules commit: 3 new permanent categories

**Scope:** Land three classifier rules + three matching grandfather
catalog sections in a single commit:

1. **`toolkit-doc-snapshot`** — `cat1.intra-repo` finding on
   `tools/integrity/docs/` paths or on `docs/integrity-toolkit-spec.md`
   or `tools/integrity/README.md`.
2. **`project-state-snapshot`** — `cat1.intra-repo` finding on
   `project-state.md` (repo root). Concrete stub already drafted at
   `docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md`
   § D.3.
3. **`retro-doc-snapshot`** — `cat1.intra-repo` finding on
   `docs/retro/` paths. Surfaced by the post-retro landing audit's
   step-1 sweep companion (retro intra-repo findings fell through to
   `other-cat1`, same pattern).

**Source:** v1.1 batch-1 retro § 6.1 item 4 (consolidated); post-retro
landing audit § D.3 + § E (refined); v1.1 batch-1 addendum § 5 P4.

**Dependencies:** None. Adds rules above the `other-cat1` fallthrough
in `classify()`; pure addition.

**Estimated scope:** ~50 LOC code + ~40 LOC catalog + ~50 LOC tests.
3–4 new tests in `test_grandfather_sweep.py`. One commit + audit report
+ SHA back-fill.

**Interaction with v1.2 P1.8:** These new categories should appear in
the bucket-correlation table from v1.2 bolt-ons probe § F.7. The new
classifier rules produce findings on swept-by-default paths (audit-doc,
toolkit-doc); P1.8's live-source filter does not interfere since these
paths are in `SWEEPABLE_PATH_PREFIXES`.

**Interaction with Convention H (v1.2 bolt-ons retro § 4.2):** This
is the case Convention H was designed for. The new categories are
*named* (not fallthrough); the sweep will process them normally. No
filter extension required. Validates Convention H's framing.

**Status:** READY.

### T1.2 — Convention H structural follow-through: `Classification.is_fallthrough`

**Scope:** Add a property (field, computed attribute, or method) to the
`Classification` dataclass that lets sweep filters query "is this a
heterogeneous fallthrough bucket?" without literal string match on
category name. Refactor P1.8's filter logic in `apply_annotations()` to
use the property instead of `category == "other-cat1"`. Also refactor
A.3 commit 4's extension (the `other-cat1-bare-path` literal match) to
use the property.

**Source:** v1.2 bolt-ons retro § 5 item 2 (the cross-axis caveat to
Convention H); v1.2 bolt-ons retro § 3.2.

**Dependencies:** None directly. But should land *before* any future
fallthrough-shaped category is introduced (otherwise the next batch
faces the same A.3-commit-4 extension friction).

**Estimated scope:** ~30 LOC code + ~20 LOC tests. One commit.

**Design decision (small):** Field vs. method vs. naming-convention.
Three options:

- (a) `is_fallthrough: bool` field on `Classification`, set explicitly
  at construction. Most explicit; requires touching every existing
  `Classification(...)` constructor.
- (b) Method `Classification.is_fallthrough()` that checks
  `self.category.startswith("other-cat")` or maintains an allowlist.
  Lower touch; less explicit.
- (c) Module-level constant `FALLTHROUGH_CATEGORIES: frozenset[str]`
  + helper `is_fallthrough_category(cls.category)`. Mirrors P1.8's
  `SWEEPABLE_PATH_PREFIXES` shape.

Architect-1 weak preference: (c), for shape-consistency with P1.8.

**Status:** READY.

### T1.3 — A.8 catalog auto-refresh

**Scope:** Make `tools/integrity/docs/grandfather-catalog.md` per-category
counts auto-refresh from `--grandfather-report` output rather than
requiring manual edits at each batch's catalog-anchor commit. v1.1
batch-1 retro § 5.5 quantified the drift as +63 entries (+6.7%) across a
single batch cycle; v1.2 + A.3 likely compounded this further.

**Source:** v1 retro spec § 13 + v1.1 batch-1 retro § 5.5 + § 6.1 item 6
+ addendum § 5 P6.

**Dependencies:** None directly. Composes naturally with T1.1 (new
categories) — landing T1.1 first means the catalog has more entries
that benefit from auto-refresh, but auto-refresh is correct
independently.

**Estimated scope:** ~80–120 LOC. New script
`tools/integrity/scripts/refresh_catalog.py` that reads
grandfather-report output and updates count parentheticals in
grandfather-catalog.md headings. Optional integration as a pre-commit
hook OR as an explicit step in the sweep CLI. ~5 tests.

**Design decision (small):** Refresh-in-place vs. emit-new-file. Stay
in-place to preserve audit trail of catalog edits in git history.

**Status:** READY.

### T1.4 — Pre-spec probe template upgrades

**Scope:** Update `docs/diagnostics/probe-template.md` (or equivalent
canonical probe-template doc; verify location at spec time) with two
concrete additions surfaced by self-review probe § H:

- (C) Enumerate 3–5 representative `header → impl` path pairs from the
  synced repo at probe time. Catches Decision-2-shaped convention
  fabrications at draft time (the v1.1 batch-1 pause-and-surface #1
  pattern).
- (D) Enumerate every call site of any function the spec proposes to
  modify. Catches signature-fabrication patterns at draft time.

**Source:** v1.1 batch-1 retro § 7.2 C+D; self-review probe § H.2
confirms both gaps are real; addendum § 5 P3.

**Dependencies:** None. Pure additive change to a process doc.

**Estimated scope:** ~30 LOC documentation. No code. ~zero tests.

**Status:** READY. (Smallest item in T1 by far; can be folded into any
other commit as a pendant.)

### T1.5 — TOML/JSON expected-values format convergence

**Scope:** `cat3.cubic-kernel` uses TOML (`expected_values.toml`);
`cat3.d3q19-*` uses JSON (`d3q19_equilibrium.expected.json`). Future
cat3 check authors face an undocumented choice. Pick one format, update
the off-format file to match. Add a brief convention note to
`tools/integrity/docs/` describing the rule for future cat3 expected-values
files.

**Source:** v1.2 bolt-ons probe § C.4; v1.2 bolt-ons retro § 5 item 1.

**Dependencies:** None. Touches two existing files plus one new
convention doc.

**Estimated scope:** ~50 LOC of file conversion + ~30 LOC doc + verify
existing tests still pass. One commit.

**Design decision (small):** TOML or JSON. Arguments either way:

- TOML: human-readable, comments allowed, schema-friendly for hand-
  authored expected values, native `tomllib` in Python 3.11+.
- JSON: machine-generated more naturally (the d3q19 harness writes its
  expected values via `json.dumps`; the cubic-kernel format was
  hand-authored). Universal Python support, no version constraints.

Architect-1 weak preference: JSON, on grounds that machine-generation
is the more common case as cat3 expands. Hand-authored TOML is a
v1-era pattern that becomes less likely as harnesses do the
derivation.

**Status:** READY.

## 5. T2 — Needs design

These items have clear scope but one or more open design decisions that
architect-1 can pick but that warrant explicit framing in a spec.

### T2.1 — Grandfather-sweep enforcement convention level

**Scope:** Pick soft/medium/hard enforcement of the
grandfather-sweep-companion convention from v1.1 batch-1 retro § 6.2.
The convention is documented (Convention B in retro § 7.2); current
enforcement is soft (authors expected to remember).

**Source:** v1.1 batch-1 retro § 6.2 + § 6.4 item 4 + § 7.2 B;
post-retro landing audit § D.2.1 banked Convention F (audit-prose
freshness) as a related sixth convention; addendum § 5 P5;
self-review probe § I.3 inferred medium-over-hard quantitatively
(96.8% un-paired rate overstates the actual finding-introduction
rate because most cat1-touching commits don't add new findings).

**Dependencies:** Architect-2 review preferred (explicit § 6.4 item).
Architect-1 can pick if review backlog persists.

**Estimated scope:**

- **Soft (status quo):** Zero LOC. Just keep the convention documented.
- **Medium (CI check):** ~100 LOC. Custom CI step that diffs the
  live-source class against the prior commit; fails when delta grows
  without paired sweep commit. Modest implementation.
- **Hard (pre-commit hook):** ~50 LOC for the hook + setup. Slow
  commits (~3-5s per commit) on cat1-scannable file edits.

**Design decision (load-bearing):** Which enforcement level. Probe
§ I.3 evidence favors medium; architect-2 has not weighed in.

**Status:** NEEDS DESIGN (with architect-2 review preferred).

### T2.2 — Audit-prose freshness mechanization

**Scope:** Convention F (audit-prose freshness) is currently a manual
discipline: authors re-verify load-bearing claims at landing time and
record drift as addenda. Mechanizing this is the v1.1 batch-1 retro
§ 6.3 "spec-vs-repo-state reconciliation at draft time" v2 candidate,
strengthened by post-retro § D.2.1's banking of Convention F itself.

**Scope shape:** A tool that scans a spec or audit-report draft for
assertions of the form `<file>:<line>`, `<phrase X is present in file
Y>`, `<API Z has shape W>` and grep-verifies each against current
repo state. Mismatches are flagged for the author before commit.

**Source:** v1.1 batch-1 retro § 6.3 (the v2-candidate originating
shape) + post-retro § D.2.1 (Convention F + mechanization candidate)
+ addendum § 5 (banked as separate item from probe-template upgrades);
v1.2 bolt-ons retro § 4.1 (Convention G's "active" language depends on
freshness verification).

**Dependencies:** None directly. Overlap with T1.4 (pre-spec probe
template upgrades): both address spec-time freshness, but at different
abstraction levels. T1.4 is "tell the human to dump these things";
this is "check the human's draft mechanically." T1.4 is the cheaper
half-step; this is the deeper mechanization.

**Estimated scope:** Larger than T1 items. Probably 200–400 LOC for a
minimum viable parser + verifier. Could be staged: phase 1 catches
just `<file>:<line>` citations; phase 2 adds API-shape assertions.

**Design decision (load-bearing):** Format of the parseable claim. Two
flavors:

<!-- integrity-allow: cat1.bare-path; retrospective-doc bare-path citation pre-v1.2 (see grandfather-catalog retro-bare-path); n/a -->
- (a) Inline assertion grammar (`<!-- verify-claim: file.py:42 has "def foo" -->`).
  Explicit but invasive; authors must remember to tag claims.
- (b) Heuristic grep of plain prose for `path:line` patterns. Lower
  author burden, more false positives.

Architect-1 weak preference: (b) with conservative match (require
backtick-fenced citations only). Refine to (a) for higher-stakes
assertions where false-positives matter.

**Status:** NEEDS DESIGN. Could also be NEEDS REVIEW depending on how
much weight one gives to v1.1 retro § 6.3's "v2 candidate" framing
(which suggested this was a later cycle), versus addendum's banking it
as more imminent.

### T2.3 — A.6 Stack C runtime optimization

**Scope:** Cache USR data across libclang invocations to reduce
~95-second Stack C scan time. Currently each TU is parsed twice (once
for symbol extraction, once for reference finding); single-parse
strategy would roughly halve wall-clock.

**Source:** v1 retro § 4 "Stack C runtime dominates CI walltime";
v1.1 batch-1 retro § 6.1 item 8 / addendum § 5 P7+.

**Dependencies:** None.

**Estimated scope:** ~150–300 LOC refactor of `stack_c.py` + minor
test-suite adjustments. Most complex part is making sure the cache is
invalidated correctly when source files change.

**Design decision (small):** In-memory cache vs. on-disk cache. Probably
in-memory (single CI run lifecycle); on-disk only helpful if local-dev
patterns favor it.

**Status:** NEEDS DESIGN (priority justification rather than design
substance — performance, not correctness, so leverage is lower than T1
items).

## 6. T3 — Needs architect-2 review

These items have a design decision with multiple defensible answers
where architect-2 perspective is explicitly requested by the
originating source.

### T3.1 — A.9 audit-citation file-pattern exclusion

**Scope:** v1.1 spec § 7 banked an architect-2 review item: would
excluding audit-doc paths from `cat1` scans entirely (vs. relying on
the grandfather classifier to bucket them) collapse the live-source
pool's signal-to-noise?

**Source:** v1.1 batch-1 retro § 6.1 item 9; v1.1 spec § 7
(architect-2 item); self-review probe quantified ~67% pool collapse
if adopted (substantial).

**Dependencies:** Architect-2 review explicitly requested.

**Estimated scope (if adopted):** Small — a path-prefix filter at the
`cat1.intra-repo` collection step. ~30 LOC.

**Design decision (load-bearing):** Whether to exclude audit-doc paths
from the scan, vs. include and bucket. Architect-2's frame matters:
excluding loses the audit trail of which audit doc had which finding;
including preserves it but creates persistent suppression-list growth.

**Status:** NEEDS REVIEW.

### T3.2 — Conventions doc home

**Scope:** v1.1 batch-1 retro § 8 question 3 asks where the convention
list lives long-term:

- `docs/CONVENTIONS.md` at repo root
- `tools/integrity/docs/conventions.md` (toolkit-scoped)
- Per-stack (e.g., `common/common-py/docs/conventions.md`)

Convention numbering has accumulated to A–I (9 conventions) plus
several phase-retro-banked conventions outside the integrity-toolkit
chain (per `docs/retro/phase11.md`). The retro is the wrong long-term
home; this is a structural-document question.

**Source:** v1.1 batch-1 retro § 8 question 3; v1.2 bolt-ons retro § 6
question 3 (convention numbering taxonomy).

**Dependencies:** None directly. But landing T1.1 / T1.2 / T1.3
without a settled conventions home means more retro/addendum prose
that the eventual conventions-doc migration has to consolidate.

**Estimated scope:** Tiny (the consolidation is the work, not the
location decision). Doc-only.

**Design decision (load-bearing-via-cumulative-cost):** Which location.

**Status:** NEEDS REVIEW.

### T3.3 — Convention numbering taxonomy

**Scope:** Currently conventions A–I are alphabetic and growing.
Re-grouping into a taxonomy (spec-time discipline / execution-time
discipline / batch-coordination / design taste / etc.) might help
future architects locate the right convention quickly.

**Source:** v1.2 bolt-ons retro § 6 question 3.

**Dependencies:** Implicitly depends on T3.2 (conventions home) being
settled. Renumbering before the home is decided risks double-
renumbering.

**Estimated scope:** Doc-only. ~50 LOC restructuring.

**Status:** NEEDS REVIEW. Lower priority than T3.2.

### T3.4 — Architect-2 review backlog: v1.1 batch-1 retro § 6.4

Per v1.1 batch-1 retro § 6.4, the following items remain open for
architect-2 attention from the v1.1 cycle:

1. § 3.1, § 3.2 root-cause framing of architect-1 fabrications
2. § 5.3 recursive blind spot procedural-vs-structural framing
3. § 6.1 priority ordering reconciliation
4. § 6.2 enforcement-level pick (overlaps with T2.1)

Plus v1.1 batch-1 retro § 8's four open questions, and v1.2 bolt-ons
retro § 6's three open questions.

This is a process-debt item, not a feature. Surface here so it's not
forgotten; mechanism for clearing it is architect-2 scheduling, not a
toolkit commit.

**Status:** NEEDS REVIEW (process debt).

## 7. T4 — Horizon (v2)

These items were banked as v2 candidates per v1 spec § 13, or
strongly v2-shaped per later retros. They are not part of v1.3 batch
planning but should remain on the radar.

| Horizon item | Origin | Notes |
|---|---|---|
| Cat 4: Runtime integration tests | v1 spec § 13 | Major scope; running binaries against canonical inputs |
| Cat 3 GPU shader coverage via headless | v1 spec § 13 | Stack B via dawn / headless Chrome; Stack C via SwiftShader |
| Multi-line citation grammar | v1 spec § 13 | aka A.4 if anyone re-banks it under that label |
| Per-sim numerical checks beyond common-* | v1 spec § 13 | Partial work surfaced in `docs/category-contexts/quantum.md` § 6.1 — banked 4 candidate cat3 checks for Ising/QUBO/Wolff that could land standalone |
| Spec-vs-implementation reconciliation | v1 spec § 13 | Architect-2 review work, distinct from T2.2's draft-time check |
| Type-aware Cat 2 matching | v1 retro § 5 | Currently token-based fallback in libclang for Stack C |
| Annotation-grammar fenced-block awareness | v1 retro § 5 | Currently handled by per-line grandfather suppression |
| `_emit_human_summary` ordering | v1.2 bolt-ons retro § 5 item 3 | Aesthetic; emit summary after stanzas not before |

The quantum-sim cat3 candidates (Ising-energy, QUBO-Ising-roundtrip,
Wolff-bond-probability, Onsager-Tc) are interesting because they would
land as cat3 entries in the existing check registration system without
requiring v2-cat-4 infrastructure. If a quantum sim batch lands first
and the user wants to seed cat3 expansion alongside, those four could
be promoted to T1. Banking here under HORIZON; if quantum sim work
intensifies, revisit.

## 8. Dependency graph and parallelism opportunities

Items that can land independently in parallel:

- **T1.1 (classifier rules)** — depends only on `grandfather.py`'s
  `classify()`. Concurrent A.2 work also touches `grandfather.py`
  (probably adding a new classifier rule). Compose-safe if both add
  rules without modifying existing rules.
- **T1.2 (Convention H fix)** — touches `Classification` dataclass +
  `apply_annotations()` + A.3's commit-4 filter extension. If A.2
  also extends `Classification`, coordinate.
- **T1.3 (A.8 auto-refresh)** — touches new script +
  `grandfather-catalog.md`. Zero overlap with A.2.
- **T1.4 (probe template upgrades)** — touches a process doc only.
  Zero overlap.
- **T1.5 (TOML/JSON convergence)** — touches `cat3_numerical/` files
  only. Zero overlap with A.2 (which targets cat2_contracts / __init__.py /
  classifier).

**Cleanest parallel composition with A.2:** T1.3 + T1.4 + T1.5. None
touches A.2's edit surface (`tools/integrity/integrity/__init__.py`,
`stack_d.py`, `public_symbol_used.py`, `grandfather.py`'s `classify()`).

**Requires coordination with A.2:** T1.1 (`classify()`-adjacent), T1.2
(`Classification` dataclass-adjacent).

## 9. Recommended v1.3 batch composition

This recommendation assumes A.2 lands first (currently in flight) and
v1.3 follows. Architect-2 review of this roadmap may change the
recommendation; this is architect-1's best read absent that review.

### 9.1 First v1.3 batch — "Classifier rules + structural cleanup"

Targets the highest-value T1 items that share scope cheaply and have
near-zero overlap with concurrent work:

| Commit | Item | Scope | Source |
|---|---|---|---|
| 1 | T1.2 — Convention H structural fix (`Classification.is_fallthrough`) | ~50 LOC | bolt-ons retro § 4.2 / § 5 |
| 2 | T1.1 — Three new classifier rules + catalog sections | ~140 LOC | retro § 6.1 item 4 |
| 3 | T1.3 — A.8 catalog auto-refresh | ~120 LOC | retro § 5.5 / § 6.1 item 6 |
| 4 | T1.4 — Probe template upgrades | ~30 LOC | retro § 7.2 C+D |
| 5 | T1.5 — TOML/JSON convergence | ~50 LOC | bolt-ons probe § C.4 |
| 6 | SHA back-fill | ~12 LOC | Convention #12 |

Six commits. ~400–500 LOC total. ~15–20 new tests. Estimated effort
comparable to v1.2 bolt-ons (4 substantive commits + back-fill landed
in ~one cycle).

**Rationale for the ordering:**

- T1.2 first so subsequent classifier additions (T1.1) can immediately
  use `is_fallthrough` rather than hardcoding category-string matches.
  Mirrors v1.2's "land sweep-side protection before check-side
  expansion" pattern (Convention G).
- T1.1 second so T1.3's auto-refresh has the new categories in scope
  on first run.
- T1.3 third — depends on T1.1 to demonstrate value but doesn't strictly
  require it.
- T1.4 fourth — process doc only; placed late to avoid blocking the
  more substantive commits.
- T1.5 fifth — cleanup; independent.
- T1.6 = SHA back-fill per Convention #12.

### 9.2 Second v1.3 batch — "Design-decision items"

After the first batch lands, T2 items become tractable. Likely
sequencing:

- T2.1 (sweep enforcement) — if architect-2 reviewed and settled
  soft/medium/hard, land the chosen level. If still unreviewed, defer.
- T2.2 (audit-prose freshness mechanization) — larger scope; could be
  its own batch.
- T2.3 (A.6 Stack C optimization) — independent of others; could be a
  micro-batch any time.

Picking among these depends on architect-2 review timing. If architect-2
review lands, T2.1 first (smallest, settled-by-review). If not, T2.3
(no dependency on review).

### 9.3 Why not bigger first batch

The v1.2 bolt-ons batch was scoped to ~5 commits / 4 substantive items.
That fit one cycle cleanly. Pushing T1's 5 items + T2.1 + T2.3 into
one batch is theoretically possible but historical evidence (v1.1
batch-1 was 8 spec items, 9 landing commits, and the retro lists
three structural complaints about the scope-vs-attention trade) argues
against it. The 6-commit first batch above stays within the bolt-ons
size band.

## 10. Open meta-questions

Items that are not v1.3 candidates themselves but inform how the v1.3
cycle is run:

1. **A.2 probe banked observations.** When the in-flight A.2 probe
   lands, it will likely add 2–5 banked observations to this roadmap.
   Recommend a § 11 addendum to this document at that point rather
   than re-drafting.
2. **Architect-2 review backlog flush.** T3.1 through T3.4 are all
   waiting on the same architect-2 attention. Worth surfacing this as
   a coordination question for the next architect-2 scheduling round.
3. **Quantum-sim cat3 expansion.** If a quantum sim phase lands and
   wants cat3 checks alongside, the four candidates from
   `docs/category-contexts/quantum.md` § 6.1 promote from HORIZON to
   T1-shaped. Worth keeping on the radar; not action-needed now.
4. **Spec line-budget overrun (v1.2 bolt-ons retro § 5 item 5).** The
   spec format is bloating as the toolkit grows. The proposed
   compression (more cross-references, fewer verbatim dumps) is itself
   a v1.3-shaped item but bank as meta because it changes spec-writing
   process, not toolkit code. Could be tackled as a sibling effort to
   the conventions-doc home decision (T3.2).

## 11. Closing

Active v1.x candidate pool after this roadmap consolidation: 11 items
across three tracking tiers (T1 / T2 / T3), plus 8 horizon items, plus
4 meta items. The roadmap is architect-1's best inventory; it does not
substitute for architect-2 review on the items flagged T3.

The recommended v1.3 first batch (§ 9.1) is six commits, ~400–500 LOC,
and stays compose-safe with concurrent A.2 work on most surfaces. T1.1
(`grandfather.py` rule additions) and T1.2 (`Classification` dataclass
extension) have shared-file overlap with A.2 and need coordination if
the two batches run in parallel.

This roadmap will need an addendum once A.2's probe lands (the probe's
banked observations are not yet visible). At that point, this document
plus the A.2 probe's banked section plus any architect-2 review notes
become the input set for the next v1.3 spec-drafting cycle.

Architect-1 self-drafted; architect-2 review remains open across this
entire v1.x backlog.

## Addendum A — landing-time SHA correction (2026-05-15)

Appended at landing per Convention C (append-only). The pre-commit
freshness check 2 (cited-SHAs-resolve) flagged a single
mis-attribution in § 3's landed-inventory table.

- **Row corrected:** "v1.1 batch-1 commit 1 (`cat2.stub-label-stale`
  check)" — the row body cites `~f661ec4` to mark approximate-match.
- **Actual introducing commit:** `af248cf feat(integrity):
  cat2.stub-label-stale (v1.1 batch 1 commit 1)`.
- **What `f661ec4` actually is:** `feat(integrity): A.5 fenced-block
  awareness across cat1 checks (v1.1 batch 1 commit 2)` — i.e. the
  *next* batch-1 commit, not the stub-label-stale introducer.
- **Why kept as `~f661ec4` in the body:** the body was not silently
  edited per the freshness-check rule. Future readers should treat
  the § 3 row's `~f661ec4` as superseded by `af248cf` per this
  addendum.

No other SHA in § 3 was mis-attributed; the remaining ten cited
SHAs all resolve and match their described commits.

## End of v1.3 candidates roadmap
