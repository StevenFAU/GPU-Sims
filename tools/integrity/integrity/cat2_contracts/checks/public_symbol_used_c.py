"""Check: cat2.public-symbol-used-c — Stack C variant via libclang.

Mode: HARD_FAIL.

Catches the same defect classes as the Stack D variant:
  - Silent-data-loss fields (ParticleFrame::radii)
  - Defined-but-unexercised public functions (vdb::writeVec3Grid)

Implementation uses libclang for USR-based semantic reference matching,
which eliminates the name-collision false-positive class that Stack D's
AST-based matching has.

Requires build/compile_commands.json. If unavailable (no cmake configure
run), the check returns no findings (graceful degrade). CI workflow
runs cmake configure before invoking integrity.

Per-stack check IDs (cat2.public-symbol-used-c here vs Stack D's
cat2.public-symbol-used) keep failure messages distinct and let
grandfather suppressions target the right stack.
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat2_contracts.stack_c import (
    BUILD_COMPILE_COMMANDS,
    discover_consumer_sources,
    extract_public_surface,
    find_references,
)
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat2.public-symbol-used-c"
MODE = FailureMode.HARD_FAIL


def run(repo_root: Path) -> list[Finding]:
    findings: list[Finding] = []

    if not (repo_root / BUILD_COMPILE_COMMANDS).is_file():
        return findings

    try:
        public_symbols = extract_public_surface(repo_root)
    except RuntimeError:
        return findings

    if not public_symbols:
        return findings

    consumer_sources = discover_consumer_sources(repo_root)
    refs_by_usr = find_references(repo_root, public_symbols, consumer_sources)

    for symbol in public_symbols:
        sites = refs_by_usr.get(symbol.usr, [])
        if sites:
            continue

        try:
            rel = str(symbol.defining_file.relative_to(repo_root))
        except ValueError:
            rel = str(symbol.defining_file)

        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=rel,
            line=symbol.defining_line,
            message=(
                f"public {symbol.kind.value} '{symbol.qualified_name}' has "
                f"no non-self consumer site under common-cpp/src, examples, "
                f"or per-sim Stack C source"
            ),
        ))

    return findings
