# Phase 11.5 commit 5 landing — boundary inset direction flip

**Date:** 2026-05-15
**Author:** sph-water operator (Phase 11.5)
**Status:** Landed (commit `dde5f22` on main)

---

## A. Change summary

Commit 4 (`40f73e9`) inset boundary samples in the wrong direction. The intended Akinci2012 convention places boundary particles in the *wall material* — one particle-radius **outside** the AABB. Commit 4 instead placed them one radius **inside** the AABB, in the fluid's domain. Result: the boundary pressure gradient attracted fluid *toward* the walls rather than pushing it away. Visual smoke showed fluid plastered against floor, ceiling, and side walls in thin grids with the center of the AABB empty.

Commit 5 flips the inset sign on all six AABB faces:

| Face | Commit 4 (wrong) | Commit 5 (fixed) |
|---|---|---|
| Floor (y=ymin)   | y = ymin + r  | **y = ymin − r** |
| Ceiling (y=ymax) | y = ymax − r  | **y = ymax + r** |
| Left (x=xmin)    | x = xmin + r  | **x = xmin − r** |
| Right (x=xmax)   | x = xmax − r  | **x = xmax + r** |
| Front (z=zmin)   | z = zmin + r  | **z = zmin − r** |
| Back (z=zmax)    | z = zmax − r  | **z = zmax + r** |

Implementation is a one-character flip per axis: `imin = dmin - r; imax = dmax + r;` (previously `dmin + r; dmax - r`). The change cascades through the face, edge, and corner passes since each uses `imin`/`imax` for both the normal-axis plane coordinate and the in-plane axis bounds. Edge and corner deduplication from commit 4 is unchanged.

---

## B. File inventory

| File | + / − |
|---|---|
| `particle-fluids/sph-water/src/main.cpp` | 19 / 9 |

The 19/9 diff is concentrated in the `generateBoundaryParticles` docblock (5 lines added explaining the commit-5 fix and clarifying the sign convention) and the two-line `imin`/`imax` computation at the top of the function body. The face/edge/corner sampling logic is unchanged.

---

## C. Verification

### Build

```
cmake --build build-debug --target sph_water --parallel
[2/2] Linking CXX executable bin/sph_water
```

Clean. Same pre-existing warnings as commits 3/4 (ImGui old-style-cast, unused stub symbols).

### Runtime launch

```
timeout 4 ./build-debug/bin/sph_water
EXIT=124   # SIGTERM from timeout, not a crash
```

Boundary log line captured:

```
[sph-water] Akinci2012: 131458 boundary particles, ~2.51 MB
```

Binary launched and stayed running for the full 4-second window.

### Integrity toolkit

Run with the parallel chat's WIP integrity changes stashed (to isolate sph-water deltas):

```
sph-water unsuppressed: 1
  particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl:7 cat1.intra-repo
```

Same 1 pre-existing finding as commits 3 and 4 (multi-line citation in the commit-3 compute_boundary_volume docblock). **0 new sph-water findings from commit 5.**

---

## D. Behavioral expectations

Fluid should now collapse under gravity and pool at the floor as a Dam-Break should; the wall-plastering pattern from commit 4 should be gone.

---

## E. Preview of commit 6

Pending the user's visual smoke. If commit 5 produces correct fluid behavior, the natural next commit is **warmstart** (DFSPH inner-iteration warmstart from previous frame's converged pressure values; 30–50% iteration count reduction). If commit 5 still shows artifacts, another diagnostic + fix cycle.

---

## F. Incidentals

### F.1 — Boundary particle count delta

| Commit | Count | Memory |
|---|---|---|
| 3 | 131,456 | 2.51 MB |
| 4 | 127,864 | 2.44 MB |
| 5 | **131,458** | **2.51 MB** |

The count *increased* slightly from commit 4 because flipping the inset shifts the face-rectangle in-plane bounds by `+r` on each side. Specifically: `u_lo = imin[axis_u] + spacing = (dmin - r) + spacing = dmin + r`, vs commit 4's `u_lo = dmin + r + spacing = dmin + 3r`. Same logic for `u_hi`. Each face is wider by 2*spacing in each in-plane axis, picking up an extra row/column of samples per face. The dedup discipline (interior + 12 edges + 8 corners, no seam overlap) is unchanged; only the placement geometry shifts.

The +2 over commit 3's count is a hex-row alignment coincidence — at this preset's specific spacing and axis ratios, the new face-rectangle layout happens to pick up two more samples than the unmodified rectangle did.

### F.2 — 20-second runtime crash (TBD)

The visual smoke run on commit 4 reportedly crashed at ~20s of runtime (separate from the wall-plastering symptom). Cause not yet diagnosed. **Will assess after commit 5 lands and the user runs visual smoke against it.** If the crash reproduces post-commit-5, the next step will be a gdb session against a long-running instance and stack-trace categorization (likely candidates: a numerical NaN propagating into descriptor binding, an alembic-writer buffer overflow at a known frame count, or a Vulkan command-buffer reuse race).

### F.3 — Coordination caveat

Parallel chat continued landing v1.1 batch commits during this work (e.g. `c5955d3 setup(phase12): land architect-1 spec at docs/phase12_lattice_boltzmann.md` between commit 4 and commit 5). Their integrity WIP files were carefully isolated this session using `git stash push -- tools/integrity/` and `git reset HEAD tools/integrity/` before staging, avoiding the inadvertent-WIP-commit defect from commit 4. No coordination revert needed.
