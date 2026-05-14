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
    assert "radii" in field_names
    assert "positions" in field_names


def test_good_contracts_c_yield_no_findings(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "good_contracts_c")
    assert findings == [], f"unexpected: {[(f.file, f.message) for f in findings]}"


def test_bad_contracts_c_flag_unused_radii(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "bad_contracts_c")
    radii = [f for f in findings if "radii" in f.message]
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
