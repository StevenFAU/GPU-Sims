"""Tests for tools/integrity/scripts/audit_prose_freshness.py (T2.2)."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


SCRIPT = Path("tools/integrity/scripts/audit_prose_freshness.py")


def _run_script(repo_root: Path, *args: str) -> tuple[int, str, str]:
    """Invoke the script as a subprocess, returning (rc, stdout, stderr)."""
    result = subprocess.run(
        [
            sys.executable,
            str(_real_script_path()),
            *args,
            "--repo-root", str(repo_root),
        ],
        capture_output=True, text=True, cwd=str(repo_root), check=False,
    )
    return result.returncode, result.stdout, result.stderr


def _real_script_path() -> Path:
    """Return the absolute path of audit_prose_freshness.py from this
    test file's location (avoids cwd ambiguity)."""
    return (
        Path(__file__).resolve().parent.parent
        / "scripts" / "audit_prose_freshness.py"
    )


def _init_git(tmp_path: Path) -> None:
    """Initialize a git repo so find_repo_root succeeds, even though
    --repo-root is passed explicitly (the script's import-time path
    resolution still expects a repo)."""
    subprocess.run(["git", "init", "-q"], cwd=tmp_path, check=True)


def test_resolves_valid_citation(tmp_path: Path) -> None:
    """A citation under a repo-local subdirectory at an in-range line resolves."""
    (tmp_path / "docs").mkdir()
    target = tmp_path / "docs" / "valid.md"
    target.write_text("line 1\nline 2\nline 3\n")
    source = tmp_path / "source.md"
    # integrity-allow: cat1.intra-repo; toolkit-own test fixture string mimicking citation grammar; n/a
    source.write_text("see `docs/valid.md:2` for details\n")
    _init_git(tmp_path)
    rc, out, _err = _run_script(tmp_path, str(source))
    assert rc == 0
    assert "all citations resolve" in out


def test_fails_on_missing_file(tmp_path: Path) -> None:
    """A citation pointing at a missing file under a repo-local subdir fails."""
    (tmp_path / "docs").mkdir()
    source = tmp_path / "source.md"
    # integrity-allow: cat1.intra-repo; toolkit-own test fixture string mimicking citation grammar; n/a
    source.write_text("see `docs/missing.md:1` for details\n")
    _init_git(tmp_path)
    rc, _out, err = _run_script(tmp_path, str(source))
    assert rc == 1
    assert "file not found" in err


def test_fails_on_out_of_range_line(tmp_path: Path) -> None:
    """A citation with a line number past EOF fails."""
    (tmp_path / "docs").mkdir()
    target = tmp_path / "docs" / "short.md"
    target.write_text("only one line\n")
    source = tmp_path / "source.md"
    # integrity-allow: cat1.intra-repo; toolkit-own test fixture string mimicking citation grammar; n/a
    source.write_text("see `docs/short.md:5` for details\n")
    _init_git(tmp_path)
    rc, _out, err = _run_script(tmp_path, str(source))
    assert rc == 1
    assert "out of range" in err


def test_range_citation_resolves(tmp_path: Path) -> None:
    """A range citation `file:start-end` resolves when in range."""
    (tmp_path / "docs").mkdir()
    target = tmp_path / "docs" / "ranged.md"
    target.write_text("line 1\nline 2\nline 3\nline 4\nline 5\n")
    source = tmp_path / "source.md"
    # integrity-allow: cat1.intra-repo; toolkit-own test fixture string mimicking citation grammar; n/a
    source.write_text("see `docs/ranged.md:2-4` for details\n")
    _init_git(tmp_path)
    rc, _out, _err = _run_script(tmp_path, str(source))
    assert rc == 0


def test_non_repo_local_citations_skipped(tmp_path: Path) -> None:
    """Upstream-style citations (path's first segment is not a repo dir)
    are skipped, not reported as failures. Documents D3-aligned scoping."""
    source = tmp_path / "source.md"
    # integrity-allow: cat1.intra-repo; toolkit-own test fixture string mimicking upstream citation grammar; n/a
    # integrity-allow: cat1.bare-path; toolkit-own test fixture string mimicking upstream citation grammar; n/a
    source.write_text(
        "see `chapter13/cpu/LBM.cpp:97` for an upstream pattern; "
# integrity-allow: cat1.bare-path; deferred-upstream-bare-path citation (Chakazul/LeniaNDK pending vendoring decision per ground-truth-sources.md); n/a
        "and `LeniaNDK.py:329` for a bare-basename upstream cite\n"
    )
    _init_git(tmp_path)
    rc, out, _err = _run_script(tmp_path, str(source))
    assert rc == 0
    assert "skipped as non-repo-local" in out
