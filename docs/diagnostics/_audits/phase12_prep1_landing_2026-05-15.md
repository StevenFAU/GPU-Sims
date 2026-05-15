---
title: Phase 12 setup-1 landing — vendor lbm-principles-practice (Krüger) at 6e2c592f
date: 2026-05-15
author: claude-code
phase: 12
status: landed
scope: setup-1-Krueger-vendoring
---

# Phase 12 setup-1 landing — vendor lbm-principles-practice (Krüger)

## Summary

Vendors the Krüger et al. *Lattice Boltzmann: Principles and Practice*
textbook companion code (MIT-licensed) under
`references/lbm-principles-practice/` as a math-pattern reference for
Phase 12 (lattice-boltzmann). The repository is **D2Q9 only**; Phase 12's
GPU shaders target **D3Q19**, so this anchor is used for the BGK
equilibrium pattern and halfway bounce-back convention only. The 3D
velocity set and ω_i weights derive from a separate algebraic registry
entry landing in setup commit 2.

## File inventory

Vendored clone (gitignored, not tracked):

```
references/lbm-principles-practice/
├── .git/                  (depth-1 clone history)
├── .gitignore
├── LICENSE.txt            (MIT, Copyright 2016 Krüger et al.)
├── README.md
├── chapter5/              (4 files; halfway bounce-back, Poiseuille BB)
├── chapter6/              (2 files)
├── chapter8/              (9 files)
├── chapter9/              (1 file)
├── chapter11/             (1 file)
└── chapter13/             (38 files; CPU + GPU LBM implementations)
```

Total vendored: 58 files (excluding `.git/`), ~724 KB on disk including
`.git/`.

## Tracked-file diff stats

```
 .gitignore                                   |  8 ++++++++
 tools/integrity/docs/ground-truth-sources.md | 18 ++++++++++++++++++
 2 files changed, 26 insertions(+)
```

Both modifications are append-only adjacent to the existing SPlisHSPlasH
precedent — no edits elsewhere in the tree.

### `.gitignore` change

Appended a comment-only block after the existing SPlisHSPlasH block
(documenting that `lbm-principles-practice/` lives under the
already-ignored `/references/` directory). The existing `/references/`
line covers the new clone; no second directive added.

### `tools/integrity/docs/ground-truth-sources.md` change

Appended a `[Krueger]` TOML stanza inside the existing `## v1 registry`
fenced block (after `[SPlisHSPlasH]`) and a matching paragraph in the
`## Notes on v1 registry contents` section documenting the D2Q9
dimensional-scope constraint and the chapter5/chapter13 citation
references.

## Resolved SHA confirmation

```
$ cd references/lbm-principles-practice && git rev-parse HEAD
6e2c592fdc3592c14dfd52f860fc1ceea930bcb0
```

Matches the spec's anchor SHA exactly. Upstream
`https://github.com/lbm-principles-practice/code` HEAD at clone time
(2026-05-15) is the same SHA recorded by the pre-draft probe — no
upstream drift since the characterization.

License: MIT, verified at `references/lbm-principles-practice/LICENSE.txt`
(Copyright (c) 2016 Timm Krüger, Halim Kusumaatmaja, Alexandr Kuzmin,
Orest Shardt, Goncalo Silva, Erlend Magnus Viggen).

## Integrity toolkit output

```
$ cd tools/integrity && python3 -m integrity --check cat1.upstream-anchor
integrity: 1 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
```

Pass: the cloned `references/lbm-principles-practice/.git` HEAD matches
the registered `[Krueger].anchor_sha` and the anchor doc (`LICENSE.txt`)
is present at the registered `vendor_root`.

## Cross-references

- `docs/diagnostics/_audits/phase12_lbm_probe_2026-05-15_architect1.md`
  — initial Phase 12 lattice-boltzmann probe
- `docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md`
  — predraft probe and D3Q19/D2Q9 dimensional-scope finding
- Phase 11.5 setup-1 precedent: SPlisHSPlasH vendoring shape mirrored
  here for the comment-block, registry-entry, and notes-section
  conventions.

## Follow-up (setup commit 2)

A separate setup commit will register `[Algebraic_D3Q19]` under
`tools/integrity/docs/algebraic/d3q19.md`, providing the 3D velocity set
and ω_i weights that this anchor does not cover.
