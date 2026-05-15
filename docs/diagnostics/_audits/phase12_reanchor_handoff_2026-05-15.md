---
title: Phase 12 — § 5 anchor re-sync handoff to architect-1
date: 2026-05-15
author: claude-code
phase: 12
status: handoff
scope: spec-reanchor-prep
---

# Phase 12 § 5 anchor re-sync handoff

The Phase 12 spec at `docs/phase12_lattice_boltzmann.md` was locked against an
assumed-future repo state (Phase 11.x in flight; per-sim CI path triggers
already landed; tier1-capture-format-reference rows in a 3-column shape).
The synced repo at HEAD `c5955d3` differs materially. Per § 0.2 hard rule 5,
Claude Code halts § 5 cross-cutting work and bundles the synced state below
for architect-1 to re-anchor in `phase12-spec-reanchor-2026-05-15.md`.

Sim-local generation (§ 4.A–4.O, Checkpoints 1–5) proceeds in parallel under
fix-forward authorization for Class B / C / D drift — see the executing-log
companion `phase12_substantive_landing_2026-05-15.md` (forthcoming).

---

## § 5.A — `.github/workflows/build-native.yml`

Both jobs (`build-ubuntu` Release, `build-ubuntu-debug` Debug) currently have
**no per-sim path triggers**. Only the root `CMakeLists.txt`, the entire
`common/common-cpp/**` tree, and the workflow file itself are watched.

Synced `paths:` blocks (identical in both `push:` and `pull_request:`):

```yaml
    paths:
      - 'CMakeLists.txt'
      - 'common/common-cpp/**'
      - '.github/workflows/build-native.yml'
```

The Release job adds `libopenvdb-dev`, `libboost-iostreams-dev`,
`libimath-dev`, `spirv-tools`, `glslang-tools` to the apt list and configures
with `-DGPU_SIMS_BUILD_EXAMPLES=ON -DGPU_SIMS_USE_OPENVDB=ON
-DGPU_SIMS_USE_ALEMBIC=ON`. The Debug job omits OpenVDB and uses
`-DGPU_SIMS_BUILD_EXAMPLES=ON -DGPU_SIMS_USE_ALEMBIC=ON`.

**Architect-1 decision needed:** does Phase 12 add per-sim path triggers for
the first time (precedent-establishing), or stay on the
`common-cpp + root + workflow` posture? Spec § 5.A as written assumes the
former.

## § 5.B — root `CMakeLists.txt`

Synced state uses **per-directory** `add_subdirectory()` calls. The lattice-
boltzmann insertion point is **already pre-staged as a commented-out line**
in a `GPU_SIMS_BUILD_VOLUMETRIC_GRID` option block:

```cmake
add_subdirectory(continuous-ca/reaction-diffusion-3d)

add_subdirectory(volumetric-grid/eulerian-smoke)
add_subdirectory(particle-fluids/sph-water)

if(GPU_SIMS_BUILD_INTEGRITY_CAT3)
    add_subdirectory(tools/integrity/drivers/integrity_cat3_stack_c)
endif()

# if(GPU_SIMS_BUILD_VOLUMETRIC_GRID)
#     add_subdirectory(volumetric-grid/lattice-boltzmann)
# endif()
```

Phase 12 unconditionally enables the line (drop the `if(...)` gate to match
the eulerian-smoke / sph-water posture, since the per-category opt-ins are
not actually used today):

```cmake
add_subdirectory(volumetric-grid/lattice-boltzmann)
```

## § 5.D — root `README.md` gallery

Gallery already has a `Lattice Boltzmann` row reading **"Not started"**.
Anchor + replacement are an in-row edit, not an insertion:

```markdown
| [Lattice Boltzmann](volumetric-grid/lattice-boltzmann/) | Volumetric grid | Native C++ | Not started |
```

Replace status to `Implemented (Phase 12)`:

```markdown
| [Lattice Boltzmann](volumetric-grid/lattice-boltzmann/) | Volumetric grid | Native C++ | Implemented (Phase 12) |
```

## § 5.E — `CHANGELOG.md`

Keep-a-Changelog format. Top of file:

```markdown
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]


## [0.12.0] - Phase 11: sph-water (Stack C particle-fluids flagship; first Alembic real-impl consumer)
...
```

The latest shipped version is `[0.12.0]`. Phase 12 lands as `[0.13.0]`
inserted between `## [Unreleased]` and `## [0.12.0]`. There is no `Phase
11.x` heading; the spec's anchor is a fabrication.

## § 5.F — `project-state.md`

### § 5.F.1 — § 3 phase ledger row

Last two rows verbatim (line numbers from synced file):

```
| 11 | sph-water (Stack C particle-fluids flagship; DFSPH; first Alembic consumer) | DFSPH inner-iter Jacobi loops; Morton spatial hash; screen-space fluid render; subgroup-size pinning; FetchContent for Alembic 1.8.10. Scaffold shipped at ``09c0d9f``; main.cpp + § 5 cross-cutting edits land in follow-up commits per `phase11_deferred_backfill.md`. SHA-backfill follow-up tidies this row and § 11 below once Phase 11's third commit resolves. | ✅ Shipped | ``09c0d9f`` |
| 10+ | Remaining sims | One phase per remaining sim. Each consumes a settled `common-` package; per-sim phases are smaller than the foundation phases. Per Phase 9 banking, the natural Alembic-real-impl consumer is sph-water (Stack C). The natural cross-stack lenia-fft consumer (Stack D Taichi + Stack B WebGPU) is post-MPM. | ⬜ Not started | — |
```

Phase 12 row inserts **between** the Phase 11 row and the `10+ — Remaining
sims` aggregate row. Schema: `| <phase> | <name> | <description> | <status>
| <SHA> |`.

### § 5.F.2 — § 6 sim-list row

Sim-list rows live at `project-state.md` lines 189–198 with a 4-column
schema `| path | name | stack | status |`. The lattice-boltzmann row
already exists as a stub:

```
| `volumetric-grid/lattice-boltzmann/` | lattice-boltzmann | C | Sim-spec stub |
```

Phase 12 replaces the stub-status text with implemented-status text
analogous to eulerian-smoke's row. Suggested form (architect-1 to confirm):

```
| `volumetric-grid/lattice-boltzmann/` | lattice-boltzmann | C | **Implemented (Phase 12)** — second Stack C volumetric-grid sim; D3Q19 BGK around a NACA airfoil; first consumer of `gpusims::vdb::writeVec3Frame` (real impl post-Phase 8); first sim using algebraic-derivation ground-truth via `tools/integrity/docs/algebraic/d3q19.md`. |
```

### § 5.F.3 — "Last updated:" line

Synced (line 3, single multi-sentence narrative paragraph):

```
> **Last updated:** end of Phase 11 — `sph-water` Stack C particle-fluids flagship, scaffold landed at `09c0d9f` with shaders and docs; main.cpp dispatch chain + cross-cutting edits in follow-up commits per `phase11_deferred_backfill.md`. Phase 11 is the first Alembic real-impl consumer (CMake `FetchContent` vendored at 1.8.10), the first Stack C user of `VK_EXT_subgroup_size_control` (Vulkan 1.3 core), and the first multi-pass off-screen render-pass construction in Stack C. The probe-before-draft-lock discipline (sharpening of the fabrication-discipline convention) is banked at this phase — five distinct fabrication shapes caught by empirical probes pre-execution, plus two more surfaced during execution itself. **Next phase:** TBD per coordinator; remaining sims at § 6.
```

Phase 12 rewrites this to "end of Phase 12 — `lattice-boltzmann` …" with
analogous narrative shape. Architect-1 supplies the new wording.

### § 5.F.4 — § 7 H3 entry for `frame_invariant`

There is no `### H3 — Capture format: \`*.bin\` extension stripping`
heading in the synced file. § 7 third-level headings use descriptive
titles (e.g., "Shader-copy destination must namespace by sim", "Commit-SHA
back-fill must use a separate follow-up commit, not `git commit --amend`",
etc.). The most recent § 7 entry is **"Comments asserting platform or
library behavior must cite verification source"** (Phase 10 retro).

Phase 12 appends `### Frame-invariant capture meta key` as a new entry
after the most recent existing ### within § 7. Insertion is unambiguous;
no anchor mismatch — but the spec's anchor reference to "H3 — Capture
format: `*.bin` extension stripping" is a fabrication.

## § 5.G — `docs/tier1-capture-format-reference.md`

### § 5.G.1 — § 1 row insert

Synced § 1 table is **4 columns** (`Sim | Phase | Stack | Top-level meta
key`), not 3 as the spec assumes. Last row:

```
| sph-water | 11 | C (C++) | `sphWater` |
```

Phase 12 row form:

```
| lattice-boltzmann | 12 | C (C++) | `latticeBoltzmann` |
```

Note: Phase 9 (`mpmMultimaterial`) and Phase 10 (`lenia` / `leniaFft`) rows
are **still missing** per the deferred-backfill bank — Phase 12 does not
fix this, per the spec's § 3.C explicit non-touch list.

### § 5.G.2 — § 2 precedent block

The most recent precedent block heading is:

```
### sph-water (Phase 11, Stack C) — **special case with packed C-style structs**
```

(Phase 9 / Phase 10 precedents are missing per the deferred-backfill bank.)

Phase 12 appends `### lattice-boltzmann (Phase 12, Stack C)` after the
sph-water block. Insertion unambiguous; only the spec's anchor heading
text was wrong.

---

## Summary table

| Spec § | Spec anchor | Synced reality | Resolution path |
|---|---|---|---|
| 5.A | `eulerian-smoke/**` path triggers exist | Path triggers don't list per-sim entries | Architect-1: precedent-establish or hold posture |
| 5.B | wildcard `add_subdirectory` | Per-directory; commented-out LBM line pre-staged | Uncomment + remove if-gate |
| 5.D | sph-water row text | Different row format; lattice row already exists as "Not started" | In-row status edit |
| 5.E | `Phase 11.x — sph-water DFSPH α-factor rewrite (in flight)` | Keep-a-Changelog `[0.12.0] - Phase 11: sph-water ...`; no 11.x heading | New `[0.13.0]` after `[Unreleased]` |
| 5.F.1 | `\| 11.x \| sph-water 2.16.1 swap …` | No 11.x row; last shipped is Phase 11 row + "10+ Remaining sims" aggregate | Insert between Phase 11 and 10+ |
| 5.F.2 | 3-column sim-list anchor | 4-column rows; LBM stub row exists | Edit-in-place stub row |
| 5.F.3 | Templated SHA-pointer line | Multi-sentence narrative | Rewrite paragraph |
| 5.F.4 | `### H3 — Capture format: \`*.bin\` extension stripping` | No such heading; §7 uses descriptive titles | Append after most-recent §7 entry |
| 5.G.1 | 3-column sph-water row | 4-column row format | New 4-column row |
| 5.G.2 | `### Phase 11 — sph-water precedent` | `### sph-water (Phase 11, Stack C) — **special case ...`** | Append new heading after sph-water block |

---

## What architect-1 returns

A single patch document `docs/phase12_lattice_boltzmann_reanchor_2026-05-15.md`
with:
- New § 5 (replacing the original § 5 wholesale).
- For each sub-edit: anchor string verbatim from synced state, exact insert/
  replace text with surrounding indentation preserved, expected post-edit
  byte length or row count for grep-verification.
- Optional: a § 5.A decision rationale (precedent-establish vs hold-posture
  on per-sim CI path triggers).

Until then, Claude Code completes Checkpoints 1–5 on sim-local files, halts
at Checkpoint 5 verification (build green) for re-anchored § 5, then resumes.
