# Phase 11.5 — interim retrospective

**Status:** paused, not closed. Sph-water DFSPH solver structurally complete per upstream Akinci2012 convention. Visible behavior still wrong — the initial column does not collapse as expected; horizontal banding persists with sparse particle haze. User is taking over with deeper diagnostic methods.

**Date paused:** 2026-05-15

**Last commit on this phase:** commit 6 (`d84b9ad` — H1 boundary placement + preset clearance) plus its audit (`c3391f7`). See `git log particle-fluids/sph-water/` for the full sequence.

---

## What shipped

Six substantive commits + fix-ups + audits. All structurally landed and CI-green.

| Commit | SHA | What |
|---|---|---|
| 1 | (pre-this-session, in Phase 11 baseline) | Host-side gradient-kernel-norm fix (48 → 8) |
| 2a | `2b53045` | DFSPH solver surface (5 new shaders, 2 SSBOs, descriptors) — no behavior change |
| 2b | `4adc84a` | Dispatch-chain rewrite + 2 jacobi_update shaders |
| (ride-along) | `09c0d9f` (citations) → `09bc7b4` (re-anchor) | Migrate citations from `SPlisHSPlasH 1.8.10 / c254caf...` to real `SPlisHSPlasH 2.16.1 / 6bff55a6...` |
| 3 | `f9f2cb9` | Akinci2012 boundary handling (5 shaders + new SSBOs + spatial hash + `compute_boundary_volume`) |
| 3 hotfix | `b648894` | Use `Buffer::stage()` for DeviceLocal boundary uploads (`uploadDirect` NULL-deref under RelWithDebInfo) |
| 4 | `40f73e9` | Boundary seam dedup + (wrong) inset into fluid domain |
| 5 | `dde5f22` | Inset flipped outside AABB (also wrong) |
| 6 | `d84b9ad` | H1 boundary placement on AABB plane + 2 × particleRadius preset clearance |

Spec citations migrated from fabricated `SPlisHSPlasH 1.8.10 / c254caf...` to real `SPlisHSPlasH 2.16.1 / 6bff55a6...` across all live shaders and docs (commit `09bc7b4`).

## What still doesn't work

User-visible Dam-Break at 1M tier, commit 6: initial column stands with horizontal banding at frame 35. Sparse particle haze in the upper region. Same defect class as the post-2b state — the boundary handling was structurally added but did not change the visible failure mode.

Diagnostic data at frame 50 shows the solver is *stable*:

- 0 NaN positions, 0 NaN velocities
- 0 AABB escapes (fail-safe working)
- 2/529,396 particles with velocity > 100 m/s (statistical noise)
- Mean velocity 2.04 m/s, std 1.93 m/s (quiet — not exploding, but also not falling under gravity as expected for a Dam-Break)
- Position ranges: x ∈ [−2.000, +2.005], y ∈ [−1.004, +2.000], z ∈ [−1.000, +1.000] (touching every AABB face, fail-safe holding)

Stability is not correctness. The solver runs without blowing up but does not produce the expected fluid behavior. The architect-chat workflow's diagnostic methods (boundary volume readback, sign-convention spot-check, particle state readback) all returned "looks correct" results while the visible behavior remained broken.

## Architect errors banked

**Error 1: Commit 4 inset wrong direction.** The prompt said "inset into the rigid body" in the rationale but gave concrete examples insetting into the fluid domain. Claude Code followed the concrete examples. Result: fluid plastered against walls. One wasted commit.

**Error 2: Commit 5 over-corrected.** Flipped inset all the way outside AABB. Result: boundary repulsion too weak, column reverted to pre-3 banded state. Another wasted commit.

**Error 3: Two failed fixes before reading upstream directly.** Should have run the boundary placement probe (`docs/diagnostics/_audits/phase11_5_boundary_placement_probe_2026-05-15.md`) before commit 4, not after commit 5. The probe was 30 minutes of read-only work and would have prevented both wasted commits.

**Error 4 (probably the active one):** confusing stability metrics with correctness. The diagnostic at commit 6 reported "0 NaN, 0 escape, quiet velocities" and the architect read that as evidence of success. The visible behavior is the truth; everything else is a proxy. The proxy data didn't catch whatever is still wrong.

## What's likely still broken (architect speculation, not verified)

The column-stays-standing-with-banding pattern is the post-2b signature: pressure correction not actually constraining density. The architect's commit-3 diagnosis was that missing Akinci2012 boundary contribution was the cause; that diagnosis appears to have been incomplete or wrong. Akinci2012 is now landed correctly per upstream and the symptom persists.

Speculative candidates for what's actually wrong:

1. **dt scaling between `compute_aij_pj` and Jacobi update.** The pre-flight in commit 3 said this was correct; the verification may have been superficial. Worth a second look with a single fluid particle test rig.
2. **Solver iteration count too low.** Current is hardcoded `DFSPH_MIN_ITER_DENSITY = 2`, `DFSPH_MAX_ITER_DIV = 100`. Upstream solver runs until `avg_density_err < eta`. With 2 iterations and no early-out, the density correction may not converge enough to produce visible collapse. Convergence early-out (currently deferred to commit 7+) might actually be load-bearing for visible correctness, not optional polish.
3. **Mass formula.** Current `m = ρ₀ * (2r)³` assumes cubic close-packed lattice; SPH initial-fill is also cubic close-packed at spacing 2r, so this should be consistent. Worth a 5-minute verification.
4. **Initial density.** If `densityAdv` at frame 0 is already wildly off ρ₀, the source term `s_i = 1 - densityAdv` is large and the solver chases its tail. A single-frame readback of `da[gid].x` (density) and `da[gid].z` (densityAdv) at frame 0 would say.
5. **Something we haven't named yet.** The architect-chat workflow has now failed three times to identify the root cause. Whatever it is may need methods this workflow doesn't have access to (RenderDoc trace, NVIDIA Nsight Graphics, single-particle test rig, comparison against running upstream SPlisHSPlasH on the same scene).

## What deferred work remains nominal

Independent of the current visible-defect problem, the following items are still on the Phase 11.5 backlog:

- Warmstart (`USE_WARMSTART` / `USE_WARMSTART_V`)
- Convergence early-out with sparse readback (may not be optional — see speculation 2)
- CFL-derived dt
- Orphan shader cleanup (`density_solve`, `divergence_solve`, `pressure_apply` + their host objects + descriptor sets)
- `ds_compute_aij_pj[1]` redundancy trim
- Compute_boundary_volume.comp.glsl docblock citation (cross-line `cat1.intra-repo` finding, queued since commit 3)

These are all deferred until the visible-defect problem is understood.

## What the architect-chat workflow did not handle well

For self-correction on the next phase:

- **Three failed fix attempts before reading upstream directly.** Diagnostic-before-fix discipline was lost partway through.
- **Proxy metrics conflated with correctness.** The user's screenshot is the ground truth; stability metrics aren't. Future fluid-sim work should treat visual smoke as the validation gate and instrument toward it explicitly.
- **Over-claimed "progress" on partial fixes.** Commits 4, 5, 6 each moved the failure mode somewhere but none produced visible fluid. The architect framed each as forward motion when the honest framing was "different broken state."

The user named this directly at the end of the session and was correct to.

## Handoff

Repo state at this commit: clean, CI-green on the toolkit-gated workflows. The sph-water binary builds, runs without crashing, and produces a recognizable but non-functional Dam-Break sim. The user is taking over with deeper diagnostic methods. No further architect-chat work on Phase 11.5 until the user surfaces something the workflow can productively act on.

Speculative next steps for whoever picks this up:

1. Run upstream SPlisHSPlasH on a matching scene and compare numerically against our solver at frame 0, 10, 100. Density values, alpha factors, pressure-rho2 values — read them off both, find where they diverge.
2. Build a minimal single-particle-against-flat-floor test rig. One fluid particle, one row of boundary samples, one timestep. Compute the force vector by hand vs by GPU and compare.
3. RenderDoc capture of a single substep loop. Read what each shader actually wrote into its output SSBO.

None of these are architect-chat work. They're deeper-instrumentation work the user is better positioned to drive.
