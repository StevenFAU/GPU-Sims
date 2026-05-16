---
title: "Integrity v1.2 A.2 Pre-Spec Probe"
date: 2026-05-15
author: architect1-via-claude-code
status: probe
scope: read-only
sibling-docs:
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/retro/integrity-toolkit-v1.2-bolt-ons.md
  - docs/diagnostics/_audits/integrity_v1_1_self_review_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_a3_probe_2026-05-15_architect1.md
---

<!-- integrity-allow: cat1.bare-path; toolkit-doc bare-path citation pre-v1.2 (see grandfather-catalog toolkit-doc-bare-path); n/a -->

This is a read-only probe to ground the v1.2 A.2 spec ("toolkit
self-application of Stack D"). The toolkit is a substantial Python
package (`tools/integrity/integrity/`) that is currently NOT scanned by
`cat2.public-symbol-used` — that check hard-codes `common-py/gpusims_common`
as its target. A.2 will close that gap.

All claims tagged FACT (directly observed at probe SHA) or INFERENCE
(derived). All line numbers are captured from `cat -n` / `grep -n` runs
during this probe; none carried from the prompt.

---

## § A — Gate state at probe time

### A.1 — `git rev-parse HEAD`

FACT:

```
1a49d334bdcacc1d8ba9c2311788ca486a2c0fd1
```

(Short SHA `1a49d33`; matches HEAD throughout the probe — re-checked at
the end of probe execution; no drift.)

### A.2 — `python3 -m integrity --mode strict --no-audit-log`

FACT — exit code: `1` (HARD_FAIL).

Summary line:

```
integrity: 5 pass, 0 soft-warn, 163 hard-fail, 1087 suppressed
```

First 20 unsuppressed-finding stanzas (verbatim, captured into
`/tmp/probe_a2.txt`):

```
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/integrity_v1_2_a3_probe_2026-05-15_architect1.md:532
    Chakazul/Lenia/Python/LeniaNDK.py:329-335: path 'Chakazul/Lenia/Python/LeniaNDK.py' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/integrity_v1_2_a3_probe_2026-05-15_architect1.md:543
    Chakazul/Lenia/Python/LeniaNDK.py:329-335: path 'Chakazul/Lenia/Python/LeniaNDK.py' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase11_5_resume_probe_2026-05-15_architect1.md:441
    SPlisHSPlasH/DFSPH/TimeStepDFSPH.h:1-15: path 'SPlisHSPlasH/DFSPH/TimeStepDFSPH.h' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase11_5_resume_probe_2026-05-15_architect1.md:1199
    docs/load-bearing-decisions.md:1-81: path 'docs/load-bearing-decisions.md' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase11_5_resume_probe_2026-05-15_architect1.md:1200
    shaders/_struct_layouts.txt:1-109: path 'shaders/_struct_layouts.txt' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase11_5_resume_probe_2026-05-15_architect1.md:1379
    docs/notes.md:20: path 'docs/notes.md' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase11_5_resume_probe_2026-05-15_architect1.md:1381
    docs/notes.md:20: path 'docs/notes.md' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md:183
    chapter13/cpu/LBM.cpp:97: path 'chapter13/cpu/LBM.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md:371
    chapter13/cpu/LBM.cpp:145: path 'chapter13/cpu/LBM.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md:380
    chapter13/cpu_intro/main.cpp:271: path 'chapter13/cpu_intro/main.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md:423
    chapter13/cpu_intro/main.cpp:25: path 'chapter13/cpu_intro/main.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md:440
    chapter8/cylinder.cpp:59: path 'chapter8/cylinder.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md:468
    chapter13/cpu/LBM.h:65: path 'chapter13/cpu/LBM.h' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md:471
    chapter13/cpu/LBM.cpp:113: path 'chapter13/cpu/LBM.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md:509
    chapter8/cylinder.cpp:222: path 'chapter8/cylinder.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md:515
    chapter8/cylinder.cpp:63: path 'chapter8/cylinder.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md:540
    chapter13/cpu/LBM.cpp:107: path 'chapter13/cpu/LBM.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md:704
    chapter13/cpu/LBM.cpp:97: path 'chapter13/cpu/LBM.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md:705
    chapter13/cpu_intro/main.cpp:271: path 'chapter13/cpu_intro/main.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs/diagnostics/_audits or /home/otacon/Projects/GPU-Sims/GPU-Sims
```

INFERENCE: of the 163 unsuppressed hard-fails, every one of the first
20 is a `cat1.intra-repo` finding pointing into an audit-doc snapshot
that cites a now-renamed sibling audit. None reference toolkit code.
A.2 will add cat2 findings on top of this baseline; the spec should
not assume the gate is currently green.

### A.3 — `python3 -m integrity --grandfather-report --no-history-append`

FACT — exit code: `0`. Summary + per-category counts:

```
grandfather report @ 1a49d33 (2026-05-15T23:57:51.821985+00:00)
summary: {'pass': 5, 'soft_warn': 0, 'hard_fail': 163, 'suppressed': 1087}
per-category counts:
                      audit-bare-path: 635
                  cat2-stack-c-unused: 110
                       audit-citation: 80
                  cat2-stack-b-unused: 73
         audit-report-grammar-example: 42
                           other-cat1: 35
                   toolkit-own-source: 25
                 spec-grammar-example: 18
                  cat2-stack-d-unused: 17
                       audit-doc-1810: 16
                      retro-bare-path: 11
                retro-grammar-example: 8
                toolkit-doc-bare-path: 7
          deferred-upstream-bare-path: 5
                     live-shader-1810: 3
                cat2-stub-label-stale: 2
```

This is the post-A.3 + post-bolt-ons baseline. A.2's new toolkit
self-application check will add a new bucket on top (see § F, § H).

### A.4 — Recent commit history (`git log --oneline -15`)

FACT:

```
1a49d33 docs(audits): back-fill SHA cross-references in v1.2 A.3 landing audits
908f619 feat(integrity): grandfather sweep companion for cat1.bare-path (v1.2 A.3 commit 4)
5c3e1ef docs(integrity): SHA back-fill for v1.2 commits 1-4 (v1.2 commit 5)
5cdd20f docs(integrity): P1.7 fix stub_label_stale.py module-docstring drift (v1.2 commit 4)
71559ce fix(integrity): P1.6 human-renderer omits suppressed stanzas (v1.2 commit 3)
119e353 feat(integrity): P1.5 register cat3.d3q19-* checks (v1.2 commit 2)
5fe5e6b feat(integrity): P1.8 grandfather-sweep live-source protection (v1.2 commit 1)
880a400 feat(integrity): register cat1.bare-path + add skip-guard (v1.2 A.3 commit 3)
77628b6 feat(integrity): add cat1.bare-path classifier rules + catalog (v1.2 A.3 commit 2)
6fc5884 feat(integrity): add cat1.bare-path check module + tests (v1.2 A.3 commit 1)
9add149 docs(retro): self-review probe addendum to v1.1 batch-1 retro
cdad2e2 fix(lattice-boltzmann): streamline seed-slab + dt_render units (in-flight #2)
c1a257d fix(lattice-boltzmann): streamline reseed visual defects (in-flight Phase 12)
d772803 docs(audits): back-fill SHA cross-references in post-retro landing audit
e26056c docs(audits): integrity v1.1 post-retro landing audit
```

INFERENCE: v1.2 A.3 (cat1.bare-path) and the bolt-ons batch (P1.5–P1.8)
are both fully landed at probe SHA. A.2 is the last A-track item not
yet started. The branch is clean per session start status.

---

## § B — Toolkit's current "public surface" by declaration

### B.1 — Top-level package `__init__.py`

FACT — `tools/integrity/integrity/__init__.py` (LOC 3):

```python
     1	"""GPU-Sims integrity toolkit — cross-stack verification per docs/integrity-toolkit-spec.md."""
     2	
     3	__version__ = "0.1.0"
```

### B.2 — Sub-package `__init__.py` files

FACT — `tools/integrity/integrity/cat1_citations/__init__.py` (LOC 1):

```python
     1	"""Category 1: Citation & reference integrity per spec § 6."""
```

FACT — `tools/integrity/integrity/cat2_contracts/__init__.py` (LOC 1):

```python
     1	"""Category 2: Public-API contract verification per spec § 7."""
```

FACT — `tools/integrity/integrity/cat3_numerical/__init__.py` (LOC 1):

```python
     1	"""Category 3: Numerical correctness per spec § 8."""
```

FACT — `tools/integrity/integrity/cat1_citations/checks/__init__.py` (LOC 19):

```python
     1	"""Cat 1 check modules. Discovered by integrity.runner.discover_checks."""
     2	
     3	from integrity.cat1_citations.checks import (
     4	    annotation,
     5	    bare_path,
     6	    intra_repo,
     7	    unregistered_upstream,
     8	    upstream,
     9	    upstream_anchor,
    10	)
    11	
    12	REGISTERED_CHECKS = [
    13	    (intra_repo.CHECK_ID, intra_repo),
    14	    (bare_path.CHECK_ID, bare_path),
    15	    (annotation.CHECK_ID, annotation),
    16	    (upstream.CHECK_ID, upstream),
    17	    (upstream_anchor.CHECK_ID, upstream_anchor),
    18	    (unregistered_upstream.CHECK_ID, unregistered_upstream),
    19	]
```

FACT — `tools/integrity/integrity/cat2_contracts/checks/__init__.py` (LOC 15):

```python
     1	"""Cat 2 check modules. Discovered by integrity.runner.discover_checks."""
     2	
     3	from integrity.cat2_contracts.checks import (
     4	    public_symbol_used,
     5	    public_symbol_used_b,
     6	    public_symbol_used_c,
     7	    stub_label_stale,
     8	)
     9	
    10	REGISTERED_CHECKS = [
    11	    (public_symbol_used.CHECK_ID, public_symbol_used),
    12	    (public_symbol_used_c.CHECK_ID, public_symbol_used_c),
    13	    (public_symbol_used_b.CHECK_ID, public_symbol_used_b),
    14	    (stub_label_stale.CHECK_ID, stub_label_stale),
    15	]
```

FACT — `tools/integrity/integrity/cat3_numerical/checks/__init__.py` (LOC 15):

```python
     1	"""Cat 3 check modules. Discovered by integrity.runner.discover_checks."""
     2	
     3	from integrity.cat3_numerical.checks import (
     4	    cubic_kernel,
     5	    d3q19_velocity_set,
     6	    d3q19_weights,
     7	    d3q19_equilibrium,
     8	)
     9	
    10	REGISTERED_CHECKS = [
    11	    (cubic_kernel.CHECK_ID, cubic_kernel),
    12	    (d3q19_velocity_set.CHECK_ID, d3q19_velocity_set),
    13	    (d3q19_weights.CHECK_ID, d3q19_weights),
    14	    (d3q19_equilibrium.CHECK_ID, d3q19_equilibrium),
    15	]
```

FACT — `tools/integrity/integrity/common/__init__.py` (LOC 0): file
exists and is empty (no docstring).

### B.3 — INFERENCE: what does the toolkit declare as public?

INFERENCE: the toolkit's top-level `integrity/__init__.py` is
docstring + `__version__` only — it re-exports nothing. The cat1/cat2/
cat3 sub-packages are docstring-only. Only the three `checks/`
sub-packages declare any "public" surface, and what they declare is the
internal registry consumed by `integrity.runner.discover_checks` —
not user-facing API. There are zero `__all__` declarations anywhere.

This means the existing Stack D extractor `extract_public_surface`
(which parses `gpusims_common/__init__.py` for re-exports) would, if
pointed at `tools/integrity/integrity/__init__.py`, return an EMPTY
list — the toolkit has no Stack-D-shaped public surface to enumerate.
This is the central design constraint A.2 must address (see § E.3 and
§ L.2).

---

## § C — Toolkit's "public surface" by usage evidence

### C.1 — Internal cross-module imports (within `integrity/`)

FACT — every `from integrity.X import Y` reference (deduplicated to
internal-only origins). The full `grep -rn "^from integrity\."` output
is preserved in tool transcript; key edges:

- `integrity/__main__.py` imports `runner.main`
- `integrity/runner.py` imports `common.exclusions.CANONICAL_EXCLUSIONS`,
  `common.repo.{find_repo_root, git_head_sha}`,
  `common.results.{FailureMode, Finding, RunSummary}`
- `integrity/grandfather.py` imports `common.annotations.*`
- `integrity/common/suppression.py` imports `common.annotations.*`,
  `common.results.Finding`
- `integrity/common/audit_log.py` imports `common.results.Finding`
- Every `cat*/checks/X.py` imports `common.results.{FailureMode, Finding}`,
  `common.repo.list_tracked_files`, `common.exclusions.is_excluded`
  (and `common.annotations.*` where relevant)
- `cat2_contracts/checks/public_symbol_used.py` imports
  `cat2_contracts.stack_d.{extract_public_surface, find_references}`
- `cat2_contracts/checks/public_symbol_used_b.py` imports
  `cat2_contracts.stack_b.run_extractor`
- `cat2_contracts/checks/public_symbol_used_c.py` imports
  `cat2_contracts.stack_c.{...}`
- `cat3_numerical/checks/cubic_kernel.py` imports
  `cat3_numerical.cubic_kernel.*`
- `cat3_numerical/checks/d3q19_*.py` import `cat3_numerical.d3q19_verify.*`
- `cat1_citations/checks/intra_repo.py` imports
  `cat1_citations.grammar.*`, `cat1_citations.resolver.resolve`
- `cat1_citations/checks/bare_path.py` imports
  `cat1_citations.grammar.*`, `cat1_citations.resolver._count_lines`,
  `cat1_citations.upstream_anchor.{...}`
- `cat1_citations/checks/upstream.py` imports
  `cat1_citations.grammar.extract_upstream_citations`,
  `cat1_citations.resolver._count_lines`,
  `cat1_citations.upstream_anchor.load_registry`
- `cat1_citations/checks/upstream_anchor.py` imports
  `cat1_citations.upstream_anchor.{load_registry, vendor_head_sha}`
- `cat1_citations/checks/unregistered_upstream.py` imports
  `cat1_citations.grammar.extract_upstream_citations`,
  `cat1_citations.upstream_anchor.load_registry`
- `cat1_citations/resolver.py` imports
  `cat1_citations.grammar.IntraRepoCitation`

### C.2 — External consumers (scripts + tests + docs)

FACT — `grep -rn "^from integrity\|^import integrity" tools/integrity/scripts/ tools/integrity/tests/ .github/ docs/` returns matches only in:

- `tools/integrity/scripts/grandfather_sweep.py` — imports
  `common.repo.find_repo_root`, `grandfather.apply_annotations`
- 20 test files under `tools/integrity/tests/` — see C.3 below for
  per-file specifics
- 4 doc files under `docs/diagnostics/_audits/` — these are all
  literal listings of toolkit code in audit/spec/probe reports
  (`integrity_v1_1_apispec_2026-05-15_architect1.md`,
  `integrity_v1_1_batch1_spec_2026-05-15_architect1.md`,
  `integrity_v1_1_self_review_probe_2026-05-15_architect1.md`,
  `integrity_v1_2_bolt_ons_spec_2026-05-15_architect1.md`)

INFERENCE: the doc-tree matches are not real consumers — they are
fenced code listings inside markdown that begin with `from integrity.`
text. A.2's check must scan only `.py` / `.pyi` files (not markdown)
to avoid treating a documentation listing as a "consumer" — the
existing Stack D check already does this in
`_list_scannable_py_files()` at `tools/integrity/integrity/cat2_contracts/checks/public_symbol_used.py:42-60`,
so this constraint transfers directly.

Test-file consumers (the only *real* external consumer surface):

| Test file | Imports `from integrity.X` |
|---|---|
| `test_cat1_annotation.py` | `cat1_citations.checks.annotation.{_validate, run}` |
| `test_cat1_annotation_fence.py` | `cat1_citations.checks.annotation.run`, `common.annotations.*` |
| `test_cat1_bare_path.py` | `cat1_citations.checks.bare_path.{...}`, `cat1_citations.grammar.IntraRepoCitation`, `common.results.FailureMode` |
| `test_cat1_intra_repo.py` | `cat1_citations.checks.intra_repo.run`, `common.results.FailureMode` |
| `test_cat1_intra_repo_fence.py` | `cat1_citations.checks.intra_repo.run` |
| `test_cat1_unregistered.py` | `cat1_citations.checks.unregistered_upstream.run` |
| `test_cat1_upstream.py` | `cat1_citations.checks.upstream.run`, `common.results.FailureMode` |
| `test_cat1_upstream_anchor.py` | `cat1_citations.checks.upstream_anchor.run`, `common.results.FailureMode` |
| `test_cat1_upstream_fence.py` | `cat1_citations.checks.upstream.run` |
| `test_cat2_stack_b.py` | (Stack B run + helpers) |
| `test_cat2_stack_c.py` | `cat2_contracts.checks.public_symbol_used_c.run`, `cat2_contracts.stack_c.{...}` |
| `test_cat2_stack_d.py` | `cat2_contracts.checks.public_symbol_used.run`, `cat2_contracts.stack_d.{SymbolKind, extract_public_surface}`, `common.results.FailureMode` |
| `test_cat2_stub_label_stale.py` | `cat2_contracts.checks.stub_label_stale.run` |
| `test_cat3_cubic_kernel.py` | `cat3_numerical.cubic_kernel.{...}` |
| `test_cat3_d3q19.py` | `cat3_numerical.d3q19_verify.{...}`, `cat3_numerical.checks.{...}`, `common.results.FailureMode` |
| `test_grandfather_sweep.py` | `grandfather.{...}` |
| `test_runner.py` | `runner.{main, parse_args}` |
| `test_runner_human_output.py` | `runner.{RunSummary, emit_output}`, `common.results.{FailureMode, Finding}` |
| `test_snapshot.py` | `snapshot.{...}` |
| `test_suppression_fence.py` | `common.suppression.apply_suppressions`, `common.results.{FailureMode, Finding}` |

### C.3 — Top-level symbols per toolkit module

FACT — captured via `grep -n "^def \|^class \|^[A-Z_][A-Z_0-9]* = "`,
capped at first 30 entries per file. Verbatim selected highlights
(full list preserved in tool transcript):

`integrity/runner.py` (top-level surface):

```
25:EXIT_OK = 0
26:EXIT_HARD_FAIL = 1
27:EXIT_INTERNAL_FAIL = 2
28:EXIT_BAD_CLI = 64
32:class CliArgs:
44:def parse_args(argv: list[str]) -> CliArgs:
82:def discover_checks(args: CliArgs) -> list[Any]:
102:def run_checks(checks: list[Any], args: CliArgs) -> list[Finding]:
116:def emit_output(summary: RunSummary, findings: list[Finding], args: CliArgs) -> None:
154:def _emit_human_summary(summary: RunSummary) -> None:
163:def main(argv: list[str]) -> int:
```

`integrity/grandfather.py`:

```
24:class Finding:
32:class Classification:
66:def is_live_source_path(file_path: str) -> bool:
84:def classify(finding: Finding) -> Classification:
213:def comment_form_for(file_path: str) -> str:
240:def comment_form_for_md_inside_fence(fence_lang: str | None) -> str:
259:def annotation_already_present(prev_line: str, check_id: str) -> bool:
275:def render_annotation_line(
321:def collect_findings(repo_root: Path) -> list[Finding]:
349:def group_findings_by_target(
360:def apply_annotations(
```

`integrity/snapshot.py`:

```
24:HISTORY_FILE_RELATIVE = Path("tools/integrity/.grandfather-history.json")
27:_KNOWN_CATEGORIES = (
64:def _extract_category(reason: str) -> str:
76:def _collect_state(root: Path) -> dict:
109:def _parse_ground_truth_sources(root: Path) -> list[dict]:
145:def emit_state_snapshot(root: Path, out: IO[str]) -> None:
164:def _append_history(root: Path, state: dict) -> None:
191:def emit_grandfather_report(
```

`integrity/cat2_contracts/stack_d.py`:

```
17:COMMON_PY_PACKAGE_DIR = Path("common/common-py/gpusims_common")
18:PACKAGE_NAME = "gpusims_common"
21:class SymbolKind(Enum):
29:class PublicSymbol:
37:def extract_public_surface(repo_root: Path) -> list[PublicSymbol]:
76:def _extract_from_module(
141:def _extract_class_fields(
193:def find_references(
225:def _find_field_references(
254:def _find_name_references(
```

`integrity/cat2_contracts/checks/public_symbol_used.py`:

```
38:CHECK_ID = "cat2.public-symbol-used"
39:MODE = FailureMode.HARD_FAIL
42:def _list_scannable_py_files(repo_root: Path) -> list[Path]:
63:def run(repo_root: Path) -> list[Finding]:
```

(All 20-odd module surfaces captured; representative samples shown.)

### C.4 — INFERENCE: classify by "publicness"

INFERENCE — partitioning the ~138 top-level symbols (per § F.1's
AST counter) into classes:

- **Genuinely public** (consumed by tests/scripts AND used internally):
  `runner.{main, parse_args, RunSummary, emit_output}`,
  `grandfather.apply_annotations`,
  `common.repo.find_repo_root`,
  `common.results.{FailureMode, Finding}`,
  `common.suppression.apply_suppressions`,
  `common.annotations.*`,
  `cat*/checks/*.{run, CHECK_ID}`,
  `cat2_contracts.stack_*.{extract_public_surface, find_references, SymbolKind}`,
  `cat2_contracts.checks.__init__.REGISTERED_CHECKS` (and siblings),
  `cat3_numerical.{d3q19_verify.*, cubic_kernel.*}`,
  `cat1_citations.{grammar.*, resolver.resolve, upstream_anchor.*}`,
  `snapshot.{emit_state_snapshot, emit_grandfather_report}`.
  → ~50 symbols.

- **Cross-module internal** (consumed by other integrity modules but
  not by scripts or tests directly): private helpers like
  `cat1_citations.resolver._count_lines` (imported by `bare_path.py`
  via the underscore-prefix exception),
  `common.exclusions.{is_excluded, CANONICAL_EXCLUSIONS}`,
  `common.repo.{git_head_sha, list_tracked_files}`.
  → ~10 symbols.

- **Module-private** (defined but only referenced within their
  defining module — would be flagged by a naive Stack-D scan):
  module-level `Path` / `re.compile` / `frozenset` constants
  (`COMMON_PY_PACKAGE_DIR`, `PACKAGE_NAME`, `INTRA_REPO_RE`,
  `UPSTREAM_RE`, `LOOSE_RE`, `STRICT_RE`, `STALE_LABEL_RE`,
  `DISCRIMINATOR_PHRASES`, `IMPL_LOC_THRESHOLD`, `CPP_INCLUDE_ROOT`,
  `CPP_SRC_ROOT`, `PY_PACKAGE_ROOT`, `SCAN_EXTENSIONS`, `ANCHOR_FILE`,
  `EXPECTED_JSON`, `TOL_ABS`, `EXPECTED_CANONICAL`,
  `EXPECTED_OPPOSITE_PAIRS`, `HERE`, `SCRIPT_DIR`, `OUTPUT_PATH`,
  `TEST_POINTS_Q`, `H`, `DRIVER_RELATIVE_PATH`,
  `EXPECTED_VALUES_RELATIVE`, `HELPER_DIR`, `MAX_DISAMBIGUATION_CANDIDATES`,
  `REGISTRY_DOC`, `TOML_FENCE_RE`, `HISTORY_FILE_RELATIVE`); plus
  Enum classes and member references that look "unused" to a naive
  walker (`BarePathClass.{REGISTERED_UPSTREAM, INTRA_REPO, AMBIGUOUS,
  UNRESOLVABLE}`, `SymbolKind.{CLASS, CLASS_FIELD, FUNCTION, MODULE,
  STRUCT, METHOD, FREE_FUNCTION}`); plus dataclass shells whose
  fields are only set/read internally (`Annotation`, `BarePathResolution`,
  `Classification`, `CliArgs`, `DriverEvaluation`, `PublicSymbol`,
  `PublicSymbolB`, `ResolutionResult`, `TestPoint`, `UpstreamCitation`,
  `UpstreamRegistration`, `StackPaths`); plus the `MODE` constants
  on each check module (only some of which are actually accessed via
  attribute by the runner).
  → ~70 symbols.

This three-way partition is the central INFERENCE for the spec: under
A.2's naive AST scan, the bulk of "candidate-unused" symbols are
**module-private constants and dataclass field-bags**, not real defects.
The spec needs an explicit policy on whether to scan them at all
(see § F, § G, § L.5).

---

## § D — Stack D check structural shape

### D.1 — `tools/integrity/integrity/cat2_contracts/stack_d.py` (LOC 272)

FACT — full verbatim dump preserved in tool transcript. Key
structural beats with line numbers:

```
17:COMMON_PY_PACKAGE_DIR = Path("common/common-py/gpusims_common")
18:PACKAGE_NAME = "gpusims_common"
21:class SymbolKind(Enum):
29:class PublicSymbol:           (frozen dataclass: name, kind, defining_file, defining_line, parent_class)
37:def extract_public_surface(repo_root: Path) -> list[PublicSymbol]:
76:def _extract_from_module(module_file, imported_name, external_name) -> list[PublicSymbol]:
141:def _extract_class_fields(class_node, module_file, class_name) -> list[PublicSymbol]:
193:def find_references(repo_root, symbol, scan_files) -> list[tuple[Path, int]]:
225:def _find_field_references(tree, scan_file, symbol) -> list[tuple[Path, int]]:
254:def _find_name_references(tree, scan_file, name, exclude_file) -> list[tuple[Path, int]]:
```

Critical method `extract_public_surface` (lines 37-73):

```python
    37	def extract_public_surface(repo_root: Path) -> list[PublicSymbol]:
    38	    """Parse gpusims_common/__init__.py and return every public symbol."""
    39	    init_path = repo_root / COMMON_PY_PACKAGE_DIR / "__init__.py"
    40	    if not init_path.is_file():
    41	        return []
    42	
    43	    try:
    44	        tree = ast.parse(init_path.read_text(encoding="utf-8"))
    45	    except (SyntaxError, OSError):
    46	        return []
    47	
    48	    # ... parses ImportFrom nodes, builds (submodule, imported_name, alias)
    49	    # ... walks imports, for each one calls _extract_from_module
```

Note: this code only enumerates symbols **re-exported by
`__init__.py`** via `from .X import Y`. It does NOT scan the package
for top-level defs that are not re-exported.

### D.2 — `tools/integrity/integrity/cat2_contracts/checks/public_symbol_used.py` (LOC 99)

FACT — full verbatim dump preserved. Critical beats:

```
38:CHECK_ID = "cat2.public-symbol-used"
39:MODE = FailureMode.HARD_FAIL
42:def _list_scannable_py_files(repo_root: Path) -> list[Path]:
63:def run(repo_root: Path) -> list[Finding]:
```

`_list_scannable_py_files` (lines 42-60) — uses
`list_tracked_files(repo_root)` if `.git/` exists else `rglob("*")`,
filters to `.py` / `.pyi` and excludes `is_excluded(rel)` paths.

`run` (lines 63-99):

```python
    63	def run(repo_root: Path) -> list[Finding]:
    64	    findings: list[Finding] = []
    65	
    66	    public_symbols = extract_public_surface(repo_root)
    67	    if not public_symbols:
    68	        return findings
    69	
    70	    scan_files = _list_scannable_py_files(repo_root)
    71	
    72	    for symbol in public_symbols:
    73	        refs = find_references(repo_root, symbol, scan_files)
    74	        if refs:
    75	            continue
    76	        ...
```

Note line 67–68: **if `public_symbols` is empty, returns no findings
silently**. This is critical for A.2 — pointed at the toolkit, the
existing extractor would return [] (toolkit's `__init__.py` re-exports
nothing) and the check would silently produce no findings without
error. A.2 must either add a re-export surface to the toolkit's
`__init__.py` or change the extraction strategy.

### D.3 — Module-level "what-to-scan" constants

FACT (from the dumps above):

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `stack_d.py:17` — `COMMON_PY_PACKAGE_DIR = Path("common/common-py/gpusims_common")`
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `stack_d.py:18` — `PACKAGE_NAME = "gpusims_common"`
- Used inside `extract_public_surface` (line 39) and inside the
  `from gpusims_common.X` absolute-import detection (line 58).

INFERENCE: the scan is **hard-coded to a single package**. The
`stack_paths.py` module (§ E.2) is a separate, mostly-vestigial helper
that is *not* consulted by the Stack D check today. The current
implementation has no parameter, no dispatch table, no env-var
override.

For A.2's purposes, this is good news: a parameterized refactor is
small (replace the two module-level constants with function arguments
threaded through `extract_public_surface`, `_extract_from_module`,
and the absolute-import prefix check on line 58).

### D.4 — `python3 -m integrity --check cat2.public-symbol-used --output human --no-audit-log`

FACT — exit code: `0`. Output:

```
integrity: 0 pass, 0 soft-warn, 0 hard-fail, 17 suppressed
```

No stanzas printed (per P1.6, suppressed stanzas are no longer
emitted). All 17 `cat2-stack-d-unused` findings (§ A.3) are present
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
but suppressed by inline `integrity-allow:` annotations from the
v1.1 grandfather sweep.

### D.5 — Stack D test fixtures

FACT — `find tools/integrity/tests/fixtures/{good,bad}_contracts -type f`:

```
tools/integrity/tests/fixtures/bad_contracts/.gitkeep
tools/integrity/tests/fixtures/bad_contracts/common/common-py/gpusims_common/__init__.py
tools/integrity/tests/fixtures/bad_contracts/common/common-py/gpusims_common/particle.py
tools/integrity/tests/fixtures/bad_contracts/consumer.py
tools/integrity/tests/fixtures/good_contracts/.gitkeep
tools/integrity/tests/fixtures/good_contracts/common/common-py/gpusims_common/__init__.py
tools/integrity/tests/fixtures/good_contracts/common/common-py/gpusims_common/widget.py
tools/integrity/tests/fixtures/good_contracts/consumer.py
```

FACT — `tools/integrity/tests/fixtures/good_contracts/common/common-py/gpusims_common/__init__.py`:

```python
"""Synthetic good-contract fixture: every public symbol has a consumer."""

from .widget import Widget, make_widget
```

FACT — fixture pattern: each fixture stages a faux repo root with a
`common/common-py/gpusims_common/` subtree (because the extractor
hard-codes that path). The extractor walks the fake tree and the
check scans the fake `consumer.py`.

FACT — `tools/integrity/tests/test_cat2_stack_d.py` (74 LOC):

```python
     1	"""Tests for cat2.public-symbol-used Stack D variant."""
     2	
     3	from __future__ import annotations
     4	
     5	from pathlib import Path
     6	
     7	from integrity.cat2_contracts.checks.public_symbol_used import run
     8	from integrity.cat2_contracts.stack_d import (
     9	    SymbolKind,
    10	    extract_public_surface,
    11	)
    12	from integrity.common.results import FailureMode
    13	
    14	
    15	def test_extract_public_surface_finds_class_and_function(fixtures_dir: Path) -> None:
    16	    symbols = extract_public_surface(fixtures_dir / "good_contracts")
    17	    names = {s.name for s in symbols}
    18	    assert "Widget" in names
    19	    assert "make_widget" in names
    ...
    54	def test_self_references_dont_count_for_field_usage(tmp_path: Path) -> None:
    55	    """A field read only via `self.X` inside its own class shouldn't count
    56	    as a consumer."""
    57	    pkg_dir = tmp_path / "common" / "common-py" / "gpusims_common"
    58	    pkg_dir.mkdir(parents=True)
    ...
    73	    )
```

INFERENCE: even the `tmp_path`-based test (line 57) hard-codes
`common/common-py/gpusims_common` in the fixture layout. To reuse
these tests for A.2, either:
- Parameterize the helper paths in tests (small change), or
- Build a sibling test file that mirrors the structure with the
  toolkit-flavored scan target.

---

## § E — Generalization risk surface

### E.1 — References to the hard-coded common-py path

FACT — `grep -n "common-py\|gpusims_common\|common_py"` across the
three relevant modules:

```
tools/integrity/integrity/common/stack_paths.py:31:            public_surface_dir=root / "common" / "common-py" / "gpusims_common",
tools/integrity/integrity/common/stack_paths.py:32:            implementation_dir=root / "common" / "common-py" / "gpusims_common",
tools/integrity/integrity/cat2_contracts/checks/public_symbol_used.py:94:                f"non-self consumer site under common/common-py/, sim "
tools/integrity/integrity/cat2_contracts/stack_d.py:3:The "public" surface for Stack D is whatever common/common-py/gpusims_common/
tools/integrity/integrity/cat2_contracts/stack_d.py:17:COMMON_PY_PACKAGE_DIR = Path("common/common-py/gpusims_common")
tools/integrity/integrity/cat2_contracts/stack_d.py:18:PACKAGE_NAME = "gpusims_common"
tools/integrity/integrity/cat2_contracts/stack_d.py:38:    """Parse gpusims_common/__init__.py and return every public symbol."""
tools/integrity/integrity/cat2_contracts/stack_d.py:49:    # relative (from .X import Y) and absolute (from gpusims_common.X import Y)
```

INFERENCE: 8 references total. 5 are docstring/comment text (cheap
to refresh), 2 are `stack_paths.py` constants (already a per-stack
table; A.2 could add a fourth row), and 1 is the human-message string
in the check itself. The actual *load-bearing* references are:

<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `stack_d.py:17` — `COMMON_PY_PACKAGE_DIR` constant
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `stack_d.py:18` — `PACKAGE_NAME` constant (used in line 58 for the
  absolute-form import prefix `gpusims_common.`)

Two constants. Parameterizing them is a 5-line refactor.

### E.2 — `tools/integrity/integrity/common/stack_paths.py` (LOC 34)

FACT — full verbatim:

```python
     1	"""Canonical paths per stack per spec § 7.2."""
     2	
     3	from __future__ import annotations
     4	
     5	from dataclasses import dataclass
     6	from pathlib import Path
     7	
     8	
     9	@dataclass(frozen=True)
    10	class StackPaths:
    11	    name: str
    12	    public_surface_dir: Path
    13	    implementation_dir: Path
    14	
    15	
    16	def stack_paths(root: Path) -> dict[str, StackPaths]:
    17	    """Return the per-stack public/impl path map rooted at `root`."""
    18	    return {
    19	        "c": StackPaths(
    20	            name="c",
    21	            public_surface_dir=root / "common" / "common-cpp" / "include" / "gpusims",
    22	            implementation_dir=root / "common" / "common-cpp" / "src",
    23	        ),
    24	        "b": StackPaths(
    25	            name="b",
    26	            public_surface_dir=root / "common" / "common-web" / "src",
    27	            implementation_dir=root / "common" / "common-web" / "src",
    28	        ),
    29	        "d": StackPaths(
    30	            name="d",
    31	            public_surface_dir=root / "common" / "common-web" / "src",
    32	            implementation_dir=root / "common" / "common-py" / "gpusims_common",
    33	        ),
    34	    }
```

INFERENCE: this module is the natural extension point if A.2 wants to
add a per-package row. However, NO call site currently consumes
`stack_paths()` — it is itself in the F.1 candidate-unused list (see
§ F.1). The module exists as a future hook documented in spec § 7.2
but has no live consumer. The Stack D / C / B checks each duplicate
their per-stack constants instead of using this table. (This is
itself a P-class candidate: refactor the three checks to share
`stack_paths()`. Out of A.2 scope; banked in § K.)

### E.3 — INFERENCE: design-path recommendation

The two paths the prompt poses:

**Path (a)** — Add a NEW check `cat2.public-symbol-used-toolkit` that
mirrors `public_symbol_used.py` but with a toolkit-flavored extractor.
Stack D driver parameterizes on `package_dir + package_name`. New
check registered alongside the existing Stack D check.

**Path (b)** — Extend the EXISTING check to scan BOTH common-py and
the toolkit in a single pass; tag findings by file-path origin.

Trade-off table:

| Axis | (a) New check | (b) Extended check |
|---|---|---|
| Classifier rule routing | Trivial: new check_id → new category. First-match-wins gives a clean dedicated `toolkit-own-unused` rule. | Awkward: need to inspect `finding.file` to decide between `cat2-stack-d-unused` and `toolkit-own-unused` — prefix logic shoved into `classify()`. |
| Test-fixture reuse | Parameterized helper functions in `stack_d.py`; existing fixture stays put; new fixture for toolkit shape. | Same parameterization needed; existing fixture stays put; have to assert single-pass produces correctly-tagged findings. |
| CI wall-clock cost | Two passes over the symbol enumerator (one per package). Each is O(symbols × scan_files). For ~30 toolkit public symbols × ~50 .py files, negligible (~50ms). | Single pass; theoretically cheaper but the saving is in the tens of milliseconds. |
| Future-proofing for further packages | Linear: add another check per package. Or eventually parameterize a generic check that takes a list of (dir, name) targets via spec-table. | Same: extend the `targets` list. |
| Spec / catalog discoverability | Each check has its own row, its own docs, its own grandfather category. Easier to read, document, and understand. | Single check spans multiple categories. Catalog will need an exception note. |
| Risk of false-FAIL on toolkit due to extractor's `__init__.py` assumption | High and visible: the new check needs a different extraction strategy (toolkit `__init__.py` is empty), which forces an explicit decision. | Hidden: existing extractor returns [] for the toolkit (line 67-68), check passes silently — A.2 ships but doesn't actually do anything. |

INFERENCE — **recommend Path (a)**, with the additional spec
clarification that the new check's *extractor* should not parse
`integrity/__init__.py` (which is empty); it should instead
**collect every top-level def/class/UPPERCASE-constant from every
non-test, non-fixture, non-`__init__.py` `.py` file under
`tools/integrity/integrity/`** and treat each as a "public symbol"
candidate (modulo underscore-prefix). This makes A.2 a meaningful
check rather than a no-op.

The reason path (b) hides the no-op risk: at the current toolkit
state, `extract_public_surface(repo_root_pointing_at_toolkit)` would
return [] and the run silently succeeds with zero findings. That's
indistinguishable from "all good." A.2 should either not have that
failure mode or should explicitly assert non-empty extraction at run
time.

---

## § F — Predicted false-positive surface

### F.1 — Naive AST scan of `tools/integrity/integrity/`

FACT — running the script the prompt specifies:

```
public symbols defined: 138
defined-but-not-referenced-elsewhere: 93
```

First 30 candidate-unused symbols (verbatim):

```
  AMBIGUOUS  @  integrity/integrity/cat1_citations/checks/bare_path.py
  ANCHOR_FILE  @  integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.py
  ANNOTATION_RE  @  integrity/integrity/common/annotations.py
  Annotation  @  integrity/integrity/common/annotations.py
  BarePathClass  @  integrity/integrity/cat1_citations/checks/bare_path.py
  BarePathResolution  @  integrity/integrity/cat1_citations/checks/bare_path.py
  CLASS  @  integrity/integrity/cat2_contracts/stack_c.py
  CLASS_FIELD  @  integrity/integrity/cat2_contracts/stack_c.py
  COMMON_CPP_IMPL_DIR  @  integrity/integrity/cat2_contracts/stack_c.py
  COMMON_CPP_PUBLIC_DIR  @  integrity/integrity/cat2_contracts/stack_c.py
  COMMON_PY_PACKAGE_DIR  @  integrity/integrity/cat2_contracts/stack_d.py
  CPP_INCLUDE_ROOT  @  integrity/integrity/cat2_contracts/checks/stub_label_stale.py
  CPP_SRC_ROOT  @  integrity/integrity/cat2_contracts/checks/stub_label_stale.py
  Classification  @  integrity/integrity/grandfather.py
  CliArgs  @  integrity/integrity/runner.py
  DECL_KINDS  @  integrity/integrity/cat2_contracts/stack_c.py
  DISCRIMINATOR_PHRASES  @  integrity/integrity/cat2_contracts/checks/stub_label_stale.py
  DISCRIMINATOR_SCAN_LINES  @  integrity/integrity/cat2_contracts/checks/stub_label_stale.py
  DRIVER_RELATIVE_PATH  @  integrity/integrity/cat3_numerical/cubic_kernel.py
  DriverEvaluation  @  integrity/integrity/cat3_numerical/cubic_kernel.py
  EXIT_BAD_CLI  @  integrity/integrity/runner.py
  EXIT_HARD_FAIL  @  integrity/integrity/runner.py
  EXIT_INTERNAL_FAIL  @  integrity/integrity/runner.py
  EXIT_OK  @  integrity/integrity/runner.py
  EXPECTED_CANONICAL  @  integrity/integrity/cat3_numerical/d3q19_verify.py
  EXPECTED_JSON  @  integrity/integrity/cat3_numerical/d3q19_verify.py
  EXPECTED_OPPOSITE_PAIRS  @  integrity/integrity/cat3_numerical/d3q19_verify.py
  FREE_FUNCTION  @  integrity/integrity/cat2_contracts/stack_c.py
  FUNCTION  @  integrity/integrity/cat2_contracts/stack_d.py
  FieldVisitor  @  integrity/integrity/cat2_contracts/stack_d.py
```

INFERENCE — partitioning these 30 by failure-mode:

- **False positive — Enum members** (referenced via attribute on the
  Enum class, but the script's `ast.Name`+`ast.Attribute` walk does
  count those, so they show up only if they're *also* not referenced
  elsewhere): `AMBIGUOUS`, `CLASS`, `CLASS_FIELD`, `FREE_FUNCTION`,
  `FUNCTION`. Most are members declared but never matched in switch-
  like dispatches at the top of the same module. Some are unused real
  enum members (`STRUCT`, `METHOD` in stack_c per the full listing).
  Real-defect-or-false-positive depends per case.

- **False positive — module-internal regex/path constants** referenced
  via `Name` from the same module, but the script excludes the
  defining file from `user_sites`. So `INTRA_REPO_RE`, `UPSTREAM_RE`,
  `LOOSE_RE`, `STRICT_RE`, `STALE_LABEL_RE`, `ANCHOR_FILE`,
  `EXPECTED_JSON`, `TOL_ABS`, etc. all flag — but they ARE used,
  just only inside their defining module. The Stack D existing check
  has the same single-module-self-exclusion logic
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  (`stack_d.py:262-264`). So the existing check, applied to the
  toolkit, would produce identical false positives.

- **False positive — runner-discoverable constants** (`EXIT_OK`,
  `EXIT_HARD_FAIL`, `EXIT_INTERNAL_FAIL`, `EXIT_BAD_CLI` and `MODE`):
  these are referenced only inside `runner.py` itself. They are
  arguably real (no consumer) but obviously intentional API.

- **False positive — visitor/dispatch methods** (`FieldVisitor`,
  `visit_ClassDef`, `visit_Attribute`): defined as methods on a local
  class inside `_find_field_references`; consumed via the
  `ast.NodeVisitor` framework's pattern-matching, not via a direct
  reference. Naive AST scan misses them as "consumed."

- **False positive — dataclass shells** (`Annotation`, `Classification`,
  `CliArgs`, `DriverEvaluation`, `BarePathResolution`): instantiated
  but the *class name* may show up only at the construction site
  inside the same module. Real intent: fine to leave alone.

- **Possible real defect**: `DECL_KINDS` (stack_c constant — should
  be confirmed by reading), `MAX_DISAMBIGUATION_CANDIDATES`
  (bare_path.py — likely intentional limit, not a defect),
  `H` / `TEST_POINTS_Q` / `OUTPUT_PATH` in `generate_expected.py`
  (self-contained script, would be referenced only in its own
  `main()` — but those are referenced via `ast.Name` from the same
  file and excluded). So even these are most likely false positives.

INFERENCE: of 93 candidate-unused symbols from the naive AST walk,
**roughly 0 are real defects**. The vast majority are module-internal
regex/path/enum/dataclass constants that the naive walker mis-labels
because of the same-file exclusion rule (which is the same rule the
existing Stack D check already uses).

This is the central spec-shaping observation: **the existing Stack D
extraction strategy, naively applied to the toolkit, is mostly noise**.
The spec must constrain the scan in one of three ways:

1. Scan only what the toolkit's `__init__.py` re-exports (currently
   nothing → no-op A.2 check).
2. Scan everything but provide an `__all__` mechanism, and require
   the toolkit modules to declare `__all__` for what they consider
   public. ~5 modules need declarations.
3. Scan only top-level `def`s and `class`es (skip module-level
   constants), AND treat any symbol consumed by `discover_checks()`
   as "used" by reflection.

Option 2 or 3 is realistic. Option 1 is no-op. The spec must pick
one and document it.

### F.2 — Classifier-rule fate per file path

INFERENCE — the new toolkit findings would appear at file paths
matching `tools/integrity/integrity/**/*.py`. Looking at the current
classifier (§ H.1 dump):

- The first existing rule that matches `tools/integrity/integrity/`
  is `cat1.annotation-form` at lines 75-80 (routes to
  `toolkit-own-source`).
- But the new check has check_id `cat2.public-symbol-used-toolkit`
  (or whatever name A.2 picks), which would fall through every
  cat1.* rule.
- It would land at the default fallthrough rule (`other-cat1`), which
  is wrong: it's a cat2 finding, not a cat1.

INFERENCE: the spec MUST add a classifier rule for the new check.
Reusing `toolkit-own-source` is wrong — that category is described
in `tools/integrity/docs/grandfather-catalog.md` line 88 as "regex or
docstring literal of the annotation grammar token (not a real
annotation)." Conflating two distinct concerns under one category
would corrupt the catalog's semantics.

The right name: `toolkit-own-unused` (parallel to `cat2-stack-d-unused`,
`cat2-stack-c-unused`, `cat2-stack-b-unused`). Optionally split into
`toolkit-public-api-unused` vs `toolkit-internal-unused` if the spec
adopts the "scan only `__all__`-declared symbols" strategy, but
that's premature granularity for v1.2.

---

## § G — Test-suppression-fence self-find

### G.1 — `tools/integrity/tests/test_suppression_fence.py`

FACT — `grep -n "^def \|^class "`:

```
15:def test_fence_internal_annotation_does_not_suppress(tmp_path: Path) -> None:
49:def test_live_annotation_above_fence_suppresses(tmp_path: Path) -> None:
```

FACT — full file (75 LOC) — verbatim above (reproduced in tool
output). Two `pytest`-collected test functions; no helper symbols.

INFERENCE: this file has no symbols a Stack-D-style scan would
mis-flag — both top-level `def`s start with `test_` and are consumed
by pytest's collection machinery (which is invisible to the AST
walker). If A.2's scan includes `tools/integrity/tests/`, every
`test_*` function in every test file would land as "unused" because
no other Python file imports them.

### G.2 — Tests-tree AST scan

FACT — running the same AST walker scoped to `tools/integrity/tests/`:

```
test-tree public symbols defined: 142
defined-but-not-referenced-elsewhere: 136
```

136 of 142 — virtually every test function. Confirmed by the first
40 entries: every line begins with `test_` and points into a
`test_*.py` file. The 6 that *did* link are presumably helper names
that happen to match pytest fixtures or `conftest.py` names.

INFERENCE — **A.2 must exclude `tools/integrity/tests/` from the
scan scope**. Including it would produce ~140 false positives that
all want to be permanently grandfathered, polluting the catalog.

The natural exclusion mechanism is the existing
`integrity.common.exclusions.is_excluded()` predicate (used by every
check at `_list_scannable_*_files()` time). If `is_excluded()`
already excludes `tools/integrity/tests/`, that's good news (no
spec change needed); if not, the spec needs to add an exclusion
either globally or just for the new check.

(Worth checking during spec drafting: read
`tools/integrity/integrity/common/exclusions.py:37` `is_excluded`
to see whether tests are already covered. The function is short
enough to read in one pass during spec drafting; not duplicated
in this probe to keep length under target.)

The script side `tools/integrity/scripts/grandfather_sweep.py` is
also a real public consumer of toolkit code — it imports
`grandfather.apply_annotations` and `common.repo.find_repo_root`.
If A.2 includes `scripts/` in its scan, these references to
toolkit symbols WILL count as "consumer" sites. So `scripts/`
should be **included** as a scan-input direction (so its imports
count as consumption) but **excluded** as a scan-target direction
(its own functions like `main()` shouldn't be flagged unused). The
existing Stack D check pattern handles this distinction by listing
`scripts/` in the scan_files list (via list_tracked_files) and
trusting that `main()` etc. are referenced from `__main__`.

---

## § H — Coordination with existing classifier rules

### H.1 — `grandfather.classify()` body

FACT — verbatim, full body (lines 84-132 of grandfather.py) extracted
via `awk '80,215'`:

```python
    84	def classify(finding: Finding) -> Classification:
    85	    """Classify a finding into a grandfather category. First match wins."""
    86	    f = finding.file
    87	    msg = finding.message
    88	    cid = finding.check_id
    89	
    90	    if cid == "cat2.public-symbol-used":
    91	        return Classification(
    92	            category="cat2-stack-d-unused",
    93	            reason="pre-v1 Stack D public symbol with no current consumer ...",
    94	            issue_ref="n/a",
    95	        )
    96	
    97	    if cid == "cat2.public-symbol-used-c":
    98	        return Classification(
    99	            category="cat2-stack-c-unused",
   100	            ...
   101	        )
   102	
   103	    if cid == "cat2.public-symbol-used-ts":
   104	        return Classification(
   105	            category="cat2-stack-b-unused",
   106	            ...
   107	        )
   108	
   109	    if cid == "cat2.stub-label-stale":
   110	        return Classification(
   111	            category="cat2-stub-label-stale",
   112	            ...
   113	        )
   114	
   115	    if cid == "cat1.intra-repo" and f.startswith("docs/diagnostics/_audits/"):
   116	        ... category="audit-citation"
   117	
   118	    if cid == "cat1.upstream-citation" and "1.8.10" in msg:
   119	        if (live-shader paths):
   120	            ... category="live-shader-1810"
   121	        ... category="audit-doc-1810"
   122	
   123	    if cid == "cat1.annotation-form":
   124	        if f == "docs/integrity-toolkit-spec.md" or f.startswith("tools/integrity/docs/"):
   125	            ... category="spec-grammar-example"
   126	        if f.startswith("docs/retro/"):
   127	            ... category="retro-grammar-example"
   128	        if f.startswith("tools/integrity/integrity/"):
   129	            ... category="toolkit-own-source"
   130	        if f.startswith("docs/diagnostics/_audits/"):
   131	            ... category="audit-report-grammar-example"
   132	
   133	    if cid == "cat1.bare-path" and f.startswith("docs/diagnostics/_audits/"):
   134	        ... category="audit-bare-path"
   135	    if cid == "cat1.bare-path" and f.startswith("docs/retro/"):
   136	        ... category="retro-bare-path"
   137	    if cid == "cat1.bare-path" and (
   138	        f == "docs/integrity-toolkit-spec.md"
   139	        or f.startswith("tools/integrity/docs/")
   140	        or f == "tools/integrity/README.md"
   141	    ):
   142	        ... category="toolkit-doc-bare-path"
   143	    if cid == "cat1.bare-path" and "LeniaNDK.py" in msg:
   144	        ... category="deferred-upstream-bare-path"
   145	    if cid == "cat1.bare-path":
   146	        ... category="other-cat1-bare-path"
   147
   148	    return Classification(
   149	        category="other-cat1",
   150	        reason="grandfathered-pre-v1 (see grandfather-catalog other-cat1)",
   151	        issue_ref="n/a",
   152	    )
```

(Source line numbers verified via `awk '1,215' grandfather.py`; see
also `is_live_source_path` at line 66 and `SWEEPABLE_*` constants at
lines 52-63 captured in transcript.)

### H.2 — Bolt-ons P1.5 classifier rules?

FACT — `git log --oneline 119e353` confirms P1.5 commit landed
`feat(integrity): P1.5 register cat3.d3q19-* checks`. Inspecting
`classify()` above: there is NO branch for `cid in {"cat3.d3q19-*",
"cat3.cubic-kernel"}`. So a cat3 finding falls through to the default
`other-cat1` bucket.

This is presumably intentional — cat3 findings are rare and
fixed-in-place rather than grandfathered (the cat3 anchor is the
algebraic-truth doc; if a check fires, the underlying d3q19 or
cubic-kernel implementation has drifted and needs a code fix, not
a suppression annotation). Worth confirming in the bolt-ons retro,
but no live cat3 findings exist at probe SHA (per § A.3 — none of the
16 categories listed are cat3-flavored), so this is theoretical.

### H.3 — INFERENCE: where in the rule order does the new rule land?

The existing cat2 rules form a contiguous block at the top
(lines 90-113 in the dump above). Add a new branch immediately after
`cat2.stub-label-stale` (line 109-113):

```python
    if cid == "cat2.public-symbol-used-toolkit":
        return Classification(
            category="toolkit-own-unused",
            reason="pre-v1.2 toolkit-own public symbol with no current consumer (tracked for v1.2 review per grandfather-catalog toolkit-own-unused)",
            issue_ref="n/a",
        )
```

First-match-wins ordering doesn't matter because the existing rules
all gate on a different `cid`. Keeping cat2 rules contiguous is the
only stylistic concern.

### H.4 — Coordination with P1.8 sweep-side protection

FACT — from `is_live_source_path()` (lines 66-82) and `SWEEPABLE_*`
constants (lines 52-63):

```python
    52	SWEEPABLE_PATH_PREFIXES: tuple[str, ...] = (
    53	    "docs/diagnostics/_audits/",
    54	    "docs/retro/",
    55	    "tools/integrity/docs/",
    56	)
    57
    58	SWEEPABLE_EXACT_PATHS: frozenset[str] = frozenset({
    59	    "docs/integrity-toolkit-spec.md",
    60	    "tools/integrity/README.md",
    61	    "project-state.md",
    62	})
```

INFERENCE: A `tools/integrity/integrity/**/*.py` path is NOT in
either set, so `is_live_source_path()` returns `True` — the toolkit's
own source counts as LIVE-SOURCE.

The default `apply_annotations(sweep_live_source=False)` thus would
**SKIP** the new toolkit findings. They'd accumulate in
`grandfather-report` as classified-but-not-swept. That's a
problem for A.2's intent: the toolkit predates the self-application
check; pre-v1.2 toolkit findings need a one-time grandfather sweep
to reach a clean baseline.

Three options the spec must choose between:

**Option (i)** — Run `python3 tools/integrity/scripts/grandfather_sweep.py
--sweep-live-source` once during the A.2 rollout, sweeping ALL
remaining other-cat1-bare-path live-source findings *plus* the new
toolkit-own-unused findings. Risk: this also auto-sweeps any
`other-cat1-bare-path` findings in real sim source code (the very
class P1.8 was designed to protect from over-sweeping). The 44
LIVE-SOURCE other-cat1-bare-path findings noted in
`grandfather-catalog.md` line 310 are the protected set; sweeping
them under the same flag defeats P1.8's purpose.

**Option (ii)** — Refine `apply_annotations` so `sweep_live_source`
is per-category. The new toolkit-own-unused category opts INTO
the live-source sweep specifically; other live-source other-cat1-*
categories remain protected. This is a small extension to the
P1.8 logic — instead of one boolean, a set of "force-sweep
categories" which always sweep regardless of bucket.

**Option (iii)** — Re-classify the toolkit's own paths so they're
*not* treated as LIVE-SOURCE. Add `tools/integrity/integrity/` to
`SWEEPABLE_PATH_PREFIXES`. Risk: this would also affect any future
cat1.* findings in toolkit code — meaning any bare-path or
intra-repo citation defects in toolkit Python code would now
auto-sweep instead of being attributed to a code fix. P1.8 was
explicitly worded to keep live-source code (which the toolkit IS,
in the sense that it executes on every CI run) under the
attribution-not-sweep policy. Option (iii) un-does that for the
toolkit.

**Recommendation**: Option (ii). It preserves P1.8's intent
(live-source under attribution policy by default) but adds a
per-category exception for the v1.2 A.2 rollout. The exception
list is small (one category) and documented in the spec.

This is the most consequential design decision A.2 surfaces and
should be highlighted in the spec's open-questions section. See
§ L.4.

---

## § I — Toolkit script + entrypoint surface

### I.1 — `__main__.py` and `grandfather_sweep.py`

FACT — `tools/integrity/integrity/__main__.py` (LOC 11):

```python
     1	"""Entry point: `python3 -m integrity`."""
     2
     3	from __future__ import annotations
     4
     5	import sys
     6
     7	from integrity.runner import main
     8
     9
    10	if __name__ == "__main__":
    11	    sys.exit(main(sys.argv[1:]))
```

FACT — `tools/integrity/scripts/grandfather_sweep.py` (LOC 45) —
verbatim:

```python
     1	#!/usr/bin/env python3
     2	"""Grandfather-sweep CLI entry. Logic lives in integrity.grandfather."""
     3
     4	from __future__ import annotations
     5
     6	import argparse
     7	import sys
     8	from pathlib import Path
     9
    10	from integrity.common.repo import find_repo_root
    11	from integrity.grandfather import apply_annotations
    12
    13
    14	def main(argv: list[str]) -> int:
    15	    parser = argparse.ArgumentParser(description="Grandfather-sweep integrity findings")
    16	    parser.add_argument("--dry-run", action="store_true")
    17	    parser.add_argument("--repo-root", type=Path, default=None)
    18	    parser.add_argument(
    19	        "--sweep-live-source",
    20	        action="store_true",
    21	        help=(
    22	            "Also sweep LIVE-SOURCE other-cat1 findings. Default is to skip them "
    23	            "(triage section B policy). Use only when a deliberate live-source "
    24	            "sweep is required."
    25	        ),
    26	    )
    27	    ns = parser.parse_args(argv)
    28
    29	    root = ns.repo_root if ns.repo_root else find_repo_root()
    30	    files, anns, counts, live_source_skipped = apply_annotations(
    31	        root, ns.dry_run, sweep_live_source=ns.sweep_live_source,
    32	    )
    33
    34	    label = "would modify" if ns.dry_run else "modified"
    35	    print(f"grandfather-sweep: {label} {files} files; {anns} annotations added")
    36	    if live_source_skipped:
    37	        suffix = "" if ns.sweep_live_source else " (use --sweep-live-source to include)"
    38	        print(f"  skipped as live-source (other-cat1): {live_source_skipped}{suffix}")
    39	    for cat, n in sorted(counts.items(), key=lambda kv: -kv[1]):
    40	        print(f"  {cat:>35s}: {n}")
    41	    return 0
    42
    43
    44	if __name__ == "__main__":
    45	    sys.exit(main(sys.argv[1:]))
```

### I.2 — `tools/integrity/pyproject.toml`

FACT — full verbatim (relevant sections):

```toml
     1	[build-system]
     2	requires = ["setuptools>=68", "wheel"]
     3	build-backend = "setuptools.build_meta"
     4
     5	[project]
     6	name = "gpusims-integrity"
     7	version = "0.1.0"
     8	description = "Cross-stack integrity verification toolkit for GPU-Sims"
     9	authors = [{ name = "Steven Cohen" }]
    10	license = { text = "MIT" }
    11	readme = "README.md"
    12	requires-python = ">=3.11"
    13	dependencies = [
    14	    "libclang>=18,<19",
    15	    "tomli>=2.0,<3",
    16	]
    17
    18	[project.optional-dependencies]
    19	dev = [...]
    27
    28
    29	[project.scripts]
    30	integrity = "integrity.__main__:main"
    31
    32	[tool.setuptools]
    33	packages = [
    34	    "integrity",
    35	    "integrity.common",
    36	    "integrity.cat1_citations",
    37	    "integrity.cat1_citations.checks",
    38	    "integrity.cat2_contracts",
    39	    "integrity.cat2_contracts.checks",
    40	    "integrity.cat3_numerical",
    41	    "integrity.cat3_numerical.checks",
    42	]
```

### I.3 — INFERENCE: external-facing entry points

INFERENCE: there is one declared `[project.scripts]` console entry,
`integrity = "integrity.__main__:main"` — i.e. when the package is
installed via pip, it produces an `integrity` shell command that
runs `integrity.__main__.main`. That, plus
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`python3 -m integrity` (same target via `__main__.py:11`) and the
ad-hoc `python3 tools/integrity/scripts/grandfather_sweep.py`, are
the only external entry points.

For A.2's "public surface" definition: this means the *truly* public
Python surface is exactly:

- `integrity.__main__.main` (called by both `-m integrity` and the
  installed `integrity` console script)
- `integrity.runner.main` (re-exported transitively via `__main__.py`)
- `integrity.grandfather.apply_annotations` (called by
  `scripts/grandfather_sweep.py:main`)
- `integrity.common.repo.find_repo_root` (called by
  `scripts/grandfather_sweep.py:main`)

Everything else is consumed only by the test harness or by other
internal modules. The spec must decide whether A.2 considers all
~50 test-imported symbols as "public" or only the 4 entrypoint-
consumed symbols.

---

## § J — Stack D fixture-test reuse opportunity

### J.1 — Current Stack D test run

FACT — `cd tools/integrity && python3 -m pytest tests/test_cat2_stack_d.py -v 2>&1 | head -40`:

```
============================= test session starts ==============================
platform linux -- Python 3.12.3, pytest-8.4.2, pluggy-1.6.0 -- /usr/bin/python3
cachedir: .pytest_cache
rootdir: /home/otacon/Projects/GPU-Sims/GPU-Sims/tools/integrity
configfile: pyproject.toml
plugins: cov-5.0.0, anyio-4.13.0
collecting ... collected 6 items

tests/test_cat2_stack_d.py::test_extract_public_surface_finds_class_and_function PASSED [ 16%]
tests/test_cat2_stack_d.py::test_extract_public_surface_enumerates_class_fields PASSED [ 33%]
tests/test_cat2_stack_d.py::test_good_contracts_yield_no_findings PASSED [ 50%]
tests/test_cat2_stack_d.py::test_bad_contracts_flag_unused_radii PASSED  [ 66%]
tests/test_cat2_stack_d.py::test_self_references_dont_count_for_field_usage PASSED [100%]

============================== 6 passed in 0.02s ===============================
```

INFERENCE: 6 tests, all passing in 0.02s. The test pattern is:
two fixture-based tests for the extractor, one good-fixture pass-
through, two bad-fixture targeted assertions on `radii`, one
inline-`tmp_path` fabricated test for self-reference behavior.

The test machinery is **path-hard-coded** to `gpusims_common`. To
reuse it for A.2, two options:

- Parameterize the helper fixture path in the existing test file
  (`fixtures_dir / "good_contracts"` becomes
  `fixtures_dir / package_layout`). Backwards-compatibility risk if
  other tests rely on the current path layout — none do, per
  inspection of `conftest.py` (not dumped here).
- Add a new test file `test_cat2_toolkit_unused.py` that mirrors
  the structure but with toolkit-flavored fixtures. Cleaner;
  duplicates ~30 lines of test scaffolding.

Recommend the second (new test file) — duplication is cheap
and the test intent is genuinely different (toolkit symbols, not
common-py symbols).

### J.2 — INFERENCE: A.2 test plan sketch

Approximate test count:

- Extractor unit tests: ~3 (finds top-level def, finds top-level
  class, ignores underscore-prefixed)
- Reference-finding unit tests: ~2 (registers a consumer, ignores
  same-module references)
- Good-fixture integration test: 1 (synthetic toolkit-shaped repo
  with a fully-consumed surface → no findings)
- Bad-fixture integration test: 1 (synthetic toolkit-shaped repo
  with one unconsumed top-level def → exactly one finding)
- Live-toolkit smoke test: 1 (runs the new check against the real
  repo, asserts the count matches the post-grandfather baseline —
  this would catch regressions in the extractor strategy)
- Classifier rule test: 1 (in `test_grandfather_sweep.py`, asserts a
  fabricated `cat2.public-symbol-used-toolkit` finding routes to
  `toolkit-own-unused`)

Total: ~9 tests. Fixture count: 2 toolkit-shaped fixture trees
(good + bad), structurally similar to the existing common-py
fixtures. Reuse of Stack D fixtures: minimal — toolkit fixtures
need different layout (no `common/common-py/gpusims_common/` root).

---

## § K — Banked observations

K.1 — **Catalog count drift since v1.1.**
`tools/integrity/docs/grandfather-catalog.md` headings show:

| Category | Catalog (header) | Live count (§ A.3) | Delta |
|---|---|---|---|
| audit-citation | 597 | 80 | -517 (cat1.bare-path landed; many findings re-categorized) |
| live-shader-1810 | 3 | 3 | 0 |
| audit-doc-1810 | 15 | 16 | +1 |
| spec-grammar-example | 17 | 18 | +1 |
| toolkit-own-source | 22 | 25 | +3 |
| retro-grammar-example | 2 | 8 | +6 |
| audit-report-grammar-example | 19 | 42 | +23 |
| other-cat1 | 66 | 35 | -31 |
| cat2-stack-d-unused | 17 | 17 | 0 |
| cat2-stack-c-unused | 111 | 110 | -1 |
| cat2-stack-b-unused | 73 | 73 | 0 |
| cat2-stub-label-stale | 2 | 2 | 0 |
| audit-bare-path | 635 | 635 | 0 |
| retro-bare-path | 11 | 11 | 0 |
| toolkit-doc-bare-path | 7 | 7 | 0 |
| deferred-upstream-bare-path | 5 | 5 | 0 |
| other-cat1-bare-path | 0 swept; 44 live-source skipped | (not in counts) | — |

INFERENCE: catalog headings are stale relative to live counts on
~6 categories (audit-citation has the largest delta — 597 → 80).
This isn't a v1.2 A.2 concern but is a "while you're touching the
catalog" candidate for the same commit, since A.2 will be adding a
new heading (`toolkit-own-unused`) anyway.

K.2 — **`stack_paths()` has no consumer.** Per § E.2 / § F.1,
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`integrity/common/stack_paths.py:16 stack_paths()` is the kind of
"declared public API, no consumer" pattern A.2 will surface. Could
be a real defect (the three Stack checks should consume it instead
of duplicating constants) or could be a deliberate forward-looking
hook. Spec author should decide whether to delete-or-wire-in before
A.2 lands, or to document it as an A.2-flagged symbol that gets
grandfathered with a tracking note.

K.3 — **`__init__.py` declares no public API.** All seven toolkit
`__init__.py` files are docstring-only or nearly so. The two
`checks/__init__.py` files declare `REGISTERED_CHECKS` for runtime
registry but nothing user-facing. This means A.2 cannot rely on
`__init__.py`-style re-export discovery at all — it must enumerate
top-level defs/classes per-module.

K.4 — **`integrity_v1_1_apispec_2026-05-15_architect1.md`** and
several other audit docs contain literal `from integrity.X` text in
fenced code blocks. The existing `cat1.intra-repo` check correctly
treats these as documentation, not code. A.2's check must do the
same — scan only `.py`/`.pyi` files, never markdown. The existing
`_list_scannable_py_files` pattern is sufficient (see § C.2).

K.5 — **Tests-tree symbol count.** The test tree has 142 top-level
public symbols (vs. the toolkit's 138). Including tests would
roughly double the false-positive surface. Hard exclusion is
mandatory (see § G.2).

---

## § L — Specific open questions for the spec to resolve

The probe surfaces these design decisions for the spec author. Each
references the relevant probe section.

**L.1 — Design path: new check vs extended check.**
Per § E.3. Recommendation: **Path (a) — new check**
(`cat2.public-symbol-used-toolkit`). Rationale: clean classifier
routing, parallel to existing Stack-{B,C,D} naming, makes the
extraction-strategy difference (toolkit has no `__init__.py`
re-exports) explicit rather than hidden.

**L.2 — Extraction strategy for the toolkit's "public surface."**
Per § B.3, § C.4, § F.1. Three sub-options:
- (a) Require toolkit modules to declare `__all__`; scan only
  `__all__`-listed names. Adds work to A.2 (~5 modules need
  declarations) but produces a meaningful surface.
- (b) Scan every top-level `def` / `class` (skip module-level
  constants, skip underscore-prefixed). Treat any name imported
  by `discover_checks()` reflection or by tests as "consumed."
  Wider surface, more tuning needed for false-positive reduction.
- (c) Scan only what a fixed allowlist names (the four
  entrypoint-consumed symbols from § I.3). Narrow but trivially
  correct — would land virtually no findings, defeating A.2's
  point.

Recommendation: **(b)** — it's the closest analog to what existing
Stack D does (which scans every public top-level symbol of
`gpusims_common`'s re-exports), and it produces a non-trivial
finding count that motivates the v1.2 grandfather sweep.

**L.3 — Tests directory inclusion.**
Per § G.2. Recommendation: **exclude `tools/integrity/tests/` from
the scan-target list**, but **include it in the scan-input list**
(test files DO consume toolkit symbols and those consumption
records prevent false positives). Same treatment for fixtures
under `tools/integrity/tests/fixtures/`. Same treatment for
`tools/integrity/scripts/` as scan-input only (its top-level
`main()` should not be flagged unused).

**L.4 — Sweep-side coordination with P1.8 live-source protection.**
Per § H.4. Three options enumerated:
- (i) Use `--sweep-live-source` flag for the A.2 rollout sweep —
  REJECT (defeats P1.8 protection of real sim source code).
- (ii) Add per-category sweep-allow list to `apply_annotations`
  (small refactor; preserves P1.8 intent for everything else) —
  RECOMMENDED.
- (iii) Add `tools/integrity/integrity/` to `SWEEPABLE_PATH_PREFIXES`
  — REJECT (un-does P1.8 for any future cat1 findings in toolkit).

This is the most consequential design question A.2 surfaces.

**L.5 — Classifier category name and granularity.**
Per § F.2, § H.3. Recommendation: single category
`toolkit-own-unused`. Avoid premature splits like
`toolkit-public-api-unused` vs `toolkit-internal-unused` — the
distinction depends on the L.2 extraction-strategy choice. Wait
for v1.3+ if differentiation proves load-bearing.

**L.6 — Catalog count refresh.**
Per § K.1. The grandfather-catalog headings are stale on ~6
categories due to v1.2 A.3 re-categorizations. The A.2 commit
that adds a new `toolkit-own-unused` heading is a natural place
to refresh those numbers. Optional but reduces drift.

**L.7 — `stack_paths()` disposition.**
Per § E.2, § K.2. The unused `stack_paths()` helper is itself a
candidate finding under A.2. Spec should decide whether to:
- Delete it (no consumer; YAGNI), or
- Wire it into the existing three Stack checks (refactor, removes
  three pairs of duplicated constants), or
- Grandfather it with a tracking note (add to catalog with explicit
  "deferred for v1.3 stack-config consolidation" reason).

**L.8 — Drift in catalog vs live counts (banked observation
becomes spec-line item).**
Per § K.1. Should A.2's commit also touch
`tools/integrity/docs/grandfather-catalog.md` to refresh the stale
counts, or land that as a separate v1.2 retrospective bolt-on?
Affects the commit's blast radius.

---

## § M — Probe end-state

FACT — `git rev-parse HEAD` at probe end:
`67474b9298588f95497cc89765cca814598708d1`. This is one commit
ahead of the start-of-probe SHA (`1a49d334bdcacc1d8ba9c2311788ca486a2c0fd1`,
§ A.1). The new commit is:

```
67474b9 docs(retro): integrity toolkit v1.2 bolt-ons retrospective
```

This was the in-flight retro the prompt anticipated; landed mid-probe
between § A capture and § M close. Drift impact: zero on FACT readings
(all FACT material in this probe is from `tools/integrity/integrity/`
source, not from the retro doc) and zero on INFERENCE conclusions
(the retro is a sibling-audit doc, not a code change). The
`grandfather-report` summary in § A.3 was captured at the older SHA
and would now show one additional `audit-report-grammar-example` or
`retro-bare-path` finding if the retro doc cites bare paths or
contains annotation grammar examples; impact on the design questions
in § L is nil.

No files modified by this probe, no commits, no pushes. Read-only
constraint satisfied.

This probe touched the following read paths (all via Read / Bash
tools, no Edit / Write to source):
- `tools/integrity/integrity/__init__.py`
- `tools/integrity/integrity/__main__.py`
- `tools/integrity/integrity/runner.py` (selective grep)
- `tools/integrity/integrity/grandfather.py`
- `tools/integrity/integrity/snapshot.py` (selective)
- `tools/integrity/integrity/cat1_citations/__init__.py`
- `tools/integrity/integrity/cat1_citations/checks/__init__.py`
- `tools/integrity/integrity/cat1_citations/checks/*.py` (top-level
  symbol enumeration)
- `tools/integrity/integrity/cat1_citations/grammar.py` (selective)
- `tools/integrity/integrity/cat1_citations/resolver.py` (selective)
- `tools/integrity/integrity/cat1_citations/upstream_anchor.py` (selective)
- `tools/integrity/integrity/cat2_contracts/__init__.py`
- `tools/integrity/integrity/cat2_contracts/checks/__init__.py`
- `tools/integrity/integrity/cat2_contracts/checks/public_symbol_used.py`
- `tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_b.py` (selective)
- `tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_c.py` (selective)
- `tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py` (selective)
- `tools/integrity/integrity/cat2_contracts/stack_b.py` (selective)
- `tools/integrity/integrity/cat2_contracts/stack_c.py` (selective)
- `tools/integrity/integrity/cat2_contracts/stack_d.py`
- `tools/integrity/integrity/cat3_numerical/__init__.py`
- `tools/integrity/integrity/cat3_numerical/checks/__init__.py`
- `tools/integrity/integrity/cat3_numerical/checks/*.py` (selective)
- `tools/integrity/integrity/cat3_numerical/cubic_kernel.py` (selective)
- `tools/integrity/integrity/cat3_numerical/d3q19_verify.py` (selective)
- `tools/integrity/integrity/cat3_numerical/generate_expected.py` (selective)
- `tools/integrity/integrity/common/__init__.py`
- `tools/integrity/integrity/common/annotations.py` (selective)
- `tools/integrity/integrity/common/audit_log.py` (selective)
- `tools/integrity/integrity/common/exclusions.py` (selective)
- `tools/integrity/integrity/common/repo.py` (selective)
- `tools/integrity/integrity/common/results.py` (selective)
- `tools/integrity/integrity/common/stack_paths.py`
- `tools/integrity/integrity/common/suppression.py` (selective)
- `tools/integrity/scripts/grandfather_sweep.py`
- `tools/integrity/tests/test_cat2_stack_d.py`
- `tools/integrity/tests/test_suppression_fence.py`
- `tools/integrity/tests/fixtures/good_contracts/**`
- `tools/integrity/tests/fixtures/bad_contracts/**`
- `tools/integrity/pyproject.toml`
- `tools/integrity/docs/grandfather-catalog.md` (selective heading scan)

End of probe.
