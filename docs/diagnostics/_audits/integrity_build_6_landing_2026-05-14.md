# Integrity Toolkit — Commit 6 Landing — 2026-05-14

Sixth of eight commits building the cross-stack integrity verification
toolkit per `docs/integrity-toolkit-spec.md` § 11. Implements
`cat2.public-symbol-used-c` against Stack C (C++/Vulkan) via libclang.
Most technically involved commit in the build sequence — libclang's
cursor API differs significantly from stdlib `ast`, and chained
member-access expressions surface a libclang AST quirk that needed a
token-based fallback.

Companion to:

- Spec: `docs/integrity-toolkit-spec.md` § 7.2, § 7.3 (Stack C surface)
- Prior commit's audit: `integrity_build_5_landing_2026-05-14.md`

---

## A. Change summary

`cat2.public-symbol-used-c` against Stack C. Implementation uses
libclang via the `clang.cindex` Python bindings. The check parses
the union of `common/common-cpp/examples/hello/main.cpp` plus every
`.cpp` under `common/common-cpp/src/` so the combined `#include`
transitive closure covers every public header in
`common/common-cpp/include/gpusims/`. Each translation unit
contributes symbols; results are deduped by `(file, line, name,
kind)` to handle libclang's tendency to assign distinct USRs to the
same logical symbol under different namespace contexts.

Reference finding is two-pass per consumer TU:

1. **Cursor walk** via `cursor.referenced.get_usr()`. Catches
   classes, free functions, methods, and most member-access cases.
   Skips declaration-kind cursors (FUNCTION_DECL, FIELD_DECL, etc.)
   so the symbol's own definition doesn't count as a "consumer."
2. **Token scan** for `.field` / `->field` accesses. Libclang hides
   intermediate member-refs in `UNEXPOSED_EXPR` wrappers on chained
   access (`f.positions.size()` — the inner `.positions` cursor is
   not exposed in the AST walk). The token pass walks raw tokens,
   identifies `.field` / `->field` patterns, and counts them. This
   matches Stack D's AST-attribute semantics: name-collision evasion
   is a known false-MISS direction (acceptable per spec for v1).

The check correctly identifies all three spec § 12 canonical Stack C
defects: `gpusims::vdb::writeVec3Grid` (defined-but-never-called),
`gpusims::abc::ParticleFrame::radii` (silent-data-loss field), and
`gpusims::vk::Buffer::deviceAddress` (no current consumers).

## B. File inventory

| File | Status | Notes |
|------|--------|-------|
| `tools/integrity/integrity/cat2_contracts/stack_c.py` | new | Libclang extractor + reference finder + dedup |
| `tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_c.py` | new | HARD_FAIL check, graceful degrade |
| `tools/integrity/integrity/cat2_contracts/checks/__init__.py` | modified | Adds Stack C check to REGISTERED_CHECKS |
| `tools/integrity/integrity/grandfather.py` | modified | New `cat2-stack-c-unused` classifier rule |
| `tools/integrity/docs/grandfather-catalog.md` | modified | New category section |
| `tools/integrity/tests/test_cat2_stack_c.py` | new | 7 unit tests with libclang-runtime skipif |
| `tools/integrity/tests/fixtures/good_contracts_c/**` | new | Synthetic happy-path fixture (no stdlib deps) |
| `tools/integrity/tests/fixtures/bad_contracts_c/**` | new | Synthetic defect-class fixture |
| 18 public-header source files | modified | Grandfather sweep annotations |

## C. Verification

### libclang environment

Local: `libclang 18.1.1` Python package; `clang.cindex.Index.create()`
succeeds. The bundled `libclang/native/libclang.so` ships with the
PyPI wheel — no system `libclang-dev` install required.

`build/compile_commands.json` produced by the existing cmake configure
step (4b CI workflow). 757KB; first entry inspected.

### Pytest output (59/59 pass)

52 prior tests + 7 new Stack C tests, all green locally:

```
tests/test_cat2_stack_c.py::test_extract_public_surface_finds_class_and_function PASSED
tests/test_cat2_stack_c.py::test_extract_public_surface_enumerates_fields PASSED
tests/test_cat2_stack_c.py::test_good_contracts_c_yield_no_findings PASSED
tests/test_cat2_stack_c.py::test_bad_contracts_c_flag_unused_radii PASSED
tests/test_cat2_stack_c.py::test_bad_contracts_c_flag_unused_function PASSED
tests/test_cat2_stack_c.py::test_used_symbols_not_flagged PASSED
tests/test_cat2_stack_c.py::test_missing_compile_commands_returns_empty PASSED

============================== 59 passed in ~10s ==============================
```

### Cat 2 Stack C smoke run (against the real repo, pre-sweep)

```
time python3 -m integrity --check cat2.public-symbol-used-c --output human

(216 findings before dedup, 111 unique after (file,line,name,kind) dedup)

real    1m35s
user    1m32s
sys     0m2s
```

Within the spec § 1.3 per-check 2-minute budget; well within CI's
10-minute job timeout.

### Spec § 12 canonical-defect hits

| Spec § 12 row | Detection |
|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `vdb::writeVec3Grid` unexercised real impl | HIT — `vdb_writer.hpp:33`, "no non-self consumer site" |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `ParticleFrame::radii` silent data-loss | HIT — `alembic_writer.hpp:24`, class_field with no consumer |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| `Buffer::deviceAddress` (Layer 2 audit defect) | HIT — `vk/buffer.hpp:62`, public method, no consumer |

### Finding breakdown (111 unique)

- `method`: 89 (mostly `gpusims::vk::*` accessors and lifecycle methods)
- `class_field`: 5 (`ParticleFrame::radii`, `CompileResult::includes`, etc.)
- `free_function`: 3 (`writeVec3Grid`, `initFrame`, `destroyFrame`)
- `struct`: 1 (`CompileResult` itself)
- 13 more methods on `Window`, `Renderer`, `ShaderCompiler` etc.

### Grandfather sweep

```
grandfather-sweep: modified 19 files; 113 annotations added
                  cat2-stack-c-unused: 111
                           other-cat1: 2
```

The 111 Cat 2 Stack C findings classify under the new
`cat2-stack-c-unused` category. The 2 other-cat1 are unrelated drift
from a new audit line.

### Post-sweep strict-mode result

```
integrity: 1 pass, 0 soft-warn, 0 hard-fail, 1033 suppressed
Exit: 0
```

### CI run

The first CI run on commit `5a1c193` failed at the dogfood test step
because the Stack C fixtures used `std::string` and `std::vector` —
in the Actions runner's libclang environment the C++ standard library
headers weren't reachable from the fixture's default include paths,
which caused the TU parse to silently produce a corrupted AST (no
findings emitted, both bad-fixture assertions failed).

The fix-up commit `b0f7bce` replaces `std::string` with `int` and
`std::vector` with `float*` in the fixtures. The defect-class
coverage is unchanged (`radii_ptr` declared-but-never-read,
`unused_function` declared+defined-but-never-called). CI status for
`b0f7bce` reported in the final landing output.

## D. Behavioral notes

- The Cat 2 Stack C check is now part of every `python -m integrity`
  invocation. Stack C runtime (~95s) is the bottleneck of a full
  toolkit run; Stack D and Cat 1 are <1s combined.
- New cat2 findings on future commits land as inline PR annotations
  via `--output github`.
- The CI workflow doesn't need changes: `compile_commands.json` is
  already produced by the existing cmake configure step (commit 4b).
- libclang 18.1.1 is pinned in `pyproject.toml`; the PyPI wheel ships
  its own libclang.so so CI doesn't need system `libclang-dev`.

## E. Preview of commit 7 — Cat 2 Stack B contract verification

Commit 7 lands `cat2_contracts/stack_b.py` for TypeScript via the TS
compiler API. Per spec § 7.3, this requires a Python subprocess to a
small TS helper script that emits JSON over stdout. The TS helper
will live at `tools/integrity/integrity/cat2_contracts/ts_helper.ts`
and be invoked via `node` (already present in the build-web CI
toolchain). The contract semantics are the same as Stack C / Stack D:
every export from `common/common-web/src/index.ts` must have at least
one consumer reference.

The implementation will be smaller than Stack C because the TS
compiler API exposes references directly (no UNEXPOSED_EXPR
equivalent). Performance should be fast (TS is happy parsing the
small common-web surface).

## F. Incidental findings

### F.1. Libclang hides chained member-refs in UNEXPOSED_EXPR

The most surprising finding of commit 6. For an expression like
`f.positions.size()`, the AST walk visits:

```
CALL_EXPR (size)
  MEMBER_REF_EXPR ''     # spelling empty, ref=std::vector::size
  UNEXPOSED_EXPR ''       # spelling empty, ref=None — this hides `.positions`!
    DECL_REF_EXPR 'f' ref=f
```

The inner `MEMBER_REF_EXPR(positions)` cursor is buried inside the
`UNEXPOSED_EXPR` wrapper and not exposed by `get_children()`.
`cursor.referenced` on the wrapper returns None. `Cursor.from_location`
at the `positions` token also returns the wrapper.

Fix: token-based fallback specifically for class fields. For each
`.field` / `->field` token sequence in any consumer TU, count as a
reference. Loses USR precision (class-name disambiguation) but
matches Stack D's AST-attribute semantics. Documented as a known
false-MISS class.

This is the same defect class Stack D had — Python's `ast` also
treats `f.positions.size()` as `Attribute(attr=positions, ctx=Load)`
followed by a method call, which works for Stack D's purposes. The
libclang quirk arises because C++'s AST is more deeply nested.

### F.2. Per-stack check IDs

I'm using `cat2.public-symbol-used-c` for the Stack C variant and
`cat2.public-symbol-used` (unsuffixed) for Stack D. Spec § 7.4 names
the check generically; the per-stack suffix is an implementation
detail. Rationale: distinct IDs let grandfather suppressions target
the right stack and let failure messages cite the right diagnostic
context (libclang vs. Python ast have different false-positive/negative
profiles). Stack B (commit 7) will continue the pattern with
`cat2.public-symbol-used-ts`.

Spec touch-up worth considering: explicitly document the per-stack
suffix convention in spec § 7.4. Deferred to commit 8's spec edits.

### F.3. USR variation across translation units

The same public symbol can get distinct USRs when parsed under
different TU contexts. For example, `Buffer::deviceAddress` was
extracted with qualified names `std::gpusims::vk::Buffer::deviceAddress`
in one TU and `gpusims::vk::Buffer::deviceAddress` in another. The
`std::` prefix is a cosmetic artifact of libclang's `semantic_parent`
walk in TUs that include `<std>` somewhere; the underlying USR is
also slightly different.

Fix: deduplicate symbols by `(file, line, name, kind)` after all TUs
are processed. Reduces 216 raw extractions to 111 unique symbols.

The cosmetic `std::` prefix in qualified names remains in error
messages and grandfather annotations. Quality-of-output issue, not
correctness — deferred to a future pass.

### F.4. Stack C smoke wall-clock is ~95s

Inside the per-check budget. Parsing every `.cpp` in `common-cpp/src/`
takes 25-40s for extraction; another 50-60s for the reference-finding
pass across consumer TUs (~10 files). Optimization avenues for v1.1:

- Cache parse results across check invocations (libclang has
  precompiled-header support)
- Parallelize TU parses (each parse is independent)
- Use clang-tidy's symbol index if available

None of these are needed for v1.

### F.5. Fixtures must avoid stdlib headers

The CI Actions runner's libclang environment couldn't find
`<string>` or `<vector>` from the fixture's default include paths.
Symptom: TU parses succeed but with corrupted ASTs (no usable
references emitted). Diagnostic: locally `pytest` ran the bad-fixture
tests in 6.7s (heavy stdc++ parse) but CI ran in 10.3s with two
failing assertions.

Fix-up commit `b0f7bce` rewrote fixtures to use plain `int` and
`float*` types. Same defect-class coverage; tests now run in <0.1s.
Worth remembering for future Stack C fixture additions: avoid stdlib
dependencies unless explicitly necessary, and pass `-resource-dir` if
they are.
