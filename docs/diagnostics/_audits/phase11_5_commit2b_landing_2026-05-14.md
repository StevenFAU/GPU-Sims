# Phase 11.5 — Commit 2b landing audit (2026-05-14)

## A. Change summary

Commit 2b lands the DFSPH dispatch-chain restructure and the two missing
Jacobi-update shaders. With this change, every kernel added in commit 2a
(`compute_density_change`, `compute_density_adv`, `compute_pressure_accel`,
`compute_aij_pj`, `apply_velocity`) is reachable from the substep loop, and
two new shaders (`jacobi_update_density`, `jacobi_update_divergence`)
perform the relaxed-Jacobi pressure update from the upstream-exact aij_pj
sum produced by `compute_aij_pj`. The legacy `pipe_density_solve`,
`pipe_divergence_solve`, and `pipe_pressure_apply` dispatches are removed
from the substep loop; their host-side objects and shader files remain on
disk and are unreachable (cleanup deferred to a later commit). The new
substep chain matches `TimeStepDFSPH::step()` per probe-3 Section A:
source-term once per outer solve, then per-iter
`compute_pressure_accel → compute_aij_pj → jacobi_update`, followed by a
final `compute_pressure_accel → apply_velocity` to integrate the converged
pressure acceleration onto particle velocity.

## B. New shader inventory

| Shader | Lines | Bindings |
|---|---|---|
| `shaders/jacobi_update_density.comp.glsl` | 60 | 0:DensityAlpha (RO SSBO), 1:AijPjScratch (RO SSBO), 2:PressureRead (RO SSBO), 3:PressureWrite (WO SSBO), 4:U (UBO, canonical 112-byte DFSPH) |
| `shaders/jacobi_update_divergence.comp.glsl` | 60 | identical to jacobi_update_density |

Both shaders use `#version 460`, `layout(local_size_x = 256) in;`, and the
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
112-byte DFSPH UBO transcribed verbatim from `density_alpha.comp.glsl:25-53`.
They do not include the kernel-helper functions (no neighbour traversal —
purely per-particle Jacobi update reading `da[]`, `aij_pj_scratch[]`,
`p_read[]` and writing `p_write[]`).

Source-term difference:

- **density**: `s_i = 1.0 - da[gid].z` (density_adv); aij_pj_scratch is the
  `solver_mode==0` output (scaled by dt*dt).
- **divergence**: `s_i = -da[gid].w` (density_change); aij_pj_scratch is the
  `solver_mode==1` output (scaled by dt).

Both apply the upstream Jacobi update with relaxation `jacobiRelax`
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
(uniform; default 0.5 per `TimeStepDFSPH.cpp:606`):
`p_new = max(p_i - jacobiRelax * (s_i - aij_pj) * alpha_over_rho2, 0)`.

## C. Verification (quoted output)

### 1. `grep -nE "pipe_(density_solve|divergence_solve|pressure_apply)\.dispatch" particle-fluids/sph-water/src/main.cpp`

```
(no matches)
```

Zero hits — the old pipelines exist on the host but are never dispatched.

### 2. `grep -nE "pipe_(compute_density_change|compute_density_adv)\.dispatch" particle-fluids/sph-water/src/main.cpp`

```
2295:                pipe_compute_density_change.dispatch(cmd, ds_compute_density_change, wg_particle, 1, 1);
2352:                pipe_compute_density_adv.dispatch(cmd, ds_compute_density_adv, wg_particle, 1, 1);
```

One hit each — source-term kernels dispatched once before their respective
inner-iteration loops.

### 3. `grep -n "pipe_compute_pressure_accel.dispatch" particle-fluids/sph-water/src/main.cpp`

```
2311:                    pipe_compute_pressure_accel.dispatch(cmd, ds_compute_pressure_accel[i % 2], wg_particle, 1, 1);
2327:                pipe_compute_pressure_accel.dispatch(cmd, ds_compute_pressure_accel[final_i % 2], wg_particle, 1, 1);
2368:                    pipe_compute_pressure_accel.dispatch(cmd, ds_compute_pressure_accel[i % 2], wg_particle, 1, 1);
2383:                pipe_compute_pressure_accel.dispatch(cmd, ds_compute_pressure_accel[final_i % 2], wg_particle, 1, 1);
```

Four hits — two per inner loop (per-iter ping-pong + final converged
pressure_accel that drives apply_velocity).

### 4. `grep -n "pipe_compute_aij_pj.dispatch" particle-fluids/sph-water/src/main.cpp`

```
2316:                        pipe_compute_aij_pj.dispatch(cmd, ds_compute_aij_pj[0], wg_particle, 1, 1,
2373:                        pipe_compute_aij_pj.dispatch(cmd, ds_compute_aij_pj[0], wg_particle, 1, 1,
```

Two hits — one inside each inner loop, with `solver_mode` push constant
selecting dt vs dt*dt scaling.

### 5. `grep -nE "pipe_jacobi_update_(density|divergence)\.dispatch" particle-fluids/sph-water/src/main.cpp`

```
2321:                    pipe_jacobi_update_divergence.dispatch(cmd, ds_jacobi_update_divergence[i % 2], wg_particle, 1, 1);
2378:                    pipe_jacobi_update_density.dispatch(cmd, ds_jacobi_update_density[i % 2], wg_particle, 1, 1);
```

Two hits — one Jacobi-update kernel per inner loop.

### 6. `grep -n "pipe_apply_velocity.dispatch" particle-fluids/sph-water/src/main.cpp`

```
2329:                pipe_apply_velocity.dispatch(cmd, ds_apply_velocity, wg_particle, 1, 1);
2385:                pipe_apply_velocity.dispatch(cmd, ds_apply_velocity, wg_particle, 1, 1);
```

Two hits — one final `apply_velocity` integrating converged pressure
acceleration onto particle velocity at the end of each solve.

### 7. `ls particle-fluids/sph-water/shaders/jacobi_update_*.comp.glsl`

```
particle-fluids/sph-water/shaders/jacobi_update_density.comp.glsl
particle-fluids/sph-water/shaders/jacobi_update_divergence.comp.glsl
```

Both files present.

### 8. `git diff --stat`

```
 .gitignore                                         |   7 +
 common/common-cpp/include/gpusims/gpu_profiler.hpp |   2 +-
 common/common-cpp/src/gpu_profiler.cpp             |   2 +-
 common/common-cpp/src/vk/context.cpp               |   1 +
 particle-fluids/sph-water/CMakeLists.txt           |   9 +
 particle-fluids/sph-water/src/main.cpp             | 405 +++++++++++++++++++--
 6 files changed, 402 insertions(+), 24 deletions(-)
```

Within `particle-fluids/sph-water/`: `src/main.cpp` and `CMakeLists.txt`
modified — no other files in sph-water touched. The two new shaders show
under untracked files (commit 2a's five shaders also remain untracked here
because commit 2a's files have not yet been `git add`-ed; this is the state
inherited from the working tree, not a regression introduced by 2b).
Repo-root entries above sph-water (`.gitignore`, `common-cpp/...`) are
pre-existing modifications carried over from earlier setup work; commit 2b
did not edit them.

## D. Behavioral expectations

The visual result should change substantially relative to commit 2a. The
correct upstream DFSPH stencil — neighbour-traversal-with-mass-and-grad-W
inside `compute_aij_pj`, per probe-3 Section J — now actually drives the
solver. Previously (commit 2a and earlier) `compute_aij_pj` was wired but
unreachable; the Jacobi update consumed a placeholder stencil baked into
the old `density_solve.comp.glsl` / `divergence_solve.comp.glsl` shaders.

Concretely, expect:

- The horizontal-banding symptom should diminish or change character. It
  was caused by the placeholder stencil mis-weighting neighbour gradients;
  the new `compute_aij_pj` uses `m_j / rho_0 * grad_W(x_i - x_j)` per
  upstream lines 869-877, which is the form the rest of the DFSPH stencil
  was designed for.
- The two-pass `compute_pressure_accel → compute_aij_pj` structure means
  per-particle pressure-acceleration is now exposed as an explicit state
  buffer; `apply_velocity` reads this buffer once at the end of each solve
  rather than folding pressure-accel integration into the Jacobi inner pass
  (which the old `pressure_apply` did).
- The fluid may exhibit new artifacts that were previously masked by the
  placeholder. Any remaining issues are structurally honest — the math is
  the upstream-canonical form — and therefore diagnosable. Likely candidates
  for follow-up scrutiny: jacobi relaxation factor, boundary handling, dt
  scaling consistency between `compute_aij_pj`'s solver_mode push constant
  and the Jacobi update's `s_i` expression, and the early-out logic that
  upstream uses to terminate the inner loop on convergence (this port still
  runs a fixed `minIter` count).

The expected outcome is "looks different and structurally honest." Whether
it looks visibly *better* depends on parameter tuning that was previously
hiding behind a wrong stencil.

## E. Incidental findings

- Commit 2a's shader files are still untracked in git (only its host-side
  wiring lives in the modified `main.cpp` / `CMakeLists.txt`). When commit
  2b is staged, the user will want to `git add` all seven new shader files
  (five from commit 2a + two from commit 2b) plus the modified files so
  the working tree comes out clean.
- The old `pipe_density_solve`, `pipe_divergence_solve`, and
  `pipe_pressure_apply` pipeline objects, their descriptor-set allocations,
  their descriptor-write helpers (`writeDfsphSolveDescriptor`,
  `writePressureApplyDescriptor`), and the corresponding `reload_*` flags
  / `W_watch` / `try_reload` calls are all unreachable after 2b but remain
  on the host side. They compile and consume one descriptor-pool slot each.
  Cleanup belongs in a follow-up commit (call it 2c or roll into 4) where
  the three orphan shader files are deleted alongside their host objects.
- The `compute_aij_pj` descriptor sets are allocated as a `[2]` ping-pong
  pair but only `ds_compute_aij_pj[0]` is used in both inner loops; both
  set indices have identical descriptor writes (binding 2a:1475-1486 in the
  pre-2b state, now shifted). The `[1]` set is allocated but never
  dispatched against. Not a regression — same state as after commit 2a —
  but a candidate for trimming when the cleanup commit lands.
- The commit message and verification block here assume the diff-chain
  counts only the dispatch-chain restructure. The total `main.cpp` diff of
  ~405 lines includes commit 2a's untracked content because git's
  base-state for the file is the last committed revision (`fc1d63a`). Of
  those 405 lines, the commit-2b-specific contribution is roughly the new
  Jacobi descriptor helpers (~50 lines), the two new `make_compute` /
  descriptor-allocate / descriptor-write blocks (~30 lines), the
  hot-reload pair (~6 lines), and the substep rewrite (~50 net new lines)
  — the remainder is the carried-over commit 2a wiring.
