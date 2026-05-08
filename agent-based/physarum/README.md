# Physarum

**Status:** Unimplemented
**Stack:** Stack B (WebGPU)
**Spec:** [`docs/sim-specs/physarum.md`](../../docs/sim-specs/physarum.md)

10M agents, 3 species with mutual repulsion, territorial boundary formation. 2D by design.

This sim is unimplemented. The specification sheet is the primary reference for design and implementation decisions; the architect chat drafts the spec, and the per-sim implementer chat refines it during implementation.

## Build

Build instructions will be added when the implementation is complete. See the [top-level README](../../README.md) for general per-stack patterns.

## Performance

To be characterized when implemented. Per [`docs/conventions.md`](../../docs/conventions.md), every sim publishes per-pass GPU times at each scale tier (laptop / desktop / HPC) once running.

## References

Canonical papers and reference implementations are tracked in the [spec sheet](../../docs/sim-specs/physarum.md).
