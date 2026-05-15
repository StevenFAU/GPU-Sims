# Lattice-Boltzmann — notes

## Known limitations (v1)

- **Inlet/outlet uses equilibrium boundary, not Zou-He.** First-order
  accurate; the wake structure is qualitatively correct but quantitative
  Re-matched studies should wait for the v1.1 Zou-He upgrade.
- **2.5D airfoil (extruded along Z).** Finite-span 3D wings deferred to a
  later sim with a more general obstacle representation.
- **F9 reloads moments only; non-equilibrium f-state is lost.** The sim
  re-equilibrates within a handful of substeps. For wind-tunnel-style
  quasi-steady-state the visual indistinguishability is acceptable;
  banked as v1.1 polish: `--capture-full-state` flag.
- **VDB export uses host-side half-to-float conversion** (~10 LOC,
  inline in main.cpp). Banked as common-cpp helper candidate when
  consumer #2 of half-to-float surfaces.
- **Hot-reload of `lattice_constants.glsl`** (`#include`d by 5 shaders)
  currently only re-triggers `collide` reload. Fan-out to all five
  consumers is banked v1.1.

## v1.1 polish backlog

- **Full Zou-He 3D inlet/outlet** with derivation anchored in d3q19.md
  § 6 (new section). Reanchors equilibrium-boundary v1.
- **AA-pattern or EsoTwist in-place streaming.** Halves f-buffer memory
  (~300 MB saved at default tier; ~2.4 GB at capture tier).
- **Fused stream-collide kernel.** ~5–10% perf on RDNA per Krüger; trades
  modularity for throughput. Both shader files can be textually fused.
- **Krüger factored equilibrium form.** Algebraically identical, ~5%
  perf via FMA chain.
- **Indexed-draw primitive restart for streamlines.** Cleaner than the
  per-streamline `vkCmdDraw` loop currently in use; lets one draw call
  emit all streamlines.
- **MRT (multiple-relaxation-time) collision.** Stability headroom at
  high Re.
- **Cubic-Hermite obstacle SDF.** Smoother halfway-BB convergence than
  the current sampled-perimeter winding-number test.
- **--capture-full-state flag.** Optional full f-state capture for
  hero-render archival use (~12 GB per F5 at capture tier).

## Frontier variants (separate phase, not v1.1)

- **16-bit moment-encoded LBM** (Chen et al. 2025); 50% memory cut.
- **Differentiable LBM** (XLB / OpenLB style).
- **GPU-AMR LBM** (Jaber et al. 2025).

These motivate the v1 architecture decisions (D2 SoA, D4 separate
dispatches) so they land additively without retrofit pain.

## Tier downgrade pattern

v1 supports tier change after `window.show()` via deferred re-allocation
(Phase 9/10/11 pattern). Visual flash at change is expected and
acceptable.
