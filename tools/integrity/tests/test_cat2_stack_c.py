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


# ---------------------------------------------------------------------------
# T2.3 single-parse refactor perf assertion (opt-in)
# ---------------------------------------------------------------------------


import os  # noqa: E402


@pytest.mark.skipif(
    os.environ.get("INTEGRITY_PERF_ASSERTIONS") != "1",
    reason=(
        "Performance assertion; set INTEGRITY_PERF_ASSERTIONS=1 to enable. "
        "Default-skipped because CI walltime is observed via action logs, "
        "not via pytest, per spec section 3.D."
    ),
)
def test_stack_c_single_parse_walltime_under_ceiling() -> None:
    """Single-parse refactor (T2.3) target: real-repo Stack C scan under
    a conservative ceiling. Pre-refactor double-parse baseline was ~95s
    locally; the refactor halves that. Ceiling set generously to avoid
    flake on slow runners (>2x expected post-refactor walltime)."""
    import time

    repo_root = Path(__file__).resolve().parents[3]
    if not (repo_root / "build" / "compile_commands.json").is_file():
        pytest.skip("compile_commands.json absent; perf assertion needs build/")
    start = time.monotonic()
    run(repo_root)
    walltime = time.monotonic() - start
    assert walltime < 120.0, (
        f"Stack C single-parse run took {walltime:.1f}s; "
        f"expected <120s ceiling (post-refactor target ~50s)"
    )
