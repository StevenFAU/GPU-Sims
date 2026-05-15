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
