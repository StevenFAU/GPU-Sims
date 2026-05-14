"""Tests for the top-level runner."""

from __future__ import annotations

from pathlib import Path

from integrity.runner import main, parse_args


def test_runner_parses_args_cleanly() -> None:
    args = parse_args(["--cat", "1", "--output", "json"])
    assert args.cat == 1
    assert args.output == "json"


def test_runner_rejects_bad_cli() -> None:
    rc = main(["--cat", "99"])
    assert rc == 64


def test_runner_runs_against_fixtures_clean(
    tmp_path: Path,
    fixtures_dir: Path,
) -> None:
    """Running against the good-citations fixture dir should exit 0."""
    rc = main([
        "--root", str(fixtures_dir / "good_citations"),
        "--output", "human",
        "--no-audit-log",
    ])
    assert rc == 0


def test_runner_runs_against_bad_fixtures_fails(
    fixtures_dir: Path,
) -> None:
    """Running against the bad-citations fixture dir should exit 1
    (HARD_FAIL findings present)."""
    rc = main([
        "--root", str(fixtures_dir / "bad_citations"),
        "--output", "human",
        "--no-audit-log",
    ])
    assert rc == 1


def test_runner_warn_only_mode_downgrades(fixtures_dir: Path) -> None:
    """Same bad fixtures, but --mode warn-only should exit 0."""
    rc = main([
        "--root", str(fixtures_dir / "bad_citations"),
        "--mode", "warn-only",
        "--no-audit-log",
    ])
    assert rc == 0
