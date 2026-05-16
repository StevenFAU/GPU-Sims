---
title: "Integrity Toolkit v1.3 Batch-1 Part-B — Execution Spec (T1.2 + T1.1)"
date: 2026-05-16
author: architect1
status: draft
audience: Claude Code (executor)
sibling-docs:
  - docs/integrity-toolkit-spec.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/retro/integrity-toolkit-v1.2-bolt-ons.md
  - docs/retro/integrity-toolkit-v1.3-candidates.md
  - docs/diagnostics/_audits/integrity_v1_3_t1_1_2_probe_2026-05-16_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_a2_probe_2026-05-15_architect1.md
---

# Integrity Toolkit v1.3 Batch-1 Part-B — Execution Spec

## 0. Execution preamble

You are Claude Code executing the v1.3 batch-1 part-B small-scope batch
covering two items from the v1.3 candidates roadmap:

- **T1.2** — Convention H structural follow-through. Add
  `FALLTHROUGH_CATEGORIES` frozenset + `is_fallthrough_category()` helper
  at module level in `grandfather.py`; refactor `apply_annotations`'s
  literal string match to use the helper.
- **T1.1** — Three new classifier rules + matching catalog sections:
  `toolkit-doc-snapshot`, `project-state-snapshot`, `retro-doc-snapshot`.

Three commits total (two substantive + SHA back-fill). Estimated total
diff: ~70 LOC code + ~40 LOC catalog + ~45 LOC tests = ~155 LOC.

### 0.1 Hard rules

1. **Execute every file creation and modification specified.**
2. **Synced repo state is authoritative over this spec.** All verbatim
   claims about file contents were grep-verified against probe SHA
   `1f7785fd6567599f948c8eee68a7641032d3ff4a` (= `1f7785f`). If HEAD
   has advanced and any cited file has been modified, **pause and
   surface; do not silently adapt.**
3. **No line numbers carried from this spec into edits without
   re-verification.** Re-anchor on current file shape via `view` or
   `grep -n` before each edit.
4. **Land commits in the order given.** T1.2 → T1.1 → SHA back-fill.
   T1.2 first per roadmap § 9.1 (the structural refactor lands before
   any new rule that could reference fallthrough behavior). Each
   commit's verification block must pass before starting the next.
5. **One audit report per commit.** Path pattern:
   `docs/diagnostics/_audits/integrity_v1_3_part_b_commit<N>_landing_2026-05-16.md`.
6. **SHA back-fill is a separate follow-up commit, never `--amend`.**
   Per Convention #12.
7. **Pull-rebase before every commit.** No parallel toolkit work is
   expected to land during this batch (the parallel session is
   drafting their part-A retro, not landing toolkit code), but
   discipline applies.
8. **Audit-prose freshness (Convention F).** Verify every quantitative
   claim and SHA reference against current disk immediately before
   committing each audit report. Discrepancies become addenda, not
   body edits.
9. **`python3`, not `python`.**
10. **Live-source-stays-red discipline (P1.8).** This batch does not
    require any `--force-sweep-category` invocations. If you find
    yourself reaching for one, pause-and-surface.

### 0.2 What this spec is small-scope about

T1.1 + T1.2 is the smallest substantive batch we've drafted since v1.0
infrastructure work. Genuinely ~155 LOC across 2 substantive commits.
The spec's length should reflect the scope — target 800-1100 lines.
This is deliberate after three consecutive probes/specs blew past
their stated budgets (banked in the closing notes).

Resist the impulse to expand the spec beyond what the work needs.
The probe at SHA `1f7785f` already grounded every claim; this spec
references and applies, not re-derives.

## 1. Goals & load-bearing decisions

### 1.1 Goals

Two roadmap items land as two substantive commits:

1. **T1.2** — Replace the literal-string-match fallthrough-bucket
   discrimination in `apply_annotations` with a forward-compatible
   module-level set + helper function. Zero behavior change; structural
   cleanup only. Convention H's banked language gets a code anchor.

2. **T1.1** — Add three classifier rules that route `cat1.intra-repo`
   findings on toolkit-doc / project-state.md / retro-doc paths to
   named permanent-suppression categories instead of falling through
   to `other-cat1`. 11 current findings re-classify (5 toolkit-doc,
   6 retro-doc, 0 project-state at probe time per § E.2 of the probe).

Plus:

3. **SHA back-fill** updates the two substantive commits' audit reports
   with cross-references.

### 1.2 Decisions locked from probe § K

All nine open questions from the probe have probe-recommended answers.
This spec confirms each.

**Decision 1 (probe K.1) — T1.2 design: option (d) `FALLTHROUGH_CATEGORIES`
+ helper.** Module-level `FALLTHROUGH_CATEGORIES: frozenset[str]` plus
`is_fallthrough_category(name: str) -> bool` helper. Mirrors P1.8's
`SWEEPABLE_PATH_PREFIXES` idiom. Zero touches to `Classification`
dataclass. One-line refactor at the filter site.

**Decision 2 (probe K.2) — T1.2 refactor scope: exactly one line.**
The mandatory refactor site is the literal-string-match in
`apply_annotations`'s LIVE-SOURCE filter (currently at
`grandfather.py:406` per probe § C.2; re-anchor at execution time).
No other call sites refactor in this commit — `snapshot.py`'s
category-name list is for reason-string extraction (different purpose),
`grandfather_sweep.py`'s print string is user-facing prose (no benefit
to refactoring), and tests that assert on category names are
deliberately testing the literal strings.

**Decision 3 (probe K.3) — Commit ordering: T1.2 → T1.1 → back-fill.**
Per roadmap § 9.1. T1.2 lands first so the conceptual model is settled
before T1.1's three new rules add to the classifier surface.

**Decision 4 (probe K.4) — T1.1 catalog mode: numeric inline + auto-refresh
verification.** New catalog sections land with numeric counts (5, 0, 6)
populated inline at commit time. T1.1's verification block runs the
T1.3 auto-refresh script in `--dry-run` mode to confirm zero drift
between the spec-time counts and post-commit live counts.

**Decision 5 (probe K.5) — `toolkit-doc-snapshot` predicate: Shape B
(three-predicate union).** Matches `f.startswith("tools/integrity/docs/")
or f == "docs/integrity-toolkit-spec.md" or f == "tools/integrity/README.md"`.
Mirrors the existing `toolkit-doc-bare-path` predicate at
`grandfather.py:189-198` exactly. Shape A (single prefix) misses
`docs/integrity-toolkit-spec.md` and `tools/integrity/README.md`; Shape
B is forward-compatible.

**Decision 6 (probe K.6) — project-state.md fossil annotations banked.**
Three `integrity-allow:` annotations on `project-state.md` at lines
559, 593, 666 currently bear the `other-cat1` reason string but have
no backing findings (probe § E.3). They are fossils from concurrent-
commit churn. T1.1 does NOT include fossil cleanup; bank for v1.3
part-C or analogous hygiene batch.

**Decision 7 (probe K.7) — T1.1 sweep companion expected diff.** The
post-T1.1 sweep companion's dry-run diff must match exactly the 11
re-classifications enumerated in probe § E.2:

- `tools/integrity/docs/algebraic/d3q19.md:175, 177` (2)
- `tools/integrity/docs/grandfather-catalog.md:199` (1)
- `tools/integrity/docs/ground-truth-sources.md:54` (2 — same line, two annotations)
- `docs/retro/integrity-toolkit-v1.1-batch1-addendum.md:134, 137, 140` (3)
- `docs/retro/integrity-toolkit-v1.1-batch1.md:332, 334, 339` (3)

If the dry-run shows extras (e.g., touches
`test_grandfather_sweep.py:74-141`'s test-string annotations), pause-
and-surface — the new rules are too broad. If it shows fewer,
something has drifted since the probe — pause-and-surface.

**Decision 8 (probe K.8) — `_KNOWN_CATEGORIES` extension.** T1.1 must
extend `snapshot.py:_KNOWN_CATEGORIES` (currently lines 27-48 per
probe § J.1) with three new entries:
- `"toolkit-doc-snapshot"`
- `"project-state-snapshot"`
- `"retro-doc-snapshot"`

**Critical: insertion ordering matters.** Per probe § J.1, this is a
substring-matched tuple; subset-matching can mis-classify if a longer
name is placed after a shorter name that's a substring of it. The new
entries should land grouped near the other cat1.intra-repo classifier
outputs (next to `"audit-citation"`), placed BEFORE `"other-cat1"` to
preserve fall-through semantics. Verify the resulting tuple by running
the grandfather-report and confirming each new category renders with
its correct count.

**This `_KNOWN_CATEGORIES` extension is structurally invisible from the
grandfather.py edit but mandatory.** The same trap surfaced in A.2
(spec missed it, Claude Code caught it) and likely contributed to
part-A's banked observation about hidden secondary touches. The spec
is calling it out explicitly per the v1.3 retro candidate "classifier
rules always require a `_KNOWN_CATEGORIES` extension."

**Decision 9 (probe K.9) — Convention H wording cross-link.** When
T1.2 lands, the v1.2 bolt-ons retro § 4.2 ("Convention H — fallthrough
discriminator") gets cross-linked into the code via a docstring on
`is_fallthrough_category()` that names the convention. Both retro
text and code can be navigated from either direction.

## 2. Architecture overview

### 2.1 Modified modules

**`tools/integrity/integrity/grandfather.py`** —
- Add module-level `FALLTHROUGH_CATEGORIES` frozenset.
- Add `is_fallthrough_category(name: str) -> bool` helper.
- Refactor the LIVE-SOURCE filter in `apply_annotations` to use the
  helper instead of the literal string match.
- (T1.1) Append three new classifier rules in the cat1 block of
  `classify()`, immediately before the `other-cat1` fall-through return.

**`tools/integrity/integrity/snapshot.py`** —
- Extend `_KNOWN_CATEGORIES` tuple with three new entries (T1.1).

**`tools/integrity/docs/grandfather-catalog.md`** —
- Add three new section entries with numeric counts (T1.1).

**`tools/integrity/tests/test_grandfather_sweep.py`** —
- Add tests for the helper function (T1.2).
- Add tests for each of three new classifier rules (T1.1).

### 2.2 No new files

This batch creates no new modules, scripts, or fixture trees. T1.4 in
part-A already landed the documentation surface (`probe-template-conventions.md`).
T1.2 and T1.1 are pure additions/modifications to existing files.

## 3. Commit 1 — T1.2 (FALLTHROUGH_CATEGORIES)

### 3.1 Pre-edit verification

Before editing, anchor on current file state:

```
view tools/integrity/integrity/grandfather.py
grep -n "FALLTHROUGH\|fallthrough" tools/integrity/integrity/grandfather.py
grep -n "other-cat1" tools/integrity/integrity/grandfather.py
```

Confirm:
- `FALLTHROUGH_CATEGORIES` does not exist yet (the grep returns empty
  for `FALLTHROUGH`)
- `apply_annotations` contains literal-string match on `other-cat1` and
  `other-cat1-bare-path` per probe § C.2 (the grep finds these
  references)

If either expectation fails, pause-and-surface.

### 3.2 Module-level constant + helper

Insert after the existing module-level constants block (likely near
`SWEEPABLE_PATH_PREFIXES`; locate via `grep -n "SWEEPABLE_PATH_PREFIXES"`).
Pattern matches the P1.8 idiom.

```python
# Categories that classify findings via fall-through (catch-all
# "other-<catN>" buckets). Findings in these categories on LIVE-SOURCE
# paths are protected from auto-sweeping by default per P1.8 and
# Convention G ("sweep-side protection before check-side expansion").
#
# Conceptually a "fallthrough bucket" is one whose classification
# isn't tied to a specific defect class — it absorbs everything that
# didn't match a named rule. Forward-compatible: when a future batch
# adds a new fallthrough bucket (e.g., other-cat3 if/when introduced),
# add it here.
#
# Per Convention H (v1.2 bolt-ons retro § 4.2 — fallthrough
# discriminator): naming the bucket structurally rather than by
# literal string match.
FALLTHROUGH_CATEGORIES: frozenset[str] = frozenset({
    "other-cat1",
    "other-cat1-bare-path",
})


def is_fallthrough_category(category: str) -> bool:
    """True if `category` is a fall-through bucket (catch-all).

    Per Convention H. Used by apply_annotations's LIVE-SOURCE filter
    to identify which categories should be protected from auto-sweep
    by default on live-source paths.
    """
    return category in FALLTHROUGH_CATEGORIES
```

### 3.3 Refactor the LIVE-SOURCE filter

Locate the current filter site:

```
grep -n "category == \"other-cat1\"\|category in (\"other-cat1\"\|other-cat1-bare-path" \
  tools/integrity/integrity/grandfather.py
```

Per probe § C.2, the current code at `apply_annotations` filters
LIVE-SOURCE skip using a literal expression like:

```python
if (is_live_source_path(file_path)
        and category in ("other-cat1", "other-cat1-bare-path")
        and not sweep_live_source
        and category not in force_sweep_categories):
    continue  # skip live-source fallthrough finding
```

(Exact shape may differ from this prose; re-anchor on actual current
code.)

Refactor to:

```python
if (is_live_source_path(file_path)
        and is_fallthrough_category(category)
        and not sweep_live_source
        and category not in force_sweep_categories):
    continue  # skip live-source fallthrough finding
```

**Behavior contract:** zero change. The set membership check is
identical to the tuple containment check. Test suite must continue to
pass without modification.

### 3.4 New tests

Add to `tools/integrity/tests/test_grandfather_sweep.py`:

```python
class TestFallthroughCategoryHelper:
    """Convention H structural follow-through (T1.2)."""

    def test_other_cat1_is_fallthrough(self):
        from integrity.grandfather import is_fallthrough_category
        assert is_fallthrough_category("other-cat1") is True

    def test_other_cat1_bare_path_is_fallthrough(self):
        from integrity.grandfather import is_fallthrough_category
        assert is_fallthrough_category("other-cat1-bare-path") is True

    def test_named_category_is_not_fallthrough(self):
        from integrity.grandfather import is_fallthrough_category
        assert is_fallthrough_category("audit-citation") is False
        assert is_fallthrough_category("toolkit-own-source") is False
        assert is_fallthrough_category("toolkit-own-unused") is False

    def test_unknown_category_is_not_fallthrough(self):
        from integrity.grandfather import is_fallthrough_category
        assert is_fallthrough_category("nonexistent") is False
        assert is_fallthrough_category("") is False

    def test_fallthrough_categories_is_frozenset(self):
        from integrity.grandfather import FALLTHROUGH_CATEGORIES
        assert isinstance(FALLTHROUGH_CATEGORIES, frozenset)
        # Frozen so future code can't mutate the discriminator at
        # runtime.

    def test_fallthrough_categories_contents(self):
        from integrity.grandfather import FALLTHROUGH_CATEGORIES
        # Pin the v1.3 baseline. When a future batch adds new
        # fallthrough categories, update this assertion intentionally.
        assert FALLTHROUGH_CATEGORIES == frozenset({
            "other-cat1",
            "other-cat1-bare-path",
        })
```

### 3.5 Verification — commit 1

```bash
# Tests pass.
cd tools/integrity && python3 -m pytest tests/ -v --tb=short
# Expected: all tests pass. +6 new tests.

# Gate state unchanged.
python3 -m integrity --mode strict --no-audit-log
# Expected: same hard-fail count as pre-commit (44 at probe time).
# T1.2 is a pure refactor with zero behavior change.

# Helper is importable.
python3 -c "from integrity.grandfather import is_fallthrough_category, FALLTHROUGH_CATEGORIES; print(FALLTHROUGH_CATEGORIES)"
# Expected: frozenset({'other-cat1', 'other-cat1-bare-path'})

# Refactor site is now using the helper.
grep -n "is_fallthrough_category" tools/integrity/integrity/grandfather.py
# Expected: at least one call site in apply_annotations + the
# definition.
```

### 3.6 Commit message — commit 1

```
feat(integrity): module-level FALLTHROUGH_CATEGORIES + helper (T1.2, v1.3 commit 1)

Adds FALLTHROUGH_CATEGORIES: frozenset[str] and
is_fallthrough_category() helper at module level in grandfather.py.
Refactors apply_annotations's LIVE-SOURCE filter to use the helper
instead of literal string match.

Convention H (v1.2 bolt-ons retro § 4.2 - fallthrough discriminator)
gets a code anchor. Forward-compatible for future fallthrough buckets:
new entries added by extending the frozenset.

Zero behavior change. Test suite unchanged. New tests pin the v1.3
fallthrough set contents.

Per v1.3 roadmap § 4 T1.2 and probe § B.4 / K.1 decision (d).
```

### 3.7 Commit 1 audit report

`docs/diagnostics/_audits/integrity_v1_3_part_b_commit1_landing_2026-05-16.md`.
Front-matter sibling-docs references this spec + the probe.
A-E structure mirrors v1.2 commit-landing reports:

- A: Change summary — Convention H code anchor + zero-behavior refactor
- B: File inventory — 1 modified module + test additions (+6 tests)
- C: Verification block verbatim
- D: Behavioral notes — pure structural refactor; LIVE-SOURCE filter
  semantic unchanged
- E: Self-review checks (§ 9 of this spec) results

## 4. Commit 2 — T1.1 (three new classifier rules)

### 4.1 Pre-edit verification

```
view tools/integrity/integrity/grandfather.py
grep -n "return Classification" tools/integrity/integrity/grandfather.py
grep -n "audit-citation\|toolkit-doc-bare-path" tools/integrity/integrity/grandfather.py
grep -n "_KNOWN_CATEGORIES" tools/integrity/integrity/snapshot.py
```

Confirm:
- The cat1 block in `classify()` is intact; the `other-cat1`
  fall-through return is the last cat1 rule.
- The `audit-citation` rule (probe § F.2 canonical mirror for
  audit-doc rules) and `toolkit-doc-bare-path` rule (probe § F.2
  canonical mirror for toolkit-doc Shape B predicate) exist as
  structural templates.
- `_KNOWN_CATEGORIES` tuple is at the probe's anchored location.

If any expectation fails, re-anchor before editing.

### 4.2 New classifier rules

Insert into `classify()` in `grandfather.py` immediately before the
`return Classification(category="other-cat1", ...)` fall-through. The
order of the three new rules among themselves doesn't matter (their
predicates are disjoint), but they all must precede the `other-cat1`
return.

```python
    # T1.1 — Three named permanent categories for cat1.intra-repo
    # findings on snapshot-style documents. Per v1.3 roadmap § 4 T1.1
    # and v1.1 batch-1 post-retro landing audit § D.3.

    if cid == "cat1.intra-repo" and (
        f.startswith("tools/integrity/docs/")
        or f == "docs/integrity-toolkit-spec.md"
        or f == "tools/integrity/README.md"
    ):
        return Classification(
            category="toolkit-doc-snapshot",
            reason="toolkit-doc snapshot intra-repo citation pre-v1.3 (see grandfather-catalog toolkit-doc-snapshot)",
            issue_ref="n/a",
        )

    if cid == "cat1.intra-repo" and f == "project-state.md":
        return Classification(
            category="project-state-snapshot",
            reason="project-state.md cross-phase snapshot intra-repo citation (see grandfather-catalog project-state-snapshot)",
            issue_ref="n/a",
        )

    if cid == "cat1.intra-repo" and f.startswith("docs/retro/"):
        return Classification(
            category="retro-doc-snapshot",
            reason="retro-doc snapshot intra-repo citation pre-v1.3 (see grandfather-catalog retro-doc-snapshot)",
            issue_ref="n/a",
        )
```

**Predicate structural mirrors:**
- `toolkit-doc-snapshot` predicate mirrors `toolkit-doc-bare-path`
  exactly (Decision 5 / probe K.5 Shape B).
- `project-state-snapshot` predicate is a single equality check.
- `retro-doc-snapshot` predicate is a single startswith check.

### 4.3 `_KNOWN_CATEGORIES` extension (Decision 8)

**Critical: this is the structurally-invisible-from-grandfather.py edit
that the spec is explicitly enforcing.** Skipping this step will cause
the grandfather-report to under-attribute the new categories at commit-
verification time.

Edit `tools/integrity/integrity/snapshot.py`. Add the three new
entries grouped with `"audit-citation"` (the existing cat1.intra-repo
classifier output, per probe J.1 recommended grouping). Insert
BEFORE `"other-cat1"` in the tuple to preserve fall-through semantics.

After the edit, the relevant portion of `_KNOWN_CATEGORIES` should
look approximately:

```python
    # cat1.intra-repo classifier outputs
    "audit-citation",
    "toolkit-doc-snapshot",
    "project-state-snapshot",
    "retro-doc-snapshot",
    # ... (other cat1 outputs) ...
    "other-cat1",
```

Verify the exact insertion point by anchoring on `"audit-citation"`:

```
grep -n "audit-citation" tools/integrity/integrity/snapshot.py
```

Place the three new entries on the lines immediately following.

### 4.4 Catalog sections

Edit `tools/integrity/docs/grandfather-catalog.md`. Add three new
sections in the cat1 block, structurally mirroring `audit-citation`
and `audit-bare-path`. Land each with NUMERIC inline counts per
Decision 4. Counts come directly from probe § E.2:

```markdown
### `toolkit-doc-snapshot` (5)

**Pattern:** `cat1.intra-repo` findings in `tools/integrity/docs/**`,
`docs/integrity-toolkit-spec.md`, or `tools/integrity/README.md`.

**Why grandfathered:** Toolkit-internal documentation references
toolkit-tracked file paths as documentation convention. Treating these
as `cat1.intra-repo` violations and grandfathering under
`other-cat1` was the pre-v1.3 behavior; v1.3 names them under their
own permanent category for clarity and to drain the `other-cat1`
fallthrough pool.

**Future treatment:** Permanent suppression. New toolkit-doc citations
may continue to cite intra-repo paths without explicit grammar
annotation; the grandfather sweep absorbs them on next run.

### `project-state-snapshot` (0)

**Pattern:** `cat1.intra-repo` findings in `project-state.md` (repo
root).

**Why grandfathered:** `project-state.md` is the cross-phase snapshot
narrative document; it references intra-repo file paths as a
documentation convention. Per v1.1 batch-1 post-retro landing audit
§ D.3, this category was sketched then; v1.3 lands it.

**Future treatment:** Permanent suppression. The count is currently 0
because all prior `cat1.intra-repo` findings on `project-state.md`
either resolved (via concurrent commits to project-state.md) or were
re-attributed when A.3 introduced `cat1.bare-path`. The category is
forward-compatible for any future findings.

**Tracked observation:** Three `integrity-allow:` fossil annotations on
`project-state.md` at lines 559, 593, 666 bear the `other-cat1` reason
string but have no backing findings. Bank for v1.3 part-C hygiene
cleanup; do not address in T1.1.

### `retro-doc-snapshot` (6)

**Pattern:** `cat1.intra-repo` findings in `docs/retro/**`.

**Why grandfathered:** Retro documents narrate cross-batch history
and cite repo file paths in prose. The post-retro landing audit's
sweep-companion observed retro-doc findings falling through to
`other-cat1`; v1.3 names them.

**Future treatment:** Permanent suppression. New retros (e.g., v1.3
batch-1 part-A retro currently being drafted by the parallel session)
may cite intra-repo paths; the grandfather sweep absorbs them.
```

### 4.5 New tests

Add to `tools/integrity/tests/test_grandfather_sweep.py`. Mirror the
existing classifier-rule test pattern (probe § G.2):

```python
class TestT11ClassifierRules:
    """v1.3 T1.1 — three new permanent cat1.intra-repo categories."""

    def test_toolkit_doc_snapshot_routes_tools_integrity_docs(self):
        from integrity.grandfather import classify
        from integrity.common.results import Finding, FailureMode
        f = Finding(
            check_id="cat1.intra-repo",
            mode=FailureMode.HARD_FAIL,
            file="tools/integrity/docs/algebraic/d3q19.md",
            line=175,
            message="some bare path citation",
        )
        c = classify(f)
        assert c.category == "toolkit-doc-snapshot"

    def test_toolkit_doc_snapshot_routes_integrity_spec(self):
        from integrity.grandfather import classify
        from integrity.common.results import Finding, FailureMode
        f = Finding(
            check_id="cat1.intra-repo",
            mode=FailureMode.HARD_FAIL,
            file="docs/integrity-toolkit-spec.md",
            line=1,
            message="some citation",
        )
        c = classify(f)
        assert c.category == "toolkit-doc-snapshot"

    def test_toolkit_doc_snapshot_routes_readme(self):
        from integrity.grandfather import classify
        from integrity.common.results import Finding, FailureMode
        f = Finding(
            check_id="cat1.intra-repo",
            mode=FailureMode.HARD_FAIL,
            file="tools/integrity/README.md",
            line=1,
            message="some citation",
        )
        c = classify(f)
        assert c.category == "toolkit-doc-snapshot"

    def test_project_state_snapshot_routes(self):
        from integrity.grandfather import classify
        from integrity.common.results import Finding, FailureMode
        f = Finding(
            check_id="cat1.intra-repo",
            mode=FailureMode.HARD_FAIL,
            file="project-state.md",
            line=559,
            message="some citation",
        )
        c = classify(f)
        assert c.category == "project-state-snapshot"

    def test_retro_doc_snapshot_routes(self):
        from integrity.grandfather import classify
        from integrity.common.results import Finding, FailureMode
        f = Finding(
            check_id="cat1.intra-repo",
            mode=FailureMode.HARD_FAIL,
            file="docs/retro/integrity-toolkit-v1.1-batch1.md",
            line=332,
            message="some citation",
        )
        c = classify(f)
        assert c.category == "retro-doc-snapshot"

    def test_new_rules_dont_match_unrelated_paths(self):
        from integrity.grandfather import classify
        from integrity.common.results import Finding, FailureMode
        # A live-source path should still fall through to other-cat1
        f = Finding(
            check_id="cat1.intra-repo",
            mode=FailureMode.HARD_FAIL,
            file="common/common-cpp/src/widget.cpp",
            line=10,
            message="some citation",
        )
        c = classify(f)
        assert c.category == "other-cat1"

    def test_new_categories_in_known_categories(self):
        from integrity.snapshot import _KNOWN_CATEGORIES
        assert "toolkit-doc-snapshot" in _KNOWN_CATEGORIES
        assert "project-state-snapshot" in _KNOWN_CATEGORIES
        assert "retro-doc-snapshot" in _KNOWN_CATEGORIES
```

7 new tests total.

### 4.6 Verification — commit 2

```bash
# Tests pass.
cd tools/integrity && python3 -m pytest tests/ -v --tb=short
# Expected: all tests pass. +7 new tests (total +13 from T1.2 + T1.1).

# Grandfather-report shows the new categories.
cd /home/otacon/Projects/GPU-Sims/GPU-Sims  # NOTE: use repo root,
# whatever that is on the execution host. If this hardcoded path
# doesn't exist, use `cd "$(git rev-parse --show-toplevel)"` instead.
python3 -m integrity --grandfather-report --no-history-append 2>&1 | \
  grep -E "toolkit-doc-snapshot|project-state-snapshot|retro-doc-snapshot|other-cat1\s"
# Expected: each new category renders with its count;
# toolkit-doc-snapshot: 5, project-state-snapshot: 0, retro-doc-snapshot: 6.
# other-cat1 count should have dropped by 11 (36 → 25).

# Auto-refresh dry-run shows zero drift on the new categories.
python3 tools/integrity/scripts/refresh_catalog_counts.py --dry-run
# Expected: no changes proposed for the three new categories
# (numeric counts already match live grandfather-report).
# If a drift is reported, pause-and-surface — the spec-time counts
# disagree with current state.

# Sweep companion dry-run.
python3 tools/integrity/scripts/grandfather_sweep.py --dry-run 2>&1 | \
  head -30
# Expected: zero proposed annotations beyond the 11 enumerated in
# Decision 7 (probe § E.2). If extras, pause-and-surface.

# Gate state.
python3 -m integrity --mode strict --no-audit-log
# Expected: hard-fail count = pre-commit baseline (44 from probe
# baseline). The new rules re-classify existing suppressed findings
# from other-cat1 to named categories; they don't introduce new
# hard-fails or remove existing ones.
```

### 4.7 Run the sweep companion

After all verification passes:

```bash
python3 tools/integrity/scripts/grandfather_sweep.py
```

The sweep should annotate exactly the 11 findings enumerated in
Decision 7. Confirm via:

```bash
git diff --name-only
```

Expected output (sweep edits only):
```
docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
docs/retro/integrity-toolkit-v1.1-batch1.md
tools/integrity/docs/algebraic/d3q19.md
tools/integrity/docs/grandfather-catalog.md
tools/integrity/docs/ground-truth-sources.md
```

5 files modified. If `git diff --name-only` shows additional files,
the sweep over-reached — pause-and-surface and revert.

Note: `grandfather-catalog.md` should appear in the sweep's diff because
it both gained the new sections (via your manual edit in § 4.4) AND
gets one new annotation on line 199 per Decision 7's enumeration. The
annotation is on a different line than the new sections, so both edits
compose cleanly.

### 4.8 Commit message — commit 2

```
feat(integrity): three new classifier rules + catalog sections (T1.1, v1.3 commit 2)

Adds three named permanent-suppression categories for cat1.intra-repo
findings on snapshot-style documents:
  - toolkit-doc-snapshot (5 current findings)
  - project-state-snapshot (0 current; forward-compatible)
  - retro-doc-snapshot (6 current findings)

11 findings re-classify from other-cat1 to the new named categories.
Extends snapshot.py:_KNOWN_CATEGORIES with the three new entries
(structurally-invisible secondary touch per A.2 commit-4 banked
observation; explicitly called out in this spec's Decision 8).

Catalog sections land with numeric inline counts; T1.3's auto-refresh
dry-run verifies zero drift.

Sweep companion lands 11 integrity-allow annotations on the
re-classified findings per Decision 7's enumerated list. No
out-of-scope sweeping.

project-state.md fossil annotations at lines 559, 593, 666 banked for
v1.3 part-C hygiene cleanup per Decision 6.

Per v1.3 roadmap § 4 T1.1 and v1.1 batch-1 post-retro landing audit
§ D.3.
```

### 4.9 Commit 2 audit report

`docs/diagnostics/_audits/integrity_v1_3_part_b_commit2_landing_2026-05-16.md`.
Same A-E structure as commit 1's audit report. § C verification block
includes the grandfather-report counts, the sweep dry-run diff, and
the auto-refresh dry-run result verbatim.

## 5. Commit 3 — SHA back-fill

After commits 1 and 2 land, edit the two audit reports' SHA placeholders
with actual values. Per Convention #12: separate commit, never
`--amend`.

### 5.1 Identify placeholders

```bash
grep -l "<COMMIT_[12]_SHA>" docs/diagnostics/_audits/integrity_v1_3_part_b_commit*_landing_2026-05-16.md
```

If the audit reports were drafted with placeholder SHAs, this grep
finds them. Replace each with the actual SHA.

### 5.2 Robust replacement (avoid HEAD~N fragility)

The parallel session's part-A spec used `HEAD~3 / HEAD~2 / HEAD~1`
which is fragile under rebase. Safer pattern: read SHAs from git log
by subject match:

```bash
COMMIT_1_SHA=$(git log --grep "T1.2" --grep "v1.3 commit 1" --all-match \
  --format="%H" -n 1)
COMMIT_2_SHA=$(git log --grep "T1.1" --grep "v1.3 commit 2" --all-match \
  --format="%H" -n 1)

# Sanity check both resolved.
test -n "$COMMIT_1_SHA" && test -n "$COMMIT_2_SHA" || {
    echo "PAUSE: SHA lookup failed"
    exit 1
}

# Replace placeholders.
sed -i "s/<COMMIT_1_SHA>/$COMMIT_1_SHA/g" \
  docs/diagnostics/_audits/integrity_v1_3_part_b_commit*_landing_2026-05-16.md
sed -i "s/<COMMIT_2_SHA>/$COMMIT_2_SHA/g" \
  docs/diagnostics/_audits/integrity_v1_3_part_b_commit*_landing_2026-05-16.md
```

### 5.3 Verification — commit 3

```bash
# No placeholders remain.
grep -l "<COMMIT_[12]_SHA>" docs/diagnostics/_audits/integrity_v1_3_part_b_commit*_landing_2026-05-16.md
# Expected: empty output.

# Every cited SHA resolves.
for sha in $(grep -oE '\b[a-f0-9]{7,40}\b' \
  docs/diagnostics/_audits/integrity_v1_3_part_b_commit*_landing_2026-05-16.md \
  | sort -u); do
    git cat-file -e "$sha" 2>/dev/null && echo "OK $sha" || echo "BAD $sha"
done
# Expected: all "OK"; if any "BAD", pause-and-surface.

# Gate state unchanged.
python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
# Expected: hard-fail count = post-commit-2 state.
```

### 5.4 Commit message — commit 3

```
docs(integrity): SHA back-fill for v1.3 part-B commits 1-2 (v1.3 commit 3)

Replace <COMMIT_N_SHA> placeholders in the part-B commit audit reports
with the actual SHAs. Per Convention #12, back-fill is a separate
follow-up commit, never --amend.
```

## 6. Cross-cutting concerns

### 6.1 Coordination

No active overlap. The parallel session is drafting their part-A retro,
which is a doc-only effort under `docs/retro/`. Their retro's existence
does not affect this batch's edit surface. If a part-A retro lands
between commits 1 and 2 here, it will trigger the inline sweep
companion in commit 2 to pick up any new `cat1.intra-repo` findings on
the new retro — which will now route to `retro-doc-snapshot`
(handled by T1.1). Clean composition.

### 6.2 Backward compatibility

- `FALLTHROUGH_CATEGORIES` and `is_fallthrough_category()` are new
  public symbols in `grandfather.py`. Existing call sites continue to
  work unchanged because `apply_annotations`'s refactor is
  behaviorally-identical to the literal-string match.
- Existing classifier rules are not touched; T1.1 only adds new rules.
- `_KNOWN_CATEGORIES` is append-only (in spirit); new entries are added
  before `"other-cat1"` to preserve substring-match semantics.
- Existing tests pass unchanged.

### 6.3 CI behavior

After commit 1: no behavioral change.
After commit 2: 11 findings re-classify; `other-cat1` count drops from
36 to 25; three new named-category counts appear (5, 0, 6). Gate
hard-fail count unchanged. Test suite grows by 13.

### 6.4 Idempotence

- Commit 1: refactor is structurally idempotent; re-running produces
  the same code.
- Commit 2: classifier rule additions are append-only; running
  `classify()` on any finding produces the same Classification.
- Commit 3: SHA back-fill is one-shot but doing it twice is harmless
  (placeholders already gone).

## 7. Pre-execution checklist

- [ ] HEAD SHA is `1f7785f` OR a descendant that hasn't modified
  `grandfather.py`, `snapshot.py`, `grandfather-catalog.md`, or
  `test_grandfather_sweep.py`. If HEAD has drifted on these,
  re-anchor.
- [ ] `python3 -m integrity --mode strict --no-audit-log` exits 1
  with ~44 hard-fails (the post-part-A baseline).
- [ ] `pytest tools/integrity/tests/ -q` reports all tests passing
  (157 expected at probe time = 153 from A.2 + 17 from T1.3 - some
  test count migrations).
- [ ] T1.3's auto-refresh script exists at
  `tools/integrity/scripts/refresh_catalog_counts.py` (landed in
  part-A commit 65a7685).
- [ ] No uncommitted local changes.

## 8. Out of scope

- Project-state.md fossil annotation cleanup (Decision 6 — banked).
- Any classifier rule beyond the three listed (e.g., a hypothetical
  `phase-doc-snapshot` for `docs/phase*.md`). The three named here
  cover the immediate `other-cat1` pool drain; future drains land in
  future batches.
- Auto-refresh of catalog counts at sweep-time (still v1.3+ candidate
  per part-A retro / roadmap T2 tier).
- Method-level public symbol scanning (still v1.3 candidate from A.2).
- T2.x items.

## 9. Self-review checks Claude Code should run

Before declaring this batch complete:

**Check 1 — Decision 7 sweep-diff exactness.** The post-commit-2
sweep's dry-run diff must match the enumerated 11-finding list in
Decision 7. If extras appear, the rules are too broad.

**Check 2 — Decision 8 `_KNOWN_CATEGORIES` extension.** Confirm the
three new entries are present in `snapshot.py` AND the grandfather-
report renders each new category with a count.

**Check 3 — Decision 4 catalog-count consistency.** The auto-refresh
`--dry-run` after commit 2 reports zero drift. If drift, the inline
counts disagree with live state.

**Check 4 — Refactor zero-behavior-change.** The strict-mode gate's
hard-fail count is unchanged from pre-commit-1 to post-commit-1.
Re-run if necessary.

**Check 5 — SHA citations in audit reports resolve.** Every SHA in
the commit-1, commit-2, and commit-3 audit reports resolves via
`git cat-file -e`.

If any check fails, pause-and-surface. Addenda not body edits per
Convention F.

## 10. Acceptance criteria summary

| Item | Done when |
|---|---|
| T1.2 helper exists | `is_fallthrough_category` and `FALLTHROUGH_CATEGORIES` are importable from `integrity.grandfather` |
| T1.2 filter refactored | `apply_annotations` uses `is_fallthrough_category()` instead of literal match |
| T1.1 three rules registered | `classify()` routes `cat1.intra-repo` on the three predicate sets to the three new categories |
| T1.1 catalog populated | Three new sections with numeric counts 5, 0, 6 |
| T1.1 `_KNOWN_CATEGORIES` extended | Grandfather-report shows each new category by name |
| Sweep companion landed | 11 annotations on exactly the 5 enumerated files |
| Tests pass | +13 new tests; full suite green |
| Gate baseline preserved | 44 hard-fails (unchanged from probe baseline) |
| All 5 self-review checks pass | per § 9 |

## End of execution spec

Three commits + three audit reports + back-fill = 4 commits total.
Total diff: ~155 LOC across code + tests + catalog + audit reports.

Spec length: drafted in ~900 lines; target was 800-1100 per the budget
discipline. If this comment block is materially longer than intended,
the budget held; if shorter, the spec under-specified somewhere.
Verify against the file's actual line count and flag in the commit-1
audit report § E if there's significant deviation.

The two most fabrication-vulnerable claims in this spec:

- **Decision 7's enumeration** of 11 findings on 5 files. If the
  current `other-cat1` set has drifted since the probe (e.g., a
  concurrent commit resolved one finding or introduced another), the
  sweep companion won't match. Re-derive the count via the probe's
  § E.2 enumeration script if needed.

- **Decision 8's `_KNOWN_CATEGORIES` placement order**. The tuple
  uses substring matching for reason-string extraction; placing a
  new entry after a substring-overlap would mis-classify. Verify by
  running grandfather-report and confirming each new category renders
  with the correct count.

Both are caught by § 9 self-review checks.
