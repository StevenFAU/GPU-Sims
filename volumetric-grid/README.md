# Volumetric grid (Eulerian)

Eulerian simulations: physical state lives on a fixed 3D grid (or 2D grid for some demos), and PDEs are solved by stencil operations across that grid. Output is naturally volumetric and ports cleanly to OpenVDB for offline rendering.

## Sims in this category

- [`eulerian-smoke/`](eulerian-smoke/) — 256³ Stam stable fluids with vorticity confinement; single-scattering ray march; moving obstacles. **Stack C (Native C++).**
- [`lattice-boltzmann/`](lattice-boltzmann/) — 512×256×256 D3Q19 LBM around an airfoil with live streamlines. **Stack C (Native C++).**

> Note: `reaction-diffusion-3d/` lives under [`continuous-ca/`](../continuous-ca/reaction-diffusion-3d/). It was originally listed here too, but project-state.md treats it as canonically continuous-CA and Phase 3 ships it there.

For category rationale see [`../docs/overarching-spec.md`](../docs/overarching-spec.md) §5–§6.
