# Integrity Toolkit v1 — Build Retrospective

**Surfaced during:** v1 build sequence (commits 1-8 plus fix-ups, 2026-05-14).

**Scope:** Lessons banked from constructing the cross-stack integrity verification toolkit per `docs/integrity-toolkit-spec.md`. Pairs with the per-commit audit reports under `docs/diagnostics/_audits/integrity_build_*_landing_2026-05-14.md`; the audits capture per-commit facts, this doc captures patterns that recurred across commits.

**Audience:** Future architects (Claude or human) drafting work that touches the integrity toolkit, extends the check set, or follows the same multi-agent build pattern on a different deliverable.

---

## 1. What got built

A nine-check toolkit gating every push and PR via `.github/workflows/integrity.yml`:

| Cat | Check ID | Coverage |
|---|---|---|
| 1 | `cat1.intra-repo` | All intra-repo citations resolve at their cited line ranges |
| 1 | `cat1.upstream-citation` | Upstream citations resolve under their registered vendor roots at the registered anchor versions |
| 1 | `cat1.upstream-anchor` | Vendored references' HEAD SHAs match the registry's anchor SHAs |
| 1 | `cat1.unregistered-upstream` | Every cited upstream name is in the ground-truth registry |
<!-- integrity-allow: cat1.annotation-form; retrospective-doc literal mention of the annotation grammar (not a real annotation); n/a -->
| 1 | `cat1.annotation-form` | All `integrity-allow:` annotations have valid grammar |
| 2 | `cat2.public-symbol-used` | Stack D Python `__init__.py` exports have non-self consumer references |
| 2 | `cat2.public-symbol-used-c` | Stack C `include/gpusims/` public surface has non-self consumer references |
| 2 | `cat2.public-symbol-used-ts` | Stack B `common-web/src/index.ts` exports have non-self consumer references |
| 3 | `cat3.cubic-kernel` | Cubic SPH kernel evaluations match analytical expected values within tolerance |

Post-v1 grandfather catalog: 1122 pre-v1 findings suppressed under 9 categories with per-category future-treatment notes at `tools/integrity/docs/grandfather-catalog.md`. Five of six spec § 12 canonical defects are now mechanically detectable.

The build sequence was eight planned commits plus four fix-up commits and one spec patch. Each commit landed an audit report. The toolkit was gating itself in CI starting from commit 4b — commits 5 through 8 each had to pass the gates the prior commits established.

## 2. Patterns that recurred across commits

### 2.1 Spec assertions about external systems drift from reality

Three separate spec errors fired during the build sequence, each caught by something executing against real content:

- **§ 12 mapping table** asserted three defect-to-check mappings that disagreed with the implemented check semantics. Caught after commit 3's smoke surfaced the LeniaNDK case under a different check than the spec named.
- **§ 9.1 apt-install list** asserted four packages; the repo's actual `find_package(Vulkan REQUIRED)` needed more. Caught by CI's cmake configure failing on commit 4b's first run.
- **§ 7.4 generic check ID** didn't account for the per-stack implementation divergence. Caught by commits 5/6/7 each needing to disambiguate; formalized as the `-c`/`-ts` suffix convention in commit 8.

The pattern: spec content asserted from an architect's memory of how external systems behave (CMake's find_package requirements, Python AST semantics, libclang cursor traversal, TypeScript compiler API surface) drifts from those systems' actual behavior. Verification gates closed the loop in every case, but the cost was real — a fix-up commit per drift instance.

**Convention banked:** spec content that asserts behavior of external systems (build tools, language toolchains, CI runners, library APIs) should be marked as `[needs-empirical-verification]` until execution confirms it. Architects should err toward "describe the contract; verify the implementation details during construction" rather than "assert the implementation details and ship."

### 2.2 Scaffolding without wiring is a silent defect class

Commit 1 landed `common/results.py` with a `Finding.suppressed` field, `common/annotations.py` with a parser, and an `Annotation` dataclass. None of this was reachable from the runner. The first time a finding could actually be suppressed was commit 4a — three commits and the better part of a build sequence later.

The defect was invisible because every test passed and every smoke run produced reasonable output. The scaffolding existed; the connective tissue didn't. Commit 4a discovered the gap when the grandfather sweep needed annotations to actually suppress findings and found that adding the annotation didn't change the runner's output.

**Convention banked:** when scaffolding a data model, the per-commit scope should require at least one end-to-end execution path that exercises the new surface. Commit 1's tests should have included a `test_runner_honors_annotation_suppression` that round-tripped from "annotation in source" through "finding flagged as suppressed in output." The absence wasn't caught by spec review.

### 2.3 Library APIs have unobvious surface

Two specific instances:

- **libclang hides chained member-access inside `UNEXPOSED_EXPR` wrappers.** `f.positions.size()` exposes the outer call cursor; the inner `.positions` cursor is buried where `get_children()` doesn't reach and `cursor.referenced` returns None. The Stack C check's first design walked cursors expecting symmetric access; it didn't catch chained member access until smoke run produced wildly wrong reference counts. Workaround: token-based fallback for `.field` / `->field` patterns. Documented as a known false-MISS class.
- **TypeScript's export specifiers look like consumer references.** The `export { unusedFunction }` line in `index.ts` made the bad-fixture's `unusedFunction` register one reference and pass the test. The TS compiler's `getSymbolAtLocation` resolved the export specifier's identifier to the same symbol the declaration produces. Workaround: skip `ImportSpecifier` and `ExportSpecifier` nodes during the AST walk. Found by fixture test failure.

The pattern: every per-stack Cat 2 implementation hit at least one library-API surprise. None of these were predictable from reading the documentation. All required execution against real content to surface.

**Convention banked:** when implementing a check that uses a non-trivial library API (libclang, TS compiler, mypy, etc.), allocate explicit budget for "library-API-quirk discovery" in the prompt. Don't claim the check is done after the synthetic fixtures pass — run smoke against the real repo and compare findings count against an independently-derived expected count. Wildly-off counts signal a library-API misunderstanding, not a spec bug.

### 2.4 CI environment is materially different from local

Three separate CI failures across the build:

- **Commit 4b run #1** — local cmake configure succeeded because the dev box had Vulkan headers installed system-wide; CI's ubuntu-24.04 runner didn't.
- **Commit 6 run #2** — local fixture worked because the fixture's `build/compile_commands.json` was present in the dev box's working tree; CI didn't have it because the root `.gitignore` excluded `build/` and the fixture's directory matched the pattern.
- **Commit 7 run #2** — local TS compilation succeeded because `npm install` had run at the repo root creating workspace symlinks; CI only ran `npm install` in the helper's directory, so `@gpusims/*` imports didn't resolve and every sim file produced false hard-fails.

The pattern: features of the local environment that the architect treated as "the environment" turn out to be local-only. CI's environment is more austere by default. The fix in each case was a workflow file addition (apt-install line, force-track flag, `npm install` step).

**Convention banked:** when designing CI workflows, enumerate every implicit environmental dependency explicitly. "Vulkan is required" → explicit apt-install. "Workspace symlinks must exist" → explicit `npm install` at the relevant scope. "compile_commands.json must be present" → explicit cmake configure step AND verification the output is gitignored-or-tracked deliberately. A local run that succeeds is not evidence the CI run will succeed.

### 2.5 Convention #8 fires more often than expected and the workflow absorbs it cheaply

Convention #8 (the architect-1 fabrication pattern) — asserting specifics from memory that turn out to be wrong — fired during this build at least eight times that left visible traces in the audit reports. Sample:

- The spec § 12 mappings (three instances, one prompt)
- The spec § 9.1 apt list
- The cat2 generalization (initially `cat2.public-field-read`, didn't cover the function-defined-but-uncalled case)
- libclang's UNEXPOSED_EXPR quirk
- TypeScript's export-specifier-as-consumer quirk
- The annotation-suppression scaffold-without-wiring gap
- The fixture-stdlib-headers issue in commit 6
- The workspace-install issue in commit 7

Each fix was a single follow-up commit (sometimes ride-along on the next planned commit, sometimes its own commit). None propagated past the next CI run. None reached production.

**Convention banked:** the multi-agent workflow (architect chat for orchestration + reasoning, Claude Code for verification + execution, CI for ground truth) is robust against Convention #8 in the sense that the architect's fabrications fail loudly and fixable cheaply. The cost per drift instance is roughly one follow-up commit. This is much cheaper than the alternative (architect verifies every assertion before drafting, slowing the work by maybe 3x) and much cheaper than the worse alternative (defects ship and cause downstream confusion).

The flip side: an architect should not feel that fabrications are "bad" or to be hidden. The audit reports document every drift instance openly; honest retraction is the norm. The workflow is built on the assumption that the architect will sometimes be wrong, and the right response is to surface the error and fix it, not to defend the original assertion.

## 3. Per-commit notes worth banking

Compressed observations from each commit's audit.

**Commit 1 (scaffold).** Largest single commit in the sequence by file count. Specifying full file content rather than letting Claude Code generate idiomatically was the right call for load-bearing modules (runner, data model, annotation grammar). For boilerplate (empty `__init__.py` files, fixture stubs), generation would have been fine.

**Commit 2 (Cat 1 intra-repo).** Caught a catastrophic-regex-backtracking bug during smoke (nested quantifiers over a slash-permissive class hung two test runs at 100% CPU for 15 minutes before kill). Single-class replacement fixed it. Worth flagging: regex authors writing for arbitrary repo content should test against the actual repo, not just synthetic fixtures.

**Commit 3 (Cat 1 upstream).** The "sharp drop" prediction for intra-repo findings was wrong — 810 → 785, not 810 → ~50. Predictions about real-repo behavior should be loose. The grammar collision between INTRA_REPO_RE and UPSTREAM_RE on `<UpstreamName> <version> <path>:<line>` text required intra-repo skip-paths under `references/`. Bare-path-to-upstream-basename (LeniaNDK case) was correctly identified as v2 work.

**Commit 4a (grandfather sweep).** Categorized-script approach was the right middle ground. Per-category reasons are informative without per-finding-issue overhead. 888 findings classified cleanly. Suppression-machinery wiring gap discovered and closed here.

**Commit 4b (CI).** First commit to live-gate. The apt-list drift caught immediately. Worth noting: the workflow's `setup-node` and `setup-python` pins should match other workflows' pins — drift between workflows is annoying.

**Commit 5 (Stack D).** Cleanest of the three Cat 2 implementations because stdlib `ast` is well-documented and has minimal surprises. The kwarg-vs-attribute distinction (constructor kwargs write fields without "reading" them) is the precise semantic that catches the radii defect class. 17 findings, all canonical, grandfathered under `cat2-stack-d-unused`.

**Commit 6 (Stack C).** Most technically involved. libclang's UNEXPOSED_EXPR quirk + USR variation across TUs + fixture stdlib-headers issue + gitignore force-track flag all combined into a multi-fix-up commit. The token-based fallback for field access is the right pragmatic v1 choice. 111 findings, all three spec § 12 canonical Stack C defects detected.

**Commit 7 (Stack B).** The TS compiler API was easier to work with than libclang — type-aware out of the box, no equivalent of UNEXPOSED_EXPR. The Python-spawns-Node bridge was straightforward. Export-specifier exclusion was the main surprise. 73 findings, no canonical defects (spec § 12 doesn't have Stack B rows).

**Commit 8 (Cat 3 + spec finalization).** Smallest implementation, cleanest design. The acceptance test (`--inject-factor-of-6` regen flow) demonstrates the check detects formula-vs-implementation disagreement; the documented v1 limitation (verifies C++ transcription, not GLSL) sets up v2 work clearly.

## 4. Honest gaps in v1 and where they go

**`cat2.stub-label-stale` deferred.** The "is this comment claiming the function is a stub when the function has a real implementation" check from spec § 12 row 5 is the only canonical defect class without v1 mechanical detection. Scope is small; deferred for lack of priority.

**Cat 3 verifies C++ transcription, not shader source.** A real GLSL bug not mirrored in the C++ driver wouldn't fire. v2 candidate: shader-level kernel verification harness. Significant complexity (Vulkan runtime in CI), so explicit v2 not v1.1.

**Stack C runtime dominates CI walltime.** ~95 seconds for full Stack C scan. Within budget but the obvious v1.1 optimization target. Likely fix: parse each TU once and cache USRs across the extraction + reference-finding passes.

**The toolkit doesn't run itself on itself.** Cat 2 catches Stack D defects in `gpusims_common` but doesn't run against `integrity`'s own package. Adding this is a v1.1 self-application item. Probably worth doing — it would catch the same scaffolding-without-wiring class of defect that commit 4a discovered, before the next instance happens.

<!-- integrity-allow: cat1.annotation-form; retrospective-doc literal mention of the annotation grammar (not a real annotation); n/a -->
**Annotation grammar doesn't understand markdown code fences.** Literal mentions of `integrity-allow:` in fenced code blocks within docs get parsed as real annotations. Handled in v1 by per-line grandfather suppression. v2 could add fenced-block awareness to the grammar.

<!-- integrity-allow: cat1.intra-repo; retro-doc snapshot intra-repo citation pre-v1.3 (see grandfather-catalog retro-doc-snapshot); n/a -->
<!-- integrity-allow: cat1.bare-path; retrospective-doc bare-path citation pre-v1.2 (see grandfather-catalog retro-bare-path); n/a -->
**Bare-path-to-upstream-basename detection deferred.** The LeniaNDK citation pattern (`LeniaNDK.py:329-335` with no version prefix) falls through the upstream grammar. v2 candidate in spec § 13.

## 5. Workflow conventions banked

For future multi-agent build sequences:

- **Audit reports per commit are load-bearing.** Each commit gets a landing report with sections A-F. The report is committed in a follow-up commit (separate SHA from the feature commit) to keep diffs focused. Future commits can cite the audit by filename.
- **Spec patches happen mid-sequence, not at the end.** Three spec patches landed during this build (commits 3.5, 4b fix-up, 5, 8 finalization). Catching spec drift early prevents downstream commits from inheriting the drift.
- **Fix-up commits are fine; `--amend` is not.** Every fix-up was a separate SHA. The git history shows the drift-and-repair sequence honestly.
- **Smoke runs against real repo before any commit lands.** Synthetic fixtures verify the implementation works; real-repo smoke verifies it works against actual content. Predictions about real-repo behavior are loose by default.
- **Honest retraction is the norm.** Architect prompts often included "if you find a reason to modify <load-bearing decision>, STOP and report." The pattern worked because Claude Code did report when it found contradictions, and the architect did revise rather than defend.
- **Per-commit prompts are large but bounded.** A typical commit prompt was 5-15K tokens with file-by-file specifications. This is more than minimum-viable but pays for itself in fewer "the spec said X but the implementation did Y" disagreements.

## 6. What to do with grandfathered suppressions over time

The 1122 grandfathered findings are not permanent. Per-category future treatment:

- **`audit-citation` (~785 findings):** permanent. Audit reports are append-only snapshots.
- **`live-shader-1810` (~10 findings):** dissolves as sph-water shader headers are next edited and the citations get updated from `1.8.10` to `2.16.1`.
- **`audit-doc-1810` (~18 findings):** permanent. The historical fabrication record is intentional.
- **`spec-grammar-example`, `toolkit-own-source`, `audit-report-grammar-example` (~30 findings combined):** permanent. Documentation-only literal mentions.
- **`cat2-stack-d-unused` (17 findings):** dissolves as `AlembicWriter`, `CameraInputState`, and the `write_float_*` functions gain consumers.
- **`cat2-stack-c-unused` (111 findings):** dissolves gradually as sims consume more of the Vulkan abstraction.
- **`cat2-stack-b-unused` (73 findings):** dissolves as Stack B sims wire more of `common-web`.
- **`other-cat1` (small):** per-entry review in v1.1.

A future architect can re-run the grandfather sweep at any time to refresh the category counts, but per-category event-driven dissolution is the expected pattern, not bulk re-sweep.

## 7. Pointers

- Spec: `docs/integrity-toolkit-spec.md`
- Audit reports: `docs/diagnostics/_audits/integrity_build_*_landing_2026-05-14.md` (eight files, scaffold through final)
- Spec-patch commit: SHA `641dc7a` (the § 12 + § 7.4 corrections discovered after commit 3)
- Grandfather catalog: `tools/integrity/docs/grandfather-catalog.md`
- Ground-truth source registry: `tools/integrity/docs/ground-truth-sources.md`
- Toolkit source: `tools/integrity/integrity/`
- Stack C driver: `tools/integrity/drivers/integrity_cat3_stack_c/`
- CI workflow: `.github/workflows/integrity.yml`

For the Phase 11.5 work this toolkit was originally motivated by, see also `docs/diagnostics/_audits/phase11_5_*.md` and `docs/retro/phase11.md`.
