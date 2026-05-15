"""Tests for A.5: fence-block awareness in cat1.intra-repo."""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.checks.intra_repo import run as intra_repo_run


def test_intra_repo_skips_fence_internal(fixtures_dir: Path) -> None:
    """cat1.intra-repo should NOT fire on dangling-path citations inside
    fenced code blocks of markdown files."""
    findings = intra_repo_run(fixtures_dir / "good_citations")
    md_findings = [f for f in findings if f.file.endswith("fenced_intra_repo.md")]
    assert md_findings == [], (
        f"fence-internal intra-repo citations should be skipped: "
        f"{[(f.file, f.line, f.message) for f in md_findings]}"
    )
