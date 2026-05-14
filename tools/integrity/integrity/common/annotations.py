# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
"""`integrity-allow:` annotation parser per spec § 3.2.

Commit 1 ships the data model and a stub parser. Commit 2 (cat1) wires
the parser into the citation checks and adds the cat1.annotation-form
check.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Annotation:
    file: Path
    line: int                # Line where the annotation appears
    check_id: str            # e.g. "cat1.upstream-anchor" or "cat2.*"
    reason: str
    issue_ref: str           # "#NNN" or "n/a"
    target_line: int         # Line the annotation suppresses (line + 1)


# Annotation grammar per spec § 3.2.
# Captures: check_id, reason, issue_ref.
# Comment-prefix stripping (//, #, <!-- -->) is done by the caller before
# applying this regex.
ANNOTATION_RE = re.compile(
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    r"integrity-allow:\s*(?P<check_id>cat\d+\.[a-z*][a-z0-9.\-*]*)\s*;\s*"
    r"(?P<reason>[^;]{8,}?)\s*;\s*"
    r"(?P<issue_ref>#\d+|n/a)\s*(?:-->)?\s*$"
)


def parse_annotation_line(text: str) -> tuple[str, str, str] | None:
    """Try to parse an annotation from a single line of comment text.

    Returns (check_id, reason, issue_ref) or None if not an annotation
    or grammar is invalid.

    Commit 1: minimal implementation. Commit 2 expands with grammar
    validation reporting (the cat1.annotation-form check).
    """
    m = ANNOTATION_RE.search(text)
    if not m:
        return None
    return (
        m.group("check_id"),
        m.group("reason").strip(),
        m.group("issue_ref"),
    )
