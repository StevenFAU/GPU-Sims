# Reaction-Diffusion 3D

**Status:** Unimplemented
**Stack:** Stack C (Native C++)
**Spec:** [`docs/sim-specs/reaction-diffusion-3d.md`](../../docs/sim-specs/reaction-diffusion-3d.md)

256³–512³ Gray-Scott "coral" volume, ray-marched iso-surface, paintable perturbations and parameter regions.

This sim is unimplemented. The specification sheet is the primary reference for design and implementation decisions; the architect chat drafts the spec, and the per-sim implementer chat refines it during implementation.

## Build

Build instructions will be added when the implementation is complete. See the [top-level README](../../README.md) for general per-stack patterns.

## Performance

To be characterized when implemented. Per [`docs/conventions.md`](../../docs/conventions.md), every sim publishes per-pass GPU times at each scale tier (laptop / desktop / HPC) once running.

## References

Canonical papers and reference implementations are tracked in the [spec sheet](../../docs/sim-specs/reaction-diffusion-3d.md).
