# Strange Attractors

**Status:** Unimplemented
**Stack:** WebGPU
**Spec:** [`docs/sim-specs/strange-attractors.md`](../../docs/sim-specs/strange-attractors.md)

10M particles integrating Lorenz / Aizawa / Thomas ODEs, motion blur, slow camera orbit. The repo's first warm-up sim.

This sim is unimplemented. The specification sheet is the primary reference for design and implementation decisions; the architect chat drafts the spec, and the per-sim implementer chat refines it during implementation.

## Build

Build instructions will be added when the implementation is complete. See the [top-level README](../../README.md) for general per-stack patterns.

## Performance

To be characterized when implemented. Per [`docs/conventions.md`](../../docs/conventions.md), every sim publishes per-pass GPU times at each scale tier (laptop / desktop / HPC) once running.

## References

Canonical papers and reference implementations are tracked in the [spec sheet](../../docs/sim-specs/strange-attractors.md).
