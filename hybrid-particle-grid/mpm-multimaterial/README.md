# MPM Multi-Material

**Status:** Implemented (Phase 9)
**Stack:** Stack D (Python / Taichi)
**Spec:** [`docs/sim-specs/mpm-multimaterial.md`](../../docs/sim-specs/mpm-multimaterial.md)

3D Material-Point-Method (MLS-MPM) sandbox with three materials — water, jelly, snow —
sharing the same grid. Three tier dropdown: 250 000 / 500 000 / 1 000 000 particles
on 96³ / 128³ / 192³ grids. The 1M tier is **capture-mode-only**: it runs at 5–15 fps
for offline-render frame capture, not real-time interaction.

Built on the canonical Taichi MLS-MPM upstream example (water + jelly + snow plasticity
branching preserved 1:1). Sand banked as v1.1 stretch — see
[`docs/load-bearing-decisions.md`](docs/load-bearing-decisions.md) for the
overarching-spec § 6 divergence rationale.

## Build

```bash
cd hybrid-particle-grid/mpm-multimaterial/python
python3 -m venv .venv && source .venv/bin/activate
pip install -e .
python main.py
```

Optional: install `python3-openvdb` (Ubuntu apt) to enable real VDB density export
(otherwise the toggle is a stub-warning no-op). Alembic export is **not available**
in Phase 9 — see [`docs/load-bearing-decisions.md`](docs/load-bearing-decisions.md) for
the deferral rationale; the natural sph-water phase becomes Alembic consumer #1.

## Controls

| Key / mouse | Action |
|---|---|
| `WASDQE` | Move (RMB held to look around) |
| `LMB` | Place a material cube at click-ray ground intersection (cap 8) |
| `M` | Cycle place-material (water → jelly → snow) |
| `F5` / `F9` | Save / load full simulation state |
| `R` | Reset to current preset |
| `Space` | Pause / unpause |
| `Esc` | Quit |

Side panels:

- **Presets** — Single Dam Break / Double Dam Break / Water-Snow-Jelly / Mixed Sandbox.
- **Tier** — 250k / 500k / 1M. 1M shows a capture-mode confirmation modal.
- **Gravity** — three sliders (X / Y / Z) in m/s².
- **Materials** — RGB color pickers per material; current "place" material display.
- **Export** — VDB density (per-frame, every 4 frames); PLY particles (per-frame, every 4 frames).

## Hot-reload

Stack D has no in-process kernel hot-reload — Taichi's `@ti.kernel` decoration captures
the function's Python AST at definition time. Dev workflow: **Ctrl+C, edit, re-run**.
See [`common/common-py/README.md`](../../common/common-py/README.md#hot-reload) for the
banked rationale.

## Performance

Measured on AMD RX 6800 XT + Taichi Vulkan (Phase 9 visual verification, polish-4 recalibration):

| Tier | Particles | Grid | Vulkan / RX 6800 XT |
|---|---|---|---|
| Default | 128 000 | 64³ | _TBD — fill in after Phase 9 polish-4 verification_ |
| Mid | 250 000 | 96³ | _TBD_ |
| Stretch (capture-mode) | 500 000 | 128³ | _TBD_ |

CUDA / RTX 2080 Ti numbers TBD once that hardware is exercised. The 1M / 192³ tier
referenced in the original Phase 9 spec is deferred to v1.1 — current dt=2e-4 violates
CFL at 192³ grid resolution. Investigation banked in `docs/notes.md`.

## Hero render

Per-frame binary PLY export (positions + per-vertex `material` int channel) feeds
[`render-pipelines/blender/render_mpm.py`](../../render-pipelines/blender/render_mpm.py)
for offline Blender Cycles renders. Three Cycles materials are constructed via the
bpy API (no `.blend` dependency): Water (transmission 1.0, IOR 1.33), Jelly
(subsurface scattering), Snow (rough diffuse + slight emission). Geometry Nodes
instances a small icosahedron per particle with material slot driven by the
named-attribute "material".

Single-still v1 deliverable; animation (--frame-start / --frame-end) is the v1.1
A100-batch hero render.

## References

- **Canonical MLS-MPM:** Hu et al. 2018 "A Moving Least Squares Material Point Method
  with Displacement Discontinuity and Two-Way Rigid Body Coupling" (SIGGRAPH).
- **Taichi reference implementation:** `taichi-dev/taichi`
  `python/taichi/examples/ggui_examples/mpm3d_ggui.py` (MIT license; per-material
  plasticity branching preserved 1:1).
- **Snow plasticity:** Stomakhin et al. 2013 "A Material Point Method for Snow
  Simulation" (SIGGRAPH).
- **Sand plasticity (v1.1 banked):** Klar et al. 2016 "Drucker-Prager Elastoplasticity
  for Sand Animation" (SIGGRAPH).
