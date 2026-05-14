"""Check: cat2.public-symbol-used-ts (Stack B variant).

Mode: HARD_FAIL.

Same defect classes as Stack D and Stack C variants. Uses the TypeScript
compiler API via a Node subprocess (TS helper at
tools/integrity/integrity/cat2_contracts/ts_helper/).

Graceful degrade when Node or the TS helper is unavailable.
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat2_contracts.stack_b import run_extractor
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat2.public-symbol-used-ts"
MODE = FailureMode.HARD_FAIL


def run(repo_root: Path) -> list[Finding]:
    findings: list[Finding] = []

    symbols = run_extractor(repo_root)
    if not symbols:
        return findings

    for symbol in symbols:
        if symbol.reference_count > 0:
            continue
        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=symbol.file,
            line=symbol.line,
            message=(
                f"public {symbol.kind} '{symbol.name}' has no non-self consumer "
                f"site under Stack B sim sources, examples, or tests"
            ),
        ))

    return findings
