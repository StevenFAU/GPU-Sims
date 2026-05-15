"""Check: cat3.d3q19-equilibrium -- D3Q19 feq() per-test-point evaluations match algebraic ground truth.

Mode: HARD_FAIL.

Reads d3q19_equilibrium.expected.json and verifies each test_points[*].feq
table against a fresh evaluation of feq(rho, u, c_i, w_i) within 1e-12
absolute tolerance. Test points pinned in d3q19.md section 4.2:
  tp1_zero_velocity   -- rho=1.0, u=(0,0,0)
  tp2_uniform_x       -- rho=1.0, u=(0.1, 0, 0)
  tp3_oblique_xy      -- rho=1.0, u=(0.05, 0.05, 0)
  tp4_density_scaled  -- rho=2.5, u=(0.05, 0, 0)
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat3_numerical.d3q19_verify import (
    load_expected_payload,
    verify_equilibrium,
)
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat3.d3q19-equilibrium"
MODE = FailureMode.HARD_FAIL

ANCHOR_FILE = "tools/integrity/docs/algebraic/d3q19.md"


def run(repo_root: Path) -> list[Finding]:
    findings: list[Finding] = []
    try:
        payload = load_expected_payload()
    except FileNotFoundError as e:
        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=ANCHOR_FILE,
            line=1,
            message=f"d3q19 expected payload missing: {e}",
        ))
        return findings

    for msg in verify_equilibrium(payload):
        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=ANCHOR_FILE,
            line=1,
            message=msg,
        ))
    return findings
