---
title: "Integrity Toolkit v1.2 Bolt-Ons — Execution Spec"
date: 2026-05-15
author: architect1
status: draft
audience: Claude Code (executor)
sibling-docs:
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/diagnostics/_audits/integrity_v1_1_self_review_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_1_post_batch_triage_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md
---

# Integrity Toolkit v1.2 Bolt-Ons — Execution Spec

## 0. Execution preamble (read this first)

You are Claude Code executing the v1.2 small-scope bolt-on batch per the
addendum at `docs/retro/integrity-toolkit-v1.1-batch1-addendum.md` § 5.
Four scope items, landed across four functional commits plus a SHA
back-fill (5 commits total). Each commit independently verifiable.
Materialize every file creation and edit specified.

### 0.1 Hard rules

1. **Execute every file creation and modification specified. Do not skip any.**
2. **The synced repo state is authoritative over this spec.** Every
   verbatim claim about file contents in this spec was grep-verified
   against probe SHA `9add1494b237e33f3dda782c821b9d7f29446068`
   (= `9add149`) during drafting per
   `docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md`.
   If HEAD has moved when you execute, re-anchor before applying — if any
   verbatim claim disagrees with disk, **pause and surface the conflict;
   do not silently adapt.**
3. **Land commits in the order given.** Each commit's verification block
   must pass before starting the next. Do not interleave.
4. **One audit report per commit.** Front-matter and structure mirror
   the v1.1 commit-N landing reports. File location pattern:
   `docs/diagnostics/_audits/integrity_v1_2_commit<N>_landing_2026-05-15.md`.
5. **SHA back-fill is a separate follow-up commit, never `--amend`.**
   Per Convention #12 (retro § 7.2). Commit 5 in this spec.
6. **The toolkit must remain green-relative-to-baseline across every
   commit.** Baseline at probe time was 4 hard-fail / 1046 suppressed.
   Each commit's verification block specifies expected
   counts post-commit. Hard-fail count must not exceed 4 at any
   intermediate state without explicit acknowledgement in the audit
   report.
7. **`python3`, not `python`.** The host has no `python` shim.

### 0.2 File-path conventions

- New check modules: `tools/integrity/integrity/cat3_numerical/checks/<id>.py`
- New tests: `tools/integrity/tests/test_<area>.py`
- Audit reports: `docs/diagnostics/_audits/integrity_v1_2_commit<N>_landing_2026-05-15.md`
- Grandfather sweep regeneration: `python3 tools/integrity/scripts/grandfather_sweep.py`
  is the entry point; do not invoke `integrity.grandfather.apply_annotations()`
  directly outside tests.

### 0.3 Convention B reminder

Every commit that touches the cat1-scannable surface (specifically: every
commit in this batch, since each lands an audit report under
`docs/diagnostics/_audits/`) must either run the grandfather sweep as
part of the commit OR ship a companion grandfather-sweep commit in the
same push. Per retro § 6.2 / § 7.2 convention B. For this batch the
sweep runs inline within each commit's verification block; no separate
sweep companions are required.

### 0.4 Coordination with concurrent A.3 work

The coordinator chat is drafting the A.3 (bare-path-to-upstream-basename)
spec concurrently. A.3 touches `cat1_citations/grammar.py`, possibly
`cat1_citations/checks/upstream.py`, and the `classify()` function in
`grandfather.py`.

This batch's overlap is **only** with `grandfather.py`. Commit 1 adds new
top-level helpers (`SWEEPABLE_PATH_PREFIXES`, `SWEEPABLE_EXACT_PATHS`,
`is_live_source_path()`) and modifies `apply_annotations()` to honor a
new `sweep_live_source` parameter. It does **not** modify `classify()` or
the `Classification` dataclass. The two sets of edits compose cleanly.

**Pull-rebase before every commit.** If `grandfather.py` has moved
under you between rebase and commit, re-verify your edits still apply.
If the rebase introduces a structural change to `Classification`
(category/reason/issue_ref) or to `classify()`'s call-site signature,
**pause and surface** — coordinator-chat work has crossed our scope and
needs coordinated resolution.

## 1. Goals & load-bearing decisions

### 1.1 Goals

Four addendum-§5 items land as four commits:

1. **P1.8** — Grandfather-sweep live-source protection (commit 1).
   Default-skip live-source `other-cat1` findings; `--sweep-live-source`
   flag for explicit opt-in; per-bucket summary line.
2. **P1.5** — Register the three `cat3.d3q19-*` checks (commit 2).
   Three new check modules consuming a relocated and refactored
   `d3q19_verify.py` algorithmic harness.
3. **P1.6** — Strict-mode human-renderer suppressed-stanza filter
   (commit 3). One-line fix to `runner.py` `emit_output()` `else`
   branch; one new test pinning the behavior.
4. **P1.7** — `stub_label_stale.py` module-docstring drift fix
   (commit 4). Replace lines 15-18 of the module docstring with the
   namespace-strip convention matching the in-function docstring at
   lines 96-105.

Plus:

5. **Commit 5 — SHA back-fill.** Updates each of commits 1–4's audit
   reports with the SHAs of the others.

### 1.2 Load-bearing decisions made during drafting

#### Decision 1 — Commit ordering: P1.8 first

The natural impulse is "new-files-first" per retro § 7.2 A, which would
put P1.5 (3 new check modules) first. **Override:** P1.8 lands first.

Rationale: P1.8 introduces the live-source-safe sweep default. Every
subsequent commit's inline grandfather-sweep step (sweeping audit-doc
findings introduced by the commit's own audit report) runs under the
new default. Without P1.8 landing first, commits 2–4 would each face
the same over-sweep failure that triggered the 9add149 pause-and-surface
(probe § A.2 confirms 4 outstanding live-source hard-fails still classify
as `other-cat1`; current sweep CLI would propose annotations on them).
The over-sweep is the active failure mode; race-exposure (the
new-files-first concern) is a theoretical concurrency issue. The active
failure dominates.

#### Decision 2 — P1.5 design: three separate check modules

Per probe § C.7 and § B.1. The registration model in
`cat3_numerical/checks/__init__.py` is `(CHECK_ID, module)` one-to-one;
sharing a module across three CHECK_IDs would re-run the harness three
times. `d3q19_verify.py`'s internal structure already separates
velocity-set / weights / equilibrium into distinct blocks (probe
§ B.1 — `build_velocity_set()`, `weight_for()` + second-moment loop,
`feq()` + per-test-point loop). Three thin check modules sharing
refactored harness helpers is the lowest-mismatch design.

#### Decision 3 — `d3q19_verify.py` relocation

The current location `tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py`
puts the algorithmic harness in the `checks/` directory alongside check
modules. The parallel `cat3.cubic-kernel` pattern (probe § C.2 + § C.3)
puts the algorithmic harness one directory up at
`cat3_numerical/cubic_kernel.py`, with the check module at
`cat3_numerical/checks/cubic_kernel.py`. P1.5 mirrors that pattern:
move `d3q19_verify.py` up one directory, leaving `checks/` populated
only by check modules.

Alongside the move: refactor `d3q19_verify.py` to expose three new
top-level functions (`verify_velocity_set()`, `verify_weights()`,
`verify_equilibrium()`) that return `list[str]` of mismatch messages
(empty = pass). Keep the existing `main()` and `__main__` guard so the
harness still runs standalone. The check modules call these new
helpers.

#### Decision 4 — P1.8 helper location

`is_live_source_path()` lands as a new top-level function in
`grandfather.py`, sibling of `classify()`. **Not** in
`integrity.common.paths` (no such module currently) or a new file.
Rationale: live-source classification is conceptually paired with
grandfather classification — they're both bucket decisions on file
paths. Co-locating reduces the import surface. Append-only addition
to `grandfather.py` is compose-safe with concurrent A.3 work on
`classify()` (different function in the same file; no edit-conflict
unless one side renames or reshapes the module's public surface).

#### Decision 5 — P1.8 sweepable-path encoding

Two structures at module top of `grandfather.py`:

```python
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
```

`is_live_source_path(file)` returns `True` iff the path is neither
prefix-matched nor exact-matched. The `tuple[str, ...]` /
`frozenset[str]` typing makes the constants immutable; explicit types
allow grep-discovery for future migrations.

The path list is taken verbatim from Steven's P1.8 directive plus
probe § G.2 cross-reference: confirms `project-state.md` is the one
P1.8-only entry beyond triage § B, and aligns with the deferred
`project-state-snapshot` classifier extension (post-retro landing
§ D.3) — the path-list approach is the interim measure pending P4's
named classifier rule.

#### Decision 6 — P1.6 fix shape

Per probe § D.2 verbatim, the human-output `else` branch is at
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`runner.py:141-145`. The fix is to insert the same
`if f.suppressed: continue` guard the github branch uses at line 134.
Three lines added: a one-line comment explaining the asymmetry origin
(probe § D.5 identified the asymmetry as original to commit `fc20ef7`,
which targeted only the github branch), plus the `if`/`continue`
pair. New test in a new file `tools/integrity/tests/test_runner_human_output.py`
to avoid bloating `test_runner.py`.

#### Decision 7 — P1.7 fix shape

Per probe § E.3. Replace exactly 4 lines of the module docstring
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
(`stub_label_stale.py:15-18`) with text matching the in-function
docstring at lines 96-105. No code changes. No test changes (no
behavior change; the in-function docstring already correctly
describes the existing code).

#### Decision 8 — Test placement

P1.8 tests extend `tools/integrity/tests/test_grandfather_sweep.py`
(probe § F.4 confirmed it currently covers the classifier and
renderer in `integrity.grandfather`, so adding sweep-behavior tests
fits its scope). Five new test functions.

P1.5 tests land in a single new file
`tools/integrity/tests/test_cat3_d3q19.py` (mirrors
`test_cat3_cubic_kernel.py`'s one-test-file-per-check-area pattern,
probe § C.6). Test functions cover all three checks since the
harness is shared.

P1.6 tests land in a new file `tools/integrity/tests/test_runner_human_output.py`
(per Decision 6).

P1.7 has no test changes (per Decision 7).

## 2. Commit plan

| # | Item | Scope | New files | Modified files | Estimated diff |
|---|---|---|---|---|---|
| 1 | P1.8 | Live-source sweep protection | `tests/test_grandfather_sweep.py` (extended; see § 3.4) | `grandfather.py`, `grandfather_sweep.py` | ~90 LOC new, ~30 LOC modified |
| 2 | P1.5 | d3q19 check registration | 3 check modules + 1 test file + relocated harness | `cat3_numerical/checks/__init__.py` | ~200 LOC new, ~50 LOC modified |
| 3 | P1.6 | runner human-renderer fix | `tests/test_runner_human_output.py` | `runner.py` | ~50 LOC new, ~3 LOC modified |
| 4 | P1.7 | stub_label_stale.py docstring | (none) | `cat2_contracts/checks/stub_label_stale.py` | ~7 LOC modified |
| 5 | SHA back-fill | Cross-link audit reports | (none) | 4 commit-landing audit reports | ~16 LOC modified |

Each commit ships with its own audit report at
`docs/diagnostics/_audits/integrity_v1_2_commit<N>_landing_2026-05-15.md`.

## 3. Commit 1 — P1.8 Grandfather-sweep live-source protection

### 3.1 Pre-commit

```bash
git pull --rebase origin main
git status   # working tree should be clean
git rev-parse HEAD   # record for the audit report
```

If `tools/integrity/integrity/grandfather.py` has structural changes
since `9add149` — specifically, if `Classification`'s field set has
changed or if `classify()`'s call-site signature has changed — **pause
and surface**. The coordinator chat may have landed A.3-related
classifier work and the helpers below need re-anchoring.

### 3.2 File modifications

#### 3.2.1 `tools/integrity/integrity/grandfather.py`

**Modification A — add live-source path constants and helper near the
top of the file**, immediately after the existing dataclasses
(`Finding`, `Classification`; probe § F.3 places these at lines
23-28 and 31-35 respectively). Insert the following block between the
`Classification` dataclass and the `classify()` function:

```python


# ---------------------------------------------------------------------------
# P1.8 — live-source vs sweepable-path bucket
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
```

**Modification B — extend `apply_annotations` signature and gate
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
findings.** The function currently lives at `grandfather.py:265-330`
(probe § F.3). Modify the signature and add a filter step.

Current signature (verify on disk; if changed, re-anchor):

```python
def apply_annotations(repo_root: Path, dry_run: bool) -> tuple[int, int, dict[str, int]]:
```

Modified signature:

```python
def apply_annotations(
    repo_root: Path,
    dry_run: bool,
    sweep_live_source: bool = False,
) -> tuple[int, int, dict[str, int], int]:
```

The fourth return value is the count of findings skipped as live-source
(see § 3.2.2 for how the CLI consumes it).

Inside `apply_annotations`, after the `collect_findings(...)` call and
before `group_findings_by_target(...)`, insert the filter step. The
exact insertion point is immediately after the line that reads:

```python
findings = collect_findings(repo_root)
```

(verify the exact wording on disk; if `collect_findings` is invoked
under a different name, re-anchor).

Insert below:

```python
    # P1.8 — protect LIVE-SOURCE other-cat1 findings from sweep by default.
    # Named classifier categories (cat2-stack-*-unused, live-shader-1810, etc.)
    # remain sweepable on live-source paths -- only the heterogeneous
    # other-cat1 fallthrough bucket is dangerous to auto-annotate on live code.
    live_source_skipped = 0
    if not sweep_live_source:
        kept: list[Finding] = []
        for f in findings:
            if classify(f).category == "other-cat1" and is_live_source_path(f.file):
                live_source_skipped += 1
                continue
            kept.append(f)
        findings = kept
```

At the function's existing `return` statement (currently
`return modified_files_count, annotations_added_count, category_counts`
or similar — verify the exact tuple shape on disk), extend it to
include the new fourth element:

```python
    return modified_files_count, annotations_added_count, category_counts, live_source_skipped
```

#### 3.2.2 `tools/integrity/scripts/grandfather_sweep.py`

Current content per probe § F.1 (31 LOC). Modify the `main()` function
to add the `--sweep-live-source` flag and per-bucket summary output.

Replace the entire body of `main()` with:

```python
def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Grandfather-sweep integrity findings")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--repo-root", type=Path, default=None)
    parser.add_argument(
        "--sweep-live-source",
        action="store_true",
        help=(
            "Also sweep LIVE-SOURCE other-cat1 findings. Default is to skip them "
            "(triage section B policy). Use only when a deliberate live-source "
            "sweep is required."
        ),
    )
    ns = parser.parse_args(argv)

    root = ns.repo_root if ns.repo_root else find_repo_root()
    files, anns, counts, live_source_skipped = apply_annotations(
        root, ns.dry_run, sweep_live_source=ns.sweep_live_source,
    )

    label = "would modify" if ns.dry_run else "modified"
    print(f"grandfather-sweep: {label} {files} files; {anns} annotations added")
    if live_source_skipped:
        suffix = "" if ns.sweep_live_source else " (use --sweep-live-source to include)"
        print(f"  skipped as live-source (other-cat1): {live_source_skipped}{suffix}")
    for cat, n in sorted(counts.items(), key=lambda kv: -kv[1]):
        print(f"  {cat:>35s}: {n}")
    return 0
```

If `--sweep-live-source` is passed, `live_source_skipped` will be 0
(the filter inside `apply_annotations` doesn't run), so the
"skipped as live-source" line is omitted.

### 3.3 New file: `tools/integrity/tests/test_grandfather_sweep.py` extensions

Probe § F.4 confirms the existing file is 155 LOC. **Do not rewrite
the existing tests.** Append the following new test functions at the
end of the file. If the file does not end with a trailing newline,
add one before the new content.

The exact appended block:

```python


# ---------------------------------------------------------------------------
# P1.8 — live-source path-bucket tests
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

    # live-shader-1810 path — particle-fluids/sph-water/shaders/ subset with 1.8.10 in message.
    named_finding = _f(
        "cat1.upstream-citation",
        "particle-fluids/sph-water/shaders/density_alpha.comp.glsl",
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
```

The `_f(...)` helper already exists in the test file (probe § F.4
shows it at the top of the existing test functions); reuse it.

### 3.4 Verification block — commit 1

Before committing, run:

```bash
# Toolkit gate stays at baseline (or better).
cd /home/otacon/Projects/GPU-Sims/GPU-Sims
python3 -m integrity --mode strict --no-audit-log
# Expected: integrity: 2 pass, 0 soft-warn, 4 hard-fail, 1046 suppressed
# Exit: 1

# All grandfather-sweep tests pass.
cd tools/integrity
python3 -m pytest tests/test_grandfather_sweep.py -v
# Expected: all tests pass (existing tests + 7 new tests = 27 total)

# Dry-run sweep against current state — confirm new default skips
# the 4 live-source hard-fails.
cd /home/otacon/Projects/GPU-Sims/GPU-Sims
python3 tools/integrity/scripts/grandfather_sweep.py --dry-run
# Expected output includes a line:
#   skipped as live-source (other-cat1): 4 (use --sweep-live-source to include)
# The 4 outstanding live-source hard-fails on docs/phase12_lattice_boltzmann.md
# (x3) and particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl
# (x1) are skipped under default behavior.

# Inline sweep companion for this commit's audit report:
python3 tools/integrity/scripts/grandfather_sweep.py
# Expected: sweeps audit-doc findings introduced by this commit's audit report
# only; the 4 live-source findings remain unsuppressed.

# Re-run strict mode after sweep.
python3 -m integrity --mode strict --no-audit-log
# Expected: 4 hard-fail (unchanged); suppressed count may increase.
```

If `pytest` reports any failure, **pause and surface** — do not commit
in a failed state.

If the dry-run sweep proposes any modifications to live-source files
(anything outside `SWEEPABLE_PATH_PREFIXES` / `SWEEPABLE_EXACT_PATHS`),
**pause and surface** — the filter logic disagrees with the path-list.

### 3.5 Audit report — commit 1

Create `docs/diagnostics/_audits/integrity_v1_2_commit1_landing_2026-05-15.md`.

Front-matter:

```yaml
---
title: "Integrity v1.2 Commit 1 — P1.8 Grandfather-Sweep Live-Source Protection"
date: 2026-05-15
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_spec_2026-05-15_architect1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
---
```

Body sections (mirror v1.1 commit-landing format):

- **§ A. Change summary** — One-paragraph description of P1.8. Cite
  the addendum § 5 + probe § F.1 + § G.5 grounding.
- **§ B. File inventory** — Files modified (`grandfather.py`,
  `grandfather_sweep.py`); files extended (`test_grandfather_sweep.py`);
  new files (this audit report). Diff stat per file.
- **§ C. Verification** — Verbatim capture of the gate run,
  pytest run, and the dry-run sweep output showing the
  "skipped as live-source" line with count=4.
- **§ D. Cross-references** — Probe sections that grounded the design
  (§ F.1, § F.2, § F.7, § G.2, § G.5).
- **§ E. Banked observations** — None expected; this is a single-purpose
  commit. If any pause-and-surface fired during execution, record it
  here.
- **§ F. Next commit** — Pointer to commit 2 (P1.5).

### 3.6 Commit message — commit 1

```
feat(integrity): P1.8 grandfather-sweep live-source protection (v1.2 commit 1)

Sweep CLI now default-skips other-cat1 findings on live-source paths
per the post-batch triage section B policy. Use --sweep-live-source to
opt in to the old behavior.

Surfaced by the over-sweep pause-and-surface during the v1.1 batch-1
addendum landing (commit 9add149): the sweep proposed annotations on
all 43 unsuppressed findings including 4 pre-existing live-source
hard-fails that retro section 4.4 requires stay red.

New top-level helpers in integrity.grandfather:
  - SWEEPABLE_PATH_PREFIXES, SWEEPABLE_EXACT_PATHS
  - is_live_source_path(file_path: str) -> bool

apply_annotations() gains a sweep_live_source: bool = False parameter
and returns a fourth element (live_source_skipped: int).

grandfather_sweep.py CLI gains --sweep-live-source flag and emits a
"skipped as live-source" summary line when relevant.

Tests: +7 in test_grandfather_sweep.py covering the path-bucket
classification and the apply_annotations filter behavior.
```

### 3.7 Commit and push — commit 1

```bash
git pull --rebase origin main
git add tools/integrity/integrity/grandfather.py \
        tools/integrity/scripts/grandfather_sweep.py \
        tools/integrity/tests/test_grandfather_sweep.py \
        docs/diagnostics/_audits/integrity_v1_2_commit1_landing_2026-05-15.md
git commit -F <commit-message-file>
git push origin main
git rev-parse HEAD   # record for SHA back-fill (commit 5)
```

## 4. Commit 2 — P1.5 Register `cat3.d3q19-*` checks

### 4.1 Pre-commit

```bash
git pull --rebase origin main
git status
git rev-parse HEAD
```

If `tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py` is
absent (probe § B.1 confirmed presence at `9add149`) or has been
moved/renamed, **pause and surface**.

### 4.2 File operations

#### 4.2.1 Relocate the harness

Move:

```
tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py
  -> tools/integrity/integrity/cat3_numerical/d3q19_verify.py
```

Use `git mv` (preserves history):

```bash
git mv tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py \
       tools/integrity/integrity/cat3_numerical/d3q19_verify.py
```

The expected-values JSON (`d3q19_equilibrium.expected.json`) is also
under `checks/` per probe § B (and the JSON file's `source` field at
probe § B.1 line 257 cites the old `checks/` path). Also relocate:

```bash
git mv tools/integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.expected.json \
       tools/integrity/integrity/cat3_numerical/d3q19_equilibrium.expected.json
```

#### 4.2.2 Refactor the harness (`cat3_numerical/d3q19_verify.py`)

After the move, edit the relocated file to expose three new top-level
verify-functions plus a JSON-loader. Keep the existing `main()` and
`__main__` guard so the harness remains runnable standalone.

**Edit A — Update the `source` field in the harness's JSON-write
section.** Probe § B.1 line 257 showed:

```python
"source": "tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py",
```

Replace with:

```python
"source": "tools/integrity/integrity/cat3_numerical/d3q19_verify.py",
```

**Edit B — Update the `EXPECTED_JSON` path constant.** Probe § B.1
lines 21-22:

```python
HERE = Path(__file__).resolve().parent
EXPECTED_JSON = HERE / "d3q19_equilibrium.expected.json"
```

These lines work after relocation without change (HERE follows the
file), but verify on disk.

**Edit C — Add a JSON loader for the check modules to share.** Insert
the following function near the top of the file, after the imports
and constants and before `build_velocity_set`:

```python
def load_expected_payload() -> dict:
    """Load the expected-values JSON payload.

    Returns the dict structure described in the docstring of main() --
    velocity_set, weights, opposite_index, test_points.
    """
    if not EXPECTED_JSON.is_file():
        raise FileNotFoundError(
            f"expected values JSON missing: {EXPECTED_JSON}; "
            f"run `python3 {Path(__file__).name}` to regenerate."
        )
    return json.loads(EXPECTED_JSON.read_text(encoding="utf-8"))
```

**Edit D — Add three top-level verify-functions.** Each takes the
loaded payload dict and returns a `list[str]` of mismatch messages
(empty = pass). Insert after `load_expected_payload` and before
`build_velocity_set` (or wherever fits the existing logical ordering;
the placement is not load-bearing).

```python
def verify_velocity_set(payload: dict) -> list[str]:
    """Verify the JSON payload's velocity_set matches the re-derivation."""
    expected = [tuple(c) for c in build_velocity_set()]
    got_raw = payload.get("velocity_set")
    if got_raw is None:
        return ["velocity_set: payload key missing"]
    got = [tuple(c) for c in got_raw]
    if got != expected:
        return [
            f"velocity_set mismatch: got {got}, want {expected}",
        ]
    return []


def verify_weights(payload: dict) -> list[str]:
    """Verify weights match the re-derivation: 1/3, 1/18 x6, 1/36 x12."""
    velocity_set = [tuple(c) for c in build_velocity_set()]
    expected_fracs = [weight_for(c) for c in velocity_set]
    expected = [float(w) for w in expected_fracs]
    got = payload.get("weights")
    if got is None:
        return ["weights: payload key missing"]
    if len(got) != len(expected):
        return [f"weights length mismatch: got {len(got)}, want {len(expected)}"]
    errs: list[str] = []
    for i, (g, e) in enumerate(zip(got, expected)):
        if abs(g - e) > TOL_ABS:
            errs.append(f"weights[{i}]: got {g!r}, want {e!r}, |diff|={abs(g-e):.3e}")
    # Sanity: sum should equal 1.0 exactly under float (rational sum gives 1).
    s = sum(got)
    if abs(s - 1.0) > TOL_ABS:
        errs.append(f"sum(weights) = {s!r}, want 1.0 (|diff|={abs(s-1.0):.3e})")
    return errs


def verify_equilibrium(payload: dict) -> list[str]:
    """Verify per-test-point feq tables match a fresh evaluation of feq()."""
    velocity_set = [tuple(c) for c in build_velocity_set()]
    weights = [float(weight_for(c)) for c in velocity_set]
    test_points = payload.get("test_points")
    if not test_points:
        return ["test_points: payload key missing or empty"]
    errs: list[str] = []
    for tp in test_points:
        name = tp.get("name", "<unnamed>")
        rho = tp["rho"]
        u = tp["u"]
        got_feq = tp.get("feq")
        if got_feq is None:
            errs.append(f"{name}: feq missing")
            continue
        recomputed = feq(rho, u[0], u[1], u[2], velocity_set, weights)
        if len(recomputed) != len(got_feq):
            errs.append(f"{name}: feq length mismatch: got {len(got_feq)}, want {len(recomputed)}")
            continue
        for i, (g, r) in enumerate(zip(got_feq, recomputed)):
            if abs(g - r) > TOL_ABS:
                errs.append(
                    f"{name}.feq[{i}]: got {g!r}, recomputed {r!r}, "
                    f"|diff|={abs(g-r):.3e} > {TOL_ABS:.0e}"
                )
    return errs
```

The existing `main()` continues to do its full re-derivation +
print-trace + JSON-write workflow. The new verify-functions are
intended for the check modules and may also be useful for unit tests.

#### 4.2.3 New file: `cat3_numerical/checks/d3q19_velocity_set.py`

```python
"""Check: cat3.d3q19-velocity-set -- D3Q19 velocity vectors match algebraic ground truth.

Mode: HARD_FAIL.

Reads d3q19_equilibrium.expected.json and verifies its velocity_set
table byte-for-byte against a fresh re-derivation. Closes the
registry-vs-implementation drift surfaced by phase12_substantive_landing
section "Convention #8 firings caught and recorded" item 9 plus
addendum section 4.3 / probe section B.5.
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat3_numerical.d3q19_verify import (
    load_expected_payload,
    verify_velocity_set,
)
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat3.d3q19-velocity-set"
MODE = FailureMode.HARD_FAIL

# Anchor file for the finding; the velocity_set lives in the JSON, but the
# user-visible artifact is the algebraic derivation doc.
ANCHOR_FILE = "tools/integrity/docs/algebraic/d3q19.md"


def run(repo_root: Path) -> list[Finding]:
    findings: list[Finding] = []
    try:
        payload = load_expected_payload()
    except FileNotFoundError as e:
        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=ANCHOR_FILE,
            line=1,
            message=f"d3q19 expected payload missing: {e}",
        ))
        return findings

    for msg in verify_velocity_set(payload):
        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=ANCHOR_FILE,
            line=1,
            message=msg,
        ))
    return findings
```

#### 4.2.4 New file: `cat3_numerical/checks/d3q19_weights.py`

Identical shape with `verify_weights` and `CHECK_ID =
"cat3.d3q19-weights"`. Full source:

```python
"""Check: cat3.d3q19-weights -- D3Q19 weights match algebraic ground truth.

Mode: HARD_FAIL.

Reads d3q19_equilibrium.expected.json and verifies its weights table
within 1e-12 absolute tolerance against a fresh re-derivation.
Confirms the three values 1/3, 1/18 (x6), 1/36 (x12) per d3q19.md
section 3.1, plus the sum-to-1 sanity check.
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat3_numerical.d3q19_verify import (
    load_expected_payload,
    verify_weights,
)
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat3.d3q19-weights"
MODE = FailureMode.HARD_FAIL

ANCHOR_FILE = "tools/integrity/docs/algebraic/d3q19.md"


def run(repo_root: Path) -> list[Finding]:
    findings: list[Finding] = []
    try:
        payload = load_expected_payload()
    except FileNotFoundError as e:
        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=ANCHOR_FILE,
            line=1,
            message=f"d3q19 expected payload missing: {e}",
        ))
        return findings

    for msg in verify_weights(payload):
        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=ANCHOR_FILE,
            line=1,
            message=msg,
        ))
    return findings
```

#### 4.2.5 New file: `cat3_numerical/checks/d3q19_equilibrium.py`

```python
"""Check: cat3.d3q19-equilibrium -- D3Q19 feq() per-test-point evaluations match algebraic ground truth.

Mode: HARD_FAIL.

Reads d3q19_equilibrium.expected.json and verifies each test_points[*].feq
table against a fresh evaluation of feq(rho, u, c_i, w_i) within 1e-12
absolute tolerance. Test points pinned in d3q19.md section 4.2:
  tp1_zero_velocity   -- rho=1.0, u=(0,0,0)
  tp2_uniform_x       -- rho=1.0, u=(0.1, 0, 0)
  tp3_oblique_xy      -- rho=1.0, u=(0.05, 0.05, 0)
  tp4_density_scaled  -- rho=2.5, u=(0.05, 0, 0)
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat3_numerical.d3q19_verify import (
    load_expected_payload,
    verify_equilibrium,
)
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat3.d3q19-equilibrium"
MODE = FailureMode.HARD_FAIL

ANCHOR_FILE = "tools/integrity/docs/algebraic/d3q19.md"


def run(repo_root: Path) -> list[Finding]:
    findings: list[Finding] = []
    try:
        payload = load_expected_payload()
    except FileNotFoundError as e:
        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=ANCHOR_FILE,
            line=1,
            message=f"d3q19 expected payload missing: {e}",
        ))
        return findings

    for msg in verify_equilibrium(payload):
        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=ANCHOR_FILE,
            line=1,
            message=msg,
        ))
    return findings
```

#### 4.2.6 Modified file: `cat3_numerical/checks/__init__.py`

Probe § C.1 verbatim content (7 LOC):

```python
"""Cat 3 check modules. Discovered by integrity.runner.discover_checks."""

from integrity.cat3_numerical.checks import cubic_kernel

REGISTERED_CHECKS = [
    (cubic_kernel.CHECK_ID, cubic_kernel),
]
```

Replace with:

```python
"""Cat 3 check modules. Discovered by integrity.runner.discover_checks."""

from integrity.cat3_numerical.checks import (
    cubic_kernel,
    d3q19_velocity_set,
    d3q19_weights,
    d3q19_equilibrium,
)

REGISTERED_CHECKS = [
    (cubic_kernel.CHECK_ID, cubic_kernel),
    (d3q19_velocity_set.CHECK_ID, d3q19_velocity_set),
    (d3q19_weights.CHECK_ID, d3q19_weights),
    (d3q19_equilibrium.CHECK_ID, d3q19_equilibrium),
]
```

#### 4.2.7 New file: `tools/integrity/tests/test_cat3_d3q19.py`

```python
"""Tests for the cat3.d3q19-* check modules and the shared harness helpers."""

from __future__ import annotations

import json
from pathlib import Path

import pytest

from integrity.cat3_numerical.d3q19_verify import (
    EXPECTED_JSON,
    build_velocity_set,
    feq,
    load_expected_payload,
    verify_equilibrium,
    verify_velocity_set,
    verify_weights,
    weight_for,
)
from integrity.cat3_numerical.checks import (
    d3q19_velocity_set,
    d3q19_weights,
    d3q19_equilibrium,
)
from integrity.common.results import FailureMode


# ---------------------------------------------------------------------------
# Harness-level tests
# ---------------------------------------------------------------------------


def test_build_velocity_set_returns_19_vectors() -> None:
    cs = build_velocity_set()
    assert len(cs) == 19


def test_build_velocity_set_first_moment_vanishes() -> None:
    cs = build_velocity_set()
    sx = sum(c[0] for c in cs)
    sy = sum(c[1] for c in cs)
    sz = sum(c[2] for c in cs)
    assert (sx, sy, sz) == (0, 0, 0)


def test_weight_for_partitions_by_squared_norm() -> None:
    # Rest vector: 1/3
    assert float(weight_for((0, 0, 0))) == pytest.approx(1.0 / 3.0)
    # Face neighbor: 1/18
    assert float(weight_for((1, 0, 0))) == pytest.approx(1.0 / 18.0)
    # Edge neighbor: 1/36
    assert float(weight_for((1, 1, 0))) == pytest.approx(1.0 / 36.0)


def test_feq_conserves_mass_at_zero_velocity() -> None:
    cs = build_velocity_set()
    ws = [float(weight_for(c)) for c in cs]
    out = feq(1.0, 0.0, 0.0, 0.0, cs, ws)
    assert sum(out) == pytest.approx(1.0, abs=1e-12)


# ---------------------------------------------------------------------------
# Check-module tests
# ---------------------------------------------------------------------------


@pytest.fixture
def repo_root(tmp_path: Path) -> Path:
    return tmp_path


def test_velocity_set_check_passes_on_correct_payload(repo_root: Path) -> None:
    findings = d3q19_velocity_set.run(repo_root)
    assert findings == []


def test_weights_check_passes_on_correct_payload(repo_root: Path) -> None:
    findings = d3q19_weights.run(repo_root)
    assert findings == []


def test_equilibrium_check_passes_on_correct_payload(repo_root: Path) -> None:
    findings = d3q19_equilibrium.run(repo_root)
    assert findings == []


def test_velocity_set_check_fails_on_corrupted_payload(
    monkeypatch: pytest.MonkeyPatch, repo_root: Path
) -> None:
    """Corrupting the velocity_set in the payload should produce a HARD_FAIL finding."""
    real = load_expected_payload()
    real["velocity_set"][0] = [9, 9, 9]  # corrupt the rest vector

    monkeypatch.setattr(
        "integrity.cat3_numerical.checks.d3q19_velocity_set.load_expected_payload",
        lambda: real,
    )
    findings = d3q19_velocity_set.run(repo_root)
    assert len(findings) == 1
    assert findings[0].check_id == "cat3.d3q19-velocity-set"
    assert findings[0].mode == FailureMode.HARD_FAIL
    assert "mismatch" in findings[0].message


def test_weights_check_fails_on_corrupted_payload(
    monkeypatch: pytest.MonkeyPatch, repo_root: Path
) -> None:
    real = load_expected_payload()
    real["weights"][0] = 0.5  # was 1/3

    monkeypatch.setattr(
        "integrity.cat3_numerical.checks.d3q19_weights.load_expected_payload",
        lambda: real,
    )
    findings = d3q19_weights.run(repo_root)
    assert len(findings) >= 1
    assert all(f.check_id == "cat3.d3q19-weights" for f in findings)


def test_equilibrium_check_fails_on_corrupted_payload(
    monkeypatch: pytest.MonkeyPatch, repo_root: Path
) -> None:
    real = load_expected_payload()
    real["test_points"][0]["feq"][0] = 999.0  # arbitrary nonsense

    monkeypatch.setattr(
        "integrity.cat3_numerical.checks.d3q19_equilibrium.load_expected_payload",
        lambda: real,
    )
    findings = d3q19_equilibrium.run(repo_root)
    assert len(findings) >= 1
    assert all(f.check_id == "cat3.d3q19-equilibrium" for f in findings)


def test_check_ids_match_registry() -> None:
    """The three CHECK_IDs must match the [Algebraic_D3Q19] used_by_checks
    entries in ground-truth-sources.md (verified by probe section B.4)."""
    assert d3q19_velocity_set.CHECK_ID == "cat3.d3q19-velocity-set"
    assert d3q19_weights.CHECK_ID == "cat3.d3q19-weights"
    assert d3q19_equilibrium.CHECK_ID == "cat3.d3q19-equilibrium"


def test_payload_is_present() -> None:
    """Pin the expected JSON's presence at the new path."""
    assert EXPECTED_JSON.is_file()
```

### 4.3 Verification block — commit 2

```bash
cd /home/otacon/Projects/GPU-Sims/GPU-Sims

# All cat3 tests pass.
cd tools/integrity
python3 -m pytest tests/test_cat3_cubic_kernel.py tests/test_cat3_d3q19.py -v
# Expected: 12 cubic-kernel tests + 12 d3q19 tests all pass.

# Standalone harness still works.
cd /home/otacon/Projects/GPU-Sims/GPU-Sims
python3 tools/integrity/integrity/cat3_numerical/d3q19_verify.py
# Expected stdout: same trace as probe section B.2 + "=== ALL CHECKS PASS ==="

# The three checks are now discoverable.
python3 -m integrity --check cat3.d3q19-velocity-set --no-audit-log
# Expected: integrity: 1 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
python3 -m integrity --check cat3.d3q19-weights --no-audit-log
# Expected: integrity: 1 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
python3 -m integrity --check cat3.d3q19-equilibrium --no-audit-log
# Expected: integrity: 1 pass, 0 soft-warn, 0 hard-fail, 0 suppressed

# Gate stays at baseline (now 5 passes instead of 2; hard-fail count unchanged).
python3 -m integrity --mode strict --no-audit-log
# Expected: integrity: 5 pass, 0 soft-warn, 4 hard-fail, 1046 suppressed (or similar)
#   - "5 pass" reflects the three newly registered checks
#   - "4 hard-fail" unchanged (the live-source pool is the same)
#   - "1046 suppressed" may shift slightly if this commit's audit report
#     introduces audit-doc findings; the inline sweep below normalizes.

# Inline sweep companion (now safe with P1.8's default).
python3 tools/integrity/scripts/grandfather_sweep.py
# Expected: sweeps only the new audit-doc findings on this commit's
# audit report; the "skipped as live-source" line should report 4.

python3 -m integrity --mode strict --no-audit-log
# Expected: 5 pass, 4 hard-fail; suppressed count finalized post-sweep.
```

If the standalone harness fails on the corrupted-payload tests, or if
the relocated file paths don't resolve, **pause and surface**.

If the discoverability check returns `0 pass` (i.e., the registration
did not take effect), check `cat3_numerical/checks/__init__.py` for
syntax errors or import errors and re-run.

### 4.4 Audit report — commit 2

Path: `docs/diagnostics/_audits/integrity_v1_2_commit2_landing_2026-05-15.md`.

Front-matter mirrors commit 1's. Body sections:

- **§ A. Change summary** — P1.5 closes Convention #8 firing #9
  from `phase12_substantive_landing_2026-05-15.md`. Three new
  CHECK_IDs registered.
- **§ B. File inventory** — 2 file moves (harness + JSON), 3 new check
  modules, 1 new test file, 1 modified registry `__init__.py`,
  1 modified harness with new top-level functions, 1 audit report.
- **§ C. Verification** — Per-check discovery output, gate state
  before/after, standalone harness output.
- **§ D. Cross-references** — Probe § B.1, § B.4, § B.5, § C.1–§ C.7;
  addendum § 4.3.
- **§ E. Banked observations** — Note that the existing `expected_values.toml`
  for cubic-kernel and the new JSON for d3q19 use different serialization
  formats. Probe § C.4 flagged this; no action recommended in this batch,
  but future cat3 checks should decide on a unified format. Bank as
  v1.3 candidate.
- **§ F. Next commit** — Pointer to commit 3 (P1.6).

### 4.5 Commit message — commit 2

```
feat(integrity): P1.5 register cat3.d3q19-* checks (v1.2 commit 2)

Three new check modules wire the existing d3q19_verify.py harness
into the check-discovery surface:
  - cat3.d3q19-velocity-set
  - cat3.d3q19-weights
  - cat3.d3q19-equilibrium

Closes the registry-vs-implementation drift surfaced as Convention #8
firing #9 in phase12_substantive_landing_2026-05-15.md and confirmed by
probe section B.5 (all three CHECK_IDs returned zero matches pre-commit).

Harness relocated from cat3_numerical/checks/d3q19_verify.py to
cat3_numerical/d3q19_verify.py to mirror the cat3.cubic-kernel
algorithmic/check split. Expected-values JSON moved alongside. Three
new top-level functions in the harness (verify_velocity_set,
verify_weights, verify_equilibrium) are the integration surface the
check modules consume.

Tests: +12 in tests/test_cat3_d3q19.py covering harness re-derivation,
check-module pass on real payload, and check-module fail on
deliberately corrupted payloads.
```

### 4.6 Commit and push — commit 2

```bash
git pull --rebase origin main
git add -A   # captures moves, new files, and registry modification
git status   # verify only the expected paths are staged
git commit -F <commit-message-file>
git push origin main
git rev-parse HEAD
```

If `git status` shows unexpected files staged, **pause and surface**.

## 5. Commit 3 — P1.6 Strict-mode human-renderer suppressed-stanza filter

### 5.1 Pre-commit

```bash
git pull --rebase origin main
git status
git rev-parse HEAD
```

If `tools/integrity/integrity/runner.py:141-145` no longer matches probe
§ D.2's verbatim content, **pause and surface**.

### 5.2 File modifications

#### 5.2.1 `tools/integrity/integrity/runner.py`

Probe § D.2 verbatim (lines 141-145):

```python
    else:
        _emit_human_summary(summary)
        for f in findings:
            sys.stdout.write(f"  {f.mode.name}: {f.check_id} at {f.file}:{f.line}\n")
            sys.stdout.write(f"    {f.message}\n")
```

Replace with:

```python
    else:
        _emit_human_summary(summary)
        # P1.6 -- mirror the github branch's suppressed filter (line 134).
        # Original commit fc20ef7 added the filter only to the github output;
        # the human branch was always rendering suppressed findings as
        # HARD_FAIL stanzas, producing a summary/stanza-count mismatch.
        for f in findings:
            if f.suppressed:
                continue
            sys.stdout.write(f"  {f.mode.name}: {f.check_id} at {f.file}:{f.line}\n")
            sys.stdout.write(f"    {f.message}\n")
```

#### 5.2.2 New file: `tools/integrity/tests/test_runner_human_output.py`

```python
"""Tests for the human-output renderer in integrity.runner.emit_output.

Pins the P1.6 fix: the default human-format output filters out suppressed
findings, matching the summary line's hard_fail count rather than emitting
HARD_FAIL stanzas for every suppressed finding.
"""

from __future__ import annotations

import io
import sys
from dataclasses import dataclass

import pytest

from integrity.common.results import FailureMode, Finding
from integrity.runner import RunSummary, emit_output


@dataclass
class _Args:
    """Minimal CliArgs stand-in for emit_output's args parameter."""
    output: str = "human"
    mode: str = "strict"
    root: object = None
    no_audit_log: bool = True


def _capture_emit(summary: RunSummary, findings: list[Finding], args: _Args) -> str:
    buf = io.StringIO()
    saved = sys.stdout
    try:
        sys.stdout = buf
        emit_output(summary, findings, args)
    finally:
        sys.stdout = saved
    return buf.getvalue()


def _make_finding(check_id: str, file: str, line: int, message: str, *,
                  suppressed: bool, mode: FailureMode = FailureMode.HARD_FAIL) -> Finding:
    return Finding(
        check_id=check_id,
        mode=mode,
        file=file,
        line=line,
        message=message,
        suppressed=suppressed,
    )


def test_human_output_omits_suppressed_stanzas() -> None:
    findings = [
        _make_finding("cat1.intra-repo", "CHANGELOG.md", 10, "bare-path", suppressed=True),
        _make_finding("cat1.intra-repo", "docs/phase12.md", 20, "bare-path", suppressed=False),
    ]
    summary = RunSummary(passes=0, soft_warns=0, hard_fails=1, suppressions=1)
    out = _capture_emit(summary, findings, _Args(output="human"))
    # The unsuppressed finding renders.
    assert "docs/phase12.md" in out
    # The suppressed finding does NOT render as a stanza.
    assert "CHANGELOG.md" not in out


def test_human_output_summary_counts_match_stanza_count() -> None:
    """The summary line says N hard-fail; exactly N HARD_FAIL stanzas should appear."""
    findings = [
        _make_finding("cat1.intra-repo", f"a/b{i}.md", 1, "x", suppressed=(i >= 3))
        for i in range(10)
    ]
    summary = RunSummary(passes=0, soft_warns=0, hard_fails=3, suppressions=7)
    out = _capture_emit(summary, findings, _Args(output="human"))

    hard_fail_lines = [line for line in out.splitlines() if "HARD_FAIL" in line]
    # 3 expected: one stanza header per unsuppressed hard-fail.
    assert len(hard_fail_lines) == 3

    # Summary line is present and reports the right counts.
    assert "3 hard-fail" in out
    assert "7 suppressed" in out


def test_github_output_unchanged_still_omits_suppressed() -> None:
    """Regression guard: P1.6 must not break the github branch's existing filter."""
    findings = [
        _make_finding("cat1.intra-repo", "CHANGELOG.md", 10, "x", suppressed=True),
        _make_finding("cat1.intra-repo", "docs/phase12.md", 20, "y", suppressed=False),
    ]
    summary = RunSummary(passes=0, soft_warns=0, hard_fails=1, suppressions=1)
    out = _capture_emit(summary, findings, _Args(output="github"))

    # ::error stanza for the unsuppressed finding only.
    error_lines = [line for line in out.splitlines() if line.startswith("::error")]
    assert len(error_lines) == 1
    assert "docs/phase12.md" in error_lines[0]


def test_human_output_no_suppressed_means_full_render() -> None:
    """When nothing is suppressed, every finding renders a stanza."""
    findings = [
        _make_finding("cat1.intra-repo", f"a/b{i}.md", 1, "x", suppressed=False)
        for i in range(5)
    ]
    summary = RunSummary(passes=0, soft_warns=0, hard_fails=5, suppressions=0)
    out = _capture_emit(summary, findings, _Args(output="human"))

    hard_fail_lines = [line for line in out.splitlines() if "HARD_FAIL" in line]
    assert len(hard_fail_lines) == 5
```

Note on the `Finding` dataclass: tests construct `Finding` objects with
a `suppressed` field. Verify on disk that `Finding` in
`integrity.common.results` (or wherever it lives — probe § F.3 referenced
the `Finding` dataclass shape in `grandfather.py`, but the runner imports
a different `Finding` from `integrity.common.results`) carries this
field. If the runner-side `Finding` does not have a `suppressed`
field at the level emit_output checks (probe § A inferred the renderer
checks `f.suppressed`), **pause and surface** — the test fixtures need
adjustment to match.

### 5.3 Verification block — commit 3

```bash
cd /home/otacon/Projects/GPU-Sims/GPU-Sims/tools/integrity

# The new test file passes.
python3 -m pytest tests/test_runner_human_output.py -v
# Expected: 4 tests pass.

# Existing runner tests still pass.
python3 -m pytest tests/test_runner.py -v
# Expected: all existing tests pass.

# Full strict-mode run — the summary and stanza count should now agree.
cd /home/otacon/Projects/GPU-Sims/GPU-Sims
python3 -m integrity --mode strict --no-audit-log 2>&1 | tail -20
# Expected: the trailing "X hard-fail, Y suppressed" line agrees with
# the count of stanzas emitted above it. Previously: 4 hard-fail / 1046
# suppressed with 1050 stanzas. After fix: 4 hard-fail / 1046+ suppressed
# with exactly 4 stanzas.

# Count check (run again for a precise tally):
python3 -m integrity --mode strict --no-audit-log 2>&1 | grep -c "HARD_FAIL:"
# Expected: 4 (matches the summary's hard-fail count).
```

If the post-fix stanza count is not exactly 4 (matching the unsuppressed
hard-fail count), **pause and surface** — the fix didn't take effect.

### 5.4 Audit report — commit 3

Path: `docs/diagnostics/_audits/integrity_v1_2_commit3_landing_2026-05-15.md`.

Body sections mirror commits 1 and 2. Specifically capture:

- **§ C.** Pre-fix vs post-fix stanza-count comparison verbatim
  (matches probe § A.2 / § D.4 anomaly resolution).
- **§ D.** Reference probe § D.2, § D.5 (asymmetry origin in `fc20ef7`).
- **§ E.** No banked observations expected; this is a clean one-line
  fix.

### 5.5 Commit message — commit 3

```
fix(integrity): P1.6 human-renderer omits suppressed stanzas (v1.2 commit 3)

The strict-mode human-format renderer was emitting HARD_FAIL stanzas
for every finding regardless of suppressed status, producing a
mismatch between the summary line ("4 hard-fail, 1046 suppressed")
and the actual stanza count (1050). The github-output branch has
always had the correct `if f.suppressed: continue` guard (added in
commit fc20ef7); the human branch never received the same filter.

One-line fix in runner.py emit_output() else-branch. Adds the same
guard the github branch uses. Plus a four-test pin in a new file
tests/test_runner_human_output.py to lock the behavior.

Confirmed by probe section A.2 / D.2 / D.4 / D.5.
```

### 5.6 Commit and push — commit 3

```bash
git pull --rebase origin main
git add tools/integrity/integrity/runner.py \
        tools/integrity/tests/test_runner_human_output.py \
        docs/diagnostics/_audits/integrity_v1_2_commit3_landing_2026-05-15.md
git commit -F <commit-message-file>
git push origin main
git rev-parse HEAD
```

Run the inline sweep companion as part of this commit's pre-push
verification:

```bash
python3 tools/integrity/scripts/grandfather_sweep.py
# Expected: sweeps only the new audit-doc findings introduced by
# this commit's audit report; "skipped as live-source: 4" line present.
```

## 6. Commit 4 — P1.7 `stub_label_stale.py` module-docstring drift

### 6.1 Pre-commit

```bash
git pull --rebase origin main
git status
git rev-parse HEAD
```

If `tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py:15-18`
no longer matches probe § E.1's verbatim content, **pause and surface**.

### 6.2 File modifications

#### 6.2.1 `tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py`

Probe § E.1 verbatim (lines 15-18):

```
15	Sibling-impl resolution (batch-1-spec Decision 2):
16	  - `.hpp`/`.h` in `common-cpp/include/<sub>/<base>.hpp` ->
17	    `common-cpp/src/<sub>/<base>.cpp` (relative path mirror)
18	  - `.py`: impl is the same file
```

Replace exactly those four lines with:

```
Sibling-impl resolution (corrected post-batch-1 per commit-1 landing audit section E.1):
  - `.hpp`/`.h` in `common-cpp/include/<namespace>/<rest>.hpp` ->
    `common-cpp/src/<rest>.cpp` (first directory component after
    `include/` is the project namespace and is stripped)
  - `.py`: impl is the same file (Python does not separate
    declaration from implementation)
```

Verify after edit that the line numbers shift only minimally (replacing
4 lines with 6 lines adds 2 lines to the file; the in-function docstring
at lines 96-105 should move to 98-107). No other content edits.

### 6.3 Verification block — commit 4

```bash
cd /home/otacon/Projects/GPU-Sims/GPU-Sims/tools/integrity

# Existing stub-label-stale tests still pass (no code change, no test
# change, but verify nothing imported the old docstring text).
python3 -m pytest tests/test_cat2_stack_d.py -v
# Expected: all existing tests pass.

# Confirm module imports cleanly (catches stray broken triple-quoted-string
# from a bad edit).
python3 -c "from integrity.cat2_contracts.checks import stub_label_stale; print('ok')"
# Expected: "ok"

# Verify the docstring text matches the code (manual grep check):
grep -A 5 "Sibling-impl resolution" \
  tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py
# Expected: both occurrences (module docstring + in-function docstring)
# now describe the namespace-strip convention; neither describes
# "relative path mirror" any more.

# Gate stays at baseline.
cd /home/otacon/Projects/GPU-Sims/GPU-Sims
python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
# Expected: "integrity: 5 pass, 0 soft-warn, 4 hard-fail, ... suppressed"
```

If the grep returns any line containing "relative path mirror",
**pause and surface** — the edit was incomplete.

### 6.4 Audit report — commit 4

Path: `docs/diagnostics/_audits/integrity_v1_2_commit4_landing_2026-05-15.md`.

Body sections — short. P1.7 is cosmetic only. Body:

- **§ A.** Change summary — one paragraph.
- **§ B.** File inventory — 1 modified file (stub_label_stale.py, ~7 lines).
- **§ C.** Verification — module import test + grep confirmation.
- **§ D.** Cross-references — probe § E.1, § E.3, § E.4; integrity_v1_1_commit1_landing_2026-05-15.md § E.1
  (the original pause-and-surface that left the docstring stale).
- **§ E.** Banked observations — none expected.
- **§ F.** Next commit — pointer to commit 5 (SHA back-fill).

### 6.5 Commit message — commit 4

```
docs(integrity): P1.7 fix stub_label_stale.py module-docstring drift (v1.2 commit 4)

The module docstring at lines 15-18 described the original-spec
"relative path mirror" sibling-impl resolution; the actual code (and
the in-function docstring at lines 96-105) implements the corrected
namespace-strip convention from pause-and-surface #1 during the v1.1
batch-1 commit-1 execution. The module-level docstring was never
updated alongside the logic correction. Cosmetic but worth fixing:
future readers trust docstrings.

Confirmed by probe section E.3 (docstring-vs-code discrepancy verbatim).
```

### 6.6 Commit and push — commit 4

```bash
git pull --rebase origin main
git add tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py \
        docs/diagnostics/_audits/integrity_v1_2_commit4_landing_2026-05-15.md
git commit -F <commit-message-file>
git push origin main
git rev-parse HEAD
```

Inline sweep companion same as previous commits:

```bash
python3 tools/integrity/scripts/grandfather_sweep.py
```

## 7. Commit 5 — SHA back-fill

### 7.1 Purpose

Commits 1–4's audit reports were authored before subsequent commits'
SHAs were known. The reports cross-reference each other (§ F "Next
commit" pointer; § A change summary mentions sibling commits' SHAs).
Commit 5 back-fills those references.

Per Convention #12 (retro § 7.2): SHA back-fill is a separate commit,
never `--amend`. Force-pushing rewritten SHAs would break links from
external readers, CI logs, and any sibling audit doc.

### 7.2 Pre-commit

```bash
git pull --rebase origin main
git status

# Record all four prior SHAs.
git log --oneline -5
# Expected: commits 4, 3, 2, 1 visible.
```

### 7.3 File modifications

Each of the four audit reports under
`docs/diagnostics/_audits/integrity_v1_2_commit<N>_landing_2026-05-15.md`
has at minimum one `<COMMIT_N_SHA>` placeholder (or whatever equivalent
shorthand was used during initial drafting — verify exact placeholder
form on disk).

For each audit report, replace placeholders with the actual SHAs:

- Commit 1's audit → replace its own `<COMMIT_1_SHA>` placeholders with
  the actual SHA of commit 1, and similarly for any forward-references
  to commits 2/3/4.
- Commits 2, 3, 4's audits → same pattern.

If no explicit placeholder was used during drafting and the audit
reports already use the actual SHAs (because they were authored
post-commit), this commit is unnecessary — verify by greping for
`<COMMIT_` placeholders across the four files:

```bash
grep -l "<COMMIT_[1-4]_SHA>" \
  docs/diagnostics/_audits/integrity_v1_2_commit*_landing_2026-05-15.md
```

If the grep returns no matches, **pause and surface** — either the
back-fill is unnecessary (skip this commit, note in the closing audit)
or the placeholder convention differed (need to identify the actual
placeholders).

### 7.4 Verification block — commit 5

```bash
# No placeholders remain.
grep -l "<COMMIT_[1-4]_SHA>" \
  docs/diagnostics/_audits/integrity_v1_2_commit*_landing_2026-05-15.md
# Expected: empty output.

# Every cross-reference resolves.
for sha in $(grep -oE '\b[a-f0-9]{7,40}\b' \
  docs/diagnostics/_audits/integrity_v1_2_commit*_landing_2026-05-15.md | \
  sort -u); do
    git cat-file -e "$sha" 2>/dev/null && echo "OK $sha" || echo "BAD $sha"
done
# Expected: all "OK"; if any "BAD", a SHA in the audit reports doesn't
# resolve in the repo. Pause-and-surface.

# Gate stays clean.
cd /home/otacon/Projects/GPU-Sims/GPU-Sims
python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
```

### 7.5 Commit message — commit 5

```
docs(integrity): SHA back-fill for v1.2 commits 1-4 (v1.2 commit 5)

Replace <COMMIT_N_SHA> placeholders in the four v1.2-commit audit
reports with the actual SHAs. Per Convention #12, back-fill is a
separate follow-up commit, never --amend.
```

### 7.6 Commit and push — commit 5

```bash
git pull --rebase origin main
git add docs/diagnostics/_audits/integrity_v1_2_commit*_landing_2026-05-15.md
git commit -F <commit-message-file>
git push origin main
git rev-parse HEAD
```

No inline sweep needed for commit 5 if it touches only audit-doc files
that are already covered by existing classifier rules (`audit-citation`
category). Verify post-push:

```bash
python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
# Expected: 5 pass, 4 hard-fail (unchanged); suppressed count incremented
# by however many new audit-doc citations the back-fill added.
```

If hard-fail count drifts (e.g., a back-fill introduced a typo'd SHA
that classify() reads as a bare-path candidate), run an inline sweep:

```bash
python3 tools/integrity/scripts/grandfather_sweep.py
```

## 8. End-state verification (after commit 5)

```bash
cd /home/otacon/Projects/GPU-Sims/GPU-Sims

# Toolkit gate.
python3 -m integrity --mode strict --no-audit-log
# Expected: 5 pass, 0 soft-warn, 4 hard-fail, ~1046+ suppressed (exact
# suppressed count depends on how many audit-doc findings the four new
# audit reports + this back-fill introduced; should not exceed ~1100).
# Exit: 1 (the 4 baseline live-source hard-fails are unchanged).

# Test suite.
cd tools/integrity
python3 -m pytest tests/ -v
# Expected: all tests pass. Count delta from probe-time baseline:
# - +7 in test_grandfather_sweep.py (P1.8)
# - +12 in test_cat3_d3q19.py (P1.5, new file)
# - +4 in test_runner_human_output.py (P1.6, new file)
# - 0 in test_cat2_stack_d.py (P1.7 is docstring-only)
# Total: +23 tests.

# Discoverability.
cd /home/otacon/Projects/GPU-Sims/GPU-Sims
python3 -m integrity --check cat3.d3q19-velocity-set --no-audit-log
python3 -m integrity --check cat3.d3q19-weights --no-audit-log
python3 -m integrity --check cat3.d3q19-equilibrium --no-audit-log
# Expected: each reports "1 pass, 0 soft-warn, 0 hard-fail, 0 suppressed".

# Human-render summary/stanza agreement.
python3 -m integrity --mode strict --no-audit-log 2>&1 | grep -c "HARD_FAIL:"
# Expected: 4 (matches summary's hard-fail count exactly).

# Sweep default behavior.
python3 tools/integrity/scripts/grandfather_sweep.py --dry-run
# Expected stdout contains: "skipped as live-source (other-cat1): 4 (use --sweep-live-source to include)"
# Expected: no proposed annotations on docs/phase12_lattice_boltzmann.md
# or particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl.

# Docstring agreement.
grep "relative path mirror" \
  tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py
# Expected: empty output.
```

If any of these checks disagree, **pause and surface** at the commit
that introduced the regression and fix forward.

## 9. References

- `docs/retro/integrity-toolkit-v1.md` — v1 retro (foundational conventions)
- `docs/retro/integrity-toolkit-v1.1-batch1.md` — v1.1 batch-1 retro (original A.x priority list)
- `docs/retro/integrity-toolkit-v1.1-batch1-addendum.md` — addendum (§ 5 P-numbering for this batch)
- `docs/diagnostics/_audits/integrity_v1_1_self_review_probe_2026-05-15_architect1.md` — self-review probe that surfaced P1.5/P1.6
- `docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md` — pre-spec probe (this batch's grounding)
- `docs/diagnostics/_audits/integrity_v1_1_post_batch_triage_2026-05-15.md` — § B triage bucket definitions (P1.8 grounding)
- `docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md` — v1.1 batch-1 execution spec (this spec's structural template)
- `docs/diagnostics/_audits/integrity_v1_1_commit1_landing_2026-05-15.md` — § E.1 pause-and-surface #1 record (P1.7 grounding)
- `docs/diagnostics/_audits/phase12_substantive_landing_2026-05-15.md` — Convention #8 firing #9 record (P1.5 grounding)

## End of execution spec
