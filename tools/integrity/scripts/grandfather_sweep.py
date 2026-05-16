#!/usr/bin/env python3
"""Grandfather-sweep CLI entry. Logic lives in integrity.grandfather."""

from __future__ import annotations

import argparse
import sys
from collections import Counter
from pathlib import Path

from integrity.common.repo import find_repo_root
from integrity.grandfather import apply_annotations, rewrite_stale_reasons


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Grandfather-sweep integrity findings")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--repo-root", type=Path, default=None)
    parser.add_argument(
        "--sweep-live-source",
        action="store_true",
        help=(
            "Also sweep LIVE-SOURCE other-cat1 findings. Default is to skip them "
            "(triage section B policy). Use only when a deliberate live-source "
            "sweep is required."
        ),
    )
    parser.add_argument(
        "--force-sweep-category",
        action="append",
        default=[],
        metavar="CATEGORY",
        help=(
            "Force-sweep findings classified into the given category, "
            "regardless of LIVE-SOURCE protection. Repeatable. Example: "
            "--force-sweep-category toolkit-own-unused. Use sparingly -- "
            "this opts a single named category out of the P1.8 live-source "
            "attribution-not-sweep policy, leaving all other LIVE-SOURCE "
            "categories protected."
        ),
    )
    parser.add_argument(
        "--rewrite-stale-reasons",
        action="store_true",
        help=(
            "Rewrite the reason text of existing annotations whose embedded "
            "category no longer matches the current classifier output. "
            "Conservative: rewrites only when category changed, not when "
            "wording-only diffs are present. Mutually exclusive with the "
            "normal sweep modes; use after adding new classifier rules."
        ),
    )
    ns = parser.parse_args(argv)

    root = ns.repo_root if ns.repo_root else find_repo_root()

    if ns.rewrite_stale_reasons:
        if ns.sweep_live_source or ns.force_sweep_category:
            print(
                "error: --rewrite-stale-reasons is mutually exclusive with "
                "--sweep-live-source / --force-sweep-category",
                file=sys.stderr,
            )
            return 2
        files, anns, rewrites = rewrite_stale_reasons(root, ns.dry_run)
        label = "would rewrite" if ns.dry_run else "rewrote"
        print(
            f"grandfather-sweep: {label} {anns} annotation reasons "
            f"across {files} files"
        )
        if rewrites:
            by_transition = Counter((r[2], r[3]) for r in rewrites)
            for (old_cat, new_cat), count in by_transition.most_common():
                print(f"  {old_cat} -> {new_cat}: {count}")
        return 0

    files, anns, counts, live_source_skipped = apply_annotations(
        root, ns.dry_run,
        sweep_live_source=ns.sweep_live_source,
        force_sweep_categories=frozenset(ns.force_sweep_category),
    )

    label = "would modify" if ns.dry_run else "modified"
    print(f"grandfather-sweep: {label} {files} files; {anns} annotations added")
    if live_source_skipped:
        suffix = "" if ns.sweep_live_source else " (use --sweep-live-source to include)"
        print(f"  skipped as live-source (other-cat1): {live_source_skipped}{suffix}")
    for cat, n in sorted(counts.items(), key=lambda kv: -kv[1]):
        print(f"  {cat:>35s}: {n}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
