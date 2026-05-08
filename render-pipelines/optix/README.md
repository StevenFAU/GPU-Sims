# render-pipelines/optix/

Standalone OptiX path tracers, when a sim wants a custom renderer rather than going through Blender or Houdini.

**Status:** Empty. No current use case.

## When this directory gets used

Possible motivations for a custom OptiX renderer over Blender/Houdini:

- A sim that wants a renderer tightly coupled to its own data structures (e.g., a strange-attractor renderer with custom motion-blur integration baked into the path tracer).
- A research demonstration of OptiX-specific features (RT cores, hardware-accelerated BVH traversal, denoising).
- A render that needs to run inside the same process as the simulation (rather than out-of-process via a cache file).

None of the current sim catalog requires this; Blender Cycles handles every sim adequately as a baseline. This directory exists so that if a future sim does want a custom OptiX renderer, the place for it is already established.

## Hardware constraint

OptiX is NVIDIA-only. Custom renderers here would run on the lab PC (2080 Ti) or HPC (A100), not on the AMD dev desktop.
