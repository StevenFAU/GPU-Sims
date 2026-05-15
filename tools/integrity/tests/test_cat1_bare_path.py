"""Tests for cat1.bare-path. Uses synthetic fixtures only."""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.checks.bare_path import (
    CHECK_ID,
    MODE,
    BarePathClass,
    _classify_bare_path,
    _passes_sanity_check,
    run,
)
from integrity.cat1_citations.grammar import IntraRepoCitation
from integrity.common.results import FailureMode


def _files_dir(fixtures_dir: Path, name: str) -> Path:
    return fixtures_dir / name


def test_check_id_and_mode() -> None:
    assert CHECK_ID == "cat1.bare-path"
    assert MODE == FailureMode.HARD_FAIL


def test_passes_sanity_check_rejects_empty() -> None:
    assert _passes_sanity_check("", 1) is False


def test_passes_sanity_check_rejects_newline() -> None:
    assert _passes_sanity_check("foo\nbar.cpp", 1) is False


def test_passes_sanity_check_rejects_line_zero() -> None:
    assert _passes_sanity_check("foo.cpp", 0) is False


def test_passes_sanity_check_rejects_leading_digit() -> None:
    assert _passes_sanity_check("9foo.cpp", 1) is False


def test_passes_sanity_check_accepts_valid_basename() -> None:
    assert _passes_sanity_check("foo.cpp", 42) is True
    assert _passes_sanity_check("_under.py", 1) is True


def test_good_bare_path_yields_no_findings(fixtures_dir: Path) -> None:
    findings = run(_files_dir(fixtures_dir, "good_bare_path"))
    assert findings == [], f"unexpected findings: {findings}"


def test_bad_bare_path_upstream_emits_suggested_rewrite(fixtures_dir: Path) -> None:
    findings = run(_files_dir(fixtures_dir, "bad_bare_path_upstream"))
    upstream = [f for f in findings if "bare upstream citation" in f.message]
    assert len(upstream) == 1
    f = upstream[0]
    assert f.check_id == "cat1.bare-path"
    assert f.mode == FailureMode.HARD_FAIL
    assert f.file == "docs/design.md"
    assert "SimUpstream 1.0.0 SimUpstream/TimeStep.cpp:42" in f.message


def test_bad_bare_path_intra_emits_suggested_full_path(fixtures_dir: Path) -> None:
    findings = run(_files_dir(fixtures_dir, "bad_bare_path_intra"))
    intra = [
        f for f in findings
        if "bare intra-repo citation" in f.message
        and "exceeds file line count" not in f.message
    ]
    assert len(intra) == 1
    assert "common/unique_widget.hpp:10" in intra[0].message


def test_bad_bare_path_intra_out_of_range_emits_line_count(fixtures_dir: Path) -> None:
    findings = run(_files_dir(fixtures_dir, "bad_bare_path_intra"))
    oor = [
        f for f in findings
        if "bare intra-repo citation" in f.message
        and "exceeds file line count" in f.message
    ]
    assert len(oor) == 1
    assert "common/unique_widget.hpp" in oor[0].message
    assert "99" in oor[0].message


def test_bad_bare_path_ambiguous_emits_disambiguation_list(fixtures_dir: Path) -> None:
    findings = run(_files_dir(fixtures_dir, "bad_bare_path_ambiguous"))
    amb = [f for f in findings if "bare basename matches" in f.message and "no git-tracked" not in f.message]
    assert len(amb) == 1
    msg = amb[0].message
    assert "candidates:" in msg
    # 7 candidates total, so the truncation marker should appear.
    assert "(2 more)" in msg
    assert ">=7" in msg
    # First few candidates alphabetical:
    assert "sim_a/shared.cpp" in msg
    assert "sim_b/shared.cpp" in msg


def test_bad_bare_path_ambiguous_truncates_at_cap(fixtures_dir: Path) -> None:
    findings = run(_files_dir(fixtures_dir, "bad_bare_path_ambiguous"))
    amb = [f for f in findings if "bare basename matches" in f.message and "no git-tracked" not in f.message]
    assert len(amb) == 1
    msg = amb[0].message
    # sim_f and sim_g should be omitted from the in-line list.
    head = msg.split("(2 more)")[0]
    assert "sim_f/shared.cpp" not in head
    assert "sim_g/shared.cpp" not in head


def test_bad_bare_path_unresolvable_emits_no_match_message(fixtures_dir: Path) -> None:
    findings = run(_files_dir(fixtures_dir, "bad_bare_path_unresolvable"))
    unres = [f for f in findings if "matches no git-tracked file" in f.message]
    assert len(unres) == 1
    assert "nonexistent_module.py" in unres[0].message


def test_dotted_path_not_flagged_by_bare_path_check(fixtures_dir: Path) -> None:
    """Full-path citations like `common/widget.hpp:1` are NOT bare paths."""
    findings = run(_files_dir(fixtures_dir, "good_bare_path"))
    bare = [f for f in findings if "/" not in f.message.split(":")[0]]
    # All citations in good_bare_path use full paths; nothing should fire.
    assert bare == []


def test_fence_internal_bare_path_not_flagged(tmp_path: Path) -> None:
    """A bare-basename citation inside a fenced code block is suppressed."""
    (tmp_path / "doc.md").write_text(
        "outside fence: nope.cpp:1\n"
        "```\n"
        "inside fence: also.cpp:1\n"
        "```\n"
    )
    findings = run(tmp_path)
    msgs = [f.message for f in findings]
    # The fenced one should not be reported.
    assert not any("also.cpp:1" in m for m in msgs)
    # The unfenced one should be reported.
    assert any("nope.cpp:1" in m for m in msgs)


def test_classify_bare_path_unit() -> None:
    """Direct unit test of _classify_bare_path for each arm."""
    citation = IntraRepoCitation(
        path="foo.cpp",
        start=1,
        end=None,
        source_file=Path("/tmp/fake.md"),
        source_line=1,
        raw="foo.cpp:1",
    )
    # REGISTERED_UPSTREAM
    res = _classify_bare_path(
        citation,
        upstream_idx={"foo.cpp": [Path("/tmp/refs/foo.cpp")]},
        intra_idx={},
    )
    assert res.class_ == BarePathClass.REGISTERED_UPSTREAM

    # INTRA_REPO
    res = _classify_bare_path(
        citation,
        upstream_idx={},
        intra_idx={"foo.cpp": [Path("/tmp/repo/foo.cpp")]},
    )
    assert res.class_ == BarePathClass.INTRA_REPO

    # AMBIGUOUS
    res = _classify_bare_path(
        citation,
        upstream_idx={},
        intra_idx={"foo.cpp": [Path("/tmp/a/foo.cpp"), Path("/tmp/b/foo.cpp")]},
    )
    assert res.class_ == BarePathClass.AMBIGUOUS

    # UNRESOLVABLE
    res = _classify_bare_path(citation, upstream_idx={}, intra_idx={})
    assert res.class_ == BarePathClass.UNRESOLVABLE
