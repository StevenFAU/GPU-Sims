# Integrity Toolkit — Commit 5 Landing — 2026-05-14

Fifth of eight commits building the cross-stack integrity verification
toolkit per `docs/integrity-toolkit-spec.md` § 11. Implements
`cat2.public-symbol-used` against Stack D (Python/Taichi) via the
stdlib `ast` module. First Category 2 check to land.

Companion to:

- Spec: `docs/integrity-toolkit-spec.md` § 7 (Cat 2 specification)
- Prior commit's audit: `integrity_build_4b_landing_2026-05-14.md`

---

## A. Change summary

`cat2.public-symbol-used` against Stack D — the simplest of the three
per-stack approaches because Python's stdlib `ast` module suffices.
Stack C (libclang) and Stack B (TS compiler API) follow in commits 6
and 7.

The check parses `common/common-py/gpusims_common/__init__.py`,
extracts every re-exported symbol, locates its definition in the
corresponding `.py` file, and enumerates fields/methods/free
functions. It then walks every `.py` file in the repo (minus
exclusions and minus the symbol's own defining file) for AST `Load`
references — `Attribute(attr=name)` for class fields, `Name(id=name)`
and `Attribute(attr=name)` for free symbols. For class fields, the
visitor tracks class scope so reads inside the field's defining class
don't count (spec § 7.4: "non-trivial consumer site outside its
defining class").

The check correctly identifies the canonical `ParticleFrame.radii`
defect class from spec § 12, plus 16 other pre-v1 Stack D symbols
with no current consumer.

## B. File inventory

| File | Status | Notes |
|------|--------|-------|
| `tools/integrity/integrity/cat2_contracts/stack_d.py` | new | Public-surface extractor + reference finder |
| `tools/integrity/integrity/cat2_contracts/checks/public_symbol_used.py` | new | The HARD_FAIL check |
| `tools/integrity/integrity/cat2_contracts/__init__.py` | new content | Module docstring |
| `tools/integrity/integrity/cat2_contracts/checks/__init__.py` | new content | REGISTERED_CHECKS list |
| `tools/integrity/integrity/runner.py` | modified | Wires Cat 2 into `discover_checks` |
| `tools/integrity/integrity/grandfather.py` | modified | Adds `cat2-stack-d-unused` classifier rule |
| `tools/integrity/docs/grandfather-catalog.md` | modified | Documents the new category |
| `tools/integrity/tests/test_cat2_stack_d.py` | new | 6 unit tests |
| `tools/integrity/tests/fixtures/good_contracts/**` | new | Synthetic happy-path fixture |
| `tools/integrity/tests/fixtures/bad_contracts/**` | new | Synthetic ParticleFrame.radii defect mirror |
| `docs/integrity-toolkit-spec.md` | modified | § 9.1 apt list aligned with working CI set |
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
| 4 Stack D source files | modified | Inline `integrity-allow:` annotations for the 17 grandfathered findings |

## C. Verification

### Pytest output (52/52 pass)

46 prior tests + 6 new Stack D tests, all green:

```
tests/test_cat1_annotation.py .....                                      [  9%]
tests/test_cat1_intra_repo.py .......                                    [ 23%]
tests/test_cat1_unregistered.py ...                                      [ 28%]
tests/test_cat1_upstream.py ....                                         [ 36%]
tests/test_cat1_upstream_anchor.py ...                                   [ 42%]
tests/test_cat2_stack_d.py ......                                        [ 53%]
tests/test_grandfather_sweep.py ...................                      [ 90%]
tests/test_runner.py .....                                               [100%]

============================== 52 passed in 0.08s ==============================
```

### Cat 2 smoke run (against the real repo, pre-sweep)

`python -m integrity --check cat2.public-symbol-used --output human`
surfaced 17 HARD_FAIL findings:

| Defining file | Symbol | Kind |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `alembic_writer.py:43` | `ParticleFrame.positions` | class field |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `alembic_writer.py:45` | `ParticleFrame.velocities` | class field |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `alembic_writer.py:46` | `ParticleFrame.radii` | class field |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `alembic_writer.py:47` | `ParticleFrame.ids` | class field |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `camera.py:34` | `CameraInputState` | class |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `camera.py:43-53` (×10) | `CameraInputState.{key_w, key_a, key_s, key_d, key_q, key_e, shift_held, mouse_right, mouse_middle}` | class fields |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `log.py:43` | `get_logger` | function |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `vdb_writer.py:60` | `write_float_grid` | function |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `vdb_writer.py:95` | `write_float_frame` | function |

Every one is a canonical pre-v1 fabrication shape per spec § 12. The
`ParticleFrame.{positions, velocities, radii, ids}` set is the
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
written-but-not-read defect: `alembic_writer.py:96` constructs frames
via `ParticleFrame(positions=x_np, count=n)` (kwarg passes, no field
read) but `AlembicWriter.write_frame` is a permanent-stub for Phase 9
and never accesses any field of its argument.

### Grandfather sweep (commit-5-specific)

```
grandfather-sweep: modified 5 files; 18 annotations added
                  cat2-stack-d-unused: 17
                           other-cat1: 1
```

The 17 Cat 2 findings classify under the new `cat2-stack-d-unused`
category. The 1 other-cat1 is unrelated drift (the spec patch's apt
list change added a new line that the cat1 grammar caught — handled
by the sweep automatically). The sweep is idempotent.

### Post-sweep strict-mode result

```
integrity: 1 pass, 0 soft-warn, 0 hard-fail, 908 suppressed
Exit: 0
```

908 = 888 (post-4b) + 17 cat2 + 3 follow-up cat1 from the spec patch
and audit-doc adjustments. Toolkit's own tests still pass.

### CI run

See § D below — run status reported in the final landing output.

## D. Behavioral notes

- The Cat 2 check is now part of every `python -m integrity` invocation
  (including the CI workflow). Cat 1 and Cat 2 run together by default.
- New cat2 findings on future commits land as inline PR annotations
  via `--output github`.
- The `cat2-stack-d-unused` category has 17 entries at v1 landing.
  Per the catalog: every entry has an intended consumer; suppressions
  are expected to dissolve as the consumers wire up (Phase 11+ for
  real-mode Alembic, v1.1 for `CameraInputState` plumbing, sim
  integration for `write_float_*`).

## E. Preview of commit 6 — Cat 2 Stack C contract verification

Commit 6 lands `cat2_contracts/stack_c.py`, the libclang variant.
Public surface = symbols declared in headers under
`common/common-cpp/include/gpusims/`. Implementation lives in
`common/common-cpp/src/`. Lessons from commit 5 that should carry
over:

- Class-scope tracking is the only non-trivial AST traversal.
  libclang exposes cursor parenting that mirrors `ast.NodeVisitor`'s
  class-stack pattern.
- Field-vs-function distinction is uniform across the check; the
  rendered message is `"public {kind} '{descriptor}'..."` and
  remains stable.
- Real defects (`vdb::writeVec3Grid` impl-not-called) are the same
  shape as Stack D's free-function findings; the message format
  carries over.

Stack C will surface its own pre-v1 fabrication set (`writeVec3Grid`,
maybe a few more in the common-cpp public API). Those will join a
new `cat2-stack-c-unused` grandfather category.

## F. Incidental findings

### F.1. AST extractor handles both relative and absolute imports

`gpusims_common/__init__.py` uses absolute imports
(`from gpusims_common.alembic_writer import AlembicWriter`), not the
spec's suggested relative form (`from .alembic_writer import ...`).
The extractor was adjusted to handle both: `level == 1` for relative,
or `level == 0` with `module.startswith("gpusims_common.")` for
absolute. Worth noting as a small spec/implementation divergence
that hit zero because the implementation was generalized
preemptively.

### F.2. Module-level instance assignment (`log = _LogProxy()`)

The `log` re-export is bound via `log = _LogProxy()` at module
top-level, not via `class log:` or `def log():`. The extractor
adds an `ast.Assign` branch so module-level name assignments classify
as `MODULE` kind. Without this, the `log` name would have been
silently skipped, and we'd have lost the most-used Stack D symbol
from the public-surface inventory.

### F.3. Kwarg constructor calls don't count as field reads

`ParticleFrame(positions=x_np, count=n)` passes `x_np` via the
constructor's `positions` parameter — the AST sees a `keyword` node,
not an `Attribute(attr='positions')`. This is correct semantically
(the field is being WRITTEN inside `__init__`, not read), and is the
exact basis on which the check correctly flags ParticleFrame.positions
as unused: every "use" in the codebase is a kwarg pass into the
constructor, never a subsequent read.

The Stack D smoke output therefore correctly identifies that
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`alembic_writer.py:96` is the only construction site, but no
subsequent code reads back the fields. This is the canonical defect
the v1 toolkit was designed to catch.

### F.4. CameraInputState surface has 11 entries flagged together

`CameraInputState` (class) + 10 fields all hit at once because the
class itself is never imported or instantiated anywhere. The
extractor reports each as a separate finding rather than collapsing
to one — this is correct (each is a separate suppression target),
and the grandfather sweep groups them under the same category-level
catalog entry. Future cleanup (when `CameraInputState` gets a
consumer) will dissolve all 11 suppressions at once.

### F.5. Spec § 9.1 apt list patched

Per commit 4b's audit F.1, the spec's example apt list was missing
Vulkan and windowing deps that the repo's CMakeLists.txt requires.
This commit aligns the spec text with the working CI set
(`build-vulkan-validationlayers + libgl1-mesa-dev + libxinerama-dev +
libxcursor-dev + libxi-dev + libxrandr-dev + libwayland-dev +
libxkbcommon-dev + spirv-tools + glslang-tools`). The workflow file
already had the correct set; this is doc-only.
