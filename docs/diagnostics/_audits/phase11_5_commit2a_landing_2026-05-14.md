---
date: 2026-05-14
audit_kind: commit-landing
phase: 11.5
commit_label: 2a
status: staged-unverified
references:
  - docs/diagnostics/_audits/phase11_5_probe3_2026-05-14_architect1.md
  - docs/diagnostics/_audits/phase11_5_commit2_verification_2026-05-14.md
  - docs/diagnostics/_audits/phase11_5_commit1_landing_2026-05-14.md
---

# Phase 11.5 commit 2a — landing record

Adds the new DFSPH solver surface (5 shaders + 2 SSBOs + 5 descriptor helpers
+ 5 pipelines + descriptor sets + hot-reload + CMakeLists) without touching
the substep dispatch chain. Visible behavior is intended to be byte-identical
to the pre-commit state because none of the new pipelines are dispatched yet
— commit 2b will rewrite the substep loop to actually call them.

## A. Change summary

This commit lands the additive surface for the DFSPH solver restructure
identified by probe-3 (the upstream `pressureSolveIteration` /
`divergenceSolveIteration` calls `computePressureAccel` at the top of each
inner iteration; our current GPU port computes it once after the loop). Five
new shaders carry the three logical operations the rewrite splits each
solver kernel into: source-term (`compute_density_adv`,
`compute_density_change`), per-particle pressure acceleration
(`compute_pressure_accel`), stencil sum (`compute_aij_pj`), and post-loop
velocity application (`apply_velocity`). Two new SSBOs back them
(`pressure_accel: N×16 B`, `aij_pj_scratch: N×4 B`). Descriptor-write
helpers, pipeline allocations, descriptor-set bindings, hot-reload watches,
and the build's shader-copy list are extended in lock-step. Nothing in the
substep dispatch chain (§ 4.G in `main.cpp`) is touched, so behavior remains
identical until commit 2b rewires the loop.

## B. New shader inventory

| Shader                                | Lines | Bindings                                                                            |
| ------------------------------------- | ----- | ----------------------------------------------------------------------------------- |
| `compute_density_adv.comp.glsl`       | 111   | 0:particles 1:density_alpha(RW) 2:cell_starts 3:sorted_index 4:UBO                  |
| `compute_density_change.comp.glsl`    | 112   | 0:particles 1:density_alpha(RW) 2:cell_starts 3:sorted_index 4:UBO                  |
| `compute_pressure_accel.comp.glsl`    | 118   | 0:particles 1:density_alpha 2:pressure_read 3:cell_starts 4:sorted_index 5:pressure_accel 6:UBO |
| `compute_aij_pj.comp.glsl`            | 123   | 0:particles 1:density_alpha 2:cell_starts 3:sorted_index 4:pressure_accel 5:aij_pj_scratch 6:UBO + PC{uint solver_mode} |
| `apply_velocity.comp.glsl`            |  51   | 0:particles(RW) 1:pressure_accel 2:UBO                                              |

Each shader inlines the canonical 112-byte DFSPH UBO verbatim (same as
`density_alpha.comp.glsl:25-53`) and the four kernel helpers
(`expand_bits_10`, `morton_encode_3d`, `kernel_W`, `kernel_gradW`). The
kernel/helper duplication across DFSPH shaders is a known maintenance issue
called out in probe-1 Section P; consolidating it is intentionally out of
scope for commit 2a.

### Stencil notes

- `compute_aij_pj` implements the correct upstream stencil
  `V_i * Σ_j (a_i − a_j) · ∇W_ij`, then scales by `dt²` (density mode,
  `pc.solver_mode==0`) or `dt` (divergence mode, `pc.solver_mode==1`). This
  replaces the current placeholder coupling
  `particleMass · dot(grad_W, grad_W)` that lives in
  `density_solve.comp.glsl` / `divergence_solve.comp.glsl`. The fix only
  takes effect once commit 2b's dispatch-chain rewrite consumes it.
- `compute_pressure_accel` uses the single-fluid-collapsed pSum
  (`p_rho2_i + p_rho2_j`) — upstream's `(ρ0_n/ρ0_s)` factor is exactly 1
  in our port.
- `compute_density_change` deliberately omits the particle-deficiency
  `if (numNeighbors < 20) densityAdv = 0` clamp; upstream applies it in
  `divergenceSolve`/`divergenceSolveIteration`, not inside
  `computeDensityChange` itself.
- All five shaders skip self explicitly (`if (j == gid) continue;`). On GPU
  this is cheaper than relying on `(v_i − v_i) = 0` or `(a_i − a_i) = 0`
  cancelling naturally.

## C. Host-side surface inventory

### New SSBOs (added to `TierResources`)

| Field            | Size        | Purpose                                            |
| ---------------- | ----------- | -------------------------------------------------- |
| `pressure_accel` | `N × 16 B`  | Per-particle pressure acceleration (vec4, .w=0)    |
| `aij_pj_scratch` | `N × 4 B`   | Per-particle aij_pj sum, dt-scaled                 |

Both share `kSsboUsage` (STORAGE | TRANSFER_SRC | TRANSFER_DST) and live in
`DeviceLocal`. `gv::Buffer` is RAII (no entry in `destroyTierResources`,
which only handles the sampler).

### New descriptor-write helpers (added after `writePressureApplyDescriptor`)

- `writeComputeDensityAdvDescriptor` — 4 SSBO + 1 UBO (bindings 0..4)
- `writeComputeDensityChangeDescriptor` — identical binding shape; trampolines
  to the above so future divergence stays in one place
- `writeComputePressureAccelDescriptor` — 6 SSBO + 1 UBO (bindings 0..6)
- `writeComputeAijPjDescriptor` — 6 SSBO + 1 UBO (bindings 0..6)
- `writeApplyVelocityDescriptor` — 2 SSBO + 1 UBO (bindings 0..2)

### New pipelines + descriptor sets

| Pipeline                       | Descriptor sets allocated                          |
| ------------------------------ | -------------------------------------------------- |
| `pipe_compute_density_adv`     | `ds_compute_density_adv` (1)                       |
| `pipe_compute_density_change`  | `ds_compute_density_change` (1)                    |
| `pipe_compute_pressure_accel`  | `ds_compute_pressure_accel[2]` (ping-pong p_read)  |
| `pipe_compute_aij_pj`          | `ds_compute_aij_pj[2]` (pre-allocated for split)   |
| `pipe_apply_velocity`          | `ds_apply_velocity` (1)                            |

The `pressure_accel` ping-pong sets bind `pressure_a` and `pressure_b` for
`PressureRead` so the Jacobi inner-loop can flip read buffers without
descriptor rewrites mid-frame. `aij_pj` sets are bound identically today —
the second set is reserved against a future divergence-vs-density binding
split (per prompt note); it is harmless to consolidate later.

Descriptor-write calls are placed in the existing `rewriteAllDescriptors`
lambda, which is invoked at initial tier-create (line 1309) AND on
tier-change (line 2315) — so the new descriptors are correctly re-written on
preset/tier swaps.

### Hot-reload + build

Five new `reload_*` booleans, five `W_watch(...)` registrations on the new
shader filenames, and five `try_reload(...)` calls in the main-loop reload
block. The five new shader filenames are appended to the explicit
`SHADER_SOURCES` list in `particle-fluids/sph-water/CMakeLists.txt` so the
build copies them next to the binary like the other DFSPH kernels.

## D. Verification

### D.1 `ls` of 5 new shaders

```
particle-fluids/sph-water/shaders/apply_velocity.comp.glsl
particle-fluids/sph-water/shaders/compute_aij_pj.comp.glsl
particle-fluids/sph-water/shaders/compute_density_adv.comp.glsl
particle-fluids/sph-water/shaders/compute_density_change.comp.glsl
particle-fluids/sph-water/shaders/compute_pressure_accel.comp.glsl
```

### D.2 `grep -c "static_assert(sizeof(Layout) == 112"` (UBO sentinel unchanged)

```
1
```

### D.3 `grep -n "pressure_accel\|aij_pj_scratch" particle-fluids/sph-water/src/main.cpp | head -30`

```
703:                                                VkBuffer pressure_accel,
710:    VkDescriptorBufferInfo b5{}; b5.buffer=pressure_accel;  b5.range=VK_WHOLE_SIZE;
737:                                        VkBuffer pressure_accel,
738:                                        VkBuffer aij_pj_scratch,
744:    VkDescriptorBufferInfo b4{}; b4.buffer=pressure_accel;  b4.range=VK_WHOLE_SIZE;
745:    VkDescriptorBufferInfo b5{}; b5.buffer=aij_pj_scratch;  b5.range=VK_WHOLE_SIZE;
769:                                         VkBuffer pressure_accel,
772:    VkDescriptorBufferInfo b1{}; b1.buffer=pressure_accel;  b1.range=VK_WHOLE_SIZE;
956:    gv::Buffer pressure_accel;       // N x 16 bytes — per-particle pressure acceleration (commit 2a; unused until commit 2b)
957:    gv::Buffer aij_pj_scratch;       // N x 4 bytes  — per-particle aij_pj sum (commit 2a; unused until commit 2b)
1007:    r.pressure_accel = gv::Buffer::create(ctx, std::size_t(particle_count) * 16,
1008:                                          kSsboUsage, gv::MemoryUsage::DeviceLocal, "pressure_accel");
1009:    r.aij_pj_scratch = gv::Buffer::create(ctx, std::size_t(particle_count) * 4,
1010:                                          kSsboUsage, gv::MemoryUsage::DeviceLocal, "aij_pj_scratch");
1254:    auto pipe_compute_pressure_accel = make_compute("compute_pressure_accel.comp.glsl",
1346:    VkDescriptorSet ds_compute_pressure_accel[2] = {
1347:        pipe_compute_pressure_accel.allocateDescriptorSet(),
1348:        pipe_compute_pressure_accel.allocateDescriptorSet(),
1460:        // pressure_accel: ds[0] reads p_read=pressure_a; ds[1] reads p_read=pressure_b.
1461:        writeComputePressureAccelDescriptor(ctx.device(), ds_compute_pressure_accel[0],
1465:            tier.pressure_accel.handle(),
1467:        writeComputePressureAccelDescriptor(ctx.device(), ds_compute_pressure_accel[1],
1471:            tier.pressure_accel.handle(),
1478:            tier.pressure_accel.handle(),
1479:            tier.aij_pj_scratch.handle(),
1484:            tier.pressure_accel.handle(),
1485:            tier.aij_pj_scratch.handle(),
1488:            tier.particles.handle(), tier.pressure_accel.handle(),
1845:    bool reload_compute_pressure_accel=false;
1871:    W_watch("compute_pressure_accel.comp.glsl", &reload_compute_pressure_accel);
```

### D.4 `grep -n "make_compute" particle-fluids/sph-water/src/main.cpp | head -30`

```
1192:    // make_compute / make_compute_pinned transferred verbatim from ES's
1199:    auto make_compute = [&](const std::string& shader_rel,
1208:    auto make_compute_pinned = [&](const std::string& shader_rel,
1230:    auto pipe_apply_emitter    = make_compute("apply_emitter.comp.glsl",
1232:    auto pipe_initial_fill     = make_compute("initial_fill.comp.glsl",
1234:    auto pipe_morton_code      = make_compute("morton_code.comp.glsl",
1236:    auto pipe_density_alpha    = make_compute("density_alpha.comp.glsl",
1238:    auto pipe_divergence_solve = make_compute("divergence_solve.comp.glsl",
1240:    auto pipe_density_solve    = make_compute("density_solve.comp.glsl",
1242:    auto pipe_integrate_forces = make_compute("integrate_forces.comp.glsl",
1245:    auto pipe_pressure_apply   = make_compute("pressure_apply.comp.glsl",
1250:    auto pipe_compute_density_adv    = make_compute("compute_density_adv.comp.glsl",
1252:    auto pipe_compute_density_change = make_compute("compute_density_change.comp.glsl",
1254:    auto pipe_compute_pressure_accel = make_compute("compute_pressure_accel.comp.glsl",
1256:    auto pipe_compute_aij_pj         = make_compute("compute_aij_pj.comp.glsl",
1259:    auto pipe_apply_velocity         = make_compute("apply_velocity.comp.glsl",
1262:    auto pipe_cell_count          = make_compute_pinned("cell_count.comp.glsl",
1264:    auto pipe_prefix_sum_local    = make_compute_pinned("prefix_sum_local.comp.glsl",
1266:    auto pipe_prefix_sum_block    = make_compute_pinned("prefix_sum_block.comp.glsl",
1269:    auto pipe_prefix_sum_block_l2 = make_compute_pinned("prefix_sum_block_l2.comp.glsl",
1271:    auto pipe_prefix_sum_addback  = make_compute_pinned("prefix_sum_addback.comp.glsl",
1273:    auto pipe_scatter             = make_compute_pinned("scatter.comp.glsl",
1275:    auto pipe_bilateral_smooth    = make_compute("bilateral_smooth.comp.glsl",
```

5 new `pipe_compute_*` / `pipe_apply_velocity` entries appended after
`pipe_pressure_apply` (8 existing DFSPH+other pipelines → 13). The
`make_compute_pinned` block (cell_count, prefix_sum_*, scatter) is unchanged.

### D.5 `git diff --stat`

```
 .gitignore                                         |   7 +
 common/common-cpp/include/gpusims/gpu_profiler.hpp |   2 +-
 common/common-cpp/src/gpu_profiler.cpp             |   2 +-
 common/common-cpp/src/vk/context.cpp               |   1 +
 particle-fluids/sph-water/CMakeLists.txt           |   6 +
 particle-fluids/sph-water/src/main.cpp             | 226 ++++++++++++++++++++-
 6 files changed, 241 insertions(+), 3 deletions(-)
```

The four non-sph-water lines (`.gitignore`, `gpu_profiler.{hpp,cpp}`,
`vk/context.cpp`) were already modified in the working tree before this
commit — they are pre-existing dirt, not part of commit 2a (see § F.1).

### D.6 `git status` (top of working tree)

```
On branch main
Your branch is up to date with 'origin/main'.

Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
	modified:   .gitignore
	modified:   common/common-cpp/include/gpusims/gpu_profiler.hpp
	modified:   common/common-cpp/src/gpu_profiler.cpp
	modified:   common/common-cpp/src/vk/context.cpp
	modified:   particle-fluids/sph-water/CMakeLists.txt
	modified:   particle-fluids/sph-water/src/main.cpp

Untracked files:
  (use "git add <file>..." to include in what will be committed)
	docs/diagnostics/
	particle-fluids/sph-water/shaders/apply_velocity.comp.glsl
	particle-fluids/sph-water/shaders/compute_aij_pj.comp.glsl
	particle-fluids/sph-water/shaders/compute_density_adv.comp.glsl
	particle-fluids/sph-water/shaders/compute_density_change.comp.glsl
	particle-fluids/sph-water/shaders/compute_pressure_accel.comp.glsl

no changes added to commit (use "git add" and/or "git commit -a")
```

All changes are unstaged; nothing has been committed or pushed.

## E. Behavioral expectations

Running the binary post-commit-2a should produce **identical visible behavior**
to pre-commit-2a. The substep dispatch chain (§ 4.G in `main.cpp`) still
runs the legacy sequence — `density_alpha → divergence_solve → integrate_forces
→ density_solve → pressure_apply` — and never touches any of the five new
pipelines or the two new SSBOs. The new descriptor sets are written every
frame against valid handles (because they live in `rewriteAllDescriptors`)
but Vulkan does not care about unused descriptor sets; `vkUpdateDescriptorSets`
just refreshes them.

The only runtime side-effects that exist before commit 2b lands are:

1. Two extra device-local allocations of `N × 16 B + N × 4 B = N × 20 B`
   (e.g., ~80 KiB at N=4096, ~80 MiB at N=4M).
2. Five extra `vkCmdComputeShader` pipeline objects living in memory.
3. Five new entries in the hot-reload watch list — editing the new shader
   files triggers a pipeline rebuild but doesn't change which pipelines run.
4. Five extra `vkUpdateDescriptorSets` calls in `rewriteAllDescriptors` —
   only invoked at startup and tier-change, never per-frame.

Validation layers, debug-name-tagged buffers, the existing 112-byte UBO
static_assert, and the existing dispatch sequence are unchanged. If commit
2a alters visible behavior, that itself is a bug to investigate before
landing 2b.

## F. Incidental findings

### F.1 Pre-existing dirty working tree

Before commit 2a touched anything, the tree already had modifications to
`.gitignore`, `common/common-cpp/include/gpusims/gpu_profiler.hpp`,
`common/common-cpp/src/gpu_profiler.cpp`, `common/common-cpp/src/vk/context.cpp`,
and `particle-fluids/sph-water/src/main.cpp` (the integrate_forces push-const
work from commit 7294ee4 was likely partially re-touched). These should be
treated separately when staging — `git add` should target only:

- `particle-fluids/sph-water/CMakeLists.txt`
- `particle-fluids/sph-water/src/main.cpp`
- `particle-fluids/sph-water/shaders/compute_*.comp.glsl`
- `particle-fluids/sph-water/shaders/apply_velocity.comp.glsl`
- this audit file under `docs/diagnostics/_audits/`

### F.2 `docs/diagnostics/` audits folder path

The prompt requested `docs/diagnostics/audits/...` (singular plural mismatch).
The actual on-disk convention from prior commits 1 and 2 is
`docs/diagnostics/_audits/` (leading underscore). This audit is written to
the existing `_audits/` folder to match convention; if the user prefers
non-underscore the file can be moved.

### F.3 `compute_density_change` reads `da[gid].z` window

`compute_density_change` writes `da[gid].w` only. In commit 2a the upstream
prerequisite — that `da[gid].z` has been freshly filled by
`compute_density_adv` for the **current** substep — is never satisfied (no
dispatches), so `da[gid].z` stays at its prior-frame value or the 0 written
by `density_alpha`. This is harmless until commit 2b correctly orders the
dispatches.

### F.4 `compute_density_adv` floor on `density_i`

Used `max(density_i, DFSPH_ALPHA_EPS)` when computing `V_i = particleMass /
density_i`. Upstream computes `V_i = mass / density` directly and assumes
non-degenerate density. The floor is a defensive GPU-port concession; if it
masks an upstream-spec divergence later, this is the spot to revisit.

### F.5 Helper-function trampoline

`writeComputeDensityChangeDescriptor` trampolines into
`writeComputeDensityAdvDescriptor` because their binding shapes are
byte-identical today. The wrapper exists so future divergence stays in one
place. This mirrors the `writeThicknessDescriptor → writeParticleSpriteDescriptor`
pattern already in `main.cpp:666-674`.

### F.6 `pressure_accel` `.w` slot is currently always 0

`compute_pressure_accel` writes `vec4(a_press, 0.0)` and `compute_aij_pj` /
`apply_velocity` read only `.xyz`. The `.w` slot is unused. If future work
needs a per-particle scalar (e.g., a deficiency flag or convergence
indicator) the slot is free.

---

**Status:** changes staged in working tree; no commit, no push, no build, no
binary run. The prompt's explicit verification checks all pass.
