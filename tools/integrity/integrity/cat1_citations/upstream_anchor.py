"""Ground-truth source registry loader + anchor SHA verification.

Per spec Appendix A. Reads tools/integrity/docs/ground-truth-sources.md,
extracts the fenced TOML block, returns a registry mapping. Provides
helpers for resolving cited paths under vendor roots and verifying the
on-disk vendor HEAD matches the documented anchor.
"""

from __future__ import annotations

import re
import subprocess
import tomllib
from dataclasses import dataclass
from pathlib import Path


REGISTRY_DOC = Path("tools/integrity/docs/ground-truth-sources.md")


# Match a fenced toml block. Non-greedy.
TOML_FENCE_RE = re.compile(
    r"^```toml\s*\n(?P<body>.*?)^```",
    re.MULTILINE | re.DOTALL,
)


@dataclass(frozen=True)
class UpstreamRegistration:
    name: str             # e.g. "SPlisHSPlasH"
    anchor_version: str   # e.g. "2.16.1"
    anchor_sha: str       # 40-char hex
    vendor_root: Path     # Relative to repo root
    anchor_doc: Path      # Relative to repo root (file documenting the anchor)
    upstream_url: str
    used_by_checks: tuple[str, ...]


def load_registry(repo_root: Path) -> dict[str, UpstreamRegistration]:
    """Parse the ground-truth-sources.md TOML block and return the registry.

    Returns an empty dict if the file is absent. Raises RuntimeError if
    the file exists but contains no parseable TOML block.
    """
    doc = repo_root / REGISTRY_DOC
    if not doc.is_file():
        return {}

    text = doc.read_text(encoding="utf-8")
    match = TOML_FENCE_RE.search(text)
    if match is None:
        raise RuntimeError(
            f"{REGISTRY_DOC}: no fenced toml block found; cannot parse registry"
        )
    body = match.group("body")
    parsed = tomllib.loads(body)

    registrations: dict[str, UpstreamRegistration] = {}
    for name, entry in parsed.items():
        if "vendor_root" not in entry:
            # Algebraic-source registration (no vendored upstream); not subject
            # to anchor-SHA verification. Consumed by cat3 numerical checks via
            # their own loader paths, not by this dataclass.
            continue
        registrations[name] = UpstreamRegistration(
            name=name,
            anchor_version=str(entry["anchor_version"]),
            anchor_sha=str(entry["anchor_sha"]),
            vendor_root=Path(entry["vendor_root"]),
            anchor_doc=Path(entry["anchor_doc"]),
            upstream_url=str(entry["upstream_url"]),
            used_by_checks=tuple(entry["used_by_checks"]),
        )
    return registrations


def vendor_head_sha(repo_root: Path, vendor_root: Path) -> str | None:
    """Return the HEAD SHA of the git clone at `vendor_root`, or None if
    the directory is absent or not a git repo."""
    abs_root = (repo_root / vendor_root).resolve()
    if not (abs_root / ".git").exists():
        return None
    try:
        result = subprocess.run(
            ["git", "-C", str(abs_root), "rev-parse", "HEAD"],
            capture_output=True,
            text=True,
            check=True,
            timeout=10,
        )
        return result.stdout.strip()
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, OSError):
        return None
