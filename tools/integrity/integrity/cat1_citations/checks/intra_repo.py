"""Check: cat1.intra-repo — every intra-repo citation resolves.

Mode: HARD_FAIL.

False positives are defended by the grammar's extension filter and the
template-token mask. False positives that still escape are suppressible
via `integrity-allow: cat1.intra-repo; <reason>; <issue-ref>`.
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.grammar import (
    extract_intra_repo_citations,
    extract_upstream_citations,
)
from integrity.cat1_citations.resolver import resolve
from integrity.common.exclusions import is_excluded
from integrity.common.repo import list_tracked_files
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat1.intra-repo"
MODE = FailureMode.HARD_FAIL


# File extensions whose contents we scan for citations.
SCAN_EXTENSIONS: frozenset[str] = frozenset({
    ".cpp", ".hpp", ".h", ".cc", ".cxx", ".c",
    ".glsl", ".wgsl",
    ".ts", ".tsx", ".d.ts",
    ".js", ".mjs", ".cjs", ".jsx",
    ".py", ".pyi",
    ".md",
})


def _has_scan_extension(path: Path) -> bool:
    name = path.name.lower()
    for ext in SCAN_EXTENSIONS:
        if name.endswith(ext):
            return True
    return False


def _list_scannable_files(root: Path) -> list[Path]:
    """List files to scan. Uses git ls-files if root is a git repo,
    otherwise walks the directory directly (for test fixtures)."""
    if (root / ".git").exists():
        return list_tracked_files(root)
    files: list[Path] = []
    for path in root.rglob("*"):
        if path.is_file():
            files.append(path)
    return files


def _is_under_references(path: str) -> bool:
    """True if the path begins with `references/` (or starts with a
    component that is the name of a vendored upstream)."""
    return path.startswith("references/")


def run(repo_root: Path) -> list[Finding]:
    """Scan all tracked files; return findings for unresolved citations."""
    findings: list[Finding] = []

    for absolute in _list_scannable_files(repo_root):
        rel = str(absolute.relative_to(repo_root))
        if is_excluded(rel):
            continue
        if not _has_scan_extension(absolute):
            continue

        try:
            text = absolute.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue

        # Spans of upstream citations on this file. Any intra-repo
        # match whose (line, path, start, end) coincides with the tail
        # of an upstream citation belongs to cat1.upstream-citation,
        # not intra-repo.
        upstream_tails: set[tuple[int, str, int, int | None]] = {
            (uc.source_line, uc.path, uc.start, uc.end)
            for uc in extract_upstream_citations(text, absolute)
        }

        for citation in extract_intra_repo_citations(text, absolute):
            if _is_under_references(citation.path):
                # Belongs to cat1.upstream-citation, not intra-repo.
                continue
            if (citation.source_line, citation.path, citation.start, citation.end) in upstream_tails:
                # Tail of an upstream citation; cat1.upstream-citation handles.
                continue
            result = resolve(citation, repo_root)
            if result.resolved_path is None or not result.in_range:
                findings.append(Finding(
                    check_id=CHECK_ID,
                    mode=MODE,
                    file=str(absolute.relative_to(repo_root)),
                    line=citation.source_line,
                    message=f"{citation.raw}: {result.reason}",
                    ground_truth_ref=None,
                ))

    return findings
