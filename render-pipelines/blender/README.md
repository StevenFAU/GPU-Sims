# render-pipelines/blender/

Blender Cycles render scripts. The default offline renderer for the repo.

**Status:** Empty. Will be populated as individual sims begin producing `.vdb` / `.abc` exports.

## Why Blender Cycles

- **Free.** No license cost, no watermark on output.
- **Scriptable.** Full Python API; renders are reproducible from a single script.
- **Headless.** `blender -b script.py -- args` runs without a display, suitable for HPC batch jobs.
- **GPU-accelerated.** HIP on AMD, OptiX on NVIDIA. The dev desktop's RX 6800 XT and the lab PC's 2080 Ti both run Cycles natively.
- **Reads `.vdb` and `.abc` natively.** Standard `mesh_sequence_cache` modifier for Alembic; native VDB volume support since Blender 2.8.

## Conventions for scripts in this directory

Each script:

1. Takes a cache directory as input (e.g., a directory of `frame_NNNN.vdb`).
2. Sets up a Blender scene programmatically — camera, lighting, materials.
3. Renders the sequence to an output directory.
4. Is reproducible: same script + same cache + same Blender version → identical output.

## Planned scripts

- `render_smoke.py` — for `volumetric-grid/eulerian-smoke/` exports.
- `render_sph.py` — for `particle-fluids/sph-water/` exports.
- `render_mpm.py` — for `hybrid-particle-grid/mpm-multimaterial/` exports.
- `render_lbm.py` — for `volumetric-grid/lattice-boltzmann/` exports.
- (Per-sim entries added as sims ship.)

## Running

```bash
blender -b -P render_smoke.py -- --input ../../volumetric-grid/eulerian-smoke/captures/ --output renders/smoke_v01/
```

The `--` separates Blender arguments from script arguments.
