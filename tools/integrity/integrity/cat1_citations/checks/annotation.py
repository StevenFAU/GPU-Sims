# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
"""Check: cat1.annotation-form — every integrity-allow: annotation is grammar-valid.

Mode: HARD_FAIL.

Validates the form per spec § 3.2:
  - check_id matches `cat<N>.<name>` or `cat<N>.*`
  - reason >= 8 chars
  - issue_ref is `#<digits>` or literal `n/a`
  - blanket `*` is rejected
"""

from __future__ import annotations

import re
from pathlib import Path

from integrity.common.annotations import (
    fence_state_per_line,
    is_markdown_path,
)
from integrity.common.exclusions import is_excluded
from integrity.common.repo import list_tracked_files
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat1.annotation-form"
MODE = FailureMode.HARD_FAIL


# Scan extensions for files that can carry annotations.
SCAN_EXTENSIONS: frozenset[str] = frozenset({
    ".cpp", ".hpp", ".h", ".cc", ".cxx", ".c",
    ".glsl", ".wgsl",
    ".ts", ".tsx", ".d.ts",
    ".js", ".mjs", ".cjs", ".jsx",
    ".py", ".pyi",
    ".md",
})


# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
# Loose pattern that finds *any* `integrity-allow:` invocation (valid or
# not) so we can report grammar failures rather than silently skipping.
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
LOOSE_RE = re.compile(r"integrity-allow:\s*(.*?)(?:-->|$)", re.IGNORECASE)


# Strict pattern matching the grammar in spec § 3.2.
STRICT_RE = re.compile(
    r"^\s*(?P<check_id>cat\d+\.(?:[a-z][a-z0-9-]*|\*))\s*;\s*"
    r"(?P<reason>.+?)\s*;\s*"
    r"(?P<issue_ref>#\d+|n/a)\s*$"
)


def _has_scan_extension(path: Path) -> bool:
    name = path.name.lower()
    for ext in SCAN_EXTENSIONS:
        if name.endswith(ext):
            return True
    return False


def _list_scannable_files(root: Path) -> list[Path]:
    """List files to scan. Uses git ls-files if root is a git repo,
    otherwise walks the directory directly (for test fixtures)."""
    if (root / ".git").exists():
        return list_tracked_files(root)
    files: list[Path] = []
    for path in root.rglob("*"):
        if path.is_file():
            files.append(path)
    return files


def _validate(body: str) -> str | None:
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    """Return None if `body` (the text after `integrity-allow:`) is valid,
    else a diagnostic string."""
    body = body.strip().rstrip("-->").strip()

    m = STRICT_RE.match(body)
    if not m:
        return f"grammar mismatch in '{body}'"

    check_id = m.group("check_id")
    reason = m.group("reason").strip()

    if check_id == "*":
        return "blanket '*' check-id is not allowed"
    if len(reason) < 8:
        return f"reason must be >= 8 chars (got {len(reason)}: '{reason}')"

    return None


def run(repo_root: Path) -> list[Finding]:
    """Scan all tracked files; report any malformed annotations."""
    findings: list[Finding] = []

    for absolute in _list_scannable_files(repo_root):
        rel = str(absolute.relative_to(repo_root))
        if is_excluded(rel):
            continue
        if not _has_scan_extension(absolute):
            continue

        try:
            text = absolute.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue

        lines_list = text.splitlines()
        if is_markdown_path(rel):
            fence_state = fence_state_per_line(lines_list)
        else:
            fence_state = [False] * len(lines_list)

        for lineno, line in enumerate(lines_list, start=1):
            if fence_state[lineno - 1]:
                continue
            for m in LOOSE_RE.finditer(line):
                body = m.group(1)
                problem = _validate(body)
                if problem is not None:
                    findings.append(Finding(
                        check_id=CHECK_ID,
                        mode=MODE,
                        file=rel,
                        line=lineno,
                        message=problem,
                    ))

    return findings
