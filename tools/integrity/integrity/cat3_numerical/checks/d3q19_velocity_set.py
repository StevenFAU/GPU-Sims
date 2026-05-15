"""Check: cat3.d3q19-velocity-set -- D3Q19 velocity vectors match algebraic ground truth.

Mode: HARD_FAIL.

Reads d3q19_equilibrium.expected.json and verifies its velocity_set
table byte-for-byte against a fresh re-derivation. Closes the
registry-vs-implementation drift surfaced by phase12_substantive_landing
section "Convention #8 firings caught and recorded" item 9 plus
addendum section 4.3 / probe section B.5.
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat3_numerical.d3q19_verify import (
    load_expected_payload,
    verify_velocity_set,
)
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat3.d3q19-velocity-set"
MODE = FailureMode.HARD_FAIL

# Anchor file for the finding; the velocity_set lives in the JSON, but the
# user-visible artifact is the algebraic derivation doc.
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

    for msg in verify_velocity_set(payload):
        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=ANCHOR_FILE,
            line=1,
            message=msg,
        ))
    return findings
