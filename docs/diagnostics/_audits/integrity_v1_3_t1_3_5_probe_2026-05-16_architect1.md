---
title: "Integrity v1.3 Batch-1 Part-A Pre-Spec Probe"
date: 2026-05-16
author: architect1-via-claude-code
status: probe
scope: read-only
sibling-docs:
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/retro/integrity-toolkit-v1.2-bolt-ons.md
  - docs/retro/integrity-toolkit-v1.3-candidates.md
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md
---

# Integrity v1.3 Batch-1 Part-A Pre-Spec Probe

Pre-spec read-only probe grounding T1.3 (catalog auto-refresh), T1.4
(probe template conventions doc), and T1.5 (TOML → JSON convergence
for cat3 expected values) per the v1.3 candidates roadmap (docs/
retro/integrity-toolkit-v1.3-candidates.md). All line numbers in this
report come from `grep -n` or `cat -n` runs in this probe; where a
prior doc cites a different line number for the same span, the
discrepancy is recorded.

---

## Section A — Gate state at probe time

### A.1 — HEAD

FACT — `git rev-parse HEAD`:
```
e079c7b980f06b993f57f05145dab755ed40ca13
```

This is one commit ahead of the v1.3 candidates roadmap commit
`a0427d9` (per § A.3 below); the additional commit is A.2 commit 1
(`e079c7b`).

### A.2 — Strict-mode run

FACT — `python3 -m integrity --mode strict --no-audit-log` summary
line:
```
integrity: 5 pass, 0 soft-warn, 53 hard-fail, 1213 suppressed
```

107 output lines total; exit code is non-zero (the strict gate is red
by design, as it has been since v1 landing). Drift from roadmap-
expected ~44 hard-fails / ~1213 suppressed: hard-fail count rose
+9 (44 → 53); suppressed count is unchanged at 1213.

INFERENCE: the +9 hard-fail drift is consistent with two
contributing sources visible since the `a0427d9` roadmap landing —
(a) A.2 commit-1 (`e079c7b`) registers the new
`cat2.public-symbol-used-toolkit` check, which surfaced new
findings into the toolkit-own surface; and (b) ongoing audit-doc
churn (the strict output includes 8 hard-fails citing
`docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md`
itself — a doc that landed after the v1.2 landing audits closed).
This does NOT affect T1.3/T1.4/T1.5 scope; the drift is informational.

First 10 unsuppressed-finding stanzas (verbatim, lines 2-21 of
strict output):

```
HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md:1453
  integrity/common/stack_paths.py:16: path 'integrity/common/stack_paths.py' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
HARD_FAIL: cat1.intra-repo at docs/phase12_lattice_boltzmann.md:203
  chapter13/cpu/LBM.cpp:97: path 'chapter13/cpu/LBM.cpp' does not resolve under …
HARD_FAIL: cat1.intra-repo at docs/phase12_lattice_boltzmann.md:351
  chapter13/cpu/LBM.cpp:97: path 'chapter13/cpu/LBM.cpp' does not resolve under …
HARD_FAIL: cat1.intra-repo at particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl:7
  SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75: path … does not resolve …
HARD_FAIL: cat1.intra-repo at tools/integrity/tests/test_cat1_bare_path.py:73
  common/unique_widget.hpp:10: path … does not resolve …
HARD_FAIL: cat1.intra-repo at tools/integrity/tests/test_cat1_bare_path.py:121
  common/widget.hpp:1: path … does not resolve …
HARD_FAIL: cat1.bare-path at CHANGELOG.md:154
  context.hpp:78: bare intra-repo citation; resolved basename suggests 'common/common-cpp/include/gpusims/vk/context.hpp:78'
HARD_FAIL: cat1.bare-path at CHANGELOG.md:154
  context.cpp:116: bare intra-repo citation; resolved basename suggests 'common/common-cpp/src/vk/context.cpp:116'
HARD_FAIL: cat1.bare-path at CHANGELOG.md:154
  context.cpp:202: bare intra-repo citation; resolved basename suggests 'common/common-cpp/src/vk/context.cpp:202'
HARD_FAIL: cat1.bare-path at common/common-py/examples/hello/hello/main.py:31
  kernel_impl.py:631: bare basename matches no git-tracked file
```

(Suppressed-finding stanzas omitted; `--mode strict` only renders
hard-fails after P1.6, so none appear here.)

### A.3 — Recent commits + staged work

FACT — `git log --oneline -10`:
```
e079c7b feat(integrity): cat2.public-symbol-used-toolkit module + fixtures + tests (v1.2 A.2 commit 1)
a0427d9 docs(retro): integrity toolkit v1.3 candidates roadmap
67474b9 docs(retro): integrity toolkit v1.2 bolt-ons retrospective
1a49d33 docs(audits): back-fill SHA cross-references in v1.2 A.3 landing audits
908f619 feat(integrity): grandfather sweep companion for cat1.bare-path (v1.2 A.3 commit 4)
5c3e1ef docs(integrity): SHA back-fill for v1.2 commits 1-4 (v1.2 commit 5)
5cdd20f docs(integrity): P1.7 fix stub_label_stale.py module-docstring drift (v1.2 commit 4)
71559ce fix(integrity): P1.6 human-renderer omits suppressed stanzas (v1.2 commit 3)
119e353 feat(integrity): P1.5 register cat3.d3q19-* checks (v1.2 commit 2)
5fe5e6b feat(integrity): P1.8 grandfather-sweep live-source protection (v1.2 commit 1)
```

Commits since `a0427d9`: one — `e079c7b` (A.2 commit 1, new file
`cat2_contracts/checks/public_symbol_used_toolkit.py` + fixtures +
tests). No commits touching `grandfather.py`,
`grandfather_sweep.py`, or `grandfather-catalog.md` have landed
since the roadmap.

FACT — `git status tools/integrity/`:
```
M tools/integrity/integrity/grandfather.py
```

FACT — `git diff HEAD --stat`:
```
 tools/integrity/integrity/grandfather.py | 30 ++++++++++++++++++++++++++++--
 1 file changed, 28 insertions(+), 2 deletions(-)
```

INFERENCE: the modified `grandfather.py` is A.2 in-progress work
(commit 2 — classifier rule for `toolkit-own-unused` + per-category
sweep-allow). This matches the A.2 probe § L.4 / § L.5
recommendations. The diff is staged-to-working-tree only; it is not
committed. Coordination implications appear in § E.

---

## Section B — T1.3 surface: grandfather-catalog + report mechanics

### B.1 — Catalog total LOC + heading shape

FACT — `wc -l tools/integrity/docs/grandfather-catalog.md`: 383
LOC. All H3 category headings (per `grep -n '^### '`):

| Line | Heading |
|---|---|
| 28  | `` ### `audit-citation` (597) `` |
| 41  | `` ### `live-shader-1810` (3) `` |
| 60  | `` ### `audit-doc-1810` (15) `` |
| 73  | `` ### `spec-grammar-example` (17) `` |
| 88  | `` ### `toolkit-own-source` (22) `` |
| 101 | `` ### `retro-grammar-example` (2) `` |
| 114 | `` ### `audit-report-grammar-example` (19) `` |
| 126 | `` ### `other-cat1` (66) `` |
| 136 | `` ### `cat2-stack-d-unused` (17) `` |
| 169 | `` ### `cat2-stack-c-unused` (111) `` |
| 204 | `` ### `cat2-stack-b-unused` (73) `` |
| 233 | `` ### `cat2-stub-label-stale` (2) `` |
| 256 | `` ### `toolkit-own-unused` (?) `` |
| 296 | `` ### `audit-bare-path` (635) `` |
| 313 | `` ### `retro-bare-path` (11) `` |
| 324 | `` ### `toolkit-doc-bare-path` (7) `` |
| 335 | `` ### `deferred-upstream-bare-path` (5) `` |
| 350 | `` ### `other-cat1-bare-path` (0 swept; 44 live-source skipped) `` |

FACT — the canonical heading regex is approximately
`` ^### \`(?P<cat>[a-z0-9-]+)\` \((?P<count>.+?)\)$ `` but two
non-conforming variants exist on disk and the T1.3 parser MUST
handle them:

- Line 256: `` ### `toolkit-own-unused` (?) `` — the parenthetical
  is a literal `?` (placeholder for the A.2 sweep run). INFERENCE:
  T1.3's parser must accept this as a "needs refresh" sentinel,
  not error out.
- Line 350: `` ### `other-cat1-bare-path` (0 swept; 44 live-source
  skipped) `` — the parenthetical contains free prose (not just an
  integer). INFERENCE: T1.3 must EITHER preserve the prose
  verbatim (skipping numeric refresh on this category) OR mechanize
  the two-number shape; the simpler-and-safer choice is preserve-
  verbatim because the live-source-skipped count is a separate
  classifier dimension not present in `--grandfather-report`'s
  output.

Headings 28–350 are interleaved with a `## Updating counts` block
(lines 13–25) that explicitly describes the manual refresh process
the T1.3 script is intended to replace, and the line:
> Auto-refresh from the history file is a v1.2 candidate.
(line 24)

INFERENCE: this prose line will need to be updated in T1.3's commit
to reflect that auto-refresh now exists; it is the only doc-prose
edit T1.3 needs beyond the heading parentheticals.

### B.2 — `--grandfather-report` output

FACT — full output of
`python3 -m integrity --grandfather-report --no-history-append`
(19 lines, exit 0):

```
grandfather report @ e079c7b (2026-05-16T01:19:02.346387+00:00)
summary: {'pass': 5, 'soft_warn': 0, 'hard_fail': 53, 'suppressed': 1213}
per-category counts:
                      audit-bare-path: 729
                  cat2-stack-c-unused: 110
                       audit-citation: 99
                  cat2-stack-b-unused: 73
         audit-report-grammar-example: 46
                           other-cat1: 36
                   toolkit-own-source: 25
                      retro-bare-path: 18
                 spec-grammar-example: 18
                       audit-doc-1810: 17
                  cat2-stack-d-unused: 17
                retro-grammar-example: 8
                toolkit-doc-bare-path: 7
          deferred-upstream-bare-path: 5
                     live-shader-1810: 3
                cat2-stub-label-stale: 2
```

FACT — emission shape: three header lines (commit/timestamp,
summary dict, label) followed by per-category lines of the form
`{cat:>35s}: {n}` (right-justified 35-char field, colon-space,
integer count). The list is sorted by count descending. Categories
with zero findings are NOT emitted at all (`toolkit-own-unused`
is absent from the report despite having a heading on disk — see
B.6 for the cross-reference). The output format is human-text,
not JSON or tabular.

FACT — category names in the report use kebab-case identifiers
that match the names quoted in the catalog headings 1:1 for every
category that appears in the report (audit-bare-path,
cat2-stack-c-unused, audit-citation, etc.). No name mismatches.

### B.3 — Source of `--grandfather-report`

FACT — `grep -rn 'grandfather.report\|grandfather_report\|--grandfather-report'
tools/integrity/integrity/`:
```
snapshot.py:1:"""State-snapshot and grandfather-report emitters (v1.1 A.7, A.8).
snapshot.py:10:- `emit_grandfather_report(root, stdout, append_history=True)` -- Human-
snapshot.py:191:def emit_grandfather_report(
snapshot.py:200:    out.write(f"grandfather report @ {state['commit']} ({state['timestamp']})\n")
runner.py:39:    grandfather_report: bool
runner.py:61:    parser.add_argument("--grandfather-report", action="store_true",
runner.py:64:                        help="With --grandfather-report, skip the history-file append (read-only mode)")
runner.py:76:        grandfather_report=ns.grandfather_report,
runner.py:174:    if args.grandfather_report:
runner.py:175:    from integrity.snapshot import emit_grandfather_report
runner.py:176:        emit_grandfather_report(
```

The CLI flag is wired in `runner.py` and dispatches to
`emit_grandfather_report(root, out, append_history=True)` defined
at `tools/integrity/integrity/snapshot.py:191-207` (verbatim
function captured during probe). The function writes to a
caller-supplied `IO[str]`, so it's also importable for
in-process use.

INFERENCE: T1.3's script SHOULD subprocess
`python3 -m integrity --grandfather-report --no-history-append`
rather than import `emit_grandfather_report` directly. Two
reasons:

- (a) The CLI surface is the toolkit's stable contract for this
  data; the internal `snapshot.py` function carries an
  `append_history` side effect on its default-True argument and
  doesn't return a parseable structure (it just `out.write`s
  human text). A subprocess call honoring the documented contract
  is more durable than an import that fights the function shape.
- (b) The subprocess boundary keeps T1.3's script independent of
  any future toolkit refactors that move `snapshot.py`. The
  text-output format is stable (per B.2's exact field widths) and
  parseable with one regex.

A direct-import path would require either refactoring
`emit_grandfather_report` to return a `dict[str, int]` (adds A.2-
adjacent change to snapshot.py), or duplicating the
`_collect_state()` traversal in T1.3's script (DRY violation).
Both are higher-blast-radius than subprocess. Recommendation:
subprocess.

### B.4 — Existing scripts inventory

FACT — `ls -la tools/integrity/scripts/`:
```
-rw-rw-r-- grandfather_sweep.py
-rw-rw-r-- __init__.py (empty)
```

FACT — `wc -l`: `grandfather_sweep.py` = 61 LOC; `__init__.py` = 0.

Verbatim dump of `grandfather_sweep.py` (61 LOC, confirms shape;
prior bolt-ons probe § F.1 captured an earlier 31-LOC version,
A.3-era growth to 61 LOC mainly due to `--force-sweep-category`
addition):

```python
#!/usr/bin/env python3
"""Grandfather-sweep CLI entry. Logic lives in integrity.grandfather."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from integrity.common.repo import find_repo_root
from integrity.grandfather import apply_annotations


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Grandfather-sweep integrity findings")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--repo-root", type=Path, default=None)
    parser.add_argument("--sweep-live-source", action="store_true", help=…)
    parser.add_argument("--force-sweep-category", action="append", default=[], …)
    ns = parser.parse_args(argv)

    root = ns.repo_root if ns.repo_root else find_repo_root()
    files, anns, counts, live_source_skipped = apply_annotations(
        root, ns.dry_run,
        sweep_live_source=ns.sweep_live_source,
        force_sweep_categories=frozenset(ns.force_sweep_category),
    )
    …
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
```

Structural shape T1.3 should mirror:

- Shebang `#!/usr/bin/env python3` + `"""…"""` one-line module
  docstring.
- `from __future__ import annotations`.
- `argparse.ArgumentParser` with `--dry-run` and `--repo-root`
  flags.
- `def main(argv: list[str]) -> int:` entry.
- `find_repo_root()` for default-root resolution.
- `if __name__ == "__main__": sys.exit(main(sys.argv[1:]))`.

T1.3-specific differences expected: no shared `--sweep-live-source`
flag (out of scope); add `--catalog-path` (defaults to
`tools/integrity/docs/grandfather-catalog.md`) and maybe
`--report-path` (defaults to running the subprocess). Same
`--dry-run` semantics (print-but-don't-write).

### B.5 — Existing catalog parsing logic

FACT — searched via
`grep -rn 'grandfather.catalog\|grandfather_catalog\|### \`' tools/integrity/`.
All matches were either:

<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
- Inline `integrity-allow:` annotation strings with
  `grandfather-catalog <category>` reasons (~50 hits across
  toolkit / tests / docs).
- Heading lines inside `grandfather-catalog.md` itself.
- README + PKG-INFO mentions of the doc by name.

No code under `tools/integrity/integrity/` or
`tools/integrity/scripts/` currently parses the
heading-with-count format. INFERENCE: T1.3 is introducing
first-time mechanical parsing of this format. There is no prior
parser to mirror or extend; the catalog has been hand-edited
since v1.1 A.8 landed. (Confirms the v1.1 batch-1 retro § 5.5
diagnosis: "Counts are manual-refresh in v1.1; auto-refresh
banked as v1.2 candidate.")

### B.6 — Catalog-vs-report drift table

FACT — cross-reference of B.1's catalog headings against B.2's
`--grandfather-report` per-category counts at HEAD `e079c7b`:

| Category | Catalog heading | `--grandfather-report` | Drift |
|---|---|---|---|
| audit-citation | 597 | 99 | -498 |
| live-shader-1810 | 3 | 3 | 0 |
| audit-doc-1810 | 15 | 17 | +2 |
| spec-grammar-example | 17 | 18 | +1 |
| toolkit-own-source | 22 | 25 | +3 |
| retro-grammar-example | 2 | 8 | +6 |
| audit-report-grammar-example | 19 | 46 | +27 |
| other-cat1 | 66 | 36 | -30 |
| cat2-stack-d-unused | 17 | 17 | 0 |
| cat2-stack-c-unused | 111 | 110 | -1 |
| cat2-stack-b-unused | 73 | 73 | 0 |
| cat2-stub-label-stale | 2 | 2 | 0 |
| toolkit-own-unused | ? | (absent) | n/a |
| audit-bare-path | 635 | 729 | +94 |
| retro-bare-path | 11 | 18 | +7 |
| toolkit-doc-bare-path | 7 | 7 | 0 |
| deferred-upstream-bare-path | 5 | 5 | 0 |
| other-cat1-bare-path | 0 swept; 44 live-source skipped | (absent) | n/a |

FACT — 11 of the 18 catalog headings have non-zero drift; 5 are
zero-drift; 2 are special-cased (`?` placeholder /
two-number-prose form). The largest drifts are:
`audit-citation` (-498, due to A.3's `cat1.bare-path` re-
categorization — the A.2 probe § K.1 predicted -517 from a
different probe-time snapshot; the difference is post-probe
audit-doc churn), `audit-bare-path` (+94, audit-doc growth since
A.3 landed), and `audit-report-grammar-example` (+27).

This table is the empirical case for T1.3's value. Drift this size
on a single batch cycle confirms the v1.1 batch-1 retro § 5.5
diagnosis. The presence of `toolkit-own-unused (?)` is the
adjacency case for the T1.3 spec — A.2 is landing this category
without a count, and T1.3's first useful run will be replacing
that `?` with the post-A.2 sweep number.

### B.7 — INFERENCE: T1.3 design sketch — surfaced decisions

The spec will need to settle these design choices. Recommendations
below, citing FACT sections.

**(1) Heading-with-no-report handling.** A category heading exists
on disk but `--grandfather-report` emits no entry for it (the
`toolkit-own-unused` case, B.6 row 13). Two semantically valid
behaviors:

- (a) Update count to `(0)`.
- (b) Leave the heading parenthetical unchanged (preserve the
  human-authored `?` placeholder until A.2's sweep populates it).

Recommendation: **(b) for v1.3 initial scope** — non-numeric
parentheticals are passed through verbatim. A.2's sweep commit will
write a numeric value when it lands; T1.3's script will refresh
it on subsequent runs. This avoids T1.3 racing A.2 over the
`toolkit-own-unused` count.

**(2) Report-with-no-heading handling.** A category in
`--grandfather-report` has no corresponding catalog heading. Per
B.6, this does not occur at HEAD `e079c7b` (every emitted category
matches a heading). But it could occur after a future check
landing that introduces a new classifier rule without companion
catalog edit (the v1.1 batch-1 retro § 6.2 "grandfather-sweep
companion" convention gap). Two behaviors:

- (a) Error out and refuse to write (forces the author to either
  add a heading or remove the classifier rule).
- (b) Append a stub heading at end-of-categories with the count
  and a `TODO: write category description` marker.
- (c) Silently skip (worst — hides the gap).

Recommendation: **(a)** — error out with a clear message naming
the missing categories. The catalog is human-authored prose
(category descriptions explain WHY each is grandfathered) and a
mechanical stub would be wrong-shaped. Forcing the author to
write the heading manually preserves the convention. This is the
T2.1 enforcement intent at script level.

**(3) Two-number-prose handling (`other-cat1-bare-path`).** Per
B.1, line 350's heading is `(0 swept; 44 live-source skipped)`.
The `--grandfather-report` output emits no `other-cat1-bare-path`
entry (it's in the `audit-bare-path` / `retro-bare-path` /
`toolkit-doc-bare-path` / `deferred-upstream-bare-path` /
`other-cat1` rules per B.2; the "other-cat1-bare-path" name lives
only in the catalog as a documentation grouping). Recommendation:
**pass through verbatim** — same rationale as (1)(b); the
two-number form encodes information the report does not carry.

**(4) Idempotency.** Re-running the script with no underlying
changes MUST produce zero diff. Required test case; covered by
the v1.1 P1.8 / A.3 sweep-companion idempotency precedent.

**(5) Output mode.** Recommendation: **in-place edit**, matching
the v1.3 candidates roadmap T1.3 design decision ("Stay in-place
to preserve audit trail of catalog edits in git history",
roadmap line 228). Add `--dry-run` to print the proposed diff
without writing.

**(6) Refresh-prose update.** The catalog's `## Updating counts`
block (lines 13–25) currently describes the manual refresh
process and ends with "Auto-refresh from the history file is a
v1.2 candidate." T1.3's spec should update this block in the
same commit that lands the script. INFERENCE: this is a
one-paragraph rewrite, not a script-mechanical edit.

---

## Section C — T1.4 surface: docs inventory + canonical examples

### C.1 — `tools/integrity/docs/` inventory

FACT — `ls -la tools/integrity/docs/`:
```
drwxrwxr-x algebraic/
-rw-rw-r-- grandfather-catalog.md (17974 bytes)
-rw-rw-r-- ground-truth-sources.md (3926 bytes)
```

Only two top-level `.md` files; no existing probe-template /
process-doc home.

### C.2 — First 20 lines of each `.md`

FACT — `grandfather-catalog.md` (lines 1-20):

```
# Integrity Toolkit — Grandfather Catalog (v1)

This document records the pre-v1 findings that were grandfathered into the
toolkit's strict-mode gate when commit 4a landed. Categories below map to
the rules in `tools/integrity/scripts/grandfather_sweep.py` (and the
classifier in `tools/integrity/integrity/grandfather.py`).

The toolkit will continue to gate CI strictly on any NEW findings introduced
after this commit. Grandfathered findings are suppressed via inline
<!-- integrity-allow: cat1.annotation-form; … -->
`integrity-allow:` annotations per spec § 3.2.

## Updating counts
…
```

FACT — `ground-truth-sources.md` (lines 1-20):

```
# Ground-truth sources for the integrity toolkit

Per spec Appendix A. Adding a source requires:

1. Vendoring the upstream under `references/<UpstreamName>/`, OR …
2. Pinning the anchor (version + SHA)
3. Updating the TOML block below
4. Updating the relevant check(s) to consume the new source

## v1 registry

The block below is parsed by `cat1_citations/upstream_anchor.py`. Everything
outside the fenced TOML block is prose for humans.
…
```

INFERENCE — neither file is a probe-template doc. Both have
narrow scopes ("grandfather catalog" / "upstream-source registry");
neither has a "conventions" or "process" section that T1.4 could
extend cleanly.

### C.3 — `docs/diagnostics/` inventory

FACT — `find docs/diagnostics -maxdepth 2 -type f -name '*.md'`
returns only files under `docs/diagnostics/_audits/`; no
`docs/diagnostics/probe-template.md` or similar exists.

FACT — `grep -rl 'probe-template\|probe template' docs/ tools/`:
returns only references inside retro / audit reports
(`v1.1-batch1.md`, `v1.1-batch1-addendum.md`,
`v1.3-candidates.md`, four landing audits) — every one of these is
a citation TO the convention, not a doc embodying it.

INFERENCE: there is no current canonical probe-template home. T1.4
creates the first.

### C.4 — INFERENCE: T1.4 landing location

Three options, with FACT-grounded reasoning:

- (a) New file at
  `tools/integrity/docs/probe-template-conventions.md`. Aligns
  with where the existing convention citations point (the v1.3
  roadmap T1.4 text says "Update `docs/diagnostics/probe-template.md`
  (or equivalent canonical probe-template doc; verify location at
  spec time)"). The toolkit-docs subtree is where the toolkit's
  process artifacts live; this fits.
- (b) Extend an existing doc. Rejected per C.2 — both existing
  toolkit docs have non-conforming scopes and no natural section
  to host the convention.
- (c) New file under `docs/diagnostics/` (e.g.,
  `docs/diagnostics/probe-template.md`). This matches the literal
  roadmap wording. But `docs/diagnostics/` currently contains ONLY
  the `_audits/` subdirectory (per C.3); landing a non-audit
  process doc there fragments the directory.

Recommendation: **(a) — new file
`tools/integrity/docs/probe-template-conventions.md`**.
Rationale: the convention is integrity-toolkit-specific (it grew
out of v1.1 batch-1 pause-and-surface moments during toolkit
landings); colocating with the other toolkit process docs (i.e.,
`grandfather-catalog.md`, `ground-truth-sources.md`) keeps the
toolkit's process surface in one tree. The roadmap's
"docs/diagnostics/probe-template.md" wording is a verify-at-spec-
time hint, not a binding location.

### C.5 — Canonical examples for Convention C
(path-resolution enumeration)

**Example 1 — Convention C violation (v1.1 commit 1
pause-and-surface #1).** From
`docs/diagnostics/_audits/integrity_v1_1_commit1_landing_2026-05-15.md`
§ E.1 (lines 167–194):

> The spec's Decision 2 asserted a 1:1 mirror between
> `include/<sub>/<base>.hpp` and `src/<sub>/<base>.cpp`. Synced repo
> state has `include/gpusims/...` stripping the `gpusims/` namespace
> component in the `src` tree — i.e.,
> `include/gpusims/alembic_writer.hpp` maps to
> `src/alembic_writer.cpp`, not `src/gpusims/alembic_writer.cpp`.
> The check's first execution attempt resolved zero impl paths and
> would have missed both canonical target cases.

Root-cause framing from v1.1 batch-1 retro § 3.1 (lines 146–151):

> The pre-spec apispec probe enumerated verbatim source listings of
> relevant modules but did not enumerate any header→impl path pairs.
> The spec drafter (architect-1) filled in the convention from
> prior assumption rather than from probe data. That assumption was
> wrong.

This is the worked example of Convention C's failure mode. T1.4's
doc should embed both fragments.

**Example 2 — Convention C followed (v1.2 A.3 probe).** From
`docs/diagnostics/_audits/integrity_v1_2_a3_probe_2026-05-15_architect1.md`
§ A.3 (lines 238–334): the probe verbatim-dumps
`cat1_citations/resolver.py` (103 LOC), including the verbatim
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
two-step resolution function (`resolve()` at resolver.py:42-65)
and the INFERENCE block explicitly naming the false-positive class
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
the v1 intra-repo check accidentally accommodates (resolver.py:42-65
will succeed on bare basenames whenever a sibling file matches).
That probe-time enumeration is precisely what Convention C asks for:
path-resolution rules dumped verbatim from the source under
inspection, before the spec drafter writes new convention text.

**Example 3 — Convention C followed (v1.2 A.2 probe).** From
`docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md`
§ C.1 (lines 278–319): the probe verbatim-enumerates every
`from integrity.X import Y` edge in the toolkit's internal
cross-module graph — which is the toolkit's analog of "path-pair
enumeration": every consumer-of-X edge is dumped, so the spec
drafter cannot fabricate an import path that doesn't exist. (The
A.2 probe's analog of the v1.1 commit-1 fabrication would have
been asserting that `cat2_contracts/checks/foo.py` exists when it
doesn't; C.1's enumeration directly forecloses that class.)

### C.6 — Canonical examples for Convention D
(call-site enumeration)

**Example 1 — Convention D violation (v1.1 commit 2
pause-and-surface #2).** From v1.1 batch-1 retro § 3.2 (lines
192–204):

> The apispec probe verbatim-dumped the annotation parser and the
> annotation check but did not enumerate which other cat1 checks
> scan markdown content. The spec drafter scoped Decision 6 from
> local knowledge of one module instead of from probe data on all
> relevant modules.

The mitigation banked (retro § 3.2 lines 198–204) is exactly
Convention D's text: "Enumerate all check modules under
`tools/integrity/integrity/cat<N>_*/checks/` whose `run()`
function reads markdown files or scans content line-by-line. For
each, dump the relevant input-handling block verbatim."

**Example 2 — Convention D followed (v1.2 bolt-ons probe
P1.6 surface).** From
`docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md`
§ D.1 (lines 797–808): the probe identifies every `emit_output`
and `_emit_human_summary` declaration plus all callers in
`runner.py`. § D.2 then dumps both functions verbatim (lines
813–865), proving the asymmetry between the `github` branch
(suppressed filter) and `human` branch (no filter). This is
exactly what Convention D asks for: enumerate every call site of
the function the spec proposes to modify, before writing the fix.

Line-number FACT (per probe-run verification): in current disk
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
state, `_emit_human_summary` is defined at `runner.py:154` and
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`emit_output` at `runner.py:116`. The bolt-ons probe § D.1 / D.2
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
cited `_emit_human_summary` at `runner.py:148`, which was correct
at probe time but stale post-`71559ce` (P1.6 fix added 6 lines).
The v1.2 A.2 probe § C.3 line 387 records the updated `:154`.
INFERENCE for T1.4: the worked example is still load-bearing as a
demonstration of "enumerate before modifying", but the embedded
line numbers should be tagged "as of probe SHA" or omitted.

**Example 3 — Convention D followed (v1.2 A.2 probe).** From the
v1.2 A.2 probe § C.1 (lines 278–319) — same dump as C.5 Example 3
— enumerates every internal import edge. § C.2 (lines 321–367)
enumerates external consumers (scripts + tests + docs); 20 test
files, 1 script, 4 doc files (with INFERENCE distinguishing real
consumers from fenced-code-listing pseudo-consumers). The dual
enumeration is what saved the A.2 spec from fabricating a
nonexistent caller in its design discussion.

### C.7 — INFERENCE: T1.4 scope estimate

Roadmap estimate: ~30 LOC of markdown.

This probe's empirical estimate: the bare Convention C + D rule
text is ~30 LOC; embedding the six worked examples per C.5 + C.6
(three per convention) adds ~70 LOC. Total ~100 LOC including
worked examples.

Recommendation: **embed all six worked examples**. The T1.4 doc
without worked examples is just two banked-convention sentences;
the worked examples are the evidence that converts the convention
from "yet another aspirational doc" to "here is the failure mode
this prevents." Per v1.1 batch-1 retro § 7.2 — the five banked
conventions are "candidates for inclusion in a permanent
CONVENTIONS doc rather than re-derivation in every retro." T1.4
is that home for two of the five (C + D).

---

## Section D — T1.5 surface: cubic_kernel + expected_values

### D.1 — `expected_values.toml` verbatim

FACT — `wc -l`: 45 LOC. Verbatim (full file):

```toml
# Expected values for cat3.cubic-kernel.
# Generated by tools/integrity/integrity/cat3_numerical/generate_expected.py
# Source: Bender-Koschier 2015 / SPlisHSPlasH 2.16.1 SPHKernels.h:43-85
# Anchor SHA: 6bff55a6eaf14083d34650f22a268ce156b62b54

[tolerance]
atol = 1e-5
rtol = 1e-5

[[test_points]]
q = 0.0
h = 1.0
expected_W = 2.54647908947033
expected_gradW_magnitude = 0

[[test_points]]
q = 0.1
h = 1.0
expected_W = 2.40896921863893
expected_gradW_magnitude = 2.59740867125973

[[test_points]]
q = 0.25
h = 1.0
expected_W = 1.8302818455568
expected_gradW_magnitude = 4.77464829275686

[[test_points]]
q = 0.5
h = 1.0
expected_W = 0.636619772367581
expected_gradW_magnitude = 3.81971863420549

[[test_points]]
q = 0.75
h = 1.0
expected_W = 0.0795774715459477
expected_gradW_magnitude = 0.954929658551372

[[test_points]]
q = 1.0
h = 1.0
expected_W = 0
expected_gradW_magnitude = 0
```

FACT — TOML structure: top-of-file comments (anchor provenance,
generator pointer) + `[tolerance]` table (2 keys) + 6
`[[test_points]]` array-of-tables (4 keys each: q, h, expected_W,
expected_gradW_magnitude). Total 6 test points spanning q ∈ {0.0,
0.1, 0.25, 0.5, 0.75, 1.0} with h=1.0.

### D.2 — `cubic_kernel.py` (harness)

FACT — `wc -l`: 111 LOC. Key spans:

```python
1   """Cubic SPH kernel numerical correctness per spec § 8.
2
3   Reads expected values from expected_values.toml, runs the Stack C
4   driver binary at build/tools/integrity/drivers/integrity_cat3_stack_c/,
…
19  import tomllib
…
27  EXPECTED_VALUES_RELATIVE = Path(
28      "tools/integrity/integrity/cat3_numerical/expected_values.toml"
29  )

48  def load_expected_values(repo_root: Path) -> tuple[list[TestPoint], dict]:
49      """Parse expected_values.toml. Returns (test_points, tolerance_dict)."""
50      path = repo_root / EXPECTED_VALUES_RELATIVE
51      if not path.is_file():
52          return [], {}
53
54      data = tomllib.loads(path.read_text(encoding="utf-8"))
55      tolerance = data.get("tolerance", {"atol": 1e-5, "rtol": 1e-5})
56      points: list[TestPoint] = []
57      for tp in data.get("test_points", []):
58          points.append(TestPoint(
59              q=float(tp["q"]),
60              h=float(tp["h"]),
61              expected_W=float(tp["expected_W"]),
62              expected_gradW_magnitude=float(tp["expected_gradW_magnitude"]),
63          ))
64      return points, tolerance
```

FACT — `grep` for `tomllib.load|tomllib.loads|toml.load` across the
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
toolkit returns only the single call at `cubic_kernel.py:54`. No
other consumer of TOML inside `cat3_numerical/`.

FACT — return shape: `tuple[list[TestPoint], dict]` where
`TestPoint` is a frozen dataclass with (q, h, expected_W,
expected_gradW_magnitude). The tolerance dict has `atol`/`rtol`
float keys.

### D.3 — `checks/cubic_kernel.py` (consumer)

FACT — `wc -l`: 99 LOC. Import path from harness:

```python
from integrity.cat3_numerical.cubic_kernel import (
    EXPECTED_VALUES_RELATIVE,
    find_driver,
    load_expected_values,
    run_driver,
    within_tolerance,
)
```

The check calls `load_expected_values(repo_root)` once at
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`checks/cubic_kernel.py:37` and references the constant
`EXPECTED_VALUES_RELATIVE` (used for `Finding.file=` field
attribution).

INFERENCE: T1.5 changes the harness implementation
(`load_expected_values` swaps `tomllib.loads` for `json.loads`)
but keeps the consumer-facing signature identical
(`tuple[list[TestPoint], dict]`). The check module needs zero
changes if the constant name is preserved (or trivial rename if
`EXPECTED_VALUES_RELATIVE` becomes `EXPECTED_VALUES_JSON_RELATIVE`).

### D.4 — `generate_expected.py` (producer)

FACT — `wc -l`: 118 LOC. Key shape:

- Module docstring identifies this as the writer of
  `expected_values.toml`.
- Two analytic functions `cubic_W(q, h)` and
  `cubic_gradW_magnitude(q, h, inject_factor_of_6=False)`.
- `TEST_POINTS_Q = [0.0, 0.1, 0.25, 0.5, 0.75, 1.0]` + `H = 1.0`.
- `main()` builds a `lines: list[str]` of literal TOML text and
  writes to `OUTPUT_PATH = SCRIPT_DIR / "expected_values.toml"`.

FACT — `git log --oneline -- expected_values.toml`:
```
f576b5e feat(integrity): Cat 3 cubic-kernel numerical correctness (commit 8 — final)
```

INFERENCE: the file has one commit in history (the original
Phase 11.5 commit-8 landing). Per the module docstring and the
header comment in D.1 ("Generated by …generate_expected.py"),
this is machine-generated. Manual edits would defeat the
provenance trail. T1.5 must update BOTH the generator and the
checked-in data file in the same commit.

### D.5 — `d3q19_equilibrium.expected.json` (JSON precedent)

FACT — top-level keys (per probe inspection of lines 1–4 and
lines 5–144): `schema_version` (int), `source` (string —
file path of the producer), `derivation` (string — file path
of the algebraic doc), `velocity_set` (list of 19 [x,y,z]
triples), `weights` (list of 19 floats), `opposite_index`
(list of 19 ints), and `test_points` (list of 4 objects with
`name`, `rho`, `u`, `feq`, `sums` fields).

FACT — file is 290 LOC, indent=2 JSON, all keys are flat at the
top level (no nested objects beyond the natural array nesting).

This is the format precedent T1.5 will converge `cubic_kernel`
toward.

### D.6 — `test_cat3_cubic_kernel.py`

FACT — `wc -l`: 109 LOC. Test functions that depend on the
expected-values file format (verbatim signatures + key
assertions):

```python
def test_load_expected_values_real_file() -> None:
    """The committed expected_values.toml should parse cleanly with 6 points."""
    repo_root = Path(__file__).resolve().parents[3]
    points, tolerance = load_expected_values(repo_root)
    assert len(points) == 6, f"expected 6 test points, got {len(points)}"
    assert tolerance == {"atol": 1e-5, "rtol": 1e-5}

def test_load_expected_values_specific_q() -> None:
    """W(q=0.0, h=1.0) should be 8/pi."""
    …

def test_load_expected_values_q_at_support_boundary() -> None:
    """W(q=1.0, h=1.0) should be 0 (kernel support cutoff)."""
    …
```

FACT — three tests load expected values via the harness
(`load_expected_values(repo_root)`), not directly. INFERENCE:
these tests need no signature change after T1.5; the harness
hides the format swap. Docstrings reference "expected_values.toml"
literally and should be updated to "expected_values.json" for
accuracy (single edit per test, three tests).

Two additional tests
(`test_driver_builds_and_runs`, `test_check_graceful_degrade_without_driver`)
do not touch the expected-values file directly; they cmake-build
the driver or call the check with a tmp_path root.

### D.7 — All `expected_values.toml` references

FACT — `grep -rn 'expected_values.toml\|expected_values\\.toml'
tools/integrity/`:

```
integrity/cat3_numerical/cubic_kernel.py:3:     docstring
integrity/cat3_numerical/cubic_kernel.py:28:    EXPECTED_VALUES_RELATIVE constant
integrity/cat3_numerical/cubic_kernel.py:49:    docstring (load_expected_values)
integrity/cat3_numerical/generate_expected.py:2:  docstring (module-level)
integrity/cat3_numerical/generate_expected.py:16: docstring ("writes to expected_values.toml")
integrity/cat3_numerical/generate_expected.py:28: OUTPUT_PATH = SCRIPT_DIR / "expected_values.toml"
tests/test_cat3_cubic_kernel.py:36:               docstring ("expected_values.toml")
```

7 textual references across 3 files. Each is a touch point for
T1.5: rename string + (if a constant) verify nothing else binds
to the old name.

### D.8 — INFERENCE: T1.5 file-operation sequence

Three options:

- (a) `git mv expected_values.toml expected_values.json` then
  edit content. Pro: git tracks the move. Con: content type
  changes, so the move-tracking is partly noise; future readers
  see "rename + rewrite" in `git log --follow` and have to read
  diff to understand.
- (b) `git rm expected_values.toml` + `git add expected_values.json`.
  Pro: explicit separation; clean reviewer experience. Con:
  history detection breaks; `git log --follow` won't trace.
- (c) Land both files temporarily (deprecation window). Overkill
  for an internal refactor with single consumer; rejected.

Recommendation: **(b)** — clean delete + add. The TOML file has
one historical commit and one consumer; preserving rename
detection has near-zero practical value. The new JSON's shape
should mirror the d3q19 schema (D.5):

```json
{
  "schema_version": 1,
  "source": "tools/integrity/integrity/cat3_numerical/generate_expected.py",
  "derivation": "SPlisHSPlasH 2.16.1 SPHKernels.h:43-85",
  "anchor_sha": "6bff55a6eaf14083d34650f22a268ce156b62b54",
  "tolerance": {"atol": 1e-5, "rtol": 1e-5},
  "test_points": [
    {"q": 0.0, "h": 1.0, "expected_W": 2.54647908947033,
     "expected_gradW_magnitude": 0},
    …
  ]
}
```

The four TOML header comments map cleanly to top-level keys
(`source`, `derivation`, `anchor_sha`); the `[tolerance]` table
becomes a nested object; the `[[test_points]]` array-of-tables
becomes a JSON list of objects. JSON has no comment syntax, so
the explanatory comments either move to the generator's docstring
(already partially present) or to a `_comment` field on the
top-level dict.

INFERENCE: there is no information loss in the format swap; the
TOML's structure is shallower than the d3q19 JSON's, so the
conversion is a one-pass rewrite.

### D.9 — `ground-truth-sources.md` (candidate doc home)

FACT — `wc -l`: 79 LOC. Structure:

- H1 title (line 1).
- Intro paragraph (lines 3–9) — "Adding a source requires: …"
  numbered list.
- `## v1 registry` (line 11) + intro sentence (lines 13–14) +
  fenced TOML block (lines 16–44) — the parsed registry.
- `## Notes on v1 registry contents` (line 46) + per-source
  prose notes (lines 48–68).
- `## Not yet registered (intentional)` (line 70) + per-deferred-
  source prose (lines 72–79).

The scope statement (lines 3–9) is "Adding a source requires …" —
specifically about upstream-source registry, not a general cat3
doc. The fenced TOML block has a load-bearing parser
(`cat1_citations/upstream_anchor.py`, per line 13). Adding an
"expected-values format" convention section to this doc would
expand its scope from "upstream-source registry" to "cat3
conventions catch-all", which fights the existing structure.

### D.10 — INFERENCE: T1.5 convention-doc location

Three options:

- (a) Appendix section in `ground-truth-sources.md`.
- (b) New file `tools/integrity/docs/cat3-conventions.md`.
- (c) Header `_comment` field in the new JSON file itself.

Per the architect-1 pre-probe preference, (a) was the first
candidate. After D.9: rejecting (a) on scope-mismatch grounds.

Recommendation: **(b) — new file
`tools/integrity/docs/cat3-conventions.md`** (~30 LOC). Rationale:

- The convention is cat3-specific (numerical-correctness checks
  reading machine-generated expected-data files).
- Mirroring `tools/integrity/docs/grandfather-catalog.md`'s shape
  (toolkit-internal process doc colocated with the toolkit code)
  is a natural fit.
- A standalone file is also the natural home for the future
  cat3-related additions surfaced by the v1.3 roadmap (T1.2,
  T2.x) — i.e., this is an evolution path, not a one-off doc.

(c) is the worst option: a JSON `_comment` field is
self-documenting but invisible to anyone who doesn't open the
file. The convention needs visibility from the cat3 docs index.

---

## Section E — A.2 staged work coordination

### E.1 — A.2 disk state

FACT — per § A.3, A.2 has landed commit 1 (`e079c7b`, new file
`cat2_contracts/checks/public_symbol_used_toolkit.py` +
fixtures + tests). A.2 commit 2 is staged in working tree:
`tools/integrity/integrity/grandfather.py` modified +28 / -2.

FACT — `git diff HEAD --stat tools/integrity/scripts/`:
no changes. The A.2 probe § E.1 anticipated changes to
`scripts/grandfather_sweep.py` (--force-sweep-category flag), but
that flag is already present in the on-disk file as of HEAD
`e079c7b` (verified in B.4 verbatim dump). INFERENCE: the
`--force-sweep-category` flag landed earlier than the A.2 probe
predicted — likely during A.3 commit 4 (`908f619`) — and is no
longer an A.2 deliverable. A.2's remaining work is grandfather.py
(staged) + catalog edits + sweep-run.

FACT — `git diff HEAD tools/integrity/docs/grandfather-catalog.md`:
no changes. The new `toolkit-own-unused` heading is already
present in the catalog on disk (line 256 of B.1) — but with the
literal `?` count, since A.2's sweep hasn't run yet. INFERENCE:
the catalog heading landed pre-A.2 (likely during the v1.2 A.3
landing per the heading's stack-config consolidation reference)
and is waiting for A.2 to populate the count.

### E.2 — Format-compatibility check for T1.3

FACT — the `toolkit-own-unused` heading at catalog line 256 uses
the standard `` ### `<category>` (<count>) `` form (with `?`
as the count token). T1.3's parser will see this as a non-numeric
parenthetical and (per § B.7 (1)) pass through verbatim until
A.2's sweep replaces `?` with a number. Once that happens, T1.3's
parser will treat `toolkit-own-unused` as a normal category.

No format incompatibility introduced by A.2.

INFERENCE: A.2 and T1.3 can land in either order. If A.2 lands
first (the natural sequence: A.2 commit 2 staged → commit 3
sweep → commit 4 catalog refresh), T1.3 inherits a catalog with
a numeric `toolkit-own-unused` count and refreshes it on first
run. If T1.3 lands first, A.2's catalog edit replaces the `?`
with a number manually; T1.3's next run produces zero diff.

The only spec-time coordination requirement: T1.3's spec MUST
explicitly document the "preserve non-numeric parenthetical
verbatim" rule (§ B.7 (1)) so that A.2's interim `?` state is
not corrupted.

---

## Section F — Banked observations

**F.1 — Other expected-values files in cat3.** Per `find tools/integrity/integrity/cat3_numerical -name 'expected*'`:
```
tools/integrity/integrity/cat3_numerical/expected_values.toml
```
Only one. The d3q19 file is `d3q19_equilibrium.expected.json`
(not matched by the `expected*` glob because the prefix is
`d3q19_…`). No other expected-data files exist. T1.5's scope is
exactly the two files it claims.

**F.2 — `--grandfather-report` format stability.** FACT — there is
no documented stability guarantee for the human-text output of
`--grandfather-report`. The format is implemented in
`tools/integrity/integrity/snapshot.py:200-204` with a hardcoded
35-char field width. INFERENCE for T1.3: the parser should be
lax-but-explicit — accept any width (split on `: ` after the
last `:`) and validate that the value parses as a non-negative
integer; reject any line that doesn't. If snapshot.py ever moves
to a structured output (JSON / sqlite), T1.3 should switch to
that format. Banked candidate for the T1.3 spec: also expose a
`--grandfather-report-json` flag on the CLI (out of scope for
T1.3 unless the spec author bundles).

**F.3 — Other docs with stale counts in headings.** FACT — searched
`grep -rn '^### .*([0-9]' tools/integrity/docs/`. The only doc
with numeric-count headings is `grandfather-catalog.md`. No other
toolkit-internal doc has this pattern. INFERENCE: T1.3's script
should target `grandfather-catalog.md` specifically (per the
v1.3 candidates roadmap T1.3 explicit scope); there is no
generalization opportunity to bank for T1.3.

**F.4 — Drift table prior-doc cross-reference.** The v1.2 A.2
probe § K.1 table (lines 1426–1444) computed the same drift table
against probe-time SHA. Differences vs § B.6 (this probe):
audit-citation 80 → 99 (+19), audit-doc-1810 16 → 17 (+1),
other-cat1 35 → 36 (+1), retro-bare-path 11 → 18 (+7),
audit-report-grammar-example 42 → 46 (+4),
audit-bare-path 635 (unchanged in A.2 probe, since A.3 had just
swept) → 729 (this probe). INFERENCE: the audit-doc bulge is the
dominant drift source between probes — audit reports under
`docs/diagnostics/_audits/` continue to grow with bare-basename
citations. This affirms the v1.3 candidates roadmap T1.3 value
case (drift across a single batch cycle is real and measurable).

**F.5 — T1.4 worked examples already exist.** Per C.5 + C.6: all
six worked examples (three per convention) are already published
in audit/retro reports. T1.4's authoring cost is excerpt-and-link,
not write-from-scratch. INFERENCE: this lowers the T1.4 spec-
drafting time below the roadmap's 30-LOC estimate (drafting cost
~15 min; doc grows to ~100 LOC including embedded excerpts).

**F.6 — `_emit_human_summary` line-number drift (prior-probe
cross-check).** Per C.6 Example 2: the bolt-ons probe § D.1
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
records `_emit_human_summary` at runner.py:148; current disk
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
puts it at runner.py:154 (6-line drift attributable to commit
`71559ce` / P1.6 fix). The v1.2 A.2 probe § C.3 recorded the
updated :154. This is the line-number-drift class the probe-
prompt preamble warned about — current FACT supersedes prior
probes.

**F.7 — `--force-sweep-category` flag drift.** Per § E.1: the
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
flag is already on disk at `grandfather_sweep.py:27-40` (verified
in B.4), but the v1.2 A.2 probe § E.1 expected it as A.2 commit
2 deliverable. The flag landed earlier than the A.2 probe
predicted — INFERENCE: during A.3 commit 4 (`908f619`). This is a
prior-doc-says-X / current-disk-says-Y reconciliation: current
disk is authoritative.

---

## Section G — Closing

Read-only constraint honored. No files modified by this probe; no
commits made by this probe; no pushes.

FACT — `git rev-parse HEAD` at probe end:
`df213120354ccb75b297f7b7cc63dd06785955f9`. HEAD has moved one
commit since § A.1 (`e079c7b`). The new commit landed during
probe execution (concurrent author work, not this probe):

```
df21312 feat(integrity): classify+catalog+apply_annotations refactor (v1.2 A.2 commit 2)
```

This is A.2 commit 2 — the work that was staged-in-working-tree
at § A.3 / § E.1 has now landed. INFERENCE: § A.3's staged-diff
description (`grandfather.py` +28 / -2) is now committed; § E.1's
"A.2 commit 2 staged" observation is now "A.2 commit 2 landed".
The new working-tree change at probe end is:

```
M tools/integrity/integrity/cat2_contracts/checks/__init__.py
```

INFERENCE: A.2 commit 3 is now in progress (registering the new
check in the checks __init__). This does not invalidate any
§ B / § C / § D finding, but the spec author should re-read § E
against post-`df21312` state when drafting — A.2's catalog edit
(replacing `?` with a numeric count for `toolkit-own-unused`) may
land before the T1.3 spec lands.

FACT — `git show df21312 --stat` shows the commit touched FIVE
files, not just `grandfather.py`:

```
docs/diagnostics/_audits/integrity_v1_2_a2_commit2_landing_2026-05-15.md | +194
tools/integrity/docs/grandfather-catalog.md                              |  +40
tools/integrity/integrity/grandfather.py                                 |  +28 / -2
tools/integrity/scripts/grandfather_sweep.py                             |  +16 / -2
tools/integrity/tests/fixtures/conftest.py                               |  +15
```

CORRECTION TO THIS PROBE'S § B.1 AND § B.6: my catalog reads
(383 LOC; `toolkit-own-unused (?)` heading at line 256) reflect
post-`df21312` state, not the § A.1 anchor state. The `git
status` at § A.3 was captured before `df21312` landed (when only
`grandfather.py` was staged-in-working-tree); the catalog reads
later in the probe ran against post-commit disk because
`df21312` landed concurrently. The `toolkit-own-unused (?)`
heading at catalog line 256 was added by `df21312` (verified via
`git diff e079c7b df21312 -- tools/integrity/docs/grandfather-catalog.md`
showing 40 insertions adding precisely that section, no
deletions). At the § A.1 anchor SHA `e079c7b`, the catalog was
343 LOC and the `toolkit-own-unused` heading did not exist.

CORRECTION TO THIS PROBE'S § B.4: the verbatim
`grandfather_sweep.py` dump (61 LOC) reflects post-`df21312`
state. At § A.1 anchor `e079c7b`, the file was 43 LOC and lacked
the `--force-sweep-category` flag. INFERENCE: § F.7's "the flag
landed earlier, during A.3 commit 4 (`908f619`)" is WRONG — the
flag was added by `df21312` during this probe's execution, not
by A.3. § A.3 captured working-tree `M grandfather.py` only;
`grandfather_sweep.py` was clean at probe start, then mutated by
`df21312` during probe.

CORRECTION TO THIS PROBE'S § E.1: the assertion "no changes to
`tools/integrity/scripts/`" and "no changes to
`grandfather-catalog.md`" was true at § A.3 capture-time but
false at probe-end — both files were modified by `df21312`. The
A.2 commit 2 scope was broader than § E.1's reconstruction.

T1.3 / T1.4 / T1.5 surface findings (§ B / § C / § D) remain
USABLE for spec drafting, but the spec author should:

- Treat § B.1 + § B.6 + § B.4 as anchored at SHA `df213120` (the
  effective probe-end state for catalog + scripts), not at `e079c7b`.
- Re-run `python3 -m integrity --grandfather-report --no-history-append`
  before drafting if any further commits land — drift is active.
- Discount § F.7 as a probe-time mis-attribution.

§ C.5 / C.6 (T1.4 worked examples) and § D.1–D.10 (T1.5 cat3
surface) are unaffected by `df21312` — that commit did not touch
`cat3_numerical/`, the audit/retro reports cited in C.5/C.6, or
`tools/integrity/docs/{ground-truth-sources.md,algebraic/}`.

End of probe.
