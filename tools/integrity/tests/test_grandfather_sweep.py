"""Tests for the grandfather-sweep classification + rendering logic."""

from __future__ import annotations

from integrity.grandfather import (
    Finding,
    annotation_already_present,
    classify,
    comment_form_for,
    is_inside_fenced_block,
    render_annotation_line,
)


def _f(check_id: str, file: str, message: str = "", line: int = 1) -> Finding:
    return Finding(check_id=check_id, file=file, line=line, message=message)


def test_audit_citation_classification() -> None:
    f = _f("cat1.intra-repo", "docs/diagnostics/_audits/probe.md")
    assert classify(f).category == "audit-citation"


def test_live_shader_1810_classification() -> None:
    f = _f(
        "cat1.upstream-citation",
        "particle-fluids/sph-water/shaders/density_alpha.comp.glsl",
# integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a
        message="SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:42: version '1.8.10' does not match",
    )
    assert classify(f).category == "live-shader-1810"


def test_audit_doc_1810_classification() -> None:
    f = _f(
        "cat1.upstream-citation",
        "docs/diagnostics/_audits/phase11_5_probe_2026-05-14_architect1.md",
# integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a
        message="SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:42: version '1.8.10' does not match",
    )
    assert classify(f).category == "audit-doc-1810"


def test_spec_grammar_example_classification() -> None:
    f = _f("cat1.annotation-form", "docs/integrity-toolkit-spec.md")
    assert classify(f).category == "spec-grammar-example"


def test_toolkit_own_source_classification() -> None:
    f = _f("cat1.annotation-form", "tools/integrity/integrity/common/annotations.py")
    assert classify(f).category == "toolkit-own-source"


def test_retro_grammar_example_classification() -> None:
    f = _f("cat1.annotation-form", "docs/retro/integrity-toolkit-v1.md")
    assert classify(f).category == "retro-grammar-example"


def test_audit_report_grammar_example_classification() -> None:
    f = _f(
        "cat1.annotation-form",
        "docs/diagnostics/_audits/integrity_build_2_landing_2026-05-14.md",
    )
    assert classify(f).category == "audit-report-grammar-example"


def test_other_cat1_fallthrough() -> None:
    f = _f("cat1.intra-repo", "some/random/file.cpp")
    assert classify(f).category == "other-cat1"


def test_comment_form_python() -> None:
# integrity-allow: cat1.annotation-form; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
    assert comment_form_for("foo/bar.py") == "# integrity-allow: {body}"


def test_comment_form_cpp() -> None:
# integrity-allow: cat1.annotation-form; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
    assert comment_form_for("foo/bar.cpp") == "// integrity-allow: {body}"


def test_comment_form_glsl() -> None:
# integrity-allow: cat1.annotation-form; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
    assert comment_form_for("foo/bar.comp.glsl") == "// integrity-allow: {body}"


def test_comment_form_markdown() -> None:
# integrity-allow: cat1.annotation-form; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
    assert comment_form_for("foo/bar.md") == "<!-- integrity-allow: {body} -->"


def test_outside_fence() -> None:
    lines = ["# Header", "Some prose `code` here", "More prose"]
    in_fence, _ = is_inside_fenced_block(lines, 1)
    assert in_fence is False


def test_inside_fence() -> None:
    lines = ["# Header", "```python", "x = 1", "```", "After"]
    in_fence, lang = is_inside_fenced_block(lines, 2)
    assert in_fence is True
    assert lang == "python"


def test_at_fence_open_line() -> None:
    """The opening fence line itself is reported as in_fence=True (we just
    toggled in)."""
    lines = ["```python", "x = 1", "```"]
    in_fence, lang = is_inside_fenced_block(lines, 0)
    assert in_fence is True
    assert lang == "python"


def test_annotation_already_present_exact_match() -> None:
# integrity-allow: cat1.annotation-form; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
    line = "// integrity-allow: cat1.intra-repo; reason here; n/a"
    assert annotation_already_present(line, "cat1.intra-repo")


def test_annotation_already_present_wildcard() -> None:
# integrity-allow: cat1.annotation-form; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
    line = "// integrity-allow: cat1.*; reason here; n/a"
    assert annotation_already_present(line, "cat1.intra-repo")


def test_annotation_not_present_different_category() -> None:
# integrity-allow: cat1.annotation-form; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
    line = "// integrity-allow: cat2.*; reason here; n/a"
    assert annotation_already_present(line, "cat1.intra-repo") is False


def test_render_single_finding_in_markdown() -> None:
    f = _f("cat1.intra-repo", "docs/diagnostics/_audits/probe.md", line=10)
    file_lines = ["line 1", "line 2", "line cited at 10"]
    out = render_annotation_line(
        [f], "docs/diagnostics/_audits/probe.md", file_lines, 9,
    )
    assert len(out) == 1
    assert "cat1.intra-repo" in out[0]
# integrity-allow: cat1.annotation-form; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a
    assert "integrity-allow:" in out[0]
    assert out[0].startswith("<!--") and out[0].endswith("-->")


def test_render_two_same_category_emits_one_specific_annotation() -> None:
    """Two findings on the same line with the same check_id classify
    to the same category; emit one specific annotation (not a wildcard)."""
    f1 = _f("cat1.intra-repo", "docs/diagnostics/_audits/x.md", line=5)
    f2 = _f("cat1.intra-repo", "docs/diagnostics/_audits/x.md", line=5)
    file_lines = [""] * 10
    out = render_annotation_line(
        [f1, f2], "docs/diagnostics/_audits/x.md", file_lines, 4,
    )
    assert len(out) == 1
    assert "cat1.intra-repo" in out[0]


# ---------------------------------------------------------------------------
# P1.8 -- live-source path-bucket tests
# ---------------------------------------------------------------------------


def test_is_live_source_path_audit_doc_paths_are_sweepable() -> None:
    from integrity.grandfather import is_live_source_path
    assert is_live_source_path("docs/diagnostics/_audits/foo.md") is False
    assert is_live_source_path("docs/diagnostics/_audits/sub/bar.md") is False
    assert is_live_source_path("docs/retro/integrity-toolkit-v1.md") is False


def test_is_live_source_path_toolkit_doc_paths_are_sweepable() -> None:
    from integrity.grandfather import is_live_source_path
    assert is_live_source_path("tools/integrity/docs/ground-truth-sources.md") is False
    assert is_live_source_path("tools/integrity/README.md") is False
    assert is_live_source_path("docs/integrity-toolkit-spec.md") is False
    assert is_live_source_path("project-state.md") is False


def test_is_live_source_path_live_source_paths_return_true() -> None:
    from integrity.grandfather import is_live_source_path
    assert is_live_source_path("docs/phase12_lattice_boltzmann.md") is True
    assert is_live_source_path("particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl") is True
    assert is_live_source_path("CHANGELOG.md") is True
    assert is_live_source_path("common/common-cpp/include/gpusims/alembic_writer.hpp") is True


def test_is_live_source_path_normalizes_backslashes() -> None:
    from integrity.grandfather import is_live_source_path
    # Windows-style path separators normalize to forward slash before matching.
    assert is_live_source_path("docs\\diagnostics\\_audits\\foo.md") is False


def test_apply_annotations_default_skips_live_source_other_cat1(tmp_path, monkeypatch) -> None:
    """With sweep_live_source=False (default), live-source other-cat1 findings are filtered out."""
    from integrity import grandfather

    # Construct a synthetic finding set: one audit-doc (sweepable) + one live-source.
    audit_finding = _f("cat1.intra-repo", "docs/diagnostics/_audits/foo.md", "X:1: path 'X' does not resolve")
    live_finding = _f("cat1.intra-repo", "some/live/path.py", "Y:1: path 'Y' does not resolve")

    # Stub collect_findings to return our synthetic set.
    monkeypatch.setattr(grandfather, "collect_findings", lambda root: [audit_finding, live_finding])

    # Need a writable repo root with the target files present so the renderer
    # has something to splice into. Create minimal fixtures.
    audit_dir = tmp_path / "docs" / "diagnostics" / "_audits"
    audit_dir.mkdir(parents=True)
    (audit_dir / "foo.md").write_text("line 1\n", encoding="utf-8")
    live_dir = tmp_path / "some" / "live"
    live_dir.mkdir(parents=True)
    (live_dir / "path.py").write_text("# line 1\n", encoding="utf-8")

    files, anns, counts, skipped = grandfather.apply_annotations(tmp_path, dry_run=True, sweep_live_source=False)
    # Live-source finding filtered out; only the audit-doc one is processed.
    assert skipped == 1
    # The audit-doc finding classifies as audit-citation (not other-cat1) so it
    # wouldn't trigger the filter even without the sweep_live_source flag.
    # Verify the live-source one was specifically skipped:
    assert "other-cat1" not in counts or counts.get("other-cat1", 0) == 0


def test_apply_annotations_sweep_live_source_includes_live_source(tmp_path, monkeypatch) -> None:
    """With sweep_live_source=True, live-source other-cat1 findings are processed."""
    from integrity import grandfather

    live_finding = _f("cat1.intra-repo", "some/live/path.py", "Y:1: path 'Y' does not resolve")
    monkeypatch.setattr(grandfather, "collect_findings", lambda root: [live_finding])

    live_dir = tmp_path / "some" / "live"
    live_dir.mkdir(parents=True)
    (live_dir / "path.py").write_text("# line 1\n", encoding="utf-8")

    files, anns, counts, skipped = grandfather.apply_annotations(tmp_path, dry_run=True, sweep_live_source=True)
    assert skipped == 0
    assert counts.get("other-cat1", 0) == 1


def test_apply_annotations_default_still_sweeps_named_category_on_live_source(tmp_path, monkeypatch) -> None:
    """Named classifier categories (like live-shader-1810) on live-source paths are still swept by default.

    P1.8 only protects the other-cat1 fallthrough bucket. Named categories are
    intentional migration-tracking; they should continue to be swept.
    """
    from integrity import grandfather

    # live-shader-1810 path -- particle-fluids/sph-water/shaders/ subset.
# integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a
    named_finding = _f(
        "cat1.upstream-citation",
        "particle-fluids/sph-water/shaders/density_alpha.comp.glsl",
# integrity-allow: cat1.upstream-citation; audit-doc reference to the historical 1.8.10 fabrication (permanent suppression); n/a
        "SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:42: version '1.8.10' does not match",
    )
    monkeypatch.setattr(grandfather, "collect_findings", lambda root: [named_finding])

    shader_dir = tmp_path / "particle-fluids" / "sph-water" / "shaders"
    shader_dir.mkdir(parents=True)
    (shader_dir / "density_alpha.comp.glsl").write_text("// line 1\n", encoding="utf-8")

    files, anns, counts, skipped = grandfather.apply_annotations(tmp_path, dry_run=True, sweep_live_source=False)
    # The finding classifies as live-shader-1810, not other-cat1, so the filter
    # doesn't apply. Should be processed normally.
    assert skipped == 0
    assert counts.get("live-shader-1810", 0) == 1


# ---------------------------------------------------------------------------
# T1.2 -- FALLTHROUGH_CATEGORIES + is_fallthrough_category (Convention H)
# ---------------------------------------------------------------------------


def test_other_cat1_is_fallthrough() -> None:
    from integrity.grandfather import is_fallthrough_category
    assert is_fallthrough_category("other-cat1") is True


def test_other_cat1_bare_path_is_fallthrough() -> None:
    from integrity.grandfather import is_fallthrough_category
    assert is_fallthrough_category("other-cat1-bare-path") is True


def test_named_category_is_not_fallthrough() -> None:
    from integrity.grandfather import is_fallthrough_category
    assert is_fallthrough_category("audit-citation") is False
    assert is_fallthrough_category("toolkit-own-source") is False
    assert is_fallthrough_category("toolkit-own-unused") is False


def test_unknown_category_is_not_fallthrough() -> None:
    from integrity.grandfather import is_fallthrough_category
    assert is_fallthrough_category("nonexistent") is False
    assert is_fallthrough_category("") is False


def test_fallthrough_categories_is_frozenset() -> None:
    from integrity.grandfather import FALLTHROUGH_CATEGORIES
    assert isinstance(FALLTHROUGH_CATEGORIES, frozenset)


def test_fallthrough_categories_contents() -> None:
    from integrity.grandfather import FALLTHROUGH_CATEGORIES
    # Pin the v1.3 baseline. When a future batch adds new fallthrough
    # categories, update this assertion intentionally.
    assert FALLTHROUGH_CATEGORIES == frozenset({
        "other-cat1",
        "other-cat1-bare-path",
    })


# ---------------------------------------------------------------------------
# T1.1 -- Three new permanent cat1.intra-repo categories
# ---------------------------------------------------------------------------


def test_toolkit_doc_snapshot_routes_tools_integrity_docs() -> None:
    f = _f("cat1.intra-repo", "tools/integrity/docs/algebraic/d3q19.md", line=175)
    assert classify(f).category == "toolkit-doc-snapshot"


def test_toolkit_doc_snapshot_routes_integrity_spec() -> None:
    f = _f("cat1.intra-repo", "docs/integrity-toolkit-spec.md", line=1)
    assert classify(f).category == "toolkit-doc-snapshot"


def test_toolkit_doc_snapshot_routes_readme() -> None:
    f = _f("cat1.intra-repo", "tools/integrity/README.md", line=1)
    assert classify(f).category == "toolkit-doc-snapshot"


def test_project_state_snapshot_routes() -> None:
    f = _f("cat1.intra-repo", "project-state.md", line=559)
    assert classify(f).category == "project-state-snapshot"


def test_retro_doc_snapshot_routes() -> None:
    f = _f("cat1.intra-repo", "docs/retro/integrity-toolkit-v1.1-batch1.md", line=332)
    assert classify(f).category == "retro-doc-snapshot"


def test_new_rules_dont_match_unrelated_live_source_paths() -> None:
    # A live-source path should still fall through to other-cat1.
    f = _f("cat1.intra-repo", "common/common-cpp/src/widget.cpp", line=10)
    assert classify(f).category == "other-cat1"


def test_new_categories_in_known_categories() -> None:
    from integrity.snapshot import _KNOWN_CATEGORIES
    assert "toolkit-doc-snapshot" in _KNOWN_CATEGORIES
    assert "project-state-snapshot" in _KNOWN_CATEGORIES
    assert "retro-doc-snapshot" in _KNOWN_CATEGORIES


# ---------------------------------------------------------------------------
# rewrite-stale-reasons (v1.3 closeout commit 1; Part-B retro section 4.1)
# ---------------------------------------------------------------------------


def _make_finding_dict(
    check_id: str,
    file: str,
    line: int,
    message: str,
    suppression_reason: str,
) -> dict:
    """Construct a JSON-shape finding dict matching what `--output json` emits."""
    return {
        "check_id": check_id,
        "mode": "HARD_FAIL",
        "file": file,
        "line": line,
        "message": message,
        "suppressed": True,
        "suppression_reason": suppression_reason,
        "suppression_issue": "n/a",
    }


def test_rewrite_stale_reasons_category_changed_rewrites(tmp_path, monkeypatch) -> None:
    """A stale-category annotation gets rewritten when classify() now
    returns a different category."""
    from integrity import grandfather

    # Fixture: project-state.md with a stale `other-cat1` annotation on a
    # cat1.intra-repo finding that now classifies as project-state-snapshot.
    fixture = tmp_path / "project-state.md"
    fixture.write_text(
        "line 1\n"
        # integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        "<!-- integrity-allow: cat1.intra-repo; "
        "grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->\n"
        "synthetic finding target line (no citation here)\n",
        encoding="utf-8",
    )

    suppressed = _make_finding_dict(
        check_id="cat1.intra-repo",
        file="project-state.md",
        line=3,
        message="synthetic: target path does not resolve",
        suppression_reason="grandfathered-pre-v1 (see grandfather-catalog other-cat1)",
    )
    monkeypatch.setattr(
        grandfather, "_collect_all_findings", lambda root: [suppressed],
    )

    files, anns, rewrites = grandfather.rewrite_stale_reasons(tmp_path, dry_run=False)

    assert anns == 1
    assert files == 1
    assert rewrites[0][2] == "other-cat1"
    assert rewrites[0][3] == "project-state-snapshot"

    new_content = fixture.read_text(encoding="utf-8")
    assert "project-state.md cross-phase snapshot intra-repo citation" in new_content
    assert "grandfathered-pre-v1" not in new_content


def test_rewrite_stale_reasons_wording_diff_only_skipped(tmp_path, monkeypatch) -> None:
    """When the annotation's wording differs but the category is the
    same, leave it alone (D1 conservative scope)."""
    from integrity import grandfather

    # Annotation already in the project-state-snapshot category; the wording
    # is slightly different but classify() would still return the same
    # category. D1: no rewrite.
    fixture = tmp_path / "project-state.md"
    fixture.write_text(
        "line 1\n"
        # integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        "<!-- integrity-allow: cat1.intra-repo; "
        "project-state-snapshot — historical citation (paraphrased); n/a -->\n"
        "synthetic finding target line (no citation here)\n",
        encoding="utf-8",
    )

    suppressed = _make_finding_dict(
        check_id="cat1.intra-repo",
        file="project-state.md",
        line=3,
        message="synthetic: target path does not resolve",
        suppression_reason=(
            "project-state-snapshot — historical citation (paraphrased)"
        ),
    )
    monkeypatch.setattr(
        grandfather, "_collect_all_findings", lambda root: [suppressed],
    )

    files, anns, rewrites = grandfather.rewrite_stale_reasons(tmp_path, dry_run=False)

    assert anns == 0
    assert files == 0
    assert rewrites == []


def test_rewrite_stale_reasons_dry_run_no_writes(tmp_path, monkeypatch) -> None:
    """Dry-run mode reports rewrites without modifying files on disk."""
    from integrity import grandfather

    fixture = tmp_path / "project-state.md"
    original = (
        "line 1\n"
        # integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        "<!-- integrity-allow: cat1.intra-repo; "
        "grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->\n"
        "synthetic finding target line (no citation here)\n"
    )
    fixture.write_text(original, encoding="utf-8")

    suppressed = _make_finding_dict(
        check_id="cat1.intra-repo",
        file="project-state.md",
        line=3,
        message="synthetic: target path does not resolve",
        suppression_reason="grandfathered-pre-v1 (see grandfather-catalog other-cat1)",
    )
    monkeypatch.setattr(
        grandfather, "_collect_all_findings", lambda root: [suppressed],
    )

    files, anns, rewrites = grandfather.rewrite_stale_reasons(tmp_path, dry_run=True)

    assert anns == 1
    assert files == 1
    assert len(rewrites) == 1
    # File on disk unchanged in dry-run.
    assert fixture.read_text(encoding="utf-8") == original


def test_rewrite_stale_reasons_preserves_comment_form(tmp_path, monkeypatch) -> None:
    """The rewrite leaves the comment-form intact: // stays //; # stays #;
    <!-- --> stays HTML-comment-wrapped."""
    from integrity import grandfather

    # Three fixtures: one Python (#), one C++ (//), one Markdown (<!-- -->).
    # Each carries a stale annotation whose finding reclassifies.
    py_file = tmp_path / "tools" / "integrity" / "integrity" / "snippet.py"
    py_file.parent.mkdir(parents=True)
    py_file.write_text(
        "x = 1\n"
        # integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        "# integrity-allow: cat1.annotation-form; "
        "grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a\n"
        "y = 2\n",
        encoding="utf-8",
    )

    cpp_file = tmp_path / "common" / "lib.cpp"
    cpp_file.parent.mkdir(parents=True)
    cpp_file.write_text(
        "int x = 1;\n"
        # integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        "// integrity-allow: cat1.bare-path; "
        "grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a\n"
        "int y = 2;\n",
        encoding="utf-8",
    )

    md_file = tmp_path / "docs" / "retro" / "notes.md"
    md_file.parent.mkdir(parents=True)
    md_file.write_text(
        "line 1\n"
        # integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        "<!-- integrity-allow: cat1.intra-repo; "
        "grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->\n"
        "line 3\n",
        encoding="utf-8",
    )

    # Fake suppressed findings for each file.
    finds = [
        _make_finding_dict(
            check_id="cat1.annotation-form",
            file="tools/integrity/integrity/snippet.py",
            line=3,
            message="y = 2 (synthetic finding line)",
            suppression_reason=(
                "grandfathered-pre-v1 (see grandfather-catalog other-cat1)"
            ),
        ),
        _make_finding_dict(
            check_id="cat1.bare-path",
            file="common/lib.cpp",
            line=3,
            message="int y = 2 (synthetic)",
            suppression_reason=(
                "grandfathered-pre-v1 (see grandfather-catalog other-cat1)"
            ),
        ),
        _make_finding_dict(
            check_id="cat1.intra-repo",
            file="docs/retro/notes.md",
            line=3,
            message="line 3 (synthetic)",
            suppression_reason=(
                "grandfathered-pre-v1 (see grandfather-catalog other-cat1)"
            ),
        ),
    ]
    monkeypatch.setattr(grandfather, "_collect_all_findings", lambda root: finds)

    files, anns, rewrites = grandfather.rewrite_stale_reasons(tmp_path, dry_run=False)
    assert files == 3
    assert anns == 3

    py_new = py_file.read_text(encoding="utf-8")
    cpp_new = cpp_file.read_text(encoding="utf-8")
    md_new = md_file.read_text(encoding="utf-8")

    # Python annotation stays `#` form.
    # integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    assert "\n# integrity-allow: cat1.annotation-form;" in py_new
    # C++ annotation stays `//` form.
    # integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    assert "\n// integrity-allow: cat1.bare-path;" in cpp_new
    # Markdown annotation stays `<!-- ... -->` HTML-comment form.
    # integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    assert "<!-- integrity-allow: cat1.intra-repo;" in md_new
    assert md_new.count("-->") >= 1


def test_rewrite_stale_reasons_mutually_exclusive_with_sweep_flags(tmp_path) -> None:
    """CLI rejects --rewrite-stale-reasons combined with --sweep-live-source
    or --force-sweep-category."""
    import subprocess
    import sys
    from pathlib import Path

    script = Path(__file__).resolve().parent.parent / "scripts" / "grandfather_sweep.py"
    repo_root_arg = ["--repo-root", str(tmp_path)]

    result = subprocess.run(
        [sys.executable, str(script), "--rewrite-stale-reasons",
         "--sweep-live-source", "--dry-run", *repo_root_arg],
        capture_output=True, text=True, check=False,
    )
    assert result.returncode == 2
    assert "mutually exclusive" in result.stderr

    result = subprocess.run(
        [sys.executable, str(script), "--rewrite-stale-reasons",
         "--force-sweep-category", "toolkit-own-unused",
         "--dry-run", *repo_root_arg],
        capture_output=True, text=True, check=False,
    )
    assert result.returncode == 2
    assert "mutually exclusive" in result.stderr


def test_rewrite_stale_reasons_no_match_returns_empty(tmp_path, monkeypatch) -> None:
    """No suppressed findings with stale categories -> no-op."""
    from integrity import grandfather

    monkeypatch.setattr(grandfather, "_collect_all_findings", lambda root: [])

    files, anns, rewrites = grandfather.rewrite_stale_reasons(tmp_path, dry_run=False)
    assert files == 0
    assert anns == 0
    assert rewrites == []
