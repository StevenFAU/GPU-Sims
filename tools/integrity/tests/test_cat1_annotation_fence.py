"""Tests for A.5: fence-block awareness in cat1.annotation-form parser
and in integrity.common.annotations helpers."""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.checks.annotation import run as annotation_run
from integrity.common.annotations import (
    fence_state_per_line,
    is_inside_fenced_block,
    is_markdown_path,
)


def test_fence_state_per_line_basic() -> None:
    lines = [
        "outside",
        "```python",
        "inside line 1",
        "inside line 2",
        "```",
        "outside again",
    ]
    state = fence_state_per_line(lines)
    assert state == [False, True, True, True, True, False]


def test_fence_state_per_line_empty() -> None:
    assert fence_state_per_line([]) == []


def test_fence_state_handles_unclosed_fence() -> None:
    """An unclosed fence leaves all lines after the opener in-fence."""
    lines = ["outside", "```", "inside", "still inside"]
    state = fence_state_per_line(lines)
    assert state == [False, True, True, True]


def test_is_markdown_path() -> None:
    assert is_markdown_path("docs/foo.md")
    assert is_markdown_path("docs/bar.rst")
    assert not is_markdown_path("src/main.py")
    assert not is_markdown_path("docs/foo.txt")


def test_is_inside_fenced_block_target_inside() -> None:
    lines = ["outside", "```", "inside", "```"]
    in_fence, _ = is_inside_fenced_block(lines, 2)
    assert in_fence is True


def test_is_inside_fenced_block_target_outside() -> None:
    lines = ["outside", "```", "inside", "```", "outside"]
    in_fence, _ = is_inside_fenced_block(lines, 4)
    assert in_fence is False


def test_annotation_check_skips_fence_internal(fixtures_dir: Path) -> None:
    """cat1.annotation-form should NOT fire on malformed annotations
    inside fenced code blocks (only on real live annotations)."""
    findings = annotation_run(fixtures_dir / "good_citations")
    fence_findings = [f for f in findings if "fenced_examples.md" in f.file]
    assert fence_findings == [], (
        f"fence-internal annotations should be skipped: "
        f"{[(f.file, f.message) for f in fence_findings]}"
    )
