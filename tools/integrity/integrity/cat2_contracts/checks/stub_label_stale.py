"""Check: cat2.stub-label-stale -- flag stale "Phase N stub" labels.

Mode: HARD_FAIL.

Closes spec section 12 row 5 (alembic_writer.hpp canonical case).
Anchors on the exact phrasing `In Phase <N>, this is a stub:` present
in both confirmed stale cases per probe v1_1_apispec section G. If the
corresponding implementation file has more than 10 non-comment LOC,
the "stub" label contradicts the implementation and is flagged.

Detection scope (batch-1-spec Decision 3):
  - C++ headers under common/common-cpp/include/**/*.{hpp,h}
  - Python modules under common/common-py/gpusims_common/**/*.py

Sibling-impl resolution (batch-1-spec Decision 2):
  - `.hpp`/`.h` in `common-cpp/include/<sub>/<base>.hpp` ->
    `common-cpp/src/<sub>/<base>.cpp` (relative path mirror)
  - `.py`: impl is the same file

False-positive guard for Stack D:
  Skip Stack D files whose top 40 lines contain `permanent stub` or
  `real-or-stub` -- both intentional discriminator phrasings per
  probe section D.2. Anchored on top-of-file because these phrasings
  appear in module docstrings.
"""

from __future__ import annotations

import re
from pathlib import Path

from integrity.common.exclusions import is_excluded
from integrity.common.repo import list_tracked_files
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat2.stub-label-stale"
MODE = FailureMode.HARD_FAIL


# Exact phrasing observed in both confirmed stale cases per probe section G.
STALE_LABEL_RE = re.compile(r"\bIn Phase \d+, this is a stub:")

# Top-of-file discriminator phrasings that override the staleness signal.
DISCRIMINATOR_PHRASES = ("permanent stub", "real-or-stub")
DISCRIMINATOR_SCAN_LINES = 40

# Implementation must have more than this many non-comment LOC to count
# as a real (non-stub) impl. Mirrors probe section D.2 heuristic.
IMPL_LOC_THRESHOLD = 10

# Lines counted as "comment-or-blank" for the impl-LOC heuristic.
_COMMENT_OR_BLANK_RE = re.compile(r"^\s*(//|#|/\*|\*|$)")


CPP_INCLUDE_ROOT = Path("common/common-cpp/include")
CPP_SRC_ROOT = Path("common/common-cpp/src")
PY_PACKAGE_ROOT = Path("common/common-py/gpusims_common")


def _list_scannable_files(repo_root: Path) -> list[Path]:
    """Return scannable files under the C++ header tree + Stack D package."""
    if (repo_root / ".git").exists():
        all_files = list_tracked_files(repo_root)
    else:
        all_files = [p for p in repo_root.rglob("*") if p.is_file()]

    out: list[Path] = []
    for absolute in all_files:
        try:
            rel = absolute.relative_to(repo_root)
        except ValueError:
            continue
        if is_excluded(str(rel)):
            continue

        ext = absolute.suffix
        rel_str = str(rel).replace("\\", "/")

        in_cpp_include = (
            rel_str.startswith(str(CPP_INCLUDE_ROOT) + "/")
            and ext in (".hpp", ".h")
        )
        in_py_package = (
            rel_str.startswith(str(PY_PACKAGE_ROOT) + "/")
            and ext == ".py"
        )

        if in_cpp_include or in_py_package:
            out.append(absolute)

    return out


def _resolve_impl_path(header_path: Path, repo_root: Path) -> Path | None:
    """Resolve the impl file for a given header/module.

    Convention (verified against synced common-cpp layout 2026-05-15):
      include/<namespace>/<rest>.hpp  ->  src/<rest>.cpp
    The first directory component after include/ is the project
    namespace (e.g. `gpusims/`) and is stripped -- the src/ tree does
    not repeat the namespace path.

    For Python files, impl is the same file (Python does not separate
    declaration from implementation)."""
    try:
        rel = header_path.relative_to(repo_root)
    except ValueError:
        return None

    rel_str = str(rel).replace("\\", "/")

    if (
        rel_str.startswith(str(CPP_INCLUDE_ROOT) + "/")
        and header_path.suffix in (".hpp", ".h")
    ):
        relative_to_include = header_path.relative_to(repo_root / CPP_INCLUDE_ROOT)
        parts = relative_to_include.parts
        if len(parts) < 2:
            return None
        namespace_stripped = Path(*parts[1:])
        impl_relative = namespace_stripped.with_suffix(".cpp")
        return repo_root / CPP_SRC_ROOT / impl_relative

    if rel_str.startswith(str(PY_PACKAGE_ROOT) + "/") and header_path.suffix == ".py":
        return header_path

    return None


def _count_non_comment_loc(text: str) -> int:
    """Count lines that are NOT pure comments or blank."""
    count = 0
    for line in text.splitlines():
        if _COMMENT_OR_BLANK_RE.match(line):
            continue
        count += 1
    return count


def _has_discriminator(text: str) -> bool:
    """Check if top-of-file text contains an intentional-stub discriminator."""
    head = "\n".join(text.splitlines()[:DISCRIMINATOR_SCAN_LINES]).lower()
    return any(phrase in head for phrase in DISCRIMINATOR_PHRASES)


def run(repo_root: Path) -> list[Finding]:
    findings: list[Finding] = []

    for header in _list_scannable_files(repo_root):
        try:
            text = header.read_text(encoding="utf-8")
        except OSError:
            continue

        if header.suffix == ".py" and _has_discriminator(text):
            continue

        for lineno, line in enumerate(text.splitlines(), start=1):
            if not STALE_LABEL_RE.search(line):
                continue

            impl_path = _resolve_impl_path(header, repo_root)
            if impl_path is None or not impl_path.is_file():
                continue

            try:
                impl_text = impl_path.read_text(encoding="utf-8")
            except OSError:
                continue

            impl_loc = _count_non_comment_loc(impl_text)
            if impl_loc <= IMPL_LOC_THRESHOLD:
                continue

            try:
                header_rel = str(header.relative_to(repo_root))
                impl_rel = str(impl_path.relative_to(repo_root))
            except ValueError:
                header_rel = str(header)
                impl_rel = str(impl_path)

            findings.append(Finding(
                check_id=CHECK_ID,
                mode=MODE,
                file=header_rel,
                line=lineno,
                message=(
                    f"\"In Phase N stub\" label is stale: implementation "
                    f"at {impl_rel} has {impl_loc} non-comment LOC "
                    f"(threshold {IMPL_LOC_THRESHOLD})"
                ),
            ))

    return findings
