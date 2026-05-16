"""Grandfather-sweep logic for the integrity toolkit v1.

Classifies HARD_FAIL findings into one of seven pre-v1 categories and
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
generates inline `integrity-allow:` annotations on the cited source
lines. See `tools/integrity/docs/grandfather-catalog.md` for the
per-category rationale.

Imported by `tools/integrity/scripts/grandfather_sweep.py` (the CLI
wrapper) and by the unit tests under `tools/integrity/tests/`.
"""

from __future__ import annotations

import json
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


@dataclass(frozen=True)
class Finding:
    check_id: str
    file: str
    line: int
    message: str


@dataclass(frozen=True)
# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
class Classification:
    category: str
    reason: str
    issue_ref: str


# ---------------------------------------------------------------------------
# P1.8 -- live-source vs sweepable-path bucket
#
# The post-batch triage (docs/diagnostics/_audits/integrity_v1_1_post_batch_triage_2026-05-15.md
# section B) defines three buckets:
#   AUDIT-DOC, TOOLKIT-DOC -- sweep (permanent suppression)
#   LIVE-SOURCE            -- attribute to introducing author, do NOT sweep
# The grandfather-sweep CLI used to sweep all unsuppressed findings, which
# over-swept LIVE-SOURCE other-cat1 findings (surfaced as a pause-and-surface
# during commit 9add149). is_live_source_path() defines the bucket boundary
# in code; apply_annotations(sweep_live_source=...) honors it.
# ---------------------------------------------------------------------------


SWEEPABLE_PATH_PREFIXES: tuple[str, ...] = (
    "docs/diagnostics/_audits/",
    "docs/retro/",
    "tools/integrity/docs/",
)


SWEEPABLE_EXACT_PATHS: frozenset[str] = frozenset({
    "docs/integrity-toolkit-spec.md",
    "tools/integrity/README.md",
    "project-state.md",
})


# Convention H (v1.2 bolt-ons retro section 4.2 -- fallthrough discriminator):
# the set of category names that classify findings via fall-through (catch-all
# "other-<catN>" buckets). Findings in these categories on LIVE-SOURCE paths
# are protected from auto-sweeping by default per P1.8.
#
# Forward-compatible: when a future batch adds a new fallthrough bucket (for
# example, other-cat3 if/when introduced), add it here. The frozenset is
# pinned by test_fallthrough_categories_contents to make future extensions
# intentional.
FALLTHROUGH_CATEGORIES: frozenset[str] = frozenset({
    "other-cat1",
    "other-cat1-bare-path",
})


def is_fallthrough_category(category: str) -> bool:
    """True if `category` is a fall-through bucket (catch-all).

    Per Convention H (v1.2 bolt-ons retro section 4.2). Used by
    apply_annotations's LIVE-SOURCE filter to identify which categories
    should be protected from auto-sweep by default on live-source paths.
    """
    return category in FALLTHROUGH_CATEGORIES


def is_live_source_path(file_path: str) -> bool:
    """Return True iff file_path is LIVE-SOURCE per the triage section B bucket.

    LIVE-SOURCE = not under any SWEEPABLE_PATH_PREFIXES prefix and not in
    SWEEPABLE_EXACT_PATHS. This is the bucket the sweep should default-skip
    for other-cat1 (fallthrough) findings; named classifier categories
    (cat2-stack-*-unused, cat2-stub-label-stale, live-shader-1810) remain
    sweepable on live-source paths by design.
    """
    normalized = file_path.replace("\\", "/")
    if normalized in SWEEPABLE_EXACT_PATHS:
        return False
    for prefix in SWEEPABLE_PATH_PREFIXES:
        if normalized.startswith(prefix):
            return False
    return True


def classify(finding: Finding) -> Classification:
    """Classify a finding into a grandfather category. First match wins."""
    f = finding.file
    msg = finding.message
    cid = finding.check_id

    if cid == "cat2.public-symbol-used":
        return Classification(
            category="cat2-stack-d-unused",
            reason="pre-v1 Stack D public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-d-unused)",
            issue_ref="n/a",
        )

    if cid == "cat2.public-symbol-used-c":
        return Classification(
            category="cat2-stack-c-unused",
            reason="pre-v1 Stack C public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-c-unused)",
            issue_ref="n/a",
        )

    if cid == "cat2.public-symbol-used-ts":
        return Classification(
            category="cat2-stack-b-unused",
            reason="pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused)",
            issue_ref="n/a",
        )

    if cid == "cat2.stub-label-stale":
        return Classification(
            category="cat2-stub-label-stale",
            reason="pre-v1.1 stale Phase-N stub label on real implementation (canonical spec section 12 row 5 -- tracked for migration as the corresponding header is next edited)",
            issue_ref="n/a",
        )

    if cid == "cat2.public-symbol-used-toolkit":
        return Classification(
            category="toolkit-own-unused",
            reason="pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused)",
            issue_ref="n/a",
        )

    if cid == "cat1.intra-repo" and f.startswith("docs/diagnostics/_audits/"):
        return Classification(
            category="audit-citation",
            reason="audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation)",
            issue_ref="n/a",
        )

    if cid == "cat1.upstream-citation" and "1.8.10" in msg:
        if (
            f.startswith("particle-fluids/sph-water/shaders/")
            or f.startswith("particle-fluids/sph-water/src/")
        ):
            return Classification(
                category="live-shader-1810",
                reason="pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (migration target tracked in grandfather-catalog live-shader-1810)",
                issue_ref="n/a",
            )
        return Classification(
            category="audit-doc-1810",
            reason="audit-doc reference to the historical 1.8.10 fabrication (permanent suppression)",
            issue_ref="n/a",
        )

    if cid == "cat1.annotation-form":
        if f == "docs/integrity-toolkit-spec.md" or f.startswith("tools/integrity/docs/"):
            return Classification(
                category="spec-grammar-example",
                reason="documentation-only literal mention of the annotation grammar (not a real annotation)",
                issue_ref="n/a",
            )
        if f.startswith("docs/retro/"):
            return Classification(
                category="retro-grammar-example",
                reason="retrospective-doc literal mention of the annotation grammar (not a real annotation)",
                issue_ref="n/a",
            )
        if f.startswith("tools/integrity/integrity/"):
            return Classification(
                category="toolkit-own-source",
                reason="regex or docstring literal of the annotation grammar token (not a real annotation)",
                issue_ref="n/a",
            )
        if f.startswith("docs/diagnostics/_audits/"):
            return Classification(
                category="audit-report-grammar-example",
                reason="audit-doc literal mention of the annotation grammar (not a real annotation)",
                issue_ref="n/a",
            )

    if cid == "cat1.bare-path" and f.startswith("docs/diagnostics/_audits/"):
        return Classification(
            category="audit-bare-path",
            reason="audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path)",
            issue_ref="n/a",
        )

    if cid == "cat1.bare-path" and f.startswith("docs/retro/"):
        return Classification(
            category="retro-bare-path",
            reason="retrospective-doc bare-path citation pre-v1.2 (see grandfather-catalog retro-bare-path)",
            issue_ref="n/a",
        )

    if cid == "cat1.bare-path" and (
        f == "docs/integrity-toolkit-spec.md"
        or f.startswith("tools/integrity/docs/")
        or f == "tools/integrity/README.md"
    ):
        return Classification(
            category="toolkit-doc-bare-path",
            reason="toolkit-doc bare-path citation pre-v1.2 (see grandfather-catalog toolkit-doc-bare-path)",
            issue_ref="n/a",
        )

    if cid == "cat1.bare-path" and "LeniaNDK.py" in msg:
        return Classification(
            category="deferred-upstream-bare-path",
            reason="deferred-upstream-bare-path citation (Chakazul/LeniaNDK pending vendoring decision per ground-truth-sources.md)",
            issue_ref="n/a",
        )

    if cid == "cat1.bare-path":
        return Classification(
            category="other-cat1-bare-path",
            reason="bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog)",
            issue_ref="n/a",
        )

    return Classification(
        category="other-cat1",
        reason="grandfathered-pre-v1 (see grandfather-catalog other-cat1)",
        issue_ref="n/a",
    )


def comment_form_for(file_path: str) -> str:
    """Return the comment-form template for a file by extension.

    Template contains `{body}` placeholder for the annotation body
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    (everything after `integrity-allow: `)."""
    p = file_path.lower()
    if p.endswith((".py", ".pyi")):
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        return "# integrity-allow: {body}"
    if p.endswith((".md", ".rst")):
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        return "<!-- integrity-allow: {body} -->"
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    return "// integrity-allow: {body}"


# Fence machinery moved to integrity.common.annotations in v1.1 (A.5)
# to make it importable by the parser and suppressor. Re-imported here
# to preserve the grandfather.py API (callers that imported from this
# module continue to work).
from integrity.common.annotations import (  # noqa: E402
    _FENCE_RE,
    is_inside_fenced_block,
)


# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
def comment_form_for_md_inside_fence(fence_lang: str | None) -> str:
    """Pick a comment form for an annotation inside a markdown code block."""
    if fence_lang is None:
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        return "// integrity-allow: {body}"
    lang = fence_lang.lower()
    if lang in ("python", "py", "toml", "yaml", "yml", "sh", "bash", "ini", "cfg"):
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        return "# integrity-allow: {body}"
    if lang in ("json",):
        # JSON has no comments; fall back to a JS-style line comment, which
        # the integrity parser will read but JSON validators won't.
        # Real JSON code-block annotations are best avoided.
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        return "// integrity-allow: {body}"
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    return "// integrity-allow: {body}"


def annotation_already_present(prev_line: str, check_id: str) -> bool:
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    """True if `prev_line` already carries an `integrity-allow:`
    annotation that covers `check_id` (specifically or via category
    wildcard)."""
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    if "integrity-allow:" not in prev_line:
        return False
    cat = check_id.split(".")[0]
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    wildcard = f"{cat}.*"
    return check_id in prev_line or wildcard in prev_line

# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a

def render_annotation_line(
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    findings_on_line: list[Finding],
    file_path: str,
    file_lines: list[str],
    line_zero_indexed: int,
) -> list[str]:
    """Render the annotation comment line(s) to insert above
    file_lines[line_zero_indexed] for the given group of same-target
    findings."""
    classifications = [(f, classify(f)) for f in findings_on_line]
    categories = {c.category for _, c in classifications}

    if file_path.lower().endswith((".md", ".rst")):
        in_fence, fence_lang = is_inside_fenced_block(file_lines, line_zero_indexed)
        if in_fence:
            template = comment_form_for_md_inside_fence(fence_lang)
        else:
            template = comment_form_for(file_path)
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    else:
        template = comment_form_for(file_path)

    if len(categories) == 1:
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        check_ids_on_line = {f.check_id for f in findings_on_line}
        cat_prefix = next(iter(check_ids_on_line)).split(".")[0]
        if len(check_ids_on_line) > 1:
            check_id_for_annotation = f"{cat_prefix}.*"
        else:
            check_id_for_annotation = next(iter(check_ids_on_line))
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        _, cls = classifications[0]
        body = f"{check_id_for_annotation}; {cls.reason}; {cls.issue_ref}"
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        return [template.format(body=body)]

    out: list[str] = []
    for f, cls in classifications:
        body = f"{f.check_id}; {cls.reason}; {cls.issue_ref}"
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
        out.append(template.format(body=body))
    return out


# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
def collect_findings(repo_root: Path) -> list[Finding]:
    """Run the integrity toolkit in JSON mode and parse non-suppressed findings."""
    result = subprocess.run(
        ["python3", "-m", "integrity", "--output", "json",
         "--no-audit-log", "--mode", "warn-only"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode not in (0, 1):
        raise RuntimeError(
            f"integrity toolkit exited {result.returncode}: {result.stderr}"
        )
    data = json.loads(result.stdout)
    findings: list[Finding] = []
    for f in data.get("findings", []):
        if f.get("suppressed"):
            continue
        findings.append(Finding(
            check_id=f["check_id"],
            file=f["file"],
            line=int(f["line"]),
            message=f["message"],
        ))
    return findings


# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
def group_findings_by_target(
    findings: Iterable[Finding],
) -> dict[tuple[str, int], list[Finding]]:
    """Group findings by (file, target_line)."""
    grouped: dict[tuple[str, int], list[Finding]] = {}
    for f in findings:
        key = (f.file, f.line)
        grouped.setdefault(key, []).append(f)
    return grouped


def apply_annotations(
    repo_root: Path,
    dry_run: bool,
    sweep_live_source: bool = False,
    force_sweep_categories: frozenset[str] = frozenset(),
) -> tuple[int, int, dict[str, int], int]:
    """Apply suppression annotations for every collected finding.

    Returns (files_modified, annotations_added, category_counts, live_source_skipped).

    Live-source skip logic (v1.2 P1.8 + v1.2 A.2 Decision 4):
      Default: protect other-cat1 / other-cat1-bare-path findings on
      LIVE-SOURCE paths from sweep (attribute, do not sweep).
      sweep_live_source=True: bypass for ALL live-source other-cat1*
      findings (the v1.1 default behavior; equivalent to "force-sweep
      other-cat1 + other-cat1-bare-path").
      force_sweep_categories: bypass live-source protection per named
      category, leaving all other LIVE-SOURCE protections in place.
      Used by v1.2 A.2 commit 4 to sweep toolkit-own-unused only.
    """
    findings = collect_findings(repo_root)

    # P1.8 -- protect LIVE-SOURCE other-cat1 findings from sweep by default.
    # Named classifier categories (cat2-stack-*-unused, live-shader-1810, etc.)
    # remain sweepable on live-source paths by design -- only the heterogeneous
    # other-cat1 fallthrough bucket is dangerous to auto-annotate on live code.
    # v1.2 A.2 Decision 4 extends the bypass surface: force_sweep_categories
    # opts specific named categories (e.g., toolkit-own-unused) into the sweep
    # even when their findings sit on LIVE-SOURCE paths, without disabling the
    # default protection for all other categories.
    live_source_skipped = 0
    if not sweep_live_source:
        kept: list[Finding] = []
        for f in findings:
            cat = classify(f).category
            if is_fallthrough_category(cat) and is_live_source_path(f.file):
                if cat in force_sweep_categories:
                    kept.append(f)
                    continue
                live_source_skipped += 1
                continue
            kept.append(f)
        findings = kept

    grouped = group_findings_by_target(findings)

    files_modified = 0
    annotations_added = 0
    category_counts: dict[str, int] = {}

    by_file: dict[str, list[int]] = {}
    for (fp, ln) in grouped:
        by_file.setdefault(fp, []).append(ln)
    for fp in by_file:
        by_file[fp].sort(reverse=True)

    for file_path, lines_desc in by_file.items():
        abs_path = repo_root / file_path
        if not abs_path.is_file():
            continue
        try:
            content = abs_path.read_text(encoding="utf-8")
        except OSError:
            continue
        file_lines = content.split("\n")

        modified_this_file = False

        for target_line in lines_desc:
            zero_idx = target_line - 1
            if zero_idx < 0 or zero_idx >= len(file_lines):
                continue

            findings_on_line = grouped[(file_path, target_line)]

            if zero_idx > 0:
                prev = file_lines[zero_idx - 1]
                covered = all(
                    annotation_already_present(prev, f.check_id)
                    for f in findings_on_line
                )
                if covered:
                    continue

            new_lines = render_annotation_line(
                findings_on_line, file_path, file_lines, zero_idx
            )

            file_lines[zero_idx:zero_idx] = new_lines
            annotations_added += len(new_lines)
            modified_this_file = True

            for f in findings_on_line:
                cls = classify(f)
                category_counts[cls.category] = category_counts.get(cls.category, 0) + 1

        if modified_this_file:
            files_modified += 1
            if not dry_run:
                abs_path.write_text("\n".join(file_lines), encoding="utf-8")

    return files_modified, annotations_added, category_counts, live_source_skipped
