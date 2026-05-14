"""Check: cat1.upstream-anchor — vendored reference HEAD matches documented anchor.

Mode: HARD_FAIL.

For each registered upstream, compares the on-disk
references/<UpstreamName>/.git/HEAD SHA against the anchor_sha in the
registry. This is the documented opt-out from the references/ exclusion
in spec § 3.4.
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.upstream_anchor import load_registry, vendor_head_sha
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat1.upstream-anchor"
MODE = FailureMode.HARD_FAIL


def run(repo_root: Path) -> list[Finding]:
    registry = load_registry(repo_root)
    findings: list[Finding] = []

    for name, reg in registry.items():
        if CHECK_ID not in reg.used_by_checks:
            # Registry entry hasn't opted in to anchor verification.
            continue
        head = vendor_head_sha(repo_root, reg.vendor_root)
        if head is None:
            # Vendor tree not present locally. CI clones it explicitly
            # (per spec § 9.1); locally it may simply be absent. Report
            # as a soft skip via a HARD_FAIL with diagnostic — CI will
            # always have it; local devs should clone or accept the
            # diagnostic.
            findings.append(Finding(
                check_id=CHECK_ID,
                mode=MODE,
                file=str(reg.anchor_doc),
                line=1,
                message=(
                    f"{name}: vendor tree at {reg.vendor_root} is not present "
                    f"or not a git repo; cannot verify anchor "
                    f"(expected SHA {reg.anchor_sha})"
                ),
            ))
            continue

        if head != reg.anchor_sha:
            findings.append(Finding(
                check_id=CHECK_ID,
                mode=MODE,
                file=str(reg.anchor_doc),
                line=1,
                message=(
                    f"{name}: vendor HEAD {head} does not match documented "
                    f"anchor {reg.anchor_sha} (version {reg.anchor_version})"
                ),
            ))

    return findings
