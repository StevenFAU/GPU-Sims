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
    assert _extract_category(
        "audit-doc snapshot (audit-citation)"
    ) == "audit-citation"
    assert _extract_category(
        "pre-v1 Stack C public symbol (cat2-stack-c-unused)"
    ) == "cat2-stack-c-unused"
    assert _extract_category("unrelated reason text") == "other"


def test_extract_category_empty() -> None:
    assert _extract_category("") == "other"
    assert _extract_category(None) == "other"  # type: ignore[arg-type]


def test_extract_category_pattern_matches() -> None:
    """Reason strings that do not embed the category name are matched
    via the secondary pattern table."""
    assert _extract_category(
        "regex or docstring literal of the annotation grammar token"
    ) == "toolkit-own-source"
    assert _extract_category(
        "audit-doc literal mention of the annotation grammar (not real)"
    ) == "audit-report-grammar-example"
    assert _extract_category(
        "documentation-only literal mention of the annotation grammar"
    ) == "spec-grammar-example"
    assert _extract_category(
        "retrospective-doc literal mention of the annotation grammar"
    ) == "retro-grammar-example"
    assert _extract_category(
        "audit-doc reference to the historical 1.8.10 fabrication"
    ) == "audit-doc-1810"
    assert _extract_category(
        "pre-v1.1 stale Phase-N stub label on real implementation"
    ) == "cat2-stub-label-stale"


def test_parse_ground_truth_sources_smoke(repo_root: Path) -> None:
    """Real ground-truth-sources.md should yield at least one upstream."""
    sources = _parse_ground_truth_sources(repo_root)
    assert len(sources) >= 1
    assert any(
        "2.16.1" in (s.get("anchor_version") or "") for s in sources
    )


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
