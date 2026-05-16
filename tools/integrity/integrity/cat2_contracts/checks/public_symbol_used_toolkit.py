"""Check: cat2.public-symbol-used-toolkit -- toolkit self-application.

Mode: HARD_FAIL.

Scans the integrity toolkit's own Python package for public symbols
(top-level def / class) that are declared but never consumed. Closes
the recursive blind spot identified in v1.1 batch-1 retro section 5.3:
the toolkit enforces public-symbol-used discipline on every other
Python package in the repo but exempts itself.

Scan-target scope (what gets scanned for being unused):
  - tools/integrity/integrity/**/*.py (excluding tests/, scripts/, fixtures/)
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

Fixture-mode fallback: if `repo_root/tools/integrity/integrity/` does
not exist (e.g., test fixtures rooted at a stand-in directory), the
check treats `repo_root` itself as the scan-target root and the only
scan-input root. Decision 3 sub-rules still apply: any file whose
relative path includes a `tests` or `scripts` component is excluded
from scan-target.
"""

from __future__ import annotations

import ast
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat2.public-symbol-used-toolkit"
MODE = FailureMode.HARD_FAIL


# Scan-target root: the toolkit's own Python package, relative to repo_root.
TOOLKIT_PACKAGE_DIR = Path("tools/integrity/integrity")

# Scan-input roots: directories whose imports count as "consumption."
# Tests and scripts are scan-input only; their own top-level symbols are not
# scanned for being unused per Decision 3.
SCAN_INPUT_DIRS = (
    Path("tools/integrity/integrity"),
    Path("tools/integrity/scripts"),
    Path("tools/integrity/tests"),
)

# Entrypoint-convention names: never flagged as unused.
ENTRYPOINT_NAMES = frozenset({"main"})

# Symbols that are conventionally consumed via REGISTERED_CHECKS reflection
# when their defining module lives under a checks/ subdirectory.
REFLECTION_SYMBOL_NAMES = frozenset({"run", "CHECK_ID", "MODE"})


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
            if (isinstance(base.value, ast.Name)
                    and base.value.id == "ast"
                    and base.attr == "NodeVisitor"):
                return True
        elif isinstance(base, ast.Name):
            if base.id == "NodeVisitor":
                return True
    return False


def _is_test_function(name: str) -> bool:
    """True if the name follows pytest collection conventions."""
    return name.startswith("test_")


def _rel_parts(file_path: Path, root: Path) -> tuple[str, ...]:
    try:
        return file_path.relative_to(root).parts
    except ValueError:
        return ()


def _is_under_tests_or_scripts(file_path: Path, root: Path) -> bool:
    """Decision-3 sub-rule: exclude tests/ and scripts/ from scan-target."""
    parts = _rel_parts(file_path, root)
    return "tests" in parts or "scripts" in parts or "fixtures" in parts


def _resolve_scan_target_root(repo_root: Path) -> Path:
    """Return the directory to walk for scan-target symbols.

    Production: `repo_root/tools/integrity/integrity` exists; use it.
    Fixture mode: that path does not exist; treat `repo_root` itself as
    the toolkit root (matching the existing Stack D fixture pattern per
    spec section 4.2 last paragraph).
    """
    canonical = repo_root / TOOLKIT_PACKAGE_DIR
    if canonical.is_dir():
        return canonical
    return repo_root


def _resolve_scan_input_roots(repo_root: Path) -> list[Path]:
    """Return the directories to walk for consumption sites."""
    roots: list[Path] = []
    for sub in SCAN_INPUT_DIRS:
        candidate = repo_root / sub
        if candidate.is_dir():
            roots.append(candidate)
    if not roots:
        roots = [repo_root]
    return roots


def _extract_public_symbols(repo_root: Path) -> list[PublicSymbol]:
    """Walk the toolkit package collecting top-level def/class.

    Per Decision 2 scan-target rules:
      - Top-level def / class only
      - No underscore-prefixed names
      - No visit_* methods inside ast.NodeVisitor subclasses (NB: methods
        are not currently scanned in v1.2 A.2; only top-level def/class)
      - No test_* functions
      - No `main` (entrypoint convention)
      - No module-level constants

    Per Decision 3:
      - Files under tests/, scripts/, or fixtures/ subdirs are skipped
        for scan-target.
    """
    symbols: list[PublicSymbol] = []
    target_abs = _resolve_scan_target_root(repo_root)

    if not target_abs.is_dir():
        return symbols

    for py_file in target_abs.rglob("*.py"):
        if "__pycache__" in py_file.parts:
            continue
        if _is_under_tests_or_scripts(py_file, target_abs):
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

    return symbols


def _extract_registered_check_modules(repo_root: Path) -> set[str]:
    """Parse every checks/__init__.py for REGISTERED_CHECKS tuples.

    Returns the set of module-reference names that appear as the second
    tuple element of a REGISTERED_CHECKS entry. Used to bless `run`,
    `CHECK_ID`, and `MODE` symbols inside those modules as consumed by
    reflection.
    """
    registered: set[str] = set()
    target_abs = _resolve_scan_target_root(repo_root)

    for init_file in target_abs.rglob("checks/__init__.py"):
        try:
            text = init_file.read_text(encoding="utf-8")
            tree = ast.parse(text)
        except (OSError, SyntaxError):
            continue

        for node in ast.walk(tree):
            if not isinstance(node, ast.Assign):
                continue
            assigns_registered = any(
                isinstance(t, ast.Name) and t.id == "REGISTERED_CHECKS"
                for t in node.targets
            )
            if not assigns_registered:
                continue
            value = node.value
            if not isinstance(value, (ast.List, ast.Tuple)):
                continue
            for elt in value.elts:
                if isinstance(elt, ast.Tuple) and len(elt.elts) >= 2:
                    mod_ref = elt.elts[1]
                    if isinstance(mod_ref, ast.Name):
                        registered.add(mod_ref.id)
                    elif isinstance(mod_ref, ast.Attribute):
                        registered.add(mod_ref.attr)

    return registered


def _file_is_under_registered_check(
    file_path: Path,
    repo_root: Path,
    registered_modules: set[str],
) -> bool:
    """True if file lives under a checks/ subdir and its stem appears
    in any REGISTERED_CHECKS tuple."""
    parts = _rel_parts(file_path, repo_root)
    if "checks" not in parts:
        return False
    return file_path.stem in registered_modules


def _file_is_entrypoint(file_path: Path, repo_root: Path) -> bool:
    """True if file is __main__.py or lives under scripts/."""
    parts = _rel_parts(file_path, repo_root)
    if file_path.name == "__main__.py":
        return True
    return "scripts" in parts


def _build_consumption_index(
    repo_root: Path,
    public_symbols: list[PublicSymbol],
) -> dict[str, set[Path]]:
    """For each public symbol name, build the set of files that consume it.

    A "consumer" is any file under SCAN_INPUT_DIRS that:
      - imports the symbol by name (ast.ImportFrom or ast.Import), OR
      - references it via ast.Name or ast.Attribute

    Self-references (file referencing its own symbol) are excluded.
    """
    consumers: dict[str, set[Path]] = defaultdict(set)
    symbol_names = {sym.name for sym in public_symbols}
    if not symbol_names:
        return consumers

    defining_files: dict[str, Path] = {}
    for sym in public_symbols:
        defining_files.setdefault(sym.name, sym.file)

    input_roots = _resolve_scan_input_roots(repo_root)
    seen_files: set[Path] = set()

    for root in input_roots:
        for py_file in root.rglob("*.py"):
            if "__pycache__" in py_file.parts:
                continue
            if py_file in seen_files:
                continue
            seen_files.add(py_file)

            try:
                text = py_file.read_text(encoding="utf-8", errors="replace")
                tree = ast.parse(text)
            except (OSError, SyntaxError):
                continue

            for node in ast.walk(tree):
                if isinstance(node, ast.ImportFrom):
                    for alias in node.names:
                        nm = alias.name
                        if nm in symbol_names and py_file != defining_files.get(nm):
                            consumers[nm].add(py_file)
                elif isinstance(node, ast.Import):
                    for alias in node.names:
                        nm = alias.name.split(".")[-1]
                        if nm in symbol_names and py_file != defining_files.get(nm):
                            consumers[nm].add(py_file)
                elif isinstance(node, ast.Name):
                    nm = node.id
                    if nm in symbol_names and py_file != defining_files.get(nm):
                        consumers[nm].add(py_file)
                elif isinstance(node, ast.Attribute):
                    nm = node.attr
                    if nm in symbol_names and py_file != defining_files.get(nm):
                        consumers[nm].add(py_file)

    return consumers


def run(repo_root: Path) -> list[Finding]:
    """Scan toolkit code; return findings for unused public symbols."""
    findings: list[Finding] = []

    public_symbols = _extract_public_symbols(repo_root)
    if not public_symbols:
        return findings

    consumption = _build_consumption_index(repo_root, public_symbols)
    registered_modules = _extract_registered_check_modules(repo_root)

    for sym in public_symbols:
        if sym.name in REFLECTION_SYMBOL_NAMES and _file_is_under_registered_check(
            sym.file, repo_root, registered_modules
        ):
            continue
        if sym.name in ENTRYPOINT_NAMES and _file_is_entrypoint(sym.file, repo_root):
            continue
        if consumption.get(sym.name):
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
