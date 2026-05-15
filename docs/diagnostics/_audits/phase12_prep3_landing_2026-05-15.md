---
title: Phase 12 setup-3 landing — architect-1 spec
date: 2026-05-15
author: claude-code
phase: 12
status: landed
scope: setup-3-spec-landing
---

# Phase 12 setup-3 landing audit

Third and final prep commit for Phase 12 (lattice-boltzmann). Lands the
architect-1 substantive-execution spec byte-for-byte at the canonical
location. The substantive Phase 12 commit will read this spec
top-to-bottom for execution.

## File inventory

| Path | Kind | Notes |
|---|---|---|
| `docs/phase12_lattice_boltzmann.md` | new | architect-1 spec, copied byte-for-byte from `/home/otacon/Downloads/phase12_lattice_boltzmann.md` |

## Diff stats

```
$ wc -l docs/phase12_lattice_boltzmann.md
3390 docs/phase12_lattice_boltzmann.md
```

One file added; 3390 lines.

## SHA-1 of landed spec

```
$ sha1sum /home/otacon/Downloads/phase12_lattice_boltzmann.md docs/phase12_lattice_boltzmann.md
001142a53df39b565282687c46a614c329933ca2  /home/otacon/Downloads/phase12_lattice_boltzmann.md
001142a53df39b565282687c46a614c329933ca2  docs/phase12_lattice_boltzmann.md
```

Source and destination hashes identical — byte-equality confirmed.

## Toolkit gate

```
$ python3 -m integrity --check cat1.upstream-anchor
integrity: 1 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
```

Unchanged from setup-2 — no regression from the spec landing.

## Notes

- Source-path discrepancy: the prompt body referenced
  `/tmp/phase12_lattice_boltzmann_input.md` but the actual spec file
  lives at `/home/otacon/Downloads/phase12_lattice_boltzmann.md`
  (174087 bytes, matching the ~172 KB estimate). The latter path was
  surfaced by the architect at message top and used as the source.
- Destination per prompt: `docs/phase12_lattice_boltzmann.md` (docs
  root). Note: there is also `docs/sim-specs/lattice-boltzmann.md`
  which is the sim-brief, not the phase-execution spec — the two
  documents coexist.
- The "Phase 11 precedent" pre-flight (`docs/phase11_sph_water.md`)
  does not exist on disk; Phase 11's spec lives at
  `docs/sim-specs/sph-water.md`. Setup-3 still followed the prompt's
  prescribed destination path.

## References

- `docs/diagnostics/_audits/phase12_lbm_probe_2026-05-15_architect1.md`
  (pre-spec probe)
- `docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md`
  (Krüger characterization, RX 6800 XT subgroup-size measurements)
- Setup-1 commit `8fe355b` — vendored lbm-principles-practice at SHA `6e2c592f`
- Setup-2 commit `0db9c73` — Algebraic_D3Q19 derivation + verifier
