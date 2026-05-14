---
title: Phase 11.5 DFSPH Architecture Probe
date: 2026-05-14
author: architect1
phase: 11.5
status: probe
scope: read-only
---

> Front-matter precedent: no precedent file found under `docs/` for a "diagnostics"/"audit"/"probe" doc. The closest stylistic precedents (`docs/retro/phase11.md`, `particle-fluids/sph-water/docs/load-bearing-decisions.md`) use prose headers, not YAML. Using the spec-provided front-matter block.

## Section A: Per-frame and per-substep pipeline trace

The substep loop is at `particle-fluids/sph-water/src/main.cpp:1908`.

Frame-scope work that runs **once per frame, outside the substep loop**:

- `pack_sort_uniform`, `pack_dfsph_uniform(substep_dt)`, `pack_render_view_uniform`, `pack_composite_uniform`, `pack_apply_emitter_uniform(substep_dt)` (`main.cpp:1888-1894`). These are host-mapped UBO writes (no GPU dispatch).
- After the substep loop closes: depth pass (graphics, `vkCmdDraw`), N bilateral_smooth compute dispatches (`main.cpp:2144-2167`), thickness graphics pass, composite graphics pass to swapchain. Discussed in Sections M/N/O.

**In-substep-loop dispatch order** (from `main.cpp:1908` through `main.cpp:2038`), with site, pipeline name, push constants, and barrier following each dispatch:

| # | Site | Pipeline | Push constants | Barrier following |
|---|------|----------|----------------|-------------------|
| 1 (cond) | `main.cpp:1913` | `pipe_apply_emitter` | none | `cs_barrier()` (`:1914`) |
| pre-2 | `main.cpp:1918-1919` | `vkCmdFillBuffer` zeroing `tier.cell_counts` + `tier.cell_counts_atomic` | — | TRANSFER->COMPUTE barrier (`:1920-1922`) |
| 2 | `main.cpp:1926` | `pipe_morton_code` | none | `cs_barrier()` (`:1928`) |
| 3 | `main.cpp:1931` | `pipe_cell_count` | none | `cs_barrier()` (`:1933`) |
| 4 | `main.cpp:1936` | `pipe_prefix_sum_local` | none | `cs_barrier()` (`:1938`) |
| 5 | `main.cpp:1943` | `pipe_prefix_sum_block` (mode=0 SCAN_ONLY) | `uint32_t mode = 0u` | `cs_barrier()` (`:1946`) |
| 6 | `main.cpp:1949` | `pipe_prefix_sum_block_l2` | none | `cs_barrier()` (`:1951`) |
| 7 | `main.cpp:1956` | `pipe_prefix_sum_block` (mode=1 ADDBACK_L2) | `uint32_t mode = 1u` | `cs_barrier()` (`:1959`) |
| 8 | `main.cpp:1962` | `pipe_prefix_sum_addback` | none | `cs_barrier()` (`:1964`) |
| 9 | `main.cpp:1967` | `pipe_scatter` | none | `cs_barrier()` (`:1969`) |
| 10 | `main.cpp:1974` | `pipe_density_alpha` | none | `cs_barrier()` (`:1976`) |
| pre-11 (cond) | `main.cpp:1981-1982` | `vkCmdFillBuffer` zero `pressure_a` and `pressure_b` | — | TRANSFER->COMPUTE barrier (`:1983-1985`) |
| 11.k (cond loop) | `main.cpp:1989` | `pipe_divergence_solve` × `max(rt.minIterDivergence, 1)` | none | `cs_barrier()` between each iter (`:1990`) |
| 12 (cond) | `main.cpp:1994` | `pipe_pressure_apply` (after divergence) | none | `cs_barrier()` (`:1996`) |
| 13 | `main.cpp:2003` | `pipe_integrate_forces` (mode=0 FORCES) | `uint32_t mode = 0u` | `cs_barrier()` (`:2006`) |
| pre-14 | `main.cpp:2009-2010` | `vkCmdFillBuffer` zero `pressure_a` and `pressure_b` | — | TRANSFER->COMPUTE barrier (`:2011-2013`) |
| 14.k (loop) | `main.cpp:2018` | `pipe_density_solve` × `max(rt.minIterDensity, 1)` | none | `cs_barrier()` between each iter (`:2019`) |
| 15 | `main.cpp:2024` | `pipe_pressure_apply` (after density) | none | `cs_barrier()` (`:2026`) |
| 16 | `main.cpp:2033` | `pipe_integrate_forces` (mode=1 POSITION) | `uint32_t mode = 1u` | `cs_barrier()` (`:2036`) |

`cs_barrier()` is defined at `main.cpp:1902-1906`:

```glsl:main.cpp:1902-1906
        auto cs_barrier = [&]() {
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT);
        };
```

This is a **single global VkMemoryBarrier2** (not per-buffer); see `common-cpp/include/gpusims/vk/frame.hpp:54-58`.

**Substep / neighbor-search nesting:**

```cpp:main.cpp:1908
        for (int sub = 0; !rt.paused && sub < rt.substeps && rt.particleCount > 0; ++sub) {
```

The loop runs from `main.cpp:1908` to the closing `}` at `main.cpp:2039`. **The entire spatial-hash pipeline (`vkCmdFillBuffer` → morton_code → cell_count → prefix-sum chain → scatter) is nested inside this loop.** So neighbor search and Morton sort are dispatched **per substep**, not per frame. Per-frame UBO packing at `main.cpp:1888-1894` runs once with `substep_dt = clamp(frame_dt / max(substeps, 1), DT_MIN, DT_MAX)` — `pack_dfsph_uniform` is called once and `dt` does not change between substeps.

## Section B: `density_solve` iteration loop

Quoted verbatim from `main.cpp:2008-2026`:

```cpp:main.cpp:2008-2026
            // Density-constancy Jacobi inner loop.
            vkCmdFillBuffer(cmd, tier.pressure_a.handle(), 0, VK_WHOLE_SIZE, 0u);
            vkCmdFillBuffer(cmd, tier.pressure_b.handle(), 0, VK_WHOLE_SIZE, 0u);
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT);
            {
                int iters = std::max(rt.minIterDensity, 1);
                for (int i = 0; i < iters; ++i) {
                    auto _ = profiler.scope(cmd, "density_solve");
                    pipe_density_solve.dispatch(cmd, ds_density_solve[i % 2], wg_particle, 1, 1);
                    cs_barrier();
                }
            }
            {
                auto _ = profiler.scope(cmd, "pressure_apply_density");
                pipe_pressure_apply.dispatch(cmd, ds_pressure_apply, wg_particle, 1, 1);
            }
            cs_barrier();
```

- **Iteration cap variable:** `iters = std::max(rt.minIterDensity, 1)` (`main.cpp:2015`). Declared as `int rt.minIterDensity = DFSPH_MIN_ITER_DENSITY` at `main.cpp:242`, with constant `DFSPH_MIN_ITER_DENSITY = 2` declared at `main.cpp:115`.
- **There is NO `maxIterDensity` actually used.** `Runtime::maxIterDensity = DFSPH_MAX_ITER_DENSITY` is declared at `main.cpp:240` but is never referenced by the solve loop. The constant `DFSPH_MAX_ITER_DENSITY = 100` is at `main.cpp:116`. The ImGui slider at `main.cpp:2252` exposes only `rt.minIterDensity` (range 1..16).
- **Tolerance / convergence threshold variable:** `rt.maxErrorDensityPercent` is declared at `main.cpp:244` (`= DFSPH_MAX_ERROR_DENSITY = 0.01f`, see `main.cpp:117`). **It is never referenced in any dispatch path; no convergence check exists.**
- **Break condition:** there is none — the loop runs exactly `max(rt.minIterDensity, 1)` iterations unconditionally. With the default `DFSPH_MIN_ITER_DENSITY = 2`, the loop runs 2 iterations every substep.
- **Density-error measurement:** **none.** No host readback of density-error, no GPU atomic accumulating error, no kernel-side error write. The shader (`density_solve.comp.glsl:111`) computes `s_i = min(1.0 - rho_adv / max(density0, 1e-7), 0.0)` per-particle, but this is consumed only inside that thread; nothing reduces it to a scalar.
- **Synchronization between iterations:** `cs_barrier()` at `main.cpp:2019` — a global VkMemoryBarrier2 with `srcStage=VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT`, `srcAccess=VK_ACCESS_2_SHADER_WRITE_BIT`, `dstStage=VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT`, `dstAccess=VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT`. Covers all buffers (global barrier).

## Section C: `divergence_solve` iteration loop

Quoted verbatim from `main.cpp:1978-1997`:

```cpp:main.cpp:1978-1997
            // Divergence-free Jacobi inner loop. Zero both pressure buffers
            // at the start; alternate ds_divergence_solve[i%2].
            if (rt.divSolverEnabled) {
                vkCmdFillBuffer(cmd, tier.pressure_a.handle(), 0, VK_WHOLE_SIZE, 0u);
                vkCmdFillBuffer(cmd, tier.pressure_b.handle(), 0, VK_WHOLE_SIZE, 0u);
                gv::memoryBarrier(cmd,
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT);
                int iters = std::max(rt.minIterDivergence, 1);
                for (int i = 0; i < iters; ++i) {
                    auto _ = profiler.scope(cmd, "divergence_solve");
                    pipe_divergence_solve.dispatch(cmd, ds_divergence_solve[i % 2], wg_particle, 1, 1);
                    cs_barrier();
                }
                {
                    auto _ = profiler.scope(cmd, "pressure_apply_div");
                    pipe_pressure_apply.dispatch(cmd, ds_pressure_apply, wg_particle, 1, 1);
                }
                cs_barrier();
            }
```

- **Iteration cap variable:** `iters = std::max(rt.minIterDivergence, 1)` (`main.cpp:1986`). Declared as `int rt.minIterDivergence = 1` at `main.cpp:243`. ImGui slider at `main.cpp:2253` exposes range 1..16.
- **`Runtime::maxIterDivergence`** declared at `main.cpp:241` (= `DFSPH_MAX_ITER_DIV = 100`, `main.cpp:118`). **Never referenced anywhere.**
- **Tolerance variable:** `rt.maxErrorDivPercent = DFSPH_MAX_ERROR_DIV = 0.1f` at `main.cpp:245` (constant at `main.cpp:119`). **Never referenced anywhere.**
- **Break condition:** none — fixed iteration count `max(rt.minIterDivergence, 1)` = 1 by default.
- **Divergence error measurement:** **none** (same posture as Section B).
- **Synchronization between iterations:** `cs_barrier()` at `main.cpp:1990`, same global barrier as Section B.
- The loop is gated by `if (rt.divSolverEnabled)` (`main.cpp:1980`), default `true` per `DFSPH_DIV_SOLVER_DEFAULT` at `main.cpp:120`.

## Section D: `kMaxPasses`

Grep results from the entire repo (Phase-11-relevant only; nothing in `particle-fluids/sph-water` or `docs/`):

```cpp:common/common-cpp/include/gpusims/gpu_profiler.hpp:47
    static constexpr std::uint32_t kMaxPasses = 256;
```

References (all in `common/common-cpp/`):

- `common-cpp/include/gpusims/gpu_profiler.hpp:47` — declaration, value `256`.
- `common-cpp/include/gpusims/gpu_profiler.hpp:106-108` — sizes `std::array<...> pass_names / cpu_begin / cpu_end`.
- `common-cpp/src/gpu_profiler.cpp:14` — `kQueriesPerFrame = 2 * GpuProfiler::kMaxPasses` (= 512 timestamp queries per frame).
- `common-cpp/src/gpu_profiler.cpp:57` — overflow guard `if (f.pass_count >= kMaxPasses)` with `logWarn(...)` and ignored pass.

**What it gates:** the per-frame GPU profiler's named-scope pass count (timestamps). It does **not** gate any DFSPH solver loop. It is unrelated to `rt.minIterDensity` / `rt.minIterDivergence`.

**Other iteration caps in scope:**

- `DFSPH_MIN_ITER_DENSITY = 2` (`main.cpp:115`)
- `DFSPH_MAX_ITER_DENSITY = 100` (`main.cpp:116`, dead)
- `DFSPH_MAX_ITER_DIV = 100` (`main.cpp:118`, dead)
- `DFSPH_MAX_ERROR_DENSITY = 0.01f` (`main.cpp:117`, dead)
- `DFSPH_MAX_ERROR_DIV = 0.1f` (`main.cpp:119`, dead)
- Runtime fields `rt.maxIterDensity`, `rt.maxIterDivergence`, `rt.maxErrorDensityPercent`, `rt.maxErrorDivPercent` (`main.cpp:240-245`, all dead)
- `rt.bilateralIterations` default `BILATERAL_ITERATIONS_DEFAULT = 4` (`main.cpp:137`), used at `main.cpp:2148`.
- Hardcoded `0..1023` glPointSize clamp in vert shader.

**Note for caller:** the probe sees no kernel-side per-iteration termination of the DFSPH solvers — they run exactly `minIter` times. The 256k@1-substep wall-clock oscillation 100ms→950ms reported in the task brief is **not** caused by an adaptive iter count (there isn't one).

## Section E: DFSPH compute shader inventory

Five compute shaders consume `tier.uniform_dfsph` (the canonical 112-byte UBO). They are:

1. `particle-fluids/sph-water/shaders/density_alpha.comp.glsl`
2. `particle-fluids/sph-water/shaders/divergence_solve.comp.glsl`
3. `particle-fluids/sph-water/shaders/density_solve.comp.glsl`
4. `particle-fluids/sph-water/shaders/integrate_forces.comp.glsl`
5. `particle-fluids/sph-water/shaders/pressure_apply.comp.glsl`

For Section L the full UBO blocks are tabulated. Here is each shader's bindings + `void main()` body.

### E.1 `density_alpha.comp.glsl`

Top-of-file (bindings/UBO):

```glsl:particle-fluids/sph-water/shaders/density_alpha.comp.glsl:10-53
layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict readonly buffer Particles {
    vec4 p[];
};
layout(set=0, binding=1, std430) restrict readonly buffer CellStarts {
    uint cell_starts[];
};
layout(set=0, binding=2, std430) restrict readonly buffer SortedIndex {
    uint sorted_index[];
};
layout(set=0, binding=3, std430) restrict writeonly buffer DensityAlpha {
    // .x = density, .y = α/ρ², .z = predicted_density (density_solve), .w = density_adv (divergence_solve)
    vec4 da[];
};
layout(set=0, binding=4, std140) uniform U {
    // Integer counts                            offset
    uint  particleCount;                       //   0
    uint  cellsPerAxisX;                       //   4
    uint  cellsPerAxisY;                       //   8
    uint  cellsPerAxisZ;                       //  12
    // SPH kernel constants
    float supportRadius;                       //  16
    float particleMass;                        //  20
    float density0;                            //  24
    float kernelNorm3D;                        //  28
    float gradKernelNorm3D;                    //  32
    // Time integration
    float dt;                                  //  36
    // Force coefficients
    float viscosity;                           //  40
    float cohesion;                            //  44
    float vorticityStrength;                   //  48
    // Solver tuning
    float jacobiRelax;                         //  52
    // Padding to align next vec4 to 16 B
    float _pad0;                               //  56
    float _pad1;                               //  60
    // Vec4 block
    vec4  gravity_pad;                         //  64  (.xyz=gravity, .w=mode)
    vec4  domainMin_cellSize;                  //  80  (.xyz=domainMin, .w=cellSize)
    vec4  domainMax_pad;                       //  96  (.xyz=domainMax)
};
// Total: 112 bytes
```

`void main()` body:

```glsl:particle-fluids/sph-water/shaders/density_alpha.comp.glsl:80-127
void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;

    vec3  pos_i       = p[gid * 8u + 0u].xyz;
    float density     = 0.0;
    vec3  sum_grad    = vec3(0.0);
    float sum_grad_sq = 0.0;

    vec3  rel_i  = (pos_i - domainMin_cellSize.xyz) / domainMin_cellSize.w;
    ivec3 cell_i = ivec3(clamp(rel_i, vec3(0.0),
        vec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ) - vec3(1.0)));

    for (int dz = -1; dz <= 1; ++dz)
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx) {
        ivec3 ncell = cell_i + ivec3(dx, dy, dz);
        if (any(lessThan(ncell, ivec3(0))) ||
            any(greaterThanEqual(ncell, ivec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ))))
            continue;
        uint nmorton = morton_encode_3d(uvec3(ncell));
        uint nstart  = cell_starts[nmorton];
        uint nend    = cell_starts[nmorton + 1u];
        for (uint k = nstart; k < nend; ++k) {
            uint j = sorted_index[k];
            vec3  pos_j = p[j * 8u + 0u].xyz;
            vec3  r_ij  = pos_i - pos_j;
            float r_mag = length(r_ij);
            float q     = r_mag / supportRadius;
            if (q >= 1.0) continue;

            density += particleMass * kernel_W(q, kernelNorm3D);

            if (j != gid && r_mag > 1e-7) {
                vec3 grad_W    = kernel_gradW(r_ij, r_mag, q, gradKernelNorm3D);
                vec3 grad_term = particleMass * grad_W;
                sum_grad      += grad_term;
                sum_grad_sq   += dot(grad_term, grad_term);
            }
        }
    }

    float denom        = max(dot(sum_grad, sum_grad) + sum_grad_sq, DFSPH_ALPHA_EPS);
    float alpha        = 1.0 / denom;
    float alpha_stored = alpha / max(density * density, DFSPH_ALPHA_EPS);

    da[gid] = vec4(density, alpha_stored, 0.0, 0.0);
}
```

Reads: `p[]` (Particles), `cell_starts[]`, `sorted_index[]`, UBO `U`.
Writes: `da[gid]` (DensityAlpha) — note **`.z` and `.w` are written 0.0 here**, contradicting the `_struct_layouts.txt` claim that those slots hold predicted_density / density_advect.

### E.2 `divergence_solve.comp.glsl`

Top-of-file (bindings/UBO) — `particle-fluids/sph-water/shaders/divergence_solve.comp.glsl:14-50`:

```glsl:particle-fluids/sph-water/shaders/divergence_solve.comp.glsl:14-50
layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict buffer Particles { vec4 p[]; };
layout(set=0, binding=1, std430) restrict readonly buffer DensityAlpha { vec4 da[]; };
layout(set=0, binding=2, std430) restrict readonly buffer CellStarts { uint cell_starts[]; };
layout(set=0, binding=3, std430) restrict readonly buffer SortedIndex { uint sorted_index[]; };
layout(set=0, binding=4, std430) restrict readonly buffer PressureRead { float p_read[]; };
layout(set=0, binding=5, std430) restrict writeonly buffer PressureWrite { float p_write[]; };
layout(set=0, binding=6, std140) uniform U {
    // Integer counts                            offset
    uint  particleCount;                       //   0
    uint  cellsPerAxisX;                       //   4
    uint  cellsPerAxisY;                       //   8
    uint  cellsPerAxisZ;                       //  12
    // SPH kernel constants
    float supportRadius;                       //  16
    float particleMass;                        //  20
    float density0;                            //  24
    float kernelNorm3D;                        //  28
    float gradKernelNorm3D;                    //  32
    // Time integration
    float dt;                                  //  36
    // Force coefficients
    float viscosity;                           //  40
    float cohesion;                            //  44
    float vorticityStrength;                   //  48
    // Solver tuning
    float jacobiRelax;                         //  52
    // Padding to align next vec4 to 16 B
    float _pad0;                               //  56
    float _pad1;                               //  60
    // Vec4 block
    vec4  gravity_pad;                         //  64  (.xyz=gravity, .w=mode)
    vec4  domainMin_cellSize;                  //  80  (.xyz=domainMin, .w=cellSize)
    vec4  domainMax_pad;                       //  96  (.xyz=domainMax)
};
// Total: 112 bytes
```

`void main()` body — `particle-fluids/sph-water/shaders/divergence_solve.comp.glsl:69-143`:

```glsl:particle-fluids/sph-water/shaders/divergence_solve.comp.glsl:69-143
void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;

    vec3  pos_i     = p[gid * 8u + 0u].xyz;
    vec3  vel_i     = p[gid * 8u + 1u].xyz;
    float density_i = da[gid].x;
    float factor_i  = da[gid].y;
    float p_v_i     = p_read[gid];

    vec3  rel_i  = (pos_i - domainMin_cellSize.xyz) / domainMin_cellSize.w;
    ivec3 cell_i = ivec3(clamp(rel_i, vec3(0.0),
        vec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ) - vec3(1.0)));

    // Pass 1: ρ̇_i = Σ_j m_j (v_i − v_j) · ∇W_ij  →  s_i = -ρ̇_i (clamped ≥ 0).
    float rho_dot = 0.0;
    for (int dz = -1; dz <= 1; ++dz)
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx) {
        ivec3 ncell = cell_i + ivec3(dx, dy, dz);
        if (any(lessThan(ncell, ivec3(0))) ||
            any(greaterThanEqual(ncell, ivec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ))))
            continue;
        uint nmorton = morton_encode_3d(uvec3(ncell));
        uint nstart  = cell_starts[nmorton];
        uint nend    = cell_starts[nmorton + 1u];
        for (uint k = nstart; k < nend; ++k) {
            uint j = sorted_index[k];
            if (j == gid) continue;
            vec3  pos_j = p[j * 8u + 0u].xyz;
            vec3  vel_j = p[j * 8u + 1u].xyz;
            vec3  r_ij  = pos_i - pos_j;
            float r_mag = length(r_ij);
            float q     = r_mag / supportRadius;
            if (q >= 1.0 || r_mag < 1e-7) continue;
            vec3 grad_W = kernel_gradW(r_ij, r_mag, q, gradKernelNorm3D);
            rho_dot += particleMass * dot(vel_i - vel_j, grad_W);
        }
    }
    float s_i = max(-rho_dot, 0.0);

    // Pass 2: aij_pj_sum = Σ_{j≠i} a_ij · p̃_v_j;  Jacobi update with relax = 0.5.
    // SKELETON COUPLING — replace with upstream-exact form per Callout 1.
    float aij_pj_sum = 0.0;
    for (int dz = -1; dz <= 1; ++dz)
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx) {
        ivec3 ncell = cell_i + ivec3(dx, dy, dz);
        if (any(lessThan(ncell, ivec3(0))) ||
            any(greaterThanEqual(ncell, ivec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ))))
            continue;
        uint nmorton = morton_encode_3d(uvec3(ncell));
        uint nstart  = cell_starts[nmorton];
        uint nend    = cell_starts[nmorton + 1u];
        for (uint k = nstart; k < nend; ++k) {
            uint j = sorted_index[k];
            if (j == gid) continue;
            vec3  pos_j     = p[j * 8u + 0u].xyz;
            float p_v_j     = p_read[j];
            vec3  r_ij      = pos_i - pos_j;
            float r_mag     = length(r_ij);
            float q         = r_mag / supportRadius;
            if (q >= 1.0 || r_mag < 1e-7) continue;
            vec3 grad_W = kernel_gradW(r_ij, r_mag, q, gradKernelNorm3D);
            // Placeholder coupling: m_j · |∇W|² scaled by h. Replace with the
            // symmetric (factor_i + factor_j)·m_j·∇W form from upstream.
            float coupling = particleMass * dot(grad_W, grad_W);
            aij_pj_sum += supportRadius * coupling * p_v_j;
        }
    }

    float factor_with_h_inv = factor_i / max(supportRadius, 1e-7);
    float new_p_v = max(p_v_i - jacobiRelax * (s_i - aij_pj_sum) * factor_with_h_inv, 0.0);
    p_write[gid]  = new_p_v;
}
```

Reads: `p[]` (Particles), `da[]`, `cell_starts[]`, `sorted_index[]`, `p_read[]`, UBO `U`.
Writes: `p_write[gid]`. **Note the SSBO descriptor on Particles is plain `buffer` (not `readonly`)** but the body only reads it.

### E.3 `density_solve.comp.glsl`

Top-of-file — `particle-fluids/sph-water/shaders/density_solve.comp.glsl:14-52`:

```glsl:particle-fluids/sph-water/shaders/density_solve.comp.glsl:14-52
layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict buffer Particles { vec4 p[]; };
layout(set=0, binding=1, std430) restrict readonly buffer DensityAlpha { vec4 da[]; };
layout(set=0, binding=2, std430) restrict readonly buffer CellStarts { uint cell_starts[]; };
layout(set=0, binding=3, std430) restrict readonly buffer SortedIndex { uint sorted_index[]; };
layout(set=0, binding=4, std430) restrict readonly buffer PressureRead { float p_read[]; };
layout(set=0, binding=5, std430) restrict writeonly buffer PressureWrite { float p_write[]; };
layout(set=0, binding=6, std140) uniform U {
    // Integer counts                            offset
    uint  particleCount;                       //   0
    uint  cellsPerAxisX;                       //   4
    uint  cellsPerAxisY;                       //   8
    uint  cellsPerAxisZ;                       //  12
    // SPH kernel constants
    float supportRadius;                       //  16
    float particleMass;                        //  20
    float density0;                            //  24
    float kernelNorm3D;                        //  28
    float gradKernelNorm3D;                    //  32
    // Time integration
    float dt;                                  //  36
    // Force coefficients
    float viscosity;                           //  40
    float cohesion;                            //  44
    float vorticityStrength;                   //  48
    // Solver tuning
    float jacobiRelax;                         //  52
    // Padding to align next vec4 to 16 B
    float _pad0;                               //  56
    float _pad1;                               //  60
    // Vec4 block
    vec4  gravity_pad;                         //  64  (.xyz=gravity, .w=mode)
    vec4  domainMin_cellSize;                  //  80  (.xyz=domainMin, .w=cellSize)
    vec4  domainMax_pad;                       //  96  (.xyz=domainMax)
};
// Total: 112 bytes
```

`void main()` body — `particle-fluids/sph-water/shaders/density_solve.comp.glsl:71-145`:

```glsl:particle-fluids/sph-water/shaders/density_solve.comp.glsl:71-145
void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;

    vec3  pos_i     = p[gid * 8u + 0u].xyz;
    vec3  vel_i     = p[gid * 8u + 1u].xyz;
    float density_i = da[gid].x;
    float factor_i  = da[gid].y;
    float p_i       = p_read[gid];

    vec3  rel_i  = (pos_i - domainMin_cellSize.xyz) / domainMin_cellSize.w;
    ivec3 cell_i = ivec3(clamp(rel_i, vec3(0.0),
        vec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ) - vec3(1.0)));

    // Pass 1: rho_adv = density_i + dt · Σ m (v_i − v_j) · ∇W;  s_i = 1 - rho_adv/ρ₀.
    float rho_dot = 0.0;
    for (int dz = -1; dz <= 1; ++dz)
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx) {
        ivec3 ncell = cell_i + ivec3(dx, dy, dz);
        if (any(lessThan(ncell, ivec3(0))) ||
            any(greaterThanEqual(ncell, ivec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ))))
            continue;
        uint nmorton = morton_encode_3d(uvec3(ncell));
        uint nstart  = cell_starts[nmorton];
        uint nend    = cell_starts[nmorton + 1u];
        for (uint k = nstart; k < nend; ++k) {
            uint j = sorted_index[k];
            if (j == gid) continue;
            vec3  pos_j = p[j * 8u + 0u].xyz;
            vec3  vel_j = p[j * 8u + 1u].xyz;
            vec3  r_ij  = pos_i - pos_j;
            float r_mag = length(r_ij);
            float q     = r_mag / supportRadius;
            if (q >= 1.0 || r_mag < 1e-7) continue;
            vec3 grad_W = kernel_gradW(r_ij, r_mag, q, gradKernelNorm3D);
            rho_dot += particleMass * dot(vel_i - vel_j, grad_W);
        }
    }
    float rho_adv = density_i + dt * rho_dot;
    float s_i     = min(1.0 - rho_adv / max(density0, 1e-7), 0.0);

    // Pass 2: aij_pj_sum with h² scaling; factor scaled by 1/h².
    float aij_pj_sum = 0.0;
    for (int dz = -1; dz <= 1; ++dz)
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx) {
        ivec3 ncell = cell_i + ivec3(dx, dy, dz);
        if (any(lessThan(ncell, ivec3(0))) ||
            any(greaterThanEqual(ncell, ivec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ))))
            continue;
        uint nmorton = morton_encode_3d(uvec3(ncell));
        uint nstart  = cell_starts[nmorton];
        uint nend    = cell_starts[nmorton + 1u];
        for (uint k = nstart; k < nend; ++k) {
            uint j = sorted_index[k];
            if (j == gid) continue;
            vec3  pos_j = p[j * 8u + 0u].xyz;
            float p_j   = p_read[j];
            vec3  r_ij  = pos_i - pos_j;
            float r_mag = length(r_ij);
            float q     = r_mag / supportRadius;
            if (q >= 1.0 || r_mag < 1e-7) continue;
            vec3 grad_W = kernel_gradW(r_ij, r_mag, q, gradKernelNorm3D);
            // Placeholder coupling — same caveat as divergence_solve.
            float coupling = particleMass * dot(grad_W, grad_W);
            aij_pj_sum += supportRadius * supportRadius * coupling * p_j;
        }
    }

    float h2 = max(supportRadius * supportRadius, 1e-7);
    float factor_with_h2_inv = factor_i / h2;
    float new_p = max(p_i - jacobiRelax * (s_i - aij_pj_sum) * factor_with_h2_inv, 0.0);
    p_write[gid] = new_p;
}
```

Reads: `p[]`, `da[]`, `cell_starts[]`, `sorted_index[]`, `p_read[]`, UBO `U`.
Writes: `p_write[gid]`.

### E.4 `integrate_forces.comp.glsl`

Top-of-file — `particle-fluids/sph-water/shaders/integrate_forces.comp.glsl:9-47`:

```glsl:particle-fluids/sph-water/shaders/integrate_forces.comp.glsl:9-47
layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict buffer Particles { vec4 p[]; };
layout(set=0, binding=1, std430) restrict readonly buffer DensityAlpha { vec4 da[]; };
layout(set=0, binding=2, std430) restrict readonly buffer CellStarts { uint cell_starts[]; };
layout(set=0, binding=3, std430) restrict readonly buffer SortedIndex { uint sorted_index[]; };
layout(set=0, binding=4, std140) uniform U {
    // Integer counts                            offset
    uint  particleCount;                       //   0
    uint  cellsPerAxisX;                       //   4
    uint  cellsPerAxisY;                       //   8
    uint  cellsPerAxisZ;                       //  12
    // SPH kernel constants
    float supportRadius;                       //  16
    float particleMass;                        //  20
    float density0;                            //  24
    float kernelNorm3D;                        //  28
    float gradKernelNorm3D;                    //  32
    // Time integration
    float dt;                                  //  36
    // Force coefficients
    float viscosity;                           //  40
    float cohesion;                            //  44
    float vorticityStrength;                   //  48
    // Solver tuning
    float jacobiRelax;                         //  52
    // Padding to align next vec4 to 16 B
    float _pad0;                               //  56
    float _pad1;                               //  60
    // Vec4 block
    vec4  gravity_pad;                         //  64  (.xyz=gravity, .w=mode)
    vec4  domainMin_cellSize;                  //  80  (.xyz=domainMin, .w=cellSize)
    vec4  domainMax_pad;                       //  96  (.xyz=domainMax)
};
// Total: 112 bytes

layout(push_constant) uniform PC {
    uint mode;
} pc;
```

`void main()` body — `particle-fluids/sph-water/shaders/integrate_forces.comp.glsl:65-129`:

```glsl:particle-fluids/sph-water/shaders/integrate_forces.comp.glsl:65-129
void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;

    vec3 pos_i = p[gid * 8u + 0u].xyz;
    vec3 vel_i = p[gid * 8u + 1u].xyz;
    int  mode  = int(pc.mode);

    if (mode == 1) {
        // POSITION_ONLY mode.
        vec3 pos_new = pos_i + dt * vel_i;
        vec3 vel_new = vel_i;
        vec3 dmin    = domainMin_cellSize.xyz;
        vec3 dmax    = domainMax_pad.xyz;
        for (int ax = 0; ax < 3; ++ax) {
            if (pos_new[ax] < dmin[ax]) { pos_new[ax] = dmin[ax]; vel_new[ax] = max(vel_new[ax], 0.0); }
            if (pos_new[ax] > dmax[ax]) { pos_new[ax] = dmax[ax]; vel_new[ax] = min(vel_new[ax], 0.0); }
        }
        p[gid * 8u + 0u].xyz = pos_new;
        p[gid * 8u + 1u].xyz = vel_new;
        return;
    }

    // FORCES_ONLY mode.
    vec3 a_visc     = vec3(0.0);
    vec3 a_cohesion = vec3(0.0);

    vec3  rel_i  = (pos_i - domainMin_cellSize.xyz) / domainMin_cellSize.w;
    ivec3 cell_i = ivec3(clamp(rel_i, vec3(0.0),
        vec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ) - vec3(1.0)));

    for (int dz = -1; dz <= 1; ++dz)
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx) {
        ivec3 ncell = cell_i + ivec3(dx, dy, dz);
        if (any(lessThan(ncell, ivec3(0))) ||
            any(greaterThanEqual(ncell, ivec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ))))
            continue;
        uint nmorton = morton_encode_3d(uvec3(ncell));
        uint nstart  = cell_starts[nmorton];
        uint nend    = cell_starts[nmorton + 1u];
        for (uint k = nstart; k < nend; ++k) {
            uint j = sorted_index[k];
            if (j == gid) continue;
            vec3  pos_j     = p[j * 8u + 0u].xyz;
            vec3  vel_j     = p[j * 8u + 1u].xyz;
            float density_j = max(da[j].x, 1e-3);
            vec3  r_ij      = pos_i - pos_j;
            float r_mag     = length(r_ij);
            float q         = r_mag / supportRadius;
            if (q >= 1.0 || r_mag < 1e-7) continue;

            float W = kernel_W(q, kernelNorm3D);
            a_visc     += viscosity * (particleMass / density_j) * (vel_j - vel_i) * W;
            a_cohesion -= cohesion  * particleMass * W * (r_ij / r_mag);
        }
    }

    // Vorticity confinement: simple curl-approximation skeleton (v1 placeholder).
    // Full Stam-style banked v1.1; for now leave as no-op weighted by vorticityStrength.
    vec3 a_vorticity = vec3(0.0) * vorticityStrength;

    vec3 a_total = gravity_pad.xyz + a_visc + a_cohesion + a_vorticity;
    p[gid * 8u + 1u].xyz = vel_i + dt * a_total;
}
```

Reads: `p[]`, `da[]`, `cell_starts[]`, `sorted_index[]`, UBO `U`, push constant `mode`.
Writes: `p[gid*8 + 1].xyz` (velocity, FORCES); `p[gid*8 + 0].xyz` and `p[gid*8 + 1].xyz` (POSITION mode).

### E.5 `pressure_apply.comp.glsl`

Top-of-file — `particle-fluids/sph-water/shaders/pressure_apply.comp.glsl:12-47`:

```glsl:particle-fluids/sph-water/shaders/pressure_apply.comp.glsl:12-47
layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict buffer Particles { vec4 p[]; };
layout(set=0, binding=1, std430) restrict readonly buffer DensityAlpha { vec4 da[]; };
layout(set=0, binding=2, std430) restrict readonly buffer PressureRead { float p_read[]; };
layout(set=0, binding=3, std430) restrict readonly buffer CellStarts { uint cell_starts[]; };
layout(set=0, binding=4, std430) restrict readonly buffer SortedIndex { uint sorted_index[]; };
layout(set=0, binding=5, std140) uniform U {
    // Integer counts                            offset
    uint  particleCount;                       //   0
    uint  cellsPerAxisX;                       //   4
    uint  cellsPerAxisY;                       //   8
    uint  cellsPerAxisZ;                       //  12
    // SPH kernel constants
    float supportRadius;                       //  16
    float particleMass;                        //  20
    float density0;                            //  24
    float kernelNorm3D;                        //  28
    float gradKernelNorm3D;                    //  32
    // Time integration
    float dt;                                  //  36
    // Force coefficients
    float viscosity;                           //  40
    float cohesion;                            //  44
    float vorticityStrength;                   //  48
    // Solver tuning
    float jacobiRelax;                         //  52
    // Padding to align next vec4 to 16 B
    float _pad0;                               //  56
    float _pad1;                               //  60
    // Vec4 block
    vec4  gravity_pad;                         //  64  (.xyz=gravity, .w=mode)
    vec4  domainMin_cellSize;                  //  80  (.xyz=domainMin, .w=cellSize)
    vec4  domainMax_pad;                       //  96  (.xyz=domainMax)
};
// Total: 112 bytes
```

`void main()` body — `particle-fluids/sph-water/shaders/pressure_apply.comp.glsl:66-109`:

```glsl:particle-fluids/sph-water/shaders/pressure_apply.comp.glsl:66-109
void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;

    vec3  pos_i     = p[gid * 8u + 0u].xyz;
    vec3  vel_i     = p[gid * 8u + 1u].xyz;
    float density_i = da[gid].x;
    float p_i       = p_read[gid];
    float rho_i2    = max(density_i * density_i, 1e-7);

    vec3  rel_i  = (pos_i - domainMin_cellSize.xyz) / domainMin_cellSize.w;
    ivec3 cell_i = ivec3(clamp(rel_i, vec3(0.0),
        vec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ) - vec3(1.0)));

    vec3 a_press = vec3(0.0);
    for (int dz = -1; dz <= 1; ++dz)
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx) {
        ivec3 ncell = cell_i + ivec3(dx, dy, dz);
        if (any(lessThan(ncell, ivec3(0))) ||
            any(greaterThanEqual(ncell, ivec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ))))
            continue;
        uint nmorton = morton_encode_3d(uvec3(ncell));
        uint nstart  = cell_starts[nmorton];
        uint nend    = cell_starts[nmorton + 1u];
        for (uint k = nstart; k < nend; ++k) {
            uint j = sorted_index[k];
            if (j == gid) continue;
            vec3  pos_j     = p[j * 8u + 0u].xyz;
            float density_j = da[j].x;
            float p_j       = p_read[j];
            vec3  r_ij      = pos_i - pos_j;
            float r_mag     = length(r_ij);
            float q         = r_mag / supportRadius;
            if (q >= 1.0 || r_mag < 1e-7) continue;
            vec3  grad_W = kernel_gradW(r_ij, r_mag, q, gradKernelNorm3D);
            float rho_j2 = max(density_j * density_j, 1e-7);
            a_press -= particleMass * (p_i / rho_i2 + p_j / rho_j2) * grad_W;
        }
    }

    vec3 vel_new = vel_i + dt * a_press;
    p[gid * 8u + 1u].xyz = vel_new;
}
```

Reads: `p[]`, `da[]`, `p_read[]`, `cell_starts[]`, `sorted_index[]`, UBO `U`.
Writes: `p[gid*8 + 1].xyz` (velocity).

## Section F: SPH kernel function W and ∇W

The kernel is **inlined per-shader, not in a shared include**. Three shaders contain `kernel_W`: `density_alpha.comp.glsl`, `integrate_forces.comp.glsl`. Four shaders contain `kernel_gradW`: `density_alpha`, `density_solve`, `divergence_solve`, `pressure_apply`.

Verbatim copy from `density_alpha.comp.glsl:68-78`:

```glsl:particle-fluids/sph-water/shaders/density_alpha.comp.glsl:68-78
float kernel_W(float q, float kernel_norm) {
    if (q < 0.5)      return kernel_norm * (6.0*q*q*q - 6.0*q*q + 1.0);
    else if (q < 1.0) { float omq = 1.0 - q; return kernel_norm * 2.0 * omq*omq*omq; }
    else              return 0.0;
}
vec3 kernel_gradW(vec3 r_ij, float r_mag, float q, float grad_kernel_norm) {
    float poly = 0.0;
    if (q < 0.5)      poly = 18.0*q*q - 12.0*q;
    else if (q < 1.0) { float omq = 1.0 - q; poly = -6.0 * omq * omq; }
    return (grad_kernel_norm * poly / r_mag) * r_ij;
}
```

- This is the **SPlisHSPlasH "CubicKernel" form** — the M4 cubic spline B-spline expressed in `q = r/h`.
- **Normalization constants come from host code** at `main.cpp:1343-1350`:

```cpp:main.cpp:1343-1350
    auto kernel_norm_3d_value = [&]() {
        float h = rt.supportRadius;
        return 8.0f / (float(M_PI) * h * h * h);
    };
    auto grad_kernel_norm_3d_value = [&]() {
        float h = rt.supportRadius;
        return 48.0f / (float(M_PI) * h * h * h * h);
    };
```

- **`8/(π h³)`** is the **3D normalization** of the SPlisHSPlasH M4 cubic-spline kernel (whose support is `r ≤ h`, parameterized by `q = r/h`). The 2D constant would be `40/(7π h²)`. So **3D, correct for 3D usage**.
- **Support-radius convention:** `h` IS the **support radius itself** (not `2h`). The kernel zeros at `q ≥ 1.0`, i.e., `r ≥ h`. This matches the SPlisHSPlasH/Bender-Koschier convention; both the shader's `if (q >= 1.0) continue;` neighbor reject and `kernel_W`'s `return 0.0` for `q ≥ 1.0` confirm it.
- **`h = 4 × particleRadius`** per `main.cpp:130, 252` and `load-bearing-decisions.md`.
- **Self-contribution W(0, h)** = `kernel_norm * (6·0 − 6·0 + 1.0)` = `kernel_norm` = `8/(π h³)`. At `h = 0.04`, `W(0) ≈ 8/(π·6.4e-5) ≈ 39788`.

**Divergence check across shaders:** I compared the four copies of `kernel_gradW` and the two copies of `kernel_W`. All copies are byte-identical except for surrounding whitespace; no divergence detected. Each shader carries an identical literal copy of:

```glsl
vec3 kernel_gradW(vec3 r_ij, float r_mag, float q, float grad_kernel_norm) {
    float poly = 0.0;
    if (q < 0.5)      poly = 18.0*q*q - 12.0*q;
    else if (q < 1.0) { float omq = 1.0 - q; poly = -6.0 * omq * omq; }
    return (grad_kernel_norm * poly / r_mag) * r_ij;
}
```

(verified in `density_alpha.comp.glsl:73-78`, `density_solve.comp.glsl:64-69`, `divergence_solve.comp.glsl:62-67`, `pressure_apply.comp.glsl:59-64`).

The duplication is a maintenance risk but currently not a behavioural risk.

## Section G: Spatial hash / Morton sort

### G.1 Cell size

```cpp:main.cpp:129-131
constexpr float PARTICLE_RADIUS_DEFAULT     = 0.01f;
constexpr float SUPPORT_RADIUS_RATIO         = 4.0f;
constexpr float CELL_SIZE_RATIO_TO_SUPPORT   = 2.0f;
```

```cpp:main.cpp:251-253
    float     particleRadius     = PARTICLE_RADIUS_DEFAULT;
    float     supportRadius      = PARTICLE_RADIUS_DEFAULT * SUPPORT_RADIUS_RATIO;
    float     cellSize           = PARTICLE_RADIUS_DEFAULT * SUPPORT_RADIUS_RATIO * CELL_SIZE_RATIO_TO_SUPPORT;
```

**`cellSize = supportRadius * 2.0`.** With defaults: `supportRadius = 0.04`, `cellSize = 0.08`. So cell size is **2× support radius**. (Section P will flag this — the standard convention for fixed-radius neighbour search is `cellSize == supportRadius` with a 3×3×3 = 27-cell traversal that covers the full kernel support. With `cellSize = 2·h`, a particle's `h`-ball fits inside its own cell and the 27-cell traversal is wildly over-conservative; alternatively, only a 2×2×2 = 8-cell stencil is strictly necessary.)

Per-axis cell count is rounded to next power of two and capped at `MAX_CELLS_PER_AXIS = 1 << MORTON_BITS_PER_AXIS = 1024`:

```cpp:main.cpp:1325-1336
    auto compute_cells_per_axis = [&](glm::uvec3& outAxes, std::uint32_t& outMax) {
        glm::vec3 ext = rt.domainMax - rt.domainMin;
        glm::uvec3 axes;
        axes.x = std::max(1u, std::uint32_t(std::ceil(ext.x / rt.cellSize)));
        axes.y = std::max(1u, std::uint32_t(std::ceil(ext.y / rt.cellSize)));
        axes.z = std::max(1u, std::uint32_t(std::ceil(ext.z / rt.cellSize)));
        axes.x = next_pow2(axes.x);
        axes.y = next_pow2(axes.y);
        axes.z = next_pow2(axes.z);
        outAxes = axes;
        outMax  = std::max({axes.x, axes.y, axes.z});
    };
```

### G.2 Hash function

Morton/Z-order (3D bit interleave, 10 bits per axis, 30 bits total). Verbatim from `morton_code.comp.glsl:24-36`:

```glsl:particle-fluids/sph-water/shaders/morton_code.comp.glsl:24-36
uint expand_bits_10(uint v) {
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

uint morton_encode_3d(uvec3 c) {
    return (expand_bits_10(c.x) << 2)
         | (expand_bits_10(c.y) << 1)
         |  expand_bits_10(c.z);
}
```

The full `morton_code.comp.glsl:38-51`:

```glsl:particle-fluids/sph-water/shaders/morton_code.comp.glsl:38-51
void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;

    vec3  pos        = p[gid * 8u + 0u].xyz;
    vec3  domain_min = domainMin_pad.xyz;
    float cell_size  = domainMin_pad.w;
    uvec3 cells_axis = uvec3(cellsXYZ_pad.xyz);

    vec3  rel  = (pos - domain_min) / cell_size;
    uvec3 cell = uvec3(clamp(rel, vec3(0.0), vec3(cells_axis) - vec3(1.0)));

    codes[gid] = morton_encode_3d(cell);
}
```

### G.3 Sort algorithm

**Counting sort** (not bitonic, not radix-byte, not VkRadixSort). Pipeline: `cell_count` (`scatter.comp.glsl`-pattern atomic count) → two-level Blelloch exclusive prefix scan → `scatter` claims output slot via atomicAdd on a separate counter.

Dispatch sites at `main.cpp:1924-1968` (Section A table rows 3-9). Counting-sort kernel quoted at Section E (cell_count, scatter, prefix_sum_*).

### G.4 Cell start / end array

`cell_starts[m]` = exclusive prefix sum of `cell_counts[]`. Constructed by `prefix_sum_local` → `prefix_sum_block` (mode 0) → `prefix_sum_block_l2` (only if `num_blocks > WG_SIZE`) → `prefix_sum_block` (mode 1, addback L2) → `prefix_sum_addback`. Final `cell_starts` write at `prefix_sum_addback.comp.glsl:23-28`:

```glsl:particle-fluids/sph-water/shaders/prefix_sum_addback.comp.glsl:23-28
void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= totalCells) return;
    uint block = gl_WorkGroupID.x;
    cell_starts[gid] = per_block[gid] + block_prefixes[block];
}
```

### G.5 Neighbor traversal pattern

**Every solver kernel visits 27 cells** (3×3×3). Identical loop body in all five DFSPH shaders; quoted from `density_alpha.comp.glsl:93-99`:

```glsl:particle-fluids/sph-water/shaders/density_alpha.comp.glsl:93-99
    for (int dz = -1; dz <= 1; ++dz)
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx) {
        ivec3 ncell = cell_i + ivec3(dx, dy, dz);
        if (any(lessThan(ncell, ivec3(0))) ||
            any(greaterThanEqual(ncell, ivec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ))))
            continue;
```

Inner neighbor walk uses `cell_starts[nmorton]` and `cell_starts[nmorton + 1u]` (`density_alpha.comp.glsl:101-102`). Note that `nmorton + 1u` lookup requires `cell_starts` be sized `totalCells + 1`, which is what `_struct_layouts.txt` claims at line 88 — but `main.cpp:881-882` allocates `r.cell_starts = max_cells * 4`, i.e., **NOT** sized for the +1 sentinel. This is a finding (Section P).

### G.6 Self-exclusion

All solver shaders except `density_alpha` explicitly skip self with `if (j == gid) continue;` near the top of the inner neighbor loop (e.g., `density_solve.comp.glsl:99`, `divergence_solve.comp.glsl:97`, `integrate_forces.comp.glsl:108`, `pressure_apply.comp.glsl:93`).

`density_alpha.comp.glsl:104` does NOT skip self for the density sum (this is correct — `density_i` must include `W(0)`), but it does skip self for the alpha-gradient terms inside the conditional at `density_alpha.comp.glsl:113`:

```glsl:particle-fluids/sph-water/shaders/density_alpha.comp.glsl:113
            if (j != gid && r_mag > 1e-7) {
```

## Section H: Mass, rest density, particle radius — consistency check

Host-side constants (`main.cpp`):

```cpp:main.cpp:127-131
constexpr float DENSITY_0     = 1000.0f;
constexpr float GRAVITY_Y     = -9.81f;
constexpr float PARTICLE_RADIUS_DEFAULT     = 0.01f;
constexpr float SUPPORT_RADIUS_RATIO         = 4.0f;
constexpr float CELL_SIZE_RATIO_TO_SUPPORT   = 2.0f;
```

Host-side derived:

```cpp:main.cpp:251-253
    float     particleRadius     = PARTICLE_RADIUS_DEFAULT;
    float     supportRadius      = PARTICLE_RADIUS_DEFAULT * SUPPORT_RADIUS_RATIO;
    float     cellSize           = PARTICLE_RADIUS_DEFAULT * SUPPORT_RADIUS_RATIO * CELL_SIZE_RATIO_TO_SUPPORT;
```

Host mass formula (`main.cpp:1339-1342`):

```cpp:main.cpp:1339-1342
    auto particle_mass = [&]() {
        float spacing = 2.0f * rt.particleRadius;
        return DENSITY_0 * spacing * spacing * spacing;
    };
```

- **`mass = ρ₀ · (2r)³`** where `r = particleRadius`. With defaults: `2r = 0.02`, `mass = 1000 · 8e-6 = 0.008` kg per particle.
- **`supportRadius = 4 · particleRadius`** (matches `load-bearing-decisions.md` § 2.2).
- **`cellSize = 2 · supportRadius`** — discussed in Section G.1 and flagged in Section P.

Sources:

| Quantity | Source | Value |
|---|---|---|
| `density0` (ρ₀) | hardcoded `DENSITY_0` constant | 1000.0 |
| `particleMass` | derived from `2·radius` and `density0` | 0.008 (default) |
| `particleRadius` | hardcoded `PARTICLE_RADIUS_DEFAULT` (no slider) | 0.01 |
| `supportRadius` | derived `radius · 4` | 0.04 |
| `cellSize` | derived `radius · 4 · 2` | 0.08 |
| `gravity` | hardcoded literal `-9.81f` at `pack_dfsph_uniform` `main.cpp:1419-1422`; `GRAVITY_Y` constant is **unused** | -9.81 |

**Host vs device consistency:** all device-side values flow through `pack_dfsph_uniform` (`main.cpp:1380-1431`), which writes them into `tier.uniform_dfsph`. The shaders read them as UBO fields (`supportRadius`, `particleMass`, `density0`). **No GLSL-side hardcoded copies exist** for these quantities — every solver shader treats them as UBO inputs. So host/device cannot disagree.

**However** the SPlisHSPlasH convention for the DFSPH rest-density particle (the "consistent" mass when initial spacing is `2r`) actually uses the **rest-density volume estimate** with the cubic-spline self-density sum, not a naive `ρ₀ · (2r)³`. This is a **minor finding** — discussed in Section P.

## Section I: Boundary conditions and dam-break initialization

**Boundary representation:** there are **no boundary particles, no SDF, no ghost particles**. The only wall handling is a hard AABB position clamp inside `integrate_forces.comp.glsl` (POSITION mode, lines 79-82, quoted in Section E.4):

```glsl
        for (int ax = 0; ax < 3; ++ax) {
            if (pos_new[ax] < dmin[ax]) { pos_new[ax] = dmin[ax]; vel_new[ax] = max(vel_new[ax], 0.0); }
            if (pos_new[ax] > dmax[ax]) { pos_new[ax] = dmax[ax]; vel_new[ax] = min(vel_new[ax], 0.0); }
        }
```

`dmin = domainMin_cellSize.xyz`, `dmax = domainMax_pad.xyz`. The clamp also zero-clips the normal velocity component (no restitution).

**Dam-break preset:** `SPH_PRESETS[0]` at `main.cpp:181-188`:

```cpp:main.cpp:181-188
    {
        "Dam-Break",
        glm::vec3(+0.5f, -1.0f, -1.0f), glm::vec3(+2.0f,  0.5f, +1.0f),
        false, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f),
        EmitterShape::None, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f), 0.0f,
        glm::vec3(-2.0f, -1.0f, -1.0f), glm::vec3(+2.0f, +2.0f, +1.0f),
        0.005f, 0.0f, 0.5f, 0.0f,
    },
```

- **Brick volume:** `(0.5, -1.0, -1.0) → (2.0, 0.5, 1.0)` → extent `(1.5, 1.5, 2.0)`. With `spacing = 2r = 0.02`:
  - `dx = 75, dy = 75, dz = 100` → 562,500 particles. **Exceeds 256k-tier capacity** (262,144) and is clamped to `rt.particleCapacity` at `main.cpp:1599`.
  - At the 1M tier (default), 562,500 particles fit fine.
- **Container bounds:** `(-2, -1, -1) → (+2, +2, +1)` (domain).
- **Initial velocity:** zero (no droplet).
- **No droplet (`add_droplet = false`).**

The initial-fill kernel (`initial_fill.comp.glsl`, see Section E shaders list) places particles in an evenly spaced 3D grid filling the brick AABB. `initial_fill.comp.glsl:41` uses cell centers offset by `+vec3(0.5)*spacing`.

## Section J: Time stepping and CFL

Per-frame substep dt derivation (`main.cpp:1888-1889`):

```cpp:main.cpp:1888-1889
        const float substep_dt = std::clamp(frame_dt / float(std::max(rt.substeps, 1)), DT_MIN, DT_MAX);
        rt.dt = substep_dt;
```

Constants:

```cpp:main.cpp:124-126
constexpr float CFL_FACTOR    = 0.5f;
constexpr float DT_MIN        = 1.0e-4f;
constexpr float DT_MAX        = 5.0e-3f;
```

- **dt is wall-clock-derived, not CFL-derived.** `frame_dt` is the host-side `std::chrono` delta between frames (clamped to `[1/240, 1/15]` at `main.cpp:1809`).
- **No max-velocity scan or sound-speed estimate.** `CFL_FACTOR = 0.5f` is **declared but never referenced** anywhere in the source tree.
- **Substep model:** `dt_substep = clamp(frame_dt / n_substeps, 1e-4, 5e-3)`. The same `dt_substep` is used by every substep in the same frame; `pack_dfsph_uniform(substep_dt)` is called once outside the loop (`main.cpp:1891`) and `dt` does not change between substeps within a frame.
- **Clamps:** `DT_MIN = 1e-4`, `DT_MAX = 5e-3`. Note that `frame_dt` is also pre-clamped to `[1/240, 1/15] ≈ [4.17e-3, 6.67e-2]`. So at `substeps=1`, `dt = clamp(frame_dt, 1e-4, 5e-3)` ≈ `5e-3` (DT_MAX) most of the time.
- **Substep count source:** UI slider `rt.substeps` (range 1..8), `main.cpp:2248`. Default `SUBSTEPS_DEFAULT = 1` (`main.cpp:143`).

## Section K: Post-fix verification — canonical DFSPH UBO (Layout)

Host-side `Layout` struct, verbatim from `main.cpp:1383-1404`:

```cpp:main.cpp:1383-1404
        struct alignas(16) Layout {
            std::uint32_t particleCount;          //  0
            std::uint32_t cellsPerAxisX;          //  4
            std::uint32_t cellsPerAxisY;          //  8
            std::uint32_t cellsPerAxisZ;          // 12
            float supportRadius;                  // 16
            float particleMass;                   // 20
            float density0;                       // 24
            float kernelNorm3D;                   // 28
            float gradKernelNorm3D;               // 32
            float dt;                             // 36
            float viscosity;                      // 40
            float cohesion;                       // 44
            float vorticityStrength;              // 48
            float jacobiRelax;                    // 52
            float _pad0;                          // 56
            float _pad1;                          // 60
            float gravity_pad[4];                 // 64
            float domainMin_cellSize[4];          // 80
            float domainMax_pad[4];               // 96
        } u{};
        static_assert(sizeof(Layout) == 112, "DFSPH canonical UBO size drift");
```

`static_assert(sizeof(Layout) == 112)` **is present** at `main.cpp:1404`. **Confirmed.**

**Comparison across all five DFSPH shaders.** Each shader's `layout(std140) uniform U {...}` block (binding numbers vary per shader, but field order/types/offsets are identical):

| Offset | Type | Field | density_alpha (b=4) | divergence_solve (b=6) | density_solve (b=6) | integrate_forces (b=4) | pressure_apply (b=5) | host |
|---|---|---|---|---|---|---|---|---|
| 0 | uint | particleCount | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 4 | uint | cellsPerAxisX | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 8 | uint | cellsPerAxisY | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 12 | uint | cellsPerAxisZ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 16 | float | supportRadius | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 20 | float | particleMass | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 24 | float | density0 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 28 | float | kernelNorm3D | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 32 | float | gradKernelNorm3D | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 36 | float | dt | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 40 | float | viscosity | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 44 | float | cohesion | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 48 | float | vorticityStrength | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 52 | float | jacobiRelax | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 56 | float | _pad0 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 60 | float | _pad1 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 64 | vec4 | gravity_pad | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ (host as `float[4]`) |
| 80 | vec4 | domainMin_cellSize | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ (host as `float[4]`) |
| 96 | vec4 | domainMax_pad | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ (host as `float[4]`) |
| Total | | | 112 | 112 | 112 | 112 | 112 | 112 |

**All five shaders match the host layout byte-for-byte and field-for-field.** Phase 11 fix-forward #1 is intact.

(Cross-check anchors: `density_alpha.comp.glsl:25-53`, `divergence_solve.comp.glsl:22-50`, `density_solve.comp.glsl:24-52`, `integrate_forces.comp.glsl:15-43`, `pressure_apply.comp.glsl:19-47`.)

## Section L: Post-fix verification — `integrate_forces` push constant

**`pack_dfsph_uniform` signature and body** — `main.cpp:1380-1431`:

```cpp:main.cpp:1380-1431
    auto pack_dfsph_uniform = [&](float dt) {
        glm::uvec3 axes; std::uint32_t maxAxis;
        compute_cells_per_axis(axes, maxAxis);
        struct alignas(16) Layout {
            std::uint32_t particleCount;          //  0
            std::uint32_t cellsPerAxisX;          //  4
            std::uint32_t cellsPerAxisY;          //  8
            std::uint32_t cellsPerAxisZ;          // 12
            float supportRadius;                  // 16
            float particleMass;                   // 20
            float density0;                       // 24
            float kernelNorm3D;                   // 28
            float gradKernelNorm3D;               // 32
            float dt;                             // 36
            float viscosity;                      // 40
            float cohesion;                       // 44
            float vorticityStrength;              // 48
            float jacobiRelax;                    // 52
            float _pad0;                          // 56
            float _pad1;                          // 60
            float gravity_pad[4];                 // 64
            float domainMin_cellSize[4];          // 80
            float domainMax_pad[4];               // 96
        } u{};
        static_assert(sizeof(Layout) == 112, "DFSPH canonical UBO size drift");
        u.particleCount   = rt.particleCount;
        u.cellsPerAxisX   = axes.x;
        u.cellsPerAxisY   = axes.y;
        u.cellsPerAxisZ   = axes.z;
        u.supportRadius   = rt.supportRadius;
        u.particleMass    = particle_mass();
        u.density0        = DENSITY_0;
        u.dt              = dt;
        u.viscosity       = rt.viscosity;
        u.cohesion        = rt.cohesion;
        u.vorticityStrength = rt.vorticityStrength;
        u.kernelNorm3D    = kernel_norm_3d_value();
        u.gradKernelNorm3D= grad_kernel_norm_3d_value();
        u.jacobiRelax     = DFSPH_JACOBI_RELAX;
        u.gravity_pad[0]  = 0.0f;
        u.gravity_pad[1]  = -9.81f;
        u.gravity_pad[2]  = 0.0f;
        u.gravity_pad[3]  = 0.0f;  // mode now lives in push constant
        u.domainMin_cellSize[0] = rt.domainMin.x;
        u.domainMin_cellSize[1] = rt.domainMin.y;
        u.domainMin_cellSize[2] = rt.domainMin.z;
        u.domainMin_cellSize[3] = rt.cellSize;
        u.domainMax_pad[0] = rt.domainMax.x;
        u.domainMax_pad[1] = rt.domainMax.y;
        u.domainMax_pad[2] = rt.domainMax.z;
        tier.uniform_dfsph.uploadDirect(&u, sizeof(u));
    };
```

**Signature: `(float dt)`. Confirmed — only `dt` is passed.**

**Push constant struct** at `integrate_forces.comp.glsl:45-47`:

```glsl:particle-fluids/sph-water/shaders/integrate_forces.comp.glsl:45-47
layout(push_constant) uniform PC {
    uint mode;
} pc;
```

Pipeline creation reserves `sizeof(std::uint32_t)` for the push constant at `main.cpp:1101-1103`:

```cpp:main.cpp:1101-1103
    auto pipe_integrate_forces = make_compute("integrate_forces.comp.glsl",
                                              {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,B,1,CS},{4,U,1,CS}},
                                              sizeof(std::uint32_t));  // mode push-const
```

**The two dispatch calls** — verbatim from `main.cpp:2000-2036`:

```cpp:main.cpp:2000-2036
            {
                auto _ = profiler.scope(cmd, "integrate_forces");
                std::uint32_t mode = 0u;
                pipe_integrate_forces.dispatch(cmd, ds_integrate_forces, wg_particle, 1, 1,
                                               &mode, sizeof(mode));   // FORCES
            }
            cs_barrier();

            // Density-constancy Jacobi inner loop.
            vkCmdFillBuffer(cmd, tier.pressure_a.handle(), 0, VK_WHOLE_SIZE, 0u);
            vkCmdFillBuffer(cmd, tier.pressure_b.handle(), 0, VK_WHOLE_SIZE, 0u);
            gv::memoryBarrier(cmd,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT);
            {
                int iters = std::max(rt.minIterDensity, 1);
                for (int i = 0; i < iters; ++i) {
                    auto _ = profiler.scope(cmd, "density_solve");
                    pipe_density_solve.dispatch(cmd, ds_density_solve[i % 2], wg_particle, 1, 1);
                    cs_barrier();
                }
            }
            {
                auto _ = profiler.scope(cmd, "pressure_apply_density");
                pipe_pressure_apply.dispatch(cmd, ds_pressure_apply, wg_particle, 1, 1);
            }
            cs_barrier();

            // Position-update advances x += dt*v with AABB clamp.
            // mode is pushed per-dispatch via push constant.
            {
                auto _ = profiler.scope(cmd, "integrate_position");
                std::uint32_t mode = 1u;
                pipe_integrate_forces.dispatch(cmd, ds_integrate_forces, wg_particle, 1, 1,
                                               &mode, sizeof(mode));   // POSITION
            }
            cs_barrier();
```

**Mode passed via push constant on both calls.** First call `mode = 0u` (FORCES, `main.cpp:2002-2004`); second call `mode = 1u` (POSITION, `main.cpp:2032-2034`).

**`pack_dfsph_uniform` call sites** — grep of `main.cpp`:

```
main.cpp:1380   auto pack_dfsph_uniform = [&](float dt) {
main.cpp:1891       pack_dfsph_uniform(substep_dt);
```

Only **one call** to `pack_dfsph_uniform`, at `main.cpp:1891`, **outside the substep loop**. **No mid-substep UBO write between the two `pipe_integrate_forces` dispatches.** Phase 11 fix-forward #2 is intact.

(Side note: the shader's UBO still declares `gravity_pad.w` and a code comment at `integrate_forces.comp.glsl:3-4` still describes the field as encoding the mode — see Section P. Behaviourally correct, the comment is stale.)

## Section M: Resource barriers within the substep loop

Full barrier inventory of the substep loop body (`main.cpp:1908-2039`). Every barrier is either `gv::memoryBarrier(...)` (calls `vkCmdPipelineBarrier2`) or expanded inside `cs_barrier()` at `main.cpp:1902-1906`. All are **global** (single VkMemoryBarrier2, no buffer range).

| # | Site | srcStage / srcAccess | dstStage / dstAccess | Between |
|---|---|---|---|---|
| 1 | `cs_barrier` `:1914` | COMPUTE / WRITE | COMPUTE / READ\|WRITE | after `apply_emitter` |
| 2 | `:1920-1922` | TRANSFER / TRANSFER_WRITE | COMPUTE / READ\|WRITE | after `vkCmdFillBuffer` (cell counters) |
| 3 | `cs_barrier` `:1928` | COMPUTE/WRITE | COMPUTE/READ\|WRITE | after `morton_code` |
| 4 | `cs_barrier` `:1933` | as #3 | as #3 | after `cell_count` |
| 5 | `cs_barrier` `:1938` | as #3 | as #3 | after `prefix_sum_local` |
| 6 | `cs_barrier` `:1946` | as #3 | as #3 | after `prefix_sum_block` (mode 0) |
| 7 | `cs_barrier` `:1951` | as #3 | as #3 | after `prefix_sum_block_l2` |
| 8 | `cs_barrier` `:1959` | as #3 | as #3 | after `prefix_sum_block` (mode 1) |
| 9 | `cs_barrier` `:1964` | as #3 | as #3 | after `prefix_sum_addback` |
| 10 | `cs_barrier` `:1969` | as #3 | as #3 | after `scatter` |
| 11 | `cs_barrier` `:1976` | as #3 | as #3 | after `density_alpha` |
| 12 | `:1983-1985` | TRANSFER / TRANSFER_WRITE | COMPUTE / READ\|WRITE | after `vkCmdFillBuffer` (divergence pressure init) |
| 13.k | `cs_barrier` `:1990` (in loop) | as #3 | as #3 | between successive `divergence_solve` iters AND before `pressure_apply_div` |
| 14 | `cs_barrier` `:1996` | as #3 | as #3 | after `pressure_apply_div` |
| 15 | `cs_barrier` `:2006` | as #3 | as #3 | after `integrate_forces` (FORCES) |
| 16 | `:2011-2013` | TRANSFER / TRANSFER_WRITE | COMPUTE / READ\|WRITE | after `vkCmdFillBuffer` (density pressure init) |
| 17.k | `cs_barrier` `:2019` (in loop) | as #3 | as #3 | between successive `density_solve` iters AND before `pressure_apply_density` |
| 18 | `cs_barrier` `:2026` | as #3 | as #3 | after `pressure_apply_density` |
| 19 | `cs_barrier` `:2036` | as #3 | as #3 | after `integrate_forces` (POSITION) |

**Solver-loop barrier coverage:** every iteration of `density_solve` and `divergence_solve` has a `cs_barrier()` immediately after it (`main.cpp:1990` and `main.cpp:2019`). This is correct — no missing inter-iteration barrier.

**`VK_WHOLE_SIZE` concern:** the descriptor writes (see `writeDfsphSolveDescriptor` at `main.cpp:557-590`) all use `range = VK_WHOLE_SIZE`. The same `pressure_a` / `pressure_b` buffers are bound to **both** `ds_divergence_solve` and `ds_density_solve` descriptor sets, but they ping-pong (`p_read = pressure_a`, `p_write = pressure_b` on even iters; swapped on odd). Two-back-to-back dispatches of the **same** pipeline on the same iteration index would be a hazard, but the loop alternates the descriptor set, so the read-buffer / write-buffer pair flips every iteration. The cs_barrier between iters resolves the hazard.

**Hot finding:** `cs_barrier()`'s `srcAccess` is **only** `SHADER_WRITE_BIT` and `dstAccess` is `SHADER_READ_BIT | SHADER_WRITE_BIT` (`main.cpp:1903-1905`). It does **not** include `SHADER_SAMPLED_READ_BIT` or `UNIFORM_READ_BIT`. None of the DFSPH-substep dispatches do sampled reads on storage images, so this is fine. The bilateral pass (outside the substep loop) uses a separate barrier (`main.cpp:2161-2163`) with `SHADER_WRITE_BIT → SHADER_READ_BIT` — also fine.

## Section N: Per-substep resource scaling (substep>=4 crash recon)

Search for `n_substeps`-scaling allocations across `main.cpp`:

- **All `gv::Buffer::create` calls happen in `createTierResources(...)` at `main.cpp:847-940`. None of the sizes mention `rt.substeps` or any substep count. Resource scaling is per-particle and per-cell only.**
- **Command buffer count:** `renderer.framesInFlight() == kMaxFramesInFlight == 2` (`common-cpp/include/gpusims/gpu_profiler.hpp:16`). Independent of substeps.
- **Profiler query pool count:** `kQueriesPerFrame = 2 * kMaxPasses = 512` per frame slot (`common-cpp/src/gpu_profiler.cpp:14`). Each substep emits roughly:
  - 1 `apply_emitter` (conditional) + 8 sort stages + 1 `density_alpha` + `minIterDivergence` `divergence_solve` + 1 `pressure_apply_div` + 1 `integrate_forces` FORCES + `minIterDensity` `density_solve` + 1 `pressure_apply_density` + 1 `integrate_forces` POSITION
  - Default counts (`minIterDivergence=1`, `minIterDensity=2`) → **~17 profiler scopes per substep**.
  - **At substeps=4**: 4 × 17 = **68 profiler scopes from the substep loop**, plus ~4 outside (depth, bilateral×N, thickness, composite) and any additional ImGui scope. At `bilateralIterations=4` that adds 4 scopes. Total ≤ ~80. **Well under `kMaxPasses=256`.**
  - **At substeps=8** (UI max): ~140 per frame. Still under cap.
  - However, if the user raises `minIterDensity` to its UI max of 16, per substep that's `8 + 1 + 16 + 1 + 1 + 16 + 1 + 1 = ~45` profiler scopes — × substeps=8 = **360 scopes**, **exceeds kMaxPasses=256** and triggers the `if (f.pass_count >= kMaxPasses) { logWarn(...); return UINT32_MAX; }` path at `common-cpp/src/gpu_profiler.cpp:57-60`. **This is a soft-fail (no crash).** Still — flag.

- **`MAX_CELLS = 1u << 18 = 262144`** (`main.cpp:952`). Independent of substep count. cell_starts buffer is sized `262144 * 4 = 1 MiB`. Independent.

- **Particle/density_alpha/pressure_a/b buffers** sized by tier capacity, not substeps. Independent.

- **`vkCmdFillBuffer` calls inside the loop** at `main.cpp:1918-1919, 1981-1982, 2009-2010` — 6 `vkCmdFillBuffer` per substep. At `substeps=8` that's 48 fill commands per frame, recorded into a single command buffer. Should be fine.

- **Command-buffer recording cost:** the substep loop records all dispatches into the single per-frame command buffer. At `substeps=8` with `minIter*=16`, each frame records ~80 `vkCmdDispatch` + ~50 barriers + 6 fills × 8 = ~48 fills → ~580 commands per frame. Vulkan command buffers easily handle this.

- **No `vkCmdSetEvent` / `vkCmdWaitEvents` usage anywhere.** Confirmed by grep — neither token appears in `main.cpp` or in `common-cpp/`.

- **Fences:** managed by `renderer.beginFrame()` / `renderer.endFrame()` — one per in-flight slot, not per substep.

**Conclusion for the substep≥4 crash:** **no obvious per-substep resource exhaustion mechanism in the codepath I examined.** The crash is most likely **not** a Vulkan-resource overflow. Candidate sources to investigate when repro becomes available:

1. Numerical blowup from accumulated pressure-step errors with the skeleton coupling term in `divergence_solve.comp.glsl` and `density_solve.comp.glsl` — flagged in Section P.
2. Particle positions becoming NaN, then the morton-code shader's `clamp` returning UB (because `(NaN - min)/cellSize` is NaN, `clamp(NaN, ...) = 0` per IEEE, but the GLSL spec is permissive).
3. Cell index overflow / wrap if domain extent grows. Not investigated further (read-only probe).

## Section O: Screen-space fluid rendering — sanity only

`particle_sprite.vert.glsl` (depth pass) reads particle positions via:

```glsl:particle-fluids/sph-water/shaders/particle_sprite.vert.glsl:23-30
void main() {
    uint vid       = uint(gl_VertexIndex);
    vec3 world_pos = p[vid * 8u + 0u].xyz;
    vec4 view_pos4 = view * vec4(world_pos, 1.0);
    v_view_pos     = view_pos4.xyz;
    v_view_radius  = particleRadius;

    gl_Position = proj * view_pos4;
```

The vertex shader's `Particles` SSBO is bound at `main.cpp:1291-1292` via `writeParticleSpriteDescriptor(... tier.particles.handle() ...)`. The thickness vert is wired identically (`main.cpp:1293-1294`).

**There is no particle position double buffer.** All compute kernels read and write the same single `tier.particles` buffer. The depth and thickness passes consume that same buffer post-substep. The depth pass is dispatched AFTER the substep loop closes (`main.cpp:2100-2126`); the substep loop ends at `main.cpp:2039`; intervening work is Alembic readback (host-mapped, no GPU writes to `particles`) and ImGui state.

**Verdict:** rendering reads the **current** frame's post-substep particle positions. The persistent translucent box outline is **not** caused by stale reads — it has to come from either:

- The hard AABB clamp in `integrate_forces` POSITION mode pinning particles to the box walls (where their depth values congregate at the wall depth and the bilateral filter smears them), or
- The thickness pass's additive blend producing a constant non-zero value at wall positions where many clamped particles overlap.

Both are simulation/post-process artifacts, not stale-buffer bugs. (Out of scope for this read-only probe to confirm exactly which.)

## Section P: Incidental findings

Bullet list — no fixes applied.

- **Cell size = 2 × support radius is unusual** (`main.cpp:131`, `CELL_SIZE_RATIO_TO_SUPPORT = 2.0f`). Standard convention for grid-accelerated SPH is `cellSize == supportRadius` with 3×3×3 traversal (27 cells). With `cellSize = 2h`, a particle's full neighbor disc fits in its own cell and immediate neighbors; the 27-cell loop scans ~8× too much volume. Performance-only at default, but worth confirming intent.
- **Dead solver tuning fields:** `rt.maxIterDensity`, `rt.maxIterDivergence`, `rt.maxErrorDensityPercent`, `rt.maxErrorDivPercent` (`main.cpp:240-245`) are declared but never referenced. Their backing constants `DFSPH_MAX_ITER_DENSITY = 100`, `DFSPH_MAX_ITER_DIV = 100`, `DFSPH_MAX_ERROR_DENSITY = 0.01f`, `DFSPH_MAX_ERROR_DIV = 0.1f` (`main.cpp:115-119`) are also dead. **The solver runs `minIter*` iterations unconditionally and never measures error.**
- **Dead `CFL_FACTOR`:** `constexpr float CFL_FACTOR = 0.5f;` at `main.cpp:124` is declared but never used. No CFL clamp anywhere; dt is purely wall-clock derived.
- **Dead `GRAVITY_Y`:** `constexpr float GRAVITY_Y = -9.81f;` at `main.cpp:128`. The actual gravity value pushed to the UBO uses a literal `-9.81f` at `main.cpp:1420`, not the constant.
- **Skeleton DFSPH coupling term:** both `divergence_solve.comp.glsl` (lines 133-136) and `density_solve.comp.glsl` (lines 135-137) carry an explicit comment that the `coupling = particleMass * dot(grad_W, grad_W)` term is a **placeholder** and should be replaced with the symmetric `(factor_i + factor_j) · m_j · ∇W` form. The current form is **not numerically equivalent to DFSPH per Bender-Koschier 2015/2017**. This is likely the root cause of the "horizontal banding under gravity" / pattern-matches-tensile-instability symptom in the task brief: without proper symmetric coupling the Jacobi solver is approximating a weak-pressure correction with the wrong stencil → density never actually constrains itself → particles stratify by their initial-fill grid layout, which is exactly the horizontal-banding signature.
- **`vorticityStrength` slider has no effect.** `integrate_forces.comp.glsl:125`:
  ```glsl
      vec3 a_vorticity = vec3(0.0) * vorticityStrength;
  ```
  Always zero; multiplied by `vorticityStrength` for a `(void)variable`-style touch-the-uniform tic. Slider does nothing.
- **`cs_barrier()` uses overbroad `READ|WRITE` dst access** (`main.cpp:1903-1905`). Probably defensive; not load-bearing.
- **`writeDfsphSolveDescriptor` ping-pong wiring is correct**: ds[0] reads pressure_a / writes pressure_b; ds[1] swaps (`main.cpp:1254-1273`). The substep-loop iterator `i % 2` selects the descriptor set; semantics match upstream DFSPH Jacobi swap.
- **`integrate_forces.comp.glsl` comment is stale:** the top-of-file docblock at lines 3-4 still says
  ```glsl
  //   gravity_pad.w == 0.0: FORCES_ONLY — v += dt · (gravity + viscosity + cohesion)
  //   gravity_pad.w == 1.0: POSITION_ONLY — x += dt · v with AABB box clamp
  ```
  but the actual implementation reads `pc.mode` (push constant) at line 71. The UBO write zeros `gravity_pad.w` (`main.cpp:1422 "mode now lives in push constant"`). Behaviour correct, comment lags.
- **`density_alpha.comp.glsl` writes `da[gid] = vec4(density, alpha_stored, 0.0, 0.0)`** — explicitly zeroing slots `.z` and `.w` every frame. `_struct_layouts.txt` § 2 claims `.z = predicted_density (density_solve Pass 1)` and `.w = density_advect (divergence_solve Pass 1)`, but neither solver shader writes them — `density_solve` and `divergence_solve` only read `da[gid].x` (density) and `da[gid].y` (alpha) and produce `p_write[gid]`. The "predicted_density" and "density_advect" slots are never populated; documentation is aspirational.
- **`cell_starts` buffer sentinel off-by-one risk:** every solver shader reads `cell_starts[nmorton + 1u]` (e.g., `density_alpha.comp.glsl:102`) to compute the end-of-cell range. For the last cell `nmorton = MAX_CELLS - 1`, this reads `cell_starts[MAX_CELLS]`. But `r.cell_starts` is sized `max_cells * 4` bytes (`main.cpp:881-882`), giving exactly `MAX_CELLS` entries → **last-cell read goes one entry past the end**. `_struct_layouts.txt` line 88 explicitly states size should be `(total_morton_cells + 1) * 4 bytes`. Probably benign on Linux + Vulkan (UB but typically reads adjacent allocation), but a sanitizer / validation layer / driver-on-different-hardware could trip.
- **`r.cell_counts` and `r.cell_counts_atomic` are both zeroed every substep** but `prefix_sum_local` reads from `r.cell_counts` and writes per-block scan into `r.cell_block_prefixes` — there's no path that writes the final cell-count value to slot `[MAX_CELLS]` of `cell_starts` (the "total count" sentinel). Related to the above.
- **Particle SSBO is sized as `particle_count * 128 bytes`** (`main.cpp:862-863`) where `particle_count` is the **tier capacity** (e.g., 256k = `262144 × 128 = 32 MiB`). The renderer dispatches `wg_particle = (rt.particleCount + 255) / 256` work groups but the AABB clamp / sort / etc. operate over the same range. Tail particles (`gid >= rt.particleCount`) are correctly early-out'd in every shader. **Note:** the depth pass draws `vkCmdDraw(cmd, rt.particleCount, 1, 0, 0)` (`main.cpp:2122`); the thickness pass also (`main.cpp:2208`). Both render exactly `rt.particleCount` point sprites.
- **`particle_mass = ρ₀ · (2r)³`** ignores the cubic-spline self-density relationship. SPlisHSPlasH typically initializes mass from the inverse of the cubic-spline self-density: `m = ρ₀ / Σ_i W(|x_i - x_j|, h)` over an idealized FCC packing. Using `ρ₀ · (2r)³` over-estimates particle mass for the 4× h-to-radius ratio because the kernel-sampled rest density at uniform `2r` spacing with `h = 4r` is significantly higher than `ρ₀`. This is a likely contributor to incorrect equilibrium behaviour.
- **No CFL or velocity sanity-clamp anywhere.** Combined with the broken pressure stencil, particles can develop arbitrarily large velocities; the AABB clamp catches positions but velocities can still spike.
- **Hot-reload watches enumerated by file name** (`main.cpp:1641-1661`) — five DFSPH compute shaders all watched. Standard pattern.
- **Common-cpp `gpu_profiler.cpp` and `gpu_profiler.hpp` are in the git modified-set** per the gitStatus header, but I did not diff them (out of scope).
- **The 256k-tier dam-break preset overflows brick count** (562k > 256k). Silently clamped at `main.cpp:1599` to capacity; no warning. Some particles are simply not placed. If the user lands on dam-break at 256k tier (the default tier is 1M but tier is user-settable), they get a half-empty brick.

---

End of report.
