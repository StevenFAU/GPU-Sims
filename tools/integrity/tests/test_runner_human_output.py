"""Tests for the human-output renderer in integrity.runner.emit_output.

Pins the P1.6 fix: the default human-format output filters out suppressed
findings, matching the summary line's hard_fail count rather than emitting
HARD_FAIL stanzas for every suppressed finding.
"""

from __future__ import annotations

import io
import sys
from dataclasses import dataclass

from integrity.common.results import FailureMode, Finding
from integrity.runner import RunSummary, emit_output


@dataclass
class _Args:
    """Minimal CliArgs stand-in for emit_output's args parameter."""
    output: str = "human"
    mode: str = "strict"
    root: object = None
    no_audit_log: bool = True


def _capture_emit(summary: RunSummary, findings: list[Finding], args: _Args) -> str:
    buf = io.StringIO()
    saved = sys.stdout
    try:
        sys.stdout = buf
        emit_output(summary, findings, args)
    finally:
        sys.stdout = saved
    return buf.getvalue()


def _make_finding(check_id: str, file: str, line: int, message: str, *,
                  suppressed: bool, mode: FailureMode = FailureMode.HARD_FAIL) -> Finding:
    return Finding(
        check_id=check_id,
        mode=mode,
        file=file,
        line=line,
        message=message,
        suppressed=suppressed,
    )


def test_human_output_omits_suppressed_stanzas() -> None:
    findings = [
        _make_finding("cat1.intra-repo", "CHANGELOG.md", 10, "bare-path", suppressed=True),
        _make_finding("cat1.intra-repo", "docs/phase12.md", 20, "bare-path", suppressed=False),
    ]
    summary = RunSummary(passes=0, soft_warns=0, hard_fails=1, suppressions=1)
    out = _capture_emit(summary, findings, _Args(output="human"))
    # The unsuppressed finding renders.
    assert "docs/phase12.md" in out
    # The suppressed finding does NOT render as a stanza.
    assert "CHANGELOG.md" not in out


def test_human_output_summary_counts_match_stanza_count() -> None:
    """The summary line says N hard-fail; exactly N HARD_FAIL stanzas should appear."""
    findings = [
        _make_finding("cat1.intra-repo", f"a/b{i}.md", 1, "x", suppressed=(i >= 3))
        for i in range(10)
    ]
    summary = RunSummary(passes=0, soft_warns=0, hard_fails=3, suppressions=7)
    out = _capture_emit(summary, findings, _Args(output="human"))

    hard_fail_lines = [line for line in out.splitlines() if "HARD_FAIL" in line]
    # 3 expected: one stanza header per unsuppressed hard-fail.
    assert len(hard_fail_lines) == 3

    # Summary line is present and reports the right counts.
    assert "3 hard-fail" in out
    assert "7 suppressed" in out


def test_github_output_unchanged_still_omits_suppressed() -> None:
    """Regression guard: P1.6 must not break the github branch's existing filter."""
    findings = [
        _make_finding("cat1.intra-repo", "CHANGELOG.md", 10, "x", suppressed=True),
        _make_finding("cat1.intra-repo", "docs/phase12.md", 20, "y", suppressed=False),
    ]
    summary = RunSummary(passes=0, soft_warns=0, hard_fails=1, suppressions=1)
    out = _capture_emit(summary, findings, _Args(output="github"))

    # ::error stanza for the unsuppressed finding only.
    error_lines = [line for line in out.splitlines() if line.startswith("::error")]
    assert len(error_lines) == 1
    assert "docs/phase12.md" in error_lines[0]


def test_human_output_no_suppressed_means_full_render() -> None:
    """When nothing is suppressed, every finding renders a stanza."""
    findings = [
        _make_finding("cat1.intra-repo", f"a/b{i}.md", 1, "x", suppressed=False)
        for i in range(5)
    ]
    summary = RunSummary(passes=0, soft_warns=0, hard_fails=5, suppressions=0)
    out = _capture_emit(summary, findings, _Args(output="human"))

    hard_fail_lines = [line for line in out.splitlines() if "HARD_FAIL" in line]
    assert len(hard_fail_lines) == 5
