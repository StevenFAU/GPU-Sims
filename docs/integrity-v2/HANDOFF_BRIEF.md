# Handoff Brief — Integrity Toolkit v2

**Date:** 2026-05-17
**From:** Prior chat session (rev 1-12 of monolithic design doc → split into two-doc structure; **rev 13 first-principles reframe and industry-standards-alignment pass added same day**)
**To:** Fresh chat session
**Read time:** 5 min for this brief, then open the two docs.

---

## ✅ Rev 17 — HANDOFF-READY (read this first; rev 16 supplement below)

> **Sign-off:** spec is ready for the coordinator chat. All 12
> pre-handoff probes executed; all 17 BBB-flagged batch backfills
> landed; all 12 rev-16 remaining-work items cleared. With § 3.17.A
> defaults applied retroactively, **0 batches at GAP** per the § 3.17
> contract (all 59 are READY or CLOSE).

### What landed this rev (rev 17)

- **§ 0.4 CI baseline at HEAD** — 7 closeout commits (4–8 of v1
  closeout) acknowledged as landed precedent (paired-sweep enforcement,
  Cat 3 Stack C driver build, SPlisHSPlasH SHA pin, Stack D isolation
  smoke, Stack C Release+Debug matrix, concurrency-group cancellation,
  structure.yml). Per Probe HHH § D.
- **§ 1.2 Cat 2 contract citations** — `stack_b.py` / `stack_c.py` /
  `stack_d.py` paths added; **`public_symbol_used_b.py` registers as
  `cat2.public-symbol-used-ts`** naming inconsistency surfaced.
  v2.33 absorbs the `stack_b.py → stack_ts.py` rename. Per Probe III.
- **§ 1.3.C resolutions** — mpm-multimaterial → Option C++ (4
  conservation-law checks; authorities Jiang 2015 + Hu 2018, not
  Stomakhin 2013); strange-attractors → BIFURCATE (2 Cat 3 +
  Cat 4 Lyapunov deferred); GGG-output placeholder replaced with
  pointer to 36-row enumeration at GGG § A.
- **§ 3.17.A Field defaults + § 3.17.A.4 Drives-sim default** —
  shifts 0/0/59 GAP → ~12/18/29 READY/CLOSE/GAP without per-batch
  work; collapses ~34 critical-interlock MISSING edges. Per BBB D.1
  + EEE F.1.
- **§ 4.0 Explicit dependency edges** — 13 edges declared explicitly
  (v2.28 ← v2.0, v2.41 ← v2.0, v2.38 ← v2.31, v2.42 ← v2.31, v2.27 ←
  v2.43, v2.53a.1 ← v2.4, v2.50 ← v2.8 [resolved HARD], v2.36 ←
  v2.5/6/7/8, v2.46 ← v2.45, v2.14 ← v2.13, v2.16 ← v2.13). Per
  EEE D.1/D.2/D.4. Machine-readable enumeration at
  `tools/integrity/docs/dep-graph/v2-deps.tsv` (53 rows).
- **§ 4 sequencing intro — QQ note** — 17 rev-10 § 3.14 defects
  remain unfixed at HEAD; "cleanup landed" framing refuted; defects
  route through v2.0/v2.1/v2.3 + Phases 1/7. Per Probe JJJ § D.1.
- **17 BBB-flagged batches backfilled** — v2.5, v2.7, v2.9, v2.10,
  v2.16, v2.20/21/22 (Cat 4 portfolio sub-batches), v2.25/26/27 (Cat
  5 check IDs), v2.32, v2.34 (46 rule IDs + INDEX schema), v2.44
  (per-stack headless smoke), v2.49 (non-trivial rollback),
  v2.53a.2 / v2.53b (Status: BLOCKED markers).
- **CCC wrong-letter cleanup completed** — Convention K → M rename
  landed across audit-trail § 2.1 + § 2.4 + 7 citation sites; v2.0
  commit 10 banks Convention M in `conventions.md`. Convention I
  attribution stricken at 3 wrong-letter sites (1 site at line
  2378 is CORRECT — left alone). Convention H replaced with
  "live-source-stays-red discipline" phrase at 2 sites.
- **DDD prefix-typo sweep** — 18 sites fixed (6× `common/audit_log.py`
  + `cat3_numerical/d3q19_verify.py` + `cat1_citations/checks/annotation.py`
  + 7 sim-side paths + 2 `test_kernels.py` disambiguations + LBM
  `src/main.cpp` + `sbom.spdx.json` forward-looking marker).
- **II three LOW polish edits** — Reynolds 1987 subtitle complete,
  Halvorsen "(community-attributed; no primary publication)"
  disclosure, RD-3D "(extended to 3D)" qualifier.
- **v2.18 budget note** per FFF — sph-water main.cpp is 3164 LOC
  (~3× RD-3D baseline assumed by adjacent Cat 4 adoption batches);
  commit shape adjusted to 100-200 LOC each.
- **v2.0 expanded 11 → 12 commits** to absorb the Convention K→M
  rename + bank.
- **Banking rows 96–97** added to audit-trail.

### Handoff state summary

| Surface | State | Notes |
|---|---|---|
| Spec at `docs/integrity-v2/toolkit-spec.md` | LANDED (3925+ lines, rev 17) | All probe outcomes absorbed |
| Audit-trail at `docs/integrity-v2/audit-trail.md` | LANDED | 97 banking rows + 22 probe registry rows |
| Probe charters at `docs/integrity-v2/charters/` | LANDED | 20 charters covering YY through JJJ |
| Probe reports at `docs/diagnostics/_audits/probe_*_report_2026-05-17.md` | LANDED | 12 pre-handoff probes executed |
| `tools/integrity/docs/dep-graph/v2-deps.tsv` | LANDED via Probe EEE | 53-row dep-graph artifact |
| `tools/integrity/docs/dep-graph/v2-deps.dot` | LANDED via Probe EEE | DOT visualization |
| § 3.17 batch contract | DEFINED + defaults landed | Coordinator chat ingests § 3.17.A defaults at dispatch |
| § 3.18 landing pipeline | DEFINED | 8 phases pre-handoff → v2-complete |
| Coordinator-chat dispatch plan | NOT YET BUILT — first action for coordinator chat | Build from § 4.0 dep-graph + Drives-sim default |
| Landing ledger at `docs/integrity-v2/landing-ledger.md` | NOT YET CREATED — coordinator chat creates on first dispatch | |
| Probe EE re-dispatch | DEFERRED — trigger condition met (spec landed) | Harness at probe_EE_report § 8 ready |

### What the coordinator chat does on session 1

1. **Ingest** `docs/integrity-v2/toolkit-spec.md` + `audit-trail.md` +
   `HANDOFF_BRIEF.md` in a fresh session.
2. **Build dispatch plan** — parse § 4 batch contracts, apply § 3.17.A
   defaults to populate missing fields, apply Drives-sim default to
   inflate dep edges, generate topological order from § 4.0.
3. **Dispatch Probe EE** (with the harness in its report § 8) to
   produce `tools/integrity/docs/touch-matrix/v2-matrix.tsv` —
   completes the parallelism map.
4. **Report dispatch plan to Steven** for sign-off before first agent
   spawn.
5. **Pre-spawn snapshot** at HEAD — capture toolkit-output JSON +
   pytest log per § 3.18 Phase C.
6. **Dispatch Phase 0**: v2.-1 (parallelize cat2.public-symbol-used-c).
   Serial — first batch.
7. **Iterate** through Phase 1 onward per landing pipeline.

### What's NOT in the spec by design

- **Probe EE output (the touch-matrix TSV)** — coordinator chat
  produces this on session 1; cannot be authored pre-handoff.
- **Landing ledger** — coordinator chat creates and maintains.
- **Per-batch SHA back-fills** — Convention #12, post-merge follow-up
  commits.
- **The 14 explicit-prose MISSING dep edges from EEE D.3 Cat 4
  fan-out** — absorbed by the Drives-sim default rule, not enumerated
  individually. Re-tighten in coordinator chat if specific Cat 4
  adoption batches need tighter sequencing.

### Outstanding follow-ups the coordinator chat will surface

These are not blockers; they're items that will land naturally during
execution rather than pre-handoff:

- **EEE per-batch dep enumeration beyond the 13 explicit edges** —
  v2.20/21/22 sub-batch sub-edges; v2.45/46/47/48 SPH conformance
  chain. Coordinator chat enumerates when it builds the dispatch
  plan.
- **AAA `ground-truth-sources.md` schema 4→6 fields** — no App. A
  in current spec; deferred to v2.5 when the MMS harness lands and
  the schema becomes load-bearing.
- **JJJ HIGH-severity uncited audits beyond QQ** — TT/SS/WW/UU
  citations to be added in v2.0 / v2.34 prep cycles by the coordinator
  chat as it touches the relevant spec sections.

---



> **Rev 16 closeout — all 12 pre-handoff probes executed.** Wave 1
> (DD, EE, FF) executed mid-rev; Wave 1.5 (AAA, BBB, CCC, DDD, FFF,
> HHH, III, JJJ) executed in parallel; Wave 2 (II, EEE) + Wave 3 (JJ,
> GGG) executed in parallel after dispatch.
>
> **Spec absorbed:** § 3.17.A Field defaults (BBB; cuts contract
> backfill cost ~34h→~6h) + § 3.17.A.4 Drives-sim default (EEE;
> collapses ~34 MISSING dep edges) + § 1.3.C resolutions (GGG mpm
> Option C++ with Jiang 2015/Hu 2018 authorities; GGG strange-
> attractors BIFURCATE; pointer to 36-row enumeration at GGG § A) +
> CCC wrong-letter fixes (Convention K→M rename via new v2.0 commit
> 10; Convention I + H attribution strikes) + v2.0 expanded to 12
> commits.
>
> **Probe verdict summary:**
> - DD / FF / III / FFF / HHH: NEW BASELINE — measured FACTs landed.
> - BBB / EEE: HANDOFF GATES — single highest-leverage edits landed.
> - GGG: SPEC-AUTHORING COMPLETE — 36-row enumeration; 2 open
>   questions resolved (mpm + strange-attractors).
> - II: CLEAN — 0 mis-citations across 14 rows.
> - JJ: NO SPEC DRIFT — closed Probe DD § E.2.
> - AAA / CCC / DDD / JJJ: REMEDIATION QUEUED — sweep work, not
>   blocking.
> - EE: BLOCKED → now re-dispatchable (spec landed at
>   `docs/integrity-v2/toolkit-spec.md`).

**Remaining pre-handoff work (mechanical sweeps + targeted backfills):**

1. **Per-batch contract backfill** for 17 named batches (BBB § D.4):
   v2.5 MMS API surface; v2.7 8-commit touches; v2.9/v2.10 shader
   paths; v2.16 mask config schema; v2.20-v2.22 Cat 4 per-sim
   sub-batches; v2.25/26/27 Cat 5 check IDs; v2.32/v2.34 per-rule doc
   pages; v2.44 per-stack smoke; v2.49 non-trivial rollback;
   v2.53a.2/v2.53b BLOCKED-status markers. Est. ~6 hours.
2. **DDD sweep:** 18 prefix typos (`common/audit_log.py` × 6 etc.) +
   1 fabrication (sbom.spdx.json L1902) + mark 16 forward-looking
   citations as such. Est. ~30 min mechanical.
3. **III spec edits:** add cites for `cat2_contracts/stack_b.py` +
   `stack_d.py` (Stack D self-app surface from § 3.6); surface b/ts
   naming inconsistency (`public_symbol_used_b.py` registers as
   `cat2.public-symbol-used-ts`) as a v2.X rename candidate.
4. **HHH closeout-commit re-framing:** 7 surplus CI capabilities
   (paired-sweep enforcement + Cat 3 Stack C driver build) — re-frame
   from "to be added" to "landed precedent" in spec sections that
   currently propose them as v2 work.
5. **JJJ uncited audits:** cite QQ (0/17 defects fixed — HIGH;
   probably v2.0 commit candidate), SS, TT, WW, UU into appropriate
   spec sections.
6. **AAA `ground-truth-sources.md` schema update:** spec App. A
   shows 4-field skeleton, live registry uses 6-field schema — update
   App. A.
7. **II three LOW polish edits:** Reynolds 1987 subtitle; Halvorsen
   community-attributed disclosure at v1.1 banking; "(extended to 3D)"
   qualifier in RD-3D § 1.0.B row.
8. **EEE per-batch enumeration:** 14 explicit-prose MISSING edges
   from § D.1–D.4; v2.50/v2.8 placeholder direction; v2.51/v2.52→v2.6
   informational relaxation (optional).
9. **v2.18 budget revision** per FFF (sph-water main.cpp 3164 LOC ≈
   3× RD-3D budget assumption).
10. **EE re-dispatch** now that spec is landed — harness at probe EE
    report § 8 ready.

The 9 mechanical/targeted items above are the gap between current
state and handoff-ready. None require fresh design decisions — they're
either backfill from prose anchors or direct application of probe
findings. The coordinator chat can start dispatching Phase 0 / 1
batches once items 1-3 are done; the rest can land in parallel with
early coding work.

---



## ⚠ Rev 16 supplement (read after rev 17, before rev 15)

**Steven's orchestration model clarified — spec is now self-contained
for handoff to ONE coordinator chat.**

This rev corrected a load-bearing misframe in rev 15 (§ 3.17 had been
written for multi-Claude-chat coordination of spec authoring; the
actual model is one downstream chat → many Claude Code agents → report
back to that chat).

**Workflow this spec hands into:**

1. ONE downstream claude.ai chat ingests `toolkit-spec.md` +
   `audit-trail.md` in a fresh session.
2. That chat parses § 4 batch contracts, builds dispatch plan via
   Probe EE file-touch matrix output, reports plan to Steven for sign-
   off.
3. Coordinator generates Claude Code prompts (one per parallel agent,
   each carrying its batch contract verbatim + pre-spawn HEAD SHA).
4. Code agents execute concurrently, each on its own branch.
5. Each agent reports back to coordinator with: declared-vs-actual
   touch set, tests added, acceptance results, branch + commit SHA.
6. Coordinator sequences merges in dependency order, re-runs toolkit
   on each merge, lands audit reports at
   `docs/diagnostics/_audits/v2.X_landing_<date>.md`.
7. Coordinator iterates to next parallel group.
8. End condition: all 57 batches landed; toolkit at 0 HARD_FAIL;
   v3 candidates surfaced.

**Spec content updates this rev (rev 16):**

- **§ 3.17 — Batch contract.** Replaces rev 15's parallel-agent-
  coordination section. Seven required fields per batch entry:
  purpose, inputs, touches (CREATE/MODIFY/DELETE/MOVE), depends-on,
  out-of-scope, acceptance (mechanically checkable), verification,
  rollback, commit-list. Coding-readiness verdict per batch: READY /
  CLOSE / GAP. Probe BBB is the handoff gate.
- **§ 3.18 — Landing pipeline.** 8 phases (pre-handoff →
  coordinator ingest → pre-spawn snapshot → parallel execution →
  reporting → landing/merge sequencing → post-merge audit → iterate).
  Coordinator chat discipline documented (never writes code; tracks
  landing ledger at `docs/integrity-v2/landing-ledger.md`).
- **§ 1.3.C — Per-sim Cat 3 candidate table.** Rewritten per Probe
  DD. Current shipped baseline acknowledged (4 Cat 3 checks, 2 of 13
  sims covered, 11 zero-coverage with no enumerated candidates).
  Table now includes coverage / status / priority columns. Full
  enumeration deferred to Probe GGG with 6-column shape. Two open
  questions surfaced: strange-attractors cat3-vs-cat4;
  mpm-multimaterial decomposition. Phase 2 sequencing reversed —
  deepen sph-water + lattice-boltzmann first, not zero-coverage sims.
- **v2.0 commit list expanded 10 → 11 commits.** New commit 0:
  catalog cleanup pre-absorb (Probe FF) — resolve `other`-category
  wedge, refresh, update stale prose, ready for Probe-YY's ~113
  findings absorption. Post-v2.0/v2.28 baseline: ~1458.

**12 new probe charters drafted this rev:**

| Probe | Scope | Priority |
|---|---|---|
| II | § 1.0.B canonical reference verification (14 sims) | Pre-handoff |
| JJ | § 1.3.B MMS technical content verification | Pre-handoff |
| AAA | Repo doc inventory at HEAD | Pre-handoff (Probe GGG pre-req) |
| BBB | Per-batch coding-readiness audit (57 batches) | **Handoff gate** |
| CCC | Convention citation audit (toolkit-wide) | Pre-handoff |
| DDD | File path / line number anchor audit | Pre-handoff |
| EEE | § 4.X phase dependency graph verification | Pre-handoff (post-BBB) |
| FFF | Sim entrypoint + per-sim infrastructure inventory | Pre-handoff |
| GGG | Per-sim Cat 3 candidate enumeration (spec-authoring) | Pre-handoff |
| HHH | CI workflow audit at HEAD | Pre-handoff |
| III | Toolkit module structure audit at HEAD | Pre-handoff |
| JJJ | Existing audit-trail content inventory | Pre-handoff |

Dispatch order (per dependencies):
- **First wave (independent):** AAA, BBB, CCC, DDD, FFF, HHH, III, JJJ
- **Second wave (depends on first):** II (uses AAA), JJ (uses AAA + II),
  GGG (uses DD + AAA + II + FFF), EEE (uses BBB)
- **Re-dispatch trigger:** Probe EE re-runs once spec lands at
  `docs/integrity-v2/` (landing-pipeline Phase A).

**Banking rows added this rev:** 80 (Probe DD baseline), 81 (Probe EE
block + harness), 82 (Probe FF baseline + cleanup sequence), 83 (§ 3.17
reframe).

**Calibrated confidence at handoff:**

- **HIGH** (verified): framework anchors (Roy 2005, DO-178C/DO-330,
  Diátaxis, SLSA, CycloneDX/SPDX), probe-grounded FACTs from
  YY/ZZ/AA/BB/CC/DD/FF (specific numbers, file paths probed at HEAD),
  § 0 first-principles framing, § 2.9.7/2.9.8.
- **MEDIUM** (inherited, reasonable): 57-batch × 12-phase scope,
  § 1.0.B citations (some probed via TT/SS, others trusted),
  § 1.3.B MMS technical content (inherited from rev 5-7), § 4.X
  phase dep graph.
- **LOW** (Convention #8 fabrication-pattern surfaces still likely):
  what v1 toolkit actually does at HEAD (probes DD/AA/BB cover a
  slice; III/HHH cover the rest), specific file paths/line numbers
  (DDD), category-context docs (AAA), convention citations (CCC).
  The 12 new probes are the structured discipline for closing this.

**One last warning to the next chat:** the spec author's most
persistent failure mode is citing draft-spec sections as if they exist
in repo (Convention #8 fabrication pattern). Probes DD/EE/FF all
flagged this. The 12 new probes are designed to catch more of it
before handoff. If you're returning to this work, the LOW-confidence
list above is where to look first.

---


## ⚠ Rev 15 supplement (read after rev 16, before rev 14 / 13)

Probe YY supplementary report (more detailed second pass) folded in,
plus parallel-orchestration scaffolding for the planned multi-agent
execution model. Headlines:

- **Spec § 1.7.R refined** — `--select ALL` 468 findings partitions
  cleanly (107 docs / 124 mechanical / 89 substantive). Current
  9-family `select` surfaces ~113 if CI ran it. Suppression baseline
  is **better than rev 14 stated**: 0 bare suppressions, all 5 sites
  rule-coded — Convention L migration cost near-zero. Ruff expansion
  staged: add S + COM at v2.28; defer D + ANN; treat `--select ALL`
  as aspirational ceiling, not target. **Convention L defined
  inline** with required / recommended / optional tiers. Forward-looking
  reasons without issue refs (the `runner.py:19` case) require
  migration, not grandfathering. build-py.yml precedent referenced:
  the CI gate pattern already exists for sim-side packages; v2.28
  extends it.

- **Spec § 3.17 added** — Parallel agent orchestration model.
  Defines five agent roles (coordinator / batch executor / probe
  agent / spec author / reviewer), seven parallelism rules
  (batch independence via touch-set disjointness; spec changes
  serialize; branch isolation; probes read-only by default;
  pre-spawn snapshot; re-anchor on dispatch; conflict resolution
  via coordinator). Concurrency-safe-vs-serial per-phase table:
  Phases 0 + 1 mostly serial; Phases 2 / 11 parallelize per-sim;
  Phases 4 / 5 / 7 / 8 parallelize freely.

- **Spec § 7.7 updated** — v1's "v2 sequencing assumes serial work"
  replaced with structural mitigations from § 3.17 (file-touch matrix,
  pre-spawn snapshot, append-only audit reports).

- **Audit-trail § 10 reserved** for the file-touch matrix artifact.
  Probe EE produces it (see below).

Three new probe charters drafted:

| Probe | Scope | Conditions |
|---|---|---|
| DD | Cat 3 invariant inventory at HEAD (shipped vs spec-only vs reframed) | Phase 2 parallelization signal |
| EE | File-touch matrix for v2.X batches | Spec § 3.17 load-bearing artifact; lands at `tools/integrity/docs/touch-matrix/v2-matrix.tsv` |
| FF | Grandfather catalog audit (size, hygiene, drift vs. live) | v2.0 / v2.28 "absorb baseline findings" pattern |

Banking row 79 added (Probe YY supplementary refinements).

---

## ⚠ Rev 14 supplement (read this first if returning, before the rev 13 section)

Probes YY/ZZ/AA/BB/CC executed at HEAD `351c66e` and folded back into
the spec. Headlines:

- **Spec § 1.7.R rewritten** — original "grandfather-catalog migration"
  framing REFUTED. New 4-part shape: CI enforcement +
  d3q19_verify.py hotspot remediation (86% of mypy errors localized
  there) + new **Convention L** (toolkit-source suppression hygiene —
  does not exist at HEAD; the rev-13 spec mis-named it Convention I)
  + per-category ruff selection decisions.
- **Spec § 1.7.Q revised** — `--output {human,json,github}` already
  exists; v2 adds `sarif` not a new `--format`. End-of-run summary
  needs to be moved leading→trailing + enriched. Suggested-fixes
  partial at HEAD (cat1.bare-path only) — v2 systematizes via
  `Finding.fix` field + `--fix` mode + unified diff.
- **Spec § 1.7.O / § 1.7.K conditioned** — TQL-5 framing holds with
  integration markers + audit_log.py wire-up. Mutmut at v2.41
  needs inner-suite < 30s and excludes audit_log.py.
- **Spec § 3.15 rewritten** — fix + gate (not consolidation): no
  FP-determinism flags applied anywhere at HEAD. "GPU atomic-add
  float reduction" source dropped (zero instances); replaced with
  scatter-slot ordering (sph-water + boids-3d). All 3 batches
  (v2.13/14/15) are fix + gate. First Cat-4 pilots: RD-2D,
  mandelbulb-explorer, physarum.
- **Phase 0 (v2.-1) re-anchored** — pre-fix toolkit baseline is 103s
  (not 275s); cat2.public-symbol-used-c is 91.7% of strict run (not
  60%); post-parallelization target ~30-35s.
- **Spec § 2.9.7 added** — `common/audit_log.py` wire-up + tests.
  HIGH-severity TQL-5 fix: module is 0% coverage, 0 production
  callers, `--no-audit-log` is a phantom flag. Priority-bumped to
  v2.0 commit 8.
- **Spec § 2.9.8 added** — pytest integration-test markers. Landed
  as v2.0 commit 9.
- **v2.0 expanded from 8 to 10 commits** (commits 8/9 new; SHA
  back-fill now commit 10).

Banking rows 73-78 added to audit-trail § 5. Probe registry rows
YY/ZZ/AA/BB/CC moved from DEFERRED to EXECUTED with verdicts.

---

## ⚠ Rev 13 supplement (read this first if returning)

After the rev 12 split into `toolkit-spec.md` + `audit-trail.md`, a
fresh chat session conducted a first-principles audit against
industry standards. The toolkit is now framed explicitly as a **code
verification framework** (Roy 2005 / Oberkampf-Roy 2010 vocabulary)
serving as the **canonical authority layer** for sim correctness.

Concrete spec additions in rev 13 (see audit-trail.md § 8 rev 13 row
for the banking; rows 62-72 for per-section provenance):

- **Spec § 0** (new) — First-principles framing: four pillars of
  authority (mechanical / anchored / reproducible / independent);
  Cat 3 declared load-bearing for the authority claim; Cats 1/2/4/5/6/7
  framed as authority-supporting infrastructure; V&V vocabulary
  anchor; TDD-restoration framing.
- **Spec § 1.0.D** (new) — Cat 3 deliverables mapped to Roy 2005 V&V
  framework. In scope: order-of-accuracy, algebraic, conservation,
  bounded-state. Out of scope: solution verification, validation, UQ.
- **Spec § 1.7.O** (new) — Tool qualification (DO-178C/DO-330
  TQL-5-equivalent discipline) + independence-of-verification
  formalization (multi-Claude-chat + Claude Code dual-agent).
- **Spec § 1.7.P** (new) — Diátaxis documentation framework
  (tutorials / how-to / reference / explanation). v2.34 expanded.
- **Spec § 1.7.Q** (new) — Findings UX standards: code frames,
  suggested fixes, ANSI color, end-of-run summary. **Conditional on
  Probe ZZ** outcome.
- **Spec § 1.7.R** (new) — Type-strictness policy for toolkit code
  itself (`mypy --strict` + `ruff check --select ALL`).
  **Conditional on Probe YY** outcome.
- **Spec § 1.8.G** (new) — SLSA Build L2 target at v2.42; L3 deferred.
- **Spec § 1.8.H** (new) — SBOM generation (CycloneDX 1.7 + SPDX 3.0
  dual emission) at v2.38.
- **Spec § 3.15** (new) — FP determinism policy consolidating
  scattered settings. **Validation by Probe CC.**
- **Spec § 3.16** (new) — CI matrix scope decision: Ubuntu 24.04 +
  Python 3.11 single, documented as deliberate scope.

Five probe charters drafted under `docs/integrity-v2/charters/`
awaiting dispatch:

| Probe | Scope | Conditions |
|---|---|---|
| YY | Toolkit-itself type-strictness (mypy/ruff/pyright) | Spec § 1.7.R |
| ZZ | Findings output format (code frames, color, summary) | Spec § 1.7.Q |
| AA | Toolkit own-test FIRST compliance + perf baseline | Spec § 1.7.K, § 1.7.O, Phase 0 |
| BB | `common/audit_log.py` coverage at HEAD | Spec § 1.7.K v2.41 |
| CC | FP determinism settings audit at HEAD | Spec § 3.15 |

All five charters reference in-repo paths per Convention K addendum;
dispatch awaits docs landing at `docs/integrity-v2/` + Steven's
go-ahead.

The remainder of this brief is the original rev-12-state context;
treat the "Open items not yet in the spec" section as superseded by
the rev 13 changes above where they overlap.

---

## What this project is

GPU-Sims (`github.com/StevenFAU/GPU-Sims`) is Steven's portfolio of 14
GPU physics simulations across three rendering stacks (TS/WebGPU,
C++/Vulkan, Taichi). The **integrity toolkit v2** is a cross-stack
verification framework that checks each sim against canonical-source
conventions: citations, contracts, numerical invariants, runtime
integration, cross-source consistency, build hygiene, and security
hygiene.

The v2 toolkit is in active design + implementation. v1 shipped some
checks; v2 closes the gaps and adds Cat 4-7. Estimated scope is ~241
substantive commits across 12 phases.

---

## The two-doc structure (NEW in rev 12 — this is the handoff)

For the first 11 revisions, this project was a single monolithic
design doc (`integrity_v2_design_2026-05-17_rev11.md`, 4,208 lines).
That doc had become 32% methodology preamble before any toolkit spec
content appeared. Steven called this out: the doc was tracking *how*
the toolkit was being designed more than *what* the toolkit was.

Rev 12 splits it into two:

| File | Purpose | Update rhythm |
|---|---|---|
| **`toolkit-spec.md`** | What the toolkit IS at v2.0-complete. | Converges. |
| **`audit-trail.md`** | How the spec was built and verified. | Append-mostly. |

**You must persist these at `docs/integrity-v2/` in the repo before
dispatching any probe against them** (per Convention K addendum;
Probe UU got blocked because rev 11 lived only in sandbox).

### What goes where

- **Spec gets the substantive outcome only.** A check's behavior, a
  phase's commit plan, a canonical reference, a fix-option's tradeoffs,
  a design decision's rationale. No "(per Probe X)" attributions
  anywhere in spec prose.
- **Audit trail gets the provenance.** "Probe X found...", "Rev N
  revised...", "FACT-verified against...", "INFERENCE based on...".
  Banking rows, probe records, methodology discipline.

When a probe returns: file its substantive findings as a banking row
in audit-trail § 5, then apply *only the corrected content* to the
spec. The audit trail sees the probe; the spec sees the fix.

---

## Current state at handoff

### Probes that have landed in rev 12 cycle

| Probe | Verdict | What changed |
|---|---|---|
| **TT** | Full canonical-reference audit (27 citations) | Found 2 new REFUTED in `sph-water/` docs; 3 SHIFTED. Fabrication pattern broadened beyond `quantum.md`. Banking rows 52-56. |
| **UU** | INCOMPLETE — re-run required | Rev 11 doc wasn't on user's filesystem; couldn't audit. Drove the Convention K addendum. Re-run after this handoff lands in repo. |
| **VV** | CONFIRMED Probe NN § Claim 4 (velocity.bin.bin mechanism) | All 6 mechanism claims FACT-verified. Spec v2.0 commit 4 now recommends Option A explicitly. Banking row 57 for the dead-code bonus finding. |
| **WW** | RETRACTS Probe NN § Claim 6 | ebbb743 cleared 0 cat1.bare-path findings; HEAD has 6 HARD_FAILs on `project-state.md`. Rev 11 row 45 retracted. Spec v2.0 commit 5 now lists 6 concrete line rewrites. Banking rows 59-61. |
| **XX** | CONFIRMED SPlisHSPlasH SHA upstream | `6bff55a6...` tagged `2.16.1`, master HEAD, vendored tree verified. Caused mid-cycle retraction of "no 2.16.x release" inference. § 4.4 auxiliary-vs-authoritative discipline added. |

### Spec changes applied in rev 12

- **§ 4 Phase 1 v2.0 commit 4** — rewritten with Option A (state-writer
  fix) preferred for backward-compat with existing capture artifacts.
- **§ 4 Phase 1 v2.0 commit 5** — rewritten with 6 concrete bare-path
  rewrites on `project-state.md` + grammar hardening regression tests.
- **§ 1.0.B lock table quantum row** — collapsed to clean 5-line
  canonical-reference statement (was 1 massive paragraph carrying
  King-fabrication audit history).
- **§ 1.0.B lock table sph-water row** — cleaned anchor reference;
  SPlisHSPlasH 2.16.1 tag verification noted.
- **Stomakhin citation** — "§ 4.1 (10-step full method) + § 5
  (constitutive model)" replaces "§ 5 + Algorithm 1".
- **EXIT_INTERNAL_FAIL=3** — now correctly framed as
  toolkit-specific extension of Ruff/ESLint convention, not strict
  convention compliance.

### Out-of-rev-doc cleanup queue (audit-trail § 9)

10 citation/path corrections to *source* documents (not the spec).
Each should land as a single PR against the source doc, referencing
its banking row. See `audit-trail.md` § 9 for the full table.

Highlights:
- `quantum.md:138` Vodeb byline (Verstraelen → Desaules + 8 more)
- `sph-water/README.md:92` Müller byline (Fetterer → Schirm + Duthaler)
- `sph-water/.../load-bearing-decisions.md:43` Williams-van der Laan
  fabrication → real van der Laan-Green-Sainz 2009
- `strange-attractors.md:222` Aizawa year 1984 → 1983
- `mandelbulb-explorer.md:182-183` title attribution
- `project-state.md` 6 bare-path rewrites (overlaps with v2.0 commit 5)

### Open items not yet in the spec

1. **`cat1.byline-verify` check spec** (Probe TT § H.2 proposal).
   Add as a v2.X batch under § 4. DOI/arXiv resolver fetch + verbatim
   compare against in-tree citation. Would catch all 4 REFUTED entries
   automatically.
2. **Probe UU re-run** once both docs land at `docs/integrity-v2/`.
3. **Probe-TT-follow-up A** — per-sim README + load-bearing-decisions
   audit (~30 citations, ~2.5h). Queued in audit-trail § 9.
4. **Probe-TT-follow-up B** — sim-spec-vs-README citation drift audit
   (~14 sim-specs cross-checked, ~2.5h). Queued.
5. **§-renumber pass** on `toolkit-spec.md` (optional). Currently
   "Scope of v2" is an unnumbered intro block before § 1; cross-
   references inside the doc were not renumbered to preserve their
   integrity. Could be done if Steven wants a fully numbered §0-§8.
6. **Glossary** — `toolkit-spec.md` could benefit from a glossary
   appendix (terms like "FACT", "INFERENCE", "live-source-red",
   "Convention K" etc. — but the FACT/INFERENCE terms only matter in
   the audit trail; glossary should be for terms the spec uses).

---

## Discipline conventions the next chat must internalize

Read `audit-trail.md` § 2 in full. Quick summary:

- **Convention K** — re-anchor (view / grep / web-fetch) before any
  edit asserting a specific location. Most fabrications caught by
  probes were memory-asserted specifics that never got re-anchored.
- **Convention #8** — never assert specifics from memory. Author
  names, line numbers, function signatures, CLI flags, version
  numbers — grep or fetch first.
- **Convention #12** — SHA back-fill is a separate follow-up commit,
  never `git commit --amend`.
- **Convention K addendum (NEW)** — design-rev artifacts must live at
  a stable repo path before any probe is dispatched against them.
- **FACT vs INFERENCE** — tag every concrete claim. Inferences are
  acceptable; mis-labeled inferences (asserted as FACT) are the
  primary failure family.
- **Four-state verdicts** — CONFIRMED / SHIFTED / REFUTED / DEFERRED
  (plus compounds DISCONFIRMED-AT-HEAD and REFRAMED).

---

## The two failure patterns to watch for

These have already cost real probe cycles. See `audit-trail.md` § 4.3
and § 4.4 for the disciplines.

### 1. Byline-confabulation (§ 4.3)

A real 1st author + a fabricated/substituted/omitted 2nd-position
author + real co-authors entirely absent. Three sub-variants:

- **Substitution** — Harris+King 2018, Vodeb+Verstraelen 2025,
  Müller+Fetterer 2007. (Real 2nd authors: nobody-named-King,
  Desaules, Schirm.)
- **Omission** — Mordvintsev "Distill Neural CA" dropping Randazzo,
  Niklasson, Levin.
- **Entire-paper-fabrication** — "Williams van der Laan 2008" —
  paper doesn't exist; closest real is van der Laan-Green-Sainz 2009.

Catch: verbatim 2nd-position-author check against an authoritative
resolver (DOI, PubMed, ACM DL). Proposed mechanical check:
`cat1.byline-verify` — to be specced as v2.X.

### 2. Auxiliary-vs-authoritative surface (§ 4.4)

Non-existence claims must be made against the authoritative surface,
not an auxiliary one. Two instances caught in rev 11→12 cycle:

- **GitHub Releases vs git tags.** Releases (manually-curated
  announcement objects) is auxiliary; the git tag set is
  authoritative.
- **Toolkit `--root <single-file>` vs full-repo invocation.**
  Single-file root produces vacuous "1 pass" output; full-repo is
  authoritative.

When claiming "X doesn't exist" or "X doesn't fire" — explicitly
identify which surface you are reading and confirm it is the
authoritative one for that claim.

---

## How a fresh chat session should start

1. **Read this brief.** (You're doing it.)
2. **Open `toolkit-spec.md` and skim the structure.** Especially the
   intro through § 1.0.B, and § 4 Phase 1 v2.0 (where rev 12
   substantive corrections live).
3. **Open `audit-trail.md` and skim § 2 (conventions) + § 4 (audit
   surfaces) + § 5 (banking log).** § 5 row 50+ is recent context;
   rows 51-61 are rev 12 additions.
4. **Confirm with Steven where the docs live in the repo.** If they
   are not yet at `docs/integrity-v2/`, prioritize that landing
   (Convention K addendum). Then Probe UU can re-run.
5. **Ask Steven what to work on next.** Common candidates:
   - Add `cat1.byline-verify` spec to `toolkit-spec.md § 4`.
   - Dispatch Probe-TT-follow-up A or B.
   - Land an out-of-rev-doc citation-fix PR (see audit-trail § 9).
   - Renumber spec § 0 → § 1, § 1 → § 2, etc. (optional cleanup).
   - Add a glossary to the spec.
   - Start drafting Phase 2+ commit specs in more detail.

---

## File inventory at handoff

Files Steven has from this session (`/mnt/user-data/outputs/`):

- `toolkit-spec-rev1.md` (~160 KB, 2,783 lines) — the new spec doc
- `audit-trail.md` (~52 KB, 470 lines) — the new audit-trail doc
- `HANDOFF_BRIEF.md` (this file)
- `integrity_v2_design_2026-05-17_rev11.md` (~274 KB) — terminal
  legacy monolithic doc, kept for archaeology
- `probe_tt_charter.md` through `probe_xx_charter.md` — original
  probe charters
- `probe_tt_report_2026-05-17.md` through
  `probe_xx_report_2026-05-17.md` — probe reports (already filed
  by Claude Code at `docs/diagnostics/_audits/` in repo)

**Recommended in-repo landing paths:**

```
docs/integrity-v2/
  toolkit-spec.md          # rename from toolkit-spec-rev1.md
  audit-trail.md           # as-is
  HANDOFF_BRIEF.md         # as-is (or move to docs/handoff/)
  charters/
    probe_tt_charter.md
    probe_uu_charter.md
    probe_vv_charter.md
    probe_ww_charter.md
    probe_xx_charter.md
```

The legacy rev 11 doc can stay in `/mnt/user-data/outputs/` or be
archived to `docs/integrity-v2/_legacy/rev11.md` for record. Future
work does not edit the legacy doc.

---

## One last thing

The prior chat made two unsafe inferences caught by probes in this
cycle (the cat1.bare-path runner-misinvocation and the GitHub-
Releases-vs-git-tags confusion). Both were honest retractions —
documented in `audit-trail.md` § 7. The pattern that produced them
was the same: confusing an auxiliary surface for an authoritative
one. Watch for this. When you find yourself about to claim X
doesn't exist / doesn't fire / wasn't done, ask yourself: which
surface am I reading, and is it the authoritative one for this
claim?
