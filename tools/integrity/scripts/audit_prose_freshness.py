#!/usr/bin/env python3
"""Audit-prose freshness check (T2.2 sibling tool).

Verifies that backtick-fenced ``path:line[-range]`` citations in spec,
retro, and audit prose resolve against the actual repo on disk.
Designed to be run pre-commit by a spec / audit author. NOT integrated
into the main integrity gate (per Decision D3, v1.3 closeout spec
section 0.3): cat1.intra-repo already covers the same surface and is
grandfathered for these paths by design; the value this tool adds is
timing (drafter runs explicitly before committing) and scope (just the
citations the drafter is asserting).

Use:
    python3 tools/integrity/scripts/audit_prose_freshness.py [PATH...]

With no positional args, scans the conventional set:
    docs/integrity-toolkit-spec.md
    docs/retro/*.md
    docs/diagnostics/_audits/*.md
    project-state.md

Exit codes:
    0 -- all citations resolve
    1 -- at least one citation failed to resolve
    2 -- bad CLI args (argparse default)
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from integrity.common.repo import find_repo_root


# Path-side accepted extensions for single-segment citations. Keeps the
# regex from matching IP:port shapes like `192.168.1.1:80` (per probe
# section G.3). A path is acceptable EITHER if it contains a `/` OR if
# it ends in one of these known source extensions.
KNOWN_EXTENSIONS = (
    "py", "ts", "tsx", "js", "jsx", "cpp", "hpp", "h", "c", "cc",
    "comp", "glsl", "frag", "vert", "wgsl", "yml", "yaml", "toml",
    "md", "rst", "txt", "json", "cmake",
)

CITATION_RE = re.compile(
    r"`(?P<path>"
    # Path-with-slash form (always accept): foo/bar.x or foo/bar
    r"[A-Za-z0-9_./\-]*/[A-Za-z0-9_./\-]+"
    r"|"
    # Single-segment form: must end in a known extension
    r"[A-Za-z0-9_\-]+\.(?:" + "|".join(KNOWN_EXTENSIONS) + r")"
    r")"
    r":(?P<start>\d+)(?:-(?P<end>\d+))?`"
)

# Belt-and-suspenders filter for IP-address false positives.
IP_PORT_RE = re.compile(r"^\d{1,3}(\.\d{1,3}){3}:\d+$")


DEFAULT_GLOBS = (
    "docs/integrity-toolkit-spec.md",
    "docs/retro/*.md",
    "docs/diagnostics/_audits/*.md",
    "project-state.md",
)


def _resolve_targets(repo_root: Path, args: list[str]) -> list[Path]:
    if args:
        return [Path(a) for a in args]
    targets: list[Path] = []
    for pattern in DEFAULT_GLOBS:
        targets.extend(sorted(repo_root.glob(pattern)))
    return targets


def _repo_local_top_dirs(repo_root: Path) -> set[str]:
    """Set of directory names at repo root. A citation path is treated as
    repo-local only if its first segment matches one of these.

    This filters out upstream-style citations (e.g., `chapter13/cpu/LBM.cpp:97`
    or bare basenames like `LeniaNDK.py:329`) that the cat1.bare-path
    grammar accepts as valid prose but that are not meant to resolve
    against the local repo. The cat1.intra-repo check uses git ls-files
    for the same gating; this tool stays cheap by checking dir presence
    only.

    Bare files at the repo root (e.g., `CHANGELOG.md`, `project-state.md`)
    are also returned by name so single-segment citations to them still
    resolve.
    """
    entries: set[str] = set()
    for child in repo_root.iterdir():
        if child.name.startswith("."):
            continue
        entries.add(child.name)
    return entries


def _check_citation(
    repo_root: Path,
    source_path: Path,
    source_line_idx: int,
    citation_path: str,
    start: int,
    end: int | None,
) -> str | None:
    """Return an error description if the citation fails, else None."""
    target = repo_root / citation_path
    if not target.is_file():
        return (
            f"{source_path}:{source_line_idx + 1}: citation "
            f"`{citation_path}:{start}` -> file not found"
        )
    try:
        line_count = sum(1 for _ in target.open(encoding="utf-8"))
    except (OSError, UnicodeDecodeError):
        return (
            f"{source_path}:{source_line_idx + 1}: citation "
            f"`{citation_path}:{start}` -> file unreadable"
        )
    cite_end = end if end is not None else start
    if start < 1 or cite_end > line_count:
        range_suffix = f"-{end}" if end is not None else ""
        return (
            f"{source_path}:{source_line_idx + 1}: citation "
            f"`{citation_path}:{start}{range_suffix}` -> out of range "
            f"(file has {line_count} lines)"
        )
    return None


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Audit-prose freshness — verify backtick-fenced "
            "`path:line` citations in spec/retro/audit prose resolve "
            "against the repo on disk."
        ),
    )
    parser.add_argument(
        "paths", nargs="*",
        help=(
            "Files to scan. Default: docs/integrity-toolkit-spec.md, "
            "docs/retro/*.md, docs/diagnostics/_audits/*.md, project-state.md"
        ),
    )
    parser.add_argument(
        "--repo-root", type=Path, default=None,
        help="Override repo root (default: auto-detect via git).",
    )
    parser.add_argument(
        "--quiet", action="store_true",
        help="Suppress success output; print failures only.",
    )
    ns = parser.parse_args(argv)

    root = ns.repo_root if ns.repo_root else find_repo_root()
    targets = _resolve_targets(root, ns.paths)
    local_first_segments = _repo_local_top_dirs(root)

    failures: list[str] = []
    citations_checked = 0
    citations_skipped_non_local = 0
    for source in targets:
        if not source.is_file():
            continue
        try:
            with source.open(encoding="utf-8") as f:
                for line_idx, line in enumerate(f):
                    for m in CITATION_RE.finditer(line):
                        path = m.group("path")
                        candidate = f"{path}:{m.group('start')}"
                        if IP_PORT_RE.match(candidate):
                            continue
                        first_segment = path.split("/", 1)[0]
                        if first_segment not in local_first_segments:
                            citations_skipped_non_local += 1
                            continue
                        citations_checked += 1
                        end_str = m.group("end")
                        end_val = int(end_str) if end_str else None
                        err = _check_citation(
                            root, source, line_idx,
                            path,
                            int(m.group("start")),
                            end_val,
                        )
                        if err is not None:
                            failures.append(err)
        except (OSError, UnicodeDecodeError) as e:
            failures.append(f"{source}: read failure: {e}")

    if not ns.quiet:
        print(
            f"audit-prose-freshness: checked {citations_checked} "
            f"citations across {len(targets)} files "
            f"({citations_skipped_non_local} skipped as non-repo-local)",
        )
    for f in failures:
        print(f, file=sys.stderr)
    if failures:
        print(
            f"audit-prose-freshness: {len(failures)} citation(s) "
            f"failed to resolve",
            file=sys.stderr,
        )
        return 1
    if not ns.quiet:
        print("audit-prose-freshness: all citations resolve")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
