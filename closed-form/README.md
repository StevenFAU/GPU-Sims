# Closed-form

No simulation in the time-stepping sense — these demos render mathematical structures directly. Strange attractors are integrated trajectories of ODE systems; the Mandelbulb is an iterated complex map rendered via distance-estimator ray marching. Both serve as warm-up tier projects to exercise the rendering and infrastructure pipeline before tackling time-evolving simulations.

## Sims in this category

- [`strange-attractors/`](strange-attractors/) — 10M particles integrating Lorenz / Aizawa / Thomas ODEs; motion blur, additive blending, slow camera orbit. **Stack B (WebGPU)** or **Stack C (Native).** First spinoff target.
- [`mandelbulb-explorer/`](mandelbulb-explorer/) — Free-fly distance-estimator ray marcher; soft shadows, orbit-trap coloring, parameter morph animations. **Stack A (Shadertoy) → Stack B (WebGPU).**

For category rationale see [`../docs/overarching-spec.md`](../docs/overarching-spec.md) §5–§6.
