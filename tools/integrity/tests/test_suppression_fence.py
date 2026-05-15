"""Tests for A.5: fence-block awareness in integrity.common.suppression.

# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
A fence-internal `integrity-allow:` line must NOT suppress a real finding
on the line immediately following the fence."""

from __future__ import annotations

from pathlib import Path

from integrity.common.results import FailureMode, Finding
from integrity.common.suppression import apply_suppressions


def test_fence_internal_annotation_does_not_suppress(tmp_path: Path) -> None:
    """A fence-internal annotation should NOT suppress a finding on the
    line following the fence."""
    md = tmp_path / "example.md"
    md.write_text(
        "\n".join([
            "# Heading",
            "",
            "```cpp",  # integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
            "// integrity-allow: cat1.intra-repo; documentation only; n/a",
            "```",
            "real_broken_citation:42",  # line 6
            "",
        ]),
        encoding="utf-8",
    )

    findings = [
        Finding(
            check_id="cat1.intra-repo",
            mode=FailureMode.HARD_FAIL,
            file="example.md",
            line=6,
            message="dangling citation",
        )
    ]

    result = apply_suppressions(findings, tmp_path)
    assert len(result) == 1
    assert result[0].suppressed is False, (
        "fence-internal annotation should not suppress findings outside the fence"
    )


def test_live_annotation_above_fence_suppresses(tmp_path: Path) -> None:
    """A live annotation OUTSIDE the fence should suppress as before."""
    md = tmp_path / "example.md"
    md.write_text(
        "\n".join([
            "# Heading",
            "",
            "<!-- integrity-allow: cat1.intra-repo; real annotation here; n/a -->",
            "real_broken_citation:42",  # line 4
            "",
        ]),
        encoding="utf-8",
    )

    findings = [
        Finding(
            check_id="cat1.intra-repo",
            mode=FailureMode.HARD_FAIL,
            file="example.md",
            line=4,
            message="dangling citation",
        )
    ]

    result = apply_suppressions(findings, tmp_path)
    assert len(result) == 1
    assert result[0].suppressed is True
