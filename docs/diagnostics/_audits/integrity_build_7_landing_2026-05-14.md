# Integrity Toolkit — Commit 7 Landing — 2026-05-14

Seventh of eight commits building the cross-stack integrity verification
toolkit per `docs/integrity-toolkit-spec.md` § 11. Implements
`cat2.public-symbol-used-ts` against Stack B (TypeScript/WebGPU) via
the TypeScript compiler API. Completes the Cat 2 surface across all
three stacks. Commit 8 (Cat 3 numerical correctness) is the final
commit.

Companion to:

- Spec: `docs/integrity-toolkit-spec.md` § 7.2, § 7.3 (Stack B surface,
  TS compiler API)
- Prior commit's audit: `integrity_build_6_landing_2026-05-14.md`

---

## A. Change summary

`cat2.public-symbol-used-ts` against Stack B. Architecturally distinct
from Stack C and Stack D: Python spawns a Node subprocess running a TS
helper that loads the TypeScript compiler API directly. The helper
parses `common/common-web/tsconfig.json`, enumerates exports from
`src/index.ts`, walks every project source file for references via
`checker.getSymbolAtLocation`, and emits a single JSON payload on
stdout. Python parses, applies the "non-self consumer" rule, and
produces findings.

The TS compiler API is type-aware out of the box. Unlike Stack C
(libclang's chained-member-ref hiding inside UNEXPOSED_EXPR) and
Stack D (textual attribute matching), Stack B's reference matching
goes through the TS type checker's symbol resolution — name
collisions with unrelated classes are disambiguated correctly.

Notable visitor logic: ImportSpecifier and ExportSpecifier nodes are
skipped during reference walking so that the re-export
`export { unusedFunction }` in index.ts doesn't count as a "consumer"
of `unusedFunction`. Only actual uses (Identifier nodes inside
function bodies, PropertyAccessExpression name fields) count.

The CI workflow gains a setup-node step + an "Install + build TS
helper" step. Node 22 matches the existing build-web and deploy-pages
workflows.

## B. File inventory

| File | Status | Notes |
|------|--------|-------|
| `tools/integrity/integrity/cat2_contracts/stack_b.py` | new | Python subprocess bridge |
| `tools/integrity/integrity/cat2_contracts/ts_helper/extract_and_find.ts` | new | Node TS helper, ~260 lines |
| `tools/integrity/integrity/cat2_contracts/ts_helper/package.json` | new | typescript ~5.5.0 pin |
| `tools/integrity/integrity/cat2_contracts/ts_helper/tsconfig.json` | new | ES2022, strict, includes only the helper |
| `tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_b.py` | new | HARD_FAIL check |
| `tools/integrity/integrity/cat2_contracts/checks/__init__.py` | modified | Registers Stack B check |
| `tools/integrity/integrity/grandfather.py` | modified | Adds cat2-stack-b-unused classifier |
| `tools/integrity/docs/grandfather-catalog.md` | modified | New category section |
| `tools/integrity/tests/test_cat2_stack_b.py` | new | 5 tests with node-availability skipif |
| `tools/integrity/tests/fixtures/good_contracts_b/**` | new | Widget happy-path TS fixture |
| `tools/integrity/tests/fixtures/bad_contracts_b/**` | new | ParticleFrame.radii + unusedFunction defect mirrors |
| `.github/workflows/integrity.yml` | modified | setup-node@v4 + TS helper build steps |
| 15 common-web .ts files | modified | Grandfather sweep annotations |

`tools/integrity/integrity/cat2_contracts/ts_helper/{node_modules,dist}/` covered by repo `.gitignore`.

## C. Verification

### Environment

- Local: Node v22.22.2 (nvm), npm 10.x, typescript ~5.5.0 from local install
- CI: setup-node@v4 with `node-version: '22'` (matches existing build-web)

### Pytest output (64/64 pass)

```
tests/test_cat2_stack_b.py::test_extract_runs_against_good_fixture PASSED
tests/test_cat2_stack_b.py::test_good_contracts_b_yields_no_findings PASSED
tests/test_cat2_stack_b.py::test_bad_contracts_b_flag_unused_radii PASSED
tests/test_cat2_stack_b.py::test_bad_contracts_b_flag_unused_function PASSED
tests/test_cat2_stack_b.py::test_missing_node_returns_empty PASSED

============================== 64 passed in 3.92s ==============================
```

(64 = 59 prior + 5 new Stack B.)

### Stack B smoke (against real repo)

```
time python3 -m integrity --check cat2.public-symbol-used-ts ...

real    0m1.0s
user    0m2.2s
sys     0m0.1s
```

73 findings. Stack B smoke is the fastest of the three Cat 2 variants
(<1s vs Stack D's ~1s vs Stack C's ~95s) — the TS compiler API is fast
and common-web is smaller than common-cpp.

### Per-finding breakdown

```
method:    33  (Camera/Buffer/Texture/ComputePipeline/RenderPipeline/Renderer)
property:  25  (Camera/GpuProfiler/ParamPanel fields)
type:       4  (Vec2 / Vec3 / Vec4 / Mat4 — convenience type aliases)
                                                            (+ misc class/function)
```

Real defect-class hits expected (canonical spec § 12 Stack B
analogues): none surfaced. The spec § 12 table has the canonical
defects for Stack C (writeVec3Grid, radii) and Stack D
(ParticleFrame.radii), not Stack B-specific cases. Stack B has its
own pre-v1 surface-vs-consumer gaps (the 73 above) but none of those
are the spec-named v0 fabrications.

### Grandfather sweep

```
grandfather-sweep: modified 15 files; 73 annotations added
                  cat2-stack-b-unused: 73
```

### Post-sweep strict-mode result

```
integrity: 1 pass, 0 soft-warn, 0 hard-fail, 1109 suppressed
Exit: 0
```

1109 = 1036 (post-commit-6) + 73 cat2 stack-b.

### CI run

Run status for commit `cc9e8c5` reported in the final landing output.

## D. Behavioral notes

- The Stack B check is now part of every `python -m integrity`
  invocation. Full toolkit run order: Cat 1 (~1s) → Stack D (<1s) →
  Stack C (~95s) → Stack B (~1s). Total ~100s; well within the
  10-min CI job timeout.
- Stack B requires the TS helper to be built before invocation. CI
  builds it in a dedicated workflow step; locally, the Python check's
  `ensure_helper_built` runs `npm install` + `tsc` automatically the
  first time the check runs against a real repo. After the initial
  build, subsequent runs reuse `dist/extract_and_find.js`.
- The helper's `dist/` and `node_modules/` are gitignored. CI rebuilds
  them on every run.
- New cat2 findings on future commits land as inline PR annotations
  via `--output github`.

## E. Preview of commit 8 — Cat 3 cubic-kernel numerical correctness

Commit 8 lands the final piece: `cat3.cubic-kernel`. Scope per spec
§ 8.2 / § 8.3:

- A Stack C driver binary (`integrity_cat3_stack_c`) that evaluates
  the cubic SPH kernel at a small set of canonical test inputs and
  emits JSON
- A `cat3_numerical/cubic_kernel.py` check that loads the upstream
  reference values from `references/SPlisHSPlasH/SPHKernels.h` (per
  the registered anchor SHA), runs the driver, and compares
- The `GPU_SIMS_BUILD_INTEGRITY_CAT3` CMake flag from spec § 9.1,
  finally wired
- Spec § 7.4 documentation update formalizing the per-stack check-ID
  suffix convention (deferred from commits 5/6/7)

After commit 8, the toolkit will catch the spec § 12 row 6 defect
(kernel_gradW factor-of-6) by reverting commit 1's kernel-norm fix in
a test branch and asserting cat3.cubic-kernel HARD_FAILs. That's the
v1 acceptance test for Cat 3.

## F. Incidental findings

### F.1. Import / export specifier exclusion

Initial smoke had `unusedFunction` showing 1 reference, which made the
bad-fixture test fail. Root cause: the TS visitor was matching the
`unusedFunction` Identifier inside the `export { unusedFunction }`
statement in `bad_contracts_b/.../src/index.ts`. The re-export
re-binds the name into the module's public surface — it's not a
consumer.

Fix: skip ImportSpecifier and ExportSpecifier nodes in the AST walk.
Identifier matches inside `import { ... }` and `export { ... }`
declarations don't count. Only actual uses count.

Same logic would apply to Stack C and Stack D conceptually, but
neither has this issue because their grammars don't produce
specifier-style nodes — Python's `ast` walks the import statement's
`names` list separately, and libclang's USR-based matching doesn't
match decls of the symbol against itself.

### F.2. TS helper path resolution from fixture roots

The TS helper lives at `tools/integrity/integrity/cat2_contracts/ts_helper/`
in the real repo. When the test fixtures pass a synthetic `repo_root`
(e.g., `tests/fixtures/good_contracts_b`), the helper directory isn't
inside that synthetic root. `stack_b.py` resolves the helper's path
via `Path(__file__).resolve().parents[4]` (the real repo root) and
falls back to `repo_root` only if the real helper isn't present.

This means tests reuse the helper build artifact from the real repo's
`dist/` directory — no per-fixture rebuild. Faster and simpler.

### F.3. Stack B's surface is smaller than Stack C's

Stack B: 227 public symbols extracted, 73 unused (32%).
Stack C: 208 unique symbols extracted (after dedup), 111 unused (53%).
Stack D: 13 public exports, 17 sub-symbols (including class fields),
17 unused (essentially the full Phase 9 stub surface).

Stack B's lower "unused fraction" reflects the more mature consumer
side (7 landed Stack B sims exercise most of the abstraction). Stack
C's higher fraction reflects the Vulkan-abstraction-layer surface
being designed for breadth but only consumed by the 4 landed Stack C
sims (sph-water, eulerian-smoke, reaction-diffusion-3d,
mpm-multimaterial).

### F.4. Per-stack check-ID suffix convention

Across commits 5-7 the check IDs settled as:

- `cat2.public-symbol-used` (Stack D, unsuffixed since it landed first)
- `cat2.public-symbol-used-c` (Stack C, libclang)
- `cat2.public-symbol-used-ts` (Stack B, TypeScript)

Spec § 7.4 calls the check `cat2.public-symbol-used` generically. The
per-stack suffix is an implementation detail that lets grandfather
suppressions target the right stack and lets failure messages cite
the right diagnostic context. Commit 8 will formalize this in spec §
7.4 — for now the convention is documented in this audit and in the
grandfather-catalog.

The Stack D variant doesn't have a `-d` suffix; that's slight
asymmetry but established by precedent. Spec edit options for commit
8:

- Add `-d` suffix to Stack D for symmetry (renames the existing
  check; suppression annotations would need updating)
- Document the unsuffixed-as-Stack-D rule as the convention
  (keeps existing annotations valid; clearest for human readers)

The second option is preferable; will land in commit 8.
