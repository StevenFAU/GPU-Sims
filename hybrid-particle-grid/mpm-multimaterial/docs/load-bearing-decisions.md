# mpm-multimaterial — load-bearing decisions

This document banks the design decisions that constrain the rest of the sim's
shape, intended for future architect chats reading this folder. Every entry
ties a decision back to its rationale + the convention in `project-state.md` § 7
it inherits or extends.

## Three-material trio: water + jelly + snow (deliberate divergence from overarching-spec § 6)

The overarching-spec § 6 catalog reads "Multi-material sandbox: sand + jelly +
water in the same scene." Phase 9 ships water + jelly + snow.

**Why the divergence:** the canonical Taichi MLS-MPM upstream example
(`taichi-dev/taichi: python/taichi/examples/ggui_examples/mpm3d_ggui.py`) is
hard-coded to water + jelly + snow with the per-material plasticity branching
already validated for all three. Sand requires Drucker-Prager plasticity (a
frictional flow rule with cone-projected stress) — referenced in Klar et al.
2016 "Drucker-Prager Elastoplasticity for Sand Animation" (SIGGRAPH) — with
**no canonical Taichi reference upstream**. Implementing sand for Phase 9 would
be original research-grade work on top of three other first-of-pattern surfaces
(common-py bootstrap; first Stack D sim; first cross-backend optimization),
violating the Phase-8-banked single-architectural-load-per-phase discipline.

**Bank rationale:** ship the well-validated trio first; sand banked v1.1
stretch with the Drucker-Prager work documented at a high level. This is the
"draft against canonical references, not confident recall" fabrication
discipline from `project-state.md` § 7.

**What v1.1 sand would look like:** material index 3 (SAND); per-material
plasticity branch in `kernels.py` substep that replaces the SVD-based plastic
clamping with Drucker-Prager cone-projection of the symmetric Cauchy stress.
The reference implementation lives in the academic literature; Stack D would
add it once a real consumer of MPM-with-sand is named.

## Alembic deferred to natural sph-water consumer (deliberate divergence from Decision #8 default)

Decision #8 ("OpenVDB / Alembic are stubs in Phase 1; real impls land with
first consumer sim") would suggest MPM-multimaterial as Alembic consumer #1
since it's the first particle-cloud sim to ship. Phase 9 explicitly declines.

**Why the divergence:** `pyalembic` is not pip-installable. PyPI's `alembic`
package is the SQLAlchemy migration tool — same-name collision; same trap on
Ubuntu apt's `python3-alembic`. Real VFX-Alembic Python bindings require
source-build of `alembic/alembic` with `-DUSE_PYALEMBIC=ON`, conda-forge
`pyalembic`, or Houdini's `hou` module — each path locks every future
contributor and CI into manual setup that conflicts with overarching-spec § 5's
pip+pyproject.toml framing.

**Three weighed options:**

1. **Source-build pyalembic on dev hardware** — works but adds ~20 minutes of
   one-time build per contributor, and CI must either replicate or skip;
   neither outcome is clean.
2. **Shell out from Python to a new Stack C `mpm_abc_writer` binary** — makes
   Stack C's never-exercised Alembic stub the first-of-pattern surface in
   Phase 9 simultaneously with common-py bootstrap + MPM Tier-2, reproducing
   the bundled-risk problem the multi-architect chain is meant to mitigate.
3. **Defer Alembic to natural sph-water consumer; Phase 9 uses Taichi's
   built-in binary PLY for particles.** PLY is industry-standard for point
   clouds, Blender reads it natively for particle systems and Geometry Nodes
   instancing, and `ti.tools.PLYWriter` ships in Taichi 1.7.

**Phase 9 ships option 3.** sph-water (when it ships in some later phase) is
the canonical particle-cloud consumer Decision #8 was banked against; that
phase becomes Alembic consumer #1 with Stack C real-impl and the Python
binding designed together. The shape "design the abstraction at the consumer
that's the best fit, not the one that ships first" generalizes the
rule-of-three convention's "two prior consumers required before promotion"
posture.

**The "missing capability" deferred:** per-particle attribute interpolation
across frames for motion blur in animation hero renders. MPM-multimaterial's
v1 hero render is a single still or short animation; PLY frames + Blender's
particle system handles that cleanly. The Alembic-specific advantage matters
only once serious offline-render animation work begins — v1.1 polish territory.

## 250k / 96^3 default tier; 1M / 192^3 = capture-mode-not-interactive

Three discrete tiers via runtime dropdown:

| Tier | Particles | Grid | Target FPS | Interactive? |
|---|---|---|---|---|
| Default | 250 000 | 96^3 | 60 fps on RX 6800 XT + Vulkan; 60 fps on RTX 2080 Ti + CUDA | yes |
| Mid | 500 000 | 128^3 | 30–60 fps | yes |
| Stretch | 1 000 000 | 192^3 | 5–15 fps | **no** (capture-mode-only) |

**Why 250k default vs upstream's 128k:** the canonical Taichi mpm3d_ggui ships
128k @ 64^3 as a conservative default tuned for entry-level GPUs (laptop dGPU,
integrated). The AMD RX 6800 XT (16 GB VRAM, ~250 W TDP) is meaningfully above
that baseline; 250k @ 96^3 is realistic. **If underperformance shows up during
user-driven visual verification, the tier-dropdown shape makes the adjustment
cheap** — same polish-pass pattern as Phase 7's boids-3d 50k → 10k density
reduction.

**Why 1M = capture-mode:** matches the eulerian-smoke 384^3 stretch tier
framing — "not real-time-interactive; for offline-render frame generation."
README warning + dropdown UX explicitly says so.

## Cross-backend optimization: CUDA + Vulkan both first-class

`ti.init(arch=ti.gpu)` lets Taichi pick CUDA when available, else Vulkan, else
CPU. Both backends are first-class. CUDA-specific hints
(`ti.loop_config(block_dim=n_grid)`) are no-ops on Vulkan but preserved in the
kernel for CUDA perf. All grid scatter uses element-wise scalar atomic-adds
(the `for d in ti.static(range(3))` shape inherited from upstream), which works
on both backends without needing the Vulkan `VK_EXT_shader_atomic_float`
extension's vector-atomic variant.

**Banked:** the AMD RX 6800 XT dev desktop runs Vulkan; the RTX 2080 Ti lab PC
runs CUDA; both are first-class targets per
`project-state.md` § 7 "Stack D dual-backend posture" (new convention added by
Phase 9).

## State capture: loose-directory inheritance from Decision #16

Stack D inherits the loose-dir JSON + binary blobs schema from Decision #16
unchanged. State captures live at `captures/capture_NNNN/state.json` +
`<name>.bin` per buffer. The JSON schema matches Stack C 1:1 (top-level
`frame`, `meta`, `buffers`; per-buffer `{name, file, bytes, count, stride, dtype}`).

Cross-stack replay (Stack D writes, Stack C reads via `gpusims::StateReader`)
is theoretically possible via the shared schema — not exercised today; the
door stays open.

## VDB density grid: single-channel `grid_m`, not multi-channel per-material

The per-frame VDB export writes the post-P2G grid mass field (`grid_m`) as a
single `"density"` channel. This does NOT discriminate per-material — water-cell
mass and jelly-cell mass are both summed into `grid_m`.

**Why single-channel v1:** the canonical Cycles render path uses the PLY
particle export's per-vertex `material` int channel for material discrimination
via Blender Geometry Nodes; the VDB density grid is for atmospheric-volume
hero-render style, not for per-material visual treatment. Multi-channel VDB is
3× the disk usage and readback cost for a visual treatment that the particle
path already covers.

**Multi-channel VDB banked v1.1** if hero renders demand it.

## No HotReloader in common-py; Ctrl+C-and-rerun workflow documented

Taichi `@ti.kernel` decoration captures the function's Python AST at definition
time; editing kernel source requires a fresh Python process. Stack D's dev
workflow is therefore Ctrl+C, edit, rerun. Documented in this sim's README and
banked in `project-state.md` § 7 "Stack D has no HotReloader" as expected
behavior, not a missing feature.
