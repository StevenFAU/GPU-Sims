"""Tests for cat1.annotation-form."""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.checks.annotation import _validate, run


def _files_dir(fixtures_dir: Path, name: str) -> Path:
    return fixtures_dir / name


def test_good_annotations_yield_no_findings(fixtures_dir: Path) -> None:
    findings = run(_files_dir(fixtures_dir, "good_citations"))
    ann_findings = [f for f in findings if f.check_id == "cat1.annotation-form"]
    assert ann_findings == [], f"unexpected annotation findings: {ann_findings}"


def test_bad_annotations_yield_three_findings(fixtures_dir: Path) -> None:
    findings = run(_files_dir(fixtures_dir, "bad_citations"))
    ann_findings = [f for f in findings if f.check_id == "cat1.annotation-form"]
    # Expect 3 issues in bad_annotation.py:
    #   1. "bogus" doesn't match cat<N>.<name> grammar
    #   2. issue_ref "not-a-ref" fails
    #   3. blanket '*' is rejected
    assert len(ann_findings) >= 3, f"expected >=3, got {[f.message for f in ann_findings]}"


def test_validate_check_id_grammar() -> None:
    assert _validate("cat1.intra-repo; sufficient reason text; #1") is None
    assert _validate("cat2.*; wildcard cat 2; n/a") is None
    # Bad check-id prefix
    assert _validate("bogus.thing; sufficient reason text; #1") is not None
    # Blanket wildcard
    assert _validate("*; sufficient reason text; #1") is not None


def test_validate_reason_length() -> None:
    assert _validate("cat1.intra-repo; short; #1") is not None
    assert _validate("cat1.intra-repo; eight ch; #1") is None  # exactly 8


def test_validate_issue_ref() -> None:
    assert _validate("cat1.intra-repo; sufficient reason text; #117") is None
    assert _validate("cat1.intra-repo; sufficient reason text; n/a") is None
    assert _validate("cat1.intra-repo; sufficient reason text; not-a-ref") is not None
