"""Tests for cat1.unregistered-upstream."""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.checks.unregistered_upstream import run


def test_registered_upstream_yields_no_findings(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "good_citations")
    assert findings == [], f"unexpected: {findings}"


def test_unregistered_upstream_is_flagged(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "bad_citations")
    unknown = [f for f in findings if "UnknownProject" in f.message]
    assert len(unknown) == 1
    assert "is not in the registry" in unknown[0].message


def test_unregistered_check_deduplicates_per_file_line(tmp_path: Path) -> None:
    """Two identical citations on different lines yield two findings;
    duplicate citations on the same line yield one."""
    (tmp_path / "doc.md").write_text(
# integrity-allow: cat1.unregistered-upstream; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
        "UnknownProject 1.0.0 a.cpp:1 and UnknownProject 1.0.0 b.cpp:2 same line\n"
# integrity-allow: cat1.unregistered-upstream; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
        "UnknownProject 1.0.0 c.cpp:3 second line\n"
    )
    findings = run(tmp_path)
    # Same upstream name on line 1 dedup'd to 1, plus line 2 = 2 findings
    unknown = [f for f in findings if f.check_id == "cat1.unregistered-upstream"]
    assert len(unknown) == 2
