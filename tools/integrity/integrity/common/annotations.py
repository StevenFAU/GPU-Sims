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


# === Fenced-block awareness (v1.1 A.5) ===
#
# Markdown documents include literal annotation grammar strings as
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
# grammar examples in fenced code blocks (the `integrity-allow:` token).
# Those examples are not real annotations and must not be parsed as such
# by either the annotation-form check or the suppression matcher.

_FENCE_RE = re.compile(r"^\s*```(?P<lang>[A-Za-z0-9_+\-]*)\s*$")


def is_inside_fenced_block(
    lines: list[str],
    target_line_zero_indexed: int,
) -> tuple[bool, str | None]:
    """Determine whether `lines[target_line_zero_indexed]` is inside a
    fenced markdown code block. The opening-fence line itself is
    considered in-fence (we toggle on at start of match)."""
    in_fence = False
    fence_lang: str | None = None
    for i, line in enumerate(lines):
        m = _FENCE_RE.match(line)
        if m:
            if in_fence:
                if i == target_line_zero_indexed:
                    return (True, fence_lang)
                in_fence = False
                fence_lang = None
            else:
                in_fence = True
                fence_lang = m.group("lang") or ""
                if i == target_line_zero_indexed:
                    return (True, fence_lang)
                continue
        if i == target_line_zero_indexed:
            return (in_fence, fence_lang)
    return (False, None)


def fence_state_per_line(lines: list[str]) -> list[bool]:
    """Single-pass version of `is_inside_fenced_block` returning a list
    where `result[i]` is True iff `lines[i]` is inside a fence (or is the
    opening/closing fence marker itself)."""
    state = [False] * len(lines)
    in_fence = False
    for i, line in enumerate(lines):
        if _FENCE_RE.match(line):
            if in_fence:
                state[i] = True
                in_fence = False
            else:
                in_fence = True
                state[i] = True
            continue
        state[i] = in_fence
    return state


def is_markdown_path(file_path: str | Path) -> bool:
    """Predicate used by parser/suppressor to gate fence awareness."""
    name = str(file_path).lower()
    return name.endswith(".md") or name.endswith(".rst")
