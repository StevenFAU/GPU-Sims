# Integrity Toolkit — Commit 4b Landing — 2026-05-14

Second half of commit 4 per `docs/integrity-toolkit-spec.md` § 11. Adds
the always-on integrity CI workflow that gates every push to main and
every PR on the post-grandfather-sweep clean state established in 4a.

Companion to:

- Spec: `docs/integrity-toolkit-spec.md` § 9 (CI integration)
- Prior commit's audit: `integrity_build_4a_landing_2026-05-14.md`

---

## A. Change summary

Adds `.github/workflows/integrity.yml` per spec § 9.1, minus the
Cat 3 Stack C driver build step (deferred to commit 8 along with the
`GPU_SIMS_BUILD_INTEGRITY_CAT3` CMake flag). The workflow checkouts,
clones the registered SPlisHSPlasH vendor tree at the pinned SHA,
installs the toolkit in editable mode with dev deps, configures
Stack C for `compile_commands.json` (commit 6 will consume), runs
the toolkit's own pytest suite with coverage, and runs the toolkit
against the repo in `--output github` mode so any new finding lands
as an inline PR annotation. The audit log is uploaded as an artifact
on every run (success or failure).

## B. File inventory

| File | Status | Bytes | Notes |
|------|--------|-------|-------|
| `.github/workflows/integrity.yml` | new | ~2.3 KB | 9-step workflow, ubuntu-24.04, 10 min timeout |

No other files touched. No toolkit-code changes.

## C. Verification

### Pre-push state

- `python -m integrity --mode strict --no-audit-log` exits 0 with
  `0 hard-fail, 888 suppressed`
- `pytest tests/` in `tools/integrity/` reports 46/46 pass

### CI run #1 (commit `f7e012d`) — FAILED at cmake configure

```
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
CMake Error at FindPackageHandleStandardArgs.cmake:233 (message):
  Could NOT find Vulkan (missing: Vulkan_LIBRARY Vulkan_INCLUDE_DIR)
Call Stack (most recent call first):
// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
  /usr/local/share/cmake-3.31/Modules/FindVulkan.cmake:595
  common/common-cpp/CMakeLists.txt:25 (find_package)
```

Spec § 9.1's prescribed apt install list was `build-essential cmake
ninja-build libimath-dev` — but the repo's CMakeLists.txt declares
`find_package(Vulkan REQUIRED)`, so cmake configure cannot succeed
without `libvulkan-dev` + `vulkan-validationlayers`. Expanded the
install list in a follow-up commit (`1e886e6`) to match
`build-native.yml`'s working set (added Vulkan + GL/X11/Wayland headers
that GLFW pulls in).

### CI run #2 (commit `1e886e6`) — SUCCESS

All 9 steps green:

| Step | Status |
|------|--------|
| Checkout | success |
| Clone vendored references (anchor-pinned) | success |
| Set up Python 3.11 | success |
| Install integrity toolkit | success |
| Install build dependencies | success |
| Configure Stack C build | success |
| Run integrity toolkit's own tests (dogfood) | success |
| Run integrity toolkit against repo | success |
| Upload audit log | success |

GitHub Actions run ID: `25886103976`.

## D. Behavioral expectations

- **Trigger:** every push to `main` and every PR (any target branch),
  plus manual `workflow_dispatch`.
- **Concurrency:** runs are grouped by `github.ref`; in-progress runs
  on the same ref are cancelled when a new commit lands.
- **PR annotations:** the toolkit invocation uses `--output github`,
  which emits `::error file=...,line=...::...` lines for each new
  HARD_FAIL. GitHub renders these inline on the affected PR diff.
- **Gating:** any new finding raises a HARD_FAIL and exits 1, which
  fails the CI check and blocks merge (assuming the branch protection
  rule includes the `Integrity / Cross-stack integrity checks` check).
  Branch protection configuration is outside the toolkit's scope; the
  user will configure it in the GitHub UI.
- **Audit-log artifact:** if the toolkit writes
  `docs/diagnostics/_audits/integrity_failures_<date>.md` during the
  run, that file is uploaded as a workflow artifact and downloadable
  for 90 days (default). On the post-4a clean state, no audit-log
  file is produced (clean runs emit nothing).

## E. Preview of commit 5 — Cat 2 Stack D contract verification

Commit 5 lands `cat2_contracts/stack_d.py`, the simplest of the three
Cat 2 stacks because Stack D uses Python's stdlib `ast` module rather
than libclang (Stack C) or the TypeScript compiler API (Stack B).

Scope: implement `cat2.public-symbol-used` against
`common/common-py/gpusims_common/`. The check enumerates every public
field/function/method in the package's `__init__.py` re-export list,
then greps every `.py` file under `common-py/` for consumer references
(`self.<name>`, `instance.<name>`, `<ClassName>.<name>`, plus
function-call patterns for public free functions). Zero non-self
references → HARD_FAIL.

Fixtures will be synthetic per the existing
`tools/integrity/tests/fixtures/` convention: a `good_contracts/`
tree with a class whose every public field has a consumer site, and a
`bad_contracts/` tree where one field is silently dropped.

The implementation is small (Stack D's public surface is currently
~12 names) and the test surface is well-bounded. Commit 5 should land
cleanly without further toolkit-core changes.

## F. Incidental findings

### F.1. Spec § 9.1 apt list is incomplete

The spec's prescribed install list (`build-essential cmake ninja-build
libimath-dev`) misses Vulkan + windowing deps that the repo's
CMakeLists.txt requires. The fix-up commit `1e886e6` documents this in
its commit message. Worth updating spec § 9.1 in a follow-up doc patch
to match the working set. (Deferred to a later spec touch-up; this
audit captures the divergence.)

### F.2. Cat 3 Stack C build step deliberately skipped

Spec § 9.1's "Build Cat 3 Stack C driver" step targets
`integrity_cat3_stack_c` and passes
`-DGPU_SIMS_BUILD_INTEGRITY_CAT3=ON`. Neither the target nor the flag
exists yet — both land in commit 8. The 4b workflow therefore omits
that step. Commit 8's diff will add it back along with the underlying
CMake plumbing.

### F.3. References clone is fast enough

The `SPlisHSPlasH` clone with `--no-checkout` followed by `git checkout
<SHA>` completes in ~25-30s on `ubuntu-24.04`. Well under the 10-minute
workflow timeout. No need for partial clone or shallow-fetch tuning.
The full workflow walltime is ~3-4 min on a clean cache.

### F.4. Toolkit dogfood coverage runs in CI

The dogfood step uses `pytest tests/ -v --cov=integrity`. Coverage is
printed to stdout but not uploaded as a Codecov artifact in v1 (no
Codecov integration set up for this repo). If coverage gating becomes
useful, that's a v2 enhancement.
