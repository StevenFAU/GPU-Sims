"""Check: cat3.cubic-kernel — Stack C cubic kernel matches upstream formula.

Mode: HARD_FAIL.

Compares the Stack C driver's evaluation of W(r,h) and the magnitude of
gradW against analytically-derived expected values from the cubic
spline formula in SPlisHSPlasH 2.16.1 SPHKernels.h:43-78 at the
registered vendor anchor SHA.

Graceful degrade: if the driver isn't built (the
GPU_SIMS_BUILD_INTEGRITY_CAT3 CMake flag wasn't set), this check
returns zero findings.
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat3_numerical.cubic_kernel import (
    EXPECTED_VALUES_RELATIVE,
    find_driver,
    load_expected_values,
    run_driver,
    within_tolerance,
)
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat3.cubic-kernel"
MODE = FailureMode.HARD_FAIL


def run(repo_root: Path) -> list[Finding]:
    findings: list[Finding] = []

    points, tolerance = load_expected_values(repo_root)
    if not points:
        return findings

    driver = find_driver(repo_root)
    if driver is None:
        # Driver not built (CMake flag not set); graceful degrade
        return findings

    atol = tolerance.get("atol", 1e-5)
    rtol = tolerance.get("rtol", 1e-5)

    evaluations = run_driver(driver, points)
    if not evaluations:
        return findings

    if len(evaluations) != len(points):
        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=str(EXPECTED_VALUES_RELATIVE),
            line=1,
            message=(
                f"driver returned {len(evaluations)} evaluations; "
                f"expected {len(points)} (one per test point)"
            ),
        ))
        return findings

    for tp, ev in zip(points, evaluations):
        if not within_tolerance(ev.W, tp.expected_W, atol, rtol):
            findings.append(Finding(
                check_id=CHECK_ID,
                mode=MODE,
                file=str(EXPECTED_VALUES_RELATIVE),
                line=1,
                message=(
                    f"W(q={tp.q}, h={tp.h}): driver={ev.W:.7g} "
                    f"expected={tp.expected_W:.7g} "
                    f"(atol={atol}, rtol={rtol})"
                ),
                ground_truth_ref="SPlisHSPlasH 2.16.1 SPHKernels.h:43-52 + spec § 8.2",
            ))

        if not within_tolerance(
            ev.gradW_magnitude, tp.expected_gradW_magnitude, atol, rtol,
        ):
            findings.append(Finding(
                check_id=CHECK_ID,
                mode=MODE,
                file=str(EXPECTED_VALUES_RELATIVE),
                line=1,
                message=(
                    f"|gradW|(q={tp.q}, h={tp.h}): driver={ev.gradW_magnitude:.7g} "
                    f"expected={tp.expected_gradW_magnitude:.7g} "
                    f"(atol={atol}, rtol={rtol})"
                ),
                ground_truth_ref="SPlisHSPlasH 2.16.1 SPHKernels.h:62-85 + spec § 8.2",
            ))

    return findings
