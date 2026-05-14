"""Citation grammar per spec § 6.2.

Parses two forms:
  1. Intra-repo: `<path>:<line>` or `<path>:<start>-<end>`
  2. Upstream:   `<UpstreamName> <version> <path>:<line>`

Commit 2 implements (1) and the parse-tree type for (2). Commit 3 wires
upstream parsing into checks.

Known false-positive classes (defended in tests, see test_cat1_intra_repo.py):
  - IPv4-like strings (192.168.1.1:80) — extension check excludes
  - Time-of-day (14:30) — extension check excludes
  - URL fragments (example.com/path:42) — resolution check excludes
  - Template tokens ({{path:line}}) — explicitly skipped

Known false-negative classes (NOT defended in v1):
  - Multi-line citations
# integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
  - Bracketed citations ([file.cpp:42])
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


# Recognized extensions. Anything else is not treated as a citation.
# Keep lowercase; matcher lowercases the extension before comparison.
RECOGNIZED_EXTENSIONS: frozenset[str] = frozenset({
    "cpp", "hpp", "h", "cc", "cxx", "c",
    "glsl", "wgsl", "comp", "frag", "vert", "mesh", "tese", "tesc",
    "ts", "tsx", "d.ts",
    "js", "mjs", "cjs", "jsx",
    "py", "pyi",
    "md", "rst",
    "toml", "yaml", "yml", "json",
    "cmake", "txt", "sh",
})


# Path: alphanumerics, _, ., /, -. No spaces. Must contain at least one
# literal dot before the colon (the extension); recognized-extension check
# below filters non-citation false positives. The single-class form (no
# nested quantifier) avoids catastrophic regex backtracking on lines that
# contain many slashes or dots.
# Line numbers: positive integers, optional range.
INTRA_REPO_RE = re.compile(
    r"(?P<path>[A-Za-z0-9_./-]+\.[A-Za-z0-9.]+)"
    r":(?P<start>\d+)(?:-(?P<end>\d+))?"
)


# Template tokens like {{path:line}} should NOT be treated as citations.
# We detect and skip them before INTRA_REPO_RE runs.
TEMPLATE_TOKEN_RE = re.compile(r"\{\{[^}]*\}\}")


@dataclass(frozen=True)
class IntraRepoCitation:
    """A parsed `<path>:<line>` or `<path>:<start>-<end>` citation."""
    path: str          # As written in the source (relative or repo-relative)
    start: int
    end: int | None    # None for single-line citations
    source_file: Path  # File where the citation appears
    source_line: int   # Line number in source_file
    raw: str           # Verbatim matched text


def _has_recognized_extension(path: str) -> bool:
    """Return True if `path` ends in a recognized file extension."""
    # Try multi-dot extensions first (e.g. .d.ts, .comp.glsl)
    for ext in RECOGNIZED_EXTENSIONS:
        if "." in ext and path.lower().endswith("." + ext):
            return True
    # Then single extensions.
    suffix = path.rsplit(".", 1)
    if len(suffix) == 2 and suffix[1].lower() in RECOGNIZED_EXTENSIONS:
        return True
    return False


def extract_intra_repo_citations(
    text: str,
    source_file: Path,
) -> list[IntraRepoCitation]:
    """Parse `text` line by line, yielding intra-repo citations.

    `source_file` is the path being scanned; embedded in each result for
    diagnostic purposes.
    """
    citations: list[IntraRepoCitation] = []
    for lineno, line in enumerate(text.splitlines(), start=1):
        # Skip lines whose only matches are inside template tokens.
        masked = TEMPLATE_TOKEN_RE.sub("", line)
        for m in INTRA_REPO_RE.finditer(masked):
            path = m.group("path")
            if not _has_recognized_extension(path):
                continue
            start = int(m.group("start"))
            end_raw = m.group("end")
            end = int(end_raw) if end_raw else None
            citations.append(IntraRepoCitation(
                path=path,
                start=start,
                end=end,
                source_file=source_file,
                source_line=lineno,
                raw=m.group(0),
            ))
    return citations


# Upstream citation grammar per spec § 6.2.
#
# Form: <UpstreamName> <version> <path>:<line>[-<end>]
#
# UpstreamName: capitalized word, alphanumerics. Distinguished from a
# regular sentence-starting word by the version token that follows.
# Version: `v1.2.3` / `1.2.3` / `HEAD` / a 7-40 char hex SHA.
#
# Known false-positive class: a capitalized sentence-starting word
# followed by a number could match (e.g. "Section 1.2.3 of the
# integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
# specification cited at TimeStep.cpp:42"). We mitigate by requiring
# the version token to be tight against the path (no comma, no period,
# no "of"/"in" between). The grammar is intentionally tight; ambiguous
# cases produce false negatives, not false positives.
UPSTREAM_RE = re.compile(
    r"(?P<upstream>[A-Z][A-Za-z0-9]+)\s+"
    r"(?P<version>v?\d+(?:\.\d+){1,3}(?:-[A-Za-z0-9]+)?|HEAD|[a-f0-9]{7,40})"
    r"\s+"
    r"(?P<path>[A-Za-z0-9_./-]+\.[A-Za-z0-9.]+)"
    r":(?P<start>\d+)(?:-(?P<end>\d+))?"
)


@dataclass(frozen=True)
class UpstreamCitation:
    """A parsed `<UpstreamName> <version> <path>:<line>` citation."""
    upstream: str       # As written, e.g. "SPlisHSPlasH"
    version: str        # As written, e.g. "2.16.1" or "HEAD" or a hex SHA
    path: str           # Path within the vendor tree
    start: int
    end: int | None
    source_file: Path
    source_line: int
    raw: str            # Verbatim matched text


def extract_upstream_citations(
    text: str,
    source_file: Path,
) -> list[UpstreamCitation]:
    """Parse `text` line by line, yielding upstream citations."""
    citations: list[UpstreamCitation] = []
    for lineno, line in enumerate(text.splitlines(), start=1):
        masked = TEMPLATE_TOKEN_RE.sub("", line)
        for m in UPSTREAM_RE.finditer(masked):
            path = m.group("path")
            if not _has_recognized_extension(path):
                continue
            start = int(m.group("start"))
            end_raw = m.group("end")
            end = int(end_raw) if end_raw else None
            citations.append(UpstreamCitation(
                upstream=m.group("upstream"),
                version=m.group("version"),
                path=path,
                start=start,
                end=end,
                source_file=source_file,
                source_line=lineno,
                raw=m.group(0),
            ))
    return citations
