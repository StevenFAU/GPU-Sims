#!/usr/bin/env python3
"""Paired-sweep enforcement (T2.1 CI check).

Verifies that any PR / push that changes cat1-scannable live-source
files in its diff also includes a paired grandfather-sweep commit
(or an explicit ``[skip-paired-sweep]`` escape-hatch tag in any
commit body in the range).

Used by .github/workflows/integrity.yml as a separate job step from
the main gate. Independent of the integrity check registry; this is
a workflow-level enforcement, not a gate check (per Decision D2,
v1.3 closeout spec section 0.3).

Exit codes:
    0 -- no live-source files changed, or a paired sweep / skip tag
         is present in the commit range
    1 -- live-source files changed without a paired sweep
    2 -- bad CLI args (argparse default)
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from integrity.common.repo import find_repo_root
from integrity.grandfather import is_live_source_path


SWEEP_MARKERS = ("grandfather-sweep", "grandfather sweep", "sweep-companion")
SKIP_TAG = "[skip-paired-sweep]"
CATALOG_PATH = "tools/integrity/docs/grandfather-catalog.md"


def _git_diff_files(base_ref: str, head_ref: str, repo_root: Path) -> list[str]:
    """Return repo-relative paths of files changed in base..head, falling
    back to HEAD~1..HEAD if the supplied refs do not resolve."""
    result = subprocess.run(
        ["git", "diff", "--name-only", f"{base_ref}...{head_ref}"],
        cwd=repo_root, capture_output=True, text=True, check=False,
    )
    if result.returncode != 0:
        result = subprocess.run(
            ["git", "diff", "--name-only", "HEAD~1...HEAD"],
            cwd=repo_root, capture_output=True, text=True, check=True,
        )
    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def _commits_in_range(
    base_ref: str,
    head_ref: str,
    repo_root: Path,
) -> list[tuple[str, str]]:
    """Return [(sha, body)] for commits in base..head."""
    result = subprocess.run(
        ["git", "log", "--format=%H%n%B%n---END---", f"{base_ref}..{head_ref}"],
        cwd=repo_root, capture_output=True, text=True, check=False,
    )
    if result.returncode != 0:
        result = subprocess.run(
            ["git", "log", "--format=%H%n%B%n---END---", "-1"],
            cwd=repo_root, capture_output=True, text=True, check=True,
        )
    commits: list[tuple[str, str]] = []
    for chunk in result.stdout.split("---END---"):
        chunk = chunk.strip()
        if not chunk:
            continue
        lines = chunk.split("\n", 1)
        sha = lines[0]
        body = lines[1] if len(lines) > 1 else ""
        commits.append((sha, body))
    return commits


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Check for paired grandfather-sweep on cat1-scannable changes",
    )
    parser.add_argument("--base-ref", default="HEAD~1")
    parser.add_argument("--head-ref", default="HEAD")
    parser.add_argument("--repo-root", type=Path, default=None)
    ns = parser.parse_args(argv)

    root = ns.repo_root if ns.repo_root else find_repo_root()
    changed_files = _git_diff_files(ns.base_ref, ns.head_ref, root)
    live_source_changed = [
        f for f in changed_files if is_live_source_path(f)
    ]

    if not live_source_changed:
        print("check-paired-sweep: no cat1-scannable live-source files changed; OK")
        return 0

    commits = _commits_in_range(ns.base_ref, ns.head_ref, root)
    for sha, body in commits:
        body_lower = body.lower()
        if SKIP_TAG.lower() in body_lower:
            print(f"check-paired-sweep: {sha[:8]} carries {SKIP_TAG}; OK")
            return 0
        if any(marker in body_lower for marker in SWEEP_MARKERS):
            print(f"check-paired-sweep: {sha[:8]} is a paired sweep commit; OK")
            return 0
    if CATALOG_PATH in changed_files:
        print(
            f"check-paired-sweep: {CATALOG_PATH} touched; "
            f"treating as paired-sweep; OK",
        )
        return 0

    print(
        f"check-paired-sweep: FAIL -- {len(live_source_changed)} cat1-scannable "
        f"live-source files changed without a paired grandfather-sweep commit.",
        file=sys.stderr,
    )
    print(
        "Either run `python3 tools/integrity/scripts/grandfather_sweep.py` and amend,",
        file=sys.stderr,
    )
    print(
        f"or add {SKIP_TAG} to a commit body if the changes won't add findings.",
        file=sys.stderr,
    )
    for f in live_source_changed[:10]:
        print(f"  changed: {f}", file=sys.stderr)
    if len(live_source_changed) > 10:
        print(f"  ... and {len(live_source_changed) - 10} more", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
