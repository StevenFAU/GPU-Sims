---
title: Phase 11.5 resume probe — ground-truth state for commit 3+ drafting
date: 2026-05-15
audience: architect-1
role: resume-probe (read-only)
upstream: references/SPlisHSPlasH/ @ 6bff55a6eaf14083d34650f22a268ce156b62b54 (SPlisHSPlasH 2.16.1)
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
scope: sph-water source inventory + upstream-citation fact-gathering for warmstart / convergence / Akinci boundary / CFL; integrity toolkit visibility audit
status: read-only — no edits, no builds, no binary runs
---

# Phase 11.5 resume probe — commit-3 prep ground truth

Read-only fact-gathering for architect-1 before commit-3 of the Phase 11.5
DFSPH sph-water work. Phase 11.5 paused after commit 2b (dispatch-chain
rewrite, SHA `4adc84a`). The integrity toolkit was then built across
commits 4a–8 + retro and is now gating CI. Commit 3+ resumes from this
report.

## Section A — Current repo state

### A.1 git log --oneline -15 (HEAD = `447ebf0`)

```
447ebf0 fix(integrity): grandfather retrospective grammar examples + sweep retro doc
fe7f38c docs(retro): land integrity toolkit v1 retrospective
bbc38f0 docs(integrity): land commit 8 audit report + grandfather post-commit-8 findings
f576b5e feat(integrity): Cat 3 cubic-kernel numerical correctness (commit 8 — final)
fc20ef7 fix(integrity): install root workspace deps in CI for Stack B + skip suppressed in github output
203b14b fix(integrity): add @types/node to TS helper devDependencies
cc9e8c5 feat(integrity): Cat 2 Stack B contract verification (commit 7)
d546304 fix(integrity): force-track fixture compile_commands.json + land commit 6 audit
b0f7bce fix(integrity): simplify Stack C fixtures to not require stdc++ headers
5a1c193 feat(integrity): Cat 2 Stack C contract verification (commit 6)
b5b0310 docs(integrity): land commit 5 audit report
96de0c3 feat(integrity): Cat 2 Stack D contract verification (commit 5)
ab39303 docs(integrity): land commit 4b audit report
1e886e6 fix(integrity): add Vulkan + windowing deps to CI workflow
f7e012d feat(integrity): add CI workflow (commit 4b)
```

### A.2 git status

```
On branch main
Your branch is up to date with 'origin/main'.

Untracked files:
  (use "git add <file>..." to include in what will be committed)
	docs/diagnostics/_audits/integrity_v1_1_apispec_2026-05-15_architect1.md
	docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md

nothing added to commit but untracked files present (use "git add" to track)
```

Working tree is clean of source modifications. The two untracked files are
unrelated audit documents from a sibling architect-1 thread on integrity
v1.1 planning; this probe adds one further new file (this report).

### A.3 HEAD SHA

`447ebf00bac4c8b46c00441c142496405078b18e`

### A.4 references/SPlisHSPlasH submodule state

- Present locally: yes, at `references/SPlisHSPlasH/`
- HEAD SHA: `6bff55a6eaf14083d34650f22a268ce156b62b54` (matches expected
  2.16.1 anchor — `integrity-anchors.yaml` registered SHA confirmed).

## Section B — Sph-water source inventory at HEAD

### B.1 Shader inventory

Twenty-eight `.glsl` files plus `_struct_layouts.txt` (109-line reference
table). Filename / line-count / referenced-in-main.cpp / one-line description:

| File | LOC | Refs in main.cpp | One-line description |
| --- | --- | --- | --- |
| `apply_emitter.comp.glsl` | 65 | 3 | Reserve-tail emitter inject; one particle per thread into `[count, capacity)` tail |
| `apply_velocity.comp.glsl` | 52 | 3 | Apply per-particle pressure acceleration to velocity (`vel_i += dt * pa[gid].xyz`) |
| `bilateral_smooth.comp.glsl` | 58 | 3 | Separable bilateral filter for screen-space fluid depth (Müller-Fetterer 2007) |
| `cell_count.comp.glsl` | 24 | 3 | atomicAdd 1 to per-cell count (boids-3d port) |
| `composite.frag.glsl` | 78 | 3 | Final screen-space-fluid composite; view-space normal from smoothed depth |
| `compute_aij_pj.comp.glsl` | 124 | 3 | DFSPH per-particle `Σ_j (a_i − a_j)·∇W` stencil (commit 2b live kernel) |
| `compute_density_adv.comp.glsl` | 112 | 3 | DFSPH density-advect source `densityAdv_i = ρ/ρ₀ + h·V·Σ(v_i−v_j)·∇W` |
| `compute_density_change.comp.glsl` | 112 | 3 | DFSPH divergence-solve source `densityChange_i = V·Σ(v_i−v_j)·∇W` |
| `compute_pressure_accel.comp.glsl` | 118 | 3 | DFSPH per-particle pressure acceleration |
| `density_alpha.comp.glsl` | 131 | 3 | DFSPH ρ_i and α/ρ² factor (multiphase-compatible form) |
| `density_solve.comp.glsl` | 151 | 3 | **ORPHAN** (post-2b — superseded by `compute_aij_pj` + `jacobi_update_density`) |
| `divergence_solve.comp.glsl` | 147 | 3 | **ORPHAN** (post-2b — superseded by `compute_aij_pj` + `jacobi_update_divergence`) |
| `fullscreen.vert.glsl` | 11 | 2 | Fullscreen triangle |
| `initial_fill.comp.glsl` | 65 | 3 | Brick + optional droplet preset distribution |
| `integrate_forces.comp.glsl` | 129 | 3 | Two-mode kernel (FORCES_ONLY / POS_INTEGRATE) gated by `gravity_pad.w` |
| `jacobi_update_density.comp.glsl` | 61 | 3 | DFSPH density-solve Jacobi update (commit 2b live kernel) |
| `jacobi_update_divergence.comp.glsl` | 61 | 3 | DFSPH divergence-solve Jacobi update (commit 2b live kernel) |
| `morton_code.comp.glsl` | 51 | 3 | 30-bit Morton code per particle (Karras 2012) |
| `particle_sprite.frag.glsl` | 40 | 2 | Sphere-imposter depth writer (writes NDC Z to R32_SFLOAT) |
| `particle_sprite.vert.glsl` | 34 | 2 | Point-sprite vertex for depth pass |
| `prefix_sum_addback.comp.glsl` | 28 | 3 | Add per-block prefix → final cell_starts (boids-3d port) |
| `prefix_sum_block.comp.glsl` | 105 | 3 | Two-mode block scan (SCAN_ONLY / ADDBACK) |
| `prefix_sum_block_l2.comp.glsl` | 60 | 3 | Recursive second-level scan when num_blocks > WG_SIZE |
| `prefix_sum_local.comp.glsl` | 69 | 3 | Per-block Blelloch exclusive scan (boids-3d port) |
| `pressure_apply.comp.glsl` | 110 | 3 | **ORPHAN** (post-2b — superseded by `apply_velocity`) |
| `scatter.comp.glsl` | 33 | 3 | Counting-sort scatter to sorted index buffer |
| `thickness.frag.glsl` | 32 | 2 | Additive Gaussian thickness contribution |
| `thickness.vert.glsl` | 30 | 2 | Vertex stage for additive thickness pass |

Three shader files (`density_solve`, `divergence_solve`, `pressure_apply`)
remain on disk as orphans after commit 2b; cleanup was explicitly deferred
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
to a later commit per `phase11_5_commit2b_landing_2026-05-14.md:179-186`.
They are still referenced by name in `main.cpp` (3 hits each: `make_compute`
declaration, `allocateDescriptorSet`, and `try_reload`) — but never
dispatched. See Section H.2.

### B.2 main.cpp at HEAD

Total line count: **2698 lines**.

### B.3 Substep dispatch chain (`grep -n "pipe_.*\.dispatch"`)

```
1901:            pipe_initial_fill.dispatch(cb, ds_initial_fill, ...);
2224:                pipe_apply_emitter.dispatch(cmd, ds_apply_emitter, wg_particle, 1, 1);
2237:                pipe_morton_code.dispatch(cmd, ds_morton_code, wg_particle, 1, 1);
2242:                pipe_cell_count.dispatch(cmd, ds_cell_count, wg_particle, 1, 1);
2247:                pipe_prefix_sum_local.dispatch(cmd, ds_prefix_sum_local, wg_cell, 1, 1);
2254:                pipe_prefix_sum_block.dispatch(cmd, ds_prefix_sum_block, ...);
2260:                pipe_prefix_sum_block_l2.dispatch(cmd, ds_prefix_sum_block_l2, 1, 1, 1);
2267:                pipe_prefix_sum_block.dispatch(cmd, ds_prefix_sum_block, ...);
2273:                pipe_prefix_sum_addback.dispatch(cmd, ds_prefix_sum_addback, wg_cell, 1, 1);
2278:                pipe_scatter.dispatch(cmd, ds_scatter, wg_particle, 1, 1);
2285:                pipe_density_alpha.dispatch(cmd, ds_density_alpha, wg_particle, 1, 1);
2298:                pipe_compute_density_change.dispatch(cmd, ds_compute_density_change, wg_particle, 1, 1);
2314:                    pipe_compute_pressure_accel.dispatch(cmd, ds_compute_pressure_accel[i % 2], wg_particle, 1, 1);
2319:                        pipe_compute_aij_pj.dispatch(cmd, ds_compute_aij_pj[0], wg_particle, 1, 1, ...);
2324:                    pipe_jacobi_update_divergence.dispatch(cmd, ds_jacobi_update_divergence[i % 2], wg_particle, 1, 1);
2330:                pipe_compute_pressure_accel.dispatch(cmd, ds_compute_pressure_accel[final_i % 2], wg_particle, 1, 1);
2332:                pipe_apply_velocity.dispatch(cmd, ds_apply_velocity, wg_particle, 1, 1);
2342:                pipe_integrate_forces.dispatch(cmd, ds_integrate_forces, wg_particle, 1, 1, ...);
2355:                pipe_compute_density_adv.dispatch(cmd, ds_compute_density_adv, wg_particle, 1, 1);
2371:                    pipe_compute_pressure_accel.dispatch(cmd, ds_compute_pressure_accel[i % 2], wg_particle, 1, 1);
2376:                        pipe_compute_aij_pj.dispatch(cmd, ds_compute_aij_pj[0], wg_particle, 1, 1, ...);
2381:                    pipe_jacobi_update_density.dispatch(cmd, ds_jacobi_update_density[i % 2], wg_particle, 1, 1);
2386:                pipe_compute_pressure_accel.dispatch(cmd, ds_compute_pressure_accel[final_i % 2], wg_particle, 1, 1);
2388:                pipe_apply_velocity.dispatch(cmd, ds_apply_velocity, wg_particle, 1, 1);
2397:                pipe_integrate_forces.dispatch(cmd, ds_integrate_forces, wg_particle, 1, 1, ...);
2523:                    pipe_bilateral_smooth.dispatch(cmd, ds, wgX, wgY, 1);
```

Maps cleanly to `TimeStepDFSPH::step()`:
emit → sort → `density_alpha` → divergence-solve init (`compute_density_change`)
→ divergence-solve inner loop (`compute_pressure_accel` → `compute_aij_pj` →
`jacobi_update_divergence`) × `DFSPH_MAX_ITER_DIV` → finalize accel +
`apply_velocity` → non-pressure forces + position-integrate (`integrate_forces`
in two modes) → density-solve init (`compute_density_adv`) → density-solve
inner loop (`compute_pressure_accel` → `compute_aij_pj` → `jacobi_update_density`)
× `DFSPH_MAX_ITER_DENSITY` → finalize accel + `apply_velocity` →
position-integrate again.

**No `pipe_density_solve`, `pipe_divergence_solve`, or `pipe_pressure_apply`
dispatches** anywhere — orphan objects remain only as declarations.

### B.4 Solver-tuning rt state (`grep -nE "rt\.(maxIter|maxError|substeps|warmstart|CFL)"`)

```
2001:        meta["substeps"]       = rt.substeps;
2081:        rt.substeps      = meta.value("substeps", rt.substeps);
2199:        const float substep_dt = std::clamp(frame_dt / float(std::max(rt.substeps, 1)), DT_MIN, DT_MAX);
2219:        for (int sub = 0; !rt.paused && sub < rt.substeps && rt.particleCount > 0; ++sub) {
2612:            ImGui::SliderInt("Substeps", &rt.substeps, 1, 8);
```

The only solver-tuning field on `rt` is `substeps`. Specifically:

- **No `rt.maxIterDensity` / `rt.maxIterDivergence`** — the substep loop
  hard-codes the inner-loop iteration counts to `DFSPH_MAX_ITER_DENSITY` /
  `DFSPH_MAX_ITER_DIV` constants (probe-1 § O confirms this is the case;
  the panel sliders that *would* feed these were never wired).
- **No `rt.maxErrorDensity` / `rt.maxErrorDivergence`** — no convergence
  early-out exists, so the percent thresholds are never consulted.
- **No `rt.warmstart`** — no warmstart code path exists.
- **No `rt.CFL` / `rt.cflFactor`** — `frame_dt` comes from host-side
  `std::chrono` (clamped to `[1/240, 1/15]` then divided by `rt.substeps`
  and clamped to `[DT_MIN, DT_MAX]`); `CFL_FACTOR` is declared but unused
  (probe-1 § J).

### B.5 Constants block (`grep -nE "constexpr.*(CFL|DT_|GRAVITY|JACOBI|MIN_ITER|MAX_ITER)"`)

```
116:constexpr int   DFSPH_MIN_ITER_DENSITY   = 2;
117:constexpr int   DFSPH_MAX_ITER_DENSITY   = 100;
119:constexpr int   DFSPH_MAX_ITER_DIV       = 100;
124:constexpr float DFSPH_JACOBI_RELAX       = 0.5f;       // SPlisHSPlasH TimeStepDFSPH.cpp:606,:692
126:constexpr float CFL_FACTOR    = 0.5f;
127:constexpr float DT_MIN        = 1.0e-4f;
128:constexpr float DT_MAX        = 5.0e-3f;
130:constexpr float GRAVITY_Y     = -9.81f;
```

Adjacent (not captured by the constexpr-form grep but in the same block):

```
118:constexpr float DFSPH_MAX_ERROR_DENSITY  = 0.01f;     // PERCENT - 0.01 = 0.01% of rho_0
120:constexpr float DFSPH_MAX_ERROR_DIV      = 0.1f;       // PERCENT - 0.1 = 0.1% of rho_0
121:constexpr bool  DFSPH_DIV_SOLVER_DEFAULT = true;
122:constexpr float DFSPH_ALPHA_EPS          = 1.0e-5f;
```

These match the SPlisHSPlasH 2.16.1 defaults at the registered anchor
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
(see Section E and `TimeStepDFSPH.h:28-37`, modulo the upstream's pluralized
boundary handling).

## Section C — live-shader-1810 grandfather inventory

### C.1 `grep -rn "1.8.10" particle-fluids/sph-water/`

```
docs/notes.md:20:      SPlisHSPlasH 1.8.10 `TimeStepDFSPH.cpp:442-692` to be translated in the
shaders/density_solve.comp.glsl:12:// integrity-allow: cat1.upstream-citation; pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (migration target tracked in grandfather-catalog live-shader-1810); n/a
shaders/density_solve.comp.glsl:13://   Source s_i = 1 - ρ_adv/ρ₀:   SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:590
README.md:28:fetches Alembic 1.8.10 via CMake `FetchContent` and requires the apt package
README.md:90:- DFSPH solver: SPlisHSPlasH 1.8.10, especially `TimeStepDFSPH.cpp` and
docs/load-bearing-decisions.md:9:1.8.10 at SHA `c254caf2705ebf5271408dd37a091aa379258a38` for every formula
docs/load-bearing-decisions.md:54:Alembic 1.8.10 pinned at SHA `c254caf2705ebf5271408dd37a091aa379258a38`. NOT
shaders/apply_velocity.comp.glsl:6:// integrity-allow: cat1.upstream-citation; pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (migration target tracked in grandfather-catalog live-shader-1810); n/a
shaders/apply_velocity.comp.glsl:7://   apply_velocity dispatch outside the inner loop (instead of mid-loop in upstream's
shaders/density_alpha.comp.glsl:6:// integrity-allow: cat1.upstream-citation; pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (migration target tracked in grandfather-catalog live-shader-1810); n/a
shaders/density_alpha.comp.glsl:7://   Cubic spline kernel: SPlisHSPlasH 1.8.10 SPHKernels.h:43-78
shaders/density_alpha.comp.glsl:8:// integrity-allow: cat1.upstream-citation; pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (migration target tracked in grandfather-catalog live-shader-1810); n/a
shaders/density_alpha.comp.glsl:9://   α-factor:            SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:813-822 / :1175-1188
shaders/density_alpha.comp.glsl:10:// integrity-allow: cat1.upstream-citation; pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (migration target tracked in grandfather-catalog live-shader-1810); n/a
shaders/density_alpha.comp.glsl:11://   α floor ε:           SPlisHSPlasH 1.8.10 TimeStepDFSPH.h:28 = 1.0e-5
shaders/compute_aij_pj.comp.glsl:11:// Reference: SPlisHSPlasH 1.8.10 TimeStepDFSPH::compute_aij_pj scalar variant
shaders/jacobi_update_divergence.comp.glsl:7:// integrity-allow: cat1.upstream-citation; pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (migration target tracked in grandfather-catalog live-shader-1810); n/a
shaders/jacobi_update_divergence.comp.glsl:8:// Reference: SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:644 (source term s_i =
shaders/pressure_apply.comp.glsl:7:// integrity-allow: cat1.upstream-citation; pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (migration target tracked in grandfather-catalog live-shader-1810); n/a
shaders/pressure_apply.comp.glsl:8://   Velocity correction: SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:514-515 (divergence)
shaders/compute_density_adv.comp.glsl:9:// Reference: SPlisHSPlasH 1.8.10 TimeStepDFSPH::computeDensityAdv (TimeStepDFSPH.cpp:1188-1242).
shaders/divergence_solve.comp.glsl:4:// integrity-allow: cat1.upstream-citation; pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (migration target tracked in grandfather-catalog live-shader-1810); n/a
shaders/divergence_solve.comp.glsl:5://   Source s_i = -ρ̇_i:           SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:662
shaders/jacobi_update_density.comp.glsl:7:// integrity-allow: cat1.upstream-citation; pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (migration target tracked in grandfather-catalog live-shader-1810); n/a
shaders/jacobi_update_density.comp.glsl:8:// Reference: SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:591 (source term s_i =
shaders/compute_density_change.comp.glsl:10:// Reference: SPlisHSPlasH 1.8.10 TimeStepDFSPH::computeDensityChange.
shaders/compute_pressure_accel.comp.glsl:10:// Reference: SPlisHSPlasH 1.8.10 TimeStepDFSPH::computePressureAccel (fluid-only branch).
src/main.cpp:115:// DFSPH defaults — SPlisHSPlasH 1.8.10 at TimeStepDFSPH.cpp:35-41.
```

### C.2 `grep -rn "c254caf" particle-fluids/sph-water/`

```
docs/load-bearing-decisions.md:9:1.8.10 at SHA `c254caf2705ebf5271408dd37a091aa379258a38` for every formula
docs/load-bearing-decisions.md:54:Alembic 1.8.10 pinned at SHA `c254caf2705ebf5271408dd37a091aa379258a38`. NOT
```

The fabricated `c254caf...` SHA persists only in `load-bearing-decisions.md`
(two hits). Probe-1 / Setup-1 noted that the upstream-anchor consensus was
re-pointed to SPlisHSPlasH 2.16.1 (SHA `6bff55a6`) at the integrity-registry
level. The doc has not been touched since.

<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
### C.3 `grep -rn "integrity-allow:.*live-shader-1810" particle-fluids/sph-water/`

```
shaders/jacobi_update_divergence.comp.glsl:7
shaders/density_alpha.comp.glsl:6
shaders/density_alpha.comp.glsl:8
shaders/density_alpha.comp.glsl:10
shaders/density_solve.comp.glsl:12
shaders/divergence_solve.comp.glsl:4
shaders/apply_velocity.comp.glsl:6
shaders/pressure_apply.comp.glsl:7
shaders/jacobi_update_density.comp.glsl:7
```

<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
Nine `integrity-allow: ... live-shader-1810 ...` directives in shaders.
Zero in `src/main.cpp` or under `docs/`.

### C.4 Suppression-site / citation-site reconciliation (FINDING)

**The two grep counts diverge, and the explanation reveals two distinct
suppression idioms in the same tree.**

| Live-source file | `1.8.10` mentions | Suppression idiom |
| --- | --- | --- |
| `shaders/density_solve.comp.glsl` | 1 (`:13`) | `live-shader-1810` (`:12`) |
| `shaders/divergence_solve.comp.glsl` | 1 (`:5`) | `live-shader-1810` (`:4`) |
| `shaders/pressure_apply.comp.glsl` | 1 (`:8`) | `live-shader-1810` (`:7`) |
| `shaders/apply_velocity.comp.glsl` | 1 (`:7`) | `live-shader-1810` (`:6`) |
| `shaders/density_alpha.comp.glsl` | 3 (`:7, :9, :11`) | `live-shader-1810` (`:6, :8, :10`) |
| `shaders/jacobi_update_density.comp.glsl` | 1 (`:8`) | `live-shader-1810` (`:7`) |
| `shaders/jacobi_update_divergence.comp.glsl` | 1 (`:8`) | `live-shader-1810` (`:7`) |
| `shaders/compute_aij_pj.comp.glsl` | 1 (`:11`) | `cat1.intra-repo grandfathered-pre-v1` (`:12`) |
| `shaders/compute_density_adv.comp.glsl` | 1 (`:9`) | `cat1.intra-repo grandfathered-pre-v1` (`:8`) |
| `shaders/compute_density_change.comp.glsl` | 1 (`:10`) | **(no adjacent suppression)** |
| `shaders/compute_pressure_accel.comp.glsl` | 1 (`:10`) | **(no adjacent suppression)** |
| `src/main.cpp` | 1 (`:115`) | `cat1.intra-repo grandfathered-pre-v1` (`:114`) |
| `docs/notes.md` | 1 (`:20`) | `cat1.intra-repo grandfathered-pre-v1` (`:19`) |
| `docs/load-bearing-decisions.md` | 2 (`:9, :54`) | none |
| `README.md` | 2 (`:28, :90`) | none |

**Findings:**

1. **Suppression-label fragmentation.** Two distinct suppression idioms
   cover what is conceptually one category (pre-v1 SPlisHSPlasH 1.8.10
   citations awaiting 2.16.1 migration). The `live-shader-1810` label
   appears on the 5 commit-1/2a shaders + the 3 commit-2b shaders that
   inherited their docblocks from older shaders + the orphans-since-2b.
   The `cat1.intra-repo grandfathered-pre-v1` label is used on the
   commit-2a shaders that were copied/authored fresh against probe-3
   excerpts (which already cited 1.8.10 in their docblocks) and on the
   docs/main.cpp sites swept by the grandfather catalog. Mechanically
   suppressed either way (Section F confirms 0 hard-fail), but
   bookkeeping is inconsistent and complicates the eventual sweep to
   2.16.1.

2. **Two shaders have a 1.8.10 citation with no adjacent suppression
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
   line:** `compute_density_change.comp.glsl:10` and
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
   `compute_pressure_accel.comp.glsl:10`. Section F confirms these
   *are* suppressed (probably by a file-scope rule covering the whole
   particle-fluids/sph-water/shaders/ tree, since they don't appear in
   the integrity hard-fail list). Worth tracing how — a sweep that
   moves all sph-water shaders to 2.16.1 cites needs to know whether
   removing the suppression also removes the surface that protects
   these two.

3. The two **README.md `1.8.10` hits** (Alembic + SPlisHSPlasH lines) and
   the two **load-bearing-decisions.md `1.8.10` / `c254caf` hits** carry
   no `integrity-allow` directive at all. They must be suppressed by
   directory-scope or other mechanism — Section F confirms they don't
   fire.

## Section D — `load-bearing-decisions.md` status

Full file (81 lines):

```markdown
# sph-water — Load-bearing decisions

This document is a sim-local quick reference. For the full reasoning, see
`docs/phase11_sph_water.md` § 2.

## DFSPH (not WCSPH, not PCISPH, not IISPH, not PBF)

Divergence-Free SPH per Bender-Koschier 2015 + 2017. Anchored to SPlisHSPlasH
1.8.10 at SHA `c254caf2705ebf5271408dd37a091aa379258a38` for every formula
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
```

**Confirmed:** the Setup-1 G.5 finding is still true post-2b. The doc was
not touched between Setup-1 (2026-05-14) and HEAD (2026-05-15). Lines 9 and
54 anchor explicitly to `SPlisHSPlasH 1.8.10 / c254caf...` (line 9) and
`Alembic 1.8.10 / c254caf...` (line 54 — note: the Alembic line is
*correct* in version but uses the same SHA string for a different vendor;
the SHA value is real for Alembic and was fabricated for SPlisHSPlasH).
The README.md hits (lines 28, 90) make the same anchor-version claim.

Both files were excluded from the integrity grandfather sweep — neither
appears in the live-shader-1810 catalog or in `cat1.intra-repo
grandfathered-pre-v1`. They must be covered by a directory-scope or
file-glob suppression; the integrity output (Section F) confirms they
are not currently flagged.

## Section E — Upstream reference fact-gathering

All quotations from `references/SPlisHSPlasH/SPlisHSPlasH/...` at SHA
`6bff55a6eaf14083d34650f22a268ce156b62b54`.

### E.1 Warmstart (`USE_WARMSTART` / `USE_WARMSTART_V`)

**E.1.a — `#define` site**

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`SPlisHSPlasH/DFSPH/TimeStepDFSPH.h:1-15`:

```cpp
#ifndef __TimeStepDFSPH_h__
#define __TimeStepDFSPH_h__

#include "SPlisHSPlasH/Common.h"
#include "SPlisHSPlasH/TimeStep.h"
#include "SimulationDataDFSPH.h"
#include "SPlisHSPlasH/SPHKernels.h"

#define USE_WARMSTART
#define USE_WARMSTART_V

namespace SPH
{
	class SimulationDataDFSPH;
```

Both are defined unconditionally (no opt-out); the upstream build always
runs with warmstart enabled.

**E.1.b — Pressure-solve start-of-frame warmstart init**

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`TimeStepDFSPH.cpp:287-307` (inside the `pressureSolve` per-particle init
loop, after `computeDensityAdv` and `m_simulationData.getFactor(...) *= invH2`):

```cpp
//////////////////////////////////////////////////////////////////////////
// For the warm start we use 0.5 times the old pressure value.
// Note: We divide the value by h^2 since we multiplied it by h^2 at the end of
// the last time step to make it independent of the time step size.
//////////////////////////////////////////////////////////////////////////
#ifdef USE_WARMSTART
                if (m_simulationData.getDensityAdv(fluidModelIndex, i) > 1.0)
                    m_simulationData.getPressureRho2(fluidModelIndex, i) = static_cast<Real>(0.5) * min(m_simulationData.getPressureRho2(fluidModelIndex, i), static_cast<Real>(0.00025)) * invH2;
                else
                    m_simulationData.getPressureRho2(fluidModelIndex, i) = 0.0;
#else
                //////////////////////////////////////////////////////////////////////////
                // If we don't use a warm start, we directly compute a pressure value
                // by multiplying the density error with the factor.
                //////////////////////////////////////////////////////////////////////////
                //m_simulationData.getPressureRho2(fluidModelIndex, i) = 0.0;
                const Real s_i = static_cast<Real>(1.0) - m_simulationData.getDensityAdv(fluidModelIndex, i);
                const Real residuum = min(s_i, static_cast<Real>(0.0));     // r = b - A*p
                m_simulationData.getPressureRho2(fluidModelIndex, i) = -residuum * m_simulationData.getFactor(fluidModelIndex, i);
#endif
```

Key facts:

- `getPressureRho2` is the per-particle `p_i / ρ_i²` value carried across
  frames.
- The warmstart "seed" pressure for the next iteration is `0.5 *
  min(prev_p_rho2, 0.00025) * invH2`. The `0.5` halving is the warmstart
  damping; the `0.00025` ceiling caps inherited pressure so a single bad
  frame can't poison subsequent frames.
- Particles with `densityAdv ≤ 1.0` (under-compressed or balanced) have
  their seed reset to zero. Only over-compressed particles inherit
  pressure.
- The `* invH2` undoes the `* h²` scaling that the end-of-frame storage
  step applied (see E.1.d) — making warmstart timestep-size-independent.

**E.1.c — Divergence-solve start-of-frame warmstart init (`USE_WARMSTART_V`)**

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`TimeStepDFSPH.cpp:444-461` (inside the `divergenceSolve` per-particle init
loop, after `computeDensityChange` + particle-deficiency clamp +
`m_simulationData.getFactor(...) *= invH`):

```cpp
//////////////////////////////////////////////////////////////////////////
// For the warm start we use 0.5 times the old pressure value.
// Divide the value by h. We multiplied it by h at the end of
// the last time step to make it independent of the time step size.
//////////////////////////////////////////////////////////////////////////
#ifdef USE_WARMSTART_V
                if (densityAdv > 0.0)
                    m_simulationData.getPressureRho2_V(fluidModelIndex, i) = static_cast<Real>(0.5) * min(m_simulationData.getPressureRho2_V(fluidModelIndex, i), static_cast<Real>(0.5)) * invH;
                else
                    m_simulationData.getPressureRho2_V(fluidModelIndex, i) = 0.0;
#else
                //////////////////////////////////////////////////////////////////////////
                // If we don't use a warm start, directly compute a pressure value
                // by multiplying the divergence error with the factor.
                //////////////////////////////////////////////////////////////////////////
                m_simulationData.getPressureRho2_V(fluidModelIndex, i) = densityAdv * m_simulationData.getFactor(fluidModelIndex, i);
#endif
```

Key facts:

- Divergence-solve carries a separate per-particle warmstart buffer
  `PressureRho2_V` (V = velocity-divergence) — independent of the
  density-solve `PressureRho2`.
- Seed = `0.5 * min(prev_pv_rho2, 0.5) * invH`. The `0.5` ceiling caps
  the inherited divergence pressure; the `* invH` factor pairs with the
  end-of-frame `* h` (note: `h`, not `h²` — divergence solve scales by
  `h` because the source term is a rate `s_i = -ρ̇`).
- Particles with `densityAdv ≤ 0.0` (not divergent) reset to zero. Only
  divergent particles inherit pressure-V.

**E.1.d — End-of-frame `* h²` (pressure) and `* h` (divergence) storage**

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
Pressure-solve end-of-frame (`TimeStepDFSPH.cpp:364-383`):

```cpp
#ifdef USE_WARMSTART
    for (unsigned int fluidModelIndex = 0; fluidModelIndex < nFluids; fluidModelIndex++)
    {
        FluidModel* model = sim->getFluidModel(fluidModelIndex);
        const int numParticles = (int)model->numActiveParticles();
        #pragma omp parallel default(shared)
        {
            #pragma omp for schedule(static)
            for (int i = 0; i < numParticles; i++)
            {
                //////////////////////////////////////////////////////////////////////////
                // Multiply by h^2, the time step size has to be removed
                // to make the pressure value independent
                // of the time step size
                //////////////////////////////////////////////////////////////////////////
                m_simulationData.getPressureRho2(fluidModelIndex, i) *= h2;
            }
        }
    }
#endif
```

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
Divergence-solve end-of-frame (`TimeStepDFSPH.cpp:521-540`):

```cpp
#ifdef USE_WARMSTART_V
    for (unsigned int fluidModelIndex = 0; fluidModelIndex < nFluids; fluidModelIndex++)
    {
        FluidModel* model = sim->getFluidModel(fluidModelIndex);
        const int numParticles = (int)model->numActiveParticles();
        #pragma omp parallel default(shared)
        {
            #pragma omp for schedule(static)
            for (int i = 0; i < numParticles; i++)
            {
                //////////////////////////////////////////////////////////////////////////
                // Multiply by h, the time step size has to be removed
                // to make the pressure value independent
                // of the time step size
                //////////////////////////////////////////////////////////////////////////
                m_simulationData.getPressureRho2_V(fluidModelIndex, i) *= h;
            }
        }
    }
#endif
```

**Architectural summary for the GPU port:** The full warmstart cycle is
(a) at the start of each solve, divide-by-h(²) and halve the stored value
(with a ceiling clamp); (b) run the inner solver; (c) at the end of each
solve, multiply by h(²). Implementing this on the GPU adds two new persistent
SSBOs (`p_rho2[]`, `pv_rho2[]`) carried across frames, two extra full-particle
dispatches per frame (the end-of-frame `*= h²` and `*= h`), and modifies the
per-particle init kernel that today seeds `p_rho2` to zero.

### E.2 Convergence early-out (pressure-solve and divergence-solve outer loops)

**E.2.a — Pressure-solve outer loop and eta formula**

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`TimeStepDFSPH.cpp:309-343`:

```cpp
        }
    }

    m_iterations = 0;

    //////////////////////////////////////////////////////////////////////////
    // Start solver
    //////////////////////////////////////////////////////////////////////////

    Real avg_density_err = 0.0;
    bool chk = false;


    //////////////////////////////////////////////////////////////////////////
    // Perform solver iterations
    //////////////////////////////////////////////////////////////////////////
    while ((!chk || (m_iterations < m_minIterations)) && (m_iterations < m_maxIterations))
    {
        chk = true;
        for (unsigned int i = 0; i < nFluids; i++)
        {
            FluidModel *model = sim->getFluidModel(i);
            const Real density0 = model->getDensity0();

            avg_density_err = 0.0;
            pressureSolveIteration(i, avg_density_err);

            // Maximal allowed density fluctuation
            const Real eta = m_maxError * static_cast<Real>(0.01) * density0;  // maxError is given in percent
            chk = chk && (avg_density_err <= eta);
        }

        m_iterations++;
    }

    INCREASE_COUNTER("DFSPH - iterations", static_cast<Real>(m_iterations));
```

**Confirmed:** `eta = m_maxError * 0.01 * density0` for pressure solve. The
`0.01` is the percent→fraction conversion since `m_maxError` is stored in
percent (1.0 ≡ 1%, 0.01 ≡ 0.01%). Loop condition is:
- continue while `m_iterations < m_minIterations` (force at least
  `m_minIterations` iterations regardless of convergence), **or**
- continue while not-converged (`!chk`), capped at `m_maxIterations`.

`chk` becomes true only after a full iteration where every fluid model
satisfies `avg_density_err <= eta`.

**E.2.b — Divergence-solve outer loop and eta formula**

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`TimeStepDFSPH.cpp:465-497`:

```cpp
    m_iterationsV = 0;

    //////////////////////////////////////////////////////////////////////////
    // Start solver
    //////////////////////////////////////////////////////////////////////////

    Real avg_density_err = 0.0;
    bool chk = false;

    //////////////////////////////////////////////////////////////////////////
    // Perform solver iterations
    //////////////////////////////////////////////////////////////////////////
    while ((!chk || (m_iterationsV < 1)) && (m_iterationsV < maxIter))
    {
        chk = true;
        for (unsigned int i = 0; i < nFluids; i++)
        {
            FluidModel *model = sim->getFluidModel(i);
            const Real density0 = model->getDensity0();

            avg_density_err = 0.0;
            divergenceSolveIteration(i, avg_density_err);

            // Maximal allowed density fluctuation
            // use maximal density error divided by time step size
            const Real eta = (static_cast<Real>(1.0) / h) * maxError * static_cast<Real>(0.01) * density0;  // maxError is given in percent
            chk = chk && (avg_density_err <= eta);
        }

        m_iterationsV++;
    }

    INCREASE_COUNTER("DFSPH - iterationsV", static_cast<Real>(m_iterationsV));
```

**Confirmed:** `eta = (1/h) * maxError * 0.01 * density0` for divergence
solve. Differs from pressure-solve by the `1/h` factor — because the
divergence source is a **rate** (`-ρ̇`), so the convergence threshold is
also a rate. `m_minIterations` is hard-coded to `1` here (literal in
the loop condition), not configurable.

**E.2.c — `avg_density_err` accumulation pattern (pressure-solve)**

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`TimeStepDFSPH.cpp:558-619`:

```cpp
    Real density_error = 0.0;

    #pragma omp parallel default(shared)
    {
        ...
        #pragma omp for reduction(+:density_error) schedule(static)
        for (int i = 0; i < numParticles; i++)
        {
            if (model->getParticleState(i) != ParticleState::Active)
                continue;

            Real aij_pj = compute_aij_pj(fluidModelIndex, i);
            aij_pj *= h * h;

            //////////////////////////////////////////////////////////////////////////
            // Compute source term: s_i = 1 - rho_adv
            // Note: that due to our multiphase handling, the multiplier rho0
            // is missing here
            //////////////////////////////////////////////////////////////////////////
            const Real& densityAdv = m_simulationData.getDensityAdv(fluidModelIndex, i);
            const Real s_i = static_cast<Real>(1.0) - densityAdv;


            //////////////////////////////////////////////////////////////////////////
            // Update the value p/rho^2 ...
            //////////////////////////////////////////////////////////////////////////
            Real& p_rho2_i = m_simulationData.getPressureRho2(fluidModelIndex, i);
            const Real residuum = min(s_i - aij_pj, static_cast<Real>(0.0));     // r = b - A*p
            //p_rho2_i -= residuum * m_simulationData.getFactor(fluidModelIndex, i);

            p_rho2_i = max(p_rho2_i - static_cast<Real>(0.5) * (s_i - aij_pj) * m_simulationData.getFactor(fluidModelIndex, i), static_cast<Real>(0.0));

            //////////////////////////////////////////////////////////////////////////
            // Compute the sum of the density errors
            //////////////////////////////////////////////////////////////////////////
            density_error -= density0 * residuum;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Compute the average density error
    //////////////////////////////////////////////////////////////////////////
    avg_density_err = density_error / numParticles;
}
```

**Confirmed:** loop-scalar `density_error` accumulates `-density0 *
residuum` per active particle (residuum is `min(s_i - aij_pj, 0)`, i.e.,
only contributions from over-compressed particles count). `avg_density_err
= density_error / numParticles` after the parallel reduction. The
`pressureSolveIteration` filters by `ParticleState::Active` (line 578).

**E.2.d — `avg_density_err` accumulation pattern (divergence-solve)**

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`TimeStepDFSPH.cpp:621-706`:

```cpp
void TimeStepDFSPH::divergenceSolveIteration(const unsigned int fluidModelIndex, Real &avg_density_err)
{
    Simulation *sim = Simulation::getCurrent();
    FluidModel *model = sim->getFluidModel(fluidModelIndex);
    const Real density0 = model->getDensity0();
    const int numParticles = (int)model->numActiveParticles();
    if (numParticles == 0)
        return;

    const unsigned int nFluids = sim->numberOfFluidModels();
    const unsigned int nBoundaries = sim->numberOfBoundaryModels();
    const Real h = TimeManager::getCurrent()->getTimeStepSize();
    const Real invH = static_cast<Real>(1.0) / h;

    Real density_error = 0.0;

    #pragma omp parallel default(shared)
    {
        ...
        #pragma omp for reduction(+:density_error) schedule(static)
        for (int i = 0; i < numParticles; i++)
        {
            Real aij_pj = compute_aij_pj(fluidModelIndex, i);
            aij_pj *= h;

            //////////////////////////////////////////////////////////////////////////
            // Compute source term: s_i = -d rho / dt
            //////////////////////////////////////////////////////////////////////////
            const Real& densityAdv = m_simulationData.getDensityAdv(fluidModelIndex, i);
            const Real s_i = -densityAdv;

            //////////////////////////////////////////////////////////////////////////
            // Update the value p/rho^2 ...
            //////////////////////////////////////////////////////////////////////////
            Real& pv_rho2_i = m_simulationData.getPressureRho2_V(fluidModelIndex, i);
            Real residuum = min(s_i - aij_pj, static_cast<Real>(0.0));     // r = b - A*p

            unsigned int numNeighbors = 0;
            for (unsigned int pid = 0; pid < sim->numberOfPointSets(); pid++)
                numNeighbors += sim->numberOfNeighbors(fluidModelIndex, pid, i);

            // in case of particle deficiency do not perform a divergence solve
            if (!sim->is2DSimulation())
            {
                if (numNeighbors < 20)
                    residuum = 0.0;
            }
            else
            {
                if (numNeighbors < 7)
                    residuum = 0.0;
            }
            //pv_rho2_i -= residuum * m_simulationData.getFactor(fluidModelIndex, i);
            pv_rho2_i = max(pv_rho2_i - static_cast<Real>(0.5)*(s_i - aij_pj) * m_simulationData.getFactor(fluidModelIndex, i), static_cast<Real>(0.0));


            //////////////////////////////////////////////////////////////////////////
            // Compute the sum of the divergence errors
            //////////////////////////////////////////////////////////////////////////
            density_error -= density0 * residuum;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Compute the average divergence error
    //////////////////////////////////////////////////////////////////////////
    avg_density_err = density_error / numParticles;
}
```

**Confirmed:** same reduction pattern, with two behavior differences:

1. **No `ParticleState::Active` filter** — every particle is summed
   (matches probe-3's § C observation).
2. **Particle-deficiency clamp** (`numNeighbors < 20` for 3D /
   `< 7` for 2D) zeroes `residuum` for under-sampled particles so they
   don't contribute to the divergence pressure update *or* the
   convergence-error sum.
3. **`aij_pj` is scaled by `h`** here (versus `h²` in the pressure-solve
   path), matching the rate-vs-delta distinction in the source term.

### E.3 Akinci2012 boundary handling

**E.3.a — `grep -rn "Akinci2012" references/SPlisHSPlasH/SPlisHSPlasH/`** (filtered to relevant call sites; 31 total hits)

```
BoundaryModel_Akinci2012.cpp:1:#include "BoundaryModel_Akinci2012.h"
BoundaryModel_Akinci2012.cpp:13:BoundaryModel_Akinci2012::BoundaryModel_Akinci2012() :
BoundaryModel_Akinci2012.cpp:48:void BoundaryModel_Akinci2012::computeBoundaryVolume()
BoundaryModel_Akinci2012.cpp:77:void BoundaryModel_Akinci2012::initModel(...)
BoundaryModel_Akinci2012.cpp:112:void BoundaryModel_Akinci2012::performNeighborhoodSearchSort()
BoundaryModel_Akinci2012.h:21:	class BoundaryModel_Akinci2012 : public BoundaryModel
DFSPH/TimeStepDFSPH.cpp:785:    if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Akinci2012)  [AVX]
DFSPH/TimeStepDFSPH.cpp:860,:922,:995,:1077  [AVX call sites — all dispatching forall_boundary_neighbors_avx(...)]
DFSPH/TimeStepDFSPH.cpp:1150:    if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Akinci2012)  [scalar]
DFSPH/TimeStepDFSPH.cpp:1217,:1272,:1335,:1401  [scalar call sites — all dispatching forall_boundary_neighbors(...)]
Emitter.cpp:51, :70  [emit-time boundary check]
TimeStep.cpp:90, :150-153  [per-frame boundary-volume call (see E.3.c)]
Simulation.cpp:391, :442  [CFL maxVel scan over boundary particles]
XSPH.cpp:102, :177  [XSPH viscosity boundary branch]
```

**E.3.b — Scalar `computeDFSPHFactor` Akinci2012 branch**

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`TimeStepDFSPH.cpp:1106-1186` (full scalar `computeDFSPHFactor`):

```cpp
void TimeStepDFSPH::computeDFSPHFactor(const unsigned int fluidModelIndex)
{
    //////////////////////////////////////////////////////////////////////////
    // Init parameters
    //////////////////////////////////////////////////////////////////////////

    Simulation *sim = Simulation::getCurrent();
    const unsigned int nFluids = sim->numberOfFluidModels();
    const unsigned int nBoundaries = sim->numberOfBoundaryModels();
    FluidModel *model = sim->getFluidModel(fluidModelIndex);
    const int numParticles = (int) model->numActiveParticles();

    #pragma omp parallel default(shared)
    {
        //////////////////////////////////////////////////////////////////////////
        // Compute pressure stiffness denominator
        //////////////////////////////////////////////////////////////////////////

        #pragma omp for schedule(static)
        for (int i = 0; i < numParticles; i++)
        {
            //////////////////////////////////////////////////////////////////////////
            // Compute gradient dp_i/dx_j * (1/kappa)  and dp_j/dx_j * (1/kappa)
            // (see Equation (8) and the previous one [BK17])
            // Note: That in all quantities rho0 is missing due to our
            // implementation of multiphase simulations.
            //////////////////////////////////////////////////////////////////////////
            const Vector3r &xi = model->getPosition(i);
            Real sum_grad_p_k = 0.0;
            Vector3r grad_p_i;
            grad_p_i.setZero();

            //////////////////////////////////////////////////////////////////////////
            // Fluid
            //////////////////////////////////////////////////////////////////////////
            forall_fluid_neighbors(
                const Vector3r grad_p_j = -fm_neighbor->getVolume(neighborIndex) * sim->gradW(xi - xj);
                sum_grad_p_k += grad_p_j.squaredNorm();
                grad_p_i -= grad_p_j;
            );

            //////////////////////////////////////////////////////////////////////////
            // Boundary
            //////////////////////////////////////////////////////////////////////////
            if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Akinci2012)
            {
                forall_boundary_neighbors(
                    const Vector3r grad_p_j = -bm_neighbor->getVolume(neighborIndex) * sim->gradW(xi - xj);
                    grad_p_i -= grad_p_j;
                );
            }

            else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Koschier2017)
            {
                forall_density_maps(
                    grad_p_i -= gradRho;
                );
            }
            else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Bender2019)
            {
                forall_volume_maps(
                    const Vector3r grad_p_j = -Vj * sim->gradW(xi - xj);
                    grad_p_i -= grad_p_j;
                );
            }

            sum_grad_p_k += grad_p_i.squaredNorm();

            //////////////////////////////////////////////////////////////////////////
            // Compute factor as: factor_i = -1 / (a_ii * rho_i^2)
            // where a_ii is the diagonal entry of the linear system
            // for the pressure A * p = source term
            //////////////////////////////////////////////////////////////////////////
            Real &factor = m_simulationData.getFactor(fluidModelIndex, i);
            if (sum_grad_p_k > m_eps)
                factor = static_cast<Real>(1.0) / (sum_grad_p_k);
            else
                factor = 0.0;
        }
    }
}
```

**Note on probe Setup-1 G.6 line numbers:** Setup-1 cited "lines 793-811
scalar variant" for the Akinci branch. At HEAD that range is actually
inside the **AVX** variant (lines 793-811 are part of the AVX
`computeDFSPHFactor` at line 735). The scalar variant lives at lines
**1150-1156** (Akinci branch only) and **1106-1186** (full function).
This is not a regression; Setup-1's line numbers were probably swapped.
Probe-3 also references the scalar variant.

The **Akinci boundary contribution** to `grad_p_i` in `computeDFSPHFactor`
is `-bm_neighbor->getVolume(neighborIndex) * sim->gradW(xi - xj)` accumulated
over boundary neighbors. Critically, the boundary contribution is NOT added
to `sum_grad_p_k` (the squared-norm accumulator) — only to `grad_p_i`
(the vector accumulator) which is then squared once after all neighbors
are summed. This asymmetry is upstream-correct and means boundary
neighbors contribute to α via the squared-vector-norm path only, not the
per-neighbor-squared-norm path.

**E.3.c — Where boundary-particle density contribution gets computed**

The same `forall_boundary_neighbors` macro pattern appears in:

- `computeDFSPHFactor` (lines 1150-1156) — boundary contribution to α
- `pressureSolveIteration / computeDensityAdv` (lines 1217-1228) —
  boundary contribution to density advection source term
- `computeDensityChange` (lines 1272-1283) — boundary contribution to
  divergence solve source term
- `compute_aij_pj` (lines 1335-1349) — boundary contribution to the
  pressure stencil
- `computePressureAccel` (lines 1401-1412) — boundary contribution to
  the per-particle pressure acceleration

Each branch uses the same Akinci2012 idiom: read `bm_neighbor->getVolume(neighborIndex)`
(per-boundary-particle volume computed at frame start, see E.3.d) and
multiply by `sim->gradW(xi - xj)` or `sim->W(xi - xj)` depending on
whether the integrand is a gradient or a kernel value.

**E.3.d — `computeBoundaryVolume` per-frame dispatch**

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`BoundaryModel_Akinci2012.cpp:48-75` (the function itself):

```cpp
void BoundaryModel_Akinci2012::computeBoundaryVolume()
{
    Simulation *sim = Simulation::getCurrent();
    const unsigned int nFluids = sim->numberOfFluidModels();
    NeighborhoodSearch *neighborhoodSearch = Simulation::getCurrent()->getNeighborhoodSearch();

    const unsigned int numBoundaryParticles = numberOfParticles();

    #pragma omp parallel default(shared)
    {
        #pragma omp for schedule(static)
        for (int i = 0; i < (int)numBoundaryParticles; i++)
        {
            Real delta = sim->W_zero();
            for (unsigned int pid = nFluids; pid < sim->numberOfPointSets(); pid++)
            {
                BoundaryModel_Akinci2012 *bm_neighbor = static_cast<BoundaryModel_Akinci2012*>(sim->getBoundaryModelFromPointSet(pid));
                for (unsigned int j = 0; j < neighborhoodSearch->point_set(m_pointSetIndex).n_neighbors(pid, i); j++)
                {
                    const unsigned int neighborIndex = neighborhoodSearch->point_set(m_pointSetIndex).neighbor(pid, i, j);
                    delta += sim->W(getPosition(i) - bm_neighbor->getPosition(neighborIndex));
                }
            }
            const Real volume = static_cast<Real>(1.0) / delta;
            m_V[i] = volume;
        }
    }
}
```

Per-boundary-particle volume is `1 / Σ_{j ∈ boundary} W(x_i - x_j)`. This
is the Akinci2012 "1/(self + boundary-neighbor kernel sum)" volume
formula. The result `m_V[i]` is what every `forall_boundary_neighbors`
branch above reads via `bm_neighbor->getVolume(neighborIndex)`.

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`computeBoundaryVolume` is called from `TimeStep.cpp:90` (initial
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
neighbor-search after `performNeighborhoodSearchSort`) and `TimeStep.cpp:150-153`
(every frame's boundary update, gated on dynamic / animated boundaries —
static boundaries' volumes are computed once and never recomputed).
Setup-1 / probe-3's note that "computeBoundaryVolume is per-frame" is
technically only true for dynamic boundaries; the current sph-water has
no dynamic boundaries (preset boxes only), so this is a one-time setup
cost rather than a per-frame cost.

### E.4 CFL timestep

**E.4.a — CFL field declarations and defaults**

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`Simulation.cpp:68-70`:

```cpp
m_cflFactor = 0.5;
m_cflMinTimeStepSize = static_cast<Real>(0.0001);
m_cflMaxTimeStepSize = static_cast<Real>(0.005);
```

Upstream defaults: `cflFactor = 0.5`, `cflMinTimeStepSize = 1e-4`,
`cflMaxTimeStepSize = 5e-3`. These match sph-water's `CFL_FACTOR = 0.5f`,
`DT_MIN = 1.0e-4f`, `DT_MAX = 5.0e-3f` constants exactly. (sph-water
clamps `frame_dt / substeps` to `[DT_MIN, DT_MAX]` but does not consult
`maxVel`.)

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`Simulation.cpp:200, :205, :210`:

```cpp
CFL_FACTOR = createNumericParameter("cflFactor", "CFL - factor", &m_cflFactor);
CFL_MIN_TIMESTEPSIZE = createNumericParameter("cflMinTimeStepSize", "CFL - min. time step size", &m_cflMinTimeStepSize);
CFL_MAX_TIMESTEPSIZE = createNumericParameter("cflMaxTimeStepSize", "CFL - max. time step size", &m_cflMaxTimeStepSize);
```

**E.4.b — `updateTimeStepSizeCFL` body**

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`Simulation.cpp:415-492` (the function that computes the next dt):

```cpp
void Simulation::updateTimeStepSizeCFL()
{
    const Real radius = m_particleRadius;
    Real h = TimeManager::getCurrent()->getTimeStepSize();
    Simulation *sim = Simulation::getCurrent();
    const unsigned int nBoundaries = sim->numberOfBoundaryModels();

    // Approximate max. position change due to current velocities
    Real maxVel = 0.0;
    const Real diameter = static_cast<Real>(2.0)*radius;

    // fluid particles
    for (unsigned int fluidModelIndex = 0; fluidModelIndex < numberOfFluidModels(); fluidModelIndex++)
    {
        FluidModel *fm = getFluidModel(fluidModelIndex);
        const unsigned int numParticles = fm->numActiveParticles();
        for (unsigned int i = 0; i < numParticles; i++)
        {
            const Vector3r &vel = fm->getVelocity(i);
            const Vector3r &accel = fm->getAcceleration(i);
            const Real velMag = (vel + accel*h).squaredNorm();
            if (velMag > maxVel)
                maxVel = velMag;
        }
    }

    // boundary particles
    if (getBoundaryHandlingMethod() == BoundaryHandlingMethods::Akinci2012)
    {
        for (unsigned int i = 0; i < numberOfBoundaryModels(); i++)
        {
            BoundaryModel_Akinci2012 *bm = static_cast<BoundaryModel_Akinci2012*>(getBoundaryModel(i));
            if (bm->getRigidBodyObject()->isDynamic() || bm->getRigidBodyObject()->isAnimated())
            {
                for (unsigned int j = 0; j < bm->numberOfParticles(); j++)
                {
                    const Vector3r &vel = bm->getVelocity(j);
                    const Real velMag = vel.squaredNorm();
                    if (velMag > maxVel)
                        maxVel = velMag;
                }
            }
        }
    }
    ... [Koschier2017 and Bender2019 branches]

    // avoid division by zero
    if (maxVel < static_cast<Real>(1.0e-9))
        maxVel = static_cast<Real>(1.0e-9);

    // Approximate max. time step size
    h = m_cflFactor * static_cast<Real>(0.4) * (diameter / (sqrt(maxVel)));

    h = min(h, m_cflMaxTimeStepSize);
    h = max(h, m_cflMinTimeStepSize);

    TimeManager::getCurrent()->setTimeStepSize(h);
}
```

**Key formula (line 487):** `h = cflFactor * 0.4 * (diameter / sqrt(maxVel))`.
The hardcoded `0.4` is the upstream CFL safety constant (a Courant-number-like
multiplier on top of `cflFactor`). `maxVel` is the **squared** velocity
magnitude over all fluid particles (note: `velMag = (vel + accel*h).squaredNorm()`,
not `.norm()`); the `sqrt(maxVel)` in the dt formula recovers the actual
maximum speed.

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
**Note on `updateTimeStepSize` (the dispatcher):** `Simulation.cpp:395-413`
has a wrapper that supports two CFL modes:
- `m_cflMethod == 1`: just call `updateTimeStepSizeCFL()`
- `m_cflMethod == 2`: call `updateTimeStepSizeCFL()`, then adapt up/down
  by ±10% if the previous frame's iteration count was outside `[5, 10]`
  (heuristic feedback from solver convergence). Mode 2 also clamps the
  returned `h` to be no greater than the previous frame's `h` (`h = min(h,
  previous_h)`), making it conservative: only ever reduces, never grows
  faster than the CFL formula allows.

For the GPU port, mode 1 is the right starting target. Mode 2 requires
reading back `m_iterations` / `m_iterationsV` to host each frame, which
re-introduces the CPU-readback issue that `load-bearing-decisions.md`'s
§ "Why fixed inner-iteration count?" explicitly avoids.

## Section F — Integrity toolkit's view of sph-water surface

### F.1 Headline result

`python3 -m integrity --output human --no-audit-log` returns:

```
integrity: 2 pass, 0 soft-warn, 0 hard-fail, 1126 suppressed
```

Net of suppressions, integrity is clean. The 1126 suppressed findings
include the full sph-water grandfather pool plus the wider repository's
pre-v1 surface.

### F.2 `grep -E "(sph-water|particle-fluids)"` on full output

The full output includes the suppressed-but-classified-as-HARD_FAIL
detail lines for every grandfathered finding. Filtered to sph-water,
the integrity tool would have classified these as hard-fails absent
suppression:

```
[sph-water findings, suppressed]
  particle-fluids/sph-water/docs/load-bearing-decisions.md:1-81: cited line 81 exceeds file line count 80
  particle-fluids/sph-water/shaders/_struct_layouts.txt:1-109: cited line 109 exceeds file line count 108
  particle-fluids/sph-water/docs/notes.md:20  [cat1.intra-repo path-doesn't-resolve]
  particle-fluids/sph-water/shaders/compute_aij_pj.comp.glsl:13  [cat1.intra-repo path-doesn't-resolve]
  particle-fluids/sph-water/shaders/compute_density_adv.comp.glsl:9  [cat1.intra-repo path-doesn't-resolve]
  particle-fluids/sph-water/shaders/density_alpha.comp.glsl:3  [cat1.intra-repo path-doesn't-resolve]
  particle-fluids/sph-water/shaders/density_solve.comp.glsl:7, :9, :15, :17, :19  [5× cat1.intra-repo]
  particle-fluids/sph-water/shaders/divergence_solve.comp.glsl:7, :9, :11  [3× cat1.intra-repo]
  particle-fluids/sph-water/src/main.cpp:115, :124  [cat1.intra-repo]
```

These are all "TimeStepDFSPH.cpp:XXX path does not resolve under
particle-fluids/sph-water/{shaders,src,docs} or repo-root" violations —
i.e., the citations point at upstream files that live under
`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/`, but the integrity rule for
`cat1.intra-repo` resolves only relative to the citing file's tree.
**Net effect:** the citations are syntactically valid as upstream cites
but are classified as cat1.intra-repo by the cite-extractor (probably
because they lack the explicit `SPlisHSPlasH 1.8.10` / `SPlisHSPlasH 2.16.1`
prefix that would route them to `cat1.upstream-citation`). All are
covered by `cat1.intra-repo grandfathered-pre-v1` suppressions.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
The first two entries (`docs/load-bearing-decisions.md:1-81 cited line
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
81 exceeds file line count 80` and `shaders/_struct_layouts.txt:1-109
cited line 109 exceeds file line count 108`) are off-by-one
range-citation errors that are also suppressed.

### F.3 `cat1.upstream-citation` filtered to `1.8.10|2.16.1`

```
SPlisHSPlasH 2.16.1 SPHKernels.h:43-85: path 'SPHKernels.h' does not resolve under references/SPlisHSPlasH
SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:591:                              version '1.8.10' does not match registered anchor '2.16.1' for SPlisHSPlasH
SPlisHSPlasH 1.8.10 SPHKernels.h:43-78:                                  version '1.8.10' does not match registered anchor '2.16.1' for SPlisHSPlasH
SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:813-822:                          version '1.8.10' does not match registered anchor '2.16.1' for SPlisHSPlasH
SPlisHSPlasH 1.8.10 TimeStepDFSPH.h:28:                                 version '1.8.10' does not match registered anchor '2.16.1' for SPlisHSPlasH
SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:514-515:                          version '1.8.10' does not match registered anchor '2.16.1' for SPlisHSPlasH
SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:662:                              version '1.8.10' does not match registered anchor '2.16.1' for SPlisHSPlasH
SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:590:                              version '1.8.10' does not match registered anchor '2.16.1' for SPlisHSPlasH
[... 13+ more repetitions of the same 6 unique mismatches across files]
SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:758-760: ...
```

Two distinct integrity findings collapse into this filter:

<!-- integrity-allow: cat1.upstream-citation; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
1. One *expected* finding: a `SPlisHSPlasH 2.16.1 SPHKernels.h:43-85`
   citation lacks a `references/SPlisHSPlasH/SPlisHSPlasH/` prefix and
   so the path doesn't resolve. (Probably from `density_alpha.comp.glsl`
   or similar; the integrity tool catches a citation form mismatch.)
2. ~15 *expected* findings: every `SPlisHSPlasH 1.8.10` mention is
   classified as a version mismatch against the registered 2.16.1 anchor.
   All suppressed via `live-shader-1810` or `cat1.intra-repo grandfathered-pre-v1`.

**Net for commit 3:** if commit 3 adds a new live citation to
`SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:XXX` it will fire as a new
cat1.upstream-citation hard-fail (the grandfather covers *existing* lines,
not new ones added in commit 3 — verified by reading the grandfather-catalog
structure that points to specific files/lines). Either:
- Author the new citation as `SPlisHSPlasH 2.16.1` (the registered
  anchor), so it routes to path-resolution against
  `references/SPlisHSPlasH/SPlisHSPlasH/` and resolves cleanly.
- Or add a new explicit `integrity-allow` directive — but this fights
  the toolkit's intent: the v2.16.1 anchor exists *so that* live code
  can cite real lines that resolve.

### F.4 `cat2.public-symbol-used-c`

Headline:

```
integrity: 0 pass, 0 soft-warn, 0 hard-fail, 111 suppressed
```

First few suppressed entries (all from `common/common-cpp/include/gpusims/`):

```
HARD_FAIL: cat2.public-symbol-used-c at common/common-cpp/include/gpusims/camera.hpp:48 — public method 'std::gpusims::Camera::mode'
HARD_FAIL: cat2.public-symbol-used-c at common/common-cpp/include/gpusims/camera.hpp:57 — public method 'std::gpusims::Camera::view'
HARD_FAIL: cat2.public-symbol-used-c at common/common-cpp/include/gpusims/camera.hpp:59 — public method 'std::gpusims::Camera::projection'
HARD_FAIL: cat2.public-symbol-used-c at common/common-cpp/include/gpusims/camera.hpp:61 — public method 'std::gpusims::Camera::viewProjection'
HARD_FAIL: cat2.public-symbol-used-c at common/common-cpp/include/gpusims/camera.hpp:64
```

None target sph-water source directly. These are all common-cpp public
surface that no Stack C consumer uses (i.e., dead-API in
common-cpp). The relevance to commit 3 is **indirect**: if commit 3
introduces a new use of one of these methods from `particle-fluids/sph-water/src/main.cpp`,
the suppression for that symbol can be lifted — verifying its presence in
the suppress list before / removal after is a useful check.

**Net for commit 3 surface:** the integrity toolkit currently flags zero
sph-water hard-fails. Adding code to sph-water that cites new upstream
references at `SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:XXX` will hard-fail;
adding code that cites `SPlisHSPlasH 2.16.1` against real lines under
`references/SPlisHSPlasH/SPlisHSPlasH/` will pass.

## Section G — Deferred-to-commit-3+ inventory

Walking the prior phase11_5 audits for items explicitly flagged as
"deferred", "follow-up", "banked", or "commit 3 / 2c / 4 / architect-2":

### G.1 Bucket A — Solver correctness (the meat of commit 3)

| Item | Source | What |
| --- | --- | --- |
| **A.1** Convergence early-out (pressure-solve) | probe-3 § B + commit-2b audit ¶164-166 + probe-1 § O | Implement `eta = m_maxError * 0.01 * density0` outer-loop break. Requires CPU readback of `avg_density_err`, OR a GPU-side reduction + sparse readback every K frames. probe-2 § P.1 + load-bearing-decisions.md "Why fixed inner-iteration count?" explicitly bank readback to v1.1. |
| **A.2** Convergence early-out (divergence-solve) | probe-3 § C + same | `eta = (1/h) * m_maxErrorV * 0.01 * density0` outer-loop break. Same readback constraint as A.1. probe-3 line 1835 explicitly flags `1/h` factor difference. |
| **A.3** Warmstart (`USE_WARMSTART`) | probe-3 ¶1839 + commit-2b audit ¶162-166 (jacobi-relax / boundary / dt-scaling neighbors) | Per-particle `p_rho2[]` SSBO + end-of-frame `*= h²` kernel + start-of-frame `0.5 * min(prev, 0.00025) * invH2` seed init. Replaces the current "init to zero" seed. |
| **A.4** Warmstart (`USE_WARMSTART_V`) | same | Per-particle `pv_rho2[]` SSBO + end-of-frame `*= h` kernel + start-of-frame `0.5 * min(prev, 0.5) * invH` seed. Divergence-solve variant. |
| **A.5** CFL-derived dt | probe-1 § J + probe-1 ¶1431, ¶1459 | Replace wall-clock `frame_dt` with `cflFactor * 0.4 * (diameter / sqrt(maxVel))`, clamped to `[cflMinTimeStepSize, cflMaxTimeStepSize]`. Requires per-frame `maxVel` reduction (GPU-side or readback). `CFL_FACTOR` is already declared and unused (line 126). |
| **A.6** Akinci2012 boundary handling | Setup-1 G.6, ¶436 + probe-3 § O | Validate that sph-water shaders match the upstream Akinci2012 branch: per-boundary-particle `volume = 1/Σ W` precomputed at frame start (static boundaries → one-time setup); per-fluid-particle boundary contribution as `-V_j * gradW(x_i - x_j)` for α-factor; analogous for density-adv / density-change / aij_pj / pressure-accel. Five upstream branch sites (§ E.3.c) to mirror. |
| **A.7** `dt` scaling between `compute_aij_pj` and Jacobi update | commit-2b audit ¶162-164 | Verify `solver_mode==0` (density) multiplies `aij_pj *= h²` and `solver_mode==1` (divergence) multiplies `aij_pj *= h`. Upstream lines 582 / 656. |
| **A.8** Mass formula validation | probe-1 § J (lines 1014-1018) + general | Confirm host `m = ρ₀ * (h × radius)³ × diameter_factor` matches upstream `m = ρ₀ * V_particle` derivation. |
| **A.9** `ParticleState::Active` filter in divergence-solve | probe-3 line 1836 | Upstream `divergenceSolveIteration` does **not** filter by `Active` (line 651 absent), but `pressureSolveIteration` does (line 578). If the GPU port has identical kernels, this is a behavioral mismatch — but it's a minor one because the GPU side doesn't track `ParticleState::Active`, all particles below `particleCount` are always active. |

### G.2 Bucket B — Cleanup

| Item | Source | What |
| --- | --- | --- |
| **B.1** Delete orphan shaders | commit-2b audit ¶179-186 | Remove from disk: `density_solve.comp.glsl`, `divergence_solve.comp.glsl`, `pressure_apply.comp.glsl`. |
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| **B.2** Delete orphan host-side pipeline objects | same | Remove `pipe_density_solve`, `pipe_divergence_solve`, `pipe_pressure_apply` declarations (`main.cpp:1284-1291`), descriptor-set allocations (`:1384-1392`), reload helpers (`:2177-2180`), and the `writeDfsphSolveDescriptor` / `writePressureApplyDescriptor` helpers (probe-2 § N or thereabouts). |
| **B.3** Re-anchor `load-bearing-decisions.md` to 2.16.1 | Setup-1 G.5 ¶432 | Replace lines 9 + 54 of `docs/load-bearing-decisions.md` to point at `SPlisHSPlasH 2.16.1` at SHA `6bff55a6...` (and verify that the doc's five non-obvious-convention list — α/ρ², h=4r, etc. — still applies to 2.16.1). |
| **B.4** Re-anchor `README.md` lines 28 + 90 | implicit from B.3 | Same as B.3 — `README.md` carries the same 1.8.10 anchor claim. |
| **B.5** Trim `ds_compute_aij_pj[1]` redundancy | commit-2b audit ¶187-192 | `ds_compute_aij_pj` is a `[2]` ping-pong but only `[0]` is dispatched; both indices have identical writes. Free one descriptor-set slot. |
| **B.6** Consolidate the live-shader-1810 vs `cat1.intra-repo grandfathered-pre-v1` suppression idioms | finding C.4 | Either re-label all 13+ live 1.8.10 citations under a single grandfather category, or — preferred — migrate the citations to `SPlisHSPlasH 2.16.1 TimeStepDFSPH.cpp:XXX` with paths-that-resolve, removing the need for any suppression. |

### G.3 Diagnostic items (read-only)

| Item | Source | What |
| --- | --- | --- |
| **D.1** Visual regression scrutiny | commit-2b audit ¶147-170 | After 2b's stencil rewrite, the horizontal-banding artifact "should diminish or change character." Run the binary, observe, characterize remaining artifacts. Likely candidates: jacobi relaxation 0.5 (currently hardcoded; upstream line 606/692), dt scaling (A.7), boundary handling (A.6), missing CFL clamp (A.5). |
| **D.2** AABB / wall handling | probe-1 § L (general) | sph-water uses an AABB position clamp; upstream uses Akinci2012 boundary particles. The AABB clamp is *the* boundary handling currently in v1 — confirm whether A.6's Akinci2012 work means *replacing* the AABB or *complementing* it. |

## Section H — Incidentals

### H.1 No non-Phase-11.5 work has touched sph-water

`git log --oneline -- particle-fluids/sph-water/` (last 10):

```
8af5672 feat(integrity): grandfather-sweep pre-v1 findings (commit 4a)
4adc84a feat(sph-water): rewrite DFSPH substep dispatch chain (commit 2b)
2b53045 feat(sph-water): add DFSPH solver restructure surface (commit 2a)
7294ee4 Phase 11 (sph-water): integrate_forces mode via push constant
83a01d6 Phase 11 (sph-water): canonical DFSPH UBO layout
be924ef fix(phase11): integrate_forces UBO layout + missing position pass
28927ca fix(phase11): depth pass writes to color, not gl_FragDepth
1f02fc1 feat(phase11): sph-water DFSPH dispatch chain + screen-space fluid render
09c0d9f feat(phase11): sph-water scaffold + shaders + § 5.H CMakeLists wire-up
706c7cb chore: phase 0 — repository skeleton
```

Only `8af5672` touched sph-water since 2b — and that was the integrity
toolkit grandfather sweep that added the `live-shader-1810` and
`cat1.intra-repo grandfathered-pre-v1` `integrity-allow` directives.
No solver / shader / behavior changes. The codebase is structurally
unchanged from the post-2b state described in
`phase11_5_commit2b_landing_2026-05-14.md`.

### H.2 Orphan shaders still on disk (confirms commit-2b audit's deferral)

```
particle-fluids/sph-water/shaders/density_solve.comp.glsl       (151 lines, May 14 16:47)
particle-fluids/sph-water/shaders/divergence_solve.comp.glsl    (147 lines, May 14 16:47)
particle-fluids/sph-water/shaders/pressure_apply.comp.glsl      (110 lines, May 14 16:47)
```

All three carry `live-shader-1810` suppression directives — which is
*misleading* now, since they are no longer "live" (no dispatch reaches
them). They are scheduled for B.1 deletion; B.6 suppression-cleanup
follows naturally if those files are deleted.

### H.3 Orphan host-side objects in main.cpp

```
1284:    auto pipe_divergence_solve = make_compute("divergence_solve.comp.glsl", ...);
1286:    auto pipe_density_solve    = make_compute("density_solve.comp.glsl", ...);
1291:    auto pipe_pressure_apply   = make_compute("pressure_apply.comp.glsl", ...);
1384-1392:    ds_divergence_solve[2], ds_density_solve[2], ds_pressure_apply  [allocations]
2177-2180:    try_reload(pipe_divergence_solve, ...) / try_reload(pipe_density_solve, ...) / try_reload(pipe_pressure_apply, ...)
```

Compile, occupy descriptor-pool slots, never run. Targeted by B.2.

### H.4 Suppression idiom split (also flagged in C.4)

Worth tracking because it complicates the eventual 2.16.1 migration:
some live-shader sites use `live-shader-1810` (intended for migration
tracking), others use the generic `cat1.intra-repo grandfathered-pre-v1`
catalog entry, and two shader files have no adjacent suppression at all
(presumably covered by a higher-scope rule, since the integrity tool
reports zero hard-fails). A single sweep that migrates citations to
2.16.1 needs to be aware of all three patterns.

### H.5 Setup-1 G.6 cited wrong line range for scalar `computeDFSPHFactor`

Setup-1 said scalar `computeDFSPHFactor` Akinci branch is at "lines
793-811." At HEAD it's actually at lines 1150-1156 (Akinci branch) inside
the scalar function at 1106-1186; the 793-811 range is in the **AVX**
twin (function at 735-1103). Probably a probe-2 / probe-3 line-number
swap. Documented here so commit 3's mapping table cites the right scalar
line numbers.

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
### H.6 docs/notes.md:20 still pre-v1

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a -->
`docs/notes.md:20` says `"SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:442-692 to
be translated in the [next phase]"` — covered by `cat1.intra-repo
grandfathered-pre-v1` suppression at `:19`. Re-anchor to 2.16.1 alongside
B.3 / B.4.

---

End of probe report. File: `docs/diagnostics/_audits/phase11_5_resume_probe_2026-05-15_architect1.md`.
