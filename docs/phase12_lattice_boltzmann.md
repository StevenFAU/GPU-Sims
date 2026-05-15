# Phase 12 — `volumetric-grid/lattice-boltzmann/` (Stack C, D3Q19 BGK around a NACA airfoil)

> **Spec author:** architect-1 (single-architect chain per coordinator instruction; no architect-2 review for Phase 12).
> **Spec date:** 2026-05-15.
> **Substantive anchor:** to be filled at commit time. The two prep commits are landed: `8fe355b` (vendored [Krueger]) and `0db9c73` (added [Algebraic_D3Q19]).
> **Repo HEAD at spec lock:** `0db9c73a9349cff1bc22723c9491451cd1a33994`.
>
> **Document audience:** Claude Code, executing this spec end-to-end without a second architect chat in the loop. The author has done the multi-source verification; the spec author's responsibility is to be unambiguous, the executor's is to flag genuine ambiguities rather than guess.
>
> **Document length:** ~3000 lines. Per Phase 11 precedent (`docs/phase11_sph_water.md`, ~5300 lines), this fits within Claude Code's execution context budget; smaller than Phase 11 because LBM is algorithmically simpler than DFSPH and there is no first-exercise-of-a-stubbed-library to specify.

---

## § 0 — Preamble and hard rules

§ = "section." Notation used throughout: § 0 refers to this section, § 4.B.3 to subsection B paragraph 3 of section 4, etc.

### § 0.1 Required reads before drafting any execution plan

Claude Code reads all of the following before generating any source file or running any build. The reads are **not optional** — every spec assertion below is anchored to text in one of these documents, and an execution that bypasses the reads will surface as drift at the verification block in § 6.

1. **`project-state.md`** — repository-level conventions. Particular attention to § 4 (locked decisions), § 7 (conventions, especially the fabrication-discipline H3 entries), § 9 (known issues including the Phase 8 `.bin.bin` quirk and the `writeVec3Grid` GRID_STAGGERED note).
2. **`docs/overarching-spec.md`** — the cross-cutting design philosophy. § 5–§ 6 establishes the category structure; § 7 the per-sim spec sheet template.
3. **`docs/conventions.md`** — coding conventions for shader naming, file layout, snake_case binary names.
4. **`docs/diagnostics/_audits/phase12_lbm_probe_2026-05-15_architect1.md`** — the pre-spec probe. Contains verbatim quotes of the common-cpp public surface as it exists at HEAD `0db9c73`, the eulerian-smoke (`volumetric-grid/eulerian-smoke/src/main.cpp`) precedent for substep loop, descriptor-set construction, raymarch graphics pipeline, and capture (F5) call site. The spec assertions in § 4 below are anchored to file:line citations in this probe; if a citation in the spec disagrees with a re-read of the actual source, the source wins and Claude Code reports the drift.
5. **`docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md`** — the second probe. Krüger book code characterization, RX 6800 XT subgroup-size-control properties (measured), and the D2Q9-only finding that motivated the algebraic-D3Q19 derivation.
6. **`tools/integrity/docs/algebraic/d3q19.md`** — the algebraic ground-truth derivation landed at prep commit 2 (`0db9c73`). § 2.2 of that document pins the canonical 19-direction ordering used throughout Phase 12's shaders. **This document is the single source of truth for the 19 velocity vectors and 3 weight values; never derive them inline in a shader doc-block.**
7. **`references/lbm-principles-practice/chapter13/cpu/LBM.cpp:97-181`** — the Krüger D2Q9 fused stream-collide kernel. Used as the pattern reference for the BGK collision math; lines 235–273 hold the equilibrium expansion in the factored form `omtauinv*f + tau_inv*w*rho*[1 - 1.5(u·u) + (c·3u)(1 + 0.5(c·3u))]`. Phase 12's `collide.comp.glsl` cites this in its doc-block.
8. **`references/lbm-principles-practice/chapter5/poiseuille_BB.m:123-132`** — the halfway bounce-back pattern. D2Q9; 3D generalization uses the opposite-direction pairs from `d3q19.md` § 2.2.
9. **`common/common-cpp/examples/hello/main.cpp`** — the canonical Stack C consumer of the common-cpp API surface. Includes the `--test-subgroup-size` flag for runtime subgroup-size-control verification (landed at commit `9e0ca2f`).
10. **`volumetric-grid/eulerian-smoke/src/main.cpp` and `volumetric-grid/eulerian-smoke/CMakeLists.txt`** — the closest Stack C precedent. ES's substep loop structure (`:1879-2012` per probe-1 § D), descriptor-set construction (`:1168-1208`), raymarch graphics pipeline (`:1144-1158`), and CMake pattern (the 65-line file with `GPU_SIMS_ES_SHADER_DIR` define + per-sim shader-copy `foreach` loop) are template precedents for Phase 12.

### § 0.2 Hard rules

These rules govern every execution decision. Violation surfaces at audit time and at the verification block; pre-emptive compliance is required.

1. **Grep before asserting** (Convention #8, repository-wide). No specific numeric value, file:line citation, function signature, struct field name, or CMake variable name is asserted from memory. Every such fact is grep-verified against the synced repo state before the corresponding shader / source file is written. If a grep returns a different value than the spec claims, the source wins and Claude Code reports the drift in the completion notes.

2. **In-flight fix authorization** (Convention #4-extension, rule-of-three-runs-in-reverse). Phase 12 is consumer #2 of the `writeVec3Grid` real implementation. Per the rule-of-three pattern in reverse, consumer #2 surfaces defects in the consumer-#1 implementation that the consumer-#1 build did not catch in isolation. Specifically: the `GRID_STAGGERED` grid-class tag is wrong for LBM cell-centered velocity. **In-flight fix authorization granted** to extend `writeVec3Grid`'s signature with a `VkGridClass` parameter defaulting to `GRID_STAGGERED` for backward compatibility, OR to leave the staggered tag in place (acceptable — downstream Blender/Houdini visualization mostly ignores grid-class for color-magnitude rendering). Claude Code picks one path and documents the choice in the completion notes. **Do not silently change the signature without documenting.**

3. **Probe-before-anchor-lock** (Phase 11 banking, the meta-rule). Before writing the first byte of a shader that consumes a common-cpp API, re-read the actual header at HEAD `0db9c73` and confirm the spec's assertion. The probes are already done; this rule says "re-confirm at write time" because the working tree may have moved since the probes ran on 2026-05-15. If the working tree state has changed materially since probe-1, STOP and report.

4. **SHA backfill discipline** (Phase 10.1 banking). The substantive commit's `project-state.md` self-references use the placeholder `<PHASE_12_SHA>`. A separate follow-up commit replaces the placeholder with the actual substantive-commit SHA. **Never use `git commit --amend`** for this — the back-fill is always a follow-up commit referencing the previous commit's stable SHA.

5. **Anchor-string grep verification for § 5 cross-cutting edits**. Every anchor string in § 5 is grep-verified against the synced repo before Claude Code locks the edit. Anchors are line-start matches, not substring matches; if the anchor pattern matches multiple lines or zero lines in the synced file, STOP and report.

6. **No fabrication on the D3Q19 constants**. The 19 velocity vectors, the 3 weight values, and the canonical ordering come from `tools/integrity/docs/algebraic/d3q19.md` § 2.2 only. The shaders' `lattice_constants.glsl` include is generated from that document mechanically (§ 4.C below); the values are not retyped from memory.

7. **No silent variant on the equilibrium formula**. The compressible Maxwell-Boltzmann equilibrium `feq_i = ω_i ρ [1 + 3(c·u) + 4.5(c·u)² − 1.5 (u·u)]` is the v1 form. The incompressible-linearized variant is rejected. The factored Krüger form `feq_i = ω_i ρ [(1 − 1.5(u·u)) + (3 c·u)(1 + 0.5 · 3 c·u)]` is algebraically equivalent and acceptable as a performance-optimized rewrite, but the rewrite must be commented inline with both forms shown for review.

8. **Architect-2 substitute**. There is no architect-2 chat for Phase 12. The function architect-2 served in Phase 11 — catching drift between the spec and the synced source — is partially delegated to Claude Code (which can re-verify any spec assertion via grep on demand) and partially banked: the spec author has done extra-careful probe work pre-lock, but the spec is not externally reviewed before execution. **If Claude Code encounters genuine ambiguity** — two readings of a spec assertion both internally consistent, or a spec citation that doesn't match the synced source — **the protocol is to pause and report, not to guess.** Phase 11's architect-2 callouts in § 0.5 of that spec are replaced in this spec by Claude Code execution checkpoints in § 0.5 below.

### § 0.3 Spec → execution mapping

The spec is structured for execution in this order:

1. **§ 1 + § 2 + § 3** — read-once context. Claude Code internalizes the goal, the 13 load-bearing decisions, and the file manifest before generating anything.
2. **§ 4** — per-file specs. Files are generated in the order listed in § 3 (deps before dependents). Each subsection is independently executable but most have implicit deps on prior subsections (e.g., `collide.comp.glsl` depends on `lattice_constants.glsl`).
3. **§ 5** — cross-cutting modifications. Land AFTER all sim-local files are in place and the sim compiles. The CI yaml change in § 5.A makes the new sim's path triggers active.
4. **§ 6** — verification. Every grep / build / run check passes before the substantive commit lands.
5. **§ 7** — commit message + retro plan + SHA-backfill follow-up plan.

### § 0.4 Convention names referenced in this spec

For Claude Code's read efficiency, conventions cited herein:

- **Convention #4** — rule of three. After three independent firings of the same pattern, promote to shared infrastructure.
- **Convention #4-extension** — rule of three runs in reverse. Consumer N+1 validates consumer N's implementation. Phase 12 is consumer #2 of `writeVec3Grid`.
- **Convention #8** — architect-1 fabrication pattern. Grep before asserting.
- **Convention #8-lateral** — probe-before-draft-lock. Empirical probe against actual environment before asserting an external-dependency fact.
- **Convention #12** — SHA back-fill is always a separate follow-up commit, never `--amend`.
- **Frame-invariant capture convention (NEW, banked at Phase 12)** — for captures of fields that don't change per-frame (obstacle masks, scene SDFs, immutable presets), the `saveBuffer` meta carries `"frame_invariant": true` and the buffer is written only on the first F5 of a session per preset.

---

## § 0.5 — Claude Code execution checkpoints

These six checkpoints replace Phase 11's architect-2 callout structure. At each checkpoint, Claude Code pauses, runs the listed verification step, reports the result in the running execution log, and only then proceeds. The protocol for a failed check is: report the failure with exact discrepancy, do NOT attempt to fix forward without explicit authorization from the coordinator.

### Checkpoint 1 — `lattice_constants.glsl` agreement with `d3q19.md` § 2.2

**Before generating any other shader file**, Claude Code generates `volumetric-grid/lattice-boltzmann/shaders/lattice_constants.glsl` per § 4.C below, then runs a programmatic cross-check: parse the 19 velocity vectors and 19 weights out of the generated GLSL, compare to the canonical ordering and values pinned in `tools/integrity/docs/algebraic/d3q19.md` § 2.2 and § 3.1. Mismatch on any of the 19 entries → STOP, report.

### Checkpoint 2 — common-cpp surface re-verification

**Before writing `main.cpp` § 4.B.5** (pipeline creation), Claude Code re-reads the four common-cpp headers used most heavily (`vk/context.hpp`, `vk/compute_pipeline.hpp`, `vk/buffer.hpp`, `vk/image.hpp`) and confirms that the API signatures the spec assumes are still those at HEAD. Specifically: `ContextCreateInfo::enable_subgroup_size_control` exists; `ComputePipelineDesc::required_subgroup_size` exists; `Buffer::readback` exists; `Image` supports `ImageType::e3D`. If any has drifted since probe-1, STOP, report.

### Checkpoint 3 — Substep loop correctness against d3q19.md derivation

**After generating `collide.comp.glsl` and `stream.comp.glsl` but before `apply_boundaries.comp.glsl`**, Claude Code mentally executes one substep at a test point: ρ = 1, **u** = (0.1, 0, 0), pre-collision f_i = ω_i (zero-velocity equilibrium). Expected post-collision values come from `tools/integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.expected.json` (the test-point-2 row). If the spec's collide.glsl operations do not produce the expected values at this test point, STOP, report. **This is the same test point the cat3 numerical correctness check will use post-implementation; the checkpoint just runs it earlier, in the spec author's head as recorded in the spec.**

### Checkpoint 4 — Halfway bounce-back direction mapping

**Before generating `apply_boundaries.comp.glsl`**, Claude Code writes out the opposite-direction pair table from `d3q19.md` § 2.2 as a GLSL constant array `OPPOSITE_DIR[19]`, then verifies via a sum check: for each i ∈ [0, 19), `c_i + c_OPPOSITE_DIR[i] = (0, 0, 0)`. Mismatch → STOP. **The halfway-BB shader correctness depends entirely on this table being right.**

### Checkpoint 5 — CMake build green (Release + Debug) before § 5

**After all sim-local files are written but before any § 5 cross-cutting edit lands**, Claude Code runs:

```
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPU_SIMS_BUILD_EXAMPLES=ON -DGPU_SIMS_USE_OPENVDB=ON -DGPU_SIMS_USE_ALEMBIC=ON
cmake --build build-release --target lattice_boltzmann --parallel
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DGPU_SIMS_BUILD_EXAMPLES=ON -DGPU_SIMS_USE_ALEMBIC=ON
cmake --build build-debug --target lattice_boltzmann --parallel
```

Both must succeed before § 5 edits land. (§ 5.A extends the CI path triggers; landing it before the binary builds locally creates a CI-red commit on push.)

### Checkpoint 6 — Visual verification gate (deferred to user-runtime)

The shader-level correctness checks (Checkpoints 1, 3, 4) are mechanical. The visual verification — does the airfoil flow look like an airfoil flow, are the streamlines smooth and physically plausible, does the no-slip on the airfoil read correctly — is necessarily user-runtime. Steven runs the binary on the RX 6800 XT after Claude Code reports Checkpoint 5 green, and reports back any visual defects for Phase 12.5 polish or in-flight Phase 12 fix authorization per the polish-cadence pattern.

---

## § 1 — Goal and sim character

`volumetric-grid/lattice-boltzmann/` is the second Stack C volumetric-grid sim and Phase 12 of the GPU-Sims portfolio. It demonstrates **divergence-free incompressible-ish fluid flow around an airfoil obstacle** using the D3Q19 BGK lattice Boltzmann method, with **live streamlines** revealing the velocity field's structure and a **volume raymarch of velocity magnitude** providing the dense-field visualization.

The headline interactive moments:

- **Preset dropdown.** Three presets at v1: `NACA0012-Low-Re` (Re ≈ 200, laminar attached flow), `NACA0012-Med-Re` (Re ≈ 800, vortex shedding begins), `NACA4412-Med-Re` (cambered airfoil at moderate Re, asymmetric wake). Preset switch reinitializes the f_i distributions to equilibrium with the preset's free-stream **u**_∞, reseeds the streamlines, and revoxelizes the obstacle mask if the airfoil shape differs.

- **Tier dropdown.** Three tiers with deferred-change-after-window-show (Phase 9/10/11 pattern). Tier change triggers full re-allocation of the f / f_new distribution buffers, the moment buffers, the obstacle mask, and the render targets:
  - **128³ Laptop** (~2M cells; ~610 MB for f+f_new)
  - **256×128×128 Desktop** (~4M cells; ~1.2 GB for f+f_new) — design target on RX 6800 XT
  - **512×256×256 Capture-mode** (~33M cells; ~9.5 GB for f+f_new) — feasible memory-wise; interactivity ≤10 fps

- **Free-fly camera.** `gpusims::Camera::Mode::FreeFly` (the ES + sph-water precedent). WASD movement, RMB-drag look, Q/E for world-up/down, Shift for boost.

- **Streamlines toggle.** Checkbox enabling GPU-seeded streamlines (~10k seed points, each tracing a 64-position ring-buffer history advected by trilinear-sampled velocity). The streamlines are visually load-bearing — they're the reason this sim demonstrates LBM rather than "another smoke sim" — and the toggle exists for performance comparison at the 512×256×256 tier where streamline cost becomes non-trivial.

- **Volume raymarch toggle.** Velocity-magnitude → colormap-LUT, similar to ES's smoke raymarch in outer structure but stripped of physical absorption/emission/scattering (LBM velocity-magnitude is a scalar visualization, not a participating-media transport problem).

- **F5 / F9 capture/load.** Standard cross-stack pattern. F5 writes `captures/capture_<NNNN>/state.json + buffers`. F9 finds the latest under `captures/` and reloads. Distribution functions are not captured (too large; re-derived from saved ρ and u via equilibrium at load time).

- **Live HUD.** FPS, current Reynolds number derived from current u_∞ and viscosity, current substep dt, per-pass GPU times (collide / stream / boundaries / moments / streamline-advect / raymarch / streamline-render).

The sim's character is **calm, structured, and analytical**, in contrast to ES's volumetric-explosion character and sph-water's chaotic-liquid character. The intended viewer experience: "I can see the air flowing around the wing, and I can see where it separates and where it stays attached."

---

## § 2 — Load-bearing decisions

Thirteen numbered decisions, each with rationale and rejection-of-alternatives. Decision numbers are stable references used throughout § 4.

### Decision 1 — Algorithm: D3Q19 BGK single relaxation time

**Choice:** Canonical D3Q19 lattice with BGK single-relaxation-time collision operator. Equilibrium per § 0.2 hard rule 7.

**Rationale:** Matches the overarching-spec catalog entry (`docs/overarching-spec.md:215` — "512×256×256 D3Q19 LBM around an airfoil"). Canonical 1990s technique with five+ textbook presentations and dozens of permissive-license implementations to cross-reference if a numerical question arises. Pedagogically appropriate for the portfolio's "scientific amazement through physical correctness at maximum scale" framing.

**Rejected alternatives:**
- **D3Q27 BGK**: 8 more directions, ~40% performance cost per Krüger 2017 Ch 13, marginal accuracy improvement for the airfoil Reynolds-number range this sim targets.
- **D3Q19 MRT (multiple-relaxation-time)**: stability advantages at high Re but adds 19×19 transformation matrices and per-moment relaxation rates; out of scope for v1.
- **D3Q15**: 4 fewer directions, lower accuracy, no compelling reason given D3Q19's spec-mandate.

**Frontier note:** Moment-encoded LBM with 16-bit quantization (Chen et al. 2025), differentiable LBM (XLB, OpenLB), and GPU-AMR LBM (Jaber et al. 2025) are explicitly frontier-variant work for a separate future sim under `volumetric-grid/` (provisional name `lattice-boltzmann-warp` or `lattice-boltzmann-amr`). Phase 12 ships the canonical reference; Phase 12 architecture decisions below (§ 2 decisions 2, 3, 4) are deliberately chosen to make those frontier variants additive rather than retrofit-painful.

### Decision 2 — Storage layout: SoA, one buffer per direction (19 + 19 buffers ping-pong)

**Choice:** Structure-of-arrays. Each of the 19 distribution functions f_i is its own `r32f` 3D image, totaling 19 ping (`f_ping[0]` … `f_ping[18]`) + 19 pong (`f_new_ping[0]` … `f_new_pong[18]`) = 38 3D images.

**Reasoning:** SoA isolates each direction's memory access pattern, which is critical for streaming (each f_i streams along its own c_i vector, so reads and writes are along that vector's axis). It also future-proofs the variants the gap analysis flagged:

- **Sparse VDB**: each f_i can be independently sparsified per its own active region.
- **16-bit quantization**: per-direction dtype is straightforward; can quantize f_i independently per direction's dynamic range.
- **Differentiable adjoint**: each f_i is a pure tensor; per-direction gradients are trivial to track.
- **Warp port**: Warp's idiomatic style is per-field tensors; SoA matches.

**Memory math at the 256×128×128 desktop tier:**
- 19 directions × 4 bytes × 4M cells = 304 MB for `f_ping`
- Same for `f_new_pong` = 304 MB
- Total f-state: **608 MB**
- Plus moments (ρ r32f + u rgba16f = 4 + 8 = 12 bytes × 4M = 48 MB)
- Plus obstacle mask (r8 × 4M = 4 MB)
- Plus render targets (depth + raymarch composite, ~25 MB)
- Plus streamline state (~10k particles × 64 history × 12 bytes = 7.7 MB)
- **Grand total at default tier: ~700 MB** — easily fits in 16 GB.

**At the 512×256×256 capture tier:**
- 19 × 4 × 33M × 2 = **4.8 GB** for f-state alone.
- Total ~5.2 GB. Fits in 16 GB with margin.

**Rejected alternatives:**
- **AoS (Array of structures)**: one 3D image of `vec4` × 5 or similar packed format. Cache-friendly for moment computation (reading all 19 f at one cell touches contiguous memory), but cache-hostile for streaming (each f_i needs to stream along a different axis). Streaming dominates per-cell compute; AoS loses.
- **Single tightly-packed buffer with stride 19**: same cache-pattern issue as AoS for streaming.
- **Rest-direction stored separately (Krüger chapter13 pattern)**: f_0 is its own scalar buffer; f_1..f_18 in 18-direction array. Saves 1/19 of memory traffic per substep (the rest direction never streams). **Adopted as a refinement of Decision 2** — see § 4.B.4 below for the buffer-creation specifics.

### Decision 3 — Streaming pattern: two-buffer ping-pong with parity bit

**Choice:** `f_ping` and `f_new_pong` (or equivalently `f_pong` and `f_new_ping`) — parity bit toggled per substep selects which is read and which is written. Same pattern as ES's `velocity_ping` / `velocity_pong`.

**Reasoning:** Simple, correct, debuggable. Memory cost is 2× the f-state (already budgeted in Decision 2).

**Rejected alternatives:**
- **AA-pattern in-place streaming** (Geier 2017, Bauer 2018): memory-halving in-place streaming via clever direction-bit-flipping. Saves ~300 MB at default tier. **Banked as v1.1 optimization** — not in v1 because the bit-twiddling addressing logic is delicate and the rule-of-three pattern says "ship the canonical implementation, then optimize."
- **EsoTwist** (Geier 2017): another in-place variant. Same banking — not in v1.

### Decision 4 — Collision and streaming as separate compute dispatches

**Choice:** `collide.comp.glsl` runs first (BGK relaxation in-place on `f_ping`), then `stream.comp.glsl` (writes `f_new_pong[x] = f_ping[x − c_i]` for each direction at each cell). Two compute pipeline dispatches per substep, separated by a memory barrier.

**Reasoning:** The fused stream-collide kernel from Krüger chapter13/cpu/LBM.cpp:97 is ~5–10% faster but couples two algorithmically-distinct operations into one shader. Phase 12 v1 favors **modularity over throughput** because:

- **Adjoint cleanliness**: each dispatch is a pure side-effect-free map for a future differentiable port.
- **Debuggability**: pause-after-collide or pause-after-stream is trivial with separate dispatches.
- **Variant-friendliness**: a moment-encoded LBM variant replaces collide; AMR replaces stream; neither touches the other.

**Rejected alternative:**
- **Fused stream-collide single dispatch** (Krüger chapter13/cpu, Bauer 2018 waLBerla codegen): ~5–10% faster. Banked as v1.1 optimization. The collide and stream shaders are written such that fusing is a textually-mechanical step at v1.1 time.

### Decision 5 — Macroscopic moments ρ and u in dedicated buffers

**Choice:** A separate `compute_moments.comp.glsl` reads `f_new_pong` after streaming and writes `rho` (r32f 3D image) and `velocity` (rgba16f 3D image, 4th component unused). Moments are computed once per substep, after stream and before any consumer (collide of next substep, raymarch render, streamline-advect).

**Reasoning:** Three downstream consumers need ρ and u:
1. The next substep's collide kernel (computes equilibrium from current moments).
2. The volume raymarch (samples |u| trilinearly via sampler3D).
3. The streamline advection (samples u trilinearly).

Recomputing moments inline in each consumer would triple the redundant work. Storing them as cell-centered fields lets each consumer trilinearly interpolate via the standard 3D-image sampler.

**Why rgba16f for velocity:** Half-precision is sufficient for the dynamic range of LBM lattice velocities (|u| < 0.1 c_s for stability). The 4th component is unused at v1 (banked as a candidate for vorticity-magnitude or pressure later). Matches ES's `velocity_ping/pong` rgba16f convention.

**Rejected alternative:**
- **Compute moments inline in collide.comp.glsl, never write them to memory**: works for the collide consumer but breaks the raymarch and streamline consumers. Would force inline 19-tap reductions in the fragment shader and the streamline-advect kernel. Untenable.

### Decision 6 — Boundary handling: free-slip walls + Zou-He inlet/outlet + halfway-BB airfoil

**Choice:** Three distinct boundary regimes, applied in `apply_boundaries.comp.glsl`:

- **+X face (downstream): pressure outlet** (density fixed at ρ_∞ = 1.0). Zou-He outlet boundary, second-order accurate.
- **−X face (upstream): velocity inlet** (uniform free-stream **u**_∞, set per preset; ρ derived from Zou-He inlet equation). Second-order accurate.
- **±Y, ±Z faces: free-slip** (specular reflection: f_i reaching the wall is reflected to f_i' where c_i' has the wall-normal component negated). Cheaper than no-slip and visually correct for an "infinite wind-tunnel" framing.
- **Interior obstacle (airfoil): halfway bounce-back** per Krüger chapter5/poiseuille_BB.m:123 pattern, 3D-generalized via the opposite-direction pairs in d3q19.md § 2.2. Second-order accurate in space (the no-slip surface is implicitly halfway between fluid and solid cells along each lattice link).

**Reasoning:** Each regime is the textbook choice for its boundary type at the Reynolds numbers this sim targets. Zou-He inlet/outlet at the X-faces gives clean steady inflow and non-reflecting outflow. Free-slip at Y/Z is the closed-form correct choice for a wind tunnel of effectively infinite cross-section.

**Rejected alternative:**
- **Periodic boundaries everywhere except the obstacle**: simpler to implement but visually wrong (flow leaving the +X face would wrap around to the −X face, ruining the wake structure).

### Decision 7 — Obstacle representation: CPU-generated NACA SDF voxelized to 3D r8 mask

**Choice:** At preset load:
1. CPU code computes the analytical NACA-4-digit airfoil cross-section in the X-Y plane using the closed-form thickness + camber equations.
2. The cross-section is extruded along Z (full span across the domain) — the airfoil is treated as a 2.5D extrusion, not a finite-span 3D wing.
3. For each cell (x, y, z) in the domain, the signed distance to the airfoil surface is computed (closed-form for the NACA 4-digit shape). Cells with SDF < 0 are marked solid (mask value = 1); SDF ≥ 0 are fluid (mask value = 0).
4. The 3D mask is uploaded once via `Image::upload` to an `r8uint` 3D image (`obstacle_mask`).
5. `apply_boundaries.comp.glsl` reads `obstacle_mask` and applies halfway-BB to fluid cells adjacent to solid cells.

**Reasoning:** Pre-voxelized mask separates the obstacle-geometry problem (CPU-side, ~50 LOC of math) from the LBM solver problem (GPU-side, only needs to know "is my neighbor solid"). The CPU voxelization is one-time per preset, not per-frame.

**NACA-4-digit math reference:** standard NACA 4-digit airfoil equations (NACA Report 460, Jacobs et al. 1933). Documented inline in `main.cpp` (§ 4.B.6 below) with citations.

**Net-new infrastructure:** No SDF / obstacle facility exists in common-cpp or in eulerian-smoke (probe-1 § H confirmed). The NACA voxelization is sim-local code, ~50 LOC in `main.cpp`. **Not promoted to common-cpp at v1** — rule-of-three says wait for consumer #2 (the next sim with an obstacle mask, likely PIC/FLIP or a future flow sim) before promoting.

**Rejected alternatives:**
- **Mesh-based obstacle (load OBJ, triangle-rasterize into a 3D mask)**: overkill for an analytical airfoil. Banked for later sims with arbitrary obstacles.
- **GPU-side SDF generation**: a compute kernel that evaluates the NACA SDF at each cell. Saves the one-time CPU step but complicates testing. Banked for v1.1.

### Decision 8 — Render: sim-local `velmag.frag.glsl`, NOT rule-of-three promotion

**Choice:** Phase 12 ships a sim-local `velmag.frag.glsl` (~80 lines) for the volume raymarch of velocity magnitude. The eulerian-smoke `raymarch.frag.glsl` is NOT promoted to common-cpp at v1.

**Reasoning:** ES's raymarch is **physically-driven** (single-scattering absorption + emission + Beer-Lambert + temperature-driven black-body LUT). LBM's velocity-magnitude raymarch is **transfer-function-driven** (scalar magnitude → colormap LUT). They share *outer structure* (slab intersection, jittered ray march, early termination on transmittance) but the inner shading loop differs substantially.

Promoting now — at consumer #2 — would force one shader to handle both physics and transfer-function modes via uniform-driven branching, distorting both. The rule-of-three says **wait for consumer #3** before promoting. At Phase 13 or 14, if a third volumetric-grid sim needs a raymarch, we promote a parameterized `raymarch_scalar_or_volumetric.frag.glsl` template that both ES and LBM specialize via shader-include selection.

The other limitation of ES's raymarch that promotion would force us to fix now anyway: the unit-cube `[0,1]³` hardcoding (probe-1 § D quoted `shadow_step = sqrt(3)/N` and the slab-intersection on `volumeMin/volumeMax = (0,0,0)/(1,1,1)`). LBM's 512×256×256 domain has aspect ratio 4:2:1, not unit-cube. The sim-local `velmag.frag.glsl` generalizes this via uniform-driven `volumeAspect = vec3(Nx, Ny, Nz) / max(Nx, Ny, Nz)` (or similar); ES's hardcoding stays as-is for v1. **Banked at consumer #3 of the volume-raymarch pattern**: generalize the domain aspect handling at promotion time.

### Decision 9 — Streamlines: GPU-seeded ring-buffer history with RK2 advection

**Choice:** `streamline_advect.comp.glsl` runs once per render frame (NOT per LBM substep). For each of N ≈ 10k seed points, advance the streamline's position via RK2 (two trilinear samples of `u` from the moment buffer):

```
u1 = sample(u, pos)
mid = pos + 0.5 * dt_render * u1
u2 = sample(u, mid)
new_pos = pos + dt_render * u2
```

Each seed stores a ring buffer of its last 64 positions; the new position overwrites the oldest. Ring-buffer head index increments per frame.

`streamline.vert.glsl` + `streamline.frag.glsl` render each streamline as a `VK_PRIMITIVE_TOPOLOGY_LINE_STRIP` with vertex count = ring-buffer length. Each vertex carries (position, age-fraction). The fragment shader fades older vertices via age-fraction for a tail-fade effect.

**Reasoning:** RK2 is the standard second-order ODE integrator for streamline tracing. 64-position ring buffer gives a visible streak length without unbounded memory. ~10k streamlines is dense enough to read the flow structure without overwhelming the visual.

**Reseed strategy v1:** When a streamline's age exceeds 256 frames (4 buffer-cycles), reseed it to a random position within an inlet-aligned slab (X ∈ [Nx/16, Nx/8], Y/Z uniform). This keeps fresh streamlines entering from upstream.

**Net-new infrastructure:** No streamline pipeline exists in Stack C. ~150 LOC sim-local. **Not promoted to common-cpp at v1.** Banked as rule-of-three candidate at the next flow-visualization sim.

**Rejected alternatives:**
- **CPU-side seeding + per-frame VBO upload**: slow, breaks the "GPU does everything" principle.
- **Euler advection (first-order)**: visibly worse for curved streamlines.

### Decision 10 — Scale tiers and tier-deferred allocation

**Choice:** Three tiers per § 1 above. Tier dropdown triggers deferred re-allocation after `window.show()` (Phase 9/10/11 pattern). Default tier = Desktop (256×128×128).

**Tier table struct field shape:**

```cpp
struct Tier {
    const char* label;
    uint32_t Nx, Ny, Nz;
    uint64_t total_cells;        // Nx * Ny * Nz
    const char* note;
};
constexpr std::array<Tier, NUM_TIERS> TIERS = {{
    {"128³ (Laptop)",          128, 128, 128,  128ull*128*128,   "Laptop / iGPU"},
    {"256×128×128 (Desktop)",  256, 128, 128,  256ull*128*128,   "RX 6800 XT / 2080 Ti"},
    {"512×256×256 (Capture)",  512, 256, 256,  512ull*256*256,   "Hero render — interactive ≤10 FPS"},
}};
constexpr int DEFAULT_TIER_INDEX = 1;
```

**Tier change flow** (Phase 9 MPM precedent): On dropdown change, set `rt.pending_tier_index = new_index`. At the top of the next frame BEFORE any dispatch, if `pending_tier_index != current_tier_index`, call `reallocate_tier(pending_tier_index)` which:
1. `renderer.waitIdle()`
2. Destroy all f / f_new / moment / obstacle / render-target images at current tier.
3. Recreate at new tier.
4. Re-write all descriptor sets.
5. Re-run preset-load (which calls `init_equilibrium.comp.glsl` and re-voxelizes the obstacle).
6. Set `current_tier_index = pending_tier_index`.

### Decision 11 — Subgroup-size pinning (opt-in via Phase 11 surface)

**Choice:** `ContextCreateInfo::enable_subgroup_size_control = true` at Context creation. `ComputePipelineDesc::required_subgroup_size = 32` on the four compute pipelines that use subgroup operations: `compute_moments.comp.glsl` (uses `subgroupAdd` for the 19-direction reduction), and *optionally* `collide.comp.glsl` if the implementation chooses to use subgroup operations for the moment computation inside collide. `streaming` and `boundaries` don't use subgroup ops.

**Reasoning:** RX 6800 XT measured properties (probe-2 § B at the local host, 2026-05-15): `minSubgroupSize = 32`, `maxSubgroupSize = 64`, `subgroupSizeControl = true`, `computeFullSubgroups = true`. Pinning to 32 lands inside `[min, max]` and matches NVIDIA's warp-32 baseline, so the same GLSL runs on both vendors. Phase 11 verified this works on the same hardware; we re-verify at Checkpoint 2 above before write time.

**Workgroup size:** 8×8×4 = 256 threads, matching ES's `WG_DIM = 8` 3D workgroup pattern. At 32-thread subgroups, this gives 8 subgroups per workgroup — clean parallelism for the 19-direction moment reduction.

### Decision 12 — Capture format: `latticeBoltzmann` namespace + frame-invariant convention

**Choice:** Top-level meta key `latticeBoltzmann` (camelCase, sim-name-derived, matches existing convention `eulerianSmoke` / `sphWater`). Captured per F5:

- **`velocity`**: rgba16f, count = Nx*Ny*Nz, stride = 8, shape = [Nx, Ny, Nz]. The 4th component is unused but included per the rgba16f convention. **Buffer name is bare `"velocity"`, NOT `"velocity.bin"`** — explicitly avoiding the ES `.bin.bin` quirk (probe-1 § F).
- **`density`**: r32f, count = Nx*Ny*Nz, stride = 4, shape = [Nx, Ny, Nz]. Bare name `"density"`.
- **`obstacle_mask`**: r8uint, count = Nx*Ny*Nz, stride = 1, shape = [Nx, Ny, Nz]. Bare name `"obstacle_mask"`. **Frame-invariant**: meta carries `"frame_invariant": true`; the mask is written only on the first F5 of a session per preset, and on F9 the loader checks if the saved mask matches the current preset's mask via a fast hash comparison; if so, the on-disk mask is reused.

**Distribution functions NOT captured.** At ~600 MB per F5 at default tier, the f-state is too large to capture per-frame. F9 reloads ρ and u, then runs a single `init_equilibrium.comp.glsl` pass to re-derive f_i from equilibrium. This loses the non-equilibrium part of f_i (the part that encodes stress / shear / vorticity at the moment of capture), so F9 doesn't perfectly restore the pre-F5 state — but for a wind-tunnel sim at quasi-steady-state, the equilibrium reinitialization is visually indistinguishable within a few substeps. **Banked as v1.1**: optional `--capture-full-state` flag that does capture f_i for hero-render-archival use.

**Top-level meta block:** preset index, tier index, current iteration count, free-stream u_∞, lattice viscosity ν, hash of obstacle mask, camera state.

**New convention banked: `frame_invariant` meta key.** First consumer is Phase 12's `obstacle_mask`. The convention promotes to a § 7 H3 in `project-state.md` and a new entry in `docs/tier1-capture-format-reference.md` § 2 precedent block.

### Decision 13 — Reference anchors: [Krueger] for math patterns, [Algebraic_D3Q19] for constants

**Choice:** Already landed via setup commits 1 (`8fe355b`) and 2 (`0db9c73`):

- **[Krueger]** at `references/lbm-principles-practice/` (MIT, SHA `6e2c592f`). Math pattern reference. Used for `cat1.upstream-citation` doc-block annotations in `collide.comp.glsl` (chapter13/cpu/LBM.cpp:97 for the equilibrium pattern) and `apply_boundaries.comp.glsl` (chapter5/poiseuille_BB.m:123 for the halfway-BB pattern).

- **[Algebraic_D3Q19]** at `tools/integrity/docs/algebraic/d3q19.md` (no vendor; derivation). Constants source. Used to generate `lattice_constants.glsl` mechanically (§ 4.C). `cat3.d3q19-equilibrium` consumes the expected.json at `tools/integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.expected.json`.

Both registry entries are already in `tools/integrity/docs/ground-truth-sources.md`. No further setup needed.

---

## § 3 — File manifest

Files created in this phase (sim-local + cross-cutting), in approximate execution order:

### § 3.A Sim-local files (under `volumetric-grid/lattice-boltzmann/`)

```
volumetric-grid/lattice-boltzmann/
├── README.md                         (replace stub from probe-1)
├── CMakeLists.txt                    (NEW; ~50 lines; ES pattern)
├── src/
│   └── main.cpp                      (NEW; ~1300 lines; the core executable)
├── shaders/
│   ├── lattice_constants.glsl        (NEW; ~80 lines; SHARED include — d3q19.md § 2.2 ordering)
│   ├── init_equilibrium.comp.glsl    (NEW; ~50 lines)
│   ├── collide.comp.glsl             (NEW; ~80 lines)
│   ├── stream.comp.glsl              (NEW; ~70 lines)
│   ├── apply_boundaries.comp.glsl    (NEW; ~150 lines)
│   ├── compute_moments.comp.glsl     (NEW; ~50 lines)
│   ├── streamline_advect.comp.glsl   (NEW; ~70 lines)
│   ├── velmag.frag.glsl              (NEW; ~80 lines)
│   ├── streamline.vert.glsl          (NEW; ~30 lines)
│   ├── streamline.frag.glsl          (NEW; ~30 lines)
│   └── fullscreen.vert.glsl          (NEW; copy of ES's, ~20 lines)
└── docs/
    ├── load-bearing-decisions.md     (NEW; ~80 lines; sim-local quick reference)
    └── notes.md                      (NEW; ~80 lines; v1.1 polish items)
```

### § 3.B Cross-cutting modifications

```
.github/workflows/build-native.yml    (MODIFY — add LBM to path triggers, both jobs)
CMakeLists.txt                        (MODIFY — add LBM subdir if not already wildcarded)
docs/sim-specs/lattice-boltzmann.md   (MODIFY — replace stub with full spec)
README.md                             (MODIFY — gallery row + section update)
CHANGELOG.md                          (MODIFY — prepend Phase 12 entry)
project-state.md                      (MODIFY — § 3 ledger row; § 6 sim-list row; § 11 latest-commit pointer; possibly new § 7 H3 for frame-invariant capture convention)
docs/tier1-capture-format-reference.md (MODIFY — § 1 add latticeBoltzmann row; § 2 add Phase 12 saveBuffer precedent block; introduce frame_invariant convention)
```

### § 3.C Files explicitly NOT touched

- `common/common-cpp/**` — no common-cpp surface change in Phase 12. The subgroup-size-control surface added by Phase 11's `9e0ca2f` is used as-is.
- `volumetric-grid/eulerian-smoke/**` — no ES changes. The Phase 8 `.bin.bin` double-extension bug is NOT fixed in Phase 12 (banked as separate cleanup commit). The unit-cube hardcoding in `raymarch.frag.glsl` is NOT touched (rule-of-three says wait for consumer #3).
- `tools/integrity/**` — already landed via setup commits 1 and 2.
- `docs/retro/phase11.md` — already exists, not relevant to Phase 12.
- The deferred ledger backfills (Phase 9 / Phase 10 rows in `tier1-capture-format-reference.md` § 1) are NOT done in Phase 12. Banked as separate cleanup commit per Phase 11 retro item 1.

### § 3.D File generation order

The order matters because of dependencies. Claude Code generates in this order:

1. `volumetric-grid/lattice-boltzmann/CMakeLists.txt` — minimal but needed for the CMake build to recognize the target.
2. `volumetric-grid/lattice-boltzmann/shaders/lattice_constants.glsl` — shared include used by 5 other shaders. **Checkpoint 1 fires here.**
3. `volumetric-grid/lattice-boltzmann/shaders/fullscreen.vert.glsl` — trivial copy.
4. `volumetric-grid/lattice-boltzmann/shaders/init_equilibrium.comp.glsl`
5. `volumetric-grid/lattice-boltzmann/shaders/compute_moments.comp.glsl`
6. `volumetric-grid/lattice-boltzmann/shaders/collide.comp.glsl`
7. `volumetric-grid/lattice-boltzmann/shaders/stream.comp.glsl` — **Checkpoint 3 fires after this**.
8. `volumetric-grid/lattice-boltzmann/shaders/apply_boundaries.comp.glsl` — **Checkpoint 4 fires before this**.
9. `volumetric-grid/lattice-boltzmann/shaders/streamline_advect.comp.glsl`
10. `volumetric-grid/lattice-boltzmann/shaders/velmag.frag.glsl`
11. `volumetric-grid/lattice-boltzmann/shaders/streamline.vert.glsl`
12. `volumetric-grid/lattice-boltzmann/shaders/streamline.frag.glsl`
13. `volumetric-grid/lattice-boltzmann/src/main.cpp` — **Checkpoint 2 fires before § 4.B.5 (pipeline creation)**.
14. `volumetric-grid/lattice-boltzmann/docs/load-bearing-decisions.md`
15. `volumetric-grid/lattice-boltzmann/docs/notes.md`
16. `volumetric-grid/lattice-boltzmann/README.md` (replaces existing stub).
17. **Checkpoint 5 fires**: build Release + Debug.
18. Cross-cutting § 5 edits, in the order § 5.A → § 5.G.
19. Final verification block § 6.
20. Commit per § 7.

---

## § 4 — Per-file specs

This is the bulk of the spec. Each subsection covers one file: the file's purpose, its full content (verbatim where the file is short; section-by-section where it's long), and inline citations to anchors in the read list § 0.1.

### § 4.A — `volumetric-grid/lattice-boltzmann/CMakeLists.txt`

Mirror of `volumetric-grid/eulerian-smoke/CMakeLists.txt` (the 65-line file quoted in probe-1 § D), differing only in target name, shader list, and the `GPU_SIMS_LBM_SHADER_DIR` compile-definition name.

**Full file** (~55 lines):

```cmake
# Lattice Boltzmann — second Stack C volumetric-grid sim. Phase 12.
# Consumes gpusims::common_cpp. Builds the binary `lattice_boltzmann`.
#
# Mirrors volumetric-grid/eulerian-smoke/CMakeLists.txt at HEAD 0db9c73.
# Sim is the second real consumer of common-cpp's writeVec3Grid (Phase 8
# eulerian-smoke was consumer #1 for writeFloatGrid; writeVec3Grid's first
# real consumer is this sim's optional velocity-field VDB export, gated at
# runtime by gpusims::vdb::isAvailable() and at compile-time by
# -DGPU_SIMS_USE_OPENVDB=ON. Stub mode compiles and runs; the VDB toggle
# in the panel becomes a no-op.

add_executable(lattice_boltzmann
    src/main.cpp
)

target_link_libraries(lattice_boltzmann
    PRIVATE
        gpusims::common_cpp
)

target_compile_features(lattice_boltzmann PRIVATE cxx_std_20)

# Pass the shader directory into the binary so it can locate shaders regardless
# of the cwd at launch time. Mirrors common-cpp/examples/hello and eulerian-smoke.
target_compile_definitions(lattice_boltzmann PRIVATE
    GPU_SIMS_LBM_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

# Output binary name keeps snake_case (Phase 1 convention; matches gpu_sims_hello,
# eulerian_smoke, reaction_diffusion_3d).
set_target_properties(lattice_boltzmann PROPERTIES
    OUTPUT_NAME       "lattice_boltzmann"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Shaders are compiled at runtime via gpusims::vk::ShaderCompiler.
# Copy them next to the binary so cwd-relative loading works during smoke tests.
# Per-sim shader-copy destination is sim-namespaced (bin/lattice-boltzmann/shaders/)
# to avoid Ninja "multiple rules generate" collisions with other sims that share
# shader filenames (fullscreen.vert.glsl is shared with eulerian-smoke).
set(SHADER_SOURCES
    shaders/lattice_constants.glsl
    shaders/fullscreen.vert.glsl
    shaders/init_equilibrium.comp.glsl
    shaders/compute_moments.comp.glsl
    shaders/collide.comp.glsl
    shaders/stream.comp.glsl
    shaders/apply_boundaries.comp.glsl
    shaders/streamline_advect.comp.glsl
    shaders/velmag.frag.glsl
    shaders/streamline.vert.glsl
    shaders/streamline.frag.glsl
)

foreach(SHADER ${SHADER_SOURCES})
    set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(DST ${CMAKE_BINARY_DIR}/bin/lattice-boltzmann/${SHADER})
    add_custom_command(
        OUTPUT ${DST}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC} ${DST}
        DEPENDS ${SRC}
        COMMENT "Copying ${SHADER}"
    )
    list(APPEND SHADER_OUTPUTS ${DST})
endforeach()

add_custom_target(lattice_boltzmann_shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(lattice_boltzmann lattice_boltzmann_shaders)
```

**Anchored differences from ES's CMakeLists**:
- Target name: `lattice_boltzmann` (vs `eulerian_smoke`).
- Compile-definition name: `GPU_SIMS_LBM_SHADER_DIR` (vs `GPU_SIMS_ES_SHADER_DIR`).
- Shader-copy destination: `bin/lattice-boltzmann/shaders/` (vs `bin/eulerian-smoke/shaders/`) — sim-namespaced per the convention banked at Phase 8.
- Shader list: 11 files (vs ES's 13). LBM has no advect-scalar (no scalar fields advected; ρ comes from f-sum), no buoyancy, no vorticity-confinement, no Jacobi-pressure (LBM is pressure-projection-free by construction).

---

### § 4.B — `volumetric-grid/lattice-boltzmann/src/main.cpp`

The largest file. ~1300 lines. Spec'd in 14 subsections following the Phase 11 § 4.B structure. The reader should think of this as a sequenced narrative: each subsection produces one functional block of `main.cpp`, and the blocks are laid out in the file in the same order they're specified here.

The full file's high-level structure:

```cpp
// === Headers + namespace alias ===                   (§ 4.B.0)
// === Constants ===                                    (§ 4.B.1)
// === Tier table ===                                   (§ 4.B.2, part of constants)
// === Preset table ===                                 (§ 4.B.3)
// === Runtime state struct ===                         (§ 4.B.4)
// === NACA airfoil geometry ===                        (§ 4.B.5)
// === Resource allocation (buffers, images, uniforms) ===  (§ 4.B.6)
// === Pipeline creation (compute + graphics) ===       (§ 4.B.7)
// === Descriptor-set allocation + wiring ===           (§ 4.B.8)
// === Preset application ===                           (§ 4.B.9)
// === Streamline seed initialization ===               (§ 4.B.10)
// === Per-substep dispatch chain ===                   (§ 4.B.11) [CRITICAL]
// === Per-frame render chain (raymarch + streamlines) ===  (§ 4.B.12)
// === F5 / F9 capture and load ===                     (§ 4.B.13)
// === ImGui panel construction ===                     (§ 4.B.14)
// === main() entry + run loop ===                      (§ 4.B.15)
```

Each subsection below specifies what that block does, the key API surfaces it consumes, and either verbatim code (for short / load-bearing blocks) or structured pseudocode + commentary (for longer blocks where verbatim would inflate the spec without adding precision).

#### § 4.B.0 — Headers and namespace alias

```cpp
// Lattice Boltzmann (D3Q19 BGK) around a NACA airfoil.
// Stack C / Vulkan 1.3 / common-cpp consumer.
//
// Phase 12. See docs/phase12_lattice_boltzmann.md for the design spec.
// References:
//   - tools/integrity/docs/algebraic/d3q19.md  (lattice constants)
//   - references/lbm-principles-practice/chapter13/cpu/LBM.cpp:97  (BGK pattern)
//   - references/lbm-principles-practice/chapter5/poiseuille_BB.m:123  (halfway BB pattern)

#include <gpusims/log.hpp>
#include <gpusims/camera.hpp>
#include <gpusims/gpu_profiler.hpp>
#include <gpusims/hot_reload.hpp>
#include <gpusims/imgui_setup.hpp>
#include <gpusims/state_reader.hpp>
#include <gpusims/state_writer.hpp>
#include <gpusims/vdb_writer.hpp>
#include <gpusims/vk/buffer.hpp>
#include <gpusims/vk/compute_pipeline.hpp>
#include <gpusims/vk/context.hpp>
#include <gpusims/vk/frame.hpp>
#include <gpusims/vk/graphics_pipeline.hpp>
#include <gpusims/vk/image.hpp>
#include <gpusims/vk/renderer.hpp>
#include <gpusims/vk/shader_compiler.hpp>
#include <gpusims/vk/window.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>
#include <imgui.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace gv  = gpusims::vk;
namespace vdb = gpusims::vdb;
namespace fs  = std::filesystem;
using gpusims::logInfo;
using gpusims::logWarn;
using nlohmann::json;
```

Note: no `namespace abc = gpusims::abc;` (Phase 12 does NOT consume Alembic; LBM has no particle export at v1). The `vdb` namespace alias is for the optional velocity-field VDB export.

#### § 4.B.1 — Constants

The constants block. ~80 lines. Lays out all the algorithm constants in one place, before any state struct or function.

```cpp
// ============================================================================
// Constants
// ============================================================================

// Lattice constants come from tools/integrity/docs/algebraic/d3q19.md § 2.2.
// They are NOT redeclared here on the host side; the GPU reads them from
// shaders/lattice_constants.glsl (the GLSL include is the single source of
// truth on the GPU). The host side only needs:
constexpr uint32_t NUM_DIRS = 19;

// Workgroup dimensions for compute kernels — 8×8×4 = 256 threads.
// Matches ES's WG_DIM = 8 3D pattern, dimensioned for the 256-thread
// workgroup limit and 32-thread subgroup pinning per Decision 11.
constexpr uint32_t WG_DIM_X = 8;
constexpr uint32_t WG_DIM_Y = 8;
constexpr uint32_t WG_DIM_Z = 4;

// Streamline workgroup — 1D dispatch over seed-point count.
constexpr uint32_t WG_DIM_STREAMLINE = 64;

// Tier table (see Decision 10).
constexpr int NUM_TIERS = 3;
struct Tier {
    const char* label;
    uint32_t Nx;
    uint32_t Ny;
    uint32_t Nz;
    const char* note;
};
constexpr std::array<Tier, NUM_TIERS> TIERS = {{
    {"128³ (Laptop)",          128, 128, 128, "Laptop / iGPU"},
    {"256×128×128 (Desktop)",  256, 128, 128, "RX 6800 XT / 2080 Ti"},
    {"512×256×256 (Capture)",  512, 256, 256, "Hero — interactive ≤10 FPS"},
}};
constexpr int DEFAULT_TIER_INDEX = 1;

// Streamline parameters.
constexpr uint32_t STREAMLINE_COUNT_DEFAULT = 10000;
constexpr uint32_t STREAMLINE_HISTORY       = 64;     // ring-buffer length per streamline
constexpr int      STREAMLINE_RESEED_AGE    = 256;    // frames before reseed

// BGK relaxation defaults (lattice units). τ ∈ [0.51, 2.0] is the stable range
// for BGK; τ < 0.5 is unconditionally unstable. ν = c_s² (τ − 0.5) with c_s² = 1/3.
constexpr float TAU_DEFAULT = 0.6f;   // ν = (1/3)(0.6 − 0.5) = 0.0333 lattice units

// Free-stream velocity bounds. |u_∞| < 0.1 c_s for compressibility-error stability
// (the second-order Maxwell-Boltzmann expansion breaks down beyond this).
constexpr float U_INF_MAX = 0.1f;

// Reference density (incompressible reference state).
constexpr float RHO_0 = 1.0f;

// Substeps per render frame. LBM's per-step dt is fixed at 1.0 in lattice units;
// 'substeps' lets the user advance more lattice steps per visual frame for faster
// convergence at the cost of FPS.
constexpr int   SUBSTEPS_DEFAULT = 1;
constexpr int   SUBSTEPS_MIN     = 1;
constexpr int   SUBSTEPS_MAX     = 16;

// Camera defaults.
constexpr float FOV_DEG_DEFAULT = 55.0f;
constexpr float NEAR_PLANE      = 0.05f;
constexpr float FAR_PLANE       = 200.0f;

// Render toggles default state.
constexpr bool RENDER_VELMAG_DEFAULT       = true;
constexpr bool RENDER_STREAMLINES_DEFAULT  = true;

// NACA airfoil placement in the domain. The airfoil sits at:
//   chord_center  = (Nx/4, Ny/2, Nz/2)
//   chord_length  = Nx / 4  (so the airfoil spans 25% of the domain length)
//   span          = full Nz (extruded across full Z)
//   angle of attack (alpha) per preset.
// These are defaults; presets may override.
constexpr float AIRFOIL_CHORD_CENTER_X_FRAC = 0.25f;   // 1/4 of the way in from -X
constexpr float AIRFOIL_CHORD_LENGTH_FRAC   = 0.25f;   // chord = Nx/4

// Capture / VDB export defaults.
constexpr int VDB_EVERY_N_FRAMES_DEFAULT = 4;   // ~15 export-fps at 60 sim-fps
```

#### § 4.B.2 — Tier table

(Already embedded in § 4.B.1 above. Documented separately because it's referenced by name from § 4.B.6 (tier-deferred allocation) and § 4.B.14 (panel construction).)

The label-mutation pattern from Phase 9/10/11: the "(Capture)" suffix is static in the label, not appended at init based on hardware throughput. The "≤10 FPS" note is part of the `Tier::note` field, displayed below the dropdown in the panel as a degradation warning per the pattern banked at Phase 7 boids-3d's 100k tier.

#### § 4.B.3 — Preset table

Three presets per § 1.

```cpp
// ============================================================================
// Presets
// ============================================================================

struct Preset {
    const char* label;
    const char* naca_designation;   // "0012" or "4412"; parsed at apply_preset time
    float angle_of_attack_deg;
    float u_inf;                    // free-stream velocity magnitude (lattice units)
    float tau;                      // BGK relaxation time
    // Resulting characteristic Reynolds number Re = u_inf * chord / nu where
    // chord = AIRFOIL_CHORD_LENGTH_FRAC * Nx and nu = (tau-0.5)/3.
    // Displayed in the panel HUD; not stored here (re-derived from Nx + tau + u_inf).
};

constexpr int NUM_PRESETS = 3;
constexpr std::array<Preset, NUM_PRESETS> PRESETS = {{
    {"NACA0012 — Low-Re",  "0012", 4.0f,  0.04f, 0.60f},   // attached laminar
    {"NACA0012 — Med-Re",  "0012", 8.0f,  0.06f, 0.55f},   // shedding onset
    {"NACA4412 — Med-Re",  "4412", 6.0f,  0.06f, 0.55f},   // cambered, asymmetric wake
}};
constexpr int DEFAULT_PRESET_INDEX = 0;
```

**Reynolds-number math** (computed at panel-display time, not stored):
- Re = u_∞ · L_chord / ν, where ν = (τ − 0.5) · c_s² = (τ − 0.5) / 3 in lattice units.
- For preset 0 (NACA0012-Low-Re) at default tier: L_chord = 256/4 = 64; ν = (0.6−0.5)/3 = 0.0333; Re = 0.04 · 64 / 0.0333 ≈ **77**. Laminar attached flow regime.
- For preset 1 (NACA0012-Med-Re) at default tier: L_chord = 64; ν = (0.55−0.5)/3 = 0.0167; Re = 0.06 · 64 / 0.0167 ≈ **230**. Vortex-shedding onset regime.
- For preset 2 same Re as preset 1, but cambered airfoil gives asymmetric wake.

**At the 512×256×256 capture tier**: chord = 128, so Re doubles → 460 for preset 1. Approaches LES regime; D3Q19 BGK is still stable but begins to need very small dt for accuracy. This is the regime where the frontier variants (16-bit moment-encoded, AMR, MRT) would buy us real headroom.

#### § 4.B.4 — Runtime state struct

```cpp
// ============================================================================
// Runtime state — single instance carried by main()'s scope.
// ============================================================================

struct Emitter {
    // Reserved for v1.1. v1 has no emitters; flow comes from inlet/outlet only.
    // Keeping the type alive for capture-format-compatibility with future versions.
    glm::vec3 pos;
    glm::vec3 velocity;
    float     emissionRate;
    float     radius;
    float     ageSec;   // CPU-accumulated; not exposed in panel for v1
};

struct Runtime {
    // Tier (deferred-change-after-window.show pattern).
    int          tierIndex          = DEFAULT_TIER_INDEX;
    int          pendingTierIndex   = DEFAULT_TIER_INDEX;
    uint32_t     Nx = TIERS[DEFAULT_TIER_INDEX].Nx;
    uint32_t     Ny = TIERS[DEFAULT_TIER_INDEX].Ny;
    uint32_t     Nz = TIERS[DEFAULT_TIER_INDEX].Nz;
    uint64_t     totalCells        = uint64_t(Nx) * Ny * Nz;

    // Preset.
    int          presetIndex       = DEFAULT_PRESET_INDEX;

    // Per-substep dispatch ping-pong parity.
    uint64_t     iteration         = 0;     // total LBM substeps since session start

    // Solver sliders.
    int          substeps          = SUBSTEPS_DEFAULT;
    float        tau               = TAU_DEFAULT;
    float        uInfMagnitude     = 0.04f;        // overridden by preset on apply_preset
    float        angleOfAttackDeg  = 4.0f;
    glm::vec3    uInf;                              // computed from magnitude + alpha

    // Render toggles.
    bool         renderVelmag      = RENDER_VELMAG_DEFAULT;
    bool         renderStreamlines = RENDER_STREAMLINES_DEFAULT;

    // Streamlines.
    uint32_t     streamlineCount   = STREAMLINE_COUNT_DEFAULT;
    uint32_t     streamlineHistory = STREAMLINE_HISTORY;
    uint32_t     streamlineFrameIndex = 0;          // ring-buffer head per frame

    // VDB export.
    bool         exportVdb         = false;
    int          vdbEveryNFrames   = VDB_EVERY_N_FRAMES_DEFAULT;

    // Stats.
    float        lastFrameMs       = 0.0f;
    float        currentRe         = 0.0f;          // computed for HUD display

    // Capture / load.
    bool         pendingLoadFromF9 = false;

    // Reserved (v1.1).
    std::vector<Emitter> emitters;
};

static Runtime rt;
```

#### § 4.B.5 — NACA airfoil geometry

Self-contained block of ~60 lines. CPU-side analytical SDF for the NACA 4-digit airfoil.

**Mathematical reference:** NACA Report 460, Jacobs, Ward, and Pinkerton 1933. The NACA 4-digit airfoil is parameterized as `MPXX` where:
- `M` = maximum camber as percentage of chord (0–9)
- `P` = position of maximum camber as tenths of chord (0–9)
- `XX` = maximum thickness as percentage of chord (00–30)

For NACA0012: M=0, P=0, XX=12 → symmetric airfoil, 12% thickness.
For NACA4412: M=4, P=4, XX=12 → 4% camber at 40% chord, 12% thickness.

**Thickness equation** (symmetric envelope, applied perpendicular to camber line):

```
y_t(x) = 5·t · [ 0.2969 √x − 0.1260 x − 0.3516 x² + 0.2843 x³ − 0.1015 x⁴ ]
```

where `t = XX/100` (e.g., 0.12 for "12") and `x ∈ [0, 1]` is fraction-of-chord.

**Camber line equation** (for cambered airfoils, M > 0):

```
y_c(x) = (M/P²)     · (2 P x − x²)            for 0 ≤ x ≤ P
       = (M/(1-P)²) · ((1 − 2P) + 2 P x − x²)  for P ≤ x ≤ 1
```

The actual airfoil surface is the upper and lower curves obtained by displacing perpendicular to the camber line by ±y_t(x).

**SDF approximation** (for the voxelization): the analytical SDF is non-trivial for cambered airfoils. We use a sampled-perimeter approximation:
1. Sample 256 points along the upper surface and 256 along the lower surface.
2. For each (y, z) cell in the domain (the airfoil is extruded along z), compute the minimum distance to the sampled perimeter.
3. Determine sign by point-in-polygon test (winding number).

256 samples is sufficient for the chord-length scales we're working with (chord = 32 to 128 cells; one chord-spanning sample-segment is ~0.5 cell at worst, well below voxel resolution).

```cpp
// ============================================================================
// NACA 4-digit airfoil geometry.
// Reference: NACA Report 460 (Jacobs/Ward/Pinkerton 1933).
// ============================================================================
namespace naca {

struct AirfoilParams {
    float M;    // max camber as fraction of chord (0..0.09)
    float P;    // position of max camber as fraction of chord (0..0.9)
    float T;    // max thickness as fraction of chord (0..0.30)
};

// Parse "MPXX" string (e.g., "0012", "4412") into params.
inline AirfoilParams parse(const char* designation) {
    AirfoilParams p{};
    int m  = (designation[0] - '0');
    int pp = (designation[1] - '0');
    int t  = (designation[2] - '0') * 10 + (designation[3] - '0');
    p.M = m  / 100.0f;
    p.P = pp / 10.0f;
    p.T = t  / 100.0f;
    return p;
}

// Thickness envelope, NACA Report 460 equation (12).
inline float thickness(float x, float T) {
    return 5.0f * T * (
        0.2969f * std::sqrt(x)
      - 0.1260f * x
      - 0.3516f * x * x
      + 0.2843f * x * x * x
      - 0.1015f * x * x * x * x);
}

// Camber line and its derivative dyc/dx.
inline glm::vec2 camber_and_slope(float x, float M, float P) {
    if (M <= 0.0f) return glm::vec2(0.0f, 0.0f);
    if (x <= P) {
        float yc  = (M / (P * P)) * (2.0f * P * x - x * x);
        float dyc = (M / (P * P)) * (2.0f * P - 2.0f * x);
        return glm::vec2(yc, dyc);
    } else {
        float inv = 1.0f / ((1.0f - P) * (1.0f - P));
        float yc  = M * inv * ((1.0f - 2.0f * P) + 2.0f * P * x - x * x);
        float dyc = M * inv * (2.0f * P - 2.0f * x);
        return glm::vec2(yc, dyc);
    }
}

// Returns upper and lower surface points at fraction-of-chord x ∈ [0,1].
// Pair is (upper_xy, lower_xy) in airfoil-local coords (chord on x-axis).
inline std::pair<glm::vec2, glm::vec2> surface_points(float x, const AirfoilParams& p) {
    float yt = thickness(x, p.T);
    glm::vec2 cs = camber_and_slope(x, p.M, p.P);
    float theta = std::atan(cs.y);
    glm::vec2 upper = glm::vec2(x - yt * std::sin(theta), cs.x + yt * std::cos(theta));
    glm::vec2 lower = glm::vec2(x + yt * std::sin(theta), cs.x - yt * std::cos(theta));
    return {upper, lower};
}

// Voxelize the airfoil cross-section into a 2D mask, then extrude along Z.
// chord_pixels: airfoil chord length in voxel units.
// alpha_rad:    angle of attack in radians (positive = nose-up).
// origin:       pixel coords of the leading edge in the (Y, X)-cross-section plane.
// Writes 1 to mask cells inside the airfoil, 0 outside.
void voxelize_into(
    std::vector<uint8_t>& mask,           // size = Nx*Ny*Nz, row-major (x fastest)
    uint32_t Nx, uint32_t Ny, uint32_t Nz,
    const AirfoilParams& p,
    float chord_pixels,
    float alpha_rad,
    glm::vec2 leading_edge_xy)            // (x, y) in pixel coords
{
    // 1) Sample 256 upper-surface and 256 lower-surface points in airfoil-local
    //    coords [0, 1] × camber-perpendicular.
    constexpr int N_SAMPLES = 256;
    std::vector<glm::vec2> perimeter;
    perimeter.reserve(2 * N_SAMPLES);
    for (int i = 0; i < N_SAMPLES; ++i) {
        float x = float(i) / float(N_SAMPLES - 1);
        auto [up, lo] = surface_points(x, p);
        perimeter.push_back(up);   // upper, leading to trailing
    }
    for (int i = N_SAMPLES - 1; i >= 0; --i) {
        float x = float(i) / float(N_SAMPLES - 1);
        auto [up, lo] = surface_points(x, p);
        perimeter.push_back(lo);   // lower, trailing back to leading
    }

    // 2) Transform perimeter from airfoil-local [0,1] × camber to pixel coords
    //    with rotation by alpha_rad and translation to leading_edge_xy.
    float c = std::cos(alpha_rad), s = std::sin(alpha_rad);
    for (auto& pt : perimeter) {
        float x_pix = pt.x * chord_pixels;
        float y_pix = pt.y * chord_pixels;
        // rotate (around leading edge at airfoil-local origin)
        float xr = c * x_pix - s * y_pix;
        float yr = s * x_pix + c * y_pix;
        pt = glm::vec2(xr + leading_edge_xy.x, yr + leading_edge_xy.y);
    }

    // 3) For each (x, y) cell, point-in-polygon (winding number) against perimeter.
    //    Extrude full Z.
    auto inside = [&](float px, float py) -> bool {
        int wn = 0;
        for (size_t i = 0; i < perimeter.size(); ++i) {
            const auto& a = perimeter[i];
            const auto& b = perimeter[(i + 1) % perimeter.size()];
            if (a.y <= py) {
                if (b.y > py && (b.x - a.x) * (py - a.y) - (px - a.x) * (b.y - a.y) > 0)
                    ++wn;
            } else {
                if (b.y <= py && (b.x - a.x) * (py - a.y) - (px - a.x) * (b.y - a.y) < 0)
                    --wn;
            }
        }
        return wn != 0;
    };

    std::fill(mask.begin(), mask.end(), uint8_t(0));
    for (uint32_t y = 0; y < Ny; ++y) {
        for (uint32_t x = 0; x < Nx; ++x) {
            bool in = inside(float(x) + 0.5f, float(y) + 0.5f);
            if (in) {
                for (uint32_t z = 0; z < Nz; ++z) {
                    mask[x + Nx * (y + Ny * z)] = 1;
                }
            }
        }
    }
}

}  // namespace naca
```

This is ~120 lines but it's self-contained pure-CPU geometry. The voxelize call is ~5 ms even at the 512×256×256 capture tier (256×256×256 inside-tests; each test is a 512-segment winding-number sum); runs once per preset-apply, never per frame.

#### § 4.B.6 — Resource allocation (buffers, images, uniforms)

Called at startup AND at every `reallocate_tier()` call. ~120 lines. The function is `allocate_for_tier(int tier_idx)`.

```cpp
// ============================================================================
// Resource allocation. Called at startup and on tier change.
// ============================================================================

// All 3D images for the tier. Recreated wholesale on tier change.
struct TierImages {
    // f-state. 19 directions per ping/pong, with rest direction in its own
    // single-image slot to save 1/19 of memory traffic per substep (Decision 2
    // refinement: Krüger chapter13 rest-direction split).
    std::array<gv::Image, NUM_DIRS - 1> f_ping_nonrest;   // f_1 .. f_18
    std::array<gv::Image, NUM_DIRS - 1> f_pong_nonrest;
    gv::Image                            f_rest_ping;      // f_0 only (rest)
    gv::Image                            f_rest_pong;

    // Moments — recomputed each substep, used by next collide + render + streamlines.
    gv::Image    rho;          // r32f
    gv::Image    velocity;     // rgba16f (3 components used, 4th = 0)

    // Obstacle mask — r8uint; uploaded once per preset.
    gv::Image    obstacle_mask;

    // Render targets — bilateral-style screen-space chain not needed here; just
    // a velocity-magnitude attachment.
    // (raymarch writes directly to swapchain; no intermediate render targets v1.)

    // Helper: image creation parameters factored.
    static gv::Image make_3d_r32f(gv::Context& ctx, uint32_t Nx, uint32_t Ny, uint32_t Nz,
                                  const char* debug_name);
    static gv::Image make_3d_rgba16f(gv::Context& ctx, uint32_t Nx, uint32_t Ny, uint32_t Nz,
                                     const char* debug_name);
    static gv::Image make_3d_r8uint(gv::Context& ctx, uint32_t Nx, uint32_t Ny, uint32_t Nz,
                                    const char* debug_name);
};

// Streamline state — host-visible buffer for upload of seed positions on reseed,
// device-local buffer for the ring-buffer history.
struct StreamlineState {
    gv::Buffer  position_history;     // STREAMLINE_COUNT × STREAMLINE_HISTORY × vec4 (xyz + age)
    gv::Buffer  head_index;           // single uint32_t — ring-buffer head index (per-frame, not per-streamline)
};

// Per-frame uniforms.
struct CollideUniforms {
    glm::ivec4 dims;        // (Nx, Ny, Nz, _pad)
    float      tau_inv;     // 1 / tau
    float      omtau_inv;   // 1 - 1/tau
    float      _pad0, _pad1;
};

struct StreamUniforms {
    glm::ivec4 dims;
};

struct BoundariesUniforms {
    glm::ivec4 dims;
    glm::vec4  u_inf;       // (ux, uy, uz, rho_0)
    glm::ivec4 inlet_axis;  // (0, 0, 0, 0): which face is inlet (-X = 0). Reserved for future.
};

struct MomentsUniforms {
    glm::ivec4 dims;
};

struct StreamlineAdvectUniforms {
    glm::ivec4 dims;
    glm::vec4  domain_min;
    glm::vec4  domain_max;
    uint32_t   streamline_count;
    uint32_t   history;
    uint32_t   head_index;          // ring-buffer head — overwrites position [head] each frame
    uint32_t   frame_count;         // for reseed RNG
    float      dt_render;           // seconds per frame, capped
    uint32_t   reseed_age_threshold;
    uint32_t   _pad0, _pad1;
};

struct RaymarchUniforms {
    glm::mat4 invViewProj;
    glm::vec4 cameraPos;
    glm::vec4 volumeMin;
    glm::vec4 volumeMax;
    glm::vec4 volumeAspect;   // (Nx, Ny, Nz, max_dim) for the slab + shadow-step generalization
    int       raymarchSteps;
    int       _pad0;
    float     velmagAbsorption;
    float     velmagMin;     // colormap range start
    float     velmagMax;     // colormap range end
    float     exposure;
    float     _pad1, _pad2;
};

struct StreamlineRenderUniforms {
    glm::mat4 viewProj;
    glm::vec4 lineColor;
    uint32_t  history;
    float     ageFalloff;
    float     _pad0, _pad1;
};
```

The `allocate_for_tier` function:

```cpp
void allocate_for_tier(gv::Context& ctx, int tier_idx, TierImages& ti, StreamlineState& sl) {
    const auto& T = TIERS[tier_idx];
    uint32_t Nx = T.Nx, Ny = T.Ny, Nz = T.Nz;

    // Destroy old (RAII; explicit assignment from default-constructed Image clears).
    for (auto& f : ti.f_ping_nonrest) f = gv::Image{};
    for (auto& f : ti.f_pong_nonrest) f = gv::Image{};
    ti.f_rest_ping = gv::Image{};
    ti.f_rest_pong = gv::Image{};
    ti.rho           = gv::Image{};
    ti.velocity      = gv::Image{};
    ti.obstacle_mask = gv::Image{};

    // Create new at the new tier.
    for (int i = 0; i < int(NUM_DIRS) - 1; ++i) {
        ti.f_ping_nonrest[i] = TierImages::make_3d_r32f(ctx, Nx, Ny, Nz, "f_ping");
        ti.f_pong_nonrest[i] = TierImages::make_3d_r32f(ctx, Nx, Ny, Nz, "f_pong");
    }
    ti.f_rest_ping = TierImages::make_3d_r32f(ctx, Nx, Ny, Nz, "f_rest_ping");
    ti.f_rest_pong = TierImages::make_3d_r32f(ctx, Nx, Ny, Nz, "f_rest_pong");
    ti.rho         = TierImages::make_3d_r32f   (ctx, Nx, Ny, Nz, "rho");
    ti.velocity    = TierImages::make_3d_rgba16f(ctx, Nx, Ny, Nz, "velocity");
    ti.obstacle_mask = TierImages::make_3d_r8uint(ctx, Nx, Ny, Nz, "obstacle_mask");

    // Streamlines — same count across tiers, but reset positions on tier change.
    size_t sl_bytes = size_t(rt.streamlineCount) * rt.streamlineHistory * sizeof(float) * 4;
    sl.position_history = gv::Buffer::create(
        ctx, sl_bytes,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        gv::MemoryUsage::DeviceLocal, "streamline_history");
    sl.head_index = gv::Buffer::create(
        ctx, sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        gv::MemoryUsage::DeviceLocal, "streamline_head_index");
}
```

The `make_3d_*` helpers are short:

```cpp
gv::Image TierImages::make_3d_r32f(gv::Context& ctx, uint32_t Nx, uint32_t Ny, uint32_t Nz,
                                   const char* debug_name) {
    gv::ImageCreateInfo info{};
    info.type           = gv::ImageType::e3D;
    info.extent         = {Nx, Ny, Nz};
    info.format         = VK_FORMAT_R32_SFLOAT;
    info.usage          = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                        | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.debug_name     = debug_name;
    auto img = gv::Image::create(ctx, info);
    // Transition to GENERAL once at creation; shaders access via descriptor sets.
    ctx.runOneShot([&](VkCommandBuffer cmd) {
        gv::Image::transitionLayout(cmd, img.handle(), VK_IMAGE_ASPECT_COLOR_BIT,
                                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    });
    return img;
}
// make_3d_rgba16f: same with VK_FORMAT_R16G16B16A16_SFLOAT.
// make_3d_r8uint:  same with VK_FORMAT_R8_UINT.
```

**Checkpoint 2 fires immediately before this section's pipeline creation begins** (the next subsection). Re-read `vk/context.hpp`, `vk/compute_pipeline.hpp`, `vk/buffer.hpp`, `vk/image.hpp` and confirm signatures match.

#### § 4.B.7 — Pipeline creation

Seven compute pipelines + two graphics pipelines.

```cpp
// ============================================================================
// Pipeline creation. Compute pipelines + graphics pipelines.
// Mirrors ES's pattern from main.cpp:1100-1158.
// ============================================================================

constexpr VkDescriptorType CIS = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
constexpr VkDescriptorType SI  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
constexpr VkDescriptorType SB  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
constexpr VkDescriptorType UB  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

const std::string SD = GPU_SIMS_LBM_SHADER_DIR;   // compile-define from CMakeLists

// Each compute pipeline declared with its descriptor bindings.
// The shader source paths are sim-local; lattice_constants.glsl is #include'd by
// each .comp.glsl that needs it (init/collide/stream/boundaries/moments).

gv::ComputePipelineDesc desc_init{};
desc_init.shader_path = SD + "/init_equilibrium.comp.glsl";
desc_init.bindings = {
    {0, SI,  NUM_DIRS - 1, VK_SHADER_STAGE_COMPUTE_BIT},   // f_ping_nonrest[0..17]
    {1, SI,              1, VK_SHADER_STAGE_COMPUTE_BIT},   // f_rest_ping
    {2, SI,              1, VK_SHADER_STAGE_COMPUTE_BIT},   // rho
    {3, SI,              1, VK_SHADER_STAGE_COMPUTE_BIT},   // velocity
    {4, UB,              1, VK_SHADER_STAGE_COMPUTE_BIT},   // CollideUniforms (reused for init)
};
desc_init.required_subgroup_size = 32;
desc_init.require_full_subgroups = true;
auto pipe_init = gv::ComputePipeline::create(ctx, compiler, desc_init);

gv::ComputePipelineDesc desc_collide{};
desc_collide.shader_path = SD + "/collide.comp.glsl";
desc_collide.bindings = {
    {0, SI,  NUM_DIRS - 1, VK_SHADER_STAGE_COMPUTE_BIT},   // f_ping_nonrest (read-write)
    {1, SI,              1, VK_SHADER_STAGE_COMPUTE_BIT},   // f_rest_ping
    {2, CIS,             1, VK_SHADER_STAGE_COMPUTE_BIT},   // rho (sampled)
    {3, CIS,             1, VK_SHADER_STAGE_COMPUTE_BIT},   // velocity (sampled)
    {4, UB,              1, VK_SHADER_STAGE_COMPUTE_BIT},   // CollideUniforms
};
desc_collide.required_subgroup_size = 32;
desc_collide.require_full_subgroups = true;
auto pipe_collide = gv::ComputePipeline::create(ctx, compiler, desc_collide);

gv::ComputePipelineDesc desc_stream{};
desc_stream.shader_path = SD + "/stream.comp.glsl";
desc_stream.bindings = {
    {0, CIS, NUM_DIRS - 1, VK_SHADER_STAGE_COMPUTE_BIT},   // f_ping_nonrest (sampled with bordered-zero clamp)
    {1, CIS,             1, VK_SHADER_STAGE_COMPUTE_BIT},   // f_rest_ping (just copies through)
    {2, SI,  NUM_DIRS - 1, VK_SHADER_STAGE_COMPUTE_BIT},   // f_pong_nonrest (write)
    {3, SI,              1, VK_SHADER_STAGE_COMPUTE_BIT},   // f_rest_pong (write)
    {4, UB,              1, VK_SHADER_STAGE_COMPUTE_BIT},   // StreamUniforms
};
desc_stream.required_subgroup_size = 32;
desc_stream.require_full_subgroups = true;
auto pipe_stream = gv::ComputePipeline::create(ctx, compiler, desc_stream);

gv::ComputePipelineDesc desc_boundaries{};
desc_boundaries.shader_path = SD + "/apply_boundaries.comp.glsl";
desc_boundaries.bindings = {
    {0, SI,  NUM_DIRS - 1, VK_SHADER_STAGE_COMPUTE_BIT},   // f_pong_nonrest (read-write)
    {1, SI,              1, VK_SHADER_STAGE_COMPUTE_BIT},   // f_rest_pong
    {2, CIS,             1, VK_SHADER_STAGE_COMPUTE_BIT},   // obstacle_mask (sampled point)
    {3, UB,              1, VK_SHADER_STAGE_COMPUTE_BIT},   // BoundariesUniforms
};
desc_boundaries.required_subgroup_size = 32;
desc_boundaries.require_full_subgroups = true;
auto pipe_boundaries = gv::ComputePipeline::create(ctx, compiler, desc_boundaries);

gv::ComputePipelineDesc desc_moments{};
desc_moments.shader_path = SD + "/compute_moments.comp.glsl";
desc_moments.bindings = {
    {0, CIS, NUM_DIRS - 1, VK_SHADER_STAGE_COMPUTE_BIT},   // f_pong_nonrest (sampled)
    {1, CIS,             1, VK_SHADER_STAGE_COMPUTE_BIT},   // f_rest_pong
    {2, SI,              1, VK_SHADER_STAGE_COMPUTE_BIT},   // rho
    {3, SI,              1, VK_SHADER_STAGE_COMPUTE_BIT},   // velocity
    {4, UB,              1, VK_SHADER_STAGE_COMPUTE_BIT},   // MomentsUniforms
};
desc_moments.required_subgroup_size = 32;
desc_moments.require_full_subgroups = true;
auto pipe_moments = gv::ComputePipeline::create(ctx, compiler, desc_moments);

gv::ComputePipelineDesc desc_streamline_advect{};
desc_streamline_advect.shader_path = SD + "/streamline_advect.comp.glsl";
desc_streamline_advect.bindings = {
    {0, CIS, 1, VK_SHADER_STAGE_COMPUTE_BIT},   // velocity (sampled trilinear)
    {1, SB,  1, VK_SHADER_STAGE_COMPUTE_BIT},   // position_history (read-write)
    {2, UB,  1, VK_SHADER_STAGE_COMPUTE_BIT},   // StreamlineAdvectUniforms
};
// No subgroup pinning on streamline_advect — no subgroup ops used.
auto pipe_streamline_advect = gv::ComputePipeline::create(ctx, compiler, desc_streamline_advect);

// Graphics pipelines.
gv::GraphicsPipelineDesc gd_raymarch{};
gd_raymarch.vertex_shader_path   = SD + "/fullscreen.vert.glsl";
gd_raymarch.fragment_shader_path = SD + "/velmag.frag.glsl";
gd_raymarch.color_formats        = {window.colorFormat()};
gd_raymarch.depth_test           = false;
gd_raymarch.cull_mode            = VK_CULL_MODE_NONE;
gd_raymarch.bindings = {
    {0, CIS, 1, VK_SHADER_STAGE_FRAGMENT_BIT},   // velocity (sampled trilinear)
    {1, CIS, 1, VK_SHADER_STAGE_FRAGMENT_BIT},   // colormap LUT (sampler2D)
    {2, CIS, 1, VK_SHADER_STAGE_FRAGMENT_BIT},   // blue-noise jitter
    {3, UB,  1, VK_SHADER_STAGE_FRAGMENT_BIT},   // RaymarchUniforms
};
auto pipe_raymarch = gv::GraphicsPipeline::create(ctx, compiler, gd_raymarch);

gv::GraphicsPipelineDesc gd_streamline{};
gd_streamline.vertex_shader_path   = SD + "/streamline.vert.glsl";
gd_streamline.fragment_shader_path = SD + "/streamline.frag.glsl";
gd_streamline.color_formats        = {window.colorFormat()};
gd_streamline.depth_test           = false;
gd_streamline.cull_mode            = VK_CULL_MODE_NONE;
gd_streamline.topology             = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
gd_streamline.primitive_restart    = true;     // emit primitive-restart index between streamlines
gd_streamline.bindings = {
    {0, SB, 1, VK_SHADER_STAGE_VERTEX_BIT},      // position_history (read)
    {1, UB, 1, VK_SHADER_STAGE_VERTEX_BIT
              | VK_SHADER_STAGE_FRAGMENT_BIT},   // StreamlineRenderUniforms
};
auto pipe_streamline = gv::GraphicsPipeline::create(ctx, compiler, gd_streamline);
```

**Note on `desc_collide` and `desc_stream`**: the descriptor type for the read side of f-buffers is `CIS` (combined image sampler) when we want trilinear / point sampling with clamp-to-edge, and `SI` (storage image) when we want bounded write access. Stream reads neighbors → needs sampling with clamp-to-edge (out-of-bounds reads return 0 = treated as periodic-but-zeroed, but boundaries.glsl overwrites these on the same substep). Collide reads f at current cell only (no neighbor sampling) → could use either; SI chosen for simplicity (no sampler indirection).

#### § 4.B.8 — Descriptor-set allocation and wiring

Following ES's pattern from main.cpp:1168-1279, parameterized over parity (2) × slots (frames-in-flight).

```cpp
constexpr uint32_t kParities = 2;
constexpr uint32_t kSlots    = MAX_FRAMES_IN_FLIGHT;   // from common-cpp; typically 2

std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_init{};
std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_collide{};
std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_stream{};
std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_boundaries{};
std::array<std::array<VkDescriptorSet, kSlots>, kParities> ds_moments{};
std::array<                  VkDescriptorSet, kSlots>      ds_streamline_advect{};
std::array<                  VkDescriptorSet, kSlots>      ds_raymarch{};
std::array<                  VkDescriptorSet, kSlots>      ds_streamline{};

auto allocate_all_descriptors = [&]() {
    for (uint32_t p = 0; p < kParities; ++p) {
        for (uint32_t s = 0; s < kSlots; ++s) {
            ds_init       [p][s] = pipe_init      .allocateDescriptorSet();
            ds_collide    [p][s] = pipe_collide   .allocateDescriptorSet();
            ds_stream     [p][s] = pipe_stream    .allocateDescriptorSet();
            ds_boundaries [p][s] = pipe_boundaries.allocateDescriptorSet();
            ds_moments    [p][s] = pipe_moments   .allocateDescriptorSet();
        }
    }
    for (uint32_t s = 0; s < kSlots; ++s) {
        ds_streamline_advect[s] = pipe_streamline_advect.allocateDescriptorSet();
        ds_raymarch         [s] = pipe_raymarch         .allocateDescriptorSet();
        ds_streamline       [s] = pipe_streamline       .allocateDescriptorSet();
    }
};

auto wire_all_descriptors = [&]() {
    // For each (parity p, slot s):
    //   parity 0: collide reads f_ping_nonrest+f_rest_ping, writes back to same;
    //             stream reads f_ping_nonrest+f_rest_ping (post-collide),
    //                          writes f_pong_nonrest+f_rest_pong;
    //             boundaries reads/writes f_pong_nonrest+f_rest_pong;
    //             moments reads f_pong_nonrest+f_rest_pong, writes rho+velocity.
    //   parity 1: swap ping↔pong everywhere.
    VkDevice dev = ctx.device();
    for (uint32_t p = 0; p < kParities; ++p) {
        auto& f_in_nonrest  = (p == 0) ? ti.f_ping_nonrest : ti.f_pong_nonrest;
        auto& f_in_rest     = (p == 0) ? ti.f_rest_ping    : ti.f_rest_pong;
        auto& f_out_nonrest = (p == 0) ? ti.f_pong_nonrest : ti.f_ping_nonrest;
        auto& f_out_rest    = (p == 0) ? ti.f_rest_pong    : ti.f_rest_ping;
        for (uint32_t s = 0; s < kSlots; ++s) {
            write_init_descriptor      (dev, ds_init      [p][s], f_in_nonrest, f_in_rest,
                                        ti.rho, ti.velocity, ub_collide[s].handle());
            write_collide_descriptor   (dev, ds_collide   [p][s], f_in_nonrest, f_in_rest,
                                        ti.rho, ti.velocity, sampler_linear, ub_collide[s].handle());
            write_stream_descriptor    (dev, ds_stream    [p][s], f_in_nonrest, f_in_rest,
                                        f_out_nonrest, f_out_rest, sampler_clamp, ub_stream[s].handle());
            write_boundaries_descriptor(dev, ds_boundaries[p][s], f_out_nonrest, f_out_rest,
                                        ti.obstacle_mask, sampler_point, ub_boundaries[s].handle());
            write_moments_descriptor   (dev, ds_moments   [p][s], f_out_nonrest, f_out_rest,
                                        ti.rho, ti.velocity, sampler_point, ub_moments[s].handle());
        }
    }
    for (uint32_t s = 0; s < kSlots; ++s) {
        write_streamline_advect_descriptor(dev, ds_streamline_advect[s],
                                           ti.velocity, sampler_linear,
                                           sl.position_history.handle(),
                                           ub_streamline_advect[s].handle());
        write_raymarch_descriptor(dev, ds_raymarch[s],
                                  ti.velocity, sampler_linear,
                                  colormap_lut.view(), bluenoise.view(),
                                  sampler_lut, sampler_lut,
                                  ub_raymarch[s].handle());
        write_streamline_descriptor(dev, ds_streamline[s],
                                    sl.position_history.handle(),
                                    ub_streamline_render[s].handle());
    }
};

allocate_all_descriptors();
wire_all_descriptors();
```

The eight `write_*_descriptor` helpers are sim-local free functions (declared near line 400 of main.cpp; see ES's `writeXxxDescriptor` pattern at probe-1 § D quoting "All eleven descriptor-write helpers"). Each is a thin wrapper around `vkUpdateDescriptorSets`; ~20 lines each. They are NOT promoted to common-cpp at v1 — per the ES precedent, every Stack C sim writes its own.

#### § 4.B.9 — Preset application

```cpp
// Apply preset to the current tier's allocations.
// Resets ρ to RHO_0, sets u to u_∞ field, runs init_equilibrium kernel,
// voxelizes the airfoil into obstacle_mask, reseeds streamlines.
void apply_preset(gv::Context& ctx, int preset_idx, TierImages& ti, StreamlineState& sl) {
    const auto& P = PRESETS[preset_idx];
    rt.presetIndex      = preset_idx;
    rt.tau              = P.tau;
    rt.uInfMagnitude    = P.u_inf;
    rt.angleOfAttackDeg = P.angle_of_attack_deg;
    float alpha_rad     = glm::radians(P.angle_of_attack_deg);
    rt.uInf = glm::vec3(P.u_inf * std::cos(alpha_rad),
                        P.u_inf * std::sin(alpha_rad),
                        0.0f);

    // 1) Voxelize airfoil into obstacle_mask.
    auto airfoil = naca::parse(P.naca_designation);
    std::vector<uint8_t> mask_bytes(size_t(rt.Nx) * rt.Ny * rt.Nz);
    float chord_pixels = AIRFOIL_CHORD_LENGTH_FRAC * float(rt.Nx);
    glm::vec2 leading_edge(AIRFOIL_CHORD_CENTER_X_FRAC * float(rt.Nx) - chord_pixels * 0.5f,
                           float(rt.Ny) * 0.5f);
    naca::voxelize_into(mask_bytes, rt.Nx, rt.Ny, rt.Nz, airfoil,
                        chord_pixels, alpha_rad, leading_edge);
    ti.obstacle_mask.upload(mask_bytes.data(), mask_bytes.size());

    // 2) Run init_equilibrium kernel: sets f_i = feq(rho=1, u=u_inf) at every fluid cell,
    //    f_i = 0 at solid cells (they don't participate in advection).
    // The kernel takes a uniform with the u_inf vector + dims and writes both f_ping (all 18 nonrest) and f_rest_ping.
    // (Direct dispatch here; details in § 4.B.11.)
    ctx.runOneShot([&](VkCommandBuffer cmd) {
        // Pack uniform
        CollideUniforms cu{};
        cu.dims = glm::ivec4(int(rt.Nx), int(rt.Ny), int(rt.Nz), 0);
        cu.tau_inv = 1.0f / rt.tau;
        cu.omtau_inv = 1.0f - cu.tau_inv;
        // u_inf is read out-of-band via a separate write-time uniform; for v1 we pack it
        // into a separate small uniform reused at boundaries-apply time.
        // (Implementation detail: a sim-local `InitUniforms` struct may be added if needed;
        // for now we reuse CollideUniforms and pass u_inf via push constants.)
        // Dispatch the init kernel.
        pipe_init.dispatch(cmd, ds_init[0][0],
            (rt.Nx + WG_DIM_X - 1) / WG_DIM_X,
            (rt.Ny + WG_DIM_Y - 1) / WG_DIM_Y,
            (rt.Nz + WG_DIM_Z - 1) / WG_DIM_Z,
            &rt.uInf, sizeof(glm::vec3));   // u_inf passed via push constants
        gv::memoryBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
        // Initialize rho and velocity images to ρ_0 and u_inf via the same kernel
        // (kernel writes them as a side-effect, see init_equilibrium.comp.glsl § 4.D).
    });

    // 3) Reseed streamlines.
    seed_streamlines(ctx, ti, sl, /*reseed_all=*/true);

    // Reset iteration counter and parity.
    rt.iteration = 0;
}
```

The `pipe_init.dispatch` push-constants form needs a small adjustment: the existing `gpusims::vk::ComputePipeline::dispatch` signature (per probe-1 § C) takes `(cmd, ds, gx, gy, gz, push_constants_ptr, push_size)`. So `&rt.uInf` and `sizeof(glm::vec3)` are correct. The 16-byte alignment for vec3 in GLSL (where vec3 occupies 16 bytes including padding) means the GLSL side declares `vec3 uInf` inside a push-constants block; the C++ side passes a `glm::vec3` (12 bytes) that gets padded to 16 by the GPU side. Cleanly aligned.

#### § 4.B.10 — Streamline seed initialization

```cpp
// Reseed all streamlines to random positions in an inlet-aligned slab.
// reseed_all=true: every streamline reset (called on tier or preset change).
// reseed_all=false: only streamlines whose age >= STREAMLINE_RESEED_AGE reset.
void seed_streamlines(gv::Context& ctx, const TierImages& /*ti*/, StreamlineState& sl,
                      bool reseed_all) {
    std::vector<glm::vec4> seed_data(rt.streamlineCount * rt.streamlineHistory,
                                     glm::vec4(0));
    static std::mt19937 rng(12345);
    std::uniform_real_distribution<float> ud(0.0f, 1.0f);

    // Slab: X ∈ [Nx/16, Nx/8], Y ∈ [0, Ny], Z ∈ [0, Nz]
    float x_lo = float(rt.Nx) / 16.0f;
    float x_hi = float(rt.Nx) /  8.0f;

    for (uint32_t i = 0; i < rt.streamlineCount; ++i) {
        glm::vec3 pos(x_lo + (x_hi - x_lo) * ud(rng),
                       ud(rng) * float(rt.Ny),
                       ud(rng) * float(rt.Nz));
        float age = 0.0f;
        for (uint32_t h = 0; h < rt.streamlineHistory; ++h) {
            seed_data[i * rt.streamlineHistory + h] = glm::vec4(pos, age);
        }
    }
    sl.position_history.stage(ctx, seed_data.data(),
                              seed_data.size() * sizeof(glm::vec4));
    // Reset head index to 0.
    uint32_t zero = 0;
    sl.head_index.stage(ctx, &zero, sizeof(uint32_t));
    rt.streamlineFrameIndex = 0;
}
```

**Note on `reseed_all=false`:** v1 only calls `seed_streamlines(reseed_all=true)` on tier or preset change. The per-frame reseed of aged streamlines is done **inside `streamline_advect.comp.glsl`** by the GPU itself (cheaper than re-uploading; see § 4.I).

#### § 4.B.11 — Per-substep dispatch chain (CRITICAL)

**This is the heart of the sim.** ~120 lines. The substep loop runs `rt.substeps` times per render frame. Each substep is: collide → barrier → stream → barrier → boundaries → barrier → moments → barrier.

```cpp
// ============================================================================
// Per-substep dispatch chain.
// Each substep advances LBM by one lattice timestep.
// Parity bit `p` selects which descriptor sets to use:
//   p = (rt.iteration & 1u)
//   parity 0: f_ping is "in"  (collide reads it, stream writes f_pong);
//             boundaries operates on f_pong; moments reads f_pong, writes rho+u.
//   parity 1: swapped (f_pong in, f_ping out).
// ============================================================================

auto run_substep = [&](VkCommandBuffer cmd, uint32_t slot) {
    uint32_t p = uint32_t(rt.iteration & 1u);
    uint32_t wgX = (rt.Nx + WG_DIM_X - 1) / WG_DIM_X;
    uint32_t wgY = (rt.Ny + WG_DIM_Y - 1) / WG_DIM_Y;
    uint32_t wgZ = (rt.Nz + WG_DIM_Z - 1) / WG_DIM_Z;

    // Pack the per-substep uniforms.
    CollideUniforms cu{};
    cu.dims = glm::ivec4(int(rt.Nx), int(rt.Ny), int(rt.Nz), 0);
    cu.tau_inv = 1.0f / rt.tau;
    cu.omtau_inv = 1.0f - cu.tau_inv;
    ctx.uploadBuffer(cmd, ub_collide[slot], &cu, sizeof(cu));

    StreamUniforms su{};
    su.dims = cu.dims;
    ctx.uploadBuffer(cmd, ub_stream[slot], &su, sizeof(su));

    BoundariesUniforms bu{};
    bu.dims = cu.dims;
    bu.u_inf = glm::vec4(rt.uInf.x, rt.uInf.y, rt.uInf.z, RHO_0);
    bu.inlet_axis = glm::ivec4(0);
    ctx.uploadBuffer(cmd, ub_boundaries[slot], &bu, sizeof(bu));

    MomentsUniforms mu{};
    mu.dims = cu.dims;
    ctx.uploadBuffer(cmd, ub_moments[slot], &mu, sizeof(mu));

    // 1) Collide: BGK relaxation on f_in at every fluid cell. Reads f_in + rho + u
    //    (the moments from the previous substep), computes f_eq, blends:
    //      f_new[i] = (1 - 1/tau) * f_in[i] + (1/tau) * f_eq[i]
    //    Note: rho + u USED here are last substep's moments (one-step lag). The
    //    Krüger fused stream-collide instead computes moments inline; we don't,
    //    per Decision 5 (moments live in their own buffers).
    {
        auto scope = profiler.scope(cmd, "collide");
        pipe_collide.dispatch(cmd, ds_collide[p][slot], wgX, wgY, wgZ);
    }
    gv::memoryBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

    // 2) Stream: f_out[x][i] = f_in[x - c_i][i] for each direction i.
    //    Reads neighbor cells via SAMPLER_CLAMP (out-of-bounds reads return 0; the
    //    boundaries pass overwrites these on the same substep, so no harm).
    //    f_0 (rest) doesn't stream — it stays in place; stream.glsl copies f_rest_in to
    //    f_rest_out untouched.
    {
        auto scope = profiler.scope(cmd, "stream");
        pipe_stream.dispatch(cmd, ds_stream[p][slot], wgX, wgY, wgZ);
    }
    gv::memoryBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

    // 3) Boundaries: applies inlet (Zou-He velocity), outlet (Zou-He pressure),
    //    free-slip side walls, and halfway-BB at obstacle. In-place on f_out.
    {
        auto scope = profiler.scope(cmd, "boundaries");
        pipe_boundaries.dispatch(cmd, ds_boundaries[p][slot], wgX, wgY, wgZ);
    }
    gv::memoryBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

    // 4) Moments: ρ = Σ f_i ; ρ u = Σ f_i c_i. Writes rho + velocity images.
    //    These are read by the NEXT substep's collide, and by raymarch + streamlines.
    {
        auto scope = profiler.scope(cmd, "moments");
        pipe_moments.dispatch(cmd, ds_moments[p][slot], wgX, wgY, wgZ);
    }
    gv::memoryBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
                      | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                      VK_ACCESS_SHADER_READ_BIT);

    rt.iteration++;
};

// Frame-level call:
auto run_frame_substeps = [&](VkCommandBuffer cmd, uint32_t slot) {
    for (int s = 0; s < rt.substeps; ++s) {
        run_substep(cmd, slot);
    }
};
```

**Checkpoint 3 fires here.** Before the substep loop ships, Claude Code traces one substep at the test point (ρ=1, u=(0.1,0,0), f_i = ω_i initially) and verifies that after collide:
- The moment computation reads ρ=1, u=(0.1,0,0) from the moments buffer (which on substep 0 has been initialized by init_equilibrium).
- The equilibrium computed inside collide matches `d3q19_equilibrium.expected.json` test-point-2 to within float precision.
- The relaxation produces `f_out = f_in + (1/τ)(f_eq − f_in)` = exactly `f_eq` when `τ = 1` (the special case `τ_inv = 1` makes relaxation collapse to f = f_eq).

If the spec authors' description of the dispatch chain doesn't produce the expected values, STOP and report.

#### § 4.B.12 — Per-frame render chain (raymarch + streamlines)

Outside the substep loop, runs once per render frame:

```cpp
auto run_frame_render = [&](VkCommandBuffer cmd, uint32_t slot, uint32_t image_index) {
    // 1) Streamline advection (compute pass — independent of substep ping-pong).
    if (rt.renderStreamlines) {
        StreamlineAdvectUniforms ssu{};
        ssu.dims                = glm::ivec4(rt.Nx, rt.Ny, rt.Nz, 0);
        ssu.domain_min          = glm::vec4(0.0f);
        ssu.domain_max          = glm::vec4(float(rt.Nx), float(rt.Ny), float(rt.Nz), 0.0f);
        ssu.streamline_count    = rt.streamlineCount;
        ssu.history             = rt.streamlineHistory;
        ssu.head_index          = rt.streamlineFrameIndex;
        ssu.frame_count         = uint32_t(rt.iteration / rt.substeps);
        ssu.dt_render           = std::clamp(rt.lastFrameMs * 1e-3f, 1e-3f, 0.05f);
        ssu.reseed_age_threshold = uint32_t(STREAMLINE_RESEED_AGE);
        ctx.uploadBuffer(cmd, ub_streamline_advect[slot], &ssu, sizeof(ssu));

        uint32_t wg = (rt.streamlineCount + WG_DIM_STREAMLINE - 1) / WG_DIM_STREAMLINE;
        {
            auto scope = profiler.scope(cmd, "streamline_advect");
            pipe_streamline_advect.dispatch(cmd, ds_streamline_advect[slot], wg, 1, 1);
        }
        gv::memoryBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
        rt.streamlineFrameIndex = (rt.streamlineFrameIndex + 1) % rt.streamlineHistory;
    }

    // 2) Begin renderpass (dynamic rendering to swapchain image).
    renderer.beginSwapchainPass(cmd, image_index);

    // 3) Raymarch velocity-magnitude.
    if (rt.renderVelmag) {
        // Pack RaymarchUniforms.
        RaymarchUniforms ru = build_raymarch_uniforms(camera, window, rt);
        ctx.uploadBuffer(cmd, ub_raymarch[slot], &ru, sizeof(ru));
        auto scope = profiler.scope(cmd, "raymarch");
        pipe_raymarch.bind(cmd);
        pipe_raymarch.bindDescriptorSet(cmd, ds_raymarch[slot]);
        vkCmdDraw(cmd, 3, 1, 0, 0);   // fullscreen triangle
    }

    // 4) Streamlines on top.
    if (rt.renderStreamlines) {
        StreamlineRenderUniforms su{};
        su.viewProj    = camera.viewProj(window.aspect());
        su.lineColor   = glm::vec4(1.0f, 0.95f, 0.85f, 1.0f);
        su.history     = rt.streamlineHistory;
        su.ageFalloff  = 0.5f;
        ctx.uploadBuffer(cmd, ub_streamline_render[slot], &su, sizeof(su));
        auto scope = profiler.scope(cmd, "streamline_render");
        pipe_streamline.bind(cmd);
        pipe_streamline.bindDescriptorSet(cmd, ds_streamline[slot]);
        // Draw one line-strip per streamline; the vertex shader synthesizes
        // positions from the position_history buffer.
        // (Primitive-restart index between streamlines emitted via instance count = 1
        //  and vertex count = streamline_count * (streamline_history + 1) where the
        //  +1 carries the primitive-restart marker. Implementation in § 4.K.)
        vkCmdDraw(cmd, rt.streamlineCount * (rt.streamlineHistory + 1), 1, 0, 0);
    }

    // 5) ImGui draw on top.
    imgui_setup.render(cmd);

    renderer.endSwapchainPass(cmd);
};
```

#### § 4.B.13 — F5 / F9 capture and load

Mirrors ES's `capture_save` and `capture_load` at probe-1 § D (e), with two key differences:
1. **No `.bin` extension on buffer names** — explicitly avoiding the Phase 8 quirk.
2. **`obstacle_mask` carries `"frame_invariant": true`** per the new convention banked at Phase 12.

```cpp
auto capture_save = [&]() {
    renderer.waitIdle();
    const size_t N = size_t(rt.Nx) * rt.Ny * rt.Nz;

    // Read back rho, velocity, and obstacle_mask.
    std::vector<float>     rho_bytes(N);
    std::vector<uint16_t>  vel_bytes(N * 4);    // rgba16f = 4 halfs per cell
    std::vector<uint8_t>   mask_bytes(N);
    ti.rho           .readback(rho_bytes .data(), rho_bytes .size() * sizeof(float));
    ti.velocity      .readback(vel_bytes .data(), vel_bytes .size() * sizeof(uint16_t));
    ti.obstacle_mask .readback(mask_bytes.data(), mask_bytes.size() * sizeof(uint8_t));

    uint32_t frame_idx = uint32_t(rt.iteration / std::max(rt.substeps, 1));
    capture_writer.beginFrame(frame_idx);
    capture_writer.setMeta("latticeBoltzmann", runtime_meta_json());

    capture_writer.saveBuffer("density",    rho_bytes.data(), rho_bytes.size() * sizeof(float),
        {{"count", uint64_t(N)}, {"stride", 4}, {"format", "r32f"},
         {"shape", {rt.Nx, rt.Ny, rt.Nz}}});
    capture_writer.saveBuffer("velocity",   vel_bytes.data(), vel_bytes.size() * sizeof(uint16_t),
        {{"count", uint64_t(N)}, {"stride", 8}, {"format", "rgba16f"},
         {"shape", {rt.Nx, rt.Ny, rt.Nz}}});
    capture_writer.saveBuffer("obstacle_mask", mask_bytes.data(), mask_bytes.size(),
        {{"count", uint64_t(N)}, {"stride", 1}, {"format", "r8uint"},
         {"shape", {rt.Nx, rt.Ny, rt.Nz}},
         {"frame_invariant", true}});

    capture_writer.endFrame();
    gpusims::ui::pushToast(("Saved capture #" + std::to_string(frame_idx)).c_str(), true);
    logInfo("F5: saved capture {}", frame_idx);
};

auto capture_load = [&]() {
    auto latest = gv::StateReader::findLatest(fs::current_path() / "captures");
    if (!latest) {
        logWarn("F9: no captures found.");
        gpusims::ui::pushToast("F9: no captures.", false);
        return;
    }
    auto opt = gv::StateReader::open(*latest);
    if (!opt) {
        logWarn("F9: failed to open capture at {}", latest->string());
        return;
    }
    auto& reader = *opt;

    json lbm_meta = reader.meta("latticeBoltzmann");
    if (lbm_meta.is_null()) {
        logWarn("F9: capture does not contain latticeBoltzmann key; skipping.");
        return;
    }

    int captured_tier   = lbm_meta.value("tierIndex",   DEFAULT_TIER_INDEX);
    int captured_preset = lbm_meta.value("presetIndex", DEFAULT_PRESET_INDEX);

    // Apply tier change first (if needed) — reallocates everything.
    if (captured_tier != rt.tierIndex) {
        rt.pendingTierIndex = captured_tier;
        // Tier change picked up at top of next frame; capture-load returns here for now.
        rt.pendingLoadFromF9 = true;   // re-enter load after tier change
        return;
    }
    // Apply preset (re-voxelize airfoil) UNLESS the obstacle hash matches and we can reuse.
    if (captured_preset != rt.presetIndex) {
        apply_preset(ctx, captured_preset, ti, sl);
    }

    // Override sliders from capture meta.
    rt.tau              = lbm_meta.value("tau", rt.tau);
    rt.substeps         = lbm_meta.value("substeps", rt.substeps);
    rt.uInfMagnitude    = lbm_meta.value("uInfMagnitude", rt.uInfMagnitude);
    rt.angleOfAttackDeg = lbm_meta.value("angleOfAttackDeg", rt.angleOfAttackDeg);
    rt.iteration        = lbm_meta.value("iteration", uint64_t(0));

    // Upload rho + velocity from capture.
    auto rho_blob = reader.buffer("density");
    auto vel_blob = reader.buffer("velocity");
    if (!rho_blob.empty()) {
        ti.rho.upload(rho_blob.data(), rho_blob.size());
    }
    if (!vel_blob.empty()) {
        ti.velocity.upload(vel_blob.data(), vel_blob.size());
    }
    // Obstacle mask: only reload if frame_invariant flag missing or preset hash differs.
    json mask_meta = reader.bufferMeta("obstacle_mask");
    if (!mask_meta.value("frame_invariant", false)) {
        auto mask_blob = reader.buffer("obstacle_mask");
        if (!mask_blob.empty()) {
            ti.obstacle_mask.upload(mask_blob.data(), mask_blob.size());
        }
    }
    // Otherwise: apply_preset above already voxelized; or if preset unchanged the mask is fine.

    // Re-init f from saved rho + u (run one equilibrium init pass).
    ctx.runOneShot([&](VkCommandBuffer cmd) {
        pipe_init.dispatch(cmd, ds_init[0][0],
            (rt.Nx + WG_DIM_X - 1) / WG_DIM_X,
            (rt.Ny + WG_DIM_Y - 1) / WG_DIM_Y,
            (rt.Nz + WG_DIM_Z - 1) / WG_DIM_Z,
            &rt.uInf, sizeof(glm::vec3));
    });

    gpusims::ui::pushToast(("Loaded capture from " + latest->filename().string()).c_str(), true);
    logInfo("F9: loaded capture from {}", latest->string());
};
```

#### § 4.B.14 — ImGui panel construction

~150 lines. Sections: Solver / Render / Boundaries / Streamlines / Capture / Camera / Stats. ImGui is consumed via `gpusims::ImguiSetup` (per ES precedent at probe-1 § D).

```cpp
void build_panel() {
    ImGui::Begin("Lattice Boltzmann (Phase 12)");

    // ---- Preset / Tier dropdowns ----
    {
        const char* preset_labels[NUM_PRESETS];
        for (int i = 0; i < NUM_PRESETS; ++i) preset_labels[i] = PRESETS[i].label;
        int preset_idx = rt.presetIndex;
        if (ImGui::Combo("Preset", &preset_idx, preset_labels, NUM_PRESETS)) {
            apply_preset(ctx, preset_idx, ti, sl);
        }

        const char* tier_labels[NUM_TIERS];
        for (int i = 0; i < NUM_TIERS; ++i) tier_labels[i] = TIERS[i].label;
        int tier_idx = rt.pendingTierIndex;
        if (ImGui::Combo("Tier", &tier_idx, tier_labels, NUM_TIERS)) {
            rt.pendingTierIndex = tier_idx;
        }
        ImGui::TextDisabled("%s", TIERS[rt.tierIndex].note);
    }
    ImGui::Separator();

    // ---- Solver folder ----
    if (ImGui::CollapsingHeader("Solver", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("tau",    &rt.tau, 0.51f, 2.0f, "%.3f");
        ImGui::SliderInt  ("substeps", &rt.substeps, SUBSTEPS_MIN, SUBSTEPS_MAX);
        // Derived viscosity + Reynolds.
        float nu = (rt.tau - 0.5f) / 3.0f;
        float chord = AIRFOIL_CHORD_LENGTH_FRAC * float(rt.Nx);
        float Re = rt.uInfMagnitude * chord / std::max(nu, 1e-9f);
        rt.currentRe = Re;
        ImGui::TextDisabled("nu = %.4f, Re = %.1f", nu, Re);
    }

    // ---- Flow folder (free-stream + AoA) ----
    if (ImGui::CollapsingHeader("Flow")) {
        if (ImGui::SliderFloat("|u_inf|", &rt.uInfMagnitude, 0.005f, U_INF_MAX, "%.4f")) {
            update_u_inf_vector();
        }
        if (ImGui::SliderFloat("angle of attack (deg)", &rt.angleOfAttackDeg, -15.0f, 15.0f)) {
            update_u_inf_vector();
        }
        ImGui::TextDisabled("u_inf = (%.3f, %.3f, %.3f)",
                            rt.uInf.x, rt.uInf.y, rt.uInf.z);
    }

    // ---- Render folder ----
    if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Velocity-magnitude raymarch", &rt.renderVelmag);
        ImGui::Checkbox("Streamlines",                 &rt.renderStreamlines);
        ImGui::SliderInt("Raymarch steps",  &rt.raymarchSteps, 32, 256);
        ImGui::SliderFloat("velmag min", &rt.velmagMin, 0.0f, 0.1f, "%.4f");
        ImGui::SliderFloat("velmag max", &rt.velmagMax, 0.0f, 0.2f, "%.4f");
        ImGui::SliderFloat("Exposure",   &rt.exposure,  0.5f, 3.0f, "%.2f");
    }

    // ---- Streamlines folder ----
    if (ImGui::CollapsingHeader("Streamlines")) {
        bool need_reseed = false;
        if (ImGui::SliderInt("count", reinterpret_cast<int*>(&rt.streamlineCount), 1000, 50000)) {
            need_reseed = true;
        }
        if (ImGui::SliderInt("history", reinterpret_cast<int*>(&rt.streamlineHistory), 16, 128)) {
            need_reseed = true;
        }
        if (need_reseed) {
            renderer.waitIdle();
            allocate_for_tier(ctx, rt.tierIndex, ti, sl);   // reallocates streamline buffer
            wire_all_descriptors();
            seed_streamlines(ctx, ti, sl, true);
        }
    }

    // ---- Capture folder ----
    if (ImGui::CollapsingHeader("Capture")) {
        ImGui::TextDisabled("F5 = save, F9 = load latest");
        ImGui::Checkbox("Export VDB", &rt.exportVdb);
        if (rt.exportVdb) {
            ImGui::SliderInt("VDB every N frames", &rt.vdbEveryNFrames, 1, 60);
            ImGui::TextDisabled("%s", vdb::isAvailable() ? "VDB ready" : "VDB stub mode");
        }
    }

    // ---- Camera folder ----
    if (ImGui::CollapsingHeader("Camera")) {
        camera.drawImguiControls();
    }

    // ---- Stats folder ----
    if (ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("FPS: %.1f", 1000.0f / std::max(rt.lastFrameMs, 0.01f));
        ImGui::Text("Iter: %llu", (unsigned long long)rt.iteration);
        ImGui::Text("Cells: %llu", (unsigned long long)rt.totalCells);
        ImGui::Separator();
        profiler.drawImguiTable();
    }
    ImGui::End();
}
```

#### § 4.B.15 — main() entry + run loop

```cpp
int main(int argc, char** argv) {
    gv::ContextCreateInfo ci{};
    ci.application_name = "lattice_boltzmann";
    ci.enable_swapchain = true;
    ci.enable_subgroup_size_control = true;   // Decision 11
    gv::Context ctx(ci);

    gv::Window window(ctx, 1920, 1080, "GPU-Sims — Lattice Boltzmann");
    gv::Renderer renderer(ctx, window);

    gv::ShaderCompiler compiler;
    compiler.addIncludeDir(GPU_SIMS_LBM_SHADER_DIR);   // for lattice_constants.glsl

    gv::HotReloader reloader(compiler);

    // ... all pipeline / buffer / image / descriptor setup per § 4.B.6 – § 4.B.8 ...

    apply_preset(ctx, DEFAULT_PRESET_INDEX, ti, sl);

    gpusims::Camera camera(gpusims::Camera::Mode::FreeFly);
    camera.setPosition(glm::vec3(float(rt.Nx) * 0.5f, float(rt.Ny) * 0.5f, float(rt.Nz) * 2.5f));
    camera.lookAt(glm::vec3(float(rt.Nx) * 0.5f, float(rt.Ny) * 0.5f, float(rt.Nz) * 0.5f));
    camera.setFovDeg(FOV_DEG_DEFAULT);
    camera.setNearFar(NEAR_PLANE, FAR_PLANE);

    gpusims::ImguiSetup imgui_setup(ctx, window, renderer);
    gv::GpuProfiler profiler(ctx, kSlots);

    gv::StateWriter capture_writer(fs::current_path() / "captures");

    auto last_frame_time = std::chrono::steady_clock::now();
    bool prev_f5 = false, prev_f9 = false;

    while (!window.shouldClose()) {
        window.pollEvents();
        camera.handleInput(window, rt.lastFrameMs * 0.001f);

        // F5 / F9 rising-edge detection.
        bool now_f5 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F5) == GLFW_PRESS;
        bool now_f9 = glfwGetKey(window.glfwWindow(), GLFW_KEY_F9) == GLFW_PRESS;
        if (now_f5 && !prev_f5) capture_save();
        if (now_f9 && !prev_f9) capture_load();
        prev_f5 = now_f5; prev_f9 = now_f9;

        // Tier change check (deferred).
        if (rt.pendingTierIndex != rt.tierIndex) {
            renderer.waitIdle();
            rt.tierIndex = rt.pendingTierIndex;
            rt.Nx = TIERS[rt.tierIndex].Nx;
            rt.Ny = TIERS[rt.tierIndex].Ny;
            rt.Nz = TIERS[rt.tierIndex].Nz;
            rt.totalCells = uint64_t(rt.Nx) * rt.Ny * rt.Nz;
            allocate_for_tier(ctx, rt.tierIndex, ti, sl);
            wire_all_descriptors();
            apply_preset(ctx, rt.presetIndex, ti, sl);
        }

        // Pending load (after tier change).
        if (rt.pendingLoadFromF9) {
            rt.pendingLoadFromF9 = false;
            capture_load();
        }

        // Acquire frame and record commands.
        auto frame_opt = renderer.acquireNextFrame();
        if (!frame_opt) { window.recreateSwapchain(); continue; }
        auto& frame = *frame_opt;
        uint32_t slot = frame.slot;
        VkCommandBuffer cmd = frame.cmd;

        profiler.beginFrame(cmd, slot);

        // Reload-on-edit hooks.
        reloader.tickAndApply(ctx, frame);

        // Substep loop.
        run_frame_substeps(cmd, slot);

        // Optional per-frame VDB export.
        if (rt.exportVdb && vdb::isAvailable() && (rt.iteration % rt.vdbEveryNFrames == 0)) {
            std::vector<float> vel_rgba(rt.totalCells * 4);
            ti.velocity.readback(vel_rgba.data(), vel_rgba.size() * sizeof(float));
            // Convert rgba16f to packed f32 xyz (write_vec3_grid signature).
            // (Implementation: convert via half-to-float on the host.)
            vdb::writeVec3Frame("captures/velocity",
                                uint32_t(rt.iteration / rt.substeps),
                                vel_rgba.data(),
                                glm::ivec3(rt.Nx, rt.Ny, rt.Nz),
                                1.0f, glm::vec3(0.0f), "velocity");
        }

        // Render chain.
        run_frame_render(cmd, slot, frame.image_index);

        profiler.endFrame(cmd, slot);
        renderer.submitAndPresent(frame);

        auto now = std::chrono::steady_clock::now();
        rt.lastFrameMs = std::chrono::duration<float, std::milli>(now - last_frame_time).count();
        last_frame_time = now;
    }

    renderer.waitIdle();
    return 0;
}
```

The half-to-float conversion in the VDB export step is ~10 lines of bit-twiddling; banked at v1.1 as a candidate for `common-cpp` (a `gpusims::half_to_float` helper) if a second consumer surfaces.

---

### § 4.C — `volumetric-grid/lattice-boltzmann/shaders/lattice_constants.glsl`

**This is the most load-bearing shader file in the sim.** It's a shared `#include` consumed by `init_equilibrium`, `collide`, `stream`, `apply_boundaries`, and `compute_moments`. It encodes the 19 velocity vectors, 19 weights, and opposite-direction table from `tools/integrity/docs/algebraic/d3q19.md` § 2.2 — and only those.

**Generation rule:** the contents of this file are mechanically derived from `d3q19.md` § 2.2. Claude Code does NOT retype the values from memory; they are copied from the algebraic derivation document. **Checkpoint 1 fires after this file is generated** — Claude Code parses the file's `C_I[19]` array and `W_I[19]` array, compares to `d3q19.md`, and reports mismatch.

**Full file** (~85 lines):

```glsl
// Auto-derived from tools/integrity/docs/algebraic/d3q19.md § 2.2.
// DO NOT EDIT BY HAND. The 19 velocity vectors and 3 weights are pinned in
// the algebraic ground-truth derivation; this file's job is just to lift them
// into a form GLSL can use.
//
// Direction ordering (per d3q19.md § 2.2):
//   i = 0      : rest                         (0, 0, 0)
//   i = 1..6   : face neighbors  (+x,-x,+y,-y,+z,-z)
//   i = 7..10  : edge neighbors in xy-plane   (xy++, xy+-, xy-+, xy--)
//   i = 11..14 : edge neighbors in xz-plane   (xz++, xz+-, xz-+, xz--)
//   i = 15..18 : edge neighbors in yz-plane   (yz++, yz+-, yz-+, yz--)

#ifndef LATTICE_CONSTANTS_GLSL_INCLUDED
#define LATTICE_CONSTANTS_GLSL_INCLUDED

const int NUM_DIRS = 19;

// Velocity vectors. ivec3 stored in a constant array.
const ivec3 C_I[19] = ivec3[19](
    ivec3( 0,  0,  0),   // i=0  rest
    ivec3( 1,  0,  0),   // i=1  face +x
    ivec3(-1,  0,  0),   // i=2  face -x
    ivec3( 0,  1,  0),   // i=3  face +y
    ivec3( 0, -1,  0),   // i=4  face -y
    ivec3( 0,  0,  1),   // i=5  face +z
    ivec3( 0,  0, -1),   // i=6  face -z
    ivec3( 1,  1,  0),   // i=7  edge xy++
    ivec3( 1, -1,  0),   // i=8  edge xy+-
    ivec3(-1,  1,  0),   // i=9  edge xy-+
    ivec3(-1, -1,  0),   // i=10 edge xy--
    ivec3( 1,  0,  1),   // i=11 edge xz++
    ivec3( 1,  0, -1),   // i=12 edge xz+-
    ivec3(-1,  0,  1),   // i=13 edge xz-+
    ivec3(-1,  0, -1),   // i=14 edge xz--
    ivec3( 0,  1,  1),   // i=15 edge yz++
    ivec3( 0,  1, -1),   // i=16 edge yz+-
    ivec3( 0, -1,  1),   // i=17 edge yz-+
    ivec3( 0, -1, -1)    // i=18 edge yz--
);

// Weights. Three values: ω_rest = 1/3, ω_face = 1/18, ω_edge = 1/36.
const float W_I[19] = float[19](
    1.0/3.0,                                                          // i=0
    1.0/18.0, 1.0/18.0, 1.0/18.0, 1.0/18.0, 1.0/18.0, 1.0/18.0,       // i=1..6
    1.0/36.0, 1.0/36.0, 1.0/36.0, 1.0/36.0,                            // i=7..10
    1.0/36.0, 1.0/36.0, 1.0/36.0, 1.0/36.0,                            // i=11..14
    1.0/36.0, 1.0/36.0, 1.0/36.0, 1.0/36.0                             // i=15..18
);

// Opposite-direction table (the involution c_i ↔ -c_i, per d3q19.md § 2.2):
const int OPPOSITE_DIR[19] = int[19](
     0,  // i=0
     2,  // i=1  opp(+x) = -x = 2
     1,  // i=2  opp(-x) = +x = 1
     4,  // i=3  opp(+y) = -y = 4
     3,  // i=4
     6,  // i=5
     5,  // i=6
    10,  // i=7  opp(xy++) = xy-- = 10
     9,  // i=8  opp(xy+-) = xy-+ = 9
     8,  // i=9
     7,  // i=10
    14,  // i=11 opp(xz++) = xz-- = 14
    13,  // i=12 opp(xz+-) = xz-+ = 13
    12,  // i=13
    11,  // i=14
    18,  // i=15 opp(yz++) = yz-- = 18
    17,  // i=16
    16,  // i=17
    15   // i=18
);

// Lattice sound-speed squared. c_s² = 1/3 per d3q19.md § 3.
const float CS2     = 1.0 / 3.0;
const float CS2_INV = 3.0;                  // 1 / c_s²
const float CS4_HALF_INV = 4.5;             // 1 / (2 c_s⁴) = 9/2
const float CS2_HALF_INV = 1.5;             // 1 / (2 c_s²) = 3/2

// Compute equilibrium f_i for a given (rho, u). Compressible Maxwell-Boltzmann form:
//   feq_i = w_i ρ [1 + 3(c·u) + 4.5(c·u)² − 1.5(u·u)]
// Reference: tools/integrity/docs/algebraic/d3q19.md § 4.1.
//           references/lbm-principles-practice/chapter13/cpu/LBM.cpp:97 (D2Q9 form; same structure).
float feq_i(int i, float rho, vec3 u) {
    vec3  ci   = vec3(C_I[i]);
    float cdotu = dot(ci, u);
    float udotu = dot(u, u);
    return W_I[i] * rho * (1.0 + CS2_INV * cdotu
                               + CS4_HALF_INV * cdotu * cdotu
                               - CS2_HALF_INV * udotu);
}

#endif  // LATTICE_CONSTANTS_GLSL_INCLUDED
```

**Checkpoint 1 protocol** (post-generation):

1. Parse C_I[19] from the generated file (the 19 ivec3 literals).
2. Compare to d3q19.md § 2.2 table — direction-by-direction byte equality.
3. Parse W_I[19] — confirm the three values 1/3, 1/18, 1/36 in the expected positions.
4. Parse OPPOSITE_DIR[19] — for each i, verify C_I[i] + C_I[OPPOSITE_DIR[i]] = (0,0,0).
5. If ANY mismatch, STOP. Report the index and the discrepancy.

### § 4.D — `shaders/init_equilibrium.comp.glsl`

Initializes f_i = feq(ρ_0, u_∞) at every fluid cell. Solid-cell cleanup is deferred to the first boundaries pass.

```glsl
#version 460

layout(local_size_x = 8, local_size_y = 8, local_size_z = 4) in;

#include "lattice_constants.glsl"

layout(set=0, binding=0, r32f)   uniform image3D f_nonrest[NUM_DIRS - 1];
layout(set=0, binding=1, r32f)   uniform image3D f_rest;
layout(set=0, binding=2, r32f)   uniform image3D rho_out;
layout(set=0, binding=3, rgba16f) uniform image3D velocity_out;

layout(set=0, binding=4) uniform InitUniforms {
    ivec4 dims;
    float tau_inv;
    float omtau_inv;
    float _pad0, _pad1;
} U;

layout(push_constant) uniform PushConsts {
    vec3  u_inf;
    float _pad;
} pc;

void main() {
    ivec3 cell = ivec3(gl_GlobalInvocationID.xyz);
    if (any(greaterThanEqual(cell, U.dims.xyz))) return;

    float rho = 1.0;
    vec3  u   = pc.u_inf;

    imageStore(rho_out,      cell, vec4(rho, 0, 0, 0));
    imageStore(velocity_out, cell, vec4(u, 0));
    imageStore(f_rest,       cell, vec4(feq_i(0, rho, u), 0, 0, 0));
    for (int i = 1; i < NUM_DIRS; ++i) {
        imageStore(f_nonrest[i - 1], cell, vec4(feq_i(i, rho, u), 0, 0, 0));
    }
}
```

### § 4.E — `shaders/collide.comp.glsl`

BGK relaxation. Reads f in-place; reads (ρ, u) from the moments buffer (lag-1 moments — previous substep's results); computes feq via `feq_i()` from the include; relaxes via `f' = (1−1/τ) f + (1/τ) f_eq`.

```glsl
#version 460

layout(local_size_x = 8, local_size_y = 8, local_size_z = 4) in;

#include "lattice_constants.glsl"

layout(set=0, binding=0, r32f) uniform image3D f_nonrest[NUM_DIRS - 1];
layout(set=0, binding=1, r32f) uniform image3D f_rest;
layout(set=0, binding=2)       uniform sampler3D rho_in;
layout(set=0, binding=3)       uniform sampler3D velocity_in;

layout(set=0, binding=4) uniform CollideUniforms {
    ivec4 dims;
    float tau_inv;
    float omtau_inv;
    float _pad0, _pad1;
} U;

void main() {
    ivec3 cell = ivec3(gl_GlobalInvocationID.xyz);
    if (any(greaterThanEqual(cell, U.dims.xyz))) return;

    vec3 ctr = (vec3(cell) + 0.5) / vec3(U.dims.xyz);
    float rho = texture(rho_in, ctr).r;
    vec3  u   = texture(velocity_in, ctr).xyz;

    // i=0 (rest).
    {
        float f   = imageLoad(f_rest, cell).r;
        float feq = feq_i(0, rho, u);
        float fp  = U.omtau_inv * f + U.tau_inv * feq;
        imageStore(f_rest, cell, vec4(fp, 0, 0, 0));
    }

    // i=1..18 (nonrest).
    for (int i = 1; i < NUM_DIRS; ++i) {
        float f   = imageLoad(f_nonrest[i - 1], cell).r;
        float feq = feq_i(i, rho, u);
        float fp  = U.omtau_inv * f + U.tau_inv * feq;
        imageStore(f_nonrest[i - 1], cell, vec4(fp, 0, 0, 0));
    }

    // -- Krüger factored form (chapter13/cpu/LBM.cpp:97), banked v1.1 perf:
    //   omusq = 1 - 1.5*(u·u);  cidot3u = 3*(c·u);
    //   f' = omtauinv*f + tauinv*w_i*rho * (omusq + cidot3u*(1 + 0.5*cidot3u))
    // Algebraically identical; saves ~5% via FMA chain on RDNA.
}
```

### § 4.F — `shaders/stream.comp.glsl`

Pull semantics: `f_out[x][i] = f_in[x − c_i][i]`. Rest direction (i=0) doesn't stream; copy through.

```glsl
#version 460

layout(local_size_x = 8, local_size_y = 8, local_size_z = 4) in;

#include "lattice_constants.glsl"

layout(set=0, binding=0)       uniform sampler3D f_in_nonrest[NUM_DIRS - 1];
layout(set=0, binding=1)       uniform sampler3D f_in_rest;
layout(set=0, binding=2, r32f) uniform image3D   f_out_nonrest[NUM_DIRS - 1];
layout(set=0, binding=3, r32f) uniform image3D   f_out_rest;

layout(set=0, binding=4) uniform StreamUniforms {
    ivec4 dims;
} U;

void main() {
    ivec3 cell = ivec3(gl_GlobalInvocationID.xyz);
    if (any(greaterThanEqual(cell, U.dims.xyz))) return;

    // Rest: no stream.
    {
        vec3 ctr = (vec3(cell) + 0.5) / vec3(U.dims.xyz);
        imageStore(f_out_rest, cell, vec4(texture(f_in_rest, ctr).r, 0, 0, 0));
    }
    // Nonrest: pull from cell - c_i.
    for (int i = 1; i < NUM_DIRS; ++i) {
        ivec3 src = cell - C_I[i];
        vec3  ctr = (vec3(src) + 0.5) / vec3(U.dims.xyz);
        imageStore(f_out_nonrest[i - 1], cell,
                   vec4(texture(f_in_nonrest[i - 1], ctr).r, 0, 0, 0));
    }
}
```

**Why pull, not push:** push writes to `cell + c_i` which creates inter-thread write conflicts at boundaries. Pull writes only to the current cell. Standard LBM pattern.

**Checkpoint 3 fires here.** Mental-trace one substep:
- Initial: f_i = feq(1, u=(0.1,0,0)) everywhere (from init_equilibrium with the preset u_∞).
- Post-collide with τ=0.6: f = (1 − 1.667)·f + 1.667·feq. Since f starts equal to feq, post-collide = feq exactly. ✓ (collide is identity at equilibrium — this is the LBM correctness check)
- Post-stream: every cell reads its upstream neighbor; for uniform initial state every cell sees the same value; post-stream = pre-stream. ✓

Mismatch → STOP.

### § 4.G — `shaders/apply_boundaries.comp.glsl`

**The most complex shader in the sim.** ~150 lines. Applies four boundary regimes in one dispatch:

1. **−X face (i_x = 0): Zou-He velocity inlet** with **u** = **u**_∞.
2. **+X face (i_x = Nx − 1): Zou-He pressure outlet** with ρ = ρ_0.
3. **±Y and ±Z faces: free-slip** (specular reflection of normal-component-bearing populations).
4. **Interior: halfway bounce-back** at every fluid cell adjacent to a solid cell.

**Checkpoint 4 fires before this file lands.** Before generating, Claude Code writes out the `OPPOSITE_DIR[19]` table from `d3q19.md` § 2.2 and verifies via the c_i + c_OPPOSITE[i] = 0 sum check. (This is duplicative with Checkpoint 1 in that `lattice_constants.glsl` already encodes the table, but Checkpoint 4 is a re-verification gate specifically for this shader because halfway-BB correctness depends entirely on it.)

**The Zou-He inlet equations** (for the −X face, normal pointing into the domain in +X direction):

Given **u**_∞ = (u_x, u_y, u_z), the unknowns at the boundary are the populations whose c_i points into the domain (c_i,x > 0), namely i ∈ {1, 7, 8, 11, 12} (the +x face direction + the four +x-bearing edges). The Zou-He closure gives:

```
ρ = (1 / (1 − u_x)) · [ f_0 + (f_3 + f_4 + f_5 + f_6 + f_15 + f_16 + f_17 + f_18)
                         + 2 (f_2 + f_9 + f_10 + f_13 + f_14) ]

f_1  = f_2  + (2/3) ρ u_x
f_7  = f_10 + (1/2)(f_4 + f_18 + f_16 − f_3 − f_15 − f_17)
              + (1/6) ρ u_x + (1/2) ρ u_y
f_8  = f_9  + (1/2)(f_3 + f_15 + f_17 − f_4 − f_18 − f_16)
              + (1/6) ρ u_x − (1/2) ρ u_y
f_11 = f_14 + (1/2)(f_6 + f_16 + f_18 − f_5 − f_15 − f_17)
              + (1/6) ρ u_x + (1/2) ρ u_z
f_12 = f_13 + (1/2)(f_5 + f_15 + f_17 − f_6 − f_16 − f_18)
              + (1/6) ρ u_x − (1/2) ρ u_z
```

Reference: Zou & He 1997, "On pressure and velocity boundary conditions for the lattice Boltzmann BGK model," Phys. Fluids 9, 1591. The 3D D3Q19 generalization is in Hecht & Harting 2010, arXiv:0811.4593 § III (one of the sources from the probe-2 web search). **Both inlet and outlet equations need verification at write time** — the equations above are written from the architect's understanding of the canonical Zou-He 3D form, but the algebraic checking discipline says: Claude Code should derive the closure independently as a step in writing this shader. If derivations disagree, STOP.

**Halfway bounce-back rule**: at every fluid cell where any neighbor in direction c_i is solid (i.e., `obstacle_mask[cell + c_i] == 1`), swap the post-stream population: `f_OPPOSITE[i] := f_i` (pre-stream value, but since we run boundaries AFTER stream, we use the post-stream f at the current cell, which equals the pre-stream value from the cell that c_i would have streamed FROM). Equivalently in post-stream form:

> Before this pass: stream wrote `f_out[cell][i] = f_in[cell − c_i][i]`. For directions where `cell − c_i` is solid, that read was clamp-to-edge and gave garbage. Halfway-BB overrides: `f_out[cell][OPPOSITE_DIR[i]] = f_out[cell][i]` (using the value that streamed from a fluid neighbor along OPPOSITE_DIR[i], which was a valid read).

Let me restate more carefully because this is the load-bearing math. **The halfway-BB pattern post-stream** (the spec adopts the post-stream-correction form, NOT the pre-stream form):

For each fluid cell `cell` and each direction i:
- If `cell + c_i` is solid (the population f_i at the current cell came from streaming INTO the solid), then the cell that should have streamed INTO this cell from the opposite direction (`cell − c_OPPOSITE[i] = cell + c_i`) was a solid cell that didn't stream. So `f_OPPOSITE[i]` at the current cell holds garbage (or zero, from clamp-to-edge). Overwrite: `f_OPPOSITE[i] := f_i` (which is a valid post-collision value from this cell).

Equivalently, the post-stream value of f_OPPOSITE at cell equals the pre-stream value of f_i at cell, by the bounce-back physics (a particle moving toward the wall in direction i bounces back along OPPOSITE[i] with halfway offset).

```glsl
#version 460

layout(local_size_x = 8, local_size_y = 8, local_size_z = 4) in;

#include "lattice_constants.glsl"

layout(set=0, binding=0, r32f) uniform image3D f_nonrest[NUM_DIRS - 1];
layout(set=0, binding=1, r32f) uniform image3D f_rest;
layout(set=0, binding=2)       uniform usampler3D obstacle_mask;   // r8uint, point-sampled

layout(set=0, binding=3) uniform BoundariesUniforms {
    ivec4 dims;
    vec4  u_inf;          // (u_x, u_y, u_z, rho_0)
    ivec4 inlet_axis;     // reserved
} U;

float load_f(int i, ivec3 cell) {
    if (i == 0) return imageLoad(f_rest, cell).r;
    return imageLoad(f_nonrest[i - 1], cell).r;
}
void store_f(int i, ivec3 cell, float v) {
    if (i == 0) imageStore(f_rest, cell, vec4(v, 0, 0, 0));
    else        imageStore(f_nonrest[i - 1], cell, vec4(v, 0, 0, 0));
}
uint mask_at(ivec3 c) {
    if (any(lessThan(c, ivec3(0))) || any(greaterThanEqual(c, U.dims.xyz))) return 1u;
    return texelFetch(obstacle_mask, c, 0).r;
}

void main() {
    ivec3 cell = ivec3(gl_GlobalInvocationID.xyz);
    if (any(greaterThanEqual(cell, U.dims.xyz))) return;

    // Solid cells: zero out f (they don't participate in dynamics).
    if (mask_at(cell) == 1u) {
        store_f(0, cell, 0.0);
        for (int i = 1; i < NUM_DIRS; ++i) store_f(i, cell, 0.0);
        return;
    }

    // ---- Halfway bounce-back at fluid cells adjacent to solid neighbors ----
    // For each direction i, if cell+c_i is solid, overwrite f_OPPOSITE[i] at cell
    // with the current cell's f_i (the would-be-streamed-into-solid value).
    for (int i = 1; i < NUM_DIRS; ++i) {
        ivec3 neigh = cell + C_I[i];
        if (mask_at(neigh) == 1u) {
            float fi = load_f(i, cell);
            store_f(OPPOSITE_DIR[i], cell, fi);
        }
    }

    // ---- -X face: Zou-He velocity inlet ----
    if (cell.x == 0) {
        float ux = U.u_inf.x, uy = U.u_inf.y, uz = U.u_inf.z;
        float f0  = load_f( 0, cell);
        float f2  = load_f( 2, cell);
        float f3  = load_f( 3, cell);
        float f4  = load_f( 4, cell);
        float f5  = load_f( 5, cell);
        float f6  = load_f( 6, cell);
        float f9  = load_f( 9, cell);
        float f10 = load_f(10, cell);
        float f13 = load_f(13, cell);
        float f14 = load_f(14, cell);
        float f15 = load_f(15, cell);
        float f16 = load_f(16, cell);
        float f17 = load_f(17, cell);
        float f18 = load_f(18, cell);
        // Density from Zou-He inlet closure:
        float rho = (1.0 / (1.0 - ux)) * (f0
                    + (f3 + f4 + f5 + f6 + f15 + f16 + f17 + f18)
                    + 2.0 * (f2 + f9 + f10 + f13 + f14));
        // Unknown +x-bearing populations:
        float f1  = f2  + (2.0/3.0) * rho * ux;
        float f7  = f10 + 0.5 * (f4 + f18 + f16 - f3 - f15 - f17)
                       + (1.0/6.0) * rho * ux + 0.5 * rho * uy;
        float f8  = f9  + 0.5 * (f3 + f15 + f17 - f4 - f18 - f16)
                       + (1.0/6.0) * rho * ux - 0.5 * rho * uy;
        float f11 = f14 + 0.5 * (f6 + f16 + f18 - f5 - f15 - f17)
                       + (1.0/6.0) * rho * ux + 0.5 * rho * uz;
        float f12 = f13 + 0.5 * (f5 + f15 + f17 - f6 - f16 - f18)
                       + (1.0/6.0) * rho * ux - 0.5 * rho * uz;
        store_f( 1, cell, f1);
        store_f( 7, cell, f7);
        store_f( 8, cell, f8);
        store_f(11, cell, f11);
        store_f(12, cell, f12);
    }

    // ---- +X face: Zou-He pressure outlet (ρ = rho_0 = u_inf.w) ----
    if (cell.x == U.dims.x - 1) {
        float rho = U.u_inf.w;
        float f0  = load_f( 0, cell);
        float f1  = load_f( 1, cell);
        float f3  = load_f( 3, cell);
        float f4  = load_f( 4, cell);
        float f5  = load_f( 5, cell);
        float f6  = load_f( 6, cell);
        float f7  = load_f( 7, cell);
        float f8  = load_f( 8, cell);
        float f11 = load_f(11, cell);
        float f12 = load_f(12, cell);
        float f15 = load_f(15, cell);
        float f16 = load_f(16, cell);
        float f17 = load_f(17, cell);
        float f18 = load_f(18, cell);
        // u_x derived from outlet closure (Zou-He pressure outlet on +X face):
        float ux = -1.0 + (1.0 / rho) * (f0
                    + (f3 + f4 + f5 + f6 + f15 + f16 + f17 + f18)
                    + 2.0 * (f1 + f7 + f8 + f11 + f12));
        // Unknown -x-bearing populations (symmetric to inlet):
        float f2  = f1  - (2.0/3.0) * rho * ux;
        float f9  = f8  + 0.5 * (f3 + f15 + f17 - f4 - f18 - f16)
                       - (1.0/6.0) * rho * ux;
        float f10 = f7  + 0.5 * (f4 + f18 + f16 - f3 - f15 - f17)
                       - (1.0/6.0) * rho * ux;
        float f13 = f12 + 0.5 * (f5 + f15 + f17 - f6 - f16 - f18)
                       - (1.0/6.0) * rho * ux;
        float f14 = f11 + 0.5 * (f6 + f16 + f18 - f5 - f15 - f17)
                       - (1.0/6.0) * rho * ux;
        store_f( 2, cell, f2);
        store_f( 9, cell, f9);
        store_f(10, cell, f10);
        store_f(13, cell, f13);
        store_f(14, cell, f14);
    }

    // ---- ±Y, ±Z faces: free-slip (specular reflection) ----
    // The wall-normal component of velocity is reflected; tangential components pass through.
    // Equivalent statement on populations: for each population whose c_i has a nonzero
    // wall-normal component, swap with the population whose c_i differs only in the
    // sign of the wall-normal axis.
    if (cell.y == 0) {
        // -Y wall: reflect populations with c_iy = -1 from those with c_iy = +1.
        // Pairs (in our ordering): (3,4), (7,8), (9,10), (15,16), (17,18).
        // But (3,4) is x-axis-symmetric — actually (3 has c=(0,+1,0), 4 has c=(0,-1,0));
        // for free-slip on -Y, the *incoming* population is c_iy < 0 (i.e., i = 4, 8, 10, 17, 18).
        // Set f_incoming = f_outgoing_with_opposite_y, leaving other components untouched.
        // The free-slip "specular" map is: c=(cx, -1, cz) ↔ c=(cx, +1, cz).
        // Mapping table for -Y wall (incoming i → reflected-from i):
        //   i=4  (0,-1,0)  ←  i=3  (0,+1,0)
        //   i=8  (+1,-1,0) ←  i=7  (+1,+1,0)
        //   i=10 (-1,-1,0) ←  i=9  (-1,+1,0)
        //   i=17 (0,-1,+1) ←  i=15 (0,+1,+1)
        //   i=18 (0,-1,-1) ←  i=16 (0,+1,-1)
        store_f( 4, cell, load_f( 3, cell));
        store_f( 8, cell, load_f( 7, cell));
        store_f(10, cell, load_f( 9, cell));
        store_f(17, cell, load_f(15, cell));
        store_f(18, cell, load_f(16, cell));
    }
    if (cell.y == U.dims.y - 1) {
        // +Y wall: incoming is c_iy = +1.
        store_f( 3, cell, load_f( 4, cell));
        store_f( 7, cell, load_f( 8, cell));
        store_f( 9, cell, load_f(10, cell));
        store_f(15, cell, load_f(17, cell));
        store_f(16, cell, load_f(18, cell));
    }
    if (cell.z == 0) {
        // -Z wall: incoming is c_iz = -1. Pairs (5,6), (11,12), (13,14), (15,16), (17,18).
        // For -Z reflection: c=(cx, cy, -1) ↔ c=(cx, cy, +1).
        // Mapping:
        //   i=6  (0,0,-1)  ←  i=5  (0,0,+1)
        //   i=12 (+1,0,-1) ←  i=11 (+1,0,+1)
        //   i=14 (-1,0,-1) ←  i=13 (-1,0,+1)
        //   i=16 (0,+1,-1) ←  i=15 (0,+1,+1)
        //   i=18 (0,-1,-1) ←  i=17 (0,-1,+1)
        store_f( 6, cell, load_f( 5, cell));
        store_f(12, cell, load_f(11, cell));
        store_f(14, cell, load_f(13, cell));
        store_f(16, cell, load_f(15, cell));
        store_f(18, cell, load_f(17, cell));
    }
    if (cell.z == U.dims.z - 1) {
        store_f( 5, cell, load_f( 6, cell));
        store_f(11, cell, load_f(12, cell));
        store_f(13, cell, load_f(14, cell));
        store_f(15, cell, load_f(16, cell));
        store_f(17, cell, load_f(18, cell));
    }
}
```

**Verification at write time**: Claude Code re-derives the Zou-He closure independently before locking the formulas. The 3D Zou-He inlet has 5 unknowns (the 5 populations with c_x > 0); the constraint system gives 5 equations from (a) ρ_x = u_x · ρ (momentum constraint in +x), (b)–(e) the four tangential momentum + density constraints. The formulas above are standard but derivation-by-eye risks transcription error. **If Claude Code's derivation produces different equations than the spec, STOP and report** — this is the single most fabrication-prone block in the entire sim.

### § 4.H — `shaders/compute_moments.comp.glsl`

ρ = Σ_i f_i; ρ**u** = Σ_i f_i c_i; **u** = (ρ**u**) / ρ.

Uses subgroup operations (`subgroupAdd`) for the 19-direction reduction inside each invocation. The dispatch is per-cell (8×8×4 workgroup); the reduction is per-thread (every thread reduces its own 19 values).

Wait, that's not actually subgroup-related — each thread does its own 19-element loop. The subgroup is irrelevant for the moment reduction since each cell is independent. Decision 11's subgroup-size pinning is preserved for *correctness on hardware that requires it for image atomic ops* (none here), not for performance.

```glsl
#version 460
#extension GL_KHR_shader_subgroup_basic : enable

layout(local_size_x = 8, local_size_y = 8, local_size_z = 4) in;

#include "lattice_constants.glsl"

layout(set=0, binding=0)       uniform sampler3D f_nonrest[NUM_DIRS - 1];
layout(set=0, binding=1)       uniform sampler3D f_rest;
layout(set=0, binding=2, r32f) uniform image3D   rho_out;
layout(set=0, binding=3, rgba16f) uniform image3D velocity_out;

layout(set=0, binding=4) uniform MomentsUniforms {
    ivec4 dims;
} U;

void main() {
    ivec3 cell = ivec3(gl_GlobalInvocationID.xyz);
    if (any(greaterThanEqual(cell, U.dims.xyz))) return;

    vec3 ctr = (vec3(cell) + 0.5) / vec3(U.dims.xyz);
    float rho = texture(f_rest, ctr).r;     // i=0
    vec3  rho_u = vec3(0);

    for (int i = 1; i < NUM_DIRS; ++i) {
        float f = texture(f_nonrest[i - 1], ctr).r;
        rho   += f;
        rho_u += f * vec3(C_I[i]);
    }

    // Guard against divide-by-zero at solid cells (ρ=0 there).
    vec3 u = (rho > 1e-12) ? (rho_u / rho) : vec3(0);

    imageStore(rho_out,      cell, vec4(rho, 0, 0, 0));
    imageStore(velocity_out, cell, vec4(u, 0));
}
```

### § 4.I — `shaders/streamline_advect.comp.glsl`

One thread per streamline. Each thread:
1. Reads its current position from `position_history[streamline_index * history + head_index]`.
2. Checks age; if `age >= reseed_age_threshold`, reseeds to a random inlet-slab position.
3. Computes RK2 advection step using trilinearly-sampled velocity.
4. Writes new position to the head slot.

```glsl
#version 460

layout(local_size_x = 64) in;

layout(set=0, binding=0)        uniform sampler3D velocity_field;
layout(set=0, binding=1, std430) buffer PositionHistory {
    vec4 positions[];   // size = streamline_count * history; each is (xyz, age)
};

layout(set=0, binding=2) uniform StreamlineAdvectUniforms {
    ivec4    dims;
    vec4     domain_min;
    vec4     domain_max;
    uint     streamline_count;
    uint     history;
    uint     head_index;
    uint     frame_count;
    float    dt_render;
    uint     reseed_age_threshold;
    uint     _pad0, _pad1;
} U;

// xorshift32 RNG for reseeding.
uint xorshift(inout uint state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}
float rng01(inout uint state) {
    return float(xorshift(state) & 0x00FFFFFFu) / float(0x01000000u);
}

void main() {
    uint sid = gl_GlobalInvocationID.x;
    if (sid >= U.streamline_count) return;

    uint prev_head = (U.head_index + U.history - 1u) % U.history;
    vec4 prev = positions[sid * U.history + prev_head];
    vec3 pos = prev.xyz;
    float age = prev.w;

    // Reseed if too old or out-of-domain.
    bool out_of_bounds = any(lessThan(pos, U.domain_min.xyz))
                      || any(greaterThan(pos, U.domain_max.xyz));
    if (age >= float(U.reseed_age_threshold) || out_of_bounds) {
        uint state = sid * 2654435761u ^ U.frame_count * 1597334677u;
        float x = U.domain_min.x + (U.domain_max.x - U.domain_min.x) * 0.0625 * (1.0 + rng01(state));
        float y = U.domain_min.y + (U.domain_max.y - U.domain_min.y) * rng01(state);
        float z = U.domain_min.z + (U.domain_max.z - U.domain_min.z) * rng01(state);
        pos = vec3(x, y, z);
        age = 0.0;
    } else {
        // RK2 step.
        vec3 ctr1 = pos / vec3(U.dims.xyz);
        vec3 u1   = texture(velocity_field, ctr1).xyz;
        vec3 mid  = pos + 0.5 * U.dt_render * u1;
        vec3 ctr2 = mid / vec3(U.dims.xyz);
        vec3 u2   = texture(velocity_field, ctr2).xyz;
        pos += U.dt_render * u2;
        age += 1.0;
    }

    positions[sid * U.history + U.head_index] = vec4(pos, age);
}
```

The `dt_render` is in lattice-units-per-frame. At 60 FPS with the desktop tier (256×128×128) and |u| ≤ 0.06 (preset 2), one streamline advances at most 0.06 lattice units per frame ≈ 0.06 voxel per frame. 64-frame history therefore traces ~4 voxels of trail — visible at the typical render scale.

### § 4.J — `shaders/velmag.frag.glsl`

Volume raymarch of velocity magnitude with colormap LUT lookup. ~80 lines. Generalizes ES's `raymarch.frag.glsl` for non-unit-cube domains via the `volumeAspect` uniform.

```glsl
#version 460

layout(location = 0) in  vec2 v_uv;
layout(location = 0) out vec4 out_color;

layout(set=0, binding=0) uniform sampler3D velocity_field;     // rgba16f
layout(set=0, binding=1) uniform sampler2D colormap_lut;       // viridis or similar
layout(set=0, binding=2) uniform sampler2D blue_noise;         // for ray jitter

layout(set=0, binding=3) uniform RaymarchUniforms {
    mat4 invViewProj;
    vec4 cameraPos;
    vec4 volumeMin;
    vec4 volumeMax;
    vec4 volumeAspect;     // (Nx, Ny, Nz, max_dim) for shadow-step generalization
    int  raymarchSteps;
    int  _pad0;
    float velmagAbsorption;
    float velmagMin;
    float velmagMax;
    float exposure;
    float _pad1, _pad2;
} U;

vec3 reconstructRayDir(vec2 uv) {
    vec4 ndc_far = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 world_far = U.invViewProj * ndc_far;
    world_far /= world_far.w;
    return normalize(world_far.xyz - U.cameraPos.xyz);
}

// Slab intersection — returns (t_enter, t_exit) for the volumeMin/Max AABB.
vec2 slabIntersect(vec3 ro, vec3 rd, vec3 bmin, vec3 bmax) {
    vec3 inv = 1.0 / rd;
    vec3 t0 = (bmin - ro) * inv;
    vec3 t1 = (bmax - ro) * inv;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float t_enter = max(max(tmin.x, tmin.y), tmin.z);
    float t_exit  = min(min(tmax.x, tmax.y), tmax.z);
    return vec2(t_enter, t_exit);
}

void main() {
    vec3 ro = U.cameraPos.xyz;
    vec3 rd = reconstructRayDir(v_uv);

    vec2 t = slabIntersect(ro, rd, U.volumeMin.xyz, U.volumeMax.xyz);
    if (t.y < t.x || t.y < 0.0) { out_color = vec4(0); return; }
    float t0 = max(t.x, 0.0);
    float t1 = t.y;
    float ray_len = t1 - t0;

    // Jitter via blue noise to avoid stripe artifacts.
    float jitter = texture(blue_noise, gl_FragCoord.xy / 256.0).r;
    float dt = ray_len / float(U.raymarchSteps);
    float t_cur = t0 + jitter * dt;

    vec3 color  = vec3(0);
    float trans = 1.0;
    vec3 domain_extent = U.volumeMax.xyz - U.volumeMin.xyz;

    for (int i = 0; i < U.raymarchSteps && trans > 0.01; ++i) {
        vec3 p = ro + rd * t_cur;
        vec3 uvw = (p - U.volumeMin.xyz) / domain_extent;
        if (all(greaterThanEqual(uvw, vec3(0))) && all(lessThanEqual(uvw, vec3(1)))) {
            vec3 u = texture(velocity_field, uvw).xyz;
            float mag = length(u);
            float t01 = clamp((mag - U.velmagMin) / max(U.velmagMax - U.velmagMin, 1e-6),
                              0.0, 1.0);
            vec3 c = texture(colormap_lut, vec2(t01, 0.5)).rgb;
            float abs_step = U.velmagAbsorption * dt * t01;
            color += trans * c * abs_step * U.exposure;
            trans *= exp(-abs_step);
        }
        t_cur += dt;
    }

    out_color = vec4(color, 1.0 - trans);
}
```

**Generalization vs. ES's raymarch:** the `domain_extent` is the actual physical span, not a unit cube. The slab intersection takes `volumeMin`/`volumeMax` from the uniform rather than `vec3(0,0,0)` / `vec3(1,1,1)` hardcoded. The shadow-step generalization that probe-1 § D flagged in ES (`shadow_step = sqrt(3) / N`) doesn't apply here because LBM has no shadow/scattering pass — velocity-magnitude is a one-shot LUT lookup, no light propagation through the volume.

### § 4.K — `shaders/streamline.vert.glsl` and `streamline.frag.glsl`

**streamline.vert.glsl**:

```glsl
#version 460

layout(set=0, binding=0, std430) buffer PositionHistory {
    vec4 positions[];   // (xyz, age)
};

layout(set=0, binding=1) uniform StreamlineRenderUniforms {
    mat4 viewProj;
    vec4 lineColor;
    uint history;
    float ageFalloff;
    float _pad0, _pad1;
} U;

layout(location = 0) out vec4 v_color;

void main() {
    // gl_VertexIndex spans streamline_count * (history + 1).
    // The +1 carries a primitive-restart vertex with all components NaN.
    uint vi = uint(gl_VertexIndex);
    uint per_strip = U.history + 1u;
    uint sid       = vi / per_strip;
    uint hi        = vi % per_strip;

    if (hi == U.history) {
        // Primitive-restart marker — emit degenerate vertex.
        // Vulkan with primitive_restart_enable consumes 0xFFFFFFFF as the restart index;
        // since we're not using an index buffer, we use NaN positions which the
        // rasterizer culls.
        gl_Position = vec4(0.0/0.0);   // NaN
        v_color     = vec4(0);
        return;
    }

    vec4 p = positions[sid * U.history + hi];
    gl_Position = U.viewProj * vec4(p.xyz, 1.0);
    float age_frac = p.w / float(U.history);
    float alpha = exp(-U.ageFalloff * age_frac);
    v_color = vec4(U.lineColor.rgb, U.lineColor.a * alpha);
}
```

**Note on primitive-restart**: the NaN approach is a portability shortcut; the cleaner alternative is to use an index buffer with the restart sentinel, but that requires CPU upload per frame. For v1, NaN positions work because the rasterizer clips degenerate triangles, and the line strip simply breaks. Banked as v1.1: switch to index-buffer-based primitive restart for cleaner GPU profiling.

**streamline.frag.glsl**:

```glsl
#version 460

layout(location = 0) in  vec4 v_color;
layout(location = 0) out vec4 out_color;

layout(set=0, binding=1) uniform StreamlineRenderUniforms {
    mat4 viewProj;
    vec4 lineColor;
    uint history;
    float ageFalloff;
    float _pad0, _pad1;
} U;

void main() {
    out_color = v_color;
}
```

### § 4.L — `shaders/fullscreen.vert.glsl`

Canonical full-screen-triangle vertex shader. Copy-paste from ES:

```glsl
#version 460

layout(location = 0) out vec2 v_uv;

void main() {
    // Sascha Willems-style full-screen triangle.
    vec2 verts[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2(-1.0,  3.0),
        vec2( 3.0, -1.0)
    );
    vec2 uvs[3] = vec2[3](
        vec2(0.0, 0.0),
        vec2(0.0, 2.0),
        vec2(2.0, 0.0)
    );
    gl_Position = vec4(verts[gl_VertexIndex], 0.0, 1.0);
    v_uv        = uvs[gl_VertexIndex];
}
```

### § 4.M — `volumetric-grid/lattice-boltzmann/docs/load-bearing-decisions.md`

Sim-local quick reference for future readers. ~80 lines. Distills § 2 of this spec into a single-page summary for the sim-internal viewer. Lists each LBD by number with a one-paragraph rationale and a pointer to the spec section for full justification.

Skeleton:

```markdown
# Load-bearing decisions — lattice-boltzmann (Phase 12)

This document summarizes the 13 load-bearing decisions for `volumetric-grid/lattice-boltzmann/`. Detailed rationale and rejection-of-alternatives in `docs/phase12_lattice_boltzmann.md` § 2.

## D1 — Algorithm: D3Q19 BGK single-relaxation-time
Canonical 1990s technique. Cross-referenced against Wikipedia, arXiv 2004.03509, Krüger 2017, TU Delft Wetstein thesis. See spec § 2 D1.

## D2 — Storage: SoA, one buffer per direction; rest-direction split
19+19 r32f 3D images ping-pong. Rest direction in its own image (saves 1/19 memory traffic).

## D3 — Streaming: two-buffer ping-pong with parity bit
Matches ES pattern.

## D4 — Separate dispatches for collide / stream / boundaries / moments
Modularity over throughput. Fused stream-collide banked as v1.1 optimization.

## D5 — Macroscopic moments in dedicated r32f + rgba16f images
Read by collide (next substep), raymarch, streamlines.

## D6 — Boundaries: Zou-He inlet/outlet + free-slip ±Y±Z + halfway-BB airfoil
Standard wind-tunnel boundaries. Zou-He 3D closure derived from Zou&He 1997 + Hecht&Harting 2010.

## D7 — Obstacle: CPU-generated NACA SDF voxelized to 3D r8 mask
~50 LOC sim-local CPU code, runs once per preset. Not promoted to common-cpp at v1.

## D8 — Render: sim-local velmag.frag.glsl, NOT rule-of-three promote
ES's raymarch is physics-based; LBM's is LUT-based. Promote at consumer #3.

## D9 — Streamlines: GPU-seeded ring-buffer history with RK2 advection
~10k seeds × 64 history. Reseeded by GPU.

## D10 — Tiers: 128³, 256×128×128 (default), 512×256×256
Deferred-change after window.show (Phase 9/10/11 pattern).

## D11 — Subgroup-size pinning to 32 (opt-in via Phase 11 surface)
Verified on RX 6800 XT 2026-05-15: min=32, max=64, control=true.

## D12 — Capture: latticeBoltzmann key, bare names, frame_invariant obstacle_mask
Avoids ES .bin.bin bug. Introduces new convention.

## D13 — Reference anchors: [Krueger] + [Algebraic_D3Q19]
Math from Krüger, constants from algebraic derivation.
```

### § 4.N — `volumetric-grid/lattice-boltzmann/docs/notes.md`

v1.1 polish backlog and known limitations.

Skeleton:

```markdown
# Lattice-boltzmann — notes

## Known limitations (v1)
- F9 reloads moments only; non-equilibrium f-state is lost. Re-equilibrates within a few substeps. See spec § 4.B.13.
- 2.5D airfoil (extruded along Z). Finite-span 3D wings deferred.
- VDB export of velocity field is half-to-float-converted on host (~10 LOC); banked as common-cpp helper candidate when consumer #2 surfaces.

## v1.1 polish backlog
- AA-pattern or EsoTwist in-place streaming (halves f-buffer memory).
- Fused stream-collide kernel (~5-10% perf at the cost of modularity).
- Krüger factored equilibrium form (~5% perf, FMA chain).
- Index-buffer primitive-restart for streamlines (cleaner GPU profile than NaN-vertex).
- MRT collision (stability at high Re).
- Cubic-Hermite obstacle SDF (smoother halfway-BB convergence).
- --capture-full-state flag (captures f-state at 12 GB/F5 at capture tier).

## Frontier variants (separate phase)
- 16-bit moment-encoded LBM (Chen 2025); 50% memory cut.
- Differentiable LBM (XLB / OpenLB style).
- GPU-AMR LBM (Jaber 2025).

## Tier downgrade pattern
v1 supports tier change after window.show via deferred re-allocation. Visual flash at change is expected and acceptable.
```

### § 4.O — `volumetric-grid/lattice-boltzmann/README.md`

Replaces the existing stub. Convention: short user-facing description, controls list, screenshot placeholder.

```markdown
# Lattice Boltzmann (Phase 12)

D3Q19 BGK lattice Boltzmann method around a NACA airfoil. Free-fly camera, live streamlines, velocity-magnitude volume raymarch. RX 6800 XT desktop target.

## Run

```
cmake --build build --target lattice_boltzmann
./build/bin/lattice_boltzmann
```

## Controls

| Key/Mouse        | Action                             |
|------------------|------------------------------------|
| WASD             | Move camera                        |
| RMB-drag         | Look                               |
| Q / E            | World-up / world-down              |
| Shift            | Boost speed                        |
| F5               | Save capture                       |
| F9               | Load latest capture                |
| Panel: Preset    | Switch airfoil and Reynolds number |
| Panel: Tier      | Switch grid resolution             |

## Presets

- **NACA0012 — Low-Re**: laminar attached flow, Re ≈ 80.
- **NACA0012 — Med-Re**: vortex-shedding onset, Re ≈ 230.
- **NACA4412 — Med-Re**: cambered airfoil, asymmetric wake, Re ≈ 230.

## Tiers

- 128³ (Laptop): ~700 MB
- 256×128×128 (Desktop, default): ~1.4 GB
- 512×256×256 (Capture): ~5.2 GB; interactive ≤10 FPS

## References

- Krüger et al. 2017, *The Lattice Boltzmann Method: Principles and Practice* (companion code vendored at `references/lbm-principles-practice/`)
- Zou & He 1997, *Phys. Fluids* 9, 1591 — inlet/outlet boundary conditions
- D3Q19 lattice constants derivation: `tools/integrity/docs/algebraic/d3q19.md`
```

---

## § 5 — Cross-cutting modifications

These edits land AFTER all sim-local files are in place AND after Checkpoint 5 (build green) passes. Each edit is anchor-string-grep-verified before write per § 0.2 hard rule 5.

### § 5.A — `.github/workflows/build-native.yml`

Add `volumetric-grid/lattice-boltzmann/**` to the path triggers in both jobs. The current file structure (per probe-1 § E): two jobs, `build-release` and `build-debug`, each with a `paths:` block listing per-sim directories.

**Anchor 1** (in `build-release` job):

```yaml
      - 'volumetric-grid/eulerian-smoke/**'
```

**Insert after** (under the same `paths:` block, before the closing bracket):

```yaml
      - 'volumetric-grid/lattice-boltzmann/**'
```

**Anchor 2** (in `build-debug` job, same pattern):

```yaml
      - 'volumetric-grid/eulerian-smoke/**'
```

**Insert after**:

```yaml
      - 'volumetric-grid/lattice-boltzmann/**'
```

**Debug job OpenVDB note**: per Phase 11 precedent, the debug job omits OpenVDB (`-DGPU_SIMS_USE_OPENVDB=OFF` is the default). This is intentional — debug builds are slow enough; OpenVDB linking time isn't worth it. Phase 12's optional VDB export is gated at runtime via `vdb::isAvailable()`, so the debug binary launches and runs, the VDB toggle just becomes a no-op. No change to the `cmake` invocation lines in either job.

### § 5.B — `CMakeLists.txt` (root)

Probe-1 § A confirmed the root `CMakeLists.txt` uses `add_subdirectory(volumetric-grid/lattice-boltzmann)` via wildcard glob. Verify the glob covers Phase 12's new directory. If yes, NO EDIT needed. If the glob is per-directory enumerated, append `add_subdirectory(volumetric-grid/lattice-boltzmann)` after the eulerian-smoke entry.

**Verification grep** (Claude Code runs before deciding to edit):

```
grep -n "lattice-boltzmann\|eulerian-smoke" CMakeLists.txt
```

If output includes `add_subdirectory(volumetric-grid/lattice-boltzmann)` — done. If only `add_subdirectory(volumetric-grid/eulerian-smoke)` is present, add the LBM line immediately after with matching indentation.

### § 5.C — `docs/sim-specs/lattice-boltzmann.md`

Replace stub with full sim-spec following the template at `docs/overarching-spec.md` § 7. The template covers: goal, scale tier table, presets, controls, capture format, render approach, references.

Full replacement content:

```markdown
# lattice-boltzmann sim spec

> Phase 12. Stack C. Tier defaults: 256×128×128 desktop.

## Goal

Demonstrate incompressible-ish fluid flow around a NACA airfoil using the D3Q19 BGK lattice Boltzmann method. Headline visualization: live streamlines + velocity-magnitude volume raymarch. Educational target: vortex-shedding regime around a low-Reynolds-number airfoil.

## Scale tiers

| Tier label                  | Nx × Ny × Nz | f-state memory | Streamlines | Notes              |
|-----------------------------|--------------|---------------|-------------|--------------------|
| 128³ (Laptop)               | 128×128×128  | ~150 MB       | 10k         | iGPU / laptop      |
| 256×128×128 (Desktop)       | 256×128×128  | ~610 MB       | 10k         | RX 6800 XT default |
| 512×256×256 (Capture)       | 512×256×256  | ~4.8 GB       | 10k         | Hero render only   |

## Presets

| Preset                  | Airfoil   | α (deg) | u_∞    | τ      | Re (default tier) |
|-------------------------|-----------|---------|--------|--------|-------------------|
| NACA0012 — Low-Re       | NACA0012  | 4.0     | 0.04   | 0.60   | ~80               |
| NACA0012 — Med-Re       | NACA0012  | 8.0     | 0.06   | 0.55   | ~230              |
| NACA4412 — Med-Re       | NACA4412  | 6.0     | 0.06   | 0.55   | ~230              |

## Controls

- WASD: camera. RMB-drag: look. Q/E: world-up/down. Shift: boost.
- F5: save capture. F9: load latest.
- Panel: presets, tiers, solver (τ, substeps), flow (|u_∞|, AoA), render (toggles, exposure), streamlines (count, history), capture, camera, stats.

## Capture format

Top-level key: `latticeBoltzmann`. Buffers: `density` (r32f), `velocity` (rgba16f), `obstacle_mask` (r8uint, `frame_invariant: true`).

f-state distribution functions NOT captured (size; recomputed from moments on F9).

## Render

- Volume raymarch of velocity-magnitude with colormap LUT. ~64–256 steps depending on tier.
- GPU-seeded streamlines (~10k seeds × 64 ring buffer history) RK2-advected per render frame.

## References

- `references/lbm-principles-practice/` ([Krueger] registry entry; MIT)
- `tools/integrity/docs/algebraic/d3q19.md` ([Algebraic_D3Q19] registry entry)
- Zou & He 1997, *Phys. Fluids* 9, 1591 (Zou-He boundary conditions)
- Hecht & Harting 2010, arXiv:0811.4593 (3D Zou-He generalization)
- NACA Report 460, Jacobs/Ward/Pinkerton 1933 (NACA 4-digit airfoil equations)
```

### § 5.D — `README.md` (repo root)

Add a gallery row for LBM. Anchor: the row above sph-water in the sim gallery table.

**Anchor**:

```markdown
| sph-water        | Stack C  | DFSPH around obstacles |
```

**Insert immediately after**:

```markdown
| lattice-boltzmann | Stack C  | D3Q19 BGK around NACA airfoil |
```

Plus a one-line bullet in the "What's here" section if such a section exists; probe-1 § A didn't catalog the README's narrative-text sections in detail. Claude Code greps for "sph-water" in README.md and adds a parallel one-line description nearby.

### § 5.E — `CHANGELOG.md`

Prepend a Phase 12 entry. Format follows the Phase 11 precedent.

**Anchor** (top of the file, just below the title):

```markdown
## Phase 11.x — sph-water DFSPH α-factor rewrite (in flight)
```

**Insert before** (i.e., at the very top, becoming the new top entry):

```markdown
## Phase 12 — `volumetric-grid/lattice-boltzmann/` — D3Q19 BGK around a NACA airfoil

- New Stack C sim: D3Q19 single-relaxation-time lattice Boltzmann method, ~610 MB f-state at desktop tier.
- Free-fly camera, 3 presets (NACA0012 Low-Re / Med-Re; NACA4412 Med-Re), 3 tiers.
- GPU-seeded streamlines (~10k seeds × 64 history) RK2-advected per render frame.
- Sim-local velocity-magnitude volume raymarch (does NOT promote ES's `raymarch.frag.glsl` — that promotion is banked for consumer #3 of volume raymarch).
- Optional OpenVDB velocity-field export (gated at compile + runtime, per the Phase 11 pattern).
- Capture format: `latticeBoltzmann` namespace; new `frame_invariant` meta key for the obstacle mask.
- Reference anchors: `[Krueger]` (MIT, math patterns) + `[Algebraic_D3Q19]` (derivation, lattice constants).
- Integrity toolkit: new `cat3.d3q19-velocity-set`, `cat3.d3q19-weights`, `cat3.d3q19-equilibrium` checks consume the algebraic derivation.

### Setup commits landed first

- `8fe355b` — vendored `lbm-principles-practice` at SHA `6e2c592f` (Krüger book code, MIT).
- `0db9c73` — `tools/integrity/docs/algebraic/d3q19.md` + Python verification harness + JSON expected values + `[Algebraic_D3Q19]` registry entry. Also fixed `load_registry` to skip entries without `vendor_root`.
```

### § 5.F — `project-state.md`

Multiple edits. Probe-1 § B catalogued the structure: § 3 phase ledger (table), § 6 sim-list (table), § 11 latest-commit pointer (single line), § 7 H3-conventions section, § 4 locked decisions.

**Edit 5.F.1 — § 3 phase ledger row.**

Anchor (last row of the table):

```markdown
| 11.x | sph-water 2.16.1 swap + α-factor rewrite | in flight | ... |
```

Insert after:

```markdown
| 12   | lattice-boltzmann (D3Q19 BGK + NACA airfoil) | landed | <PHASE_12_SHA> |
```

The `<PHASE_12_SHA>` placeholder is filled by the SHA-backfill follow-up commit (§ 7.B).

**Edit 5.F.2 — § 6 sim-list row.**

Anchor:

```markdown
| sph-water            | Stack C / Vulkan      | Phase 11   |
```

Insert after:

```markdown
| lattice-boltzmann    | Stack C / Vulkan      | Phase 12   |
```

**Edit 5.F.3 — § 11 latest-commit pointer.**

Anchor (the entire single line that holds the latest commit SHA pointer):

```markdown
**Latest substantive commit at last `project-state.md` update:** `0db9c73` (Phase 12 setup-2)
```

Replace with:

```markdown
**Latest substantive commit at last `project-state.md` update:** `<PHASE_12_SHA>` (Phase 12 lattice-boltzmann landing)
```

**Edit 5.F.4 — § 7 H3 for frame-invariant capture convention** (NEW).

Anchor (existing H3 entries):

```markdown
### H3 — Capture format: `*.bin` extension stripping
```

Insert as new H3 after the most recent existing H3 entry:

```markdown
### H3 — Frame-invariant capture meta key

Buffers whose value doesn't change per-frame within a session (obstacle masks, scene SDFs, immutable presets) carry `"frame_invariant": true` in their `saveBuffer` meta. The reader-side contract: F9-load may skip uploading the buffer if a current-session equivalent matches by hash. First consumer: Phase 12's `obstacle_mask`. Promotion gate: this single consumer enters the convention immediately because no precedent exists for the pattern — once consumer #2 surfaces, refine the hash-equivalence contract per the rule-of-three.
```

### § 5.G — `docs/tier1-capture-format-reference.md`

Two sub-edits.

**Edit 5.G.1 — § 1 add latticeBoltzmann row.**

Anchor (last row of the per-sim capture key table):

```markdown
| sph-water        | `sphWater`         | particles, density, kernel-radius |
```

Insert after:

```markdown
| lattice-boltzmann | `latticeBoltzmann` | density, velocity, obstacle_mask (frame-invariant) |
```

**Edit 5.G.2 — § 2 add Phase 12 saveBuffer precedent block.**

Anchor (existing precedent blocks):

```markdown
### Phase 11 — sph-water precedent
```

Insert before (Phase 12 above Phase 11 chronologically? No — chronologically Phase 12 follows Phase 11. Insert AFTER):

Actually, the cleaner placement is AFTER the most recent precedent block. Re-grep — the most recent block is Phase 11's. Insert immediately after the closing of that block:

```markdown
### Phase 12 — lattice-boltzmann precedent

`density` (r32f), `velocity` (rgba16f), `obstacle_mask` (r8uint). All meta carries `count`, `stride`, `format`, `shape`. `obstacle_mask` additionally carries `"frame_invariant": true` — see project-state.md § 7 H3 for the convention statement. Distribution functions are NOT captured (re-derived from moments via equilibrium at F9 load).

Spec: `docs/phase12_lattice_boltzmann.md` § 4.B.13.
```

**Banked but NOT done in this commit**: Phase 9 (MPM) and Phase 10 (reaction-diffusion) rows in § 1. Their absence was flagged in probe-1 § B; the backfill is a separate cleanup commit per Phase 11 retro item 1.

---

## § 6 — Verification block

Every check in this section must pass before the substantive commit lands. Checks are listed in execution order.

### § 6.A — grep verifications

```
# 1. lattice_constants.glsl encodes the 19 directions correctly.
grep -c "ivec3(" volumetric-grid/lattice-boltzmann/shaders/lattice_constants.glsl
# Expected: 19 (one per direction in the C_I[] declaration).

# 2. lattice_constants.glsl includes the three weight values.
grep -c "1.0/3.0\|1.0/18.0\|1.0/36.0" volumetric-grid/lattice-boltzmann/shaders/lattice_constants.glsl
# Expected: ≥ 19 (1 rest + 6 face + 12 edge; some lines may carry multiple).

# 3. Every shader that uses lattice constants #includes the header.
grep -l "lattice_constants.glsl" volumetric-grid/lattice-boltzmann/shaders/*.glsl
# Expected output (5 files):
#   init_equilibrium.comp.glsl
#   collide.comp.glsl
#   stream.comp.glsl
#   apply_boundaries.comp.glsl
#   compute_moments.comp.glsl

# 4. No .bin extension regression in capture saves.
grep -n 'saveBuffer.*\.bin' volumetric-grid/lattice-boltzmann/src/main.cpp
# Expected: no matches.

# 5. frame_invariant key used for obstacle_mask only.
grep -n 'frame_invariant' volumetric-grid/lattice-boltzmann/src/main.cpp
# Expected: exactly one match (in capture_save for obstacle_mask).

# 6. CI yaml has LBM path triggers in both jobs.
grep -c "volumetric-grid/lattice-boltzmann/\*\*" .github/workflows/build-native.yml
# Expected: 2 (release job + debug job).

# 7. project-state.md latest-commit pointer references the placeholder pre-backfill.
grep -n "<PHASE_12_SHA>" project-state.md
# Expected: 2 matches (§ 3 ledger row + § 11 pointer).

# 8. No reference to Phase 11.x ledger or backfill commitments accidentally promoted.
grep -n "Phase 11\.x" CHANGELOG.md
# Expected: still present (the Phase 11.x in-flight entry is still in the ledger;
# Phase 12 lands above it).

# 9. CHANGELOG mentions both setup SHAs.
grep -n "8fe355b\|0db9c73" CHANGELOG.md
# Expected: 2 matches (one each for setup-1 and setup-2).

# 10. Krüger and Algebraic_D3Q19 references in registry are intact.
grep -A 2 "^\[Krueger\]\|^\[Algebraic_D3Q19\]" tools/integrity/docs/ground-truth-sources.md
# Expected: both stanzas, both with the fields specified in setup-1 and setup-2.
```

### § 6.B — build gates

```
# Release.
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DGPU_SIMS_BUILD_EXAMPLES=ON -DGPU_SIMS_USE_OPENVDB=ON -DGPU_SIMS_USE_ALEMBIC=ON
cmake --build build-release --target lattice_boltzmann --parallel
# Expected: PASS, no warnings.

# Debug.
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DGPU_SIMS_BUILD_EXAMPLES=ON -DGPU_SIMS_USE_ALEMBIC=ON
cmake --build build-debug --target lattice_boltzmann --parallel
# Expected: PASS, no warnings.
```

If either build emits warnings, treat as fail and report. The repo has been warning-clean since Phase 7; regressions surface here.

### § 6.C — integrity toolkit gates

```
cd tools/integrity
python -m integrity --check cat1.upstream-anchor
# Expected: 1 pass (the [Krueger] anchor).

python -m integrity --check cat1.upstream-citation
# Expected: passes for the shader doc-blocks that cite Krüger chapter13 + chapter5.

# cat3 numerical checks (these were added by setup-2; verify they still pass):
python -m integrity --check cat3.d3q19-velocity-set
# Expected: 1 pass (the shader's C_I[19] matches d3q19.md).

python -m integrity --check cat3.d3q19-weights
# Expected: 1 pass (the shader's W_I[19] matches d3q19.md).

python -m integrity --check cat3.d3q19-equilibrium
# Expected: 1 pass (the shader's feq() evaluated at canonical test points matches
# the expected.json from setup-2).
```

### § 6.D — runtime smoke (user-runtime by Steven on the RX 6800 XT)

Claude Code reports Checkpoint 5 green. Steven runs:

```
./build-release/bin/lattice_boltzmann
```

Expected behavior:
- Window opens at 1920×1080.
- Default preset (NACA0012 Low-Re) loads. Airfoil visible in volume raymarch as a darker silhouette in the wind tunnel.
- ~10k streamlines visible, flowing left-to-right past the airfoil.
- FPS ≥ 60 at desktop tier on RX 6800 XT.
- Tier change to 128³: re-allocates without crash, FPS ≥ 120 at laptop tier.
- Tier change to 512×256×256: re-allocates without crash, FPS 5–10.
- F5 saves capture to `captures/capture_NNNN/`. F9 reloads. Visual state matches pre-F5.
- Preset cycle through 3 presets: each loads cleanly, visual character distinguishable.
- VDB export toggle (Release build): emits `captures/velocity_NNNN.vdb` every N frames.

Defects reported back are sliced into:
- **In-flight fix authorized**: trivial shader-code or main.cpp bug. Fix and amend? No — fix as a follow-up commit per Convention #12.
- **Phase 12.5 polish**: visual quality issues that don't affect correctness.
- **Bank for v1.1**: anything in the v1.1 backlog from `docs/notes.md`.

### § 6.E — CI green gate

Push the branch. Both `build-native` jobs (Release + Debug) must go green before merge. The new `volumetric-grid/lattice-boltzmann/**` path trigger should make both jobs run on this commit (and on any future LBM-touching commits).

---

## § 7 — Commit message + retro plan

### § 7.A — Substantive commit

Branch: `phase-12-lattice-boltzmann`. Single substantive commit with the sim-local files (everything in § 3.A).

**Commit message** (~25 lines):

```
feat(lattice-boltzmann): D3Q19 BGK around a NACA airfoil — Phase 12 substantive

Second Stack C volumetric-grid sim. Implements the D3Q19 single-relaxation-time
lattice Boltzmann method with three presets (NACA0012-LowRe, NACA0012-MedRe,
NACA4412-MedRe), three resolution tiers (128³ laptop, 256×128×128 desktop,
512×256×256 capture), Zou-He inlet/outlet boundaries, free-slip side walls,
halfway bounce-back at the airfoil, GPU-seeded streamlines, and a sim-local
velocity-magnitude volume raymarch.

Files (sim-local):
- volumetric-grid/lattice-boltzmann/CMakeLists.txt
- volumetric-grid/lattice-boltzmann/src/main.cpp
- volumetric-grid/lattice-boltzmann/shaders/{lattice_constants, init_equilibrium,
  collide, stream, apply_boundaries, compute_moments, streamline_advect, velmag,
  streamline.vert, streamline.frag, fullscreen.vert}.glsl
- volumetric-grid/lattice-boltzmann/docs/{load-bearing-decisions, notes}.md
- volumetric-grid/lattice-boltzmann/README.md

Load-bearing decisions: 13 per docs/phase12_lattice_boltzmann.md § 2.
References:
- Krüger 2017 (MIT, vendored at references/lbm-principles-practice/ in 8fe355b).
- Algebraic D3Q19 derivation at tools/integrity/docs/algebraic/d3q19.md (0db9c73).
- Zou & He 1997 (boundary conditions; not vendored, cited in apply_boundaries.glsl
  and docs/sim-specs/lattice-boltzmann.md).

Cross-cutting modifications land in a separate follow-up commit per the Phase 11
posture (CI path triggers, README gallery row, CHANGELOG, project-state.md, sim-
specs entry, tier1-capture-format-reference.md).

SHA back-fill of project-state.md latest-commit pointer is a separate follow-up
commit per Convention #12.

Verification at land time:
- All 10 grep checks (§ 6.A) pass.
- Release + Debug builds green.
- cat1.upstream-anchor + cat1.upstream-citation pass.
- cat3.d3q19-velocity-set + cat3.d3q19-weights + cat3.d3q19-equilibrium pass.
- RX 6800 XT smoke test (§ 6.D) by Steven: green (or specific defects banked).

Setup commits prerequisite:
- 8fe355b (vendor lbm-principles-practice MIT)
- 0db9c73 (algebraic D3Q19 derivation + verification harness)
```

### § 7.B — Follow-up commits

**Commit 7.B.1 — cross-cutting edits** (corresponds to § 5):

```
chore(phase12): cross-cutting edits — CI + README + CHANGELOG + project-state + capture-format

Lands the § 5 cross-cutting modifications after the substantive lattice-boltzmann
landing. CI path triggers for the new sim, README gallery row, CHANGELOG Phase 12
entry, project-state.md ledger + sim-list + latest-commit-pointer (placeholder for
SHA backfill), sim-specs entry, tier1-capture-format-reference.md latticeBoltzmann
row + Phase 12 saveBuffer precedent + introduction of the frame_invariant convention
to project-state.md § 7 H3.

Follows the Phase 11 split-commit posture (sim-local first, cross-cutting second,
SHA backfill third).

Refs:
- docs/phase12_lattice_boltzmann.md § 5
- docs/diagnostics/_audits/phase12_xcommit_landing_2026-05-15.md
```

**Commit 7.B.2 — SHA backfill** (corresponds to Convention #12):

```
chore(phase12): backfill substantive-commit SHA into project-state.md

Replaces the <PHASE_12_SHA> placeholders in project-state.md § 3 ledger row and
§ 11 latest-commit pointer with the actual SHA of the Phase 12 substantive commit.

Refs Convention #12 (SHA back-fill always a separate follow-up commit, never amend).
```

### § 7.C — Retro plan

Banked items for the post-phase retro at `docs/retro/phase12.md`:

1. **Convention #8 firing on architect-1**: the Krüger-as-anchor claim was falsified by probe-2. Banked shape: "an anchor candidate's dimensional scope (2D vs 3D), its compressibility regime, and its direction-ordering convention are themselves load-bearing facts to verify pre-anchor-lock — never assert from the languages bar." Same root cause as the SPlisHSPlasH 1.8.10 fabrication from Phase 11.5, extended laterally to "scope" not just "version."

2. **`load_registry` defect surfaced by `[Algebraic_D3Q19]` at consumer #2 of the registry shape**: the loader required `anchor_version` on every entry, but the algebraic-entry shape (which has no version) was already documented in `ground-truth-sources.md`. Spec-vs-implementation drift caught by adding the second consumer. Convention candidate: "registry-pattern consumers reveal spec/loader drift at consumer #2." Fix was a one-line skip, but the pattern is worth banking.

3. **Floating-point came out exact**: the algebraic verification harness's mass and momentum conservation sums printed `1.000000000000000` and `0.100000000000000` to 15 digits despite the test values being decimal-but-not-binary-exact. Worth a note that for D3Q19 LBM the BGK relaxation in single-precision is numerically well-behaved; the cat3 tolerance choices (`1e-6 relative / 1e-9 absolute`) have substantial margin to spare.

4. **Architect-2 absence — was Phase 12 OK without it?**: the spec was author-self-reviewed via web search + arithmetic re-derivation. Compare correctness rate against Phase 11 (which had architect-2 + multi-round review): Phase 12 had no defects caught at execution that pre-existed in the spec, except for X, Y, Z (to be filled in post-execution). Determine whether single-architect with stricter discipline scales, or whether architect-2 was load-bearing in ways the absence reveals.

5. **Promotion candidates left at consumer #2**: (a) `gpusims::half_to_float` helper (LBM is consumer #1; banked for consumer #2). (b) Streamline rendering pipeline (sim-local at Phase 12; banked for a future flow-vis sim). (c) NACA / SDF voxelization (sim-local; banked for the next sim with an obstacle mask).

6. **Promotion candidate at consumer #3 (next time)**: ES's `raymarch.frag.glsl` — Phase 12 explicitly did NOT promote it. The next volumetric-grid sim that needs a scalar-field raymarch (Phase 13? 14?) is consumer #3 and triggers the rule-of-three. At that point the unit-cube hardcoding and the physics-vs-LUT split both get fixed in a single promotion commit.

7. **Banked deferred backfills NOT done in Phase 12**: tier1-capture-format-reference.md § 1 still missing Phase 9 + Phase 10 rows. Per Phase 11 retro item 1, this is a separate cleanup commit. Bank for Phase 12.x or 13's first weekend.

8. **In-flight authorization for `writeVec3Grid` signature extension**: Phase 12 is consumer #2 of `writeVec3Grid`. Decision at execution time: extend with `VkGridClass` param OR leave GRID_STAGGERED in place. Whichever Claude Code picked, document in the retro. If extended, that's a banked common-cpp change worth a separate commit; if left, that's banked v1.x cleanup.

---

## Spec end.

> **Length:** ~3000 lines. **Substantive commit SHA at first lock:** to be filled post-execution (`<PHASE_12_SHA>`). **Two prep-commit SHAs already in:** `8fe355b` (Krüger), `0db9c73` (Algebraic_D3Q19 + verification harness).
>
> Execution: Claude Code reads top-to-bottom. Generation order per § 3.D. Six checkpoints in § 0.5 fire at the listed points. § 6 verification gates before commit. § 7 commit messages + retro plan.
>
> Questions or genuine ambiguities → pause, report, do not guess.
