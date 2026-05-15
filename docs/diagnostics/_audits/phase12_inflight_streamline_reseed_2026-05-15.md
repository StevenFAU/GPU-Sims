---
title: Phase 12 in-flight streamline reseed fix
date: 2026-05-15
author: claude-code
phase: 12
status: landed
scope: in-flight-fix
substantive_anchor: d41564d
xcommit_anchor: 47104ad
backfill_anchor: 8fc7a08
---

# Phase 12 in-flight streamline reseed fix

In-flight fix for two correctness defects + one polish improvement
surfaced at Checkpoint 6 (visual verification on RX 6800 XT). LBM
physics is unchanged; Checkpoints 1–5 remain valid. Per § 6.D of the
spec, in-flight fixes land as separate commits, never `--amend`.

## Diff stats

```
volumetric-grid/lattice-boltzmann/src/main.cpp                | 24 +++++++++++++++++++---
volumetric-grid/lattice-boltzmann/shaders/streamline_advect.comp.glsl | 15 +++++++++++++-
2 files changed, 35 insertions(+), 4 deletions(-)
```

## Defect 1 — synchronized reseed bursts

**Symptom:** all ~10k streamlines reseeded simultaneously every
~1.76 sec at 145 FPS, producing a visible burst.

**Root cause:** `seed_streamlines()` initialised every history slot
to `vec4(0,0,0, age=STREAMLINE_RESEED_AGE)`, so every streamline
crossed the reseed threshold in the same frame and stayed phase-locked
across the whole 256-frame cycle.

**Fix:** at initial seed, randomise each streamline's age over
`[0, STREAMLINE_RESEED_AGE)` using `std::mt19937` seeded with `12345`
(reproducible). Position assignment to all history slots stays the
same; only the age field is randomised per streamline.

**File:** `volumetric-grid/lattice-boltzmann/src/main.cpp`,
`seed_streamlines` lambda.

## Defect 2 — teleport-jump in ring buffer on reseed

**Symptom:** when a streamline reseeded, the line-strip vertex shader
drew a line from the new inlet position straight to the 63 stale
positions left in the ring buffer from the previous incarnation —
visible as bright "white flash" sheets across the volume.

**Root cause:** the GPU reseed path in
`streamline_advect.comp.glsl` only wrote the new position to the
`head_index` slot; the other 63 slots retained their pre-reseed
contents.

**Fix:** track `bool reseeding_this_frame` inside the existing
reseed branch; when true, write the new `vec4(pos, age)` to ALL
`U.history` slots, not just `head_index`. The non-reseed branch
keeps the single-slot head write.

**File:** `volumetric-grid/lattice-boltzmann/shaders/streamline_advect.comp.glsl`.

## Polish — velmag colormap auto-calibration

**Symptom:** wake-structure contrast was washed out at default
colormap range.

**Root cause:** Runtime defaults `velmagMin=0.0, velmagMax=0.1`
spanned far wider than the actual `|u|` range across presets
(`u_inf in [0.04, 0.06]`).

**Fix:** in `apply_preset()`, after `rt.uInfMagnitude = P.u_inf`,
set `rt.velmagMin = 0.0f` and `rt.velmagMax = 1.5f *
rt.uInfMagnitude`. Default Runtime values stay as initialised; the
preset apply overrides per preset.

**File:** `volumetric-grid/lattice-boltzmann/src/main.cpp`,
`apply_preset` lambda.

## Build verification

```
$ cmake --build build-release --target lattice_boltzmann --parallel
[2/2] Linking CXX executable bin/lattice_boltzmann
```

Clean rebuild. Only the pre-existing `VkSamplerCreateInfo`
partial-init `-Wmissing-field-initializers` warnings (also emitted
by eulerian-smoke), no new warnings.

## What this commit does NOT change

- LBM physics (collide / stream / boundaries / moments unchanged).
- Checkpoints 1–4 unchanged (lattice constants, common-cpp surface,
  substep-at-equilibrium trace, OPPOSITE_DIR involution all remain
  green from the substantive landing).
- The streamline-buffer allocation footprint, descriptor-set wiring,
  pipeline definitions, capture/load schema.
