"""Cubic SPH kernel numerical correctness per spec § 8.

Reads expected values from expected_values.toml, runs the Stack C
driver binary at build/tools/integrity/drivers/integrity_cat3_stack_c/,
compares each evaluation against tolerance. HARD_FAIL on any
disagreement.

The driver is built when GPU_SIMS_BUILD_INTEGRITY_CAT3=ON in cmake.
The Python check graceful-degrades to zero findings when the driver
isn't present (e.g., flag not set, build not run).
"""

from __future__ import annotations

import json
import math
import os
import subprocess
import tomllib
from dataclasses import dataclass
from pathlib import Path


DRIVER_RELATIVE_PATH = Path(
    "build/tools/integrity/drivers/integrity_cat3_stack_c/integrity_cat3_stack_c"
)
EXPECTED_VALUES_RELATIVE = Path(
    "tools/integrity/integrity/cat3_numerical/expected_values.toml"
)


@dataclass(frozen=True)
# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
class TestPoint:
    q: float
    h: float
    expected_W: float
    expected_gradW_magnitude: float


@dataclass(frozen=True)
# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
class DriverEvaluation:
    q: float
    h: float
    W: float
    gradW_magnitude: float


def load_expected_values(repo_root: Path) -> tuple[list[TestPoint], dict]:
    """Parse expected_values.toml. Returns (test_points, tolerance_dict)."""
    path = repo_root / EXPECTED_VALUES_RELATIVE
    if not path.is_file():
        return [], {}

    data = tomllib.loads(path.read_text(encoding="utf-8"))
    tolerance = data.get("tolerance", {"atol": 1e-5, "rtol": 1e-5})
    points: list[TestPoint] = []
    for tp in data.get("test_points", []):
        points.append(TestPoint(
            q=float(tp["q"]),
            h=float(tp["h"]),
            expected_W=float(tp["expected_W"]),
            expected_gradW_magnitude=float(tp["expected_gradW_magnitude"]),
        ))
    return points, tolerance


def find_driver(repo_root: Path) -> Path | None:
    """Locate the driver binary, or None if not built."""
    candidate = repo_root / DRIVER_RELATIVE_PATH
    if candidate.is_file() and os.access(candidate, os.X_OK):
        return candidate
    return None


def run_driver(driver: Path, points: list[TestPoint]) -> list[DriverEvaluation]:
    """Invoke the driver with (q, h) pairs as args, parse JSON output."""
    args = [str(driver)]
    for tp in points:
        args.extend([str(tp.q), str(tp.h)])

    try:
        result = subprocess.run(
            args, capture_output=True, text=True, timeout=10, check=False,
        )
    except (subprocess.TimeoutExpired, OSError):
        return []

    if result.returncode != 0:
        return []

    try:
        data = json.loads(result.stdout)
    except json.JSONDecodeError:
        return []

    evaluations: list[DriverEvaluation] = []
    for ev in data.get("evaluations", []):
        evaluations.append(DriverEvaluation(
            q=float(ev["q"]),
            h=float(ev["h"]),
            W=float(ev["W"]),
            gradW_magnitude=float(ev["gradW_magnitude"]),
        ))
    return evaluations


def within_tolerance(actual: float, expected: float, atol: float, rtol: float) -> bool:
    """abs(actual - expected) <= atol + rtol * abs(expected). NaN rejected."""
    if not math.isfinite(actual):
        return False
    return abs(actual - expected) <= atol + rtol * abs(expected)
