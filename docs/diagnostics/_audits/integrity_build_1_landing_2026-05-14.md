# Integrity Toolkit — Commit 1 Landing — 2026-05-14

First of eight commits building the cross-stack integrity verification
toolkit per `docs/integrity-toolkit-spec.md` § 11. This commit scaffolds
the package, runner, common utilities, and test harness — no checks
are implemented yet.

Companion to:

- Spec: `docs/integrity-toolkit-spec.md`
- Ground-truth probe (now committed alongside this scaffold):
  `docs/diagnostics/_audits/integrity_toolkit_probe_2026-05-14_architect1.md`

---

## A. Change summary

Lands `tools/integrity/` as a complete Python package scaffold: editable-
installable via `pip install -e tools/integrity[dev]`, runnable via
`python -m integrity`, and tested by a pytest harness. The runner CLI is
fully wired (argument parsing, exit codes, output formats, mode handling)
and the common utilities — result types, canonical exclusion list,
repo/git helpers, per-stack path map, annotation-grammar parser, and
audit-log writer — are landed in their final shape. The cat1/cat2/cat3
package directories are empty placeholders so subsequent commits land
narrowly inside them without churn elsewhere. Also stages the previously
untracked integrity-toolkit ground-truth probe audit, which is the
load-bearing reference document for every subsequent commit's design
decisions.

---

## B. File inventory

Total: 27 files created, 606 lines added in tracked sources (excluding
the staged probe-audit document).

| File | Lines |
| --- | --- |
| `tools/integrity/pyproject.toml` | 66 |
| `tools/integrity/README.md` | 64 |
| `tools/integrity/integrity/__init__.py` | 3 |
| `tools/integrity/integrity/__main__.py` | 11 |
| `tools/integrity/integrity/runner.py` | 147 |
| `tools/integrity/integrity/common/__init__.py` | 0 |
| `tools/integrity/integrity/common/annotations.py` | 52 |
| `tools/integrity/integrity/common/audit_log.py` | 67 |
| `tools/integrity/integrity/common/exclusions.py` | 37 |
| `tools/integrity/integrity/common/repo.py` | 41 |
| `tools/integrity/integrity/common/results.py` | 51 |
| `tools/integrity/integrity/common/stack_paths.py` | 34 |
| `tools/integrity/integrity/cat1_citations/__init__.py` | 0 |
| `tools/integrity/integrity/cat1_citations/checks/__init__.py` | 0 |
| `tools/integrity/integrity/cat2_contracts/__init__.py` | 0 |
| `tools/integrity/integrity/cat2_contracts/checks/__init__.py` | 0 |
| `tools/integrity/integrity/cat3_numerical/__init__.py` | 0 |
| `tools/integrity/integrity/cat3_numerical/checks/__init__.py` | 0 |
| `tools/integrity/tests/conftest.py` | 12 |
| `tools/integrity/tests/test_runner.py` | 21 |
| `tools/integrity/tests/fixtures/.gitkeep` | 0 |
| `tools/integrity/tests/fixtures/bad_citations/.gitkeep` | 0 |
| `tools/integrity/tests/fixtures/bad_contracts/.gitkeep` | 0 |
| `tools/integrity/tests/fixtures/good_citations/.gitkeep` | 0 |
| `tools/integrity/tests/fixtures/good_contracts/.gitkeep` | 0 |
| `tools/integrity/tests/fixtures/numerical/.gitkeep` | 0 |
| **Total** | **606** |

Plus the staged probe audit:
`docs/diagnostics/_audits/integrity_toolkit_probe_2026-05-14_architect1.md`
(previously untracked working-tree file).

---

## C. Verification

### C.1 Editable install

```text
$ python3 -m pip install --break-system-packages -e '.[dev]'
[...]
Successfully built gpusims-integrity
Installing collected packages: libclang, tomli, ruff, pluggy, pathspec,
  mypy_extensions, librt, iniconfig, coverage, pytest, mypy,
  gpusims-integrity, pytest-cov
Successfully installed coverage-7.14.0 gpusims-integrity-0.1.0
  iniconfig-2.3.0 libclang-18.1.1 librt-0.11.0 mypy-1.20.2
  mypy_extensions-1.1.0 pathspec-1.1.1 pluggy-1.6.0 pytest-8.4.2
  pytest-cov-5.0.0 ruff-0.15.13 tomli-2.4.1
```

### C.2 pytest

```text
$ pytest tests/ -v
============================= test session starts =============================
platform linux -- Python 3.12.3, pytest-8.4.2, pluggy-1.6.0
rootdir: /home/otacon/Projects/GPU-Sims/GPU-Sims/tools/integrity
configfile: pyproject.toml
plugins: cov-5.0.0, anyio-4.13.0
collecting ... collected 3 items

tests/test_runner.py::test_runner_exits_clean_with_no_checks_registered PASSED [ 33%]
tests/test_runner.py::test_runner_rejects_bad_cli PASSED                 [ 66%]
tests/test_runner.py::test_runner_filter_by_check_runs_clean PASSED      [100%]

============================== 3 passed in 0.01s ==============================
```

### C.3 CLI smoke tests from repo root

```text
$ python3 -m integrity --output human
integrity: 0 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
Exit: 0

$ python3 -m integrity --output json
{
  "schema_version": 1,
  "commit": "56ac393",
  "summary": {
    "pass": 0,
    "soft_warn": 0,
    "hard_fail": 0,
    "suppressed": 0
  },
  "findings": []
}
Exit: 0

$ python3 -m integrity --cat 1
integrity: 0 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
Exit: 0
```

JSON output validates against the schema sketched in spec § 5.4: a
top-level `schema_version`, short-HEAD `commit`, four-bucket `summary`,
and a `findings` array (empty for commit 1).

Note: the embedded `commit` value above (`56ac393`) is the HEAD SHA at
the moment the smoke test ran — i.e., before this scaffold commit
landed. Subsequent runs from main will surface `51bd8d0` until further
commits move HEAD.

---

## D. Behavioral expectations

- **No existing CI gates are affected.** The six pre-existing workflows
  in `.github/workflows/` are untouched. The toolkit ships with no CI
  job — that lands in commit 4.
- **`python -m integrity` is non-failing for now.** The check registry
  is empty, so the runner always returns exit 0. This is by design:
  the next several commits build out checks one category at a time,
  and the grandfather sweep (also commit 4) catalogues every legacy
  finding before strict-mode gating goes live.
- **No new files are scanned, no new transforms are applied.** Nothing
  in the repo outside `tools/integrity/` is read or modified by this
  scaffold.
- **Dependencies added to the dev environment.** `libclang`, `tomli`,
  `ruff`, `mypy`, `pytest`, `pytest-cov` are installable via the
  `[dev]` extra of `tools/integrity/pyproject.toml`. They are not
  installed into any other project's environment.

---

## E. Preview — commit 2 scope

Commit 2 will implement **Category 1, intra-repo citations**:

- A citation-extractor that walks every tracked source/doc file (modulo
  the canonical exclusions in `integrity/common/exclusions.py`) and
  parses `file:line` and `file:line-line` style citations.
- `cat1.dangling-citation` — every cited path must resolve and every
  cited line range must lie within the cited file's line count at the
  current commit.
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
- `cat1.annotation-form` — every `integrity-allow:` annotation must
  parse against the grammar in `integrity/common/annotations.py`; the
  check_id, reason length (≥ 8 chars), and issue_ref form are all
  validated.
- Wires `discover_checks` in `runner.py` to import and register the
  cat1 check modules.
- Expands `tools/integrity/tests/` with fixture-driven tests for both
  good and bad citation forms (the empty `good_citations/` and
  `bad_citations/` fixture directories scaffolded here are where those
  fixtures land).

Commit 3 follows with the upstream-citation + anchor-verification
checks; commit 4 lands the grandfather sweep and the CI integration.

---

## F. Incidental findings

- Local Python is `python3` (3.12.3) — `python` is not on `$PATH`. The
  scaffold's `python -m integrity` documentation in the README is the
  canonical user-facing form, and `python3 -m integrity` works
  equivalently. Subsequent commits should keep the README's `python`
  invocation but expect contributors on Debian/Ubuntu-style systems to
  alias accordingly.
- Editable install via `pip install --break-system-packages -e .[dev]`
  proceeded cleanly on the system Python. No virtualenv is required for
  local toolkit work, though one would be wise for projects that pin
  divergent dependency versions.
- The push that landed this commit also pushed the prior local-only
  commit `56ac393` (the v1 spec landing), which had not yet reached
  `origin/main` at the time commit 1 was authored.
