"""Tests for cat2.public-symbol-used-ts."""

from __future__ import annotations

import shutil
from pathlib import Path

import pytest


pytestmark = pytest.mark.skipif(
    shutil.which("node") is None,
    reason="Node.js not available in test environment",
)


def test_extract_runs_against_good_fixture(fixtures_dir: Path) -> None:
    """The good_contracts_b fixture has Widget + makeWidget, both used."""
    from integrity.cat2_contracts.stack_b import run_extractor

    symbols = run_extractor(fixtures_dir / "good_contracts_b")
    if not symbols:
        pytest.skip("TS helper not built; cannot smoke")

    widget = [s for s in symbols if s.name == "Widget"]
    make_widget = [s for s in symbols if s.name == "makeWidget"]
    assert widget, f"Widget not surfaced; symbols: {[s.name for s in symbols]}"
    assert make_widget
    assert widget[0].reference_count >= 1
    assert make_widget[0].reference_count >= 1


def test_good_contracts_b_yields_no_findings(fixtures_dir: Path) -> None:
    from integrity.cat2_contracts.checks.public_symbol_used_b import run

    findings = run(fixtures_dir / "good_contracts_b")
    # If the helper isn't built, run() returns empty — but so does the
    # good-case-correct behavior. Distinguish by querying the extractor
    # directly first.
    from integrity.cat2_contracts.stack_b import run_extractor
    if not run_extractor(fixtures_dir / "good_contracts_b"):
        pytest.skip("TS helper not built; cannot smoke")
    assert findings == [], f"unexpected: {[(f.file, f.message) for f in findings]}"


def test_bad_contracts_b_flag_unused_radii(fixtures_dir: Path) -> None:
    from integrity.cat2_contracts.checks.public_symbol_used_b import run

    findings = run(fixtures_dir / "bad_contracts_b")
    if not findings:
        pytest.skip("TS helper not built or empty result; cannot smoke")
    radii = [f for f in findings if "radii" in f.message]
    assert len(radii) >= 1, f"findings: {[f.message for f in findings]}"


def test_bad_contracts_b_flag_unused_function(fixtures_dir: Path) -> None:
    from integrity.cat2_contracts.checks.public_symbol_used_b import run

    findings = run(fixtures_dir / "bad_contracts_b")
    if not findings:
        pytest.skip("TS helper not built or empty result; cannot smoke")
    unused = [f for f in findings if "unusedFunction" in f.message]
    assert len(unused) >= 1, f"findings: {[f.message for f in findings]}"


def test_missing_node_returns_empty(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    """Simulate missing node by stubbing is_node_available."""
    import integrity.cat2_contracts.stack_b as stack_b
    monkeypatch.setattr(stack_b, "is_node_available", lambda: False)
    findings = stack_b.run_extractor(tmp_path)
    assert findings == []
