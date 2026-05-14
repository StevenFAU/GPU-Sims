"""Stack B public-API surface check via TS compiler API subprocess.

Spawns Node with the TS helper script in ts_helper/dist/ that uses the
TypeScript compiler API to enumerate exports from
common/common-web/src/index.ts and find references across the project.

Graceful degrade: no node on PATH → empty findings. Helper not built →
attempt build via npm install + tsc. Helper exits non-zero → empty
findings with stderr log.
"""

from __future__ import annotations

import json
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


HELPER_DIR = Path("tools/integrity/integrity/cat2_contracts/ts_helper")


@dataclass(frozen=True)
class PublicSymbolB:
    name: str
    kind: str
    file: str
    line: int
    reference_count: int


def is_node_available() -> bool:
    return shutil.which("node") is not None


def ensure_helper_built(repo_root: Path) -> bool:
    """Build the TS helper if not already built. Returns True on success."""
    helper_dir = repo_root / HELPER_DIR
    dist_js = helper_dir / "dist" / "extract_and_find.js"
    if dist_js.is_file():
        return True

    if not helper_dir.is_dir():
        return False

    if not (helper_dir / "node_modules").is_dir():
        result = subprocess.run(
            ["npm", "install", "--silent"],
            cwd=helper_dir,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            sys.stderr.write(
                "cat2.public-symbol-used-ts: npm install failed:\n"
                f"{result.stderr}\n"
            )
            return False

    result = subprocess.run(
        ["npx", "tsc", "--project", "tsconfig.json"],
        cwd=helper_dir,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        sys.stderr.write(
            "cat2.public-symbol-used-ts: helper build failed:\n"
            f"{result.stderr}\n"
        )
        return False

    return dist_js.is_file()


def run_extractor(repo_root: Path) -> list[PublicSymbolB]:
    """Spawn the Node helper, parse JSON, return symbol list."""
    if not is_node_available():
        return []

    # The TS helper lives in the integrity-toolkit installation tree; find
    # it via this module's path so test fixtures (which set repo_root to a
    # synthetic directory) can still reuse the real helper build artifact.
    # __file__: <repo_root>/tools/integrity/integrity/cat2_contracts/stack_b.py
    #   parents[4] = <repo_root>
    toolkit_repo_root = Path(__file__).resolve().parents[4]
    if not (toolkit_repo_root / HELPER_DIR).is_dir():
        toolkit_repo_root = repo_root

    if not ensure_helper_built(toolkit_repo_root):
        return []

    helper_js = toolkit_repo_root / HELPER_DIR / "dist" / "extract_and_find.js"
    if not helper_js.is_file():
        return []

    try:
        result = subprocess.run(
            ["node", str(helper_js), str(repo_root)],
            capture_output=True,
            text=True,
            timeout=180,
        )
    except subprocess.TimeoutExpired:
        sys.stderr.write("cat2.public-symbol-used-ts: helper timed out\n")
        return []

    if result.returncode != 0:
        sys.stderr.write(
            f"cat2.public-symbol-used-ts: helper exited {result.returncode}\n"
            f"stderr: {result.stderr}\n"
        )
        return []

    try:
        data = json.loads(result.stdout)
    except json.JSONDecodeError as e:
        sys.stderr.write(f"cat2.public-symbol-used-ts: malformed JSON: {e}\n")
        return []

    symbols: list[PublicSymbolB] = []
    for s in data.get("symbols", []):
        symbols.append(PublicSymbolB(
            name=s["name"],
            kind=s["kind"],
            file=s["file"],
            line=int(s["line"]),
            reference_count=len(s.get("references", [])),
        ))
    return symbols
