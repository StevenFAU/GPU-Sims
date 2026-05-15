"""Check: cat3.d3q19-weights -- D3Q19 weights match algebraic ground truth.

Mode: HARD_FAIL.

Reads d3q19_equilibrium.expected.json and verifies its weights table
within 1e-12 absolute tolerance against a fresh re-derivation.
Confirms the three values 1/3, 1/18 (x6), 1/36 (x12) per d3q19.md
section 3.1, plus the sum-to-1 sanity check.
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat3_numerical.d3q19_verify import (
    load_expected_payload,
    verify_weights,
)
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat3.d3q19-weights"
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

    for msg in verify_weights(payload):
        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=ANCHOR_FILE,
            line=1,
            message=msg,
        ))
    return findings
