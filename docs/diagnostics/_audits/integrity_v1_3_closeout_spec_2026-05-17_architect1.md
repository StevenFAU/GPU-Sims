---
title: "Integrity Toolkit v1.3 Closeout — Execution Spec"
date: 2026-05-17
author: architect1
status: draft
audience: Claude Code (executor)
sibling-docs:
  - docs/integrity-toolkit-spec.md
  - docs/retro/integrity-toolkit-v1.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/retro/integrity-toolkit-v1.2-bolt-ons.md
  - docs/retro/integrity-toolkit-v1.3-candidates.md
  - docs/retro/integrity-toolkit-v1.3-batch1-part-a.md
  - docs/retro/integrity-toolkit-v1.3-batch1-part-b.md
---

# Integrity Toolkit v1.3 Closeout — Execution Spec

## 0. Execution preamble (read this first)

You are Claude Code executing the v1.3 closeout batch — eight commits that
ship the remaining banked items (rewrite-stale-reasons, T2.1, T2.2, T2.3,
T3.1–T3.4, Part-C) and mark the v1 milestone closed. After this batch,
the toolkit is in steady state: no further v1.x work is planned. v2
horizon items (Cat 4 runtime, type-aware Cat 2, etc.) remain available
if a forcing function appears but are not on a planned schedule.

### 0.1 Hard rules

1. **Execute every file creation, modification, and removal specified.
   Do not skip any.**

2. **Synced repo state is authoritative over this spec.** Every verbatim
   claim about file contents in this spec was anchored against project
   knowledge during drafting; the architect did not run a fresh probe
   against current HEAD (deliberate scope choice — see § 1.5). Re-anchor
   every cited line number, function signature, and import path via
   `view` or `grep -n` before each edit. If anything has moved, **pause
   and surface; do not silently adapt.**

3. **No line numbers carried from this spec into edits without
   re-verification.** Every line citation here is an anchor sketch per
   Convention K. The Part-B retro § 3.3 documented this failure mode
   explicitly: cite-without-re-grep at execution time is the dominant
   class of architect-1 fabrication.

4. **Land commits in the order given.** Commits 1 through 7 are
   substantive; commit 8 is SHA back-fill per Convention #12. Each
   commit's verification block must pass before starting the next.
   Do not interleave.

5. **One audit report per commit.** Path pattern:
   `docs/diagnostics/_audits/integrity_v1_3_closeout_commit<N>_landing_2026-05-17.md`.
   Front-matter and structure mirror the v1.3 part-A commit-landing
   reports. Commit 8 gets an audit report too (Part-A retro § 6 banked
   the omission).

6. **SHA back-fill is a separate follow-up commit, never `--amend`.**
   Per Convention #12.

7. **Pull-rebase before every commit.** Concurrent toolkit work is not
   expected during this batch (it's a closeout), but discipline applies.

8. **`python3`, not `python`.** Per Convention E.

9. **Audit-prose freshness (Convention F).** Verify every quantitative
   claim and SHA reference against current disk immediately before
   committing each audit report. Discrepancies become addenda, not
   body edits.

10. **Live-source-stays-red discipline (P1.8).** No commit in this batch
    requires `--force-sweep-category` or `--sweep-live-source`. If any
    commit produces live-source findings that look like they want
    sweeping, **pause and surface** — the intent is closeout, not
    live-source cleanup.

11. **The toolkit must remain green-relative-to-baseline across every
    commit.** Baseline at probe time (per Part-B § 1 quantitative table):
    60 hard-fails. Hard-fail count may grow by N per commit if N new
    audit reports are added that themselves carry citations; this is
    expected sweep-companion behavior (Convention J) and not a regression.
    Use the commit's audit-report citations as the predicted growth
    delta.

12. **SHA placeholder discipline in audit reports.** Each audit report
    for commits 1-7 that references a downstream sibling commit's SHA
    MUST use the literal placeholder `<COMMIT_N_SHA>` (e.g., `<COMMIT_2_SHA>`
    in commit 1's audit report referring to commit 2). Commit 8 then
    grep-resolves every placeholder against `git log --grep` and
    replaces with the resolved 7-char SHA. Without explicit placeholder
    use, commit 8 has nothing to back-fill and the convention silently
    breaks. Per Part-A commit 4 banked observation.

### 0.2 Pause-and-surface triggers

Pause and surface to the user (do not silently adapt) if any of these fire:

- Any file cited in this spec has materially changed shape since project
  knowledge was last synced. "Materially changed" means: function
  signatures differ, imports differ, the structural template the spec
  references no longer exists.
- The rewrite-stale-reasons mode's `--rewrite-stale-reasons` flag,
  after dry-run, proposes rewrites on more than 30 annotations.
  Spec's prediction is ~10–20; more than 30 means either the
  conservative-rewrite logic is too permissive or a concurrent commit
  has churned the classifier state.
- T2.3's Stack C single-parse refactor changes the test suite's pass
  count (it should remain at whatever the post-commit-1 baseline is —
  T2.3 is pure refactor, zero behavior change).
- T2.1's CI workflow check (paired-sweep detection) flags more than
  20 cat1-scannable changed files in the diff that the workflow is
  added to. If many, the heuristic is too sensitive; tune before
  committing.
- The fossil cleanup in commit 6 surfaces project-state.md changes
  beyond the three `cat1.intra-repo` annotation lines named (559, 593,
  666 — confirmed by probe § B.8). The adjacent `cat1.bare-path`
  annotations on the next line (560, 594, 667) stay in place; probe
  § G.2 found they're not actually suppressing the findings below
  them, but removing them is out of scope for closeout.
- Any test added by this batch fails on first run.

### 0.3 Decisions log

Decisions that architect-1 made in this spec without architect-2 review,
per Part-B retro § 5.4. Each is named here and elaborated where it lands.

| ID | Item | Decision |
|----|------|----------|
| D1 | Rewrite-stale-reasons scope | **Conservative.** Rewrite annotation reason text ONLY when `classify(finding).category` differs from the parsed category embedded in the existing annotation's reason. Wording-only diffs (same category, different prose) are NOT rewritten. Rationale: surgical, minimizes history churn, captures the actual T1.1-style reclassification case the retro flagged. |
| D2 | T2.1 enforcement level | **Medium (CI check).** Pre-commit hooks slow every edit; soft hasn't held across three retros. CI check that fails when cat1-scannable live-source files changed in the PR diff without a paired grandfather-sweep commit. See commit 4 § 5.B for the heuristic. |
| D3 | T2.2 mechanization scope | **Sibling tool, not gate-integrated.** `tools/integrity/scripts/audit_prose_freshness.py` is a standalone pre-commit utility that scans backtick-fenced `path:line` citations in spec/retro/audit prose and verifies they resolve. NOT registered as an integrity check (cat1.intra-repo already covers the same surface and is grandfathered for these paths by design). |
| D4 | T3.1 (A.9) audit-citation exclusion | **Rejected.** Keep-and-bucket preserves per-finding audit attribution; the grandfather catalog already segments these into named categories. Quantified ~67% pool collapse isn't worth losing the audit trail. If reconsidered, that's v2 scope. |
| D5 | T3.2 (conventions home) | **`tools/integrity/docs/conventions.md`.** Toolkit-scoped conventions live with the toolkit. Repo-root `CONVENTIONS.md` over-claims (conventions outside the integrity chain — e.g., from `docs/retro/phase11.md` — are not the same lineage). |
| D6 | T3.3 (numbering taxonomy) | **Four-bucket taxonomy** per Part-A retro § 5.2: spec-time discipline / execution-time discipline / batch-coordination / design-taste. Conventions keep their A–K letters (alphabetic-as-historical-anchor) but get grouped under the bucket headers. |
| D7 | T3.4 (architect-2 backlog) | **Formally bank as unresolved** for items 1–4 of v1.1 batch-1 retro § 6.4, each with explicit "no architect-2 review obtained; architect-1 decision deferred to v2 reconsideration" disclosure in the conventions doc. |

These decisions are landed in commit 5 (conventions doc + T3 decisions
document) with explicit disclosure. Push back on the user BEFORE landing
if any of D1–D7 looks wrong from the executing session's perspective.

## 1. Goals & load-bearing decisions

### 1.1 What this batch closes

| Commit | Item | Source |
|--------|------|--------|
| 1 | **Rewrite-stale-reasons** — `--rewrite-stale-reasons` sweep mode | Part-B retro § 4.1 |
| 2 | **T2.3** — Stack C runtime single-parse refactor | Roadmap T2.3; v1 retro § 4 |
| 3 | **T2.2** — Audit-prose freshness sibling tool | Roadmap T2.2; v1.1 retro § 6.3 |
| 4 | **T2.1** — Sweep enforcement (CI check, medium) | Roadmap T2.1; v1.1 retro § 6.2 |
| 5 | **T3.1 → T3.4** decisions + conventions home + taxonomy | Roadmap T3.1–T3.4 |
| 6 | **Part-C** — project-state.md fossil cleanup | Part-B retro Decision 6; probe § K.6 |
| 7 | **v1-closed marker** — project-state.md phase ledger | This spec |
| 8 | SHA back-fill | Convention #12 |

### 1.2 What this batch does NOT do

- Does **not** ship v2 horizon items (Cat 4 runtime, multi-line citation
  grammar, type-aware Cat 2, GPU shader coverage via headless). Banked.
- Does **not** drain remaining other-cat1 or other-cat1-bare-path
  findings beyond what the commit-1 rewrite-stale-reasons pass touches.
  Live-source residue stays attributed; that's the design.
- Does **not** add new check modules to the main gate. T2.2 ships as a
  sibling tool; T2.1 ships as a workflow check, not an integrity check.
- Does **not** seek architect-2 review. The user has formally opted out;
  commit 5 documents this and resolves the previously-pending items
  with explicit disclosure.

### 1.3 Parallel composition (optional)

The spec is written for serial execution in a single Claude Code session
because that's the lowest-coordination shape. If the user wants to spawn
multiple sessions, the following groupings compose without merge
conflict:

- **Session A:** commits 1 → 3 (touches `grandfather.py`,
  `grandfather_sweep.py`, new sibling script under `tools/integrity/scripts/`)
- **Session B:** commits 2 → 4 (touches `cat2_contracts/stack_c.py`,
  `.github/workflows/integrity.yml` — independent surfaces)
- **Session C:** commits 5 → 6 → 7 (touches `tools/integrity/docs/conventions.md`
  new, `project-state.md`)
- **Whichever session lands last** runs commit 8 (SHA back-fill)
  covering all preceding commits.

If parallel mode is chosen, each session must announce its commits in
its first audit report's § A so coordination is observable.

Default: serial. The retros document that concurrent sessions add real
coordination friction (the rewrite-stale-reasons commit would itself benefit from landing
first under serial), and an 8-commit batch is not large enough to
justify the overhead.

### 1.4 Quantitative end-state predictions

Anchor-sketch (verify at execution time per Convention K):

- **Commit 1 (rewrite-stale-reasons):** ~80 LOC added in `grandfather.py` +
  `grandfather_sweep.py`; ~6 new tests in
  `test_grandfather_sweep.py`. Dry-run on landing should propose
  rewrites on 10–20 annotations across audit-doc and retro-doc paths
  (the conservative-rewrite scope; pure historical fixups from
  pre-Part-B classifications that no longer match current `classify()`
  output). Anything outside that range is a pause-and-surface trigger.
- **Commit 2 (T2.3):** Net diff in `stack_c.py` ~+50/−80 LOC after the
  refactor (single-parse with USR cache). Test suite count unchanged.
  Stack C scan time: target ~50% reduction (95s → ~50s); pause if no
  reduction observed in CI logs of the first push.
- **Commit 3 (T2.2):** New `audit_prose_freshness.py` ~120 LOC + 4
  tests in a new `test_audit_prose_freshness.py`. Standalone tool;
  does not touch gate behavior.
- **Commit 4 (T2.1):** `.github/workflows/integrity.yml` gains a new
  job step (~25 lines of yaml). New script
  `tools/integrity/scripts/check_paired_sweep.py` ~80 LOC + 3 tests.
- **Commit 5 (conventions doc):** New `tools/integrity/docs/conventions.md`
  ~400 LOC consolidating A–K with disclosure on T3 decisions. No code
  touches except potentially `tools/integrity/README.md` getting a
  one-line pointer.
- **Commit 6 (fossil cleanup):** 3 deletions in `project-state.md`
  at the lines named in Part-B Decision 6. Zero new lines.
- **Commit 7 (v1-closed marker):** ~15 lines added to `project-state.md`
  in the phase ledger and a closing § paragraph.
- **Commit 8 (SHA back-fill):** ≤10 lines per audit report being
  back-filled.

Test suite end-state: 183 (Part-B baseline) + ~16 = ~199 passing.

Gate hard-fail end-state: ~70–80 (60 baseline + audit-report citations
from 8 new audit-doc files); does not represent a regression. The
sweep companion in commit 1 (rewrite-stale-reasons dry-run + apply) will likely
absorb some of this growth.

### 1.5 Pre-spec probe status

A pre-spec probe was executed by Claude Code at HEAD `a1c9121` and the
report lives at
`docs/diagnostics/_audits/integrity_v1_3_closeout_probe_2026-05-17_architect1-via-claude-code.md`.
The probe raised 8 flags (G.0–G.9 in the report); 4 are resolved
inline in this spec (G.1 § 2.C.1 wiring, G.3 § 4.C.1 citation regex,
G.6 § 5.C.2 GitHub Actions fetch pattern, G.7 § 6.C.1 Convention F
formatting), 2 are absorbed as bonus items (G.5 README cleanup folded
into commit 7, G.4 banked as a future micro-task), and 2 are banked
as known issues in commit 7's v1-closed marker (G.2 project-state.md
suppression bug). Quantitative baseline (G.8) and convention text
drift (G.9) both PASSed.

This spec's line-number citations and structural claims are anchored
against the probe report. Hard Rule 3 still applies — re-verify at
edit time. Convention K still applies — anchor-sketch labeling on any
inferred content.

If a discrepancy at execution time turns out to be material (e.g., the
function whose body this spec describes has been refactored between
probe time and execution time), pause-and-surface per Hard Rule 2.

---

## 2. Commit 1 — Rewrite-stale-reasons feature (Part-B § 4.1 implementation)

### 2.A Purpose & sources

Per Part-B retro § 4.1: when a new classifier rule re-routes existing
findings from one category to another, the existing inline annotations
continue to carry the old category's reason text. The sweep's
`annotation_already_present` check matches on `check_id`, not on
`suppression_reason`, so already-annotated findings are correctly
identified as covered and skipped — but the reason text remains stale.

Without this feature, every classifier-rule addition that reclassifies
existing findings requires manual reason-text rewriting (as happened in
Part-B commit 2: 23 annotation lines rewritten across 7 files by hand).

**Naming note.** Part-B retro § 4.1 calls this "Convention I." That is
a collision — the letter I was already used in the v1.2 bolt-ons retro
§ 4.3 for "Cross-batch scope discipline." This spec treats the
rewrite-stale-reasons capability as a *feature* (not a convention) and
preserves the existing Convention I letter for cross-batch scope
discipline. The conventions doc landed in commit 5 documents the
collision explicitly.

Scope per Decision D1 (Hard Rule 0.3): rewrite ONLY when category
changed, not when wording changed.

### 2.B Pre-edit verification

```bash
view tools/integrity/integrity/grandfather.py
grep -n "annotation_already_present\|FALLTHROUGH_CATEGORIES\|apply_annotations" \
  tools/integrity/integrity/grandfather.py
view tools/integrity/scripts/grandfather_sweep.py
grep -n "argparse\|--sweep-live-source\|--force-sweep-category" \
  tools/integrity/scripts/grandfather_sweep.py
```

Confirm at HEAD:

- `apply_annotations(repo_root, dry_run, sweep_live_source, force_sweep_categories)`
  returns `tuple[int, int, dict[str, int], int]` (probe § B.1
  confirmed: `grandfather.py:425-430`).
- `classify(finding) -> Classification` (probe § B.1: `grandfather.py:110`).
- `Classification` dataclass fields are `category`, `reason`, `issue_ref`
  (probe § B.1: `grandfather.py:31-36`, frozen=True).
- `Finding` dataclass fields are `check_id`, `file`, `line`, `message`
  (probe § B.1: `grandfather.py:23-28`). **No `suppressed` field
  currently exists** (probe § G.1).
- `FALLTHROUGH_CATEGORIES: frozenset[str]` (probe § B.1: line 76) and
  `is_fallthrough_category(category) -> bool` (line 82).
- `annotation_already_present(prev_line: str, check_id: str) -> bool`
  (probe § B.1: line 322).
- Sweep CLI has `--dry-run`, `--repo-root`, `--sweep-live-source`,
  `--force-sweep-category` (probe § B.2).

If any expectation fails, **re-anchor before editing.**

### 2.C File modifications

#### 2.C.1 `tools/integrity/integrity/grandfather.py`

Add a new helper function for category-classification-from-existing-annotation,
and a new top-level entrypoint `rewrite_stale_reasons` that walks the
repo and rewrites annotations whose embedded category no longer matches
`classify()`.

Insert AFTER `apply_annotations`'s closing return (verify via
`grep -n "^def " tools/integrity/integrity/grandfather.py` and pick the
appropriate insertion point — should be near the end of the file, after
`apply_annotations`).

Anchor sketch (verify shape at edit time):

```python
# integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a
def _parsed_category_from_reason(reason: str) -> str | None:
    """Extract the category name from an existing annotation's reason string.

    Annotation reasons follow the pattern produced by classify(); the category
    name appears as a substring (the snapshot._extract_category logic is the
    canonical reference). Returns None if no known category matches.
    """
    from integrity.snapshot import _extract_category, _KNOWN_CATEGORIES  # noqa: PLC0415
    cat = _extract_category(reason)
    return cat if cat in _KNOWN_CATEGORIES else None


def rewrite_stale_reasons(
    repo_root: Path,
    dry_run: bool,
) -> tuple[int, int, list[tuple[str, int, str, str]]]:
    """Rewrite annotation reason strings whose embedded category no longer
    matches classify() output. Conservative scope (D1): only rewrites when
    parsed-category differs from current-classify-category; wording-only
    drift is NOT rewritten.

    Note: this iterates ALL findings (not just suppressed ones). The
    inner annotation-search loop handles the "no annotation above this
    finding" case implicitly by parse_annotation_line returning None.
    Per probe G.1, no `is_suppressed` helper nor `Finding.suppressed`
    field exists; this implementation does not require either.

    Returns:
        (files_modified, annotations_rewritten, rewrites_detail)
        where rewrites_detail is a list of (path, line, old_cat, new_cat).
    """
    from integrity.common.annotations import parse_annotation_line  # noqa: PLC0415

    findings = collect_findings(repo_root)
    rewrites: list[tuple[str, int, str, str]] = []
    by_path: dict[str, list[tuple[int, str, str]]] = {}

    for f in findings:
        # Locate the annotation line (annotation grammar suppresses the
        # immediately-following line; conservative search ±2 to tolerate
        # fenced-code-block / multi-annotation-stack context like
        # project-state.md lines 559-560 where two annotations stack).
        path = repo_root / f.file
        try:
            lines = path.read_text(encoding="utf-8").splitlines()
        except (OSError, UnicodeDecodeError):
            continue
        for ann_idx in range(max(0, f.line - 3), min(len(lines), f.line)):
            ann_line = lines[ann_idx]
            parsed = parse_annotation_line(ann_line)
            if parsed is None:
                continue
            ann_check_id, ann_reason, _ = parsed
            # Match annotation's check_id against the finding's check_id
            # (specifically or via category wildcard `catN.*`).
            if ann_check_id != f.check_id and not (
                ann_check_id.endswith(".*")
                and f.check_id.startswith(ann_check_id[:-2] + ".")
            ):
                continue
            old_cat = _parsed_category_from_reason(ann_reason)
            new_classification = classify(f)
            new_cat = new_classification.category
            # D1 conservative scope: rewrite only when category transitioned
            # to a different known category. Category renames where the old
            # category is no longer in _KNOWN_CATEGORIES return None and
            # are skipped (handle renames in a future explicit operation).
            if old_cat is None or old_cat == new_cat:
                continue
            rewrites.append((f.file, ann_idx + 1, old_cat, new_cat))
            by_path.setdefault(f.file, []).append(
                (ann_idx, ann_line, new_classification.reason)
            )
            break  # one annotation per finding; stop searching

    files_modified = 0
    annotations_rewritten = 0
    if not dry_run:
        for file_rel, edits in by_path.items():
            path = repo_root / file_rel
            try:
                lines = path.read_text(encoding="utf-8").splitlines(keepends=False)
            except (OSError, UnicodeDecodeError):
                continue
            modified = False
            for ann_idx, _ann_line, new_reason in edits:
                # Reconstruct the annotation line with new reason.
                # Preserve comment-form (// vs # vs <!-- -->) and indentation.
                new_line = _rewrite_annotation_reason(lines[ann_idx], new_reason)
                if new_line != lines[ann_idx]:
                    lines[ann_idx] = new_line
                    modified = True
                    annotations_rewritten += 1
            if modified:
                # Preserve trailing newline if present.
                original_text = path.read_text(encoding="utf-8")
                content = "\n".join(lines)
                if original_text.endswith("\n"):
                    content += "\n"
                path.write_text(content, encoding="utf-8")
                files_modified += 1
    else:
        # Dry-run: count what would be rewritten without writing.
        annotations_rewritten = len(rewrites)
        files_modified = len(by_path)

    return files_modified, annotations_rewritten, rewrites


def _rewrite_annotation_reason(line: str, new_reason: str) -> str:
    """Rewrite the reason segment of an annotation line in place.

    Preserves the comment-form (// vs # vs <!-- -->) and the check_id +
    issue_ref segments. Annotation grammar:
      <comment-prefix> integrity-allow: <check_id>; <reason>; <issue_ref> <comment-suffix>
    """
    import re  # noqa: PLC0415
    # Match the reason segment between the first and second `;` after `integrity-allow:`
    # integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a
    pattern = re.compile(
        # integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a
        r"(integrity-allow:\s*[^;]+;\s*)([^;]+?)(\s*;\s*(?:#\d+|n/a))"
    )
    return pattern.sub(lambda m: f"{m.group(1)}{new_reason}{m.group(3)}", line, count=1)
```

**Implementation notes (revised per probe G.1):**

- The original spec draft assumed an `is_suppressed(f)` helper that
  doesn't exist (probe § G.1). The revised function above iterates all
  findings and lets `parse_annotation_line` return `None` for the
  no-annotation case — the loop continues naturally. No new helper or
  dataclass field is required.
- `_extract_category` and `_KNOWN_CATEGORIES` live in
  `tools/integrity/integrity/snapshot.py` at lines 27-48 / 72-81 per
  probe § B.7. The lazy import inside `_parsed_category_from_reason`
  avoids circular-import risk.
- The ±3 line search window accommodates multi-annotation stacks like
  the project-state.md lines 559-560 case (probe § B.8): when two
  annotations stack on consecutive lines preceding a finding, the
  search finds whichever annotation matches the finding's `check_id`
  first.
- Per D1, category renames where the old category no longer appears in
  `_KNOWN_CATEGORIES` are silently skipped (the `_parsed_category_from_reason`
  returns None). This is intentional: rename operations are a separate
  explicit concern, not covered by the rewrite-stale-reasons mode.

#### 2.C.2 `tools/integrity/scripts/grandfather_sweep.py`

Add a new flag `--rewrite-stale-reasons`. When set, the CLI invokes
`rewrite_stale_reasons` instead of `apply_annotations`. The flags are
mutually exclusive (rewriting stale reasons is a different operation
from applying new annotations).

Insert after the existing `--force-sweep-category` argparse line:

```python
    parser.add_argument(
        "--rewrite-stale-reasons",
        action="store_true",
        help=(
            "Rewrite the reason text of existing annotations whose embedded "
            "category no longer matches the current classifier output. "
            "Conservative: only rewrites when category changed, not when "
            "wording-only diffs are present. Mutually exclusive with the "
            "normal sweep modes; use after adding new classifier rules."
        ),
    )
```

Modify `main()` to branch on the flag. Anchor sketch:

```python
    if ns.rewrite_stale_reasons:
        if ns.sweep_live_source or ns.force_sweep_category:
            print("error: --rewrite-stale-reasons is mutually exclusive with sweep flags", file=sys.stderr)
            return 2
        from integrity.grandfather import rewrite_stale_reasons  # noqa: PLC0415
        files, anns, rewrites = rewrite_stale_reasons(root, ns.dry_run)
        label = "would rewrite" if ns.dry_run else "rewrote"
        print(f"grandfather-sweep: {label} {anns} annotation reasons across {files} files")
        if rewrites:
            # Group by (old_cat, new_cat) for summary
            from collections import Counter  # noqa: PLC0415
            by_transition = Counter((r[2], r[3]) for r in rewrites)
            for (old_cat, new_cat), count in by_transition.most_common():
                print(f"  {old_cat} -> {new_cat}: {count}")
        return 0
    # ...existing apply_annotations branch unchanged...
```

#### 2.C.3 `tools/integrity/tests/test_grandfather_sweep.py`

Add 6 new tests at the end of the file. Anchor sketch (the test fixtures
follow the existing `_f` helper pattern):

```python
def test_rewrite_stale_reasons_category_changed_rewrites(tmp_path: Path) -> None:
    """When a finding's annotation has a stale category in its reason and
    classify() now returns a different category, rewrite the reason in place."""
    # Setup: create a fixture file with a stale annotation
    # Verify: rewrite_stale_reasons(dry_run=False) rewrites it
    ...


def test_rewrite_stale_reasons_wording_diff_only_skipped() -> None:
    """When the reason wording differs but the category is the same, skip
    (D1 conservative scope)."""
    ...


def test_rewrite_stale_reasons_dry_run_no_writes(tmp_path: Path) -> None:
    """Dry-run mode reports the rewrites without modifying files."""
    ...


def test_rewrite_stale_reasons_preserves_comment_form() -> None:
    """A // ... rewrite stays //; a # ... stays #; a <!-- ... --> stays
    enclosed in HTML comment markers."""
    ...


def test_rewrite_stale_reasons_mutually_exclusive_with_sweep_flags() -> None:
    """CLI rejects --rewrite-stale-reasons combined with --sweep-live-source
    or --force-sweep-category."""
    ...


def test_rewrite_stale_reasons_no_match_returns_empty() -> None:
    """When no annotations have category drift, the operation is a no-op."""
    ...
```

Fill in fixture bodies by mirroring existing tests' shape; verify by
running them in the verification block.

### 2.D Verification

```bash
cd tools/integrity
python3 -m pytest tests/ -q
# Expected: previous count + 6 new tests, all passing.

# Dry-run the sweep against current state.
python3 tools/integrity/scripts/grandfather_sweep.py --rewrite-stale-reasons --dry-run
# Expected: 10-20 rewrites proposed across audit-doc / retro-doc files,
# all category transitions involving toolkit-doc-snapshot, retro-doc-snapshot,
# or project-state-snapshot (the three categories from Part-B T1.1).
# If >30 rewrites proposed, pause-and-surface.

# Apply the sweep.
python3 tools/integrity/scripts/grandfather_sweep.py --rewrite-stale-reasons
# Expected output mirrors dry-run with "rewrote N" instead of "would rewrite N".

# Re-verify the gate.
python3 -m integrity --mode strict --no-audit-log
# Expected: hard-fail count unchanged (rewriting suppression text doesn't
# change finding-suppression status, only the reason wording).

# Mutual-exclusion check.
python3 tools/integrity/scripts/grandfather_sweep.py --rewrite-stale-reasons --sweep-live-source
# Expected: exit code 2, error message on stderr.
```

If any check fails, **pause and surface** at this commit.

### 2.E Commit message

```
feat(integrity): --rewrite-stale-reasons sweep mode (v1.3 closeout commit 1)

Adds `rewrite_stale_reasons` to `integrity.grandfather` and a matching
`--rewrite-stale-reasons` flag to `grandfather_sweep.py`. Detects
annotations whose embedded category no longer matches classify() output
and rewrites the reason in place. Conservative scope per Decision D1
(spec § 0.3): rewrites only when category changed, not when wording-only
diffs are present.

Mutually exclusive with --sweep-live-source / --force-sweep-category to
prevent operator confusion (the rewrite and the sweep are distinct
operations on distinct entry points).

Closes Part-B retro § 4.1 banked observation. The Part-B commit 2 manual
reason-rewrite pass (23 annotations across 7 files by hand) is no longer
necessary for future classifier-rule additions.

Note: Part-B § 4.1 called this "Convention I" but the letter I was
already used in v1.2 bolt-ons retro § 4.3. This is a feature, not a
convention; the conventions doc landed in v1.3 closeout commit 5
documents the collision.

Tests: +6 in test_grandfather_sweep.py.
```

### 2.F Audit report

Create `docs/diagnostics/_audits/integrity_v1_3_closeout_commit1_landing_2026-05-17.md`.

Front-matter:

```yaml
---
title: "Integrity v1.3 Closeout Commit 1 — Rewrite-stale-reasons sweep mode"
date: 2026-05-17
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md
  - docs/retro/integrity-toolkit-v1.3-batch1-part-b.md
---
```

Body sections (mirror v1.3 part-A commit-landing format):

- **§ A. Change summary** — One-paragraph description of the rewrite mode.
- **§ B. File inventory** — Files modified with diff stats.
- **§ C. Verification** — Verbatim capture of pytest, dry-run sweep,
  apply sweep, gate re-run, mutual-exclusion check.
- **§ D. Behavioral notes** — D1 conservative scope; mutual-exclusion
  rationale; comment-form preservation.
- **§ E. Banked observations** — Any pause-and-surface fires; any
  surprise in the rewrite count vs. spec's 10–20 prediction.
- **§ F. Cross-references** — Part-B retro § 4.1; spec § 2.

---

## 3. Commit 2 — T2.3 Stack C runtime optimization (single-parse refactor)

### 3.A Purpose & sources

Per roadmap T2.3 / v1 retro § 4 / v1.1 batch-1 retro § 6.1 item 8: the
Stack C check `cat2.public-symbol-used-c` parses every translation unit
twice — once for symbol extraction (across `include/gpusims/` headers'
consumer TUs), once for reference finding (across `common/common-cpp/src/`
consumer TUs). The double parse is the dominant contributor to the
~95-second Stack C scan walltime.

Refactor: parse each TU once, run symbol extraction and reference
collection passes over the cached cursor walk.

### 3.B Pre-edit verification

```bash
view tools/integrity/integrity/cat2_contracts/stack_c.py
grep -n "^def \|index.parse\|TranslationUnit" \
  tools/integrity/integrity/cat2_contracts/stack_c.py
# Verify the entry-point function name (run / main / etc.) and identify
# the two parse passes:
grep -n "index = clang.cindex.Index.create()" \
  tools/integrity/integrity/cat2_contracts/stack_c.py
# Expected: TWO occurrences (one in extraction, one in find_references).
view tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_c.py
```

### 3.C File modifications

#### 3.C.1 `tools/integrity/integrity/cat2_contracts/stack_c.py`

Goal: single-parse strategy. The refactor introduces an intermediate
data structure that holds the parsed TU plus its USR-keyed indices, so
both extraction and reference-finding consult the same parse.

Conceptual shape (anchor sketch — verify the actual entry points at
edit time):

```python
@dataclass
class ParsedTU:
    source: Path
    tu: Any                            # clang.cindex.TranslationUnit
    public_symbols: list[PublicSymbol]  # extracted in the same parse
    field_targets_by_name: dict[str, list[PublicSymbol]]


def parse_translation_units(
    repo_root: Path,
    sources: list[Path],
    public_dir: Path,
) -> list[ParsedTU]:
    """Parse each TU once. Returns a list of ParsedTU records with both
    public-symbol extraction and field-target indices computed against the
    same parse."""
    index = clang.cindex.Index.create()
    parsed: list[ParsedTU] = []
    for source in sources:
        if not source.is_file():
            continue
        args = _load_compile_args(repo_root, source)
        try:
            tu = index.parse(str(source), args=args)
        except clang.cindex.TranslationUnitLoadError:
            continue
        # Extraction pass (in-cursor walk).
        symbols: list[PublicSymbol] = []
        seen_usrs: set[str] = set()
        _walk_for_public_decls(tu.cursor, public_dir, symbols, seen_usrs, [])
        # Build field-name index for token-scan pass.
        field_targets_by_name: dict[str, list[PublicSymbol]] = {}
        for s in symbols:
            if s.kind == SymbolKind.CLASS_FIELD:
                field_targets_by_name.setdefault(s.name, []).append(s)
        parsed.append(ParsedTU(
            source=source,
            tu=tu,
            public_symbols=symbols,
            field_targets_by_name=field_targets_by_name,
        ))
    return parsed
```

The extraction entry point (current `extract_public_symbols` or similar
— re-anchor at edit time) becomes a thin wrapper that calls
`parse_translation_units(repo_root, header_sources, public_dir)` and
concatenates `parsed_tu.public_symbols` with deduplication.

The reference-finding entry point (current `find_references`) takes the
parsed-TU list instead of re-parsing. Iterate over `parsed_tu.tu`
running `_collect_refs` and `_collect_field_token_refs` against
`target_usrs` derived from all symbols across all parsed TUs.

The check module's run() function (`public_symbol_used_c.py`) calls
`parse_translation_units` once, derives `target_usrs` and
`consumer_sources` from it, then iterates a single pass that does both
symbol-side dedup and reference-side counting.

#### 3.C.2 USR-dedup hoisting

Move the post-extraction USR deduplication (the
`(file, line, name, kind)` dedup from v1 build-6 retro § F.3) into
the refactored extraction step so it happens once per parse.

#### 3.C.3 Cache invalidation considerations

In-memory single-CI-run lifecycle per the roadmap T2.3 decision. The
`ParsedTU` list is built fresh per `run()` invocation. No on-disk cache.

If a future CI optimization wants on-disk caching, it can build on this
data structure; current scope keeps it in-memory.

#### 3.C.4 Tests

Existing Stack C tests should pass without modification (pure refactor,
zero behavior change). If any test depended on the double-parse pattern
(unlikely; tests should be black-box against the check's verdict), it
needs updating.

Add ONE new performance assertion test (skip-by-default unless an env
var is set):

```python
@pytest.mark.skipif(
    os.environ.get("INTEGRITY_PERF_ASSERTIONS") != "1",
    reason="Performance assertion; set INTEGRITY_PERF_ASSERTIONS=1 to enable",
)
def test_stack_c_single_parse_walltime_under_threshold(tmp_path: Path) -> None:
    """Single-parse refactor target: <50s for the full Stack C scan
    against the real repo. Pre-refactor baseline: ~95s. Skip in default
    CI; CI walltime is observed via the action log."""
    # Run public_symbol_used_c.run() against the repo root; assert <50s.
    ...
```

### 3.D Verification

```bash
cd tools/integrity
python3 -m pytest tests/ -q
# Expected: same count as post-commit-1 baseline (refactor preserves
# behavior; the one new perf-assertion test is skip-by-default).

# Gate state.
python3 -m integrity --mode strict --no-audit-log
# Expected: hard-fail count unchanged from post-commit-1.

# Per-check smoke (Stack C specifically).
python3 -m integrity --check cat2.public-symbol-used-c --no-audit-log
# Expected: same finding count as before the refactor.

# Note CI walltime in the audit report once first push completes.
```

### 3.E Commit message

```
perf(integrity): T2.3 single-parse Stack C refactor (v1.3 closeout commit 2)

Refactors cat2_contracts/stack_c.py to parse each translation unit once
and run both symbol extraction and reference collection against the
cached parse. Eliminates the double-parse pattern that dominated the
~95-second Stack C scan walltime.

Target: ~50% wall-clock reduction (95s -> ~50s). Verified via CI logs;
local pytest runtime is dominated by fixture overhead and is not a clean
benchmark.

Pure refactor, zero behavior change. Test suite count unchanged; one
new opt-in performance-assertion test (skip by default; INTEGRITY_PERF_ASSERTIONS=1
to enable).

Closes roadmap T2.3 / v1 retro § 4 / v1.1 retro § 6.1 item 8.
```

### 3.F Audit report

Path: `docs/diagnostics/_audits/integrity_v1_3_closeout_commit2_landing_2026-05-17.md`.
Mirror commit 1 structure. Include CI walltime observation in § C if the
first push has run by audit-write time; otherwise note "CI walltime to
be captured in commit 8's audit or in a follow-up addendum."

---

## 4. Commit 3 — T2.2 audit-prose freshness sibling tool

### 4.A Purpose & sources

Per roadmap T2.2 / v1.1 batch-1 retro § 6.3: a mechanical check for
load-bearing repo-state assertions in spec/audit/retro prose, verified
against actual disk. Closes the feedback-loop gap from "execution time"
(the existing gate's `cat1.intra-repo`) to "draft time" (a tool the spec
author runs explicitly before committing).

Per Decision D3 (Hard Rule 0.3): scoped as a sibling tool, NOT a gate
check. `cat1.intra-repo` already covers the same surface and the
grandfather catalog deliberately suppresses these findings on audit-doc
and retro-doc paths. Adding the same check to the gate would create
duplicate findings or require ungrandfathering everything. The sibling
tool's value is timing (pre-commit) and surface (the drafter's intent),
not check coverage.

### 4.B Pre-edit verification

```bash
ls tools/integrity/scripts/
# Confirm grandfather_sweep.py + __init__.py exist.
view tools/integrity/integrity/common/annotations.py
# Confirm parse_annotation_line and ANNOTATION_RE shapes for reuse if needed.
```

### 4.C File modifications

#### 4.C.1 `tools/integrity/scripts/audit_prose_freshness.py` (new)

Anchor sketch (~120 LOC):

```python
#!/usr/bin/env python3
"""Audit-prose freshness check (T2.2 sibling tool).

Verifies that backtick-fenced `path:line[-range]` citations in spec,
retro, and audit prose resolve against the actual repo. Designed to be
run pre-commit by a spec/audit author, NOT as part of the integrity gate.

Use:
    python3 tools/integrity/scripts/audit_prose_freshness.py [PATH...]

With no args, scans the conventional set:
    docs/integrity-toolkit-spec.md
    docs/retro/*.md
    docs/diagnostics/_audits/*.md
    project-state.md

Exit codes:
    0 — all citations resolve
    1 — at least one citation failed to resolve (file missing or line out of range)
    2 — bad CLI args
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from integrity.common.repo import find_repo_root


# Match backtick-fenced citations: `path:line` or `path:start-end`.
# Per probe § G.3: bare `host.tld.suffix:port` patterns (like `192.168.1.1:80`)
# match a naive `[A-Za-z0-9_./\-]+\.[A-Za-z0-9_]+:[0-9]+` regex. We tighten
# by requiring EITHER (a) the path contains a `/` (so `Path/to/file.py:42`
# matches but `192.168.1.1:80` does not) OR (b) the file extension is in a
# known source-extension whitelist.
KNOWN_EXTENSIONS = (
    "py", "ts", "tsx", "js", "jsx", "cpp", "hpp", "h", "c", "cc",
    "comp", "glsl", "frag", "vert", "wgsl", "yml", "yaml", "toml",
    "md", "rst", "txt", "json", "cmake",
)
# integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a
CITATION_RE = re.compile(
    r"`(?P<path>"
    # Path-with-slash form (always accept): foo/bar.x or foo/bar
    r"[A-Za-z0-9_./\-]*/[A-Za-z0-9_./\-]+"
    r"|"
    # Single-segment form: must end in a known extension
    r"[A-Za-z0-9_\-]+\.(?:" + "|".join(KNOWN_EXTENSIONS) + r")"
    r")"
    r":(?P<start>\d+)(?:-(?P<end>\d+))?`"
)
# Filter for IP-address false positives even if the patterns above accept them.
IP_PORT_RE = re.compile(r"^\d{1,3}(\.\d{1,3}){3}:\d+$")


# Default paths to scan if no args provided.
DEFAULT_GLOBS = (
    "docs/integrity-toolkit-spec.md",
    "docs/retro/*.md",
    "docs/diagnostics/_audits/*.md",
    "project-state.md",
)


def _resolve_targets(repo_root: Path, args: list[str]) -> list[Path]:
    if args:
        return [Path(a) for a in args]
    targets: list[Path] = []
    for pattern in DEFAULT_GLOBS:
        targets.extend(sorted(repo_root.glob(pattern)))
    return targets


def _check_citation(
    repo_root: Path,
    source_path: Path,
    source_line_idx: int,
    citation_path: str,
    start: int,
    end: int | None,
) -> str | None:
    """Returns an error description if the citation fails, else None."""
    target = repo_root / citation_path
    if not target.is_file():
        return f"{source_path}:{source_line_idx + 1}: citation `{citation_path}:{start}` -> file not found"
    try:
        line_count = sum(1 for _ in target.open(encoding="utf-8"))
    except (OSError, UnicodeDecodeError):
        return f"{source_path}:{source_line_idx + 1}: citation `{citation_path}:{start}` -> file unreadable"
    cite_end = end if end is not None else start
    if start < 1 or cite_end > line_count:
        return (
            f"{source_path}:{source_line_idx + 1}: citation `{citation_path}:{start}"
            f"{'-' + str(end) if end is not None else ''}` -> out of range "
            f"(file has {line_count} lines)"
        )
    return None


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Audit-prose freshness — verify backtick-fenced citations in spec/retro/audit prose",
    )
    parser.add_argument(
        "paths", nargs="*",
        help="Files to scan. Default: docs/integrity-toolkit-spec.md, docs/retro/*.md, "
             "docs/diagnostics/_audits/*.md, project-state.md",
    )
    parser.add_argument(
        "--repo-root", type=Path, default=None,
        help="Override repo root (default: auto-detect via git)",
    )
    parser.add_argument(
        "--quiet", action="store_true",
        help="Suppress success output; only print failures",
    )
    ns = parser.parse_args(argv)

    root = ns.repo_root if ns.repo_root else find_repo_root()
    targets = _resolve_targets(root, ns.paths)

    failures: list[str] = []
    citations_checked = 0
    for source in targets:
        if not source.is_file():
            continue
        try:
            with source.open(encoding="utf-8") as f:
                for line_idx, line in enumerate(f):
                    for m in CITATION_RE.finditer(line):
                        # Probe § G.3 filter: skip IP:port false positives that
                        # slip through the path-side pattern (rare with the
                        # known-extensions whitelist; belt-and-suspenders).
                        candidate = f"{m.group('path')}:{m.group('start')}"
                        if IP_PORT_RE.match(candidate):
                            continue
                        citations_checked += 1
                        end = int(m.group("end")) if m.group("end") else None
                        err = _check_citation(
                            root, source, line_idx,
                            m.group("path"),
                            int(m.group("start")),
                            end,
                        )
                        if err is not None:
                            failures.append(err)
        except (OSError, UnicodeDecodeError) as e:
            failures.append(f"{source}: read failure: {e}")

    if not ns.quiet:
        print(f"audit-prose-freshness: checked {citations_checked} citations across {len(targets)} files")
    for f in failures:
        print(f, file=sys.stderr)
    if failures:
        print(f"audit-prose-freshness: {len(failures)} citation(s) failed to resolve", file=sys.stderr)
        return 1
    if not ns.quiet:
        print("audit-prose-freshness: all citations resolve")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
```

Make the file executable: `chmod +x tools/integrity/scripts/audit_prose_freshness.py`.

#### 4.C.2 `tools/integrity/tests/test_audit_prose_freshness.py` (new)

Four tests:

```python
"""Tests for tools/integrity/scripts/audit_prose_freshness.py."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pytest


SCRIPT = Path("tools/integrity/scripts/audit_prose_freshness.py")


def _run_script(repo_root: Path, *args: str) -> tuple[int, str, str]:
    result = subprocess.run(
        [sys.executable, str(repo_root / SCRIPT), *args, "--repo-root", str(repo_root)],
        capture_output=True, text=True, cwd=str(repo_root),
    )
    return result.returncode, result.stdout, result.stderr


def test_resolves_valid_citation(tmp_path: Path) -> None:
    """A citation pointing at an existing file at an in-range line resolves."""
    target = tmp_path / "valid.md"
    target.write_text("line 1\nline 2\nline 3\n")
    source = tmp_path / "source.md"
    source.write_text("see `valid.md:2` for details\n")
    # Initialize a git repo so find_repo_root works
    subprocess.run(["git", "init", "-q"], cwd=tmp_path, check=True)
    rc, out, err = _run_script(tmp_path, str(source))
    assert rc == 0
    assert "all citations resolve" in out


def test_fails_on_missing_file(tmp_path: Path) -> None:
    """A citation pointing at a missing file fails."""
    source = tmp_path / "source.md"
    source.write_text("see `missing.md:1` for details\n")
    subprocess.run(["git", "init", "-q"], cwd=tmp_path, check=True)
    rc, _, err = _run_script(tmp_path, str(source))
    assert rc == 1
    assert "file not found" in err


def test_fails_on_out_of_range_line(tmp_path: Path) -> None:
    """A citation with a line number past EOF fails."""
    target = tmp_path / "short.md"
    target.write_text("only one line\n")
    source = tmp_path / "source.md"
    source.write_text("see `short.md:5` for details\n")
    subprocess.run(["git", "init", "-q"], cwd=tmp_path, check=True)
    rc, _, err = _run_script(tmp_path, str(source))
    assert rc == 1
    assert "out of range" in err


def test_range_citation_resolves(tmp_path: Path) -> None:
    """A range citation `file:start-end` resolves when in range."""
    target = tmp_path / "ranged.md"
    target.write_text("line 1\nline 2\nline 3\nline 4\nline 5\n")
    source = tmp_path / "source.md"
    source.write_text("see `ranged.md:2-4` for details\n")
    subprocess.run(["git", "init", "-q"], cwd=tmp_path, check=True)
    rc, _, _ = _run_script(tmp_path, str(source))
    assert rc == 0
```

#### 4.C.3 `tools/integrity/README.md` — pointer to the sibling tool

Add a short § documenting the audit-prose-freshness sibling tool. Anchor
sketch (verify the existing README structure at edit time):

```markdown
## Sibling tools

### audit-prose-freshness

A pre-commit utility that verifies backtick-fenced `path:line` citations
in spec, retro, and audit prose resolve against the actual repo. Not
part of the main gate; intended for spec authors to run before
committing.

```
python3 tools/integrity/scripts/audit_prose_freshness.py [PATHS...]
```

Default scans: `docs/integrity-toolkit-spec.md`, `docs/retro/*.md`,
`docs/diagnostics/_audits/*.md`, `project-state.md`. Exits non-zero if
any citation fails to resolve.
```

### 4.D Verification

```bash
cd tools/integrity
python3 -m pytest tests/test_audit_prose_freshness.py -v
# Expected: 4 new tests pass.

# Smoke against the real repo.
python3 tools/integrity/scripts/audit_prose_freshness.py --quiet
# Expected: prints failures (if any) to stderr; exit code 0 if all resolve.
# Some failures are likely on first run — these are real findings the
# tool was built to surface. NOT a regression. Capture the count in
# the audit report and bank for follow-up.
```

If audit-prose-freshness reports >50 failures, **pause and surface** —
the regex may be too permissive. <50 is expected (the existing audit
corpus has accumulated some drift).

### 4.E Commit message

```
feat(integrity): T2.2 audit-prose-freshness sibling tool (v1.3 closeout commit 3)

Adds `tools/integrity/scripts/audit_prose_freshness.py` — a standalone
pre-commit utility that verifies backtick-fenced `path:line` citations
in spec/retro/audit prose resolve against the actual repo.

Sibling tool, NOT integrated with the main gate (per Decision D3, spec
§ 0.3). cat1.intra-repo already covers the surface; this tool's value
is timing (drafter runs explicitly before committing) and scope (just
the citations the drafter is asserting, not the full repo).

Closes roadmap T2.2 / v1.1 retro § 6.3.

Tests: +4 in test_audit_prose_freshness.py.
README updated with sibling-tools section.
```

### 4.F Audit report

Path: `docs/diagnostics/_audits/integrity_v1_3_closeout_commit3_landing_2026-05-17.md`.
Standard structure. Capture the first-run failure count in § C as
baseline data for any future cleanup.

---

## 5. Commit 4 — T2.1 sweep enforcement (CI check, medium)

### 5.A Purpose & sources

Per roadmap T2.1 / v1.1 batch-1 retro § 6.2: the post-batch triage
pattern repeats — a commit edits a cat1-scannable live-source file
without running the grandfather sweep, the gate goes red on the
introducing commit, the user has to chase it down. Convention G
("sweep-side protection lands before check-side scope expansion") is
documented but author discipline alone hasn't held.

Per Decision D2 (Hard Rule 0.3): CI check, not pre-commit hook.

### 5.B Heuristic

A CI job step that, on PR or push:

1. Computes the diff between the PR's head and merge base (or, for
   push-to-main, the previous commit).
2. Filters changed files to those matching the cat1-scannable
   live-source set: any file under `common/`, `particle-fluids/`,
   `volumetric-grid/`, `continuous-ca/`, `hybrid-particle-grid/`,
   `agent-based/`, `closed-form/`, plus `docs/phase*.md` (the live
   phase spec set). Use the `is_live_source_path` helper (from
   `integrity.grandfather`) to compute this consistently with P1.8.
3. Computes whether any of the commits in the diff is a paired
   grandfather-sweep commit. Heuristic: commit message contains
   `grandfather-sweep`, `grandfather sweep`, or
   `sweep-companion`, OR the diff touches
   `tools/integrity/docs/grandfather-catalog.md` (the catalog
   refresh from the sweep is the load-bearing artifact).
4. If cat1-scannable live-source changed AND no paired sweep commit
   exists in the diff, fail the job with a clear message:
   "Cat1-scannable live-source files changed without a paired
   grandfather-sweep commit. Either run `python3
   tools/integrity/scripts/grandfather_sweep.py` and amend, or
   add `[skip-paired-sweep]` to the commit body if the changes
   intentionally won't add findings."

The `[skip-paired-sweep]` escape hatch is required because
documentation-only changes to live-source paths legitimately don't
require a sweep, and forcing one would create noise.

### 5.C File modifications

#### 5.C.1 `tools/integrity/scripts/check_paired_sweep.py` (new)

Anchor sketch (~80 LOC):

```python
#!/usr/bin/env python3
"""Paired-sweep enforcement (T2.1 CI check).

Verifies that any PR / push that changes cat1-scannable live-source
files includes a paired grandfather-sweep commit (or an explicit
[skip-paired-sweep] tag).

Used by .github/workflows/integrity.yml as a separate job step from the
main gate. Independent of the integrity check registry.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from integrity.common.repo import find_repo_root
from integrity.grandfather import is_live_source_path


SWEEP_MARKERS = ("grandfather-sweep", "grandfather sweep", "sweep-companion")
SKIP_TAG = "[skip-paired-sweep]"
CATALOG_PATH = "tools/integrity/docs/grandfather-catalog.md"


def _git_diff_files(base_ref: str, head_ref: str, repo_root: Path) -> list[str]:
    result = subprocess.run(
        ["git", "diff", "--name-only", f"{base_ref}...{head_ref}"],
        cwd=repo_root, capture_output=True, text=True, check=False,
    )
    if result.returncode != 0:
        # Fall back to last commit only
        result = subprocess.run(
            ["git", "diff", "--name-only", "HEAD~1...HEAD"],
            cwd=repo_root, capture_output=True, text=True, check=True,
        )
    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def _commits_in_range(base_ref: str, head_ref: str, repo_root: Path) -> list[tuple[str, str]]:
    """Returns list of (sha, message_body) for commits in the range."""
    result = subprocess.run(
        ["git", "log", "--format=%H%n%B%n---END---", f"{base_ref}...{head_ref}"],
        cwd=repo_root, capture_output=True, text=True, check=False,
    )
    if result.returncode != 0:
        result = subprocess.run(
            ["git", "log", "--format=%H%n%B%n---END---", "-1"],
            cwd=repo_root, capture_output=True, text=True, check=True,
        )
    commits: list[tuple[str, str]] = []
    chunks = result.stdout.split("---END---")
    for chunk in chunks:
        chunk = chunk.strip()
        if not chunk:
            continue
        lines = chunk.split("\n", 1)
        sha = lines[0]
        body = lines[1] if len(lines) > 1 else ""
        commits.append((sha, body))
    return commits


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Check for paired grandfather-sweep")
    parser.add_argument("--base-ref", default="HEAD~1")
    parser.add_argument("--head-ref", default="HEAD")
    parser.add_argument("--repo-root", type=Path, default=None)
    ns = parser.parse_args(argv)

    root = ns.repo_root if ns.repo_root else find_repo_root()
    changed_files = _git_diff_files(ns.base_ref, ns.head_ref, root)
    live_source_changed = [
        f for f in changed_files if is_live_source_path(f)
    ]

    if not live_source_changed:
        print("check-paired-sweep: no cat1-scannable live-source files changed; OK")
        return 0

    commits = _commits_in_range(ns.base_ref, ns.head_ref, root)
    for sha, body in commits:
        body_lower = body.lower()
        if SKIP_TAG.lower() in body_lower:
            print(f"check-paired-sweep: {sha[:8]} carries {SKIP_TAG}; OK")
            return 0
        if any(marker in body_lower for marker in SWEEP_MARKERS):
            print(f"check-paired-sweep: {sha[:8]} is a paired sweep commit; OK")
            return 0
    if CATALOG_PATH in changed_files:
        print(f"check-paired-sweep: {CATALOG_PATH} touched; treating as paired-sweep; OK")
        return 0

    print(
        f"check-paired-sweep: FAIL — {len(live_source_changed)} cat1-scannable "
        f"live-source files changed without a paired grandfather-sweep commit.",
        file=sys.stderr,
    )
    print(
        "Either run `python3 tools/integrity/scripts/grandfather_sweep.py` and amend,",
        file=sys.stderr,
    )
    print(
        f"or add {SKIP_TAG} to the commit body if the changes won't add findings.",
        file=sys.stderr,
    )
    for f in live_source_changed[:10]:
        print(f"  changed: {f}", file=sys.stderr)
    if len(live_source_changed) > 10:
        print(f"  ... and {len(live_source_changed) - 10} more", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
```

#### 5.C.2 `.github/workflows/integrity.yml` — add paired-sweep step

Insert a new step AFTER the "Install integrity toolkit" step (to ensure
`integrity.grandfather` is importable) and BEFORE the "Run integrity
toolkit against repo" step.

Per probe § E.1 / § G.6: `actions/checkout@v4 fetch-depth: 1` fetches
only the head commit, so `git fetch origin <SHA>` does not work
reliably against arbitrary historical SHAs (GitHub's
`uploadpack.allowReachableSHA1InWant` requires the SHA to be
reachable from a ref). The probe enumerated three workable patterns;
this spec adopts **option 2: fetch the base branch ref at runtime,
keep `fetch-depth: 1` at the workflow's checkout step**, so non-diff
steps stay fast.

```yaml
      - name: Check paired grandfather-sweep
        if: github.event_name == 'pull_request'
        env:
          BASE_REF: ${{ github.event.pull_request.base.ref }}
          BASE_SHA: ${{ github.event.pull_request.base.sha }}
          HEAD_SHA: ${{ github.event.pull_request.head.sha }}
        run: |
          # Fetch the base branch ref so the diff range resolves.
          # fetch-depth: 1 on checkout means we only have the head commit;
          # fetching the base ref (a real ref name, not a bare SHA) is the
          # reliable path per probe § E.1.
          git fetch --no-tags --depth=1 origin "$BASE_REF"
          # Now $BASE_SHA is reachable for the diff.
          python3 tools/integrity/scripts/check_paired_sweep.py \
            --base-ref "$BASE_SHA" --head-ref "$HEAD_SHA"
```

The `if: github.event_name == 'pull_request'` guard means push-to-main
doesn't trigger the check. Rationale: push-to-main is typically the
merge of a PR that already passed; running the check there would
double-fire. PR-only is the right semantic level.

**No change to `fetch-depth` at the checkout step.** The other workflow
steps continue to receive a depth-1 checkout for fast clones.

If the fetch fails (e.g., the base branch was force-deleted between PR
open and CI run — rare), the `check_paired_sweep.py` script should
detect the failure and pause-and-surface rather than passing silently.
The script already falls back to `HEAD~1...HEAD` per § 5.C.1 if
`git diff` returns non-zero; that fallback is the safety net here.

#### 5.C.3 `tools/integrity/tests/test_check_paired_sweep.py` (new)

Three tests. Anchor sketch (use a tmp_path-based fixture git repo,
similar to test_audit_prose_freshness.py):

```python
def test_no_live_source_changes_passes(tmp_path: Path) -> None:
    """When only docs/ or audit-doc files change, the check passes."""
    ...


def test_live_source_change_without_sweep_fails(tmp_path: Path) -> None:
    """When a cat1-scannable live-source file changes and no sweep commit
    appears in the range, exit code 1."""
    ...


def test_skip_tag_overrides_check(tmp_path: Path) -> None:
    """[skip-paired-sweep] in any commit body short-circuits to pass."""
    ...
```

### 5.D Verification

```bash
cd tools/integrity
python3 -m pytest tests/test_check_paired_sweep.py -v
# Expected: 3 new tests pass.

# Local smoke against current branch.
python3 tools/integrity/scripts/check_paired_sweep.py --base-ref HEAD~5 --head-ref HEAD
# Output depends on local history; verify the script runs cleanly.

# Lint the workflow yaml.
yamllint .github/workflows/integrity.yml || true
# (yamllint not required to pass; this is just a sanity check.)
```

The CI check itself will exercise on the next PR after this batch lands.
Commit 8's audit report should capture the first-run result.

### 5.E Commit message

```
feat(integrity): T2.1 paired-sweep CI enforcement (v1.3 closeout commit 4)

Adds tools/integrity/scripts/check_paired_sweep.py and wires it into
.github/workflows/integrity.yml as a PR-only job step. Fails when
cat1-scannable live-source files changed in the PR diff without a
paired grandfather-sweep commit.

Escape hatch: [skip-paired-sweep] in any commit body in the PR range
short-circuits the check. Use sparingly (docs-only live-source touches
that legitimately don't add findings).

Per Decision D2 (medium enforcement level, spec § 0.3): CI check, not
pre-commit hook. Pre-commit slows every edit; soft hasn't held across
three retros.

Closes roadmap T2.1 / v1.1 retro § 6.2.

Tests: +3 in test_check_paired_sweep.py.
```

### 5.F Audit report

Path: `docs/diagnostics/_audits/integrity_v1_3_closeout_commit4_landing_2026-05-17.md`.
Standard structure.

---

## 6. Commit 5 — Conventions doc + T3 decisions

### 6.A Purpose & sources

Closes T3.1 (Decision D4), T3.2 (D5), T3.3 (D6), T3.4 (D7). The
conventions previously banked across retros (A through K) get a real
home; the long-stalled architect-2 review items get explicit decisions
with disclosure.

### 6.B Pre-edit verification

```bash
ls tools/integrity/docs/
view tools/integrity/docs/grandfather-catalog.md  # for stylistic anchor
grep -l "^> \*\*[A-K]\.\*\*" docs/retro/  # find where conventions are stored
grep -nE "^### [0-9]\.[0-9] Convention" docs/retro/integrity-toolkit-*.md
```

Confirm:
- Conventions A–E are banked in `docs/retro/integrity-toolkit-v1.1-batch1.md` § 7.2.
- Convention F is banked in `docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md` § D.2.1.
- Conventions G, H, I are banked in `docs/retro/integrity-toolkit-v1.2-bolt-ons.md` § 4.
- Conventions J, K are banked in `docs/retro/integrity-toolkit-v1.3-batch1-part-a.md` § 4.
- The user-facing Convention numbering #8 (from `project-state.md` §
  7) is **separate** from the lettered chain; it's the original
  "architect-1 fabrication" convention banked long before the integrity
  toolkit existed and lives in the project-wide convention list.

### 6.C File modifications

#### 6.C.1 `tools/integrity/docs/conventions.md` (new)

The doc body. Per probe § C.1-C.4 the verbatim text of all 11
conventions is available in the cited retro sections. Claude Code
should copy each blockquote text exactly from the source retro and
paste it into the appropriate taxonomy slot.

**Per probe § G.7:** Convention F currently lives as indented prose in
`docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md`
§ D.2.1 rather than as a `> **F.** ...` blockquote. When migrating to
conventions.md, **normalize Convention F to the same blockquote shape
as A-E, G-K** so the doc renders consistently. The text is unchanged;
only the surrounding formatting normalizes.

**Per probe § G.4 (bonus):** `_KNOWN_CATEGORIES` in `snapshot.py` has
no pinning test, unlike `FALLTHROUGH_CATEGORIES`. This is a minor
hygiene gap. The closeout doesn't add the test — the gap is banked
in commit 7's v1-closed marker as a "available if forcing function
appears" hygiene item, not a load-bearing v1 finishing touch.

```markdown
# Integrity Toolkit Conventions

Conventions banked across the v1.x cycle for the integrity toolkit
chain. Each convention is a short rule that emerged from a specific
failure or design observation; the `Source` line points to the retro
where it was originated.

This doc is the **canonical home** for these conventions (resolves
T3.2 per Decision D5 in
`docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md`
§ 0.3). Retros may add new conventions; whenever a convention is added
in a retro, mirror it here on the next batch.

The taxonomy groups by intent-of-use, not by origin date. The letters
(A–K and beyond) are the historical anchors; the taxonomy section
headings are how a reader looking for "which convention applies to my
current task?" should find the right one.

> **Disclosure:** This doc consolidates conventions banked by
> architect-1 (chat agents) and signed off via the same source's
> retros. No architect-2 review pass has been obtained on the
> convention list (the user formally opted out of architect-2 at
> the start of v1.2). The conventions are load-bearing in practice
> and have been validated by repeated firings, but they have not been
> independently reviewed.

## Spec-time discipline

### Convention C — Probe API surfaces before drafting a spec

> ... [verbatim from v1.1 batch-1 retro § 7.2 C] ...

**Source:** v1.1 batch-1 retro § 7.2 C.

### Convention D — ... [verbatim] ...

**Source:** v1.1 batch-1 retro § 7.2 D.

### Convention K — Anchor-sketch labeling for spec content from inference

> **K.** When a spec section constructs content from probe data plus
> architect-1 inference (rather than from verbatim verified content
> on disk), label the section explicitly as an "anchor sketch — verify
> at execution time" rather than presenting it as canonical. ...
> [verbatim from v1.3 part-A retro § 4.2]

**Source:** v1.3 batch-1 part-A retro § 4.2.

## Execution-time discipline

### Convention A — ... [verbatim] ...

**Source:** v1.1 batch-1 retro § 7.2 A.

### Convention F — Audit-prose freshness

> ... [verbatim from post-retro landing audit § D.2.1] ...

**Source:** v1.1 post-retro landing audit § D.2.1.

## Batch coordination

### Convention G — Sweep-side protection lands before check-side scope expansion

> **G.** Sweep-side protection lands before check-side scope
> expansion. When a v1.x batch adds a check that will classify into
> broad buckets (live-source / audit-doc / retro-doc / etc.), the
> corresponding sweep CLI protection rule must land before or
> alongside the check registration. ...
> [verbatim from v1.2 bolt-ons retro § 4.1]

**Source:** v1.2 bolt-ons retro § 4.1.

### Convention I — Cross-batch scope discipline

> **I.** When a sweep CLI run during a small-scope batch's verification
> picks up findings outside the batch's scope, do not opportunistically
> sweep them. Defer to the responsible batch's own sweep companion.

**Source:** v1.2 bolt-ons retro § 4.3.

**Note on convention numbering:** Convention I is the *batch-coordination*
convention. The Part-B retro § 4.1 also references "Convention I" as
the *rewrite-stale-reasons sweep mode* T2 candidate. The Part-B reference
was a numbering collision; the rewrite-stale-reasons feature does not
have a banked convention letter and is tracked as a feature implementation
in the v1.3 closeout spec § 2, not in this doc.

### Convention J — Sweep companions operate across commit boundaries when multi-file commits land

> **J.** A grandfather-sweep companion within a single commit
> operates on the cat1-scannable surface as it exists at sweep-run
> time. ...
> [verbatim from v1.3 part-A retro § 4.1]

**Source:** v1.3 batch-1 part-A retro § 4.1.

## Design taste

### Convention B — ... [verbatim] ...

**Source:** v1.1 batch-1 retro § 7.2 B.

### Convention E — ... [verbatim] ...

**Source:** v1.1 batch-1 retro § 7.2 E.

### Convention H — Filter rules query properties, not literals

> **H.** When implementing sweep filters or similar policy rules over
> a typed surface (`Classification`, `FailureMode`, etc.), prefer
> queries against properties of the type ("is this fallthrough?",
> "is this hard-fail?", "is this live-source?") over literal string
> matches against specific values. ...
> [verbatim from v1.2 bolt-ons retro § 4.2]

**Source:** v1.2 bolt-ons retro § 4.2.

## Decisions resolved without architect-2 review

This section banks the T3 items that have been "needs architect-2 review"
across multiple retros. The user formally opted out of architect-2 at
the start of v1.2; per Part-B retro § 5.4 these items needed either a
decision or formal banking. Each gets an architect-1 decision with
explicit disclosure.

### D4 — T3.1 (A.9 audit-citation exclusion): rejected

**The proposal:** Exclude audit-doc paths (`docs/diagnostics/_audits/`)
from `cat1.intra-repo` scans entirely, rather than scanning and
bucketing findings under the `audit-citation` grandfather category.

**Quantified leverage (per v1.1 self-review probe):** ~67% pool collapse
if adopted.

**Decision: rejected.** Keep-and-bucket preserves the audit trail of
which audit doc had which finding. The grandfather catalog already
segments these into named categories (`audit-citation`,
`audit-bare-path`). Pool size is not the load-bearing metric; per-finding
attribution is.

**Status:** architect-1-decided, no architect-2 review obtained.
Reconsideration is v2 scope.

### D5 — T3.2 (conventions doc home): `tools/integrity/docs/conventions.md`

**The proposal:** Three locations were on the table:
- `docs/CONVENTIONS.md` (repo root)
- `tools/integrity/docs/conventions.md` (toolkit-scoped)
- Per-stack (e.g., `common/common-py/docs/conventions.md`)

**Decision: `tools/integrity/docs/conventions.md`.** This is the file
you are reading. Toolkit-scoped conventions live with the toolkit.
The repo-root location was rejected because conventions outside the
integrity chain (e.g., from `docs/retro/phase11.md`) are not the same
lineage and would be confused with these.

**Status:** architect-1-decided, no architect-2 review obtained.

### D6 — T3.3 (numbering taxonomy): four-bucket

**The proposal:** Re-group A–K into a taxonomy or keep alphabetic.

**Decision: four-bucket taxonomy** per v1.3 part-A retro § 5.2:
- Spec-time discipline
- Execution-time discipline
- Batch coordination
- Design taste

Letters preserved as historical anchors. Section headings are how
readers navigate.

**Status:** architect-1-decided, no architect-2 review obtained.

### D7 — T3.4 (architect-2 backlog from v1.1 batch-1 retro § 6.4): formally banked unresolved

**The proposal:** Four items have been "needs architect-2 review"
across multiple retros:
1. v1.1 retro § 3.1 / § 3.2 root-cause framing of architect-1
   fabrications
2. v1.1 retro § 5.3 recursive-blind-spot procedural-vs-structural
   framing
3. v1.1 retro § 6.1 priority ordering
4. v1.1 retro § 6.2 enforcement-level pick (overlaps with T2.1, now
   resolved by D2)

**Decision: formally bank as unresolved.** Items 1–3 remain
architect-1-perspective only. The framings the retros offer are the
banked default; if architect-2 ever weighs in (v2 or beyond), the
banked framings should be revisited.

Item 4 is resolved by Decision D2 (medium / CI check) in the v1.3
closeout spec.

**Status:** architect-1-decided to defer items 1–3 indefinitely; item 4
resolved as part of v1.3 closeout.

## Living document

When new conventions are banked in future retros, mirror them here in
the appropriate taxonomy section. The retro remains the source-of-truth
for origination context; this doc is the navigation surface.

This doc is the canonical home; references to "the conventions" in
spec drafts, retros, and audit reports should point here from now on.
```

The verbatim convention texts (denoted `... [verbatim] ...` above)
should be copied from the source retros at edit time. Claude Code:
read each source retro, copy the convention's blockquote text exactly,
and paste it into the appropriate slot in this doc.

#### 6.C.2 `tools/integrity/README.md` — pointer

Add a short § referencing the new conventions doc (verify the README's
existing structure at edit time):

```markdown
## Conventions

Toolkit conventions are documented at
[`tools/integrity/docs/conventions.md`](docs/conventions.md). Banked
across the v1.x cycle; references in spec/retro/audit prose should
point there.
```

### 6.D Verification

```bash
# Gate state unchanged.
python3 -m integrity --mode strict --no-audit-log

# audit-prose-freshness on the new doc.
python3 tools/integrity/scripts/audit_prose_freshness.py \
  tools/integrity/docs/conventions.md
# Expected: zero failures (the new doc cites only retro paths, all of
# which exist).
```

### 6.E Commit message

```
docs(integrity): conventions doc + T3 decisions (v1.3 closeout commit 5)

Adds tools/integrity/docs/conventions.md as the canonical home for the
toolkit's banked conventions (resolves T3.2 / Decision D5). Conventions
A through K migrated from retros, regrouped into four buckets per
Decision D6 taxonomy (spec-time / execution-time / batch-coordination /
design-taste).

Decisions D4 (T3.1 rejected), D5 (this doc's location), D6 (taxonomy),
D7 (T3.4 banked unresolved) all documented in the doc's "Decisions
resolved without architect-2 review" section with explicit disclosure
per Part-B retro § 5.4.

README updated with conventions pointer.

Closes roadmap T3.1 through T3.4.
```

### 6.F Audit report

Path: `docs/diagnostics/_audits/integrity_v1_3_closeout_commit5_landing_2026-05-17.md`.
Standard structure. In § D (behavioral notes), capture which
convention texts were sourced from which retros — this is the
load-bearing reference for any future reader who wants to verify the
migration.

---

## 7. Commit 6 — project-state.md fossil cleanup (Part-C)

### 7.A Purpose & sources

Per Part-B retro Decision 6 / probe § E.3 / probe § K.6: three
`integrity-allow:` annotations on `project-state.md` at lines 559, 593,
666 carry the `other-cat1` reason string but have no backing findings.
They are fossils — the underlying findings either resolved (the cited
paths were fixed) or migrated to `cat1.bare-path` (now suppressed by
separate annotations).

Part-B intentionally banked the cleanup. This commit lands it.

### 7.B Pre-edit verification

Probe § B.8 confirmed:
- Three `cat1.intra-repo` annotations at `project-state.md` lines
  559, 593, 666 (exact match, no drift from spec assertions).
- Three `cat1.bare-path` annotations at the immediately-following
  lines 560, 594, 667 (these are NOT fossils in intent, but probe
  § G.2 found they are not actually suppressing the cat1.bare-path
  findings at 561/595/668 — those 5 findings are firing as HARD_FAIL
  in the baseline).

This commit deletes ONLY the three `cat1.intra-repo` fossils. The
three `cat1.bare-path` annotations stay in place even though they're
currently not load-bearing in effect; their removal would either
(a) do nothing if they're already inert, or (b) silently make the
gate state worse if a future suppression fix activates them. Banking
G.2 as a known issue (commit 7) is the closeout-scope answer.

```bash
view project-state.md
grep -n "integrity-allow.*other-cat1\b" project-state.md
# Expected (per probe § B.8): three lines, 559, 593, 666 (cat1.intra-repo).
# If line numbers have drifted from probe time, re-anchor.

grep -n "integrity-allow.*other-cat1-bare-path" project-state.md
# Expected: three lines, 560, 594, 667 (cat1.bare-path). These are
# NOT to be deleted in this commit.

# Confirm no cat1.intra-repo finding fires anywhere on project-state.md
# (per probe § B.8, the fossil hypothesis is confirmed for these):
python3 -m integrity --mode strict --no-audit-log 2>&1 | \
  grep "cat1.intra-repo.*project-state\.md"
# Expected: empty output. If any cat1.intra-repo finding fires on
# project-state.md, those annotations are NOT fossils — pause-and-surface.
```

If a `cat1.intra-repo` finding exists at any of the three lines + 1
(560, 594, 667 — note these are the bare-path annotation lines, the
finding line would actually be 561, 595, 668 in 1-indexed), the
annotation is NOT a fossil — it's load-bearing. **Pause and surface.**

### 7.C File modifications

Delete the three `cat1.intra-repo` fossil annotation lines from
`project-state.md`. Verbatim from probe § B.8:

- Line 559: `<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->`
- Line 593: `<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->`
- Line 666: `<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->`

**Do NOT delete** lines 560, 594, 667 (the `cat1.bare-path`
annotations). These are addressed by the G.2 banking in commit 7,
not by deletion here.

After the three deletions, the document re-flows; the cumulative
effect on subsequent line numbers is -3 (one shift per deletion).

If any of the three target lines does NOT exactly match the verbatim
text above (e.g., the comment wrapping differs, or the text wraps
across two lines), **pause and surface** — the annotation isn't a
straightforward HTML comment and may be content rather than a
suppression.

### 7.D Verification

```bash
# Gate state unchanged or slightly different.
python3 -m integrity --mode strict --no-audit-log
# Expected: hard-fail count unchanged or +0/-0 (deleting an unused
# suppression doesn't unsuppress any finding because there was nothing
# to suppress). If hard-fail count grows by 3, the annotations were NOT
# fossils — revert and pause-and-surface.

grep -n "integrity-allow.*other-cat1" project-state.md
# Expected: empty output (the three were the only such annotations on
# project-state.md per probe § E.3).
```

### 7.E Commit message

```
chore(integrity): remove project-state.md fossil annotations (v1.3 closeout commit 6)

Removes three `integrity-allow: cat1.intra-repo; ... other-cat1; n/a`
annotations from project-state.md (probe-time lines 559, 593, 666 per
Part-B probe § E.3). These annotations have no backing findings; the
underlying cat1.intra-repo findings either resolved via concurrent
commits or migrated to cat1.bare-path after A.3 introduced that check.

Closes Part-B retro Decision 6 / probe § K.6 banked cleanup.

Verified: gate state hard-fail count unchanged after deletion (no
findings unsuppressed because the annotations were inert).
```

### 7.F Audit report

Path: `docs/diagnostics/_audits/integrity_v1_3_closeout_commit6_landing_2026-05-17.md`.
Standard structure. Capture the pre-deletion and post-deletion strict
mode summary lines side-by-side in § C.

---

## 8. Commit 7 — project-state.md v1-closed marker + bonus cleanups

### 8.A Purpose & sources

Three loosely-coupled items in one commit, all about marking the v1
milestone closed:

1. **v1-closed marker in project-state.md** — the canonical statement
   that v1 is shipped. Future readers see the steady-state status at a
   glance.
2. **README implementation-status cleanup** (probe § G.5) — the
   `tools/integrity/README.md` "Implementation status" section shows
   commits 2-8 as unchecked despite all having shipped through v1.0 -
   v1.2. Stale by ~9 months.
3. **Known-issue banking** for the two probe flags not absorbed
   inline: G.2 (project-state.md cat1.bare-path suppression not firing
   on lines 561/595/668) and G.4 (`_KNOWN_CATEGORIES` lacks a pinning
   test like FALLTHROUGH_CATEGORIES has).

Both known issues are real but neither is closeout-scope:

- G.2 requires investigating the runner-side suppression pipeline,
  which is a non-trivial debugging task. The findings have been
  hard-failing through Part-B's 60-baseline, so it's a long-standing
  issue, not a new regression.
- G.4 is a tiny micro-test (one assertion: `len(_KNOWN_CATEGORIES) ==
  N` for some pinned N) but adding it inside this closeout would
  expand scope.

Both get explicit "available if forcing function appears" banks.

### 8.B Pre-edit verification

Probe § B.8 confirmed `project-state.md` structure:

- Section ledger lives at `## 3. Phase ledger` (line 58).
- Phase ledger table columns: `#`, `Phase`, `Scope`, `Status`, `Shipped at`.
- Status glyphs: `✅` (done) / `⬜` (not started).
- "Shipped at" SHAs are wrapped in backticks.
- Last row is "10+ Remaining sims" with `⬜ Not started`.
- Section 8 is "Things explicitly deferred" — natural home for the
  banked-items list.
- Section 9 is "Known issues" — natural home for the G.2 / G.4
  banked-known-issue entries.

Probe § B.9 confirmed `tools/integrity/README.md` shape:

- 64 LOC; no Tools/Scripts section; no TOC.
- "Implementation status" checklist shows commits 2-8 unchecked despite
  shipping in v1.0-v1.2 (probe § G.5).

```bash
view project-state.md
grep -n "^# \|^## " project-state.md | head -20
# Should match probe § B.8 output. If different, re-anchor.

view tools/integrity/README.md
# Confirm the stale "Implementation status" section is still present.
```

### 8.C File modifications

#### 8.C.1 `project-state.md` phase ledger row

Append a row to the phase ledger table (after the "10+ Remaining sims"
row at the bottom). Per probe § B.8 the columns are `#`, `Phase`,
`Scope`, `Status`, `Shipped at`:

```
| 12.5 | Integrity toolkit v1 closed | The toolkit's gate covers Cat 1 citations, Cat 2 contracts, Cat 3 numerical correctness across three stacks. v1.3 closeout batch shipped Convention I (rewrite-stale-reasons feature per Part-B § 4.1), T2.1/2.2/2.3, T3.1-3.4 decisions with disclosure, fossil cleanup, and this marker. Conventions live at `tools/integrity/docs/conventions.md`. v2 horizon items remain available if a forcing function appears. No further v1.x work is planned. | ✅ Done | `<COMMIT_8_SHA>` |
```

The `<COMMIT_8_SHA>` placeholder is back-filled by commit 8 per
Convention #12. Verify the table's last row uses the same SHA-in-
backticks convention before placing this row.

#### 8.C.2 `project-state.md` § 8 (Things explicitly deferred) — banked items

Append to `## 8. Things explicitly deferred`:

```markdown
### Integrity toolkit v1.x banked-but-not-planned items (post-closeout)

The v1.3 closeout batch shipped 2026-05-17. Items below are available
if a forcing function appears but are not on a planned schedule:

- **T2 items:** none (all landed in v1.3 closeout).
- **T3 items:** D4–D7 resolved in `tools/integrity/docs/conventions.md`;
  T3.4 items 1–3 formally banked unresolved per conventions.md
  disclosure.
- **T4 horizon (v2 candidates):** Category 4 runtime integration tests;
  type-aware Cat 2 matching (currently token-based for Stack C);
  GPU shader coverage via headless (dawn / SwiftShader);
  per-sim cat3 expansion (quantum sim candidates already drafted
  per `docs/category-contexts/quantum.md` § 6.1);
  multi-line citation grammar;
  spec-vs-implementation reconciliation (architect-2 review work);
  `_emit_human_summary` ordering polish.
- **Part-D banked:** none. Part-C (project-state.md fossil cleanup)
  closed by commit 6 of the closeout batch.
```

#### 8.C.3 `project-state.md` § 9 (Known issues) — G.2 and G.4 banks

Append to `## 9. Known issues`:

```markdown
### Integrity toolkit known issues banked from v1.3 closeout probe

These were surfaced by the v1.3 closeout pre-spec probe
(`docs/diagnostics/_audits/integrity_v1_3_closeout_probe_2026-05-17_architect1-via-claude-code.md`
§ G.2 / § G.4) and intentionally NOT addressed in the closeout batch
to keep scope bounded.

- **G.2 (suppression non-firing on project-state.md cat1.bare-path).**
  Three `cat1.bare-path` annotations on `project-state.md:560/594/667`
  do not suppress the cat1.bare-path findings at 561/595/668 — those
  5 findings are firing as HARD_FAIL despite the annotations being
  in the immediately-preceding-line position the suppression grammar
  specifies. Either there's a real bug in the runner-side suppression
  pipeline, or markdown-context multi-annotation stacks interact
  differently than expected. Long-standing (predates v1.3 part-B's
  60-baseline). Available for v2 investigation.

- **G.4 (`_KNOWN_CATEGORIES` no pinning test).** Unlike
  `FALLTHROUGH_CATEGORIES`, the `_KNOWN_CATEGORIES` tuple in
  `tools/integrity/integrity/snapshot.py` has no unit test pinning its
  contents. A one-line `assert len(_KNOWN_CATEGORIES) == N` style test
  would catch silent edits. Tiny scope; available whenever the next
  toolkit micro-batch lands.
```

#### 8.C.4 `tools/integrity/README.md` — stale implementation-status cleanup

Per probe § G.5: the README's "Implementation status" section is
~9 months stale, showing commits 2-8 as unchecked despite all having
shipped through v1.0-v1.2.

Replace the existing "Implementation status" section with a current
status block:

```markdown
## Status

Integrity toolkit v1 is shipped and closed as of 2026-05-17 per the
v1.3 closeout batch. The gate runs on every push and PR via
`.github/workflows/integrity.yml`. Conventions are documented at
[`docs/conventions.md`](docs/conventions.md). v2 horizon items are
banked in `project-state.md` § 8; no v1.x work is planned beyond
maintenance.
```

The section heading is changed from `## Implementation status` to
`## Status` since the prior "implementation in progress" framing no
longer applies.

#### 8.C.5 `project-state.md` "Last updated" prose paragraph (optional)

If `project-state.md` has a "Last updated" header block near the top
(probe didn't verify; check via `view`), update it to reflect the
v1.3 closeout. If no such block exists, skip this sub-step.

### 8.D Verification

```bash
python3 -m integrity --mode strict --no-audit-log
# Expected: hard-fail count may grow by a small number (this commit
# adds prose with citations that audit-prose-freshness should validate;
# but cat1.intra-repo may also fire on the new citations until the
# sweep companion absorbs them).

python3 tools/integrity/scripts/audit_prose_freshness.py project-state.md
# Expected: zero failures.
```

### 8.E Commit message

```
docs: mark integrity toolkit v1 closed + bonus cleanups (v1.3 closeout commit 7)

Three loosely-coupled doc updates marking the v1 milestone closed:

1. project-state.md phase ledger row + § 8 banked-items list + § 9
   known-issues banks (G.2 suppression on project-state.md cat1.bare-path,
   G.4 _KNOWN_CATEGORIES pinning test). Both known issues are real but
   out-of-scope for closeout; banked as "available if forcing function
   appears."

2. tools/integrity/README.md "Implementation status" section replaced
   with current "Status" block. The prior section was ~9 months stale
   (claimed commits 2-8 unimplemented; all shipped v1.0-v1.2). Per
   probe § G.5.

3. The integrity toolkit v1 milestone marker. No further v1.x work is
   planned. v2 horizon items remain available; see project-state.md
   § 8 for the banked list.

Closes v1.3 closeout spec § 1.1.
```

### 8.F Audit report

Path: `docs/diagnostics/_audits/integrity_v1_3_closeout_commit7_landing_2026-05-17.md`.
Standard structure.

---

## 9. Commit 8 — SHA back-fill

### 9.A Purpose & sources

Per Convention #12 (retro § 7.2 from v1.2 bolt-ons): SHA back-fill is a
separate commit, never `--amend`. The seven preceding commits each
landed with their audit report citing `<COMMIT_N_SHA>` placeholders for
downstream sibling-doc references. This commit replaces every placeholder
with the resolved SHA.

### 9.B Pre-edit verification

```bash
# Enumerate placeholders across the seven audit reports.
grep -l "<COMMIT_[1-7]_SHA>" docs/diagnostics/_audits/integrity_v1_3_closeout_commit*_landing_2026-05-17.md

# Capture the seven SHAs.
for n in 1 2 3 4 5 6 7; do
  sha=$(git log --format=%H --grep="v1.3 closeout commit $n" -1)
  echo "commit $n: $sha"
done
# If the grep returns ambiguous results, fall back to manual identification
# via `git log --oneline -15` and pattern-match on the commit message lines.
```

### 9.C File modifications

Replace every `<COMMIT_N_SHA>` placeholder in commit-1 through commit-7
audit reports with the resolved short SHA (7 chars). Number of edits:
typically 1 to 3 per audit report (the "next commit" pointer + any
inline reference to a sibling commit).

If the grandfather catalog has drifted since the start of the batch,
also refresh it:

```bash
python3 tools/integrity/scripts/refresh_catalog_counts.py
# (assuming this script exists from v1.3 part-A T1.3; verify via view)
```

### 9.D Verification

```bash
# All SHAs resolve.
for sha in $(grep -hoE '\b[0-9a-f]{7,40}\b' docs/diagnostics/_audits/integrity_v1_3_closeout_commit*_landing_2026-05-17.md | sort -u); do
  git cat-file -e "$sha" 2>/dev/null && echo "OK $sha" || echo "MISSING $sha"
done | grep MISSING || echo "all SHAs resolve"
# Expected: "all SHAs resolve".

# No placeholders remain.
grep -l "<COMMIT_[1-7]_SHA>" docs/diagnostics/_audits/integrity_v1_3_closeout_commit*_landing_2026-05-17.md
# Expected: empty output.

# Gate state at batch close.
python3 -m integrity --mode strict --no-audit-log
```

### 9.E Commit message

```
docs(audits): SHA back-fill for v1.3 closeout commits 1-7 (v1.3 closeout commit 8)

Replaces <COMMIT_N_SHA> placeholders in the seven preceding commit-landing
audit reports with resolved SHAs. Per Convention #12: separate follow-up
commit, never --amend.

Catalog count refresh: zero drift (or report actual drift if any).
```

### 9.F Audit report

Path: `docs/diagnostics/_audits/integrity_v1_3_closeout_commit8_landing_2026-05-17.md`.
Brief — mirrors v1.3 part-A commit-4 audit (the back-fill audit). Per
Part-A retro § 6 banked observation, the audit report IS required even
for SHA back-fill commits.

---

## 10. References

- `docs/integrity-toolkit-spec.md` — v1 canonical spec.
- `docs/retro/integrity-toolkit-v1.md` — v1 retro (foundational conventions A–E).
- `docs/retro/integrity-toolkit-v1.1-batch1.md` — § 7.2 (conventions A–E), § 6.1 (T2.3 leverage), § 6.2 (T2.1 enforcement options), § 6.3 (T2.2 origination), § 6.4 (T3.4 architect-2 backlog).
- `docs/retro/integrity-toolkit-v1.1-batch1-addendum.md` — P-numbering, batch-2 priority.
- `docs/retro/integrity-toolkit-v1.2-bolt-ons.md` — § 4 (conventions G, H, I).
- `docs/retro/integrity-toolkit-v1.3-candidates.md` — § 4 (T1), § 5 (T2), § 6 (T3), § 7 (T4).
- `docs/retro/integrity-toolkit-v1.3-batch1-part-a.md` — § 4 (conventions J, K), § 5.2 (taxonomy proposal).
- `docs/retro/integrity-toolkit-v1.3-batch1-part-b.md` — § 4.1 (Convention I rewrite-stale-reasons proposal), § 5.1 (pause recommendation), § 5.4 (T3 backlog formal-bank recommendation).
- `docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md` — § D.2.1 (Convention F audit-prose freshness origination).
- `docs/diagnostics/_audits/integrity_v1_3_t1_1_2_probe_2026-05-16_architect1.md` — § E.3 (project-state.md fossil annotation evidence).

## End of execution spec
