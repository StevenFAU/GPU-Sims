# Phase 12 — § 5 reanchor patch

> **Date:** 2026-05-15
> **Author:** architect-1
> **Companion:** `docs/diagnostics/_audits/phase12_reanchor_handoff_2026-05-15.md` (Claude Code handoff bundle)
> **Replaces:** § 5 of `docs/phase12_lattice_boltzmann.md` wholesale.
>
> **Reanchor reason:** the original § 5 was written against an assumed-future repo state (Phase 11.x in flight, per-sim CI path triggers, 3-column tier1 reference rows, etc). Nine of the ten anchors failed grep against synced HEAD `c5955d3`; only § 5.F.4 verified clean. This patch rewrites § 5 against the actual synced state surfaced by Claude Code's handoff bundle.
>
> **Bank for Phase 12 retro:** five `cat1.intra-repo` fabrications in § 5 anchor strings, all caught by the spec's own § 0.2 hard rule 5 at Checkpoint 5. Same Convention #8 root cause as the Krüger-D3Q19 anchor, the common-cpp API surface, the Zou-He closure equations, the `writeVec3Frame` API name, and the `docs/phase11_sph_water.md` location — six firings total in Phase 12. The 6-checkpoint protocol caught all six before any reached production code. Worth a retro item on its own.

---

## § 5 — Cross-cutting modifications (REANCHORED)

These edits land AFTER all sim-local files are in place AND after Checkpoint 5 (build green) passes. Each edit anchors against synced HEAD `c5955d3` text verbatim. Post-edit verification: grep for the new content; expected match counts listed.

### § 5.A — `.github/workflows/build-native.yml`

**Decision:** wildcard `volumetric-grid/**` (Option 3 per architect-1 callback). Most robust against the "forgotten trigger line on the next sim" failure mode within the volumetric-grid category. Banked for separate v1.x cleanup commit: extend with `particle-fluids/**` and `continuous-ca/**` to bring all sim categories under CI watch — that's not Phase 12's responsibility.

**Anchor** (identical block in both `build-ubuntu` and `build-ubuntu-debug` jobs, in both `push:` and `pull_request:` sections — **four occurrences total**):

```yaml
    paths:
      - 'CMakeLists.txt'
      - 'common/common-cpp/**'
      - '.github/workflows/build-native.yml'
```

**Replace with**:

```yaml
    paths:
      - 'CMakeLists.txt'
      - 'common/common-cpp/**'
      - 'volumetric-grid/**'
      - '.github/workflows/build-native.yml'
```

**Verification grep** (post-edit):

```
grep -c "volumetric-grid/\*\*" .github/workflows/build-native.yml
# Expected: 4 (two jobs × push/pull_request)
```

### § 5.B — root `CMakeLists.txt`

**Anchor** (verbatim):

```cmake
# if(GPU_SIMS_BUILD_VOLUMETRIC_GRID)
#     add_subdirectory(volumetric-grid/lattice-boltzmann)
# endif()
```

**Replace with**:

```cmake
add_subdirectory(volumetric-grid/lattice-boltzmann)
```

(Drops the commented-out three-line block — including the if-gate, which isn't used by ES or sph-water — and replaces with a single unconditional `add_subdirectory` line matching the eulerian-smoke / sph-water posture above.)

**Note for Claude Code:** the report from Checkpoint 5 indicates this edit was already applied opportunistically to enable the build gate. Confirm the line is present and the commented block is absent; if so, no edit needed and this subsection becomes a no-op.

**Verification grep**:

```
grep -n "add_subdirectory(volumetric-grid/lattice-boltzmann)" CMakeLists.txt
# Expected: exactly 1 match, uncommented.

grep -n "# add_subdirectory(volumetric-grid/lattice-boltzmann)" CMakeLists.txt
# Expected: 0 matches (the commented block is removed).
```

### § 5.C — `docs/sim-specs/lattice-boltzmann.md`

(Unchanged from original spec. The sim-brief at `docs/sim-specs/lattice-boltzmann.md` is replaced wholesale with the content given in original spec § 5.C. The handoff bundle confirms this file exists as a stub.)

### § 5.D — root `README.md` gallery

**Anchor** (verbatim, in-row edit):

```markdown
| [Lattice Boltzmann](volumetric-grid/lattice-boltzmann/) | Volumetric grid | Native C++ | Not started |
```

**Replace with**:

```markdown
| [Lattice Boltzmann](volumetric-grid/lattice-boltzmann/) | Volumetric grid | Native C++ | Implemented (Phase 12) |
```

**Verification grep**:

```
grep -c "Lattice Boltzmann.*Not started" README.md
# Expected: 0.

grep -c "Lattice Boltzmann.*Implemented (Phase 12)" README.md
# Expected: 1.
```

### § 5.E — `CHANGELOG.md`

Insert a new `[0.13.0]` block between `## [Unreleased]` and `## [0.12.0]`.

**Anchor** (verbatim):

```markdown
## [Unreleased]


## [0.12.0] - Phase 11: sph-water (Stack C particle-fluids flagship; first Alembic real-impl consumer)
```

**Replace with**:

```markdown
## [Unreleased]


## [0.13.0] - Phase 12: lattice-boltzmann (Stack C volumetric-grid; D3Q19 BGK around a NACA airfoil)

### Added
- `volumetric-grid/lattice-boltzmann/` — second Stack C volumetric-grid sim. D3Q19 single-relaxation-time lattice Boltzmann method, ~610 MB f-state at the desktop tier (256×128×128). Three presets (NACA0012-LowRe, NACA0012-MedRe, NACA4412-MedRe), three resolution tiers (128³ laptop, 256×128×128 desktop, 512×256×256 capture).
- GPU-seeded streamlines (~10k seeds × 64-position ring-buffer history) RK2-advected per render frame.
- Sim-local velocity-magnitude volume raymarch (does NOT promote ES's `raymarch.frag.glsl` — that promotion is banked for consumer #3 of volume raymarch).
- Optional OpenVDB velocity-field export via `gpusims::vdb::writeVec3Grid` (first real consumer post-Phase 8; gated at compile + runtime).
- Capture format: `latticeBoltzmann` top-level meta key; new `frame_invariant` meta-field convention for the obstacle mask.
- Integrity toolkit: new `[Algebraic_D3Q19]` registry pattern (algebraic-derivation ground-truth, no vendored upstream) at `tools/integrity/docs/algebraic/d3q19.md` with independent Python verification harness.
- Reference: `[Krueger]` registry entry (`references/lbm-principles-practice/` at SHA `6e2c592f`, MIT) for BGK + halfway-bounce-back math patterns.

### Notes
- Boundary handling at v1: equilibrium-distribution inlet/outlet (first-order); free-slip ±Y/±Z side walls; halfway bounce-back at the airfoil. Zou-He second-order inlet/outlet banked for v1.1 with derivation doc + verification harness at `tools/integrity/docs/algebraic/zou_he_d3q19.md` (to be created).
- Six Convention #8 architect-fabrications were caught by the 6-checkpoint protocol during Phase 12 execution before any reached production code; banked as retro item 9. The single-architect-plus-checkpoints model is functioning but the checkpoints are load-bearing.

### Prep commits landed before substantive Phase 12
- `8fe355b` — vendor `lbm-principles-practice` MIT reference at SHA `6e2c592f`.
- `0db9c73` — `tools/integrity/docs/algebraic/d3q19.md` + Python verification harness + `d3q19_equilibrium.expected.json` + `[Algebraic_D3Q19]` registry entry. Includes a `load_registry` fix to skip entries without `vendor_root` (the algebraic registry pattern was documented but not loader-supported until this commit; surfaced as registry-consumer-#2 spec/loader drift).
- `c5955d3` — architect-1 spec at `docs/phase12_lattice_boltzmann.md`.


## [0.12.0] - Phase 11: sph-water (Stack C particle-fluids flagship; first Alembic real-impl consumer)
```

**Verification grep**:

```
grep -c "^## \[0.13.0\]" CHANGELOG.md
# Expected: 1.

grep -n "\[Algebraic_D3Q19\]" CHANGELOG.md
# Expected: at least 1 match in the new block.
```

### § 5.F — `project-state.md`

Four sub-edits.

#### § 5.F.1 — § 3 phase ledger row

Insert a Phase 12 row between the Phase 11 row and the `10+ — Remaining sims` aggregate row.

**Anchor** (the two-row block, verbatim from the handoff bundle):

```
| 11 | sph-water (Stack C particle-fluids flagship; DFSPH; first Alembic consumer) | DFSPH inner-iter Jacobi loops; Morton spatial hash; screen-space fluid render; subgroup-size pinning; FetchContent for Alembic 1.8.10. Scaffold shipped at ``09c0d9f``; main.cpp + § 5 cross-cutting edits land in follow-up commits per `phase11_deferred_backfill.md`. SHA-backfill follow-up tidies this row and § 11 below once Phase 11's third commit resolves. | ✅ Shipped | ``09c0d9f`` |
| 10+ | Remaining sims | One phase per remaining sim. Each consumes a settled `common-` package; per-sim phases are smaller than the foundation phases. Per Phase 9 banking, the natural Alembic-real-impl consumer is sph-water (Stack C). The natural cross-stack lenia-fft consumer (Stack D Taichi + Stack B WebGPU) is post-MPM. | ⬜ Not started | — |
```

**Replace with** (Phase 12 row inserted between):

```
| 11 | sph-water (Stack C particle-fluids flagship; DFSPH; first Alembic consumer) | DFSPH inner-iter Jacobi loops; Morton spatial hash; screen-space fluid render; subgroup-size pinning; FetchContent for Alembic 1.8.10. Scaffold shipped at ``09c0d9f``; main.cpp + § 5 cross-cutting edits land in follow-up commits per `phase11_deferred_backfill.md`. SHA-backfill follow-up tidies this row and § 11 below once Phase 11's third commit resolves. | ✅ Shipped | ``09c0d9f`` |
| 12 | lattice-boltzmann (Stack C volumetric-grid; D3Q19 BGK around a NACA airfoil) | Second Stack C volumetric-grid sim. D3Q19 single-relaxation-time LBM, 19+19 r32f 3D-image ping-pong, GPU-seeded RK2-advected streamlines, sim-local velocity-magnitude raymarch. First sim consuming algebraic-derivation ground-truth (`[Algebraic_D3Q19]` registry pattern via `tools/integrity/docs/algebraic/d3q19.md`); first real consumer of `gpusims::vdb::writeVec3Grid` post-Phase 8. Boundary v1: equilibrium-distribution inlet/outlet + halfway-BB airfoil; Zou-He banked for v1.1. Three prep commits (``8fe355b``, ``0db9c73``, ``c5955d3``) landed pre-substantive. | ✅ Shipped | ``<PHASE_12_SHA>`` |
| 10+ | Remaining sims | One phase per remaining sim. Each consumes a settled `common-` package; per-sim phases are smaller than the foundation phases. Per Phase 9 banking, the natural Alembic-real-impl consumer is sph-water (Stack C). The natural cross-stack lenia-fft consumer (Stack D Taichi + Stack B WebGPU) is post-MPM. | ⬜ Not started | — |
```

(The `<PHASE_12_SHA>` placeholder is filled by the SHA-backfill follow-up commit per Convention #12.)

**Verification grep**:

```
grep -c "^| 12 | lattice-boltzmann" project-state.md
# Expected: 1.

grep -c "<PHASE_12_SHA>" project-state.md
# Expected: at least 1 (this row; § 5.F.3 below may add another).
```

#### § 5.F.2 — § 6 sim-list row

Edit-in-place on the existing stub row. The synced row has 4 columns; the spec's original anchor assumed 3 columns and is invalid.

**Anchor** (verbatim):

```
| `volumetric-grid/lattice-boltzmann/` | lattice-boltzmann | C | Sim-spec stub |
```

**Replace with**:

```
| `volumetric-grid/lattice-boltzmann/` | lattice-boltzmann | C | **Implemented (Phase 12)** — second Stack C volumetric-grid sim; D3Q19 BGK around a NACA airfoil; first real consumer of `gpusims::vdb::writeVec3Grid` (real impl post-Phase 8 `writeFloatGrid`); first sim consuming algebraic-derivation ground-truth via `tools/integrity/docs/algebraic/d3q19.md` ([Algebraic_D3Q19] registry pattern). Equilibrium-distribution inlet/outlet boundary at v1; Zou-He 3D banked for v1.1 with derivation doc + verification harness. |
```

**Note on `writeVec3Grid` vs `writeVec3Frame`:** the original spec § 4.B.15 referred to `writeVec3Frame`; that's a fabricated API name. The actual common-cpp API is `gpusims::vdb::writeVec3Grid` per Claude Code's Class B substitutions log. This subsection's text uses the correct name.

**Verification grep**:

```
grep -c "Implemented (Phase 12).*D3Q19 BGK around a NACA airfoil" project-state.md
# Expected: 1.

grep -c "Sim-spec stub.*lattice-boltzmann\|lattice-boltzmann.*Sim-spec stub" project-state.md
# Expected: 0 (the stub-status text is gone).
```

#### § 5.F.3 — "Last updated:" line

**Anchor** (verbatim, line 3 of synced project-state.md):

```
> **Last updated:** end of Phase 11 — `sph-water` Stack C particle-fluids flagship, scaffold landed at `09c0d9f` with shaders and docs; main.cpp dispatch chain + cross-cutting edits in follow-up commits per `phase11_deferred_backfill.md`. Phase 11 is the first Alembic real-impl consumer (CMake `FetchContent` vendored at 1.8.10), the first Stack C user of `VK_EXT_subgroup_size_control` (Vulkan 1.3 core), and the first multi-pass off-screen render-pass construction in Stack C. The probe-before-draft-lock discipline (sharpening of the fabrication-discipline convention) is banked at this phase — five distinct fabrication shapes caught by empirical probes pre-execution, plus two more surfaced during execution itself. **Next phase:** TBD per coordinator; remaining sims at § 6.
```

**Replace with**:

```
> **Last updated:** end of Phase 12 — `lattice-boltzmann` Stack C volumetric-grid sim, D3Q19 BGK around a NACA airfoil at a 256×128×128 desktop default tier. Substantive commit at `<PHASE_12_SHA>`; three prep commits (`8fe355b` Krüger vendoring, `0db9c73` algebraic D3Q19 derivation + verification harness, `c5955d3` architect-1 spec) landed before substantive. Phase 12 is the second Stack C volumetric-grid sim (after Phase 8 eulerian-smoke), the first sim consuming algebraic-derivation ground-truth via `tools/integrity/docs/algebraic/d3q19.md` (the `[Algebraic_D3Q19]` registry pattern, paired with the vendored `[Krueger]` MIT math reference), and the first real consumer of `gpusims::vdb::writeVec3Grid` post-Phase 8's `writeFloatGrid`. The single-architect-plus-checkpoints execution model was validated this phase: six Convention #8 architect-fabrications (Krüger D3Q19 scope at probe-2; § 5 anchor strings, common-cpp API surface, Zou-He boundary closure, `writeVec3Frame` API name, `docs/phase11_sph_water.md` location) were all caught by the 6-checkpoint protocol or by prep-landing audits before reaching production code. Equilibrium-distribution boundary shipped as v1 fallback for Zou-He; Zou-He 3D banked for v1.1 with algebraic derivation doc + verification harness at `tools/integrity/docs/algebraic/zou_he_d3q19.md` (to be created). **Next phase:** TBD per coordinator; remaining sims at § 6.
```

**Verification grep**:

```
grep -c "Last updated:.*Phase 12.*lattice-boltzmann" project-state.md
# Expected: 1.

grep -c "Last updated:.*Phase 11" project-state.md
# Expected: 0 (the Phase 11 wording is replaced).
```

#### § 5.F.4 — § 7 new H3 entry (frame-invariant capture meta key)

Appended after the most recent existing § 7 third-level heading. The original spec anchored against `### H3 — Capture format: \`*.bin\` extension stripping` — that heading does not exist; § 7 uses descriptive titles, not H3 numbering.

**Anchor** (the most recent existing § 7 entry per handoff bundle):

```
### Comments asserting platform or library behavior must cite verification source
```

**Insert (append a new ### entry immediately after this section closes — i.e., before the next ## H2 or ### H3 heading)**:

```markdown
### Frame-invariant capture meta key

Buffers whose value doesn't change per-frame within a session (obstacle masks, scene SDFs, immutable presets) carry `"frame_invariant": true` in their `saveBuffer` meta. The reader-side contract: F9-load may skip uploading the buffer if a current-session equivalent matches by hash. First consumer: Phase 12's `obstacle_mask`. Promotion gate: this single consumer enters the convention immediately because no precedent exists for the pattern — once consumer #2 surfaces, refine the hash-equivalence contract per the rule-of-three (Convention #4).
```

**Verification grep**:

```
grep -c "^### Frame-invariant capture meta key" project-state.md
# Expected: 1.
```

### § 5.G — `docs/tier1-capture-format-reference.md`

Two sub-edits.

#### § 5.G.1 — § 1 add latticeBoltzmann row

The synced table is 4 columns (`Sim | Phase | Stack | Top-level meta key`), not 3 as the spec's original anchor assumed.

**Anchor** (verbatim):

```
| sph-water | 11 | C (C++) | `sphWater` |
```

**Insert immediately after**:

```
| lattice-boltzmann | 12 | C (C++) | `latticeBoltzmann` |
```

**Verification grep**:

```
grep -c "^| lattice-boltzmann | 12 | C (C++) | \`latticeBoltzmann\` |" docs/tier1-capture-format-reference.md
# Expected: 1.
```

#### § 5.G.2 — § 2 precedent block

Append a new precedent block after the existing sph-water block.

**Anchor** (verbatim heading):

```
### sph-water (Phase 11, Stack C) — **special case with packed C-style structs**
```

**Locate the end of the sph-water block** (the next `###` heading, or end-of-file if none), then **insert immediately before** that boundary:

```markdown
### lattice-boltzmann (Phase 12, Stack C)

Buffers:

- `density` — `r32f`, count = Nx·Ny·Nz, stride = 4, shape = `[Nx, Ny, Nz]`. Bare name (no `.bin` extension — explicitly avoids the Phase 8 `.bin.bin` quirk).
- `velocity` — `rgba16f`, count = Nx·Ny·Nz, stride = 8, shape = `[Nx, Ny, Nz]`. The 4th component is unused at v1; banked as a candidate for vorticity-magnitude or pressure in v1.1. Bare name.
- `obstacle_mask` — `r8uint`, count = Nx·Ny·Nz, stride = 1, shape = `[Nx, Ny, Nz]`. Bare name. **Carries `"frame_invariant": true` in its meta** — the buffer is uploaded only on the first F5 of a session per preset; F9-load may skip the upload if the current-session preset's mask hash matches. First consumer of the `frame_invariant` convention; see `project-state.md` § 7 for the convention statement.

Distribution functions f_i are **not captured**: at ~610 MB per F5 at the desktop tier they are too large for per-frame archival. F9 reloads `density` + `velocity`, then runs a single `init_equilibrium.comp.glsl` pass to re-derive f_i from the equilibrium-Maxwell-Boltzmann distribution at the loaded macroscopic state. This loses the non-equilibrium component (the part of f_i encoding stress / shear / vorticity at the F5 instant) but for the quasi-steady-state wind-tunnel regime Phase 12 targets, the re-equilibrated state is visually indistinguishable within a few substeps. Banked as v1.1 polish: optional `--capture-full-state` flag emitting f_i at ~10 GB/F5 for hero-render-archival use.

Top-level meta block (`latticeBoltzmann`): `tierIndex`, `presetIndex`, `iteration`, `tau`, `substeps`, `uInfMagnitude`, `angleOfAttackDeg`, hash of obstacle mask, camera state. Spec reference: `docs/phase12_lattice_boltzmann.md` § 4.B.13.
```

**Note on insertion mechanics for Claude Code:** the sph-water block's body extends from the `### sph-water (Phase 11, ...)` heading until the next `### ` heading or EOF. Insert the new lattice-boltzmann block at that boundary (after the sph-water content, before the next sibling heading or at EOF if sph-water is the last block).

**Verification grep**:

```
grep -c "^### lattice-boltzmann (Phase 12, Stack C)" docs/tier1-capture-format-reference.md
# Expected: 1.
```

---

## Summary of changes from original spec § 5

| Sub-edit | Original anchor | Synced anchor | Net change |
|---|---|---|---|
| 5.A | `eulerian-smoke/**` (didn't exist) | `paths:` block w/ no per-sim entries | New wildcard `volumetric-grid/**` added (precedent-establishing) |
| 5.B | wildcard glob assumption | Commented-out stub pre-staged | Uncomment + remove if-gate (may already be done by Claude Code at Checkpoint 5) |
| 5.C | (unchanged from original) | (unchanged) | Replace stub at sim-specs/lattice-boltzmann.md |
| 5.D | sph-water row insertion | Lattice row already exists as "Not started" | In-row status edit |
| 5.E | Phase 11.x heading (fabricated) | Keep-a-Changelog `[0.13.0]` insert | New version block between Unreleased and 0.12.0 |
| 5.F.1 | 11.x row (fabricated) | Insert between Phase 11 row and 10+ aggregate | New row, schema-matched |
| 5.F.2 | 3-col anchor (wrong) | 4-col stub row exists | Edit-in-place |
| 5.F.3 | Templated SHA-pointer (fabricated) | Multi-sentence narrative | Rewrite paragraph |
| 5.F.4 | H3 numbering (fabricated) | Descriptive § 7 titles | Append new ### entry |
| 5.G.1 | 3-col row (wrong) | 4-col row format | New 4-col row |
| 5.G.2 | "Phase 11 — sph-water precedent" (wrong heading) | Actual heading verbatim | Append new ### block |

---

## Banked items surfaced by this reanchor

1. **CI path triggers in other sim categories** (small v1.x cleanup): add `particle-fluids/**` and `continuous-ca/**` to `build-native.yml` to bring all sim categories under CI watch. Not Phase 12's responsibility but a natural follow-up to § 5.A.
2. **`docs/tier1-capture-format-reference.md` Phase 9 + Phase 10 backfill** (still missing per the deferred-backfill bank from Phase 11 retro item 1). Phase 12 explicitly does not touch this — separate cleanup commit per its own retro item.
3. **Zou-He 3D derivation doc** (`tools/integrity/docs/algebraic/zou_he_d3q19.md`) — to be created for v1.1 Zou-He boundary upgrade. Same shape as `d3q19.md`.
4. **`load_registry` v1.2 refinement** (banked at the prep-2 commit's audit): named permanent registry-entry kinds (`algebraic`, `upstream`, `audit-doc-snapshot`, etc.) so the loader doesn't need silent-skip heuristics on `vendor_root` presence. Surfaced as registry-consumer-#2 spec/loader drift.

---

## Patch end.

> Apply § 5.A through § 5.G in any order (no inter-dependencies). Run the § 6 verification block from the original spec after all § 5 edits land. Then proceed to § 7 substantive commit + cross-cutting commit + SHA-backfill commit per Convention #12.
