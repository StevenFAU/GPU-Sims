---
title: "Integrity Toolkit v1.1 Pre-Spec Probe"
date: 2026-05-15
author: architect1
status: probe
scope: read-only
sibling-docs:
  - docs/integrity-toolkit-spec.md
  - docs/retro/integrity-toolkit-v1.md
---

# Integrity Toolkit v1.1 Pre-Spec Probe

Read-only inventory grounding the v1.1+v2 integrity-toolkit spec draft. Every
finding is tagged `FACT` (directly observed) or `INFERENCE` (derived).

## Section A — Current toolkit file tree

### A.1 `find tools/integrity -type f ...` (FACT)

Full enumeration ran cleanly. Structure (pytest cache + fixture mirror-tree
collapsed for noise; everything else verbatim):

```
tools/integrity/
├── README.md, pyproject.toml
├── docs/{grandfather-catalog,ground-truth-sources}.md
├── drivers/integrity_cat3_stack_c/{CMakeLists.txt, main.cpp}
├── gpusims_integrity.egg-info/{dependency_links,entry_points,requires,
│                               SOURCES,top_level}.txt + PKG-INFO
├── scripts/{__init__.py, grandfather_sweep.py}
├── integrity/
│   ├── {__init__,__main__,runner,grandfather}.py
│   ├── common/
│   │   └── {__init__,annotations,audit_log,exclusions,repo,results,
│   │        stack_paths,suppression}.py
│   ├── cat1_citations/
│   │   ├── {__init__,grammar,resolver,upstream_anchor}.py
│   │   └── checks/
│   │       └── {__init__,annotation,intra_repo,unregistered_upstream,
│   │            upstream_anchor,upstream}.py
│   ├── cat2_contracts/
│   │   ├── {__init__,stack_b,stack_c,stack_d}.py
│   │   ├── ts_helper/        # node project — extract_and_find.ts + tsconfig
│   │   └── checks/
│   │       └── {__init__,public_symbol_used,           # Stack D / Python
│   │            public_symbol_used_b,                  # Stack B / TS
│   │            public_symbol_used_c}.py               # Stack C / libclang
│   └── cat3_numerical/
│       ├── {__init__,cubic_kernel,generate_expected}.py + expected_values.toml
│       └── checks/{__init__,cubic_kernel}.py
└── tests/
    ├── conftest.py
    ├── fixtures/{good,bad}_{citations,contracts,contracts_b,contracts_c}/
    └── test_{cat1_annotation,cat1_intra_repo,cat1_unregistered,
              cat1_upstream_anchor,cat1_upstream,cat2_stack_b,cat2_stack_c,
              cat2_stack_d,cat3_cubic_kernel,grandfather_sweep,runner}.py
```

Note: `cat1_citations/upstream_anchor.py` (registry-side helper, 88 LOC) and
`cat1_citations/checks/upstream_anchor.py` (the check, 63 LOC) coexist — same
basename in two directories.

### A.2 Line counts for every `*.py` under `tools/integrity/integrity/` (FACT)

Total: **3299 LOC** across 35 Python files. Heavy hitters (>=99 LOC):

- 531 `cat2_contracts/stack_c.py` — libclang AST walker
- 344 `grandfather.py` — classifier rules for 11 categories
- 272 `cat2_contracts/stack_d.py` — Python ast-based public-symbol detector
- 177 `cat1_citations/grammar.py` — annotation parser
- 173 `runner.py` — check discovery + dispatch
- 132 `cat2_contracts/stack_b.py` — ts_helper driver
- 124 / 123 / 118 / 111 / 109 / 103 — `upstream.py`, `annotation.py`,
  `generate_expected.py`, `cubic_kernel.py`, `intra_repo.py`, `resolver.py`
- 99 / 99 / 95 / 88 — `public_symbol_used.py`, `checks/cubic_kernel.py`,
  `unregistered_upstream.py`, `upstream_anchor.py` (registry helper)

Remaining 19 files each <80 LOC (common/*, `checks/__init__.py`, etc.).

### A.3 Presence/absence of specific paths (FACT)

| Path | Status |
| --- | --- |
| `cat3_numerical/stack_b_driver/` | ABSENT |
| `cat3_numerical/stack_d_runner.py` | ABSENT |
| `cat2_contracts/checks/stub_label_stale.py` | ABSENT |
| `cat2_contracts/checks/public_field_read.py` | ABSENT |
| `cat2_contracts/checks/public_symbol_def.py` | ABSENT |
| `cat2_contracts/checks/re_export_match.py` | ABSENT |
| `cat1_citations/checks/audit_log.py` | ABSENT |
| `cat1_citations/checks/exclusion_list.py` | ABSENT |

**INFERENCE:** none of the proposed v1.1 / v2 check modules exist yet. The
shipped toolkit covers Cat 1 (5 checks), Cat 2 (3 checks — one per stack), and
Cat 3 (one cubic-kernel check against the Stack C driver only). Stack B and
Stack D Cat 3 paths are unrepresented.

## Section B — Registered checks

### B.1–B.3 Check-registry `__init__.py` files (FACT — all read verbatim)

- `cat1_citations/checks/__init__.py` (17 LOC): registers `intra_repo`,
  `annotation`, `upstream`, `upstream_anchor`, `unregistered_upstream` (5 modules).
- `cat2_contracts/checks/__init__.py` (13 LOC): registers
  `public_symbol_used` (Stack D), `public_symbol_used_c` (Stack C),
  `public_symbol_used_b` (Stack B) — in that order.
- `cat3_numerical/checks/__init__.py` (7 LOC): single import `cubic_kernel`;
  `REGISTERED_CHECKS = [(cubic_kernel.CHECK_ID, cubic_kernel)]`.

### B.4 `rg -n '^CHECK_ID = ' tools/integrity/integrity/` (FACT)

```
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
cat3_numerical/checks/cubic_kernel.py:30:CHECK_ID = "cat3.cubic-kernel"
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
cat1_citations/checks/intra_repo.py:25:CHECK_ID = "cat1.intra-repo"
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
cat1_citations/checks/annotation.py:23:CHECK_ID = "cat1.annotation-form"
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
cat1_citations/checks/upstream_anchor.py:19:CHECK_ID = "cat1.upstream-anchor"
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
cat1_citations/checks/upstream.py:25:CHECK_ID = "cat1.upstream-citation"
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
cat1_citations/checks/unregistered_upstream.py:25:CHECK_ID = "cat1.unregistered-upstream"
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
cat2_contracts/checks/public_symbol_used_b.py:20:CHECK_ID = "cat2.public-symbol-used-ts"
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
cat2_contracts/checks/public_symbol_used_c.py:35:CHECK_ID = "cat2.public-symbol-used-c"
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
cat2_contracts/checks/public_symbol_used.py:38:CHECK_ID = "cat2.public-symbol-used"
```

Nine `CHECK_ID`s live; only `cat1.upstream-anchor` is registered in `__init__`
but uses the same value as its module — consistent with B.1 listing.

## Section C — Current grandfather state

### C.1 `python3 -m integrity --mode strict --output json --no-audit-log` — summary block (FACT)

```json
"summary": {
    "pass": 2,
    "soft_warn": 0,
    "hard_fail": 0,
    "suppressed": 1126
}
```

Run was made from repo root with `python3` (not `python` — system has no
`python` shim; FACT). Schema version: 1. Commit: 447ebf0.

### C.2 Findings by `check_id` (FACT, derived from same JSON)

| check_id | count |
| --- | ---: |
| cat1.intra-repo | 816 |
| cat2.public-symbol-used-c | 111 |
| cat2.public-symbol-used-ts | 73 |
| cat1.annotation-form | 69 |
| cat1.upstream-citation | 37 |
| cat2.public-symbol-used | 17 |
| cat1.unregistered-upstream | 3 |
| **total** | **1126** |

All current findings are suppressed (zero hard_fail / soft_warn). `pass=2`
reflects checks with no findings to report at all
(`cat1.upstream-anchor` and `cat3.cubic-kernel`, INFERENCE).

### C.3 Findings by `suppression_reason` (FACT)

| reason (truncated) | count |
| --- | ---: |
| audit-doc snapshot of pre-v1 codebase (audit-citation) | 761 |
| pre-v1 Stack C public symbol with no current consumer (cat2-stack-c-unused) | 111 |
| grandfathered-pre-v1 (other-cat1) | 73 |
| pre-v1 Stack B public symbol with no current consumer (cat2-stack-b-unused) | 73 |
| documentation-only literal mention of the annotation grammar | 24 |
| regex/docstring literal of the annotation grammar token | 21 |
| audit-doc reference to the historical 1.8.10 fabrication (permanent) | 21 |
| pre-v1 Stack D public symbol with no current consumer (cat2-stack-d-unused) | 17 |
| audit-doc literal mention of the annotation grammar | 14 |
| pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (live-shader-1810) | 9 |
| retrospective-doc literal mention of the annotation grammar | 2 |

### C.4 Grandfather catalog categories (FACT — from `tools/integrity/docs/grandfather-catalog.md`)

11 categories in document order: `audit-citation`, `live-shader-1810`,
`audit-doc-1810`, `spec-grammar-example`, `toolkit-own-source`,
`retro-grammar-example`, `audit-report-grammar-example`, `other-cat1`,
`cat2-stack-d-unused`, `cat2-stack-c-unused`, `cat2-stack-b-unused`.

The catalog prose **does not** record per-category numeric tallies (FACT —
verified by reading the doc); it carries approximate ranges (e.g.
"~10 entries", "~108 entries"). The runtime `suppression_reason` table in
C.3 is the authoritative live count.

<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
### C.5 Top 30 files by `integrity-allow:` literal density (FACT)

```
204  docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md
124  docs/diagnostics/_audits/phase11_5_probe_2026-05-14_architect1.md
 84  docs/diagnostics/_audits/sims_lenia_probe1_2026-05-14_architect3b.md
 84  docs/diagnostics/_audits/phase11_5_probe2_2026-05-14_architect1.md
 48  docs/diagnostics/_audits/commoncpp_unexercised_2026-05-14_architect2.md
 45  docs/diagnostics/_audits/integrity_toolkit_probe_2026-05-14_architect1.md
 41  docs/integrity-toolkit-spec.md
 26  docs/diagnostics/_audits/phase11_5_probe3_2026-05-14_architect1.md
 22  tools/integrity/integrity/grandfather.py
 22  docs/diagnostics/_audits/phase11_5_setup1_2026-05-14_setup1.md
 18  tools/integrity/tests/test_grandfather_sweep.py
 18  common/common-cpp/include/gpusims/camera.hpp
 17  tools/integrity/docs/grandfather-catalog.md
 13  docs/diagnostics/_audits/sims_lenia_chakazul_2026-05-14_architect3b.md
 13  docs/diagnostics/_audits/integrity_build_5_landing_2026-05-14.md
 12  common/common-web/src/paramPanel.ts, common/common-web/src/camera.ts
 11  docs/diagnostics/_audits/sims_lenia_synthesis_2026-05-14_architect3b.md
 10  docs/diagnostics/_audits/integrity_build_3_landing_2026-05-14.md,
       common/common-py/gpusims_common/camera.py
  9  common/common-cpp/include/gpusims/vk/{window,image,context,buffer}.hpp (×4)
  8  cat1_citations/checks/annotation.py, three landing-doc audits,
       vk/{graphics,compute}_pipeline.hpp (×2)
  6  particle-fluids/sph-water/shaders/density_solve.comp.glsl
```

**INFERENCE:** the long tail is dominated by audit reports (8 of top 10 are
under `docs/diagnostics/_audits/`); the heaviest live-code file is
`common/common-cpp/include/gpusims/camera.hpp` at 18 annotations.

## Section D — Stale stub-label candidates

### D.1 Hits for `\bstub\b` under live common-stack source (FACT)

12 hits total, in 4 files:

- `common/common-py/gpusims_common/vdb_writer.py` (3 hits — module docstring "real-or-stub", import-guard comment, `is_available()` docstring)
- `common/common-py/gpusims_common/alembic_writer.py` (7 hits — repeated "permanent stub for Phase 9" framing)
- `common/common-cpp/include/gpusims/vdb_writer.hpp:12` ("In Phase 1, this is a stub: if GPU_SIMS_HAVE_OPENVDB is not defined…")
- `common/common-cpp/include/gpusims/alembic_writer.hpp:13` ("In Phase 1, this is a stub: if GPU_SIMS_HAVE_ALEMBIC is not defined…")

No hits under `common/common-cpp/src/` or `common/common-web/src/`.

### D.2 Per-hit implementation-body classification (FACT for LOC, INFERENCE for "stale")

| Label site | Label phrasing | Impl file | Non-comment LOC | Verdict |
| --- | --- | --- | ---: | --- |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| `alembic_writer.hpp:13` | "Phase 1 stub" | `src/alembic_writer.cpp` | 99 | **STALE** — canonical spec § 12 row 5 |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| `vdb_writer.hpp:12` | "Phase 1 stub" | `src/vdb_writer.cpp` | 135 | **STALE** |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `gpusims_common/vdb_writer.py:1/31/56` | "real-or-stub" (discriminator) | (same file) | 150 | NOT STALE — describes runtime mode |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `gpusims_common/alembic_writer.py:1/3/69/91/93/96/116` | "permanent stub for Phase 9" | (same file) | 85 | NOT STALE — explicitly permanent; rule-of-three deferred (project-state.md §7) |

Heuristic: "non-trivial body" = `rg -v '^\s*(//|#|$)'` LOC > 10. The two
C++ headers' "Phase 1 stub" framing now contradicts the 99 / 135 LOC sibling
implementations.

### D.3 Spec § 12 row 5 canonical case (FACT)

**Confirmed.** `common/common-cpp/include/gpusims/alembic_writer.hpp` lines 11–16
still read verbatim: *"In Phase 1, this is a stub: if GPU_SIMS_HAVE_ALEMBIC is
not defined at compile time, all functions log a warning on first call and
return false. Real implementations land when the first Alembic-consuming sim
(likely SPH water) is built."* Meanwhile `common/common-cpp/src/alembic_writer.cpp`
is now 117 total / 99 non-comment LOC, with a real `AlembicWriter::writeFrame`
gated on `GPU_SIMS_HAVE_ALEMBIC` (the disabled path keeps the log-and-return;
the enabled path is a working Ogawa/AbcGeom writer). The stub label is stale.

## Section E — CI walltime baseline

### E.1 Latest CI run on `main` (FACT — `gh api`)

Run `25895404678` (push of 447ebf0, "fix(integrity): grandfather retrospective…").
Total wall clock: **4m 59s**. Per-step (rounded to seconds):

| Step | Duration |
| --- | ---: |
| Set up job + Checkout + setup-python + setup-node (steps 1–5) | ~3s combined |
| Clone vendored references (anchor-pinned) | 2s |
| Install workspace deps (root npm) + build TS helper | 5s |
| Install integrity toolkit (pip install -e .[dev]) | 8s |
| Install build deps (apt) | 17s |
| Configure Stack C build (CMake) | 31s |
| **Build Cat 3 Stack C driver** | **1s** |
| Dogfood pytest | 46s |
| **Run integrity toolkit against repo** | **184s (3m 4s)** |
| Upload audit log + post-actions | ~2s |

### E.2 Local walltime (FACT)

`time python3 -m integrity --no-audit-log` from repo root:

```
real    1m39.373s
user    1m37.887s
sys     0m2.710s
```

Cat 3 driver was **not** rebuilt for this run (binary already absent locally;
the Cat 3 cubic-kernel check would skip silently if its driver isn't on disk
— INFERENCE based on `cat3_numerical/checks/cubic_kernel.py` being the only
Cat 3 module and its 99 LOC scope). Practical effect: the local 1m39s does
not include Cat 3.

**INFERENCE:** CI dominant cost = step 13 (3m 4s) — repo-wide static analysis
in `cat2.public-symbol-used-c` (libclang AST over Stack C TUs) plus
`cat1.intra-repo` (regex sweep over all docs). The same step runs locally in
~1m39s; the ~85s delta to CI is consistent with cold-cache I/O on Actions
runners (INFERENCE).

## Section F — Cat 2 false-positive sample

Sampled 5 Stack D and 5 Stack C suppressions with `random.seed(42)`.

### F.1 Stack D (`cat2.public-symbol-used`) — 5 samples (FACT)

| # | Symbol (file:line) | Refs found | Classification |
| --- | --- | --- | --- |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| 1 | `ParticleFrame.ids` (alembic_writer.py:51) | only Python decl; C++ struct field at `alembic_writer.hpp:26` (different stack) | TRUE-POSITIVE |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| 2 | `ParticleFrame.positions` (alembic_writer.py:44) | kwarg call `examples/hello/main.py:223`; same-file docstring; no `.positions` field-read | TRUE-POSITIVE (canonical spec § 12) |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| 3 | `CameraInputState.shift_held` (camera.py:57) | 7 `.shift_held` refs **all in C++** (sph-water:942, eulerian-smoke:444, rd3d:334, common-cpp src/camera.cpp:60, examples/hello/main.cpp:51, camera.hpp:19) | TRUE-POSITIVE — Python field has no Python consumer |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| 4 | `CameraInputState` class (camera.py:35) | C++ consumers only; web TS `input.ts` is a comment-only reference | TRUE-POSITIVE |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| 5 | `write_float_grid` (vdb_writer.py:61) | re-export in `__init__.py`; one self-call from `write_float_frame` (vdb_writer.py:112); no external consumer | TRUE-POSITIVE |

### F.2 Stack C (`cat2.public-symbol-used-c`) — 5 samples (FACT)

| # | Symbol (decl file:line) | Refs found | Classification |
| --- | --- | --- | --- |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| 1 | `StateReader::bufferMeta` (state_reader.hpp:39) | impl `state_reader.cpp:57`; one self-call in `.cpp:66`; TS twin `stateReader.ts:76` (Stack B) | TRUE-POSITIVE |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| 2 | `Camera::fromJson` (camera.hpp:121) | **active consumers**: sph-water/main.cpp:2083, rd3d/main.cpp:766, examples/hello/main.cpp:377 | **FALSE-POSITIVE** |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 3 | `vk::Window::swapchain` (vk/window.hpp:57) | declaration only; zero `\.swapchain()` / `->swapchain()` / `::swapchain` call sites | TRUE-POSITIVE |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| 4 | `Camera::resetArcball` (camera.hpp:100) | impl `camera.cpp:27`; TS twin `camera.ts:216` (Stack B); no C++ call site in src/, examples/, per-sim | TRUE-POSITIVE |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| 5 | `vk::ShaderCompiler::compileFile` (vk/shader_compiler.hpp:51) | impl `shader_compiler.cpp:117`; **active callers**: `vk/compute_pipeline.cpp:70` and `:178`, `vk/graphics_pipeline.cpp:193` | **FALSE-POSITIVE** |

### F.3 Tallies (FACT)

- **Stack D (n=5):** 5 TP / 0 FP / 0 ambiguous.
- **Stack C (n=5):** 3 TP / 2 FP / 0 ambiguous.
- **Combined (n=10):** 8 TP / 2 FP / 0 ambiguous.

**INFERENCE:** both Stack C FPs (`Camera::fromJson`, `ShaderCompiler::compileFile`)
have real call sites in `common-cpp/src/` and (for `fromJson`) in per-sim Stack C
source; they're heavily exercised. This points to the Stack C check missing call
expressions where the receiver type isn't fully resolved — e.g. via references,
non-pointer member access, or B/C name-collision (`Camera`). v1.1 spec should
expect a fraction of the 111-row `cat2-stack-c-unused` catalog to dissolve once
the resolver is strengthened. 2/5 is too small to extrapolate confidently, but
extrapolation suggests ~20% of 111 entries (low-confidence INFERENCE).

## Section G — References tree state

### G.1 SPlisHSPlasH submodule (FACT)

```
$ git -C references/SPlisHSPlasH rev-parse HEAD
6bff55a6eaf14083d34650f22a268ce156b62b54
$ git -C references/SPlisHSPlasH describe --tags --always
2.16.1
```

### G.2 Match against `tools/integrity/docs/ground-truth-sources.md` (FACT)

Registered TOML block reads `anchor_version = "2.16.1"`,
`anchor_sha = "6bff55a6…62b54"`, `vendor_root = "references/SPlisHSPlasH"`,
`anchor_doc = ".gitignore"`,
`used_by_checks = ["cat1.upstream-citation", "cat1.upstream-anchor", "cat3.cubic-kernel"]`.
**Match.** Live tree SHA = registered SHA; tag = registered version.

### G.3 Other vendored references (FACT)

`ls references/` → only `SPlisHSPlasH`. Per ground-truth-sources.md
"Not yet registered (intentional)": `Chakazul/Lenia (LeniaNDK)` is the
deliberate test case for `cat1.unregistered-upstream`.

## Section H — Sim numerical content inventory

For each sim, primary algorithmic file → algorithm one-liner → upstream
citation status → per-sim Cat 3 difficulty estimate. Status / spec / stack
inferred from each sim's README header.

| Sim | Primary file | Algorithm | Upstream reference | Cat 3 difficulty |
| --- | --- | --- | --- | --- |
| `agent-based/boids-3d` | `web/shaders/flock_update.compute.wgsl` (Stack B, WGSL) | Reynolds separation+alignment+cohesion over 27-cell spatial hash, + leader attraction (cosine falloff) + predator-flee (linear) | Reynolds (1987) cited in spec sheet; **not** in `ground-truth-sources.md` | MEDIUM — per-particle force vectors are easy to hash; closed-form test is constant-density toy case; no vendored upstream → harder ground truth |
| `agent-based/physarum` | `web/shaders/agent_move.compute.wgsl` (Stack B, WGSL) | Multi-species Jones (2010) sense-3-points / steer / move / deposit | Jones (2010) cited in README; **not** registered | MEDIUM — per-agent state is hashable; closed-form is a single-agent straight-line trajectory in zero-field |
| `closed-form/mandelbulb-explorer` | `web/shaders/raymarch.frag.wgsl` (referenced from main.ts) | Distance-estimator raymarcher of the Mandelbulb iterated map (Daniel White 2009) | White (2009) cited in README; **not** registered | LOW — pure closed-form; DE at known reference points is trivially checkable per-pixel |
| `closed-form/strange-attractors` | `web/shaders/integrate.compute.wgsl` (Stack B, WGSL) | RK4 over Lorenz / Aizawa / Thomas ODEs | Lorenz 1963 / Aizawa 1984 / Thomas 1999 cited in README; **not** registered | LOW — closed-form RK4 over published ODEs; per-particle state easy to hash |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| `continuous-ca/lenia-fft` | `python/lenia_fft/main.py` + `python/lenia_fft/kernels.py` (Stack D, Python) | Bert Chan Lenia: FFT-convolution of state with quad4 kernel, Gaussian growth map, Euler update | Chakazul/LeniaNDK cited in `presets.py:11`; **intentionally unregistered** (test case for `cat1.unregistered-upstream`) | MEDIUM — per-cell state hashable; kernel polynomial closed-form; FFT-backend selection adds non-determinism for byte-exact compare |
| `continuous-ca/neural-ca` | (unimplemented — README only) | Neural CA grow-from-seed (planned Stack D + B) | tracked only in spec sheet | N/A — not yet implementable |
| `continuous-ca/reaction-diffusion-2d` | `web/shaders/rd_update.compute.wgsl` (Stack B, WGSL) | Gray-Scott PDE, 5-point Laplacian, forward Euler | Munafo / Pearson presets cited in README; **not** registered | LOW — Gray-Scott has analytic steady states (Turing instability boundary) + per-pixel hash |
| `continuous-ca/reaction-diffusion-3d` | `shaders/rd_update.comp.glsl` (Stack C, GLSL) | Gray-Scott PDE, 7-point Laplacian on 3D sampler3D | shared with 2D variant | LOW — same as 2D, just one more dimension |
| `hybrid-particle-grid/mpm-multimaterial` | `python/mpm_multimaterial/kernels.py` (Stack D, Taichi) | MLS-MPM, per-material (WATER/JELLY/SNOW) plasticity branching from upstream `mpm3d_ggui.py` | upstream `taichi-dev/taichi mpm3d_ggui.py` cited in kernel header; **not** registered | HIGH — Taichi backend non-determinism (CUDA-vs-Vulkan atomic-add ordering), three plasticity paths to cover, global state vs per-particle |
| `particle-fluids/pic-flip` | (unimplemented) | Planned PIC/FLIP solver | tracked only in spec sheet | N/A |
| `particle-fluids/sph-water` | `shaders/*.comp.glsl` (Stack C, GLSL — DFSPH family) | DFSPH (density + divergence solves), cubic-kernel SPH, Morton-sorted spatial hash, Jacobi inner loops | **SPlisHSPlasH 2.16.1 registered** in ground-truth-sources.md; live shaders still carry `1.8.10` anchors per `live-shader-1810` grandfather category | LOW — the only sim with both a vendored upstream AND a landed Cat 3 check (`cat3.cubic-kernel`); per-particle state hashable, kernel closed-form known |
| `quantum/ising-dwave` | (unimplemented) | 2D/3D Ising on D-Wave annealer | tracked only in spec sheet | N/A |
| `volumetric-grid/eulerian-smoke` | `shaders/jacobi_pressure.comp.glsl` and friends (Stack C, GLSL) | Stable-Fluids–style semi-Lagrangian advection, Jacobi pressure-Poisson (7-point), vorticity confinement, buoyancy | named "Stable Fluids" lineage (Stam) implicit in shader comments; **not** registered | MEDIUM — per-cell grid state easy to hash; analytic Poisson test (divergence-free input → zero pressure delta) feasible; no vendored upstream → derivation lives in load-bearing-decisions |
| `volumetric-grid/lattice-boltzmann` | (unimplemented) | Planned D3Q19 LBM around airfoil | tracked only in spec sheet | N/A |

**INFERENCE:** 10/14 sim directories are implemented; 4 are README-only
placeholders. Only one implemented sim (`sph-water`) has a registered
ground-truth upstream — every other sim relies on README citations the
`cat1.unregistered-upstream` check would treat exactly like LeniaNDK.
v1.1 Cat 3 expansion is therefore bottlenecked on the same vendoring
decision documented under `ground-truth-sources.md` § "Not yet registered".

## Constraints check (self-attestation)

- Toolkit code, configuration, and grandfather catalog **not modified**.
- Grandfather sweep **not run** (only `python3 -m integrity` and `gh api` used).
- `references/` tree **not modified** (only `git rev-parse` / `git describe`).
- All commands succeeded except `python` (no `python` shim; substituted
  `python3`, reported verbatim in § C.1). Target length 200–400 lines.
