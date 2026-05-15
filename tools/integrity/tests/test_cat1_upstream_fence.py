"""Tests for A.5: fence-block awareness in cat1.upstream-citation."""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.checks.upstream import run as upstream_run


def test_upstream_skips_fence_internal(fixtures_dir: Path) -> None:
    """cat1.upstream-citation should NOT fire on version-mismatched
    upstream citations inside fenced code blocks of markdown files."""
    findings = upstream_run(fixtures_dir / "good_citations")
    md_findings = [f for f in findings if f.file.endswith("fenced_upstream.md")]
    assert md_findings == [], (
        f"fence-internal upstream citations should be skipped: "
        f"{[(f.file, f.line, f.message) for f in md_findings]}"
    )
