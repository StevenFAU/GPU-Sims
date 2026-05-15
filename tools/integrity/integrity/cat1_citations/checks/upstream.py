"""Check: cat1.upstream-citation — every upstream citation resolves.

Mode: HARD_FAIL.

Resolution rules per spec § 6.3 (upstream half):
  1. Map <upstream> to a vendor root via the registry
  2. If <upstream> not in registry, this check skips (cat1.unregistered-upstream handles)
  3. If <version> doesn't match anchor_version and isn't 'HEAD', HARD_FAIL
  4. Resolve <path> under vendor_root
  5. Check line range against file line count
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.grammar import extract_upstream_citations
from integrity.cat1_citations.resolver import _count_lines
from integrity.cat1_citations.upstream_anchor import load_registry
from integrity.common.annotations import (
    fence_state_per_line,
    is_markdown_path,
)
from integrity.common.exclusions import is_excluded
from integrity.common.repo import list_tracked_files
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat1.upstream-citation"
MODE = FailureMode.HARD_FAIL


SCAN_EXTENSIONS = frozenset({
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
    if (root / ".git").exists():
        return list_tracked_files(root)
    return [p for p in root.rglob("*") if p.is_file()]


def run(repo_root: Path) -> list[Finding]:
    registry = load_registry(repo_root)
    findings: list[Finding] = []

    for absolute in _list_scannable_files(repo_root):
        try:
            rel = str(absolute.relative_to(repo_root))
        except ValueError:
            continue
        if is_excluded(rel):
            continue
        if not _has_scan_extension(absolute):
            continue

        try:
            text = absolute.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue

        if is_markdown_path(rel):
            fence_state = fence_state_per_line(text.splitlines())
        else:
            fence_state = None

        for citation in extract_upstream_citations(text, absolute):
            if (
                fence_state is not None
                and 0 < citation.source_line <= len(fence_state)
                and fence_state[citation.source_line - 1]
            ):
                continue
            reg = registry.get(citation.upstream)
            if reg is None:
                # cat1.unregistered-upstream handles this case
                continue

            # Version check: must match anchor_version exactly, or be HEAD
            normalized_version = citation.version.lstrip("v")
            if normalized_version != reg.anchor_version and citation.version != "HEAD":
                findings.append(Finding(
                    check_id=CHECK_ID,
                    mode=MODE,
                    file=rel,
                    line=citation.source_line,
                    message=(
                        f"{citation.raw}: version '{citation.version}' does not "
                        f"match registered anchor '{reg.anchor_version}' for "
                        f"{reg.name}"
                    ),
                ))
                continue

            # Resolve path under vendor_root
            candidate = (repo_root / reg.vendor_root / citation.path).resolve()
            if not candidate.is_file():
                findings.append(Finding(
                    check_id=CHECK_ID,
                    mode=MODE,
                    file=rel,
                    line=citation.source_line,
                    message=(
                        f"{citation.raw}: path '{citation.path}' does not "
                        f"resolve under {reg.vendor_root}"
                    ),
                ))
                continue

            # Line range check
            line_count = _count_lines(candidate)
            end_to_check = citation.end if citation.end is not None else citation.start
            if citation.start < 1 or end_to_check > line_count:
                findings.append(Finding(
                    check_id=CHECK_ID,
                    mode=MODE,
                    file=rel,
                    line=citation.source_line,
                    message=(
                        f"{citation.raw}: line {end_to_check} exceeds "
                        f"{reg.vendor_root}/{citation.path} line count {line_count}"
                    ),
                ))

    return findings
