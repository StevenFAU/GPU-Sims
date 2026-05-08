# render-pipelines/houdini/

Houdini Karma render projects. The premium renderer for hero shots — particularly smoke (volume rendering with multi-scatter and proper light transport) and SPH (level-set surface reconstruction with surface tension).

**Status:** Empty. Pending Houdini access.

## Houdini access

This directory's contents require **Houdini Education** ($75/year for validated students) or **Houdini Indie** ($269/year) or higher. Houdini Apprentice (free) has watermarked output and is not suitable for portfolio hero stills.

The simulations themselves work without Houdini — Cycles in [`../blender/`](../blender/) is the default renderer. Houdini renders are additive: when access is available, the same `.vdb` and `.abc` cache files that Blender consumes are also consumed by Karma to produce a higher-quality alternate render of the same simulation. The simulation code never changes.

## Why Karma for these specific sims

- **Smoke (`volumetric-grid/eulerian-smoke/`).** Karma's multi-scatter volume rendering is genuinely better than Cycles for thick volumetric media. The "Cycles smoke" look is recognizable to anyone in the field; Karma smoke holds up next to film VFX.
- **SPH water (`particle-fluids/sph-water/`).** Houdini's particle-to-surface workflow (VDB-based level set extraction, smoothing, surface tension reconstruction) is a built-in industry-standard pipeline. Doing the equivalent in Blender is more code for lower-quality output.
- **MPM (`hybrid-particle-grid/mpm-multimaterial/`).** Granular materials look better in Karma; Cycles handles them but is not as art-directable.

Other sims (closed-form, agent-based, CA, LBM, Ising) render fine in Cycles and don't need Houdini.

## Planned projects

- `smoke.hip` — Karma render of cached smoke `.vdb` sequences.
- `sph_water.hip` — Particle-to-surface and Karma render of SPH `.abc` sequences.
- `mpm_multimaterial.hip` — Multi-material rendering of MPM exports.

These will be populated when Houdini access is available.
