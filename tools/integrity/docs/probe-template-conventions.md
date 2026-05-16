# Probe Template Conventions

This doc records the probe-template conventions banked by v1.1
batch-1 retro § 7.2 as items C and D. Both conventions emerged from
real architect-1 fabrications during v1.1 batch-1 execution
(pause-and-surface #1 and #2). They were carried forward into the
v1.2 A.3 / A.2 probe designs and validated. This doc is the canonical
home for the convention text plus worked examples; the v1.1 batch-1
retro § 7.2 framing remains the originating record.

The conventions are scoped to integrity-toolkit pre-spec probes
specifically. They are useful elsewhere, but the worked-example
evidence is integrity-toolkit-internal.

---

## Convention C — Path-resolution enumeration

**When the spec proposes to do path resolution (mapping one path
shape to another, e.g. header → impl, or basename → repo-resolved
path), the pre-spec probe MUST enumerate three to five representative
input-output pairs from the synced repo state, drawn from the actual
code or directory layout that will inform the spec.**

The probe's job here is to make the path-resolution rule discoverable
from probe data rather than from architect-1 prior assumption.

### Failure mode this prevents

The v1.1 batch-1 commit-1 `stub_label_stale.py` check landed with
Decision 2 asserting a 1:1 mirror between `include/<sub>/<base>.hpp`
and `src/<sub>/<base>.cpp`. Synced repo state had
`include/gpusims/...` stripping the `gpusims/` namespace component in
the `src` tree — i.e., `include/gpusims/alembic_writer.hpp` maps to
`src/alembic_writer.cpp`, not `src/gpusims/alembic_writer.cpp`. The
check's first execution attempt resolved zero impl paths and would
have missed both canonical target cases. The fabrication was caught
at execution time (Hard Rule 2) but only after the spec had been
drafted, reviewed, and approved.

The pre-spec apispec probe enumerated verbatim source listings of
relevant modules but did not enumerate any header→impl path pairs.
The spec drafter (architect-1) filled in the convention from prior
assumption rather than from probe data. That assumption was wrong.

(Source: `docs/diagnostics/_audits/integrity_v1_1_commit1_landing_2026-05-15.md`
§ E.1 + v1.1 batch-1 retro § 3.1.)

### Examples of Convention C followed

**Example 1 — A.3 probe** (path-resolution rules for bare basenames).
`docs/diagnostics/_audits/integrity_v1_2_a3_probe_2026-05-15_architect1.md`
§ A.3 verbatim-dumps `cat1_citations/resolver.py` (the existing
intra-repo path resolver) and includes an INFERENCE block explicitly
naming the false-positive class the resolver accidentally
accommodates (it succeeds on bare basenames whenever a sibling file
matches). The probe-time enumeration is precisely what Convention C
asks for: the path-resolution rules dumped verbatim from the source
under inspection, before the spec drafter writes new convention text.

**Example 2 — A.2 probe** (toolkit internal cross-module imports).
`docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md`
§ C.1 verbatim-enumerates every `from integrity.X import Y` edge in
the toolkit's internal cross-module graph. This is the toolkit's
analog of path-pair enumeration: every consumer-of-X edge is dumped,
so the spec drafter cannot fabricate an import path that doesn't
exist. The A.2 probe's analog of the v1.1 commit-1 fabrication would
have been asserting that `cat2_contracts/checks/foo.py` exists when
it doesn't; C.1's enumeration directly forecloses that class.

---

## Convention D — Call-site enumeration

**When the spec proposes a behavioral change to a function, method,
or module-level helper, the pre-spec probe MUST enumerate every call
site of the changed item in the synced repo state, with verbatim
context for each call site sufficient to evaluate whether the
behavioral change is compatible with that call site's expectations.**

The probe's job here is to make the change's blast radius
discoverable from probe data rather than from architect-1 local
knowledge of "the obvious caller."

### Failure mode this prevents

The v1.1 batch-1 commit-2 `cat1.annotation-form` check landed with
Decision 6 scoping the markdown-content scan to a single check
module. Other cat1 checks (intra-repo, upstream-citation) also scan
markdown files; the scope was too narrow and the check missed
findings the spec author hadn't realized were in scope.

The apispec probe verbatim-dumped the annotation parser and the
annotation check but did not enumerate which other cat1 checks scan
markdown content. The spec drafter scoped Decision 6 from local
knowledge of one module instead of from probe data on all relevant
modules.

(Source: v1.1 batch-1 retro § 3.2.)

### Examples of Convention D followed

**Example 1 — v1.2 bolt-ons probe** (emit_output asymmetry).
`docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md`
§ D.1 enumerates `emit_output` and `_emit_human_summary` declarations
plus all callers in `runner.py`. § D.2 then dumps both functions
verbatim, proving the asymmetry between the github branch (which
filtered suppressed findings) and the human branch (which did not).
This is exactly what Convention D asks for: enumerate every call
site of the function the spec proposes to modify, before writing the
fix. Line numbers in the bolt-ons probe's § D.1 are stale relative
to current disk (the bolt-ons fix added 6 lines); the
methodology survives the line-number drift, the specific citations
do not.

**Example 2 — A.2 probe** (internal + external consumer enumeration).
`docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md`
§ C.1 enumerates every internal import edge in the toolkit. § C.2
enumerates external consumers (scripts + tests + docs), with an
INFERENCE block distinguishing real consumers from fenced-code-listing
pseudo-consumers. The dual enumeration is what kept the A.2 spec from
fabricating a nonexistent caller in its design discussion: every
caller's actual import shape was visible in the probe before the
spec wrote new design text.

---

## How to apply these conventions

When you author a pre-spec probe:

1. **Audit the spec's expected behavioral changes.** For each
   proposed change to path-resolution logic, add a probe section
   enumerating 3–5 representative path pairs from the synced repo
   (Convention C). For each proposed change to a function's
   behavior, add a probe section enumerating every call site
   (Convention D).
2. **Dump verbatim.** Both conventions are explicit: dump the source,
   don't paraphrase. Paraphrase introduces a translation layer that
   can leak architect-1 prior assumption back in.
3. **Tag with anchor SHA.** Line numbers drift over time; the
   probe-time anchor SHA is the load-bearing claim. Worked examples
   in this doc deliberately omit line numbers from the bolt-ons
   probe's § D.1 because those numbers are now stale; the
   methodology is what transfers.
4. **Treat as required, not optional.** Both conventions exist
   because their absence caused real architect-1 fabrications that
   pause-and-surface caught at execution time. Skipping them shifts
   detection from draft-time to execution-time and costs
   pause-and-surface cycles.

## Related conventions

- **Convention A** (new-files-first commit decomposition) — v1.1
  batch-1 retro § 7.2.
- **Convention B** (grandfather-sweep companion) — v1.1 batch-1
  retro § 7.2.
- **Convention E** (spec-author-self-test review) — v1.1 batch-1
  retro § 7.2.
- **Convention F** (audit-prose freshness) — v1.1 batch-1 post-retro
  audit § D.2.1.
- **Convention G** (sweep-side protection lands before check-side
  scope expansion) — v1.2 bolt-ons retro § 4.1.
- **Convention H** (filter rules query properties, not literals) —
  v1.2 bolt-ons retro § 4.2.
- **Convention I** (cross-batch scope discipline) — v1.2 bolt-ons
  retro § 4.3.

A permanent CONVENTIONS-doc home for the full set is a v1.3 candidate
(roadmap § 5 T3.2); until that lands, conventions live in the retros
that bank them, with worked examples in audit reports.
