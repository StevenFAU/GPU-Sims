---
title: "Integrity Toolkit v1.1 API/Shape Probe"
date: 2026-05-15
author: architect1
status: probe
scope: read-only
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md
  - docs/integrity-toolkit-spec.md
---

# Integrity Toolkit v1.1 API/Shape Probe

Verbatim source listings to anchor the v1.1 execution-spec draft. Every code
block is labelled with its repo-relative path. No file in this probe exceeds
400 lines, so every listing is **complete** (no truncations).

## Section A — Check module template

### A.1 `tools/integrity/integrity/cat2_contracts/checks/public_symbol_used.py`

```python
# tools/integrity/integrity/cat2_contracts/checks/public_symbol_used.py
"""Check: cat2.public-symbol-used — every public symbol has a consumer.

Mode: HARD_FAIL.

Covers two defect shapes:
  - Silent-data-loss fields (e.g. ParticleFrame.radii): public class
    field declared but never read by any consumer
  - Defined-but-unexercised public functions (e.g. vdb::writeVec3Grid):
    public function declared and implemented but never called

This commit (commit 5) implements Stack D only. Stack C and Stack B
follow in commits 6 and 7.

Known false-positive class:
  - A field name that happens to match an unrelated class's field
    name. AST-based matching is not type-aware in v1; v2 may add
    type-aware matching via mypy's API. This is a false-MISS
    direction (check passes when it shouldn't), not false-FAIL.

Known false-negative class:
  - Symbols accessed via `getattr(instance, "name")` or string-based
    introspection. Out of scope for v1.
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat2_contracts.stack_d import (
    extract_public_surface,
    find_references,
)
from integrity.common.exclusions import is_excluded
from integrity.common.repo import list_tracked_files
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat2.public-symbol-used"
MODE = FailureMode.HARD_FAIL


def _list_scannable_py_files(repo_root: Path) -> list[Path]:
    """List all .py / .pyi files in the repo, excluding canonical exclusions."""
    if (repo_root / ".git").exists():
        all_files = list_tracked_files(repo_root)
    else:
        all_files = [p for p in repo_root.rglob("*") if p.is_file()]

    out: list[Path] = []
    for absolute in all_files:
        try:
            rel = str(absolute.relative_to(repo_root))
        except ValueError:
            continue
        if is_excluded(rel):
            continue
        if absolute.suffix not in (".py", ".pyi"):
            continue
        out.append(absolute)
    return out


def run(repo_root: Path) -> list[Finding]:
    findings: list[Finding] = []

    public_symbols = extract_public_surface(repo_root)
    if not public_symbols:
        return findings

    scan_files = _list_scannable_py_files(repo_root)

    for symbol in public_symbols:
        refs = find_references(repo_root, symbol, scan_files)
        if refs:
            continue

        try:
            rel = str(symbol.defining_file.relative_to(repo_root))
        except ValueError:
            rel = str(symbol.defining_file)

        symbol_descriptor = (
            f"{symbol.parent_class}.{symbol.name}"
            if symbol.parent_class
            else symbol.name
        )
        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=rel,
            line=symbol.defining_line,
            message=(
                f"public {symbol.kind.value} '{symbol_descriptor}' has no "
                f"non-self consumer site under common/common-py/, sim "
                f"Python packages, examples, or tests"
            ),
        ))

    return findings
```

### A.2 `tools/integrity/integrity/cat2_contracts/checks/__init__.py`

```python
# tools/integrity/integrity/cat2_contracts/checks/__init__.py
"""Cat 2 check modules. Discovered by integrity.runner.discover_checks."""

from integrity.cat2_contracts.checks import (
    public_symbol_used,
    public_symbol_used_b,
    public_symbol_used_c,
)

REGISTERED_CHECKS = [
    (public_symbol_used.CHECK_ID, public_symbol_used),
    (public_symbol_used_c.CHECK_ID, public_symbol_used_c),
    (public_symbol_used_b.CHECK_ID, public_symbol_used_b),
]
```

### A.3 `tools/integrity/integrity/cat2_contracts/stack_d.py`

```python
# tools/integrity/integrity/cat2_contracts/stack_d.py
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
```

## Section B — Grandfather machinery

### B.1 `tools/integrity/integrity/grandfather.py`

```python
# tools/integrity/integrity/grandfather.py
"""Grandfather-sweep logic for the integrity toolkit v1.

Classifies HARD_FAIL findings into one of seven pre-v1 categories and
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
generates inline `integrity-allow:` annotations on the cited source
lines. See `tools/integrity/docs/grandfather-catalog.md` for the
per-category rationale.

Imported by `tools/integrity/scripts/grandfather_sweep.py` (the CLI
wrapper) and by the unit tests under `tools/integrity/tests/`.
"""

from __future__ import annotations

import json
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


@dataclass(frozen=True)
class Finding:
    check_id: str
    file: str
    line: int
    message: str


@dataclass(frozen=True)
class Classification:
    category: str
    reason: str
    issue_ref: str


def classify(finding: Finding) -> Classification:
    """Classify a finding into a grandfather category. First match wins."""
    f = finding.file
    msg = finding.message
    cid = finding.check_id

    if cid == "cat2.public-symbol-used":
        return Classification(
            category="cat2-stack-d-unused",
            reason="pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused)",
            issue_ref="n/a",
        )

    if cid == "cat2.public-symbol-used-c":
        return Classification(
            category="cat2-stack-c-unused",
            reason="pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused)",
            issue_ref="n/a",
        )

    if cid == "cat2.public-symbol-used-ts":
        return Classification(
            category="cat2-stack-b-unused",
            reason="pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused)",
            issue_ref="n/a",
        )

    if cid == "cat1.intra-repo" and f.startswith("docs/diagnostics/_audits/"):
        return Classification(
            category="audit-citation",
            reason="audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation)",
            issue_ref="n/a",
        )

    if cid == "cat1.upstream-citation" and "1.8.10" in msg:
        if (
            f.startswith("particle-fluids/sph-water/shaders/")
            or f.startswith("particle-fluids/sph-water/src/")
        ):
            return Classification(
                category="live-shader-1810",
                reason="pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (migration target tracked in grandfather-catalog live-shader-1810)",
                issue_ref="n/a",
            )
        return Classification(
            category="audit-doc-1810",
            reason="audit-doc reference to the historical 1.8.10 fabrication (permanent suppression)",
            issue_ref="n/a",
        )

    if cid == "cat1.annotation-form":
        if f == "docs/integrity-toolkit-spec.md" or f.startswith("tools/integrity/docs/"):
            return Classification(
                category="spec-grammar-example",
                reason="documentation-only literal mention of the annotation grammar (not a real annotation)",
                issue_ref="n/a",
            )
        if f.startswith("docs/retro/"):
            return Classification(
                category="retro-grammar-example",
                reason="retrospective-doc literal mention of the annotation grammar (not a real annotation)",
                issue_ref="n/a",
            )
        if f.startswith("tools/integrity/integrity/"):
            return Classification(
                category="toolkit-own-source",
                reason="regex or docstring literal of the annotation grammar token (not a real annotation)",
                issue_ref="n/a",
            )
        if f.startswith("docs/diagnostics/_audits/"):
            return Classification(
                category="audit-report-grammar-example",
                reason="audit-doc literal mention of the annotation grammar (not a real annotation)",
                issue_ref="n/a",
            )

    return Classification(
        category="other-cat1",
        reason="grandfathered-pre-v1 (see grandfather-catalog other-cat1)",
        issue_ref="n/a",
    )


def comment_form_for(file_path: str) -> str:
    """Return the comment-form template for a file by extension.

    Template contains `{body}` placeholder for the annotation body
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    (everything after `integrity-allow: `)."""
    p = file_path.lower()
    if p.endswith((".py", ".pyi")):
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        return "# integrity-allow: {body}"
    if p.endswith((".md", ".rst")):
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        return "<!-- integrity-allow: {body} -->"
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    return "// integrity-allow: {body}"


_FENCE_RE = re.compile(r"^\s*```(?P<lang>[A-Za-z0-9_+\-]*)\s*$")


def is_inside_fenced_block(
    lines: list[str],
    target_line_zero_indexed: int,
) -> tuple[bool, str | None]:
    """Determine whether `lines[target_line_zero_indexed]` is inside a
    fenced markdown code block. The opening-fence line itself is
    considered in-fence (we toggle on at start of match)."""
    in_fence = False
    fence_lang: str | None = None
    for i, line in enumerate(lines):
        m = _FENCE_RE.match(line)
        if m:
            if in_fence:
                if i == target_line_zero_indexed:
                    return (True, fence_lang)
                in_fence = False
                fence_lang = None
            else:
                in_fence = True
                fence_lang = m.group("lang") or ""
                if i == target_line_zero_indexed:
                    return (True, fence_lang)
                continue
        if i == target_line_zero_indexed:
            return (in_fence, fence_lang)
    return (False, None)


def comment_form_for_md_inside_fence(fence_lang: str | None) -> str:
    """Pick a comment form for an annotation inside a markdown code block."""
    if fence_lang is None:
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        return "// integrity-allow: {body}"
    lang = fence_lang.lower()
    if lang in ("python", "py", "toml", "yaml", "yml", "sh", "bash", "ini", "cfg"):
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        return "# integrity-allow: {body}"
    if lang in ("json",):
        # JSON has no comments; fall back to a JS-style line comment, which
        # the integrity parser will read but JSON validators won't.
        # Real JSON code-block annotations are best avoided.
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        return "// integrity-allow: {body}"
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    return "// integrity-allow: {body}"


def annotation_already_present(prev_line: str, check_id: str) -> bool:
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    """True if `prev_line` already carries an `integrity-allow:`
    annotation that covers `check_id` (specifically or via category
    wildcard)."""
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    if "integrity-allow:" not in prev_line:
        return False
    cat = check_id.split(".")[0]
    wildcard = f"{cat}.*"
    return check_id in prev_line or wildcard in prev_line


def render_annotation_line(
    findings_on_line: list[Finding],
    file_path: str,
    file_lines: list[str],
    line_zero_indexed: int,
) -> list[str]:
    """Render the annotation comment line(s) to insert above
    file_lines[line_zero_indexed] for the given group of same-target
    findings."""
    classifications = [(f, classify(f)) for f in findings_on_line]
    categories = {c.category for _, c in classifications}

    if file_path.lower().endswith((".md", ".rst")):
        in_fence, fence_lang = is_inside_fenced_block(file_lines, line_zero_indexed)
        if in_fence:
            template = comment_form_for_md_inside_fence(fence_lang)
        else:
            template = comment_form_for(file_path)
    else:
        template = comment_form_for(file_path)

    if len(categories) == 1:
        check_ids_on_line = {f.check_id for f in findings_on_line}
        cat_prefix = next(iter(check_ids_on_line)).split(".")[0]
        if len(check_ids_on_line) > 1:
            check_id_for_annotation = f"{cat_prefix}.*"
        else:
            check_id_for_annotation = next(iter(check_ids_on_line))
        _, cls = classifications[0]
        body = f"{check_id_for_annotation}; {cls.reason}; {cls.issue_ref}"
        return [template.format(body=body)]

    out: list[str] = []
    for f, cls in classifications:
        body = f"{f.check_id}; {cls.reason}; {cls.issue_ref}"
        out.append(template.format(body=body))
    return out


def collect_findings(repo_root: Path) -> list[Finding]:
    """Run the integrity toolkit in JSON mode and parse non-suppressed findings."""
    result = subprocess.run(
        ["python3", "-m", "integrity", "--output", "json",
         "--no-audit-log", "--mode", "warn-only"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode not in (0, 1):
        raise RuntimeError(
            f"integrity toolkit exited {result.returncode}: {result.stderr}"
        )
    data = json.loads(result.stdout)
    findings: list[Finding] = []
    for f in data.get("findings", []):
        if f.get("suppressed"):
            continue
        findings.append(Finding(
            check_id=f["check_id"],
            file=f["file"],
            line=int(f["line"]),
            message=f["message"],
        ))
    return findings


def group_findings_by_target(
    findings: Iterable[Finding],
) -> dict[tuple[str, int], list[Finding]]:
    """Group findings by (file, target_line)."""
    grouped: dict[tuple[str, int], list[Finding]] = {}
    for f in findings:
        key = (f.file, f.line)
        grouped.setdefault(key, []).append(f)
    return grouped


def apply_annotations(
    repo_root: Path,
    dry_run: bool,
) -> tuple[int, int, dict[str, int]]:
    """Apply suppression annotations for every collected finding.

    Returns (files_modified, annotations_added, category_counts)."""
    findings = collect_findings(repo_root)
    grouped = group_findings_by_target(findings)

    files_modified = 0
    annotations_added = 0
    category_counts: dict[str, int] = {}

    by_file: dict[str, list[int]] = {}
    for (fp, ln) in grouped:
        by_file.setdefault(fp, []).append(ln)
    for fp in by_file:
        by_file[fp].sort(reverse=True)

    for file_path, lines_desc in by_file.items():
        abs_path = repo_root / file_path
        if not abs_path.is_file():
            continue
        try:
            content = abs_path.read_text(encoding="utf-8")
        except OSError:
            continue
        file_lines = content.split("\n")

        modified_this_file = False

        for target_line in lines_desc:
            zero_idx = target_line - 1
            if zero_idx < 0 or zero_idx >= len(file_lines):
                continue

            findings_on_line = grouped[(file_path, target_line)]

            if zero_idx > 0:
                prev = file_lines[zero_idx - 1]
                covered = all(
                    annotation_already_present(prev, f.check_id)
                    for f in findings_on_line
                )
                if covered:
                    continue

            new_lines = render_annotation_line(
                findings_on_line, file_path, file_lines, zero_idx
            )

            file_lines[zero_idx:zero_idx] = new_lines
            annotations_added += len(new_lines)
            modified_this_file = True

            for f in findings_on_line:
                cls = classify(f)
                category_counts[cls.category] = category_counts.get(cls.category, 0) + 1

        if modified_this_file:
            files_modified += 1
            if not dry_run:
                abs_path.write_text("\n".join(file_lines), encoding="utf-8")

    return files_modified, annotations_added, category_counts
```

### B.2 `tools/integrity/scripts/grandfather_sweep.py`

```python
# tools/integrity/scripts/grandfather_sweep.py
#!/usr/bin/env python3
"""Grandfather-sweep CLI entry. Logic lives in integrity.grandfather."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from integrity.common.repo import find_repo_root
from integrity.grandfather import apply_annotations


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Grandfather-sweep integrity findings")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--repo-root", type=Path, default=None)
    ns = parser.parse_args(argv)

    root = ns.repo_root if ns.repo_root else find_repo_root()
    files, anns, counts = apply_annotations(root, ns.dry_run)

    label = "would modify" if ns.dry_run else "modified"
    print(f"grandfather-sweep: {label} {files} files; {anns} annotations added")
    for cat, n in sorted(counts.items(), key=lambda kv: -kv[1]):
        print(f"  {cat:>35s}: {n}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
```

## Section C — CLI surface

### C.1 `tools/integrity/integrity/__main__.py`

```python
# tools/integrity/integrity/__main__.py
"""Entry point: `python -m integrity`."""

from __future__ import annotations

import sys

from integrity.runner import main


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
```

### C.2 `tools/integrity/integrity/runner.py`

```python
# tools/integrity/integrity/runner.py
"""Top-level runner: parse CLI, discover checks, dispatch, summarize.

Commit 1 ships a stub runner. The check-discovery and dispatch logic
is structured but returns an empty findings list, since no checks are
registered yet. Commits 2+ will register check modules.

See docs/integrity-toolkit-spec.md § 5 for the full CLI surface.
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from integrity.common.exclusions import CANONICAL_EXCLUSIONS  # noqa: F401  (used in later commits)
from integrity.common.repo import find_repo_root, git_head_sha
from integrity.common.results import FailureMode, Finding, RunSummary


# Exit codes per spec § 5.2
EXIT_OK = 0
EXIT_HARD_FAIL = 1
EXIT_INTERNAL_FAIL = 2
EXIT_BAD_CLI = 64


@dataclass
class CliArgs:
    cat: int | None
    check: str | None
    mode: str
    root: Path
    output: str
    no_audit_log: bool


def parse_args(argv: list[str]) -> CliArgs:
    parser = argparse.ArgumentParser(
        prog="integrity",
        description="GPU-Sims integrity toolkit — cross-stack verification",
    )
    parser.add_argument("--cat", type=int, choices=[1, 2, 3], default=None,
                        help="Run only the named category")
    parser.add_argument("--check", type=str, default=None,
                        help="Run only the named check, e.g. cat1.upstream-anchor")
    parser.add_argument("--mode", choices=["strict", "warn-only"], default="strict",
                        help="strict honors HARD_FAIL/SOFT_WARN; warn-only converts all to SOFT_WARN")
    parser.add_argument("--root", type=Path, default=None,
                        help="Override repo root (default: auto-detect via git)")
    parser.add_argument("--output", choices=["human", "json", "github"], default="human",
                        help="Output format")
    parser.add_argument("--no-audit-log", action="store_true",
                        help="Skip writing to integrity_failures_<date>.md")

    ns = parser.parse_args(argv)
    return CliArgs(
        cat=ns.cat,
        check=ns.check,
        mode=ns.mode,
        root=ns.root if ns.root else find_repo_root(),
        output=ns.output,
        no_audit_log=ns.no_audit_log,
    )


def discover_checks(args: CliArgs) -> list[Any]:
    """Discover registered check modules per --cat / --check filters."""
    from integrity.cat1_citations.checks import REGISTERED_CHECKS as cat1_checks

    all_checks: list[tuple[str, Any]] = []
    if args.cat is None or args.cat == 1:
        all_checks.extend(cat1_checks)
    if args.cat is None or args.cat == 2:
        from integrity.cat2_contracts.checks import REGISTERED_CHECKS as cat2_checks
        all_checks.extend(cat2_checks)
    if args.cat is None or args.cat == 3:
        from integrity.cat3_numerical.checks import REGISTERED_CHECKS as cat3_checks
        all_checks.extend(cat3_checks)

    if args.check is not None:
        all_checks = [(cid, mod) for cid, mod in all_checks if cid == args.check]

    return all_checks


def run_checks(checks: list[Any], args: CliArgs) -> list[Finding]:
    """Execute the given checks against args.root, return all findings."""
    findings: list[Finding] = []
    for check_id, module in checks:
        try:
            check_findings = module.run(args.root)
            findings.extend(check_findings)
        except Exception as e:
            # A check-internal exception is INTERNAL_FAIL; re-raise so the
            # main() handler emits the diagnostic and exits 2.
            raise RuntimeError(f"check {check_id} raised: {e}") from e
    return findings


def emit_output(summary: RunSummary, findings: list[Finding], args: CliArgs) -> None:
    """Emit results in the chosen format."""
    if args.output == "json":
        payload = {
            "schema_version": 1,
            "commit": git_head_sha(args.root),
            "summary": {
                "pass": summary.passes,
                "soft_warn": summary.soft_warns,
                "hard_fail": summary.hard_fails,
                "suppressed": summary.suppressions,
            },
            "findings": [f.to_dict() for f in findings],
        }
        json.dump(payload, sys.stdout, indent=2)
        sys.stdout.write("\n")
    elif args.output == "github":
        for f in findings:
            if f.suppressed:
                continue
            kind = "error" if f.mode == FailureMode.HARD_FAIL else "warning"
            sys.stdout.write(
                f"::{kind} file={f.file},line={f.line}::{f.check_id}: {f.message}\n"
            )
        _emit_human_summary(summary)
    else:
        _emit_human_summary(summary)
        for f in findings:
            sys.stdout.write(f"  {f.mode.name}: {f.check_id} at {f.file}:{f.line}\n")
            sys.stdout.write(f"    {f.message}\n")


def _emit_human_summary(summary: RunSummary) -> None:
    sys.stdout.write(
        f"integrity: {summary.passes} pass, "
        f"{summary.soft_warns} soft-warn, "
        f"{summary.hard_fails} hard-fail, "
        f"{summary.suppressions} suppressed\n"
    )


def main(argv: list[str]) -> int:
    try:
        args = parse_args(argv)
    except SystemExit as e:
        return EXIT_BAD_CLI if e.code != 0 else EXIT_OK

    try:
        checks = discover_checks(args)
        findings = run_checks(checks, args)
        from integrity.common.suppression import apply_suppressions
        findings = apply_suppressions(findings, args.root)
        summary = RunSummary(
            passes=sum(1 for cid, _ in checks
                       if not any(f.check_id == cid for f in findings)),
            soft_warns=sum(1 for f in findings if f.mode == FailureMode.SOFT_WARN),
            hard_fails=sum(1 for f in findings
                           if f.mode == FailureMode.HARD_FAIL and not f.suppressed),
            suppressions=sum(1 for f in findings if f.suppressed),
        )
        emit_output(summary, findings, args)

        if summary.hard_fails > 0 and args.mode == "strict":
            return EXIT_HARD_FAIL
        return EXIT_OK
    except Exception as e:
        sys.stderr.write(f"integrity: INTERNAL_FAIL: {type(e).__name__}: {e}\n")
        import traceback
        traceback.print_exc(file=sys.stderr)
        return EXIT_INTERNAL_FAIL
```

## Section D — Annotation parser + grammar

### D.1 `tools/integrity/integrity/common/annotations.py`

```python
# tools/integrity/integrity/common/annotations.py
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
"""`integrity-allow:` annotation parser per spec § 3.2.

Commit 1 ships the data model and a stub parser. Commit 2 (cat1) wires
the parser into the citation checks and adds the cat1.annotation-form
check.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Annotation:
    file: Path
    line: int                # Line where the annotation appears
    check_id: str            # e.g. "cat1.upstream-anchor" or "cat2.*"
    reason: str
    issue_ref: str           # "#NNN" or "n/a"
    target_line: int         # Line the annotation suppresses (line + 1)


# Annotation grammar per spec § 3.2.
# Captures: check_id, reason, issue_ref.
# Comment-prefix stripping (//, #, <!-- -->) is done by the caller before
# applying this regex.
ANNOTATION_RE = re.compile(
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    r"integrity-allow:\s*(?P<check_id>cat\d+\.[a-z*][a-z0-9.\-*]*)\s*;\s*"
    r"(?P<reason>[^;]{8,}?)\s*;\s*"
    r"(?P<issue_ref>#\d+|n/a)\s*(?:-->)?\s*$"
)


def parse_annotation_line(text: str) -> tuple[str, str, str] | None:
    """Try to parse an annotation from a single line of comment text.

    Returns (check_id, reason, issue_ref) or None if not an annotation
    or grammar is invalid.

    Commit 1: minimal implementation. Commit 2 expands with grammar
    validation reporting (the cat1.annotation-form check).
    """
    m = ANNOTATION_RE.search(text)
    if not m:
        return None
    return (
        m.group("check_id"),
        m.group("reason").strip(),
        m.group("issue_ref"),
    )
```

### D.2 `tools/integrity/integrity/common/suppression.py`

```python
# tools/integrity/integrity/common/suppression.py
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
"""Inline `integrity-allow:` annotation application per spec § 3.2.

After checks produce raw findings, this module marks each finding as
suppressed if the line immediately preceding the cited line carries a
valid annotation covering the finding's check_id (specifically or via a
`cat<N>.*` wildcard).

Spec § 3.2: "an annotation suppresses checks for the immediate next
line or expression." V1 honors this for the immediately-preceding line
only; same-line / multi-line forms are deferred.
"""

from __future__ import annotations

from pathlib import Path

from integrity.common.annotations import parse_annotation_line
from integrity.common.results import Finding


def _matches(annotation_check_id: str, finding_check_id: str) -> bool:
    if annotation_check_id == finding_check_id:
        return True
    if annotation_check_id.endswith(".*"):
        prefix = annotation_check_id[:-2]
        return finding_check_id.startswith(prefix + ".")
    return False


def apply_suppressions(findings: list[Finding], repo_root: Path) -> list[Finding]:
    """Mark each finding as suppressed if a valid annotation precedes
    its cited line. Returns the list with `suppressed`, `suppression_reason`,
    and `suppression_issue` populated where applicable."""

    by_file: dict[str, list[Finding]] = {}
    for f in findings:
        by_file.setdefault(f.file, []).append(f)

    for file_path, file_findings in by_file.items():
        abs_path = repo_root / file_path
        if not abs_path.is_file():
            continue
        try:
            file_lines = abs_path.read_text(encoding="utf-8", errors="replace").splitlines()
        except OSError:
            continue

        for f in file_findings:
            zero_idx = f.line - 1
            if zero_idx <= 0 or zero_idx > len(file_lines):
                continue
            # Walk upward through the contiguous block of annotation lines
            # immediately above the cited line. Multiple annotations stacked
            # above one line (e.g., mixed-category groups produced by the
            # grandfather sweep) all count as "immediately preceding."
            j = zero_idx - 1
            while j >= 0:
                line_text = file_lines[j]
                parsed = parse_annotation_line(line_text)
                if parsed is None:
                    break
                check_id, reason, issue_ref = parsed
                if _matches(check_id, f.check_id):
                    f.suppressed = True
                    f.suppression_reason = reason
                    f.suppression_issue = issue_ref
                    break
                j -= 1

    return findings
```

### D.3 `tools/integrity/integrity/cat1_citations/checks/annotation.py`

```python
# tools/integrity/integrity/cat1_citations/checks/annotation.py
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
"""Check: cat1.annotation-form — every integrity-allow: annotation is grammar-valid.

Mode: HARD_FAIL.

Validates the form per spec § 3.2:
  - check_id matches `cat<N>.<name>` or `cat<N>.*`
  - reason >= 8 chars
  - issue_ref is `#<digits>` or literal `n/a`
  - blanket `*` is rejected
"""

from __future__ import annotations

import re
from pathlib import Path

from integrity.common.exclusions import is_excluded
from integrity.common.repo import list_tracked_files
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat1.annotation-form"
MODE = FailureMode.HARD_FAIL


# Scan extensions for files that can carry annotations.
SCAN_EXTENSIONS: frozenset[str] = frozenset({
    ".cpp", ".hpp", ".h", ".cc", ".cxx", ".c",
    ".glsl", ".wgsl",
    ".ts", ".tsx", ".d.ts",
    ".js", ".mjs", ".cjs", ".jsx",
    ".py", ".pyi",
    ".md",
})


# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
# Loose pattern that finds *any* `integrity-allow:` invocation (valid or
# not) so we can report grammar failures rather than silently skipping.
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
LOOSE_RE = re.compile(r"integrity-allow:\s*(.*?)(?:-->|$)", re.IGNORECASE)


# Strict pattern matching the grammar in spec § 3.2.
STRICT_RE = re.compile(
    r"^\s*(?P<check_id>cat\d+\.(?:[a-z][a-z0-9-]*|\*))\s*;\s*"
    r"(?P<reason>.+?)\s*;\s*"
    r"(?P<issue_ref>#\d+|n/a)\s*$"
)


def _has_scan_extension(path: Path) -> bool:
    name = path.name.lower()
    for ext in SCAN_EXTENSIONS:
        if name.endswith(ext):
            return True
    return False


def _list_scannable_files(root: Path) -> list[Path]:
    """List files to scan. Uses git ls-files if root is a git repo,
    otherwise walks the directory directly (for test fixtures)."""
    if (root / ".git").exists():
        return list_tracked_files(root)
    files: list[Path] = []
    for path in root.rglob("*"):
        if path.is_file():
            files.append(path)
    return files


def _validate(body: str) -> str | None:
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    """Return None if `body` (the text after `integrity-allow:`) is valid,
    else a diagnostic string."""
    body = body.strip().rstrip("-->").strip()

    m = STRICT_RE.match(body)
    if not m:
        return f"grammar mismatch in '{body}'"

    check_id = m.group("check_id")
    reason = m.group("reason").strip()

    if check_id == "*":
        return "blanket '*' check-id is not allowed"
    if len(reason) < 8:
        return f"reason must be >= 8 chars (got {len(reason)}: '{reason}')"

    return None


def run(repo_root: Path) -> list[Finding]:
    """Scan all tracked files; report any malformed annotations."""
    findings: list[Finding] = []

    for absolute in _list_scannable_files(repo_root):
        rel = str(absolute.relative_to(repo_root))
        if is_excluded(rel):
            continue
        if not _has_scan_extension(absolute):
            continue

        try:
            text = absolute.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue

        for lineno, line in enumerate(text.splitlines(), start=1):
            for m in LOOSE_RE.finditer(line):
                body = m.group(1)
                problem = _validate(body)
                if problem is not None:
                    findings.append(Finding(
                        check_id=CHECK_ID,
                        mode=MODE,
                        file=rel,
                        line=lineno,
                        message=problem,
                    ))

    return findings
```

**Where does fenced-block (A.5) awareness live?** In `grandfather.py` (Section
B.1), in `_FENCE_RE`, `is_inside_fenced_block`, `comment_form_for_md_inside_fence`,
and the call in `render_annotation_line`. It is **applied only at annotation-emit
time**, not at suppression-parse time — `suppression.py` and the annotation
parser in `annotations.py` are fence-unaware.

## Section E — Stack-path config

### E.1 `tools/integrity/integrity/common/stack_paths.py`

```python
# tools/integrity/integrity/common/stack_paths.py
"""Canonical paths per stack per spec § 7.2."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class StackPaths:
    name: str
    public_surface_dir: Path
    implementation_dir: Path


def stack_paths(root: Path) -> dict[str, StackPaths]:
    """Return the per-stack public/impl path map rooted at `root`."""
    return {
        "c": StackPaths(
            name="c",
            public_surface_dir=root / "common" / "common-cpp" / "include" / "gpusims",
            implementation_dir=root / "common" / "common-cpp" / "src",
        ),
        "b": StackPaths(
            name="b",
            public_surface_dir=root / "common" / "common-web" / "src",
            implementation_dir=root / "common" / "common-web" / "src",
        ),
        "d": StackPaths(
            name="d",
            public_surface_dir=root / "common" / "common-py" / "gpusims_common",
            implementation_dir=root / "common" / "common-py" / "gpusims_common",
        ),
    }
```

### E.2 `tools/integrity/integrity/common/exclusions.py`

```python
# tools/integrity/integrity/common/exclusions.py
"""Canonical exclusion paths per spec § 3.4.

Adding to this list is a Cat 1 fail unless the change carries
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
an `integrity-allow: cat1.exclusion-list` annotation.
"""

from __future__ import annotations

# Path patterns excluded from analysis by every check unless the check
# explicitly opts in (e.g., cat1.upstream-anchor reads references/.git/HEAD
# deliberately). Patterns are matched as glob-suffix substrings against
# repo-relative paths.
CANONICAL_EXCLUSIONS: tuple[str, ...] = (
    "node_modules/",
    "build/",
    "build-",            # matches build-test-alembic/ and other build-*/
    ".venv/",
    "__pycache__/",
    "references/",
    "_deps/",
    "dist/",
    ".git/",
    ".claude/",
    "captures/",
    "alembic_export/",
    "vdb_export/",
    "gallery/",
    # Synthetic test fixtures per spec § 11: fixtures are deliberately
    # isolated from the real repo. They contain intentionally malformed
    # annotations and synthetic upstream citations against a synthetic
    # registry. Excluding here matches the spec's stated intent.
    "tools/integrity/tests/fixtures/",
)


def is_excluded(path: str) -> bool:
    """Return True if `path` matches any canonical exclusion pattern."""
    normalized = path.replace("\\", "/")
    for pattern in CANONICAL_EXCLUSIONS:
        if pattern in normalized:
            return True
    return False
```

## Section F — Grandfather catalog format

Catalog total length: 236 lines (longer than 120, so first 80 + last 40 below).

### F.1 First 80 lines of `tools/integrity/docs/grandfather-catalog.md`

```markdown
# tools/integrity/docs/grandfather-catalog.md  (lines 1-80)
# Integrity Toolkit — Grandfather Catalog (v1)

This document records the pre-v1 findings that were grandfathered into the
toolkit's strict-mode gate when commit 4a landed. Categories below map to
the rules in `tools/integrity/scripts/grandfather_sweep.py` (and the
classifier in `tools/integrity/integrity/grandfather.py`).

The toolkit will continue to gate CI strictly on any NEW findings introduced
after this commit. Grandfathered findings are suppressed via inline
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
`integrity-allow:` annotations per spec § 3.2.

## Categories

### `audit-citation`

**Pattern:** `cat1.intra-repo` findings in files under `docs/diagnostics/_audits/`.

**Why grandfathered:** Audit reports are snapshots of the codebase at a specific
moment. Citations were valid at audit time; subsequent code drift made some
unresolvable. Audit reports are append-only by convention; retroactively
editing them would erase the historical record.

**Future treatment:** Permanent suppression. New audit reports landing after
v1 may reference paths-that-no-longer-exist; if so, those new citations get
the same suppression at write-time.

### `live-shader-1810`

**Pattern:** `cat1.upstream-citation` findings citing `SPlisHSPlasH 1.8.10`
in live code under `particle-fluids/sph-water/shaders/` or
`particle-fluids/sph-water/src/`.

**Why grandfathered:** The Phase 11.5 setup-1 audit established that the
`1.8.10` anchor was fabricated; the vendored upstream is `2.16.1`. The
live citations in shaders and host code were copied from pre-setup-1 drafts
and still use the old version label. Rewriting them to `2.16.1` is a real
migration item but separate from the integrity toolkit's v1 landing.

**Tracking:** This category has ~10 entries. They are the migration target;
when sph-water's load-bearing-decisions.md and shader headers are next
edited, the citations should be updated to `2.16.1` and the suppressions
removed.

**Future treatment:** Remove suppression on each citation when the
corresponding shader/source file is next modified for unrelated reasons.

### `audit-doc-1810`

**Pattern:** `cat1.upstream-citation` findings citing `SPlisHSPlasH 1.8.10`
in any file NOT under `particle-fluids/sph-water/shaders/` or
`particle-fluids/sph-water/src/`.

**Why grandfathered:** Audit reports and spec docs reference the historical
fabrication intentionally — they document that `1.8.10` was the wrong
anchor. Migrating these citations to `2.16.1` would erase the historical
record of what was wrong.

**Future treatment:** Permanent suppression.

### `spec-grammar-example`

**Pattern:** `cat1.annotation-form` findings in `docs/integrity-toolkit-spec.md`
or files under `tools/integrity/docs/`.

<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
**Why grandfathered:** The spec and toolkit docs include `integrity-allow:`
strings as illustrative grammar examples (in tables, in prose, in code
fences). The `cat1.annotation-form` check parses every such literal as if
it were a real annotation and validates the grammar. Many of these examples
deliberately demonstrate invalid grammar (the "bad" examples) and would
always fail the check.

**Future treatment:** Permanent suppression on these docs.

### `toolkit-own-source`

**Pattern:** `cat1.annotation-form` findings in files under
`tools/integrity/integrity/`.
```

### F.2 Last 40 lines of `tools/integrity/docs/grandfather-catalog.md`

```markdown
# tools/integrity/docs/grandfather-catalog.md  (lines 197-236)
`common/common-web/src/index.ts` with no current consumer in any
Stack B sim, example, or test. Breakdown:

- 33 methods on `Camera`, `Buffer`, `Texture`, `ComputePipeline`,
  `RenderPipeline`, `Renderer` — abstraction-layer surface exposed
  for the seven landed Stack B sims (strange-attractors,
  mandelbulb-explorer, reaction-diffusion-2d, physarum, boids-3d,
  lenia-fft, neural-ca) but not all touched.
- 25 properties on `Camera`, `GpuProfiler`, `ParamPanel`, etc. —
  same pattern.
- 4 type aliases (`Vec2`, `Vec3`, `Vec4`, `Mat4`) — exported as
  convenience types; sims use their own local Vec types or rely on
  the WebGPU API's `Float32Array` directly. v1.1 candidate for
  trimming the surface or wiring as the canonical name.

Stack B uses the TypeScript compiler API (type-aware), so the
name-collision false-positive class that Stack D and Stack C have
does not apply. Detection is precise.

**Future treatment:** Per-symbol review as sims consume the API.
Suppressions should dissolve as sim code wires the abstraction.

## Suppression-annotation discipline

Each suppressed finding has an inline annotation per spec § 3.2:

<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
    integrity-allow: <check-id>; <reason from category>; n/a

Issue-refs are `n/a` for v1 grandfather suppressions; no per-finding GitHub
issues were created. The `live-shader-1810` category is the only one with
intended future cleanup (when those citations are next edited), and it is
tracked at the category level rather than per-annotation.

## Removing a suppression

When the underlying finding is fixed (e.g., a citation is updated, a
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
field gains a consumer), remove the corresponding `integrity-allow:`
annotation. The toolkit's CI check will pass without it.
```

### F.3 Numeric counts in category headings?

**No.** The category headings in `grandfather-catalog.md` are bare
``### `name` `` markers — none carry a numeric count or `[N findings]`
suffix. Verified with `rg -n '^###.*\([0-9]+\)|^###.*\[[0-9]+' tools/integrity/docs/grandfather-catalog.md`,
which returned zero matches. Per-category counts appear only as approximate
prose ("~10 entries", "~108 entries") inside the body of two categories.

## Section G — Stub-label verbatim

### G.1 `common/common-cpp/include/gpusims/alembic_writer.hpp` (lines 1–25)

```cpp
// common/common-cpp/include/gpusims/alembic_writer.hpp  (lines 1-25)
#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

#include <glm/glm.hpp>

namespace gpusims {

// Alembic writer for particle-fluid sims and meshes.
//
// In Phase 1, this is a stub: if GPU_SIMS_HAVE_ALEMBIC is not defined at
// compile time, all functions log a warning on first call and return false.
// Real implementations land when the first Alembic-consuming sim (likely
// SPH water) is built.

namespace abc {

// Particle frame for streaming particle exports.
struct ParticleFrame {
    const float*    positions  = nullptr;  // 3 floats per particle (x, y, z)
    const float*    velocities = nullptr;  // 3 floats per particle (optional)
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
    const float*    radii      = nullptr;  // 1 float per particle (optional)
```

### G.2 `common/common-cpp/include/gpusims/vdb_writer.hpp` (lines 1–25)

```cpp
// common/common-cpp/include/gpusims/vdb_writer.hpp  (lines 1-25)
#pragma once

#include <cstdint>
#include <filesystem>

#include <glm/glm.hpp>

namespace gpusims {

// OpenVDB writer for volumetric grid sims.
//
// In Phase 1, this is a stub: if GPU_SIMS_HAVE_OPENVDB is not defined at
// compile time (i.e., GPU_SIMS_USE_OPENVDB=OFF in CMake), all functions log
// a warning on first call and return false. When OpenVDB is enabled, real
// implementations are provided.
//
// Data convention:
//   - 3D grids are linearized x-fastest, then y, then z.
//   - voxel_size is in world units per cell.
//   - origin is the world-space position of voxel (0,0,0) corner.

namespace vdb {

// Write a single dense float grid to a .vdb file. Returns false on failure.
// integrity-allow: cat2.public-symbol-used-c; pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused); n/a
```

The shared stale phrase across both files is `In Phase <N>, this is a stub:`
where `<N>` is `1` in both current cases. The body continues with a
`GPU_SIMS_HAVE_<X>` guard discussion. New `cat2.stub-label-stale` regex will
need to anchor on this exact phrasing.

## Section H — Cat 2 fixture layout

### H.1 `find ... good_contracts_c bad_contracts_c good_contracts bad_contracts`

```
tools/integrity/tests/fixtures/bad_contracts_c/build/compile_commands.json
tools/integrity/tests/fixtures/bad_contracts_c/include/widget/widget.hpp
tools/integrity/tests/fixtures/bad_contracts/common/common-py/gpusims_common/__init__.py
tools/integrity/tests/fixtures/bad_contracts/common/common-py/gpusims_common/particle.py
tools/integrity/tests/fixtures/bad_contracts/consumer.py
tools/integrity/tests/fixtures/bad_contracts_c/src/consumer.cpp
tools/integrity/tests/fixtures/bad_contracts_c/src/widget.cpp
tools/integrity/tests/fixtures/bad_contracts/.gitkeep
tools/integrity/tests/fixtures/good_contracts_c/build/compile_commands.json
tools/integrity/tests/fixtures/good_contracts_c/include/widget/widget.hpp
tools/integrity/tests/fixtures/good_contracts/common/common-py/gpusims_common/__init__.py
tools/integrity/tests/fixtures/good_contracts/common/common-py/gpusims_common/widget.py
tools/integrity/tests/fixtures/good_contracts/consumer.py
tools/integrity/tests/fixtures/good_contracts_c/src/consumer.cpp
tools/integrity/tests/fixtures/good_contracts_c/src/widget.cpp
tools/integrity/tests/fixtures/good_contracts/.gitkeep
```

### H.2 `find ... good_contracts_b bad_contracts_b`

```
tools/integrity/tests/fixtures/bad_contracts_b/closed-form/example/web/src/main.ts
tools/integrity/tests/fixtures/bad_contracts_b/common/common-web/src/index.ts
tools/integrity/tests/fixtures/bad_contracts_b/common/common-web/src/particle.ts
tools/integrity/tests/fixtures/bad_contracts_b/common/common-web/src/unused.ts
tools/integrity/tests/fixtures/bad_contracts_b/common/common-web/tsconfig.json
tools/integrity/tests/fixtures/good_contracts_b/closed-form/example/web/src/main.ts
tools/integrity/tests/fixtures/good_contracts_b/common/common-web/src/index.ts
tools/integrity/tests/fixtures/good_contracts_b/common/common-web/src/widget.ts
tools/integrity/tests/fixtures/good_contracts_b/common/common-web/tsconfig.json
```

### H.3 Smallest good_contracts_c fixture by LOC (`src/consumer.cpp`, 6 lines)

```cpp
// tools/integrity/tests/fixtures/good_contracts_c/src/consumer.cpp
#include "widget/widget.hpp"

int use_widget() {
    auto w = widget::make_widget(5);
    return w.name_len + w.count;
}
```

### H.4 Smallest bad_contracts_c fixture by LOC (`src/consumer.cpp`, 10 lines)

```cpp
// tools/integrity/tests/fixtures/bad_contracts_c/src/consumer.cpp
#include "widget/widget.hpp"

int use_frame() {
    auto f = widget::make_frame(10);
    int n = 0;
    if (f.positions != nullptr) {
        n += 1;
    }
    return n + f.count;
}
```

## Section I — Existing tests for Cat 2 Stack C

### I.1 `tools/integrity/tests/test_cat2_stack_c.py` (73 lines, full verbatim)

```python
# tools/integrity/tests/test_cat2_stack_c.py
"""Tests for cat2.public-symbol-used-c."""

from __future__ import annotations

from pathlib import Path

import pytest

pytest.importorskip("clang.cindex", reason="libclang not installed")


def _has_libclang_runtime() -> bool:
    """Try to create an Index; libclang.so must be findable."""
    try:
        import clang.cindex
        clang.cindex.Index.create()
        return True
    except Exception:
        return False


pytestmark = pytest.mark.skipif(
    not _has_libclang_runtime(),
    reason="libclang runtime library not found",
)


from integrity.cat2_contracts.checks.public_symbol_used_c import run  # noqa: E402
from integrity.cat2_contracts.stack_c import (  # noqa: E402
    SymbolKind,
    extract_public_surface,
)


def test_extract_public_surface_finds_class_and_function(fixtures_dir: Path) -> None:
    symbols = extract_public_surface(fixtures_dir / "good_contracts_c")
    names = {s.name for s in symbols}
    assert "Widget" in names or any(s.qualified_name.endswith("Widget") for s in symbols)


def test_extract_public_surface_enumerates_fields(fixtures_dir: Path) -> None:
    symbols = extract_public_surface(fixtures_dir / "bad_contracts_c")
    field_names = {s.name for s in symbols if s.kind == SymbolKind.CLASS_FIELD}
    assert "radii_ptr" in field_names
    assert "positions" in field_names


def test_good_contracts_c_yield_no_findings(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "good_contracts_c")
    assert findings == [], f"unexpected: {[(f.file, f.message) for f in findings]}"


def test_bad_contracts_c_flag_unused_radii(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "bad_contracts_c")
    radii = [f for f in findings if "radii_ptr" in f.message]
    assert len(radii) == 1, f"findings: {[f.message for f in findings]}"


def test_bad_contracts_c_flag_unused_function(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "bad_contracts_c")
    unused_fn = [f for f in findings if "unused_function" in f.message]
    assert len(unused_fn) == 1, f"findings: {[f.message for f in findings]}"


def test_used_symbols_not_flagged(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "bad_contracts_c")
    positions = [f for f in findings if "positions" in f.message]
    assert positions == [], f"positions was incorrectly flagged: {positions}"


def test_missing_compile_commands_returns_empty(tmp_path: Path) -> None:
    findings = run(tmp_path)
    assert findings == []
```

## Section J — README / docs `python -m integrity` vs `python3 -m integrity`

### J.1 `rg -n '\bpython -m integrity\b' tools/integrity/ docs/ README.md`

```
tools/integrity/integrity/__main__.py:1:"""Entry point: `python -m integrity`."""
tools/integrity/README.md:18:python -m integrity
tools/integrity/README.md:21:python -m integrity --mode warn-only
tools/integrity/README.md:24:python -m integrity --cat 1
tools/integrity/README.md:25:python -m integrity --check cat1.upstream-anchor
tools/integrity/README.md:28:python -m integrity --output json
tools/integrity/README.md:31:python -m integrity --output github
docs/integrity-toolkit-spec.md:254:    │   ├── __main__.py                    # `python -m integrity` entry
docs/integrity-toolkit-spec.md:331:python -m integrity [--cat <N>] [--check <id>] [--mode <strict|warn-only>] \
docs/integrity-toolkit-spec.md:359:| CI: full run | `python -m integrity --output github` |
docs/integrity-toolkit-spec.md:360:| Local: pre-commit | `python -m integrity --mode warn-only` |
docs/integrity-toolkit-spec.md:361:| Local: debugging one check | `python -m integrity --check cat1.upstream-anchor` |
docs/integrity-toolkit-spec.md:753:        run: python -m integrity --output github
docs/diagnostics/_audits/integrity_build_8_landing_2026-05-14.md:170:- The Cat 3 check is now part of every `python -m integrity`
docs/diagnostics/_audits/integrity_build_7_landing_2026-05-14.md:138:- The Stack B check is now part of every `python -m integrity`
docs/diagnostics/_audits/integrity_build_6_landing_2026-05-14.md:160:- The Cat 2 Stack C check is now part of every `python -m integrity`
docs/diagnostics/_audits/integrity_build_4b_landing_2026-05-14.md:39:- `python -m integrity --mode strict --no-audit-log` exits 0 with
docs/diagnostics/_audits/integrity_build_2_landing_2026-05-14.md:188:- **`python -m integrity` is now failing locally.** Running the
docs/diagnostics/_audits/integrity_build_5_landing_2026-05-14.md:76:`python -m integrity --check cat2.public-symbol-used --output human`
docs/diagnostics/_audits/integrity_build_5_landing_2026-05-14.md:137:- The Cat 2 check is now part of every `python -m integrity` invocation
docs/diagnostics/_audits/integrity_build_4a_landing_2026-05-14.md:7:`python -m integrity --mode strict` exits 0 against the real repo.
docs/diagnostics/_audits/integrity_build_1_landing_2026-05-14.md:20:`python -m integrity`, and tested by a pytest harness. The runner CLI is
docs/diagnostics/_audits/integrity_build_1_landing_2026-05-14.md:151:- **`python -m integrity` is non-failing for now.** The check registry
docs/diagnostics/_audits/integrity_build_1_landing_2026-05-14.md:196:  scaffold's `python -m integrity` documentation in the README is the
```

### J.2 `rg -n '\bpython3 -m integrity\b' tools/integrity/ docs/ README.md`

```
docs/diagnostics/_audits/integrity_build_3_landing_2026-05-14.md:135:`python3 -m integrity --mode strict --no-audit-log` exits 1 (HARD_FAIL).
docs/diagnostics/_audits/integrity_build_3_landing_2026-05-14.md:136:`python3 -m integrity --mode warn-only --no-audit-log` exits 0.
docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md:130:### C.1 `python3 -m integrity --mode strict --output json --no-audit-log` — summary block (FACT)
docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md:280:`time python3 -m integrity --no-audit-log` from repo root:
docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md:397:- Grandfather sweep **not run** (only `python3 -m integrity` and `gh api` used).
docs/diagnostics/_audits/integrity_build_6_landing_2026-05-14.md:93:time python3 -m integrity --check cat2.public-symbol-used-c --output human
docs/diagnostics/_audits/integrity_build_2_landing_2026-05-14.md:110:$ python3 -m integrity --output human --no-audit-log
docs/diagnostics/_audits/integrity_build_2_landing_2026-05-14.md:168:$ python3 -m integrity --mode warn-only --no-audit-log
docs/diagnostics/_audits/integrity_build_1_landing_2026-05-14.md:112:$ python3 -m integrity --output human
docs/diagnostics/_audits/integrity_build_1_landing_2026-05-14.md:116:$ python3 -m integrity --output json
docs/diagnostics/_audits/integrity_build_1_landing_2026-05-14.md:130:$ python3 -m integrity --cat 1
docs/diagnostics/_audits/integrity_build_1_landing_2026-05-14.md:197:  canonical user-facing form, and `python3 -m integrity` works
docs/diagnostics/_audits/integrity_build_8_landing_2026-05-14.md:117:$ time python3 -m integrity --mode strict --no-audit-log
docs/diagnostics/_audits/integrity_build_8_landing_2026-05-14.md:134:$ time python3 -m integrity --check cat3.cubic-kernel --output human --no-audit-log
docs/diagnostics/_audits/integrity_build_8_landing_2026-05-14.md:152:$ python3 -m integrity --check cat3.cubic-kernel --output human --no-audit-log
docs/diagnostics/_audits/integrity_build_7_landing_2026-05-14.md:89:time python3 -m integrity --check cat2.public-symbol-used-ts ...
```

### J.3 Delta summary (drives 5.B docs sweep)

The **non-audit, user-facing surface** still uses `python -m integrity`
exclusively:

- `tools/integrity/integrity/__main__.py:1` — docstring tagline
- `tools/integrity/README.md` lines 18, 21, 24, 25, 28, 31 (six invocations)
- `docs/integrity-toolkit-spec.md` lines 254, 331, 359, 360, 361, 753 (six)
- 7 audit-log entries under `docs/diagnostics/_audits/`

The `python3 -m integrity` form appears **only** inside append-only audit
reports (build-N landing reports, the v1.1 probe). No README, spec, or CI
workflow uses `python3`. Drift to fix for 5.B is the 12-call surface in
`README.md` + `integrity-toolkit-spec.md`, plus the one-line docstring in
`__main__.py`. The audit/diagnostic entries are append-only and should not
be rewritten (per the `audit-citation` grandfather rationale in F.1).

## Constraints check (self-attestation)

- Toolkit code, configuration, grandfather catalog: **not modified**.
- `references/` tree: **not touched**.
- All files dumped verbatim were ≤400 lines (largest: `grandfather.py` at
  344 lines). No truncations applied. The catalog itself is 236 lines and
  was excerpted per the explicit first-80/last-40 instruction in § F.
- All commands ran cleanly; no failures to report.
