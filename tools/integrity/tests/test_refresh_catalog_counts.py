"""Tests for refresh_catalog_counts.py.

Covers parser correctness on the canonical and non-canonical heading shapes
surfaced by the v1.3 probe § B.1, the refresh logic's preservation of
non-numeric parentheticals, the error-on-missing-heading invariant, and the
idempotency property required by probe § B.7 (4).
"""

from __future__ import annotations

import sys
import textwrap
from pathlib import Path

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
