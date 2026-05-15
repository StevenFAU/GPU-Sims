"""State-snapshot and grandfather-report emitters (v1.1 A.7, A.8).

Two entry points:

- `emit_state_snapshot(root, stdout)` -- A self-contained JSON document
  describing the toolkit's complete state at a single moment: commit SHA,
  timestamp, registered checks, registered upstream sources, full
  per-category suppression counts. Intended as the "verification
  provenance" anchor for spec drafts (v1.1 spec section D.1).
- `emit_grandfather_report(root, stdout, append_history=True)` -- Human-
  readable per-category table to stdout; optionally appends a JSON entry
  to `tools/integrity/.grandfather-history.json` (a time series).
"""

from __future__ import annotations

import datetime as dt
import json
import subprocess
from pathlib import Path
from typing import IO


HISTORY_FILE_RELATIVE = Path("tools/integrity/.grandfather-history.json")


_KNOWN_CATEGORIES = (
    "audit-citation",
    "live-shader-1810",
    "audit-doc-1810",
    "spec-grammar-example",
    "toolkit-own-source",
    "retro-grammar-example",
    "audit-report-grammar-example",
    "cat2-stack-d-unused",
    "cat2-stack-c-unused",
    "cat2-stack-b-unused",
    "cat2-stub-label-stale",
    "other-cat1",
)


def _extract_category(reason: str) -> str:
    """Match the suppression_reason text to a known category."""
    lowered = (reason or "").lower()
    for cat in _KNOWN_CATEGORIES:
        if cat in lowered:
            return cat
    return "other"


def _collect_state(root: Path) -> dict:
    """Run the toolkit in JSON warn-only mode and aggregate state."""
    result = subprocess.run(
        ["python3", "-m", "integrity",
         "--output", "json", "--no-audit-log", "--mode", "warn-only"],
        cwd=root,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode not in (0, 1):
        raise RuntimeError(
            f"integrity toolkit exited {result.returncode}: {result.stderr}"
        )
    data = json.loads(result.stdout)

    per_category: dict[str, int] = {}
    for f in data.get("findings", []):
        if not f.get("suppressed"):
            continue
        reason = f.get("suppression_reason", "")
        category = _extract_category(reason)
        per_category[category] = per_category.get(category, 0) + 1

    return {
        "schema_version": 1,
        "timestamp": dt.datetime.now(dt.timezone.utc).isoformat(),
        "commit": data.get("commit", "unknown"),
        "summary": data.get("summary", {}),
        "per_category": per_category,
    }


def _parse_ground_truth_sources(root: Path) -> list[dict]:
    """Parse `tools/integrity/docs/ground-truth-sources.md` for upstream
    anchor blocks. Permissive line-based parser; tolerates either inline
    or fenced TOML-shaped entries."""
    path = root / "tools" / "integrity" / "docs" / "ground-truth-sources.md"
    if not path.is_file():
        return []

    text = path.read_text(encoding="utf-8")
    blocks: list[dict] = []
    current: dict = {}
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("anchor_version"):
            _, _, val = stripped.partition("=")
            current["anchor_version"] = val.strip().strip('"').strip("'")
        elif stripped.startswith("anchor_sha"):
            _, _, val = stripped.partition("=")
            current["anchor_sha"] = val.strip().strip('"').strip("'")
        elif stripped.startswith("vendor_root"):
            _, _, val = stripped.partition("=")
            current["vendor_root"] = val.strip().strip('"').strip("'")
        elif stripped.startswith("name"):
            _, _, val = stripped.partition("=")
            current["name"] = val.strip().strip('"').strip("'")
        elif stripped == "":
            if current.get("anchor_version") and current.get("anchor_sha"):
                blocks.append(current)
            current = {}

    if current.get("anchor_version") and current.get("anchor_sha"):
        blocks.append(current)

    return blocks


def emit_state_snapshot(root: Path, out: IO[str]) -> None:
    """Emit a complete toolkit-state JSON document to `out`."""
    state = _collect_state(root)

    from integrity.cat1_citations.checks import REGISTERED_CHECKS as cat1
    from integrity.cat2_contracts.checks import REGISTERED_CHECKS as cat2
    from integrity.cat3_numerical.checks import REGISTERED_CHECKS as cat3
    state["registered_checks"] = {
        "cat1": [cid for cid, _ in cat1],
        "cat2": [cid for cid, _ in cat2],
        "cat3": [cid for cid, _ in cat3],
    }

    state["registered_upstreams"] = _parse_ground_truth_sources(root)

    json.dump(state, out, indent=2)
    out.write("\n")


def _append_history(root: Path, state: dict) -> None:
    """Append `state` (truncated to history-relevant fields) to the
    history file."""
    history_path = root / HISTORY_FILE_RELATIVE
    history_path.parent.mkdir(parents=True, exist_ok=True)

    if history_path.is_file():
        try:
            history = json.loads(history_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            history = []
    else:
        history = []

    history.append({
        "timestamp": state["timestamp"],
        "commit": state["commit"],
        "summary": state["summary"],
        "per_category": state["per_category"],
    })

    history_path.write_text(
        json.dumps(history, indent=2) + "\n",
        encoding="utf-8",
    )


def emit_grandfather_report(
    root: Path,
    out: IO[str],
    append_history: bool = True,
) -> None:
    """Emit a human-readable per-category table and optionally append
    to the history file."""
    state = _collect_state(root)

    out.write(f"grandfather report @ {state['commit']} ({state['timestamp']})\n")
    out.write(f"summary: {state['summary']}\n")
    out.write("per-category counts:\n")
    for cat, n in sorted(state["per_category"].items(), key=lambda kv: -kv[1]):
        out.write(f"  {cat:>35s}: {n}\n")

    if append_history:
        _append_history(root, state)
