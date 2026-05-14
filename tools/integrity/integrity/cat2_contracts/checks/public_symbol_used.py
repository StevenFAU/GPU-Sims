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
