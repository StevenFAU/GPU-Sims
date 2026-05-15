# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
"""Inline `integrity-allow:` annotation application per spec § 3.2.

After checks produce raw findings, this module marks each finding as
suppressed if the line immediately preceding the cited line carries a
valid annotation covering the finding's check_id (specifically or via a
`cat<N>.*` wildcard).

Spec § 3.2: "an annotation suppresses checks for the immediate next
line or expression." V1 honors this for the immediately-preceding line
only; same-line / multi-line forms are deferred.
"""

from __future__ import annotations

from pathlib import Path

from integrity.common.annotations import (
    fence_state_per_line,
    is_markdown_path,
    parse_annotation_line,
)
from integrity.common.results import Finding


def _matches(annotation_check_id: str, finding_check_id: str) -> bool:
    if annotation_check_id == finding_check_id:
        return True
    if annotation_check_id.endswith(".*"):
        prefix = annotation_check_id[:-2]
        return finding_check_id.startswith(prefix + ".")
    return False


def apply_suppressions(findings: list[Finding], repo_root: Path) -> list[Finding]:
    """Mark each finding as suppressed if a valid annotation precedes
    its cited line. Returns the list with `suppressed`, `suppression_reason`,
    and `suppression_issue` populated where applicable."""

    by_file: dict[str, list[Finding]] = {}
    for f in findings:
        by_file.setdefault(f.file, []).append(f)

    for file_path, file_findings in by_file.items():
        abs_path = repo_root / file_path
        if not abs_path.is_file():
            continue
        try:
            file_lines = abs_path.read_text(encoding="utf-8", errors="replace").splitlines()
        except OSError:
            continue

        is_md = is_markdown_path(file_path)
        fence_state = fence_state_per_line(file_lines) if is_md else None

        for f in file_findings:
            zero_idx = f.line - 1
            if zero_idx <= 0 or zero_idx > len(file_lines):
                continue
            # If the finding's own line is inside a fenced block, it is
            # itself a documentation example; no suppression is meaningful.
            if (
                is_md
                and fence_state is not None
                and zero_idx < len(fence_state)
                and fence_state[zero_idx]
            ):
                continue
            # Walk upward through the contiguous block of annotation lines
            # immediately above the cited line. Skip fence-internal lines --
            # an annotation inside a fenced example is not a live annotation.
            j = zero_idx - 1
            while j >= 0:
                if is_md and fence_state is not None and fence_state[j]:
                    break
                line_text = file_lines[j]
                parsed = parse_annotation_line(line_text)
                if parsed is None:
                    break
                check_id, reason, issue_ref = parsed
                if _matches(check_id, f.check_id):
                    f.suppressed = True
                    f.suppression_reason = reason
                    f.suppression_issue = issue_ref
                    break
                j -= 1

    return findings
