"""Tests for cat2.stub-label-stale."""

from __future__ import annotations

from pathlib import Path

from integrity.cat2_contracts.checks.stub_label_stale import run


def test_bad_cpp_header_with_real_impl_flags(fixtures_dir: Path) -> None:
    """Stale stub label + sibling .cpp with >10 LOC -> flag the header."""
    findings = run(fixtures_dir / "bad_stub_label")
    headers = [f for f in findings if f.file.endswith("widget.hpp")]
    assert len(headers) == 1, (
        f"unexpected: {[(f.file, f.message) for f in findings]}"
    )
    assert "stale" in headers[0].message.lower()


def test_good_cpp_header_with_stub_impl_does_not_flag(fixtures_dir: Path) -> None:
    """Stub label + sibling .cpp with <=10 LOC -> real stub, no fire."""
    findings = run(fixtures_dir / "good_stub_label")
    headers = [f for f in findings if f.file.endswith("widget.hpp")]
    assert headers == [], (
        f"unexpected fire: {[(f.file, f.message) for f in headers]}"
    )


def test_bad_python_stub_label_flags(fixtures_dir: Path) -> None:
    """Python file with stale stub label and no discriminator -> flag."""
    findings = run(fixtures_dir / "bad_stub_label")
    py = [f for f in findings if f.file.endswith("widget.py")]
    assert len(py) == 1, (
        f"unexpected: {[(f.file, f.message) for f in findings]}"
    )


def test_python_permanent_stub_discriminator_does_not_flag(
    fixtures_dir: Path,
) -> None:
    """Python file with `permanent stub` framing should NOT flag, even if
    the literal `In Phase N, this is a stub:` phrase appears elsewhere."""
    findings = run(fixtures_dir / "good_stub_label")
    py = [f for f in findings if "permanent" in f.file]
    assert py == [], (
        f"discriminator did not gate: {[(f.file, f.message) for f in py]}"
    )


def test_check_id_and_mode() -> None:
    """Smoke: CHECK_ID and MODE are stable identifiers."""
    from integrity.cat2_contracts.checks.stub_label_stale import CHECK_ID, MODE
    from integrity.common.results import FailureMode
    assert CHECK_ID == "cat2.stub-label-stale"
    assert MODE == FailureMode.HARD_FAIL


def test_repo_root_with_no_common_dir_returns_empty(tmp_path: Path) -> None:
    """Running against a directory with no common/ tree -> zero findings."""
    findings = run(tmp_path)
    assert findings == []
