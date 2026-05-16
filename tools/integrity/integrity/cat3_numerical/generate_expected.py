#!/usr/bin/env python3
"""Generate expected_values.json from the cubic spline kernel formula.

See tools/integrity/docs/cat3-conventions.md for the file-format
convention (JSON-as-canonical for cat3 expected-values files).

Analytical derivation from Bender-Koschier 2015 / SPlisHSPlasH 2.16.1
# integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
SPHKernels.h:43-85. Run manually when the test point set changes or
the registered upstream anchor SHA bumps and the formula needs
re-verification against the new source.

Usage:
    python generate_expected.py                  # normal regen
    python generate_expected.py --inject-factor-of-6
                                                 # inject the Phase 11.5 commit 1
                                                 # gradient defect for acceptance testing

The script writes to expected_values.json in the same directory.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
OUTPUT_PATH = SCRIPT_DIR / "expected_values.json"


# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
def cubic_W(q: float, h: float) -> float:
# integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
    """Cubic spline kernel W(r,h) per SPHKernels.h:37-55.

    Returns 0 outside support (q > 1).
    """
    if q > 1.0:
        return 0.0
    norm = 8.0 / (math.pi * h * h * h)
    if q <= 0.5:
        return norm * (6.0 * q * q * q - 6.0 * q * q + 1.0)
    return norm * 2.0 * (1.0 - q) ** 3


# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
def cubic_gradW_magnitude(q: float, h: float, inject_factor_of_6: bool = False) -> float:
# integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
    """|gradW(r,h)| per SPHKernels.h:62-85.

    The gradient direction is along r-hat; we report magnitude only.
    norm = 8/(pi*h^4); inner poly = 18q^2 - 12q; outer poly = 6*(1-q)^2.

    If inject_factor_of_6=True, returns 6 times the correct value
    (the Phase 11.5 commit 1 defect class). Used for acceptance testing.
    """
    if q > 1.0 or q == 0.0:
        return 0.0
    norm = 8.0 / (math.pi * h * h * h * h)
    if q <= 0.5:
        magnitude = norm * abs(18.0 * q * q - 12.0 * q)
    else:
        magnitude = norm * 6.0 * (1.0 - q) ** 2

    if inject_factor_of_6:
        magnitude *= 6.0

    return magnitude


TEST_POINTS_Q = [0.0, 0.1, 0.25, 0.5, 0.75, 1.0]
H = 1.0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--inject-factor-of-6",
        action="store_true",
        help="Inject the Phase 11.5 commit-1 gradient defect (acceptance testing)",
    )
    args = parser.parse_args()

    inject = args.inject_factor_of_6

    test_points = []
    for q in TEST_POINTS_Q:
        test_points.append({
            "q": q,
            "h": H,
            "expected_W": cubic_W(q, H),
            "expected_gradW_magnitude": cubic_gradW_magnitude(
                q, H, inject_factor_of_6=inject
            ),
        })

    payload = {
        "schema_version": 1,
        "source": "tools/integrity/integrity/cat3_numerical/generate_expected.py",
# integrity-allow: cat1.upstream-citation; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
        "derivation": "SPlisHSPlasH 2.16.1 SPHKernels.h:43-85",
        "anchor_sha": "6bff55a6eaf14083d34650f22a268ce156b62b54",
        "tolerance": {"atol": 1e-5, "rtol": 1e-5},
        "test_points": test_points,
    }

    OUTPUT_PATH.write_text(
        json.dumps(payload, indent=2) + "\n",
        encoding="utf-8",
    )
    print(f"Wrote {OUTPUT_PATH}")
    if inject:
        print("Defect injected. Restore via: python generate_expected.py (no flag)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
