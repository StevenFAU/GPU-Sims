---
title: "Integrity Toolkit v1.1 — Batch 1 Execution Spec"
date: 2026-05-15
author: architect1
status: draft
audience: Claude Code (executor)
sibling-docs:
  - docs/integrity-toolkit-spec.md
  - docs/retro/integrity-toolkit-v1.md
  - docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_1_apispec_2026-05-15_architect1.md
---

# Integrity Toolkit v1.1 — Batch 1 Execution Spec

## 0. Execution preamble (read this first)

You are Claude Code executing batch 1 of integrity-toolkit v1.1 work per
the v1.1 spec draft. Land three commits in order, each independently
verifiable. Materialize every file creation and edit specified.

**Hard rules:**

1. **Execute every file creation and modification specified. Do not skip any.**
2. **The synced repo state is authoritative over this spec.** If any
   verbatim claim about file contents conflicts with what's actually
   on disk at `main`'s HEAD, **pause and surface the conflict; do not
   silently adapt.** All API claims in this spec were grep-verified
   against `447ebf0` during drafting per `apispec_2026-05-15` probe.
   If HEAD has moved, re-anchor before applying.
3. **Land commits in the order given.** Each commit's verification
   block must pass before starting the next. Do not interleave.
4. **One audit report per commit.** Front-matter and structure mirror
   the v1 build-N landing reports. File location:
   `docs/diagnostics/_audits/integrity_v1_1_commit<N>_landing_2026-05-15.md`.
5. **SHA back-fill is a separate follow-up commit, never `--amend`.**
   Per Convention #12.
6. **The toolkit must remain green across every commit.** After each
   commit, `python3 -m integrity --mode strict --no-audit-log` must
   exit 0 against the real repo. New findings introduced by a commit
   must be either fixed in the same commit or grandfathered with an
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
   `integrity-allow:` annotation explaining why.
7. **`python3`, not `python`.** The host has no `python` shim
   (probe § C.1). Commit 3 standardizes user-facing docs on `python3`;
   in the meantime, every command you run uses `python3`.

**File-path conventions:**

- New check modules: `tools/integrity/integrity/cat<N>_<name>/checks/<id>.py`
- New tests: `tools/integrity/tests/test_<area>.py`
- New fixtures: `tools/integrity/tests/fixtures/<good|bad>_<area>/`
- Grandfather sweep regeneration: `tools/integrity/scripts/grandfather_sweep.py`
  is the entry point; do not invoke `integrity.grandfather.apply_annotations()`
  directly.

## 1. Goals & load-bearing decisions

### 1.1 Goals

Three commits land five v1.1 items from the v1.1 spec draft:

1. **A.1** — `cat2.stub-label-stale` (commit 1). Closes the only spec § 12
   canonical defect lacking v1 detection. Catches two confirmed stale cases
   (`alembic_writer.hpp`, `vdb_writer.hpp`) plus any future occurrence of
   the canonical "Phase N stub" phrasing.
2. **A.5** — markdown fenced-block awareness in the annotation parser and
   suppressor (commit 2). Shrinks the `spec-grammar-example`,
   `retro-grammar-example`, and `audit-report-grammar-example` grandfather
   categories (~40 entries combined).
3. **A.7** — grandfather drain instrumentation: `--grandfather-report` and
   `--state-snapshot` CLI flags (commit 3). Persists timestamped per-category
   counts to `tools/integrity/.grandfather-history.json`.
4. **A.8** — per-category live tallies populated into
   `tools/integrity/docs/grandfather-catalog.md` headings (commit 3).
5. **5.B** — standardize user-facing docs on `python3 -m integrity`
   (commit 3).

**Deferred from this batch** (revisit after batch-1 lands):

- A.2 (self-application of Cat 2 to toolkit source) — depends on whether
  `tools/integrity/integrity/__init__.py` declares a meaningful public
  surface. Architect-2 review item: re-export canonical surface
  explicitly first, then schedule A.2.
- A.3 (bare-path-to-upstream-basename), A.4 (multi-line citation grammar),
  A.6 (Stack C runtime optimization), A.9 (audit-citation file-pattern
  exclusion) — separate batches per v1.1 spec § 3.

### 1.2 Load-bearing decisions

The following ten decisions are locked. Conflicts with synced source
require pause-and-surface, not silent adaptation.

**Decision 1 — A.1 detection anchors on the exact phrase `In Phase \d+, this is a stub:`.**
This is the phrasing actually present in both stale cases
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
(`alembic_writer.hpp:11` and `vdb_writer.hpp:12`, probe § G.1, G.2).
Anchoring on this specific phrase rather than `\bstub\b` avoids firing
on intentional `permanent stub for Phase 9` (Stack D, intentional) and
`real-or-stub` (Stack D discriminator pattern) framings (probe § D.2).

**Decision 2 — A.1 staleness heuristic = "impl file has > 10 non-comment LOC".**
Mirrors the v1.1 probe § D.2 heuristic that confirmed both canonical cases.
"Non-comment LOC" counted via `rg -c -v '^\s*(//|#|/\*|\*|$)'` equivalent.
If the impl file doesn't exist, the label is NOT stale (real stub).

**Decision 3 — A.1 covers C++ headers + .py files.**
Scope: `common/common-cpp/include/**/*.{hpp,h}` and
`common/common-py/gpusims_common/**/*.py`. Stack B (`.ts`) is excluded:
TypeScript "stub" usage is rare and probe § D.1 found zero hits.

**Decision 4 — A.5 fence logic is refactored from `grandfather.py` into `common/annotations.py`.**
Probe § D.3 confirmed fence awareness currently lives only in
`grandfather.py` (Section B.1 of apispec) — applied at annotation-emit
time but NOT at suppression-parse time or annotation-form-check time.
Moving the constant + function to `common/annotations.py` makes it
importable by `common/suppression.py` and
`cat1_citations/checks/annotation.py`. `grandfather.py` re-imports
to preserve API.

**Decision 5 — A.5 fence awareness applies to `.md` and `.rst` files only.**
Other file types (`.py`, `.cpp`, `.hpp`, etc.) don't have markdown fences.
The `grandfather.py:render_annotation_line` path already gates on
`file_path.lower().endswith((".md", ".rst"))`; the new parser/suppressor
gates use the same predicate.

**Decision 6 — A.5 fence-internal annotations are ignored entirely.**
A line inside a fenced code block is neither a candidate for
`cat1.annotation-form` grammar reporting NOR a valid suppression
annotation for a finding on the line immediately following it. This
matches the intent of fences as illustrative content.

**Decision 7 — A.7 `--grandfather-report` is human-readable + appends to a history file.**
Default behavior: emit a human-readable per-category table to stdout
AND append a JSON entry to `tools/integrity/.grandfather-history.json`.
Suppress the append with `--no-history-append` for one-shot reads.

**Decision 8 — A.7 `--state-snapshot` emits a self-contained JSON document to stdout.**
Includes: commit SHA, timestamp, registered checks, registered upstream
sources, full per-category suppression counts, registered ground-truth
sources. Intended for D.1 (spec-draft verification provenance) — replaces
"probe-from-scratch-each-time" in architect chats. Does NOT touch
the history file.

**Decision 9 — A.8 populates per-category counts via parenthetical suffix on category headings.**
Format: `` ### `<category-name>` (<count>) `` — e.g. `` ### `audit-citation` (761) ``.
Numbers are populated from the current run; the headings now carry data
but updates are still manual on subsequent commits (auto-update is a
v1.2 candidate). Approximate-prose ranges inside category bodies (`~10
entries`, `~108 entries`) are removed.

**Decision 10 — 5.B docs sweep touches three files only.**
Per probe § J.3: `tools/integrity/README.md` (6 sites),
`docs/integrity-toolkit-spec.md` (6 sites), and
`tools/integrity/integrity/__main__.py:1` docstring (1 site). Audit
reports under `docs/diagnostics/_audits/` are append-only and stay
unchanged — this is the same convention that drives the `audit-citation`
grandfather category.

## 2. Commit plan

| Commit | Items | Files changed | Estimated diff size |
|---|---|---|---|
| 1 | A.1 (`cat2.stub-label-stale`) | 4 new (check + 2 fixture dirs + test) + 4 modified (check registry, classifier, catalog, exclusions if needed) | ~400 LOC new, ~30 LOC modified |
| 2 | A.5 (fence awareness in parser + suppressor) | 2 modified (`common/annotations.py`, `common/suppression.py`, `cat1_citations/checks/annotation.py`, `grandfather.py`) + 2 new tests + 1-2 new fixtures | ~150 LOC new, ~60 LOC modified |
| 3 | A.7 + A.8 + 5.B | `runner.py` modified, `grandfather-catalog.md` regenerated, `README.md` + spec + `__main__.py` swept | ~200 LOC new (state-snapshot machinery), ~100 LOC modified (catalog), ~13 sites swept |

Each commit ships with a single audit report under
`docs/diagnostics/_audits/integrity_v1_1_commit<N>_landing_2026-05-15.md`.

## 3. Commit 1 — A.1 `cat2.stub-label-stale`

### 3.1 New file: `tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py`

```python
# tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py
"""Check: cat2.stub-label-stale — flag "Phase N stub" labels on real impls.

Mode: HARD_FAIL.

Closes spec § 12 row 5 (`alembic_writer.hpp` canonical case). Anchors on
the exact phrasing `In Phase <N>, this is a stub:` present in both
confirmed stale cases per probe v1_1_apispec § G. If the corresponding
implementation file has more than 10 non-comment LOC, the "stub" label
contradicts the implementation and is flagged.

Detection scope (Decision 3):
  - C++ headers under common/common-cpp/include/**/*.{hpp,h}
  - Python modules under common/common-py/gpusims_common/**/*.py

Sibling-impl resolution (Decision 2):
  - `.hpp`/`.h` in `common-cpp/include/<sub>/<base>.hpp` →
    `common-cpp/src/<sub>/<base>.cpp` (relative path mirror)
  - `.py`: impl is the same file

False-positive guard for Stack D:
  Skip Stack D files whose top 40 lines contain `permanent stub` or
  `real-or-stub` — both intentional discriminator phrasings per
  probe § D.2. Anchored on top-of-file because these phrasings appear
  in module docstrings.
"""

from __future__ import annotations

import re
from pathlib import Path

from integrity.common.exclusions import is_excluded
from integrity.common.repo import list_tracked_files
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat2.stub-label-stale"
MODE = FailureMode.HARD_FAIL


# Exact phrasing observed in both confirmed stale cases per probe § G.
STALE_LABEL_RE = re.compile(r"\bIn Phase \d+, this is a stub:")

# Top-of-file discriminator phrasings that override the staleness signal
# for Stack D files.
DISCRIMINATOR_PHRASES = ("permanent stub", "real-or-stub")
DISCRIMINATOR_SCAN_LINES = 40

# Implementation must have more than this many non-comment LOC to count
# as a real (non-stub) impl. Mirrors probe § D.2 heuristic.
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

        in_cpp_include = rel_str.startswith(str(CPP_INCLUDE_ROOT) + "/") and ext in (".hpp", ".h")
        in_py_package = rel_str.startswith(str(PY_PACKAGE_ROOT) + "/") and ext == ".py"

        if in_cpp_include or in_py_package:
            out.append(absolute)

    return out


def _resolve_impl_path(header_path: Path, repo_root: Path) -> Path | None:
    """Resolve the impl file for a given header/module per Decision 2."""
    try:
        rel = header_path.relative_to(repo_root)
    except ValueError:
        return None

    rel_str = str(rel).replace("\\", "/")

    if rel_str.startswith(str(CPP_INCLUDE_ROOT) + "/") and header_path.suffix in (".hpp", ".h"):
        # `include/<sub>/<base>.hpp` → `src/<sub>/<base>.cpp`
        relative_to_include = header_path.relative_to(repo_root / CPP_INCLUDE_ROOT)
        impl_relative = relative_to_include.with_suffix(".cpp")
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

        # Skip Stack D files with intentional-stub framing.
        if header.suffix == ".py" and _has_discriminator(text):
            continue

        # Find stale-label hits.
        for lineno, line in enumerate(text.splitlines(), start=1):
            if not STALE_LABEL_RE.search(line):
                continue

            impl_path = _resolve_impl_path(header, repo_root)
            if impl_path is None or not impl_path.is_file():
                # No impl file → real stub, not stale.
                continue

            try:
                impl_text = impl_path.read_text(encoding="utf-8")
            except OSError:
                continue

            impl_loc = _count_non_comment_loc(impl_text)
            if impl_loc <= IMPL_LOC_THRESHOLD:
                # Impl is itself a stub.
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
```

**Verbatim claims to confirm against synced source:**

- `tools/integrity/integrity/common/exclusions.py` exports `is_excluded` (apispec § E.2).
- `tools/integrity/integrity/common/repo.py` exports `list_tracked_files` (apispec § A.1 references this import).
- `tools/integrity/integrity/common/results.py` exports `FailureMode` and `Finding` (apispec § A.1 references both).

### 3.2 Modified file: `tools/integrity/integrity/cat2_contracts/checks/__init__.py`

Add `stub_label_stale` to the imports and `REGISTERED_CHECKS`. Final content:

```python
# tools/integrity/integrity/cat2_contracts/checks/__init__.py
"""Cat 2 check modules. Discovered by integrity.runner.discover_checks."""

from integrity.cat2_contracts.checks import (
    public_symbol_used,
    public_symbol_used_b,
    public_symbol_used_c,
    stub_label_stale,
)

REGISTERED_CHECKS = [
    (public_symbol_used.CHECK_ID, public_symbol_used),
    (public_symbol_used_c.CHECK_ID, public_symbol_used_c),
    (public_symbol_used_b.CHECK_ID, public_symbol_used_b),
    (stub_label_stale.CHECK_ID, stub_label_stale),
]
```

### 3.3 New fixtures

#### `tools/integrity/tests/fixtures/bad_stub_label/include/widget/widget.hpp`

```cpp
// tools/integrity/tests/fixtures/bad_stub_label/include/widget/widget.hpp
#pragma once

namespace widget {

// Widget operations.
//
// In Phase 1, this is a stub: if WIDGET_REAL is not defined at compile
// time, all functions log a warning and return false.

struct Frame {
    int count = 0;
};

bool write_frame(const Frame& f);

}  // namespace widget
```

#### `tools/integrity/tests/fixtures/bad_stub_label/src/widget.cpp`

```cpp
// tools/integrity/tests/fixtures/bad_stub_label/src/widget.cpp
#include "widget/widget.hpp"

#include <iostream>

namespace widget {

namespace {

bool write_frame_real(const Frame& f) {
    std::cout << "writing " << f.count << " entries\n";
    return true;
}

bool write_frame_stub(const Frame& f) {
    (void)f;
    return false;
}

}  // namespace

bool write_frame(const Frame& f) {
#ifdef WIDGET_REAL
    return write_frame_real(f);
#else
    return write_frame_stub(f);
#endif
}

}  // namespace widget
```

(Non-comment LOC: 14, well above the 10-line threshold.)

#### `tools/integrity/tests/fixtures/good_stub_label/include/widget/widget.hpp`

```cpp
// tools/integrity/tests/fixtures/good_stub_label/include/widget/widget.hpp
#pragma once

namespace widget {

// Widget operations.
//
// In Phase 1, this is a stub: if WIDGET_REAL is not defined at compile
// time, all functions log a warning and return false.

bool write_frame(int count);

}  // namespace widget
```

#### `tools/integrity/tests/fixtures/good_stub_label/src/widget.cpp`

```cpp
// tools/integrity/tests/fixtures/good_stub_label/src/widget.cpp
#include "widget/widget.hpp"

namespace widget {

bool write_frame(int count) {
    (void)count;
    return false;
}

}  // namespace widget
```

(Non-comment LOC: 5, under threshold — real stub, no fire.)

#### `tools/integrity/tests/fixtures/bad_stub_label/py_package/gpusims_common/widget.py`

```python
# tools/integrity/tests/fixtures/bad_stub_label/py_package/gpusims_common/widget.py
"""Widget module.

In Phase 1, this is a stub: callers should expect partial functionality.
"""


def make_widget(count: int) -> dict:
    out = {}
    out["count"] = count
    out["positions"] = [0.0] * (3 * count)
    out["velocities"] = [0.0] * (3 * count)
    out["radii"] = [1.0] * count
    out["ids"] = list(range(count))
    out["timestamps"] = [0.0] * count
    return out


def consume_widget(w: dict) -> int:
    return w["count"] * 2 + len(w["positions"])
```

(Non-comment LOC: 12. Note: this fixture lives under `py_package/` so it
doesn't interfere with the canonical `common/common-py/...` scan path
when the check runs with `repo_root=fixture_dir`. The fixture's check
invocation will pass `fixture_dir / "py_package"` as a root override —
see the test in § 3.5.)

#### `tools/integrity/tests/fixtures/good_stub_label/py_package/gpusims_common/permanent.py`

```python
# tools/integrity/tests/fixtures/good_stub_label/py_package/gpusims_common/permanent.py
"""Widget module.

This is a permanent stub for Phase 9. Real implementation is banked.

In Phase 1, this is a stub: this exact phrasing should NOT fire because
the file is marked as a permanent stub above.
"""


def make_widget(count: int) -> dict:
    """Permanent stub — returns empty dict."""
    return {"count": count}
```

(Tests the discriminator-phrase exclusion. Has the "Phase N stub"
phrasing but also "permanent stub" — should not fire.)

### 3.4 Modified file: `tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py` fixture-root mode

The check needs to support fixture testing. The base implementation in
§ 3.1 already uses hardcoded `CPP_INCLUDE_ROOT`, `CPP_SRC_ROOT`, and
`PY_PACKAGE_ROOT` constants relative to the repo root passed in. For
fixture testing, the test passes `fixture_dir` as the repo root and
expects the same constants to apply relative to that root. Tests verify
this by placing fixture files at paths mirroring the production layout:
`fixture_dir/common/common-cpp/include/...` etc.

**No code change needed to § 3.1.** Fixtures must mirror the production
layout. Reorganize the fixture tree to:

```
tools/integrity/tests/fixtures/bad_stub_label/
  common/common-cpp/include/widget/widget.hpp
  common/common-cpp/src/widget.cpp
  common/common-py/gpusims_common/widget.py

tools/integrity/tests/fixtures/good_stub_label/
  common/common-cpp/include/widget/widget.hpp
  common/common-cpp/src/widget.cpp
  common/common-py/gpusims_common/permanent.py
```

(Adjust the fixture file paths in § 3.3 accordingly. The fixture file
*contents* are unchanged; only the *paths* mirror production.)

### 3.5 New test file: `tools/integrity/tests/test_cat2_stub_label_stale.py`

```python
# tools/integrity/tests/test_cat2_stub_label_stale.py
"""Tests for cat2.stub-label-stale."""

from __future__ import annotations

from pathlib import Path

import pytest

from integrity.cat2_contracts.checks.stub_label_stale import run


def test_bad_cpp_header_with_real_impl_flags(fixtures_dir: Path) -> None:
    """Stale stub label + sibling .cpp with >10 LOC → flag the header."""
    findings = run(fixtures_dir / "bad_stub_label")
    headers = [f for f in findings if f.file.endswith("widget.hpp")]
    assert len(headers) == 1, f"unexpected: {[(f.file, f.message) for f in findings]}"
    assert "stale" in headers[0].message.lower()


def test_good_cpp_header_with_stub_impl_does_not_flag(fixtures_dir: Path) -> None:
    """Stub label + sibling .cpp with ≤10 LOC → real stub, no fire."""
    findings = run(fixtures_dir / "good_stub_label")
    headers = [f for f in findings if f.file.endswith("widget.hpp")]
    assert headers == [], f"unexpected fire: {[(f.file, f.message) for f in headers]}"


def test_bad_python_stub_label_flags(fixtures_dir: Path) -> None:
    """Python file with stale stub label and no discriminator → flag."""
    findings = run(fixtures_dir / "bad_stub_label")
    py = [f for f in findings if f.file.endswith("widget.py")]
    assert len(py) == 1, f"unexpected: {[(f.file, f.message) for f in findings]}"


def test_python_permanent_stub_discriminator_does_not_flag(
    fixtures_dir: Path,
) -> None:
    """Python file with `permanent stub` framing should NOT flag, even if
    the literal `In Phase N, this is a stub:` phrase appears elsewhere."""
    findings = run(fixtures_dir / "good_stub_label")
    py = [f for f in findings if "permanent" in f.file]
    assert py == [], f"discriminator did not gate: {[(f.file, f.message) for f in py]}"


def test_check_id_and_mode() -> None:
    """Smoke: CHECK_ID and MODE are stable identifiers."""
    from integrity.cat2_contracts.checks.stub_label_stale import CHECK_ID, MODE
    from integrity.common.results import FailureMode
    assert CHECK_ID == "cat2.stub-label-stale"
    assert MODE == FailureMode.HARD_FAIL


def test_repo_root_with_no_common_dir_returns_empty(tmp_path: Path) -> None:
    """Running against a directory with no common/ tree → zero findings."""
    findings = run(tmp_path)
    assert findings == []
```

**Verbatim claim:** `tools/integrity/tests/conftest.py` defines a
`fixtures_dir` pytest fixture (per apispec § A.1 listing, file present
at 12 LOC). Verify on first reading that the fixture exists and resolves
to `tools/integrity/tests/fixtures/`. If absent or mis-pointing,
pause-and-surface.

### 3.6 Modified file: `tools/integrity/integrity/grandfather.py` — classifier rule

Add a new classifier rule before the fallback for `cat2.stub-label-stale`.
Insert after the existing `cat2.public-symbol-used-ts` block (apispec § B.1
lines 485–490 in the spec):

```python
    if cid == "cat2.stub-label-stale":
        return Classification(
            category="cat2-stub-label-stale",
            reason="pre-v1.1 stale Phase-N stub label on real implementation (canonical spec § 12 row 5; tracked for migration as the corresponding header is next edited)",
            issue_ref="n/a",
        )
```

### 3.7 Modified file: `tools/integrity/docs/grandfather-catalog.md`

Add a new category section in document order, after `cat2-stack-b-unused`
and before "Suppression-annotation discipline". Insert at the appropriate
location (before the line that reads `## Suppression-annotation discipline`):

```markdown
### `cat2-stub-label-stale`

**Pattern:** `cat2.stub-label-stale` findings — `In Phase N, this is a stub:`
labels where the corresponding implementation has more than 10 non-comment
LOC.

**Why grandfathered:** Two canonical cases exist in the repo as of v1.1
landing: `common/common-cpp/include/gpusims/alembic_writer.hpp:11`
(impl 99 non-comment LOC) and `common/common-cpp/include/gpusims/vdb_writer.hpp:12`
(impl 135 non-comment LOC). Both labels were carried over from Phase 1
when the surfaces were genuine stubs; subsequent Alembic and OpenVDB
enablement work landed real implementations without revising the header
labels. These are migration targets, not permanent suppressions.

**Tracking:** Two entries. Both have known migration paths: when the
headers are next edited for unrelated reasons, the "In Phase 1, this is
a stub:" framing should be replaced with the runtime-mode discriminator
shape used in the Stack D twins (e.g., `// Real-or-stub depending on
GPU_SIMS_HAVE_ALEMBIC`).

**Future treatment:** Remove suppression on each header when the
header is next modified. Permanent suppressions are not expected.
```

### 3.8 Run the grandfather sweep

After the new check is registered and the classifier rule is in place,
the check will fire on `alembic_writer.hpp` and `vdb_writer.hpp`. Run the
sweep to apply suppression annotations:

```
cd /path/to/repo
python3 -m pip install --break-system-packages -e tools/integrity[dev]
python3 tools/integrity/scripts/grandfather_sweep.py
```

Expected output:

```
grandfather-sweep: modified 2 files; 2 annotations added
  cat2-stub-label-stale: 2
```

<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
The sweep inserts an `// integrity-allow:` line immediately above the
stub-label line in each header. Verify the annotations are well-formed
by re-running `python3 -m integrity --mode strict --no-audit-log` and
confirming `0 hard_fail`.

### 3.9 Commit 1 verification block

After all edits land:

1. **Pytest:** `cd tools/integrity && pytest tests/ -v`. Expected: all
   prior tests still pass; new `test_cat2_stub_label_stale.py` passes
   (6 tests).
2. **Real-repo run:**
   `python3 -m integrity --mode strict --no-audit-log`.
   Expected: exit 0; summary `pass: N, soft_warn: 0, hard_fail: 0, suppressed: 1128`
   (1126 baseline from probe + 2 new stub-label-stale findings now
   grandfathered).
3. **Check ID smoke:**
   `python3 -m integrity --check cat2.stub-label-stale --output human --no-audit-log`.
   Expected: 0 hard_fail (both findings suppressed by the sweep).
4. **Sweep idempotence:** Re-run
   `python3 tools/integrity/scripts/grandfather_sweep.py`. Expected:
   `modified 0 files; 0 annotations added` — annotations from step 3.8
   are already present.
5. **CI dry-run on PR:** GitHub Actions integrity workflow passes on
   the PR.

### 3.10 Commit 1 audit report

Land `docs/diagnostics/_audits/integrity_v1_1_commit1_landing_2026-05-15.md`
with:

- **A. Change summary** — A.1 implementation per this spec § 3
- **B. File inventory** — 1 new check module, 1 new test, 6 new fixture files,
  4 modified (`__init__.py`, `grandfather.py`, `grandfather-catalog.md`,
  and the two grandfathered headers)
- **C. Verification** — verbatim output of all five verification steps
- **D. Behavioral notes** — both canonical spec § 12 row 5 cases detected;
  `vdb_writer.hpp` is a bonus catch beyond what spec § 12 named
- **E. Incidental findings** — any surprises encountered during execution

## 4. Commit 2 — A.5 markdown fenced-block awareness

### 4.1 Modified file: `tools/integrity/integrity/common/annotations.py`

Add the fence machinery from `grandfather.py` to the common module.
Append the following to the existing file (after the existing
`parse_annotation_line` function):

```python
# === Fenced-block awareness (A.5) ===
#
# integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a
# Markdown documents include literal `integrity-allow:` strings as
# grammar examples in fenced code blocks. Those examples are not real
# annotations and must not be parsed as such by either the
# annotation-form check or the suppression matcher.

_FENCE_RE = re.compile(r"^\s*```(?P<lang>[A-Za-z0-9_+\-]*)\s*$")


def is_inside_fenced_block(
    lines: list[str],
    target_line_zero_indexed: int,
) -> tuple[bool, str | None]:
    """Determine whether `lines[target_line_zero_indexed]` is inside a
    fenced markdown code block. The opening-fence line itself is
    considered in-fence (we toggle on at start of match)."""
    in_fence = False
    fence_lang: str | None = None
    for i, line in enumerate(lines):
        m = _FENCE_RE.match(line)
        if m:
            if in_fence:
                if i == target_line_zero_indexed:
                    return (True, fence_lang)
                in_fence = False
                fence_lang = None
            else:
                in_fence = True
                fence_lang = m.group("lang") or ""
                if i == target_line_zero_indexed:
                    return (True, fence_lang)
                continue
        if i == target_line_zero_indexed:
            return (in_fence, fence_lang)
    return (False, None)


def fence_state_per_line(lines: list[str]) -> list[bool]:
    """Single-pass version of `is_inside_fenced_block` returning a list
    where `result[i]` is True iff `lines[i]` is inside a fence (or is the
    opening/closing fence marker itself)."""
    state = [False] * len(lines)
    in_fence = False
    for i, line in enumerate(lines):
        if _FENCE_RE.match(line):
            if in_fence:
                state[i] = True  # closing fence line itself is in-fence
                in_fence = False
            else:
                in_fence = True
                state[i] = True  # opening fence line itself is in-fence
            continue
        state[i] = in_fence
    return state


def is_markdown_path(file_path: str | Path) -> bool:
    """Predicate used by parser/suppressor to gate fence awareness."""
    name = str(file_path).lower()
    return name.endswith(".md") or name.endswith(".rst")
```

### 4.2 Modified file: `tools/integrity/integrity/grandfather.py`

Replace the local fence machinery with imports from `common.annotations`.
Locate the existing block (apispec § B.1 lines 565–594, the
`_FENCE_RE` constant and `is_inside_fenced_block` function) and replace
with:

```python
from integrity.common.annotations import (
    _FENCE_RE,
    is_inside_fenced_block,
)
```

Keep `comment_form_for_md_inside_fence` in `grandfather.py` (it's
grandfather-emit-specific). Verify that `render_annotation_line` still
imports `is_inside_fenced_block` from the new location — it should
work transparently after the re-import.

### 4.3 Modified file: `tools/integrity/integrity/cat1_citations/checks/annotation.py`

Add fence-state computation per file. Modify `run()` (apispec § D.3
lines 1245–1274) to skip fence-internal lines:

Find the existing line:

```python
        for lineno, line in enumerate(text.splitlines(), start=1):
            for m in LOOSE_RE.finditer(line):
```

Replace with:

```python
        lines_list = text.splitlines()
        if is_markdown_path(rel):
            fence_state = fence_state_per_line(lines_list)
        else:
            fence_state = [False] * len(lines_list)

        for lineno, line in enumerate(lines_list, start=1):
            if fence_state[lineno - 1]:
                continue
            for m in LOOSE_RE.finditer(line):
```

Add the import at the top of the file (after the existing imports):

```python
from integrity.common.annotations import (
    fence_state_per_line,
    is_markdown_path,
)
```

### 4.4 Modified file: `tools/integrity/integrity/common/suppression.py`

Make the upward walk fence-aware. Locate `apply_suppressions` (apispec
§ D.2 lines 1105–1145). Find the existing block:

```python
        for f in file_findings:
            zero_idx = f.line - 1
            if zero_idx <= 0 or zero_idx > len(file_lines):
                continue
            # Walk upward through the contiguous block of annotation lines
            # immediately above the cited line. Multiple annotations stacked
            # above one line (e.g., mixed-category groups produced by the
            # grandfather sweep) all count as "immediately preceding."
            j = zero_idx - 1
            while j >= 0:
                line_text = file_lines[j]
                parsed = parse_annotation_line(line_text)
                if parsed is None:
                    break
                check_id, reason, issue_ref = parsed
                if _matches(check_id, f.check_id):
                    f.suppressed = True
                    f.suppression_reason = reason
                    f.suppression_issue = issue_ref
                    break
                j -= 1
```

Replace with:

```python
        is_md = is_markdown_path(file_path)
        fence_state = fence_state_per_line(file_lines) if is_md else None

        for f in file_findings:
            zero_idx = f.line - 1
            if zero_idx <= 0 or zero_idx > len(file_lines):
                continue
            # If the finding's own line is inside a fenced block, it is
            # itself a documentation example; no suppression is meaningful.
            if is_md and fence_state is not None and zero_idx < len(fence_state) and fence_state[zero_idx]:
                continue
            # Walk upward through the contiguous block of annotation lines
            # immediately above the cited line. Skip fence-internal lines —
            # an annotation inside a fenced example is not a live annotation.
            j = zero_idx - 1
            while j >= 0:
                if is_md and fence_state is not None and fence_state[j]:
                    break
                line_text = file_lines[j]
                parsed = parse_annotation_line(line_text)
                if parsed is None:
                    break
                check_id, reason, issue_ref = parsed
                if _matches(check_id, f.check_id):
                    f.suppressed = True
                    f.suppression_reason = reason
                    f.suppression_issue = issue_ref
                    break
                j -= 1
```

Add the import at the top of the file (after the existing imports):

```python
from integrity.common.annotations import (
    fence_state_per_line,
    is_markdown_path,
)
```

### 4.5 New fixture: `tools/integrity/tests/fixtures/good_citations/fenced_examples.md`

```markdown
<!-- tools/integrity/tests/fixtures/good_citations/fenced_examples.md -->
# Fenced annotation examples

// integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a
This file contains intentionally-malformed `integrity-allow:` strings
inside fenced code blocks. The cat1.annotation-form check must skip
these — they are documentation examples, not live annotations.

## Example 1: malformed grammar inside Python fence

```python
# This is a malformed annotation example:
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
# integrity-allow: malformed-grammar-here-no-semicolons
def foo():
    pass
```

## Example 2: blanket `*` inside generic fence

```
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
// integrity-allow: *; this is blanket and would be invalid; n/a
struct Bar;
```

## Example 3: short reason inside fence

```cpp
// integrity-allow: cat1.intra-repo; too short; n/a
int x;
```

End of file.
```

### 4.6 New fixture: `tools/integrity/tests/fixtures/good_citations/fence_no_suppress.md`

```markdown
<!-- tools/integrity/tests/fixtures/good_citations/fence_no_suppress.md -->
# Fence-internal annotations do not suppress

// integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a
This file tests that an `integrity-allow:` annotation inside a fenced
block does NOT suppress a real finding on the next line.

```cpp
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
<!-- integrity-allow: cat1.intra-repo; n/a; n/a -->
```

// integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a
Real broken citation outside any fence: see `nonexistent_file.cpp:42`
for details. (This citation should be flagged by cat1.intra-repo and
should NOT be suppressed by the fence-internal annotation above.)
```

(Note: the test for fixture 4.6 needs the toolkit's intra-repo check to
actually fire on this fixture; the citation should resolve to nothing.)

### 4.7 New test file: `tools/integrity/tests/test_cat1_annotation_fence.py`

```python
# tools/integrity/tests/test_cat1_annotation_fence.py
"""Tests for A.5: fence-block awareness in cat1.annotation-form parser
and in common/suppression.py."""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.checks.annotation import run as annotation_run
from integrity.common.annotations import (
    fence_state_per_line,
    is_inside_fenced_block,
    is_markdown_path,
)


def test_fence_state_per_line_basic() -> None:
    lines = [
        "outside",
        "```python",
        "inside line 1",
        "inside line 2",
        "```",
        "outside again",
    ]
    state = fence_state_per_line(lines)
    assert state == [False, True, True, True, True, False]


def test_fence_state_per_line_empty() -> None:
    assert fence_state_per_line([]) == []


def test_fence_state_handles_unclosed_fence() -> None:
    """An unclosed fence leaves all lines after the opener in-fence."""
    lines = ["outside", "```", "inside", "still inside"]
    state = fence_state_per_line(lines)
    assert state == [False, True, True, True]


def test_is_markdown_path() -> None:
    assert is_markdown_path("docs/foo.md")
    assert is_markdown_path("docs/bar.rst")
    assert not is_markdown_path("src/main.py")
    assert not is_markdown_path("docs/foo.txt")


def test_is_inside_fenced_block_target_inside() -> None:
    lines = ["outside", "```", "inside", "```"]
    in_fence, lang = is_inside_fenced_block(lines, 2)
    assert in_fence is True


def test_is_inside_fenced_block_target_outside() -> None:
    lines = ["outside", "```", "inside", "```", "outside"]
    in_fence, _ = is_inside_fenced_block(lines, 4)
    assert in_fence is False


def test_annotation_check_skips_fence_internal(fixtures_dir: Path) -> None:
    """cat1.annotation-form should NOT fire on malformed annotations
    inside fenced code blocks (only on real live annotations)."""
    findings = annotation_run(fixtures_dir / "good_citations" / "fenced_examples.md")
    # Pre-A.5: 3 grammar findings on this fixture.
    # Post-A.5: 0 (all are fence-internal).
    fence_findings = [f for f in findings if "fenced_examples.md" in f.file]
    assert fence_findings == [], (
        f"fence-internal annotations should be skipped: {[(f.file, f.message) for f in fence_findings]}"
    )
```

Note: this test imports `annotation_run` and runs it directly. Adjust
the test to match the actual `run()` signature in
`cat1_citations/checks/annotation.py`. The test runs against a directory
root, not a single file; pass `fixtures_dir / "good_citations"` if `run`
expects a directory, or refactor to pass the markdown file's containing
directory.

**Verification of the test harness:** before writing the test, read
`tools/integrity/tests/test_cat1_annotation.py` (existing file per
apispec § A.1, 47 LOC); mirror its `run` invocation pattern. If the
existing pattern differs from what's drafted here, adjust.

### 4.8 Suppression test in `tools/integrity/tests/test_suppression_fence.py`

```python
# tools/integrity/tests/test_suppression_fence.py
"""Tests for A.5: fence-block awareness in common/suppression.py.

# integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a
A `integrity-allow:` annotation inside a fenced code block must NOT
suppress a real finding on the line immediately following the fence."""

from __future__ import annotations

from pathlib import Path

from integrity.common.results import FailureMode, Finding
from integrity.common.suppression import apply_suppressions


def test_fence_internal_annotation_does_not_suppress(tmp_path: Path) -> None:
    """A fence-internal annotation should NOT suppress a finding on the
    line following the fence."""
    md = tmp_path / "example.md"
    md.write_text(
        "\n".join([
            "# Heading",
            "",
            "```cpp",
# integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a
            "// integrity-allow: cat1.intra-repo; documentation only; n/a",
            "```",
            "real_broken_citation:42",  # line 6
            "",
        ]),
        encoding="utf-8",
    )

    findings = [
        Finding(
            check_id="cat1.intra-repo",
            mode=FailureMode.HARD_FAIL,
            file="example.md",
            line=6,
            message="dangling citation",
        )
    ]

    result = apply_suppressions(findings, tmp_path)
    assert len(result) == 1
    assert result[0].suppressed is False, (
        "fence-internal annotation should not suppress findings outside the fence"
    )


def test_live_annotation_above_fence_suppresses(tmp_path: Path) -> None:
    """A live annotation OUTSIDE the fence should suppress as before."""
    md = tmp_path / "example.md"
    md.write_text(
        "\n".join([
            "# Heading",
            "",
            "<!-- integrity-allow: cat1.intra-repo; real annotation; n/a -->",
            "real_broken_citation:42",  # line 4
            "",
        ]),
        encoding="utf-8",
    )

    findings = [
        Finding(
            check_id="cat1.intra-repo",
            mode=FailureMode.HARD_FAIL,
            file="example.md",
            line=4,
            message="dangling citation",
        )
    ]

    result = apply_suppressions(findings, tmp_path)
    assert len(result) == 1
    assert result[0].suppressed is True
```

### 4.9 Run the grandfather sweep again

With A.5 in place, the `cat1.annotation-form` check stops firing on
~40 fence-internal examples. The previously-applied suppressions for
those examples become dead weight (they still match valid grammar but
the underlying finding no longer exists).

The grandfather sweep is idempotent — it only adds annotations, never
removes them. So those dead suppressions remain in place, harmless but
clutter. **Do not remove them in commit 2.** They'll be cleaned up
incidentally when the affected docs are next edited (same pattern as
the `live-shader-1810` category).

Verify the drop:

```
python3 -m integrity --output json --no-audit-log --mode warn-only \
  | python3 -c "import json, sys; d=json.load(sys.stdin); print(sum(1 for f in d['findings'] if f['check_id']=='cat1.annotation-form'))"
```

Expected: a number less than 69 (the pre-A.5 count from probe § C.2).
Target: ~28 (69 minus the ~40 fence-internal entries; the
`toolkit-own-source` category at 21 entries is NOT in fences and
remains).

### 4.10 Commit 2 verification block

1. **Pytest:** `cd tools/integrity && pytest tests/ -v`. All prior tests
   pass plus new fence tests pass.
2. **Real-repo run:** `python3 -m integrity --mode strict --no-audit-log`
   exits 0.
3. **Grammar finding drop:** the JSON-pipe command from § 4.9 reports
   ~28-35 `cat1.annotation-form` findings (down from 69).
4. **Existing suppressions still suppress:** the pre-existing live
   annotations above non-fence findings continue to suppress as before.
   Verified by `--output json` showing no new HARD_FAIL.
5. **CI passes** on the PR.

### 4.11 Commit 2 audit report

`docs/diagnostics/_audits/integrity_v1_1_commit2_landing_2026-05-15.md`
with the same A–E section structure as commit 1's report. Notable items:

- B: 4 modified (`common/annotations.py`, `grandfather.py`,
  `cat1_citations/checks/annotation.py`, `common/suppression.py`) + 3
  new (2 fixtures, 2 test files).
- D: report the pre/post `cat1.annotation-form` finding count delta;
  flag the dead-suppression cleanup as a future-treatment item.

## 5. Commit 3 — A.7 + A.8 + 5.B

### 5.1 Modified file: `tools/integrity/integrity/runner.py` — `--grandfather-report` and `--state-snapshot`

Extend the CLI per Decisions 7 and 8. Locate `parse_args` (apispec § C.2
lines 874–900) and add:

```python
    parser.add_argument("--grandfather-report", action="store_true",
                        help="Emit per-category grandfather counts to stdout and append a timestamped entry to .grandfather-history.json")
    parser.add_argument("--no-history-append", action="store_true",
                        help="With --grandfather-report, skip the history file append (read-only mode)")
    parser.add_argument("--state-snapshot", action="store_true",
                        help="Emit a full toolkit-state JSON snapshot to stdout and exit")
```

Add to `CliArgs` dataclass:

```python
@dataclass
class CliArgs:
    cat: int | None
    check: str | None
    mode: str
    root: Path
    output: str
    no_audit_log: bool
    grandfather_report: bool
    no_history_append: bool
    state_snapshot: bool
```

Add to the return of `parse_args`:

```python
        grandfather_report=ns.grandfather_report,
        no_history_append=ns.no_history_append,
        state_snapshot=ns.state_snapshot,
```

Modify `main` to short-circuit on these flags. After `args = parse_args(argv)`:

```python
    if args.state_snapshot:
        from integrity.snapshot import emit_state_snapshot
        emit_state_snapshot(args.root, sys.stdout)
        return EXIT_OK

    if args.grandfather_report:
        from integrity.snapshot import emit_grandfather_report
        emit_grandfather_report(args.root, sys.stdout,
                                append_history=not args.no_history_append)
        return EXIT_OK
```

### 5.2 New file: `tools/integrity/integrity/snapshot.py`

```python
# tools/integrity/integrity/snapshot.py
"""State-snapshot and grandfather-report emitters (A.7, A.8).

Two entry points:

- `emit_state_snapshot(root, stdout)` — A self-contained JSON document
  describing the toolkit's complete state at a single moment: commit SHA,
  timestamp, registered checks, registered upstream sources, full
  per-category suppression counts. Intended as the "verification
  provenance" anchor for spec drafts (v1.1 spec § D.1).
- `emit_grandfather_report(root, stdout, append_history=True)` — Human-
  readable per-category table to stdout; optionally appends a JSON entry
  to `tools/integrity/.grandfather-history.json` (a time series).
"""

from __future__ import annotations

import datetime as dt
import json
import subprocess
import sys
from pathlib import Path
from typing import IO


HISTORY_FILE_RELATIVE = Path("tools/integrity/.grandfather-history.json")


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
        # The reason string includes a `(<category>)` parenthetical in
        # most grandfather-applied annotations per grandfather.py
        # classifier output. Extract by suffix match against known
        # category names. If extraction fails, bucket as "other".
        category = _extract_category(reason)
        per_category[category] = per_category.get(category, 0) + 1

    return {
        "schema_version": 1,
        "timestamp": dt.datetime.now(dt.timezone.utc).isoformat(),
        "commit": data.get("commit", "unknown"),
        "summary": data.get("summary", {}),
        "per_category": per_category,
    }


_KNOWN_CATEGORIES = (
    "audit-citation",
    "live-shader-1810",
    "audit-doc-1810",
    "spec-grammar-example",
    "toolkit-own-source",
    "retro-grammar-example",
    "audit-report-grammar-example",
    "other-cat1",
    "cat2-stack-d-unused",
    "cat2-stack-c-unused",
    "cat2-stack-b-unused",
    "cat2-stub-label-stale",
)


def _extract_category(reason: str) -> str:
    """Match the suppression_reason text to a known category."""
    lowered = reason.lower()
    for cat in _KNOWN_CATEGORIES:
        if cat in lowered:
            return cat
    return "other"


def emit_state_snapshot(root: Path, out: IO[str]) -> None:
    """Emit a complete toolkit-state JSON document to `out`."""
    state = _collect_state(root)

    # Registered checks.
    from integrity.cat1_citations.checks import REGISTERED_CHECKS as cat1
    from integrity.cat2_contracts.checks import REGISTERED_CHECKS as cat2
    from integrity.cat3_numerical.checks import REGISTERED_CHECKS as cat3
    state["registered_checks"] = {
        "cat1": [cid for cid, _ in cat1],
        "cat2": [cid for cid, _ in cat2],
        "cat3": [cid for cid, _ in cat3],
    }

    # Registered upstreams from ground-truth-sources.md (parse the doc
    # for `anchor_version` blocks).
    state["registered_upstreams"] = _parse_ground_truth_sources(root)

    json.dump(state, out, indent=2)
    out.write("\n")


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


def _parse_ground_truth_sources(root: Path) -> list[dict]:
    """Parse `tools/integrity/docs/ground-truth-sources.md` for upstream
    anchor blocks. Returns a list of {name, version, sha, vendor_root}
    dicts.

    The doc uses TOML-style blocks; this is a permissive line-based
    parser that doesn't import a TOML library."""
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
            current["version"] = val.strip().strip('"').strip("'")
        elif stripped.startswith("anchor_sha"):
            _, _, val = stripped.partition("=")
            current["sha"] = val.strip().strip('"').strip("'")
        elif stripped.startswith("vendor_root"):
            _, _, val = stripped.partition("=")
            current["vendor_root"] = val.strip().strip('"').strip("'")
        elif stripped.startswith("name"):
            _, _, val = stripped.partition("=")
            current["name"] = val.strip().strip('"').strip("'")
        elif stripped == "":
            if current.get("version") and current.get("sha"):
                blocks.append(current)
            current = {}

    if current.get("version") and current.get("sha"):
        blocks.append(current)

    return blocks
```

**Verification claim:** `tools/integrity/docs/ground-truth-sources.md`
exists and contains `anchor_version = "2.16.1"` and
`anchor_sha = "6bff55a6..."` per probe § G.2. If the doc uses a
different syntax (e.g. fenced TOML blocks), the parser in
`_parse_ground_truth_sources` may need to be adjusted — read the doc
first and confirm the format before implementing.

### 5.3 New file: `tools/integrity/.grandfather-history.json`

Seed with an empty list:

```json
[]
```

This file is checked into the repo. Subsequent `--grandfather-report`
runs append to it. The first real entry will land when this commit's
verification block runs the report.

### 5.4 Modified file: `tools/integrity/docs/grandfather-catalog.md` — populate counts (A.8)

For each category heading in the catalog, append a parenthetical count
populated from the current `python3 -m integrity --grandfather-report`
run. Per Decision 9:

Before:

```markdown
### `audit-citation`

**Pattern:** ...
```

After:

```markdown
### `audit-citation` (761)

**Pattern:** ...
```

Apply this transformation to all 12 categories (11 original from
apispec § F.1 + the new `cat2-stub-label-stale` from commit 1). Use the
actual count from the report; do not invent numbers.

Also remove any approximate-prose ranges inside category bodies (e.g.
the `~10 entries` mention inside `live-shader-1810`'s body, the
`~108 entries` mention inside `cat2-stack-c-unused`'s body). These are
now redundant with the heading suffix.

**Implementation note:** the count refresh is a one-time manual edit
in this commit. Future updates (when counts change) are tracked as v1.2
items (auto-refresh from the history file). Document the manual-update
expectation in a new sub-section near the top of the catalog:

```markdown
## Updating counts

The per-category counts in the headings above reflect the toolkit state
at the time this catalog was last manually refreshed. To refresh:

    python3 -m integrity --grandfather-report --no-history-append

Then update each category heading's parenthetical with the count from
the report. Auto-refresh is a v1.2 candidate.
```

Insert this section after the introductory paragraphs and before the
first category heading.

### 5.5 5.B docs sweep — `python` → `python3`

Apply textual substitution to three files per Decision 10. Use exact
literal match (case-sensitive, word-boundary anchored).

**`tools/integrity/README.md`:** replace each of the 6 occurrences of
`python -m integrity` with `python3 -m integrity`.

**`docs/integrity-toolkit-spec.md`:** replace each of the 6 occurrences
similarly. Note that one occurrence (line 254 per probe § J.1) is
inside a directory-tree code block:

```
│   ├── __main__.py                    # `python -m integrity` entry
```

Replace this too — the code block is illustrative documentation, not
verbatim code.

**`tools/integrity/integrity/__main__.py:1`:** the docstring tagline.
Before:

```python
"""Entry point: `python -m integrity`."""
```

After:

```python
"""Entry point: `python3 -m integrity`."""
```

**Do NOT touch** audit reports under `docs/diagnostics/_audits/` —
append-only convention per `audit-citation` grandfather rationale.

### 5.6 Commit 3 verification block

1. **State snapshot smoke:**
   `python3 -m integrity --state-snapshot > /tmp/state.json && python3 -m json.tool /tmp/state.json`.
   Expected: valid JSON with `schema_version`, `timestamp`, `commit`,
   `summary`, `per_category`, `registered_checks` (12 IDs across cat1/cat2/cat3),
   `registered_upstreams` (≥ 1 entry — SPlisHSPlasH 2.16.1).
2. **Grandfather report smoke:**
   `python3 -m integrity --grandfather-report --no-history-append`.
   Expected: human-readable table with all 12 categories and counts.
3. **History append:**
   `python3 -m integrity --grandfather-report` followed by
   `cat tools/integrity/.grandfather-history.json | python3 -m json.tool`.
   Expected: history file contains exactly 1 entry (since seeded
   empty in § 5.3).
4. **Catalog tally verification:** grep each category heading in
   `tools/integrity/docs/grandfather-catalog.md` and confirm the
   parenthetical count matches the report's count for that category.
5. **Docs sweep verification:**
   `rg -n '\bpython -m integrity\b' tools/integrity/README.md docs/integrity-toolkit-spec.md tools/integrity/integrity/__main__.py`.
   Expected: 0 matches.
   `rg -n '\bpython3 -m integrity\b' tools/integrity/README.md docs/integrity-toolkit-spec.md tools/integrity/integrity/__main__.py`.
   Expected: ≥ 13 matches.
6. **Pytest:** existing tests pass; no new tests required for this
   commit beyond the ones for A.7 (see § 5.7).
7. **Real-repo run:** `python3 -m integrity --mode strict --no-audit-log`
   exits 0.
8. **CI passes** on the PR.

### 5.7 New tests for A.7

Add `tools/integrity/tests/test_snapshot.py`:

```python
# tools/integrity/tests/test_snapshot.py
"""Tests for A.7 state-snapshot and grandfather-report machinery."""

from __future__ import annotations

import io
import json
from pathlib import Path

from integrity.snapshot import (
    _extract_category,
    _parse_ground_truth_sources,
    emit_state_snapshot,
)


def test_extract_category_matches_known() -> None:
    assert _extract_category("audit-doc snapshot (audit-citation)") == "audit-citation"
    assert _extract_category("pre-v1 Stack C public symbol (cat2-stack-c-unused)") == "cat2-stack-c-unused"
    assert _extract_category("unrelated reason text") == "other"


def test_parse_ground_truth_sources_smoke(repo_root: Path) -> None:
    """Real ground-truth-sources.md should yield at least one upstream."""
    sources = _parse_ground_truth_sources(repo_root)
    assert len(sources) >= 1
    assert any("2.16.1" in s.get("version", "") for s in sources)


def test_emit_state_snapshot_smoke(repo_root: Path) -> None:
    """The snapshot emitter produces valid JSON with required fields."""
    out = io.StringIO()
    emit_state_snapshot(repo_root, out)
    data = json.loads(out.getvalue())
    assert "schema_version" in data
    assert "timestamp" in data
    assert "commit" in data
    assert "registered_checks" in data
    assert "registered_upstreams" in data
    assert isinstance(data["registered_checks"]["cat1"], list)
    assert isinstance(data["per_category"], dict)
```

**Verification claim:** `tools/integrity/tests/conftest.py` (12 LOC per
apispec § A.1) defines a `repo_root` pytest fixture. If absent, add one
that resolves to the git repo root via `git rev-parse --show-toplevel`.
Otherwise, mirror the existing pattern.

### 5.8 Commit 3 audit report

`docs/diagnostics/_audits/integrity_v1_1_commit3_landing_2026-05-15.md`.
Notable items:

- B: 1 modified (`runner.py`), 1 new (`snapshot.py`), 1 new
  (`.grandfather-history.json`), 1 modified (`grandfather-catalog.md`),
  3 modified for 5.B (README, spec, `__main__.py`), 1 new test
  (`test_snapshot.py`).
- C: verbatim output of `--state-snapshot` (first 30 lines), verbatim
  `--grandfather-report` table, verbatim grep outputs from step 5.
- D: note that A.8 counts are now manual-refresh; v1.2 auto-refresh
  is a candidate.

## 6. Cross-cutting concerns

### 6.1 Idempotence

Every commit must be re-runnable. Specifically:

- Commit 1's grandfather sweep is idempotent by construction
  (apispec § B.1 — `annotation_already_present` check).
- Commit 2's parser/suppressor changes are pure functions of input;
  re-applying produces the same output.
- Commit 3's catalog tallies are manual; re-running the snapshot emitter
  produces an additional history entry but does not modify other files.

### 6.2 Backward compatibility

- All existing CHECK_IDs remain registered with unchanged semantics.
- All existing CLI flags continue to work.
- All existing test fixtures continue to pass.
- The grandfather classifier picks up the new `cat2-stub-label-stale`
  rule via the first-match-wins order documented in apispec § B.1
  line 466 ("First match wins"). Insert the new rule at the position
  specified in § 3.6.

### 6.3 CI behavior

CI runs are unchanged in structure but:

- After commit 1: 2 new findings, both suppressed. Wall clock unchanged
  (stub-label-stale scan is small — covers ~30 header files plus a
  small number of `.py` files).
- After commit 2: ~40 fewer findings. Wall clock unchanged or slightly
  faster (one fewer iteration through fence-internal lines per markdown
  file scan).
- After commit 3: ~0 additional wall clock for non-snapshot runs;
  snapshot/report runs add ~2 seconds for the subprocess re-invocation.

### 6.4 SHA back-fill

After each commit lands, the audit report's first line ("Companion to:")
will reference a placeholder for the upcoming next-commit's audit. Once
all three commits land, a single SHA back-fill commit updates all three
audit reports' "Companion to:" lines with the actual SHAs. This back-fill
commit is the only `git commit --amend`-shaped action in the sequence,
and it lands as a separate commit per Convention #12 (NEVER use
`--amend`; back-fill is a normal commit that edits docs).

## 7. Pre-execution checklist

Before starting commit 1, Claude Code must verify:

- [ ] HEAD SHA at execution time is `447ebf0` (or a descendant that
  hasn't modified any of the files this spec touches). If HEAD has
  drifted, surface for re-anchoring.
- [ ] `references/SPlisHSPlasH` HEAD matches the registered SHA
  `6bff55a6eaf14083d34650f22a268ce156b62b54` (probe § G.1).
- [ ] `python3 -m integrity --mode strict --no-audit-log` exits 0
  against the current `main`.
- [ ] `pytest tools/integrity/tests/ -v` reports the baseline test
  count (probe § A.1: `test_*.py` files exist for `cat1_annotation`,
  `cat1_intra_repo`, `cat1_unregistered`, `cat1_upstream_anchor`,
  `cat1_upstream`, `cat2_stack_b`, `cat2_stack_c`, `cat2_stack_d`,
  `cat3_cubic_kernel`, `grandfather_sweep`, `runner`).
- [ ] `tools/integrity/tests/conftest.py` exports `fixtures_dir` and
  (if commit 3 reaches its tests) `repo_root` pytest fixtures.
- [ ] `tools/integrity/docs/ground-truth-sources.md` exists and uses
  the syntax the snapshot parser in § 5.2 expects (probe § G.2 claim).
- [ ] `.github/workflows/integrity.yml` is in place and CI is green
  on the current `main`.

## 8. Substantive completion criteria

Batch 1 is complete when:

1. All three commits have landed on `main` in order.
2. All three audit reports are in `docs/diagnostics/_audits/`.
3. SHA back-fill commit has landed.
4. CI is green for the entire batch.
5. v1.1 spec § 7 acceptance criteria for Milestone A items A.1, A.5,
   A.7, A.8 are met (the remaining Milestone A items — A.2, A.3, A.4,
   A.6 — remain deferred).
6. The `python3` standardization (5.B) is complete in user-facing docs.

## 9. Out of scope for this batch

Reaffirmed deferrals per § 1.1:

- **A.2** (toolkit self-application): requires design decision on
  `tools/integrity/integrity/__init__.py` re-export shape. Architect-2
  review item.
- **A.3, A.4** (grammar extensions for bare-path-to-upstream-basename
  and multi-line citations): combined design pass.
- **A.6** (Stack C runtime optimization): largest scope; needs
  architectural design (USR cache, parallelization strategy).
- **A.9** (audit-citation file-pattern exclusion): pending architect-2
  review per v1.1 spec § 7 open question.
- All Milestone B, C, D items.

## End of execution spec

Total file count: ~6 new + ~8 modified across three commits. Total
estimated diff: ~750 lines new + ~200 lines modified. Each commit's
audit report adds ~200 LOC of report prose.

The three commits are independently revertable. The grandfather sweep
in commit 1 modifies live source (annotation insertions on two headers);
if reverted, those annotations remain harmless but should be removed
in the revert commit for cleanliness.
