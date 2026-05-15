---
title: Phase 12 in-flight fix #2 — streamline seed-slab + dt_render units
date: 2026-05-15
author: claude-code
phase: 12
status: landed
scope: in-flight-fix-2
substantive_anchor: d41564d
prior_inflight_anchor: c1a257d
---

# Phase 12 in-flight fix #2 — streamline seed-slab + dt_render units

Two compounding bugs in the streamline rendering surfaced at the
post-c1a257d visual verification on RX 6800 XT. Both are pre-existing
spec-vs-execution drift that was masked by an unintended side-effect
in the original `seed_streamlines` (all streamlines initialized at
`age=STREAMLINE_RESEED_AGE` so the GPU bumped them all on frame 1).
The c1a257d age randomization was correct in itself but removed the
masking side-effect, exposing the underlying defects.

LBM physics is unchanged. Checkpoints 1–5 remain valid.

## Defect 3 — initial seed position is `(0,0,0)`, not inlet slab

**Symptom:** after `seed_streamlines()` (initial launch, preset
change, tier change, streamline-count change), most streamlines were
clustered at the volume corner `(0,0,0)` — out of typical camera
view — and stayed there for ~256 frames as the GPU's age-threshold
reseed slowly cycled them into real inlet positions. User-visible:
"streamlines no longer visible in normal operation."

**Root cause:** `seed_streamlines()` hardcoded `const glm::vec3
pos(0.0f)` and wrote that to every history slot. The original spec
§ 4.B.10 specified inlet-slab seeding (matching the GPU reseed-branch
slab in `streamline_advect.comp.glsl`); the landed code seeded at
the corner instead. **Pre-c1a257d this was masked**: every streamline
had `age = STREAMLINE_RESEED_AGE` at seed time, so on frame 1 the
GPU's age-threshold check fired for all of them and they were all
bumped to real inlet-slab positions by the GPU reseed branch. The
c1a257d randomization into `[0, STREAMLINE_RESEED_AGE)` left most
streamlines below threshold, so they sat at `(0,0,0)` waiting for
their age to tick up.

**Fix:** `seed_streamlines()` now picks a random inlet-slab position
per streamline using the same slab geometry as the GPU reseed branch
(`x ∈ [Nx/16, Nx/8]`, `y ∈ [0, Ny]`, `z ∈ [0, Nz]`). The single
random position is written to all 64 history slots per streamline,
matching the GPU reseed-branch behavior. The c1a257d age
randomization is preserved.

**File:** `volumetric-grid/lattice-boltzmann/src/main.cpp`,
`seed_streamlines` lambda.

## Defect 4 — `dt_render` units mismatch (seconds vs lattice steps)

**Symptom:** even with streamlines correctly placed, individual line
strips were sub-voxel-length and visually point-like rather than
trail-like. The visible "haze" was dominated by the population
distribution rather than per-streamline trails, and "preset change"
visuals had a sluggish washed-out character.

**Root cause:** the host computed
```cpp
ssu.dt_render = std::clamp(rt.lastFrameMs * 0.001f, 1.0e-3f, 0.05f);
```
which converts last-frame milliseconds to seconds and clamps to
`[1e-3, 5e-2]`. The shader, however, multiplies `dt_render * u`
where `u` is in **lattice-units-per-lattice-step**, not
lattice-units-per-second. At 145 FPS with `u_inf = 0.04`, per-frame
advance was `0.0069 × 0.04 ≈ 0.00028` voxels — three orders of
magnitude smaller than expected. Spec § 4.B.12 had this units
mismatch baked in; nothing in the 6-checkpoint protocol caught it
because the trace at Checkpoint 3 examined LBM substep correctness,
not streamline rendering kinematics.

**Fix:**
```cpp
ssu.dt_render = std::min(float(rt.substeps), 4.0f);
```
`dt_render` is now `rt.substeps` lattice steps per render frame
(the actual number of LBM steps advanced by the dispatch chain
between two streamline-advect calls), capped at `4.0` so a high
`substeps` setting (16) doesn't push streamlines off the domain in
one render frame at near-max `u_inf`.

At default `substeps = 1`, `dt_render = 1.0`. Per-frame advance =
`1.0 × 0.04 = 0.04` voxels. Over 64-frame ring buffer = `~2.5`
voxels of trail length per streamline. That matches the spec's
implied design ("64-position ring buffer gives a visible streak
length without unbounded memory").

**File:** `volumetric-grid/lattice-boltzmann/src/main.cpp`,
streamline_advect dispatch site (line ~1609).

## c1a257d's role

The age-randomization landed at c1a257d (defect #1: synchronized
reseed bursts) was **correct in isolation** — staggering reseed
events across the cycle is the right behavior. But it removed the
side-effect that masked Defect 3: pre-c1a257d, all-`age=threshold`
seeding meant the GPU bumped every streamline to a valid inlet
position on frame 1, hiding the wrong CPU-side position. c1a257d
is the test that surfaced the latent defect, not the cause of it.

The same reasoning applies to Defect 4: pre-c1a257d's frame-1 GPU
reseed put streamlines at fresh inlet positions every frame for
all streamlines, so the visible "haze" was dominated by the
inlet-slab seeding distribution, not per-streamline drift. The
sub-voxel `dt_render` was always wrong, but the visual was masked
by the perpetual-frame-1-reseed dynamic.

## Convention #8 firing tally

The cumulative count for Phase 12 is now **10 firings**, all caught
before the phase closed:

| # | Fabrication | Where caught |
|---:|---|---|
| 1 | Krüger D3Q19 scope | Probe-2 |
| 2 | § 5 cross-cutting anchor strings | Checkpoint 5 |
| 3 | common-cpp API surface | Checkpoint 2 |
| 4 | Zou-He 3D inlet/outlet closure | Checkpoint 5 deep |
| 5 | `writeVec3Frame` API name | Checkpoint 5 |
| 6 | `docs/phase11_sph_water.md` location | Prep-3 landing |
| 7 | § 5.A expected grep count of 4 | Patch-apply verification |
| 8 | § 5.G.2 sph-water heading anchor | Patch-apply verification |
| 9 | `cat3.d3q19-*` integrity check IDs | § 6.C verification |
| 10 | Streamline seed slab + `dt_render` units | **Checkpoint 6 visual** |

Banked for Phase 12 retro: spec-vs-execution drift in
`seed_streamlines()` (spec said inlet slab, code wrote `(0,0,0)`)
plus units-mismatch in `dt_render` (spec § 4.B.12 had host-seconds
vs GPU-lattice-step inconsistency). Both surfaced only at the
visual gate. The 6-checkpoint protocol's mechanical verification
strength is unchanged but visual verification (Checkpoint 6)
remains the only line of defense for "code-vs-design-intent" drift
in renderer-only code paths.

## Build verification

```
$ cmake --build build-release --target lattice_boltzmann --parallel
[2/2] Linking CXX executable bin/lattice_boltzmann
```

Clean rebuild. Only the pre-existing `VkSamplerCreateInfo`
`-Wmissing-field-initializers` warnings (also emitted by
eulerian-smoke); no new warnings from these changes.
