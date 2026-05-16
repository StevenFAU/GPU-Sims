---
title: "Integrity Toolkit v1.2 A.2 — Execution Spec (toolkit self-application)"
date: 2026-05-15
author: architect1
status: draft
audience: Claude Code (executor)
sibling-docs:
  - docs/integrity-toolkit-spec.md
  - docs/retro/integrity-toolkit-v1.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/retro/integrity-toolkit-v1.2-bolt-ons.md
  - docs/diagnostics/_audits/integrity_v1_2_a3_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md
  - docs/integrity-toolkit-v1.3-candidates.md
---

# Integrity Toolkit v1.2 A.2 — Execution Spec (toolkit self-application)

## 0. Execution preamble

You are Claude Code executing v1.2 A.2 per the v1.1 batch-1 retro § 6.1
priority 2 (toolkit self-application). Add a new
`cat2.public-symbol-used-toolkit` check that scans the toolkit's own
Python package for declared-but-unused public symbols, refactor
`apply_annotations` to support a per-category force-sweep set
(extending P1.8), and land the v1.2 baseline grandfather sweep for
toolkit-own findings.

Land four commits plus audit reports plus SHA back-fill. Each commit
independently verifiable.

**Hard rules:**

1. **Execute every file creation and modification specified. Do not skip any.**
2. **Synced repo state is authoritative.** All API claims, file paths,
   line counts, and existing function shapes in this spec were
   grep-verified against probe SHA `67474b9298588f95497cc89765cca814598708d1`
   (the integrity_v1_2_a2_probe_2026-05-15 report's end-state SHA).
   If HEAD has advanced and any cited file has been modified, pause
   and surface; do not silently adapt.
3. **No line numbers carried from this spec into edits without re-verification.**
   Every line number cited below is captured from the probe at SHA
   `67474b9`. Before editing any line, re-anchor on the current file's
   shape via `view` or `grep -n`.
4. **Land commits in the order given.** Each commit's verification
   block must pass before starting the next. Commits 1, 2, 3 leave
   the gate in a known intermediate state; commit 4 closes it. Land
   commits 2-4 within a single PR window (no other PRs intervening),
   because commit 3 activates a check that will be red until commit 4
   sweeps.
5. **One audit report per commit.** Location:
   `docs/diagnostics/_audits/integrity_v1_2_a2_commit<N>_landing_2026-05-15.md`.
6. **SHA back-fill is a separate commit, never `--amend`.** Per
   Convention #12.
7. **Pull-rebase before every commit.** The parallel session may be
   landing other work concurrently. Run `git pull --rebase origin main`
   and `git log --oneline -10` before every edit-and-commit cycle.
8. **Audit-prose freshness check.** Before committing any audit
   report, verify every SHA and quantitative claim against current
   disk. Discrepancies become addenda, not paraphrases — the
   convention validated by the parallel session's v1.3 roadmap
   correction at `a0427d9`.
9. **Live-source-stays-red discipline.** P1.8's bucket-decision logic
   protects `tools/integrity/integrity/` paths as LIVE-SOURCE by
   default. A.2's commit 4 sweep explicitly opts in for the
   `toolkit-own-unused` category only, via the new
   `force_sweep_categories` parameter Decision 4 introduces. Do NOT
   pass `--sweep-live-source`; that would defeat P1.8 for all
   live-source other-cat1 findings, not just toolkit-own-unused.
10. **`python3`, never `python`.** Host has no `python` shim.

**Coordination context.** The v1.3 candidates roadmap landed at
`a0427d9` with the parallel session's `67474b9` bolt-ons retro
committed before that. The v1.3 roadmap's Addendum A documents an
SHA citation error (`~f661ec4` → `af248cf`) caught by the
audit-prose freshness convention. A.2 cites neither commit's audit
artifacts directly; the sibling-docs front-matter references both
for context. If the v1.3 roadmap has additional v1.2-relevant items
not surfaced here, surface them before commit 1 starts.

## 1. Goals & load-bearing decisions

### 1.1 Goal

Close the recursive blind spot identified in v1.1 batch-1 retro § 5.3
and § 3.4: the toolkit currently enforces public-symbol-used discipline
on every other Python package in the repo (`common-py/gpusims_common`)
but exempts itself. A.2 adds the missing check.

The check is a new `cat2.public-symbol-used-toolkit`, structurally
parallel to the existing Stack D check but with three critical
extraction-strategy differences (Decision 2) because the toolkit's
`__init__.py` files declare no public surface and the toolkit consumes
many symbols via reflection that a naive AST walk misses.

### 1.2 Load-bearing decisions

The following ten decisions are locked.

**Decision 1 — Design path: new check, not extended.**
Per probe § E.3 / L.1. New check at
`tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_toolkit.py`.
Path (b) (extending the existing `public_symbol_used.py`) would hide a
no-op failure mode against the toolkit's empty `__init__.py` and
conflate classifier routing. New check is the right shape.

**Decision 2 — Extraction strategy: strict scan + reflection-aware consumption.**
Per probe § B.3 / C.4 / F.1 / L.2. The naive AST walk (probe § F.1)
produces 93 candidate-unused symbols, virtually all false positives
from same-file-exclusion logic and reflection-consumed symbols. The
strict scan limits findings to real signal:

**Scan target rules** (what gets scanned for being unused):

- Top-level `def` and `class` statements only. Module-level constants
  (e.g., `INTRA_REPO_RE`, `STALE_LABEL_RE`, `ANCHOR_FILE`,
  `EXPECTED_JSON`, `MODE`, `EXIT_OK`, `EXIT_HARD_FAIL`) are NOT
  scanned. These are pervasively module-internal and produce ~70 of
  the 93 naive-walker false positives.
- Underscore-prefixed names are skipped (convention: module-private).
- `visit_*` method names inside any class that inherits from
  `ast.NodeVisitor` are skipped (consumed by framework via dispatch).
  Detection: if a `class C(ast.NodeVisitor):` declaration is the
  enclosing class, skip its `visit_*` methods.
- `test_*` function names are skipped (consumed by pytest collection;
  see also Decision 3).

**Scan input rules** (what counts as "consumption"):

A symbol is consumed if ANY of:
- It appears in any `REGISTERED_CHECKS` tuple inside any
  `checks/__init__.py` file under `tools/integrity/integrity/`.
- It is the `run`, `CHECK_ID`, or `MODE` symbol of a module whose
  qualified name appears in `REGISTERED_CHECKS`.
- It is named `main` and lives in `__main__.py` or
  `tools/integrity/scripts/`.
- It is imported by ANY `.py` file under `tools/integrity/integrity/`,
  `tools/integrity/scripts/`, or `tools/integrity/tests/`. Imports are
  detected via `ast.ImportFrom` and `ast.Import` nodes.
- It is referenced by name (`ast.Name`) or attribute access
  (`ast.Attribute`) from a module other than its defining module.

**Expected post-filter finding count:** 5-15 real findings under
this strategy (down from 93 naive). Commit 4's verification block
specifies a hard ceiling of 30 — if more than 30 surface, the
extraction is too loose and the spec needs to pause-and-surface.

**Decision 3 — Tests directory: scan-input yes, scan-target no.**
Per probe § G.2 / L.3. `tools/integrity/tests/` and
`tools/integrity/tests/fixtures/` are EXCLUDED from the scan-target
list (their `test_*` functions are consumed by pytest collection,
which AST walking can't see — including them would flag ~140 false
positives). They are INCLUDED in the scan-input list (test files DO
import toolkit symbols, and those imports count as consumption
evidence). The implementation uses the existing
`integrity.common.exclusions.is_excluded()` predicate or a new
explicit check-target filter.

`tools/integrity/scripts/` has the same treatment: scan-input only.
Its top-level `main()` functions should not flag (they're entrypoint
convention), but their imports of toolkit symbols do count as
consumption.

**Decision 4 — P1.8 coordination: per-category force-sweep set.**
Per probe § H.4 / L.4. The most consequential decision. P1.8's
`apply_annotations(repo_root, dry_run, sweep_live_source=False)`
currently uses a single boolean to override LIVE-SOURCE protection.
A.2 needs the toolkit-own-unused category specifically to be swept
during the v1.2 baseline rollout, but the protected set of other
LIVE-SOURCE categories (sim source bare paths, etc.) must remain
protected.

Refactor `apply_annotations` to accept a
`force_sweep_categories: frozenset[str] = frozenset()` parameter.
Default behavior unchanged (no categories force-swept). The CLI
adds a new flag `--force-sweep-category <name>` (repeatable).
The existing `--sweep-live-source` flag is preserved for
backwards compatibility but its semantics change to "equivalent
to `--force-sweep-category other-cat1`" — i.e., the existing
behavior is special-cased as the v1.1 default.

Both flags are mutually composable. A.2's commit 4 sweep invokes
`--force-sweep-category toolkit-own-unused` (without
`--sweep-live-source`). The result: toolkit-own-unused findings get
swept; other LIVE-SOURCE other-cat1 findings stay protected.

**Decision 5 — Classifier category: single `toolkit-own-unused`.**
Per probe § L.5. Avoid premature splits (toolkit-public-api-unused
vs toolkit-internal-unused). New classifier rule in
`grandfather.py:classify()`, inserted in the contiguous cat2 block
immediately after `cat2.stub-label-stale` (probe § H.3).

**Decision 6 — Catalog drift refresh in same commit as new category addition.**
Per probe § K.1 / L.6 / L.8. Six categories drifted notably since
the last manual refresh (notably `audit-citation` from 597 to ~80
after A.3 re-categorized audit-doc bare-path findings into the new
`audit-bare-path` category). A.2's commit that adds
`toolkit-own-unused` to the catalog is the natural place to refresh
the other stale numbers. Auto-refresh stays banked for v1.3.

**Decision 7 — `stack_paths()` grandfathered with explicit tracking note.**
Per probe § L.7. `tools/integrity/integrity/common/stack_paths.py`'s
`stack_paths()` function is a real "declared public API, no consumer"
finding. Wiring it into the existing Stack checks is the principled
answer but expands A.2's scope into a multi-module refactor.
Deletion forecloses future stack-config consolidation. The spec
chooses grandfathered-with-tracking: the toolkit-own-unused catalog
section explicitly names `stack_paths()` as the first tracked entry
with reason `"helper declared for future stack-config consolidation;
tracked for v1.3 consolidation work."`

**Decision 8 — Sanity check on post-filter finding count.**
A.2's commit 4 verification block runs the new check in warn-only
mode BEFORE running the grandfather sweep. The expected finding
count is 5-15 (under Decision 2's strict scan rules). The hard
ceiling is 30. If the actual count exceeds 30, pause-and-surface
— the extraction strategy is too loose. If the count is unexpectedly
low (under 3), pause-and-surface — the extraction is too strict and
A.2 is no-op.

**Decision 9 — `__init__.py` files unchanged.**
Per probe § B.1 / B.3. The toolkit's `__init__.py` files declare no
public surface today. A.2 does NOT modify them to add `__all__`
declarations or re-exports. Future architects may decide to make
the toolkit's public surface explicit (banked for v1.3); A.2's
scope is "add the missing check," not "re-architect the public
surface." Decision 2's strict scan strategy works against the
current docstring-only `__init__.py` files without modification.

**Decision 10 — `--probe-expected-findings` mode for verification only.**
The runner does NOT gain a new CLI flag for the expected-count
check. Decision 8's verification is implemented by Claude Code
running `python3 -m integrity --check cat2.public-symbol-used-toolkit
--output json --mode warn-only --no-audit-log` and parsing the
finding count in shell, not by adding new code to the toolkit
itself. Keeps the toolkit's CLI surface stable.

## 2. Architecture overview

### 2.1 New module

**`tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_toolkit.py`**
— new check module, ~250 LOC. Owns:

- `CHECK_ID = "cat2.public-symbol-used-toolkit"`, `MODE = FailureMode.HARD_FAIL`
- `TOOLKIT_PACKAGE_DIR = Path("tools/integrity/integrity")` (the scan-target root)
- Scan-input directories: `tools/integrity/integrity/`,
  `tools/integrity/scripts/`, `tools/integrity/tests/`
- Symbol extraction: `_extract_public_symbols(repo_root) ->
  dict[name, list[absolute_path]]` (top-level def/class only,
  per Decision 2 scan-target rules)
- Consumption tracking:
  `_build_consumption_index(repo_root, public_symbols) ->
  dict[name, set[absolute_path]]` (per Decision 2 scan-input rules)
- Reflection consumers: `_extract_registered_check_consumers(repo_root)
  -> set[str]` (parses every `checks/__init__.py` for
  `REGISTERED_CHECKS` tuples and pulls in `run`, `CHECK_ID`, `MODE`
  per registered module)
- Visitor pattern detection: `_is_ast_visitor_method(node, class_node)
  -> bool` (skips `visit_*` methods in `ast.NodeVisitor` subclasses)
- `run(repo_root: Path) -> list[Finding]`

### 2.2 Modified modules

**`tools/integrity/integrity/grandfather.py`** —
- Add new classifier rule for `cat2.public-symbol-used-toolkit` →
  `toolkit-own-unused` (Decision 5)
- Refactor `apply_annotations` signature to accept
  `force_sweep_categories: frozenset[str] = frozenset()`
- Update `is_live_source_path` callers to consult both
  `sweep_live_source` AND `force_sweep_categories` (Decision 4)

**`tools/integrity/scripts/grandfather_sweep.py`** — Add the new
`--force-sweep-category <name>` flag (repeatable via argparse
`action="append"`). The flag accumulates into a frozenset passed to
`apply_annotations`. The existing `--sweep-live-source` flag is
preserved.

**`tools/integrity/integrity/cat2_contracts/checks/__init__.py`** —
Register `public_symbol_used_toolkit` per the existing pattern
(commit 3).

**`tools/integrity/integrity/common/exclusions.py`** — Verify
existing exclusion list covers `tools/integrity/tests/fixtures/`
correctly. If not, add. (May be a no-op; probe didn't surface this
as a defect, only flagged it as a check item for spec drafting.)

**`tools/integrity/docs/grandfather-catalog.md`** — Add new
`toolkit-own-unused` section (Decision 5 + Decision 7); refresh
six drifted category counts (Decision 6).

### 2.3 New fixtures

Two new fixture trees under `tools/integrity/tests/fixtures/`:

- `good_toolkit_self/` — fixture with a mock toolkit-style package
  where all public symbols are consumed; check yields zero findings
- `bad_toolkit_self/` — fixture with public symbols that are
  declared but not consumed; check yields findings for each

Each fixture mirrors the production layout: a top-level mock
`integrity/` package directory plus a mock `scripts/` and `tests/`
directory. The fixtures don't have `.git/`, so the check falls
through to the `rglob('*')` branch (matching the existing Stack D
fixture pattern).

### 2.4 New tests

**`tools/integrity/tests/test_cat2_public_symbol_used_toolkit.py`** —
~250 LOC, ~16 tests. Coverage:

```
test_check_id_and_mode                                    # smoke
test_extract_public_symbols_includes_top_level_def        # extraction
test_extract_public_symbols_includes_top_level_class      # extraction
test_extract_public_symbols_excludes_module_constants     # extraction
test_extract_public_symbols_excludes_underscore_prefixed  # extraction
test_extract_public_symbols_excludes_visit_methods        # extraction
test_extract_public_symbols_excludes_test_functions       # extraction
test_consumption_index_records_imports                    # consumption
test_consumption_index_records_name_references            # consumption
test_consumption_index_records_attribute_access           # consumption
test_registered_checks_treated_as_consumers               # reflection
test_main_in_entrypoints_treated_as_consumed              # reflection
test_good_toolkit_self_yields_no_findings                 # fixture: good
test_bad_toolkit_self_emits_findings_for_unused           # fixture: bad
test_bad_toolkit_self_does_not_emit_for_consumed          # fixture: negative
test_real_repo_finding_count_in_expected_range            # smoke (skips in CI)
```

The last test is a smoke test that runs the check against the real
repo and asserts the finding count is in [3, 30]. Marked
`pytest.mark.slow` and `pytest.mark.skipif(condition, reason)` so
it doesn't run in fast-test mode but is available for local
verification.

## 3. Commit plan

| Commit | Items | Files changed | Race profile |
|---|---|---|---|
| 1 | New check module + fixtures + tests (NOT registered) | 1 new module + 2 fixture trees + 1 new test file | Race-immune (all new files) |
| 2 | Classifier rule + catalog section + `apply_annotations` refactor + sweep CLI flag | grandfather.py +1 rule + refactor, grandfather_sweep.py +flag, grandfather-catalog.md +section + 6 refreshed counts | Medium risk (touches grandfather.py + grandfather_sweep.py; pull-rebase before edit) |
| 3 | Register check in cat2 `__init__.py` | 1-line change | Low risk |
| 4 | Grandfather sweep companion (force-sweep toolkit-own-unused only) | Sweep produces toolkit-own-unused annotations; no live-source sims touched | High risk (touches every toolkit file with a finding) |

Plus SHA back-fill.

Total estimated diff: ~250 LOC new (check module) + ~250 LOC new
(tests) + 2 new fixture trees + ~50 LOC modified (grandfather.py,
grandfather_sweep.py, cat2 `__init__.py`, catalog) + commit-4
grandfather-sweep annotations on toolkit files.

## 4. Commit 1 — New check module + fixtures + tests

### 4.1 New file: `tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_toolkit.py`

Mirror the style of `public_symbol_used.py` (Stack D) and `intra_repo.py`.

```python
"""Check: cat2.public-symbol-used-toolkit — toolkit self-application.

Mode: HARD_FAIL.

Scans the integrity toolkit's own Python package for public symbols
(top-level def / class) that are declared but never consumed. Closes
the recursive blind spot identified in v1.1 batch-1 retro section 5.3:
the toolkit enforces public-symbol-used discipline on every other
Python package in the repo but exempts itself.

Scan-target scope (what gets scanned for being unused):
  - tools/integrity/integrity/**/*.py (excluding tests/, fixtures/)
  - Top-level `def` and `class` only; module-level constants skipped
  - Underscore-prefixed names skipped
  - `visit_*` methods on ast.NodeVisitor subclasses skipped
  - `test_*` functions skipped (consumed by pytest collection)
  - `main` in entrypoint files skipped (entrypoint convention)

Scan-input scope (what counts as consumption):
  - imports from any .py under tools/integrity/{integrity,scripts,tests}
  - Name and Attribute references from any module other than the
    defining one
  - REGISTERED_CHECKS tuples in any checks/__init__.py treat the
    referenced module's run / CHECK_ID / MODE as consumed
  - main in __main__.py or scripts/ is treated as consumed

Per v1.2 A.2 spec Decision 2 (strict + reflection-aware) and Decision 3
(tests as scan-input only).
"""

from __future__ import annotations

import ast
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

from integrity.common.exclusions import is_excluded
from integrity.common.repo import list_tracked_files
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat2.public-symbol-used-toolkit"
MODE = FailureMode.HARD_FAIL


# Scan-target root: the toolkit's own Python package.
TOOLKIT_PACKAGE_DIR = Path("tools/integrity/integrity")

# Scan-input roots: directories whose imports count as "consumption."
# Tests and scripts are scan-input only; their own symbols are not
# scanned per Decision 3.
SCAN_INPUT_DIRS = (
    Path("tools/integrity/integrity"),
    Path("tools/integrity/scripts"),
    Path("tools/integrity/tests"),
)

# Entrypoint-convention names: never flagged as unused.
ENTRYPOINT_NAMES = frozenset({"main", "__main__"})


@dataclass(frozen=True)
class PublicSymbol:
    """A top-level def or class declared in toolkit code."""
    name: str
    file: Path
    line: int


def _is_ast_visitor_class(class_node: ast.ClassDef) -> bool:
    """True if the class inherits (directly) from ast.NodeVisitor."""
    for base in class_node.bases:
        if isinstance(base, ast.Attribute):
            # e.g., ast.NodeVisitor
            if (isinstance(base.value, ast.Name)
                    and base.value.id == "ast"
                    and base.attr == "NodeVisitor"):
                return True
        elif isinstance(base, ast.Name):
            # e.g., from ast import NodeVisitor
            if base.id == "NodeVisitor":
                return True
    return False


def _is_test_function(name: str) -> bool:
    """True if the name follows pytest collection conventions."""
    return name.startswith("test_")


def _extract_public_symbols(repo_root: Path) -> list[PublicSymbol]:
    """Walk tools/integrity/integrity/ collecting top-level def/class.

    Per Decision 2 scan-target rules:
      - Top-level def / class only
      - No underscore-prefixed names
      - No visit_* methods inside ast.NodeVisitor subclasses
      - No test_* functions
      - No module-level constants

    Note: This walks the FILE-LEVEL top-level only (not nested defs).
    A class's methods are scanned (a public method on a public class
    is still scanned) UNLESS the class is an ast.NodeVisitor subclass
    AND the method is a visit_* method.
    """
    symbols: list[PublicSymbol] = []
    target_abs = repo_root / TOOLKIT_PACKAGE_DIR

    if not target_abs.is_dir():
        return symbols

    for py_file in target_abs.rglob("*.py"):
        if "__pycache__" in py_file.parts:
            continue
        try:
            text = py_file.read_text(encoding="utf-8", errors="replace")
            tree = ast.parse(text)
        except (OSError, SyntaxError):
            continue

        for node in tree.body:
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                name = node.name
                if name.startswith("_"):
                    continue
                if _is_test_function(name):
                    continue
                if name in ENTRYPOINT_NAMES:
                    continue
                symbols.append(PublicSymbol(name=name, file=py_file,
                                            line=node.lineno))

            elif isinstance(node, ast.ClassDef):
                name = node.name
                if name.startswith("_"):
                    continue
                symbols.append(PublicSymbol(name=name, file=py_file,
                                            line=node.lineno))

                # Walk class body for public methods (skip visit_*
                # methods on ast.NodeVisitor subclasses).
                is_visitor = _is_ast_visitor_class(node)
                for child in node.body:
                    if isinstance(child, (ast.FunctionDef,
                                           ast.AsyncFunctionDef)):
                        if child.name.startswith("_"):
                            continue
                        if is_visitor and child.name.startswith("visit_"):
                            continue
                        # Class methods are not scanned in v1.2 A.2;
                        # only top-level def/class. Defer method-level
                        # scanning to v1.3 if needed.

    return symbols


def _extract_registered_check_consumers(repo_root: Path) -> set[str]:
    """Parse every checks/__init__.py for REGISTERED_CHECKS tuples.

    Returns a set of qualified module names whose `run`, `CHECK_ID`,
    and `MODE` should be treated as consumed by reflection.
    """
    consumed: set[str] = set()
    target_abs = repo_root / TOOLKIT_PACKAGE_DIR

    for init_file in target_abs.rglob("checks/__init__.py"):
        try:
            text = init_file.read_text(encoding="utf-8")
            tree = ast.parse(text)
        except (OSError, SyntaxError):
            continue

        for node in ast.walk(tree):
            # Find: REGISTERED_CHECKS = [ (cid, module), ... ]
            if isinstance(node, ast.Assign):
                for target in node.targets:
                    if (isinstance(target, ast.Name)
                            and target.id == "REGISTERED_CHECKS"):
                        if isinstance(node.value, ast.List):
                            for elt in node.value.elts:
                                if isinstance(elt, ast.Tuple) and len(elt.elts) >= 2:
                                    # Second tuple element is the module name
                                    mod_ref = elt.elts[1]
                                    if isinstance(mod_ref, ast.Name):
                                        consumed.add(mod_ref.id)
                                    elif isinstance(mod_ref, ast.Attribute):
                                        # e.g., bare_path (from import)
                                        consumed.add(mod_ref.attr)

    # For each consumed module name, the conventional check API is:
    #   run, CHECK_ID, MODE
    # We treat all three as consumed by reflection.
    # (The naming convention is enforced by discover_checks in
    # runner.py.)
    return consumed


def _build_consumption_index(
    repo_root: Path,
    public_symbols: list[PublicSymbol],
) -> dict[str, set[Path]]:
    """For each public symbol, build the set of files that consume it.

    A "consumer" is any file under SCAN_INPUT_DIRS that:
      - imports the symbol by name, OR
      - references it via ast.Name or ast.Attribute

    Self-references (file referencing its own symbol) are excluded.
    """
    consumers: dict[str, set[Path]] = defaultdict(set)
    symbol_names = {sym.name for sym in public_symbols}
    defining_files = {sym.name: sym.file for sym in public_symbols}

    for input_root in SCAN_INPUT_DIRS:
        root_abs = repo_root / input_root
        if not root_abs.is_dir():
            continue
        for py_file in root_abs.rglob("*.py"):
            if "__pycache__" in py_file.parts:
                continue
            try:
                text = py_file.read_text(encoding="utf-8", errors="replace")
                tree = ast.parse(text)
            except (OSError, SyntaxError):
                continue

            for node in ast.walk(tree):
                if isinstance(node, ast.ImportFrom):
                    for alias in node.names:
                        if alias.name in symbol_names:
                            if py_file != defining_files.get(alias.name):
                                consumers[alias.name].add(py_file)
                elif isinstance(node, ast.Import):
                    for alias in node.names:
                        if alias.name in symbol_names:
                            if py_file != defining_files.get(alias.name):
                                consumers[alias.name].add(py_file)
                elif isinstance(node, ast.Name):
                    if node.id in symbol_names:
                        if py_file != defining_files.get(node.id):
                            consumers[node.id].add(py_file)
                elif isinstance(node, ast.Attribute):
                    if node.attr in symbol_names:
                        if py_file != defining_files.get(node.attr):
                            consumers[node.attr].add(py_file)

    return consumers


def run(repo_root: Path) -> list[Finding]:
    """Scan toolkit code; return findings for unused public symbols."""
    findings: list[Finding] = []

    public_symbols = _extract_public_symbols(repo_root)
    consumption = _build_consumption_index(repo_root, public_symbols)
    reflection_consumed = _extract_registered_check_consumers(repo_root)

    for sym in public_symbols:
        # Reflection consumers: run/CHECK_ID/MODE of any registered check
        # module are always consumed.
        if sym.name in ("run", "CHECK_ID", "MODE"):
            # Check if defining file is in a registered checks/ subdirectory
            try:
                rel = sym.file.relative_to(repo_root)
                if "/checks/" in str(rel).replace("\\", "/"):
                    continue
            except ValueError:
                pass

        consumers_count = len(consumption.get(sym.name, set()))
        if consumers_count > 0:
            continue

        try:
            file_rel = str(sym.file.relative_to(repo_root))
        except ValueError:
            file_rel = str(sym.file)

        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=file_rel,
            line=sym.line,
            message=(
                f"public symbol '{sym.name}' declared but has no consumer "
                f"outside its defining module"
            ),
            ground_truth_ref=None,
        ))

    return findings
```

**Verbatim claims to confirm against synced source before editing:**

- `from integrity.common.exclusions import is_excluded` — confirmed by
  existing Stack D check pattern per probe § D.2.
- `from integrity.common.repo import list_tracked_files` — confirmed.
- `from integrity.common.results import FailureMode, Finding` — confirmed.
- `Finding(check_id=..., mode=..., file=..., line=..., message=..., ground_truth_ref=None)`
  — Finding's constructor shape verified via probe-cited usage in
  bare_path.py and intra_repo.py.

### 4.2 New fixtures

#### `tools/integrity/tests/fixtures/good_toolkit_self/`

```
good_toolkit_self/
├── integrity/
│   ├── __init__.py
│   ├── module_a.py        (defines `helper_a`; consumed by module_b)
│   └── module_b.py        (imports and calls helper_a)
└── tests/
    └── test_module_a.py   (imports helper_a — counts as consumption)
```

`integrity/__init__.py`: empty file.

`integrity/module_a.py`:
```python
def helper_a(x: int) -> int:
    return x * 2
```

`integrity/module_b.py`:
```python
from integrity.module_a import helper_a

def main_b() -> int:
    return helper_a(21)
```

`tests/test_module_a.py`:
```python
from integrity.module_a import helper_a

def test_helper_a_doubles():
    assert helper_a(3) == 6
```

But there's a problem: `main_b` is itself a top-level def. It's not
named `main` (entrypoint convention) so it doesn't get the entrypoint
exclusion. If nothing imports `main_b`, the check would flag it. So
the fixture needs another consumer for `main_b` to be a "good"
fixture.

Adjusted: add a third module `integrity/module_c.py`:
```python
from integrity.module_b import main_b

if __name__ == "__main__":
    print(main_b())
```

Expected: `run(fixture_dir)` yields zero findings.

#### `tools/integrity/tests/fixtures/bad_toolkit_self/`

```
bad_toolkit_self/
├── integrity/
│   ├── __init__.py
│   ├── module_a.py        (defines `consumed_helper` and `orphan_helper`)
│   └── module_b.py        (imports consumed_helper only)
└── tests/
    └── test_module_a.py   (imports consumed_helper only)
```

`integrity/module_a.py`:
```python
def consumed_helper(x: int) -> int:
    return x + 1


def orphan_helper(x: int) -> int:
    """This function has no consumers — should be flagged."""
    return x - 1


class OrphanClass:
    """This class also has no consumers — should be flagged."""
    pass


PRIVATE_CONSTANT = 42  # Module-level constant — should NOT be flagged
                       # (Decision 2 excludes module constants from scan).


def _underscore_helper():  # Underscore-prefixed — should NOT be flagged
    return None
```

`integrity/module_b.py`:
```python
from integrity.module_a import consumed_helper

def main_b():
    return consumed_helper(0)
```

`tests/test_module_a.py`:
```python
from integrity.module_a import consumed_helper

def test_consumed_helper():
    assert consumed_helper(0) == 1
```

Expected: `run(fixture_dir)` yields exactly 2 findings — `orphan_helper`
and `OrphanClass`. Does NOT yield findings for `PRIVATE_CONSTANT`
(module constant), `_underscore_helper` (underscore-prefixed), or
`consumed_helper` (consumed by module_b and test_module_a).

If `main_b` doesn't have its own consumer in this fixture, it will
ALSO surface as a finding. Decide: either add a third module that
consumes `main_b`, or accept 3 findings total. The test assertions
should match whatever fixture shape is built.

### 4.3 New test file: `tools/integrity/tests/test_cat2_public_symbol_used_toolkit.py`

Mirror the structure of `tests/test_cat2_stack_d.py` per probe § D.5.
Read that file first via `view` to anchor on the existing test idioms
(pytest fixtures, fixture-dir pattern, assertion style).

Expected test list (16 tests per § 2.4). Key assertions:

```python
def test_check_id_and_mode():
    from integrity.cat2_contracts.checks.public_symbol_used_toolkit import (
        CHECK_ID, MODE,
    )
    from integrity.common.results import FailureMode
    assert CHECK_ID == "cat2.public-symbol-used-toolkit"
    assert MODE == FailureMode.HARD_FAIL


def test_good_toolkit_self_yields_no_findings(fixtures_dir):
    findings = run(fixtures_dir / "good_toolkit_self")
    assert findings == [], (
        f"unexpected: {[(f.file, f.line, f.message) for f in findings]}"
    )


def test_bad_toolkit_self_emits_findings_for_unused(fixtures_dir):
    findings = run(fixtures_dir / "bad_toolkit_self")
    names = sorted({f.message.split("'")[1] for f in findings})
    assert "orphan_helper" in names
    assert "OrphanClass" in names


def test_bad_toolkit_self_does_not_emit_for_consumed(fixtures_dir):
    findings = run(fixtures_dir / "bad_toolkit_self")
    names = {f.message.split("'")[1] for f in findings}
    assert "consumed_helper" not in names
    assert "PRIVATE_CONSTANT" not in names
    assert "_underscore_helper" not in names


@pytest.mark.slow
def test_real_repo_finding_count_in_expected_range(repo_root):
    """Sanity check: post-filter finding count is in [3, 30] per Decision 8."""
    findings = run(repo_root)
    count = len(findings)
    assert 3 <= count <= 30, (
        f"finding count {count} outside expected [3, 30]; "
        f"extraction strategy may be miscalibrated"
    )
```

The last test enforces Decision 8 mechanically and runs against the
real repo. Mark `slow` so it doesn't run in fast-test mode.

### 4.4 Commit 1 verification

1. `pytest tools/integrity/tests/test_cat2_public_symbol_used_toolkit.py -v`
   — all 16 tests pass.
2. `pytest tools/integrity/tests/ -v` — full suite green; test count
   grows by ~16.
3. `python3 -m integrity --check cat2.public-symbol-used-toolkit
   --output human --no-audit-log` — **expected: unknown check or empty
   output**, because the check is NOT yet registered. This is the
   intermediate state by design.
4. `python3 -m integrity --mode strict --no-audit-log` — baseline gate
   state unchanged from pre-commit.

### 4.5 Commit 1 audit report

`docs/diagnostics/_audits/integrity_v1_2_a2_commit1_landing_2026-05-15.md`.
Mirror the v1.2 A.3 commit-1 landing report's A-E structure.

## 5. Commit 2 — Classifier rule + catalog + `apply_annotations` refactor

This is the medium-risk commit. The `grandfather.py` edits compose
with anything the parallel session may have added since the probe
SHA, but pull-rebase before editing to be safe.

### 5.1 Modified file: `tools/integrity/integrity/grandfather.py`

**Pre-edit verification:** `view tools/integrity/integrity/grandfather.py`.
Confirm the contiguous cat2 classifier rule block, the
`is_live_source_path()` function, and the `apply_annotations()`
function still exist. Re-anchor on current line numbers.

**Edit 1: Add classifier rule.** Insert immediately after the
`cat2.stub-label-stale` rule (in the contiguous cat2 block):

```python
    if cid == "cat2.public-symbol-used-toolkit":
        return Classification(
            category="toolkit-own-unused",
            reason="pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused)",
            issue_ref="n/a",
        )
```

**Edit 2: Refactor `apply_annotations` signature.** Currently per
probe § H.4:
```python
def apply_annotations(repo_root: Path, dry_run: bool,
                     sweep_live_source: bool = False):
```

Change to:
```python
def apply_annotations(
    repo_root: Path,
    dry_run: bool,
    sweep_live_source: bool = False,
    force_sweep_categories: frozenset[str] = frozenset(),
):
```

**Edit 3: Update the live-source check inside `apply_annotations`.**
Find the call site where `is_live_source_path()` is consulted (per
probe § H.4, around the LIVE-SOURCE skip logic). Currently the
predicate is:

```python
if is_live_source_path(file_path) and not sweep_live_source:
    continue  # skip live-source finding
```

Change to:

```python
if (is_live_source_path(file_path)
        and not sweep_live_source
        and category not in force_sweep_categories):
    continue  # skip live-source finding
```

The exact line will need re-anchoring; the principle is "the
live-source skip is bypassed if the finding's category is in
`force_sweep_categories`, in addition to the existing
`sweep_live_source` bypass."

**Validation:** the existing test suite includes tests for
`apply_annotations`'s LIVE-SOURCE skip behavior (per the bolt-ons
batch's P1.8 tests). All must continue to pass without modification —
the new parameter defaults to empty and the existing call sites that
don't pass `force_sweep_categories` see identical behavior.

### 5.2 Modified file: `tools/integrity/scripts/grandfather_sweep.py`

**Pre-edit verification:** `view tools/integrity/scripts/grandfather_sweep.py`
per probe § I.1. The file is ~45 LOC.

Add the new flag to argparse:

```python
    parser.add_argument(
        "--force-sweep-category",
        action="append",
        default=[],
        metavar="CATEGORY",
        help=(
            "Force-sweep findings classified into the given category, "
            "regardless of LIVE-SOURCE protection. Repeatable. "
            "Example: --force-sweep-category toolkit-own-unused. "
            "Use sparingly — this opts a category out of the P1.8 "
            "live-source attribution-not-sweep policy."
        ),
    )
```

Pass the accumulated list (converted to frozenset) to
`apply_annotations`:

```python
    files, anns, counts, live_source_skipped = apply_annotations(
        root,
        ns.dry_run,
        sweep_live_source=ns.sweep_live_source,
        force_sweep_categories=frozenset(ns.force_sweep_category),
    )
```

### 5.3 Modified file: `tools/integrity/docs/grandfather-catalog.md`

**Edit 1: Add new `toolkit-own-unused` section.** Insert in the cat2
block (alongside `cat2-stack-d-unused`, `cat2-stack-c-unused`,
`cat2-stack-b-unused`, `cat2-stub-label-stale`). Use placeholder count
`(?)`; commit 4's verification refreshes.

```markdown
### `toolkit-own-unused` (?)

**Pattern:** `cat2.public-symbol-used-toolkit` findings — public
symbols (top-level `def` or `class`) declared in
`tools/integrity/integrity/**/*.py` with no consumer in
`tools/integrity/{integrity,scripts,tests}/`. Per v1.2 A.2 Decision 2
strict scan rules: module-level constants, underscore-prefixed names,
`visit_*` methods on ast.NodeVisitor subclasses, `test_*` functions,
and `main`/`__main__` are NOT scanned.

**Why grandfathered:** v1.2 A.2 introduces the toolkit self-application
check. Findings present at A.2 landing time are pre-existing
declared-public-but-unused symbols accumulated across batches 0-1.x.
The v1.2 baseline sweep grandfathers them.

**Tracking notes:**

- `stack_paths()` at `tools/integrity/integrity/common/stack_paths.py`
  is the first tracked entry. Per v1.2 A.2 Decision 7: declared as a
  helper for future stack-config consolidation; tracked for v1.3
  consolidation work. When v1.3 wires `stack_paths()` into the Stack
  checks (consolidating the three pairs of duplicated path constants),
  this entry will resolve.

**Future treatment:** Per-entry review during v1.3 stack-config
consolidation. Auto-refresh of this section's count is banked for
v1.3 (Decision 6 keeps refresh manual in v1.2).
```

**Edit 2: Refresh six drifted category counts per Decision 6.**
After commit 4's grandfather sweep completes, run
`python3 -m integrity --grandfather-report --no-history-append`
and update each drifted category heading's `(?)` placeholder with
the actual count. The probe surfaced six drifted categories; the
actual refresh happens in commit 4's audit step.

Commit 2 leaves the counts as `(?)` placeholders; commit 4 fills them.

### 5.4 Commit 2 verification

1. `pytest tools/integrity/tests/ -v` — all tests pass. The existing
   P1.8 tests continue to pass (the refactor preserves default
   behavior).
2. `python3 -c "from integrity.grandfather import apply_annotations;
   import inspect; sig = inspect.signature(apply_annotations);
   assert 'force_sweep_categories' in sig.parameters"` — the new
   parameter is present.
3. `python3 tools/integrity/scripts/grandfather_sweep.py --help` —
   the help text shows the new `--force-sweep-category` flag.
4. `python3 -m integrity --mode strict --no-audit-log` — baseline
   gate state unchanged (the check is still not registered).
5. Smoke test the classifier:
   ```python
   python3 -c "
   from integrity.grandfather import classify
   from integrity.common.results import Finding, FailureMode
   f = Finding(check_id='cat2.public-symbol-used-toolkit',
               mode=FailureMode.HARD_FAIL,
               file='tools/integrity/integrity/foo.py',
               line=10, message='test')
   print(classify(f).category)
   "
   ```
   Expected: `toolkit-own-unused`.

### 5.5 Commit 2 audit report

`docs/diagnostics/_audits/integrity_v1_2_a2_commit2_landing_2026-05-15.md`.

## 6. Commit 3 — Register the check

### 6.1 Modified file: `tools/integrity/integrity/cat2_contracts/checks/__init__.py`

**Pre-edit verification:** `view tools/integrity/integrity/cat2_contracts/checks/__init__.py`.
Read the existing `REGISTERED_CHECKS` tuple layout. Mirror it.

Add the import and the tuple entry per the existing pattern. Specifically:

- Add `public_symbol_used_toolkit` to the imports list
- Add `(public_symbol_used_toolkit.CHECK_ID, public_symbol_used_toolkit)`
  to the `REGISTERED_CHECKS` list

### 6.2 Commit 3 verification

1. `pytest tools/integrity/tests/ -v` — all tests pass.
2. `python3 -m integrity --check cat2.public-symbol-used-toolkit
   --output json --mode warn-only --no-audit-log 2>/dev/null
   | python3 -c "
   import json, sys
   d = json.load(sys.stdin)
   print(f\"finding count: {len(d['findings'])}\")
   "`
   — **expected: a number in [3, 30] per Decision 8.**

3. **Sanity check on finding count (Decision 8 enforcement):**
   ```
   count=$(python3 -m integrity --check cat2.public-symbol-used-toolkit \
     --output json --mode warn-only --no-audit-log 2>/dev/null \
     | python3 -c "import json,sys; print(len(json.load(sys.stdin)['findings']))")
   if [ "$count" -lt 3 ] || [ "$count" -gt 30 ]; then
     echo "PAUSE: finding count $count outside expected [3, 30]"
     echo "Extraction strategy may be miscalibrated."
     echo "Inspect the findings list and verify Decision 2 sub-rules"
     echo "are working as intended before proceeding to commit 4."
     exit 1
   fi
   echo "OK: finding count $count is in expected range"
   ```

   If the count is outside [3, 30]: pause-and-surface; do not
   proceed to commit 4.

4. `python3 -m integrity --mode strict --no-audit-log` — **expected:
   the check is now active and contributes new HARD_FAILs.** The gate
   is intentionally red between commit 3 and commit 4's sweep. This
   is the same intermediate state as A.3's commit 3 / 4 pair.

5. Confirm cat1.intra-repo and cat1.bare-path findings are unchanged
   (this commit doesn't affect them).

### 6.3 Commit 3 audit report

`docs/diagnostics/_audits/integrity_v1_2_a2_commit3_landing_2026-05-15.md`.
Notable items:
- C: capture the warn-only finding count and the actual finding
  details verbatim — this is the toolkit-own-unused baseline that
  commit 4's sweep grandfathers
- D: explicitly note the intermediate red gate; commit 4 closes it
- E: confirm the finding list includes `stack_paths()` (validation
  of Decision 7) — if `stack_paths()` is NOT in the finding list, the
  extraction strategy is too loose somewhere; pause-and-surface

## 7. Commit 4 — Grandfather sweep companion + catalog count refresh

### 7.1 Pre-sweep snapshot

Capture the toolkit-own-unused finding list verbatim before running
the sweep. Used for the commit-4 audit § C.

```
python3 -m integrity --check cat2.public-symbol-used-toolkit \
  --output json --mode warn-only --no-audit-log 2>/dev/null \
  | python3 -m json.tool > /tmp/a2_pre_sweep.json

cat /tmp/a2_pre_sweep.json
```

### 7.2 Run the grandfather sweep with force-sweep flag

```
python3 tools/integrity/scripts/grandfather_sweep.py \
  --force-sweep-category toolkit-own-unused
```

**Critical:** do NOT pass `--sweep-live-source`. The force-sweep flag
opts in ONLY the toolkit-own-unused category. Other LIVE-SOURCE
other-cat1 findings (sim bare-path findings attributed to introducing
authors) remain protected per P1.8.

**Verify the sweep's output:**
- The summary should report "modified N files" where N matches the
  finding count from commit 3's verification.
- The per-category counts should include `toolkit-own-unused: N`
  with the same N.
- The "skipped as live-source" message should NOT include any
  toolkit-own-unused entries.
- The "skipped as live-source" message MAY still include 44 (or
  current value) other-cat1 entries — those are the sim-source
  bare-path findings protected by P1.8 and they remain protected.

### 7.3 Catalog count refresh

After the sweep, refresh the six drifted category counts plus the
new `toolkit-own-unused` count. Run:

```
python3 -m integrity --grandfather-report --no-history-append
```

Update each category heading's parenthetical count in
`tools/integrity/docs/grandfather-catalog.md`:

- `toolkit-own-unused (?)` → `toolkit-own-unused (<actual>)`
- For the six drifted categories (likely `audit-citation`,
  `audit-bare-path`, `other-cat1-bare-path`, possibly
  `audit-doc-1810`, `toolkit-doc-bare-path`, and one more): update
  to the live counts.

Do NOT update any catalog text other than the parenthetical counts.
The category descriptions, why-grandfathered prose, and future-treatment
notes stay unchanged.

### 7.4 Verify `stack_paths()` is in the toolkit-own-unused bucket

Per Decision 7. Verify the post-sweep state includes an
`integrity-allow:` annotation on `stack_paths.py` for
`stack_paths()`. Run:

```
grep -n "integrity-allow: cat2.public-symbol-used-toolkit" \
  tools/integrity/integrity/common/stack_paths.py
```

Expected: at least one match.

### 7.5 Final gate state

```
python3 -m integrity --mode strict --no-audit-log
```

**Expected:** the gate closes back to a state similar to pre-A.2
baseline. The toolkit-own-unused findings are now suppressed by
inline annotations. The 44 (or current value) pre-existing LIVE-SOURCE
hard-fails attributed to introducing authors remain unsuppressed by
design.

If the gate hard-fail count is HIGHER than pre-A.2 baseline (i.e.,
not all toolkit-own-unused findings got swept), pause-and-surface.
This usually means a finding was misclassified by the new classifier
rule or the force-sweep flag didn't reach all findings.

### 7.6 Commit 4 audit report

`docs/diagnostics/_audits/integrity_v1_2_a2_commit4_landing_2026-05-15.md`.
Notable items:

- A: Change summary — sweep landed, catalog counts refreshed
- B: File inventory — every file the sweep modified (most under
  `tools/integrity/integrity/`)
- C: Pre-sweep finding list verbatim (from § 7.1)
- D: P1.8 coordination — `--force-sweep-category toolkit-own-unused`
  was used; `--sweep-live-source` was NOT used; other LIVE-SOURCE
  protections preserved (confirm via the sweep output's "skipped as
  live-source" count, which should match pre-A.2 baseline minus zero)
- E: Final gate state + outstanding hard-fail attribution
- F: Decision 7 validation — `stack_paths()` annotation landed at
  expected location

## 8. Cross-cutting concerns

### 8.1 Coordination with parallel session

The parallel session may be working on v1.3 roadmap items per
`docs/integrity-toolkit-v1.3-candidates.md` (landed at `a0427d9`). The
overlap surfaces:

- **`grandfather.py`:** A.2 commit 2 edits `apply_annotations` and
  `classify()`. If the parallel session is also editing
  `grandfather.py` (e.g., for additional classifier rules from v1.3
  candidates), pull-rebase before committing and inspect for conflicts.
- **`grandfather_sweep.py`:** A.2 commit 2 adds a new flag. If the
  parallel session is also adding flags, pull-rebase and inspect.
- **`grandfather-catalog.md`:** A.2 commit 2 adds a section. Catalog
  sections are append-only; pull-rebase resolves cleanly.

No code-file overlap with other A.x items. Decision 2's strict scan
rules don't depend on A.3's cat1.bare-path implementation.

### 8.2 Backward compatibility

- All existing CHECK_IDs remain registered with unchanged semantics.
- `apply_annotations` signature gains an optional parameter; existing
  call sites (the CLI itself, any test that calls it) continue to
  work without modification because the default is empty frozenset.
- `--sweep-live-source` flag is preserved; semantics are documented
  to mean "force-sweep other-cat1" but the existing behavior is
  unchanged (because P1.8's logic still consults `sweep_live_source`
  in addition to `force_sweep_categories`).
- Grandfather sweep is idempotent.

### 8.3 CI behavior

- After commit 1: no behavioral change (check unregistered).
- After commit 2: no behavioral change (classifier rule dormant
  until commit 3 registers the check).
- After commit 3: gate red with 5-15 new HARD_FAILs from
  cat2.public-symbol-used-toolkit. CI fails on PRs landed between
  commit 3 and commit 4. **Land commits 3 and 4 in the same PR**, or
  with no intervening PRs.
- After commit 4: gate green except for pre-existing LIVE-SOURCE
  residue (per § 7.5).

CI wall-clock impact: the new check adds two AST passes over
`tools/integrity/integrity/**/*.py` (~38 files) plus one pass over
`tools/integrity/scripts/` and `tools/integrity/tests/`. Estimated
+2-5 seconds on the strict-mode CI run; well within the existing
budget.

### 8.4 Idempotence

- Commit 1: pure new files.
- Commit 2: classifier rule insert is append-only; signature change
  is one-way but the default-arg pattern keeps callers compatible.
- Commit 3: one-line registration; idempotent.
- Commit 4: grandfather sweep is idempotent per existing
  `annotation_already_present` check.

### 8.5 SHA back-fill

After all four commits land, one back-fill commit edits each audit
report's "Companion to:" / SHA cross-reference placeholders with the
actual landed SHAs. Per Convention #12: never `--amend`. The
back-fill is a separate `docs(audits): back-fill SHA cross-references
in v1.2 A.2 audits` commit.

## 9. Pre-execution checklist

Before starting commit 1:

- [ ] HEAD SHA is `a0427d9` (the v1.3 candidates roadmap landing) OR a
  descendant that hasn't modified `grandfather.py`,
  `grandfather_sweep.py`, `grandfather-catalog.md`, the cat2
  `__init__.py`, or any test_cat2_* file. If HEAD has drifted on these,
  re-anchor before editing.
- [ ] `python3 -m integrity --mode strict --no-audit-log` exits 1 with
  ~44 (or current value) hard-fails — the pre-A.2 baseline.
- [ ] `pytest tools/integrity/tests/ -q` reports all tests passing.
- [ ] `tools/integrity/tests/conftest.py` defines `fixtures_dir` and
  `repo_root` pytest fixtures.
- [ ] No uncommitted local changes in the toolkit directory.
- [ ] `.github/workflows/integrity.yml` is green on current `main`.

## 10. Out of scope

Reaffirmed deferrals:

- **A.4** (multi-line citation grammar) — separate v1.2 batch.
- **A.6** (Stack C runtime optimization) — separate v1.2 batch.
- **A.9** (audit-citation file-pattern exclusion) — architect-2
  review.
- **Stack-config consolidation** (wiring `stack_paths()` into the
  Stack checks) — v1.3 candidate per Decision 7.
- **Toolkit `__init__.py` re-export pass** — v1.3 candidate per
  Decision 9.
- **Auto-refresh of catalog category counts** — v1.3 candidate per
  Decision 6.
- **Method-level (not just top-level) public symbol scanning** —
  v1.3 candidate; A.2 scans only top-level def/class per Decision 2.
- **Cat 4** (runtime integration tests) — v2 candidate.
- **Cat 5** (spec-vs-implementation reconciliation) — v2 candidate.

## 11. Self-review checks Claude Code should run

Before declaring A.2 complete, Claude Code runs these self-review
checks (mirroring the discipline that caught the v1.3 roadmap's SHA
citation error):

**Check 1 — Sibling-doc references resolve.** For each commit's audit
report, verify every sibling-doc path in the front-matter exists at
the cited path on disk.

**Check 2 — Quantitative claims match disk.** For each commit's audit
report, verify any quantitative claim ("X findings," "N files
modified," "category count is Y") against current disk state at
audit-commit time.

**Check 3 — SHA citations resolve to expected commits.** For each
commit's audit report's SHA mentions, verify `git show <sha>` resolves
and the resolved commit's subject matches the audit's description.

**Check 4 — Decision 2 + Decision 7 + Decision 8 enforcement.**

- Decision 2: confirm via the strict test fixtures that module
  constants, underscore-prefixed names, `visit_*` methods, and
  `test_*` functions are NOT scanned. Test
  `test_extract_public_symbols_*` cases enforce this.
- Decision 7: confirm `stack_paths()` is in the post-sweep
  toolkit-own-unused findings (the canonical first tracked entry).
- Decision 8: confirm the finding count in commit 3 verification
  is in [3, 30]. If outside, pause-and-surface.

**Check 5 — Body of audit reports unedited after addendum.** Per the
v1.3 roadmap's audit-prose freshness convention: if any audit report
needs correction after landing, append an addendum rather than
editing the body. The body stays as the at-direction-time record.

If any check fails, pause-and-surface before declaring A.2 complete.
The checks themselves are mechanical — Claude Code runs them as
verification, not as judgment calls.

## 12. Acceptance criteria summary

| Item | Done when |
|---|---|
| New check registered | `python3 -m integrity --check cat2.public-symbol-used-toolkit` returns findings |
| Decision 2 strict scan works | test fixtures pass; module constants / underscore / visit_* / test_* are excluded |
| Decision 3 tests dir treatment | tests excluded as scan-target, included as scan-input |
| Decision 4 P1.8 coordination | `--force-sweep-category` flag works; other LIVE-SOURCE categories remain protected |
| Decision 5 classifier rule | `classify()` routes to `toolkit-own-unused` |
| Decision 6 catalog refresh | drifted counts updated in commit 4 |
| Decision 7 stack_paths tracked | annotation present on `stack_paths.py`; catalog notes the entry |
| Decision 8 finding count sanity | warn-only count in [3, 30]; commit 3 verification enforces |
| Sweep companion landed | toolkit-own-unused findings suppressed; LIVE-SOURCE 44-finding baseline unchanged |
| CI green | except for pre-existing LIVE-SOURCE residue (sim-author owned) |
| All 5 self-review checks pass | per § 11 |

## 13. Open questions resolved by the probe (recap)

- **L.1 Design path** → Decision 1 (new check)
- **L.2 Extraction strategy** → Decision 2 (strict + reflection-aware,
  with explicit sub-rules)
- **L.3 Tests inclusion** → Decision 3 (scan-input yes, scan-target no)
- **L.4 P1.8 coordination** → Decision 4 (per-category force-sweep)
- **L.5 Classifier category** → Decision 5 (single `toolkit-own-unused`)
- **L.6 Catalog refresh timing** → Decision 6 (same commit as new
  category addition)
- **L.7 `stack_paths()` disposition** → Decision 7 (grandfathered with
  tracking note)
- **L.8 Drift bolt-on timing** → Decision 6 (same commit)

## End of execution spec

Total estimated diff: ~250 LOC new (check module) + ~250 LOC new
(tests) + 2 new fixture trees (~80 LOC content) + ~50 LOC modified
(grandfather.py + grandfather_sweep.py + cat2 checks `__init__.py` +
grandfather-catalog.md) + commit-4 grandfather-sweep annotations
(5-15 new annotations on toolkit files).

Four commits + four audit reports + one SHA back-fill = 9 commits
total.

The two highest-risk surfaces:

- **Commit 2's `apply_annotations` refactor** is the load-bearing
  P1.8-coordination change. The default-argument pattern preserves
  backward compatibility, but the existing test suite's coverage of
  the P1.8 paths needs to stay green. If any test fails after this
  refactor, pause-and-surface — the refactor may have changed
  semantics in a way the spec didn't anticipate.

- **Commit 3's red-gate intermediate state** between commit 3 and
  commit 4 is the same pattern as A.3's commit 3 / 4 pair. Land
  commits 3 and 4 in the same PR to minimize the window. If CI
  fails on a PR landed between commit 3 and commit 4, that's the
  intermediate state asserting itself, not a defect.

The audit-prose freshness convention (validated by the parallel
session's v1.3 roadmap SHA citation correction) is the discipline
that catches the kind of fabrication that ships specs. § 11 codifies
it as mechanical self-review checks for each commit's audit report.
