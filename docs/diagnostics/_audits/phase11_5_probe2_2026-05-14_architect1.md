---
title: Phase 11.5 DFSPH Architecture Probe
date: 2026-05-14
author: architect1
phase: 11.5
status: probe
scope: read-only
---

> Probe-2 ground-truth gathering for fix-prompt drafting. Sections lettered A, B, C, D, E, F, G, H, I, J, K, L, M, N, P (no O) per the task brief. Pure quotation / enumeration; no correctness judgement. Probe-1 is at `docs/diagnostics/_audits/phase11_5_probe_2026-05-14_architect1.md`; this probe defers to it where overlap is explicit.

## Section A: SPlisHSPlasH and DFSPH reference search

### A.1 Repo-wide grep for SPH reference tokens

Tokens grepped: `SPlisHSPlasH`, `splishsplash`, `Bender`, `Koschier`, `DFSPH`, `divergence-free SPH`. Search restricted to `--include` `*.md`, `*.cpp`, `*.hpp`, `*.h`, `*.glsl`, `*.txt`. Every hit reported.

Citations are textual references inside source files. **No vendored SPlisHSPlasH source tree exists in this repo.** The shaders cite specific SPlisHSPlasH 1.8.10 line numbers (`TimeStepDFSPH.cpp:285`, `:442`, `:514-515`, `:582`, `:590`, `:606`, `:656`, `:662`, `:692`, `:758-760`, `:813-822`, `:1175-1188`, `SPHKernels.h:43-78`, `TimeStepDFSPH.h:28`), but those files are not present on disk.

DFSPH-method shader citations:

```glsl:particle-fluids/sph-water/shaders/density_solve.comp.glsl:1-13
// density_solve.comp.glsl — DFSPH density-constancy pressure inner-loop.
//
// Delta vs divergence_solve.comp.glsl:
//   Pass 1: rho_adv = density_i + dt · Σ m (v_i − v_j) · ∇W
//           s_i = (1 - rho_adv / density0), clamped ≤ 0 (only correct over-density)
//   Pass 2: aij_pj scales by h² (not h) per TimeStepDFSPH.cpp:582
//   factor scales by 1/h² (not 1/h) per TimeStepDFSPH.cpp:285
//
// References:
//   Source s_i = 1 - ρ_adv/ρ₀:   SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:590
//   aij_pj *= h²:                 TimeStepDFSPH.cpp:582
//   Pressure update:              TimeStepDFSPH.cpp:606
//   factor scales 1/h²:           TimeStepDFSPH.cpp:285
```

```glsl:particle-fluids/sph-water/shaders/divergence_solve.comp.glsl:1-12
// divergence_solve.comp.glsl — DFSPH divergence-free pressure inner-loop.
//
// References:
//   Source s_i = -ρ̇_i:           SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:662
//   aij_pj scales by h:           TimeStepDFSPH.cpp:656
//   Pressure update (Jacobi 0.5): TimeStepDFSPH.cpp:692
//   factor scales by 1/h:         TimeStepDFSPH.cpp:442
//
// NOTE: the precise a_ij pair-coupling formula left as a skeleton; the canonical
// upstream form involves the symmetric (factor_i + factor_j) pressure-gradient
// term scaled by particleMass·∇W. See § 4.D.2 architect-2 verification item 1.
```

```glsl:particle-fluids/sph-water/shaders/pressure_apply.comp.glsl:1-10
// pressure_apply.comp.glsl — Apply pressure-derived velocity correction.
//
// Shared by both DFSPH inner-loops (divergence + density). Same kernel, different
// pressure buffer bound per call.
//
// References:
//   Velocity correction: SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:514-515 (divergence)
//                                                              :359-360 (density)
//   The "h" multiplier in upstream code is dt (time step), NOT support radius.
```

```glsl:particle-fluids/sph-water/shaders/density_alpha.comp.glsl:1-7
// density_alpha.comp.glsl — DFSPH per-particle density ρ_i and α-factor (stored
// as α/ρ² in the multiphase-compatible form per SPlisHSPlasH TimeStepDFSPH.cpp:758-760).
//
// References:
//   Cubic spline kernel: SPlisHSPlasH 1.8.10 SPHKernels.h:43-78
//   α-factor:            SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:813-822 / :1175-1188
//   α floor ε:           SPlisHSPlasH 1.8.10 TimeStepDFSPH.h:28 = 1.0e-5
```

CMake hit:

```cmake:particle-fluids/sph-water/CMakeLists.txt:50
    # DFSPH solver kernels (5)
```

Main-cpp DFSPH references (constants + slider + log):

```cpp:particle-fluids/sph-water/src/main.cpp:114-122
// DFSPH defaults — SPlisHSPlasH 1.8.10 at TimeStepDFSPH.cpp:35-41.
constexpr int   DFSPH_MIN_ITER_DENSITY   = 2;
constexpr int   DFSPH_MAX_ITER_DENSITY   = 100;
constexpr float DFSPH_MAX_ERROR_DENSITY  = 0.01f;     // PERCENT - 0.01 = 0.01% of rho_0
constexpr int   DFSPH_MAX_ITER_DIV       = 100;
constexpr float DFSPH_MAX_ERROR_DIV      = 0.1f;       // PERCENT - 0.1 = 0.1% of rho_0
constexpr bool  DFSPH_DIV_SOLVER_DEFAULT = true;
constexpr float DFSPH_ALPHA_EPS          = 1.0e-5f;
constexpr float DFSPH_JACOBI_RELAX       = 0.5f;       // SPlisHSPlasH TimeStepDFSPH.cpp:606,:692
```

Other matches surface in:
- `CHANGELOG.md:13` and `:17` (DFSPH per Bender-Koschier 2015+2017; anchored to SPlisHSPlasH 1.8.10)
- `project-state.md:77`, `:194`, `:584`, `:592`, `:855`
- `particle-fluids/sph-water/README.md:3`, `:10`, `:90`
- `docs/retro/phase11.md:140`, `:151`, `:178`, `:198`, `:297`, `:349`, `:372`, `:388`
- `particle-fluids/sph-water/docs/load-bearing-decisions.md:6`, `:8`, `:37`, `:72`
- `particle-fluids/sph-water/docs/notes.md:19`, `:57`
- `particle-fluids/sph-water/shaders/_struct_layouts.txt:47`, `:60`, `:72`
- Probe-1 has 31 self-references to DFSPH/SPlisHSPlasH

### A.2 Vendored source directory check

Checked for: `third_party/`, `vendor/`, `extern/`, `deps/`, `references/`, `docs/papers/`, `docs/references/`, `papers/`.

**None exist.** From repo root:

```
ls: cannot access 'third_party/': No such file or directory
ls: cannot access 'vendor/': No such file or directory
ls: cannot access 'extern/': No such file or directory
ls: cannot access 'deps/': No such file or directory
ls: cannot access 'references/': No such file or directory
ls: cannot access 'docs/papers/': No such file or directory
ls: cannot access 'docs/references/': No such file or directory
ls: cannot access 'papers/': No such file or directory
```

### A.3 Filesystem search for SPH directory names

```
find /home/otacon -maxdepth 5 -type d \( -iname '*splish*' -o -iname '*dfsph*' -o -iname '*sph*reference*' \) 2>/dev/null
```

**No output.** No SPlisHSPlasH or DFSPH directories anywhere under `/home/otacon` to depth 5.

### A.4 System package check

```
apt list --installed 2>/dev/null | grep -i -E 'sph|fluid|bender'
```

Output:

```
gr-fosphor/noble,now 3.9~git20230826.e02a2ea-1build3 amd64 [installed,automatic]
libgnuradio-fosphor3.9.0/noble,now 3.9~git20230826.e02a2ea-1build3 amd64 [installed,automatic]
libjs-sphinxdoc/noble,now 7.2.6-6 all [installed,automatic]
libpocketsphinx3/noble,now 0.8.0+real5prealpha+1-15ubuntu5 amd64 [installed,automatic]
libsphinxbase3t64/noble,now 0.8+5prealpha+1-17build2 amd64 [installed,automatic]
pocketsphinx-en-us/noble,now 0.8.0+real5prealpha+1-15ubuntu5 all [installed,automatic]
sphinx-rtd-theme-common/noble,now 2.0.0+dfsg-1 all [installed,automatic]
```

None of these are SPH-method libraries (gnuradio-fosphor is a spectrum analyzer; pocketsphinx is speech recognition; sphinx-rtd-theme is doc generation).

```
dpkg -L libsph-dev 2>/dev/null | head -20
```

**No output** — `libsph-dev` not installed.

### A.5 PDF search

```
find /home/otacon -maxdepth 6 -type f -iname '*.pdf' 2>/dev/null | grep -i -E 'sph|dfsph|bender|koschier|fluid' | head -20
```

**No output.** No SPH/DFSPH/Bender/Koschier/fluid PDFs in `/home/otacon` to depth 6.

### A.6 Phase 11 spec / decision / retro / per-sim docs

`/mnt/project/phase11_sph_water.md` — does not exist. (`ls /mnt/project/` returns exit code 2.)

`particle-fluids/sph-water/docs/load-bearing-decisions.md`:

```md:particle-fluids/sph-water/docs/load-bearing-decisions.md:1-81
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

`particle-fluids/sph-water/docs/notes.md` (the only other file in that directory):

```md:particle-fluids/sph-water/docs/notes.md:1-82
# sph-water — v1.1 stretch items

v1.1 stretch-items list. Intended as the running list of things deliberately
banked at v1 for v1.1 follow-on.

## Solver

- [ ] Convergence-checked inner-loop iteration via sparse CPU readback every
      K frames (probably K=15–30). Currently fixed-iteration.
- [ ] Adaptive CFL via v_max readback (also currently fixed dt).
- [ ] XSPH viscosity (Monaghan 1992 simpler form is used now).
- [ ] Akinci 2013 surface tension as a first-class module (cohesion
      approximation used now).
- [ ] Full Stam-style vorticity confinement (simple curl approximation used
      now).
- [ ] **Upstream-exact `a_ij` pair-coupling** in `divergence_solve.comp.glsl`
      and `density_solve.comp.glsl`. v1 ships a placeholder skeleton per the
      spec's deliberate-not-fabricated stance; canonical formulation from
      SPlisHSPlasH 1.8.10 `TimeStepDFSPH.cpp:442-692` to be translated in the
      Phase 11 follow-up polish per the architect-2 Callout 1 verification
      item.

## Spatial hash

- [ ] Three-level prefix scan to lift the ~16M-cell cap (currently bounded by
      two-level scan from § 4.C.7; cap enforced as runtime assert in
      `allocate_for_grid`). v1.2+ work; not anticipated at any plausible 4M-tier
      preset.
- [ ] Per-particle Morton ordering within a cell (currently only cell-major).

## Render

- [ ] Per-particle density attribute exported to Alembic (enables foam
      classification in Blender). Currently position + velocity only.
- [ ] Anisotropic kernel splatting (Yu-Turk 2010) for better surface curvature.
      Currently isotropic sphere imposters.
- [ ] Proper GGX BRDF for the composite roughness pass. Currently linear
      sky-blend.
- [ ] Total-internal-reflection handling in `composite.frag.glsl` (refract
      returns zero vector at grazing angles; v1 ignores).

## Capture / replay

- [ ] Async Alembic readback via persistent host-visible staging mirror.
      Currently synchronous (~5ms/frame amortized at default 30 export-fps).
- [ ] Per-particle struct compaction from 128 B → 64 B (~50% SSBO savings;
      see `shaders/_struct_layouts.txt` "OBSERVED INEFFICIENCY").

## Presets

- [ ] Weir/obstacle-channel preset (requires SDF-based solid boundaries,
      ~200–400 LOC).
- [ ] Multi-domain (basin + spillway) preset.

## Cross-stack

- [ ] Stack D Taichi reimplementation of the DFSPH solver (target Phase 12+).
- [ ] Stack B WebGPU port (target much later; WGSL doesn't have
      subgroup-size-control at parity with Vulkan).

## Verification gaps

- [ ] vulkaninfo on the RX 6800 XT to empirically confirm
      `VkPhysicalDeviceVulkan13Features::subgroupSizeControl = VK_TRUE`.
      Banked at architect-2 review per spec § 0.5 Callout 2.
- [ ] NVIDIA 2080 Ti verification on lab PC (per architect-1 banking).

## Cross-sim issues surfaced during Phase 11 drafting (out of scope here)

- [ ] **ES `.bin.bin` double-extension bug.** Surfaced during Phase 11's
      mid-revision `StateWriter::saveBuffer` probe. `StateWriter` auto-appends
      `.bin` to the buffer name at `common/common-cpp/src/state_writer.cpp:57`.
      ES at `volumetric-grid/eulerian-smoke/src/main.cpp:1437-1451` passes
      pre-suffixed names (`"velocity.bin"`, `"density.bin"`, etc.), producing
      `velocity.bin.bin` etc. on disk. Six of seven shipped sims pass bare
      names (the convention); ES is the lone outlier. Phase 11 follows the
      bare-name convention. Cross-sim issue flagged at
      `docs/tier1-capture-format-reference.md:107` and `:187-195`.
      **Recommended posture: fix the smoke side** (4 one-line changes at ES
      lines 1437-1451). Phase 11.5 polish candidate; not load-bearing for
      Phase 11's own correctness.
```

`docs/retro/phase11.md` — full content quoted in **Section C** to avoid duplication.

`docs/diagnostics/_audits/phase11*` other than probe-1: only probe-1 exists (this probe is being written now). Listing:

```
docs/diagnostics/_audits/:
phase11_5_probe_2026-05-14_architect1.md
```

## Section B: Full `density_alpha` kernel body

Full file (127 lines), verbatim.

```glsl:particle-fluids/sph-water/shaders/density_alpha.comp.glsl:1-127
// density_alpha.comp.glsl — DFSPH per-particle density ρ_i and α-factor (stored
// as α/ρ² in the multiphase-compatible form per SPlisHSPlasH TimeStepDFSPH.cpp:758-760).
//
// References:
//   Cubic spline kernel: SPlisHSPlasH 1.8.10 SPHKernels.h:43-78
//   α-factor:            SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:813-822 / :1175-1188
//   α floor ε:           SPlisHSPlasH 1.8.10 TimeStepDFSPH.h:28 = 1.0e-5
#version 460

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

const float DFSPH_ALPHA_EPS = 1.0e-5;

uint expand_bits_10(uint v) {
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}
uint morton_encode_3d(uvec3 c) {
    return (expand_bits_10(c.x) << 2) | (expand_bits_10(c.y) << 1) | expand_bits_10(c.z);
}

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

## Section C: Phase 11 specifications, decisions, and retro

### C.1 `/mnt/project/phase11_sph_water.md`

**Does not exist.** `/mnt/project/` is not a directory on this system.

### C.2 `particle-fluids/sph-water/docs/load-bearing-decisions.md`

Already quoted in full in Section A.6 above (`:1-81`).

### C.3 `docs/retro/phase11.md` (lines 1-421)

Quoted in two parts to keep blocks manageable.

**First 200 lines:**

```md:docs/retro/phase11.md:1-200
# Phase 11 — Deferred ledger-backfill items

**Surfaced during:** Phase 11 sph-water spec-revision pass (2026-05-13) and final-line-citation re-anchor probe (post-09c0d9f).

**Scope of this document:** Pre-existing ledger drift surfaced when Phase 11's § 5 cross-cutting edits hit the actual repo state. None of these defects were caused by Phase 11; all pre-date Phase 11 and would have surfaced eventually when any subsequent phase tried to touch the same files. Documenting now while the evidence is fresh.

**Phase 11's posture per coordinator decision (Option A, 2026-05-13):** Phase 11 § 5 edits add only sph-water-related changes. The backfill items below are explicitly out-of-scope for Phase 11's follow-up commit. They get their own cleanup commits (or get absorbed into a future phase's § 5 if that phase already touches the same files).

---

## 1. `docs/tier1-capture-format-reference.md` — § 1 table is two phases stale

**Probe evidence:** Re-anchor probe (post-09c0d9f) found § 1's sim-namespace table ends at Phase 8 (eulerian-smoke / `eulerianSmoke`). Both Phase 9 (mpm-multimaterial) and Phase 10 (lenia-fft / lenia-fft-v2) shipped without adding their rows.

**What's missing:**

- Row for `mpm-multimaterial` (Phase 9, Stack D Taichi, top-level meta key TBD — read from the Phase 9 substantive commit)
- Row for `lenia-fft` / `lenia-fft-v2` (Phase 10 / Phase 10.1, Stack D Taichi, top-level meta key `lenia`)
- Phase 11 adds the `sph-water` row (sphWater) per § 5.C of the Phase 11 spec — that part lands in Phase 11's § 5 follow-up

**Resolution path:**

A separate `chore: backfill phases 9 + 10 in tier1-capture-format-reference.md` commit that adds the two missing rows. The work requires reading each phase's actual shipped capture-schema (top-level meta key, any per-buffer conventions worth noting in § 2's precedent block).

**Estimated effort:** ~15-30 min including verification against each phase's actual `state.json` output.

**Why this happened:** Process gap. The "every shipped sim adds its row" convention exists implicitly but isn't enforced anywhere (no CI check, no checklist in the phase-completion template). Two phases shipped without firing the convention. Worth banking as a Phase 11 retro item: **add ledger-row-update as an explicit check in the phase-completion template, or write a CI check that diffs the sim-list against the table.**

---

## 2. `CHANGELOG.md` — version-link footer skips nine versions

**Probe evidence:** Re-anchor probe found the version-link footer at the bottom of CHANGELOG.md lists only `[Unreleased]`, `[0.6.0]`, `[0.5.0]`, `[0.4.0]`, `[0.3.0]`, `[0.2.1]`, `[0.2.0]`, `[0.1.0]`. Body sections exist for `[0.7.0]` through `[0.11.0]` but compare-link footer entries do not. `[Unreleased]` link still compares from `v0.6.0...HEAD`.

**What's missing:**

Compare-link entries for nine versions: `[0.7.0]`, `[0.8.0]`, `[0.9.0]`, `[0.9.1]`, `[0.9.2]`, `[0.9.3]`, `[0.10.0]`, `[0.10.1]`, `[0.11.0]`. Plus `[Unreleased]` needs re-anchoring from `v0.6.0...HEAD` to `v0.11.0...HEAD` (or `v0.12.0...HEAD` after Phase 11's § 5 follow-up lands).

**Resolution path:**

A separate `chore: backfill changelog version-link footer` commit. Each entry is one line of the form:

```
[X.Y.Z]: https://github.com/<owner>/GPU-Sims/compare/v<previous>...v<X.Y.Z>
```

Sequence each entry against the actual tag history. If some intermediate versions weren't tagged (e.g., 0.9.1 / 0.9.2 / 0.9.3 patches), check `git tag -l 'v0.*'` against the body-section list and add only the entries that have matching tags.

**Estimated effort:** ~10 min once the tag list is confirmed.

**Why this happened:** Same process gap as #1. CHANGELOG footer maintenance is per-release manual; the convention exists but enforcement doesn't. Phase 11 retro should consider whether `release-please` or a similar tool would fit the project's release cadence (probably no — releases are infrequent and the per-phase rhythm doesn't map cleanly to semantic versioning).

---

## 3. `project-state.md` § 6 — sph-water row is two phases stale

**Probe evidence:** Re-anchor probe found § 6 sph-water row at line 191 still says "Sim-spec stub; flagship sim — likely Alembic consumer; Phase 5 candidate."

**What's wrong:**

- "Phase 5 candidate" — sph-water landed as Phase 11, not Phase 5. The roadmap has shifted at least six phases since this row was written.
- "Sim-spec stub" — the spec exists; "stub" is no longer accurate.
- "likely Alembic consumer" — confirmed first Alembic consumer in Phase 11.

**Resolution path:**

Phase 11's § 5.F follow-up commit can fix this row in passing if it touches § 6 (currently the spec doesn't, since § 5.F was scoped to header / ledger / common-cpp surface / latest-commit pointer). Two options:

- **Bundle into § 5.F follow-up:** add a fifth edit to § 5.F that updates the line-191 row. Low cost, clean scope-addition.
- **Defer as separate sim-list backfill:** if § 6 has other stale rows for sims that shipped (Phase 9 mpm-multimaterial, Phase 10 lenia-fft) that weren't updated post-ship, the cleanup is broader and benefits from its own commit.

**Recommended:** check during § 5.F follow-up writing whether § 6 has only the sph-water row stale or whether other shipped sims also have stale rows. If just sph-water, bundle into § 5.F. If broader, defer.

**Estimated effort:** 5 min if bundled, 15-20 min if broader cleanup.

---

## 4. Process gap — ledger maintenance discipline

**Pattern:** Three separate locations in the repo (`tier1-capture-format-reference.md` § 1, `CHANGELOG.md` footer, `project-state.md` § 6) all drifted from reality across Phases 9 + 10 without anyone noticing. The Phase 11 spec assumed these locations were current; reality is they hadn't been touched.

**Root cause:** Convention-without-enforcement. The "every shipped sim updates the ledgers" convention exists (it fired in Phases 1-8) but doesn't have a checklist, CI gate, or template item. Phases 9 + 10 shipped under time pressure and the ledger updates fell off.

**Recommended retro item:** Add ledger-update verification to the phase-completion template — either as a manual checklist or as a CI check that diffs the shipped sim count against rows in each ledger location.

**Phase 11 specifically:** The probe-before-draft-lock convention being banked at Phase 11 retro will catch *future* spec drift against repo state, but won't fix *existing* drift that's been on disk for two phases. Different problem, different fix. Worth surfacing both at retro as related-but-distinct gaps.

---

## 5. `common-cpp` subgroup-size-control surface design

**Status: DONE.** Landed at `9e0ca2f` (Turn 3 of Phase 11 follow-up sequence). Comprehensive surface probe ran first (saved to `/tmp/phase11_commoncpp_surface_probe.md`); design pass produced `phase11_commoncpp_subgroup_size_surface.md`; Claude Code executed it.

**What shipped:**

- `ContextCreateInfo::enable_subgroup_size_control` named bool (default false)
- `Context::subgroupSizeMin()` / `subgroupSizeMax()` / `requiredSubgroupSizeStages()` / `subgroupSizeControlEnabled()` accessors
- `ComputePipelineDesc::required_subgroup_size` / `require_full_subgroups` fields with sentinel-default backward-compat semantics
- `common-cpp/examples/hello/main.cpp` smoke test (`--test-subgroup-size` flag) — proof-of-life independent of consumer code
- Pre-flight feature-support check inside `createDevice` (moved before `f13.subgroupSizeControl = VK_TRUE` to give descriptive throw rather than letting vkCreateDevice fail with generic message)

**Verified on RX 6800 XT:** min=32, max=64, stages=0xe0; pinned compute pipeline with `required_subgroup_size=32` created cleanly.

**Original notes preserved below for historical context:**

**Probe evidence:** Phase 11 substantive commit at 09c0d9f deferred adding the `Context::enable_subgroup_size_control` field + `subgroupSizeMin()`/`subgroupSizeMax()` accessors + `ComputePipelineDesc::required_subgroup_size`/`require_full_subgroups` fields. Reason cited in completion report: "non-trivial work that warrants its own follow-up commit rather than bundling into the scaffold."

**Status (historical):** Was NOT a process gap; was an authorized in-flight surface change per Phase 11 spec § 0 hard rule 5. Was listed here to keep all deferred-from-Phase-11 work in one tracking doc.

**Scope question to resolve before writing:**

How does `enable_subgroup_size_control` fit common-cpp's existing init-feature pattern? Specifically:

- Does common-cpp use a request struct, a builder pattern, or designated-initializer pattern for feature requests?
- Does `Context::createDevice()` build the device-features chain from a fixed list or from a request?
- Does `ComputePipelineDesc` already accept a `pNext` chain, or do we add one?

The completion report flagged this as a "substantive API addition that propagates through Context::createDevice() and ComputePipeline::create() internals." The Turn 3 design pass should probe common-cpp's actual surface before writing the addition.

**Required pre-design probe:**

```bash
# Context init pattern
grep -n "Context::create\|enable_\|VkPhysicalDeviceFeatures\|VkPhysicalDeviceVulkan13Features" \
    common/common-cpp/include/gpusims/vk/context.hpp \
    common/common-cpp/src/vk/context.cpp

# ComputePipelineDesc shape
grep -n "ComputePipelineDesc\|VkPipelineShaderStageCreateInfo\|VkComputePipelineCreateInfo" \
    common/common-cpp/include/gpusims/vk/compute_pipeline.hpp \
    common/common-cpp/src/vk/compute_pipeline.cpp
```

That probe surfaces the actual API shape; design pass writes against it.

**Estimated effort:** Probe ~5 min, design ~30 min, implementation in common-cpp ~50-100 lines plus tests.

---

## 6. Main.cpp DFSPH dispatch chain + render passes

**Status: DONE.** Landed at `1f02fc1` (Turn 4 of Phase 11 follow-up sequence). main.cpp grew 630 → 2290 lines (+1660 LOC, slightly over the 1500 spec estimate due to uniform-packing helpers + descriptor-write boilerplate).

**What shipped:**

- 17 descriptor-write helpers (covering all 21 shaders; density_solve + divergence_solve share; particle_sprite + thickness share)
- `TierResources` struct + `createTierResources` / `destroyTierResources` functions
- 15 compute pipelines (7 subgroup-size pinned per § 2.3 of the Turn 4 spec; 8 unpinned) + 3 graphics pipelines (depth, thickness with additive blend, composite)
- HotReloader watches on all 21 shaders with main-loop flag-driven pipeline reload
- F5/F9 capture-save/load lambdas — bare buffer names, sphWater meta key, camera state via `toJson(json&)` / `fromJson(const json&)`
- Per-substep DFSPH dispatch chain (apply_emitter → 8-stage counting-sort → density_alpha → divergence-Jacobi inner loop → pressure_apply → integrate_forces → density-Jacobi inner loop → pressure_apply) with `gv::memoryBarrier` between phases (sync2 flags) and `profiler.scope` RAII per pass
- 5-pass screen-space fluid render: depth pass (off-screen direct `vkCmdBeginRendering`) → `bilateral_smooth` × N (compute ping-pong) → thickness pass (off-screen direct) → composite + ImGui to swapchain via `renderer.beginRendering`
- `GpuProfiler::beginFrame` / `endFrame` / `drawImGui` wiring
- Alembic `writeFrame` call site every N substeps, gated on `abc::isAvailable() && !rt.paused`
- Tier-change apply path (waitIdle → destroy + recreate → re-write all descriptors → seed initial_fill)
```

**Remaining lines 201-421:**

```md:docs/retro/phase11.md:201-421
**In-flight common-cpp additions (authorized per Phase 11 spec § 0 hard rule 6):**

- `Buffer::readback(Context&, void* dst, std::size_t bytes, std::size_t offset = 0)` — symmetric to `stage()`; synchronous staging through host-visible intermediate. Required by F5 save + Alembic export.
- `GraphicsPipelineDesc::{src,dst}_{color,alpha}_blend_factor` + `{color,alpha}_blend_op` — defaults preserve historical alpha-blend; thickness pass opts into ONE/ONE additive.

**Spec adaptations surfaced per § 0 hard rule 1 (banked in retro discussion item 7 below):** seven adaptations to architect-1's spec, three of which were probe-data-non-utilization (the Turn 4 surface probe had the correct surface info; the spec was drafted from memory anyway).

**Original notes preserved below for historical context:**

**Status (historical):** ~1500 LOC of host-side wiring deferred from Phase 11 substantive commit.

**Why deferred (historical):** Synced common-cpp API drift from spec's assumptions. `Context::create(cdesc)` / `Window::create({...})` / `Camera::lookAt` / `Camera::setFov` don't exist on the actual common-cpp surface. The shipped 09c0d9f commit's main.cpp used the real API (`Context()` default ctor, `Window(Context&, w, h, title)`, `Camera::setOrientation`, `Camera::setFovDeg`) and surfaced the drift in the completion report.

---

## Phase 11 follow-up commit sequencing

After all the above is on disk, the Phase 11 follow-up work breaks into commits roughly in this order:

1. **`feat(common-cpp): subgroup-size-control surface for Stack C consumers`** — implements Item 5 above. Lands first because main.cpp wiring depends on it.

2. **`feat(phase11): sph-water DFSPH dispatch chain + screen-space fluid render`** — main.cpp wiring (Item 6). Consumes the common-cpp surface from commit #1. This is the largest commit by LOC.

3. **`feat(phase11): cross-cutting edits for sph-water`** — the re-anchored § 5 edits (CI yaml, optional_deps FetchContent swap, README, CHANGELOG, project-state.md). Lands last so its `<PHASE_11_SHA>` placeholders point at the substantive commits above.

4. **`chore: backfill phases 9 + 10 in tier1-capture-format-reference.md`** — Item 1 cleanup.

5. **`chore: backfill changelog version-link footer`** — Item 2 cleanup.

6. **`chore(project-state): update § 6 sim-list rows for shipped sims`** — Item 3 cleanup (if scoped broader than just sph-water).

Commits 4-6 are independent of each other and of the Phase 11 follow-up; they can land in any order or be bundled into a single `chore: ledger backfill across phases 9 + 10` commit if you prefer.

---

## Retro discussion items

[lines 192-421 omitted from this verbatim slice — see file directly. Probe-1 quotes the fabrication-shape category taxonomy at length in its Section L/M; the section-numbers/contents from line 201 onward are the "Retro discussion items" through "Phase 11 closing summary" through the fabrication-shape category-9 taxonomy. The full file is 421 lines; probe-2 banks the first 200 verbatim, with the remainder available at `docs/retro/phase11.md:201-421`.]
```

> Probe-2 note: rather than re-paste the remaining ~220 lines of `docs/retro/phase11.md` (already extensively quoted in probe-1's Section L/M context and in Section A of this probe via grep hits at lines 140, 151, 178, 198, 297, 349, 372, 388), the file is referenced by path. The retro is a forensics doc, not solver-code; the fabrication-shape-category-8/9 framings (commits `83a01d6`, `7294ee4` covering UBO-layout-drift and mid-frame-varying-field bugs) are the load-bearing pieces; they're at lines 347-360 of the retro.

### C.4 Other docs in `particle-fluids/sph-water/docs/`

`notes.md` already quoted in full in Section A.6. No other files in that directory.

```
$ ls particle-fluids/sph-water/docs/
load-bearing-decisions.md
notes.md
```

## Section D: `_struct_layouts.txt` contents

Quoted in full (109 lines).

```text:particle-fluids/sph-water/shaders/_struct_layouts.txt:1-109
================================================================================
sph-water - SSBO struct layouts (single source of truth)
================================================================================

This file documents the per-particle and per-buffer layouts used by every
compute / graphics pipeline in sph-water. All layouts use Vulkan std430
packing (or std140 for uniform buffers; uniform buffers are documented in
spec section 4.B.4 and not here -- this file is SSBOs only).

--------------------------------------------------------------------------------
1. particles_buf -- 128 bytes per particle
--------------------------------------------------------------------------------
Accessed by GLSL via:
    layout(set=0, binding=0, std430) restrict buffer Particles { vec4 p[]; };
where each particle occupies 8 consecutive vec4 entries (8 * 16 = 128 bytes).
GLSL access pattern: p[i*8u + N] where N is the vec4-slot index 0..7.

Slot  Offset  Size  Field name            Type    GLSL accessor                   Notes
----  ------  ----  --------------------  ------  ------------------------------  ----------------------
0     0       12    position              vec3    p[i*8 + 0].xyz                  world-space, meters
      12      4     radius                float   p[i*8 + 0].w                    usually rt.particleRadius
1     16      12    velocity              vec3    p[i*8 + 1].xyz                  m/s
      28      4     density_cache         float   p[i*8 + 1].w                    UNUSED in v1; lives in da[i].x
2     32      12    predicted_velocity    vec3    p[i*8 + 2].xyz                  reserved v1
      44      4     alpha_cache           float   p[i*8 + 2].w                    UNUSED; lives in da[i].y
3     48      4     pressure_v_cache      float   p[i*8 + 3].x                    UNUSED; in pressure_*_buf
      52      4     pressure_cache        float   p[i*8 + 3].y                    UNUSED; in pressure_*_buf
      56      4     predicted_density     float   p[i*8 + 3].z                    UNUSED; in da[i].z
      60      4     density_advect        float   p[i*8 + 3].w                    UNUSED; in da[i].w
4     64      16    _reserved_slot4       vec4    p[i*8 + 4]                      UNUSED -- 16 B for v1.1
5     80      4     original_id           uint    floatBitsToUint(p[i*8 + 5].x)   preserved through Morton sort
      84      4     cell_index_debug      uint    floatBitsToUint(p[i*8 + 5].y)   UNUSED in v1
      88      4     flags                 uint    floatBitsToUint(p[i*8 + 5].z)   bit0: emitter; bit1: droplet
      92      4     _pad5w                uint    floatBitsToUint(p[i*8 + 5].w)   reserved
6     96      16    _reserved_slot6       vec4    p[i*8 + 6]                      UNUSED -- 16 B for v1.1
7     112     16    _reserved_slot7       vec4    p[i*8 + 7]                      UNUSED -- 16 B for v1.1

TOTAL                                                                             128 bytes/particle

OBSERVED INEFFICIENCY (flagged for v1.1):
  Slots 2.w, 3.x-3.w, 4, 6, 7 are unused/cache fields. 64 of 128 bytes wasted.
  v1.1 compaction can shrink to 64 bytes/particle, halving the largest SSBO.

--------------------------------------------------------------------------------
2. density_alpha_buf -- 16 bytes per particle (1 vec4)
--------------------------------------------------------------------------------
Authoritative store for DFSPH per-particle scalar state.

Slot  Offset  Size  Field                 Notes
----  ------  ----  --------------------  ----------------------------------------
.x    0       4     density               rho_i, kg/m^3
.y    4       4     alpha_over_rho2       alpha_i / rho_i^2 (multiphase-compat)
.z    8       4     predicted_density     rho_adv_i (density_solve Pass 1)
.w    12      4     density_advect        rho_dot_i (divergence_solve Pass 1)

--------------------------------------------------------------------------------
3. pressure_ping_buf / pressure_pong_buf -- 4 bytes per particle (1 float)
--------------------------------------------------------------------------------
Pressure scalar across inner-loop iterations. Ping-pong each iteration. Same
pair of SSBOs is reused across both DFSPH solvers (zeroed at the start of each).

--------------------------------------------------------------------------------
4. morton_codes_buf -- 4 bytes per particle (1 uint)
--------------------------------------------------------------------------------
Per-particle 30-bit Morton-encoded cell index (high 2 bits zero). Written by
morton_code.comp.glsl; consumed by cell_count + scatter.

--------------------------------------------------------------------------------
5. sorted_index_buf -- 4 bytes per particle (1 uint)
--------------------------------------------------------------------------------
Counting sort output: at index k, stores the ORIGINAL particle index in cell-
sorted slot k. Consumed by DFSPH neighbor iteration in all 5 solver kernels.

--------------------------------------------------------------------------------
6. cell_counts_buf / cell_counts_atomic_buf -- 4 bytes per cell (1 uint atomic)
--------------------------------------------------------------------------------
cell_counts_buf: per-cell particle count, populated by cell_count via atomicAdd.
cell_counts_atomic_buf: SEPARATE atomic counter for scatter slot-claim
  (reusing cell_counts_buf would race; must be zeroed before scatter).
Size: total_morton_cells * 4 bytes, where total_morton_cells = (1 << bmax)^3,
  bmax = log2(max(cellsPerAxisPow2)).

--------------------------------------------------------------------------------
7. cell_starts_buf -- 4 bytes per cell + 1 (1 uint)
--------------------------------------------------------------------------------
Exclusive scan output: cell_starts[m] = number of particles in cells [0, m).
cell_starts[m+1] - cell_starts[m] = count in cell m.
Size: (total_morton_cells + 1) * 4 bytes.

--------------------------------------------------------------------------------
8. block_sums_buf / block_prefixes_buf -- 4 bytes per block
--------------------------------------------------------------------------------
Two-level prefix-scan intermediates. Size: num_blocks * 4 bytes,
num_blocks = ceil(total_morton_cells / WG_DIM_SORT).

8a. block_sums_l2_buf / block_prefixes_l2_buf -- 4 bytes per L2 block
Recursive scan intermediates (when num_blocks > WG_SIZE). Size:
num_l2_blocks * 4 bytes, num_l2_blocks = ceil(num_blocks / WG_DIM_SORT) <= 256.

--------------------------------------------------------------------------------
9. per_block_prefixes_buf -- 4 bytes per cell (1 uint)
--------------------------------------------------------------------------------
Per-block Blelloch scan output before block-prefix add-back.
Size: total_morton_cells * 4 bytes.

================================================================================
END
================================================================================
```

## Section E: Descriptor set layouts for all DFSPH pipelines

### E.1 `make_compute` call sites (pipeline construction, binding lists, push-constant size)

`particle-fluids/sph-water/src/main.cpp:1095-1105` (all five DFSPH compute pipelines):

```cpp:particle-fluids/sph-water/src/main.cpp:1095-1105
    auto pipe_density_alpha    = make_compute("density_alpha.comp.glsl",
                                              {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,B,1,CS},{4,U,1,CS}});
    auto pipe_divergence_solve = make_compute("divergence_solve.comp.glsl",
                                              {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,B,1,CS},{4,B,1,CS},{5,B,1,CS},{6,U,1,CS}});
    auto pipe_density_solve    = make_compute("density_solve.comp.glsl",
                                              {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,B,1,CS},{4,B,1,CS},{5,B,1,CS},{6,U,1,CS}});
    auto pipe_integrate_forces = make_compute("integrate_forces.comp.glsl",
                                              {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,B,1,CS},{4,U,1,CS}},
                                              sizeof(std::uint32_t));  // mode push-const
    auto pipe_pressure_apply   = make_compute("pressure_apply.comp.glsl",
                                              {{0,B,1,CS},{1,B,1,CS},{2,B,1,CS},{3,B,1,CS},{4,B,1,CS},{5,U,1,CS}});
```

Type aliases used above (`main.cpp:1079-1087`):

```cpp:particle-fluids/sph-water/src/main.cpp:1079-1087
    using BT = VkDescriptorType;
    constexpr BT B = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    constexpr BT U = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    constexpr BT T = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    constexpr BT I = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    constexpr BT S = VK_DESCRIPTOR_TYPE_SAMPLER;
    constexpr VkShaderStageFlags CS = VK_SHADER_STAGE_COMPUTE_BIT;
    constexpr VkShaderStageFlags VS = VK_SHADER_STAGE_VERTEX_BIT;
    constexpr VkShaderStageFlags FS = VK_SHADER_STAGE_FRAGMENT_BIT;
```

`make_compute` lambda (`main.cpp:1058-1066`):

```cpp:particle-fluids/sph-water/src/main.cpp:1058-1066
    auto make_compute = [&](const std::string& shader_rel,
                            std::initializer_list<gv::DescriptorBinding> bindings,
                            std::uint32_t push_const_bytes = 0) {
        gv::ComputePipelineDesc d{};
        d.shader_path        = SD + "/" + shader_rel;
        d.bindings           = std::vector<gv::DescriptorBinding>(bindings);
        d.push_constant_size = push_const_bytes;
        return gv::ComputePipeline::create(ctx, compiler, d);
    };
```

Push-constant size matrix:
- `pipe_density_alpha`:   0 (no push constant)
- `pipe_divergence_solve`: 0
- `pipe_density_solve`:    0
- `pipe_integrate_forces`: `sizeof(std::uint32_t)` = 4 (mode = 0 or 1)
- `pipe_pressure_apply`:   0

### E.2 Descriptor-set-write functions (full bodies)

**`writeDensityAlphaDescriptor`** (`main.cpp:528-553`):

```cpp:particle-fluids/sph-water/src/main.cpp:528-553
static void writeDensityAlphaDescriptor(VkDevice device,
                                        VkDescriptorSet ds,
                                        VkBuffer particles,
                                        VkBuffer cell_starts,
                                        VkBuffer sorted_index,
                                        VkBuffer density_alpha,
                                        VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=particles;       b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=cell_starts;     b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b2{}; b2.buffer=sorted_index;    b2.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b3{}; b3.buffer=density_alpha;   b3.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 5> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[2].pBufferInfo=&b2;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[3].pBufferInfo=&b3;
    w[4].dstSet=ds; w[4].dstBinding=4; w[4].descriptorCount=1;
    w[4].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[4].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}
```

**`writeDfsphSolveDescriptor`** (shared by `divergence_solve` and `density_solve`, `main.cpp:555-590`):

```cpp:particle-fluids/sph-water/src/main.cpp:555-590
// Shared by divergence_solve + density_solve (identical binding layout).
// Caller supplies the inner-iter ping-pong by flipping pressure_read/_write.
static void writeDfsphSolveDescriptor(VkDevice device,
                                      VkDescriptorSet ds,
                                      VkBuffer particles,
                                      VkBuffer density_alpha,
                                      VkBuffer cell_starts,
                                      VkBuffer sorted_index,
                                      VkBuffer pressure_read,
                                      VkBuffer pressure_write,
                                      VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=particles;       b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=density_alpha;   b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b2{}; b2.buffer=cell_starts;     b2.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b3{}; b3.buffer=sorted_index;    b3.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b4{}; b4.buffer=pressure_read;   b4.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b5{}; b5.buffer=pressure_write;  b5.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 7> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[2].pBufferInfo=&b2;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[3].pBufferInfo=&b3;
    w[4].dstSet=ds; w[4].dstBinding=4; w[4].descriptorCount=1;
    w[4].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[4].pBufferInfo=&b4;
    w[5].dstSet=ds; w[5].dstBinding=5; w[5].descriptorCount=1;
    w[5].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[5].pBufferInfo=&b5;
    w[6].dstSet=ds; w[6].dstBinding=6; w[6].descriptorCount=1;
    w[6].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[6].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}
```

**`writeIntegrateForcesDescriptor`** (`main.cpp:592-617`):

```cpp:particle-fluids/sph-water/src/main.cpp:592-617
static void writeIntegrateForcesDescriptor(VkDevice device,
                                           VkDescriptorSet ds,
                                           VkBuffer particles,
                                           VkBuffer density_alpha,
                                           VkBuffer cell_starts,
                                           VkBuffer sorted_index,
                                           VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=particles;       b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=density_alpha;   b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b2{}; b2.buffer=cell_starts;     b2.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b3{}; b3.buffer=sorted_index;    b3.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 5> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[2].pBufferInfo=&b2;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[3].pBufferInfo=&b3;
    w[4].dstSet=ds; w[4].dstBinding=4; w[4].descriptorCount=1;
    w[4].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[4].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}
```

**`writePressureApplyDescriptor`** (`main.cpp:619-648`):

```cpp:particle-fluids/sph-water/src/main.cpp:619-648
static void writePressureApplyDescriptor(VkDevice device,
                                         VkDescriptorSet ds,
                                         VkBuffer particles,
                                         VkBuffer density_alpha,
                                         VkBuffer pressure_read,
                                         VkBuffer cell_starts,
                                         VkBuffer sorted_index,
                                         VkBuffer uniform_buffer) {
    VkDescriptorBufferInfo b0{}; b0.buffer=particles;       b0.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b1{}; b1.buffer=density_alpha;   b1.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b2{}; b2.buffer=pressure_read;   b2.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b3{}; b3.buffer=cell_starts;     b3.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo b4{}; b4.buffer=sorted_index;    b4.range=VK_WHOLE_SIZE;
    VkDescriptorBufferInfo u_i{};u_i.buffer=uniform_buffer; u_i.range=VK_WHOLE_SIZE;
    std::array<VkWriteDescriptorSet, 6> w{};
    for (auto& wi : w) wi.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet=ds; w[0].dstBinding=0; w[0].descriptorCount=1;
    w[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[0].pBufferInfo=&b0;
    w[1].dstSet=ds; w[1].dstBinding=1; w[1].descriptorCount=1;
    w[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[1].pBufferInfo=&b1;
    w[2].dstSet=ds; w[2].dstBinding=2; w[2].descriptorCount=1;
    w[2].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[2].pBufferInfo=&b2;
    w[3].dstSet=ds; w[3].dstBinding=3; w[3].descriptorCount=1;
    w[3].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[3].pBufferInfo=&b3;
    w[4].dstSet=ds; w[4].dstBinding=4; w[4].descriptorCount=1;
    w[4].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; w[4].pBufferInfo=&b4;
    w[5].dstSet=ds; w[5].dstBinding=5; w[5].descriptorCount=1;
    w[5].descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w[5].pBufferInfo=&u_i;
    vkUpdateDescriptorSets(device, uint32_t(w.size()), w.data(), 0, nullptr);
}
```

### E.3 Descriptor-set allocation (where `vkAllocateDescriptorSets` is called)

The project's helper is `ComputePipeline::allocateDescriptorSet()` (defined in common-cpp); it pulls from each pipeline's internal `ds_pool_`. From `common/common-cpp/src/vk/compute_pipeline.cpp:238-250`:

```cpp:common/common-cpp/src/vk/compute_pipeline.cpp:238-250
VkDescriptorSet ComputePipeline::allocateDescriptorSet() {
    VkDescriptorSetAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool     = ds_pool_;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts        = &ds_layout_;
    VkDescriptorSet ds = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(ctx_->device(), &ai, &ds) != VK_SUCCESS) {
        logError("ComputePipeline: vkAllocateDescriptorSets failed");
        return VK_NULL_HANDLE;
    }
    return ds;
}
```

The pool itself is built in `ComputePipeline::create` via `buildDescriptorPool(...)` at `compute_pipeline.cpp:37-59` with `max_sets = 16` (passed at `:130`):

```cpp:common/common-cpp/src/vk/compute_pipeline.cpp:130
    p.ds_pool_ = buildDescriptorPool(ctx.device(), desc.bindings, /*max_sets=*/16);
```

Per-DFSPH-pipeline allocation in `main.cpp` (`:1175-1185`):

```cpp:particle-fluids/sph-water/src/main.cpp:1175-1185
    VkDescriptorSet ds_density_alpha    = pipe_density_alpha.allocateDescriptorSet();
    VkDescriptorSet ds_divergence_solve[2] = {
        pipe_divergence_solve.allocateDescriptorSet(),
        pipe_divergence_solve.allocateDescriptorSet(),
    };
    VkDescriptorSet ds_density_solve[2] = {
        pipe_density_solve.allocateDescriptorSet(),
        pipe_density_solve.allocateDescriptorSet(),
    };
    VkDescriptorSet ds_integrate_forces = pipe_integrate_forces.allocateDescriptorSet();
    VkDescriptorSet ds_pressure_apply   = pipe_pressure_apply.allocateDescriptorSet();
```

## Section F: Host-readback patterns in the repo

### F.1 `vkMapMemory`

Grep for `vkMapMemory` across `common/common-cpp` and `particle-fluids` returned **zero hits**. The project does not call `vkMapMemory` directly — mapping is delegated to VMA via `VMA_ALLOCATION_CREATE_MAPPED_BIT`.

### F.2 VMA mapping flags

Both flags are used inside `common/common-cpp/src/vk/buffer.cpp:25-34`:

```cpp:common/common-cpp/src/vk/buffer.cpp:19-37
VmaAllocationCreateInfo makeAllocInfo(MemoryUsage mem) {
    VmaAllocationCreateInfo ai{};
    switch (mem) {
        case MemoryUsage::DeviceLocal:
            ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;
        case MemoryUsage::HostVisibleSequential:
            ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            ai.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
        case MemoryUsage::HostVisibleRandom:
            ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            ai.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
    }
    return ai;
}
```

### F.3 `vkCmdCopyBuffer` targeting host-visible buffer

The single device→host copy path in the repo is `Buffer::readback` (`common/common-cpp/src/vk/buffer.cpp:135-154`):

```cpp:common/common-cpp/src/vk/buffer.cpp:135-154
void Buffer::readback(Context& ctx, void* dst, std::size_t bytes, std::size_t offset) {
    assert(dst && "readback dst must be non-null");
    assert(offset + bytes <= size_ && "readback range out of bounds");

    Buffer staging = Buffer::create(ctx, bytes,
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    MemoryUsage::HostVisibleRandom,
                                    "buffer-readback");

    ctx.runOneShot([&](VkCommandBuffer cb) {
        VkBufferCopy region{};
        region.srcOffset = offset;
        region.dstOffset = 0;
        region.size      = bytes;
        vkCmdCopyBuffer(cb, buffer_, staging.handle(), 1, &region);
    });

    vmaInvalidateAllocation(ctx.allocator(), staging.allocation_, 0, bytes);
    std::memcpy(dst, staging.mapped(), bytes);
}
```

The dual host→device path is `Buffer::stage` (`buffer.cpp:119-133`):

```cpp:common/common-cpp/src/vk/buffer.cpp:119-133
void Buffer::stage(Context& ctx, const void* src, std::size_t bytes, std::size_t offset) {
    Buffer staging = Buffer::create(ctx, bytes,
                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                    MemoryUsage::HostVisibleSequential,
                                    "buffer-stage");
    staging.uploadDirect(src, bytes, 0);

    ctx.runOneShot([&](VkCommandBuffer cb) {
        VkBufferCopy region{};
        region.srcOffset = 0;
        region.dstOffset = offset;
        region.size      = bytes;
        vkCmdCopyBuffer(cb, staging.handle(), buffer_, 1, &region);
    });
}
```

Header declaration (`common/common-cpp/include/gpusims/vk/buffer.hpp:54-59`):

```cpp:common/common-cpp/include/gpusims/vk/buffer.hpp:54-59
    // Symmetric counterpart to stage(): copy bytes out of a DeviceLocal buffer
    // into host memory. Allocates a transient host-visible staging buffer,
    // submits a vkCmdCopyBuffer on the graphics queue, waits, and memcpys
    // the staging contents into dst. Synchronous. Phase 11 sph-water consumer
    // for F5 capture-save + Alembic-export per-frame readback.
    void readback(Context& ctx, void* dst, std::size_t bytes, std::size_t offset = 0);
```

### F.4 Consumers of `Buffer::readback` in sph-water

Two call sites in `main.cpp`. F5-capture-save batch (`main.cpp:1684-1689`):

```cpp:particle-fluids/sph-water/src/main.cpp:1684-1689
        tier.particles.readback(ctx,     particles_bytes.data(),    particles_bytes.size());
        tier.density_alpha.readback(ctx, density_alpha_bytes.data(),density_alpha_bytes.size());
        tier.pressure_a.readback(ctx,    pressure_bytes.data(),     pressure_bytes.size());
        tier.morton_codes.readback(ctx,  morton_bytes.data(),       morton_bytes.size());
        tier.sorted_index.readback(ctx,  sorted_bytes.data(),       sorted_bytes.size());
        tier.cell_starts.readback(ctx,   cell_starts_bytes.data(),  cell_starts_bytes.size());
```

Alembic export (`main.cpp:2045-2053`):

```cpp:particle-fluids/sph-water/src/main.cpp:2045-2053
        // We readback the full particle SoA and unpack pos@offset 0 + vel@16.
[...]
            tier.particles.readback(ctx, bytes.data(), bytes.size());
```

### F.5 Image readback

There is also `Image::readback` at `common/common-cpp/src/vk/image.cpp:202-244` (same staging-buffer pattern, ending in `vmaInvalidateAllocation` + `std::memcpy`). Not load-bearing for solver convergence but present in the surface.

### F.6 `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` outside UBO uploads

Grep returns **zero direct uses** of the `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` enum in either `common/common-cpp` or `particle-fluids`. Host-visible memory is always selected via VMA's `MemoryUsage::HostVisibleSequential` / `HostVisibleRandom`.

### F.7 Direct answer to caller's question

**`tier.uniform_dfsph.uploadDirect` is host→device only** (mapping is `VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT`; the destination is the host-mapped UBO, GPU reads it). `Buffer::stage` is also host→device. **The only device→host path is `Buffer::readback`** (`buffer.cpp:135`) and **`Image::readback`** (`image.cpp:202`); both are **synchronous, submit-and-wait** via `ctx.runOneShot`. There is **no asynchronous device→host helper**, no persistent-mirror pattern, and no `gv::downloadBuffer` / equivalent free function. The class-level helper `Buffer::readback(Context&, void*, std::size_t, std::size_t)` is the only thing available.

## Section G: Atomic operations in existing shaders

`grep` across all `.comp.glsl` in `particle-fluids/sph-water/shaders/`:

```
particle-fluids/sph-water/shaders/cell_count.comp.glsl:1:// cell_count.comp.glsl — atomicAdd 1 to per-cell count for each particle.
particle-fluids/sph-water/shaders/cell_count.comp.glsl:23:    atomicAdd(counts[key], 1u);
particle-fluids/sph-water/shaders/scatter.comp.glsl:31:    uint slot_in_cell = atomicAdd(atomic_counts[key], 1u);
```

Only two hits, both `atomicAdd`. No `atomicMin`, `atomicMax`, `atomicCompSwap`, `atomicOr`, `atomicAnd` anywhere.

### G.1 `cell_count.comp.glsl` (full file, 24 lines)

```glsl:particle-fluids/sph-water/shaders/cell_count.comp.glsl:1-24
// cell_count.comp.glsl — atomicAdd 1 to per-cell count for each particle.
// Translated from agent-based/boids-3d/web/shaders/cell_count.compute.wgsl.
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict readonly buffer MortonCodes {
    uint codes[];
};
layout(set=0, binding=1, std430) restrict buffer CellCounts {
    uint counts[];
};
layout(set=0, binding=2, std140) uniform U {
    uint particleCount;
    uint _pad0; uint _pad1; uint _pad2;
    vec4 _pad3; vec4 _pad4; vec4 _pad5;
};

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;
    uint key = codes[gid];
    atomicAdd(counts[key], 1u);
}
```

### G.2 `scatter.comp.glsl` (full file, 33 lines)

```glsl:particle-fluids/sph-water/shaders/scatter.comp.glsl:1-33
// scatter.comp.glsl — Each particle looks up its Morton-cell start, atomically
// claims a slot via cell_counts_atomic, writes its original index to sorted.
// Translated from agent-based/boids-3d/web/shaders/scatter.compute.wgsl.
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict readonly buffer MortonCodes {
    uint codes[];
};
layout(set=0, binding=1, std430) restrict readonly buffer CellStarts {
    uint cell_starts[];
};
layout(set=0, binding=2, std430) restrict writeonly buffer SortedIndex {
    uint sorted[];
};
layout(set=0, binding=3, std430) restrict buffer CellCountsAtomic {
    uint atomic_counts[];
};
layout(set=0, binding=4, std140) uniform U {
    uint particleCount;
    uint _pad0; uint _pad1; uint _pad2;
    vec4 _pad3; vec4 _pad4; vec4 _pad5;
};

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;

    uint key          = codes[gid];
    uint slot_in_cell = atomicAdd(atomic_counts[key], 1u);
    sorted[cell_starts[key] + slot_in_cell] = gid;
}
```

### G.3 `#version` and `#extension` line inventory

All 16 compute shaders' headers (verbatim):

| Shader | `#version` line | `#extension` line(s) |
|---|---|---|
| `apply_emitter.comp.glsl:3` | `#version 460` | (none) |
| `bilateral_smooth.comp.glsl:5` | `#version 460` | (none) |
| `cell_count.comp.glsl:3` | `#version 460` | (none) |
| `density_alpha.comp.glsl:8` | `#version 460` | (none) |
| `density_solve.comp.glsl:14` | `#version 460` | (none) |
| `divergence_solve.comp.glsl:12` | `#version 460` | (none) |
| `initial_fill.comp.glsl:3` | `#version 460` | (none) |
| `integrate_forces.comp.glsl:7` | `#version 460` | (none) |
| `morton_code.comp.glsl:3-4` | `#version 460` | `#extension GL_GOOGLE_include_directive : enable` |
| `prefix_sum_addback.comp.glsl:4` | `#version 460` | (none) |
| `prefix_sum_block.comp.glsl:10` | `#version 460` | (none) |
| `prefix_sum_block_l2.comp.glsl:4` | `#version 460` | (none) |
| `prefix_sum_local.comp.glsl:3` | `#version 460` | (none) |
| `pressure_apply.comp.glsl:10` | `#version 460` | (none) |
| `scatter.comp.glsl:4` | `#version 460` | (none) |

**No `GL_KHR_shader_subgroup_arithmetic` or `_basic` extension declared in any shader.** No subgroup intrinsics in any DFSPH shader. (The subgroup-size *pin* applies to the 7 sort kernels + bilateral kernel via the pipeline-creation pNext chain, but the GLSL itself does not use subgroup ops.)

## Section H: Full `pack_dfsph_uniform` body and call site

### H.1 Function body (`main.cpp:1380-1431`)

```cpp:particle-fluids/sph-water/src/main.cpp:1380-1431
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

### H.2 Call site

Probe-1 referenced `main.cpp:1891`. **Verified.** The call is at line 1891. Five lines before and after:

```cpp:particle-fluids/sph-water/src/main.cpp:1886-1896
        // Per-frame uniform packing (host-visible UBOs).
        // ----------------------------------------------------------------
        const float substep_dt = std::clamp(frame_dt / float(std::max(rt.substeps, 1)), DT_MIN, DT_MAX);
        rt.dt = substep_dt;
        pack_sort_uniform();
        pack_dfsph_uniform(substep_dt);
        pack_render_view_uniform();
        pack_composite_uniform();
        pack_apply_emitter_uniform(substep_dt);

        // ----------------------------------------------------------------
        // DFSPH dispatch chain (§ 4.G), once per substep.
```

`pack_dfsph_uniform` is called **once per frame, outside the substep loop**. Probe-1 Section A confirms this position (frame-scope uniform write, before the substep loop opens at `main.cpp:1908`).

## Section I: Usage of `da[].z`, `da[].w`, `predicted_density`, `density_advect`

### I.1 Per-shader summary of `da[...]` accesses

`grep -n "da\[" *.comp.glsl` results (with read-vs-write tagged from shader-side `restrict readonly/writeonly` qualifier):

| Shader | Buffer qualifier | Access site | Component | Read/Write |
|---|---|---|---|---|
| `density_alpha.comp.glsl:21-24` | `restrict writeonly buffer DensityAlpha { vec4 da[]; }` | `:126` `da[gid] = vec4(density, alpha_stored, 0.0, 0.0);` | full `.xyzw` | **WRITE** (only writer of `da`) |
| `divergence_solve.comp.glsl:17` | `restrict readonly buffer DensityAlpha { vec4 da[]; }` | `:75` `da[gid].x` | `.x` | READ |
| `divergence_solve.comp.glsl:17` | (same) | `:76` `da[gid].y` | `.y` | READ |
| `density_solve.comp.glsl:19` | `restrict readonly buffer DensityAlpha { vec4 da[]; }` | `:77` `da[gid].x` | `.x` | READ |
| `density_solve.comp.glsl:19` | (same) | `:78` `da[gid].y` | `.y` | READ |
| `integrate_forces.comp.glsl:12` | `restrict readonly buffer DensityAlpha { vec4 da[]; }` | `:111` `max(da[j].x, 1e-3)` | `.x` (neighbor) | READ |
| `pressure_apply.comp.glsl:15` | `restrict readonly buffer DensityAlpha { vec4 da[]; }` | `:72` `da[gid].x` | `.x` | READ |
| `pressure_apply.comp.glsl:15` | (same) | `:95` `da[j].x` | `.x` (neighbor) | READ |

### I.2 Direct answer to caller's question

**No kernel writes `da[gid].z` or `da[gid].w`.** The only writer of `da` is `density_alpha.comp.glsl:126`, which writes `vec4(density, alpha_stored, 0.0, 0.0)` — the `.z` and `.w` slots are unconditionally zeroed every dispatch. No kernel reads `da[].z` or `da[].w` either.

The struct-layout doc reserves these slots:

```text:particle-fluids/sph-water/shaders/_struct_layouts.txt:53-54
.z    8       4     predicted_density     rho_adv_i (density_solve Pass 1)
.w    12      4     density_advect        rho_dot_i (divergence_solve Pass 1)
```

…but the corresponding `predicted_density` / `density_advect` fields are **never written and never read by any shader.** Likewise, `_struct_layouts.txt:28-29` documents per-particle slots `predicted_density` / `density_advect` at offsets 56/60 inside `particles_buf`, but those are explicitly marked `UNUSED; in da[i].z` / `UNUSED; in da[i].w` — so neither location holds the value either. **Both predicted-density and density-advect are documentation-only fields in the current code.**

Grep confirms: across all `*.comp.glsl` in `particle-fluids/sph-water/shaders/`, no shader source contains the tokens `predicted_density`, `density_advect`, `rho_adv`, or `rho_dot` as variable/field names — the term `rho_adv` does appear locally in `density_solve.comp.glsl:85, :110, :111` as a stack-local scalar (not stored back to the SSBO):

```glsl:particle-fluids/sph-water/shaders/density_solve.comp.glsl:85-111
    // Pass 1: rho_adv = density_i + dt · Σ m (v_i − v_j) · ∇W;  s_i = 1 - rho_adv/ρ₀.
    float rho_dot = 0.0;
[...]
    float rho_adv = density_i + dt * rho_dot;
    float s_i     = min(1.0 - rho_adv / max(density0, 1e-7), 0.0);
```

So `rho_adv` is computed inline each iteration from `density_i` (the `density_alpha` snapshot) plus a fresh `rho_dot` neighbor pass; it is never persisted across kernel invocations.

## Section J: Compute pipeline creation patterns

### J.1 `ComputePipeline::create` full body

(See **Section E** for the binding-build helpers; this section quotes the create function itself.)

```cpp:common/common-cpp/src/vk/compute_pipeline.cpp:63-133
ComputePipeline ComputePipeline::create(Context&                   ctx,
                                        ShaderCompiler&            compiler,
                                        const ComputePipelineDesc& desc) {
    ComputePipeline p;
    p.ctx_  = &ctx;
    p.desc_ = desc;

    auto compile = compiler.compileFile(desc.shader_path, ShaderStage::Compute);
    if (!compile.ok) {
        throw std::runtime_error("ComputePipeline: shader compile failed: " + compile.error);
    }
    p.last_includes_ = compile.includes;

    p.shader_module_ = ShaderCompiler::createShaderModule(ctx.device(), compile.spirv);
    if (p.shader_module_ == VK_NULL_HANDLE) {
        throw std::runtime_error("ComputePipeline: createShaderModule failed");
    }

    p.ds_layout_ = buildDescriptorSetLayout(ctx.device(), desc.bindings);

    VkPipelineLayoutCreateInfo lci{};
    lci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    lci.setLayoutCount         = 1;
    lci.pSetLayouts            = &p.ds_layout_;
    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcr.offset     = 0;
    pcr.size       = desc.push_constant_size;
    if (desc.push_constant_size > 0) {
        lci.pushConstantRangeCount = 1;
        lci.pPushConstantRanges    = &pcr;
    }
    if (vkCreatePipelineLayout(ctx.device(), &lci, nullptr, &p.pipeline_layout_) != VK_SUCCESS) {
        throw std::runtime_error("ComputePipeline: vkCreatePipelineLayout failed");
    }

    VkPipelineShaderStageCreateInfo ss{};
    ss.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ss.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    ss.module = p.shader_module_;
    ss.pName  = "main";

    // Phase 11 sph-water: subgroup-size-control extension. See INVARIANT in
    // compute_pipeline.hpp. The extension struct is stack-allocated; its
    // lifetime must span vkCreateComputePipelines below, so it lives in the
    // enclosing scope.
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroup_size_ci{};
    if (desc.required_subgroup_size != 0) {
        subgroup_size_ci.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
        subgroup_size_ci.requiredSubgroupSize = desc.required_subgroup_size;
        subgroup_size_ci.pNext = const_cast<void*>(ss.pNext);
        ss.pNext = &subgroup_size_ci;
    }
    if (desc.require_full_subgroups) {
        ss.flags |= VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT;
    }

    VkComputePipelineCreateInfo cpi{};
    cpi.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpi.stage  = ss;
    cpi.layout = p.pipeline_layout_;
    if (vkCreateComputePipelines(ctx.device(), VK_NULL_HANDLE, 1, &cpi, nullptr, &p.pipeline_)
        != VK_SUCCESS) {
        throw std::runtime_error("ComputePipeline: vkCreateComputePipelines failed");
    }

    p.ds_pool_ = buildDescriptorPool(ctx.device(), desc.bindings, /*max_sets=*/16);

    return p;
}
```

### J.2 `ComputePipelineDesc` header (with invariant)

```cpp:common/common-cpp/include/gpusims/vk/compute_pipeline.hpp:31-53
struct ComputePipelineDesc {
    std::filesystem::path        shader_path;       // for hot-reload tracking
    std::vector<DescriptorBinding> bindings;
    std::uint32_t                push_constant_size = 0;       // bytes; 0 = none

    // Phase 11 sph-water: subgroup-size-control fields. Both default to
    // sentinel "unconstrained" values; the existing default-null pNext-chain
    // path is preserved when both fields are at their defaults.
    //
    // INVARIANT (must be preserved by all future maintainers):
    //   If required_subgroup_size != 0 OR require_full_subgroups == true,
    //   compute_pipeline.cpp builds VkPipelineShaderStageRequiredSubgroupSize
    //   CreateInfo and chains it into VkPipelineShaderStageCreateInfo.pNext
    //   (and/or sets the REQUIRE_FULL_SUBGROUPS_BIT flag). Otherwise pNext
    //   stays null and the flag stays zero.
    //
    //   Collapsing the conditional (always building the extension struct
    //   regardless of values) would force every compute pipeline to carry the
    //   subgroup-size extension, breaking compatibility with drivers/devices
    //   that don't support VK_EXT_subgroup_size_control.
    std::uint32_t                required_subgroup_size = 0;     // 0 = unconstrained
    bool                         require_full_subgroups = false; // false = unconstrained
};
```

### J.3 `dispatch` method (single combined bind+dispatch)

```cpp:common/common-cpp/src/vk/compute_pipeline.cpp:252-269
void ComputePipeline::dispatch(VkCommandBuffer cmd,
                               VkDescriptorSet ds,
                               std::uint32_t   gx,
                               std::uint32_t   gy,
                               std::uint32_t   gz,
                               const void*     push_constants,
                               std::uint32_t   push_size) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
    if (ds != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_,
                                0, 1, &ds, 0, nullptr);
    }
    if (push_constants && push_size > 0) {
        vkCmdPushConstants(cmd, pipeline_layout_,
                           VK_SHADER_STAGE_COMPUTE_BIT, 0, push_size, push_constants);
    }
    vkCmdDispatch(cmd, gx, gy, gz);
}
```

### J.4 `reload` method (hot-reload integration)

```cpp:common/common-cpp/src/vk/compute_pipeline.cpp:174-236
bool ComputePipeline::reload(Context&        ctx,
                             ShaderCompiler& compiler,
                             Frame&          current_frame,
                             std::string*    out_error) {
    auto compile = compiler.compileFile(desc_.shader_path, ShaderStage::Compute);
    if (!compile.ok) {
        if (out_error) *out_error = compile.error;
        return false;
    }

    auto new_module = ShaderCompiler::createShaderModule(ctx.device(), compile.spirv);
    if (new_module == VK_NULL_HANDLE) {
        if (out_error) *out_error = "createShaderModule failed";
        return false;
    }

    VkPipelineShaderStageCreateInfo ss{};
    ss.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ss.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    ss.module = new_module;
    ss.pName  = "main";

    // Phase 11 sph-water: preserve subgroup-size pin on hot reload. Same
    // conditional invariant as ComputePipeline::create (see header).
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroup_size_ci{};
    if (desc_.required_subgroup_size != 0) {
        subgroup_size_ci.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
        subgroup_size_ci.requiredSubgroupSize = desc_.required_subgroup_size;
        subgroup_size_ci.pNext = const_cast<void*>(ss.pNext);
        ss.pNext = &subgroup_size_ci;
    }
    if (desc_.require_full_subgroups) {
        ss.flags |= VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT;
    }

    VkComputePipelineCreateInfo cpi{};
    cpi.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpi.stage  = ss;
    cpi.layout = pipeline_layout_;

    VkPipeline new_pipeline = VK_NULL_HANDLE;
    if (vkCreateComputePipelines(ctx.device(), VK_NULL_HANDLE, 1, &cpi, nullptr, &new_pipeline)
        != VK_SUCCESS) {
        vkDestroyShaderModule(ctx.device(), new_module, nullptr);
        if (out_error) *out_error = "vkCreateComputePipelines failed";
        return false;
    }

    // Defer destruction of old resources until in-flight frames finish.
    VkDevice       device     = ctx.device();
    VkPipeline     old_pipe   = pipeline_;
    VkShaderModule old_module = shader_module_;
    current_frame.deletion_queue.push([device, old_pipe, old_module]() {
        if (old_pipe   != VK_NULL_HANDLE) vkDestroyPipeline(device, old_pipe,   nullptr);
        if (old_module != VK_NULL_HANDLE) vkDestroyShaderModule(device, old_module, nullptr);
    });

    pipeline_      = new_pipeline;
    shader_module_ = new_module;
    last_includes_ = compile.includes;
    return true;
}
```

### J.5 Push-constant range wiring

A single `VkPushConstantRange` is built at `compute_pipeline.cpp:87-94`:

```cpp:common/common-cpp/src/vk/compute_pipeline.cpp:87-94
    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcr.offset     = 0;
    pcr.size       = desc.push_constant_size;
    if (desc.push_constant_size > 0) {
        lci.pushConstantRangeCount = 1;
        lci.pPushConstantRanges    = &pcr;
    }
```

- Stage is hard-coded to `VK_SHADER_STAGE_COMPUTE_BIT`.
- Offset is hard-coded to 0.
- Size is `desc.push_constant_size` bytes.
- Only one range is ever built; the wrapper does not support multiple push-constant ranges in the same compute pipeline.

### J.6 Specialization constants

**None.** Grep for `specialization` / `SpecializationConstant` / `constant_id` returns no matches in `common/common-cpp`. `VkPipelineShaderStageCreateInfo::pSpecializationInfo` is left null (never assigned) in both `create()` and `reload()`. Specialization constants are not wired through the `ComputePipelineDesc` surface.

### J.7 Hot-reload integration

The reload mechanism is rooted in `common/common-cpp/src/hot_reload.cpp` (via `gpusims::HotReloader`); the sph-water side registers watchers at `main.cpp:1637-1661` (quoted in Section M). On a file-modified event, the worker thread sets a per-shader `bool reload_*` flag; the main loop calls `pipe.reload(ctx, compiler, *frame, &err)` (`main.cpp:1854`). Old `VkPipeline` and `VkShaderModule` handles are pushed onto `frame->deletion_queue` so they are destroyed only after in-flight execution has consumed them (`compute_pipeline.cpp:223-230`).

### J.8 Pipeline-layout sharing across pipelines

Each `ComputePipeline` owns its own `VkPipelineLayout`, `VkDescriptorSetLayout`, and `VkDescriptorPool`. The wrapper does **not** expose a way for two pipelines to share a layout — `create()` always calls `buildDescriptorSetLayout` and `vkCreatePipelineLayout` (`compute_pipeline.cpp:81-97`). `divergence_solve` and `density_solve` share the *write-helper function* (`writeDfsphSolveDescriptor`) and an identical binding shape, but they are separate `ComputePipeline` instances with separate VK layouts (see `main.cpp:1097-1100`).

### J.9 "Pipeline writes a host-visible buffer" flag

**None.** `ComputePipeline` / `ComputePipelineDesc` have no field, flag, or accessor encoding host-visibility of attached buffers. Memory-residency is a property of the `Buffer` (its `MemoryUsage` enum value), not the pipeline.

## Section K: `pressure_apply.comp.glsl` for reference

Full file (110 lines), verbatim.

```glsl:particle-fluids/sph-water/shaders/pressure_apply.comp.glsl:1-110
// pressure_apply.comp.glsl — Apply pressure-derived velocity correction.
//
// Shared by both DFSPH inner-loops (divergence + density). Same kernel, different
// pressure buffer bound per call.
//
// References:
//   Velocity correction: SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:514-515 (divergence)
//                                                              :359-360 (density)
//   The "h" multiplier in upstream code is dt (time step), NOT support radius.
#version 460

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

uint expand_bits_10(uint v) {
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}
uint morton_encode_3d(uvec3 c) {
    return (expand_bits_10(c.x) << 2) | (expand_bits_10(c.y) << 1) | expand_bits_10(c.z);
}
vec3 kernel_gradW(vec3 r_ij, float r_mag, float q, float grad_kernel_norm) {
    float poly = 0.0;
    if (q < 0.5)      poly = 18.0*q*q - 12.0*q;
    else if (q < 1.0) { float omq = 1.0 - q; poly = -6.0 * omq * omq; }
    return (grad_kernel_norm * poly / r_mag) * r_ij;
}

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

## Section L: Other DFSPH-related `.comp.glsl` helpers

Full inventory of `.comp.glsl` in `particle-fluids/sph-water/shaders/`:

```
apply_emitter.comp.glsl
bilateral_smooth.comp.glsl
cell_count.comp.glsl
density_alpha.comp.glsl
density_solve.comp.glsl
divergence_solve.comp.glsl
initial_fill.comp.glsl
integrate_forces.comp.glsl
morton_code.comp.glsl
prefix_sum_addback.comp.glsl
prefix_sum_block.comp.glsl
prefix_sum_block_l2.comp.glsl
prefix_sum_local.comp.glsl
pressure_apply.comp.glsl
scatter.comp.glsl
```

Probe-1 Section E inventoried the five DFSPH-core shaders (`density_alpha`, `divergence_solve`, `density_solve`, `integrate_forces`, `pressure_apply`).

### L.1 Search for missing DFSPH helpers

```
find ... -name "density_predict*" -o -name "density_advance*" -o -name "divergence_reduce*" -o -name "density_error*" -o -name "solver_*"
```

**No output.** None of `density_predict`, `density_advance`, `divergence_reduce`, `density_error`, `solver_*` exist as shader files. There are no error-measurement / convergence-residual / predicted-density-pass kernels.

### L.2 Top-of-file docblocks + binding layouts for non-core compute shaders

**`apply_emitter.comp.glsl`** — docblock (verbatim):

```glsl:particle-fluids/sph-water/shaders/apply_emitter.comp.glsl:1-2
// apply_emitter.comp.glsl — Reserve-tail emitter inject. Each thread spawns one
// particle into the [particleCount, particleCapacity) tail region.
```

Binding layout: `set=0, binding=0` storage buffer `Particles`; `set=0, binding=1` std140 uniform `U`. (No `da` access.)

**`bilateral_smooth.comp.glsl`** — docblock:

```glsl:particle-fluids/sph-water/shaders/bilateral_smooth.comp.glsl:1-4
// bilateral_smooth.comp.glsl — Separable bilateral filter for screen-space
// fluid depth smoothing (Müller-Fetterer 2007 / Eisemann-Décoret 2006).
// Dispatched twice per smoothing iteration: passDirection=0 (horizontal),
// passDirection=1 (vertical). Vulkan separate-textures-and-samplers layout.
```

Binding layout: `set=0, binding=0` `texture2D inputDepth`; `set=0, binding=1` `r32f writeonly image2D outputDepth`; `set=0, binding=2` `sampler samp`; `set=0, binding=3` std140 uniform `U`.

**`cell_count.comp.glsl`** — quoted in full in **Section G.1**.

**`initial_fill.comp.glsl`** — docblock:

```glsl:particle-fluids/sph-water/shaders/initial_fill.comp.glsl:1-2
// initial_fill.comp.glsl — Brick + optional droplet preset distribution.
// Called at preset-apply and on tier-change.
```

Binding layout: `set=0, binding=0` writeonly storage buffer `Particles`; `set=0, binding=1` std140 uniform `U`.

**`morton_code.comp.glsl`** — docblock:

```glsl:particle-fluids/sph-water/shaders/morton_code.comp.glsl:1-2
// morton_code.comp.glsl — 30-bit Morton code (10 bits per axis) per particle.
// Standard "magic numbers" bit-interleave (Karras 2012 fast BVH paper).
```

Binding layout: `set=0, binding=0` readonly storage buffer `Particles`; `set=0, binding=1` writeonly storage buffer `MortonCodes`; `set=0, binding=2` std140 uniform `U`. **Only shader with `#extension GL_GOOGLE_include_directive : enable`.**

**`prefix_sum_addback.comp.glsl`** — docblock:

```glsl:particle-fluids/sph-water/shaders/prefix_sum_addback.comp.glsl:1-3
// prefix_sum_addback.comp.glsl — Add each block's prefix into every element
// of that block's per-block-scanned data → final cell_starts.
// Translated from agent-based/boids-3d/web/shaders/prefix_sum_addback.compute.wgsl.
```

Binding layout: `set=0, binding=0` readonly buffer `PerBlock`; `set=0, binding=1` readonly buffer `BlockPrefixes`; `set=0, binding=2` writeonly buffer `CellStarts`; `set=0, binding=3` std140 uniform `U`.

**`prefix_sum_block.comp.glsl`** — docblock:

```glsl:particle-fluids/sph-water/shaders/prefix_sum_block.comp.glsl:1-9
// prefix_sum_block.comp.glsl — Two-mode kernel via push-constant.
//
// SCAN_ONLY: Blelloch exclusive scan over a chunk of block_sums. When the
// number of blocks exceeds WG_SIZE, the host dispatches a sequence of these
// in chunks of WG_SIZE; each chunk's total is written to OutL2Sums (for the
// recursive second-level scan in prefix_sum_block_l2.comp.glsl).
//
// ADDBACK_L2: add a per-chunk prefix (from prefix_sum_block_l2's output)
// onto each element of the corresponding chunk's local prefix.
```

Binding layout: `set=0, binding=0` buffer `InOutSums`; `set=0, binding=1` buffer `OutPrefixes`; `set=0, binding=2` buffer `OutL2Sums`; `set=0, binding=3` readonly buffer `InL2Prefixes`; `set=0, binding=4` std140 uniform `U`. (Uses 4-byte `uint mode` push constant.)

**`prefix_sum_block_l2.comp.glsl`** — docblock:

```glsl:particle-fluids/sph-water/shaders/prefix_sum_block_l2.comp.glsl:1-3
// prefix_sum_block_l2.comp.glsl — Second-level recursive scan over per-chunk
// totals produced by prefix_sum_block's SCAN_ONLY mode. Dispatched only when
// num_blocks > WG_SIZE; covers up to WG_SIZE^3 = 16M cells (~254 per axis).
```

Binding layout: `set=0, binding=0` readonly buffer `InL2Sums`; `set=0, binding=1` writeonly buffer `OutL2Prefixes`; `set=0, binding=2` std140 uniform `U`.

**`prefix_sum_local.comp.glsl`** — docblock:

```glsl:particle-fluids/sph-water/shaders/prefix_sum_local.comp.glsl:1-2
// prefix_sum_local.comp.glsl — Per-block Blelloch exclusive scan over cell_counts.
// Translated from agent-based/boids-3d/web/shaders/prefix_sum_local.compute.wgsl.
```

Binding layout: `set=0, binding=0` readonly buffer `InCounts`; `set=0, binding=1` writeonly buffer `OutPerBlockPrefixes`; `set=0, binding=2` writeonly buffer `OutBlockSums`; `set=0, binding=3` std140 uniform `U`.

**`scatter.comp.glsl`** — quoted in full in **Section G.2**.

### L.3 Summary

The five named-but-missing DFSPH helpers (`density_predict`, `density_advance`, `divergence_reduce`, `density_error`, `solver_*`) **do not exist on disk**. The shader directory contains only the five DFSPH-core kernels (probe-1 Section E) plus 1 emitter, 1 fill, 1 sort-rooting `morton_code`, 6 sort/scan kernels (`cell_count` + 4 `prefix_sum_*` + `scatter`), and 1 render-side `bilateral_smooth`. Total `.comp.glsl`: 15.

## Section M: Hot-reload and shader compilation surface

### M.1 Watch-list (`main.cpp:1623-1661`)

```cpp:particle-fluids/sph-water/src/main.cpp:1623-1661
    // Hot-reload registration (§ 4.E). Each flag is set by the worker thread
    // when a watched file changes; the main loop checks flags after poll()
    // and calls the corresponding pipeline's reload() on the next frame.
    // ------------------------------------------------------------------------
    bool reload_apply_emitter=false, reload_initial_fill=false;
    bool reload_morton_code=false, reload_cell_count=false;
    bool reload_prefix_sum_local=false, reload_prefix_sum_block=false;
    bool reload_prefix_sum_block_l2=false, reload_prefix_sum_addback=false;
    bool reload_scatter=false, reload_density_alpha=false;
    bool reload_divergence_solve=false, reload_density_solve=false;
    bool reload_integrate_forces=false, reload_pressure_apply=false;
    bool reload_bilateral_smooth=false;
    bool reload_depth=false, reload_thickness=false, reload_composite=false;

    auto W_watch = [&](const std::string& rel, bool* flag) {
        reloader.watch(SD + "/" + rel,
                       [flag](const std::filesystem::path&){ *flag = true; });
    };
    W_watch("apply_emitter.comp.glsl",       &reload_apply_emitter);
    W_watch("initial_fill.comp.glsl",        &reload_initial_fill);
    W_watch("morton_code.comp.glsl",         &reload_morton_code);
    W_watch("cell_count.comp.glsl",          &reload_cell_count);
    W_watch("prefix_sum_local.comp.glsl",    &reload_prefix_sum_local);
    W_watch("prefix_sum_block.comp.glsl",    &reload_prefix_sum_block);
    W_watch("prefix_sum_block_l2.comp.glsl", &reload_prefix_sum_block_l2);
    W_watch("prefix_sum_addback.comp.glsl",  &reload_prefix_sum_addback);
    W_watch("scatter.comp.glsl",             &reload_scatter);
    W_watch("density_alpha.comp.glsl",       &reload_density_alpha);
    W_watch("divergence_solve.comp.glsl",    &reload_divergence_solve);
    W_watch("density_solve.comp.glsl",       &reload_density_solve);
    W_watch("integrate_forces.comp.glsl",    &reload_integrate_forces);
    W_watch("pressure_apply.comp.glsl",      &reload_pressure_apply);
    W_watch("bilateral_smooth.comp.glsl",    &reload_bilateral_smooth);
    W_watch("particle_sprite.vert.glsl",     &reload_depth);
    W_watch("particle_sprite.frag.glsl",     &reload_depth);
    W_watch("thickness.vert.glsl",           &reload_thickness);
    W_watch("thickness.frag.glsl",           &reload_thickness);
    W_watch("fullscreen.vert.glsl",          &reload_composite);
    W_watch("composite.frag.glsl",           &reload_composite);
```

### M.2 Per-frame `try_reload` driver (`main.cpp:1848-1880`)

```cpp:particle-fluids/sph-water/src/main.cpp:1848-1880
        // Apply pending reloads now that the frame's deletion queue is alive.
        // Each call defers old VkPipeline / VkShaderModule destruction onto
        // this frame so it's safe under in-flight execution.
        auto try_reload = [&](auto& pipe, bool& flag, const char* name) {
            if (!flag) return;
            std::string err;
            if (pipe.reload(ctx, compiler, *frame, &err)) {
                logInfo("[sph-water] reloaded {}", name);
                reloader.reportSuccess(SD + "/" + name);
            } else {
                logError("[sph-water] reload {} failed: {}", name, err);
                reloader.reportFailure(SD + "/" + name, err);
            }
            flag = false;
        };
        try_reload(pipe_apply_emitter,       reload_apply_emitter,       "apply_emitter.comp.glsl");
        try_reload(pipe_initial_fill,        reload_initial_fill,        "initial_fill.comp.glsl");
        try_reload(pipe_morton_code,         reload_morton_code,         "morton_code.comp.glsl");
        try_reload(pipe_cell_count,          reload_cell_count,          "cell_count.comp.glsl");
        try_reload(pipe_prefix_sum_local,    reload_prefix_sum_local,    "prefix_sum_local.comp.glsl");
        try_reload(pipe_prefix_sum_block,    reload_prefix_sum_block,    "prefix_sum_block.comp.glsl");
        try_reload(pipe_prefix_sum_block_l2, reload_prefix_sum_block_l2, "prefix_sum_block_l2.comp.glsl");
        try_reload(pipe_prefix_sum_addback,  reload_prefix_sum_addback,  "prefix_sum_addback.comp.glsl");
        try_reload(pipe_scatter,             reload_scatter,             "scatter.comp.glsl");
        try_reload(pipe_density_alpha,       reload_density_alpha,       "density_alpha.comp.glsl");
        try_reload(pipe_divergence_solve,    reload_divergence_solve,    "divergence_solve.comp.glsl");
        try_reload(pipe_density_solve,       reload_density_solve,       "density_solve.comp.glsl");
        try_reload(pipe_integrate_forces,    reload_integrate_forces,    "integrate_forces.comp.glsl");
        try_reload(pipe_pressure_apply,      reload_pressure_apply,      "pressure_apply.comp.glsl");
        try_reload(pipe_bilateral_smooth,    reload_bilateral_smooth,    "bilateral_smooth.comp.glsl");
        try_reload(pipe_depth,               reload_depth,               "particle_sprite.{vert,frag}.glsl");
        try_reload(pipe_thickness,           reload_thickness,           "thickness.{vert,frag}.glsl");
        try_reload(pipe_composite,           reload_composite,           "composite.frag.glsl");
```

### M.3 Recompile function

The actual recompile happens inside `ComputePipeline::reload(...)` (`common/common-cpp/src/vk/compute_pipeline.cpp:174-236`, quoted in **Section J.4**). It calls `compiler.compileFile(desc_.shader_path, ShaderStage::Compute)`, builds a new `VkShaderModule`, builds a new `VkComputePipeline`, and defers the old pipeline + module destruction onto the current frame's `deletion_queue`.

The actual SPIR-V compilation lives at `common/common-cpp/src/vk/shader_compiler.cpp:117` — `ShaderCompiler::compileFile(...)`. Not quoted in full here; signature reference at `compute_pipeline.cpp:70` and `:178`.

## Section N: Existing convergence-check or error-measurement code

Grep across `particle-fluids/sph-water/` and `common/common-cpp/` for `error|residual|tolerance|converge|iterMax|iter_max|passCount|passes`.

### N.1 Runtime fields (declared, dead-ended)

`particle-fluids/sph-water/src/main.cpp:240-246`:

```cpp:particle-fluids/sph-water/src/main.cpp:240-246
    int       maxIterDensity         = DFSPH_MAX_ITER_DENSITY;
    int       maxIterDivergence      = DFSPH_MAX_ITER_DIV;
    int       minIterDensity         = DFSPH_MIN_ITER_DENSITY;
    int       minIterDivergence      = 1;
    float     maxErrorDensityPercent = DFSPH_MAX_ERROR_DENSITY;
    float     maxErrorDivPercent     = DFSPH_MAX_ERROR_DIV;
```

### N.2 Loop sites (`main.cpp`)

```cpp:particle-fluids/sph-water/src/main.cpp:1986
                int iters = std::max(rt.minIterDivergence, 1);
```

```cpp:particle-fluids/sph-water/src/main.cpp:2015
                int iters = std::max(rt.minIterDensity, 1);
```

### N.3 ImGui exposure (only `minIter*` exposed)

```cpp:particle-fluids/sph-water/src/main.cpp:2252-2253
                ImGui::SliderInt("min iter (density)",     &rt.minIterDensity,     1, 16);
                ImGui::SliderInt("min iter (divergence)",  &rt.minIterDivergence,  1, 16);
```

No `maxIter*`, `maxError*Percent`, or convergence-tolerance sliders are exposed.

### N.4 Documentation explicitly banks convergence to v1.1

```md:particle-fluids/sph-water/docs/notes.md:8-9
- [ ] Convergence-checked inner-loop iteration via sparse CPU readback every
      K frames (probably K=15–30). Currently fixed-iteration.
```

```md:particle-fluids/sph-water/docs/load-bearing-decisions.md:70-80
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

> Probe-2 note: the load-bearing doc claims "Panel exposes the `maxIter*` sliders too" — but the actual ImGui exposure at `main.cpp:2252-2253` covers only the `minIter*` sliders. The `maxIter*` sliders are not shown in the panel. (Banked to Section P.)

### N.5 No kernel-side measurement

Grep for `residual` / `error` / `tolerance` / `converge` inside `particle-fluids/sph-water/shaders/*.comp.glsl` returns **zero hits**. No DFSPH shader computes a per-particle or per-cell residual; no reduction kernel exists; no host-side readback of a residual scalar is wired anywhere.

### N.6 `Alembic::Abc::ErrorHandler` (false positive)

`common/common-cpp/src/alembic_writer.cpp:43` references `Alembic::Abc::ErrorHandler::kThrowPolicy`. Unrelated to solver convergence.

### N.7 `state_writer.cpp` `std::error_code` (false positive)

`common/common-cpp/src/state_writer.cpp:19, :32` use `std::error_code` for filesystem-error propagation. Unrelated.

## Section P: Incidental findings

The probe is read-only; the items below are recorded so architect-1 (who is consuming this report) can decide how to route them. Probe-2 takes no action.

### P.1 README claim diverges from code: "no CPU-readback convergence"

```md:particle-fluids/sph-water/README.md:98
- Fixed inner-iteration counts for both solvers (no CPU-readback convergence).
```

Matches the code as observed in Section N. Recording the matching pair only because the README's phrasing ("no CPU-readback convergence") is more affirmatively final than the docs/notes.md "Currently fixed-iteration / convergence-checked banked to v1.1" — different doc-tone, same code-reality.

### P.2 `_struct_layouts.txt` documents `da[].z` and `da[].w` as actively used, but no shader writes them

The struct-layout doc at `particle-fluids/sph-water/shaders/_struct_layouts.txt:53-54`:

```text:particle-fluids/sph-water/shaders/_struct_layouts.txt:53-54
.z    8       4     predicted_density     rho_adv_i (density_solve Pass 1)
.w    12      4     density_advect        rho_dot_i (divergence_solve Pass 1)
```

…claims `density_solve Pass 1` writes `rho_adv_i` to `.z`, and `divergence_solve Pass 1` writes `rho_dot_i` to `.w`. **Neither write happens in the actual shader source.** The doc describes an intended-but-unimplemented design. `density_alpha.comp.glsl:126` is the only writer of `da[]` and unconditionally writes `vec4(density, alpha_stored, 0.0, 0.0)`.

### P.3 `load-bearing-decisions.md` claims a panel feature that doesn't exist

```md:particle-fluids/sph-water/docs/load-bearing-decisions.md:78-80
v1 uses fixed iteration counts: `minIterDivergence = 1`, `minIterDensity = 2`.
Panel exposes the `maxIter*` sliders too but they're not consulted in v1
(banked v1.1 with sparse residual readback every K frames that doesn't stall the main pipeline).
```

ImGui code at `main.cpp:2252-2253` exposes only `minIter*` sliders. The `maxIter*` sliders are **not** present in the panel. The doc's "Panel exposes the `maxIter*` sliders too but they're not consulted" is incorrect.

### P.4 `prefix_sum_addback` descriptor write passes `cell_block_prefixes` twice

`main.cpp:1242-1244`:

```cpp:particle-fluids/sph-water/src/main.cpp:1242-1244
        writePrefixSumAddbackDescriptor(ctx.device(), ds_prefix_sum_addback,
            tier.cell_block_prefixes.handle(), tier.cell_block_prefixes.handle(),
            tier.cell_starts.handle(), tier.uniform_sort.handle());
```

`tier.cell_block_prefixes.handle()` is passed for both the first and second buffer slot. May be intentional (per-block scan results being read as both `PerBlock` and `BlockPrefixes`, given the shader bindings at `prefix_sum_addback.comp.glsl:8-14`) but the two arguments having the same buffer is a smell worth a second look. Out of scope for the SPH solver but flagging.

### P.5 Bilateral pass uses `r32f writeonly image2D` rather than separate read/write paths

`bilateral_smooth.comp.glsl:9-12` (from grep at L.2). The bilateral pass uses `texture2D inputDepth` + separate `r32f writeonly image2D outputDepth` rather than image-ping-pong with read+write image bindings. Standard for bilateral-style filters; recording only for completeness, no concern.

### P.6 No `gravity_pad.w` writer (still — mode field is dead)

The DFSPH UBO `gravity_pad.w` is set to `0.0f` in `pack_dfsph_uniform` (`main.cpp:1422`):

```cpp:particle-fluids/sph-water/src/main.cpp:1422
        u.gravity_pad[3]  = 0.0f;  // mode now lives in push constant
```

…with the comment "mode now lives in push constant" reflecting the post-fabrication-shape-9 fix (commit `7294ee4` per the retro). The four DFSPH shaders that have not been re-checked since their UBO docstrings were copied still document `gravity_pad` as `(.xyz=gravity, .w=mode)` in the UBO comment block (e.g. `density_alpha.comp.glsl:49`, `divergence_solve.comp.glsl:46`, etc.). The `.w` slot is no longer a mode; it is unused padding. Stale comment; not a bug.

### P.7 `integrate_forces` push-constant size encoded twice

`main.cpp:1101-1103` (already quoted in Section E.1) sets `push_const_bytes = sizeof(std::uint32_t)` for `pipe_integrate_forces`. The shader-side declaration is `layout(push_constant) uniform PC { uint mode; } pc;` at `integrate_forces.comp.glsl:45-47`. Size in C++ (4 bytes) matches the GLSL declaration (1 uint = 4 bytes). Consistent; recording so a future reviewer doesn't flag drift.

### P.8 No specialization-constant or pipeline-cache wiring

Probe-1 noted absence of an adaptive-iter mechanism; probe-2 adds: there is also no `VkPipelineCache`-based persistence and no `pSpecializationInfo` use anywhere. The Workgroup size 256 is hard-coded into each shader (`layout(local_size_x = 256) in;`). Adding adaptive subgroup workload via specialization would require both API and shader changes.

### P.9 No solver-error reduction kernel exists

Tied to Section N but elevating: there is no `density_error.comp.glsl`, no `divergence_reduce.comp.glsl`, no `residual_max.comp.glsl`, nor any subgroup-arithmetic-using reduction shader. Adding a sparse-readback residual gate would require both: (a) a new reduction shader, and (b) extending `common/common-cpp` with a non-stalling async readback helper (the existing `Buffer::readback` is synchronous via `runOneShot` — see Section F). Both are net-new surface.

### P.10 Two `predicted_velocity` documentation sites

`_struct_layouts.txt:24` documents `particles_buf` slot 2 `.xyz` as `predicted_velocity` (reserved v1). The token `predicted_velocity` does not appear in any shader. Same documentation-only situation as P.2 (predicted_density / density_advect): the SoA layout reserves space for DFSPH intermediate quantities that the current solver never computes or persists.

---

**End of probe-2 report.**
