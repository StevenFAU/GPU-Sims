# Integrity Toolkit — Grandfather Catalog (v1)

This document records the pre-v1 findings that were grandfathered into the
toolkit's strict-mode gate when commit 4a landed. Categories below map to
the rules in `tools/integrity/scripts/grandfather_sweep.py` (and the
classifier in `tools/integrity/integrity/grandfather.py`).

The toolkit will continue to gate CI strictly on any NEW findings introduced
after this commit. Grandfathered findings are suppressed via inline
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
`integrity-allow:` annotations per spec § 3.2.

## Updating counts

The per-category counts in the headings below reflect the most recent
run of the auto-refresh script:

    python3 tools/integrity/scripts/refresh_catalog_counts.py

The script reads `python3 -m integrity --grandfather-report
--no-history-append`'s output and rewrites each numeric `(N)`
parenthetical in this file's category headings. Non-numeric
parentheticals (e.g. `(?)` placeholders or free-prose forms like
`(0 swept; 44 live-source skipped)`) are preserved verbatim.

To preview proposed changes without writing:

    python3 tools/integrity/scripts/refresh_catalog_counts.py --dry-run

The script errors out if any category in the report lacks a
corresponding heading in this file — that signals a missed
grandfather-catalog entry that needs human authoring (the catalog's
per-category prose explains WHY each category is grandfathered and
should not be mechanically stubbed).

Run the refresh whenever catalog counts drift far enough to mislead
a reader. The v1.3 candidates roadmap § 4 T1.3 banked the
auto-refresh as the resolution for the manual-refresh debt v1.1
batch-1 retro § 5.5 quantified at +6.7% drift per batch cycle.

## Categories

### `audit-citation` (101)

**Pattern:** `cat1.intra-repo` findings in files under `docs/diagnostics/_audits/`.

**Why grandfathered:** Audit reports are snapshots of the codebase at a specific
moment. Citations were valid at audit time; subsequent code drift made some
unresolvable. Audit reports are append-only by convention; retroactively
editing them would erase the historical record.

**Future treatment:** Permanent suppression. New audit reports landing after
v1 may reference paths-that-no-longer-exist; if so, those new citations get
the same suppression at write-time.

### `live-shader-1810` (3)

**Pattern:** `cat1.upstream-citation` findings citing `SPlisHSPlasH 1.8.10`
in live code under `particle-fluids/sph-water/shaders/` or
`particle-fluids/sph-water/src/`.

**Why grandfathered:** The Phase 11.5 setup-1 audit established that the
`1.8.10` anchor was fabricated; the vendored upstream is `2.16.1`. The
live citations in shaders and host code were copied from pre-setup-1 drafts
and still use the old version label. Rewriting them to `2.16.1` is a real
migration item but separate from the integrity toolkit's v1 landing.

**Tracking:** Entries are the migration target; when sph-water's
load-bearing-decisions.md and shader headers are next edited, the
citations should be updated to `2.16.1` and the suppressions removed.

**Future treatment:** Remove suppression on each citation when the
corresponding shader/source file is next modified for unrelated reasons.

### `audit-doc-1810` (17)

**Pattern:** `cat1.upstream-citation` findings citing `SPlisHSPlasH 1.8.10`
in any file NOT under `particle-fluids/sph-water/shaders/` or
`particle-fluids/sph-water/src/`.

**Why grandfathered:** Audit reports and spec docs reference the historical
fabrication intentionally — they document that `1.8.10` was the wrong
anchor. Migrating these citations to `2.16.1` would erase the historical
record of what was wrong.

**Future treatment:** Permanent suppression.

### `spec-grammar-example` (18)

**Pattern:** `cat1.annotation-form` findings in `docs/integrity-toolkit-spec.md`
or files under `tools/integrity/docs/`.

<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
**Why grandfathered:** The spec and toolkit docs include `integrity-allow:`
strings as illustrative grammar examples (in tables, in prose, in code
fences). The `cat1.annotation-form` check parses every such literal as if
it were a real annotation and validates the grammar. Many of these examples
deliberately demonstrate invalid grammar (the "bad" examples) and would
always fail the check.

**Future treatment:** Permanent suppression on these docs.

### `toolkit-own-source` (25)

**Pattern:** `cat1.annotation-form` findings in files under
`tools/integrity/integrity/`.

**Why grandfathered:** The toolkit's own source code references the
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
`integrity-allow:` grammar in docstrings and regex literals (specifically
in `common/annotations.py` and the checks that look for the grammar).
These are not real annotations; they are the parser definition itself.

**Future treatment:** Permanent suppression on these files.

### `retro-grammar-example` (8)

**Pattern:** `cat1.annotation-form` findings in files under `docs/retro/`.

**Why grandfathered:** Retrospective documents describe the toolkit's
own grammar in prose. The `cat1.annotation-form` check parses every
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
`integrity-allow:` literal as if it were a real annotation; literals
inside retro-doc prose are documentation, not annotations. Same
reason class as `spec-grammar-example`.

**Future treatment:** Permanent suppression on these docs.

### `audit-report-grammar-example` (51)

**Pattern:** `cat1.annotation-form` findings in files under
`docs/diagnostics/_audits/`.

**Why grandfathered:** Audit reports document toolkit findings, which means
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
they quote `integrity-allow:` strings. Same reason class as
`spec-grammar-example`.

**Future treatment:** Permanent suppression.

### `other-cat1` (36)

**Pattern:** Any other `cat1.*` finding not matched by the rules above.

**Why grandfathered:** Catch-all for the long tail. If this category has
non-trivial entries after the sweep, they should be inspected case-by-case
and likely promoted to a more specific category.

**Future treatment:** Per-entry review in v2.

### `cat2-stack-d-unused` (17)

**Pattern:** `cat2.public-symbol-used` findings against Stack D's public
surface (commit 5).

**Why grandfathered:** The commit-5 smoke run surfaced 17 Stack D public
symbols with no current consumer. Sample shapes:

- `ParticleFrame.{positions, velocities, radii, ids}` — fields written
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
<!-- integrity-allow: cat1.bare-path; toolkit-doc bare-path citation pre-v1.2 (see grandfather-catalog toolkit-doc-bare-path); n/a -->
  via the dataclass constructor in `alembic_writer.py:96`
  (`ParticleFrame(positions=x_np, count=n)`) but never read by
  `AlembicWriter.write_frame` because the writer is permanent-stub
  mode for Phase 9. The canonical defect class per spec § 12.
- `CameraInputState` and its 10 fields — declared API, no current
  caller. v1.1 input-state plumbing.
- `get_logger` — only `log` (the proxy) is currently consumed; the
  factory remains for v1.1 advanced-logging callers.
- `write_float_frame`, `write_float_grid` — VDB writer free
  functions queued for sim integration.

These are precisely the fabrication shapes the toolkit was designed
to catch, including the canonical `ParticleFrame.radii` instance from
spec § 12. They are grandfathered (not blocked) so commit 5 can land;
future commits will either provide consumers (real-mode Alembic in
Phase 11+, CameraInputState wiring in v1.1) or trim the surface.

**Future treatment:** Per-symbol review when the corresponding
sim/feature lands. Remove suppression as the consumer wires up.
Permanent suppressions are NOT expected for this category — every
entry has an intended consumer.

### `cat2-stack-c-unused` (110)

**Pattern:** `cat2.public-symbol-used-c` findings against Stack C's
public surface (commit 6).

**Why grandfathered:** The commit-6 smoke run surfaced 111 Stack C
public symbols (classes, structs, fields, free functions, methods)
declared in `common/common-cpp/include/gpusims/` with no current
consumer in `common-cpp/src/`, `common-cpp/examples/`, or per-sim
Stack C source. Includes the canonical spec § 12 defects:

<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
<!-- integrity-allow: cat1.bare-path; toolkit-doc bare-path citation pre-v1.2 (see grandfather-catalog toolkit-doc-bare-path); n/a -->
- `gpusims::vdb::writeVec3Grid` — declared in `vdb_writer.hpp:33`,
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
  implemented at `src/vdb_writer.cpp:97`, never called.
- `gpusims::abc::ParticleFrame::radii` — declared in
  `alembic_writer.hpp`, written by constructor in the host code at
  `src/main.cpp` but never read by `AlembicWriter::writeFrame`
  (the Stack C twin of the Stack D defect).
- `gpusims::vk::Buffer::deviceAddress` — declared in `vk/buffer.hpp`,
  no current consumer.

The remaining entries are mostly `gpusims::vk::*` methods on
`Window`, `Renderer`, `Image`, `ShaderCompiler`, etc. — pieces of the
Vulkan abstraction layer that are exposed for v1.1 sim integration
but not yet wired by any of the four landed sims (sph-water,
eulerian-smoke, reaction-diffusion-3d, mpm-multimaterial Phase 9
state).

**Future treatment:** Per-symbol review as sims consume the API.
Suppressions should dissolve naturally; the canonical
`writeVec3Grid` and `radii` instances are the migration markers
flagged in spec § 12.

### `cat2-stack-b-unused` (73)

**Pattern:** `cat2.public-symbol-used-ts` findings against Stack B's
public surface (commit 7).

**Why grandfathered:** The commit-7 smoke run surfaced 73 Stack B
public symbols (types, class properties, methods) exported from
`common/common-web/src/index.ts` with no current consumer in any
Stack B sim, example, or test. Breakdown:

- 33 methods on `Camera`, `Buffer`, `Texture`, `ComputePipeline`,
  `RenderPipeline`, `Renderer` — abstraction-layer surface exposed
  for the seven landed Stack B sims (strange-attractors,
  mandelbulb-explorer, reaction-diffusion-2d, physarum, boids-3d,
  lenia-fft, neural-ca) but not all touched.
- 25 properties on `Camera`, `GpuProfiler`, `ParamPanel`, etc. —
  same pattern.
- 4 type aliases (`Vec2`, `Vec3`, `Vec4`, `Mat4`) — exported as
  convenience types; sims use their own local Vec types or rely on
  the WebGPU API's `Float32Array` directly. v1.1 candidate for
  trimming the surface or wiring as the canonical name.

Stack B uses the TypeScript compiler API (type-aware), so the
name-collision false-positive class that Stack D and Stack C have
does not apply. Detection is precise.

**Future treatment:** Per-symbol review as sims consume the API.
Suppressions should dissolve as sim code wires the abstraction.

### `cat2-stub-label-stale` (2)

**Pattern:** `cat2.stub-label-stale` findings -- `In Phase N, this is a stub:`
labels where the corresponding implementation has more than 10 non-comment
LOC.

**Why grandfathered:** Two canonical cases exist in the repo as of v1.1
landing: `common/common-cpp/include/gpusims/alembic_writer.hpp:11`
(impl 99 non-comment LOC) and `common/common-cpp/include/gpusims/vdb_writer.hpp:12`
(impl 135 non-comment LOC). Both labels were carried over from Phase 1
when the surfaces were genuine stubs; subsequent Alembic and OpenVDB
enablement work landed real implementations without revising the header
labels. These are migration targets, not permanent suppressions.

**Tracking:** Two entries. Both have known migration paths: when the
headers are next edited for unrelated reasons, the "In Phase 1, this is
a stub:" framing should be replaced with the runtime-mode discriminator
shape used in the Stack D twins (e.g., `// Real-or-stub depending on
GPU_SIMS_HAVE_ALEMBIC`).

**Future treatment:** Remove suppression on each header when the
header is next modified. Permanent suppressions are not expected.

### `toolkit-own-unused` (24)

**Pattern:** `cat2.public-symbol-used-toolkit` findings -- top-level
public `def` or `class` symbols declared in
`tools/integrity/integrity/**/*.py` with no consumer in
`tools/integrity/{integrity,scripts,tests}/`. Per v1.2 A.2 Decision 2
strict scan rules: module-level constants, underscore-prefixed names,
`visit_*` methods on `ast.NodeVisitor` subclasses, `test_*` functions,
and `main`/`__main__` entrypoint names are NOT scanned. Per Decision 3:
the `tests/`, `tests/fixtures/`, and `scripts/` subdirectories are
included as scan-input (their imports count as consumption) but
excluded as scan-target (their own top-level symbols are not scanned
for being unused).

**Why grandfathered:** v1.2 A.2 introduces the toolkit self-application
check that closes the recursive blind spot named in v1.1 batch-1 retro
section 5.3. Findings present at A.2 landing time are pre-existing
declared-public-but-unused symbols accumulated across batches 0 through
1.x; the v1.2 baseline sweep grandfathers them. Going forward, any new
public symbol added to the toolkit must have a non-self consumer or
carry a per-entry tracking note here.

**Tracking notes:**

- `stack_paths()` at `tools/integrity/integrity/common/stack_paths.py`
  is the first tracked entry. Per v1.2 A.2 Decision 7: declared as a
  helper for future stack-config consolidation; tracked for v1.3
  consolidation work. When v1.3 wires `stack_paths()` into the Stack
  checks (consolidating the three pairs of duplicated path constants),
  this entry will resolve and the annotation can be removed.

**Future treatment:** Per-entry review during v1.3 stack-config
consolidation. Most entries are runner / classifier / generator
internals consumed only within the defining module; the spec's
strict-scan rule produces a true positive in those cases (a public
shape with no cross-module consumer), but the right fix is usually
underscore-prefix renaming rather than wiring up a new consumer.
Auto-refresh of this section's count is banked for v1.3 (Decision 6
keeps refresh manual in v1.2).

### `audit-bare-path` (747)

**Pattern:** `cat1.bare-path` findings in files under
`docs/diagnostics/_audits/`.

<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
**Why grandfathered:** Audit reports cite repo files and upstream files
by bare basename as a documentation convention. The v1.2 `cat1.bare-path`
check requires fully-qualified paths or upstream-registry citations;
audit-doc bare paths are append-only by the same convention as
`audit-citation`. New audit reports may continue to cite bare paths
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
with `integrity-allow:` annotations applied via the grandfather sweep.

**Future treatment:** Permanent suppression on audit-doc paths. The
v1.3 may revisit whether new audit reports should use full paths.

### `retro-bare-path` (18)
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->

**Pattern:** `cat1.bare-path` findings in files under `docs/retro/`.

**Why grandfathered:** Retro documents narrate cross-phase history and
cite files by bare basename to keep prose readable. Same rationale as
`audit-bare-path`.

**Future treatment:** Permanent suppression on retro paths.

### `toolkit-doc-bare-path` (7)

**Pattern:** `cat1.bare-path` findings in `docs/integrity-toolkit-spec.md`,
`tools/integrity/docs/**`, or `tools/integrity/README.md`.

**Why grandfathered:** Toolkit-internal documentation cites toolkit
modules by bare basename when discussing the toolkit's own
implementation. Same rationale as `spec-grammar-example`.

**Future treatment:** Permanent suppression on toolkit-doc paths.

### `deferred-upstream-bare-path` (5)

**Pattern:** `cat1.bare-path` findings whose message text references
`LeniaNDK.py`.

**Why grandfathered:** The Chakazul/Lenia upstream is not yet vendored
per `tools/integrity/docs/ground-truth-sources.md` "Not yet registered"
section. Citations to `LeniaNDK.py:<line>` are known-pending vendoring;
once vendored, these will become valid registered-upstream citations.
Suppressing under a named category (rather than `other-cat1-bare-path`)
keeps the category-pool drain measurable.

**Future treatment:** Remove suppression on each citation when the
Chakazul upstream is registered.

### `other-cat1-bare-path` (0 swept; 44 live-source skipped)

**Pattern:** `cat1.bare-path` findings not matching any of the four
specific rules above.

**Why grandfathered:** Fall-through bucket for bare-path findings in
live-source paths (sim code, common-* code, phase specs under
`docs/phase*.md`). These should be addressed by their introducing
authors per the v1.1 batch-1 triage hybrid policy: live-source bare
paths get attributed to authors and fixed, not swept.

**Future treatment:** Each entry should resolve as its introducing
author rewrites the citation. The category-count drain is the signal
to monitor. P1.8 (grandfather-sweep live-source protection) ensures
these aren't auto-swept in v1.2+.

## Suppression-annotation discipline

Each suppressed finding has an inline annotation per spec § 3.2:

<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
    integrity-allow: <check-id>; <reason from category>; n/a

Issue-refs are `n/a` for v1 grandfather suppressions; no per-finding GitHub
issues were created. The `live-shader-1810` category is the only one with
intended future cleanup (when those citations are next edited), and it is
tracked at the category level rather than per-annotation.

## Removing a suppression

When the underlying finding is fixed (e.g., a citation is updated, a
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
field gains a consumer), remove the corresponding `integrity-allow:`
annotation. The toolkit's CI check will pass without it.
