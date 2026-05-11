# Eulerian Smoke — v1.1+ polish notes

This file is the prioritized backlog of polish items that landed outside Phase 8's v1 scope. Each entry includes an effort estimate and the rationale for why it's polish rather than v1.

## Priority 1.0 — Moving obstacles

The overarching-spec's eulerian-smoke entry lists "moving obstacles" as part of the v1 commitment. Phase 8 explicitly deferred this to v1.1 to keep scope tractable. Implementation outline:

- Add a `solid_mask` 3D image (r8ui, 256³ × 1 byte = 16 MB at default tier). 0 = fluid, 1 = solid.
- Add an "Obstacles" UI section in the panel: place spheres / boxes via clicks with the same `recreateGridResources`-style "Apply" idiom, or live-rasterize as the user drags — the simpler path.
- Add an `obstacle_rasterize` compute pass that runs before advection: for each obstacle, write 1 into the relevant cells.
- Modify advection / buoyancy / vorticity / divergence / Jacobi / project / boundaries kernels to short-circuit when `solid_mask[i,j,k] == 1`. The simplest contract: velocity zero, pressure boundary-condition treats solid cells like wall cells, density / temperature held at the solid cell's previous value (no flux through solids).
- Add a "solid-cell tint" render contribution in the raymarch (so the user can see the obstacle shape).

Estimated effort: 1–2 weeks of solver + render work. Visual benefit: significant — obstacles let the user create "smoke around a sphere" / "candle flame deflecting off a card" style scenes that recruiters recognize from production effects work.

## Priority 1.1 — Free-slip wall boundary conditions

Phase 8 ships with no-slip walls (zero velocity at all wall cells, both normal and tangential components). The more visually-correct choice for rising-plume smoke is free-slip (normal velocity zero, tangential velocity unchanged) — smoke slides along the walls realistically rather than "sticking." For a free rising plume that barely touches the side walls, the distinction is invisible; with obstacles (priority 1.0) it becomes visible. Implement by modifying `apply_boundaries.comp.glsl` to reflect the tangential velocity components rather than zero them.

Estimated effort: 1 day. Bundle with priority 1.0 — they're solving the same problem at different scopes.

## Priority 1.2 — MGPCG pressure solver

Jacobi at 40 iterations is acceptable for smoke; for water (where incompressibility error is more visible) it'd be inadequate. The standard upgrade is multigrid preconditioned conjugate gradient (MGPCG). Reference: McAdams/Sifakis/Teran 2010. Effort: 2–4 weeks. Visual benefit: modest for smoke; significant for follow-on sims (sph-water).

## Priority 1.3 — Animation hero render

The Blender script supports `--frame-start` / `--frame-end` from v1; v1.1 is producing an actual animation deliverable. Requires A100 access (240 frames × 512 samples × 1920×1080 ≈ many GPU-hours on dev hardware; trivial on A100). Deliverable: 8-second smoke-plume animation, 30 fps, 1080p. Banked for when HPC slot opens up.

## Priority 1.4 — Per-frame temperature VDB export *(superseded by 1.14)*

~~v1 ships density-per-frame + temperature-one-shot. For animation rendering with temperature-driven emission, per-frame temperature is needed. Implementation is trivial (one more `writeFloatFrame` call inside the export loop); the cost is the doubled disk write per frame. v1.1 widens the panel toggle to "Export density + temperature per frame."~~

*Superseded by Priority 1.14 (Unified VDB recording UX), which subsumes per-frame temperature export via a single recording toggle that writes density + temperature synchronously to a per-run subdirectory with metadata. Retained here for historical reference of the original narrower scope.*

## Priority 1.5 — GPU isosurface render alternative

The volume raymarch produces smoke-style results. For "stylized smoke" looks (e.g., the cartoonish Hayao Miyazaki smoke aesthetic), a marching-cubes isosurface at the density-threshold might be more aesthetic. RD-3D has the same v1.1 entry — implementing once would benefit both sims.

## Priority 1.6 — Particle motion-blur via velocity-pass rendering

If the offline render ever needs motion-blurred smoke (e.g., for an in-camera move), Blender Cycles can read a velocity grid for vector-based motion blur. Requires per-frame velocity export, which needs the slow `writeVec3Grid` path optimized. Two implementation options: (a) use `openvdb::Vec3SGrid::Accessor` with batched setValue (4–8× faster than the synced naive loop); (b) write three scalar grids (vx, vy, vz) via `copyFromDense` and assemble on the Blender side. Banked because the v1 hero render is a single still that doesn't need motion blur.

## Priority 1.7 — HDR bloom + better tonemap

Phase 8's render is Reinhard-tonemap-inline-in-the-fragment-shader. RD-3D has the same v1.1 entry — implementing HDR ping-pong + half-res bloom + decoupled ACES tonemap pass once would benefit every Stack C volume sim.

## Priority 1.8 — Fullscreen-borderless window mode

Repo-wide v1.1 across every Stack C sim. Requires extending `gv::Window` with a fullscreen toggle. Banked at the common-cpp level.

## Priority 1.9 — Smoke-trail emitter dragging

Currently the user clicks LMB to place an emitter; v1.1 could let the user drag to place a *trail* of emitters along the cursor path. Cleaner UX for sketching smoke shapes (signature, logo, etc.) Estimated effort: 1 day. Modest visual benefit; might be worth bundling with priority 1.0's obstacle UI rework.

## Priority 1.10 — A wind / gravity slider

A constant world-space force vector added each substep. Gives the user "smoke drifts to one side as it rises" effect for stylistic scenes. Trivial to implement (one uniform field + one extra dispatch). 0.5 day of work.

## Priority 1.11 — D3Q19 LBM cross-sim share

Phase 8's solver stack is Stam-tradition. The neighboring `volumetric-grid/lattice-boltzmann/` sim will use a fundamentally different solver (LBM streaming + collision). The two sims share the volume raymarch render path; promote `raymarch.frag.glsl` to a common-cpp shader-include after LBM ships (consumer #2 of the volume-raymarch shader pattern). Banked as the rule-of-three candidate at consumer #3 (likely a future volumetric-grid sim).

## Priority 1.0′ — Active density+temperature drain at open ceiling

v1's `apply_boundaries.comp.glsl` zeros velocity at the five no-slip faces but does NOT drain density or temperature at the `y = N-1` ceiling. Net effect: injection has no matching outflow, so the domain monotonically saturates over ~30 seconds for any preset with continuous emission. Workaround in v1: tune `densityDissipation ≈ 0.015` and `temperatureDissipation ≈ 0.018` to produce quasi-steady-state plumes that look correct over the demo window. Real fix in v1.1: one additional branch in `apply_boundaries` that zeros density and temperature at `y = N-1` cells (matches the "open ceiling" boundary the visual intent assumes). Estimated effort: 1 hour. Bundle with priority 1.1 (free-slip walls) since both are `apply_boundaries.comp.glsl` edits.

## Priority 1.12 — VSync default + panel toggle (depends on common-cpp amendment)

The thermal observation during Phase 8 visual verification (RX 6800 XT fans maxed under sustained compute) traces to common-cpp's hardcoded MAILBOX-preferred present mode in `choosePresentMode()` (`common/common-cpp/src/vk/window.cpp`). The real fix requires a common-cpp API amendment first — banked separately as a candidate Phase 8.5 or broader common-cpp hardening phase exposing `VkPresentModeKHR` at `Window` construction with FIFO as the new default. After that lands, eulerian-smoke adds a "VSync" checkbox to the Rendering panel, defaulted ON, plumbed through to the new present-mode selection. Workaround in the meantime: launch with `vblank_mode=3 ./build/bin/eulerian_smoke` for FIFO at the Mesa driver layer.

## Priority 1.13 — Re-evaluate default grid tier from 256³ to 192³

Phase 8 testing surfaced that 256³ runs significantly slower than 192³ on the RX 6800 XT, and the visual quality difference is modest for interactive exploration — the portfolio hero render uses offline Blender Cycles anyway, where solver tier is irrelevant. Consider making 192³ the default tier in the SMOKE_PRESETS / tier dropdown, 256³ the "high quality" tier, and 384³ the stretch tier with the existing degradation warning. v1.1 polish, not a defect. Estimated effort: trivial (one-line constant flip + dropdown label reshuffle).

## Priority 1.14 — Unified VDB recording UX (category-architect work)

The current capture flow has two independent toggles: "Export VDB density per frame" (continuous) and "Export current temperature snapshot" (one-shot). This produces unpaired density/temperature files at different frame indices, which is ergonomically wrong for Blender Cycles consumption where the black-body emission shader needs spatially correlated temperature and density.

Proposed redesign (category-architect scope):

- Replace both controls with a single "Record VDB sequence" checkbox in the State panel.
- When toggled ON: auto-create a new subdirectory `vdb_export/run_NNNN/` (NNNN auto-incremented from highest existing). Write a `recording.json` metadata file with the preset name, all RuntimeState parameters, grid tier, sim commit SHA (build-time embedded), and start timestamp.
- Every render frame while toggled ON: synchronously write `density_NNNN.vdb` AND `temperature_NNNN.vdb` to the run subdirectory, with NNNN starting at 0000 for the first recorded frame and counting up.
- When toggled OFF: update `recording.json` with end timestamp and frame count. Display "Last recording: run_NNNN/ (X frames)" in the State panel until the next toggle-on.
- The existing "Export current temperature snapshot" button is removed (subsumed).
- Lazy directory creation — no empty directories for runs that never started.

This was identified by the repo architect during Phase 8's hero-render exploration. The bad ergonomics blocked end-to-end Blender validation in the Phase 8 session itself, which is why the validation step was deferred to category-architect scope (alongside the hero render aesthetic itself). The recording UX fix is a prerequisite to comfortable iterative hero-render work.

Estimated effort: 2–4 hours of category-architect work (panel UX + per-frame write coordination + `recording.json` scaffolding + auto-numbered subdir logic).
