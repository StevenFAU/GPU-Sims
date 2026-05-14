"""Stack D public-API surface extractor per spec § 7.2, § 7.3.

The "public" surface for Stack D is whatever common/common-py/gpusims_common/
__init__.py re-exports. This module parses that init file, locates each
re-exported symbol's definition, and enumerates the symbol's accessible
fields / methods / etc.
"""

from __future__ import annotations

import ast
from dataclasses import dataclass
from enum import Enum
from pathlib import Path


COMMON_PY_PACKAGE_DIR = Path("common/common-py/gpusims_common")
PACKAGE_NAME = "gpusims_common"


class SymbolKind(Enum):
    CLASS = "class"
    CLASS_FIELD = "class_field"
    FUNCTION = "function"
    MODULE = "module"  # Re-exported instance or submodule alias


@dataclass(frozen=True)
class PublicSymbol:
    name: str                   # Externally-visible name (after re-export)
    kind: SymbolKind
    defining_file: Path         # Where this symbol's definition lives
    defining_line: int          # Line of the def/class/assign
    parent_class: str | None    # For CLASS_FIELD, the owning class name


def extract_public_surface(repo_root: Path) -> list[PublicSymbol]:
    """Parse gpusims_common/__init__.py and return every public symbol."""
    init_path = repo_root / COMMON_PY_PACKAGE_DIR / "__init__.py"
    if not init_path.is_file():
        return []

    try:
        tree = ast.parse(init_path.read_text(encoding="utf-8"))
    except (SyntaxError, OSError):
        return []

    # Collect (submodule_name, imported_name, alias_or_None) for both
    # relative (from .X import Y) and absolute (from gpusims_common.X import Y)
    # forms.
    imports: list[tuple[str, str, str | None]] = []
    for node in ast.iter_child_nodes(tree):
        if not isinstance(node, ast.ImportFrom):
            continue
        submodule: str | None = None
        if node.level == 1 and node.module:
            submodule = node.module
        elif node.level == 0 and node.module and node.module.startswith(PACKAGE_NAME + "."):
            submodule = node.module[len(PACKAGE_NAME) + 1:]
        if submodule is None:
            continue
        for alias in node.names:
            imports.append((submodule, alias.name, alias.asname))

    symbols: list[PublicSymbol] = []
    for submodule, name, alias in imports:
        external_name = alias if alias else name
        module_file = repo_root / COMMON_PY_PACKAGE_DIR / f"{submodule}.py"
        if not module_file.is_file():
            continue
        symbols.extend(_extract_from_module(module_file, name, external_name))

    return symbols


def _extract_from_module(
    module_file: Path,
    imported_name: str,
    external_name: str,
) -> list[PublicSymbol]:
    """Find `imported_name` inside `module_file` and enumerate it."""
    try:
        tree = ast.parse(module_file.read_text(encoding="utf-8"))
    except (SyntaxError, OSError):
        return []

    out: list[PublicSymbol] = []

    for node in ast.iter_child_nodes(tree):
        if isinstance(node, ast.ClassDef) and node.name == imported_name:
            out.append(PublicSymbol(
                name=external_name,
                kind=SymbolKind.CLASS,
                defining_file=module_file,
                defining_line=node.lineno,
                parent_class=None,
            ))
            out.extend(_extract_class_fields(node, module_file, external_name))
            return out
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)) and node.name == imported_name:
            out.append(PublicSymbol(
                name=external_name,
                kind=SymbolKind.FUNCTION,
                defining_file=module_file,
                defining_line=node.lineno,
                parent_class=None,
            ))
            return out
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id == imported_name:
                    out.append(PublicSymbol(
                        name=external_name,
                        kind=SymbolKind.MODULE,
                        defining_file=module_file,
                        defining_line=node.lineno,
                        parent_class=None,
                    ))
                    return out
        if isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name) and node.target.id == imported_name:
            out.append(PublicSymbol(
                name=external_name,
                kind=SymbolKind.MODULE,
                defining_file=module_file,
                defining_line=node.lineno,
                parent_class=None,
            ))
            return out

    # If we didn't find the name as a class/function/assign, treat as MODULE
    # (e.g., a submodule alias `from . import log`).
    return [PublicSymbol(
        name=external_name,
        kind=SymbolKind.MODULE,
        defining_file=module_file,
        defining_line=1,
        parent_class=None,
    )]


def _extract_class_fields(
    class_node: ast.ClassDef,
    module_file: Path,
    class_name: str,
) -> list[PublicSymbol]:
    """Enumerate public fields of a class. Two sources:

    1. Top-level `field_name = ...` / `field_name: T = ...` in class body
       (class attributes, dataclass fields).
    2. `self.field_name = ...` inside any method (instance attributes).

    Private names (leading underscore) are excluded.
    """
    fields: list[PublicSymbol] = []
    seen_names: set[str] = set()

    def add(name: str, lineno: int) -> None:
        if name.startswith("_") or name in seen_names:
            return
        fields.append(PublicSymbol(
            name=name,
            kind=SymbolKind.CLASS_FIELD,
            defining_file=module_file,
            defining_line=lineno,
            parent_class=class_name,
        ))
        seen_names.add(name)

    for node in class_node.body:
        if isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name):
            add(node.target.id, node.lineno)
        elif isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name):
                    add(target.id, node.lineno)
        elif isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            for sub in ast.walk(node):
                if isinstance(sub, ast.Assign):
                    for target in sub.targets:
                        if (isinstance(target, ast.Attribute) and
                                isinstance(target.value, ast.Name) and
                                target.value.id == "self"):
                            add(target.attr, sub.lineno)
                elif isinstance(sub, ast.AnnAssign):
                    if (isinstance(sub.target, ast.Attribute) and
                            isinstance(sub.target.value, ast.Name) and
                            sub.target.value.id == "self"):
                        add(sub.target.attr, sub.lineno)

    return fields


def find_references(
    repo_root: Path,
    symbol: PublicSymbol,
    scan_files: list[Path],
) -> list[tuple[Path, int]]:
    """Find references to `symbol` in `scan_files`. The defining file is
    excluded for non-class-field kinds; class-field references inside the
    defining class are also excluded."""
    refs: list[tuple[Path, int]] = []
    for scan_file in scan_files:
        if not scan_file.is_file():
            continue
        try:
            src = scan_file.read_text(encoding="utf-8")
        except OSError:
            continue
        try:
            tree = ast.parse(src)
        except SyntaxError:
            continue

        if symbol.kind == SymbolKind.CLASS_FIELD:
            refs.extend(_find_field_references(tree, scan_file, symbol))
        else:
            refs.extend(_find_name_references(
                tree, scan_file, symbol.name,
                exclude_file=symbol.defining_file,
            ))

    return refs


def _find_field_references(
    tree: ast.AST,
    scan_file: Path,
    symbol: PublicSymbol,
) -> list[tuple[Path, int]]:
    """Find Attribute(attr=symbol.name) loads outside the field's defining class."""
    refs: list[tuple[Path, int]] = []

    class FieldVisitor(ast.NodeVisitor):
        def __init__(self) -> None:
            self.class_stack: list[str] = []

        def visit_ClassDef(self, node: ast.ClassDef) -> None:
            self.class_stack.append(node.name)
            self.generic_visit(node)
            self.class_stack.pop()

        def visit_Attribute(self, node: ast.Attribute) -> None:
            if (node.attr == symbol.name and
                    isinstance(node.ctx, ast.Load) and
                    (not self.class_stack or
                     self.class_stack[-1] != symbol.parent_class)):
                refs.append((scan_file, node.lineno))
            self.generic_visit(node)

    FieldVisitor().visit(tree)
    return refs


def _find_name_references(
    tree: ast.AST,
    scan_file: Path,
    name: str,
    exclude_file: Path,
) -> list[tuple[Path, int]]:
    """Find Name(name) and Attribute(attr=name) loads.

    Returns empty list when `scan_file` is the defining file itself."""
    if scan_file.resolve() == exclude_file.resolve():
        return []

    refs: list[tuple[Path, int]] = []
    for node in ast.walk(tree):
        if isinstance(node, ast.Name) and node.id == name and isinstance(node.ctx, ast.Load):
            refs.append((scan_file, node.lineno))
        elif isinstance(node, ast.Attribute) and node.attr == name and isinstance(node.ctx, ast.Load):
            refs.append((scan_file, node.lineno))
    return refs
