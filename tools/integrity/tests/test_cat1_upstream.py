"""Tests for cat1.upstream-citation."""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.checks.upstream import run
from integrity.common.results import FailureMode


def test_good_upstream_citations_yield_no_findings(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "good_citations")
    upstream_findings = [f for f in findings if f.check_id == "cat1.upstream-citation"]
    assert upstream_findings == [], f"unexpected: {upstream_findings}"


def test_wrong_version_is_flagged(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "bad_citations")
    wrong = [f for f in findings if "9.9.9" in f.message]
    assert len(wrong) == 1
    assert wrong[0].mode == FailureMode.HARD_FAIL
    assert "does not match registered anchor" in wrong[0].message


def test_dangling_upstream_path_is_flagged(fixtures_dir: Path) -> None:
    findings = run(fixtures_dir / "bad_citations")
    dangling = [f for f in findings if "nonexistent.cpp" in f.message]
    assert len(dangling) == 1
    assert "does not resolve under" in dangling[0].message


def test_unregistered_upstream_is_not_flagged_by_upstream_check(fixtures_dir: Path) -> None:
    """The upstream-citation check skips unregistered upstreams; that's
    cat1.unregistered-upstream's job."""
    findings = run(fixtures_dir / "bad_citations")
    unknown = [f for f in findings if "UnknownProject" in f.message]
    assert unknown == [], f"upstream-citation should not flag unknown upstreams: {unknown}"
