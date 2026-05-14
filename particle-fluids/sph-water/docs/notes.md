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
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
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
