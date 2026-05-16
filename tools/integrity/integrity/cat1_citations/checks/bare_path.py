"""Check: cat1.bare-path - bare-basename citations are not allowed.

Mode: HARD_FAIL.

A "bare-path citation" is a citation of the form `<basename>:<line>` or
`<basename>:<start>-<end>` where <basename> has no directory prefix
(no `/` in the path component). Bare paths are structurally ambiguous:
a basename like `main.cpp` matches dozens of files in this repo and
several files in the references/ tree.

This check classifies each bare-path citation into one of four arms:

  - REGISTERED-UPSTREAM: basename matches a file in references/.
    HARD_FAIL with a suggested rewrite to the registered upstream form
    (`<UpstreamName> <anchor_version> <full-vendor-path>:<line>`).

  - INTRA-REPO: basename matches exactly one git-tracked intra-repo
    file (outside references/). HARD_FAIL with a suggested rewrite to
    the full intra-repo path.

  - AMBIGUOUS: basename matches multiple intra-repo files. HARD_FAIL
    with a disambiguation candidate list (alphabetical, capped at 5,
    with truncation marker for remaining).

  - UNRESOLVABLE: basename matches no tracked file. HARD_FAIL with
    a simple message. Regex-noise pre-filter excludes obvious false
    positives (basenames containing newline characters, empty
    basenames, line-zero citations).

Index scope: `list_tracked_files(repo_root)` (NOT rglob). This matches
the existing cat1.intra-repo discipline and excludes build artifacts,
node_modules, .venv, etc. Do not widen this scope without changing
the AMBIGUOUS-count estimate in the v1.2 spec.

Coordination with cat1.intra-repo: cat1.intra-repo skip-guards bare
paths (`if "/" not in citation.path: continue`); bare paths flow
exclusively to this check.
"""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path

from integrity.cat1_citations.grammar import (
    IntraRepoCitation,
    extract_intra_repo_citations,
    extract_upstream_citations,
)
from integrity.cat1_citations.resolver import _count_lines
from integrity.cat1_citations.upstream_anchor import (
    UpstreamRegistration,
    load_registry,
)
from integrity.common.annotations import (
    fence_state_per_line,
    is_markdown_path,
)
from integrity.common.exclusions import is_excluded
from integrity.common.repo import list_tracked_files
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat1.bare-path"
MODE = FailureMode.HARD_FAIL


SCAN_EXTENSIONS: frozenset[str] = frozenset({
    ".cpp", ".hpp", ".h", ".cc", ".cxx", ".c",
    ".glsl", ".wgsl",
    ".ts", ".tsx", ".d.ts",
    ".js", ".mjs", ".cjs", ".jsx",
    ".py", ".pyi",
    ".md",
})


MAX_DISAMBIGUATION_CANDIDATES = 5


class BarePathClass(Enum):
    REGISTERED_UPSTREAM = "registered-upstream"
    INTRA_REPO = "intra-repo"
    AMBIGUOUS = "ambiguous"
    UNRESOLVABLE = "unresolvable"


@dataclass(frozen=True)
# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
class BarePathResolution:
    """Result of classifying a bare-path citation."""
    class_: BarePathClass
    matches: list[Path] = field(default_factory=list)


def _has_scan_extension(path: Path) -> bool:
    name = path.name.lower()
    for ext in SCAN_EXTENSIONS:
        if name.endswith(ext):
            return True
    return False


def _passes_sanity_check(basename: str, line: int) -> bool:
    """Filter regex noise per Decision 6.

    Returns True iff:
      - basename starts with letter or underscore
      - basename contains no newline/carriage-return characters
      - line is a positive integer
    """
    if not basename:
        return False
    if line < 1:
        return False
    if basename[0] != "_" and not basename[0].isalpha():
        return False
    if "\n" in basename or "\r" in basename:
        return False
    return True


def _build_basename_indices(
    repo_root: Path,
) -> tuple[dict[str, list[Path]], dict[str, list[Path]]]:
    """Build basename -> list of paths for upstream and intra-repo trees.

    Index scope is `list_tracked_files` (Decision 7). Returns
    (upstream_idx, intra_idx) where each maps basename to a list of
    absolute Paths.
    """
    upstream_idx: dict[str, list[Path]] = defaultdict(list)
    intra_idx: dict[str, list[Path]] = defaultdict(list)

    if (repo_root / ".git").exists():
        all_files = list_tracked_files(repo_root)
    else:
        all_files = [p for p in repo_root.rglob("*") if p.is_file()]

    references_prefix = "references/"
    for absolute in all_files:
        try:
            rel = str(absolute.relative_to(repo_root)).replace("\\", "/")
        except ValueError:
            continue
        if rel.startswith(references_prefix):
            upstream_idx[absolute.name].append(absolute)
        else:
            intra_idx[absolute.name].append(absolute)

    for idx in (upstream_idx, intra_idx):
        for k in idx:
            idx[k].sort()

    return upstream_idx, intra_idx


def _resolve_upstream_for_path(
    abs_path: Path,
    repo_root: Path,
    registry: dict[str, UpstreamRegistration],
) -> tuple[str, str, str] | None:
    """Map a vendored absolute path back to (upstream_name,
    anchor_version, vendor-relative path). Returns None if not in any
    registered vendor_root.
    """
    try:
        rel = str(abs_path.relative_to(repo_root)).replace("\\", "/")
    except ValueError:
        return None
    for upstream_name, reg in registry.items():
        vendor_root = str(reg.vendor_root).replace("\\", "/").rstrip("/")
        if rel.startswith(vendor_root + "/"):
            vendor_relative = rel[len(vendor_root) + 1:]
            return (upstream_name, reg.anchor_version, vendor_relative)
    return None


def _classify_bare_path(
    citation: IntraRepoCitation,
    upstream_idx: dict[str, list[Path]],
    intra_idx: dict[str, list[Path]],
) -> BarePathResolution:
    """Classify a bare-basename citation into one of the four arms."""
    basename = citation.path

    upstream_matches = upstream_idx.get(basename, [])
    intra_matches = intra_idx.get(basename, [])

    if upstream_matches and not intra_matches:
        return BarePathResolution(
            class_=BarePathClass.REGISTERED_UPSTREAM,
            matches=list(upstream_matches[:1]),
        )

    if len(intra_matches) == 1 and not upstream_matches:
        return BarePathResolution(
            class_=BarePathClass.INTRA_REPO,
            matches=list(intra_matches),
        )

    all_matches = list(intra_matches) + list(upstream_matches)
    if len(all_matches) >= 2:
        return BarePathResolution(
            class_=BarePathClass.AMBIGUOUS,
            matches=all_matches,
        )

    return BarePathResolution(
        class_=BarePathClass.UNRESOLVABLE,
        matches=[],
    )


def _format_message(
    citation: IntraRepoCitation,
    resolution: BarePathResolution,
    repo_root: Path,
    registry: dict[str, UpstreamRegistration],
) -> str:
    """Build the HARD_FAIL message per Decisions 3-6."""
    cls = resolution.class_

    if cls == BarePathClass.REGISTERED_UPSTREAM:
        abs_match = resolution.matches[0]
        info = _resolve_upstream_for_path(abs_match, repo_root, registry)
        if info is None:
            rel = str(abs_match.relative_to(repo_root)).replace("\\", "/")
            return (
                f"{citation.raw}: bare upstream citation; matches "
                f"{rel} but the file is not under any registered vendor_root"
            )
        upstream_name, anchor_version, vendor_relative = info
        line_range = (
            f"{citation.start}-{citation.end}" if citation.end else str(citation.start)
        )
        return (
            f"{citation.raw}: bare upstream citation; suggested rewrite: "
            f"'{upstream_name} {anchor_version} {vendor_relative}:{line_range}'"
        )

    if cls == BarePathClass.INTRA_REPO:
        abs_match = resolution.matches[0]
        rel = str(abs_match.relative_to(repo_root)).replace("\\", "/")
        line_count = _count_lines(abs_match)
        end_to_check = citation.end if citation.end is not None else citation.start
        line_range = (
            f"{citation.start}-{citation.end}" if citation.end else str(citation.start)
        )
        if citation.start < 1 or end_to_check > line_count:
            return (
                f"{citation.raw}: bare intra-repo citation; resolved basename "
                f"to {rel}, but line {end_to_check} exceeds file line count "
                f"{line_count}"
            )
        return (
            f"{citation.raw}: bare intra-repo citation; resolved basename "
            f"suggests '{rel}:{line_range}'"
        )

    if cls == BarePathClass.AMBIGUOUS:
        total = len(resolution.matches)
        truncated = total > MAX_DISAMBIGUATION_CANDIDATES
        shown = resolution.matches[:MAX_DISAMBIGUATION_CANDIDATES]
        rels = [str(p.relative_to(repo_root)).replace("\\", "/") for p in shown]
        candidates_str = ", ".join(f"'{r}'" for r in rels)
        count_str = f">={total}" if truncated else str(total)
        suffix = (
            f", ... ({total - MAX_DISAMBIGUATION_CANDIDATES} more)"
            if truncated else ""
        )
        return (
            f"{citation.raw}: bare basename matches {count_str} files; "
            f"candidates: [{candidates_str}{suffix}]"
        )

    return f"{citation.raw}: bare basename matches no git-tracked file"


def _list_scannable_files(root: Path) -> list[Path]:
    """List files to scan for citations. Mirrors intra_repo.py."""
    if (root / ".git").exists():
        return list_tracked_files(root)
    files: list[Path] = []
    for path in root.rglob("*"):
        if path.is_file():
            files.append(path)
    return files


def run(repo_root: Path) -> list[Finding]:
    """Scan all tracked files; return findings for bare-path citations."""
    findings: list[Finding] = []
    registry = load_registry(repo_root)
    upstream_idx, intra_idx = _build_basename_indices(repo_root)

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

        upstream_tails: set[tuple[int, str, int, int | None]] = {
            (uc.source_line, uc.path, uc.start, uc.end)
            for uc in extract_upstream_citations(text, absolute)
        }

        for citation in extract_intra_repo_citations(text, absolute):
            if (
                fence_state is not None
                and 0 < citation.source_line <= len(fence_state)
                and fence_state[citation.source_line - 1]
            ):
                continue
            if "/" in citation.path:
                continue
            if not _passes_sanity_check(citation.path, citation.start):
                continue
            if (citation.source_line, citation.path, citation.start, citation.end) in upstream_tails:
                continue

            resolution = _classify_bare_path(citation, upstream_idx, intra_idx)
            message = _format_message(citation, resolution, repo_root, registry)

            findings.append(Finding(
                check_id=CHECK_ID,
                mode=MODE,
                file=rel,
                line=citation.source_line,
                message=message,
                ground_truth_ref=None,
            ))

    return findings
