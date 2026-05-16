"""Tests for tools/integrity/scripts/check_paired_sweep.py (T2.1)."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pytest


SCRIPT = (
    Path(__file__).resolve().parent.parent / "scripts" / "check_paired_sweep.py"
)


def _git(repo_root: Path, *args: str) -> str:
    """Run git in `repo_root` and return stdout."""
    return subprocess.run(
        ["git", "-c", "user.email=test@example.com",
         "-c", "user.name=test", *args],
        cwd=repo_root, capture_output=True, text=True, check=True,
    ).stdout


def _init_repo(tmp_path: Path) -> None:
    """Initialize a tmp git repo with one anchor commit on main."""
    _git(tmp_path, "init", "-q", "-b", "main")
    (tmp_path / "README.md").write_text("anchor\n")
    _git(tmp_path, "add", "README.md")
    _git(tmp_path, "commit", "-q", "-m", "anchor")


def _run_script(repo_root: Path, *args: str) -> tuple[int, str, str]:
    result = subprocess.run(
        [sys.executable, str(SCRIPT), *args,
         "--repo-root", str(repo_root)],
        cwd=str(repo_root), capture_output=True, text=True, check=False,
    )
    return result.returncode, result.stdout, result.stderr


def test_no_live_source_changes_passes(tmp_path: Path) -> None:
    """When only docs/ or audit-doc files change, the check passes."""
    _init_repo(tmp_path)
    base = _git(tmp_path, "rev-parse", "HEAD").strip()
    (tmp_path / "docs").mkdir()
    (tmp_path / "docs" / "diagnostics").mkdir()
    (tmp_path / "docs" / "diagnostics" / "_audits").mkdir()
    audit = tmp_path / "docs" / "diagnostics" / "_audits" / "audit.md"
    audit.write_text("an audit doc\n")
    _git(tmp_path, "add", str(audit))
    _git(tmp_path, "commit", "-q", "-m", "docs: audit")
    head = _git(tmp_path, "rev-parse", "HEAD").strip()

    rc, out, _err = _run_script(
        tmp_path, "--base-ref", base, "--head-ref", head,
    )
    assert rc == 0
    assert "no cat1-scannable live-source files changed" in out


def test_live_source_change_without_sweep_fails(tmp_path: Path) -> None:
    """When a cat1-scannable live-source file changes and no sweep
    commit appears in the range, exit code 1."""
    _init_repo(tmp_path)
    base = _git(tmp_path, "rev-parse", "HEAD").strip()
    (tmp_path / "common").mkdir()
    live = tmp_path / "common" / "widget.cpp"
    live.write_text("// live source\n")
    _git(tmp_path, "add", str(live))
    _git(tmp_path, "commit", "-q", "-m", "feat: add live source")
    head = _git(tmp_path, "rev-parse", "HEAD").strip()

    rc, _out, err = _run_script(
        tmp_path, "--base-ref", base, "--head-ref", head,
    )
    assert rc == 1
    assert "without a paired grandfather-sweep" in err


def test_skip_tag_overrides_check(tmp_path: Path) -> None:
    """An explicit [skip-paired-sweep] in any commit body in the range
    short-circuits the check to pass."""
    _init_repo(tmp_path)
    base = _git(tmp_path, "rev-parse", "HEAD").strip()
    (tmp_path / "common").mkdir()
    live = tmp_path / "common" / "widget.cpp"
    live.write_text("// live source, docs-only intent\n")
    _git(tmp_path, "add", str(live))
    _git(
        tmp_path, "commit", "-q",
        "-m", "feat: add live source\n\nNo new findings expected. [skip-paired-sweep]",
    )
    head = _git(tmp_path, "rev-parse", "HEAD").strip()

    rc, out, _err = _run_script(
        tmp_path, "--base-ref", base, "--head-ref", head,
    )
    assert rc == 0
    assert "skip-paired-sweep" in out
