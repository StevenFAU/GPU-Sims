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

## Tier calibration matches canonical upstream baseline (post-Phase-9 visual verification)

Three discrete tiers via runtime dropdown:

| Tier | Particles | Grid | Notes |
|---|---|---|---|
| Default | 128 000 | 64³ | matches canonical `mpm3d_ggui` upstream baseline; 60 fps interactive target |
| Mid | 250 000 | 96³ | larger demos; ~15-30 fps interactive |
| Stretch | 500 000 | 128³ | capture-mode-only; ~5-10 fps |

**Original Phase 9 spec had different numbers.** The first draft proposed
250k/96³ as the default with 60 fps target on RX 6800 XT + Vulkan, scaled
up from upstream's 128k/64³ baseline by 2× particles + 3.4× grid cells
via confident-recall extrapolation. Visual verification measured 10-15
fps at that tier — meaningfully below interactive. The architect-1 chat
drafted scaling assumptions without measuring upstream's actual baseline
or running comparison benchmarks on the target hardware. Same
fabrication-discipline shape as the Phase 9 schema and toolchain misses,
different surface (perf rather than correctness). Banked under the § 7
convention "Strict-mode CI configurations must be exercised against
representative code at draft time" — extended-meta would be "Perf claims
require measurement against representative code at draft time, not
extrapolation."

**1M / 192^3 tier deferred to v1.1.** The original stretch tier proposed
1 000 000 particles on a 192³ grid as capture-mode-only. Visual
verification showed particle-NaN explosion on unpause at this tier,
root-caused to two factors: (a) CFL violation at high grid resolution
with the spec's dt=2e-4 — particles cross too many grid cells per
substep to stay numerically stable, and (b) the BOUND=3 boundary-cell
band is relatively thinner at 192³ (1.6% of grid) than at 96³ (3.1%),
so particles approaching boundaries have less margin before exiting.
Stable shipping of the 1M tier requires tier-dependent dt scaling
(roughly dt ∝ 1/grid_size for explicit time integration of MPM-style
flows) and possibly tier-dependent BOUND. Implementation banked v1.1.

**Why 500k / 128³ becomes the stretch instead.** Same capture-mode
framing as the original 1M tier — "not real-time-interactive; for
offline-render frame generation." Same confirmation modal idiom
inherited from eulerian-smoke's 384³ stretch tier. The user-facing
expectation is unchanged; only the headline particle count shrinks
by 2× to a value the current dt actually supports stably.

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

## User-emitter reserve-tail allocation (polish-5)

LMB-place "sparse user-placed material-cube emitters" need to introduce NEW
material into the running simulation without destructively reclaiming
preset particles. The constraint: Taichi fields are fixed-size allocations,
so the total particle count is bounded at tier-creation time and cannot
grow mid-simulation without a kernel-recompile penalty (1-3 s UI lockup).

**Pre-polish-5 (broken):** `init_volumes` distributed `state.n_particles`
across preset volumes; the last volume took `state.n_particles - next_p`
to absorb roundoff (filling the entire pool). LMB-place computed
`slot_size = state.n_particles // MAX_USER_EMITTERS` and claimed
particles from `state.n_particles - (user_emitter_count + 1) * slot_size`
onward — i.e., from the END of the array, which the preset had already
filled. Result: each LMB-place destructively re-tagged 16 000 of the
preset's particles (default tier) as the new emitter material, teleporting
them from their preset position to the click location and resetting their
v/C/F/Jp state. Visually: the scene lost preset water in proportion to
emitters placed. Physically: a magic transmuter that no MLS-MPM equation
supports — particles are the simulation's mass-carrying state, and
arbitrary relabeling violates the data → visual fidelity contract.

**Post-polish-5 (reserve-tail):** two new module-level constants in
`main.py`:

```python
EMITTER_SLOT_SIZE: Final[int] = 2000
EMITTER_RESERVE_SIZE: Final[int] = MAX_USER_EMITTERS * EMITTER_SLOT_SIZE  # = 16 000
```

`init_volumes` now distributes `max_particles = state.n_particles -
EMITTER_RESERVE_SIZE` across preset volumes (instead of the full
`state.n_particles`). The tail region — indices
`(state.n_particles - EMITTER_RESERVE_SIZE)..state.n_particles` — stays
`F_used=0` (parked at `_UNUSED_PARK_POS`, invisible, not advanced by
the MLS-MPM substep).

LMB-place now computes
`first = state.n_particles - EMITTER_RESERVE_SIZE + user_emitter_count * EMITTER_SLOT_SIZE`
and claims `EMITTER_SLOT_SIZE` particles from the reserve. Physically:
mass introduced into the simulation from outside the domain, the same
open-system model Houdini / Mantaflow / Blender's Mantaflow integration
use for fluid emitters. Each click adds exactly
`EMITTER_SLOT_SIZE × p_mass` to the system; the value is documentable
in capture metadata for experimental logs.

**Sizing rationale for `EMITTER_SLOT_SIZE = 2000`:** at 64³ grid, the
natural per-particle volume is `p_vol = (dx * 0.5)³ ≈ 4.77e-7 unit³`.
A 0.08³ user cube has volume `5.12e-4 unit³`; density-matched count
`≈ 1070`. 2000 gives ~2× the natural density, within order-of-magnitude
of typical preset water (~1-3M particles/unit³). Total reserve at
default tier: `8 × 2000 = 16 000 particles = 12.5% of the 128k pool`.
Preset capacity drops from 128k to 112k — visually negligible. Tunable
per-tier in v1.1 if hero renders demand it.

**Inheritance from rule-of-three:** Option B (dynamic recycle from
inactive-particle scan) was considered and deferred. Rule-of-three says
defer abstractions until consumer #3. v1 has one consumer
(mpm-multimaterial); reserve-tail is sufficient. Future Stack D sims
that delete particles mid-sim (e.g., spalling fracture, particles
exiting the domain) would promote to Option B; that's the natural
consumer-#3 trigger.

**Save/load roundtrip (polish-5 bonus fix):** `do_save_state` writes
both `user_emitter_count` and `place_material` into the
`mpmMultimaterial` meta wrapper; `do_load_state` now restores both
(pre-polish-5 it didn't, latent roundtrip bug). After F9 load, the
next LMB-place resumes from the saved emitter slot index using the
saved place_material.

**Known corner case (banked, not fixed):** State captures saved BEFORE
polish-5 have `used=1` across positions `0..n_particles-1` (legacy
preset fills the entire pool). Loading those captures post-polish-5
restores the legacy layout; the reserve region is still active from
the loaded scene. The first LMB-place after such a load destructively
overwrites particles in the reserve tail (legacy behavior). Workaround:
after loading a legacy capture, press R to re-initialize from the
current preset under reserve-tail allocation, then place emitters.
Auto-migration of legacy captures was considered and rejected (would
silently delete loaded particles, worse UX than the corner case).

Cross-reference: convention `§ 7 "Stack D dual-backend posture"` and
`§ 7 "Architect-1 onboarding includes tier1-capture-format-reference.md"`
in `project-state.md` — both apply here as inheritance points (the
fix preserves Stack D backend agnosticism; the saved-state contract
follows the cross-stack schema from the capture-format reference).
