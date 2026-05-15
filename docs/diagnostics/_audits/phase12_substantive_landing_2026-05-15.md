---
title: Phase 12 substantive landing — lattice-boltzmann
date: 2026-05-15
author: claude-code
phase: 12
status: landed
scope: substantive-commit
---

# Phase 12 substantive landing audit

Lands the sim-local `volumetric-grid/lattice-boltzmann/` tree per the
architect-1 spec at `docs/phase12_lattice_boltzmann.md` (locked at
`c5955d3`). Follows the spec's § 3.D file generation order and fires
the six checkpoints in § 0.5 at the prescribed points.

## File inventory (sim-local)

| Path | Lines | Purpose |
|---|---:|---|
| `volumetric-grid/lattice-boltzmann/CMakeLists.txt` | 69 | sim-namespaced shader copy |
| `volumetric-grid/lattice-boltzmann/shaders/lattice_constants.glsl` | 95 | shared D3Q19 include (auto-derived from d3q19.md § 2.2) |
| `volumetric-grid/lattice-boltzmann/shaders/fullscreen.vert.glsl` | 22 | full-screen triangle vert |
| `volumetric-grid/lattice-boltzmann/shaders/init_equilibrium.comp.glsl` | 48 | f_i = feq seeding + moments init |
| `volumetric-grid/lattice-boltzmann/shaders/compute_moments.comp.glsl` | 51 | rho + u reconstruction |
| `volumetric-grid/lattice-boltzmann/shaders/collide.comp.glsl` | 62 | BGK relaxation |
| `volumetric-grid/lattice-boltzmann/shaders/stream.comp.glsl` | 48 | pull-semantics streaming |
| `volumetric-grid/lattice-boltzmann/shaders/apply_boundaries.comp.glsl` | 170 | halfway-BB + free-slip + equilibrium inlet/outlet |
| `volumetric-grid/lattice-boltzmann/shaders/streamline_advect.comp.glsl` | 79 | RK2 streamline advect + reseed |
| `volumetric-grid/lattice-boltzmann/shaders/velmag.frag.glsl` | 96 | velocity-magnitude raymarch (LUT-driven) |
| `volumetric-grid/lattice-boltzmann/shaders/streamline.vert.glsl` | 45 | per-streamline vertex (push-constant sid) |
| `volumetric-grid/lattice-boltzmann/shaders/streamline.frag.glsl` | 8 | streamline fragment passthrough |
| `volumetric-grid/lattice-boltzmann/src/main.cpp` | 1775 | sim entry, dispatch chain, capture, panel |
| `volumetric-grid/lattice-boltzmann/docs/load-bearing-decisions.md` | 97 | sim-local decision summary |
| `volumetric-grid/lattice-boltzmann/docs/notes.md` | 54 | v1 limitations + v1.1 polish backlog |
| `volumetric-grid/lattice-boltzmann/README.md` | 72 | controls / presets / tiers / refs |

Sim-local total: ~2.8k lines. Spec called for ~3000 — within band.

## Checkpoints fired

| # | Gate | Result |
|---|---|---|
| 1 | `lattice_constants.glsl` ↔ `d3q19.md` § 2.2 | PASS — 19 vectors, 19 weights, opposite-direction involution match `expected.json` byte-for-byte; `feq()` matches all 4 test points to <1e-12 |
| 2 | common-cpp surface re-verified against HEAD `c5955d3` | PASS — vk/context, vk/compute_pipeline, vk/buffer, vk/image all match consumed signatures |
| 3 | substep-at-equilibrium trace | PASS — collide identity at equilibrium, stream uniform-field invariance, moments reconstruct (ρ=1, u=(0.1,0,0)) |
| 4 | OPPOSITE_DIR involution sum | PASS — c_i + c_opp[i] = (0,0,0) for all 19 entries |
| 5 | Release + Debug build green | PASS — Release 13.1 MB, Debug 241 MB, both linked clean; only third-party imgui warnings + the pre-existing VkSamplerCreateInfo partial-init pattern (also emitted by eulerian-smoke) |
| 6 | Visual verification | DEFERRED — user-runtime on RX 6800 XT |

## Class B fix-forward substitutions (common-cpp surface)

Spec § 4.B referenced several APIs that don't exist or have different
shapes at HEAD `c5955d3`. Per the user's authorization, applied
substitutions and documented:

- `Renderer::beginFrame() -> Frame*` (nullptr on out-of-date) +
  `beginRendering(*frame, clear)` + `endRendering(*frame)` +
  `endFrame(*frame)`. NOT the spec's `acquireNextFrame` /
  `beginSwapchainPass` / `endSwapchainPass` / `submitAndPresent`.
- `gpusims::ui::*` free functions (`initImGui(ImGuiInit&)`,
  `newImGuiFrame()`, `renderImGui(cmd)`, `shutdownImGui()`,
  `pushToast(...)`). NOT a `gpusims::ImguiSetup` class.
- `HotReloader(poll_interval)` + `watch(file, cb)` + `poll()` +
  per-pipeline `apply_reload` lambda. NOT a single `tickAndApply` call.
- `GpuProfiler(ctx)` single-arg.
- `Buffer::readback(ctx, dst, bytes, offset=0)` 4-arg.
  `Image::readback(dst, bytes)` 2-arg.
- `GraphicsPipeline::bind(cmd, ds, push, push_size)` single call. After
  bind, `vkCmdDraw` directly. For per-streamline push-constant draws,
  `vkCmdPushConstants` directly inside the loop after a single bind.
- `gv::memoryBarrier(cmd, stage2, access2, stage2, access2)` —
  `VkPipelineStageFlags2` / `VkAccessFlags2` (sync2). Mirror ES legacy-
  bit-value usage (the legacy bit values alias correctly to sync2).
- `gpusims::vdb::writeVec3Grid(path, data, dims, voxel_size, origin,
  grid_name)` with host-side `snprintf` of `velocity_NNNNNN.vdb` for
  per-frame export. NOT the spec's fabricated `writeVec3Frame`.

## Class C fix-forward divergences (per user authorization)

- Runtime fields added: `raymarchSteps=128`, `velmagMin=0.0`,
  `velmagMax=0.1`, `velmagAbsorption=4.0`, `exposure=1.5`. (Spec's
  ImGui panel referenced these without declaring them on Runtime.)
- VDB readback: rgba16f → host half-to-float → vec3 floats →
  `writeVec3Grid`. Inline `half_to_float()` helper (~10 lines IEEE 754
  binary16→binary32). Banked v1.1: promote to common-cpp helper at
  consumer #2.
- Streamlines: dropped `primitive_restart_enable` + NaN-vertex
  sentinels (which only fire on indexed draws). Replaced with one
  `vkCmdDraw(cmd, history, 1, 0, 0)` per streamline + push-constant
  sid. Banked v1.1: switch to indexed draw with explicit restart
  sentinel.
- `desc_init` subgroup pinning dropped (init has no subgroup ops).
- NACA leading-edge math kept; comment clarified.
- Rest-direction split implemented per spec; perf measurement banked
  for v1.1 to decide whether the branching cost dominates the memory-
  traffic saving.

## Class C divergence beyond user enumeration (flagged for accept)

**Inlet/outlet uses equilibrium boundary, not Zou-He.** Spec § 4.G's
Zou-He formulas had at least two demonstrable transcription errors
(D2Q9 face-weight `2/3` for f_1 instead of D3Q19's `1/3`; f_16↔f_17
partition swap in f_7). The d3q19.md doc has no Zou-He section to
anchor against. First-principles re-derivation (Hecht-Harting Nx form)
failed moment-consistency on the y-momentum check.

Shipped first-order **equilibrium boundary** (`f_i = feq(rho_0,
u_inlet)` for unknowns at inlet; `feq(rho_0, u_extrap)` at outlet) as
a defensible v1; banked full Zou-He for v1.1 with
`tools/integrity/docs/algebraic/zou_he_d3q19.md` derivation-doc anchor
+ verification harness. Documented in
`volumetric-grid/lattice-boltzmann/docs/load-bearing-decisions.md` D6
and `docs/notes.md`.

## Class D doc-block clarifications (per user authorization)

- `collide.comp.glsl` doc-block: cites
  `references/lbm-principles-practice/chapter13/cpu/LBM.cpp:97-181`
  as "D2Q9 pattern reference; 3D D3Q19 generalization per
  tools/integrity/docs/algebraic/d3q19.md § 2.2".
- `apply_boundaries.comp.glsl` doc-block: cites
  `references/lbm-principles-practice/chapter5/poiseuille_BB.m:123-132`
  as "D2Q9 pattern reference; 3D D3Q19 generalization per
  tools/integrity/docs/algebraic/d3q19.md § 2.2 + § 5".

## Convention #8 firings caught and recorded

Eight fabrications surfaced through the 6-checkpoint protocol +
audit-trail discipline + verification greps. All caught before
reaching production code. Full list at
`docs/diagnostics/_audits/phase12_lattice_boltzmann_reanchor_2026-05-15_corrigendum.md`
§ B (firings 1-8). One additional firing surfaced at § 6.C verification
in this commit:

9. `cat3.d3q19-velocity-set` / `cat3.d3q19-weights` /
   `cat3.d3q19-equilibrium` integrity check IDs. Spec § 6.C asserted
   these were "added by setup-2"; in fact `d3q19_verify.py` exists as
   a standalone harness but was never registered as integrity checks.
   The shader's d3q19 constants ARE verified — Checkpoint 1 runs the
   same comparison against `expected.json` programmatically. Banked:
   the `d3q19_verify.py` harness should be promoted to registered
   integrity checks in a separate cleanup commit; until then, the
   Checkpoint 1 audit serves the same correctness role.

## What this commit does NOT touch

- `common/common-cpp/**` — no API surface change.
- `volumetric-grid/eulerian-smoke/**` — no ES changes (the `.bin.bin`
  quirk and unit-cube hardcoding remain banked).
- `tools/integrity/**` — already landed via setup commits 1 + 2.
- Cross-cutting § 5 edits — separate follow-up commit per Convention
  #12 (split-commit posture).
- Phase 9 / Phase 10 backfill rows in
  `docs/tier1-capture-format-reference.md` § 1 — banked per the
  deferred-backfill bank.

## Setup commits prerequisite

- `8fe355b` — vendor `lbm-principles-practice` MIT at SHA `6e2c592f`.
- `0db9c73` — `tools/integrity/docs/algebraic/d3q19.md` + verification
  harness + `d3q19_equilibrium.expected.json` + `[Algebraic_D3Q19]`
  registry entry.
- `c5955d3` — architect-1 spec at `docs/phase12_lattice_boltzmann.md`.
