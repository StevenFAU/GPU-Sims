# sph-water

DFSPH-formulated Smoothed Particle Hydrodynamics water simulation at 1M–4M
particles. First Stack C Tier-2 flagship since eulerian-smoke (Phase 8); first
Alembic real-impl consumer in the repo; first Stack C user of
`VK_EXT_subgroup_size_control` (Vulkan 1.3 core).

## Highlights

- **DFSPH solver** (Bender-Koschier 2015 + 2017): two inner pressure-correction
  loops per substep — divergence-free correction + density-constancy correction.
- **Morton-sorted spatial hash** at 1M–4M particles: 6 GPU compute kernels
  translated from the Phase 7 boids-3d Stack B counting-sort precedent.
- **Screen-space fluid render** (Müller-Fetterer 2007): depth pass → bilateral
  smooth → thickness pass → Fresnel-blended composite with refraction.
- **Alembic per-frame export** (`.abc` files) for offline Blender Cycles renders;
  velocity-driven motion blur via native Alembic `OPoints` schema.
- **Four scene presets**: Dam-Break (canonical SPH demo), Central-Fountain
  (sustained emitter against gravity), Droplet-Impact (high-velocity collision),
  Pour-from-Source (sustained surface stream).
- **LMB-paint emitter brush** for adding particles at runtime, with reserve-tail
  allocation (Phase 9 MPM polish-5 precedent transferred to Stack C).

## Build

Built as part of the standard top-level CMake build. Alembic export is gated by
`-DGPU_SIMS_USE_ALEMBIC=ON` (default OFF). With Alembic enabled, the build
fetches Alembic 1.8.10 via CMake `FetchContent` and requires the apt package
`libimath-dev`.

```bash
sudo apt install libimath-dev
cmake -S . -B build -DGPU_SIMS_USE_ALEMBIC=ON
cmake --build build --parallel
./build/bin/sph_water
```

Without Alembic, the sim runs normally; the export toggle becomes a no-op and
the panel reports "stub mode".

## Controls

| Input | Action |
|-------|--------|
| WASD | Camera movement (forward / left / back / right) |
| Q / E | Camera up / down (world-space) |
| Shift + WASD | Camera boost (3× speed) |
| RMB drag | Camera look |
| LMB click / drag | Place emitter at cursor (paint plane perpendicular to view) |
| Space | Pause / unpause |
| F5 | Capture state to `./captures/capture_<NNNN>/` |
| F9 | Reload latest capture |
| Esc | Exit |

## Presets

| Index | Name | Behavior |
|-------|------|----------|
| 0 | Dam-Break | Particle brick on +X side collapses under gravity |
| 1 | Central-Fountain | Sparse seed + sustained emitter pushing upward |
| 2 | Droplet-Impact | Thin pool + falling droplet → crown splash |
| 3 | Pour-from-Source | Empty domain filling from a rectangular source on top face |

## Tiers

| Index | Particle count | Notes |
|-------|----------------|-------|
| 0 | 256k | comfortable headroom |
| 1 | 1M | default; design target ~60 FPS on RX 6800 XT |
| 2 | 2M | reduced FPS; interesting visuals |
| 3 | 4M | capture-mode; not interactive |

## Hero render

The hero render lives at `render-pipelines/blender/render_sph.py`. It consumes
the Alembic `.abc` file written by the sim during interactive use (toggle
"Export Alembic" in the panel; configure frame interval). Cycles GPU fallback
chain: OptiX → HIP → CUDA → fail-loud.

```bash
blender --background --python render-pipelines/blender/render_sph.py -- \
    --abc-path /path/to/sph_water.abc \
    --output /path/to/output.png \
    --samples 256 \
    --resolution 1920 1080
```

## Design references

- DFSPH solver: SPlisHSPlasH 1.8.10, especially `TimeStepDFSPH.cpp` and
  `SPHKernels.h`. See `docs/load-bearing-decisions.md` for file:line anchors.
- Screen-space fluid: Müller-Fetterer 2007 "Screen Space Meshes",
  Williams-van der Laan 2008 follow-up.
- Spatial hash: agent-based/boids-3d (Phase 7) Stack B counting-sort precedent.

## Limitations (v1)

- Fixed inner-iteration counts for both solvers (no CPU-readback convergence).
- Simple cohesion approximation (not Akinci 2013 surface tension).
- No solid boundaries other than the AABB box (no obstacles, weirs).
- Foam classification deferred to v1.1 (currently no per-particle attribute
  exported to Alembic beyond position + velocity).
- The `a_ij` pair-coupling formulas in `divergence_solve.comp.glsl` and
  `density_solve.comp.glsl` are placeholder skeletons; the upstream-exact
  formulation is banked for the Phase 11 follow-up polish per the spec's
  architect-2 callout 1 verification item.

See `docs/notes.md` for the full v1.1 stretch-items list.
