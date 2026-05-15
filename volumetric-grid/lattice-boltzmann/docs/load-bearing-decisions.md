# Load-bearing decisions — lattice-boltzmann (Phase 12)

This document summarises the 13 load-bearing decisions for
`volumetric-grid/lattice-boltzmann/`. Detailed rationale and
rejection-of-alternatives in `docs/phase12_lattice_boltzmann.md` § 2.

## D1 — Algorithm: D3Q19 BGK single-relaxation-time

Canonical 1990s LBM with Maxwell-Boltzmann equilibrium. Pinned in
`tools/integrity/docs/algebraic/d3q19.md`. Rejected: D3Q27 (cost), D3Q15
(accuracy), MRT (scope).

## D2 — Storage: SoA, one buffer per direction; rest-direction split

19 r32f 3D images per parity, ping-pong. Rest direction (i=0) lives in
its own scalar image to skip streaming for it (1/19 memory traffic
saved). Future-proofs sparse / quantized / differentiable variants.

## D3 — Streaming: two-buffer ping-pong with parity bit

Same pattern as eulerian-smoke. AA-pattern / EsoTwist banked v1.1.

## D4 — Separate dispatches for collide / stream / boundaries / moments

Modularity over throughput. Fused stream-collide banked as v1.1
optimisation (~5-10% on RDNA per Krüger).

## D5 — Macroscopic moments in dedicated r32f + rgba16f images

ρ (r32f) and **u** (rgba16f, 4th component unused) live in their own 3D
images. Three consumers: next substep's collide, raymarch, streamlines.
Inline reduction in each consumer would triple redundant work.

## D6 — Boundaries: equilibrium inlet/outlet (v1) + free-slip ±Y±Z + halfway-BB

**Class C divergence from spec § 4.G**: spec called for full Zou-He
second-order inlet/outlet; v1 ships **equilibrium boundary** (`f_i =
feq(ρ_0, u_inlet)` for unknowns) because the spec's Zou-He formulas
contained transcription errors (D2Q9 vs. D3Q19 face-weight coefficient,
partition swap) and re-derivation surfaced moment-consistency issues.
First-order accurate; self-consistent with `init_equilibrium`. Banked as
v1.1 polish: full Zou-He upgrade with anchored derivation.

Free-slip on ±Y±Z and halfway-BB at the airfoil are unchanged from
spec.

## D7 — Obstacle: CPU-generated NACA SDF voxelised to 3D r8 mask

Analytical NACA 4-digit airfoil cross-section, extruded along Z (2.5D
wing). ~50 LOC sim-local CPU code, runs once per preset apply. Not
promoted to common-cpp at v1.

## D8 — Render: sim-local `velmag.frag.glsl`, NOT rule-of-three promote

ES's `raymarch.frag.glsl` is physics-based (single-scatter + BB);
LBM's is transfer-function-driven (|u| → LUT). Promote at consumer #3 of
the volume-raymarch pattern (next volumetric-grid sim).

## D9 — Streamlines: GPU-seeded ring-buffer history with RK2 advection

~10k seeds × 64-history ring buffer per sid; RK2 step per render frame.
**Class C divergence from spec § 4.B.12**: dropped
`primitive_restart_enable` + NaN-vertex sentinels (which only fire on
*indexed* draws) in favour of one `vkCmdDraw` per streamline with a
push-constant sid. Banked v1.1: switch to indexed draw with explicit
restart sentinel.

## D10 — Tiers: 128³, 256×128×128 (default), 512×256×256

Deferred-change after `Renderer::beginFrame()` (Phase 9/10/11 pattern).
Tier change triggers `renderer.waitIdle()` + full f-state reallocation
+ descriptor rewire + preset re-apply.

## D11 — Subgroup-size pinning to 32 on collide / stream / boundaries / moments

Phase 11 surface (`ContextCreateInfo::enable_subgroup_size_control`,
`ComputePipelineDesc::required_subgroup_size`). RX 6800 XT measured
`minSubgroupSize=32, maxSubgroupSize=64`; pinning to 32 lands in-range
and matches NVIDIA warp-32 baseline. **Class C divergence**: dropped
pinning on `init_equilibrium` (no subgroup ops; pinning is overhead).

## D12 — Capture: `latticeBoltzmann` key, bare names, frame_invariant obstacle_mask

Top-level meta key `latticeBoltzmann` (camelCase per cross-stack
convention). Buffer names bare (`density`, `velocity`, `obstacle_mask`)
to avoid the Phase 8 `.bin.bin` quirk. `obstacle_mask` carries
`"frame_invariant": true` — new convention banked at Phase 12. F-state
distribution functions NOT captured; F9 re-derives via
`init_equilibrium` from saved ρ, **u**.

## D13 — Reference anchors: [Krueger] for math patterns, [Algebraic_D3Q19] for constants

Krüger book code (vendored at `references/lbm-principles-practice/`,
SHA `6e2c592f`) is **D2Q9 only** — used as pattern reference for BGK
and halfway-BB shape; doc-blocks make this explicit. The 19 directions,
3 weights, opposite-direction table, and equilibrium formula come
exclusively from `tools/integrity/docs/algebraic/d3q19.md` § 2.2 / 4.1.
