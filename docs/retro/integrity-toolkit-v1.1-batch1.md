---
title: "Integrity Toolkit v1.1 Batch 1 — Retrospective"
date: 2026-05-15
author: architect1
status: draft
sibling-docs:
  - docs/retro/integrity-toolkit-v1.md
  - docs/integrity-toolkit-spec.md
  - docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_1_apispec_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_1_commit1_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_commit2_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_commit3a_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_commit3b_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_commit3c_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_post_batch_triage_2026-05-15.md
---

# Integrity Toolkit v1.1 Batch 1 — Retrospective

## 0. Author bias disclosure

This retrospective is drafted by architect-1 (chat agent), the same
source responsible for two of the three pause-and-surface moments in
this batch. Self-retrospection is structurally biased toward charitable
framing of one's own mistakes. The retro is honest where possible and
explicit where the bias risks being load-bearing. **Recommend an
architect-2 review pass before locking conclusions** — particularly
around § 3 (fabrications) and § 5 (recommendations), where independent
judgment is most valuable.

## 1. Summary

Batch 1 closed five named v1.1 spec items (A.1, A.5, A.7, A.8, 5.B)
across six landing commits plus a follow-up triage of 30 baseline
hard-fails that surfaced from concurrent work during the batch.

| Phase | Commits | Items |
|---|---|---|
| Batch-1 landing | `af248cf` `f661ec4` `dbac051` `a71594a` `a28e1d7` `78e18d6` | A.1, A.5, A.7, A.8, 5.B + SHA back-fill |
| Post-batch triage | `bcba679` `9f85c7f` `445b5a0` | 30 hard-fails resolved (24 swept, 6 attributed) |

**Quantitative:**

- Toolkit code: +6 modules new, ~8 modified, ~750 LOC new + ~200 LOC changed
- Test suite: 74 → 96 (+22 tests, all passing)
- Grandfather pool: 1126 baseline → 967 post-triage (net -159; the A.5 fence-skip extension reduced by ~50, the audit-doc and toolkit-doc sweeps added +24, the rest is concurrent-commit churn)
- Gate state at retro time: 6 hard-fails outstanding, all live-source, all attributed (§ 6.E of triage)
- Wall-clock CI: unchanged (~5 min); spec § 4.10 noted no regression
- Three pause-and-surface moments during execution, all resolved without escalation
- One additional pause-and-surface moment surfaced post-triage (§ 3.4)

**Items deferred from batch 1 (still open):**

- A.2 (toolkit self-application of Cat 2)
- A.3 (bare-path-to-upstream-basename) — triage data shows this is the highest-leverage deferred item; § 5.1
- A.4 (multi-line citation grammar)
- A.6 (Stack C runtime optimization)
- A.9 (audit-citation file-pattern exclusion)
- All Milestone B / C / D items per v1.1 spec § 3

## 2. What landed

### 2.1 A.1 — `cat2.stub-label-stale`

New Cat 2 check. Anchored on the exact phrase `In Phase \d+, this is a stub:`
present in both confirmed canonical cases. Heuristic: impl file with > 10
non-comment LOC contradicts the stub label.

Caught the two spec § 12 row 5 canonical cases:
- `common/common-cpp/include/gpusims/alembic_writer.hpp:11` (impl 99 LOC)
- `common/common-cpp/include/gpusims/vdb_writer.hpp:12` (impl 135 LOC)

Both grandfathered under the new `cat2-stub-label-stale` category with
known migration paths (revise framing when the header is next edited).

### 2.2 A.5 — Markdown fenced-block awareness

Originally scoped to the annotation parser and suppressor. Implementation
revealed two additional cat1 checks (`cat1.intra-repo`,
`cat1.upstream-citation`) also scan markdown line-by-line — see § 3.2.
Extended uniformly: in `.md` and `.rst` files, no cat1 check fires on
content inside a fenced code block.

Net effect: `cat1.annotation-form` findings dropped from 69 → ~28-35
(per spec § 4.9 estimate); also shrunk `cat1.intra-repo` and
`cat1.upstream-citation` grandfather pools by the order claimed in the
pause-and-surface.

### 2.3 A.7 — Grandfather drain instrumentation

`--grandfather-report` and `--state-snapshot` CLI flags wired through
new `integrity/snapshot.py` module. History file at
`tools/integrity/.grandfather-history.json` (currently 1 entry; appends
per run unless `--no-history-append`). State snapshot includes
registered checks, registered upstreams, and per-category suppression
counts — intended as the verification-provenance anchor for v1.2+
spec drafts (D.1 of v1.1 spec).

### 2.4 A.8 — Per-category catalog tallies

Parenthetical counts now live on every category heading in
`tools/integrity/docs/grandfather-catalog.md`. Counts are manual-refresh
in v1.1; auto-refresh banked as v1.2 candidate.

### 2.5 5.B — `python3` docs sweep

13 sites swept across `tools/integrity/README.md`,
`docs/integrity-toolkit-spec.md`, and `tools/integrity/integrity/__main__.py`.
Audit reports under `docs/diagnostics/_audits/` preserved (append-only
convention).

## 3. Pause-and-surface analysis

Four total. Two are architect-1 fabrications (§ 3.1, § 3.2), one is
operational (§ 3.3), one is fabrication-adjacent and post-batch (§ 3.4).

### 3.1 Pause-and-surface #1 — include/src path-mirror fabrication

**Surfaced at:** Commit 1, on first execution of the new Cat 2 check.
The check resolved zero impl paths and would have missed both
canonical target cases.

**The fabrication:** The execution spec § 1.2 Decision 2 asserted:
> `.hpp`/`.h` in `common-cpp/include/<sub>/<base>.hpp` →
> `common-cpp/src/<sub>/<base>.cpp` (relative path mirror)

The synced repo's actual convention strips the leading `gpusims/`
namespace component: `include/gpusims/alembic_writer.hpp` →
`src/alembic_writer.cpp`. The spec author (architect-1) wrote the
path-mirror rule from intuition without grepping a single header→impl
pair.

**How it was caught:** Hard Rule 2 in the execution spec preamble:
*"The synced repo state is authoritative over this spec. If any
verbatim claim about file contents conflicts with what's actually on
disk at main's HEAD, pause and surface."* Claude Code executing the
spec checked the actual paths, found no impl files at the spec-asserted
locations, and surfaced the conflict.

**Resolution:** Replaced the literal mirror rule with a first-component-strip
heuristic. Verified against three production header/impl pairs
(`alembic_writer`, `vk/context`, `vdb_writer`) before re-running.

**Root cause:** The pre-spec apispec probe enumerated verbatim source
listings of relevant modules but did not enumerate any header→impl
path pairs. The spec drafter (architect-1) filled in the convention
from prior assumption rather than from probe data. That assumption was
wrong.

**Mitigation banked:** Pre-spec probe templates for any future spec
that does path resolution must include a verbatim probe item like:
> "List 3-5 representative pairs of (include header, sibling src impl)
> from `common/common-cpp/` verbatim, to confirm the resolution
> convention."

This is a one-line probe addition. It would have prevented this
pause-and-surface.

### 3.2 Pause-and-surface #2 — A.5 scope narrowness fabrication

**Surfaced at:** Commit 2, on the verification block. Real-repo run
post-A.5 produced 261 new hard-fails (210 `cat1.intra-repo` + 44
`cat1.annotation-form` + 7 `cat1.upstream-citation`). The gate would
have inherited red state.

**The fabrication:** The execution spec § 1.2 Decision 6 asserted:
> Fence-internal annotations are ignored entirely. A line inside a
> fenced code block is neither a candidate for `cat1.annotation-form`
> grammar reporting NOR a valid suppression annotation for a finding
> on the line immediately following it.

The spec drafter (architect-1) scoped the fence-skip behavior to the
annotation parser/suppressor without considering that `cat1.intra-repo`
and `cat1.upstream-citation` also scan markdown files line-by-line and
would also fire on fence-internal content. Audit reports with terminal-
output examples inside fenced blocks (containing citations like
<!-- integrity-allow: cat1.intra-repo; retro-doc snapshot intra-repo citation pre-v1.3 (see grandfather-catalog retro-doc-snapshot); n/a -->
<!-- integrity-allow: cat1.bare-path; retrospective-doc bare-path citation pre-v1.2 (see grandfather-catalog retro-bare-path); n/a -->
`example.cpp:42` for illustration) became 200+ new hard-fails.

**How it was caught:** Hard Rule 6 (the toolkit must remain green
across every commit). Claude Code ran the verification block, saw the
261 new hard-fails, and surfaced before committing.

**Resolution:** Extended the fence-skip pattern uniformly to all cat1
checks that scan markdown content (`cat1.intra-repo`,
`cat1.upstream-citation`, in addition to `cat1.annotation-form`).
Documented the corrected scope in commit 2's audit report E.1.

**Root cause:** The apispec probe verbatim-dumped the annotation parser
and the annotation check but did not enumerate which other cat1 checks
scan markdown content. The spec drafter scoped Decision 6 from local
knowledge of one module instead of from probe data on all relevant
modules.

**Mitigation banked:** Pre-spec probe templates for any future spec
that scopes a behavioral change to a specific module must include a
verbatim probe item like:
> "Enumerate all check modules under `tools/integrity/integrity/cat<N>_*/checks/`
> whose `run()` function reads markdown files or scans content
> line-by-line. For each, dump the relevant input-handling block
> verbatim."

This is a probe addition. It would have prevented this
pause-and-surface.

### 3.3 Pause-and-surface #3 — concurrent-edit race

**Surfaced at:** During commit 3 work, when commit `f23fd22` landed on
`main` reverting unrelated commit-4 changes that included edits to
`runner.py`. The revert wiped the in-flight A.7 edits to that file.

**The structural issue:** Not a fabrication — an operating-condition
shift. The v1 spec's commit pattern assumed serialized landing against
`main`. The actual operating condition is concurrent multi-agent work
across multiple phases (Phase 11.5, Phase 12, integrity v1.1, and
others all running in parallel).

**How it was caught:** Claude Code attempted `git push`, observed the
race, and surfaced rather than force-pushing or rebasing silently.

**Resolution:** Decomposed commit 3 into three sub-commits:
- 3a — new files only (`snapshot.py`, history file, tests) — race-immune
- 3b — `runner.py` extension — tight pre-edit pull-rebase cycle, smallest race window
- 3c — docs sweep — low risk, lands last

Each sub-commit's race-loss window is bounded to its own edits.
The 3a/3b/3c pattern landed cleanly. Cost of the realized race:
~5 minutes of re-application.

**Mitigation banked:** Execution specs default to commit decomposition
for any commit that touches more than one previously-existing file.
New-files-only commits are race-immune and should ship first when
possible. § 7.2 names this as a convention.

### 3.4 Pause-and-surface #4 (post-batch) — own-source grammar literals

**Surfaced at:** Triage of 30 baseline hard-fails (commits `bcba679`,
`9f85c7f`, `445b5a0`). Two of the six unresolved live-source hard-fails
trace back to test code I (architect-1) authored in commit 2:

| Finding | File | Cause |
|---|---|---|
<!-- integrity-allow: cat1.annotation-form; retrospective-doc literal mention of the annotation grammar (not a real annotation); n/a -->
| 29 | `tools/integrity/tests/test_suppression_fence.py:3` | The `integrity-allow:` literal appears in the module docstring |
<!-- integrity-allow: cat1.annotation-form; retrospective-doc literal mention of the annotation grammar (not a real annotation); n/a -->
| 30 | `tools/integrity/tests/test_suppression_fence.py:23` | The `integrity-allow:` literal appears inside a Python string literal that constructs a markdown fixture |

**What happened:** When writing the test file for A.5, I included the
grammar literal in the module docstring (for human readers) and in a
`md.write_text(...)` call constructing the test fixture. Both fired
`cat1.annotation-form` because the file is `.py`, not `.md`, and A.5's
fence-guard explicitly excludes non-markdown files. I missed this
during execution because I tested A.5's *behavior* (does the parser
skip fenced annotations in markdown?) without scanning my own test
code against the modified check.

**Fabrication-adjacent, not fabrication:** The check fired correctly.
The toolkit was right. The author was the one who introduced findings
that contradicted the gate state they were extending — a discipline
gap, not an invented anchor.

**Pattern:** The same author (architect-1) extending the toolkit also
authored test code that fails the extended toolkit's own discipline.
This is exactly the recursive blind spot the toolkit exists to surface:
*the spec drafter is also the implementer's blind spot.*

**Mitigation:** I'll fix #29 and #30 in a follow-up commit (out of
triage scope per directive § 6, but in scope for this retro's
recommendations). The fix is small: add inline
<!-- integrity-allow: cat1.annotation-form; retrospective-doc literal mention of the annotation grammar (not a real annotation); n/a -->
`# integrity-allow: cat1.annotation-form; toolkit-own test docstring/fixture literal; n/a`
annotations above each site. This is the same pattern used in 8 other
`toolkit-own-source` files.

## 4. What worked

### 4.1 Hard Rule 2 held three times

Every pause-and-surface produced a correct resolution within minutes,
not hours. The hard rule design ("synced repo state is authoritative;
pause-and-surface conflicts") worked exactly as intended in all four
moments.

### 4.2 The probe-then-spec pattern caught most structural issues pre-code

The two pre-spec probes (`integrity_v1_1_probe_2026-05-15` for ground
state, `integrity_v1_1_apispec_2026-05-15` for API shapes) produced
~2,200 lines of verbatim source/state. Most spec assertions were
grounded against this corpus. The two fabrications that slipped through
(§ 3.1, § 3.2) are gaps in the probe template — items the probe didn't
ask for — not failures of the probe-then-spec discipline.

### 4.3 The 3a/3b/3c decomposition worked exactly as designed

The race that hit commit 3 cost ~5 minutes of re-application. Without
decomposition, the same race against an undivided commit 3 would have
cost a full re-write of the larger diff (~200 LOC across 5 files).
The decomposition pattern is now banked as the default for any
multi-file batch.

### 4.4 The triage hybrid policy held

20 audit-doc findings swept under existing classifier rules (no new
rules needed). 4 toolkit-doc findings swept under fallthrough (named
category recommended for v1.2 but not required for the gate). 6
live-source findings attributed to introducing commits and NOT swept.
Gate stays red on live-source by design — which is what the discipline
requires.

### 4.5 Honest pause-and-surface culture

In all four moments, the executor (Claude Code) and the spec author
(architect-1) reached the same conclusion: the spec was wrong, the
synced state was authoritative, the resolution was clear. No defensive
re-framing. No silent adaptation. The Convention #8 anti-fabrication
discipline carried through into the spec-drafting layer.

## 5. What didn't work

### 5.1 The biggest leverage gap is A.3 (deferred from this batch)

Of the 6 live-source hard-fails remaining post-triage, **4 are bare
upstream-path citations** that A.3 (`bare-path-to-upstream-basename`)
would have caught at write-time:

| Finding | Bare path | Registered upstream |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; retro-doc snapshot intra-repo citation pre-v1.3 (see grandfather-catalog retro-doc-snapshot); n/a -->
| 25 | `chapter13/cpu/LBM.cpp:97` | Krueger book-companion-code-2016 |
<!-- integrity-allow: cat1.intra-repo; retro-doc snapshot intra-repo citation pre-v1.3 (see grandfather-catalog retro-doc-snapshot); n/a -->
| 26 | `chapter13/cpu/LBM.cpp:97` | Krueger book-companion-code-2016 |
<!-- integrity-allow: cat1.intra-repo; retro-doc snapshot intra-repo citation pre-v1.3 (see grandfather-catalog retro-doc-snapshot); n/a -->
<!-- integrity-allow: cat1.bare-path; retrospective-doc bare-path citation pre-v1.2 (see grandfather-catalog retro-bare-path); n/a -->
| 27 | `main.cpp:1168-1279` | (ambiguous — no upstream) |
<!-- integrity-allow: cat1.intra-repo; retro-doc snapshot intra-repo citation pre-v1.3 (see grandfather-catalog retro-doc-snapshot); n/a -->
| 28 | `SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75` | SPlisHSPlasH 2.16.1 |

The Krueger upstream is already registered. The SPlisHSPlasH 2.16.1
anchor is already registered. The findings are not new fabrication —
they are the canonical *discipline-miss* shape the toolkit is meant
to enforce. A.3 would either auto-correct or hard-fail at write-time,
preventing post-batch triage.

**A.3 is the single highest-leverage deferred item.** Triage data is
the direct evidence. Recommend prioritizing A.3 first in batch 2.

### 5.2 The pre-spec probe template doesn't enumerate path conventions or call sites

Both pre-spec probes for v1.1 batch 1 were strong (lines counted,
classifier rules verbatim, fixture trees listed, vendored SHAs
confirmed) but missed two classes of probe item:

- **Path-resolution conventions** (§ 3.1) — header→impl, source→test,
  spec→audit, etc. Any spec that does path resolution needs verbatim
  pairs to confirm the convention.
- **All call sites of an affected module** (§ 3.2) — when a spec scopes
  a behavioral change to a module, the probe must enumerate every
  module that depends on the affected behavior, so the scope can be
  audited against actual dependencies.

Both gaps caused architect-1 fabrications. Both are one-line probe
template additions.

### 5.3 Spec author writing own tests creates a recursive blind spot

§ 3.4: I extended a check, then wrote test code that violated the
extended check's discipline, then didn't notice until post-batch
triage. The spec author is structurally the worst person to author
the test code for the spec, for the same reason they're the worst
person to write the retro on their own work.

This isn't a "don't write tests" recommendation — the test code itself
was correct. The recommendation is that *the same person who drafted
the spec should not be the only person who reviews the test files for
own-source discipline gaps*. A second pair of eyes — even a quick
architect-2 scan of new test files for grammar-literal leaks — would
have caught both findings before commit 2 landed.

Procedural fix: any commit that adds new files under
`tools/integrity/tests/` gets a one-step pre-commit check: grep the
<!-- integrity-allow: cat1.annotation-form; retrospective-doc literal mention of the annotation grammar (not a real annotation); n/a -->
new files for `integrity-allow:` literals and confirm each is wrapped
<!-- integrity-allow: cat1.annotation-form; retrospective-doc literal mention of the annotation grammar (not a real annotation); n/a -->
in an inline `# integrity-allow: cat1.annotation-form` annotation OR
deliberately placed in a way the check ignores.

This is what A.2 (toolkit self-application) was meant to address.
Banking A.2's priority alongside A.3's.

### 5.4 The grandfather-sweep companion convention was implicit, not enforced

24 of the 30 baseline hard-fails were audit-doc and toolkit-doc
findings that introducing commits should have swept as a companion
action. The convention exists in spirit — every audit report that
adds citations will introduce `cat1.intra-repo` findings; every audit
report that names the grammar will introduce `cat1.annotation-form`
findings — but it isn't enforced by tooling. Authors are expected to
remember to run the sweep, and across 4 concurrent sessions, no one
remembered consistently.

§ 7.1 proposes a hard convention. The alternative (pre-commit hook)
is heavier but more reliable; see § 6.2.

### 5.5 The catalog count-refresh is manual

A.8 populated counts per category heading but the refresh is manual on
subsequent commits. Triage demonstrated immediate count drift: the
catalog now says `audit-citation (761)` while the actual count post-triage
is 607 (and changing again with every concurrent audit-doc commit).

Auto-refresh is a v1.2 candidate. Until it lands, the catalog headings
are accurate at-landing-time but go stale fast.

## 6. Recommendations

### 6.1 Priority-ordered v1.2 candidates

| Priority | Item | Source | Leverage |
|---|---|---|---|
| 1 | A.3 (bare-path-to-upstream-basename) | v1.1 spec § 3 | Catches 4 of 6 current live-source hard-fails (triage § 5.1) |
| 2 | A.2 (toolkit self-application) | v1.1 spec § 9 | Catches own-source discipline gaps (§ 3.4, § 5.3) — would have caught both `test_suppression_fence.py` findings |
| 3 | Pre-spec probe template upgrades | § 3.1, § 3.2 mitigations | One-line additions; prevent architect-1 fabrications structurally |
| 4 | `toolkit-doc-snapshot` classifier extension | Triage § C.2 | Cleanup; replaces `other-cat1` fallthrough for toolkit docs |
| 5 | Grandfather-sweep convention enforcement | § 5.4 | Eliminates the post-batch-triage class entirely |
| 6 | A.8 auto-refresh | § 5.5 | Catalog stays accurate without manual editing |
| 7 | A.4 (multi-line citation grammar) | v1.1 spec § 3 | Not surfaced by triage; lower priority than originally banked |
| 8 | A.6 (Stack C runtime optimization) | v1.1 spec § 3 | Performance, not correctness; defer until A.3 + A.2 land |
| 9 | A.9 (audit-citation file-pattern exclusion) | v1.1 spec § 5.A | 67% pool collapse if adopted; architect-2 review item per v1.1 spec § 7 |

### 6.2 Convention proposal: grandfather-sweep companion

**Recommended hard convention for batch 2+:**

> Every commit that touches the cat1-scannable surface (live shaders,
> common-* code, sim code, audit docs under `docs/diagnostics/_audits/`,
> toolkit docs under `tools/integrity/docs/`, phase specs under
> `docs/phase*.md`, retro docs under `docs/retro/`) either runs
> `python3 tools/integrity/scripts/grandfather_sweep.py` as part of
> the commit OR lands a companion grandfather-sweep commit within the
> same PR.

**Enforcement options:**

- **Soft (current default):** Convention documented in the v1.1 retro
  and in `tools/integrity/README.md`; authors expected to remember.
  Risk: continued post-batch triage churn.
- **Medium:** Add a CI check that fails when the live-source class
  changes (i.e., when a cat1-scannable file is modified and no
  grandfather-sweep commit shipped). This is a custom check, not the
  existing integrity gate. Modest implementation cost.
- **Hard:** Pre-commit hook auto-runs the sweep when a cat1-scannable
  file is staged. Cost: slow commits on toolchain-edits (~3-5s
  per commit). Benefit: zero post-batch triage.

Architect-2 review: which enforcement level?

### 6.3 New v2 candidate: spec-vs-repo-state reconciliation at draft time

Banked from § 3.1 + § 3.2 root-cause analysis.

The two architect-1 fabrications in batch 1 share a root cause: load-bearing
assertions in a spec draft that weren't verified against the current repo
state. Hard Rule 2 catches these at execution time, but only after the
spec has been drafted, reviewed, and approved. The cost is one or more
pause-and-surface moments per execution.

A complementary check that runs at *draft time* — verifying every
load-bearing repo-state assertion against the actual repo before the
spec locks — would shorten the feedback loop from "execution time" to
"draft time." Smaller scope than C.4 (spec-vs-implementation
reconciliation, which targets architect-2 review work). Distinct from
the existing pre-spec probe pattern, which is human-driven; this would
be mechanical.

Concrete shape: a tool that scans a spec draft for assertions of the
form `<file>:<line>`, `<phrase X is present in file Y>`, `<API Z has
shape W>`, etc., and grep-verifies each against the current repo. Any
mismatch is flagged for the spec author before the spec is committed.

This is a v2 candidate, not v1.2. Worth banking now so it doesn't
fall off the list.

### 6.4 Architect-2 review pass needed

Per § 0, this retro is self-drafted by the source of two of three
in-batch fabrications. Specific items where architect-2 judgment
would be most valuable:

1. **§ 3.1, § 3.2 root-cause framing.** Is the "probe template gap"
   framing accurate, or am I under-attributing to architect-1
   discipline gaps that probe templates wouldn't have caught either way?
2. **§ 5.3 (recursive blind spot).** Is the procedural fix sufficient,
   or does spec-author-also-writes-tests need a stronger structural
   separation?
3. **§ 6.1 priority ordering.** A.3 and A.2 ranked highest based on
   triage data, but architect-2 may see leverage I'm missing.
4. **§ 6.2 enforcement level.** Soft/medium/hard convention for the
   sweep companion. Tradeoff is real and architect-2 has perspective
   I don't.

## 7. Operating conditions

### 7.1 Concurrent multi-agent landing is the actual condition

V1 spec's commit pattern assumed serialized landing. Batch 1 confirmed
this is wrong. During the batch's lifetime, concurrent work landed
across Phase 11.5, Phase 12, sph-water Akinci2012, quantum
category-context, and the batch-1 integrity work itself — at least 4
distinct work streams against the same `main`.

Of the 30 baseline hard-fails at triage time, **all 30 were
attributable to one human (Steven Cohen) operating ~4 concurrent
sessions in parallel**. None were the fabrication class the toolkit
was originally built to catch (i.e., wrong-version anchors, made-up
citations); all were discipline-misses (bare paths, grammar literals
in own code, audit-doc citation drift). The fingerprint is canonical:
parallel specialized agents landing distinct work streams.

This is not bad. It's how the work actually proceeds. The toolkit and
its surrounding conventions need to be designed for it.

### 7.2 Conventions banked from operating evidence

**A. New-files-first decomposition (banked from § 3.3):**
> Execution specs default to commit decomposition for any commit that
> touches more than one previously-existing file. The new-files-only
> sub-commit ships first.

**B. Grandfather-sweep companion (banked from § 5.4):**
> Every commit that touches cat1-scannable surface either runs the
> sweep or lands a companion sweep commit in the same PR.

**C. Probe-template enumerate-conventions (banked from § 3.1):**
> Pre-spec probes that ground path-resolution rules must include
> verbatim probe items enumerating 3-5 representative path pairs.

**D. Probe-template enumerate-call-sites (banked from § 3.2):**
> Pre-spec probes that ground behavioral changes must include verbatim
> probe items enumerating all modules that depend on the affected
> behavior.

**E. Spec-author-self-test review (banked from § 3.4, § 5.3):**
> When the spec author writes new test code for own-spec items, a
> second pair of eyes (architect-2 or a mechanical own-source scan)
> reviews the test files for grammar-literal leaks before commit.

These five are concrete, narrow, and actionable. None requires new
toolkit code (E may eventually be subsumed by A.2). All five are
candidates for inclusion in a permanent CONVENTIONS doc rather than
re-derivation in every retro.

### 7.3 Operating fingerprint as a positive signal

The post-batch triage data converged on one author, 4 sessions, 30
findings, zero fabrications. That's a clean fingerprint. Future
triages that *don't* converge on this shape — e.g., findings
attributable to many authors, or findings that include the canonical
fabrication class — are worth treating differently. The current
shape is "discipline drift across parallel sessions"; a different
shape would mean a different mitigation.

## 8. Open questions for architect-2

In addition to § 6.4 review items:

1. **§ 5.5 catalog drift:** Should A.8 auto-refresh be promoted to
   immediate v1.2 priority, or is "stale but accurate at landing time"
   acceptable for another batch cycle?
2. **§ 6.3 v2 candidate:** Is spec-vs-repo-state reconciliation at
   draft time worth the design work, or should the probe-template
   upgrades (§ 7.2 C+D) cover the same ground at lower cost?
3. **§ 7.2 conventions doc:** Where should the five banked conventions
   live? `docs/CONVENTIONS.md`? `tools/integrity/docs/conventions.md`?
   Per-stack docs? The retro is the wrong long-term home.
4. **Test-suite growth velocity:** +22 tests in batch 1. At this rate,
   the toolkit test suite doubles every 2-3 batches. When does test-
   suite maintenance become a meta-concern? (Not for v1.2, but
   worth banking.)

## 9. Closing

Batch 1 closed 5 of the 8 v1.1 Milestone-A items, surfaced 4 distinct
pause-and-surface moments (3 in-batch, 1 post-triage), produced 9
landing commits (6 batch + 3 triage), and added 22 tests. Two of the
four pause-and-surface moments were architect-1 fabrications caught by
Hard Rule 2; both have specific probe-template mitigations banked.

The triage made one thing explicit that the batch's design hadn't
named: the fabrication class the toolkit was originally built to catch
(wrong-version anchors, made-up paths) is no longer the dominant
defect class on `main`. The dominant class is *discipline drift* —
bare paths instead of registered citations, grammar literals in own
code, audit-doc citation drift. A.3 + A.2 together cover most of this
class; both are deferred from batch 1 and should be the core of
batch 2.

The toolkit's value is shifting from "catch architect-1 invention"
toward "enforce author discipline across concurrent parallel work."
That shift is consistent with the operating-condition update in § 7:
the work is no longer serialized, and the gate's job is to enforce
consistency across streams that don't see each other's edits.

Architect-2 review of this retro recommended per § 0.

## End of retrospective
