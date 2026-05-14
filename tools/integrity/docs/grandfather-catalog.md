# Integrity Toolkit — Grandfather Catalog (v1)

This document records the pre-v1 findings that were grandfathered into the
toolkit's strict-mode gate when commit 4a landed. Categories below map to
the rules in `tools/integrity/scripts/grandfather_sweep.py` (and the
classifier in `tools/integrity/integrity/grandfather.py`).

The toolkit will continue to gate CI strictly on any NEW findings introduced
after this commit. Grandfathered findings are suppressed via inline
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
`integrity-allow:` annotations per spec § 3.2.

## Categories

### `audit-citation`

**Pattern:** `cat1.intra-repo` findings in files under `docs/diagnostics/_audits/`.

**Why grandfathered:** Audit reports are snapshots of the codebase at a specific
moment. Citations were valid at audit time; subsequent code drift made some
unresolvable. Audit reports are append-only by convention; retroactively
editing them would erase the historical record.

**Future treatment:** Permanent suppression. New audit reports landing after
v1 may reference paths-that-no-longer-exist; if so, those new citations get
the same suppression at write-time.

### `live-shader-1810`

**Pattern:** `cat1.upstream-citation` findings citing `SPlisHSPlasH 1.8.10`
in live code under `particle-fluids/sph-water/shaders/` or
`particle-fluids/sph-water/src/`.

**Why grandfathered:** The Phase 11.5 setup-1 audit established that the
`1.8.10` anchor was fabricated; the vendored upstream is `2.16.1`. The
live citations in shaders and host code were copied from pre-setup-1 drafts
and still use the old version label. Rewriting them to `2.16.1` is a real
migration item but separate from the integrity toolkit's v1 landing.

**Tracking:** This category has ~10 entries. They are the migration target;
when sph-water's load-bearing-decisions.md and shader headers are next
edited, the citations should be updated to `2.16.1` and the suppressions
removed.

**Future treatment:** Remove suppression on each citation when the
corresponding shader/source file is next modified for unrelated reasons.

### `audit-doc-1810`

**Pattern:** `cat1.upstream-citation` findings citing `SPlisHSPlasH 1.8.10`
in any file NOT under `particle-fluids/sph-water/shaders/` or
`particle-fluids/sph-water/src/`.

**Why grandfathered:** Audit reports and spec docs reference the historical
fabrication intentionally — they document that `1.8.10` was the wrong
anchor. Migrating these citations to `2.16.1` would erase the historical
record of what was wrong.

**Future treatment:** Permanent suppression.

### `spec-grammar-example`

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

### `toolkit-own-source`

**Pattern:** `cat1.annotation-form` findings in files under
`tools/integrity/integrity/`.

**Why grandfathered:** The toolkit's own source code references the
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
`integrity-allow:` grammar in docstrings and regex literals (specifically
in `common/annotations.py` and the checks that look for the grammar).
These are not real annotations; they are the parser definition itself.

**Future treatment:** Permanent suppression on these files.

### `audit-report-grammar-example`

**Pattern:** `cat1.annotation-form` findings in files under
`docs/diagnostics/_audits/`.

**Why grandfathered:** Audit reports document toolkit findings, which means
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
they quote `integrity-allow:` strings. Same reason class as
`spec-grammar-example`.

**Future treatment:** Permanent suppression.

### `other-cat1`

**Pattern:** Any other `cat1.*` finding not matched by the rules above.

**Why grandfathered:** Catch-all for the long tail. If this category has
non-trivial entries after the sweep, they should be inspected case-by-case
and likely promoted to a more specific category.

**Future treatment:** Per-entry review in v2.

### `cat2-stack-d-unused`

**Pattern:** `cat2.public-symbol-used` findings against Stack D's public
surface (commit 5).

**Why grandfathered:** The commit-5 smoke run surfaced 17 Stack D public
symbols with no current consumer. Sample shapes:

- `ParticleFrame.{positions, velocities, radii, ids}` — fields written
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
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
