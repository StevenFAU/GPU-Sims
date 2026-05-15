# Phase 11.5 commit 4 landing — boundary sampling fix (dedup + inset)

**Date:** 2026-05-15
**Author:** sph-water operator (Phase 11.5)
**Status:** Landed (commit 40f73e9 on main, with revert f23fd22 for an unintended runner.py drift)

---

## A. Change summary

Commit 3 + hotfix (b648894) landed Akinci2012 boundary handling that ran without crashing but produced **stratified-sheet fluid behavior** instead of correct fluid dynamics. The diagnostic session traced the cause to two geometric defects in `generateBoundaryParticles` — both fixed here:

1. **AABB seam coincident samples.** Each AABB edge had 2× and each corner 3× coincident boundary particles because each of the six face passes sampled its full rectangle without trimming the shared rim. Fluid particles near edges/corners received 2–3× boundary density contribution, producing asymmetric over-pressure and the standing-wave / quantized-layer pattern observed in the visual smoke.

2. **Boundary samples at the AABB plane.** With the AABB hard-clamp reduced to a catastrophic-escape fail-safe (commit 3 design), fluid particle centers could approach within `~particleRadius` of the boundary plane. Boundary samples sitting *on* that plane meant `q → 0` and per-neighbor density boost of ~`ρ_0 * V_b * W(0) ≈ 398 kg/m³`, catastrophically over-pressuring particles near the wall.

**Fix:** rewrite `generateBoundaryParticles` to (a) inset all samples by one `particleRadius` into the rigid body, mirroring the upstream Akinci2012 convention where the "boundary surface" is a particle layer just inside the wall, and (b) trim each face's hex grid by one `spacing` on every rim, with separate 12-edge and 8-corner passes claiming the seams exactly once.

The diagnostic verified V_b values were correct (mean within 14% of analytical, the discrepancy explained by exactly the edge/corner duplicates this commit fixes) and all five fluid-shader Akinci branches were sign-correct against upstream. The algebra was right; the geometry was wrong.

---

## B. File inventory

### Modified files (commit `40f73e9`)

| File | + / − |
|---|---|
| `particle-fluids/sph-water/src/main.cpp` | 115 / 25 |

(The substantive part is the rewrite of `generateBoundaryParticles` — about 100 lines, replacing a 50-line implementation with a 100-line one that includes the edge and corner passes plus a longer docblock explaining both decisions.)

### Followup revert (commit `f23fd22`)

| File | + / − |
|---|---|
| `tools/integrity/integrity/runner.py` | 0 / 25 |

This is an unintended-drift revert. During the commit-4 staging step I had run `git stash push -- tools/integrity/` followed by `git checkout stash@{0} -- tools/integrity/` to restore the parallel chat's WIP after running an integrity check. The latter command staged the WIP files in the index; the subsequent `git add particle-fluids/sph-water/src/main.cpp` left them staged. The commit therefore included 25 lines of `runner.py` belonging to the parallel chat's v1.1 batch (new `--grandfather-report` and `--state-snapshot` CLI flags + an import from a not-yet-committed `integrity.snapshot` module). The follow-up commit restores `runner.py` to its pre-commit-4 state. No functional impact: the new flags weren't usable from the tree as committed because `integrity/snapshot.py` was not staged either, so any attempt to use them would have hit an `ImportError` immediately.

This is a coordination defect (cf. commit 3 audit Section F.6), not a sph-water defect. Lesson recorded for future sessions: when a stash/pop dance leaves files staged in the index, use `git reset HEAD <path>` to unstage before `git add`-ing the intended path.

---

## C. Verification

### Build

```
cmake --build build-debug --target sph_water --parallel
[2/2] Linking CXX executable bin/sph_water
```

Clean build. Same set of pre-existing warnings as commit 3 (ImGui old-style-cast, unused stub symbols). No errors.

### Runtime launch

```
timeout 4 ./build-debug/bin/sph_water
EXIT=124   # SIGTERM from timeout, not a crash
```

Boundary log line captured during the run:

```
[sph-water] Akinci2012: 127864 boundary particles, ~2.44 MB
```

Binary launched and stayed running for the full 4-second window. No segfault, no Vulkan validation chatter in the log preamble. (Validation layers remain disabled in RelWithDebInfo builds per the commit-3 hotfix audit's incidental note.)

### Integrity toolkit

Local integrity status (run with the parallel chat's WIP integrity changes stashed, to isolate sph-water-attributable findings):

```
integrity: 2 pass, 0 soft-warn, 26 hard-fail, 944 suppressed
sph-water unsuppressed: 1
  particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl:7 cat1.intra-repo
```

- **0 new sph-water findings introduced by commit 4.** The 1 sph-water unsuppressed finding pre-dates commit 4 (it's a multi-line citation in the commit-3 `compute_boundary_volume.comp.glsl` docblock; the upstream regex requires the version and path on a single line). Out of scope for commit 4 per the prompt's "no shader changes" guardrail.
- The remaining 25 unsuppressed findings live in `tools/integrity/` and `docs/` and are caused by the parallel chat's in-flight v1.1 batch grandfather-logic shifts — same situation as commit 3's audit Section F.6.

### CI

Pushed to `origin/main`:
- `40f73e9 fix(sph-water): dedup AABB seams + inset boundary samples (commit 4)` — substantive fix
- `f23fd22 revert(integrity): unintended runner.py changes from commit 4` — coordination revert

CI status TBD at audit-write time; pushed after the local launch and integrity gates were green for sph-water.

---

## D. Behavioral expectations

When the binary is launched against the default Dam-Break preset (1M tier) after this commit, the user should observe:

- **Boundary preprocessing log line at preset load:**
  `[sph-water] Akinci2012: 127864 boundary particles, ~2.44 MB`
  (down from `131456 boundary particles, ~2.51 MB` in commit 3; the drop is dedup of ~3.6k seam particles.)

- **Column collapse without horizontal stratification.** The previously-observed quantized horizontal sheets should dissipate. Fluid in the initial brick should fall under gravity, hit the floor, and spread laterally into a pool — the canonical Dam-Break behavior.

- **No q→0 catastrophic boundary pressure spikes.** With samples inset by `particleRadius`, a fluid particle that approaches the AABB plane is at `q ≈ 0.5` from the nearest boundary sample (kernel value `W(q=0.5) ≈ 9,947`, ~4× smaller than `W(0) ≈ 39,789`), so the boundary repulsion is firm but not explosive.

- **No fluid passing through the AABB.** Fluid pressure plus the residual fail-safe clamp (still triggers at >0.5 × particleRadius outside the AABB) keep particles inside the box.

If the user still observes stratification or other quantized-layer artifacts after this commit, the next thing to check is whether the cohesion / viscosity coefficients are tuned for boundary-handled DFSPH (different from the pre-commit-3 hard-clamp regime where the boundary pressure was implicit). That would be a tuning task, not an algorithmic one.

---

## E. Preview of commit 5

Architect-1 will decide based on the user's visual smoke results. Plausible next commits (in order of likely priority):

- **Warmstart (`USE_WARMSTART` / `USE_WARMSTART_V`)** — DFSPH inner-iteration warmstart from the previous frame's converged pressure values. Mirrors upstream `warmstartPressureSolve` / `warmstartDivergenceSolve`. Expected effect: ~30–50% reduction in iterations-to-convergence, smoother behavior at high-velocity regions like the initial column collapse.

- **Convergence early-out** — sparse `avg_density_err` readback every K frames against the `eta` threshold; lets `maxIterDensity` and `maxIterDiv` actually gate the inner loop length instead of the current fixed `minIter*` count.

- **Orphan shader cleanup** — `density_solve.comp.glsl`, `divergence_solve.comp.glsl`, `pressure_apply.comp.glsl` are now unused. Removing them and their pipeline objects / descriptor sets / orphaned 1.8.10 citations clears the remaining `cat1.upstream-citation` grandfather suppressions for `live-shader-1810`.

If the user's smoke shows commit 4 still has fluid behavior issues, the next commit will be another diagnostic + fix cycle rather than warmstart. The Akinci2012 implementation is intricate enough that a second tuning pass would not be surprising.

---

## F. Incidentals

### F.1 — Boundary particle count, before vs after

| Preset | Commit 3 | Commit 4 | Δ |
|---|---|---|---|
| Dam-Break (4×3×2 m, r=0.01, 1M tier) | 131,456 | 127,864 | −3,592 (−2.7%) |

The drop is roughly:
- Face-rim trimming: ~−4.5k samples (each face loses one row + column at each rim)
- Plus 12-edge pass adds: ~+1.8k (single-line samples along each edge)
- Plus 8-corner pass adds: +8
- Net: −2.7k–3.6k depending on hex-row offset alignment

Anything in the [1%–3%] range matches the prompt's estimate of "1.5–2k duplicates" from the diagnostic.

### F.2 — Memory cost delta

The boundary buffers are allocated at `BOUNDARY_PARTICLE_CAP = 500k` regardless of live count (so the spatial-hash bindings stay constant across presets). **Allocated memory is unchanged.** Live memory:

| | Commit 3 | Commit 4 |
|---|---|---|
| Live boundary particles + volumes | 131,456 × (16+4) = 2.51 MB | 127,864 × (16+4) = 2.44 MB |
| Live boundary cell grid (morton + sorted_index + cell_starts) | ~3.5 MB (driven by MAX_CELLS for cell_starts) | ~3.5 MB (unchanged) |
| Total live | ~6.0 MB | ~5.9 MB |

Reduction is in the noise; the boundary count drop is the load-bearing change, not memory.

### F.3 — Coordination defect

The commit-4 substantive commit (`40f73e9`) inadvertently included a 25-line change to `tools/integrity/integrity/runner.py` from the parallel chat's WIP. Follow-up commit `f23fd22` reverts that portion. Root cause: running `git checkout stash@{0} -- tools/integrity/` to restore parallel-chat WIP after an isolated-integrity-check pass leaves the restored files staged in the index. A subsequent `git add <intended-path>` does not unstage the inadvertently-staged WIP. Future fix: either use `git stash pop` directly (preserves working-tree-only modifications without staging) or follow each `git checkout stash@{0} -- <path>` with `git reset HEAD <path>` to unstage before adding the intended changes.

### F.4 — Pre-existing finding flagged

`particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl:7` has a `cat1.intra-repo` unsuppressed finding for the cross-line citation:

```
// Boundary-only neighbor scan; mirrors the upstream pattern at SPlisHSPlasH
// 2.16.1 SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75 (computeBoundaryVolume).
```

The `<Upstream> <version> <path>` regex requires single-line matching; the path on line 7 is parsed as an intra-repo citation that doesn't resolve under any nearby directory. This was introduced in commit 3 and surfaced once the parallel chat's stricter v1.1 grandfather logic landed. **Out of scope for commit 4** but worth queuing for the next sph-water touch (one-line fix: join lines 6-7 into a single docblock line for the citation).
