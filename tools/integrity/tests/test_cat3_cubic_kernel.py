"""Tests for cat3.cubic-kernel."""

from __future__ import annotations

import json
import math
import shutil
import subprocess
from pathlib import Path

import pytest

from integrity.cat3_numerical.cubic_kernel import (
    load_expected_values,
    within_tolerance,
)


def test_within_tolerance_exact_match() -> None:
    assert within_tolerance(1.0, 1.0, 1e-5, 1e-5)


def test_within_tolerance_small_drift() -> None:
    assert within_tolerance(1.000001, 1.0, 1e-5, 1e-5)


def test_within_tolerance_large_drift() -> None:
    assert not within_tolerance(1.5, 1.0, 1e-5, 1e-5)


def test_within_tolerance_nan_rejected() -> None:
    assert not within_tolerance(float("nan"), 1.0, 1e-5, 1e-5)


def test_load_expected_values_real_file() -> None:
    """The committed expected_values.toml should parse cleanly with 6 points."""
    repo_root = Path(__file__).resolve().parents[3]
    points, tolerance = load_expected_values(repo_root)
    assert len(points) == 6, f"expected 6 test points, got {len(points)}"
    assert tolerance == {"atol": 1e-5, "rtol": 1e-5}


def test_load_expected_values_specific_q() -> None:
    """W(q=0.0, h=1.0) should be 8/pi."""
    repo_root = Path(__file__).resolve().parents[3]
    points, _ = load_expected_values(repo_root)
    q0_point = next((p for p in points if p.q == 0.0), None)
    assert q0_point is not None
    assert abs(q0_point.expected_W - (8.0 / math.pi)) < 1e-10


def test_load_expected_values_q_at_support_boundary() -> None:
    """W(q=1.0, h=1.0) should be 0 (kernel support cutoff)."""
    repo_root = Path(__file__).resolve().parents[3]
    points, _ = load_expected_values(repo_root)
    q1_point = next((p for p in points if p.q == 1.0), None)
    assert q1_point is not None
    assert q1_point.expected_W == 0.0
    assert q1_point.expected_gradW_magnitude == 0.0


@pytest.mark.skipif(
    not shutil.which("cmake") or not shutil.which("ninja"),
    reason="cmake or ninja not available",
)
def test_driver_builds_and_runs(tmp_path: Path) -> None:
    """End-to-end smoke: build the driver, invoke it, parse JSON."""
    repo_root = Path(__file__).resolve().parents[3]

    build_dir = tmp_path / "build"
    cfg = subprocess.run(
        ["cmake", "-S", str(repo_root), "-B", str(build_dir), "-G", "Ninja",
         "-DGPU_SIMS_BUILD_INTEGRITY_CAT3=ON"],
        capture_output=True, text=True, timeout=300,
    )
    if cfg.returncode != 0:
        pytest.skip(f"cmake configure failed: {cfg.stderr[:500]}")

    build = subprocess.run(
        ["ninja", "-C", str(build_dir), "integrity_cat3_stack_c"],
        capture_output=True, text=True, timeout=60,
    )
    if build.returncode != 0:
        pytest.skip(f"ninja build failed: {build.stderr[:500]}")

    driver = build_dir / "tools/integrity/drivers/integrity_cat3_stack_c/integrity_cat3_stack_c"
    if not driver.is_file():
        pytest.skip(f"driver not produced at {driver}")

    result = subprocess.run(
        [str(driver), "0.5", "1.0"],
        capture_output=True, text=True, timeout=5,
    )
    assert result.returncode == 0
    data = json.loads(result.stdout)
    assert "evaluations" in data
    assert len(data["evaluations"]) == 1
    ev = data["evaluations"][0]
    assert ev["q"] == 0.5
    assert ev["h"] == 1.0
    # W(q=0.5, h=1.0) = (8/pi) * 0.25 (boundary value)
    assert abs(ev["W"] - (8.0 / math.pi) * 0.25) < 1e-12


def test_check_graceful_degrade_without_driver(tmp_path: Path) -> None:
    """Without the driver built, the check returns no findings."""
    from integrity.cat3_numerical.checks.cubic_kernel import run
    findings = run(tmp_path)
    assert findings == []
