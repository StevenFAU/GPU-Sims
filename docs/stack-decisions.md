# Per-sim stack assignments

This document records which of the four reusable stacks (A / B / C / D) is the primary target for each simulation in the catalog, along with brief rationale. The four stacks are defined in [`overarching-spec.md`](overarching-spec.md) § 3.

| Sim | Category | Primary stack | Secondary | Rationale |
|---|---|---|---|---|
| Strange attractors | Closed-form | B (WebGPU) | C optional | Web-shareable, embeddable; 10M particles fits comfortably in WebGPU. Native variant if pushing 100M+. |
| Mandelbulb explorer | Closed-form | A → B | — | Start in Shadertoy for shader iteration; port to WebGPU for portability and free-fly camera. |
| Physarum | Agent-based | B (WebGPU) | — | 10M agents on WebGPU is comfortable. 2D only by design — no need for native. |
| Boids-3D | Agent-based | B (WebGPU) | — | 100k boids + 1k predators in 3D. WebGPU sufficient. |
| Reaction-diffusion 2D | Continuous CA | A (Shadertoy) | — | Single fragment shader. Ideal Shadertoy use case. |
| Reaction-diffusion 3D | Continuous CA | C (Native C++) | — | 256³–512³ Gray-Scott with ray-marched iso-surface. Native compute for scale. |
| Lenia (FFT) | Continuous CA | D (Python) | B (deploy) | FFT convolution research is best in Python (Taichi/JAX). Deploy interactive version in WebGPU. |
| Neural CA | Continuous CA | D (Python) | B (deploy) | PyTorch training on lab PC; deploy interactive damage-and-regenerate demo in WebGPU. |
| Eulerian smoke | Volumetric grid | C (Native C++) | — | 256³ Stam stable fluids. Native compute is the path that hits the hardware ceiling. |
| Lattice Boltzmann | Volumetric grid | C (Native C++) | — | 512×256×256 D3Q19 LBM. Native compute. |
| SPH water | Particle fluids | C (Native C++) | — | 2–4M particles, hand-rolled spatial hash, screen-space rendering. Engineering-dense; native compute. |
| PIC/FLIP | Particle fluids | C (Native C++) | — | Stretch goal. Better-looking than pure SPH but algorithmically harder. |
| MPM multi-material | Hybrid particle-grid | D (Python/Taichi) | — | The 88-line MLS-MPM reference is pedagogically perfect; algorithm complexity justifies the side-trip into Python. |
| Ising on D-Wave | Quantum | D-Wave Leap | B (visualization) | D-Wave is a quantum annealer; Ising model is the natural workload. WebGPU for live visualization of spin configurations. |

## Notes on stack overrides

A per-sim spec sheet may override the assignment above with documented cause. Common reasons:

- **Performance ceiling not hit on the chosen stack.** If a WebGPU sim can't sustain interactive rates at the target scale, escalate to native (Stack C).
- **Existing reference implementation.** If a canonical reference is in a specific stack (e.g., Bert Chan's Lenia in Python/JAX), match the stack to leverage it.
- **Toolchain availability.** If a critical library is only available in one stack, the choice is forced.

Overrides are recorded in the per-sim spec sheet under "Stack assignment and rationale" (template § 3).

## What "Stack C primary" actually means in practice

Native C++ sims have flexibility on the GPU backend:

- **OpenGL 4.6 compute** is the path of least friction given the user's existing OpenGL skill. Recommended default for Stack C.
- **Vulkan 1.3 compute** offers better profiling, explicit synchronization, and is industry-standard for next-gen graphics. Recommended for sims that profiling reveals are bottlenecked on synchronization overhead.
- **WebGPU via dawn or wgpu-native** is an acceptable variant when cross-platform native is wanted. Less mature than Vulkan/OpenGL but improving.

The choice is per-sim; the architect or implementer chat picks based on the sim's needs.
