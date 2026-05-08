# Particle fluids (Lagrangian)

Lagrangian simulations: physical state lives on particles that move through space. Spatial queries (neighbor finding, density estimation) are the core engineering challenge. Output ports cleanly to Alembic for offline rendering.

## Sims in this category

- [`sph-water/`](sph-water/) — 2–4M-particle SPH water with hand-rolled GPU spatial hash (Morton sort) and screen-space rendering with refraction. The most engineering-dense mid-tier project in the repo. **Stack C (Native C++).**
- [`pic-flip/`](pic-flip/) — PIC/FLIP fluid solver. Better-looking than pure SPH but harder; stretch goal. **Stack C (Native C++).**

For category rationale see [`../docs/overarching-spec.md`](../docs/overarching-spec.md) §5–§6.
