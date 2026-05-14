"""Git / repo helpers."""

from __future__ import annotations

import subprocess
from pathlib import Path


def find_repo_root(start: Path | None = None) -> Path:
    """Walk upward from `start` (default cwd) to find the git repo root."""
    cwd = start if start else Path.cwd()
    cur = cwd.resolve()
    while cur != cur.parent:
        if (cur / ".git").exists():
            return cur
        cur = cur.parent
    raise RuntimeError(f"No git repo found from {cwd}")


def git_head_sha(root: Path) -> str:
    """Return short HEAD SHA for the repo at `root`."""
    result = subprocess.run(
        ["git", "rev-parse", "--short", "HEAD"],
        cwd=root,
        capture_output=True,
        text=True,
        check=True,
    )
    return result.stdout.strip()


def list_tracked_files(root: Path) -> list[Path]:
    """Return all git-tracked files under `root` as repo-relative paths."""
    result = subprocess.run(
        ["git", "ls-files"],
        cwd=root,
        capture_output=True,
        text=True,
        check=True,
    )
    return [root / line for line in result.stdout.splitlines() if line]
