"""Canonical exclusion paths per spec § 3.4.

Adding to this list is a Cat 1 fail unless the change carries
an `integrity-allow: cat1.exclusion-list` annotation.
"""

from __future__ import annotations

# Path patterns excluded from analysis by every check unless the check
# explicitly opts in (e.g., cat1.upstream-anchor reads references/.git/HEAD
# deliberately). Patterns are matched as glob-suffix substrings against
# repo-relative paths.
CANONICAL_EXCLUSIONS: tuple[str, ...] = (
    "node_modules/",
    "build/",
    "build-",            # matches build-test-alembic/ and other build-*/
    ".venv/",
    "__pycache__/",
    "references/",
    "_deps/",
    "dist/",
    ".git/",
    ".claude/",
    "captures/",
    "alembic_export/",
    "vdb_export/",
    "gallery/",
)


def is_excluded(path: str) -> bool:
    """Return True if `path` matches any canonical exclusion pattern."""
    normalized = path.replace("\\", "/")
    for pattern in CANONICAL_EXCLUSIONS:
        if pattern in normalized:
            return True
    return False
