# Phase 11.5 commit 6 landing — H1 boundary placement + preset clearance

**Date:** 2026-05-15
**Author:** sph-water operator (Phase 11.5)
**Status:** Landed (commit `d84b9ad` on main)

---

## A. Change summary

The probe at `docs/diagnostics/_audits/phase11_5_boundary_placement_probe_2026-05-15.md` established that the inset adjustments in commits 4 and 5 were both on the wrong axis. Upstream Akinci2012 places boundary samples directly on the rigid body's mesh surface — no inset. What upstream actually maintains is a **2 × particleRadius clearance** between the fluid initial position and the boundary surface (`DamBreakModel.json` puts the fluid block 2 × r above the floor and 2 × r from the closest x-wall).

This commit restores upstream H1 geometry and fixes the preset clearance:

- **`generateBoundaryParticles`** reverts to commit-3 placement: `imin = dmin`, `imax = dmax`. Samples sit on the AABB plane itself with no offset. Commit-4's seam-deduplication discipline (face-rim trim + 12 edge passes + 8 corner passes) is preserved — it was a genuine defect independent of inset.

- **All four sph-water presets** are audited and adjusted so every initial fluid brick is inset by at least 0.02 (= 2 × particleRadius at the 1M-tier default) from every AABB face. Brick coordinates updated for three of four presets; one (Pour-from-Source) had no initial brick and required no change.

The source of the design decision is the probe report's Section E recommendation, cited directly in the docblock of `generateBoundaryParticles`.

---

## B. File inventory

| File | + / − |
|---|---|
| `particle-fluids/sph-water/src/main.cpp` | 44 / 27 |

Diff breakdown:
- `generateBoundaryParticles` docblock: ~20 lines rewritten (the commit 4 / 5 narrative is preserved for archeology; the new top-level state is H1).
- `generateBoundaryParticles` body: 2 lines changed (the `imin`/`imax` initializers).
- `SPH_PRESETS` block: 3 preset entries updated with new brick coordinates + clarifying comments; a header comment added above the array explaining the clearance invariant.

The face / edge / corner sampling loops, the `BOUNDARY_PARTICLE_CAP`, and the boundary buffer allocation block are unchanged.

---

## C. Verification

### Build

```
cmake --build build-debug --target sph_water --parallel
[2/2] Linking CXX executable bin/sph_water
```

Clean. Same set of pre-existing warnings as commits 3–5.

### 4-second runtime launch

```
timeout 4 ./build-debug/bin/sph_water
EXIT=124   # SIGTERM from timeout, not a crash
[sph-water] Akinci2012: 129658 boundary particles, ~2.47 MB
```

### 30-second runtime stability

```
timeout 32 ./build-debug/bin/sph_water
EXIT=124   # clean SIGTERM
```

7 startup log lines, then quiet for the full 30 s. No crash, no warning, no NaN-report path triggered, no Vulkan validation chatter. The commit-4-era 20-second crash did **not** reproduce, consistent with the probe's hypothesis that the crash was a downstream consequence of the wall-plastering numerical instability rather than a separate defect.

### Integrity toolkit

```
sph-water unsuppressed: 1
  particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl:7 cat1.intra-repo
```

Same pre-existing finding as commits 3–5 (multi-line citation in `compute_boundary_volume.comp.glsl`; queued for the next sph-water touch). **0 new sph-water findings from commit 6.**

### CI

Pushed to `origin/main`:
- `d84b9ad fix(sph-water): H1 boundary placement + preset clearance (commit 6)`

CI status TBD at audit-write time; pushed after the launch and integrity gates were green for sph-water.

---

## D. Behavioral expectations

When the user runs visual smoke against commit 6:

- **Dam-Break:** the fluid brick (now inset 0.02 from −y/+x/−z/+z) should fall under gravity and collapse into a pool on the floor. The expected canonical SPH dam-break behavior — splash on impact, lateral spread, eventual pooling. Wall-plastering and stratified sheets should both be gone.

- **Central-Fountain:** thin floor pool plus the cylinder emitter pushing fluid upward. Pool should sit on the floor; emitter stream should fountain upward and fall back.

- **Droplet-Impact:** thin floor pool plus a falling droplet that hits the pool and produces a crown splash.

- **Pour-from-Source:** empty domain that fills from a rectangular source on the top face. Fluid streams downward into the empty box and accumulates.

The user will run visual smoke separately and report. If any of these still produce wrong behavior, the next step is another targeted diagnostic (likely cohesion / viscosity tuning, since the boundary geometry is now upstream-correct).

---

## E. Preview of commit 7

Conditional on visual smoke verifying commit 6 produces correct fluid behavior. Plausible next commits in priority order:

- **Warmstart (`USE_WARMSTART` / `USE_WARMSTART_V`)** — DFSPH inner-iteration warmstart from the previous frame's converged pressure values. 30–50% reduction in iterations-to-convergence.

- **Convergence early-out** — sparse `avg_density_err` readback against `eta` threshold; lets `maxIter*` actually gate the inner loop instead of the fixed `minIter*` count.

- **Orphan shader cleanup** — `density_solve.comp.glsl`, `divergence_solve.comp.glsl`, `pressure_apply.comp.glsl` are now unused. Removing them also clears the remaining `live-shader-1810` grandfather suppressions.

If commit 6 still produces wrong behavior, the next commit will be diagnostic rather than feature work.

---

## F. Incidentals

### F.1 — Boundary particle count history

| Commit | Inset | Count | Memory |
|---|---|---|---|
| 3 | none (samples on AABB) | 131,456 | 2.51 MB |
| 4 | +r into AABB (wrong) | 127,864 | 2.44 MB |
| 5 | −r outside AABB (wrong direction) | 131,458 | 2.51 MB |
| **6** | **none (samples on AABB)** | **129,658** | **2.47 MB** |

Commit 6 lands on H1 placement like commit 3, but with commit-4-style dedup (face-rim trim + 12 edge passes + 8 corner passes). The 129,658 count is ~1.8k below commit 3's 131,456 — exactly the duplicate count commit 4 was meant to remove. So this commit is "commit 3 placement, commit 4 dedup" combined.

### F.2 — Preset brick coordinate changes

| Preset | Before (brick min → max) | After |
|---|---|---|
| Dam-Break | (+0.5, **−1.0**, **−1.0**) → (**+2.0**, +0.5, **+1.0**) | (+0.52, −0.98, −0.98) → (+1.98, +0.5, +0.98) |
| Central-Fountain | (**−1.0**, **−1.0**, **−1.0**) → (**+1.0**, −0.95, **+1.0**) | (−0.98, −0.98, −0.98) → (+0.98, −0.95, +0.98) |
| Droplet-Impact | (**−1.5**, **−1.0**, **−1.5**) → (**+1.5**, −0.97, **+1.5**) | (−1.48, −0.98, −1.48) → (+1.48, −0.97, +1.48) |
| Pour-from-Source | (0, 0, 0) → (0, 0, 0) | (unchanged — no initial brick) |

Bolded coordinates in the "Before" column are the ones that touched the AABB domain (zero clearance). All bricks are now inset 0.02 from every AABB face where they previously touched it. The shifts are small enough to preserve each preset's visual intent.

### F.3 — Long-run stability

The commit-4 visual smoke reportedly crashed at ~20 s with `X connection to :0 broken (explicit kill or server shutdown)`. The 65-s headless run against commit 5 (probe Section B) did not reproduce the crash, and the 30-s headless run against commit 6 likewise produces no crash. **Working hypothesis:** the crash was a downstream consequence of the wall-plastering numerical instability — over-pressured fluid eventually produced a NaN that propagated into either descriptor binding or the alembic-writer buffer. With H1 geometry + preset clearance, the wall-plastering doesn't happen, so the NaN-producing pathway should be quiescent.

If the user reproduces the 20-s crash in interactive visual smoke against commit 6, the next diagnostic step is `LD_PRELOAD=libVkLayer_khronos_validation.so` to surface Vulkan errors (validation layers are currently compiled out in `RelWithDebInfo`) plus a `gdb` attach on a long-running instance.

### F.4 — Pre-existing finding flagged (unchanged from commit 4 audit)

`particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl:7` still has the cross-line citation `cat1.intra-repo` finding. Out of scope for commit 6 (shader-docblock-only touch, queued for the next sph-water shader cleanup).

### F.5 — Coordination

Parallel chat continued landing v1.1 batch commits during this work (`dbac051 feat(integrity): A.7 snapshot module + seed history (v1.1 batch 1 commit 3a)` and `a71594a feat(integrity): A.7 CLI flags --state-snapshot / --grandfather-report (v1.1 batch 1 commit 3b)` between commits 5 and 6). The pattern from commit 4's audit Section F.3 was applied this time: `git stash push -- tools/integrity/` followed by `git checkout stash@{0} -- tools/integrity/` and `git reset HEAD tools/integrity/` before `git add particle-fluids/...`. No inadvertent-WIP-commit defect.
