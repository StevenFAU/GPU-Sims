# sph-water — Load-bearing decisions

This document is a sim-local quick reference. For the full reasoning, see
`docs/phase11_sph_water.md` § 2.

## DFSPH (not WCSPH, not PCISPH, not IISPH, not PBF)

Divergence-Free SPH per Bender-Koschier 2015 + 2017. Anchored to SPlisHSPlasH
2.16.1 at SHA `6bff55a6eaf14083d34650f22a268ce156b62b54` for every formula
citation. Five non-obvious upstream conventions encoded:

1. Support-radius parameterization (q = r/h, cutoff q ≤ 1).
2. h = 4 × particle radius (some codebases use 2× or 3×; we pin to 4×).
3. Stored α is α/ρ² (not raw α; multiphase-compatible form).
4. Jacobi relaxation 0.5 (undocumented in the 2017 paper; only in source).
5. `maxError` is percent: 0.01 means 0.01% of ρ₀ = 1e-4 fractional.

## Morton-sorted spatial hash

Counting sort by Morton-encoded cell index, 6 compute kernels translated from
boids-3d's Stack B precedent (plus a 7th `prefix_sum_block_l2.comp.glsl` for
the recursive second-level scan when `num_blocks > WG_SIZE`). Z-order curve
gives better cache locality on 27-cell neighbor iteration vs row-major
linearization. Cell grid is rounded up to next power of 2 per axis (so Morton
encoding densely fills the cell range). At 4M-tier worst case the cell count
is bounded by `WG_SIZE^3 = 16M` cells (~254 per axis); larger requires a
three-level scan banked for v1.2+.

## Vulkan subgroup-size pinning

`VK_EXT_subgroup_size_control` (Vulkan 1.3 core) —
`VkPipelineShaderStageRequiredSubgroupSizeCreateInfo` pinned to
`max(32, deviceMinSubgroupSize)` at pipeline creation. Eliminates the
AMD-wavefront-64 vs NVIDIA-wavefront-32 vs Adreno-wavefront-various divergence
at the architectural level — no per-vendor branches in any GLSL.

Applied to: 7 sort kernels + 1 bilateral kernel. NOT applied to the 5 DFSPH
kernels (per-particle parallel, no subgroup arithmetic, no benefit from
pinning).

## Screen-space fluid render (Müller-Fetterer 2007)

Five-pass pipeline: point-sprite depth → bilateral smooth (N iterations,
default N=4 H+V) → additive thickness → composite with Fresnel + Beer-Lambert
+ procedural sky. New for Stack C — Phase 8 eulerian-smoke's raymarcher was
single-pass color-only; this is the first multi-pass off-screen render-pass
construction via direct `vkCmdBeginRenderingKHR`.

Fresnel via Schlick's approximation, F0 = 0.02 for water. Normal reconstruction
via `cross(ddy(P_view), ddx(P_view))` per the paper sec 3.1.

## Alembic packaging via CMake FetchContent

Alembic 1.8.10 pinned at SHA `c254caf2705ebf5271408dd37a091aa379258a38`. NOT
1.8.11 — that version raises `cmake_minimum_required` to 3.29 and Ubuntu 24.04
noble ships CMake 3.28.3. Vendor via FetchContent because `libalembic-dev` was
dropped from noble after jammy. Apt deps: `libimath-dev` only.

Build cost: ~11 s clean on dev hardware (1.89 s configure + 9.48 s build,
107 ninja steps); ~30–60 s estimated on CI; `actions/cache@v4` keyed on
Alembic SHA caches it for ~10 s warm hits.

## Reserve-tail emitter allocation

Per-tier particle pool split into `[0, n_preset)` for the preset's initial
distribution and `[n_preset, n_preset + EMITTER_RESERVE)` for LMB-painted
emitter particles. Particle count is monotonic during a session; preset
reload zeroes the emitter tail. Pattern transferred from Phase 9 MPM polish-5.

## Why fixed inner-iteration count (not convergence-checked)?

SPlisHSPlasH iterates each inner pressure-solver loop until a CPU-readback
residual falls below a threshold. For a real-time GPU sim at 60+ FPS over
1M+ particles, per-frame CPU readback stalls the pipeline (the readback
requires a sync barrier defeating async dispatch).

v1 uses fixed iteration counts: `minIterDivergence = 1`, `minIterDensity = 2`.
Panel exposes the `maxIter*` sliders too but they're not consulted in v1
(banked v1.1 with sparse residual readback every K frames that doesn't stall
the main pipeline).
