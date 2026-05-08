# Lattice Boltzmann

**Status:** Unimplemented
**Stack:** Stack C (Native C++)
**Spec:** [`docs/sim-specs/lattice-boltzmann.md`](../../docs/sim-specs/lattice-boltzmann.md)

512×256×256 D3Q19 LBM around an airfoil, with live streamlines.

This sim is unimplemented. The specification sheet is the primary reference for design and implementation decisions; the architect chat drafts the spec, and the per-sim implementer chat refines it during implementation.

## Build

Build instructions will be added when the implementation is complete. See the [top-level README](../../README.md) for general per-stack patterns.

## Performance

To be characterized when implemented. Per [`docs/conventions.md`](../../docs/conventions.md), every sim publishes per-pass GPU times at each scale tier (laptop / desktop / HPC) once running.

## References

Canonical papers and reference implementations are tracked in the [spec sheet](../../docs/sim-specs/lattice-boltzmann.md).
