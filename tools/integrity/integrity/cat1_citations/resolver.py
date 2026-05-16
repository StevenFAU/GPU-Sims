"""Intra-repo citation resolution per spec § 6.3.

Resolution order:
  1. Relative to the source file's directory
  2. Relative to the repo root
  3. Unresolved → FAIL
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from integrity.cat1_citations.grammar import IntraRepoCitation


@dataclass(frozen=True)
# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
class ResolutionResult:
    citation: IntraRepoCitation
    resolved_path: Path | None     # None if unresolved
    file_line_count: int | None    # None if unresolved
    in_range: bool                 # True if start/end fall within line count
    reason: str                    # Diagnostic message


def _count_lines(path: Path) -> int:
    """Count newline-terminated lines in `path`. A trailing-no-newline
    final line still counts."""
    try:
        with path.open("rb") as f:
            data = f.read()
        if not data:
            return 0
        count = data.count(b"\n")
        if not data.endswith(b"\n"):
            count += 1
        return count
    except OSError:
        return 0


def resolve(
    citation: IntraRepoCitation,
    repo_root: Path,
) -> ResolutionResult:
    """Try to resolve a citation. Returns resolution metadata."""
    # Try relative to source file's directory
    src_dir = citation.source_file.parent
    candidate = (src_dir / citation.path).resolve()
    if candidate.is_file():
        return _check_range(citation, candidate)

    # Try relative to repo root
    candidate = (repo_root / citation.path).resolve()
    if candidate.is_file():
        return _check_range(citation, candidate)

    return ResolutionResult(
        citation=citation,
        resolved_path=None,
        file_line_count=None,
        in_range=False,
        reason=f"path '{citation.path}' does not resolve under "
               f"{src_dir} or {repo_root}",
    )


def _check_range(citation: IntraRepoCitation, resolved: Path) -> ResolutionResult:
    line_count = _count_lines(resolved)
    end_to_check = citation.end if citation.end is not None else citation.start

    if citation.start < 1:
        return ResolutionResult(
            citation=citation,
            resolved_path=resolved,
            file_line_count=line_count,
            in_range=False,
            reason=f"start line {citation.start} is < 1",
        )
    if end_to_check > line_count:
        return ResolutionResult(
            citation=citation,
            resolved_path=resolved,
            file_line_count=line_count,
            in_range=False,
            reason=f"cited line {end_to_check} exceeds file line count {line_count}",
        )
    if citation.end is not None and citation.end < citation.start:
        return ResolutionResult(
            citation=citation,
            resolved_path=resolved,
            file_line_count=line_count,
            in_range=False,
            reason=f"end line {citation.end} is before start {citation.start}",
        )

    return ResolutionResult(
        citation=citation,
        resolved_path=resolved,
        file_line_count=line_count,
        in_range=True,
        reason="ok",
    )
