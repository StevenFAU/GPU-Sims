#!/usr/bin/env python3
"""Refresh per-category counts in tools/integrity/docs/grandfather-catalog.md.

Reads grandfather-catalog.md, runs `python3 -m integrity --grandfather-report
--no-history-append`, parses the per-category counts from the report, and
updates each `### `<category>` (<count>)` heading's parenthetical to match
the report. Non-numeric parentheticals (e.g. `?` placeholder, free-prose forms)
are preserved verbatim.

Reports a category present-in-report-but-absent-from-catalog as an error;
the catalog is human-authored prose (each section explains WHY the category
is grandfathered) and a mechanical stub would be wrong-shaped.

Idempotent: re-running with no underlying changes produces zero diff.

Usage:
    python3 tools/integrity/scripts/refresh_catalog_counts.py
    python3 tools/integrity/scripts/refresh_catalog_counts.py --dry-run
    python3 tools/integrity/scripts/refresh_catalog_counts.py \\
        --catalog-path tools/integrity/docs/grandfather-catalog.md
"""

from __future__ import annotations

import argparse
import difflib
import re
import subprocess
import sys
from pathlib import Path

from integrity.common.repo import find_repo_root


CATALOG_DEFAULT = Path("tools/integrity/docs/grandfather-catalog.md")

# Per probe § B.1: heading shape is `### \`<category>\` (<count>)`.
HEADING_RE = re.compile(r"^### `(?P<cat>[a-z0-9-]+)` \((?P<count>.+?)\)\s*$")

# Per probe § B.2: report lines are `{cat:>35s}: {n}` after the
# `per-category counts:` label line.
REPORT_LINE_RE = re.compile(r"^\s*(?P<cat>[a-z0-9-]+):\s+(?P<count>\d+)\s*$")

# Numeric parenthetical = decimal non-negative integer; eligible for refresh.
NUMERIC_COUNT_RE = re.compile(r"^\d+$")


def fetch_report_counts(repo_root: Path) -> dict[str, int]:
    """Run --grandfather-report --no-history-append and parse the output.

    Returns a {category_name: count} map. Raises subprocess.CalledProcessError
    on non-zero exit; raises ValueError if a line matches REPORT_LINE_RE but
    fails int parsing (defensive against format drift).
    """
    result = subprocess.run(
        ["python3", "-m", "integrity",
         "--grandfather-report", "--no-history-append"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=True,
    )
    counts: dict[str, int] = {}
    in_per_category = False
    for line in result.stdout.splitlines():
        if line.strip() == "per-category counts:":
            in_per_category = True
            continue
        if not in_per_category:
            continue
        match = REPORT_LINE_RE.match(line)
        if match:
            cat = match.group("cat")
            try:
                counts[cat] = int(match.group("count"))
            except ValueError as e:
                raise ValueError(
                    f"refresh_catalog_counts: report line '{line!r}' matched "
                    f"the regex but count failed int parsing: {e}"
                ) from e
    return counts


def parse_catalog_headings(catalog_text: str) -> list[tuple[int, str, str, bool]]:
    """Parse all H3 category headings from catalog text.

    Returns a list of (line_index, category, count_str, is_numeric) tuples
    in document order. `line_index` is 0-based.
    """
    headings: list[tuple[int, str, str, bool]] = []
    for idx, line in enumerate(catalog_text.splitlines()):
        match = HEADING_RE.match(line)
        if match:
            cat = match.group("cat")
            count_str = match.group("count")
            is_numeric = bool(NUMERIC_COUNT_RE.match(count_str))
            headings.append((idx, cat, count_str, is_numeric))
    return headings


def build_refreshed_text(
    catalog_text: str,
    report_counts: dict[str, int],
) -> tuple[str, list[str], list[str]]:
    """Build the refreshed catalog text.

    Returns (refreshed_text, errors, updates):
        refreshed_text: the catalog with numeric parentheticals refreshed
        errors: list of error messages (e.g. categories in report but not catalog)
        updates: list of human-readable update descriptions for --dry-run output
    """
    lines = catalog_text.splitlines(keepends=True)
    headings = parse_catalog_headings(catalog_text)
    catalog_categories = {cat for _, cat, _, _ in headings}

    errors: list[str] = []
    updates: list[str] = []

    # Per probe § B.7 (2): report-with-no-heading is an error.
    for cat in report_counts:
        if cat not in catalog_categories:
            errors.append(
                f"category '{cat}' has count {report_counts[cat]} in "
                f"--grandfather-report but no heading in catalog "
                f"(add a `### `{cat}` (...)` section before refreshing)"
            )

    if errors:
        return catalog_text, errors, updates

    # Refresh numeric parentheticals; preserve everything else verbatim.
    for idx, cat, count_str, is_numeric in headings:
        if not is_numeric:
            # Non-numeric parenthetical preserved verbatim per § B.7 (1)/(3).
            continue
        report_count = report_counts.get(cat)
        if report_count is None:
            # Category in catalog but not in report; zero-finding case.
            # Per § B.7 (1) flavor: leave heading unchanged.
            continue
        if int(count_str) == report_count:
            # Already correct; no-op.
            continue
        # Rewrite this line.
        old_line = lines[idx]
        new_line = HEADING_RE.sub(
            lambda m, c=report_count: f"### `{m.group('cat')}` ({c})",
            old_line.rstrip("\n"),
        ) + ("\n" if old_line.endswith("\n") else "")
        lines[idx] = new_line
        updates.append(
            f"  {cat:>35s}: {count_str} -> {report_count}"
        )

    return "".join(lines), errors, updates


def render_diff(old_text: str, new_text: str, path: Path) -> str:
    """Render a unified diff for --dry-run output."""
    return "".join(
        difflib.unified_diff(
            old_text.splitlines(keepends=True),
            new_text.splitlines(keepends=True),
            fromfile=str(path),
            tofile=str(path) + " (refreshed)",
        )
    )


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Refresh grandfather-catalog.md per-category counts"
    )
    parser.add_argument("--dry-run", action="store_true",
                        help="Print the proposed diff without writing")
    parser.add_argument("--repo-root", type=Path, default=None,
                        help="Override the repo root (default: auto-detect)")
    parser.add_argument("--catalog-path", type=Path, default=None,
                        help=f"Override the catalog path "
                             f"(default: {CATALOG_DEFAULT})")
    ns = parser.parse_args(argv)

    repo_root = ns.repo_root if ns.repo_root else find_repo_root()
    catalog_path = (
        ns.catalog_path
        if ns.catalog_path
        else repo_root / CATALOG_DEFAULT
    )

    if not catalog_path.is_file():
        print(f"refresh_catalog_counts: catalog not found at {catalog_path}",
              file=sys.stderr)
        return 2

    try:
        report_counts = fetch_report_counts(repo_root)
    except subprocess.CalledProcessError as e:
        print(f"refresh_catalog_counts: --grandfather-report failed "
              f"(exit {e.returncode}):\n{e.stderr}", file=sys.stderr)
        return 3

    catalog_text = catalog_path.read_text(encoding="utf-8")
    refreshed_text, errors, updates = build_refreshed_text(
        catalog_text, report_counts
    )

    if errors:
        print("refresh_catalog_counts: errors found:", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        return 1

    if catalog_text == refreshed_text:
        print(f"refresh_catalog_counts: no changes needed "
              f"({len(report_counts)} categories checked)")
        return 0

    if ns.dry_run:
        print(f"refresh_catalog_counts: would update {len(updates)} headings:")
        for u in updates:
            print(u)
        print()
        print(render_diff(catalog_text, refreshed_text, catalog_path))
        return 0

    catalog_path.write_text(refreshed_text, encoding="utf-8")
    print(f"refresh_catalog_counts: updated {len(updates)} headings in "
          f"{catalog_path}:")
    for u in updates:
        print(u)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
