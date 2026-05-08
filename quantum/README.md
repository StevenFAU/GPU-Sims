# Quantum

Demos that use real quantum hardware. The repo's quantum work is constrained by the hardware honestly — D-Wave's quantum annealer solves QUBO (quadratic unconstrained binary optimization) problems, which is a natural match for the Ising model and a wrong-shape match for continuous PDEs. We do not pretend quantum hardware accelerates fluid simulation.

## Sims in this category

- [`ising-dwave/`](ising-dwave/) — 2D / 3D Ising model on D-Wave's quantum annealer, with real-time visualization of spin configurations and phase transitions. **D-Wave Leap** + **Stack B (WebGPU)** for visualization.

For category rationale and the full reasoning behind the D-Wave scoping, see [`../docs/overarching-spec.md`](../docs/overarching-spec.md) §2 and §6, and [`../docs/root-context-distilled.md`](../docs/root-context-distilled.md) Decision 5.
