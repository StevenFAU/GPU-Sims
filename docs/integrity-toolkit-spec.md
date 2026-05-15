---
title: Integrity Toolkit — Specification v1
date: 2026-05-14
author: architect1
status: spec-locked
scope: permanent cross-stack integrity verification infrastructure
audience: Claude Code (executor), future contributors (maintainers)
sibling-docs:
  - docs/overarching-spec.md
  - docs/conventions.md
  - docs/diagnostics/_audits/integrity_toolkit_probe_2026-05-14_architect1.md
---

# Integrity Toolkit — Specification v1

## 0. Reader's note

This document specifies the design and build sequence for a permanent integrity verification toolkit covering the GPU-Sims repo across all three stacks (B: TypeScript/WebGPU, C: C++/Vulkan, D: Python/Taichi).

The toolkit exists because the project has accumulated multiple documented instances of "Convention #8" — confident assertions in code, comments, and docs that were later discovered to be wrong or fabricated. The toolkit's job is to mechanically verify the kinds of assertions that prior fabrications hid behind, on every commit, indefinitely.

This spec is the contract between architect-1 (drafter) and the execution chain (Claude Code, per-module). Execution prompts will reference specific sections of this spec rather than restating their contents.

The spec is dense by intention. Each subsection commits to a specific choice (tool, path, format, rule) rather than describing alternatives. Where real ambiguity exists, it is named explicitly with `[OPEN]` and a deferral decision.

## 1. Purpose & design philosophy

### 1.1 What the toolkit defends

The toolkit defends three properties of the repo:

1. **Citation integrity** — every `file:line` style citation in source, shader, doc, or test resolves to a real file at a real line, and (for upstream citations) to a vendored reference at a documented anchor.
2. **Contract verification** — every public API field, function, and declared behavior in `common/common-cpp/`, `common/common-web/`, and `common/common-py/` has implementation matching its declaration, with no silent gaps (the `ParticleFrame::radii` defect class).
3. **Numerical correctness against upstream** — where the codebase claims to implement an upstream algorithm, that implementation can be mechanically compared against the upstream reference at chosen test inputs and asserted to agree within a documented tolerance.

These three properties are Categories 1, 2, and 3.

### 1.2 What the toolkit does NOT defend (v1)

- **Runtime integration** (Category 4) — actually running binaries against canonical inputs and asserting output snapshots. Deferred until v1 is fortified. The hypothesis: Cat 4 maintenance cost is high; doing it before the structural layer is solid means snapshots churn for the wrong reasons. Reconsidered after one full project cycle on v1.
- **Performance regressions** — out of scope permanently. Performance work has different verification semantics (statistical, hardware-dependent) and belongs in a separate tool if ever built.
- **Security/dependency scanning** — out of scope. Dependabot and similar tools cover this.
- **Spec internal consistency** — checking that a phase spec's claims match the eventual landed code is what audit reports do, not what the toolkit does. The toolkit verifies claims that are in code and docs as shipped; it does not adjudicate intent.

### 1.3 Design principles

1. **Mechanical, deterministic, fast.** Every check produces the same verdict on the same input regardless of when it runs or where. CI runtime budget: ≤ 2 minutes wall-clock for all of Cat 1 + Cat 2 + Cat 3 on a clean checkout.
2. **Modular per-test tooling.** Each check is a separate module under `tools/integrity/integrity/cat<N>_<name>/checks/`. Modules are independently testable, independently disable-able, and independently suppressible per finding.
3. **Ground truth from verifiable sources only.** Mathematical proofs, working code at pinned upstream SHAs, peer-reviewed CS research. Never "architect remembers." Never "common knowledge." The toolkit itself follows Convention #8 discipline.
4. **Hard failure for mechanical checks; soft for numerical.** Cat 1 and Cat 2 are deterministic and have no false-positive risk worth tolerating; they hard-fail CI. Cat 3 has legitimate floating-point tolerance questions and can flake on hardware/driver edges; it soft-warns by default with opt-in hard-fail per test.
5. **The toolkit lints itself.** A `tools/integrity/tests/` directory exercises every check on synthetic fixtures (good cases pass, bad cases fail). The toolkit's own CI gate includes running its own tests.
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
6. **No retroactive cleanup required for v1 to land.** When the toolkit starts running, every existing finding becomes a Cat 1/2/3 fail. The toolkit ships with a one-time grandfather pass: every existing failure gets a `// integrity-allow: grandfathered-pre-v1` annotation with a tracking issue. New code gets no grandfather. This makes v1 actionable without blocking on a sweep through existing audit findings.

## 2. Scope summary

### 2.1 In scope for v1

| Item | Coverage |
|------|----------|
| Cat 1: Citation integrity | All three stacks |
| Cat 2: Contract verification | All three stacks |
| Cat 3: Numerical correctness | All three stacks (test infrastructure proportional to existing per-stack testing maturity) |
| CI gating | Hard-fail Cat 1/2; soft-warn + audit-log Cat 3 |
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
| Suppression mechanism | Inline `integrity-allow:` annotation |
| Self-testing | Toolkit's own tests, runnable via `pytest tools/integrity/tests/` |
| Documentation | Per-check `how-to-add` docs, failure-mode reference, grandfather catalog |

### 2.2 Deferred (revisit after v1 settles)

| Item | Reason for deferral |
|------|---------------------|
| Cat 4 runtime integration | High maintenance, premature without Cat 1-3 solid |
| Cat 5+ design questions | Defer until v1 stable for one project cycle |
| Per-sim shader-level numerical checks | Cat 3 v1 covers common-* APIs and core SPH; per-sim shader math is too dispersed for v1 |

### 2.3 Out of scope permanently

| Item | Why out of scope |
|------|------------------|
| Performance regression detection | Different semantics; separate tool if ever built |
| Security/dep scanning | Dependabot + npm/pip audit cover this |
| Spec-vs-implementation reconciliation | Audit reports do this; toolkit doesn't adjudicate intent |
| Style/lint (markdown, ruff, mypy, clang-format, prettier) | Existing CI workflows already cover these |

## 3. Operating principles

### 3.1 Failure modes

Three failure modes; every check declares exactly one.

**HARD_FAIL** (Cat 1 + Cat 2 default)

- Check fails → CI job exits non-zero → PR/push is blocked from merge
- Toolkit writes failure detail to GitHub Actions step output (annotation)
- Toolkit writes failure entry to `docs/diagnostics/_audits/integrity_failures_<YYYY-MM-DD>.md` (created if absent; appended if present)
- Suppression possible per-finding via inline annotation (§ 3.2)

**SOFT_WARN** (Cat 3 default)

- Check fails → CI job continues, exit 0
- Toolkit emits GitHub Actions warning-level annotation on the failing line/file
- Toolkit writes failure entry to the same per-date audit-log file
- Per-test opt-in to HARD_FAIL via the test's metadata (§ 8.5)

**INTERNAL_FAIL** (toolkit's own machinery broke)

- Always HARD_FAIL; CI exits non-zero
- Toolkit prints diagnostic to stderr including the python traceback
- No suppression possible. If the toolkit breaks, fix the toolkit first.

A "check" is the smallest unit of verification — one function in a module under `cat<N>_<name>/checks/`. A check's mode is declared in its module docstring and enforced by the runner.

### 3.2 Suppression annotation grammar

When a check would HARD_FAIL on a finding, the code can suppress the failure with an inline annotation immediately preceding (same line or line above) the offending code. Annotations are reviewed quarterly per § 3.3.

Annotation forms by language:

```cpp
// integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a
// integrity-allow: <check-id>; <reason>; <issue-ref>
// e.g.:
// integrity-allow: cat1.upstream-anchor; SPlisHSPlasH 1.8.10 anchor pre-v1; #117
```

```typescript
// integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a
// integrity-allow: <check-id>; <reason>; <issue-ref>
```

```python
# integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a
# integrity-allow: <check-id>; <reason>; <issue-ref>
```

```glsl
// integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a
// integrity-allow: <check-id>; <reason>; <issue-ref>
```

```wgsl
// integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a
// integrity-allow: <check-id>; <reason>; <issue-ref>
```

Markdown:

```markdown
// integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a
<!-- integrity-allow: <check-id>; <reason>; <issue-ref> -->
```

**Required fields:**

- `<check-id>` — fully qualified check identifier, e.g. `cat1.upstream-anchor`
- `<reason>` — human-readable rationale, ≥ 8 characters
- `<issue-ref>` — GitHub issue number prefixed with `#`, or the literal string `n/a` for permanent suppressions with no tracking issue

**Wildcards:**

- `<check-id>` may be `cat<N>.*` to suppress all checks in a category for the immediate next line. Disallowed at module/file scope.
- `<check-id>` may NOT be `*`. Blanket suppression is a smell; if you need it, file an issue first.

**Scope:** an annotation suppresses checks for the **immediate next line or expression**. Multi-line suppressions require multi-line annotations. There is no file-level or block-level suppression.

<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
**Validation:** the toolkit's own Cat 1 has a check that verifies all existing `integrity-allow:` annotations: every `<check-id>` resolves to a real check, every `<issue-ref>` resolves (issue exists; or is literally `n/a`), every annotation has a non-empty reason. Suppression-annotation fabrication is itself a Cat 1 fail.

### 3.3 Audit-log integration

Every HARD_FAIL or SOFT_WARN produces an entry in:

```
docs/diagnostics/_audits/integrity_failures_<YYYY-MM-DD>.md
```

Format (append-only):

```markdown
---
date: 2026-05-14
author: integrity-toolkit
phase: ci-run
status: failure-record
scope: machine-generated; do not edit by hand
---

## Run <timestamp> — commit <short-sha>

### cat1.upstream-anchor — HARD_FAIL

**File:** `particle-fluids/sph-water/docs/load-bearing-decisions.md:9`
**Check:** Upstream anchor SHA matches vendored reference HEAD
**Found:** `SPlisHSPlasH 1.8.10 at SHA c254caf2705ebf5271408dd37a091aa379258a38`
**Expected:** SHA recorded in `.gitignore:1-5` = `6bff55a6eaf14083d34650f22a268ce156b62b54`
**Why fail:** Documented anchor does not match the vendored `references/SPlisHSPlasH/.git/HEAD` SHA.

(... next entry ...)
```

The integrity-toolkit's own Cat 1 has a check verifying that audit-log entries themselves do not contain unresolved citations (recursion-safe).

<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
**Quarterly review process:** maintainers (or future tooling) scan the month's audit-log files plus all `integrity-allow:` annotations. Stale suppressions get removed; new findings get the appropriate fix or suppression. This is process, not toolkit-enforced.

### 3.4 Canonical exclusion paths

Every toolkit check excludes the following paths from analysis, except where a specific check explicitly opts in (e.g., Cat 1's upstream-anchor check reads `references/<lib>/.git/HEAD` deliberately):

```
node_modules/
**/node_modules/
build/
build-*/
.venv/
**/.venv/
**/__pycache__/
references/
**/_deps/
dist/
**/dist/
.git/
.claude/
captures/
alembic_export/
vdb_export/
gallery/
```

<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
These are codified in `tools/integrity/integrity/common/exclusions.py` as a single canonical list. Adding to this list is itself a Cat 1 fail unless done via an annotated change with `integrity-allow: cat1.exclusion-list`.

### 3.5 The toolkit follows Convention #8

The toolkit itself is subject to fabrication risk. Architects writing checks may confidently assert "this regex catches all citation forms" and be wrong. Defenses:

- **Every check has tests in `tools/integrity/tests/`.** Tests include both positive cases (real citations) and negative cases (lookalikes that should NOT trigger). Tests use synthetic fixtures, not real repo files, so test data isn't subject to repo drift.
- **Every check declares its known false-positive and false-negative classes** in its module docstring. If a class isn't named, it isn't defended.
- **The toolkit runs on itself.** Cat 1 verifies toolkit-internal citations. Cat 2 verifies the toolkit's own public surface (`tools/integrity/integrity/__init__.py` exports). The toolkit eating its own dogfood is the primary defense against fabrication-in-toolkit.

## 4. Repo layout

The toolkit lives at `tools/integrity/` at the repo root. `tools/` is greenfield (does not currently exist).

```
tools/
└── integrity/
    ├── README.md                          # Brief: how to run, where to look on failure
    ├── pyproject.toml                     # Toolkit's own deps + ruff/mypy/pytest config
    ├── integrity/                         # Top-level Python package
    │   ├── __init__.py
    │   ├── __main__.py                    # `python3 -m integrity` entry
    │   ├── runner.py                      # Orchestrator + CLI surface
    │   ├── common/                        # Shared utilities
    │   │   ├── __init__.py
// integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a
    │   │   ├── annotations.py             # Parse integrity-allow: comments
    │   │   ├── exclusions.py              # Canonical exclude paths
    │   │   ├── audit_log.py               # Append to integrity_failures_<date>.md
    │   │   ├── results.py                 # Result type + failure-mode enum
    │   │   ├── stack_paths.py             # Canonical paths per stack
    │   │   └── repo.py                    # Git helpers (HEAD SHA, file listing)
    │   ├── cat1_citations/                # Category 1
    │   │   ├── __init__.py
    │   │   ├── grammar.py                 # Citation regexes + parse-tree types
    │   │   ├── resolver.py                # file:line resolution (repo + references/)
    │   │   ├── upstream_anchor.py         # Vendored-ref HEAD vs. doc anchor
    │   │   └── checks/                    # Individual check modules
    │   │       ├── intra_repo.py          # cat1.intra-repo
    │   │       ├── upstream.py            # cat1.upstream-citation
    │   │       ├── upstream_anchor.py     # cat1.upstream-anchor
    │   │       ├── annotation.py          # cat1.annotation-form
    │   │       └── audit_log.py           # cat1.audit-log-recursion
    │   ├── cat2_contracts/                # Category 2
    │   │   ├── __init__.py
    │   │   ├── stack_c.py                 # libclang-based public-API audit
    │   │   ├── stack_b.py                 # TypeScript compiler API
    │   │   ├── stack_d.py                 # Python ast module
    │   │   └── checks/
    │   │       ├── public_field_read.py   # cat2.public-field-read
    │   │       ├── public_symbol_def.py   # cat2.public-symbol-def
    │   │       ├── re_export_match.py     # cat2.re-export-match
    │   │       └── stub_label_stale.py    # cat2.stub-label-stale
    │   └── cat3_numerical/                # Category 3
    │       ├── __init__.py
    │       ├── golden.py                  # Golden-value table format + loader
    │       ├── stack_c_driver/            # C++ driver (CMake target)
    │       │   ├── CMakeLists.txt
    │       │   ├── main.cpp
    │       │   └── checks/
    │       │       └── cubic_kernel.cpp   # cat3.cubic-kernel
    │       ├── stack_b_driver/            # TypeScript driver (vite-built)
    │       │   ├── package.json
    │       │   ├── main.ts
    │       │   └── checks/
    │       │       └── (per-check files)
    │       ├── stack_d_runner.py          # pytest integration for Stack D
    │       └── checks/                    # Stack-independent metadata
    │           ├── cubic_kernel.toml      # Anchored to references/SPlisHSPlasH
    │           └── (per-check .toml)
    ├── tests/                              # Toolkit's own tests
    │   ├── conftest.py
    │   ├── fixtures/                       # Synthetic source files for testing
    │   │   ├── good_citations/
    │   │   ├── bad_citations/
    │   │   ├── good_contracts/
    │   │   ├── bad_contracts/
    │   │   └── numerical/
    │   ├── test_cat1_citations.py
    │   ├── test_cat2_contracts.py
    │   ├── test_cat3_numerical.py
    │   ├── test_runner.py
    │   └── test_annotations.py
    └── docs/
        ├── architecture.md                 # How the parts fit together
        ├── how-to-add-a-check.md           # Tutorial
        ├── failure-modes.md                # Reference
        ├── grandfather-catalog.md          # Pre-v1 suppressions, dated
        └── ground-truth-sources.md         # Cat 3 references with pinned SHAs
```

The toolkit is a single Python package + per-stack drivers for Cat 3. No JS package at repo root for the toolkit itself; Stack B's Cat 3 driver ships inside `tools/integrity/integrity/cat3_numerical/stack_b_driver/` as its own npm workspace member.

## 5. Top-level runner

### 5.1 CLI

```
python3 -m integrity [--cat <N>] [--check <id>] [--mode <strict|warn-only>] \
                    [--root <path>] [--output <format>] [--no-audit-log]
```

**Options:**

| Flag | Default | Meaning |
|------|---------|---------|
| `--cat` | (all) | Run only the named category (1, 2, or 3) |
| `--check` | (all) | Run only the named check, e.g. `cat1.upstream-anchor` |
| `--mode` | `strict` | `strict`: honor declared HARD_FAIL/SOFT_WARN. `warn-only`: convert all HARD_FAIL to SOFT_WARN (for local development). |
| `--root` | repo root | Override repo root for tests. |
| `--output` | `human` | `human`: console summary. `json`: machine-readable. `github`: emit GitHub Actions annotations. |
| `--no-audit-log` | off | Skip writing to `integrity_failures_<date>.md` (used by the toolkit's own tests to avoid polluting the audits dir during test runs) |

### 5.2 Exit codes

| Code | Meaning |
|------|---------|
| 0 | All checks passed (or any failures were SOFT_WARN-only) |
| 1 | One or more HARD_FAIL findings |
| 2 | INTERNAL_FAIL — toolkit machinery broke |
| 64 | Bad CLI invocation |

### 5.3 Invocation patterns

| Context | Command |
|---------|---------|
| CI: full run | `python3 -m integrity --output github` |
| Local: pre-commit | `python3 -m integrity --mode warn-only` |
| Local: debugging one check | `python3 -m integrity --check cat1.upstream-anchor` |
| Toolkit's own tests | `pytest tools/integrity/tests/ -v` |

### 5.4 JSON output schema

```json
{
  "schema_version": 1,
  "run_id": "2026-05-14T15:23:01Z--3a7f9b2",
  "commit": "3a7f9b2c1d4e5...",
  "duration_seconds": 47.3,
  "summary": {"pass": 142, "soft_warn": 3, "hard_fail": 0, "suppressed": 8},
  "findings": [
    {
      "check_id": "cat3.cubic-kernel-q0.5",
      "mode": "SOFT_WARN",
      "file": "particle-fluids/sph-water/shaders/density_alpha.comp.glsl",
      "line": 76,
      "message": "GPU kernel_gradW returned -8.952e+06; expected -1.492e+06 ± 1e-6 relative; ratio 6.00000",
      "ground_truth_ref": "references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h:62-85",
      "suppressed": false
    }
  ],
  "suppressions": [
    {
      "annotation_file": "particle-fluids/sph-water/docs/load-bearing-decisions.md",
      "annotation_line": 9,
      "check_id": "cat1.upstream-anchor",
      "reason": "SPlisHSPlasH 1.8.10 anchor pre-v1 grandfather",
      "issue_ref": "#117"
    }
  ]
}
```

The JSON schema is versioned via a top-level `"schema_version"` field. v1 = `1`.

## 6. Category 1: Citation & reference integrity

### 6.1 What's checked

Every citation in the form `<path>:<line>` or `<path>:<start>-<end>` appearing in:

- Source files (`.cpp`, `.hpp`, `.h`, `.cc`)
- Shader files (`.glsl`, `.wgsl`, `.comp.glsl`, `.frag.glsl`, `.vert.glsl`)
- TypeScript files (`.ts`, `.d.ts`)
- Python files (`.py`)
- Markdown files (`.md`)

…within both single-line and multi-line comments (`//`, `/* */`, `#`, `<!-- -->`) and inline-backtick markdown spans.

Plus every **upstream citation** in the form `<UpstreamName> <version> <path>:<line>` (the lead capitalized word identifies a vendored reference; matched against a registry in `tools/integrity/docs/ground-truth-sources.md`).

### 6.2 Citation grammar (formal)

Regex (PCRE, single capture-group form for clarity; production parser is in `cat1_citations/grammar.py`):

```
INTRA_REPO_CITATION:
    \b(?P<path>[A-Za-z0-9_./-]+\.[A-Za-z]{1,8}):(?P<start>\d+)(?:-(?P<end>\d+))?\b

UPSTREAM_CITATION:
    \b(?P<upstream>[A-Z][A-Za-z0-9]+)\s+
    (?P<version>v?\d+(?:\.\d+){0,3}(?:-[A-Za-z0-9]+)?|HEAD|[a-f0-9]{7,40})
    \s+
    (?P<path>[A-Za-z0-9_./-]+\.[A-Za-z]{1,8}):(?P<start>\d+)(?:-(?P<end>\d+))?\b
```

**Known false-positive classes (named, defended in tests):**

- IPv4-like strings (`192.168.1.1:80`) — excluded by requiring an extension match in `{cpp, hpp, h, cc, glsl, wgsl, ts, py, md, ...}`
- Time-of-day strings (`14:30`) — excluded by requiring a `.<ext>` prefix
- URL fragments (`example.com/path:42`) — excluded by checking the path resolves on-disk before declaring it a citation
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
<!-- integrity-allow: cat1.bare-path; toolkit-doc bare-path citation pre-v1.2 (see grandfather-catalog toolkit-doc-bare-path); n/a -->
- `_template.md` placeholder tokens (`{{path:line}}`) — explicitly skipped per the probe's Section P note on `markdown.yml:46`
- Code blocks fenced with `~~~` or triple-backtick — content inside is excluded except when the surrounding doc explicitly tags the block as a citation list

**Known false-negative classes (named, defended in tests):**

<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
<!-- integrity-allow: cat1.bare-path; toolkit-doc bare-path citation pre-v1.2 (see grandfather-catalog toolkit-doc-bare-path); n/a -->
- Citations split across line breaks (e.g., `SPlisHSPlasH 1.8.10\nTimeStepDFSPH.cpp:1370`) — NOT supported in v1; v2 may add multi-line citation support
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
<!-- integrity-allow: cat1.bare-path; toolkit-doc bare-path citation pre-v1.2 (see grandfather-catalog toolkit-doc-bare-path); n/a -->
- Bracketed citations (`[file.cpp:42]`) — NOT supported in v1 (no precedent in repo per probe Section E)

### 6.3 Resolution rules

**For intra-repo citations:**

1. Resolve `<path>` relative to the file containing the citation
2. If unresolved, try repo root
3. If still unresolved, declare FAIL
4. If resolved, check that `<start>` ≤ file line count, and `<end>` ≤ file line count if specified

**For upstream citations:**

1. Map `<upstream>` to a vendored reference root via the registry in `tools/integrity/docs/ground-truth-sources.md`. Registry format:

   ```toml
   [SPlisHSPlasH]
   anchor_version = "2.16.1"
   anchor_sha     = "6bff55a6eaf14083d34650f22a268ce156b62b54"
   vendor_root    = "references/SPlisHSPlasH"
   anchor_doc     = ".gitignore"
   ```

2. If `<upstream>` not in registry, declare FAIL (the citation references something not vendored; this is the LeniaNDK case)
3. If `<version>` does not match `anchor_version` and is not `HEAD`, declare FAIL (the SPlisHSPlasH 1.8.10 case — wrong-version citation)
4. Resolve `<path>` under `vendor_root`
5. Check `<start>` and `<end>` against file line count

### 6.4 Checks (Category 1)

| Check ID | Description | Failure mode |
|----------|-------------|--------------|
| `cat1.intra-repo` | All intra-repo citations resolve to real files at real lines | HARD_FAIL |
| `cat1.upstream-citation` | Upstream citations resolve under registered vendor roots at the documented version | HARD_FAIL |
| `cat1.upstream-anchor` | For every registered upstream, `vendor_root/.git/HEAD` SHA matches `anchor_sha` | HARD_FAIL |
| `cat1.unregistered-upstream` | Detect probable upstream citations (capitalized name pattern) for upstreams NOT in the registry, e.g. the LeniaNDK case | HARD_FAIL |
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
| `cat1.annotation-form` | All `integrity-allow:` annotations have valid grammar, real check IDs, real (or `n/a`) issue refs | HARD_FAIL |
| `cat1.audit-log-recursion` | Citations inside `_audits/integrity_failures_*.md` resolve cleanly (the audit-log itself doesn't contribute drift) | HARD_FAIL |
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
| `cat1.exclusion-list` | The canonical exclusion list in `exclusions.py` has not been modified outside of an explicit `integrity-allow: cat1.exclusion-list` commit | HARD_FAIL |

### 6.4.1 Bare-path citation limitation

<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
<!-- integrity-allow: cat1.bare-path; toolkit-doc bare-path citation pre-v1.2 (see grandfather-catalog toolkit-doc-bare-path); n/a -->
The upstream-citation grammar requires the `<UpstreamName> <version>` prefix. Bare-path citations to known-upstream basenames — e.g., `LeniaNDK.py:329-335` written as a Python comment, where the human reader understands the implicit reference to Chakazul/Lenia — do NOT match `UPSTREAM_RE`. These currently fall through to `cat1.intra-repo`, which flags them when the path doesn't resolve locally.

This is a v1 limitation, not a defect. It is correct behavior given the v1 grammar. Bare-path-to-upstream-basename detection is enumerated as a v2 candidate in § 13.

## 7. Category 2: Public-API contract verification

### 7.1 What's checked

For each stack's public API surface, contract assertions are mechanically verified:

- **Public-symbol-used:** every public struct/class field, every public free function, and every public member method has at least one consumer site outside its defining class. Covers two defect shapes: silent-data-loss fields like `ParticleFrame::radii`, and defined-but-unexercised public functions like `vdb::writeVec3Grid`.
- **Public-symbol-def:** every declared-but-not-defined symbol in a public header has a corresponding implementation
- **Re-export-match:** for stacks with re-export modules (`common/common-web/src/index.ts`, `common/common-py/gpusims_common/__init__.py`), the re-exported names match the documented public surface
- **Stub-label-stale:** "stub", "placeholder", "skeleton" markers in public headers/modules are checked against the actual implementation (per the `alembic_writer.hpp` "stub" label being stale since Phase 1)

### 7.2 Public surface definitions

Per stack, the "public" surface is defined as:

**Stack C:** all symbols declared in headers under `common/common-cpp/include/gpusims/` and recursively in `common/common-cpp/include/gpusims/vk/`. Implementation lives in `common/common-cpp/src/`.

**Stack B:** all symbols re-exported by `common/common-web/src/index.ts`. Implementation: all `.ts` files under `common/common-web/src/`.

**Stack D:** all symbols re-exported by `common/common-py/gpusims_common/__init__.py`. Implementation: all `.py` files under `common/common-py/gpusims_common/`.

### 7.3 Per-stack implementation

**Stack C (`stack_c.py`):**

- Use `libclang` (Python bindings, package `libclang>=18`)
- Parse using `compile_commands.json` from `build/` (CI must build at least to the configure step; per § 9.3 CI runs `cmake -S . -B build` before invoking integrity)
- For each public struct/class:
  - Enumerate fields via cursor traversal
  - For each public method declared:
    - Confirm definition exists in `src/`
    - Confirm declaration's parameter types match definition's
- For each public field of every public struct:
  - Find references via `clang_findReferencesInFile`-equivalent over `common/common-cpp/src/`
  - If zero non-self references (the field is only assigned/read by its own struct's constructor): HARD_FAIL with diagnostic naming the field

**Stack B (`stack_b.py`):**

- Use TypeScript compiler API via the `typescript` npm package (already in `devDependencies` at root). Invoked via a Python subprocess to a small TS script `tools/integrity/integrity/cat2_contracts/ts_helper.ts` that emits JSON. (Alternative considered: `ts-morph` — rejected for v1 because adding a dep when the TS compiler API suffices is friction.)
- For each export in `common/common-web/src/index.ts`:
  - Confirm export exists in a `src/` file
  - For interfaces/types: confirm every member is used somewhere in consumer code (a per-sim grep for `.<member>` access patterns)

**Stack D (`stack_d.py`):**

- Use stdlib `ast` module
- For `common/common-py/gpusims_common/__init__.py`:
  - Parse the file and collect every `from .X import Y`, `from .X import *`
  - Resolve each name to its definition
  - For each public class: enumerate fields via `ast.ClassDef.body` iteration over `ast.Assign`/`ast.AnnAssign`
  - For each public field of each public class:
    - Grep `common/common-py/gpusims_common/**/*.py` for references to `self.<field>` or `instance.<field>` or `<ClassName>.<field>`
    - Zero references = HARD_FAIL

### 7.4 Checks (Category 2)

| Check ID | Description | Failure mode |
|----------|-------------|--------------|
| `cat2.public-symbol-used` | Every public struct/class field, free function, and member method has at least one non-trivial consumer site outside its defining class | HARD_FAIL |
| `cat2.public-symbol-def` | Every declared public symbol has a corresponding definition | HARD_FAIL |
| `cat2.re-export-match` | Re-export modules (`index.ts`, `__init__.py`) match the smoke-import contract recorded in CI | HARD_FAIL |
| `cat2.stub-label-stale` | Files with "stub"/"placeholder"/"skeleton" in their docstring or header comment match their actual implementation status | HARD_FAIL |

### 7.4.1 Per-stack check-ID suffix convention

Per the precedent set across commits 5-7, Cat 2 checks that have per-stack implementations use a stack-suffix on the check ID:

| Check ID | Stack | Implementation |
|---|---|---|
| `cat2.public-symbol-used` | Stack D (Python) | stdlib `ast` |
| `cat2.public-symbol-used-c` | Stack C (C++) | libclang |
| `cat2.public-symbol-used-ts` | Stack B (TypeScript) | TS compiler API via Node subprocess |

The unsuffixed `cat2.public-symbol-used` defaults to Stack D since it was the first to land. Future Cat 2 checks added per-stack should follow this convention: unsuffixed for Stack D, `-c` for Stack C, `-ts` for Stack B.

## 8. Category 3: Numerical correctness vs upstream

### 8.1 What's checked

For each numerical algorithm in the repo that claims an upstream reference, Cat 3 evaluates the implementation at a small set of canonical test inputs and compares against either:

- **Algebraically-derived expected values** (when the math is closed-form and architect-1 + Claude Code agree on the derivation, with the derivation recorded in `tools/integrity/docs/ground-truth-sources.md`), OR
- **Numerically-computed expected values from the vendored upstream reference** (when the upstream code is compilable and executable in CI, with the binary checked into `cat3_numerical/expected_values/` as a pinned blob)

### 8.2 Ground truth sources (v1 set)

For v1, the following ground-truth sources are registered. Each has:

- An upstream reference
- A documented anchor (version + SHA)
- A test-input table
- An expected-output table derived from the upstream

**v1 sources:**

| Source | Vendor root | Anchor | Used by |
|--------|-------------|--------|---------|
| SPlisHSPlasH `CubicKernel::W` | `references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h:14-91` | 2.16.1 / 6bff55a6 | sph-water `kernel_W` in all DFSPH shaders |
| SPlisHSPlasH `CubicKernel::gradW` | `references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h:62-85` | 2.16.1 / 6bff55a6 | sph-water `kernel_gradW` in all DFSPH shaders |
| Algebraic | (closed-form, see `docs/ground-truth-sources.md`) | n/a | Morton encode/decode, prefix-sum correctness |

The v1 set is deliberately small. Adding a new source requires:

1. Vendoring the upstream (or pinning algebraic derivation in docs)
2. Adding a `<name>.toml` to `cat3_numerical/checks/`
3. Implementing the per-stack runner code
4. Updating `docs/ground-truth-sources.md`

This is the rate-limiting factor for Cat 3 expansion. By design.

### 8.3 Golden value table format

Each Cat 3 check has a `.toml` config:

```toml
# tools/integrity/integrity/cat3_numerical/checks/cubic_kernel.toml
[meta]
check_id = "cat3.cubic-kernel"
description = "GPU kernel_W and kernel_gradW match SPlisHSPlasH CubicKernel"
# integrity-allow: cat1.upstream-citation; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
ground_truth = "SPlisHSPlasH 2.16.1 SPHKernels.h:14-91"
failure_mode = "HARD_FAIL"  # opt-in upgrade from SOFT_WARN default
tolerance_relative = 1.0e-6
tolerance_absolute = 1.0e-9

[stacks]
c = "stack_c_driver/checks/cubic_kernel.cpp"
b = "stack_b_driver/checks/cubic_kernel.ts"
d = "stack_d_runner.py::test_cubic_kernel"

[[test_points]]
name = "knot_q_0.5"
h = 0.04
r_x = 0.02
r_y = 0.0
r_z = 0.0
expected_W = "computed at build time from references/"
expected_gradW_x = "computed at build time from references/"

[[test_points]]
name = "interior_q_0.25"
h = 0.04
r_x = 0.01
r_y = 0.0
r_z = 0.0
expected_W = "..."
expected_gradW_x = "..."
```

The `"computed at build time from references/"` placeholder is filled by a build-time step that compiles a tiny C++ harness against `references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h`, evaluates the kernel at each test point, and writes the results into a sibling `cubic_kernel.expected.json`. This is checked into the repo per § 8.4.

### 8.4 Expected-value generation

The build-time step lives at `tools/integrity/integrity/cat3_numerical/regenerate_expected.py`. It is **not** part of the per-commit CI run; it is run manually when the upstream anchor changes (and verified to produce the same output via a hash check).

Workflow:

1. Maintainer updates `references/SPlisHSPlasH` (e.g., upstream bump)
2. Maintainer runs `python tools/integrity/integrity/cat3_numerical/regenerate_expected.py`
3. The script compiles the per-source harnesses, executes them, writes `<check>.expected.json` files
4. The script's output includes a SHA-256 of each `.expected.json` for review
5. Maintainer commits the changed `.expected.json` files plus the updated `ground-truth-sources.md` registry

CI's per-commit Cat 3 run only **consumes** these expected values; it does not regenerate them. Regeneration is a deliberate human action with its own review pattern.

### 8.5 Per-stack runners

**Stack C:** A small CMake target `integrity_cat3_stack_c` builds a driver executable that includes each per-check `.cpp` file. Each implementation file exposes a `extern "C" double evaluate(...)` symbol or similar; the driver iterates test points and emits JSON to stdout. The Python runner parses stdout and compares against expected.

**Stack B:** A Vite-built TS bundle. WebGPU is not exercised in CI (there is no GPU on GitHub Actions Ubuntu runners). Stack B Cat 3 v1 tests **CPU-side computations only** — e.g., the host code that prepares shader uniforms. GPU kernel correctness via Cat 3 is deferred to v2 when a `dawn` headless WebGPU driver path is added.

**Stack D:** Pytest fixtures under `common/common-py/tests/` and per-sim `tests/`. Cat 3 tests run alongside existing Stack D tests as a distinguished pytest mark (`@pytest.mark.integrity_cat3`). The runner collects results via pytest's JSON reporter and merges into the toolkit's JSON output.

### 8.6 Checks (Category 3 v1)

| Check ID | Stack(s) | Description |
|----------|----------|-------------|
| `cat3.cubic-kernel` | C, D | GPU/Taichi kernel_W and kernel_gradW match SPlisHSPlasH CubicKernel within 1e-6 relative |
| `cat3.morton-encode` | C, B, D | Morton/Z-order encode of (x,y,z) matches reference algebraic derivation in `docs/ground-truth-sources.md` |
| `cat3.prefix-sum` | C, B | Two-level prefix-sum on a 1024-element test array agrees with `numpy.cumsum` to bit-identical precision |

This is the v1 set. It's small. By design — v1's Cat 3 establishes the infrastructure and adds checks only where they have immediate value against known-fabrication-shape risks.

## 9. CI integration

### 9.1 New workflow

`.github/workflows/integrity.yml`:

```yaml
name: Integrity

on:
  push:
    branches: [main]
  pull_request:
  workflow_dispatch:

permissions:
  contents: read
  pull-requests: write   # for PR annotations

concurrency:
  group: integrity-${{ github.ref }}
  cancel-in-progress: true

jobs:
  integrity:
    name: Cross-stack integrity checks
    runs-on: ubuntu-24.04
    timeout-minutes: 10

    steps:
      - name: Checkout
        uses: actions/checkout@v4
        with:
          submodules: false
          fetch-depth: 1

      - name: Clone vendored references (anchor-pinned)
        run: |
          mkdir -p references
          git clone --no-checkout https://github.com/InteractiveComputerGraphics/SPlisHSPlasH.git references/SPlisHSPlasH
          git -C references/SPlisHSPlasH checkout 6bff55a6eaf14083d34650f22a268ce156b62b54

      - name: Set up Python 3.11
        uses: actions/setup-python@v5
        with:
          python-version: '3.11'

      - name: Install integrity toolkit
        working-directory: tools/integrity
        run: |
          python -m pip install --upgrade pip
          pip install -e .[dev]

      - name: Install build dependencies (for cmake configure + future Cat 3 driver)
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            build-essential cmake ninja-build \
            libvulkan-dev vulkan-validationlayers \
            libgl1-mesa-dev libxinerama-dev libxcursor-dev libxi-dev libxrandr-dev \
            libwayland-dev libxkbcommon-dev \
            libimath-dev \
            spirv-tools glslang-tools

      - name: Configure Stack C build (for compile_commands.json + Cat 3 driver)
        run: |
          cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
            -DGPU_SIMS_BUILD_INTEGRITY_CAT3=ON

      - name: Build Cat 3 Stack C driver
        run: cmake --build build --target integrity_cat3_stack_c

      - name: Run integrity toolkit's own tests (dogfood)
        working-directory: tools/integrity
        run: pytest tests/ -v --cov=integrity

      - name: Run integrity toolkit against repo
        run: python3 -m integrity --output github

      - name: Upload audit log
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: integrity-audit-log
          path: docs/diagnostics/_audits/integrity_failures_*.md
          if-no-files-found: ignore
```

### 9.2 Path triggers

The integrity workflow runs on **every push and PR** without path filtering. This is consistent with `structure.yml`'s pattern (the only other always-on workflow).

Rationale: any commit can introduce a citation drift, an API contract break, or a numerical regression. Stack-specific path filters would let a Stack B-only change break a Stack C citation in a doc without triggering the check.

### 9.3 PR annotation behavior

In `--output github` mode, the toolkit emits GitHub Actions workflow commands for findings:

```
::error file=<path>,line=<n>::cat1.upstream-anchor: <message>
::warning file=<path>,line=<n>::cat3.cubic-kernel: <message>
```

GitHub renders these as inline PR annotations, making the failure location immediately visible during review.

### 9.4 Existing workflows: no changes required for v1

The integrity workflow is independent; it does not modify or interact with the existing 6 workflows. v2 may consider folding markdown's markdownlint check into a Cat 1 check, but v1 keeps the existing markdown workflow as-is.

## 10. The toolkit's own tests

Every check has at least:

- **One positive test:** synthetic fixture where the check should pass
- **One negative test:** synthetic fixture where the check should fail, asserting both the failure verdict and the exact diagnostic message
- **Annotation suppression test:** the negative case suppressed with a valid annotation should pass
- **Annotation grammar tests:** invalid annotations are detected

Fixtures live under `tools/integrity/tests/fixtures/` and are deliberately isolated from the real repo. The toolkit's tests do not depend on any real repo content; this prevents test breakage when actual code changes.

The toolkit's `pyproject.toml` includes a pytest invocation that runs the tests with coverage reporting. The integrity workflow runs the toolkit's own tests as a preliminary step before running the toolkit against the repo (per § 9.1). If the toolkit's own tests fail, the workflow exits before running the toolkit against the repo. This prevents a broken toolkit from masking real failures.

## 11. Phased build sequence

The toolkit is large. It builds in eight commits, each individually reviewable, each with its own audit report. Each commit is a separate execution prompt; this spec is the contract every prompt references.

| # | Commit title | Scope |
|---|--------------|-------|
| 1 | `feat(integrity): scaffold toolkit package + runner` | `tools/integrity/` directory, `pyproject.toml`, `integrity/__main__.py`, `integrity/runner.py` stub, `integrity/common/` utilities, README. No checks implemented. CI not added yet. Toolkit's own test scaffolding (`pytest` runs and passes with zero tests). |
| 2 | `feat(integrity): Cat 1 citation parsing + intra-repo resolution` | `cat1_citations/grammar.py`, `cat1_citations/resolver.py`, `cat1_citations/checks/intra_repo.py`, `cat1_citations/checks/annotation.py`. Fixtures + tests. |
| 3 | `feat(integrity): Cat 1 upstream-citation + anchor verification` | `cat1_citations/upstream_anchor.py`, `cat1_citations/checks/upstream.py`, `cat1_citations/checks/upstream_anchor.py`, `cat1_citations/checks/unregistered_upstream.py`. Ground-truth-sources registry with SPlisHSPlasH entry. Tests. |
| 4 | `feat(integrity): grandfather pre-v1 findings + CI integration` | One-time sweep: every existing finding gets a grandfather annotation, every annotation has a GitHub issue tracked. `.github/workflows/integrity.yml` lands. Cat 1 active in CI. |
| 5 | `feat(integrity): Cat 2 Stack D contract verification` | `cat2_contracts/stack_d.py` + checks. Stack D first because it has the simplest AST tooling (stdlib `ast`). Smoke-imports + `__init__.py` re-exports. |
| 6 | `feat(integrity): Cat 2 Stack C contract verification` | `cat2_contracts/stack_c.py` via libclang. Public-field-read check (catches the radii defect class). |
| 7 | `feat(integrity): Cat 2 Stack B contract verification` | `cat2_contracts/stack_b.py` via TypeScript compiler API. `index.ts` re-exports. |
| 8 | `feat(integrity): Cat 3 cubic-kernel numerical correctness` | `cat3_numerical/` infrastructure, expected-value generator, `cubic_kernel.toml`, all three per-stack runners. The infrastructure is established here; additional Cat 3 checks are follow-up commits. |

After commit 8, the toolkit's v1 is feature-complete. Subsequent commits add additional Cat 3 checks one at a time (morton-encode, prefix-sum, …). Cat 4 work waits per § 2.2.

### 11.1 Per-commit verification gates

Every commit's execution prompt produces an audit report at `docs/diagnostics/_audits/integrity_build_<N>_landing_<date>.md` per the pattern Phase 11.5 used.

Each commit must:

- Pass its own added tests (fixtures + checks land together)
- Pass all prior commits' tests
- Not break the existing 6 CI workflows
- Pass `pytest tools/integrity/tests/` end-to-end

The grandfather sweep (commit 4) is the boundary commit: before it, the toolkit exists but does not gate CI. After it, the toolkit gates CI for all v1 categories progressively as they land.

## 12. Existing fabrication cases the toolkit must catch

These are the named instances of Convention #8 the toolkit must mechanically detect. If a check is added and one of these cases would *not* be caught, that's a hole to fix in v1, not a v2 deferral.

| Case | Source | Cat | Check |
|------|--------|-----|-------|
| SPlisHSPlasH 1.8.10 anchor (Setup-1) | `particle-fluids/sph-water/docs/load-bearing-decisions.md:9` and 27 other citation sites | 1 | `cat1.upstream-citation` (wrong-version on every cite using `1.8.10`; `cat1.upstream-anchor` validates vendor HEAD against registry SHA, a separate concern) |
| LeniaNDK.py citation without vendoring | `continuous-ca/lenia-fft/python/lenia_fft/presets.py:11` | 1 | `cat1.intra-repo` (path doesn't resolve locally; bare-path form falls through upstream grammar — see § 6.4 note) |
| ParticleFrame::radii silent data-loss | `common/common-cpp/src/alembic_writer.cpp:51-82` | 2 | `cat2.public-symbol-used` |
| `vdb::writeVec3Grid` unexercised real impl | `common/common-cpp/src/vdb_writer.cpp:97-145` | 2 | `cat2.public-symbol-used` |
| Stale "stub" label on alembic_writer.hpp | `common/common-cpp/include/gpusims/alembic_writer.hpp` | 2 | `cat2.stub-label-stale` |
| kernel_gradW factor-of-6 (commit 1 fix) | `particle-fluids/sph-water/src/main.cpp:1349` (pre-fix) | 3 | `cat3.cubic-kernel` (catches formula-vs-implementation drift; v1 check transcribes the GLSL kernel to a host-side C++ driver and verifies driver output against analytical expected values; direct GLSL/WGSL shader-level verification is v2 candidate per § 13) |

Each row is a concrete v1 acceptance test. The toolkit must catch every one of these against synthetic fixtures during its own tests, and must catch them in the real repo when run against an artificial regression (e.g., reverting commit 1's kernel-norm fix in a test branch and asserting `cat3.cubic-kernel` HARD_FAILs).

## 13. Deferred to v2+ / out of scope permanently

### v2 candidates (revisit after v1 settles)

- **Cat 4: Runtime integration tests** — actually running binaries against canonical inputs and asserting on outputs
- **Cat 3 GPU shader coverage via headless WebGPU/Vulkan** — Stack B GPU kernels via dawn or headless Chrome; Stack C compute kernels via SwiftShader
- **Multi-line citation grammar** — citations split across lines
- **Per-sim numerical checks beyond common-*** — sim-local algorithms (boids flocking rules, RD parameter regimes, etc.)
- **Spec-vs-implementation reconciliation** — verifying phase spec claims against the actual landed code
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
<!-- integrity-allow: cat1.bare-path; toolkit-doc bare-path citation pre-v1.2 (see grandfather-catalog toolkit-doc-bare-path); n/a -->
- **Bare-path-to-upstream-basename detection** — extend Cat 1 to detect bare-path citations like `LeniaNDK.py:329-335` (no version prefix) when the basename matches a registered upstream's known files. Requires a per-upstream alias list or basename index. Surfaces fabrication-shape citations that the v1 upstream grammar misses.
- **GLSL/WGSL shader-level kernel verification** — v1 Cat 3 verifies the C++ transcription of the cubic kernel against analytical expected values. A shader-level harness that loads the actual GLSL/WGSL source, dispatches it to a compute pipeline, reads back values, and compares against expected — would catch drift between shader source and the host-side C++ driver port. Requires Vulkan/WebGPU runtime setup; significantly heavier than v1's host-only driver.

### Out of scope permanently

- Performance regression detection
- Security/dep scanning (Dependabot covers this)
- Style/lint (existing workflows cover this)
- Lookahead: catching things that haven't gone wrong yet, when the cost of the check exceeds the cost of catching the failure post-hoc via audits

## Appendix A: Ground-truth source registry (v1)

Lives at `tools/integrity/docs/ground-truth-sources.md`. Skeleton:

```toml
# Ground-truth sources for Cat 1 upstream-citation and Cat 3 numerical
# correctness. Adding a source requires (1) vendoring the upstream or
# documenting an algebraic derivation, (2) pinning the anchor, (3)
# updating this registry.

[SPlisHSPlasH]
anchor_version = "2.16.1"
anchor_sha     = "6bff55a6eaf14083d34650f22a268ce156b62b54"
vendor_root    = "references/SPlisHSPlasH"
anchor_doc     = ".gitignore"
upstream_url   = "https://github.com/InteractiveComputerGraphics/SPlisHSPlasH"
used_by_checks = ["cat3.cubic-kernel"]

# Algebraic sources: no vendor root; derivation is in docs/ground-truth-sources.md
[Algebraic_Morton30]
derivation     = "docs/algebraic/morton30.md"
used_by_checks = ["cat3.morton-encode"]
```

## Appendix B: Annotation grammar reference

Single-source-of-truth, reproduced inside `tools/integrity/docs/`.

Form per language:

| Language | Comment form |
|----------|-------------|
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
| C++ / GLSL / WGSL / TypeScript | `// integrity-allow: <check-id>; <reason>; <issue-ref>` |
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
| Python | `# integrity-allow: <check-id>; <reason>; <issue-ref>` |
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
| Markdown | `<!-- integrity-allow: <check-id>; <reason>; <issue-ref> -->` |

Field reference per § 3.2. Validation per `cat1.annotation-form` check.

## Appendix C: Mapping known audit findings to checks

One-time mapping from every finding in `docs/diagnostics/_audits/*` to either:

- A v1 check that catches it (with check ID)
- A v2 deferral (with rationale)
- "Out of scope for the toolkit" (with rationale)

This mapping is the source of truth for the grandfather sweep in commit 4. The full mapping is built during commit 4 execution by reading every prior audit report and slotting each finding. It lives at `tools/integrity/docs/grandfather-catalog.md`.
