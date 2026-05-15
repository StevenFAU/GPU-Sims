# Lattice Boltzmann (Phase 12)

D3Q19 BGK lattice Boltzmann method around a NACA airfoil. Free-fly
camera, live GPU-seeded streamlines, velocity-magnitude volume
raymarch. Stack C / Vulkan 1.3 / common-cpp. RX 6800 XT desktop
target.

**Status:** Implemented (Phase 12)
**Stack:** Stack C (Native C++)
**Spec:** [`docs/sim-specs/lattice-boltzmann.md`](../../docs/sim-specs/lattice-boltzmann.md)
**Phase spec:** [`docs/phase12_lattice_boltzmann.md`](../../docs/phase12_lattice_boltzmann.md)

## Build

From repo root:

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DGPU_SIMS_BUILD_EXAMPLES=ON -DGPU_SIMS_USE_OPENVDB=ON \
    -DGPU_SIMS_USE_ALEMBIC=ON
cmake --build build --target lattice_boltzmann
./build/bin/lattice_boltzmann
```

OpenVDB is optional; without it the VDB-export panel toggle becomes a
no-op and the binary still runs.

## Controls

| Key / Mouse | Action                              |
|-------------|-------------------------------------|
| WASD        | Move camera                         |
| RMB-drag    | Look                                |
| Q / E       | World-down / world-up               |
| Shift       | Boost movement speed                |
| F5          | Save capture                        |
| F9          | Load latest capture                 |
| Panel       | Presets, tiers, sliders, toggles    |

## Presets

- **NACA0012 — Low-Re**: laminar attached flow, Re ~ 80 at default tier.
- **NACA0012 — Med-Re**: vortex-shedding onset, Re ~ 230.
- **NACA4412 — Med-Re**: cambered airfoil, asymmetric wake, Re ~ 230.

## Tiers

| Tier                     | Cells   | f-state   | Notes                  |
|--------------------------|---------|-----------|------------------------|
| 128^3 (Laptop)           | 2.1 M   | ~150 MB   | Laptop / iGPU          |
| 256x128x128 (Desktop)    | 4.2 M   | ~610 MB   | RX 6800 XT default     |
| 512x256x256 (Capture)    | 33.6 M  | ~4.8 GB   | Hero render; ~5 FPS    |

Tier change is deferred-apply (after `window.show()`) per the Phase
9 / 10 / 11 pattern.

## References

- Krüger et al. 2017, *The Lattice Boltzmann Method: Principles and
  Practice* — math-pattern reference. Companion code vendored at
  `references/lbm-principles-practice/` (MIT, SHA `6e2c592f`). The
  vendored code is D2Q9 only; D3Q19 generalisations are pinned in the
  algebraic derivation below.
- `tools/integrity/docs/algebraic/d3q19.md` — D3Q19 velocity set,
  weights, opposite-direction table, equilibrium formula, halfway
  bounce-back. Authoritative ground-truth source for this sim.

## v1 limitations

See [`docs/notes.md`](docs/notes.md) for the v1.1 polish backlog. Most
notably: inlet/outlet uses first-order equilibrium boundary in v1;
Zou-He second-order upgrade is banked.
