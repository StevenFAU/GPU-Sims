"""Tests for the cat3.d3q19-* check modules and the shared harness helpers."""

from __future__ import annotations

from pathlib import Path

import pytest

from integrity.cat3_numerical.d3q19_verify import (
    EXPECTED_JSON,
    build_velocity_set,
    feq,
    load_expected_payload,
    verify_equilibrium,
    verify_velocity_set,
    verify_weights,
    weight_for,
)
from integrity.cat3_numerical.checks import (
    d3q19_velocity_set,
    d3q19_weights,
    d3q19_equilibrium,
)
from integrity.common.results import FailureMode


# ---------------------------------------------------------------------------
# Harness-level tests
# ---------------------------------------------------------------------------


def test_build_velocity_set_returns_19_vectors() -> None:
    cs = build_velocity_set()
    assert len(cs) == 19


def test_build_velocity_set_first_moment_vanishes() -> None:
    cs = build_velocity_set()
    sx = sum(c[0] for c in cs)
    sy = sum(c[1] for c in cs)
    sz = sum(c[2] for c in cs)
    assert (sx, sy, sz) == (0, 0, 0)


def test_weight_for_partitions_by_squared_norm() -> None:
    # Rest vector: 1/3
    assert float(weight_for((0, 0, 0))) == pytest.approx(1.0 / 3.0)
    # Face neighbor: 1/18
    assert float(weight_for((1, 0, 0))) == pytest.approx(1.0 / 18.0)
    # Edge neighbor: 1/36
    assert float(weight_for((1, 1, 0))) == pytest.approx(1.0 / 36.0)


def test_feq_conserves_mass_at_zero_velocity() -> None:
    cs = build_velocity_set()
    ws = [float(weight_for(c)) for c in cs]
    out = feq(1.0, 0.0, 0.0, 0.0, cs, ws)
    assert sum(out) == pytest.approx(1.0, abs=1e-12)


# ---------------------------------------------------------------------------
# Check-module tests
# ---------------------------------------------------------------------------


@pytest.fixture
def repo_root(tmp_path: Path) -> Path:
    return tmp_path


def test_velocity_set_check_passes_on_correct_payload(repo_root: Path) -> None:
    findings = d3q19_velocity_set.run(repo_root)
    assert findings == []


def test_weights_check_passes_on_correct_payload(repo_root: Path) -> None:
    findings = d3q19_weights.run(repo_root)
    assert findings == []


def test_equilibrium_check_passes_on_correct_payload(repo_root: Path) -> None:
    findings = d3q19_equilibrium.run(repo_root)
    assert findings == []


def test_velocity_set_check_fails_on_corrupted_payload(
    monkeypatch: pytest.MonkeyPatch, repo_root: Path
) -> None:
    """Corrupting the velocity_set in the payload should produce a HARD_FAIL finding."""
    real = load_expected_payload()
    real["velocity_set"][0] = [9, 9, 9]  # corrupt the rest vector

    monkeypatch.setattr(
        "integrity.cat3_numerical.checks.d3q19_velocity_set.load_expected_payload",
        lambda: real,
    )
    findings = d3q19_velocity_set.run(repo_root)
    assert len(findings) == 1
    assert findings[0].check_id == "cat3.d3q19-velocity-set"
    assert findings[0].mode == FailureMode.HARD_FAIL
    assert "mismatch" in findings[0].message


def test_weights_check_fails_on_corrupted_payload(
    monkeypatch: pytest.MonkeyPatch, repo_root: Path
) -> None:
    real = load_expected_payload()
    real["weights"][0] = 0.5  # was 1/3

    monkeypatch.setattr(
        "integrity.cat3_numerical.checks.d3q19_weights.load_expected_payload",
        lambda: real,
    )
    findings = d3q19_weights.run(repo_root)
    assert len(findings) >= 1
    assert all(f.check_id == "cat3.d3q19-weights" for f in findings)


def test_equilibrium_check_fails_on_corrupted_payload(
    monkeypatch: pytest.MonkeyPatch, repo_root: Path
) -> None:
    real = load_expected_payload()
    real["test_points"][0]["feq"][0] = 999.0  # arbitrary nonsense

    monkeypatch.setattr(
        "integrity.cat3_numerical.checks.d3q19_equilibrium.load_expected_payload",
        lambda: real,
    )
    findings = d3q19_equilibrium.run(repo_root)
    assert len(findings) >= 1
    assert all(f.check_id == "cat3.d3q19-equilibrium" for f in findings)


def test_check_ids_match_registry() -> None:
    """The three CHECK_IDs must match the [Algebraic_D3Q19] used_by_checks
    entries in ground-truth-sources.md (verified by probe section B.4)."""
    assert d3q19_velocity_set.CHECK_ID == "cat3.d3q19-velocity-set"
    assert d3q19_weights.CHECK_ID == "cat3.d3q19-weights"
    assert d3q19_equilibrium.CHECK_ID == "cat3.d3q19-equilibrium"


def test_payload_is_present() -> None:
    """Pin the expected JSON's presence at the new path."""
    assert EXPECTED_JSON.is_file()
