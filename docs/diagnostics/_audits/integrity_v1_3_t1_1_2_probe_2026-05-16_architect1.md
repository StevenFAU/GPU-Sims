---
title: "Integrity v1.3 Batch-1 Part-B Pre-Spec Probe"
date: 2026-05-16
author: architect1-via-claude-code
status: probe
scope: read-only
sibling-docs:
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/retro/integrity-toolkit-v1.2-bolt-ons.md
  - docs/retro/integrity-toolkit-v1.3-candidates.md
  - docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_t1_3_5_probe_2026-05-16_architect1.md
---

# Integrity v1.3 Batch-1 Part-B Pre-Spec Probe

## § 0 — Purpose and scope

Read-only probe to ground a v1.3 batch-1 *part-B* execution spec covering
the two T1 items left after the parallel session's part-A (T1.3 + T1.5
+ T1.4 + SHA back-fill) landed:

- **T1.1** — three new permanent classifier categories
  (`toolkit-doc-snapshot`, `project-state-snapshot`, `retro-doc-snapshot`)
  + matching grandfather-catalog sections.
- **T1.2** — Convention H structural follow-through: a
  `Classification`-level fallthrough discriminator that replaces the
  literal `category in {"other-cat1", "other-cat1-bare-path"}` matches
  in `apply_annotations()`.

Probe is read-only. Every claim is tagged **FACT** (directly observed in
this probe) or **INFERENCE** (derived). Length target 600-900 lines per
the v1.2 bolt-ons retro § 5 item 5 spec-oversizing concern; T1.1 + T1.2
are the smallest joint scope in the v1.3 inventory (~140 + ~30 LOC est.).

Probe-start HEAD captured at top of § A.1. Probe-end HEAD captured at
§ K closing.

## § A — Current gate state and baseline (post-A.2, post v1.3 part-A)

### A.1 — HEAD and probe-start SHA

```
$ git rev-parse HEAD
1f7785fd6567599f948c8eee68a7641032d3ff4a
```

**FACT.** Probe-start SHA is `1f7785f`. This is the SHA back-fill commit
for v1.3 batch-1 part-A commits 1–3 (per `git log --oneline -15` in § A.4).

### A.2 — Strict-mode baseline

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 5 pass, 0 soft-warn, 44 hard-fail, 1262 suppressed
```

Exit code: 0 (FACT — captured `EXIT:0` despite hard-fail count; the
exit-code semantics treat the per-finding hard-fails as informational
under `--mode strict --no-audit-log`).

First 5 of 15 captured unsuppressed-finding stanzas (FACT, verbatim;
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
remaining 10 stanzas are bare-path findings on `CHANGELOG.md:154`,
`common/common-py/examples/hello/hello/main.py:31`, and six
`continuous-ca/lenia-fft/docs/load-bearing-decisions.md:236-283` lines
— all `cat1.bare-path` of the form documented in the v1.2 A.3 probe;
none touch T1.1 or T1.2 surfaces):

```
HARD_FAIL: cat1.intra-repo at docs/phase12_lattice_boltzmann.md:203
  chapter13/cpu/LBM.cpp:97: path 'chapter13/cpu/LBM.cpp' does not resolve under .../docs or repo root
HARD_FAIL: cat1.intra-repo at docs/phase12_lattice_boltzmann.md:351
  chapter13/cpu/LBM.cpp:97: path 'chapter13/cpu/LBM.cpp' does not resolve under .../docs or repo root
HARD_FAIL: cat1.intra-repo at particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl:7
  SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75: path does not resolve
HARD_FAIL: cat1.intra-repo at tools/integrity/tests/test_cat1_bare_path.py:73
  common/unique_widget.hpp:10: path does not resolve (test fixture)
HARD_FAIL: cat1.intra-repo at tools/integrity/tests/test_cat1_bare_path.py:121
  common/widget.hpp:1: path does not resolve (test fixture)
```

**FACT.** Hard-fail count is **44** (not 45 from A.2's Addendum A).

**INFERENCE.** Drift since A.2 Addendum A baseline is **-1 hard-fail**.
The likely cause is one of the v1.3 part-A landings (T1.3 / T1.4 / T1.5
or the SHA back-fill) inadvertently resolved one hard-fail; this should
be confirmed in the spec's pre-flight check but does not affect T1.1 or
T1.2 scope. None of part-A's three substantive commits touched live
shader / sim code where the unsuppressed pool lives.

### A.3 — Grandfather-report baseline (per-category counts)

```
$ python3 -m integrity --grandfather-report --no-history-append
grandfather report @ 1f7785f (2026-05-16T11:59:29.514161+00:00)
summary: {'pass': 5, 'soft_warn': 0, 'hard_fail': 44, 'suppressed': 1262}
per-category counts:
                      audit-bare-path: 747
                  cat2-stack-c-unused: 110
                       audit-citation: 101
                  cat2-stack-b-unused: 73
         audit-report-grammar-example: 51
                           other-cat1: 36
                   toolkit-own-source: 25
                   toolkit-own-unused: 24
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

**FACT.** `other-cat1` count is **36**. This is the bucket T1.1 will
reshape (some subset will reclassify to the three new named categories).

**FACT.** Exit code 0. Total per-category sum = 1262 = suppressed total
in summary; report counts every annotated finding (the report's
per-category counts come from `snapshot.py:_extract_category` matching
reason-string text against `_KNOWN_CATEGORIES`; see § F.3).

**INFERENCE.** Compared to v1.2 bolt-ons probe / A.2 probe expected
post-A.2 baseline, the suppressed-total `1262` matches the live-source
44 + suppressed 1262 = 1306 total findings which is consistent with no
drift in the *suppressed* pool since A.2 landing.

### A.4 — Recent commit history

```
$ git log --oneline -15
1f7785f docs(integrity): SHA back-fill for v1.3 batch-1 part-A commits 1-3 (v1.3 commit 4)
9e3afa9 docs(integrity): T1.4 probe template conventions doc (v1.3 commit 3)
72a2d26 refactor(integrity): T1.5 cat3 expected-values TOML -> JSON (v1.3 commit 2)
65a7685 feat(integrity): T1.3 catalog auto-refresh script (v1.3 commit 1)
9f527dc docs(audits): A.2 commit-4 audit Addendum A -- post-landing +1 gate drift
4673c41 docs(integrity): SHA back-fill for v1.2 A.2 commits 1-4 (v1.2 A.2 commit 5)
9c8979a feat(integrity): grandfather sweep for toolkit-own-unused (v1.2 A.2 commit 4)
926aa30 feat(integrity): register cat2.public-symbol-used-toolkit (v1.2 A.2 commit 3)
df21312 feat(integrity): classify+catalog+apply_annotations refactor (v1.2 A.2 commit 2)
e079c7b feat(integrity): cat2.public-symbol-used-toolkit module + fixtures + tests (v1.2 A.2 commit 1)
a0427d9 docs(retro): integrity toolkit v1.3 candidates roadmap
67474b9 docs(retro): integrity toolkit v1.2 bolt-ons retrospective
1a49d33 docs(audits): back-fill SHA cross-references in v1.2 A.3 landing audits
908f619 feat(integrity): grandfather sweep companion for cat1.bare-path (v1.2 A.3 commit 4)
5c3e1ef docs(integrity): SHA back-fill for v1.2 commits 1-4 (v1.2 commit 5)
```

**FACT.** v1.3 batch-1 part-A is fully landed across `65a7685` → `1f7785f`
(four commits: T1.3, T1.5, T1.4, SHA back-fill). The part-A spec's
ordering matches the v1.3 candidates roadmap § 9.1 row sequence with
T1.2 + T1.1 deferred to part-B.

**INFERENCE.** Coordination axis for part-B: no concurrent edits to
`grandfather.py` since A.2 commit 2 (`df21312`) and A.2 commit 4
(`9c8979a`). T1.1 and T1.2 own `grandfather.py` for part-B with no
contention.

## § B — Classification dataclass current shape

### B.1 — Verbatim head + structurally-important interior of `grandfather.py`

**FACT.** File totals **472 LOC** (per `wc -l`).

Verbatim head (lines 1-36) — module docstring + the two dataclasses
that T1.2 touches:

```python:tools/integrity/integrity/grandfather.py
     1	"""Grandfather-sweep logic for the integrity toolkit v1.
     2	
     3	Classifies HARD_FAIL findings into one of seven pre-v1 categories and
     4	# integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a
     5	generates inline `integrity-allow:` annotations on the cited source
     6	lines. See `tools/integrity/docs/grandfather-catalog.md` for the
     7	per-category rationale.
     8	
     9	Imported by `tools/integrity/scripts/grandfather_sweep.py` (the CLI
    10	wrapper) and by the unit tests under `tools/integrity/tests/`.
    11	"""
    12	
    13	from __future__ import annotations
    14	
    15	import json
    16	import re
    17	import subprocess
    18	from dataclasses import dataclass
    19	from pathlib import Path
    20	from typing import Iterable
    21	
    22	
    23	@dataclass(frozen=True)
    24	class Finding:
    25	    check_id: str
    26	    file: str
    27	    line: int
    28	    message: str
    29	
    30	
    31	@dataclass(frozen=True)
    32	# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (...); n/a
    33	class Classification:
    34	    category: str
    35	    reason: str
    36	    issue_ref: str
```

[...elided lines 37-83 — P1.8 module docstring, `SWEEPABLE_PATH_PREFIXES`
(line 53-57), `SWEEPABLE_EXACT_PATHS` (line 60-64), `is_live_source_path()`
(line 67-82). Re-quoted in § H...]

Verbatim of the `classify()` function's two fallthrough returns and the
two T1.2-relevant `cat1.bare-path` predicate-stacks (lines 189-218):

```python:tools/integrity/integrity/grandfather.py
   189	    if cid == "cat1.bare-path" and (
   190	        f == "docs/integrity-toolkit-spec.md"
   191	        or f.startswith("tools/integrity/docs/")
   192	        or f == "tools/integrity/README.md"
   193	    ):
   194	        return Classification(
   195	            category="toolkit-doc-bare-path",
   196	            reason="toolkit-doc bare-path citation pre-v1.2 (...)",
   197	            issue_ref="n/a",
   198	        )
   199	
   200	    if cid == "cat1.bare-path" and "LeniaNDK.py" in msg:
   201	        return Classification(
   202	            category="deferred-upstream-bare-path",
   203	            reason="deferred-upstream-bare-path citation (...)",
   204	            issue_ref="n/a",
   205	        )
   206	
   207	    if cid == "cat1.bare-path":
   208	        return Classification(
   209	            category="other-cat1-bare-path",
   210	            reason="bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog)",
   211	            issue_ref="n/a",
   212	        )
   213	
   214	    return Classification(
   215	        category="other-cat1",
   216	        reason="grandfathered-pre-v1 (see grandfather-catalog other-cat1)",
   217	        issue_ref="n/a",
   218	    )
```

[...elided lines 85-188 — the remainder of `classify()`: cat2 rules
(91-124), `audit-citation` rule (126-131), `1.8.10` rule pair (133-147),
`cat1.annotation-form` rule (149-173), `audit-bare-path` rule (175-180),
`retro-bare-path` rule (182-187). Each is a `if cid == "..."` + `return
Classification(category="...", reason="...", issue_ref="n/a")`. Full rule
inventory captured as a table in § D.2 below...]

[...elided lines 220-369 — `comment_form_for`, fence helpers,
`render_annotation_line`, `collect_findings`, `group_findings_by_target`.
None touch `Classification` directly or carry T1.1/T1.2 scope...]

`apply_annotations()` (lines 371-472) is the second T1.2 touch site;
verbatim re-quote captured in § C.2 / § H.1 below.

### B.2 — `Classification` dataclass shape

**FACT** (lines 31-36 above):

```python
@dataclass(frozen=True)
class Classification:
    category: str
    reason: str
    issue_ref: str
```

Programmatic confirmation (FACT):

```
$ python3 -c "from integrity.grandfather import Classification; ..."
Classification fields: ['category', 'reason', 'issue_ref']
Classification frozen: True
Classification methods (non-dunder): []
```

- **FACT** — three string fields, `frozen=True`, no `__post_init__`,
  no methods, no properties.
- **FACT** — `Classification` has 18 construction sites in
  `grandfather.py` (lines 92, 99, 106, 113, 120, 127, 138, 143, 151,
  157, 163, 169, 176, 183, 194, 201, 208, 214); zero construction sites
  elsewhere in the toolkit. Confirmed by `grep 'Classification('
  tools/integrity/integrity/ tools/integrity/scripts/ tools/integrity/tests/`.

### B.3 — Construction-site inventory

**FACT.** All 18 sites set exactly the three documented fields
(`category=...`, `reason=...`, `issue_ref="n/a"`). Every existing site
uses positional-less keyword args. There are no `Classification(...)`
calls in tests or scripts.

**INFERENCE.** This is favorable for T1.2: a frozen dataclass with three
fields and 18 keyword-call sites is mechanically refactorable to any of
the design options in § B.4. Adding a fourth field with a default is a
one-line dataclass edit + zero call-site touches. Adding a property is
one method definition + zero call-site touches. Adding a module-level
helper is zero touches to `Classification` itself.

### B.4 — T1.2 design-option triage

T1.2 has multiple design options. The prompt enumerates three; the v1.3
candidates roadmap § 4 T1.2 enumerates three with one different shape.
Union of the four distinct options:

**Option (a) — Field with default.** Add `is_fallthrough: bool = False`
to the dataclass; flip True at lines 207-212 and 214-218. *Pro:*
explicit at construction site. *Con:* touches dataclass shape; field
is per-category metadata, not per-classification.

**Option (b) — Computed property.** `@property is_fallthrough(self):
return self.category.startswith("other-cat")`. *Pro:* zero touches to
18 construction sites. *Con:* implicit naming convention; any
`other-cat*` name auto-becomes fallthrough; any non-`other-cat*`
fallthrough slips through.

**Option (c) — Factory classmethods.** `Classification.fallthrough(...)`
vs. `Classification(...)`. *Pro:* discriminates at construction site
without adding a field. *Con:* two construction patterns; not queryable
on the resulting instance.

**Option (d) — Module-level set + helper** (the roadmap's option (c)):

```python
FALLTHROUGH_CATEGORIES: frozenset[str] = frozenset({
    "other-cat1",
    "other-cat1-bare-path",
})

def is_fallthrough_category(category: str) -> bool:
    return category in FALLTHROUGH_CATEGORIES
```

*Pro:* mirrors `SWEEPABLE_PATH_PREFIXES` / `SWEEPABLE_EXACT_PATHS`
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
(grandfather.py:53-64); single source of truth; one-line edit to add a
future fallthrough; no coupling to category-name spelling. *Con:* set
lives separately from construction site; new fallthrough requires
remembering to register.

**INFERENCE — architect-1-via-claude-code recommendation: option (d)**.

Rationale:

1. Shape-consistent with the file's existing P1.8 idiom
   (`SWEEPABLE_PATH_PREFIXES` is exactly the same pattern: a frozenset
   of identifiers used by a predicate function consulted from
   `apply_annotations()`). Reusing the idiom is the highest-leverage
   way to fold T1.2 into existing code aesthetics, which serves
   Convention H's intent of "shape-consistent" structural fixes.
2. Backward-compatible. No `Classification` field/method changes;
   construction sites untouched; tests under `test_grandfather_sweep.py`
   need only one new test for the helper itself.
3. Refactor cost in `apply_annotations()` is mechanical:
   `if cat in ("other-cat1", "other-cat1-bare-path")` (line 406)
   becomes `if is_fallthrough_category(cat)`. One-line change.
4. Forward-compatible with option (a). If a future batch wants
   construction-site discrimination, option (a) can be layered atop
   (d) without churn (the set becomes the auto-populated derivation).
   The reverse is not true: option (a) first, option (d) second forces
   a `Classification` shape rollback.
5. Roadmap § 4 T1.2 already states "Architect-1 weak preference: (c),
   for shape-consistency with P1.8" — and the roadmap's (c) is this
   option (d). The prompt's enumeration was missing the
   set-and-helper option that the roadmap originally banked.

**Pause-and-surface for spec.** The four options should be enumerated
in the spec's design-decision section so architect-2 (or architect-1 at
spec time) can confirm or override. The recommendation here is
weakly-held; (a) is a defensible second choice if the spec author wants
construction-time discrimination.

## § C — Fallthrough-bucket identification sites (T1.2 refactor targets)

### C.1 — Literal string match grep

```
$ grep -rn 'other-cat1\|other-cat2\|other-cat3' \
    tools/integrity/integrity/ tools/integrity/scripts/ tools/integrity/tests/
```

**FACT** (filtered to code call-sites; doc-only hits in tests and
docstrings noted separately):

| File:line | Site type | T1.2 refactor target? |
|---|---|---|
| `tools/integrity/integrity/grandfather.py:209` | `category="other-cat1-bare-path"` (construction) | NO (this is the source of the name) |
| `tools/integrity/integrity/grandfather.py:215` | `category="other-cat1"` (construction) | NO (this is the source of the name) |
| `tools/integrity/integrity/grandfather.py:406` | `if cat in ("other-cat1", "other-cat1-bare-path") and is_live_source_path(...)` | **YES** — sole filter call site |
| `tools/integrity/integrity/snapshot.py:46-47` | `"other-cat1-bare-path", "other-cat1"` in `_KNOWN_CATEGORIES` tuple | NO (reason-string extraction; not the live-source filter) |
| `tools/integrity/scripts/grandfather_sweep.py:22, 54` | help-text + print string | NO (user-facing text) |
| `tools/integrity/tests/test_grandfather_sweep.py:69, 218, 234, etc.` | test assertions on classifier output | NO (testing category-name correctness; T1.2 should leave these intact) |

**INFERENCE.** T1.2's *code* refactor is exactly one line:
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`grandfather.py:406` is the only call site that gates behavior on the
fallthrough property. The other matches are either the source-of-truth
strings (the construction sites that *produce* the names) or
user-facing/reporting strings that should keep the literal names for
operator readability.

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
Wider context of line 406 (FACT — from `grandfather.py:402-413` per
§ B.1 dump):

```python
    live_source_skipped = 0
    if not sweep_live_source:
        kept: list[Finding] = []
        for f in findings:
            cat = classify(f).category
            if cat in ("other-cat1", "other-cat1-bare-path") and is_live_source_path(f.file):
                if cat in force_sweep_categories:
                    kept.append(f)
                    continue
                live_source_skipped += 1
                continue
            kept.append(f)
        findings = kept
```

### C.2 — `apply_annotations()` signature and filter hotspot

Signature (line 371-376) + the P1.8 filter block (lines 393-413) —
the entire T1.2 code-side scope:

```python:tools/integrity/integrity/grandfather.py
   371	def apply_annotations(
   372	    repo_root: Path,
   373	    dry_run: bool,
   374	    sweep_live_source: bool = False,
   375	    force_sweep_categories: frozenset[str] = frozenset(),
   376	) -> tuple[int, int, dict[str, int], int]:
   ...
   393	    # P1.8 -- protect LIVE-SOURCE other-cat1 findings from sweep by default.
   394	    # Named classifier categories (cat2-stack-*-unused, live-shader-1810, etc.)
   395	    # remain sweepable on live-source paths by design -- only the heterogeneous
   396	    # other-cat1 fallthrough bucket is dangerous to auto-annotate on live code.
   397	    # v1.2 A.2 Decision 4 extends the bypass surface: force_sweep_categories
   398	    # opts specific named categories (e.g., toolkit-own-unused) into the sweep
   399	    # even when their findings sit on LIVE-SOURCE paths, without disabling the
   400	    # default protection for all other categories.
   401	    live_source_skipped = 0
   402	    if not sweep_live_source:
   403	        kept: list[Finding] = []
   404	        for f in findings:
   405	            cat = classify(f).category
   406	            if cat in ("other-cat1", "other-cat1-bare-path") and is_live_source_path(f.file):
   407	                if cat in force_sweep_categories:
   408	                    kept.append(f)
   409	                    continue
   410	                live_source_skipped += 1
   411	                continue
   412	            kept.append(f)
   413	        findings = kept
```

The literal-match at line 406 is the structural target of T1.2 per
roadmap § 4 T1.2: "Refactor P1.8's filter logic in `apply_annotations()`
to use the property instead of `category == 'other-cat1'`."

### C.3 — A.3 commit 4 extension to LIVE-SOURCE filter logic

The v1.2 bolt-ons retro § 3.2 canonical text claims A.3 commit 4
extended the LIVE-SOURCE filter to handle `other-cat1-bare-path`.
Verifying:

**FACT.** A.3 commit 4 is `908f619` (per § A.4 row). The literal in
`grandfather.py:406` is currently `cat in ("other-cat1", "other-cat1-bare-path")`
— both names appear, confirming the extension landed. A.2 commit 2
(`df21312`) is the "classify+catalog+apply_annotations refactor" commit
that landed the `force_sweep_categories` parameter alongside; the two
extensions compose at line 406 cleanly.

**INFERENCE.** Convention H ("fallthrough discriminator") was banked
in the v1.2 bolt-ons retro § 4.2 precisely because the A.3 commit 4
extension at line 406 was a *literal-list append*: the maintainer had
to remember to extend the tuple by hand. A.2 commit 2's parameter add
(`force_sweep_categories`) widens this surface further. T1.2's refactor
turns the literal-list at line 406 into a single set-membership check,
which collapses any future fallthrough-bucket addition to a one-line
edit at the set definition rather than a search-and-extend across
filter code.

### C.4 — Forward-compatible fallthrough enumeration

**FACT.** Current fallthrough categories per `classify()`:

- `other-cat1` (line 215; the `cat1.*` last-resort fallthrough)
- `other-cat1-bare-path` (line 209; the `cat1.bare-path` fallthrough)

**FACT.** No `other-cat2` or `other-cat3` category currently exists.
The cat2 classifier rules (lines 91-124) each produce a named category;
there is no fall-through `return` for unmatched `cat2.*` findings.

**INFERENCE.** Under T1.2 option (d), `FALLTHROUGH_CATEGORIES` starts
with the two current entries. If a future batch introduces a `cat2.*`
fallthrough (e.g., `other-cat2` for `cat2.public-symbol-used*` not
matching any stack-specific rule), the set gains a third entry; the
filter logic at line 406 is untouched. If a future batch introduces a
`cat3.*` fallthrough (less likely; cat3 checks all carry explicit
named categories), same story.

The set is **not** auto-populated from a naming-convention check
(that would be option b). Authors of new fallthrough categories must
register them. This is desirable: it forces explicit thought about
whether a new bucket *is* fallthrough-shaped, rather than auto-treating
any `other-*`-named category as one.

## § D — `classify()` function current state

### D.1 — Signature and rule block

`classify()` signature (FACT, line 85):

```python
def classify(finding: Finding) -> Classification:
    """Classify a finding into a grandfather category. First match wins."""
    f = finding.file
    msg = finding.message
    cid = finding.check_id
    ...
```

Rule body spans lines 91-218 (123 LOC). First-match-wins semantics
documented in the docstring.

### D.2 — Rule ordering (current state)

**FACT.** Rule order in `classify()`:

| # | Lines | check_id | Path predicate | Category |
|---|---|---|---|---|
| 1 | 91-96 | `cat2.public-symbol-used` | (none) | `cat2-stack-d-unused` |
| 2 | 98-103 | `cat2.public-symbol-used-c` | (none) | `cat2-stack-c-unused` |
| 3 | 105-110 | `cat2.public-symbol-used-ts` | (none) | `cat2-stack-b-unused` |
| 4 | 112-117 | `cat2.stub-label-stale` | (none) | `cat2-stub-label-stale` |
| 5 | 119-124 | `cat2.public-symbol-used-toolkit` | (none) | `toolkit-own-unused` |
| 6 | 126-131 | `cat1.intra-repo` | `docs/diagnostics/_audits/` | `audit-citation` |
| 7 | 133-147 | `cat1.upstream-citation` + `"1.8.10" in msg` | sph-water shaders/src → `live-shader-1810`; else `audit-doc-1810` | (two outcomes) |
| 8 | 149-173 | `cat1.annotation-form` | spec/docs/retro/integrity/_audits dispatch | `spec-grammar-example` / `retro-grammar-example` / `toolkit-own-source` / `audit-report-grammar-example` |
| 9 | 175-180 | `cat1.bare-path` | `docs/diagnostics/_audits/` | `audit-bare-path` |
| 10 | 182-187 | `cat1.bare-path` | `docs/retro/` | `retro-bare-path` |
| 11 | 189-198 | `cat1.bare-path` | spec.md / toolkit/docs/ / toolkit/README.md | `toolkit-doc-bare-path` |
| 12 | 200-205 | `cat1.bare-path` + `"LeniaNDK.py" in msg` | (none) | `deferred-upstream-bare-path` |
| 13 | 207-212 | `cat1.bare-path` (any remaining) | (none) | `other-cat1-bare-path` |
| — | 214-218 | (any remaining) | (none) | `other-cat1` |

**FACT.** Counts:

- cat2 rules: **5** (#1-5). T1.1 adds zero in this block.
- cat1.upstream-citation rules: **1** (#7, two sub-outcomes).
- cat1.annotation-form rules: **1** (#8, four sub-outcomes).
- cat1.intra-repo rules: **1** (#6, `audit-citation` only). **T1.1
  adds three more in this block** (`toolkit-doc-snapshot`,
  `project-state-snapshot`, `retro-doc-snapshot`).
- cat1.bare-path rules: **5** (#9-13).
- Default fallthrough: **1** (line 214-218, the `other-cat1` last resort).

**FACT.** The cat1.intra-repo → other-cat1 path: any
`cat1.intra-repo` finding that doesn't match rule #6 falls through all
of #7-#13 (none of which apply to `cat1.intra-repo`) and lands at the
default at line 214-218. T1.1's three new rules insert between rule #6
(audit-doc) and rule #7 (1.8.10), or equivalently anywhere before the
final fallthrough; they only match `cat1.intra-repo` so position among
unrelated rules is irrelevant for behavior. **Recommended insertion
point** (INFERENCE): immediately after rule #6, lines 132-onward.
This keeps all `cat1.intra-repo` rules contiguous.

### D.3 — Conflict check with existing rules

**FACT.** None of the three new rules' patterns are already covered by
existing classifier code:

- `cat1.intra-repo` + `f.startswith("tools/integrity/docs/")` — not
  matched by any rule above. Falls through to line 214-218 (`other-cat1`).
  T1.1 promotes it to `toolkit-doc-snapshot`.
- `cat1.intra-repo` + `f == "project-state.md"` — not matched. Falls
  through to `other-cat1`. T1.1 promotes to `project-state-snapshot`.
- `cat1.intra-repo` + `f.startswith("docs/retro/")` — not matched.
  Falls through to `other-cat1`. T1.1 promotes to `retro-doc-snapshot`.

**INFERENCE.** T1.1 is genuinely a code-change item (not a docs/tests-only
shift). The three rules are pure additions; no existing rule is modified
or reordered. This matches roadmap § 4 T1.1 framing: "Adds rules above
the `other-cat1` fallthrough in `classify()`; pure addition."

**FACT — verification via probe-template's "post-batch-triage § C.2 sketched
rule" check.** The triage doc proposed:

```python
if cid == "cat1.intra-repo" and (
    f.startswith("tools/integrity/docs/")
    or f == "docs/integrity-toolkit-spec.md"
    or f == "tools/integrity/README.md"
):
    return Classification(category="toolkit-doc-snapshot", ...)
```

This sketch has **three path predicates**, including the spec.md and
README.md exact-paths. The spec author should confirm whether T1.1
adopts this 3-path form or the simpler `f.startswith("tools/integrity/docs/")`
single-predicate form (which would miss the two exact paths).
**Pause-and-surface for spec**: § K item 5.

**FACT — verification via post-retro landing audit § D.3 sketched
`project-state-snapshot` rule.** Sketch (lines 277-282):

```python
if cid == "cat1.intra-repo" and f == "project-state.md":
    return Classification(
        category="project-state-snapshot",
        reason="project-state.md cross-phase reflection snapshot (...)",
        issue_ref="n/a",
    )
```

Adopt as-is. Single exact-path predicate; no ambiguity.

**FACT.** No equivalent prior sketch exists for `retro-doc-snapshot`;
the roadmap's § 4 T1.1 row #3 is its only banking. Inferring shape:

```python
if cid == "cat1.intra-repo" and f.startswith("docs/retro/"):
    return Classification(
        category="retro-doc-snapshot",
        reason="retro-doc snapshot of pre-v1 codebase (...)",
        issue_ref="n/a",
    )
```

Mirrors the existing `retro-bare-path` and `retro-grammar-example`
rules' path-predicate form.

## § E — Currently-other-cat1 findings on T1.1's target paths

### E.1 — Current `other-cat1` baseline

**FACT.** Per § A.3 grandfather-report: 36 findings classify as
`other-cat1`. This is the post-A.2 baseline; T1.1's three rules will
reshape it.

### E.2 — Enumeration of `other-cat1` findings by path prefix

**FACT.** Programmatic enumeration (via `--output json` + reason-string
extraction matching `snapshot.py:_extract_category`), grouped by path
prefix:

| Path prefix | Count | Check IDs |
|---|---|---|
| `tools/integrity/tests/` | 10 | 8 × `cat1.annotation-form` on `test_grandfather_sweep.py` (lines 74, 79, 84, 89, 116, 122, 128, 141); 2 × `cat1.unregistered-upstream` on `test_cat1_unregistered.py:27, 29` |
| `docs/diagnostics/_audits/` | 6 | `integrity_v1_2_a3_probe_2026-05-15:1047` (×2, `cat1.intra-repo`); 3 × `cat1.upstream-citation` in build/triage/probe audits; 1 × `cat1.unregistered-upstream` in build-3 landing |
| `docs/retro/` | **6** | All `cat1.intra-repo`: `v1.1-batch1-addendum.md:134, 137, 140` and `v1.1-batch1.md:332, 334, 339` |
| OTHER (long tail) | 5 | `CHANGELOG.md:92`; `continuous-ca/lenia-fft/docs/notes.md:63`; `particle-fluids/sph-water/docs/notes.md:20`; `particle-fluids/sph-water/src/main.cpp:352`; `tools/integrity/drivers/integrity_cat3_stack_c/main.cpp:10` |
| `tools/integrity/docs/` | **5** | All `cat1.intra-repo`: `algebraic/d3q19.md:175, 177`; `grandfather-catalog.md:199`; `ground-truth-sources.md:54` (×2 — same line, two citations) |
| `tools/integrity/integrity/` | 4 | All `cat1.upstream-citation`: `cat3_numerical/checks/cubic_kernel.py:8, 79, 96`; `cat3_numerical/generate_expected.py:105` |

**Total:** 36 (matches § A.3 grandfather-report count).

**FACT — T1.1 re-classification counts (cat1.intra-repo only, since the
three new rules all gate on `cid == "cat1.intra-repo"`):**

| Target path prefix | New category | cat1.intra-repo count from other-cat1 |
|---|---|---|
| `tools/integrity/docs/` | `toolkit-doc-snapshot` | **5** (4 distinct findings; ground-truth-sources.md:54 has two on the same line) |
| `docs/retro/` | `retro-doc-snapshot` | **6** |
| `project-state.md` | `project-state-snapshot` | **0** |

Total re-classified: **11**. Post-T1.1 `other-cat1` count: **25**
(36 − 11).

### E.3 — Surprising finding: `project-state-snapshot` reclassifies zero findings

**FACT.** `project-state.md` contributes **0** entries to the current
`other-cat1` bucket. This contradicts the v1.1 post-retro landing audit
§ D.3 framing, which characterized project-state.md as
"falls through to LIVE-SOURCE by default".

Investigation (FACT — from § C.1 grep results and direct file read):

- `project-state.md` has three `integrity-allow:` annotations
  bearing the `other-cat1` reason string at lines 559, 593, 666
  (per `grep -rn 'see grandfather-catalog other-cat1' project-state.md`).
- Strict-mode hard-fails on `project-state.md` are all `cat1.bare-path`
  (lines 561, 595, 668), not `cat1.intra-repo`.
- The current `--grandfather-report`-counted `other-cat1` set in § E.2
  has zero `project-state.md` entries.

**INFERENCE.** Two possibilities, both plausible:

1. The 3 historical `other-cat1` annotations on `project-state.md` are
   "fossil" annotations from earlier sweeps. Their underlying findings
   have either resolved (the cited paths were fixed) or migrated to
   `cat1.bare-path` (the post-A.3 check that subsumed some intra-repo
   patterns into bare-path). The annotations themselves no longer
   correspond to live suppressions; they may even be redundant decoration.
2. Inline annotation grammar parses these as line-comments-above
   covering the next line, but the next line has a different finding
   shape than the annotation's `check_id`, so the suppression doesn't
   match and the finding is now classified under cat1.bare-path
   (which itself is suppressed by separate annotations or falls into
   `audit-bare-path` / `retro-bare-path`).

**INFERENCE — implication for T1.1.** The `project-state-snapshot`
rule is still worth adding (it's costless in code; it covers any
*future* `cat1.intra-repo` finding on `project-state.md`) but has
zero immediate re-classification impact. The new catalog section will
have a `(0)` initial count (or `(?)` placeholder per § F.1).

**Pause-and-surface for spec — § K item 6**: should the spec confirm
this analysis by spot-checking one of the three "fossil" annotations
at `project-state.md:559/593/666`, and consider removing them in T1.1's
sweep companion? Out-of-scope cleanup; bank for a follow-on commit.

## § F — Catalog format conventions

### F.1 — Full verbatim dump of `tools/integrity/docs/grandfather-catalog.md`

**FACT.** File totals **398 LOC** (per `wc -l`).

[...elided lines 1-42 — file header, "Updating counts" prose, category
header. Critical fact: lines 13-39 describe T1.3's auto-refresh script
behavior; lines 38-39 reference the v1.3 candidates roadmap...]

One representative existing category section (FACT — verbatim
`audit-citation` at catalog lines 43-54) that T1.1's new sections
should mirror structurally:

```markdown:tools/integrity/docs/grandfather-catalog.md
    43	### `audit-citation` (101)
    44	
    45	**Pattern:** `cat1.intra-repo` findings in files under `docs/diagnostics/_audits/`.
    46	
    47	**Why grandfathered:** Audit reports are snapshots of the codebase at a specific
    48	moment. Citations were valid at audit time; subsequent code drift made some
    49	unresolvable. Audit reports are append-only by convention; retroactively
    50	editing them would erase the historical record.
    51	
    52	**Future treatment:** Permanent suppression. New audit reports landing after
    53	v1 may reference paths-that-no-longer-exist; if so, those new citations get
    54	the same suppression at write-time.
```

Two further structural mirrors (FACT — same triple-bold-key shape):

- `audit-bare-path` heading at line 311 (count `747`) — same form,
  bare-path analog.
- `toolkit-doc-bare-path` heading at line 339 (count `7`) — three-
  predicate pattern matching exactly the shape proposed for T1.1's
  `toolkit-doc-snapshot` (§ K.5 Shape B).

[...elided remaining catalog sections — `cat2-stack-*-unused`,
`retro-bare-path`, etc. — same per-section structure (Pattern + Why
grandfathered + Future treatment); read source if needed...]

### F.2 — Structural mirror for T1.1's new sections

**INFERENCE — recommended structure for T1.1's three new catalog
sections** (each ~12-15 lines of markdown):

```markdown
### `toolkit-doc-snapshot` (5)

**Pattern:** `cat1.intra-repo` findings in files under
`tools/integrity/docs/` (and `docs/integrity-toolkit-spec.md`,
`tools/integrity/README.md` — per spec author's choice; see § K).

**Why grandfathered:** Toolkit-internal documentation cites the
toolkit's own modules and the vendored Krüger book-companion code as
reference material. Citations are documentation, not source-to-source
intra-repo refs; the cited path may move or stay, but the reference
remains valid as historical anchor.

**Future treatment:** Permanent suppression on toolkit-doc paths.
Mirrors `audit-citation` (analogous pattern at a different doc tier).
```

```markdown
### `project-state-snapshot` (0)

**Pattern:** `cat1.intra-repo` findings at `project-state.md`.

**Why grandfathered:** `project-state.md` is a cross-phase narrative
snapshot of the codebase at landing time. Like audit reports, its
citations are valid at write-time and may stale as code moves; like
retros, it is append-only by convention.

**Future treatment:** Permanent suppression on this exact path.
```

```markdown
### `retro-doc-snapshot` (6)

**Pattern:** `cat1.intra-repo` findings in files under `docs/retro/`.

**Why grandfathered:** Retrospective documents narrate cross-phase
history and cite repo files by intra-repo path. Same rationale as
`audit-citation` and `retro-bare-path` (the bare-path analog).

**Future treatment:** Permanent suppression on retro paths. Mirrors
`audit-citation` and `retro-bare-path`.
```

These plus T1.3's `(?)` placeholder convention (see § I.3): the catalog
should land with numeric counts if T1.1's sweep companion is run inline
(recommended), or with `(?)` if the auto-refresh script is invoked
post-commit. Per § I, T1.3 has landed, so the numeric form is preferred.

### F.3 — Updating-counts section status

**FACT** (lines 13-39 of catalog — already part of post-T1.3 state):
The "Updating counts" section now describes the auto-refresh script.
T1.3 has landed (commit `65a7685`), so this prose is the post-T1.3
canonical form. T1.1 does not need to edit this section.

The auto-refresh script behavior (FACT, from catalog lines 30-34):
"The script errors out if any category in the report lacks a
corresponding heading in this file." This means T1.1 must add the
three new headings *before* the auto-refresh script can be re-run
without erroring. Same-commit ordering: classifier rules first, then
catalog headings, then auto-refresh (or human-authored counts).

## § G — Test pattern for new classifier rules

### G.1 — Test file shape

**FACT.** `tools/integrity/tests/test_grandfather_sweep.py` totals
**263 LOC** with 21 test functions:

- 13 classifier-result tests (lines 19-69): one per existing category
  + one `test_other_cat1_fallthrough`.
- 4 comment-form tests (lines 72-89).
- 3 fenced-block tests (lines 92-111).
- 3 annotation-already-present tests (lines 114-129).
- 2 render-annotation tests (lines 132-155).
- 4 P1.8 live-source tests (lines 163-263).

### G.2 — Canonical mirror tests for T1.1's new rules

**FACT** (verbatim — three representative existing classifier-result
tests):

```python:tools/integrity/tests/test_grandfather_sweep.py
    19	def test_audit_citation_classification() -> None:
    20	    f = _f("cat1.intra-repo", "docs/diagnostics/_audits/probe.md")
    21	    assert classify(f).category == "audit-citation"
```

```python:tools/integrity/tests/test_grandfather_sweep.py
    54	def test_retro_grammar_example_classification() -> None:
    55	    f = _f("cat1.annotation-form", "docs/retro/integrity-toolkit-v1.md")
    56	    assert classify(f).category == "retro-grammar-example"
```

```python:tools/integrity/tests/test_grandfather_sweep.py
    67	def test_other_cat1_fallthrough() -> None:
    68	    f = _f("cat1.intra-repo", "some/random/file.cpp")
    69	    assert classify(f).category == "other-cat1"
```

**INFERENCE — recommended test additions for T1.1** (one per new rule;
4th test optional):

```python
def test_toolkit_doc_snapshot_classification() -> None:
    f = _f("cat1.intra-repo", "tools/integrity/docs/algebraic/d3q19.md")
    assert classify(f).category == "toolkit-doc-snapshot"


def test_project_state_snapshot_classification() -> None:
    f = _f("cat1.intra-repo", "project-state.md")
    assert classify(f).category == "project-state-snapshot"


def test_retro_doc_snapshot_classification() -> None:
    f = _f("cat1.intra-repo", "docs/retro/integrity-toolkit-v1.md")
    assert classify(f).category == "retro-doc-snapshot"
```

Optional 4th test — rule-ordering guard:

```python
def test_other_cat1_fallthrough_still_works_for_unmatched_intra_repo() -> None:
    """Verify T1.1's three new rules don't shadow the other-cat1 fallthrough."""
    f = _f("cat1.intra-repo", "some/sim/path.cpp")
    assert classify(f).category == "other-cat1"
```

The existing `test_other_cat1_fallthrough` at line 67 already covers
this case (it uses `some/random/file.cpp`). The 4th test is redundant
unless the spec author wants explicit ordering coverage.

### G.3 — Test additions for T1.2

**INFERENCE — T1.2 test additions under option (d)** (one test per
helper + one integration test):

```python
def test_is_fallthrough_category_for_known_buckets() -> None:
    from integrity.grandfather import is_fallthrough_category
    assert is_fallthrough_category("other-cat1") is True
    assert is_fallthrough_category("other-cat1-bare-path") is True


def test_is_fallthrough_category_for_named_categories() -> None:
    from integrity.grandfather import is_fallthrough_category
    assert is_fallthrough_category("audit-citation") is False
    assert is_fallthrough_category("toolkit-own-unused") is False
    assert is_fallthrough_category("audit-bare-path") is False
```

The existing P1.8 tests at lines 192-263 already exercise the
`apply_annotations()` filter end-to-end; they continue to pass under
T1.2's refactor (because the refactor preserves observable behavior —
same findings filter the same way). No new integration test is needed,
but the existing tests serve as the regression check.

**Estimated total test additions:** 3 (T1.1, one per rule) + 2 (T1.2,
helper-level) = **5 new tests**. Roadmap § 4 T1.1 estimated 3-4 tests
for T1.1 alone; this analysis tightens that to 3. Roadmap § 4 T1.2
estimated tests not separately broken out; 2 is parsimonious.

## § H — P1.8 filter logic post-A.2

### H.1 — Filter call site

Already captured in § B.1 (lines 401-413) and § C.2 (re-quoted at
lines 393-410 with comments). The literal-list at line 406 is the sole
T1.2 refactor target.

### H.2 — `apply_annotations()` signature confirmation

**FACT.** Programmatic confirmation:

```
$ python3 -c "from integrity.grandfather import apply_annotations; import inspect; print(inspect.signature(apply_annotations))"
(repo_root: 'Path', dry_run: 'bool', sweep_live_source: 'bool' = False, force_sweep_categories: 'frozenset[str]' = frozenset()) -> 'tuple[int, int, dict[str, int], int]'
```

Matches v1.2 A.2 Decision 4 spec exactly: `force_sweep_categories`
parameter is present, default empty frozenset, accepts named categories
to opt into sweep even when their findings sit on LIVE-SOURCE paths.

### H.3 — Pre- vs. post-T1.2 behavior

**Current behavior** (FACT, line 406):

```
LIVE-SOURCE skip applies iff:
    is_live_source_path(file) AND
    cat in ("other-cat1", "other-cat1-bare-path") AND
    cat NOT IN force_sweep_categories
```

**Post-T1.2 behavior under option (d)**:

```
LIVE-SOURCE skip applies iff:
    is_live_source_path(file) AND
    is_fallthrough_category(cat) AND
    cat NOT IN force_sweep_categories
```

**INFERENCE.** Behaviorally identical for the current set of categories
(the helper returns True iff `cat in FALLTHROUGH_CATEGORIES = frozenset({"other-cat1", "other-cat1-bare-path"})`).
The refactor is purely structural — same findings filter the same way.
This is the key design property that makes T1.2 low-risk: existing
tests pass without modification because observable behavior is
preserved.

### H.4 — Composition with T1.1

T1.1 introduces three new *named* categories. None of them are
fallthrough (they have explicit path predicates and are not on the
fallthrough-set membership list). Post-T1.1 + T1.2:

- `is_fallthrough_category("toolkit-doc-snapshot")` returns **False**.
- `is_fallthrough_category("project-state-snapshot")` returns **False**.
- `is_fallthrough_category("retro-doc-snapshot")` returns **False**.

**FACT.** All three new categories' file-path predicates target paths
under `SWEEPABLE_PATH_PREFIXES` or `SWEEPABLE_EXACT_PATHS`:

- `tools/integrity/docs/` → in `SWEEPABLE_PATH_PREFIXES` (line 56).
- `docs/retro/` → in `SWEEPABLE_PATH_PREFIXES` (line 55).
- `project-state.md` → in `SWEEPABLE_EXACT_PATHS` (line 63).

So `is_live_source_path()` returns **False** for any file matching a
T1.1 rule. The LIVE-SOURCE skip path is not even consulted: findings
classified to T1.1's new categories are swept normally on first run.
**Confirms roadmap § 4 T1.1's "Interaction with v1.2 P1.8" framing
exactly.**

## § I — Coordination with parallel session's T1.3/T1.4/T1.5

### I.1 — Part-A landing status

**FACT.** Per § A.4 commit log:

| Commit | Subject | T1 item |
|---|---|---|
| `65a7685` | feat(integrity): T1.3 catalog auto-refresh script (v1.3 commit 1) | T1.3 |
| `72a2d26` | refactor(integrity): T1.5 cat3 expected-values TOML -> JSON (v1.3 commit 2) | T1.5 |
| `9e3afa9` | docs(integrity): T1.4 probe template conventions doc (v1.3 commit 3) | T1.4 |
| `1f7785f` | docs(integrity): SHA back-fill for v1.3 batch-1 part-A commits 1-3 (v1.3 commit 4) | SHA back-fill |

All part-A commits are on main. The part-B scope (T1.2 + T1.1) is the
remaining work.

### I.2 — T1.3 auto-refresh script behavior (relevant to T1.1)

**FACT.** `tools/integrity/scripts/refresh_catalog_counts.py` exists
(per `ls` in § I.1 verification commands). Tests at
`tools/integrity/tests/test_refresh_catalog_counts.py` confirm:

- The script parses numeric `(N)` parentheticals in catalog category
  headings and rewrites them to match current `--grandfather-report`
  output.
- It preserves non-numeric parentheticals verbatim (e.g., `(?)` and
  `(0 swept; 44 live-source skipped)`).
- It errors out if a reported category lacks a corresponding catalog
  heading.

**INFERENCE — T1.1 implication.** T1.1 must add catalog headings for
the three new categories *before* running auto-refresh (otherwise the
script will error). Three ordering choices in the spec:

1. T1.1 adds catalog headings with **numeric** counts authored by the
   spec author (5 / 0 / 6 per § E.2) at commit time. No auto-refresh
   needed in T1.1's commit; auto-refresh remains a separate cadence.
2. T1.1 adds catalog headings with **`(?)`** placeholders at commit
   time. Auto-refresh is run in a follow-up commit (or as part of
   T1.1's commit, after the heading-adds).
3. T1.1 adds catalog headings with numeric counts AND runs
   auto-refresh inline as a verification step (the script's `--dry-run`
   reports zero diffs, confirming the manually-authored counts match
   live state).

**Recommended (INFERENCE):** option (3). Numeric counts at write-time
give immediate documentation value; `--dry-run` auto-refresh as the
inline verification step confirms accuracy. Matches Decision 6 from
v1.2 A.2 ("refresh in same commit as new category addition") with
T1.3's auto-refresh as the mechanical verifier.

### I.3 — Sweep companion vs. catalog refresh

**FACT.** T1.1's three new rules re-classify 11 *existing* annotated
findings (per § E.2). These findings already carry `integrity-allow:`
annotations bearing the `other-cat1` reason string. Post-T1.1, those
annotations' reason strings no longer match the new categories' reason
strings. Two consequences:

1. **Grandfather-report counts shift.** The `other-cat1` count drops
   from 36 → 25; new categories show 5 / 0 / 6 (via reason-string
   re-extraction once annotations are updated).
2. **Annotations themselves are stale.** Until each affected
   annotation's reason text is updated, `_extract_category` in
   `snapshot.py` will still match it to `other-cat1` by reason-string
   keyword (since the historical reason text contains
   `see grandfather-catalog other-cat1`).

**INFERENCE — sweep companion required.** T1.1's commit must include
a re-sweep that rewrites the 11 affected annotations' reason strings
to match the new categories. Concretely:

```bash
# After classifier rules + catalog headings land:
python3 tools/integrity/scripts/grandfather_sweep.py --dry-run
# Verify expected diff: 11 annotations on lines:
#   tools/integrity/docs/algebraic/d3q19.md:175, 177
#   tools/integrity/docs/grandfather-catalog.md:199
#   tools/integrity/docs/ground-truth-sources.md:54 (x2)
#   docs/retro/integrity-toolkit-v1.1-batch1-addendum.md:134, 137, 140
#   docs/retro/integrity-toolkit-v1.1-batch1.md:332, 334, 339
python3 tools/integrity/scripts/grandfather_sweep.py
```

**FACT.** The sweep companion is mechanically supported: the existing
`grandfather_sweep.py` CLI already handles re-classification of
findings whose reason text doesn't match current `classify()` output
— the `annotation_already_present()` helper at `grandfather.py:268-280`
checks for category-prefix coverage, which means existing annotations
with the old reason are recognized as already-covering but get
re-written if the reason text changes.

**Pause-and-surface for spec — § K item 7.** The spec author should
confirm whether the sweep companion's diff matches the 11 expected
re-classifications exactly, or whether other annotations also get
rewritten (e.g., other-cat1-bare-path annotations on the same lines).

### I.4 — Catalog auto-refresh inline in T1.1 commit (Decision 6 alignment)

Per § I.2 option (3): include `python3 tools/integrity/scripts/refresh_catalog_counts.py --dry-run`
in the T1.1 commit's verification block. Expected output: zero diffs
(if the spec author authored numeric counts correctly). If diffs
appear, the verification block records the actual numeric form so the
sweep companion + auto-refresh produce the catalog in the same commit.

## § J — Banked observations

Incidental findings during this probe that aren't direct T1.1 or T1.2
scope but inform spec drafting.

### J.1 — `_KNOWN_CATEGORIES` tuple in `snapshot.py` requires extension

**FACT** (per § F.3 grep + `snapshot.py:27-48`): the
`_KNOWN_CATEGORIES` tuple at `snapshot.py:27-48` enumerates every
category recognized by the reason-string extractor. T1.1's three new
categories must be added to this tuple, or the auto-refresh script and
grandfather-report will fail to recognize them.

**INFERENCE.** This is a *secondary* code touch for T1.1: not only
`grandfather.py:classify()` but also `snapshot.py:_KNOWN_CATEGORIES`.
Roadmap § 4 T1.1's scope estimate ("~50 LOC code + ~40 LOC catalog
+ ~50 LOC tests") implicitly covers this; the spec should call it out
explicitly.

The ordering convention (lines 40-42 of `snapshot.py`):

```python
# v1.2 A.3 bare-path categories (longer names first so they match
# before "other-cat1" which is a substring of "other-cat1-bare-path").
```

T1.1's three new categories should be added *before* `other-cat1` in
the tuple to ensure correct substring matching. Specifically:
`toolkit-doc-snapshot`, `project-state-snapshot`, `retro-doc-snapshot`
do not have substring conflicts with each other or with existing
categories (FACT — none is a substring of another), but ordering still
matters for the `for cat in _KNOWN_CATEGORIES` loop's first-match
semantics.

### J.2 — Three-category prose-vs-code mismatch risk

The v1.2 A.2 spec surfaced three "prose says X, code doesn't enforce X"
gaps at execution time. Equivalent risks in T1.1 / T1.2:

- **T1.1 risk 1.** The spec's "5 / 0 / 6 reclassification" counts (per
  § E.2) are derived at probe time. If the v1.3 part-A commits or any
  intervening commit alters live `other-cat1` membership, the counts
  drift. Mitigation: spec drafting captures the count derivation
  command (the `python3 -c "from integrity.grandfather import ..."`
  enumeration in § E.2) so the spec author re-runs it at landing time
  and corrects the spec/verification-block counts inline.
- **T1.1 risk 2.** The catalog's `(N)` parenthetical numbers are
  fact-claims about current state. Once T1.3 auto-refresh exists,
  the spec author must explicitly invoke it (or hand-author counts
  that match auto-refresh's output) — otherwise the catalog's stated
  counts diverge from `--grandfather-report` immediately.
- **T1.2 risk 1.** The set-name choice (`FALLTHROUGH_CATEGORIES`) is
  documented in the v1.2 bolt-ons retro § 4.2 as "Convention H." If
  the spec uses a different identifier, the convention's banked
  language doesn't match the code. Mitigation: pin the identifier in
  the spec.

### J.3 — Tests beyond `test_grandfather_sweep.py` that touch `Classification`

**FACT** (per § C.1 grep + repeated searches): **zero** tests outside
`test_grandfather_sweep.py` construct or reference `Classification`
directly. T1.2's option (d) refactor needs only the new tests in § G.3;
no other test file requires modification.

### J.4 — `other-cat1` 5-finding "OTHER" bucket worth banking

Per § E.2, 5 of the 36 `other-cat1` findings sit on paths that none of
the three new T1.1 rules cover:

- `CHANGELOG.md:92`
- `continuous-ca/lenia-fft/docs/notes.md:63`
- `particle-fluids/sph-water/docs/notes.md:20`
- `particle-fluids/sph-water/src/main.cpp:352`
- `tools/integrity/drivers/integrity_cat3_stack_c/main.cpp:10`

**INFERENCE.** These are the residual long-tail. Per the catalog's
`other-cat1` "Future treatment" (line 149: "Per-entry review in v2"),
they remain in `other-cat1` after T1.1. The post-T1.1 `other-cat1`
count of 25 includes these 5 plus the 10 `tools/integrity/tests/`
entries (which are intra-test annotations of the grammar literal —
similar to `toolkit-own-source` but on tests) plus the 6
`docs/diagnostics/_audits/` entries (which are reason-stringed as
`other-cat1` historically but should classify under `audit-citation`
or `audit-doc-1810` if their `cat1.*` cid matches the right rule).

**Bank for follow-up.** A `test-source-grammar-example` category (a
twin of `toolkit-own-source`, but for test files matching
`cat1.annotation-form`) would absorb the 8 test_grandfather_sweep.py
entries. Bank as a v1.3 candidate; out of scope for T1.1's named scope.

### J.5 — Probe-template T1.4 conventions doc landed

**FACT** (per § A.4 row `9e3afa9`): T1.4 landed a "probe template
conventions doc." If this probe deviates from those conventions, the
deviation is unintentional. The conventions doc is at
`tools/integrity/docs/probe-template-conventions.md` (per
`ls tools/integrity/docs/` in implicit probe state); the spec author
should diff this probe against the doc's conventions before
finalizing T1.1's spec to demonstrate compliance.

### J.6 — `(0 swept; 44 live-source skipped)` heading for `other-cat1-bare-path` is special-cased

**FACT** (catalog line 365): the `other-cat1-bare-path` heading uses a
free-prose parenthetical `(0 swept; 44 live-source skipped)` rather
than a numeric count. T1.3's auto-refresh test at
`test_refresh_catalog_counts.py:42` explicitly verifies this form
parses with `is_numeric=False` and is preserved unchanged.

**INFERENCE.** T1.1's new categories should use numeric `(N)`
parentheticals (the default form), not free prose. The free-prose form
is a special case for buckets whose count is itself ambiguous (swept
vs. attributed); T1.1's new categories are all permanent-suppression
and have a single canonical count.

## § K — Specific open questions for the spec to resolve

Mirroring the v1.2 A.3 and A.2 probes' closing open-questions lists.
Each item references the relevant probe section.

### K.1 — T1.2 design choice (§ B.4)

Four options enumerated in § B.4: field-with-default (a),
computed-property (b), factory-classmethods (c), module-level set +
helper (d).

**Probe recommendation:** option (d). Rationale: shape-consistent with
`SWEEPABLE_PATH_PREFIXES`; zero touches to `Classification`; one-line
filter refactor; cleanest forward path. Matches v1.3 candidates roadmap
§ 4 T1.2 architect-1 weak preference (which uses the same letter (c)
internally but with different option enumeration).

**Spec decision needed.** Confirm (d) or pick alternative.

### K.2 — T1.2 refactor scope (§ C.4)

Refactor `grandfather.py:406` is the one mandatory call site. Other
potential refactor targets:

- `snapshot.py:46-47` — keeps the literal names (these are for
  reason-string-extraction, not fallthrough discrimination). **No
  refactor.**
- `grandfather_sweep.py:54` — user-facing print string. **No refactor.**
- Tests that assert on category names. **No refactor.**

**Spec decision needed.** Confirm scope is exactly one line, or
expand. **Probe recommendation:** exactly one line.

### K.3 — T1.1 / T1.2 commit ordering

Roadmap § 9.1 orders T1.2 → T1.1 (the structural refactor lands first,
then the additive rules use it). The "additive-before-refactor"
counter-argument is weaker here because T1.1's new rules don't *touch*
the fallthrough filter (their categories aren't fallthrough), so T1.1
doesn't benefit from T1.2's landing.

**Probe recommendation:** T1.2 first, then T1.1 — per roadmap. T1.2's
structural cleanup is a single-commit micro-refactor with zero
behavior change; landing it first keeps the diff minimal and unblocks
the conceptual model for any future fallthrough-category addition.

**Spec decision needed.** Confirm ordering. **No strong reason to
deviate from roadmap.**

### K.4 — T1.1 catalog placeholder mode (§ I.2 / § I.3)

Three options enumerated in § I.2: numeric inline (1), `(?)` placeholder
(2), numeric + auto-refresh verification (3).

**Probe recommendation:** option (3). Authoring numeric counts gives
immediate documentation value; the auto-refresh `--dry-run` in the
verification block confirms accuracy. Aligns with Decision 6 from A.2
("refresh in same commit as new category addition").

**Spec decision needed.** Confirm (3) or pick alternative.

### K.5 — T1.1 `toolkit-doc-snapshot` predicate breadth (§ D.3)

Two predicate shapes for the `toolkit-doc-snapshot` rule:

- **Shape A (single prefix):** `f.startswith("tools/integrity/docs/")`.
  Misses `docs/integrity-toolkit-spec.md` and `tools/integrity/README.md`.
- **Shape B (three predicates):** `f.startswith("tools/integrity/docs/")
  or f == "docs/integrity-toolkit-spec.md" or f == "tools/integrity/README.md"`.
  Mirrors the existing `toolkit-doc-bare-path` rule at
  `grandfather.py:189-198`. Matches the post-batch-triage § C.2 sketch.

**Probe recommendation:** Shape B. Reasons:

1. Mirrors `toolkit-doc-bare-path`'s predicate exactly; symmetry with
   the bare-path sibling rule is desirable.
2. Per § E.2 enumeration, the 5 current `other-cat1` findings on
   `tools/integrity/docs/` would all match shape A. But future
   `cat1.intra-repo` findings on `docs/integrity-toolkit-spec.md`
   would not — they'd fall through to `other-cat1`, contrary to the
   classification intent. Shape B is forward-compatible.

**Spec decision needed.** Confirm Shape B.

### K.6 — `project-state.md` fossil-annotation cleanup (§ E.3)

Three `integrity-allow:` annotations on `project-state.md` at lines
559, 593, 666 bear the `other-cat1` reason string but appear to be
fossils (the underlying findings have either resolved or migrated to
`cat1.bare-path`).

**Probe recommendation:** Bank as follow-up; do NOT include in T1.1's
sweep companion. T1.1 is a category-add commit, not a fossil-cleanup
commit. The sweep companion (per § I.3) targets specifically the 11
re-classifications expected by the new rules; fossils are a separate
hygiene class.

**Spec decision needed.** Confirm bank-for-follow-up; pause-and-surface
if the spec author wants to fold cleanup into T1.1.

### K.7 — T1.1 sweep-companion expected diff (§ I.3)

The sweep companion's expected diff should match exactly the 11
re-classifications:

- `tools/integrity/docs/algebraic/d3q19.md:175, 177` (2)
- `tools/integrity/docs/grandfather-catalog.md:199` (1)
- `tools/integrity/docs/ground-truth-sources.md:54` (2 — same line)
- `docs/retro/integrity-toolkit-v1.1-batch1-addendum.md:134, 137, 140` (3)
- `docs/retro/integrity-toolkit-v1.1-batch1.md:332, 334, 339` (3)

**Spec decision needed.** Verify the sweep's dry-run diff matches this
exact list during the verification block. Pause-and-surface if extras
appear (e.g., the 8 `test_grandfather_sweep.py:74-141` annotations get
inadvertently touched).

### K.8 — `_KNOWN_CATEGORIES` tuple extension (§ J.1)

T1.1 must extend `snapshot.py:27-48` with three new entries
(`toolkit-doc-snapshot`, `project-state-snapshot`, `retro-doc-snapshot`),
ordered before `other-cat1` to preserve substring-match semantics.

**Probe recommendation:** Group them with the cat1.intra-repo
classifier outputs (next to `audit-citation`) for visual grouping.

**Spec decision needed.** Confirm the new entries' ordering in the
tuple; minor stylistic call.

### K.9 — Convention-H banked language alignment (§ J.2)

The v1.2 bolt-ons retro § 4.2 banks "Convention H — fallthrough
discriminator." The T1.2 spec should pin the chosen identifier
(`is_fallthrough_category` / `FALLTHROUGH_CATEGORIES`) into the
convention's language so future readers find both directions
(retro → code, code → retro).

**Spec decision needed.** Cross-link Convention H wording.

## § K closing — Probe-end SHA

Probe-end HEAD recorded as the final read-only action:

```
$ git rev-parse HEAD
1f7785fd6567599f948c8eee68a7641032d3ff4a
```

**FACT.** HEAD did not move during probe execution. No drift. No
concurrent landing between probe start (§ A.1) and probe end.

---

## Probe summary

- **Probe-start SHA:** `1f7785f`
- **Probe-end SHA:** `1f7785f`
- **Strict gate baseline:** 44 hard-fail (−1 from A.2 Addendum A's 45;
  drift on read-only paths; no impact on T1.1/T1.2 scope).
- **Grandfather-report `other-cat1`:** 36 → projected 25 post-T1.1
  (11 re-classified across three new categories: 5 `toolkit-doc-snapshot`,
  0 `project-state-snapshot`, 6 `retro-doc-snapshot`).
- **T1.2 design recommendation:** option (d) — module-level
  `FALLTHROUGH_CATEGORIES: frozenset[str]` + `is_fallthrough_category()`
  helper. Mirrors P1.8 idiom; one-line filter refactor.
- **T1.1 design recommendations:** Shape B predicate for
  `toolkit-doc-snapshot`; numeric inline counts + auto-refresh dry-run
  verification; in-commit sweep companion for the 11
  re-classifications; `_KNOWN_CATEGORIES` tuple extension.
- **Scope confirmation:** T1.1 ~50 LOC code + ~40 LOC catalog +
  ~30 LOC tests; T1.2 ~10 LOC code + ~15 LOC tests. Joint commit
  budget: 2 substantive commits (T1.2 then T1.1) + sweep companion
  bundled into T1.1 commit + SHA back-fill = 3 commits total.

Probe is non-normative. Spec author should re-verify counts at landing
time per § J.2 risk-1; the `other-cat1` enumeration script in § E.2 is
the canonical re-derivation.

End of probe.
