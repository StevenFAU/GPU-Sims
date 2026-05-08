# render-pipelines/

The offline render pipeline. Every sim writes its state to industry-standard cache formats (`.vdb` for volumetric grids, `.abc` for particles and meshes), and the scripts and projects in this directory consume those caches to produce path-traced cinematic output.

**Architectural note.** The simulations themselves are renderer-agnostic. They have no `#ifdef` for which renderer is downstream; they just write open-standard files. Switching renderers — or rendering the same sim in multiple renderers — is purely a matter of which script in this directory consumes the cache.

## Subdirectories

- [`blender/`](blender/) — Blender Cycles render scripts. **Default renderer** for the repo. Free, scriptable in Python, headless via `blender -b`, GPU-accelerated on AMD (HIP) and NVIDIA (OptiX). All sims have a Cycles render path.
- [`houdini/`](houdini/) — Houdini Karma render projects. **Premium renderer** for hero shots, particularly smoke (volume rendering with multi-scatter) and SPH (level-set surface reconstruction). Requires Houdini Education ($75/year for validated students) or higher. Empty until access is acquired; the simulations themselves work without Houdini.
- [`optix/`](optix/) — Standalone OptiX path tracers. For sims that want a custom renderer rather than going through Blender or Houdini. Empty until a use case arises.

## Workflow

1. Run a sim with `--record <output-dir>` (or hit the in-app "record" hotkey, TBD) to dump frame-by-frame state to `<output-dir>/frame_NNNN.{vdb,abc}`.
2. Run the appropriate render script: `python render-pipelines/blender/render_smoke.py <input-dir> <output-dir>` or equivalent.
3. The result is a path-traced image sequence ready for stitching into video or selecting hero stills.

For HPC hero runs, render scripts are headless-runnable and can be submitted as batch jobs. See `docs/sim-specs/<sim>.md` §7 for per-sim hero render targets.
