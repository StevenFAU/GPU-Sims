---
title: Lenia-FFT Chakazul Citation Verification (Layer 3 Batch B external probe)
date: 2026-05-14
author: architect-3b
layer: 3
batch: B
sim: continuous-ca/lenia-fft
status: complete
scope: external upstream verification (no clone, no vendoring, no commit)
verdict: CONFIRMED — citation resolves at master HEAD
cross_workstream: none
---

> Setup-1-analogue for the Layer 3 Batch B audit workstream. The Phase 10 spec
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
> banks a polyring kernel extension at `Chakazul/Lenia/Python/LeniaNDK.py:329-335`
> (no SHA pin). The triage probe flagged this as structurally similar to the
> fabricated SPlisHSPlasH 1.8.10 anchor that Layer 1 Setup-1 found at sph-water.
> This probe verifies the citation against upstream master HEAD and reports the
> outcome before any source-tree audit of lenia-fft itself. **Verdict: confirmed.**
> The cited line range is the polyring kernel-assembly function; the adjacent
> `kernel_core[0] = quad4` and `kn=1 → dict-key 0` claims also resolve cleanly.
> Section G flags two non-obvious findings that matter for probe-1 and for the
> eventual commit-sequence proposal.

## Section A: Anchor decision

The Phase 10 spec (`phase10_lenia_fft_v2.md`) makes three load-bearing claims
about the Chakazul/Lenia upstream that this probe verifies:

1. **Spec line 6 (banking statement):** "Polyring (multi-peak kernel via
   b-string) kernel extension banked v1.1+ with documented formula anchor at
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
   `Chakazul/Lenia/Python/LeniaNDK.py:329-335`".
2. **Spec line 6 (kernel-form correction):** "kernel index 1 (JSON kn=1 via
   off-by-one indexing, which maps to dict-key 0 in `LeniaNDK.py`'s
   `kernel_core` registry = quad4)".
3. **Spec line 4135 (verification provenance):** "Round 3 (architect-2 +
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
   Claude Code): ... polyring formula anchor at LeniaNDK.py:329-335".

The Phase 10 spec is shipped at commit `7065d32` (per `project-state.md` § 3
phase ledger). **The spec pins no SHA or tag for the Chakazul upstream** —
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
the citation is bare `LeniaNDK.py:329-335`. The natural anchor for this probe
is therefore upstream master HEAD as of audit time.

**Anchor selected for this probe:** Chakazul/Lenia master HEAD at
`adfc542939266de7f4bb7ebb552e8499701ee107`.

**Acknowledged limitation:** GitHub's REST commits-by-path endpoint was
rate-limited from the audit's egress IP during this probe, so I was unable
to retrieve the LeniaNDK.py commit history and confirm that lines 329-335
held the same content at architect-2 round-3 review time (approximately
May 2026, per Phase 10 spec preamble). The function structure verified
below (`kernel_shell` dispatching through a `kernel_core` registry) is
canonical Lenia-by-Chan and structurally mature (it is referenced from
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
the GUI code at `LeniaNDK.py:2176` as a stable internal API); a recent
restructure of these lines is unlikely but not externally falsified.

## Section B: Upstream existence verification

```
$ git ls-remote https://github.com/Chakazul/Lenia.git HEAD
adfc542939266de7f4bb7ebb552e8499701ee107	HEAD
```

```
$ git ls-remote --tags https://github.com/Chakazul/Lenia.git
d8704577841e7107fd74da729cbfab8501982980	refs/tags/v3.0
584eab49bc5c7b078d80554ccb17360a97279a60	refs/tags/v3.5
```

```
$ git ls-remote --heads https://github.com/Chakazul/Lenia.git
065904eaf6044c8730369743e990e7d4d5adfe2b	refs/heads/dependabot/pip/Python/mako-1.2.2
e3626142a84e16714fbf56f187cf880ba1c5d5db	refs/heads/dependabot/pip/Python/numpy-1.22.0
b0e61b7a3ad218022d4bdce0d1f9bcff74955bf6	refs/heads/dependabot/pip/Python/scipy-1.10.0
adfc542939266de7f4bb7ebb552e8499701ee107	refs/heads/master
```

- **Repo exists.** HEAD on master is `adfc542939266de7f4bb7ebb552e8499701ee107`.
- **Tags:** `v3.0`, `v3.5`. The spec's bare citation could reasonably resolve
  against any of {master HEAD, `v3.5`, `v3.0`}; this probe uses master HEAD.
- **Branches:** `master` + three dependabot-pip branches. No active development
  branch sequence — master is the canonical line.

Contrast with Layer 1 Setup-1's blocking probe: SPlisHSPlasH `1.8.10` did not
exist as a tag at all upstream (tag layout jumped from `1.3.1` to `2.0.0`).
Chakazul/Lenia's tag layout is plausible (`v3.0`, `v3.5`) and the repo is
real. **Existence verification passes.**

## Section C: File fetch

```
$ curl -sSL -o LeniaNDK.py \
    'https://raw.githubusercontent.com/Chakazul/Lenia/master/Python/LeniaNDK.py'
$ wc -l LeniaNDK.py
2610 LeniaNDK.py
```

- **File exists** at the cited upstream path `Python/LeniaNDK.py`.
- **Line count:** 2610. The cited range (329-335) is safely within the file.

## Section D: Citation audit

### D.1 Citation table

| # | Spec claim | Source | Upstream evidence | Verdict |
|---|---|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 1 | "polyring kernel extension at LeniaNDK.py:329-335" | spec line 6 (banking) | `kernel_shell` method at 329-335 implements polyring assembly via b-string | **CONFIRMED** |
| 2 | "kernel_core[0] = quad4 = (4r(1-r))^4" | spec line 6 (correction) | `kernel_core` registry at 289-294, key 0 is `lambda r: (4 * r * (1-r))**4` | **CONFIRMED** |
| 3 | "off-by-one kn=1 → dict-key 0" | spec line 6 (correction) | `kernel_shell:334` reads `Automaton.kernel_core[params.get('kn') - 1]` | **CONFIRMED** |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 4 | "polyring formula anchor at LeniaNDK.py:329-335" | spec line 4135 (provenance) | same as #1 | **CONFIRMED** |

### D.2 Verbatim evidence

**Citation 1 + 4 — the cited line range, master HEAD `adfc542`:**

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
```python:Chakazul/Lenia/Python/LeniaNDK.py:329
    def kernel_shell(self, r, params):
        B = len(params['b'])
        Br = B * r
        bs = np.asarray([float(f) for f in params['b']])
        b = bs[np.minimum(np.floor(Br).astype(int), B-1)]
        kfunc = Automaton.kernel_core[params.get('kn') - 1]
        return (r<1) * kfunc(np.minimum(Br % 1, 1)) * b
```

This is the polyring kernel assembly. Reading the function:

- `params['b']` is the b-string — the list of per-ring peak heights (single-peak
  is `[1]`, multi-peak is e.g. `[1, 0.5, 1]`).
- `B = len(params['b'])` is the number of concentric rings.
- `Br = B * r` re-scales the radial coordinate so each ring occupies a full
  `[0,1]` interval.
- `bs[floor(Br)]` selects the peak height for whichever ring `r` falls into.
- `kfunc = kernel_core[kn-1]` dispatches to the unit-disc kernel function
  (quad4 for kn=1, bump4 for kn=2, etc).
- `kfunc(Br % 1)` evaluates the kernel function on the fractional position
  within the current ring.
- `(r<1) * ... * b` zeros the kernel outside the unit disc and weights by the
  peak height.

This is precisely the b-string-driven multi-peak kernel construction that the
Phase 10 spec banks for v1.1+. The line citation is structurally correct.

**Citation 2 — the kernel_core registry, lines 289-294:**

// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
```python:Chakazul/Lenia/Python/LeniaNDK.py:289
    kernel_core = {
        0: lambda r: (4 * r * (1-r))**4,  # polynomial (quad4)
        1: lambda r: np.exp( 4 - 1 / (r * (1-r)) ),  # exponential / gaussian bump (bump4)
        2: lambda r, q=1/4: (r>=q)*(r<=1-q),  # step (stpz1/4)
        3: lambda r, q=1/4: (r>=q)*(r<=1-q) + (r<q)*0.5 # staircase (life)
    }
```

Key 0 IS `lambda r: (4 * r * (1-r))**4`. The spec's claim that
`kernel_core[0] = quad4 = (4r(1-r))^4` matches upstream byte-for-byte.

The spec's claim that key 1 is bump4 also matches: `lambda r: np.exp(4 - 1/(r*(1-r)))`.

**Citation 3 — the off-by-one dispatch:**

The off-by-one is at line 334 inside `kernel_shell` (the same range as Citation 1):

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
```python:Chakazul/Lenia/Python/LeniaNDK.py:334
        kfunc = Automaton.kernel_core[params.get('kn') - 1]
```

`kn=1` (from `animals.json`) becomes index `0` in the registry, which is quad4.
`kn=2` would be bump4, and so on.

### D.3 Summary statistics

- **CONFIRMED:** 4 / 4
- **DRIFT:** 0
- **NOT_FOUND:** 0
- **Overall match rate:** 100% against master HEAD `adfc542`.

## Section E: Contrast with Layer 1 Setup-1

Layer 1's blocking probe (`phase11_5_setup1_2026-05-14_blocked.md`) found:

1. SPlisHSPlasH tag `1.8.10` did not exist upstream (tag layout: `1.1.0`,
   `1.2.0`, `1.3.0`, `1.3.1`, then jumped to `2.0.0`).
2. The SHA `c254caf2705ebf5271408dd37a091aa379258a38` was a copy-paste from
   an unrelated Alembic citation in the same `load-bearing-decisions.md`.

Both anchor components were fabricated.

This probe finds:

1. The Chakazul/Lenia repo exists, master HEAD is `adfc542`, tag layout
   (`v3.0`, `v3.5`) is plausible.
2. The cited file `Python/LeniaNDK.py` exists at 2610 lines.
3. The cited line range (329-335) contains the function the spec claims
   it contains.
4. The adjacent claims (`kernel_core[0] = quad4`, `kn-1` off-by-one) are
   also verified.

**The two cases are structurally different despite the triage's structural
similarity flag.** The SPlisHSPlasH case was a confident-recall fabrication
in a load-bearing source-of-truth (architect's own decision doc, naming a
specific tag and SHA). The Chakazul case is a bare-line citation against a
real upstream file, and the citation is accurate. The fabrication-smell
signal — "precise upstream citation, no vendored reference" — is necessary
but not sufficient for fabrication; the upstream verification is the
sufficient test, and Chakazul passes it.

## Section F: Why the citation appears to have held up

The spec's verification provenance at line 4133-4137 names five Claude
Code verification rounds during draft:

> Round 3 (architect-2 + Claude Code): full 122-creature single-peak
> enumeration vs `animals.json`; Hydrogeminium polyring confirmation;
> LeniaNDK.py kernel-core registry inspection; polyring formula anchor
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
> at LeniaNDK.py:329-335

The architect-2 round-3 cross-review explicitly ran a Claude Code probe
against upstream `LeniaNDK.py`, inspected the `kernel_core` registry, and
recorded the polyring anchor at 329-335. This is exactly the
chat-orchestrates / Claude-Code-verifies discipline that the project's
diagnostic toolchain documentation describes. The probe report was
archived at `/tmp/p10presets/phase10_preset_report.md` — ephemeral, not
in the repo — but its findings landed in the spec correctly.

**The mechanism that protected this citation from Convention #8 fabrication
is the same mechanism the project explicitly documents as protection
against Convention #8: ground-truth verification via Claude Code against
the actual upstream artifact, performed during architect-2 cross-review.**
The Phase 11 sph-water SPlisHSPlasH anchor was not protected by this
mechanism — `load-bearing-decisions.md` contained the fabricated tag from
the original sim authoring, not from a draft-time architect-2 + Claude
Code verification round. The architectural lesson is that the
verification discipline works when it is applied; the failures show up
where it wasn't.

## Section G: Incidental findings

### G.1 The Phase 10 spec itself documents a v1 Convention #8 incident

`phase10_lenia_fft_v2.md` line 6 contains a long inline retraction:

> **The kernel formula is upstream-faithful quad4: `K(r) = (4·r·(1-r))^4`
> for `r ∈ [0,1]`, zero outside — NOT the bump4 `exp(4 - 1/(r·(1-r)))`
> formula in this spec's v1.** The bump4 formula in v1 was upstream's
> kernel index 2 (JSON kn=2); the cited creatures all use upstream's
> kernel index 1 (JSON kn=1 via off-by-one indexing, which maps to
> dict-key 0 in `LeniaNDK.py`'s `kernel_core` registry = quad4).

This is **a documented Convention #8 incident in lenia-fft's draft
history**: architect-1's v1 draft confidently cited the bump4 formula
as the canonical Lenia kernel, against animals-JSON creature data
that all use quad4. The mismatch was caught by architect-2 round-3
verification (`/tmp/p10presets/phase10_preset_report.md`) and corrected
in v2 before ship.

Pattern match against `project-state.md:543` (MPM perf extrapolation,
retracted at polish-4): same mechanism — an architect's confident-recall
upstream claim disproved by ground-truth verification against the actual
upstream artifact, retraction landed in tree.

**Implication:** lenia-fft and mpm-multimaterial each carry at least one
in-tree-documented Convention #8 incident already. The audit reports
should cite these as project-internal precedent for the pattern, not as
a separate finding the audit is surfacing.

### G.2 The spec does not pin a SHA for the Chakazul upstream

// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
The Phase 10 spec banks `LeniaNDK.py:329-335` with no SHA or tag pin.
This worked at draft time because architect-2 round-3 verified against
the then-current upstream, and the lines happened to be in the same
position at audit time (master HEAD `adfc542`). It will keep working
as long as upstream doesn't substantially restructure that file.

**Recommendation for commit-sequence phase:** propose a doc-only follow-on
commit updating `continuous-ca/lenia-fft/docs/load-bearing-decisions.md`'s
polyring-banking section to add the SHA pin
`adfc542939266de7f4bb7ebb552e8499701ee107` (or master HEAD at audit time).
Shape: same as the Phase 11.5 sph-water Setup-1 `load-bearing-decisions.md`
out-of-band fix (Section G.5 of Layer 1's Setup-1 report). One-line citation
strengthening, zero code change, eliminates the ambiguity for future
audits.

Not in scope to apply here. Banked for the commit-sequence proposal.

### G.3 The probe-1 audit of lenia-fft source can rely on this finding

With the Chakazul citation verified, probe-1 against `continuous-ca/lenia-fft/`
can focus on:

- Whether the **source code** implements quad4 (not bump4) — verifying the
  v1→v2 spec correction actually shipped.
- Whether the preset table's b-strings match `animals.json` byte-for-byte
  for the four cited creatures.
- The banked-marker density (7 `banked` + 4 `stub` + 7 `v1.1` per the
  triage probe's count).
- The GGUI Y-convention asymmetry between `cursor_to_field_cell` and
  `cursor_in_any_panel`.
- Whether v1.1 polyring banking lives in a banked-not-shipped code path
  or has any partial implementation that could mislead future maintainers.

The upstream-citation surface is closed.

## Section H: Verdict and handoff

// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
**Verdict:** CONFIRMED. The Phase 10 spec's `Chakazul/Lenia/Python/LeniaNDK.py:329-335`
citation, and the adjacent kernel_core / kn-off-by-one claims, resolve cleanly
against upstream master HEAD `adfc542939266de7f4bb7ebb552e8499701ee107`.

**Triage flag status:** The high-priority Convention #8 fabrication-smell flag
that the triage probe raised on structural-similarity-to-SPlisHSPlasH grounds
is REFUTED for this citation.

**Next probe:** lenia-fft structural inventory (probe-1), filed as
`sims_lenia_probe1_2026-05-14_architect3b.md`.

End of Chakazul citation verification.
