---
title: "Integrity Toolkit v1.1 Batch 1 — Retrospective Addendum"
date: 2026-05-15
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/diagnostics/_audits/integrity_v1_1_self_review_probe_2026-05-15_architect1.md
---

# Integrity Toolkit v1.1 Batch 1 — Retrospective Addendum

## 1. Purpose

The original retro at `docs/retro/integrity-toolkit-v1.1-batch1.md` was
self-drafted by architect-1, the same source responsible for the in-batch
fabrications. § 0 of that retro disclosed the bias and § 6.4 recommended an
architect-2 review pass. The chosen alternative was a
self-review-probe-then-architect-1-amend cycle: a comprehensive read-only
probe
(`docs/diagnostics/_audits/integrity_v1_1_self_review_probe_2026-05-15_architect1.md`)
verified every mechanically-checkable retro claim against repo state. This
addendum records the probe's findings.

The original retro file is not edited. Retros are append-only by the same
convention that governs audit reports (audit-citation grandfather
rationale: the historical record is the value; retroactive edits erase
the load-bearing context). This addendum is the correction layer.

## 2. Confirmed claims (no action needed)

The following original-retro claims were verified by the self-review probe
and stand as written:

1. **§ 2.1 — `cat2.stub-label-stale` catches both canonical headers.**
   Probe § E.2 confirms findings at
   `common/common-cpp/include/gpusims/alembic_writer.hpp` (impl 82 LOC) and
   `common/common-cpp/include/gpusims/vdb_writer.hpp` (impl 114 LOC). Both
   suppressed under `cat2-stub-label-stale`.
2. **§ 2.2 — A.5 fence-skip extended to `cat1.intra-repo` and
   `cat1.upstream-citation`.** Probe § B.10 + § B.11 confirms `fence_state`
   construction and skip-logic in both check modules.
3. **§ 3.4 — Own-source annotations landed in `a42085a`.** Probe § E.5
   confirms `tools/integrity/tests/test_suppression_fence.py:3` and `:23`
<!-- integrity-allow: cat1.annotation-form; retrospective-doc literal mention of the annotation grammar (not a real annotation); n/a -->
   carry the inline `# integrity-allow: cat1.annotation-form` annotations.
4. **§ 6.1 — A.3 / A.2 / classifier-rule deferrals.** Probe § E.6 confirms
   none of A.3, A.2, `toolkit-doc-snapshot`, or `project-state-snapshot`
   landed silently. All remain deferred.
5. **§ 9 — Fabrication-class shift.** Probe § F quantifies the shift:
   spec § 12 baseline = ~50% original-fabrication / ~50% hybrid; current
   live state = ~2% original-fabrication in suppressed pool and 0% in
   outstanding hard-fails. **5×–25× reduction in fabrication share**;
   strongest signal is the 0/4 outstanding-hard-fail ratio.
6. **§ 5.5 — Catalog tally drift.** Probe § C.4 quantifies drift at
   **+63 entries (+6.7%)** between the catalog's manual-refresh anchor
   (`c3391f7`) and probe time. Per-category: `audit-citation` +28,
   `audit-report-grammar-example` +19, `other-cat1` +10, `retro-grammar-example`
   +5, `toolkit-own-source` +2, `cat2-stack-c-unused` −1. Empirical support
   for the retro's anticipation that A.8 manual-refresh goes stale fast.
7. **§ 1 — Test-count growth 74 → 96 (+22).** Probe § D.2 confirms
   exactly 96 tests collected; +22 vs the v1.1 apispec probe's 74-test
   baseline.

## 3. Refuted claims (corrections)

### 3.1 — Module/LOC inventory (corrects original retro § 1)

**Original claim:** "+6 modules new, ~8 modified, ~750 LOC new + ~200 LOC
changed".

**Actual (from `git diff --stat af248cf~1..d772803 -- tools/integrity/integrity/`):**

- **3 new Python modules:** `snapshot.py`, `stub_label_stale.py`,
  `d3q19_verify.py`.
- **1 new JSON file:** `d3q19_equilibrium.expected.json` (290 LOC of
  expected-values data, not a code module).
- **11 modified Python modules:** `__main__.py`, `annotation.py`,
  `intra_repo.py`, `unregistered_upstream.py`, `upstream.py`,
  `upstream_anchor.py`, `cat2_contracts/checks/__init__.py`,
  `common/annotations.py`, `common/suppression.py`, `grandfather.py`,
  `runner.py`.
- **1157 insertions / 35 deletions total.**
- Of which **~571 LOC are co-landed Phase 12 d3q19 work** (see § 4.2).
  Excluding the d3q19 lines: ~586 insertions for v1.1-batch-1-proper.

The "+6 modules" figure overstated new modules by 2–3. The "~8 modified"
figure understated by 3. The LOC figures don't reconcile with diff-stat in
either direction (1157 total vs ~950 implied by the retro's stack of
~750+200).

The corrective discipline (also reaffirmed in § 6): retro inventory
figures should be transcribed from `git diff --stat` at retro draft time,
not estimated.

### 3.2 — History file state (corrects original retro § 2.3)

**Original claim:** "History file at
`tools/integrity/.grandfather-history.json` (currently 1 entry; appends
per run unless `--no-history-append`)".

**Actual:** the file is an empty array (`[]`). `git log` shows a single
commit (`dbac051`) touching the file; `git show
dbac051:tools/integrity/.grandfather-history.json` is also `[]`. The
commit subject "seed history" created an empty-array file; it never seeded
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
a real entry. The append machinery in `snapshot.py:178-181` is functional
— but no commit since `dbac051` has landed a populated entry, and runs
since (including the probe-time `--no-history-append` invocation) correctly
did not append.

Note also: history-file appends are uncommitted side effects of the CLI.
A reader expecting "appends per run" to grow the file across commits has
to also notice that the file is git-tracked, so growth requires explicit
commits. This is worth documenting in `snapshot.py` itself.

**Recommendation banked for batch 2:** either commit one real seed entry
to the history file (one line of toolkit invocation, then `git add` +
commit), or remove the "seed" framing from `snapshot.py`'s module
docstring and from the dbac051-era documentation. Either resolution is
cheap; the mismatch should not persist into v1.2.

### 3.3 — A.3 leverage (refines original retro § 5.1)

**Original claim:** "4 of 6 outstanding live-source hard-fails are
bare-path patterns A.3 would catch".

**Actual (post-retro, post-`a42085a` annotation):** 4 outstanding
hard-fails remain (the original 6 minus the two own-source findings
annotated by `a42085a`). A.3 at basic basename-match scope catches:

<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
- `chapter13/cpu/LBM.cpp:97` at `docs/phase12_lattice_boltzmann.md:203` —
  Krueger registered. **CATCHABLE.**
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
- `chapter13/cpu/LBM.cpp:97` at `docs/phase12_lattice_boltzmann.md:351` —
  same. **CATCHABLE.**
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
- `SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75` at
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
  `compute_boundary_volume.comp.glsl:7` — SPlisHSPlasH 2.16.1 registered.
  **CATCHABLE.**
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
- `main.cpp:1168-1279` at `docs/phase12_lattice_boltzmann.md:1276` —
  basename `main.cpp` is **AMBIGUOUS** (matches dozens of intra-repo files
  + multiple upstream files). A.3 at basic scope cannot disambiguate
  without an additional policy.

**Revised leverage statement:** A.3 catches **3 of 4 directly**; the
4th is an AMBIGUOUS-BASENAME case requiring a disambiguation policy
A.3 at basic scope doesn't have. A.3 remains the single highest-leverage
deferred item — the original retro's prioritization stands — but the
exact leverage is "3 of 4 + 1 ambiguous-needs-policy" rather than "4 of
4."

The policy decision for AMBIGUOUS basenames (HARD_FAIL / SOFT_WARN /
IGNORE) is the second-largest open question for A.3 implementation. Probe
§ G.2 counts **226 AMBIGUOUS candidates** across the broader repo corpus,
so this isn't a one-off edge case.

### 3.4 — Operating-condition framing (refines original retro § 7.1)

**Original framing:** "concurrent multi-agent landing".

**Actual (per probe § K):** 100% of past-30-day commits are by a single
human author (`Steven Cohen`, 139 commits over 30 days). 81.9% of
consecutive commits land within one hour of each other. The collision
pattern the retro describes (4 concurrent work-streams against the same
`main`) is real and the consequence — multi-stream churn against a shared
toolkit gate — is correctly identified. The framing should be sharpened:

> Single-human, multi-session concurrent landing across parallel Claude
> Code agents.

The convention recommendations stand; what matters for the convention
design is the same-`main`-shared-gate property, not whether the
operator is one human or many. But the **fingerprint to monitor in future
retros** is `single-author + many-within-1hr commits` specifically. A
future shift toward multi-human concurrent landing would change the
operating shape and may warrant different convention design.

## 4. New scope surfaced by probe (not in original retro)

### 4.1 — Strict-mode human-renderer suppressed-stanza bug

`tools/integrity/integrity/runner.py:141-145` (the `else` branch of
`emit_output`) iterates all findings and emits each as a HARD_FAIL stanza
without filtering on `f.suppressed`. The summary line reports the correct
counts (`4 hard-fail, 1007 suppressed`), but the stanza list dumps
suppressed findings as if they were HARD_FAIL stanzas. The `github`-output
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
branch at `runner.py:133-139` correctly filters with `if f.suppressed:
continue`. The `human`-output branch does not.

Symptom: a `python3 -m integrity --mode strict --no-audit-log | head -10`
invocation appears to show 4-5+ HARD_FAIL stanzas including suppressed
ones (e.g. `CHANGELOG.md:92`, `common/common-py/examples/hello/...`) while
the summary line correctly reports 4 unsuppressed. The human-readable
output and the summary line are mutually inconsistent.

Fix: one-line addition of `if f.suppressed: continue` to the human-output
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
branch (between `runner.py:143` and `runner.py:144`).

**Banked for batch 2** as P1.6.

### 4.2 — d3q19 verification machinery landed unmentioned

281 LOC `tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py`
+ 290 LOC
`tools/integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.expected.json`
+ `tools/integrity/docs/algebraic/d3q19.md` landed inside the
v1.1-batch-1 SHA range (per probe § E.1's `git diff --stat`). This is
Phase 12 setup work that co-landed; the original retro scoped explicitly
to "named v1.1 spec items (A.1, A.5, A.7, A.8, 5.B)" and excluded the
Phase 12 work.

Acknowledging here for inventory completeness. No corrective action — the
retro's scope choice was deliberate — but the LOC figures in § 3.1 above
must be read against this co-landing.

### 4.3 — Registry declares checks that aren't registered

`tools/integrity/docs/ground-truth-sources.md` `[Algebraic_D3Q19]` entry
declares:

```toml
[Algebraic_D3Q19]
derivation     = "tools/integrity/docs/algebraic/d3q19.md"
expected_data  = "tools/integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.expected.json"
used_by_checks = ["cat3.d3q19-velocity-set", "cat3.d3q19-weights", "cat3.d3q19-equilibrium"]
```

None of those three CHECK_IDs are registered in
`tools/integrity/integrity/cat3_numerical/checks/__init__.py` — that file
registers only `cat3.cubic-kernel`. The verification harness exists in
`d3q19_verify.py` (281 LOC of derivation + per-test-point evaluation) but
is not wired through the toolkit's check-discovery surface. Running
`python3 -m integrity --check cat3.d3q19-velocity-set --no-audit-log`
returns zero findings because the discovery loop finds no module with
that CHECK_ID.

This is exactly the kind of registry-vs-implementation drift A.2 (toolkit
self-application) would mechanically catch. Worth banking as a concrete
acceptance test for A.2 when it lands: the new check should fire on the
`[Algebraic_D3Q19]` `used_by_checks` entries until each is registered.

**Banked for batch 2** as P1.5.

### 4.4 — `stub_label_stale.py` docstring drift

The top-of-module docstring at
`tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py:15-18`
describes the resolution convention from the original execution spec
Decision 2:

> `.hpp`/`.h` in `common-cpp/include/<sub>/<base>.hpp` ->
> `common-cpp/src/<sub>/<base>.cpp` (relative path mirror)

The code below (`_resolve_impl_path`, lines 95-128) implements the
**corrected** namespace-stripping logic from pause-and-surface #1
(retro § 3.1): `include/<namespace>/<rest>.hpp -> src/<rest>.cpp`. The
in-function docstring at lines 98-105 documents the corrected convention
accurately; the module-level docstring still describes the fabricated
literal-mirror rule. Docstring lies about code.

Cosmetic but worth fixing. **Banked for batch 2** as P1.7.

## 5. Batch-2 priority order (revised post-probe)

Confirmed unchanged from original retro § 6.1 priority 1:

- **P1 — A.3 (bare-path-to-upstream-basename).**
  - Leverage: catches 3 of 4 outstanding hard-fails directly (§ 3.3
    above).
  - Backlog cost: 164 registered-upstream-bare + 290 intra-repo-bare
    candidates across the broader corpus (probe § G.2); ~80% route to
    `audit-citation` via the existing classifier rule.
  - Policy decision needed: AMBIGUOUS basenames (226 candidates;
    probe § G.2 + G.3) → HARD_FAIL / SOFT_WARN / IGNORE. Resolve at
    spec-draft time.

Added to batch-2 scope from the probe:

- **P1.5 — Register `cat3.d3q19-*` checks (§ 4.3).** Modest scope;
  closes the registry-vs-implementation drift. Three CHECK_IDs needed:
  `cat3.d3q19-velocity-set`, `cat3.d3q19-weights`,
  `cat3.d3q19-equilibrium`. May land as a single check module with three
  sub-tests, or three separate modules — design choice for spec drafting.
  Uses the existing `d3q19_verify.py` harness; no new algebraic work.
- **P1.6 — Fix strict-mode human-renderer suppressed-stanza filter
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
  (§ 4.1).** One-line fix to `runner.py:143-145` (insert `if
  f.suppressed: continue`). No test changes needed beyond confirming
  summary/stanza consistency in a small unit test.
- **P1.7 — Update `stub_label_stale.py` module docstring (§ 4.4).**
  Cosmetic; replaces the literal-mirror description with the
  namespace-strip description that matches `_resolve_impl_path`.

Confirmed unchanged from original retro § 6.1 remaining priorities:

- **P2 — A.2 (toolkit self-application).** § 4.3 surfaces a concrete
  acceptance test.
- **P3 — Pre-spec probe template upgrades (retro § 7.2 C+D).** Probe
  § H.2 confirms both gaps are real.
- **P4 — `toolkit-doc-snapshot` + `project-state-snapshot` classifier
  extensions.**
- **P5 — Grandfather-sweep MEDIUM enforcement (CI check, not pre-commit
  hook).** Probe § I.3 inferred medium over hard: the mechanical 96.8%
  un-paired rate overstates the actual finding-introduction rate because
  most cat1-touching commits don't add new findings. A CI check that
  fails when the live-source delta grows without a paired sweep is the
  right level.
- **P6 — A.8 auto-refresh** (catalog auto-tally from history). § 6.7%
  drift across one batch cycle quantifies the need.
- **P7+ — A.4, A.6, A.9** per original retro § 6.1.

## 6. Audit-prose freshness convention reaffirmed

The post-retro landing audit
(`docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md`
§ D.2.1) banked a sixth convention: an audit-prose freshness check.
Re-read existing on-disk content before drafting prose; verify
load-bearing assertions against `git diff --stat` / `grep` / file reads
rather than memory.

The self-review probe itself surfaced an example of this same failure
shape: the original retro's "+6 modules new, ~750 LOC" claim (§ 3.1
above) was authored without re-checking diff-stat at retro draft time.
Same failure shape as the in-batch fabrications (assertion without
verification). The cost was a small inventory miss; the principle is
identical to Hard Rule 2's "synced repo state is authoritative."

Convention F (audit-prose freshness) holds. Recommend adding to the
formal conventions list when one is drafted (per original retro § 8
question 3 — `docs/CONVENTIONS.md` vs `tools/integrity/docs/conventions.md`
vs per-stack; still open).

## 7. Closing

The original retro's load-bearing conclusions hold:

- A.3 is the highest-leverage deferred item (refined to 3 of 4, not 4 of
  4).
- The fabrication class the toolkit was built to catch is no longer the
  dominant defect class on `main` (quantified at 5×–25× reduction).
- Concurrent multi-session landing is the operating condition.
- The five banked conventions (new-files-first decomposition,
  grandfather-sweep companion, probe-template enumerate-conventions,
  probe-template enumerate-call-sites, spec-author-self-test review)
  are all still right.

The original retro's inventory was sloppy and three pieces of real scope
were missed; this addendum records both. Batch 2 scope is finalized in
§ 5 above; spec drafting can begin from this state.

## End of addendum
