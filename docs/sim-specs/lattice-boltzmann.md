# lattice-boltzmann sim spec

> Phase 12. Stack C. Tier defaults: 256x128x128 desktop.

## Goal

Demonstrate incompressible-ish fluid flow around a NACA airfoil using
the D3Q19 BGK lattice Boltzmann method. Headline visualization: live
GPU-seeded streamlines plus a velocity-magnitude volume raymarch.
Educational target: vortex-shedding regime around a low-Reynolds-number
airfoil.

## Scale tiers

| Tier label                  | Nx x Ny x Nz | f-state memory | Streamlines | Notes              |
|-----------------------------|--------------|----------------|-------------|--------------------|
| 128^3 (Laptop)              | 128x128x128  | ~150 MB        | 10k         | iGPU / laptop      |
| 256x128x128 (Desktop)       | 256x128x128  | ~610 MB        | 10k         | RX 6800 XT default |
| 512x256x256 (Capture)       | 512x256x256  | ~4.8 GB        | 10k         | Hero render only   |

## Presets

| Preset                  | Airfoil   | alpha (deg) | u_inf  | tau    | Re (default tier) |
|-------------------------|-----------|-------------|--------|--------|-------------------|
| NACA0012 - Low-Re       | NACA0012  | 4.0         | 0.04   | 0.60   | ~80               |
| NACA0012 - Med-Re       | NACA0012  | 8.0         | 0.06   | 0.55   | ~230              |
| NACA4412 - Med-Re       | NACA4412  | 6.0         | 0.06   | 0.55   | ~230              |

## Controls

- WASD: camera. RMB-drag: look. Q/E: world-up/down. Shift: boost.
- F5: save capture. F9: load latest.
- Panel: presets, tiers, solver (tau, substeps), flow (|u_inf|, AoA),
  render (toggles, exposure), streamlines (count, history), capture,
  camera, stats.

## Capture format

Top-level meta key: `latticeBoltzmann`. Buffers: `density` (r32f),
`velocity` (rgba16f), `obstacle_mask` (r8uint, `frame_invariant: true`).

f-state distribution functions NOT captured (size; recomputed from
moments via equilibrium at F9 load).

## Render

- Volume raymarch of velocity-magnitude with colormap LUT. ~64-256
  steps depending on tier.
- GPU-seeded streamlines (~10k seeds x 64 ring-buffer history) RK2-
  advected per render frame.

## Boundaries (v1)

- Inlet (-X) / outlet (+X): equilibrium boundary (first-order
  accurate); Zou-He second-order banked for v1.1.
- +-Y, +-Z faces: free-slip (specular reflection).
- Interior airfoil: halfway bounce-back per
  `tools/integrity/docs/algebraic/d3q19.md` § 5.

## References

- `references/lbm-principles-practice/` ([Krueger] registry entry; MIT,
  SHA `6e2c592f`) — D2Q9 pattern reference for BGK equilibrium and
  halfway bounce-back.
- `tools/integrity/docs/algebraic/d3q19.md` ([Algebraic_D3Q19] registry
  entry) — D3Q19 velocity set, weights, opposite-direction table,
  equilibrium formula. Authoritative ground truth for this sim's
  shader constants.
- Zou & He 1997, *Phys. Fluids* 9, 1591 — Zou-He boundary conditions
  (referenced for v1.1 Zou-He upgrade; not used in v1).
- Hecht & Harting 2010, arXiv:0811.4593 — 3D Zou-He generalisation.
- NACA Report 460, Jacobs/Ward/Pinkerton 1933 — NACA 4-digit airfoil
  equations (used for the analytical SDF voxelisation in
  `volumetric-grid/lattice-boltzmann/src/main.cpp`).
