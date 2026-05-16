---
title: "Integrity Toolkit v1.3 Batch-1 Part-A — Execution Spec"
date: 2026-05-16
author: architect1
status: draft
audience: Claude Code (executor)
sibling-docs:
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/retro/integrity-toolkit-v1.2-bolt-ons.md
  - docs/retro/integrity-toolkit-v1.3-candidates.md
  - docs/diagnostics/_audits/integrity_v1_3_t1_3_5_probe_2026-05-16_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md
---

# Integrity Toolkit v1.3 Batch-1 Part-A — Execution Spec

## 0. Execution preamble (read this first)

You are Claude Code executing the v1.3 batch-1 part-A small-scope batch
covering three items from the v1.3 candidates roadmap:

- **T1.3** — Catalog auto-refresh script (roadmap § 4 T1.3)
- **T1.5** — TOML → JSON convergence for cat3 expected values (roadmap § 4 T1.5)
- **T1.4** — Probe template conventions doc (roadmap § 4 T1.4)

Four commits total (three substantive + SHA back-fill). Each
commit's verification block must pass before starting the next.
Materialize every file creation, modification, and removal specified.

### 0.1 Hard rules

1. **Execute every file creation, modification, and removal specified.
   Do not skip any.**
2. **The synced repo state is authoritative over this spec.** Every
   verbatim claim about file contents was grep-verified against probe
   SHA `df213120354ccb75b297f7b7cc63dd06785955f9` (= `df21312`) during
   drafting per the pre-spec probe at
   `docs/diagnostics/_audits/integrity_v1_3_t1_3_5_probe_2026-05-16_architect1.md`.
   The probe's own § G correction block flags that anchoring shifted
   from `e079c7b` to `df21312` mid-probe; this spec uses the corrected
   anchor.

   If HEAD has moved past `df21312` when you execute, **re-anchor
   before applying** — specifically, A.2 commit 3 was in-progress at
   probe end (registering `cat2.public-symbol-used-toolkit` in
   `cat2_contracts/checks/__init__.py`). If A.2 has landed further
   commits that touch any of the surfaces in § 3 / § 4 / § 5 of this
   spec, **pause and surface the conflict; do not silently adapt.**

3. **Land commits in the order given.** T1.3 → T1.5 → T1.4 → SHA
   back-fill. Each commit's verification block must pass before
   starting the next. Do not interleave.

4. **One audit report per commit.** Front-matter and structure mirror
   the v1.2 commit-N landing reports. File location pattern:
   `docs/diagnostics/_audits/integrity_v1_3_commit<N>_landing_2026-05-16.md`.

5. **SHA back-fill is a separate follow-up commit, never `--amend`.**
   Per Convention #12 (retro § 7.2). Commit 4 in this spec.

6. **The toolkit must remain green-relative-to-baseline across every
   commit.** Baseline at probe SHA `df21312` was approximately
   **53 hard-fails / 1213 suppressed** (per probe § A.2; this exceeds
   the v1.2-bolt-ons-era baseline of 44 because A.2 commit 1
   registered the new `cat2.public-symbol-used-toolkit` check, which
   surfaces toolkit-internal unused-symbol findings). Each commit's
   verification block specifies expected counts post-commit. Hard-fail
   count must not exceed the pre-commit baseline by more than the
   audit-doc bare-path findings the commit's own audit report introduces.

7. **`python3`, not `python`.** The host has no `python` shim.

8. **Live-source-stays-red discipline (P1.8 / Convention G).** Force-
   sweep is per-category only. None of this batch's commits should
   require `--force-sweep-category`; if you find yourself reaching for
   it, **pause and surface.**

9. **Audit-prose freshness (Convention F).** Before each commit, run
   the verification block in full. If any verbatim claim in the
   commit's audit report disagrees with current disk, record the
   discrepancy as an addendum section in the same audit report; do
   not silently rewrite the body.

### 0.2 File-path conventions

- New scripts: `tools/integrity/scripts/<name>.py`
- New tests: `tools/integrity/tests/test_<area>.py`
- New toolkit docs: `tools/integrity/docs/<name>.md`
- Audit reports: `docs/diagnostics/_audits/integrity_v1_3_commit<N>_landing_2026-05-16.md`
- Grandfather sweep entry: `python3 tools/integrity/scripts/grandfather_sweep.py`

### 0.3 Convention B reminder (sweep companion)

Every commit in this batch lands an audit report under
`docs/diagnostics/_audits/`, which is in the SWEEPABLE_PATH_PREFIXES
bucket per P1.8. Each commit's verification block runs an inline sweep
companion as the final step before commit — this sweeps the new
audit-doc bare-path findings the commit introduces, keeps the gate
clean, and lets the next commit's verification anchor against a stable
baseline.

### 0.4 Coordination with concurrent A.2 work

A.2 (toolkit self-application) has landed commits 1 (`e079c7b`) and 2
(`df21312`). A.2 commit 3 was in-progress at probe end, modifying
`tools/integrity/integrity/cat2_contracts/checks/__init__.py`. Further
A.2 commits will likely land while this batch executes.

This batch's edit surface overlap with A.2:

- **`tools/integrity/docs/grandfather-catalog.md`** — A.2 may run a
  sweep that updates the `toolkit-own-unused (?)` heading to a numeric
  count. T1.3 (commit 1) reads but does not write this file in normal
  operation (its `--dry-run` path inspects it; its write path edits
  parentheticals only). If A.2 lands a sweep between this batch's
  commit 1 and commit 4, the catalog state shifts; the T1.3 script is
  designed to handle that (probe § B.7 (1) — non-numeric parenthetical
  is preserved, numeric is refreshed).
- **No other shared files.** T1.3's script, T1.5's cat3 files, and
  T1.4's doc do not overlap with A.2's surface.

**Pull-rebase before every commit.** If a rebase introduces structural
changes to `grandfather-catalog.md`'s heading format (the
`` ### `<category>` (<count>) `` shape), **pause and surface** — T1.3's
parser depends on the format.

## 1. Goals & load-bearing decisions

### 1.1 Goals

Three roadmap items land as three commits:

1. **T1.3** — A new script at
   `tools/integrity/scripts/refresh_catalog_counts.py` that refreshes
   the per-category count parentheticals in
   `tools/integrity/docs/grandfather-catalog.md` from the output of
   `python3 -m integrity --grandfather-report --no-history-append`.
   Idempotent. In-place edit. Includes `--dry-run` and `--catalog-path`
   flags.
2. **T1.5** — Replace `cat3_numerical/expected_values.toml` with
   `cat3_numerical/expected_values.json` (d3q19-shape schema). Update
   `cubic_kernel.py` to load JSON instead of TOML; update
   `generate_expected.py` to emit JSON; update the one test docstring
   that references the old filename. Land a new
   `tools/integrity/docs/cat3-conventions.md` documenting the
   JSON-as-canonical-format convention.
3. **T1.4** — A new doc at
   `tools/integrity/docs/probe-template-conventions.md` documenting
   Convention C (path-resolution enumeration) and Convention D
   (call-site enumeration) from v1.1 batch-1 retro § 7.2, with six
   worked examples already published in existing audit/retro reports.

Plus:

4. **Commit 4 — SHA back-fill.** Updates the three substantive
   commits' audit reports with the SHAs of the others.

### 1.2 Load-bearing decisions made during drafting

#### Decision 1 — Commit ordering: T1.3 → T1.5 → T1.4 → SHA back-fill

T1.3 lands first per new-files-first convention (retro § 7.2 A): it
introduces the largest new-files surface (script + test file). T1.5
second so T1.4's process doc can cite T1.5 as a worked example of the
"new file as v1.3 product" pattern. T1.4 last because it is smallest
(pure prose + embedded excerpts) and is the natural closing item for
the batch. SHA back-fill last per Convention #12.

This ordering also bounds A.2 race exposure: T1.3 reads `grandfather-catalog.md`
but doesn't write it in the common case; landing it first means
subsequent A.2 sweep companions can rewrite the catalog without
breaking T1.3's tests (the tests use fixture files, not the live
catalog).

#### Decision 2 — T1.3 subprocess-not-import (per probe § B.3)

The script invokes
`python3 -m integrity --grandfather-report --no-history-append` as a
subprocess rather than importing `emit_grandfather_report` from
`integrity.snapshot`. Rationale per probe § B.3: the CLI surface is
the toolkit's stable contract for this data; the internal function
has a side-effecting default argument (`append_history=True`) and
writes human-text to a caller-supplied `IO[str]` rather than returning
a parseable structure. Subprocess + regex parsing is more durable
across future toolkit refactors.

#### Decision 3 — T1.3 design choices (per probe § B.7)

All six design choices are settled per probe § B.7 recommendations:

1. **Heading-with-no-report:** preserve verbatim (handles the
   `toolkit-own-unused (?)` placeholder until A.2's sweep populates it).
2. **Report-with-no-heading:** error out with a clear message naming
   the missing categories.
3. **Two-number-prose handling (`other-cat1-bare-path`):** preserve
   verbatim.
4. **Idempotency:** required test case.
5. **Output mode:** in-place edit with `--dry-run` flag that prints
   the proposed diff without writing.
6. **Refresh-prose update:** the catalog's `## Updating counts` block
   (lines 13–25 at probe time) is updated in the same commit to
   reflect that auto-refresh now exists.

#### Decision 4 — T1.3 catalog-heading parser

The canonical heading regex per probe § B.1:

```
^### `(?P<cat>[a-z0-9-]+)` \((?P<count>.+?)\)$
```

The parser uses `re.match` per line. A heading is "numeric"
(eligible for refresh) iff its `count` group matches `^\d+$`
(decimal non-negative integer); otherwise it is "non-numeric" and
preserved verbatim regardless of whether the report has a count for
that category.

#### Decision 5 — T1.3 report-line parser

Per probe § B.2, lines of the form `{cat:>35s}: {n}` (right-justified
35-char field, colon-space, integer). The parser uses a regex that
splits on the LAST `:` (to tolerate hypothetical future emission of
hyphenated category names with embedded colons, though none exist
today):

```
^\s*(?P<cat>[a-z0-9-]+):\s+(?P<count>\d+)\s*$
```

Header lines from `--grandfather-report` (commit/timestamp, summary
dict, "per-category counts:" label) do not match this regex and are
ignored. Lines that match but where the count fails int parsing
trigger an error (defensive — probe § F.2 noted no documented format
stability guarantee).

#### Decision 6 — T1.5 file-operation sequence (per probe § D.8)

`git rm expected_values.toml` + `git add expected_values.json`. Per
probe § D.8: rename detection has near-zero practical value for a
content-type change, and the explicit delete+add is cleaner in
review.

#### Decision 7 — T1.5 JSON schema (per probe § D.8 + § D.5)

Top-level shape mirrors d3q19's schema:

```
{
  "schema_version": 1,
  "source": "tools/integrity/integrity/cat3_numerical/generate_expected.py",
  "derivation": "SPlisHSPlasH 2.16.1 SPHKernels.h:43-85",
  "anchor_sha": "6bff55a6eaf14083d34650f22a268ce156b62b54",
  "tolerance": {"atol": 1e-5, "rtol": 1e-5},
  "test_points": [ ... ]
}
```

The four TOML header comments (anchor provenance, generator pointer,
upstream-source pointer, anchor SHA) map to top-level keys (`source`,
`derivation`, `anchor_sha`). The `[tolerance]` table becomes a nested
object. The `[[test_points]]` array-of-tables becomes a JSON list of
objects. Numeric values are preserved byte-for-byte from the TOML
source (e.g., `0` stays `0`, not `0.0` — the harness's `float()` cast
handles either).

#### Decision 8 — T1.5 doc home (per probe § D.10)

New file at `tools/integrity/docs/cat3-conventions.md`. Probe § D.9
showed `ground-truth-sources.md` has narrow scope ("upstream-source
registry, parsed by `cat1_citations/upstream_anchor.py`") and adding
an "expected-values format" section would fight its structure. The
new file is also positioned as a growth path for future cat3
conventions (T1.2 follow-through, T2.x candidates).

#### Decision 9 — T1.4 worked-example embedding (per probe § C.7)

Embed all six worked examples (three per convention). Probe § C.7
notes the doc-without-examples is just two banked-convention
sentences; the embedded examples are the evidence that converts the
convention from "yet another aspirational doc" to "here is the
failure mode this prevents." Per v1.1 batch-1 retro § 7.2, conventions
should ultimately live in a "permanent CONVENTIONS doc rather than
re-derivation in every retro"; T1.4 is that home for Conventions C
and D.

Probe § F.6 noted that the bolt-ons probe § D.1 example carries a
stale line number (`runner.py:148` → current disk `:154`). For T1.4's
embedded example, line numbers are tagged "as of probe SHA" or
omitted; the worked-example payload is the methodology, not the
specific line citation.

#### Decision 10 — Test directory placement

T1.3 tests land in `tools/integrity/tests/test_refresh_catalog_counts.py`
(new file). Mirrors the per-script-test-file pattern that
`test_grandfather_sweep.py` set. T1.5 tests are existing modifications
to `test_cat3_cubic_kernel.py` (docstring-only updates plus one
optional new test verifying JSON-schema parse). T1.4 has no tests
(pure prose doc).

## 2. Commit plan

| # | Item | New files | Modified files | Removed files | Estimated diff |
|---|---|---|---|---|---|
| 1 | T1.3 | `scripts/refresh_catalog_counts.py`, `tests/test_refresh_catalog_counts.py`, audit report | `docs/grandfather-catalog.md` (prose update) | (none) | ~170 LOC new, ~12 LOC modified |
| 2 | T1.5 | `cat3_numerical/expected_values.json`, `docs/cat3-conventions.md`, audit report | `cat3_numerical/cubic_kernel.py`, `cat3_numerical/generate_expected.py`, `tests/test_cat3_cubic_kernel.py` | `cat3_numerical/expected_values.toml` | ~80 LOC new, ~30 LOC modified, ~45 LOC removed |
| 3 | T1.4 | `docs/probe-template-conventions.md`, audit report | (none) | (none) | ~130 LOC new |
| 4 | SHA back-fill | (none) | 3 commit-landing audit reports | (none) | ~12 LOC modified |

Total: ~390 LOC across new + modified + removed. ~12 new tests
(T1.3) + 1 optional new test (T1.5). Four commits + push.

## 3. Commit 1 — T1.3 catalog auto-refresh script

### 3.1 Pre-commit

```bash
git pull --rebase origin main
git status   # working tree should be clean
git rev-parse HEAD   # record for the audit report
```

If `tools/integrity/docs/grandfather-catalog.md` has structural changes
since `df21312` — specifically, if the H3 heading format has changed
(e.g., new categories using `## ` H2 or no parenthetical) — **pause
and surface**.

If A.2 has landed commits past `df21312` that touch the catalog,
verify the catalog still uses the same heading format. Specifically:

```bash
grep -c "^### \`" tools/integrity/docs/grandfather-catalog.md
```

Should report 18 (per probe § B.1) or more (if A.2 added new
categories). If fewer, **pause and surface**.

### 3.2 New file: `tools/integrity/scripts/refresh_catalog_counts.py`

```python
#!/usr/bin/env python3
"""Refresh per-category counts in tools/integrity/docs/grandfather-catalog.md.

Reads grandfather-catalog.md, runs `python3 -m integrity --grandfather-report
--no-history-append`, parses the per-category counts from the report, and
updates each `### `<category>` (<count>)` heading's parenthetical to match
the report. Non-numeric parentheticals (e.g. `?` placeholder, free-prose forms)
are preserved verbatim.

Reports a category present-in-report-but-absent-from-catalog as an error;
the catalog is human-authored prose (each section explains WHY the category
is grandfathered) and a mechanical stub would be wrong-shaped.

Idempotent: re-running with no underlying changes produces zero diff.

Usage:
    python3 tools/integrity/scripts/refresh_catalog_counts.py
    python3 tools/integrity/scripts/refresh_catalog_counts.py --dry-run
    python3 tools/integrity/scripts/refresh_catalog_counts.py \\
        --catalog-path tools/integrity/docs/grandfather-catalog.md
"""

from __future__ import annotations

import argparse
import difflib
import re
import subprocess
import sys
from pathlib import Path

from integrity.common.repo import find_repo_root


CATALOG_DEFAULT = Path("tools/integrity/docs/grandfather-catalog.md")

# Per probe § B.1: heading shape is `### \`<category>\` (<count>)`.
HEADING_RE = re.compile(r"^### `(?P<cat>[a-z0-9-]+)` \((?P<count>.+?)\)\s*$")

# Per probe § B.2: report lines are `{cat:>35s}: {n}` after the
# `per-category counts:` label line.
REPORT_LINE_RE = re.compile(r"^\s*(?P<cat>[a-z0-9-]+):\s+(?P<count>\d+)\s*$")

# Numeric parenthetical = decimal non-negative integer; eligible for refresh.
NUMERIC_COUNT_RE = re.compile(r"^\d+$")


def fetch_report_counts(repo_root: Path) -> dict[str, int]:
    """Run --grandfather-report --no-history-append and parse the output.

    Returns a {category_name: count} map. Raises subprocess.CalledProcessError
    on non-zero exit; raises ValueError if a line matches REPORT_LINE_RE but
    fails int parsing (defensive against format drift).
    """
    result = subprocess.run(
        ["python3", "-m", "integrity",
         "--grandfather-report", "--no-history-append"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=True,
    )
    counts: dict[str, int] = {}
    in_per_category = False
    for line in result.stdout.splitlines():
        if line.strip() == "per-category counts:":
            in_per_category = True
            continue
        if not in_per_category:
            continue
        match = REPORT_LINE_RE.match(line)
        if match:
            cat = match.group("cat")
            try:
                counts[cat] = int(match.group("count"))
            except ValueError as e:
                raise ValueError(
                    f"refresh_catalog_counts: report line '{line!r}' matched "
                    f"the regex but count failed int parsing: {e}"
                ) from e
    return counts


def parse_catalog_headings(catalog_text: str) -> list[tuple[int, str, str, bool]]:
    """Parse all H3 category headings from catalog text.

    Returns a list of (line_index, category, count_str, is_numeric) tuples
    in document order. `line_index` is 0-based.
    """
    headings: list[tuple[int, str, str, bool]] = []
    for idx, line in enumerate(catalog_text.splitlines()):
        match = HEADING_RE.match(line)
        if match:
            cat = match.group("cat")
            count_str = match.group("count")
            is_numeric = bool(NUMERIC_COUNT_RE.match(count_str))
            headings.append((idx, cat, count_str, is_numeric))
    return headings


def build_refreshed_text(
    catalog_text: str,
    report_counts: dict[str, int],
) -> tuple[str, list[str], list[str]]:
    """Build the refreshed catalog text.

    Returns (refreshed_text, errors, updates):
        refreshed_text: the catalog with numeric parentheticals refreshed
        errors: list of error messages (e.g. categories in report but not catalog)
        updates: list of human-readable update descriptions for --dry-run output
    """
    lines = catalog_text.splitlines(keepends=True)
    headings = parse_catalog_headings(catalog_text)
    catalog_categories = {cat for _, cat, _, _ in headings}

    errors: list[str] = []
    updates: list[str] = []

    # Per probe § B.7 (2): report-with-no-heading is an error.
    for cat in report_counts:
        if cat not in catalog_categories:
            errors.append(
                f"category '{cat}' has count {report_counts[cat]} in "
                f"--grandfather-report but no heading in catalog "
                f"(add a `### `{cat}` (...)` section before refreshing)"
            )

    if errors:
        return catalog_text, errors, updates

    # Refresh numeric parentheticals; preserve everything else verbatim.
    for idx, cat, count_str, is_numeric in headings:
        if not is_numeric:
            # Non-numeric parenthetical preserved verbatim per § B.7 (1)/(3).
            continue
        report_count = report_counts.get(cat)
        if report_count is None:
            # Category in catalog but not in report; zero-finding case.
            # Per § B.7 (1) flavor: leave heading unchanged.
            continue
        if int(count_str) == report_count:
            # Already correct; no-op.
            continue
        # Rewrite this line.
        old_line = lines[idx]
        new_line = HEADING_RE.sub(
            lambda m, c=report_count: f"### `{m.group('cat')}` ({c})",
            old_line.rstrip("\n"),
        ) + ("\n" if old_line.endswith("\n") else "")
        lines[idx] = new_line
        updates.append(
            f"  {cat:>35s}: {count_str} -> {report_count}"
        )

    return "".join(lines), errors, updates


def render_diff(old_text: str, new_text: str, path: Path) -> str:
    """Render a unified diff for --dry-run output."""
    return "".join(
        difflib.unified_diff(
            old_text.splitlines(keepends=True),
            new_text.splitlines(keepends=True),
            fromfile=str(path),
            tofile=str(path) + " (refreshed)",
        )
    )


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Refresh grandfather-catalog.md per-category counts"
    )
    parser.add_argument("--dry-run", action="store_true",
                        help="Print the proposed diff without writing")
    parser.add_argument("--repo-root", type=Path, default=None,
                        help="Override the repo root (default: auto-detect)")
    parser.add_argument("--catalog-path", type=Path, default=None,
                        help=f"Override the catalog path "
                             f"(default: {CATALOG_DEFAULT})")
    ns = parser.parse_args(argv)

    repo_root = ns.repo_root if ns.repo_root else find_repo_root()
    catalog_path = (
        ns.catalog_path
        if ns.catalog_path
        else repo_root / CATALOG_DEFAULT
    )

    if not catalog_path.is_file():
        print(f"refresh_catalog_counts: catalog not found at {catalog_path}",
              file=sys.stderr)
        return 2

    try:
        report_counts = fetch_report_counts(repo_root)
    except subprocess.CalledProcessError as e:
        print(f"refresh_catalog_counts: --grandfather-report failed "
              f"(exit {e.returncode}):\n{e.stderr}", file=sys.stderr)
        return 3

    catalog_text = catalog_path.read_text(encoding="utf-8")
    refreshed_text, errors, updates = build_refreshed_text(
        catalog_text, report_counts
    )

    if errors:
        print("refresh_catalog_counts: errors found:", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        return 1

    if catalog_text == refreshed_text:
        print(f"refresh_catalog_counts: no changes needed "
              f"({len(report_counts)} categories checked)")
        return 0

    if ns.dry_run:
        print(f"refresh_catalog_counts: would update {len(updates)} headings:")
        for u in updates:
            print(u)
        print()
        print(render_diff(catalog_text, refreshed_text, catalog_path))
        return 0

    catalog_path.write_text(refreshed_text, encoding="utf-8")
    print(f"refresh_catalog_counts: updated {len(updates)} headings in "
          f"{catalog_path}:")
    for u in updates:
        print(u)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
```

### 3.3 New file: `tools/integrity/tests/test_refresh_catalog_counts.py`

```python
"""Tests for refresh_catalog_counts.py.

Covers parser correctness on the canonical and non-canonical heading shapes
surfaced by the v1.3 probe § B.1, the refresh logic's preservation of
non-numeric parentheticals, the error-on-missing-heading invariant, and the
idempotency property required by probe § B.7 (4).
"""

from __future__ import annotations

import io
import sys
import textwrap
from pathlib import Path

import pytest

# The script is in tools/integrity/scripts/, importable via path manipulation.
SCRIPTS_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPTS_DIR))

import refresh_catalog_counts as rcc  # noqa: E402


# ---------------------------------------------------------------------------
# Catalog heading parser tests
# ---------------------------------------------------------------------------


def test_parse_canonical_heading() -> None:
    """`### \\`foo\\` (42)` parses to (line_index, 'foo', '42', is_numeric=True)."""
    text = "### `foo` (42)\nbody\n"
    headings = rcc.parse_catalog_headings(text)
    assert headings == [(0, "foo", "42", True)]


def test_parse_placeholder_heading() -> None:
    """`### \\`toolkit-own-unused\\` (?)` parses with is_numeric=False (§ B.1 line 256 form)."""
    text = "### `toolkit-own-unused` (?)\nbody\n"
    headings = rcc.parse_catalog_headings(text)
    assert headings == [(0, "toolkit-own-unused", "?", False)]


def test_parse_prose_heading() -> None:
    """`### \\`other-cat1-bare-path\\` (0 swept; 44 live-source skipped)` parses with is_numeric=False (§ B.1 line 350 form)."""
    text = "### `other-cat1-bare-path` (0 swept; 44 live-source skipped)\nbody\n"
    headings = rcc.parse_catalog_headings(text)
    assert headings == [
        (0, "other-cat1-bare-path", "0 swept; 44 live-source skipped", False),
    ]


def test_parse_multiple_headings_in_document_order() -> None:
    """Multiple headings parse in document order with correct line indices."""
    text = textwrap.dedent("""\
        # Title
        ## Section
        body
        ### `alpha` (10)
        body
        ### `beta` (?)
        body
        ### `gamma` (5)
        """)
    headings = rcc.parse_catalog_headings(text)
    assert [(cat, count, is_num) for _, cat, count, is_num in headings] == [
        ("alpha", "10", True),
        ("beta", "?", False),
        ("gamma", "5", True),
    ]
    # Document-order line indices.
    assert [h[0] for h in headings] == [3, 5, 7]


def test_parse_ignores_h2_and_other_levels() -> None:
    """## and #### headings do NOT match (H3 only)."""
    text = textwrap.dedent("""\
        ## `alpha` (10)
        #### `beta` (20)
        ### `gamma` (5)
        """)
    headings = rcc.parse_catalog_headings(text)
    assert [h[1] for h in headings] == ["gamma"]


# ---------------------------------------------------------------------------
# Report-line parser tests
# ---------------------------------------------------------------------------


def test_report_line_regex_canonical() -> None:
    """Per probe § B.2, lines like `      foo: 42` parse correctly."""
    match = rcc.REPORT_LINE_RE.match("                            foo: 42")
    assert match is not None
    assert match.group("cat") == "foo"
    assert match.group("count") == "42"


def test_report_line_regex_rejects_summary_dict() -> None:
    """The summary dict line should NOT match (it contains braces and colons)."""
    match = rcc.REPORT_LINE_RE.match(
        "summary: {'pass': 5, 'soft_warn': 0, 'hard_fail': 53, 'suppressed': 1213}"
    )
    assert match is None


def test_report_line_regex_rejects_header() -> None:
    """The header line should NOT match."""
    match = rcc.REPORT_LINE_RE.match(
        "grandfather report @ df21312 (2026-05-16T01:19:02+00:00)"
    )
    assert match is None


# ---------------------------------------------------------------------------
# Refresh logic tests
# ---------------------------------------------------------------------------


def test_refresh_updates_numeric_count() -> None:
    """Numeric parenthetical gets refreshed to match report."""
    catalog = "### `audit-citation` (597)\nbody\n"
    report = {"audit-citation": 99}
    refreshed, errors, updates = rcc.build_refreshed_text(catalog, report)
    assert errors == []
    assert refreshed == "### `audit-citation` (99)\nbody\n"
    assert len(updates) == 1


def test_refresh_preserves_placeholder_verbatim() -> None:
    """`(?)` placeholder is preserved verbatim even if report has count for it."""
    catalog = "### `toolkit-own-unused` (?)\nbody\n"
    # Even if the report has a numeric count for this category, the catalog's
    # non-numeric parenthetical is preserved per § B.7 (1).
    report = {"toolkit-own-unused": 42}
    refreshed, errors, updates = rcc.build_refreshed_text(catalog, report)
    assert errors == []
    assert refreshed == catalog  # unchanged
    assert updates == []


def test_refresh_preserves_prose_verbatim() -> None:
    """Two-number-prose parenthetical is preserved verbatim per § B.7 (3)."""
    catalog = "### `other-cat1-bare-path` (0 swept; 44 live-source skipped)\nbody\n"
    report = {}  # not in report
    refreshed, errors, updates = rcc.build_refreshed_text(catalog, report)
    assert errors == []
    assert refreshed == catalog
    assert updates == []


def test_refresh_errors_on_missing_heading() -> None:
    """Category in report but not in catalog raises an error per § B.7 (2)."""
    catalog = "### `alpha` (10)\nbody\n"
    report = {"alpha": 10, "missing-cat": 5}
    refreshed, errors, updates = rcc.build_refreshed_text(catalog, report)
    assert refreshed == catalog  # unchanged on error
    assert len(errors) == 1
    assert "missing-cat" in errors[0]
    assert updates == []


def test_refresh_idempotent_when_already_correct() -> None:
    """Already-correct catalog produces zero changes per § B.7 (4)."""
    catalog = "### `alpha` (10)\n### `beta` (20)\nbody\n"
    report = {"alpha": 10, "beta": 20}
    refreshed, errors, updates = rcc.build_refreshed_text(catalog, report)
    assert errors == []
    assert refreshed == catalog
    assert updates == []


def test_refresh_idempotent_after_first_refresh() -> None:
    """Two consecutive refreshes produce identical output."""
    catalog = "### `alpha` (5)\n### `beta` (10)\nbody\n"
    report = {"alpha": 99, "beta": 10}
    first, errors_1, _ = rcc.build_refreshed_text(catalog, report)
    assert errors_1 == []
    second, errors_2, updates_2 = rcc.build_refreshed_text(first, report)
    assert errors_2 == []
    assert second == first
    assert updates_2 == []


def test_refresh_zero_count_in_catalog_not_in_report_preserved() -> None:
    """Category in catalog but absent from report is preserved unchanged."""
    catalog = "### `dormant-cat` (0)\nbody\n"
    report = {}  # category not emitted (zero-finding)
    refreshed, errors, updates = rcc.build_refreshed_text(catalog, report)
    assert errors == []
    assert refreshed == catalog
    assert updates == []


def test_refresh_handles_mixed_numeric_and_nonnumeric() -> None:
    """A catalog with mixed numeric, placeholder, and prose parentheticals refreshes only numeric."""
    catalog = textwrap.dedent("""\
        ### `audit-citation` (597)
        ### `toolkit-own-unused` (?)
        ### `other-cat1-bare-path` (0 swept; 44 live-source skipped)
        ### `audit-bare-path` (635)
        """)
    report = {
        "audit-citation": 99,
        "audit-bare-path": 729,
    }
    refreshed, errors, updates = rcc.build_refreshed_text(catalog, report)
    assert errors == []
    expected = textwrap.dedent("""\
        ### `audit-citation` (99)
        ### `toolkit-own-unused` (?)
        ### `other-cat1-bare-path` (0 swept; 44 live-source skipped)
        ### `audit-bare-path` (729)
        """)
    assert refreshed == expected
    assert len(updates) == 2


# ---------------------------------------------------------------------------
# Integration tests (in-process — no subprocess)
# ---------------------------------------------------------------------------


def test_script_imports_cleanly() -> None:
    """The script module imports without side effects."""
    # If we got here, the top-level `import refresh_catalog_counts` succeeded.
    assert hasattr(rcc, "main")
    assert hasattr(rcc, "fetch_report_counts")
    assert hasattr(rcc, "build_refreshed_text")
    assert hasattr(rcc, "parse_catalog_headings")
```

### 3.4 Modified file: `tools/integrity/docs/grandfather-catalog.md`

Probe § B.1 quoted lines 13–25 (the `## Updating counts` block).
Current text ends with: `Auto-refresh from the history file is a v1.2
candidate.`

Replace the entire `## Updating counts` block (whatever its current
line numbers; locate via `grep -n "^## Updating counts"`) with:

```markdown
## Updating counts

The per-category counts in the headings below reflect the most recent
run of the auto-refresh script:

    python3 tools/integrity/scripts/refresh_catalog_counts.py

The script reads `python3 -m integrity --grandfather-report
--no-history-append`'s output and rewrites each numeric `(N)`
parenthetical in this file's category headings. Non-numeric
parentheticals (e.g. `(?)` placeholders or free-prose forms like
`(0 swept; 44 live-source skipped)`) are preserved verbatim.

To preview proposed changes without writing:

    python3 tools/integrity/scripts/refresh_catalog_counts.py --dry-run

The script errors out if any category in the report lacks a
corresponding heading in this file — that signals a missed
grandfather-catalog entry that needs human authoring (the catalog's
per-category prose explains WHY each category is grandfathered and
should not be mechanically stubbed).

Run the refresh whenever catalog counts drift far enough to mislead
a reader. The v1.3 candidates roadmap § 4 T1.3 banked the
auto-refresh as the resolution for the manual-refresh debt v1.1
batch-1 retro § 5.5 quantified at +6.7% drift per batch cycle.
```

### 3.5 Verification block — commit 1

Before committing:

```bash
cd /home/otacon/Projects/GPU-Sims/GPU-Sims

# All new tests pass.
cd tools/integrity
python3 -m pytest tests/test_refresh_catalog_counts.py -v
# Expected: 12 tests pass.

# Existing test suite still passes.
python3 -m pytest tests/ -v --tb=short
# Expected: all existing tests + 12 new = clean.

# Script runs cleanly against current disk (in-process integration).
cd /home/otacon/Projects/GPU-Sims/GPU-Sims
python3 tools/integrity/scripts/refresh_catalog_counts.py --dry-run
# Expected output: "would update <N> headings:" with the drift table
# from probe § B.6 (or its post-df21312 evolution if A.2 commits
# have landed in between). The diff should show only parenthetical
# changes; no other edits.

# Idempotent confirmation.
python3 tools/integrity/scripts/refresh_catalog_counts.py
# Expected: "updated <N> headings: ..." OR "no changes needed".
python3 tools/integrity/scripts/refresh_catalog_counts.py
# Expected: "no changes needed (... categories checked)". The second
# run produces zero output diff.

# Gate stays at baseline (or within audit-doc growth budget).
python3 -m integrity --mode strict --no-audit-log
# Expected: hard-fail count = pre-commit baseline + new audit-doc
# bare-paths from this commit's audit report.

# Inline sweep companion (sweeps the new audit-doc findings only).
python3 tools/integrity/scripts/grandfather_sweep.py
# Expected: sweeps the audit report's bare-paths; "skipped as
# live-source: <N>" line present and matches pre-commit baseline.

# Re-run gate post-sweep.
python3 -m integrity --mode strict --no-audit-log
# Expected: hard-fail count = pre-commit baseline (audit-doc findings
# now suppressed); suppressed count increased by the swept findings.

# Re-run refresh (post-sweep, post-inline-edits to catalog) to confirm
# the catalog is current.
python3 tools/integrity/scripts/refresh_catalog_counts.py
# Expected: "no changes needed" OR small drift from this commit's
# own sweep updating audit-bare-path / audit-citation counts.
```

If `pytest` reports any failure, **pause and surface** — do not
commit in a failed state.

If the refresh script reports an error (missing heading), the
expected cause is that the report includes a category not yet
documented in the catalog. **Pause and surface** — this signals a
missed grandfather-catalog entry that needs human authoring.

### 3.6 Audit report — commit 1

Create
`docs/diagnostics/_audits/integrity_v1_3_commit1_landing_2026-05-16.md`.

Front-matter:

```yaml
---
title: "Integrity v1.3 Commit 1 — T1.3 Catalog Auto-Refresh Script"
date: 2026-05-16
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_t1_3_5_probe_2026-05-16_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_t1_3_5_spec_2026-05-16_architect1.md
  - docs/retro/integrity-toolkit-v1.3-candidates.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
---
```

Body sections (mirror v1.2 commit-landing format):

- **§ A. Change summary** — One paragraph: T1.3 lands the auto-refresh
  script per roadmap § 4 T1.3 and probe § B.7's six design decisions.
  Cite the v1.1 batch-1 retro § 5.5 drift estimate (+6.7%) and probe
  § B.6's empirical drift table (audit-citation: -498, audit-bare-path:
  +94, etc.) as the empirical case.
- **§ B. File inventory** — Files created
  (`scripts/refresh_catalog_counts.py`, `tests/test_refresh_catalog_counts.py`,
  this audit report); files modified
  (`docs/grandfather-catalog.md` prose-only update at `## Updating counts`).
  Diff stat per file.
- **§ C. Verification** — Verbatim capture of pytest output (12 tests
  pass), script dry-run output (with the drift table), idempotency
  check output, gate state before/after.
- **§ D. Design decisions applied** — Cross-reference each of probe
  § B.7's six recommendations to the implementation (which function
  in the script handles each case, which test pins the behavior).
- **§ E. Banked observations** — None expected for this commit; if any
  surfaced during execution (e.g., new heading variants on disk that
  the parser had to accommodate), record here.
- **§ F. Cross-references** — Probe § B sections (B.1 heading shape,
  B.2 report format, B.3 subprocess decision, B.6 drift table, B.7
  design choices).
- **§ G. Next commit** — Pointer to commit 2 (T1.5).

### 3.7 Commit message — commit 1

```
feat(integrity): T1.3 catalog auto-refresh script (v1.3 commit 1)

A new script at tools/integrity/scripts/refresh_catalog_counts.py
refreshes the per-category counts in grandfather-catalog.md from
the output of `python3 -m integrity --grandfather-report
--no-history-append`. Idempotent. In-place edit with --dry-run.

Surfaced by v1.1 batch-1 retro § 5.5: catalog counts drift +6.7%
per batch cycle under concurrent audit-doc churn; v1.3 batch
empirically measured -498 audit-citation drift (597 -> 99) after
A.3's bare-path re-categorization, plus +94 audit-bare-path
(635 -> 729) from continued audit-doc growth.

Design per pre-spec probe § B.7:
  - Subprocess --grandfather-report (not direct import); see § B.3.
  - Preserve non-numeric parentheticals (?, prose) verbatim.
  - Error out on report-with-no-heading (catalog is human-authored).
  - In-place edit with --dry-run flag.

Tests: +12 in tests/test_refresh_catalog_counts.py covering the
heading parser (canonical + placeholder + prose forms), the
report-line parser, the refresh logic (preserve/update/error
branches), and idempotency.

Catalog prose update: `## Updating counts` block rewritten to
describe the auto-refresh script as the canonical workflow.
```

### 3.8 Commit and push — commit 1

```bash
git pull --rebase origin main
git add tools/integrity/scripts/refresh_catalog_counts.py \
        tools/integrity/tests/test_refresh_catalog_counts.py \
        tools/integrity/docs/grandfather-catalog.md \
        docs/diagnostics/_audits/integrity_v1_3_commit1_landing_2026-05-16.md
git status   # verify only the expected paths are staged
git commit -F <commit-message-file>
git push origin main
git rev-parse HEAD   # record for SHA back-fill (commit 4)
```

If `git status` shows unexpected files staged, **pause and surface**.

## 4. Commit 2 — T1.5 TOML → JSON convergence

### 4.1 Pre-commit

```bash
git pull --rebase origin main
git status
git rev-parse HEAD
```

Verify the cat3 surface hasn't shifted since `df21312`:

```bash
ls tools/integrity/integrity/cat3_numerical/
# Expected files: __init__.py, cubic_kernel.py, d3q19_verify.py,
#   d3q19_equilibrium.expected.json, expected_values.toml,
#   generate_expected.py, checks/

test -f tools/integrity/integrity/cat3_numerical/expected_values.toml || \
  echo "PAUSE: expected_values.toml not at probed location"
test -f tools/integrity/integrity/cat3_numerical/expected_values.json && \
  echo "PAUSE: expected_values.json already exists pre-commit"
```

If either pause message fires, **pause and surface**.

### 4.2 File operations

#### 4.2.1 New file: `tools/integrity/integrity/cat3_numerical/expected_values.json`

```json
{
  "schema_version": 1,
  "source": "tools/integrity/integrity/cat3_numerical/generate_expected.py",
  "derivation": "SPlisHSPlasH 2.16.1 SPHKernels.h:43-85",
  "anchor_sha": "6bff55a6eaf14083d34650f22a268ce156b62b54",
  "tolerance": {
    "atol": 1e-5,
    "rtol": 1e-5
  },
  "test_points": [
    {
      "q": 0.0,
      "h": 1.0,
      "expected_W": 2.54647908947033,
      "expected_gradW_magnitude": 0
    },
    {
      "q": 0.1,
      "h": 1.0,
      "expected_W": 2.40896921863893,
      "expected_gradW_magnitude": 2.59740867125973
    },
    {
      "q": 0.25,
      "h": 1.0,
      "expected_W": 1.8302818455568,
      "expected_gradW_magnitude": 4.77464829275686
    },
    {
      "q": 0.5,
      "h": 1.0,
      "expected_W": 0.636619772367581,
      "expected_gradW_magnitude": 3.81971863420549
    },
    {
      "q": 0.75,
      "h": 1.0,
      "expected_W": 0.0795774715459477,
      "expected_gradW_magnitude": 0.954929658551372
    },
    {
      "q": 1.0,
      "h": 1.0,
      "expected_W": 0,
      "expected_gradW_magnitude": 0
    }
  ]
}
```

Numeric values are transcribed byte-for-byte from probe § D.1's TOML
verbatim. The `0` integer literals are preserved as-is (the harness's
`float()` cast handles either type).

#### 4.2.2 Removed file: `tools/integrity/integrity/cat3_numerical/expected_values.toml`

```bash
git rm tools/integrity/integrity/cat3_numerical/expected_values.toml
```

Per Decision 6 / probe § D.8. Do NOT use `git mv`; the content type
change makes rename detection misleading.

#### 4.2.3 Modified file: `tools/integrity/integrity/cat3_numerical/cubic_kernel.py`

Per probe § D.7, four touch points. Apply each edit verbatim against
disk (verify with `grep -n` before each):

**Edit A — module docstring (probe § D.2 line 3 region):**

Old text (verify on disk):
```
Reads expected values from expected_values.toml, runs the Stack C
driver binary at build/tools/integrity/drivers/integrity_cat3_stack_c/,
```

New text:
```
Reads expected values from expected_values.json, runs the Stack C
driver binary at build/tools/integrity/drivers/integrity_cat3_stack_c/,
```

**Edit B — `tomllib` import (probe § D.2 line 19):**

Old text:
```python
import tomllib
```

New text:
```python
import json
```

**Edit C — `EXPECTED_VALUES_RELATIVE` constant (probe § D.2 line 27-29):**

Old text:
```python
EXPECTED_VALUES_RELATIVE = Path(
    "tools/integrity/integrity/cat3_numerical/expected_values.toml"
)
```

New text:
```python
EXPECTED_VALUES_RELATIVE = Path(
    "tools/integrity/integrity/cat3_numerical/expected_values.json"
)
```

**Edit D — `load_expected_values` function (probe § D.2 lines 48-64):**

Old text (verify on disk; entire function body):
```python
def load_expected_values(repo_root: Path) -> tuple[list[TestPoint], dict]:
    """Parse expected_values.toml. Returns (test_points, tolerance_dict)."""
    path = repo_root / EXPECTED_VALUES_RELATIVE
    if not path.is_file():
        return [], {}

    data = tomllib.loads(path.read_text(encoding="utf-8"))
    tolerance = data.get("tolerance", {"atol": 1e-5, "rtol": 1e-5})
    points: list[TestPoint] = []
    for tp in data.get("test_points", []):
        points.append(TestPoint(
            q=float(tp["q"]),
            h=float(tp["h"]),
            expected_W=float(tp["expected_W"]),
            expected_gradW_magnitude=float(tp["expected_gradW_magnitude"]),
        ))
    return points, tolerance
```

New text:
```python
def load_expected_values(repo_root: Path) -> tuple[list[TestPoint], dict]:
    """Parse expected_values.json. Returns (test_points, tolerance_dict).

    See tools/integrity/docs/cat3-conventions.md for the file-format
    convention (JSON-as-canonical for cat3 expected-values files).
    """
    path = repo_root / EXPECTED_VALUES_RELATIVE
    if not path.is_file():
        return [], {}

    data = json.loads(path.read_text(encoding="utf-8"))
    tolerance = data.get("tolerance", {"atol": 1e-5, "rtol": 1e-5})
    points: list[TestPoint] = []
    for tp in data.get("test_points", []):
        points.append(TestPoint(
            q=float(tp["q"]),
            h=float(tp["h"]),
            expected_W=float(tp["expected_W"]),
            expected_gradW_magnitude=float(tp["expected_gradW_magnitude"]),
        ))
    return points, tolerance
```

The only semantic changes are `tomllib.loads` → `json.loads` and the
docstring update. Function signature, return type, and TestPoint
construction are unchanged.

#### 4.2.4 Modified file: `tools/integrity/integrity/cat3_numerical/generate_expected.py`

Per probe § D.4 + § D.7, three touch points plus the TOML-emission
logic in `main()`. Apply each edit:

**Edit A — module docstring (probe § D.7 line 2):**

Old text (verify):
```
Generator for expected_values.toml — analytic re-derivation of the cubic
SPH kernel test points.
```

New text:
```
Generator for expected_values.json — analytic re-derivation of the cubic
SPH kernel test points. See tools/integrity/docs/cat3-conventions.md.
```

(Exact wording of the existing line varies; verify with `grep -n
"expected_values" tools/integrity/integrity/cat3_numerical/generate_expected.py`
and replace each .toml mention with .json.)

**Edit B — `OUTPUT_PATH` constant (probe § D.4 line 28):**

Old text:
```python
OUTPUT_PATH = SCRIPT_DIR / "expected_values.toml"
```

New text:
```python
OUTPUT_PATH = SCRIPT_DIR / "expected_values.json"
```

**Edit C — `main()` body (TOML literal-text emission).**

Per probe § D.4: `main()` builds a `lines: list[str]` of literal TOML
text and writes to `OUTPUT_PATH`. Replace the entire emission logic
with JSON serialization. The expected structure is the same as the
new `expected_values.json` content from § 4.2.1.

Locate the `main()` body via `grep -n "^def main\\|^if __name__" tools/integrity/integrity/cat3_numerical/generate_expected.py`
and replace the body. Suggested shape:

```python
def main() -> int:
    test_points = []
    for q in TEST_POINTS_Q:
        test_points.append({
            "q": q,
            "h": H,
            "expected_W": cubic_W(q, H),
            "expected_gradW_magnitude": cubic_gradW_magnitude(q, H),
        })

    payload = {
        "schema_version": 1,
        "source": "tools/integrity/integrity/cat3_numerical/generate_expected.py",
        "derivation": "SPlisHSPlasH 2.16.1 SPHKernels.h:43-85",
        "anchor_sha": "6bff55a6eaf14083d34650f22a268ce156b62b54",
        "tolerance": {"atol": 1e-5, "rtol": 1e-5},
        "test_points": test_points,
    }

    OUTPUT_PATH.write_text(
        json.dumps(payload, indent=2) + "\n",
        encoding="utf-8",
    )
    print(f"wrote {OUTPUT_PATH.relative_to(SCRIPT_DIR.parents[3]) if OUTPUT_PATH.is_relative_to(SCRIPT_DIR.parents[3]) else OUTPUT_PATH}")
    return 0
```

Verify the rest of `main()` (any pre-write print statements, the
`__name__ == "__main__"` guard, the analytic helpers `cubic_W` /
`cubic_gradW_magnitude` / `TEST_POINTS_Q` / `H`) is unchanged. Add
`import json` at the top of the file if not already present.

After the edit, **run the generator and verify the output matches
§ 4.2.1 byte-for-byte:**

```bash
python3 tools/integrity/integrity/cat3_numerical/generate_expected.py
diff tools/integrity/integrity/cat3_numerical/expected_values.json \
     <(echo "<content from § 4.2.1>")
# Expected: zero diff. If diff is non-empty, the generator output
# disagrees with the committed file; pause-and-surface.
```

#### 4.2.5 Modified file: `tools/integrity/tests/test_cat3_cubic_kernel.py`

Per probe § D.6, three test docstrings reference `expected_values.toml`
literally. Update each via search-and-replace:

```bash
sed -i 's/expected_values\.toml/expected_values.json/g' \
  tools/integrity/tests/test_cat3_cubic_kernel.py
```

Or apply equivalent edits manually. Verify with:

```bash
grep -n "expected_values" tools/integrity/tests/test_cat3_cubic_kernel.py
# Expected: no `.toml` matches remain.
```

No new tests are required for T1.5; the existing
`test_load_expected_values_real_file`,
`test_load_expected_values_specific_q`, and
`test_load_expected_values_q_at_support_boundary` cover the
file-format swap by virtue of going through `load_expected_values()`.

#### 4.2.6 New file: `tools/integrity/docs/cat3-conventions.md`

```markdown
# Cat 3 — Numerical Correctness Check Conventions

This doc records conventions for cat3 (numerical correctness) checks
that go beyond what `docs/integrity-toolkit-spec.md` § 8 covers. New
conventions land here as they are surfaced by cat3 work.

## Expected-values file format

Cat 3 checks that consume machine-generated expected-values files
SHOULD use JSON, not TOML. The canonical schema mirrors the format
used by `d3q19_equilibrium.expected.json`:

```
{
  "schema_version": <int>,
  "source": "<repo-relative path of generator script>",
  "derivation": "<vendored-source path with line range, OR algebraic doc reference>",
  "anchor_sha": "<vendored-source anchor SHA — optional for purely algebraic derivations>",
  "tolerance": {"atol": <float>, "rtol": <float>},
  "test_points": [
    {<per-test-point expected-vs-actual data>},
    ...
  ]
}
```

Optional extension fields per check (e.g. `velocity_set`, `weights`,
`opposite_index` for d3q19). Check-specific fields go alongside
`test_points`, not nested inside it, unless the check's natural
structure is nested.

### Rationale

JSON for cat3 expected-values files (not TOML):

1. **Machine-generation is the common case.** Algebraic re-derivation
   harnesses (`d3q19_verify.py`, `generate_expected.py`) emit data
   structures via `json.dumps` directly, without round-tripping
   through TOML serialization. TOML's value here was its
   human-authorability for v1's hand-curated cubic-kernel data; that
   value diminishes as cat3 expands toward more harness-driven
   derivations.
2. **Universal Python support.** JSON parsing is in the standard
   library across every supported Python version; `tomllib` requires
   Python 3.11+.
3. **Stack consistency.** The toolkit already emits JSON via
   `--output json` for finding payloads; using JSON for cat3
   expected-values reduces the number of structured-data formats the
   toolkit reasons about.

### Comments and provenance

JSON has no comment syntax. Provenance fields the v1 TOML emitted as
top-of-file comments (anchor SHA, generator pointer, upstream-source
pointer) move to top-level keys (`source`, `derivation`,
`anchor_sha`). Free-form notes that don't fit the schema can go in an
optional `_comment` key, but prefer adding a named schema field if
the note is structurally meaningful.

### Existing cat3 expected-values files

- `tools/integrity/integrity/cat3_numerical/expected_values.json` —
  cubic-kernel; schema_version 1; anchor SHA from SPlisHSPlasH 2.16.1.
  Generated by `tools/integrity/integrity/cat3_numerical/generate_expected.py`.
- `tools/integrity/integrity/cat3_numerical/d3q19_equilibrium.expected.json` —
  D3Q19 BGK equilibrium; schema_version 1; derivation is the algebraic
  doc at `tools/integrity/docs/algebraic/d3q19.md`. Generated by
  `tools/integrity/integrity/cat3_numerical/d3q19_verify.py`.

The v1 TOML format (`expected_values.toml`) was removed in v1.3 batch-1
part-A; per this convention, future cat3 expected-values files use
`.json`.
```

### 4.3 Verification block — commit 2

```bash
cd /home/otacon/Projects/GPU-Sims/GPU-Sims

# Existing cat3 tests still pass with the JSON file in place.
cd tools/integrity
python3 -m pytest tests/test_cat3_cubic_kernel.py -v
# Expected: all existing tests pass (no test changes; the swap is
# format-internal to load_expected_values).

# Generator round-trip: regenerate the JSON and confirm it matches
# the committed file byte-for-byte.
cd /home/otacon/Projects/GPU-Sims/GPU-Sims
python3 tools/integrity/integrity/cat3_numerical/generate_expected.py
git diff --quiet tools/integrity/integrity/cat3_numerical/expected_values.json
# Expected: zero diff (exit 0). If diff is non-empty, pause-and-surface.

# Verify no .toml references remain anywhere in the toolkit.
grep -rn "expected_values\.toml" tools/integrity/ docs/
# Expected: no matches. (Some references in old audit reports under
# docs/diagnostics/_audits/ are acceptable as historical record.)
grep -rn "expected_values\.toml" tools/integrity/
# Expected: zero matches in toolkit code/tests/docs proper.

# The check itself runs with the new JSON.
python3 -m integrity --check cat3.cubic-kernel --no-audit-log
# Expected: same behavior as pre-commit (0 findings if driver built,
# graceful-degrade to 0 findings if not).

# Gate stays at baseline.
python3 -m integrity --mode strict --no-audit-log
# Expected: hard-fail count = pre-commit baseline + new audit-doc
# bare-paths from this commit's audit report + cat3-conventions.md.

# Inline sweep companion.
python3 tools/integrity/scripts/grandfather_sweep.py
# Expected: sweeps the new audit-doc findings; "skipped as live-source"
# count unchanged from pre-commit.

# Run the new refresh script (from commit 1) to confirm catalog stays
# current after this commit's sweep.
python3 tools/integrity/scripts/refresh_catalog_counts.py
# Expected: small update (audit-bare-path count increased by the sweep)
# OR "no changes needed".
```

If `pytest` reports any failure, **pause and surface**.

If the generator round-trip produces a diff, the JSON content in
§ 4.2.1 disagrees with the generator output; that's a spec-time
fabrication on my part. **Pause and surface** with the diff captured;
fix forward by updating the generator's output to match § 4.2.1 OR
update § 4.2.1 to match the generator (whichever is correct —
mathematically the values are determined by the analytic helpers).

### 4.4 Audit report — commit 2

Path:
`docs/diagnostics/_audits/integrity_v1_3_commit2_landing_2026-05-16.md`.

Front-matter mirrors commit 1's. Body sections:

- **§ A. Change summary** — T1.5 converges cat3 expected-values
  format to JSON per probe § C.4 / roadmap § 4 T1.5. Cite the v1.2
  bolt-ons probe § C.4 as the originating observation. Note that the
  d3q19 file's existing JSON schema is the precedent and the
  cubic-kernel file's TOML is the outlier the convergence targets.
- **§ B. File inventory** — 2 new files (JSON data, cat3-conventions
  doc), 1 removed file (TOML), 3 modified files (cubic_kernel.py,
  generate_expected.py, test_cat3_cubic_kernel.py), 1 audit report.
  Diff stat per file.
- **§ C. Verification** — Per-test pass output, generator round-trip
  byte-comparison, no-.toml-references confirmation, gate state.
- **§ D. Design decisions applied** — Cross-reference Decision 6
  (`git rm + git add`) and Decision 7 (JSON schema mirroring d3q19),
  citing probe § D.8 / § D.5.
- **§ E. Banked observations** — None expected. If a generator
  round-trip mismatch surfaced and was resolved, record here.
- **§ F. Cross-references** — Probe § D.1–§ D.10; bolt-ons probe
  § C.4 (originating observation).
- **§ G. Next commit** — Pointer to commit 3 (T1.4).

### 4.5 Commit message — commit 2

```
refactor(integrity): T1.5 cat3 expected-values TOML -> JSON (v1.3 commit 2)

Converges cat3 expected-values file format to JSON. cubic-kernel's
expected_values.toml is removed and replaced with expected_values.json
following the d3q19 schema (schema_version, source, derivation,
anchor_sha, tolerance, test_points). The harness's load_expected_values
swaps tomllib.loads for json.loads; the generator emits JSON via
json.dumps. Function signature and TestPoint construction unchanged.

New doc at tools/integrity/docs/cat3-conventions.md records JSON-as-
canonical-format for future cat3 expected-values files. The doc is
also positioned as a growth path for future cat3 conventions.

Per the v1.2 bolt-ons probe § C.4 and v1.3 candidates roadmap § 4
T1.5: machine-generation is the common case (TOML's hand-authorability
diminishes as cat3 expands), JSON has universal Python support, and
the toolkit already emits JSON for --output json finding payloads.

Numeric values transcribed byte-for-byte from the TOML source per
probe § D.1; generator round-trip verified byte-identical with the
committed file.

No test changes beyond docstring updates (the format swap is internal
to load_expected_values, which keeps its existing signature).
```

### 4.6 Commit and push — commit 2

```bash
git pull --rebase origin main
git add -A   # captures new files, modified files, and the .toml removal
git status   # verify only expected paths are staged
git commit -F <commit-message-file>
git push origin main
git rev-parse HEAD
```

## 5. Commit 3 — T1.4 probe template conventions doc

### 5.1 Pre-commit

```bash
git pull --rebase origin main
git status
git rev-parse HEAD
```

Verify the target doc location doesn't already exist:

```bash
test ! -f tools/integrity/docs/probe-template-conventions.md || \
  echo "PAUSE: probe-template-conventions.md already exists pre-commit"
```

If the pause message fires, **pause and surface**.

### 5.2 New file: `tools/integrity/docs/probe-template-conventions.md`

```markdown
# Probe Template Conventions

This doc records the probe-template conventions banked by v1.1
batch-1 retro § 7.2 as items C and D. Both conventions emerged from
real architect-1 fabrications during v1.1 batch-1 execution
(pause-and-surface #1 and #2). They were carried forward into the
v1.2 A.3 / A.2 probe designs and validated. This doc is the canonical
home for the convention text plus worked examples; the v1.1 batch-1
retro § 7.2 framing remains the originating record.

The conventions are scoped to integrity-toolkit pre-spec probes
specifically. They are useful elsewhere, but the worked-example
evidence is integrity-toolkit-internal.

---

## Convention C — Path-resolution enumeration

**When the spec proposes to do path resolution (mapping one path
shape to another, e.g. header → impl, or basename → repo-resolved
path), the pre-spec probe MUST enumerate three to five representative
input-output pairs from the synced repo state, drawn from the actual
code or directory layout that will inform the spec.**

The probe's job here is to make the path-resolution rule discoverable
from probe data rather than from architect-1 prior assumption.

### Failure mode this prevents

The v1.1 batch-1 commit-1 `stub_label_stale.py` check landed with
Decision 2 asserting a 1:1 mirror between `include/<sub>/<base>.hpp`
and `src/<sub>/<base>.cpp`. Synced repo state had
`include/gpusims/...` stripping the `gpusims/` namespace component in
the `src` tree — i.e., `include/gpusims/alembic_writer.hpp` maps to
`src/alembic_writer.cpp`, not `src/gpusims/alembic_writer.cpp`. The
check's first execution attempt resolved zero impl paths and would
have missed both canonical target cases. The fabrication was caught
at execution time (Hard Rule 2) but only after the spec had been
drafted, reviewed, and approved.

The pre-spec apispec probe enumerated verbatim source listings of
relevant modules but did not enumerate any header→impl path pairs.
The spec drafter (architect-1) filled in the convention from prior
assumption rather than from probe data. That assumption was wrong.

(Source: `docs/diagnostics/_audits/integrity_v1_1_commit1_landing_2026-05-15.md`
§ E.1 + v1.1 batch-1 retro § 3.1.)

### Examples of Convention C followed

**Example 1 — A.3 probe** (path-resolution rules for bare basenames).
`docs/diagnostics/_audits/integrity_v1_2_a3_probe_2026-05-15_architect1.md`
§ A.3 verbatim-dumps `cat1_citations/resolver.py` (the existing
intra-repo path resolver) and includes an INFERENCE block explicitly
naming the false-positive class the resolver accidentally
accommodates (it succeeds on bare basenames whenever a sibling file
matches). The probe-time enumeration is precisely what Convention C
asks for: the path-resolution rules dumped verbatim from the source
under inspection, before the spec drafter writes new convention text.

**Example 2 — A.2 probe** (toolkit internal cross-module imports).
`docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md`
§ C.1 verbatim-enumerates every `from integrity.X import Y` edge in
the toolkit's internal cross-module graph. This is the toolkit's
analog of path-pair enumeration: every consumer-of-X edge is dumped,
so the spec drafter cannot fabricate an import path that doesn't
exist. The A.2 probe's analog of the v1.1 commit-1 fabrication would
have been asserting that `cat2_contracts/checks/foo.py` exists when
it doesn't; C.1's enumeration directly forecloses that class.

---

## Convention D — Call-site enumeration

**When the spec proposes a behavioral change to a function, method,
or module-level helper, the pre-spec probe MUST enumerate every call
site of the changed item in the synced repo state, with verbatim
context for each call site sufficient to evaluate whether the
behavioral change is compatible with that call site's expectations.**

The probe's job here is to make the change's blast radius
discoverable from probe data rather than from architect-1 local
knowledge of "the obvious caller."

### Failure mode this prevents

The v1.1 batch-1 commit-2 `cat1.annotation-form` check landed with
Decision 6 scoping the markdown-content scan to a single check
module. Other cat1 checks (intra-repo, upstream-citation) also scan
markdown files; the scope was too narrow and the check missed
findings the spec author hadn't realized were in scope.

The apispec probe verbatim-dumped the annotation parser and the
annotation check but did not enumerate which other cat1 checks scan
markdown content. The spec drafter scoped Decision 6 from local
knowledge of one module instead of from probe data on all relevant
modules.

(Source: v1.1 batch-1 retro § 3.2.)

### Examples of Convention D followed

**Example 1 — v1.2 bolt-ons probe** (emit_output asymmetry).
`docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md`
§ D.1 enumerates `emit_output` and `_emit_human_summary` declarations
plus all callers in `runner.py`. § D.2 then dumps both functions
verbatim, proving the asymmetry between the github branch (which
filtered suppressed findings) and the human branch (which did not).
This is exactly what Convention D asks for: enumerate every call
site of the function the spec proposes to modify, before writing the
fix. Line numbers in the bolt-ons probe's § D.1 are stale relative
to current disk (the bolt-ons fix added 6 lines); the
methodology survives the line-number drift, the specific citations
do not.

**Example 2 — A.2 probe** (internal + external consumer enumeration).
`docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md`
§ C.1 enumerates every internal import edge in the toolkit. § C.2
enumerates external consumers (scripts + tests + docs), with an
INFERENCE block distinguishing real consumers from fenced-code-listing
pseudo-consumers. The dual enumeration is what kept the A.2 spec from
fabricating a nonexistent caller in its design discussion: every
caller's actual import shape was visible in the probe before the
spec wrote new design text.

---

## How to apply these conventions

When you author a pre-spec probe:

1. **Audit the spec's expected behavioral changes.** For each
   proposed change to path-resolution logic, add a probe section
   enumerating 3–5 representative path pairs from the synced repo
   (Convention C). For each proposed change to a function's
   behavior, add a probe section enumerating every call site
   (Convention D).
2. **Dump verbatim.** Both conventions are explicit: dump the source,
   don't paraphrase. Paraphrase introduces a translation layer that
   can leak architect-1 prior assumption back in.
3. **Tag with anchor SHA.** Line numbers drift over time; the
   probe-time anchor SHA is the load-bearing claim. Worked examples
   in this doc deliberately omit line numbers from the bolt-ons
   probe's § D.1 because those numbers are now stale; the
   methodology is what transfers.
4. **Treat as required, not optional.** Both conventions exist
   because their absence caused real architect-1 fabrications that
   pause-and-surface caught at execution time. Skipping them shifts
   detection from draft-time to execution-time and costs
   pause-and-surface cycles.

## Related conventions

- **Convention A** (new-files-first commit decomposition) — v1.1
  batch-1 retro § 7.2.
- **Convention B** (grandfather-sweep companion) — v1.1 batch-1
  retro § 7.2.
- **Convention E** (spec-author-self-test review) — v1.1 batch-1
  retro § 7.2.
- **Convention F** (audit-prose freshness) — v1.1 batch-1 post-retro
  audit § D.2.1.
- **Convention G** (sweep-side protection lands before check-side
  scope expansion) — v1.2 bolt-ons retro § 4.1.
- **Convention H** (filter rules query properties, not literals) —
  v1.2 bolt-ons retro § 4.2.
- **Convention I** (cross-batch scope discipline) — v1.2 bolt-ons
  retro § 4.3.

A permanent CONVENTIONS-doc home for the full set is a v1.3 candidate
(roadmap § 5 T3.2); until that lands, conventions live in the retros
that bank them, with worked examples in audit reports.
```

### 5.3 Verification block — commit 3

```bash
cd /home/otacon/Projects/GPU-Sims/GPU-Sims

# Doc file present and readable.
test -f tools/integrity/docs/probe-template-conventions.md
wc -l tools/integrity/docs/probe-template-conventions.md
# Expected: ~130 LOC.

# Doc has no obvious markdown errors (light validation).
python3 -c "
content = open('tools/integrity/docs/probe-template-conventions.md').read()
assert content.startswith('# Probe Template Conventions')
assert 'Convention C' in content
assert 'Convention D' in content
assert content.count('### Examples') == 0  # no orphan example headers
assert content.count('## Convention') == 2  # exactly two convention sections
print('ok')
"

# Gate stays at baseline.
python3 -m integrity --mode strict --no-audit-log
# Expected: hard-fail count = pre-commit baseline + new audit-doc
# bare-paths from this commit's audit report + the probe-template
# doc's many cross-references (the doc cites a lot of paths).

# Inline sweep companion.
python3 tools/integrity/scripts/grandfather_sweep.py
# Expected: sweeps new audit-doc + toolkit-doc findings; live-source
# count unchanged.

# Refresh catalog post-sweep.
python3 tools/integrity/scripts/refresh_catalog_counts.py
# Expected: small update or "no changes needed".
```

### 5.4 Audit report — commit 3

Path:
`docs/diagnostics/_audits/integrity_v1_3_commit3_landing_2026-05-16.md`.

Body sections (commit 3 is the smallest):

- **§ A. Change summary** — T1.4 lands the probe-template conventions
  doc per roadmap § 4 T1.4. Cite v1.1 batch-1 retro § 7.2 C+D as the
  originating conventions and probe § C.5 / § C.6 as the
  worked-example source.
- **§ B. File inventory** — 1 new file (probe-template-conventions.md,
  ~130 LOC), 1 audit report. Diff stat.
- **§ C. Verification** — Doc presence + light validation output, gate
  state.
- **§ D. Cross-references** — Probe § C.5 (Convention C examples),
  § C.6 (Convention D examples), § C.7 (worked-example scope
  decision). Retro § 7.2 C+D (originating conventions).
- **§ E. Banked observations** — Note the line-number-drift
  precedent (probe § F.6): the bolt-ons probe § D.1's
  `_emit_human_summary` cite at line 148 is now stale at line 154.
  This is the worked example for "anchor SHA matters; line numbers
  drift" that Convention C's "tag with anchor SHA" guidance addresses.
- **§ F. Next commit** — Pointer to commit 4 (SHA back-fill).

### 5.5 Commit message — commit 3

```
docs(integrity): T1.4 probe template conventions doc (v1.3 commit 3)

A new doc at tools/integrity/docs/probe-template-conventions.md
records Convention C (path-resolution enumeration) and Convention D
(call-site enumeration) from v1.1 batch-1 retro § 7.2, with three
worked examples per convention drawn from existing audit/retro
reports:
  - Convention C violation:   v1.1 commit-1 pause-and-surface #1
  - Convention C followed:    A.3 probe § A.3, A.2 probe § C.1
  - Convention D violation:   v1.1 commit-2 pause-and-surface #2
  - Convention D followed:    bolt-ons probe § D.1/D.2, A.2 probe § C.1/C.2

The doc is the canonical home for both conventions until the v1.3
candidates roadmap § 5 T3.2 (permanent CONVENTIONS doc location)
lands.

Per probe § C.4 + § C.7: there is no existing canonical probe-template
home in the repo (`docs/diagnostics/` contains only `_audits/`; no
`docs/diagnostics/probe-template.md` exists), so T1.4 creates the
first. Per probe § C.7 the bare convention text is ~30 LOC but
embedding the six worked examples grows the doc to ~130 LOC; the
worked examples are the load-bearing content that converts the
convention from aspirational to evidence-grounded.
```

### 5.6 Commit and push — commit 3

```bash
git pull --rebase origin main
git add tools/integrity/docs/probe-template-conventions.md \
        docs/diagnostics/_audits/integrity_v1_3_commit3_landing_2026-05-16.md
git status
git commit -F <commit-message-file>
git push origin main
git rev-parse HEAD
```

## 6. Commit 4 — SHA back-fill

### 6.1 Purpose

Commits 1–3's audit reports were authored before subsequent commits'
SHAs were known. The reports cross-reference each other (§ G "Next
commit" pointer; § A change summary may cite sibling commits' SHAs).
Commit 4 back-fills those references.

Per Convention #12 (retro § 7.2): SHA back-fill is a separate commit,
never `--amend`.

### 6.2 Pre-commit

```bash
git pull --rebase origin main
git status

# Record all three prior SHAs.
git log --oneline -4
# Expected: commits 3, 2, 1 visible.
```

### 6.3 File modifications

Each of the three audit reports under
`docs/diagnostics/_audits/integrity_v1_3_commit<N>_landing_2026-05-16.md`
has at minimum one `<COMMIT_N_SHA>` placeholder for forward-references.

Locate placeholders:

```bash
grep -l "<COMMIT_[1-3]_SHA>" \
  docs/diagnostics/_audits/integrity_v1_3_commit*_landing_2026-05-16.md
```

If the grep returns no matches, the back-fill is unnecessary
(commits 1–3 already cite each other by actual SHA OR don't cite SHAs
at all). In that case, **pause and surface** to confirm whether to
skip commit 4 or to add cross-references.

If the grep returns matches, replace each placeholder with the
actual SHA from `git log`:

- `<COMMIT_1_SHA>` → SHA of commit 1 (T1.3)
- `<COMMIT_2_SHA>` → SHA of commit 2 (T1.5)
- `<COMMIT_3_SHA>` → SHA of commit 3 (T1.4)

Use `sed` or equivalent:

```bash
sed -i "s/<COMMIT_1_SHA>/$(git rev-parse HEAD~3)/g" \
  docs/diagnostics/_audits/integrity_v1_3_commit*_landing_2026-05-16.md
sed -i "s/<COMMIT_2_SHA>/$(git rev-parse HEAD~2)/g" \
  docs/diagnostics/_audits/integrity_v1_3_commit*_landing_2026-05-16.md
sed -i "s/<COMMIT_3_SHA>/$(git rev-parse HEAD~1)/g" \
  docs/diagnostics/_audits/integrity_v1_3_commit*_landing_2026-05-16.md
```

### 6.4 Verification block — commit 4

```bash
# No placeholders remain.
grep -l "<COMMIT_[1-3]_SHA>" \
  docs/diagnostics/_audits/integrity_v1_3_commit*_landing_2026-05-16.md
# Expected: empty output.

# Every cited SHA resolves.
for sha in $(grep -oE '\b[a-f0-9]{7,40}\b' \
  docs/diagnostics/_audits/integrity_v1_3_commit*_landing_2026-05-16.md | \
  sort -u); do
    git cat-file -e "$sha" 2>/dev/null && echo "OK $sha" || echo "BAD $sha"
done
# Expected: all "OK"; if any "BAD", pause-and-surface.

# Gate stays clean.
cd /home/otacon/Projects/GPU-Sims/GPU-Sims
python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
# Expected: hard-fail count = pre-commit (audit-doc edits don't add
# new findings since the modified lines already had findings on them).
```

### 6.5 Commit message — commit 4

```
docs(integrity): SHA back-fill for v1.3 batch-1 part-A commits 1-3 (v1.3 commit 4)

Replace <COMMIT_N_SHA> placeholders in the three v1.3-batch-1-part-A
commit audit reports with the actual SHAs. Per Convention #12,
back-fill is a separate follow-up commit, never --amend.
```

### 6.6 Commit and push — commit 4

```bash
git pull --rebase origin main
git add docs/diagnostics/_audits/integrity_v1_3_commit*_landing_2026-05-16.md
git commit -F <commit-message-file>
git push origin main
git rev-parse HEAD
```

No inline sweep needed for commit 4 if it only touches audit-doc paths
that are already covered by existing classifier rules. Verify
post-push:

```bash
python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
# Expected: hard-fail count unchanged from commit 3 post-sweep state.
```

If hard-fail count drifts (e.g., a back-fill SHA looks like a bare
path the new A.3 check flags), run an inline sweep:

```bash
python3 tools/integrity/scripts/grandfather_sweep.py
```

## 7. End-state verification (after commit 4)

```bash
cd /home/otacon/Projects/GPU-Sims/GPU-Sims

# Toolkit gate.
python3 -m integrity --mode strict --no-audit-log
# Expected: hard-fail count within audit-doc-growth budget from pre-batch
# baseline (~53 from probe § A.2, plus growth from four audit reports +
# probe-template-conventions.md + cat3-conventions.md + the catalog
# prose edit). Live-source-skipped count unchanged from pre-batch.

# Test suite.
cd tools/integrity
python3 -m pytest tests/ -v --tb=short
# Expected: all tests pass. Count delta from probe-time baseline:
# - +12 in test_refresh_catalog_counts.py (T1.3, new file)
# - 0 in test_cat3_cubic_kernel.py (T1.5; docstring-only edits, no
#   signature changes)
# - 0 elsewhere (T1.4 is doc-only)
# Total: +12 tests.

# Catalog stays current.
cd /home/otacon/Projects/GPU-Sims/GPU-Sims
python3 tools/integrity/scripts/refresh_catalog_counts.py --dry-run
# Expected: "no changes needed" OR a small drift from the batch's own
# sweep activity. If the dry-run reports a non-trivial diff, run
# without --dry-run to refresh.

# Cat3 generator round-trip.
python3 tools/integrity/integrity/cat3_numerical/generate_expected.py
git diff --quiet tools/integrity/integrity/cat3_numerical/expected_values.json
# Expected: zero diff (exit 0).

# Per-check discoverability (cat3.cubic-kernel still passes).
python3 -m integrity --check cat3.cubic-kernel --no-audit-log
# Expected: 1 pass, 0 hard-fail (or 0 pass / 0 hard-fail if driver
# not built — graceful-degrade unchanged).

# No expected_values.toml lingering.
test ! -f tools/integrity/integrity/cat3_numerical/expected_values.toml
# Expected: file absent.
```

If any check fails, **pause and surface** at the commit that
introduced the regression and fix forward.

## 8. References

- `docs/integrity-toolkit-spec.md` § 13 — v2 candidates origin list
- `docs/retro/integrity-toolkit-v1.1-batch1.md` § 7.2 — Conventions A–E (and C+D as the v1.1-banked items T1.4 promotes)
- `docs/retro/integrity-toolkit-v1.1-batch1.md` § 5.5 — Catalog drift quantification (+6.7%/cycle)
- `docs/retro/integrity-toolkit-v1.1-batch1-addendum.md` § 5 — P-numbered v1.2 priority list
- `docs/retro/integrity-toolkit-v1.2-bolt-ons.md` § 4 — Conventions G/H/I
- `docs/retro/integrity-toolkit-v1.3-candidates.md` § 4 — T1 ready-to-spec items including T1.3/T1.4/T1.5
- `docs/diagnostics/_audits/integrity_v1_3_t1_3_5_probe_2026-05-16_architect1.md` — Pre-spec probe (anchor for this spec's verbatim claims)
- `docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md` § C.1, § C.2 — Convention C/D worked-example sources
- `docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md` § C.4, § D.1, § D.2 — Originating observation for T1.5 + Convention D worked example
- `docs/diagnostics/_audits/integrity_v1_1_commit1_landing_2026-05-15.md` § E.1 — Convention C violation worked-example source
- `docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md` § D.2.1 — Convention F (audit-prose freshness)

## End of execution spec
