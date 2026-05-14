#!/usr/bin/env python3
"""Grandfather-sweep CLI entry. Logic lives in integrity.grandfather."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from integrity.common.repo import find_repo_root
from integrity.grandfather import apply_annotations


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Grandfather-sweep integrity findings")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--repo-root", type=Path, default=None)
    ns = parser.parse_args(argv)

    root = ns.repo_root if ns.repo_root else find_repo_root()
    files, anns, counts = apply_annotations(root, ns.dry_run)

    label = "would modify" if ns.dry_run else "modified"
    print(f"grandfather-sweep: {label} {files} files; {anns} annotations added")
    for cat, n in sorted(counts.items(), key=lambda kv: -kv[1]):
        print(f"  {cat:>35s}: {n}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
