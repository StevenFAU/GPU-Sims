---
title: "Integrity v1.2 Commit 1 — P1.8 Grandfather-Sweep Live-Source Protection"
date: 2026-05-15
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_spec_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_a3_commit1_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_2_a3_commit2_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_2_a3_commit3_landing_2026-05-15.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
---

# Integrity v1.2 Commit 1 — P1.8 Grandfather-Sweep Live-Source Protection

## § A. Change summary

P1.8 lands the live-source-safe default for the grandfather-sweep CLI per
addendum § 5 / probe § F.1 / § G.5. The post-batch triage
(`docs/diagnostics/_audits/integrity_v1_1_post_batch_triage_2026-05-15.md` § B)
defines three buckets: AUDIT-DOC, TOOLKIT-DOC (both sweep-eligible), and
LIVE-SOURCE (attribute to introducing author, do NOT sweep). The CLI used
to sweep all unsuppressed findings, which over-swept LIVE-SOURCE
`other-cat1` findings — the over-sweep was the surfacing failure that
forced pause-and-surface during commit `9add149` (probe § A.2). The
filter now lives in code: `is_live_source_path()` defines the bucket
boundary, and `apply_annotations(sweep_live_source=False)` honors it by
default. `--sweep-live-source` is the explicit opt-in for the old
all-paths behavior.

## § B. File inventory

- `tools/integrity/integrity/grandfather.py` — modified.
  - Added `SWEEPABLE_PATH_PREFIXES`, `SWEEPABLE_EXACT_PATHS`,
    `is_live_source_path()` block after the `Classification` dataclass.
  - Extended `apply_annotations()` signature with
    `sweep_live_source: bool = False`; added a fourth return element
    `live_source_skipped: int`.
  - Inserted the LIVE-SOURCE filter step immediately after the
    `collect_findings()` call.
  - Plus inline-sweep annotations for toolkit-own-source grammar
    literals (the sweep companion's scope on this file).
- `tools/integrity/scripts/grandfather_sweep.py` — replaced `main()` body.
  - Added `--sweep-live-source` argparse flag.
  - Threaded `sweep_live_source=ns.sweep_live_source` into the
    `apply_annotations()` call.
  - Added the "skipped as live-source" summary line (printed only when
    `live_source_skipped > 0`).
- `tools/integrity/tests/test_grandfather_sweep.py` — appended 7 new tests
  covering `is_live_source_path()` (4 tests) and
  `apply_annotations()` filter behavior (3 tests).
- `tools/integrity/docs/grandfather-catalog.md` — inline-sweep
  spec-grammar-example annotations on the new bare-path catalog
  entries introduced by A.3 commit 2.
- `docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md`,
  `docs/diagnostics/_audits/integrity_v1_2_bolt_ons_spec_2026-05-15_architect1.md`,
  `docs/diagnostics/_audits/integrity_v1_2_commit1_landing_2026-05-15.md` —
  new (probe + spec + this landing report).

**Sweep companion scope:** This commit deliberately ships only the
inline-sweep changes that fall on files it is already modifying
(`grandfather.py` + `grandfather-catalog.md`). The broader
A.3-introduced bare-path sweep (584 audit-bare-path + 43
other-cat1-bare-path + 11 retro-bare-path + 7 toolkit-doc-bare-path
attempted annotations across ~50 audit/retro/spec docs) is **out
of scope here** — A.3 commit 3 (`880a400`) explicitly states "Commit
4 closes the gate via the grandfather sweep companion." That work
belongs to A.3's planned commit 4, not to this commit. P1.8 (the
filter) and A.3 sweep companion (the bulk annotations) are
independent landings.

## § C. Verification

### C.1 Pre-edit toolkit gate (post-A.3 baseline)

```
integrity: 2 pass, 0 soft-warn, 652 hard-fail, 421 suppressed
```

**Acknowledgement per spec § 0.1 hard-rule #6:** the probe-time
baseline was 4 hard-fail. Between probe SHA `9add149` and the start of
this commit's execution, the coordinator-chat A.3 work landed:

- `6fc5884` — `feat(integrity): add cat1.bare-path check module + tests (v1.2 A.3 commit 1)`
- `77628b6` — `feat(integrity): add cat1.bare-path classifier rules + catalog (v1.2 A.3 commit 2)`
- `880a400` — `feat(integrity): register cat1.bare-path + add skip-guard (v1.2 A.3 commit 3)`

A.3 commit 3 explicitly notes: "Gate goes intentionally red at this
commit (653 hard-fails, mostly new cat1.bare-path findings). Commit 4
closes the gate via the grandfather sweep companion." The 652 baseline
above is this A.3-commit-3-post / A.3-commit-4-pre state.

Spec § 0.4 hard-pauses on structural changes to `Classification` or
`classify()`'s signature; neither changed (A.3 added new branches to
`classify()` returning the same `Classification` dataclass). P1.8
helpers compose cleanly with A.3 additions — `is_live_source_path()`
sits above `classify()` and operates on file paths, not classification
categories. **No pause-and-surface fired.**

### C.2 New tests

```
$ cd tools/integrity && python3 -m pytest tests/test_grandfather_sweep.py -v
27 passed in 0.03s
```

20 existing + 7 new (4 path-bucket tests + 3 `apply_annotations` filter
tests). All pass.

Full toolkit test suite:
```
$ python3 -m pytest tests/
119 passed in 144s
```

### C.3 Dry-run sweep — confirms P1.8 default behavior

```
$ python3 tools/integrity/scripts/grandfather_sweep.py --dry-run
grandfather-sweep: would modify 50 files; 577 annotations added
  skipped as live-source (other-cat1): 6 (use --sweep-live-source to include)
                      audit-bare-path: 584
                 other-cat1-bare-path: 43
                      retro-bare-path: 11
                toolkit-doc-bare-path: 7
                 spec-grammar-example: 1
                       audit-doc-1810: 1
```

The "skipped as live-source (other-cat1): 6" line confirms the P1.8
default behavior:

- 6 LIVE-SOURCE `cat1.intra-repo` findings classify as `other-cat1`
  and are correctly skipped — the intended P1.8 protection. These are:
  - 3 in `docs/phase12_lattice_boltzmann.md` (unresolvable chapter13 +
    main.cpp citations)
  - 1 in `particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl`
  - 2 from A.3 fixtures
    (`tools/integrity/tests/test_cat1_bare_path.py`)

The remaining 647 categorized findings are the A.3-leftover sweep
workload (deferred to A.3 commit 4, not absorbed here).

### C.4 Post-commit gate (this commit only, no broad sweep)

The gate will remain at ~652 hard-fail until A.3 commit 4 lands. P1.8
itself does not change the hard-fail count; it changes only which
findings the *sweep CLI* default-includes for annotation. The
hard-fail bucket is unchanged by P1.8 because:

- Findings classified as `other-cat1` on LIVE-SOURCE paths (6 in
  current state) were never auto-suppressed by the sweep, and they
  remain unsuppressed — same as before P1.8, but now by intentional
  policy rather than accidental over-sweep.
- All other findings (646) remain in whatever state A.3 left them
  (mostly unsuppressed, awaiting A.3 commit 4's sweep companion).

## § D. Cross-references

- Probe § F.1, § F.2 — current `grandfather_sweep.py` content and
  CLI surface.
- Probe § F.3 — `grandfather.py` dataclass + `apply_annotations`
  layout used to anchor the edit points.
- Probe § F.7 — confirms `apply_annotations` is the public surface;
  the CLI is a thin wrapper.
- Probe § G.2 — confirms `project-state.md` belongs in
  `SWEEPABLE_EXACT_PATHS`.
- Probe § G.5 — the four outstanding live-source hard-fails at
  probe time; now 6 with the 2 new A.3 fixtures.
- Triage § B — bucket definitions (AUDIT-DOC / TOOLKIT-DOC / LIVE-SOURCE).
- Retro § 4.4, § 6.2, § 7.2 — Convention B (sweep companion) +
  post-retro landing § D.3 (deferred `project-state-snapshot` classifier).
- A.3 commit-1, -2, -3 audits — context for the 652-hard-fail baseline
  this commit inherited.

## § E. Banked observations

- **A.3 introduced an unsweepable live-source bare-path bucket.**
  `other-cat1-bare-path` on live-source paths (43 findings) is
  structurally analogous to `other-cat1` on live-source paths (the
  bucket P1.8 protects), but with `cat1.bare-path` rather than
  `cat1.intra-repo`. P1.8 v2 should extend the live-source protection
  to cover this; either by including `other-cat1-bare-path` in the
  filter test, or by generalizing to "any cat1 finding falling to a
  catch-all on a LIVE-SOURCE path". **Bank as v1.3 candidate.**
- **Sweep groups multiple same-line findings into stacked annotations.**
  Observed during the rejected broader-scope sweep attempt: 10
  identical `# integrity-allow: cat1.annotation-form` annotations
  stacked sequentially above a single target line in `grandfather.py`
  (separate findings on the same line, each adding its own annotation
  comment). Functionally correct (the suppressor only needs one to
  silence the line) but ugly. The render path in
  `render_annotation_line()` does collapse same-category findings on
  the same line; the duplication happens when the sweep iterates
  lines in descending order and inserts the annotation block above
  each target, accumulating duplicates if the file already has
  matching annotations. **Bank as v1.3 sweep-quality candidate.**
- **No pause-and-surface fired during commit 1 execution.** The
  spec § 3.1 structural pre-conditions held (`Classification`
  unchanged, `classify()` signature compatible). The composition
  with A.3 was clean despite the volume of new findings.

## § F. Next commit

Commit 2 (P1.5) — register the three `cat3.d3q19-*` checks. SHA
back-fill at commit 5.
