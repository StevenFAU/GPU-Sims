# Hybrid particle-grid

Methods that maintain both a particle representation and a background grid, transferring quantities between them each step. The Material Point Method (MPM) is the canonical example and unifies fluids, granular materials, and elastic solids in one solver.

## Sims in this category

- [`mpm-multimaterial/`](mpm-multimaterial/) — Multi-material MLS-MPM sandbox: sand + jelly + water in the same scene, ~1M particles + 256³ grid. **Stack D (Python / Taichi).** The repo's sole primary Stack D commitment; the algorithm payoff justifies the side-trip from native compute.

For category rationale see [`../docs/overarching-spec.md`](../docs/overarching-spec.md) §5–§6.
