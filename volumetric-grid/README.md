# Volumetric grid (Eulerian)

Eulerian simulations: physical state lives on a fixed 3D grid (or 2D grid for some demos), and PDEs are solved by stencil operations across that grid. Output is naturally volumetric and ports cleanly to OpenVDB for offline rendering.

## Sims in this category

- [`eulerian-smoke/`](eulerian-smoke/) — 256³ Stam stable fluids with vorticity confinement; single-scattering ray march; moving obstacles. **Stack C (Native C++).**
- [`lattice-boltzmann/`](lattice-boltzmann/) — 512×256×256 D3Q19 LBM around an airfoil with live streamlines. **Stack C (Native C++).**
- [`reaction-diffusion-3d/`](reaction-diffusion-3d/) — 256³–512³ Gray-Scott "coral" volume; ray-marched iso-surface; paintable perturbations. **Stack C (Native C++).** (Categorized here by behavior; algorithmically also relates to continuous CA.)

For category rationale see [`../docs/overarching-spec.md`](../docs/overarching-spec.md) §5–§6.
