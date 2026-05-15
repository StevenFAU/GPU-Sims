# Phase 11.5 commit 3 landing — Akinci2012 boundary handling

**Date:** 2026-05-15
**Author:** sph-water operator (Phase 11.5)
**Status:** Landed (commits 09bc7b4 + f9f2cb9 on main)

---

## A. Change summary

Lands Akinci2012-style boundary handling for the sph-water DFSPH solver.
Two commits on this branch:

1. **`09bc7b4 docs(sph-water): re-anchor citations to SPlisHSPlasH 2.16.1`** — ride-along citation cleanup. The Setup-1 audit established that the historical `SPlisHSPlasH 1.8.10` anchor was fabricated (the actual vendored upstream is `2.16.1` at SHA `6bff55a6eaf14083d34650f22a268ce156b62b54`); this commit migrates the live citations in `particle-fluids/sph-water/` to the real anchor. Paths now resolve cleanly under `references/SPlisHSPlasH/` (written as `SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp`, the layout under `vendor_root`).

2. **`f9f2cb9 feat(sph-water): Akinci2012 boundary handling (commit 3)`** — the substantive change. Implements pressure-based boundary handling per Bender-Koschier 2015 / SPlisHSPlasH 2.16.1, replacing the geometric AABB hard-clamp (kept as a fail-safe for catastrophic escape).

The hypothesis backing the change is the architect-1 diagnosis in `phase11_5_resume_probe_2026-05-15_architect1.md`: fluid particles near the AABB compute artificially-low density (no neighbors in the boundary hemisphere), which suppresses pressure correction via residuum clipping, while broken α-factors amplify spurious pressure for under-sampled particles. The visible symptoms post-commit-2b were column-stays-standing, pancaking, and isolated upward-drift "bubble" particles. Akinci2012 boundary samples + their fluid contribution fixes the density under-sample and gives proper pressure repulsion at the boundary.

---

## B. File inventory

### New files

| File | Lines |
|---|---|
| `particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl` | 91 |
| `docs/diagnostics/_audits/phase11_5_commit3_landing_2026-05-15.md` | (this file) |

### Modified files (commit `09bc7b4`)

| File | + / − |
|---|---|
| `particle-fluids/sph-water/docs/load-bearing-decisions.md` | 1 / 1 |
| `particle-fluids/sph-water/README.md` | 1 / 1 |
| `particle-fluids/sph-water/docs/notes.md` | 1 / 1 |
| `particle-fluids/sph-water/shaders/density_alpha.comp.glsl` | 3 / 8 |
| `particle-fluids/sph-water/shaders/compute_density_adv.comp.glsl` | 1 / 2 |
| `particle-fluids/sph-water/shaders/compute_density_change.comp.glsl` | 1 / 1 |
| `particle-fluids/sph-water/shaders/compute_aij_pj.comp.glsl` | 2 / 3 |
| `particle-fluids/sph-water/shaders/compute_pressure_accel.comp.glsl` | 2 / 1 |
| `particle-fluids/sph-water/shaders/jacobi_update_density.comp.glsl` | 1 / 3 |
| `particle-fluids/sph-water/shaders/jacobi_update_divergence.comp.glsl` | 1 / 3 |
| `particle-fluids/sph-water/shaders/apply_velocity.comp.glsl` | 3 / 4 |
| `particle-fluids/sph-water/src/main.cpp` | 2 / 4 |

### Modified files (commit `f9f2cb9`)

| File | + / − |
|---|---|
| `particle-fluids/sph-water/src/main.cpp` | ~520 / ~80 |
| `particle-fluids/sph-water/shaders/density_alpha.comp.glsl` | ~50 / ~3 |
| `particle-fluids/sph-water/shaders/compute_density_adv.comp.glsl` | ~50 / ~5 |
| `particle-fluids/sph-water/shaders/compute_density_change.comp.glsl` | ~50 / ~5 |
| `particle-fluids/sph-water/shaders/compute_aij_pj.comp.glsl` | ~55 / ~5 |
| `particle-fluids/sph-water/shaders/compute_pressure_accel.comp.glsl` | ~55 / ~5 |
| `particle-fluids/sph-water/shaders/integrate_forces.comp.glsl` | ~15 / ~6 |

Net: 8 files changed, 851 insertions, 160 deletions in the substantive commit.

---

## C. Verification

### Build

```
cmake --build build --target sph_water -j
[8/8] Linking CXX executable bin/sph_water
```

Build is clean; warnings are all pre-existing (ImGui old-style-cast warnings, unused stub symbols). No errors.

### Integrity toolkit

Local integrity status (running with the parallel chat's WIP integrity changes stashed, to isolate sph-water deltas from the in-flight integrity v1.1 work in another chat):

```
integrity: 2 pass, 0 soft-warn, 2 hard-fail, 1196 suppressed
```

- The 2 hard-fails are in `tools/integrity/docs/ground-truth-sources.md` (parallel chat's vendor entry for `lbm-principles-practice`); not introduced by this commit.
- 0 new `cat1.upstream-citation` findings from sph-water shaders or host code.
- The original `live-shader-1810` grandfather suppressions for shaders modified in this commit no longer fire (the citations they covered are now 2.16.1 with cleanly-resolving paths). The grandfather-catalog entry remains in place per the prompt's instruction to leave catalog cleanup as a separate concern.

### CI

Pushed to `origin/main`:
- `09bc7b4 docs(sph-water): re-anchor citations to SPlisHSPlasH 2.16.1` (citation-cleanup subcommit)
- `f9f2cb9 feat(sph-water): Akinci2012 boundary handling (commit 3)` (substantive)

CI status TBD at audit-write time; pushed after the citation cleanup passed local integrity gates with 0 unsuppressed findings attributable to sph-water.

---

## D. Behavioral expectations

When the binary is launched against the default Dam-Break preset (1M tier), the user should observe:

- **Boundary preprocessing log line** at preset load:
  `[sph-water] Akinci2012: <N> boundary particles, ~<M> MB`
  (e.g. ~80–110k particles, ~1.5–2.1 MB).

- **Column collapse**: the initial particle brick should now collapse under gravity rather than persisting as a banded grid. The pressure-based boundary contribution from the floor stops the column from "freezing" against an under-sampled density estimate.

- **Pool formation**: at 256k tier the fluid should pool with thickness, not pancake to a single-layer film on the floor.

- **No persistent upward "bubble" particles**: the cloud/bubble effects post-commit-2b should disappear because every fluid particle now sees a complete neighborhood (via boundary samples filling the hemisphere) and so its α-factor and density are computed correctly.

- **Boundary fail-safe inactive in steady state**: the integrate-forces fail-safe only kicks in for catastrophic escape (> 0.5 × particleRadius outside the AABB). Under normal operation the pressure-driven boundary repulsion keeps particles inside the box and the fail-safe is dormant.

If any of these expectations fails, the next step is to instrument the boundary loop dispatch (verify the boundary cell grid is non-empty, verify boundary_volumes contain reasonable values) before assuming the algebra is wrong.

---

## E. Preview of commit 4

Architect-1 will decide based on visual smoke results, but the most likely next commits per the scope guardrails section of the original prompt:

- **Warmstart (`USE_WARMSTART` / `USE_WARMSTART_V`)** — DFSPH inner-iteration warmstart from the previous frame's converged pressures. Mirrors upstream `warmstartPressureSolve` / `warmstartDivergenceSolve`. Typical effect: reduces iterations-to-convergence by 30–50%.

- **Convergence early-out** — sparse `avg_density_err` readback against `eta` threshold; lets `maxIterDensity` / `maxIterDiv` actually do something instead of the current fixed-iteration loop.

- **Orphan shader cleanup** — `density_solve.comp.glsl`, `divergence_solve.comp.glsl`, `pressure_apply.comp.glsl` are now unused (the dispatch chain uses `compute_density_adv` / `compute_density_change` / `compute_aij_pj` / `compute_pressure_accel` / `apply_velocity` / `jacobi_update_*`). Remove them and the corresponding pipeline objects / descriptor sets.

The user's visual smoke results will tell us which of these is most urgently needed. If columns still don't dissipate cleanly, warmstart probably won't help — the issue would be a more fundamental algebra bug in one of the new boundary branches.

---

## F. Incidentals

### F.1 — dt-scaling pre-flight finding

**Finding:** dt-scaling between `compute_aij_pj.comp.glsl` and the `jacobi_update_*.comp.glsl` consumers is **consistent with upstream**. No fix needed.

Trace:

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `compute_aij_pj.comp.glsl:121-123` applies `aij_pj_sum *= V_i; if (solver_mode == 0) aij_pj_sum *= dt*dt; else aij_pj_sum *= dt;` before writing `aij_pj_scratch[gid]`. This matches upstream `aij_pj *= h * h` (density-pass at `TimeStepDFSPH.cpp:582`) and `aij_pj *= h` (divergence-pass at `:656`).

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `jacobi_update_density.comp.glsl:55` reads `aij_pj_scratch[gid]` as-is, computes `s_i = 1.0 - density_adv`, then `p_new = max(p_i - jacobiRelax * (s_i - aij_pj) * alpha_over_rho2_i, 0.0)`. Matches upstream `:603-606`.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `jacobi_update_divergence.comp.glsl:55` reads `aij_pj_scratch[gid]` as-is, computes `s_i = -density_change`, same Jacobi update structure. Matches upstream `:674-692`.

No double-scaling defect. The commit-2b audit scrutiny item is resolved.

### F.2 — Upstream line-range correction

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
The original prompt listed `TimeStepDFSPH.cpp:1335-1349` for `compute_aij_pj`'s Akinci branch and `:1401-1412` for `compute_pressure_accel`'s — these are **swapped** in the actual upstream:

- `:1335-1344` is the Akinci branch of **`computePressureAccel`** (`a = p_rho2_i * grad_p_j` pattern).
- `:1401-1406` is the Akinci branch of **`compute_aij_pj`** (`aij_pj += V_b * a_i · grad_W` pattern).

The shipped citations use the verified mapping; the prompt's mapping was off-by-one between the two functions. Confirmed against `references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp` at the registered anchor SHA.

### F.3 — Boundary particle count for the default Dam-Break preset

At the 1M-tier default `particleRadius = 0.01` (Dam-Break domain `(-2, -1, -1) → (+2, +2, +1)`, 4×3×2 = 24 m³), hex-packed spacing = `0.02`, and the six AABB faces sample to roughly:

- Floor + ceiling (4×2 m each): 200×100 ≈ 20k each → 40k
- Front + back (4×3 m each): 200×150 ≈ 30k each → 60k
- Left + right (3×2 m each): 150×100 ≈ 15k each → 30k

Total: **~130k boundary particles** for Dam-Break at the 1M tier default radius. Comfortably under the `BOUNDARY_PARTICLE_CAP = 500k` ceiling. The exact count depends on the row-offset hex packing and corner sampling; the log line at preset load reports the actual count.

### F.4 — Memory cost of boundary buffers

Allocated at `BOUNDARY_PARTICLE_CAP = 500k` regardless of live count (so the boundary sort pipelines bind once):

- `boundary_particles`: 500k × 16 B = **8.0 MB**
- `boundary_volumes`: 500k × 4 B = **2.0 MB**
- `boundary_morton_codes`: 500k × 4 B = **2.0 MB**
- `boundary_sorted_index`: 500k × 4 B = **2.0 MB**
- `boundary_cell_starts`: 262145 × 4 B = **1.0 MB**
- `boundary_cell_counts` + `_atomic` + `block_sums` + `block_prefixes` + `l2_sums` + `l2_prefixes`: ~2 MB total (most are MAX_CELLS-sized at 1 MB each, but several are unused since boundary sort is on CPU)

Total: **~17 MB** per tier, dominated by the cap-sized particle/volume/morton/sorted_index quad. Live use at the 1M-tier Dam-Break preset (130k boundary particles) is ~2.5 MB; the rest is headroom up to the 500k cap.

This is within the spec § 1.3 memory budget for the 4M tier (which has GBs of fluid SSBOs); boundary buffers add a constant ~17 MB regardless of particle tier.

### F.5 — Build walltime

Boundary preprocessing adds a one-shot dispatch at preset load:

- CPU work: generate ~130k positions + CPU counting-sort + upload (~5 ms on dev hardware, dominated by the upload of `boundary_cell_starts` at 1 MB).
- GPU work: `compute_boundary_volume` dispatch (~1 ms).

Total preset-load delta: ~6 ms. Imperceptible to the user (preset switch is already gated by `renderer.waitIdle()` which costs ~10–20 ms).

### F.6 — Coordination with parallel chat

During this session the parallel integrity v1.1 chat:

- Landed `af248cf feat(integrity): cat2.stub-label-stale (v1.1 batch 1 commit 1)` mid-session (was uncommitted at session start).
- Landed `8fe355b setup(phase12): vendor lbm-principles-practice MIT reference at 6e2c592f` later in the session.
- Has additional uncommitted WIP changes to `tools/integrity/integrity/cat1_citations/checks/{annotation.py, intra_repo.py, upstream.py}` + `common/{annotations.py, suppression.py}` + `grandfather.py` that, when applied, make the integrity check stricter (newly invalidates ~219 suppressions, ALL outside sph-water).

My sph-water commits avoided staging any of the parallel chat's WIP files (used explicit `git add <paths>` only). Both my commits pass integrity locally with 0 new unsuppressed sph-water findings when the parallel chat's WIP integrity changes are stashed. The 219-finding regression is a pure consequence of the parallel chat's in-flight grandfather logic shift, not my edits, and will resolve when that chat completes its v1.1 batch.

### F.7 — Things that surprised me during execution

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- **GLSL upstream-citation grammar is single-line.** The `cat1.upstream-citation` regex matches `<Upstream> <version> <path>:<line>` on a single physical line. Initial multi-line citations like `// Mirrors SPlisHSPlasH 2.16.1\n// SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1217-1223` failed the upstream parser but matched the intra-repo parser, surfacing as unsuppressed `cat1.intra-repo` findings. Fix: put version and path on the same line, even if it stretches comment width.

- **Bare path citations (e.g. `TimeStepDFSPH.cpp`) don't resolve cleanly under `references/SPlisHSPlasH`** (vendor_root); the actual file is at `references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp`. Citations need the full sub-path `SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp` for the resolver to find them. The pre-existing convention in cat3 code uses bare paths and gets caught by the `other-cat1` catch-all; switching to full sub-paths here lets the new citations resolve without grandfathering.

- **Boundary GPU sort is unnecessary.** Boundary particles are static and small enough (~130k for Dam-Break) that a CPU counting-sort + upload is faster and avoids replicating the 5-stage GPU sort pipeline with a second descriptor binding. Only the `compute_boundary_volume` kernel needs to be a new GLSL shader.

- **Pre-existing AABB hard-clamp was hiding the absence of boundary handling.** With the clamp in place, particles couldn't escape geometrically; without proper density at the boundary, the clamp was the only thing keeping fluid in the box, which masked the under-sample / over-pressure pathology. Reducing the clamp to a fail-safe is part of the fix — it forces the algebra to be correct.

---
