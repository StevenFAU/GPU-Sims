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
