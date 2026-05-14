"""Tests for cat2.public-symbol-used Stack D variant."""

from __future__ import annotations

from pathlib import Path

from integrity.cat2_contracts.checks.public_symbol_used import run
from integrity.cat2_contracts.stack_d import (
    SymbolKind,
    extract_public_surface,
)
from integrity.common.results import FailureMode


def test_extract_public_surface_finds_class_and_function(fixtures_dir: Path) -> None:
    symbols = extract_public_surface(fixtures_dir / "good_contracts")
    names = {s.name for s in symbols}
    assert "Widget" in names
    assert "make_widget" in names


def test_extract_public_surface_enumerates_class_fields(fixtures_dir: Path) -> None:
    symbols = extract_public_surface(fixtures_dir / "good_contracts")
    widget_fields = [
        s for s in symbols
        if s.parent_class == "Widget" and s.kind == SymbolKind.CLASS_FIELD
    ]
    field_names = {s.name for s in widget_fields}
    assert "name" in field_names
    assert "count" in field_names


def test_good_contracts_yield_no_findings(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "good_contracts")
    assert findings == [], f"unexpected: {[(f.file, f.line, f.message) for f in findings]}"


def test_bad_contracts_flag_unused_radii(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "bad_contracts")
    radii_findings = [f for f in findings if "radii" in f.message]
    assert len(radii_findings) == 1
    assert radii_findings[0].mode == FailureMode.HARD_FAIL
    assert "ParticleFrame.radii" in radii_findings[0].message


def test_bad_contracts_pass_read_fields(fixtures_dir: Path) -> None:
    """Fields that ARE read (positions, velocities, ids) should not be flagged."""
    findings = run(fixtures_dir / "bad_contracts")
    for read_name in ("positions", "velocities", "ids"):
        matches = [f for f in findings if f"ParticleFrame.{read_name}" in f.message]
        assert matches == [], f"{read_name} was incorrectly flagged: {matches}"


def test_self_references_dont_count_for_field_usage(tmp_path: Path) -> None:
    """A field read only via `self.X` inside its own class shouldn't count
    as a consumer."""
    pkg_dir = tmp_path / "common" / "common-py" / "gpusims_common"
    pkg_dir.mkdir(parents=True)
    (pkg_dir / "__init__.py").write_text("from .thing import Thing\n")
    (pkg_dir / "thing.py").write_text(
        "class Thing:\n"
        "    def __init__(self) -> None:\n"
        "        self.value = 0\n"
        "    def get(self) -> int:\n"
        "        return self.value\n"
    )

    findings = run(tmp_path)
    value_findings = [f for f in findings if "Thing.value" in f.message]
    assert len(value_findings) == 1, (
        f"expected Thing.value to be flagged as unused; "
        f"findings: {[(f.file, f.message) for f in findings]}"
    )
