"""Check: cat1.unregistered-upstream — every cited upstream is in the registry.

Mode: HARD_FAIL.

# integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
# integrity-allow: cat1.bare-path; deferred-upstream-bare-path citation (Chakazul/LeniaNDK pending vendoring decision per ground-truth-sources.md); n/a
Catches citations like `LeniaNDK.py:329-335` against the unvendored
Chakazul/Lenia upstream. The fix is either (a) vendor and register the
upstream, or (b) rewrite the citation to remove the upstream-version
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
form. Either way, an inline `integrity-allow: cat1.unregistered-upstream`
annotation can be used as an intentional opt-out.
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.grammar import extract_upstream_citations
from integrity.cat1_citations.upstream_anchor import load_registry
from integrity.common.annotations import (
    fence_state_per_line,
    is_markdown_path,
)
from integrity.common.exclusions import is_excluded
from integrity.common.repo import list_tracked_files
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat1.unregistered-upstream"
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
    registered_names = set(registry.keys())
    findings: list[Finding] = []

    # Track each upstream name we've already reported at each file:line to
    # avoid spamming the audit log with N copies of the same name
    seen: set[tuple[str, str, int]] = set()

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
            if citation.upstream in registered_names:
                continue
            key = (citation.upstream, rel, citation.source_line)
            if key in seen:
                continue
            seen.add(key)
            findings.append(Finding(
                check_id=CHECK_ID,
                mode=MODE,
                file=rel,
                line=citation.source_line,
                message=(
                    f"{citation.raw}: upstream '{citation.upstream}' is not "
                    f"in the registry at tools/integrity/docs/ground-truth-sources.md"
                ),
            ))

    return findings
