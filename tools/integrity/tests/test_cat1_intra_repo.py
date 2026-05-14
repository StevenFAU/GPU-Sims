"""Tests for cat1.intra-repo. Uses synthetic fixtures only."""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.checks.intra_repo import run
from integrity.common.results import FailureMode


def _files_dir(fixtures_dir: Path, name: str) -> Path:
    return fixtures_dir / name


def test_good_citations_yield_no_findings(fixtures_dir: Path) -> None:
    findings = run(_files_dir(fixtures_dir, "good_citations"))
    intra_repo_findings = [f for f in findings if f.check_id == "cat1.intra-repo"]
    assert intra_repo_findings == [], f"unexpected findings: {intra_repo_findings}"


def test_dangling_citation_is_flagged(fixtures_dir: Path) -> None:
    findings = run(_files_dir(fixtures_dir, "bad_citations"))
    dangling = [f for f in findings if "nope.cpp" in f.message]
    assert len(dangling) == 1
    assert dangling[0].mode == FailureMode.HARD_FAIL
    assert "does not resolve" in dangling[0].message


def test_out_of_range_line_is_flagged(fixtures_dir: Path) -> None:
    findings = run(_files_dir(fixtures_dir, "bad_citations"))
    oor = [f for f in findings if "9999" in f.message]
    assert len(oor) == 1
    assert "exceeds file line count" in oor[0].message


def test_template_token_is_not_a_citation(tmp_path: Path) -> None:
    """`{{path:line}}` is a placeholder, not a citation."""
    (tmp_path / "doc.md").write_text("see `{{file.cpp:42}}` placeholder\n")
    # Need a fake .git so list_tracked_files works; skip that by calling
    # the grammar layer directly.
    from integrity.cat1_citations.grammar import extract_intra_repo_citations
    citations = extract_intra_repo_citations(
        "see `{{file.cpp:42}}` placeholder",
        tmp_path / "doc.md",
    )
    assert citations == []


def test_time_of_day_is_not_a_citation() -> None:
    """`14:30` lacks a recognized extension; should not parse as a citation."""
    from integrity.cat1_citations.grammar import extract_intra_repo_citations
    citations = extract_intra_repo_citations(
        "meeting at 14:30 sharp",
        Path("/tmp/fake.md"),
    )
    assert citations == []


def test_ipv4_port_is_not_a_citation() -> None:
    from integrity.cat1_citations.grammar import extract_intra_repo_citations
    citations = extract_intra_repo_citations(
        "connect to 192.168.1.1:80 for the test",
        Path("/tmp/fake.md"),
    )
    assert citations == []
