---
title: Integrity toolkit v1.3 closeout pre-spec probe
author: architect-1 (via claude-code)
date: 2026-05-17
type: pre-spec-probe
scope: integrity toolkit v1.3 closeout batch
status: read-only — disk untouched outside this report
inputs:
  - HEAD a1c9121
  - integrity --mode strict --no-audit-log (1 invocation)
  - pytest --collect-only (1 invocation)
  - git, grep, read-only file inspection
---

# Integrity toolkit v1.3 closeout pre-spec probe (read-only)

Probe-only report — no edits, no commits, no script execution beyond
the integrity gate and pytest collection. Tags: FACT = directly
observed; INFERENCE = derived.

---

## § A — Baseline state

### A.1 — HEAD SHA (FACT, verbatim)

```
a1c912159d3c946aee7d33b06b36fd265d63d878
```

### A.2 — Gate summary (FACT, verbatim — last 5 lines from strict mode)

```
  HARD_FAIL: cat1.annotation-form at docs/diagnostics/_audits/integrity_v1_3_part_b_commit2_landing_2026-05-16.md:175
    grammar mismatch in '` lines carrying the'
  HARD_FAIL: cat1.annotation-form at docs/diagnostics/_audits/integrity_v1_3_part_b_spec_2026-05-16_architect1.md:140
    grammar mismatch in '` annotations on `project-state.md` at lines'
  HARD_FAIL: cat1.annotation-form at tools/integrity/docs/grandfather-catalog.md:86
    grammar mismatch in '` fossil annotations on'
  HARD_FAIL: cat1.unregistered-upstream at tools/integrity/tests/test_cat1_bare_path.py:62
    SimUpstream 1.0.0 SimUpstream/TimeStep.cpp:42: upstream 'SimUpstream' is not in the registry at tools/integrity/docs/ground-truth-sources.md
```

Top-line summary (FACT, verbatim — first line of the same run):

```
integrity: 5 pass, 0 soft-warn, 60 hard-fail, 1263 suppressed
```

Exit code: `1` (gate red). 60 hard-fails matches the Part-B retro § 1
baseline (FACT — confirmed).

### A.3 — Test count baseline (FACT, verbatim — last 3 lines)

```
no tests ran in 0.05s

==================================================================== 183 tests collected in 0.08s ===================================================================
```

(The first "no tests ran" line is a stderr/footer artifact of
`--collect-only -q` combined with the harness; the count line confirms
**183 tests collected**, matching Part-B retro § 1 baseline.)

### A.4 — Recent history (FACT, verbatim, `git log --oneline -10`)

```
a1c9121 docs(retro): integrity-toolkit v1.3 batch-1 part-B retrospective
67e19c1 docs(integrity): SHA back-fill for v1.3 part-B commits 1-2 (v1.3 part-B commit 3)
710ac93 feat(integrity): three new classifier rules + catalog sections (T1.1, v1.3 part-B commit 2)
239d7a2 feat(integrity): module-level FALLTHROUGH_CATEGORIES + helper (T1.2, v1.3 part-B commit 1)
edc28d1 docs(retro): integrity toolkit v1.3 batch-1 part-A retro
1f7785f docs(integrity): SHA back-fill for v1.3 batch-1 part-A commits 1-3 (v1.3 commit 4)
9e3afa9 docs(integrity): T1.4 probe template conventions doc (v1.3 commit 3)
72a2d26 refactor(integrity): T1.5 cat3 expected-values TOML -> JSON (v1.3 commit 2)
65a7685 feat(integrity): T1.3 catalog auto-refresh script (v1.3 commit 1)
9f527dc docs(audits): A.2 commit-4 audit Addendum A -- post-landing +1 gate drift
```

No commit message contains "v1.3 closeout" (FACT — verified by visual scan).

---

## § B — Per-surface verification

### B.1 — `tools/integrity/integrity/grandfather.py`

- **LOC:** 526 (FACT).
- **Landmarks** (FACT, verbatim from `grep -n ...`):

```
23:@dataclass(frozen=True)
31:@dataclass(frozen=True)
53:SWEEPABLE_PATH_PREFIXES: tuple[str, ...] = (
60:SWEEPABLE_EXACT_PATHS: frozenset[str] = frozenset({
76:FALLTHROUGH_CATEGORIES: frozenset[str] = frozenset({
82:def is_fallthrough_category(category: str) -> bool:
92:def is_live_source_path(file_path: str) -> bool:
110:def classify(finding: Finding) -> Classification:
275:def comment_form_for(file_path: str) -> str:
303:def comment_form_for_md_inside_fence(fence_lang: str | None) -> str:
322:def annotation_already_present(prev_line: str, check_id: str) -> bool:
338:def render_annotation_line(
385:def collect_findings(repo_root: Path) -> list[Finding]:
414:def group_findings_by_target(
425:def apply_annotations(
```

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **`Classification` dataclass (FACT, verbatim, grandfather.py:31-36):**

```python
@dataclass(frozen=True)
# integrity-allow: cat2.public-symbol-used-toolkit; pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused); n/a
class Classification:
    category: str
    reason: str
    issue_ref: str
```

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **`Finding` dataclass for context (FACT, verbatim, grandfather.py:23-28):**

```python
@dataclass(frozen=True)
class Finding:
    check_id: str
    file: str
    line: int
    message: str
```

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **`classify()` signature (FACT, verbatim, grandfather.py:110):**

```python
def classify(finding: Finding) -> Classification:
```

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **`apply_annotations()` signature (FACT, verbatim, grandfather.py:425-430):**

```python
def apply_annotations(
    repo_root: Path,
    dry_run: bool,
    sweep_live_source: bool = False,
    force_sweep_categories: frozenset[str] = frozenset(),
) -> tuple[int, int, dict[str, int], int]:
```

- **`annotation_already_present()` signature + body (FACT, verbatim,
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  grandfather.py:322-334):**

```python
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
```

- **Final `return Classification(category="other-cat1", ...)` in
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  `classify()` (FACT, verbatim, grandfather.py:268-272):**

```python
    return Classification(
        category="other-cat1",
        reason="grandfathered-pre-v1 (see grandfather-catalog other-cat1)",
        issue_ref="n/a",
    )
```

- **Top-level `is_suppressed` helper:** does **NOT** exist
  (FACT — searched with `grep -n is_suppressed`; only matches are the
  word "suppressed" appearing inside comments and in `collect_findings`
  body at line 402 which reads `f.get("suppressed")` from the toolkit's
  own JSON output).

- **`Finding.suppressed: bool` field on the dataclass:** does **NOT**
  exist (FACT — see verbatim `Finding` quote above; fields are
  `check_id`, `file`, `line`, `message` only). The `suppressed`
  property is read from the JSON-shape `dict` returned by
  `integrity --output json` inside `collect_findings()` at
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  `grandfather.py:402`, but the parsed `Finding` instance carries no
  such field.

- **INFERENCE flag** for spec § 3.2 wiring: if the closeout spec
  assumes either `is_suppressed(finding)` top-level OR `Finding.suppressed:
  bool`, both assumptions are currently false. Adding one of them is
  a real diff (either widen `Finding` to include a `suppressed: bool`
  with default `False`, or define `is_suppressed()` against the JSON
  dict the runner emits). See § G.1 for the flag.

### B.2 — `tools/integrity/scripts/grandfather_sweep.py`

- **LOC:** 61 (FACT).
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **`main()` body (FACT, verbatim, grandfather_sweep.py:14-57):**

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
    parser.add_argument(
        "--force-sweep-category",
        action="append",
        default=[],
        metavar="CATEGORY",
        help=(
            "Force-sweep findings classified into the given category, "
            "regardless of LIVE-SOURCE protection. Repeatable. Example: "
            "--force-sweep-category toolkit-own-unused. Use sparingly -- "
            "this opts a single named category out of the P1.8 live-source "
            "attribution-not-sweep policy, leaving all other LIVE-SOURCE "
            "categories protected."
        ),
    )
    ns = parser.parse_args(argv)

    root = ns.repo_root if ns.repo_root else find_repo_root()
    files, anns, counts, live_source_skipped = apply_annotations(
        root, ns.dry_run,
        sweep_live_source=ns.sweep_live_source,
        force_sweep_categories=frozenset(ns.force_sweep_category),
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

### B.3 — `tools/integrity/integrity/cat2_contracts/stack_c.py`

- **LOC:** 531 (FACT).
- **Landmarks** (FACT, verbatim, filtered to `def `/parse/Index.create):

```
55:def _load_compile_args(repo_root: Path, source_file: Path) -> list[str]:
112:def _find_representative_tu(repo_root: Path) -> Path | None:
129:def extract_public_surface(repo_root: Path) -> list[PublicSymbol]:
150:    index = clang.cindex.Index.create()
154:            tu = index.parse(
176:def _representative_tus(repo_root: Path) -> list[Path]:
192:def _walk_for_public_decls(
316:def _qualified_name(cursor) -> str:
327:def find_references(
353:    index = clang.cindex.Index.create()
359:            tu = index.parse(str(source), args=args)
372:def _find_matching_field_at_token(
406:def _collect_field_token_refs(
459:def _collect_refs(
506:def discover_consumer_sources(repo_root: Path) -> list[Path]:
```

- **First `Index.create()` / `index.parse(...)` call site (FACT,
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  verbatim, stack_c.py:148-160):**

```python
    symbols: list[PublicSymbol] = []
    seen_usrs: set[str] = set()
    index = clang.cindex.Index.create()
    for tu_source in tu_sources:
        args = _load_compile_args(repo_root, tu_source)
        try:
            tu = index.parse(
                str(tu_source), args=args,
                options=clang.cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD,
            )
        except clang.cindex.TranslationUnitLoadError:
            continue
        _walk_for_public_decls(tu.cursor, public_dir, symbols, seen_usrs, class_stack=[])
```

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  Containing function: **`extract_public_surface()`** (stack_c.py:129).

- **Second `Index.create()` / `index.parse(...)` call site (FACT,
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  verbatim, stack_c.py:351-363):**

```python
    refs: dict[str, list[tuple[Path, int]]] = {usr: [] for usr in target_usrs}

    index = clang.cindex.Index.create()
    for source in consumer_sources:
        if not source.is_file():
            continue
        args = _load_compile_args(repo_root, source)
        try:
            tu = index.parse(str(source), args=args)
        except clang.cindex.TranslationUnitLoadError:
            continue

        _collect_refs(tu.cursor, target_usrs, refs, class_stack=[])
        if field_targets_by_name:
            _collect_field_token_refs(
                tu, source, field_targets_by_name, refs,
            )
```

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  Containing function: **`find_references()`** (stack_c.py:327).

- **Function called by `public_symbol_used_c.py`'s `run()`:** both
  `extract_public_surface(repo_root)` and
  `find_references(repo_root, public_symbols, consumer_sources)`
  (FACT — see B.4 below for the verbatim `run()` showing this).
  These are the two functions that internally re-instantiate
  `clang.cindex.Index.create()` — once each per call, so each
  `cat2.public-symbol-used-c` run creates **two** libclang Index
  instances and parses TUs across both. This is the perf-relevant
  shape the closeout spec's commit-2 perf assertion will baseline.

### B.4 — `tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_c.py`

`run()` body (FACT, verbatim — full file is small; entry point
is `def run(repo_root: Path) -> list[Finding]` at line 40):

```python
def run(repo_root: Path) -> list[Finding]:
    findings: list[Finding] = []

    if not (repo_root / BUILD_COMPILE_COMMANDS).is_file():
        return findings

    try:
        public_symbols = extract_public_surface(repo_root)
    except RuntimeError:
        return findings

    if not public_symbols:
        return findings

    consumer_sources = discover_consumer_sources(repo_root)
    refs_by_usr = find_references(repo_root, public_symbols, consumer_sources)

    for symbol in public_symbols:
        sites = refs_by_usr.get(symbol.usr, [])
        if sites:
            continue

        try:
            rel = str(symbol.defining_file.relative_to(repo_root))
        except ValueError:
            rel = str(symbol.defining_file)

        findings.append(Finding(
            check_id=CHECK_ID,
            mode=MODE,
            file=rel,
            line=symbol.defining_line,
            message=(
                f"public {symbol.kind.value} '{symbol.qualified_name}' has "
                f"no non-self consumer site under common-cpp/src, examples, "
                f"or per-sim Stack C source"
            ),
        ))

    return findings
```

Module-level constants the runner reads (FACT, from the same file):

```python
CHECK_ID = "cat2.public-symbol-used-c"
MODE = FailureMode.HARD_FAIL
```

Graceful-degrade path (no `build/compile_commands.json`) returns empty
findings without invoking libclang. This is load-bearing for the
closeout-spec assumption that the perf assertion can run in a no-CI
local context (FACT — confirmed by reading lines 41-43).

### B.5 — `.github/workflows/integrity.yml`

Full verbatim (102 lines, FACT — file ends at the blank line after
`if-no-files-found: ignore`):

```yaml
name: Integrity

on:
  push:
    branches: [main]
  pull_request:
  workflow_dispatch:

permissions:
  contents: read
  pull-requests: write

concurrency:
  group: integrity-${{ github.ref }}
  cancel-in-progress: true

jobs:
  integrity:
    name: Cross-stack integrity checks
    runs-on: ubuntu-24.04
    timeout-minutes: 10

    steps:
      - name: Checkout
        uses: actions/checkout@v4
        with:
          submodules: false
          fetch-depth: 1

      - name: Clone vendored references (anchor-pinned)
        run: |
          mkdir -p references
          git clone --no-checkout https://github.com/InteractiveComputerGraphics/SPlisHSPlasH.git references/SPlisHSPlasH
          git -C references/SPlisHSPlasH checkout 6bff55a6eaf14083d34650f22a268ce156b62b54

      - name: Set up Python 3.11
        uses: actions/setup-python@v5
        with:
          python-version: '3.11'

      - name: Set up Node.js 22
        uses: actions/setup-node@v4
        with:
          node-version: '22'

      - name: Install workspace deps (for Stack B module resolution)
        run: npm install --silent

      - name: Install + build TS helper for Stack B integrity check
        working-directory: tools/integrity/integrity/cat2_contracts/ts_helper
        run: |
          npm install --silent
          npx tsc --project tsconfig.json

      - name: Install integrity toolkit
        working-directory: tools/integrity
        run: |
          python -m pip install --upgrade pip
          pip install -e .[dev]

      - name: Install build dependencies (for compile_commands.json)
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            build-essential \
            cmake \
            ninja-build \
            pkg-config \
            libimath-dev \
            libvulkan-dev \
            vulkan-validationlayers \
            libgl1-mesa-dev \
            libxinerama-dev \
            libxcursor-dev \
            libxi-dev \
            libxrandr-dev \
            libwayland-dev \
            libxkbcommon-dev

      - name: Configure Stack C build (for compile_commands.json + Cat 3 driver)
        run: |
          cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
            -DGPU_SIMS_BUILD_INTEGRITY_CAT3=ON

      - name: Build Cat 3 Stack C driver
        run: ninja -C build integrity_cat3_stack_c

      - name: Run integrity toolkit's own tests (dogfood)
        working-directory: tools/integrity
        run: pytest tests/ -v --cov=integrity

      - name: Run integrity toolkit against repo
        run: python -m integrity --output github

      - name: Upload audit log
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: integrity-audit-log
          path: docs/diagnostics/_audits/integrity_failures_*.md
          if-no-files-found: ignore
```

- **`fetch-depth` value on the checkout step:** `1` (FACT,
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  `integrity.yml:28`).

- **Implication for closeout commit-4 § 5.C.2 audit-prose freshness
  scoping:** with `fetch-depth: 1`, `git log` only sees the latest
  commit on the checked-out ref. `git fetch origin <SHA>` cannot
  resolve historical SHAs at all on PRs without explicit deepening
  (`fetch-depth: 0` or a runtime `git fetch --depth=N`). The
  PR-diff-range pattern `git diff <base-sha>...<head-sha>` is not
  available out of the box and needs an explicit fetch step. See § E
  below.

### B.6 — `tools/integrity/integrity/common/annotations.py`

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **`ANNOTATION_RE` (FACT, verbatim, annotations.py:31-36):**

```python
ANNOTATION_RE = re.compile(
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    r"integrity-allow:\s*(?P<check_id>cat\d+\.[a-z*][a-z0-9.\-*]*)\s*;\s*"
    r"(?P<reason>[^;]{8,}?)\s*;\s*"
    r"(?P<issue_ref>#\d+|n/a)\s*(?:-->)?\s*$"
)
```

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **`parse_annotation_line()` (FACT, verbatim, annotations.py:39-55):**

```python
def parse_annotation_line(text: str) -> tuple[str, str, str] | None:
    """Try to parse an annotation from a single line of comment text.

    Returns (check_id, reason, issue_ref) or None if not an annotation
    or grammar is invalid.

    Commit 1: minimal implementation. Commit 2 expands with grammar
    validation reporting (the cat1.annotation-form check).
    """
    m = ANNOTATION_RE.search(text)
    if not m:
        return None
    return (
        m.group("check_id"),
        m.group("reason").strip(),
        m.group("issue_ref"),
    )
```

- **Observation (INFERENCE):** the `reason` capture `[^;]{8,}?` is
  non-greedy and bound only on lower (≥ 8 chars); `issue_ref` accepts
  `#\d+` or `n/a` and is `$`-anchored after optional `-->`. The
  audit-prose freshness check the closeout spec proposes (commit 4)
  will likely need to call `parse_annotation_line()` on each
  candidate line, then re-classify against current findings. Any
  spec change here must keep `parse_annotation_line()`'s signature
  stable; downstream callers depend on it.

### B.7 — `tools/integrity/integrity/snapshot.py`

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **`_KNOWN_CATEGORIES` (FACT, verbatim, snapshot.py:27-55):**

```python
_KNOWN_CATEGORIES = (
    "audit-citation",
    # v1.3 T1.1 -- cat1.intra-repo snapshot-doc categories. Grouped with
    # audit-citation (the existing cat1.intra-repo classifier output).
    # Placed before "other-cat1" to preserve fall-through semantics for
    # the substring-matched extraction.
    "toolkit-doc-snapshot",
    "project-state-snapshot",
    "retro-doc-snapshot",
    "live-shader-1810",
    "audit-doc-1810",
    "spec-grammar-example",
    "toolkit-own-source",
    "retro-grammar-example",
    "audit-report-grammar-example",
    "cat2-stack-d-unused",
    "cat2-stack-c-unused",
    "cat2-stack-b-unused",
    "cat2-stub-label-stale",
    "toolkit-own-unused",
    # v1.2 A.3 bare-path categories (longer names first so they match
    # before "other-cat1" which is a substring of "other-cat1-bare-path").
    "audit-bare-path",
    "retro-bare-path",
    "toolkit-doc-bare-path",
    "deferred-upstream-bare-path",
    "other-cat1-bare-path",
    "other-cat1",
)
```

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **`_REASON_PATTERNS` (FACT, verbatim, snapshot.py:58-69):**

```python
_REASON_PATTERNS: tuple[tuple[str, str], ...] = (
    # Reason substrings that uniquely identify a category for the
    # entries whose classifier reason does not include the category name.
    ("regex or docstring literal of the annotation grammar", "toolkit-own-source"),
    ("audit-doc literal mention of the annotation grammar", "audit-report-grammar-example"),
    ("documentation-only literal mention of the annotation grammar",
     "spec-grammar-example"),
    ("retrospective-doc literal mention of the annotation grammar",
     "retro-grammar-example"),
    ("historical 1.8.10 fabrication", "audit-doc-1810"),
    ("stale phase-n stub label", "cat2-stub-label-stale"),
)
```

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **`_extract_category()` (FACT, verbatim, snapshot.py:72-81):**

```python
def _extract_category(reason: str) -> str:
    """Match the suppression_reason text to a known category."""
    lowered = (reason or "").lower()
    for cat in _KNOWN_CATEGORIES:
        if cat in lowered:
            return cat
    for pattern, category in _REASON_PATTERNS:
        if pattern in lowered:
            return category
    return "other"
```

- **Observation (INFERENCE):** `_KNOWN_CATEGORIES` is a fixed
  ordered tuple with the longer-name-before-substring discipline
  pinned via comments. Closeout work that introduces new categories
  (e.g., the spec may add an `audit-prose-stale` bucket per § C.2's
  Convention F) must register them here AND honor the ordering
  invariant. There's no unit test pinning the tuple membership,
  unlike `FALLTHROUGH_CATEGORIES` which is pinned by
  `test_fallthrough_categories_contents` per a docstring comment in
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  grandfather.py:74.

### B.8 — `project-state.md`

- **LOC:** 888 (FACT).

- **Line numbers 559 / 593 / 666:** all three are `cat1.intra-repo`
  `other-cat1` annotations as the spec asserts (FACT, `grep -n
  "integrity-allow.*other-cat1" project-state.md`):

```
559:<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
560:<!-- integrity-allow: cat1.bare-path; bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog); n/a -->
593:<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
594:<!-- integrity-allow: cat1.bare-path; bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog); n/a -->
666:<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
667:<!-- integrity-allow: cat1.bare-path; bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog); n/a -->
```

**No drift** — annotation lines match the spec's 559/593/666 anchors
exactly. Each is followed immediately by a `cat1.bare-path`
`other-cat1-bare-path` annotation at the next line (560/594/667).

- **5-line windows around each annotation pair (FACT, verbatim):**

  **Lines 555-565 (around 559-560):**

```
### Comments asserting platform or library behavior must cite verification source

Comments that assert non-obvious platform or library behavior (coordinate orientations, byte conventions, GPU driver quirks, OS-specific paths, library-internal contracts) must cite the verification source: an inline sandbox probe, a verification report at file:line, or a documented Claude Code escape-hatch test. "Verbatim inherited from prior sim" is not sufficient — the prior sim's working behavior may rely on positional symmetries that don't transfer. Spec drafters author these comments with citations; architect-2 review catches missing ones; Claude Code execution preserves them.

<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
<!-- integrity-allow: cat1.bare-path; bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog); n/a -->
Canonical example: Phase 10 polish-4 surfaced two contradictory comments in `main.py` about Taichi GGUI cursor-y origin — line 164 claimed y=0 at bottom, line 203 claimed y=0 at bottom (inherited verbatim from Phase 9 MPM main.py:306-318). Empirical reality on Taichi 1.7.4 / Vulkan / Ubuntu 24.04 is y=0 at TOP. MPM's flip worked despite the wrong comment because MPM's panels sit in a region where the inversion is symmetric. Lenia's paint surface exposed the inconsistency. Banked Phase 10 retro.

### Combined-multi-sim-venv testing for shared-pattern adoption
```

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
Bare-path citation on line 561 is `main.py:306-318` — same defect
class the cat1.bare-path annotation on line 560 covers.

  **Lines 589-600 (around 593-594):**

```
2. **Alembic packaging absence** (pre-draft probe). `libalembic-dev` dropped from Ubuntu 24.04 noble; 1.8.11 incompatible with noble's CMake 3.28.3; apt deps slim to `libimath-dev` only with `USE_HDF5=OFF`; two legacy CMake flags non-existent in 1.8.x; `find_package` resolves post-`FetchContent_MakeAvailable` without `add_subdirectory` fallback.

3. **`StateWriter::saveBuffer` signature** (mid-revision probe). Synced is 4-arg `(name, data, bytes, meta = {})` with count/stride/format/shape in the nlohmann::json meta. Architect-1's 5-arg fabrication and architect-2's 3-arg recall were both wrong; only the probe was right.

<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
<!-- integrity-allow: cat1.bare-path; bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog); n/a -->
4. **Buffer-naming convention** (mid-revision probe). `StateWriter` auto-appends `.bin` at `state_writer.cpp:57`; six of seven shipped sims pass bare names; ES is the lone outlier producing real `velocity.bin.bin` files on disk. Phase 11 follows bare-name; ES bug stays out of scope.
```

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
Bare-path citation on line 595 is `state_writer.cpp:57`.

  **Lines 662-675 (around 666-667):**

```
- **lil-gui `persistKey` + preset-dropdown interaction (Stack B).** ...
### Stack C (common-cpp)

<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
<!-- integrity-allow: cat1.bare-path; bare-path citation (other-cat1-bare-path; review per category in grandfather-catalog); n/a -->
- **Build (native) Debug-job: `createDebugMessenger` name-collision — resolved Phase 8.5.1 (latent since Phase 1).** `common/common-cpp/src/vk/context.cpp:207` ... [renamed sites: `context.hpp:78` declaration, `context.cpp:116` ctor call, `context.cpp:202` definition] ...
```

Line 668 triggers three cat1.bare-path findings:
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`context.hpp:78`, `context.cpp:116`, `context.cpp:202`.

- **Gate output for project-state.md (FACT, verbatim):**

```
  HARD_FAIL: cat1.bare-path at project-state.md:561
  HARD_FAIL: cat1.bare-path at project-state.md:595
  HARD_FAIL: cat1.bare-path at project-state.md:668
  HARD_FAIL: cat1.bare-path at project-state.md:668
  HARD_FAIL: cat1.bare-path at project-state.md:668
```

  **NO `cat1.intra-repo` findings fire on project-state.md** at any
  line (FACT — grep over the full hard-fail list shows zero
  `project-state.md` mentions with `cat1.intra-repo`).

- **Fossil hypothesis interpretation (INFERENCE):**
  - The three `cat1.intra-repo` annotations at 559/593/666 are
    **fossils** (no cat1.intra-repo finding fires against the lines
    they sit above — confirmed). This matches the spec's hypothesis
    exactly.
  - The three `cat1.bare-path` annotations at 560/594/667 are
    **load-bearing in intent**, but the cat1.bare-path findings at
    561/595/668 are **firing as HARD_FAIL anyway**. So either
    (a) suppression is broken for these specific lines, or
    (b) the annotations cover only the line *immediately* below and
    the bare-path target is *two* lines below the cat1.intra-repo
    annotation. See § G.2 for the flag.

- **Section structure (FACT, verbatim, `grep -n "^# \|^## "`):**

```
1:# GPU-Sims — Project State
19:## 1. Project overview
31:## 2. Architectural shape
58:## 3. Phase ledger
85:## 4. Locked architectural decisions
114:## 5. Per-stack package surface area
181:## 6. Per-category status
202:## 7. Conventions
613:## 8. Things explicitly deferred
627:## 9. Known issues
716:## 10. Onboarding prompts
862:## 11. Quick reference
```

  No existing `## v1-closed` or equivalent v1-closeout marker.
  Natural insertion sites: appended to § 9 (Known issues — "v1.x
  integrity closed") or as a new ## 12 / sub-row at the bottom of
  § 3 phase ledger.

- **Phase ledger table — header + last row (FACT, verbatim):**

```
| # | Phase | Scope | Status | Shipped at |
|---|-------|-------|--------|------------|
...
| 10+ | Remaining sims | One phase per remaining sim. Each consumes a settled `common-` package; per-sim phases are smaller than the foundation phases. Per Phase 9 banking, the natural Alembic-real-impl consumer is sph-water (Stack C). The natural cross-stack lenia-fft consumer (Stack D Taichi + Stack B WebGPU) is post-MPM. | ⬜ Not started | — |
```

  Columns: `#`, `Phase`, `Scope`, `Status`, `Shipped at`. Status
  glyphs use `✅`/`⬜`. Shipped-at SHAs are backticked.

### B.9 — `tools/integrity/README.md`

- **LOC:** 64 (FACT).
- **Full file (FACT, verbatim):**

```markdown
# GPU-Sims Integrity Toolkit

Cross-stack verification toolkit per `docs/integrity-toolkit-spec.md`.

## What it checks

- **Category 1: Citation integrity** — every `file:line` citation resolves; upstream citations match vendored references
- **Category 2: Contract verification** — every public API field, function, and declared behavior has matching implementation
- **Category 3: Numerical correctness** — implementations of upstream algorithms match the upstream reference at canonical test points

## Running locally

​```bash
# Install (editable, with dev deps):
pip install -e tools/integrity[dev]

# Run all checks (strict — honors HARD_FAIL):
python3 -m integrity

# Local-development mode (downgrades HARD_FAIL to warnings):
python3 -m integrity --mode warn-only

# Run a single category or check:
python3 -m integrity --cat 1
python3 -m integrity --check cat1.upstream-anchor

# JSON output:
python3 -m integrity --output json

# GitHub Actions annotation output:
python3 -m integrity --output github
​```

## Running the toolkit's own tests

​```bash
pytest tools/integrity/tests/ -v
​```

## On failure

CI failures appear as:

1. GitHub Actions inline PR annotations
2. Entries in `docs/diagnostics/_audits/integrity_failures_<YYYY-MM-DD>.md`

To suppress a finding with an inline annotation (per spec § 3.2):

​```cpp
// integrity-allow: cat1.upstream-anchor; SPlisHSPlasH 1.8.10 anchor pre-v1; #117
​```

See `tools/integrity/docs/failure-modes.md` and `tools/integrity/docs/grandfather-catalog.md` for details.

## Implementation status

- [x] Commit 1: scaffold (this commit)
- [ ] Commit 2: Cat 1 citation parsing + intra-repo resolution
- [ ] Commit 3: Cat 1 upstream-citation + anchor verification
- [ ] Commit 4: grandfather sweep + CI integration
- [ ] Commit 5: Cat 2 Stack D
- [ ] Commit 6: Cat 2 Stack C
- [ ] Commit 7: Cat 2 Stack B
- [ ] Commit 8: Cat 3 cubic-kernel
```

(Zero-width separator inserted before each triple-backtick fence to
keep this report parseable as markdown; the actual file has bare
fences.)

- **No "Tools" or "Scripts" section** exists (FACT).
- **No table of contents** (FACT).
- **Implementation status checkboxes are stale** (INFERENCE): the
  README claims commits 2-8 are unimplemented but they all shipped in
  v1.0 - v1.2. This is a real freshness defect the closeout commit
  could clean up alongside the v1.3 closure update.

---

## § C — Convention verbatim text

### C.1 — Conventions A, B, C, D, E (FACT, verbatim, v1.1 retro § 7.2)

Surrounding paragraph context (preceding § 7.2's heading at line 525):

> ### 7.2 Conventions banked from operating evidence

Bodies (FACT, verbatim, lines 527-549):

> **A. New-files-first decomposition (banked from § 3.3):**
> > Execution specs default to commit decomposition for any commit that
> > touches more than one previously-existing file. The new-files-only
> > sub-commit ships first.
>
> **B. Grandfather-sweep companion (banked from § 5.4):**
> > Every commit that touches cat1-scannable surface either runs the
> > sweep or lands a companion sweep commit in the same PR.
>
> **C. Probe-template enumerate-conventions (banked from § 3.1):**
> > Pre-spec probes that ground path-resolution rules must include
> > verbatim probe items enumerating 3-5 representative path pairs.
>
> **D. Probe-template enumerate-call-sites (banked from § 3.2):**
> > Pre-spec probes that ground behavioral changes must include verbatim
> > probe items enumerating all modules that depend on the affected
> > behavior.
>
> **E. Spec-author-self-test review (banked from § 3.4, § 5.3):**
> > When the spec author writes new test code for own-spec items, a
> > second pair of eyes (architect-2 or a mechanical own-source scan)
> > reviews the test files for grammar-literal leaks before commit.

Trailing context (lines 550-553):

> These five are concrete, narrow, and actionable. None requires new
> toolkit code (E may eventually be subsumed by A.2). All five are
> candidates for inclusion in a permanent CONVENTIONS doc rather than
> re-derivation in every retro.

### C.2 — Convention F (FACT, verbatim, post-retro landing audit § D.2.1)

Preceding context (lines 251-253):

> Recommend banking a sixth operating-condition convention alongside
> retro § 7.2 A–E:

Body (FACT, verbatim, lines 254-259):

>     F. Audit-prose freshness check. Audit reports drafted at
>        direction time and landed later by an executor should
>        verify the gate-state claims against current disk
>        immediately before commit. Discrepancies become addenda
>        (not paraphrases) to preserve the audit trail of when
>        each claim was authored vs landed.

Trailing context (line 261):

> This is a v1.2 candidate; not implemented in this session.

(NB: Convention F's body is formatted as an indented prose block
rather than a blockquote, in contrast to A-E and G-K. The wording
"audit reports drafted at direction time" — not "drafted by the
toolkit" — means F is currently a workflow guideline, not yet a
toolkit-enforced rule. The closeout spec's commit-4 cat1
audit-prose-freshness check would convert F's intent into an
enforced check.)

### C.3 — Conventions G, H, I (FACT, verbatim, v1.2 bolt-ons retro § 4)

Preceding context (lines 261-266):

> ## 4. Banked conventions (additions to retro § 7.2)
>
> Inheriting from `docs/retro/integrity-toolkit-v1.1-batch1.md` § 7.2
> (conventions A–E) and the post-retro landing audit § D.2.1
> (convention F). Three new entries:

Body G (FACT, verbatim, lines 267-285):

> ### 4.1 Convention G — Sweep-side protection lands before check-side scope expansion
>
> > **G.** Sweep-side protection lands before check-side scope
> > expansion. When a v1.x batch adds a check that will classify into
> > broad buckets (live-source / audit-doc / retro-doc / etc.), the
> > corresponding sweep CLI protection rule must land before or
> > alongside the check registration. Specifically: if a check produces
> > live-source findings whose intended treatment is "attribute, do not
> > sweep," the live-source filter must be active in the sweep CLI
> > before the check is registered.
> >
> > "Active" means the filter must cover the category space the new
> > check will produce. If the new check creates a new classifier
> > category, the filter needs extending to recognize that category as
> > a sweep-protected bucket — landing both within the same batch (or
> > coordinating across batches with an explicit hand-off) is
> > sufficient. See v1.2 § 3.2 for the v1.2/A.3 interaction case where
> > the literal-match filter did not cover A.3's new fallthrough
> > category and required an immediate extension.

Body H (FACT, verbatim, lines 287-300):

> ### 4.2 Convention H — Filter rules query properties, not literals
>
> > **H.** When implementing sweep filters or similar policy rules over
> > a typed surface (`Classification`, `FailureMode`, etc.), prefer
> > queries against properties of the type ("is this fallthrough?",
> > "is this hard-fail?", "is this live-source?") over literal string
> > matches against specific values.
> >
> > Literal matches do not generalize over new values added concurrently
> > by other batches; property queries do. See v1.2 § 3.1 for the
> > positive case (path-axis property query absorbing 2 additional
> > findings under concurrent landing) and § 3.2 for the negative
> > contrast (category-axis literal match requiring an extension when
> > A.3 introduced `other-cat1-bare-path`).

Body I (FACT, verbatim, lines 302-316):

> ### 4.3 Convention I — Cross-batch scope discipline
>
> > **I.** When a sweep CLI run during a small-scope batch's verification
> > picks up findings outside the batch's scope, do not opportunistically
> > sweep them. Defer to the responsible batch's own sweep companion.
> > This keeps audit reports clean (one batch's work doesn't co-mingle
> > with another's), preserves the per-batch landing audit's leverage
> > as forensic evidence, and prevents scope creep that may invalidate
> > the batch's own design assumptions.
> >
> > In v1.2 bolt-ons, ~647 `cat1.bare-path` findings were available to
> > sweep during P1.5–P1.7's inline sweep companions. Deferring to
> > A.3 commit 4's planned sweep companion (`908f619`) preserved both
> > batches' coherence and gave A.3's forensic record sole ownership of
> > the bare-path sweep.

### C.4 — Conventions J, K (FACT, verbatim, v1.3 Part-A retro § 4)

Preceding context (lines 303-306):

> ## 4. Banked conventions (additions to retro § 7.2)
>
> Inheriting conventions A–I from the v1.1 batch-1 retro and v1.2
> bolt-ons retro. Adding J, K.

Body J (FACT, verbatim, lines 308-328):

> ### 4.1 Convention J — Sweep companions operate across commit boundaries when multi-file commits land
>
> > **J.** A grandfather-sweep companion within a single commit
> > operates on the cat1-scannable surface as it exists at sweep-run
> > time. When a commit lands multiple new audit-doc files in a single
> > push (e.g., probe + spec + audit report), the sweep may not catch
> > every finding in one pass; residual findings carry into the next
> > commit's pre-commit gate and are swept by that commit's sweep
> > companion.
> >
> > This is not a defect; it's a mechanical consequence of how the
> > sweep operates on findings. The implication for spec authors:
> > expect a transient gate spike at the boundary between multi-file
> > commits, with the spike collapsing under the next sweep companion.
> > Convention B's "sweep companion within the commit" requirement
> > still holds; this is a clarification of the per-commit sweep's
> > scope, not an exception.
> >
> > See v1.3 batch-1 part-A § 3.3 for the case where commit 1 landed
> > probe + spec + audit report and 12 unswept findings carried into
> > commit 2.

Body K (FACT, verbatim, lines 330-350):

> ### 4.2 Convention K — Anchor-sketch labeling for spec content from inference
>
> > **K.** When a spec section constructs content from probe data plus
> > architect-1 inference (rather than from verbatim verified content
> > on disk), label the section explicitly as an "anchor sketch — verify
> > at execution time" rather than presenting it as canonical. Anchor
> > sketches name a likely failure mode upfront; the verification block
> > for the same section should include a check that confirms the
> > sketch against execution-time disk.
> >
> > The failure mode this prevents: an executor reading canonical-
> > looking content trusts it without re-deriving, and disagreement
> > between sketch and reality surfaces only at the next verification
> > step (or worse, after a commit lands with subtly-wrong content).
> >
> > See v1.3 batch-1 part-A § 3.2 for the case where spec § 4.2.1
> > presented JSON content from inferred TOML translation rather than
> > from generator output; the integer-vs-float and precision-digit
> > mismatches surfaced at the commit-2 generator round-trip check.
> > Labeling the section as an anchor sketch would have flagged this
> > at hand-off rather than at execution.

All eleven (A–K) located in the cited sections. No expected
convention is missing from C.1–C.4. (FACT.)

---

## § D — Citation regex corpus sample

### D.1 — First regex sample (FACT, verbatim, head -30 of sorted-unique output)

Command:
`grep -rhoE '`[A-Za-z0-9_./\-]+\.[A-Za-z0-9_]+:[0-9]+(-[0-9]+)?`' docs/integrity-toolkit-spec.md docs/retro/ docs/diagnostics/_audits/ project-state.md | sort -u | head -30`

```
`192.168.1.1:80`
`alembic_writer.cpp:17-24`
`alembic_writer.cpp:36-88`
`alembic_writer.cpp:51-82`
`alembic_writer.cpp:61`
`alembic_writer.cpp:63-65`
`alembic_writer.cpp:66`
`alembic_writer.cpp:66-75`
`alembic_writer.cpp:90-96`
`alembic_writer.hpp:11`
`alembic_writer.hpp:13`
`alembic_writer.hpp:13-16`
`alembic_writer.hpp:21`
`alembic_writer.hpp:21-27`
`alembic_writer.hpp:24`
`alembic_writer.hpp:26`
`alembic_writer.hpp:31`
`alembic_writer.hpp:44`
`alembic_writer.py:43`
`alembic_writer.py:45`
`alembic_writer.py:46`
`alembic_writer.py:47`
`alembic_writer.py:96`
`apply_emitter.comp.glsl:3`
`bilateral_smooth.comp.glsl:5`
`bilateral_smooth.comp.glsl:9-12`
`BoundaryModel_Akinci2012.cpp:13-21`
`BoundaryModel_Akinci2012.cpp:48-75`
`BoundaryModel_Akinci2012.hpp:1-116`
`buffer.cpp:108-145`
```

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
(`BoundaryModel_Akinci2012.h:1-116` appears as `.h` in the corpus
but `head -30` truncated alphabetically — verified by running
`sort -u | wc -l`, the corpus is much larger than 30 unique entries.)

### D.2 — Second regex sample (FACT, verbatim, head -10)

Command:
`grep -rhoE '`[a-zA-Z0-9_./\-]+/[a-zA-Z0-9_./\-]+:[0-9]+(-[0-9]+)?`' docs/integrity-toolkit-spec.md docs/retro/ docs/diagnostics/_audits/ project-state.md | sort -u | head -10`

```
`cat2_contracts/checks/stub_label_stale.py:1-25`
`cat3_numerical/generate_expected.py:105`
`Chakazul/Lenia/Python/LeniaNDK.py:329-335`
`chapter13/cpu_intro/main.cpp:271`
`chapter13/cpu/LBM.cpp:97`
`chapter13/gpu/LBM.cu:172`
`chapter5/poiseuille_BB.m:123`
`chapter5/poiseuille_BB.m:93`
`chapter8/cylinder.cpp:222`
`chapter8/cylinder.cpp:63`
```

### D.3 — Regex-malformedness observations (INFERENCE)

- **IP-address false positive:** `192.168.1.1:80` matches the first
  regex. The pattern `[A-Za-z0-9_./\-]+\.[A-Za-z0-9_]+:[0-9]+` is
  ambiguous with `<ip>:port` because `[A-Za-z0-9_./\-]+` happily eats
  numeric labels and dots. The audit-prose freshness tool will need
  to either (a) require the path-side to contain `/` or to end in a
  known source-extension whitelist (`.cpp`/`.py`/etc.), or (b)
  pre-filter against `\d+\.\d+\.\d+\.\d+:\d+`. Flag § G.3.

- **Multi-colon `file:line:col` form:** running an additional grep
  for `:[0-9]+:[0-9]+` against the same corpus returned **zero
  matches** (FACT — empty output). The corpus does not currently use
  `file:42:5` line-and-column citation form, so the audit-prose tool
  can safely assume single-`:line[-N]` shape. (Worth re-confirming if
  tool consumers ever start citing column positions.)

- **Embedded backticks:** every citation in the sample is wrapped in
  paired backticks. No multi-line or unpaired backtick captures
  observed in the first 30 entries.

- **Same basename collisions:** `main.py:N` and `main.cpp:N` recur
  with multiple distinct line numbers (the same defect class the
  cat1.bare-path findings on project-state.md surface). The freshness
  tool will share the cat1.bare-path classifier's basename-resolver
  for "which on-disk file does this citation refer to" — INFERENCE,
  worth confirming in spec design.

---

## § E — GitHub Actions diff-resolution

### E.1 — Canonical pattern (INFERENCE + outside-knowledge synthesis)

`actions/checkout@v4` with `fetch-depth: 1` (current workflow setting,
§ B.5) fetches **only the head commit**. `git fetch origin <SHA>`
**does NOT work** by default against GitHub remotes for arbitrary
historical SHAs — the remote refuses uploads of objects unreachable
from any ref unless `uploadpack.allowAnySHA1InWant` or
`uploadpack.allowReachableSHA1InWant` is enabled on the remote (the
latter is GitHub's default since 2018, but the SHA must still be
reachable from a *ref*, not just any commit in the repo's history).

Three workable patterns:

1. **Increase `fetch-depth` permanently.** `fetch-depth: 0` fetches
   full history. Slowest checkout, simplest code: any
   `git diff <base-sha>...<head-sha>` "just works." Cost: roughly
   linear in repo size; on the current repo (manageable history,
   ~9000 commits estimated by `git rev-list HEAD --count` — INFERENCE,
   not measured), full-history checkout adds maybe 5-15 seconds.

2. **Fetch the base branch ref at runtime.** Keep `fetch-depth: 1`,
   then run `git fetch --depth=1 origin main` (or the PR base) before
   computing the diff. Cost: one extra `git fetch` per workflow run.

3. **Use `${{ github.event.pull_request.base.sha }}` + selective
   fetch.** GitHub Actions exposes the base SHA in the event payload.
   For a PR run, `git fetch origin <base-sha>:refs/remotes/origin/<base-sha>`
   with `--depth=1` plus a follow-up unshallow if the diff range is
   wide. The clean form: `git fetch --no-tags --depth=1 origin
   <base-sha>` followed by `git diff <base-sha>...HEAD`.

**Recommendation for closeout spec commit-4 § 5.C.2:** option 2 (or
3 for PR runs specifically). Both keep the default-fast `fetch-depth:
1` checkout for non-diff steps and only pay the diff-range cost when
the audit-prose freshness check runs. Option 1 (`fetch-depth: 0`)
is simpler but more expensive on every run.

### E.2 — Workflow-level flag (FACT + INFERENCE)

If the spec adopts option 1 (`fetch-depth: 0`), the workflow checkout
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
step at `integrity.yml:28` needs to change from `fetch-depth: 1` to
`fetch-depth: 0`. This is a workflow-global change visible to every
job, not just the audit-prose freshness step.

If the spec adopts option 2 or 3, the workflow checkout step does
NOT change, but a new runtime fetch step is inserted before the
"Run integrity toolkit against repo" step.

**Flag for spec commit-4 § 5.C.2:** make the choice explicit. The
current spec text (per the cover note: not yet on disk, see § G.0)
likely needs a revision pass naming the chosen pattern.

---

## § F — Stack C runtime baseline

### F.1 — CI wall-clock data unavailable (FACT)

`gh run list --workflow=integrity.yml --branch=main --limit=20`
shows that all 20 most recent runs ended in `cancelled` or `failure`
(11 cancelled, 9 failure). The most-recent run (25964487465 at
2026-05-16T14:31:40Z) ran 10m17s before cancellation; the cancelled
step was "Run integrity toolkit against repo". With cancelled status,
GitHub Actions does not record reliable step durations.

`gh run view <run-id> --json jobs` returned `startedAt`/`completedAt`
as `None` for every step in cancelled runs (FACT — null values).

No successful run on `main` in the last 20 runs. The last successful
integrity workflow run on `main` predates the 20-run window — pulling
further history is gated behind additional `gh api` calls and is
not in scope for this read-only probe.

**Conclusion (INFERENCE):** the v1.3 part-B retro § 1 baseline noted
the perf-assertion test in closeout commit 2 would run on first push
instead. That guidance still holds — CI logs cannot provide a
pre-spec wall-clock baseline. The perf assertion should be authored
with a conservative ceiling (e.g., "Stack C cat2 check completes in
< 120 s on the CI runner") and refined on the first post-landing run.

---

## § G — Pause-and-surface findings (banked for architect-1 spec revision)

### G.0 — Closeout spec not yet on disk

**FACT:** `ls docs/diagnostics/_audits/ | grep closeout` returns
empty. The spec file
`docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md`
**does not exist on disk** at HEAD `a1c9121`. Full per-section
re-anchoring against spec assertions is therefore deferred to
spec-on-disk time. This probe banks raw observations and flags
spec-content assumptions where they are inferrable from the cover
note (line-anchor expectations, fossil hypothesis, commit-4 § 5.C.2
fetch-depth wording, etc.). The architect-1 chat should reconcile
this probe's observations against the in-flight spec draft before
hand-off.

### G.1 — `is_suppressed` and `Finding.suppressed` both absent

**FACT** (§ B.1): neither a top-level `is_suppressed(...)` helper
nor a `Finding.suppressed: bool` field exists. The closeout spec's
§ 3.2 wiring (per the cover note) assumes one of them. If the spec
introduces an audit-prose freshness check that consumes
`Finding`-shaped objects, it needs either (a) a new helper
`is_suppressed(finding: Finding) -> bool` that re-runs the
suppression logic against the source, OR (b) widening `Finding` to
carry the `suppressed` bit from the JSON output through to the
classifier.

**Recommendation:** option (b) (add `suppressed: bool` to `Finding`
with a default of `False` and populate it from the JSON dict at
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`grandfather.py:402`). This is a 3-line dataclass change plus the
populate line; no callers will break because the field has a
default.

### G.2 — Suppression not firing on project-state.md cat1.bare-path

**FACT** (§ B.8): three `cat1.bare-path` annotation lines at
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
project-state.md:560/594/667 do not suppress the cat1.bare-path
findings at 561/595/668 (5 total findings, all HARD_FAIL). The
annotations and findings share the same `cat1.bare-path` check_id
and sit on the immediately-preceding line, which is exactly the
shape `annotation_already_present()` checks for (§ B.1).

**Open questions for spec revision:**

- Is the suppressor consulting `lines[zero_idx - 1]` (one above) or
  scanning further? `annotation_already_present()` in grandfather.py
  is used by the sweep CLI, not the runner-side suppressor. The
  runner has its own suppression pipeline — its code path needs
  verification.
- Is there a multi-annotation-stack interaction (lines 559 + 560
  both carry annotations; does the runner see both, or only the
  immediately-adjacent one)?
- Closeout commit-4 (audit-prose freshness) needs this to be sorted
  out before its check fires — otherwise the freshness check will
  surface false stale-annotation positives.

The hypothesis "fossils at 559/593/666; load-bearing at
560/594/667" is **half-correct**: cat1.intra-repo annotations are
fossils as predicted, but the cat1.bare-path annotations next to
them are *not actually load-bearing* in their current form (the
findings they should suppress are firing as HARD_FAIL). So either
all six lines are fossils-in-effect, OR there is a real suppression
bug on these specific lines. Either way, the spec's anchor-line
rationale needs revisiting.

### G.3 — Citation regex IP-address false positive

**FACT** (§ D.1): `192.168.1.1:80` matches the first citation
regex. The closeout audit-prose freshness tool needs to either
constrain the path side to require `/` or filter against
`\d+\.\d+\.\d+\.\d+:\d+` before classifying. Minor but worth a
spec-text note.

### G.4 — `_KNOWN_CATEGORIES` has no pinning test

**FACT** (§ B.7): `FALLTHROUGH_CATEGORIES` is pinned by
`test_fallthrough_categories_contents` (per the grandfather.py
docstring at line 74). `_KNOWN_CATEGORIES` in `snapshot.py` has
no analogous pinning test (verified by grep — no test references
`_KNOWN_CATEGORIES` by name in `tools/integrity/tests/`).

If the closeout spec introduces a new snapshot category for the
audit-prose freshness rule (or for v1-closed markers), the new
entry needs registering in `_KNOWN_CATEGORIES` AND ordered before
"other-cat1" / "other-cat1-bare-path" per the existing comment
discipline. Worth banking a parallel pinning test for
`_KNOWN_CATEGORIES` as a small bonus item in the closeout.

### G.5 — `tools/integrity/README.md` implementation-status table is stale

**FACT** (§ B.9): the README's "Implementation status" section
shows commits 2-8 as unchecked, which is wildly out-of-date — these
have all shipped through v1.0 - v1.2. This is real freshness
defect and a natural cleanup item for the closeout.

### G.6 — Workflow `fetch-depth: 1` blocks audit-prose freshness baseline

**FACT** (§ B.5, § E.1): `fetch-depth: 1` precludes any
`<base-sha>...<head-sha>` diff without an additional fetch step or
deepening of the checkout. The spec's commit-4 § 5.C.2 (per cover
note) needs to name a specific approach. See § E for three patterns.

### G.7 — Convention F body uses indented prose rather than blockquote

**FACT** (§ C.2): Convention F is formatted as a prose block, while
A-E and G-K use `> > **X.**` blockquotes. Not a content drift, but
worth noting if the closeout spec's commit-1 instructs "render all
11 conventions in a single CONVENTIONS doc" — the renderer needs to
normalize the format.

### G.8 — Quantitative baseline reconciliation (PASS)

**FACT:** 60 hard-fails and 183 tests collected match the v1.3
part-B retro § 1 quantitative baseline exactly (§§ A.2, A.3). No
drift since 2026-05-16 part-B retro. No flag.

### G.9 — No spec/retro convention text drift visible (PASS)

**FACT** (§ C.1-C.4): all 11 conventions present, verbatim, in
expected sections. The probe matched the conventions to the cited
sections without needing to fall back to alternative locations. No
flag.

---

## § H — Closing

Probe complete; **8 flags raised** in § G (G.1 — missing
`is_suppressed`/`Finding.suppressed`; G.2 — half-broken project-state
suppression; G.3 — regex IP false positive; G.4 — unpinned
`_KNOWN_CATEGORIES`; G.5 — stale README status table; G.6 —
`fetch-depth: 1` blocks audit-prose freshness baseline; G.7 — minor
Convention F formatting inconsistency; G.0 — spec not yet on disk).

Spec at
`docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md`
(when landed) may need patches around: **§ 3.2 (Finding wiring,
flag G.1), § 4 (project-state line-anchor rationale, flag G.2),
audit-prose freshness regex section (flag G.3), commit-4 § 5.C.2
GitHub Actions diff-resolution syntax (flag G.6).** The other flags
(G.4, G.5, G.7) are minor and can be absorbed as bonus items during
spec authoring rather than blocking revisions.

Two PASSes worth banking explicitly: quantitative baseline (G.8) and
convention text drift check (G.9).

— end of probe report —
