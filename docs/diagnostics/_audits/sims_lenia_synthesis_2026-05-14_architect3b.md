---
title: Lenia-FFT Audit Synthesis + Commit Sequence Proposal (Layer 3 Batch B)
date: 2026-05-14
author: architect-3b
layer: 3
batch: B
sim: continuous-ca/lenia-fft
status: complete
scope: synthesis of prior probes; commit-sequence proposal for user
inputs:
  - sims_lenia_chakazul_2026-05-14_architect3b.md (external upstream verification)
  - sims_lenia_probe1_2026-05-14_architect3b.md (structural inventory)
verdict: sim is broadly healthy; six commits proposed; no probe-2 needed
cross_workstream:
  - flag-for-common-py: ParamPanel.folder() y-coord convention
  - flag-for-common-py: StateWriter per-buffer schema introspection coverage
  - flag-for-common-py: VdbWriter stub-mode behavior
---

> Closes the Layer 3 Batch B audit cycle for `continuous-ca/lenia-fft/`. Two
> probes preceded this synthesis: an external upstream verification of the
> Chakazul citation (`sims_lenia_chakazul_*`) and a structural inventory
> probe of the sim source tree (`sims_lenia_probe1_*`). Both are
> independently readable. This report pulls their findings together,
> ranks them by audit consequence, proposes a six-commit landing
> sequence (mostly doc-only and hygiene; no behavior change), flags
> three items for a future common-py audit layer, and identifies one
> latent structural issue (GGUI Y-convention) that the sim cannot fix
> alone — it requires a consumer #3 sim with panels outside the
> upper-left quadrant to break the load-bearing coincidence definitively.
>
> **No probe-2 required.** Probe-1 surfaced no follow-on questions whose
> evidence requires deeper inspection. The commit-sequence proposal
> below is grounded in fully-cited probe-1 evidence; no new probes are
> needed before landing the commits.

## Section A: Audit cycle summary

### A.1 Probes run

| # | Probe | File | Method | Outcome |
|---|---|---|---|---|
| 1 | External upstream verification | `sims_lenia_chakazul_2026-05-14_architect3b.md` | direct upstream fetch (no clone, no vendor) | CONFIRMED — Chakazul citation accurate at master HEAD `adfc542` |
| 2 | Structural inventory (probe-1) | `sims_lenia_probe1_2026-05-14_architect3b.md` | Claude Code, read-only, sections A-P | 6 findings, 0 deliberate-skeleton paths |

### A.2 Triage-targeted fabrication smells, status

| Triage flag | Source | Status |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| Polyring formula anchor at `Chakazul/Lenia/Python/LeniaNDK.py:329-335` (analog of SPlisHSPlasH 1.8.10) | `sims_prioritization_2026-05-14_triage.md` § C.2 | **REFUTED** — citation verified clean at master HEAD; see Chakazul probe Section D |
| GGUI Y-convention asymmetry (`cursor_to_field_cell` vs `cursor_in_any_panel`) | triage § B "Banked items" | **CONFIRMED** as load-bearing coincidence; see § B.3 below |

The triage's high-priority fabrication smell is refuted. The
medium-priority structural-coincidence flag is confirmed.

### A.3 Prior in-tree Convention #8 incidents in lenia-fft (existing retros)

For audit-completeness, two prior Convention #8 incidents in this sim are
already documented in tree:

1. **Phase 10 spec v1 bump4-vs-quad4 mismatch.** The original draft of
   `phase10_lenia_fft_v2.md` cited the bump4 formula (`exp(4 - 1/(r*(1-r)))`)
   as the canonical Lenia kernel. Architect-2 round-3 cross-review enumerated
   the 122 single-peak creatures in upstream `animals.json` and observed they
   all use `kn=1`, which dispatches via the off-by-one to `kernel_core[0] =
   quad4`. v2 of the spec inline-retracts the bump4 claim. Documented at
   `phase10_lenia_fft_v2.md` line 6 ("The kernel formula is upstream-faithful
   quad4… NOT the bump4 formula in this spec's v1").

2. **Phase 10 polish-4 GGUI Y-convention banking.** Found during
   visual-verification gate. Two contradictory cy-origin assumptions in
   `cursor_to_field_cell` and `cursor_in_any_panel`. Banked as undisproven
   coincidence. Documented in `continuous-ca/lenia-fft/docs/notes.md` Polish-4
   section.

Pattern match against MPM's `project-state.md` line 543 (60 fps perf
extrapolation, retracted at polish-4): all three are
draft-time-confident-recall claims disproved by ground-truth verification
against upstream artifacts or visual gates, with retraction landed in
tree. **The project's existing Convention #8 retraction culture is working
as designed; this audit adds further detail rather than uncovering a
hidden pattern.**

## Section B: Findings ranked by audit consequence

### B.1 [TIER-1] In-source docstring contradiction: numpy round-trip claim

**Source:** probe-1 Section F (lines 595–614 of probe-1) + Section P (lines
1494–1508).

**Evidence:** Two in-source comments make incompatible claims about the
numpy round-trip cost when the Taichi real-space backend is selected:

```python:continuous-ca/lenia-fft/python/lenia_fft/main.py:226
Pulls state to numpy, hands to the selected FFT/convolution backend,
pushes the new state back. The numpy round-trip is the price of
backend-agnostic dispatch — same boundary used by every FFT consumer.
Taichi-real-space backend skips the numpy round-trip internally (it
reads/writes the Taichi field directly), but the public step() interface
is still numpy-in/numpy-out for API consistency.
```

```python:continuous-ca/lenia-fft/python/lenia_fft/fft_backend.py:18
# Convention: each backend takes a numpy array `state_np` (n_grid, n_grid)
# float32 and returns a numpy array of the same shape and dtype. This costs
# 2 numpy↔Taichi round-trips per step (caller's to_numpy + from_numpy
# around the call); the Taichi real-space backend additionally does its
# own from_numpy + to_numpy inside step() to populate the shared
# `state.state_2d` field. Acceptable per backend-agnostic-dispatch goal;
# revisit if the FFT backends ship and real-space becomes legacy.
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The `main.py:226` docstring says the real-space backend "skips the
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
numpy round-trip internally." The `fft_backend.py:18` comment correctly
states that real-space "additionally does its own from_numpy + to_numpy
inside step()." These describe the same code differently and the second
description is what the code does.

**Audit class:** Convention #8 — in-source claim disproved by inspection
of the artifact it describes. Same shape as the Phase 10 spec v1
bump4-vs-quad4 retraction.

**Severity:** Low. Behavior is correct; only the docstring overstates an
optimization that the wrapper doesn't deliver. A maintainer reading the
docstring and trying to perf-tune the real-space backend would draw the
wrong conclusion ("the round-trip already isn't there, look elsewhere"),
when actually the wrapper round-trip is the optimization opportunity.

**Disposition:** Doc-only fix. See commit 1.

### B.2 [TIER-1] Spec test names diverge from shipped test names

**Source:** probe-1 Section L.

**Evidence:** Phase 10 spec § 5 (and `phase10_lenia_fft_v2.md` line 6
top-of-spec) names three load-bearing CI tests:

- `test_select_backend_factory`
- `test_preset_stability_all`
- `test_capture_schema_round_trip`

Actual test names in `python/tests/test_kernels.py`:

- `test_select_backend_factory_falls_back` (line 265)
- `test_apply_preset_stability_2d` (line 218)
- `test_capture_schema_round_trip` (line 307, unchanged)

Two of three diverge by suffix. The contracts the tests cover (factory
fall-through, all-four-preset stability, capture-schema round trip)
match the spec; the names don't.

**Cross-check:** `project-state.md` line 656 already has the actual
names (`test_select_backend_factory_falls_back`,
`test_apply_preset_stability_2d`, `test_capture_schema_round_trip`),
suggesting the spec text drifted relative to the shipped CI test names
between v2 draft and ship. The retro at project-state.md line 656
references these names accurately.

**Audit class:** spec-source drift (not Convention #8). The spec
made a forward-looking commitment about test names; the shipped tests
chose more descriptive names; the spec wasn't updated post-ship.

**Severity:** Low. The phase spec is a shipped doc, not a contract.
Project-state.md is the authoritative reference and it's already correct.

**Disposition:** Doc-only fix to the shipped Phase 10 spec, OR explicit
acknowledgment in `load-bearing-decisions.md` that the test names drifted
between spec and ship. The lighter touch is the latter. See commit 2.

### B.3 [TIER-2] GGUI Y-convention is structural coincidence, not soundness

**Source:** probe-1 Section G (full text at lines 710–852 of probe-1).

**Evidence:** Probe-1 Section G analyzes `cursor_to_field_cell` and
`cursor_in_any_panel` (both in `main.py`) called from the same brush
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
call site at `main.py:498`. Both consume `cur = window.get_cursor_pos()`.
The first treats cy=0 as TOP; the second treats cy=0 as BOTTOM (and
flips via `cy_top = 1.0 - cy_bottom`).

Only one cy-origin can be literally correct for the cursor at any given
moment. Both functions empirically work. Probe-1's analysis: panel rects
all live in the upper-left quadrant (`x ∈ [0.02, 0.24]`, `y ∈ [0.02,
0.99]`), and the flip lands a top-of-window cursor inside the top panel
rect in either interpretation. The two empirical observations cannot
both be literally true without a mediating quirk — most likely a
handedness mismatch inside `gpusims_common.ParamPanel`'s panel-positioner
that the lenia-side flip happens to "double-correct" for.

**Audit class:** load-bearing coincidence (already banked). Not a
fabrication; the in-source notes correctly identify this as undisproven.
Probe-1 confirms the flag is warranted — the conditions for it to break
(consumer #3 with panels outside upper-left, or a refactor of
ParamPanel internals) are concretely identifiable.

**Severity:** Medium when consumer #3 lands; low until then. The current
lenia + MPM panel layouts both fall in the upper-left quadrant by
convention rather than constraint; a future Stack D sim that places
panels right-aligned or full-width will likely surface this.

**Disposition:** Two changes:

1. Expand the load-bearing-decisions section to name the breakage
   condition explicitly, including which side of the flip is most likely
   the one to investigate first (ParamPanel internals — the y-flip looks
   like double-correction inside `cursor_in_any_panel`).
2. Cross-workstream flag for the future common-py audit layer: when
   `gpusims_common.ParamPanel.folder()` gets a deep audit, the
   y-coord-origin contract is the load-bearing question.

See commit 3 for the load-bearing-decisions update; see § D for the
common-py flag.

### B.4 [TIER-2] b-string data model not present, spec framing implies it is

**Source:** probe-1 Section D.

**Evidence:** Phase 10 v2 spec frames the polyring banking as "Phase 10
ships single-peak only (b="1" via the quad4 path)" — implying b is a
value in the data model that happens to be constrained to "1". The
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
actual `LeniaPreset` dataclass at `presets.py:57` has no `b` field, no
`kn` field. The four shipped presets are hard-coded to single-shell
quad4 via direct kernel-radius-only parameterization.

```python:continuous-ca/lenia-fft/python/lenia_fft/presets.py:57
@dataclass(frozen=True)
class LeniaPreset:
    """One named Lenia creature."""

    name: str
    dim: int                         # 2 or 3
    kernel_radius: int               # R
    time_resolution: float           # T (dt = 1/T)
    mu: float                        # growth mean
    sigma: float                     # growth std
    seed_radius_cells: float         # initial random-blob radius
```

There is no parser for `params.b`, no branch on `len(b) > 1`, no
fallback to first peak. The codepath has no concept of `b` at all.

**Audit class:** spec-vs-code framing mismatch (not Convention #8). The
spec was honest about banking polyring for v1.1+; the framing of "ships
single-peak via b='1'" is slightly misleading because the v1.1 extension
will require a `LeniaPreset` schema change, not an "add multi-peak
handling to existing b machinery."

**Severity:** Very low. Banking is real; the actual extension work is
documented in `notes.md`. The framing-clarity issue surfaces only when
someone reads the spec and goes hunting for the b-field that doesn't
exist.

**Disposition:** Optional one-line clarification in
`load-bearing-decisions.md`'s polyring section. Not load-bearing for
v1.1 implementation work. See commit 3 (folded into the same
load-bearing-decisions update as B.3).

### B.5 [TIER-3] `imgui.ini` committed

**Source:** probe-1 Section P (final bullet, line 1532).

**Evidence:** `continuous-ca/lenia-fft/python/imgui.ini` is in the
repository. GGUI writes panel-position state to this file on every
panel-move; committing it makes git-status noisy whenever the user
interacts with panels.

**Audit class:** source-control hygiene.

**Severity:** Very low.

**Disposition:** Gitignore + remove from tree. See commit 4. Verify
that other Stack D sims (hello, mpm-multimaterial) have the same hygiene
issue; if yes, fold into the same commit. Probe-1 didn't grep for that;
the commit prompt should.

### B.6 [TIER-3] Polyring banking citation lacks a SHA pin

**Source:** Chakazul probe Section G.2.

// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
**Evidence:** The Phase 10 spec banks `LeniaNDK.py:329-335` with no SHA
or tag pin. Verification at architect-2 round-3 worked because the
master-HEAD-at-that-time happened to have the function at those lines;
verification at this audit's master HEAD (`adfc542`) also worked. A
substantial upstream restructure would invalidate the citation.

**Audit class:** citation robustness (not Convention #8 — the citation
is currently accurate).

**Severity:** Very low; an in-tree pin removes ambiguity for future
audits.

**Disposition:** Add SHA pin to the polyring banking section in
`load-bearing-decisions.md`. See commit 3 (folded into the same
load-bearing-decisions update).

### B.7 Findings NOT found (negative results)

For audit completeness — these are things probe-1 actively looked for
and did not find:

- **No deliberate-skeleton code paths** in numerical, state, backend,
  or panel logic. Probe-1 Section N enumerates three `n_grid`-arg-unused
  cases and the `do_load_state` `pass`-marker; all four are documented
  as deferred-API-symmetry or no-op-marker patterns, not hidden
  incompleteness.
- **No `TODO`/`FIXME`/`XXX` markers** anywhere in the sim source.
  Banking is uniformly via `banked` / `v1.1` comments. Convention
  compliance is clean.
- **Test coverage matches spec intent** despite test name drift. All
  three load-bearing CI tests exist and exercise their named contracts.
- **Quad4 kernel is implemented bit-for-bit per upstream**: code form
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
  at `kernels.py:73` (and `:88` for 3D) is `(4.0 * r * (1.0 - r)) ** 4`
  with the `if r < 1.0` unit-disc support, matching upstream
  `kernel_core[0]`. The v1→v2 spec correction (bump4 → quad4) is
  reflected in the shipped source.
- **All preset citations to `animals.json`** are real; probe-1 Section
  M enumerates 27 distinct citation sites across source and docs, all
  pointing at real upstream artifacts.

## Section C: Commit-sequence proposal

Five commits, all doc-only or housekeeping. No behavior change. Each
commit is independently revertable and lands its own SHA.

### Commit 1: Fix numpy round-trip docstring contradiction

**Files touched:** `continuous-ca/lenia-fft/python/lenia_fft/main.py`

**Change:** Rewrite `step_2d` docstring (current lines 226–231) to
accurately describe the round-trip cost.

**Proposed text** (replacement for the second paragraph of the docstring):

```
The full real-space-backend cost per step is two numpy↔Taichi round
trips: one in this wrapper (state.state_2d.to_numpy() →
convolver.step(state_np) → state.state_2d.from_numpy(...)) and one
inside TaichiRealSpaceConvolver.step itself (the backend populates
the shared state.state_2d field via from_numpy, runs the Taichi
kernel, reads back via to_numpy). The pattern is acceptable for
backend-agnostic dispatch but is the obvious perf-tuning opportunity
when the real-space path is the bottleneck — see
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
fft_backend.py:18-23 for the cross-reference.
```

**Rationale:** Aligns the `main.py` docstring with the
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
`fft_backend.py:18-23` comment that already correctly describes the
behavior. No behavior change.

**Verification post-landing:** `grep -nE "skips the numpy round-trip"
continuous-ca/lenia-fft/` should return zero hits.

### Commit 2: Reconcile spec test names with shipped test names

**Files touched:** `continuous-ca/lenia-fft/docs/load-bearing-decisions.md`

**Change:** Add a short subsection under the relevant CI section noting
that the three load-bearing CI tests use the more-specific names
`test_select_backend_factory_falls_back`,
`test_apply_preset_stability_2d`, and `test_capture_schema_round_trip`,
which differ from the wording in `phase10_lenia_fft_v2.md` § 5 (`_falls_back`
and `_2d` were added in shipped source for descriptive precision).

**Rationale:** Avoids modifying the shipped Phase 10 spec (which is a
historical artifact and not load-bearing for future work) while making
the actual names easy to find from the sim's own documentation.
project-state.md line 656 already uses the actual names; this brings
the sim-local doc into alignment with project-state.md.

**Alternative considered + rejected:** Editing the shipped Phase 10
v2 spec to use the actual names. Rejected because phase specs are
historical artifacts; their drift relative to shipped source is
expected and documented elsewhere.

### Commit 3: Expand load-bearing-decisions for GGUI Y-asymmetry, polyring banking SHA pin, b-field framing

**Files touched:** `continuous-ca/lenia-fft/docs/load-bearing-decisions.md`

**Change:** Three load-bearing-decisions edits in one commit (related —
all are post-audit clarifications to existing banking entries):

(a) **GGUI Y-asymmetry section** (currently exists per probe-1
Section J / `notes.md` Polish-4 reference):

Add a "Structural assessment + breakage conditions" paragraph that
names:

- the asymmetry as a load-bearing coincidence, not structural soundness;
- the breakage conditions: consumer #3 with panels outside the
  upper-left quadrant `x ∈ [0.02, 0.24], y ∈ [0.02, 0.99]`, OR a
  refactor of `gpusims_common.ParamPanel` internals that changes the
  panel-positioner y-coord convention;
- the most-likely investigation entry point: ParamPanel's
  `folder()` panel-positioner appears to be flipped relative to its
  documented y=0-at-top convention, and the `cursor_in_any_panel` flip
  double-corrects for it. Pinning this down requires reading
  ParamPanel internals (out-of-scope for lenia-fft).

(b) **Polyring banking section** — add a SHA pin to the upstream
citation:

```
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Upstream reference: Chakazul/Lenia/Python/LeniaNDK.py:329-335
                   at master HEAD adfc542939266de7f4bb7ebb552e8499701ee107
                   (verified by Layer 3 Batch B audit, 2026-05-14;
                   see docs/diagnostics/_audits/sims_lenia_chakazul_2026-05-14_architect3b.md)
```

(c) **b-field data-model clarification** — add one sentence under the
polyring banking note:

```
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The current LeniaPreset dataclass (presets.py:57) carries no b or kn
field; v1 ships single-peak via a hard-wired single-shell quad4 kernel,
not via a constrained b="1" parameter. The v1.1 polyring extension
therefore requires a LeniaPreset schema addition (b: str, kn: int)
alongside the new init_kernel_polyring_*d kernels — not just a
multi-peak loop in an existing b-aware kernel.
```

**Rationale:** Three related clarifications, all about already-banked
items. Folding into one commit keeps the commit count manageable; each
edit is independently citable from this audit report. No behavior
change.

### Commit 4: Gitignore + remove `imgui.ini`

**Files touched:**
- `continuous-ca/lenia-fft/python/.gitignore` (or
  `continuous-ca/lenia-fft/.gitignore` if it exists)
- `continuous-ca/lenia-fft/python/imgui.ini` (removed)
- Cross-check: `hybrid-particle-grid/mpm-multimaterial/python/imgui.ini`
  and `common/common-py/examples/hello/imgui.ini` for parallel hygiene
  (probe-1 didn't grep this; the commit prompt should — if found,
  include in this commit).

**Change:** Append `imgui.ini` (or equivalent glob if multiple) to the
appropriate .gitignore. Remove the tracked `imgui.ini` from the tree.

**Rationale:** GGUI writes panel-position state to this file on every
panel move; committing it makes git-status noisy. Source-control
hygiene.

**Verification post-landing:** `git status --short` after moving panels
in any Stack D sim should show no `imgui.ini` modifications.

### Commit 5: Audit reference back-fill in project-state.md

**Files touched:** `docs/project-state.md`

**Change:** Add a short "Audit references" entry to project-state.md
§ 9 (Known issues) cross-referencing this audit's outputs:

```
- Lenia-FFT Layer 3 Batch B audit (2026-05-14). Three artifacts in
  docs/diagnostics/_audits/:
    sims_lenia_chakazul_2026-05-14_architect3b.md (external upstream verification)
    sims_lenia_probe1_2026-05-14_architect3b.md (structural inventory)
    sims_lenia_synthesis_2026-05-14_architect3b.md (synthesis + commit sequence)
  Findings: 1 in-source docstring contradiction (numpy round-trip;
  fixed at commit 1 above), 1 confirmed structural coincidence (GGUI
  Y-asymmetry; banked at commit 3), spec-source drift on test names
  (acknowledged at commit 2), polyring SHA pin (commit 3), imgui.ini
  hygiene (commit 4). Audit triage-flagged Chakazul-citation
  fabrication smell REFUTED.
```

**Rationale:** Makes the audit findings discoverable from
project-state.md without forcing readers to dig through the audit
directory. Same shape as the existing Phase-9-retro entries at lines
525 / 543 / 656.

### Commit sequencing

Commits 1, 2, 3, 4 are independent and can land in any order. Commit
5 should land last so it can reference the final SHAs of the prior four.
Layer 1 Phase 11.5 used the same pattern (substantive commits land
first; reference back-fill last; SHAs back-filled at retro time, never
via `--amend`, per Convention #12 in the user memory).

**Recommended landing order:** 1 → 2 → 3 → 4 → 5.

**Suggested commit-message prefix convention** (matching the project's
existing pattern from Phase 11.5):

- Commit 1: `lenia-fft: align step_2d docstring with actual round-trip cost (audit Layer 3 Batch B)`
- Commit 2: `lenia-fft: acknowledge CI test name drift vs Phase 10 spec (audit Layer 3 Batch B)`
- Commit 3: `lenia-fft: clarify GGUI Y-asymmetry, polyring SHA pin, b-field framing (audit Layer 3 Batch B)`
- Commit 4: `gpusims: gitignore imgui.ini in Stack D sims (audit Layer 3 Batch B)`
- Commit 5: `docs: link Layer 3 Batch B lenia-fft audit from project-state (audit Layer 3 Batch B)`

## Section D: Cross-workstream flags for common-py audit

Three items surfaced by probe-1 Section O that are out of scope for
this audit (per the brief: "do not deep-audit common-py itself; if
common-py is the load-bearing constraint for either sim, flag for a
future Layer 2-equivalent audit"). All three are blocking the lenia-fft
findings from being fully resolved at the sim layer; all three are
appropriate for a future common-py audit layer.

### D.1 `ParamPanel.folder()` y-coord convention is the unresolved root of B.3

The GGUI Y-asymmetry diagnosed in § B.3 most likely stems from
`gpusims_common.ParamPanel`'s panel-positioner being flipped relative
to its documented y=0-at-top convention. The `cursor_in_any_panel`
flip then double-corrects for an upstream flip already inside
ParamPanel.

Conclusively distinguishing "ParamPanel is flipped" from "GGUI cursor
convention disagrees with ParamPanel panel-position convention by
design" requires reading ParamPanel's panel-positioner code, which is
in common-py.

### D.2 `StateWriter`/`StateReader` per-buffer schema fields are not introspected

The cross-stack capture schema documented at
`docs/tier1-capture-format-reference.md` § 1 specifies per-buffer
`{count, stride, format, shape}` fields. Probe-1 Section H confirms
that lenia's `do_save_state` invokes the StateWriter API as documented
and `test_capture_schema_round_trip` exercises the round trip — but
the test asserts byte-identity of state/lut and presence of the
`leniaFft` meta key only. The per-buffer `count/stride/format/shape`
fields themselves are not inspected; schema drift inside common-py
would not be caught at this layer.

### D.3 `VdbWriter` stub-mode behavior is opaque from the sim

`continuous-ca/lenia-fft/python/lenia_fft/main.py:553–560` issues a
`VdbWriter.write_frame()` call unconditionally; the GUI label flips
between "real" and "stub" based on `VdbWriter.is_available()`, but
the call still fires when in stub mode. Whether stub-mode is a
silent no-op, a warn-and-no-op, or has some other side-effect is
opaque from this sim. Relevant to the 3D-tier export path; out of
scope here.

## Section E: Open items not actioned by the commit sequence

- **Polyring v1.1+ extension.** Banked correctly; this audit does not
  change banking. The b-field clarification in commit 3(c) tightens the
  framing but does not advance v1.1 implementation.
- **GGUI Y-asymmetry full fix.** Pending consumer #3 promotion review
  and/or the common-py audit (§ D.1). Commit 3(a) clarifies the
  breakage conditions; the fix itself is deferred.
- **Performance investigation of the wrapper-side round trip.** The
  fixed docstring (commit 1) explicitly identifies the wrapper round
  trip as a perf-tuning opportunity. Whether to take it is a v1.1
  question; not actioned here.

## Section F: Verdict

**lenia-fft is in broadly healthy structural shape.** The triage's
high-priority Convention #8 fabrication-smell flag against the
Chakazul citation is refuted by external upstream verification. The
medium-priority GGUI Y-asymmetry structural-coincidence flag is
confirmed and bankable with a clarification commit. Two minor in-source
contradictions (numpy round-trip docstring; spec test-name drift) and
one hygiene item (imgui.ini) are addressable in doc-only commits. No
deliberate-skeleton code paths exist in the actual numerical, state,
backend, or panel logic.

The project's existing Convention #8 retraction culture (Phase 10 v1
bump4-vs-quad4, Phase 10 polish-4 GGUI banking, Phase 9 polish-4 60 fps
perf extrapolation) caught and documented prior incidents in this sim
before audit time. This audit adds two further findings (numpy
round-trip docstring; spec test-name drift) without uncovering a
hidden pattern.

**Five doc-only / hygiene commits proposed. No behavior change.**

End of synthesis. Ready for user review and commit-sequence dispatch.
