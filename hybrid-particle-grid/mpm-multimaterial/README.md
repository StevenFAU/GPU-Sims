# MPM Multi-Material

**Status:** Unimplemented
**Stack:** Stack D (Python / Taichi)
**Spec:** [`docs/sim-specs/mpm-multimaterial.md`](../../docs/sim-specs/mpm-multimaterial.md)

Multi-material MLS-MPM sandbox: sand + jelly + water in the same scene, ~1M particles + 256³ grid.

This sim is unimplemented. The specification sheet is the primary reference for design and implementation decisions; the architect chat drafts the spec, and the per-sim implementer chat refines it during implementation.

## Build

Build instructions will be added when the implementation is complete. See the [top-level README](../../README.md) for general per-stack patterns.

## Performance

To be characterized when implemented. Per [`docs/conventions.md`](../../docs/conventions.md), every sim publishes per-pass GPU times at each scale tier (laptop / desktop / HPC) once running.

## References

Canonical papers and reference implementations are tracked in the [spec sheet](../../docs/sim-specs/mpm-multimaterial.md).
