"""Audit log writer per spec § 3.3.

Appends to docs/diagnostics/_audits/integrity_failures_<YYYY-MM-DD>.md.
"""

from __future__ import annotations

import datetime as _dt
from pathlib import Path

from integrity.common.results import Finding


_AUDIT_FRONT_MATTER_TEMPLATE = """\
---
date: {date}
author: integrity-toolkit
phase: ci-run
status: failure-record
scope: machine-generated; do not edit by hand
---

"""


def audit_log_path(root: Path, when: _dt.date | None = None) -> Path:
    """Return the audit log path for `when` (default: today UTC)."""
    if when is None:
        when = _dt.datetime.now(_dt.timezone.utc).date()
    date_str = when.isoformat()
    return root / "docs" / "diagnostics" / "_audits" / f"integrity_failures_{date_str}.md"


def append_findings(
    root: Path,
    findings: list[Finding],
    commit_sha: str,
    timestamp: _dt.datetime | None = None,
) -> Path:
    """Append `findings` to today's audit log. Creates the file with
    front-matter if it does not yet exist. Returns the audit log path."""
    if timestamp is None:
        timestamp = _dt.datetime.now(_dt.timezone.utc)

    path = audit_log_path(root, timestamp.date())
    path.parent.mkdir(parents=True, exist_ok=True)

    is_new = not path.exists()
    with path.open("a", encoding="utf-8") as f:
        if is_new:
            f.write(_AUDIT_FRONT_MATTER_TEMPLATE.format(date=timestamp.date().isoformat()))

        f.write(f"## Run {timestamp.isoformat()} — commit {commit_sha}\n\n")
        for finding in findings:
            f.write(f"### {finding.check_id} — {finding.mode.value}\n\n")
            f.write(f"**File:** `{finding.file}:{finding.line}`\n\n")
            f.write(f"**Message:** {finding.message}\n\n")
            if finding.ground_truth_ref:
                f.write(f"**Ground truth:** `{finding.ground_truth_ref}`\n\n")
            if finding.suppressed:
                f.write(
                    f"**Suppressed:** {finding.suppression_reason} "
                    f"(issue {finding.suppression_issue})\n\n"
                )
            f.write("\n")

    return path
