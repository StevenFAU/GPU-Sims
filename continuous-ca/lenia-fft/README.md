# Lenia (FFT-convolution)

**Status:** Unimplemented
**Stack:** Stack D (Python / Taichi) + Stack B (WebGPU deploy)
**Spec:** [`docs/sim-specs/lenia-fft.md`](../../docs/sim-specs/lenia-fft.md)

2048² real-time Lenia via FFT convolution; automated parameter search for stable creatures.

This sim is unimplemented. The specification sheet is the primary reference for design and implementation decisions; the architect chat drafts the spec, and the per-sim implementer chat refines it during implementation.

## Build

Build instructions will be added when the implementation is complete. See the [top-level README](../../README.md) for general per-stack patterns.

## Performance

To be characterized when implemented. Per [`docs/conventions.md`](../../docs/conventions.md), every sim publishes per-pass GPU times at each scale tier (laptop / desktop / HPC) once running.

## References

Canonical papers and reference implementations are tracked in the [spec sheet](../../docs/sim-specs/lenia-fft.md).
