"""Tests for the top-level runner. Commit 1 verifies the scaffold runs."""

from __future__ import annotations

from integrity.runner import main


def test_runner_exits_clean_with_no_checks_registered() -> None:
    """Commit 1: the runner runs with zero registered checks and exits 0."""
    rc = main(["--output", "human"])
    assert rc == 0


def test_runner_rejects_bad_cli() -> None:
    rc = main(["--cat", "99"])
    assert rc == 64


def test_runner_filter_by_check_runs_clean() -> None:
    rc = main(["--check", "cat1.upstream-anchor", "--output", "json"])
    assert rc == 0
