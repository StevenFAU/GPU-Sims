"""pytest fixtures shared across integrity tests."""

from __future__ import annotations

import subprocess
from pathlib import Path

import pytest


@pytest.fixture
def fixtures_dir() -> Path:
    return Path(__file__).parent / "fixtures"


@pytest.fixture
def repo_root() -> Path:
    """Resolve to the git repo root for tests that need it."""
    result = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        capture_output=True,
        text=True,
        check=True,
    )
    return Path(result.stdout.strip())
