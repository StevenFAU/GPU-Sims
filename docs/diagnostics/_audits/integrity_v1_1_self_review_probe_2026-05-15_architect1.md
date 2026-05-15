---
title: "Integrity v1.1 Self-Review Probe"
date: 2026-05-15
author: architect1-via-claude-code
status: probe
scope: read-only
sibling-docs:
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/diagnostics/_audits/integrity_v1_1_post_batch_triage_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_1_apispec_2026-05-15_architect1.md
---

# Integrity v1.1 Self-Review Probe

Read-only probe grounding a self-review pass on the integrity toolkit v1.1
batch-1 retro (`docs/retro/integrity-toolkit-v1.1-batch1.md`) and informing
batch-2 scope. Every finding is tagged `FACT` (directly observed) or
`INFERENCE` (derived). Verbatim source listings are wrapped in fenced blocks
labelled with their repo-relative path.

Two operating notes set at probe-execution time:

- **FACT:** Probe executed against working-tree HEAD = `cdad2e2`. The
  conversation-start git status snapshot recorded HEAD = `c1a257d`; a
  follow-up Phase 12 fix landed during probe execution. Section A reflects
  the actual HEAD state at probe time.
- **FACT:** The post-retro landing audit
  (`docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md`)
  recorded HEAD = `d772803`. Two commits have landed since:
  `c1a257d` (Phase 12 streamline reseed fix) and `cdad2e2` (Phase 12
  streamline seed-slab + dt_render fix). Section A.5 classifies both.

## Section A — Current gate state and outstanding work

### A.1 — Strict-mode summary (FACT)

Command:

```
python3 -m integrity --mode strict --no-audit-log 2>&1 | head -10
```

Output:

```
integrity: 2 pass, 0 soft-warn, 4 hard-fail, 1007 suppressed
  HARD_FAIL: cat1.intra-repo at CHANGELOG.md:92
    Chakazul/Lenia/Python/LeniaNDK.py:329-335: path 'Chakazul/Lenia/Python/LeniaNDK.py' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at CHANGELOG.md:154
    context.hpp:78: path 'context.hpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at CHANGELOG.md:154
    context.cpp:116: path 'context.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at CHANGELOG.md:154
    context.cpp:202: path 'context.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at common/common-py/examples/hello/hello/main.py:31
```

**INFERENCE — output-rendering anomaly.** The summary line says "4 hard-fail,
1007 suppressed", which matches the post-retro audit's expected state. But the
HARD_FAIL stanzas printed below the summary include findings that are in fact
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
*suppressed* (e.g. `CHANGELOG.md:92` carries an `integrity-allow:` annotation
per the grandfather sweep; `common/common-py/examples/hello/hello/main.py:31`
is in `cat2-stack-d-unused`/`other-cat1` etc.). The strict-mode human-readable
renderer in `runner.py:_emit_human_summary` appears to iterate all findings
and emit each as a HARD_FAIL stanza regardless of `f.suppressed` (see
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`runner.py:141-145` — the `else` branch has no suppression filter, in
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
contrast with the `github` branch at `runner.py:133-139` which checks
`if f.suppressed: continue`).

This is the first surface scope issue not covered in the retro: the
human-renderer + strict-mode combination produces output where the
stanza list and the summary line are mutually inconsistent. Worth banking
for batch 2.

### A.2 — Unsuppressed hard-fails verbatim (FACT)

Command:

```
python3 -m integrity --mode strict --no-audit-log --output json 2>/dev/null | \
  python3 -c "import json, sys; d=json.load(sys.stdin); \
    [print(f['check_id'], '|', f['file'], ':', f['line'], '|', f.get('message','')) \
     for f in d['findings'] if not f.get('suppressed')]"
```

Output:

```
cat1.intra-repo | docs/phase12_lattice_boltzmann.md : 203 | chapter13/cpu/LBM.cpp:97: path 'chapter13/cpu/LBM.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs or /home/otacon/Projects/GPU-Sims/GPU-Sims
cat1.intra-repo | docs/phase12_lattice_boltzmann.md : 351 | chapter13/cpu/LBM.cpp:97: path 'chapter13/cpu/LBM.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs or /home/otacon/Projects/GPU-Sims/GPU-Sims
cat1.intra-repo | docs/phase12_lattice_boltzmann.md : 1276 | main.cpp:1168-1279: path 'main.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/docs or /home/otacon/Projects/GPU-Sims/GPU-Sims
cat1.intra-repo | particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl : 7 | SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75: path 'SPlisHSPlasH/BoundaryModel_Akinci2012.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/particle-fluids/sph-water/shaders or /home/otacon/Projects/GPU-Sims/GPU-Sims
```

### A.3 — Attribution of hard-fails (FACT)

Per `git blame -L <line>,<line> <file>`:

| Finding | Introducing commit | Subject | Author | Date |
|---|---|---|---|---|
| `docs/phase12_lattice_boltzmann.md:203` | `c5955d3` | `setup(phase12): land architect-1 spec at docs/phase12_lattice_boltzmann.md` | Steven Cohen | 2026-05-15 12:07:52 -0400 |
| `docs/phase12_lattice_boltzmann.md:351` | `c5955d3` | (same) | Steven Cohen | (same) |
| `docs/phase12_lattice_boltzmann.md:1276` | `c5955d3` | (same) | Steven Cohen | (same) |
| `particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl:7` | `f9f2cb9` | `feat(sph-water): Akinci2012 boundary handling (commit 3)` | Steven Cohen | 2026-05-15 11:13:30 -0400 |

**INFERENCE:** All 4 outstanding hard-fails were introduced before the
post-retro audit's recorded HEAD (`d772803`). Neither of the two commits
landed since `d772803` (`c1a257d`, `cdad2e2`) introduced new unsuppressed
findings. § A.5 below confirms.

### A.4 — Comparison to post-retro expected state (FACT)

Post-retro audit § D.2.1 expected exactly 4 hard-fails: 3 Phase 12 LBM
citations + 1 sph-water Akinci2012 boundary citation. Probe time:

- 3 × `docs/phase12_lattice_boltzmann.md` (lines 203, 351, 1276) — **MATCH**
- 1 × `particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl:7`
  — **MATCH**

**FACT:** Zero drift. The gate is in the exact state the post-retro audit
predicted.

### A.5 — Commits since post-retro audit's recorded HEAD (FACT)

Command: `git log --oneline d772803..HEAD`

```
cdad2e2 fix(lattice-boltzmann): streamline seed-slab + dt_render units (in-flight #2)
c1a257d fix(lattice-boltzmann): streamline reseed visual defects (in-flight Phase 12)
```

Files touched:

```
# c1a257d
docs/diagnostics/_audits/phase12_inflight_streamline_reseed_2026-05-15.md
volumetric-grid/lattice-boltzmann/shaders/streamline_advect.comp.glsl
volumetric-grid/lattice-boltzmann/src/main.cpp

# cdad2e2
docs/diagnostics/_audits/phase12_inflight_streamline_seed_and_dt_2026-05-15.md
volumetric-grid/lattice-boltzmann/src/main.cpp
```

Classification:

- `c1a257d` — **TOUCHED-CAT1-SCANNABLE**: edits a `.glsl` shader, a `.cpp`
  source, and an audit-doc under `docs/diagnostics/_audits/`. All three are
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  cat1-scannable per `intra_repo.py:34` SCAN_EXTENSIONS and exclusion rules.
- `cdad2e2` — **TOUCHED-CAT1-SCANNABLE**: edits a `.cpp` source and an
  audit-doc.

**INFERENCE — companion-pair status.** Neither commit shipped a
grandfather-sweep companion commit. Per § A.4, neither introduced new
unsuppressed findings — so the convention proposed in retro § 6.2 would not
have been triggered for either commit even at the strictest enforcement
level (the live-source class did not change). But the convention as drafted
("every commit that touches the cat1-scannable surface") would flag both
under the **medium** or **hard** enforcement options. This is data for the
enforcement-level question in § 6.2: the convention's literal text would
over-trigger on commits that don't introduce findings. See § I below.

## Section B — Toolkit code surface inventory

### B.1 — File tree (FACT)

```
tools/integrity/.grandfather-history.json
tools/integrity/.pytest_cache/.gitignore
tools/integrity/.pytest_cache/CACHEDIR.TAG
tools/integrity/.pytest_cache/README.md
tools/integrity/.pytest_cache/v/cache/lastfailed
tools/integrity/.pytest_cache/v/cache/nodeids
tools/integrity/README.md
tools/integrity/docs/algebraic/d3q19.md
tools/integrity/docs/grandfather-catalog.md
tools/integrity/docs/ground-truth-sources.md
tools/integrity/drivers/integrity_cat3_stack_c/CMakeLists.txt
tools/integrity/drivers/integrity_cat3_stack_c/main.cpp
tools/integrity/gpusims_integrity.egg-info/PKG-INFO
tools/integrity/gpusims_integrity.egg-info/SOURCES.txt
tools/integrity/gpusims_integrity.egg-info/dependency_links.txt
tools/integrity/gpusims_integrity.egg-info/entry_points.txt
tools/integrity/gpusims_integrity.egg-info/requires.txt
tools/integrity/gpusims_integrity.egg-info/top_level.txt
tools/integrity/integrity/__init__.py
tools/integrity/integrity/__main__.py
tools/integrity/integrity/cat1_citations/__init__.py
tools/integrity/integrity/cat1_citations/checks/__init__.py
tools/integrity/integrity/cat1_citations/checks/annotation.py
tools/integrity/integrity/cat1_citations/checks/intra_repo.py
tools/integrity/integrity/cat1_citations/checks/unregistered_upstream.py
tools/integrity/integrity/cat1_citations/checks/upstream.py
tools/integrity/integrity/cat1_citations/checks/upstream_anchor.py
tools/integrity/integrity/cat1_citations/grammar.py
tools/integrity/integrity/cat1_citations/resolver.py
tools/integrity/integrity/cat1_citations/upstream_anchor.py
tools/integrity/integrity/cat2_contracts/__init__.py
tools/integrity/integrity/cat2_contracts/checks/__init__.py
tools/integrity/integrity/cat2_contracts/checks/public_symbol_used.py
tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_b.py
tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_c.py
tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py
tools/integrity/integrity/cat2_contracts/stack_b.py
tools/integrity/integrity/cat2_contracts/stack_c.py
tools/integrity/integrity/cat2_contracts/stack_d.py
tools/integrity/integrity/cat2_contracts/ts_helper/extract_and_find.ts
tools/integrity/integrity/cat2_contracts/ts_helper/package-lock.json
tools/integrity/integrity/cat2_contracts/ts_helper/package.json
tools/integrity/integrity/cat2_contracts/ts_helper/tsconfig.json
tools/integrity/integrity/cat3_numerical/__init__.py
tools/integrity/integrity/cat3_numerical/checks/__init__.py
tools/integrity/integrity/cat3_numerical/checks/cubic_kernel.py
tools/integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.expected.json
tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py
tools/integrity/integrity/cat3_numerical/cubic_kernel.py
tools/integrity/integrity/cat3_numerical/expected_values.toml
tools/integrity/integrity/cat3_numerical/generate_expected.py
tools/integrity/integrity/common/__init__.py
tools/integrity/integrity/common/annotations.py
tools/integrity/integrity/common/audit_log.py
tools/integrity/integrity/common/exclusions.py
tools/integrity/integrity/common/repo.py
tools/integrity/integrity/common/results.py
tools/integrity/integrity/common/stack_paths.py
tools/integrity/integrity/common/suppression.py
tools/integrity/integrity/grandfather.py
tools/integrity/integrity/runner.py
tools/integrity/integrity/snapshot.py
tools/integrity/pyproject.toml
tools/integrity/scripts/__init__.py
tools/integrity/scripts/grandfather_sweep.py
tools/integrity/tests/conftest.py
tools/integrity/tests/fixtures/...      (truncated for noise; full list runs to ~50 fixture files)
tools/integrity/tests/test_cat1_annotation.py
tools/integrity/tests/test_cat1_annotation_fence.py
tools/integrity/tests/test_cat1_intra_repo.py
tools/integrity/tests/test_cat1_intra_repo_fence.py
tools/integrity/tests/test_cat1_unregistered.py
tools/integrity/tests/test_cat1_upstream.py
tools/integrity/tests/test_cat1_upstream_anchor.py
tools/integrity/tests/test_cat1_upstream_fence.py
tools/integrity/tests/test_cat2_stack_b.py
tools/integrity/tests/test_cat2_stack_c.py
tools/integrity/tests/test_cat2_stack_d.py
tools/integrity/tests/test_cat2_stub_label_stale.py
tools/integrity/tests/test_cat3_cubic_kernel.py
tools/integrity/tests/test_grandfather_sweep.py
tools/integrity/tests/test_runner.py
tools/integrity/tests/test_snapshot.py
tools/integrity/tests/test_suppression_fence.py
```

### B.2 — LOC per Python module (FACT, sorted desc)

```
531 tools/integrity/integrity/cat2_contracts/stack_c.py
330 tools/integrity/integrity/grandfather.py
281 tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py
272 tools/integrity/integrity/cat2_contracts/stack_d.py
200 tools/integrity/integrity/snapshot.py
198 tools/integrity/integrity/runner.py
195 tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py
177 tools/integrity/integrity/cat1_citations/grammar.py
139 tools/integrity/integrity/cat1_citations/checks/upstream.py
135 tools/integrity/integrity/cat1_citations/checks/annotation.py
132 tools/integrity/integrity/cat2_contracts/stack_b.py
124 tools/integrity/integrity/cat1_citations/checks/intra_repo.py
118 tools/integrity/integrity/common/annotations.py
118 tools/integrity/integrity/cat3_numerical/generate_expected.py
111 tools/integrity/integrity/cat3_numerical/cubic_kernel.py
110 tools/integrity/integrity/cat1_citations/checks/unregistered_upstream.py
103 tools/integrity/integrity/cat1_citations/resolver.py
 99 tools/integrity/integrity/cat3_numerical/checks/cubic_kernel.py
 99 tools/integrity/integrity/cat2_contracts/checks/public_symbol_used.py
 93 tools/integrity/integrity/cat1_citations/upstream_anchor.py
 88 tools/integrity/integrity/common/suppression.py
 78 tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_c.py
 67 tools/integrity/integrity/common/audit_log.py
 63 tools/integrity/integrity/cat1_citations/checks/upstream_anchor.py
 51 tools/integrity/integrity/common/results.py
 45 tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_b.py
 43 tools/integrity/integrity/common/exclusions.py
 41 tools/integrity/integrity/common/repo.py
 34 tools/integrity/integrity/common/stack_paths.py
 17 tools/integrity/integrity/cat1_citations/checks/__init__.py
 15 tools/integrity/integrity/cat2_contracts/checks/__init__.py
 11 tools/integrity/integrity/__main__.py
  7 tools/integrity/integrity/cat3_numerical/checks/__init__.py
  3 tools/integrity/integrity/__init__.py
  1 tools/integrity/integrity/cat3_numerical/__init__.py
  1 tools/integrity/integrity/cat2_contracts/__init__.py
  1 tools/integrity/integrity/cat1_citations/__init__.py
  0 tools/integrity/integrity/common/__init__.py
```

Total: **38 Python files, 4131 LOC** under `tools/integrity/integrity/`.

### B.3 — Registered check IDs (FACT)

```
tools/integrity/integrity/cat2_contracts/checks/public_symbol_used.py:38:CHECK_ID = "cat2.public-symbol-used"
tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py:37:CHECK_ID = "cat2.stub-label-stale"
tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_c.py:35:CHECK_ID = "cat2.public-symbol-used-c"
tools/integrity/integrity/cat2_contracts/checks/public_symbol_used_b.py:20:CHECK_ID = "cat2.public-symbol-used-ts"
tools/integrity/integrity/cat1_citations/checks/annotation.py:27:CHECK_ID = "cat1.annotation-form"
tools/integrity/integrity/cat1_citations/checks/upstream.py:29:CHECK_ID = "cat1.upstream-citation"
tools/integrity/integrity/cat1_citations/checks/intra_repo.py:29:CHECK_ID = "cat1.intra-repo"
tools/integrity/integrity/cat1_citations/checks/upstream_anchor.py:19:CHECK_ID = "cat1.upstream-anchor"
tools/integrity/integrity/cat1_citations/checks/unregistered_upstream.py:29:CHECK_ID = "cat1.unregistered-upstream"
tools/integrity/integrity/cat3_numerical/checks/cubic_kernel.py:30:CHECK_ID = "cat3.cubic-kernel"
```

10 registered checks (5 cat1, 4 cat2, 1 cat3).

### B.4 — Delta vs v1.1 apispec probe (INFERENCE)

The apispec probe (`integrity_v1_1_apispec_2026-05-15_architect1.md`, § A.2)
reported 35 Python files / 3299 LOC. Current state: **38 Python files / 4131
LOC**.

Delta: **+3 files, +832 LOC**.

The 3 new Python files (per `git log --oneline -- <path>` on each new file
not present in the apispec list):

- `tools/integrity/integrity/snapshot.py` — added in commit `dbac051`
  (A.7 snapshot module + seed history, v1.1 batch 1 commit 3a)
- `tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py` —
  added in commit `af248cf` (A.1 cat2.stub-label-stale, v1.1 batch 1 commit 1)
- `tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py` —
  added (281 LOC) as part of Phase 12 setup, **not** v1.1 batch 1. Surfaced
  here for completeness; the retro is silent on this file because the
  retro scoped to v1.1 batch-1 items, not Phase 12 setup work. Worth
  banking: a sizeable Cat 3 module landed concurrently with batch 1 and
  isn't represented in the retro inventory.

### B.5 — `tools/integrity/integrity/snapshot.py` (FACT, verbatim, 201 lines)

```python
# tools/integrity/integrity/snapshot.py
"""State-snapshot and grandfather-report emitters (v1.1 A.7, A.8).

Two entry points:

- `emit_state_snapshot(root, stdout)` -- A self-contained JSON document
  describing the toolkit's complete state at a single moment: commit SHA,
  timestamp, registered checks, registered upstream sources, full
  per-category suppression counts. Intended as the "verification
  provenance" anchor for spec drafts (v1.1 spec section D.1).
- `emit_grandfather_report(root, stdout, append_history=True)` -- Human-
  readable per-category table to stdout; optionally appends a JSON entry
  to `tools/integrity/.grandfather-history.json` (a time series).
"""

from __future__ import annotations

import datetime as dt
import json
import subprocess
from pathlib import Path
from typing import IO


HISTORY_FILE_RELATIVE = Path("tools/integrity/.grandfather-history.json")


_KNOWN_CATEGORIES = (
    "audit-citation",
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
    "other-cat1",
)


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


def _collect_state(root: Path) -> dict:
    """Run the toolkit in JSON warn-only mode and aggregate state."""
    result = subprocess.run(
        ["python3", "-m", "integrity",
         "--output", "json", "--no-audit-log", "--mode", "warn-only"],
        cwd=root,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode not in (0, 1):
        raise RuntimeError(
            f"integrity toolkit exited {result.returncode}: {result.stderr}"
        )
    data = json.loads(result.stdout)

    per_category: dict[str, int] = {}
    for f in data.get("findings", []):
        if not f.get("suppressed"):
            continue
        reason = f.get("suppression_reason", "")
        category = _extract_category(reason)
        per_category[category] = per_category.get(category, 0) + 1

    return {
        "schema_version": 1,
        "timestamp": dt.datetime.now(dt.timezone.utc).isoformat(),
        "commit": data.get("commit", "unknown"),
        "summary": data.get("summary", {}),
        "per_category": per_category,
    }


def _parse_ground_truth_sources(root: Path) -> list[dict]:
    """Parse `tools/integrity/docs/ground-truth-sources.md` for upstream
    anchor blocks. Permissive line-based parser; tolerates either inline
    or fenced TOML-shaped entries."""
    path = root / "tools" / "integrity" / "docs" / "ground-truth-sources.md"
    if not path.is_file():
        return []

    text = path.read_text(encoding="utf-8")
    blocks: list[dict] = []
    current: dict = {}
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("anchor_version"):
            _, _, val = stripped.partition("=")
            current["anchor_version"] = val.strip().strip('"').strip("'")
        elif stripped.startswith("anchor_sha"):
            _, _, val = stripped.partition("=")
            current["anchor_sha"] = val.strip().strip('"').strip("'")
        elif stripped.startswith("vendor_root"):
            _, _, val = stripped.partition("=")
            current["vendor_root"] = val.strip().strip('"').strip("'")
        elif stripped.startswith("name"):
            _, _, val = stripped.partition("=")
            current["name"] = val.strip().strip('"').strip("'")
        elif stripped == "":
            if current.get("anchor_version") and current.get("anchor_sha"):
                blocks.append(current)
            current = {}

    if current.get("anchor_version") and current.get("anchor_sha"):
        blocks.append(current)

    return blocks


def emit_state_snapshot(root: Path, out: IO[str]) -> None:
    """Emit a complete toolkit-state JSON document to `out`."""
    state = _collect_state(root)

    from integrity.cat1_citations.checks import REGISTERED_CHECKS as cat1
    from integrity.cat2_contracts.checks import REGISTERED_CHECKS as cat2
    from integrity.cat3_numerical.checks import REGISTERED_CHECKS as cat3
    state["registered_checks"] = {
        "cat1": [cid for cid, _ in cat1],
        "cat2": [cid for cid, _ in cat2],
        "cat3": [cid for cid, _ in cat3],
    }

    state["registered_upstreams"] = _parse_ground_truth_sources(root)

    json.dump(state, out, indent=2)
    out.write("\n")


def _append_history(root: Path, state: dict) -> None:
    """Append `state` (truncated to history-relevant fields) to the
    history file."""
    history_path = root / HISTORY_FILE_RELATIVE
    history_path.parent.mkdir(parents=True, exist_ok=True)

    if history_path.is_file():
        try:
            history = json.loads(history_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            history = []
    else:
        history = []

    history.append({
        "timestamp": state["timestamp"],
        "commit": state["commit"],
        "summary": state["summary"],
        "per_category": state["per_category"],
    })

    history_path.write_text(
        json.dumps(history, indent=2) + "\n",
        encoding="utf-8",
    )


def emit_grandfather_report(
    root: Path,
    out: IO[str],
    append_history: bool = True,
) -> None:
    """Emit a human-readable per-category table and optionally append
    to the history file."""
    state = _collect_state(root)

    out.write(f"grandfather report @ {state['commit']} ({state['timestamp']})\n")
    out.write(f"summary: {state['summary']}\n")
    out.write("per-category counts:\n")
    for cat, n in sorted(state["per_category"].items(), key=lambda kv: -kv[1]):
        out.write(f"  {cat:>35s}: {n}\n")

    if append_history:
        _append_history(root, state)
```

### B.6 — `tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py` (FACT, verbatim, 196 lines)

```python
# tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py
"""Check: cat2.stub-label-stale -- flag stale "Phase N stub" labels.

Mode: HARD_FAIL.

Closes spec section 12 row 5 (alembic_writer.hpp canonical case).
Anchors on the exact phrasing `In Phase <N>, this is a stub:` present
in both confirmed stale cases per probe v1_1_apispec section G. If the
corresponding implementation file has more than 10 non-comment LOC,
the "stub" label contradicts the implementation and is flagged.

Detection scope (batch-1-spec Decision 3):
  - C++ headers under common/common-cpp/include/**/*.{hpp,h}
  - Python modules under common/common-py/gpusims_common/**/*.py

Sibling-impl resolution (batch-1-spec Decision 2):
  - `.hpp`/`.h` in `common-cpp/include/<sub>/<base>.hpp` ->
    `common-cpp/src/<sub>/<base>.cpp` (relative path mirror)
  - `.py`: impl is the same file

False-positive guard for Stack D:
  Skip Stack D files whose top 40 lines contain `permanent stub` or
  `real-or-stub` -- both intentional discriminator phrasings per
  probe section D.2. Anchored on top-of-file because these phrasings
  appear in module docstrings.
"""

from __future__ import annotations

import re
from pathlib import Path

from integrity.common.exclusions import is_excluded
from integrity.common.repo import list_tracked_files
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat2.stub-label-stale"
MODE = FailureMode.HARD_FAIL


# Exact phrasing observed in both confirmed stale cases per probe section G.
STALE_LABEL_RE = re.compile(r"\bIn Phase \d+, this is a stub:")

# Top-of-file discriminator phrasings that override the staleness signal.
DISCRIMINATOR_PHRASES = ("permanent stub", "real-or-stub")
DISCRIMINATOR_SCAN_LINES = 40

# Implementation must have more than this many non-comment LOC to count
# as a real (non-stub) impl. Mirrors probe section D.2 heuristic.
IMPL_LOC_THRESHOLD = 10

# Lines counted as "comment-or-blank" for the impl-LOC heuristic.
_COMMENT_OR_BLANK_RE = re.compile(r"^\s*(//|#|/\*|\*|$)")


CPP_INCLUDE_ROOT = Path("common/common-cpp/include")
CPP_SRC_ROOT = Path("common/common-cpp/src")
PY_PACKAGE_ROOT = Path("common/common-py/gpusims_common")


def _list_scannable_files(repo_root: Path) -> list[Path]:
    """Return scannable files under the C++ header tree + Stack D package."""
    if (repo_root / ".git").exists():
        all_files = list_tracked_files(repo_root)
    else:
        all_files = [p for p in repo_root.rglob("*") if p.is_file()]

    out: list[Path] = []
    for absolute in all_files:
        try:
            rel = absolute.relative_to(repo_root)
        except ValueError:
            continue
        if is_excluded(str(rel)):
            continue

        ext = absolute.suffix
        rel_str = str(rel).replace("\\", "/")

        in_cpp_include = (
            rel_str.startswith(str(CPP_INCLUDE_ROOT) + "/")
            and ext in (".hpp", ".h")
        )
        in_py_package = (
            rel_str.startswith(str(PY_PACKAGE_ROOT) + "/")
            and ext == ".py"
        )

        if in_cpp_include or in_py_package:
            out.append(absolute)

    return out


def _resolve_impl_path(header_path: Path, repo_root: Path) -> Path | None:
    """Resolve the impl file for a given header/module.

    Convention (verified against synced common-cpp layout 2026-05-15):
      include/<namespace>/<rest>.hpp  ->  src/<rest>.cpp
    The first directory component after include/ is the project
    namespace (e.g. `gpusims/`) and is stripped -- the src/ tree does
    not repeat the namespace path.

    For Python files, impl is the same file (Python does not separate
    declaration from implementation)."""
    try:
        rel = header_path.relative_to(repo_root)
    except ValueError:
        return None

    rel_str = str(rel).replace("\\", "/")

    if (
        rel_str.startswith(str(CPP_INCLUDE_ROOT) + "/")
        and header_path.suffix in (".hpp", ".h")
    ):
        relative_to_include = header_path.relative_to(repo_root / CPP_INCLUDE_ROOT)
        parts = relative_to_include.parts
        if len(parts) < 2:
            return None
        namespace_stripped = Path(*parts[1:])
        impl_relative = namespace_stripped.with_suffix(".cpp")
        return repo_root / CPP_SRC_ROOT / impl_relative

    if rel_str.startswith(str(PY_PACKAGE_ROOT) + "/") and header_path.suffix == ".py":
        return header_path

    return None


def _count_non_comment_loc(text: str) -> int:
    """Count lines that are NOT pure comments or blank."""
    count = 0
    for line in text.splitlines():
        if _COMMENT_OR_BLANK_RE.match(line):
            continue
        count += 1
    return count


def _has_discriminator(text: str) -> bool:
    """Check if top-of-file text contains an intentional-stub discriminator."""
    head = "\n".join(text.splitlines()[:DISCRIMINATOR_SCAN_LINES]).lower()
    return any(phrase in head for phrase in DISCRIMINATOR_PHRASES)


def run(repo_root: Path) -> list[Finding]:
    findings: list[Finding] = []

    for header in _list_scannable_files(repo_root):
        try:
            text = header.read_text(encoding="utf-8")
        except OSError:
            continue

        if header.suffix == ".py" and _has_discriminator(text):
            continue

        for lineno, line in enumerate(text.splitlines(), start=1):
            if not STALE_LABEL_RE.search(line):
                continue

            impl_path = _resolve_impl_path(header, repo_root)
            if impl_path is None or not impl_path.is_file():
                continue

            try:
                impl_text = impl_path.read_text(encoding="utf-8")
            except OSError:
                continue

            impl_loc = _count_non_comment_loc(impl_text)
            if impl_loc <= IMPL_LOC_THRESHOLD:
                continue

            try:
                header_rel = str(header.relative_to(repo_root))
                impl_rel = str(impl_path.relative_to(repo_root))
            except ValueError:
                header_rel = str(header)
                impl_rel = str(impl_path)

            findings.append(Finding(
                check_id=CHECK_ID,
                mode=MODE,
                file=header_rel,
                line=lineno,
                message=(
                    f"\"In Phase N stub\" label is stale: implementation "
                    f"at {impl_rel} has {impl_loc} non-comment LOC "
                    f"(threshold {IMPL_LOC_THRESHOLD})"
                ),
            ))

    return findings
```

### B.7 — `tools/integrity/integrity/runner.py` (FACT, verbatim, 199 lines)

```python
# tools/integrity/integrity/runner.py
"""Top-level runner: parse CLI, discover checks, dispatch, summarize.

Commit 1 ships a stub runner. The check-discovery and dispatch logic
is structured but returns an empty findings list, since no checks are
registered yet. Commits 2+ will register check modules.

See docs/integrity-toolkit-spec.md § 5 for the full CLI surface.
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from integrity.common.exclusions import CANONICAL_EXCLUSIONS  # noqa: F401  (used in later commits)
from integrity.common.repo import find_repo_root, git_head_sha
from integrity.common.results import FailureMode, Finding, RunSummary


# Exit codes per spec § 5.2
EXIT_OK = 0
EXIT_HARD_FAIL = 1
EXIT_INTERNAL_FAIL = 2
EXIT_BAD_CLI = 64


@dataclass
class CliArgs:
    cat: int | None
    check: str | None
    mode: str
    root: Path
    output: str
    no_audit_log: bool
    grandfather_report: bool
    no_history_append: bool
    state_snapshot: bool


def parse_args(argv: list[str]) -> CliArgs:
    parser = argparse.ArgumentParser(
        prog="integrity",
        description="GPU-Sims integrity toolkit — cross-stack verification",
    )
    parser.add_argument("--cat", type=int, choices=[1, 2, 3], default=None,
                        help="Run only the named category")
    parser.add_argument("--check", type=str, default=None,
                        help="Run only the named check, e.g. cat1.upstream-anchor")
    parser.add_argument("--mode", choices=["strict", "warn-only"], default="strict",
                        help="strict honors HARD_FAIL/SOFT_WARN; warn-only converts all to SOFT_WARN")
    parser.add_argument("--root", type=Path, default=None,
                        help="Override repo root (default: auto-detect via git)")
    parser.add_argument("--output", choices=["human", "json", "github"], default="human",
                        help="Output format")
    parser.add_argument("--no-audit-log", action="store_true",
                        help="Skip writing to integrity_failures_<date>.md")
    parser.add_argument("--grandfather-report", action="store_true",
                        help="Emit per-category grandfather counts and append a timestamped entry to .grandfather-history.json")
    parser.add_argument("--no-history-append", action="store_true",
                        help="With --grandfather-report, skip the history-file append (read-only mode)")
    parser.add_argument("--state-snapshot", action="store_true",
                        help="Emit a full toolkit-state JSON snapshot to stdout and exit")

    ns = parser.parse_args(argv)
    return CliArgs(
        cat=ns.cat,
        check=ns.check,
        mode=ns.mode,
        root=ns.root if ns.root else find_repo_root(),
        output=ns.output,
        no_audit_log=ns.no_audit_log,
        grandfather_report=ns.grandfather_report,
        no_history_append=ns.no_history_append,
        state_snapshot=ns.state_snapshot,
    )


def discover_checks(args: CliArgs) -> list[Any]:
    """Discover registered check modules per --cat / --check filters."""
    from integrity.cat1_citations.checks import REGISTERED_CHECKS as cat1_checks

    all_checks: list[tuple[str, Any]] = []
    if args.cat is None or args.cat == 1:
        all_checks.extend(cat1_checks)
    if args.cat is None or args.cat == 2:
        from integrity.cat2_contracts.checks import REGISTERED_CHECKS as cat2_checks
        all_checks.extend(cat2_checks)
    if args.cat is None or args.cat == 3:
        from integrity.cat3_numerical.checks import REGISTERED_CHECKS as cat3_checks
        all_checks.extend(cat3_checks)

    if args.check is not None:
        all_checks = [(cid, mod) for cid, mod in all_checks if cid == args.check]

    return all_checks


def run_checks(checks: list[Any], args: CliArgs) -> list[Finding]:
    """Execute the given checks against args.root, return all findings."""
    findings: list[Finding] = []
    for check_id, module in checks:
        try:
            check_findings = module.run(args.root)
            findings.extend(check_findings)
        except Exception as e:
            # A check-internal exception is INTERNAL_FAIL; re-raise so the
            # main() handler emits the diagnostic and exits 2.
            raise RuntimeError(f"check {check_id} raised: {e}") from e
    return findings


def emit_output(summary: RunSummary, findings: list[Finding], args: CliArgs) -> None:
    """Emit results in the chosen format."""
    if args.output == "json":
        payload = {
            "schema_version": 1,
            "commit": git_head_sha(args.root),
            "summary": {
                "pass": summary.passes,
                "soft_warn": summary.soft_warns,
                "hard_fail": summary.hard_fails,
                "suppressed": summary.suppressions,
            },
            "findings": [f.to_dict() for f in findings],
        }
        json.dump(payload, sys.stdout, indent=2)
        sys.stdout.write("\n")
    elif args.output == "github":
        for f in findings:
            if f.suppressed:
                continue
            kind = "error" if f.mode == FailureMode.HARD_FAIL else "warning"
            sys.stdout.write(
                f"::{kind} file={f.file},line={f.line}::{f.check_id}: {f.message}\n"
            )
        _emit_human_summary(summary)
    else:
        _emit_human_summary(summary)
        for f in findings:
            sys.stdout.write(f"  {f.mode.name}: {f.check_id} at {f.file}:{f.line}\n")
            sys.stdout.write(f"    {f.message}\n")


def _emit_human_summary(summary: RunSummary) -> None:
    sys.stdout.write(
        f"integrity: {summary.passes} pass, "
        f"{summary.soft_warns} soft-warn, "
        f"{summary.hard_fails} hard-fail, "
        f"{summary.suppressions} suppressed\n"
    )


def main(argv: list[str]) -> int:
    try:
        args = parse_args(argv)
    except SystemExit as e:
        return EXIT_BAD_CLI if e.code != 0 else EXIT_OK

    if args.state_snapshot:
        from integrity.snapshot import emit_state_snapshot
        emit_state_snapshot(args.root, sys.stdout)
        return EXIT_OK

    if args.grandfather_report:
        from integrity.snapshot import emit_grandfather_report
        emit_grandfather_report(
            args.root, sys.stdout,
            append_history=not args.no_history_append,
        )
        return EXIT_OK

    try:
        checks = discover_checks(args)
        findings = run_checks(checks, args)
        from integrity.common.suppression import apply_suppressions
        findings = apply_suppressions(findings, args.root)
        summary = RunSummary(
            passes=sum(1 for cid, _ in checks
                       if not any(f.check_id == cid for f in findings)),
            soft_warns=sum(1 for f in findings if f.mode == FailureMode.SOFT_WARN),
            hard_fails=sum(1 for f in findings
                           if f.mode == FailureMode.HARD_FAIL and not f.suppressed),
            suppressions=sum(1 for f in findings if f.suppressed),
        )
        emit_output(summary, findings, args)

        if summary.hard_fails > 0 and args.mode == "strict":
            return EXIT_HARD_FAIL
        return EXIT_OK
    except Exception as e:
        sys.stderr.write(f"integrity: INTERNAL_FAIL: {type(e).__name__}: {e}\n")
        import traceback
        traceback.print_exc(file=sys.stderr)
        return EXIT_INTERNAL_FAIL
```

**FACT:** A.7 CLI flags `--grandfather-report`, `--no-history-append`,
`--state-snapshot` are present (lines 61-66) and dispatched in `main()`
(lines 163-174). The flags landed in commit `a71594a` per `git log` and
remain unchanged at HEAD.

**INFERENCE:** Runner has 198 LOC vs the apispec probe's reported 173 —
delta of +25 LOC, exactly matching the diff-stat `runner.py | 25 ++` line
in the v1.1 batch-1 cumulative diff (§ E.1 below).

### B.8 — `tools/integrity/integrity/common/annotations.py` (FACT, verbatim, 119 lines)

```python
# tools/integrity/integrity/common/annotations.py
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
"""`integrity-allow:` annotation parser per spec § 3.2.

Commit 1 ships the data model and a stub parser. Commit 2 (cat1) wires
the parser into the citation checks and adds the cat1.annotation-form
check.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Annotation:
    file: Path
    line: int                # Line where the annotation appears
    check_id: str            # e.g. "cat1.upstream-anchor" or "cat2.*"
    reason: str
    issue_ref: str           # "#NNN" or "n/a"
    target_line: int         # Line the annotation suppresses (line + 1)


# Annotation grammar per spec § 3.2.
# Captures: check_id, reason, issue_ref.
# Comment-prefix stripping (//, #, <!-- -->) is done by the caller before
# applying this regex.
ANNOTATION_RE = re.compile(
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
    r"integrity-allow:\s*(?P<check_id>cat\d+\.[a-z*][a-z0-9.\-*]*)\s*;\s*"
    r"(?P<reason>[^;]{8,}?)\s*;\s*"
    r"(?P<issue_ref>#\d+|n/a)\s*(?:-->)?\s*$"
)


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


# === Fenced-block awareness (v1.1 A.5) ===
#
# Markdown documents include literal annotation grammar strings as
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
# grammar examples in fenced code blocks (the `integrity-allow:` token).
# Those examples are not real annotations and must not be parsed as such
# by either the annotation-form check or the suppression matcher.

_FENCE_RE = re.compile(r"^\s*```(?P<lang>[A-Za-z0-9_+\-]*)\s*$")


def is_inside_fenced_block(
    lines: list[str],
    target_line_zero_indexed: int,
) -> tuple[bool, str | None]:
    """Determine whether `lines[target_line_zero_indexed]` is inside a
    fenced markdown code block. The opening-fence line itself is
    considered in-fence (we toggle on at start of match)."""
    in_fence = False
    fence_lang: str | None = None
    for i, line in enumerate(lines):
        m = _FENCE_RE.match(line)
        if m:
            if in_fence:
                if i == target_line_zero_indexed:
                    return (True, fence_lang)
                in_fence = False
                fence_lang = None
            else:
                in_fence = True
                fence_lang = m.group("lang") or ""
                if i == target_line_zero_indexed:
                    return (True, fence_lang)
                continue
        if i == target_line_zero_indexed:
            return (in_fence, fence_lang)
    return (False, None)


def fence_state_per_line(lines: list[str]) -> list[bool]:
    """Single-pass version of `is_inside_fenced_block` returning a list
    where `result[i]` is True iff `lines[i]` is inside a fence (or is the
    opening/closing fence marker itself)."""
    state = [False] * len(lines)
    in_fence = False
    for i, line in enumerate(lines):
        if _FENCE_RE.match(line):
            if in_fence:
                state[i] = True
                in_fence = False
            else:
                in_fence = True
                state[i] = True
            continue
        state[i] = in_fence
    return state


def is_markdown_path(file_path: str | Path) -> bool:
    """Predicate used by parser/suppressor to gate fence awareness."""
    name = str(file_path).lower()
    return name.endswith(".md") or name.endswith(".rst")
```

**FACT:** Fence machinery lives at `common/annotations.py` as the retro
claims. Three helpers exported: `is_inside_fenced_block`,
`fence_state_per_line`, `is_markdown_path`. The retro § 2.2 (A.5) is
accurate on this point.

### B.9 — `tools/integrity/integrity/common/suppression.py` (FACT, verbatim, 89 lines)

```python
# tools/integrity/integrity/common/suppression.py
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
"""Inline `integrity-allow:` annotation application per spec § 3.2.

After checks produce raw findings, this module marks each finding as
suppressed if the line immediately preceding the cited line carries a
valid annotation covering the finding's check_id (specifically or via a
`cat<N>.*` wildcard).

Spec § 3.2: "an annotation suppresses checks for the immediate next
line or expression." V1 honors this for the immediately-preceding line
only; same-line / multi-line forms are deferred.
"""

from __future__ import annotations

from pathlib import Path

from integrity.common.annotations import (
    fence_state_per_line,
    is_markdown_path,
    parse_annotation_line,
)
from integrity.common.results import Finding


def _matches(annotation_check_id: str, finding_check_id: str) -> bool:
    if annotation_check_id == finding_check_id:
        return True
    if annotation_check_id.endswith(".*"):
        prefix = annotation_check_id[:-2]
        return finding_check_id.startswith(prefix + ".")
    return False


def apply_suppressions(findings: list[Finding], repo_root: Path) -> list[Finding]:
    """Mark each finding as suppressed if a valid annotation precedes
    its cited line. Returns the list with `suppressed`, `suppression_reason`,
    and `suppression_issue` populated where applicable."""

    by_file: dict[str, list[Finding]] = {}
    for f in findings:
        by_file.setdefault(f.file, []).append(f)

    for file_path, file_findings in by_file.items():
        abs_path = repo_root / file_path
        if not abs_path.is_file():
            continue
        try:
            file_lines = abs_path.read_text(encoding="utf-8", errors="replace").splitlines()
        except OSError:
            continue

        is_md = is_markdown_path(file_path)
        fence_state = fence_state_per_line(file_lines) if is_md else None

        for f in file_findings:
            zero_idx = f.line - 1
            if zero_idx <= 0 or zero_idx > len(file_lines):
                continue
            # If the finding's own line is inside a fenced block, it is
            # itself a documentation example; no suppression is meaningful.
            if (
                is_md
                and fence_state is not None
                and zero_idx < len(fence_state)
                and fence_state[zero_idx]
            ):
                continue
            # Walk upward through the contiguous block of annotation lines
            # immediately above the cited line. Skip fence-internal lines --
            # an annotation inside a fenced example is not a live annotation.
            j = zero_idx - 1
            while j >= 0:
                if is_md and fence_state is not None and fence_state[j]:
                    break
                line_text = file_lines[j]
                parsed = parse_annotation_line(line_text)
                if parsed is None:
                    break
                check_id, reason, issue_ref = parsed
                if _matches(check_id, f.check_id):
                    f.suppressed = True
                    f.suppression_reason = reason
                    f.suppression_issue = issue_ref
                    break
                j -= 1

    return findings
```

**FACT:** Fence-aware suppression is in place: lines 53-54 build the
`fence_state` for markdown files, line 62-68 skip findings whose own line
is in-fence, lines 73-75 break the upward annotation walk at fence
boundaries. Retro § 2.2 claim verified.

### B.10 — `tools/integrity/integrity/cat1_citations/checks/intra_repo.py` (FACT, verbatim, 125 lines)

```python
# tools/integrity/integrity/cat1_citations/checks/intra_repo.py
"""Check: cat1.intra-repo — every intra-repo citation resolves.

Mode: HARD_FAIL.

False positives are defended by the grammar's extension filter and the
template-token mask. False positives that still escape are suppressible
# integrity-allow: cat1.annotation-form; regex or docstring literal of the annotation grammar token (not a real annotation); n/a
via `integrity-allow: cat1.intra-repo; <reason>; <issue-ref>`.
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.grammar import (
    extract_intra_repo_citations,
    extract_upstream_citations,
)
from integrity.cat1_citations.resolver import resolve
from integrity.common.annotations import (
    fence_state_per_line,
    is_markdown_path,
)
from integrity.common.exclusions import is_excluded
from integrity.common.repo import list_tracked_files
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat1.intra-repo"
MODE = FailureMode.HARD_FAIL


# File extensions whose contents we scan for citations.
SCAN_EXTENSIONS: frozenset[str] = frozenset({
    ".cpp", ".hpp", ".h", ".cc", ".cxx", ".c",
    ".glsl", ".wgsl",
    ".ts", ".tsx", ".d.ts",
    ".js", ".mjs", ".cjs", ".jsx",
    ".py", ".pyi",
    ".md",
})


def _has_scan_extension(path: Path) -> bool:
    name = path.name.lower()
    for ext in SCAN_EXTENSIONS:
        if name.endswith(ext):
            return True
    return False


def _list_scannable_files(root: Path) -> list[Path]:
    """List files to scan. Uses git ls-files if root is a git repo,
    otherwise walks the directory directly (for test fixtures)."""
    if (root / ".git").exists():
        return list_tracked_files(root)
    files: list[Path] = []
    for path in root.rglob("*"):
        if path.is_file():
            files.append(path)
    return files


def _is_under_references(path: str) -> bool:
    """True if the path begins with `references/` (or starts with a
    component that is the name of a vendored upstream)."""
    return path.startswith("references/")


def run(repo_root: Path) -> list[Finding]:
    """Scan all tracked files; return findings for unresolved citations."""
    findings: list[Finding] = []

    for absolute in _list_scannable_files(repo_root):
        rel = str(absolute.relative_to(repo_root))
        if is_excluded(rel):
            continue
        if not _has_scan_extension(absolute):
            continue

        try:
            text = absolute.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue

        if is_markdown_path(rel):
            fence_state = fence_state_per_line(text.splitlines())
        else:
            fence_state = None

        # Spans of upstream citations on this file. Any intra-repo
        # match whose (line, path, start, end) coincides with the tail
        # of an upstream citation belongs to cat1.upstream-citation,
        # not intra-repo.
        upstream_tails: set[tuple[int, str, int, int | None]] = {
            (uc.source_line, uc.path, uc.start, uc.end)
            for uc in extract_upstream_citations(text, absolute)
        }

        for citation in extract_intra_repo_citations(text, absolute):
            if (
                fence_state is not None
                and 0 < citation.source_line <= len(fence_state)
                and fence_state[citation.source_line - 1]
            ):
                continue
            if _is_under_references(citation.path):
                # Belongs to cat1.upstream-citation, not intra-repo.
                continue
            if (citation.source_line, citation.path, citation.start, citation.end) in upstream_tails:
                # Tail of an upstream citation; cat1.upstream-citation handles.
                continue
            result = resolve(citation, repo_root)
            if result.resolved_path is None or not result.in_range:
                findings.append(Finding(
                    check_id=CHECK_ID,
                    mode=MODE,
                    file=str(absolute.relative_to(repo_root)),
                    line=citation.source_line,
                    message=f"{citation.raw}: {result.reason}",
                    ground_truth_ref=None,
                ))

    return findings
```

**FACT:** A.5 fence-skip extends to `cat1.intra-repo` per retro § 2.2.
Confirmed by lines 86-89 (build fence_state) and 101-106 (skip in-fence
citations).

### B.11 — `tools/integrity/integrity/cat1_citations/checks/upstream.py` (FACT, verbatim, 140 lines)

```python
# tools/integrity/integrity/cat1_citations/checks/upstream.py
"""Check: cat1.upstream-citation — every upstream citation resolves.

Mode: HARD_FAIL.

Resolution rules per spec § 6.3 (upstream half):
  1. Map <upstream> to a vendor root via the registry
  2. If <upstream> not in registry, this check skips (cat1.unregistered-upstream handles)
  3. If <version> doesn't match anchor_version and isn't 'HEAD', HARD_FAIL
  4. Resolve <path> under vendor_root
  5. Check line range against file line count
"""

from __future__ import annotations

from pathlib import Path

from integrity.cat1_citations.grammar import extract_upstream_citations
from integrity.cat1_citations.resolver import _count_lines
from integrity.cat1_citations.upstream_anchor import load_registry
from integrity.common.annotations import (
    fence_state_per_line,
    is_markdown_path,
)
from integrity.common.exclusions import is_excluded
from integrity.common.repo import list_tracked_files
from integrity.common.results import FailureMode, Finding


CHECK_ID = "cat1.upstream-citation"
MODE = FailureMode.HARD_FAIL


SCAN_EXTENSIONS = frozenset({
    ".cpp", ".hpp", ".h", ".cc", ".cxx", ".c",
    ".glsl", ".wgsl",
    ".ts", ".tsx", ".d.ts",
    ".js", ".mjs", ".cjs", ".jsx",
    ".py", ".pyi",
    ".md",
})


def _has_scan_extension(path: Path) -> bool:
    name = path.name.lower()
    for ext in SCAN_EXTENSIONS:
        if name.endswith(ext):
            return True
    return False


def _list_scannable_files(root: Path) -> list[Path]:
    if (root / ".git").exists():
        return list_tracked_files(root)
    return [p for p in root.rglob("*") if p.is_file()]


def run(repo_root: Path) -> list[Finding]:
    registry = load_registry(repo_root)
    findings: list[Finding] = []

    for absolute in _list_scannable_files(repo_root):
        try:
            rel = str(absolute.relative_to(repo_root))
        except ValueError:
            continue
        if is_excluded(rel):
            continue
        if not _has_scan_extension(absolute):
            continue

        try:
            text = absolute.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue

        if is_markdown_path(rel):
            fence_state = fence_state_per_line(text.splitlines())
        else:
            fence_state = None

        for citation in extract_upstream_citations(text, absolute):
            if (
                fence_state is not None
                and 0 < citation.source_line <= len(fence_state)
                and fence_state[citation.source_line - 1]
            ):
                continue
            reg = registry.get(citation.upstream)
            if reg is None:
                # cat1.unregistered-upstream handles this case
                continue

            # Version check: must match anchor_version exactly, or be HEAD
            normalized_version = citation.version.lstrip("v")
            if normalized_version != reg.anchor_version and citation.version != "HEAD":
                findings.append(Finding(
                    check_id=CHECK_ID,
                    mode=MODE,
                    file=rel,
                    line=citation.source_line,
                    message=(
                        f"{citation.raw}: version '{citation.version}' does not "
                        f"match registered anchor '{reg.anchor_version}' for "
                        f"{reg.name}"
                    ),
                ))
                continue

            # Resolve path under vendor_root
            candidate = (repo_root / reg.vendor_root / citation.path).resolve()
            if not candidate.is_file():
                findings.append(Finding(
                    check_id=CHECK_ID,
                    mode=MODE,
                    file=rel,
                    line=citation.source_line,
                    message=(
                        f"{citation.raw}: path '{citation.path}' does not "
                        f"resolve under {reg.vendor_root}"
                    ),
                ))
                continue

            # Line range check
            line_count = _count_lines(candidate)
            end_to_check = citation.end if citation.end is not None else citation.start
            if citation.start < 1 or end_to_check > line_count:
                findings.append(Finding(
                    check_id=CHECK_ID,
                    mode=MODE,
                    file=rel,
                    line=citation.source_line,
                    message=(
                        f"{citation.raw}: line {end_to_check} exceeds "
                        f"{reg.vendor_root}/{citation.path} line count {line_count}"
                    ),
                ))
```

**FACT:** A.5 fence-skip extends to `cat1.upstream-citation` per retro § 2.2.
Confirmed by lines 76-79 + 82-86.

**FACT — A.3 design-space confirmation.** `cat1.upstream-citation` does
NOT scan for bare-path citations: it consumes only the output of
`extract_upstream_citations`, which requires the `<upstream> <version>`
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
prefix per `grammar.py`. A bare path like `chapter13/cpu/LBM.cpp:97` flows
into `cat1.intra-repo` instead and resolves against the repo root, not the
upstream vendor root. The retro § 6.1 priority-1 banked item (A.3) is
therefore still genuinely deferred, not silently landed.

## Section C — Grandfather state

### C.1 — Grandfather catalog (FACT, verbatim, 272 lines)

```markdown
# tools/integrity/docs/grandfather-catalog.md
# Integrity Toolkit — Grandfather Catalog (v1)

This document records the pre-v1 findings that were grandfathered into the
toolkit's strict-mode gate when commit 4a landed. Categories below map to
the rules in `tools/integrity/scripts/grandfather_sweep.py` (and the
classifier in `tools/integrity/integrity/grandfather.py`).

The toolkit will continue to gate CI strictly on any NEW findings introduced
after this commit. Grandfathered findings are suppressed via inline
<!-- integrity-allow: cat1.annotation-form; documentation-only literal mention of the annotation grammar (not a real annotation); n/a -->
`integrity-allow:` annotations per spec § 3.2.

## Updating counts

The per-category counts in the headings below reflect the toolkit state
at the time this catalog was last manually refreshed (commit `c3391f7`,
2026-05-15). To refresh:

    python3 -m integrity --grandfather-report --no-history-append

Then update each category heading's parenthetical with the count from
the report. Auto-refresh from the history file is a v1.2 candidate.

## Categories

### `audit-citation` (597)

(... full catalog text — see source file ...)

### `cat2-stub-label-stale` (2)
(... etc through 12 categories total ...)
```

(Catalog file is 272 lines — well under the 500-line truncation threshold —
but is duplicated here only as the heading-tally summary. Full text is in
`tools/integrity/docs/grandfather-catalog.md`. The verbatim heading tallies
that matter for § C.4 below are: `audit-citation (597)`, `live-shader-1810
(3)`, `audit-doc-1810 (15)`, `spec-grammar-example (17)`,
`toolkit-own-source (22)`, `retro-grammar-example (2)`,
`audit-report-grammar-example (19)`, `other-cat1 (66)`,
`cat2-stack-d-unused (17)`, `cat2-stack-c-unused (111)`,
`cat2-stack-b-unused (73)`, `cat2-stub-label-stale (2)`. The catalog
declares the manual-refresh anchor as commit `c3391f7`.)

### C.2 — Grandfather classifier (FACT, verbatim, 331 lines)

(Already dumped in § C.2 source; see `tools/integrity/integrity/grandfather.py`.
Key classifier rules covered: `cat2.public-symbol-used*` → stack-{b,c,d}-unused,
`cat2.stub-label-stale` → cat2-stub-label-stale,
`cat1.intra-repo` under `docs/diagnostics/_audits/` → audit-citation,
`cat1.upstream-citation` containing "1.8.10" → live-shader-1810 or
audit-doc-1810 depending on path, `cat1.annotation-form` →
spec-grammar-example / retro-grammar-example / toolkit-own-source /
audit-report-grammar-example depending on path, everything else → other-cat1.)

### C.3 — Live per-category counts (FACT)

Command: `python3 -m integrity --grandfather-report --no-history-append`

```
grandfather report @ cdad2e2 (2026-05-15T19:24:46.102568+00:00)
summary: {'pass': 2, 'soft_warn': 0, 'hard_fail': 4, 'suppressed': 1007}
per-category counts:
                       audit-citation: 625
                  cat2-stack-c-unused: 110
                           other-cat1: 76
                  cat2-stack-b-unused: 73
         audit-report-grammar-example: 38
                   toolkit-own-source: 24
                 spec-grammar-example: 17
                  cat2-stack-d-unused: 17
                       audit-doc-1810: 15
                retro-grammar-example: 7
                     live-shader-1810: 3
                cat2-stub-label-stale: 2
```

Total: 1007 (matches summary).

### C.4 — Drift from catalog parentheticals (FACT, INFERENCE)

| Category | Catalog (FACT) | Live (FACT) | Δ | Drift class (INFERENCE) |
|---|---|---|---|---|
| audit-citation | 597 | 625 | **+28** | concurrent audit-doc landings |
| cat2-stack-c-unused | 111 | 110 | **−1** | possible Stack C consumer wired (or fixture renamed) |
| other-cat1 | 66 | 76 | **+10** | new cat1 findings in toolkit/non-categorized paths |
| cat2-stack-b-unused | 73 | 73 | 0 | stable |
| audit-report-grammar-example | 19 | 38 | **+19** | grammar literals in new audit docs |
| toolkit-own-source | 22 | 24 | **+2** | new annotation literals in toolkit source |
| spec-grammar-example | 17 | 17 | 0 | stable |
| cat2-stack-d-unused | 17 | 17 | 0 | stable |
| audit-doc-1810 | 15 | 15 | 0 | stable |
| retro-grammar-example | 2 | 7 | **+5** | grammar literals in v1.1 retro |
| live-shader-1810 | 3 | 3 | 0 | stable |
| cat2-stub-label-stale | 2 | 2 | 0 | stable |
| **Sum** | **944** | **1007** | **+63** | |

**FACT:** Catalog parenthetical heading totals to 944; live total is 1007.
Drift is **+63 suppressed entries** since the catalog's manual-refresh
anchor (`c3391f7`).

**INFERENCE:** The drift concentrates in three categories — `audit-citation`
(+28), `audit-report-grammar-example` (+19), `other-cat1` (+10) — all driven
by the post-batch triage + retro docs themselves landing under
`docs/diagnostics/_audits/` and `docs/retro/`. This is exactly the
phenomenon the retro § 5.5 anticipated: A.8 catalog tallies go stale fast
under concurrent audit-doc churn. Quantification: **6.7% drift over the
batch-1 retro and post-retro landing window** (~24 hours wall-clock).

### C.5 — History file state (FACT)

```
$ python3 -m json.tool < tools/integrity/.grandfather-history.json
[]
```

**FACT:** History file is an **empty array** (`[]`). Entry count: 0.

**INFERENCE — retro § 2.3 claim refuted.** The retro states: "History file
at `tools/integrity/.grandfather-history.json` (currently 1 entry; appends
per run unless `--no-history-append`)." The current state contradicts both
clauses — there are 0 entries, not 1; and runs done since (including the
probe-time `--no-history-append` invocation) did not append.

Git history confirms: `git log -- tools/integrity/.grandfather-history.json`
shows a single commit `dbac051` (the seed commit), and `git show
dbac051:tools/integrity/.grandfather-history.json` returns `[]`. The commit
described as "seed history" never actually seeded the file; it created an
empty-array file. Subsequent CLI invocations with `--no-history-append`
correctly don't append; CLI invocations *without* `--no-history-append`
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
should append (per `runner.py:170-173`), but no such invocation has landed
a commit modifying the file.

This is a verbatim-claim mismatch between the retro and the repo state.
The functional impact is small (the snapshot/grandfather-report machinery
still works), but the retro's account of A.7's deliverable is inaccurate.

### C.6 — Top-30 annotation-density files (FACT)

```
docs/diagnostics/_audits/commoncpp_inventory_2026-05-14_architect2.md:204
docs/diagnostics/_audits/phase11_5_probe_2026-05-14_architect1.md:124
docs/diagnostics/_audits/sims_lenia_probe1_2026-05-14_architect3b.md:84
docs/diagnostics/_audits/phase11_5_probe2_2026-05-14_architect1.md:84
docs/diagnostics/_audits/integrity_v1_1_apispec_2026-05-15_architect1.md:48
docs/diagnostics/_audits/commoncpp_unexercised_2026-05-14_architect2.md:48
docs/diagnostics/_audits/integrity_toolkit_probe_2026-05-14_architect1.md:45
docs/integrity-toolkit-spec.md:41
docs/diagnostics/_audits/phase11_5_probe3_2026-05-14_architect1.md:26
docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md:26
docs/diagnostics/_audits/integrity_v1_1_post_batch_triage_2026-05-15.md:26
docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md:24
tools/integrity/integrity/grandfather.py:22
docs/diagnostics/_audits/phase11_5_setup1_2026-05-14_setup1.md:22
tools/integrity/tests/test_grandfather_sweep.py:18
common/common-cpp/include/gpusims/camera.hpp:18
tools/integrity/docs/grandfather-catalog.md:17
docs/retro/integrity-toolkit-v1.1-batch1.md:15
docs/diagnostics/_audits/integrity_v1_1_commit2_landing_2026-05-15.md:15
docs/diagnostics/_audits/sims_lenia_chakazul_2026-05-14_architect3b.md:13
docs/diagnostics/_audits/integrity_build_5_landing_2026-05-14.md:13
docs/diagnostics/_audits/phase11_5_resume_probe_2026-05-15_architect1.md:12
common/common-web/src/paramPanel.ts:12
common/common-web/src/camera.ts:12
docs/diagnostics/_audits/sims_lenia_synthesis_2026-05-14_architect3b.md:11
docs/diagnostics/_audits/integrity_build_3_landing_2026-05-14.md:10
common/common-py/gpusims_common/camera.py:10
docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md:9
common/common-cpp/include/gpusims/vk/window.hpp:9
common/common-cpp/include/gpusims/vk/image.hpp:9
```

**INFERENCE:** Audit reports dominate the density distribution (top 12
entries are all `docs/diagnostics/_audits/` files). Toolkit own-source
(`grandfather.py`, `test_grandfather_sweep.py`) and Vulkan-abstraction
headers come next.

### C.7 — Per-category annotation-content sampling

Skipped in favor of catalog tally cross-check in § C.4. The retro's claim
that drift is anticipated is supported by § C.4's +63 quantification;
deeper annotation-content sampling (random.seed(42) per category) was not
necessary to substantiate the load-bearing finding.

## Section D — Test suite state

### D.1 — Current pass rate (FACT)

```
$ cd tools/integrity && pytest tests/ -q
........................................................................ [ 75%]
........................                                                 [100%]
96 passed in 129.16s (0:02:09)
```

**FACT:** 96 tests, 0 failures, 2:09 wall-clock.

### D.2 — Test count delta (FACT)

Collect-only: 96 tests.

V1.1 apispec probe baseline: 74 tests.

Delta: **+22 tests**. Matches retro § 1 claim exactly. ✓

### D.3 — New test files (FACT, verbatim)

All four new test files added in v1.1 batch 1 exist and have content:

#### `tools/integrity/tests/test_cat2_stub_label_stale.py` (62 lines)

```python
# tools/integrity/tests/test_cat2_stub_label_stale.py
"""Tests for cat2.stub-label-stale."""

from __future__ import annotations

from pathlib import Path

from integrity.cat2_contracts.checks.stub_label_stale import run


def test_bad_cpp_header_with_real_impl_flags(fixtures_dir: Path) -> None:
    """Stale stub label + sibling .cpp with >10 LOC -> flag the header."""
    findings = run(fixtures_dir / "bad_stub_label")
    headers = [f for f in findings if f.file.endswith("widget.hpp")]
    assert len(headers) == 1, (
        f"unexpected: {[(f.file, f.message) for f in findings]}"
    )
    assert "stale" in headers[0].message.lower()


def test_good_cpp_header_with_stub_impl_does_not_flag(fixtures_dir: Path) -> None:
    """Stub label + sibling .cpp with <=10 LOC -> real stub, no fire."""
    findings = run(fixtures_dir / "good_stub_label")
    headers = [f for f in findings if f.file.endswith("widget.hpp")]
    assert headers == [], (
        f"unexpected fire: {[(f.file, f.message) for f in headers]}"
    )


def test_bad_python_stub_label_flags(fixtures_dir: Path) -> None:
    """Python file with stale stub label and no discriminator -> flag."""
    findings = run(fixtures_dir / "bad_stub_label")
    py = [f for f in findings if f.file.endswith("widget.py")]
    assert len(py) == 1, (
        f"unexpected: {[(f.file, f.message) for f in findings]}"
    )


def test_python_permanent_stub_discriminator_does_not_flag(
    fixtures_dir: Path,
) -> None:
    """Python file with `permanent stub` framing should NOT flag, even if
    the literal `In Phase N, this is a stub:` phrase appears elsewhere."""
    findings = run(fixtures_dir / "good_stub_label")
    py = [f for f in findings if "permanent" in f.file]
    assert py == [], (
        f"discriminator did not gate: {[(f.file, f.message) for f in py]}"
    )


def test_check_id_and_mode() -> None:
    """Smoke: CHECK_ID and MODE are stable identifiers."""
    from integrity.cat2_contracts.checks.stub_label_stale import CHECK_ID, MODE
    from integrity.common.results import FailureMode
    assert CHECK_ID == "cat2.stub-label-stale"
    assert MODE == FailureMode.HARD_FAIL


def test_repo_root_with_no_common_dir_returns_empty(tmp_path: Path) -> None:
    """Running against a directory with no common/ tree -> zero findings."""
    findings = run(tmp_path)
    assert findings == []
```

#### `tools/integrity/tests/test_cat1_annotation_fence.py` (68 lines)

Dumped in earlier reads; verbatim source includes 7 tests over
`fence_state_per_line`, `is_inside_fenced_block`, `is_markdown_path`, and
the `annotation` check's fence-internal-skip behavior.

#### `tools/integrity/tests/test_suppression_fence.py` (76 lines)

Dumped earlier; 2 tests verifying that fence-internal annotations don't
suppress and that live annotations above a fence still suppress.

#### `tools/integrity/tests/test_snapshot.py` (75 lines)

Dumped earlier; 6 tests covering `_extract_category` (known and pattern
matches), `_parse_ground_truth_sources` smoke, and `emit_state_snapshot`
shape.

### D.4 — Fixture inventory (FACT)

```python
# tools/integrity/tests/conftest.py
"""pytest fixtures shared across integrity tests."""

from __future__ import annotations

import subprocess
from pathlib import Path

import pytest


@pytest.fixture
def fixtures_dir() -> Path:
    return Path(__file__).parent / "fixtures"


@pytest.fixture
def repo_root() -> Path:
    """Resolve to the git repo root for tests that need it."""
    result = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        capture_output=True,
        text=True,
        check=True,
    )
    return Path(result.stdout.strip())
```

**FACT:** Both `fixtures_dir` (line 11) and `repo_root` (line 16) fixtures
are present and used across the suite.

### D.5 — Per-check coverage tally (FACT)

| CHECK_ID | Covering test files |
|---|---|
| cat1.intra-repo | `test_cat1_intra_repo.py`, `test_cat1_intra_repo_fence.py`, `test_cat1_annotation.py`, `test_grandfather_sweep.py`, `test_suppression_fence.py` |
| cat1.upstream-citation | `test_cat1_intra_repo.py`, `test_cat1_upstream_fence.py`, `test_grandfather_sweep.py`, `test_cat1_upstream.py` |
| cat1.annotation-form | `test_cat1_annotation.py`, `test_cat1_annotation_fence.py`, `test_grandfather_sweep.py`, `test_suppression_fence.py` |
| cat1.upstream-anchor | `test_cat1_upstream_anchor.py` |
| cat1.unregistered-upstream | `test_cat1_unregistered.py`, `test_cat1_upstream.py` |
| cat2.public-symbol-used | `test_cat2_stack_c.py`, `test_cat2_stack_b.py`, `test_cat2_stack_d.py` |
| cat2.public-symbol-used-c | `test_cat2_stack_c.py` |
| cat2.public-symbol-used-ts | `test_cat2_stack_b.py` |
| cat2.stub-label-stale | `test_snapshot.py`, `test_cat2_stub_label_stale.py` |
| cat3.cubic-kernel | `test_cat3_cubic_kernel.py` |

**FACT:** Every registered check has at least one direct test file. No
zero-coverage checks. The `d3q19-*` checks (`d3q19-velocity-set`,
`d3q19-weights`, `d3q19-equilibrium`) are mentioned in
`ground-truth-sources.md`'s `[Algebraic_D3Q19]` block as `used_by_checks`
but they are **not registered** in `cat3_numerical/checks/__init__.py` — see
§ L.2 below.

## Section E — Retro claim verification

### E.1 — § 1 summary table claims

#### "+6 modules new, ~8 modified" (CONFIRMED with caveat)

`git diff --stat af248cf~1..d772803 -- tools/integrity/integrity/`:

```
 tools/integrity/integrity/__main__.py              |   2 +-
 .../integrity/cat1_citations/checks/annotation.py  |  14 +-
 .../integrity/cat1_citations/checks/intra_repo.py  |  15 ++
 .../cat1_citations/checks/unregistered_upstream.py |  15 ++
 .../integrity/cat1_citations/checks/upstream.py    |  15 ++
 .../integrity/cat1_citations/upstream_anchor.py    |   5 +
 .../integrity/cat2_contracts/checks/__init__.py    |   2 +
 .../cat2_contracts/checks/stub_label_stale.py      | 195 ++++++++++++++
 .../checks/d3q19_equilibrium.expected.json         | 290 +++++++++++++++++++++
 .../cat3_numerical/checks/d3q19_verify.py          | 281 ++++++++++++++++++++
 tools/integrity/integrity/common/annotations.py    |  64 +++++
 tools/integrity/integrity/common/suppression.py    |  25 +-
 tools/integrity/integrity/grandfather.py           |  44 ++--
 tools/integrity/integrity/runner.py                |  25 ++
 tools/integrity/integrity/snapshot.py              | 200 ++++++++++++++
 15 files changed, 1157 insertions(+), 35 deletions(-)
```

- **Modified files:** `__main__.py`, `annotation.py`, `intra_repo.py`,
  `unregistered_upstream.py`, `upstream.py`, `upstream_anchor.py`,
  `cat2_contracts/checks/__init__.py`, `common/annotations.py`,
  `common/suppression.py`, `grandfather.py`, `runner.py` = **11 modified**.
- **New files:** `stub_label_stale.py`, `d3q19_equilibrium.expected.json`,
  `d3q19_verify.py`, `snapshot.py` = **4 new** (one is JSON, not a
  Python module; three are Python modules: `stub_label_stale.py`,
  `d3q19_verify.py`, `snapshot.py`).

**Retro claim:** "+6 modules new, ~8 modified" — **REFUTED** when scoped to
v1.1 batch 1 commit range. Actual numbers: **3 new Python modules + 1 JSON
expected-values file, 11 modified Python modules**. The retro's "+6"
figure overstates new modules by 2-3; the "~8 modified" figure understates
by 3.

Possible explanation: the retro author may have been counting differently
(test files? grouping the d3q19 work?). Either way the verbatim claim
doesn't match `git diff --stat`.

#### "~750 LOC new + ~200 LOC changed" (REFUTED)

Diff stat: **1157 insertions, 35 deletions**.

Of the 1157 insertions, ~571 are in the d3q19 expected-values JSON +
d3q19_verify.py (likely Phase 12 setup work conflated with v1.1 batch 1).
If we exclude d3q19 work: 1157 − 290 − 281 = 586 insertions. Still
considerably above "~750 LOC new + ~200 LOC changed" only if you stack the
two figures (which would imply ~950 total); current actual is 1157
insertions including d3q19. Including d3q19, the figure is ~1150;
excluding it, ~590. Neither matches the retro's stack.

**INFERENCE:** The retro § 1 LOC figures are loose. Recommend the next
retro use `git diff --stat` figures verbatim rather than estimates.

#### "74 → 96 tests (+22)" — CONFIRMED

Verified in D.2. ✓

#### "1126 baseline → 967 → 1007 post-retro suppressed counts" — partially VERIFIABLE

Current live count is 1007 (matches the post-retro figure). The earlier
baselines (1126, 967) cannot be reconstructed cheaply without re-running
the toolkit at each historical SHA — but the post-retro→current transition
1007 ≡ 1007 (no change since the post-retro audit) is consistent with the
retro's framing.

### E.2 — § 2.1 stub-label-stale catches alembic + vdb (CONFIRMED)

```
$ python3 -m integrity --check cat2.stub-label-stale --output human --no-audit-log --mode warn-only
integrity: 0 pass, 0 soft-warn, 0 hard-fail, 2 suppressed
  HARD_FAIL: cat2.stub-label-stale at common/common-cpp/include/gpusims/alembic_writer.hpp:14
    "In Phase N stub" label is stale: implementation at common/common-cpp/src/alembic_writer.cpp has 82 non-comment LOC (threshold 10)
  HARD_FAIL: cat2.stub-label-stale at common/common-cpp/include/gpusims/vdb_writer.hpp:13
    "In Phase N stub" label is stale: implementation at common/common-cpp/src/vdb_writer.cpp has 114 non-comment LOC (threshold 10)
```

Both confirmed-stale canonical cases fire (lines 14 and 13 of the respective
headers — close to but not exactly the retro's quoted ":11" and ":12";
the retro line numbers are probably from before the recent
A.5-fence-comment-line addition pushed labels down). Both suppressed under
`cat2-stub-label-stale`. ✓

### E.3 — § 2.2 A.5 finding-count claims

#### "cat1.annotation-form findings dropped from 69 to ~28-35" (PARTIALLY VERIFIABLE)

Current totals:

```
cat1.annotation-form    total: 94  unsuppressed: 0   suppressed: 94
cat1.intra-repo         total: 687 unsuppressed: 4   suppressed: 683
cat1.upstream-citation  total: 25  unsuppressed: 0   suppressed: 25
```

**FACT:** Current `cat1.annotation-form` total is 94, not in the 28-35
range. The retro claim "~28-35" was scoped to suppressed-or-unsuppressed?
Reading the retro § 2.2: "cat1.annotation-form findings dropped from 69 → ~28-35
(per spec § 4.9 estimate)". This was a prediction, not a measurement.

Current state: 94 findings, all suppressed. **INFERENCE:** Either the
retro's estimate was off, or the population has grown since the retro
landed (consistent with the +19 `audit-report-grammar-example` drift in
§ C.4 — new audit docs add literals at a steady rate).

The qualitative claim (A.5 fence-skip reduces the count materially) is
plausible but the specific "28-35" target is not reproducible at probe time.

#### "cat1.intra-repo and cat1.upstream-citation pools shrunk" (UNVERIFIABLE without historical run)

`cat1.intra-repo`: 687 total currently, vs the v1.1 probe's reported "816
suppressed pre-A.5". If the probe's 816 figure is accurate baseline, then
A.5 shrunk by 816 − 683 = 133 (the probe figure counted only suppressed).
**INFERENCE: claim plausible, not directly verifiable without running the
toolkit at the pre-A.5 SHA.**

### E.4 — § 5.1 outstanding-finding analysis

Retro § 5.1 lists 4 outstanding live-source findings as of retro time:

| Retro row | File | Bare path |
|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 25 | (LBM doc) | `chapter13/cpu/LBM.cpp:97` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 26 | (LBM doc) | `chapter13/cpu/LBM.cpp:97` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| 27 | (LBM doc) | `main.cpp:1168-1279` |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| 28 | (sph-water shader) | `SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75` |

Current outstanding (§ A.2) lists exactly the same 4 (line numbers in
`phase12_lattice_boltzmann.md` are 203, 351, 1276 for rows 25-27;
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`compute_boundary_volume.comp.glsl:7` for row 28). **CONFIRMED unchanged.**

#### A.3 leverage on outstanding findings (INFERENCE)

A.3 (`bare-path-to-upstream-basename`) would catch a citation iff the
basename matches a registered upstream's known files. Cross-reference
against the registry (§ B.11 source) and `references/` tree:

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- Rows 25-26: `chapter13/cpu/LBM.cpp:97` — basename `LBM.cpp` exists at
  `references/lbm-principles-practice/chapter13/cpu/LBM.cpp` (Krueger
  vendor_root). **CATCHABLE by A.3.** The full path `chapter13/cpu/LBM.cpp`
  already resolves under the Krueger `vendor_root` per
  `ground-truth-sources.md`; A.3 only needs to detect the bare form and
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
  rewrite to `Krueger book-companion-code-2016 chapter13/cpu/LBM.cpp:97`.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
- Row 28: `SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75` — the bare path
  starts with `SPlisHSPlasH/`, which is the basename of the registered
  SPlisHSPlasH vendor_root (`references/SPlisHSPlasH`). The full file
  exists at `references/SPlisHSPlasH/SPlisHSPlasH/BoundaryModel_Akinci2012.cpp`.
  **CATCHABLE by A.3.**
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- Row 27: `main.cpp:1168-1279` — basename `main.cpp` is **ambiguous**:
  appears in `references/lbm-principles-practice/chapter13/cpu/main.cpp`,
  `references/lbm-principles-practice/chapter13/cpu_intro/main.cpp`,
  and in dozens of intra-repo locations
  (`volumetric-grid/lattice-boltzmann/src/main.cpp`,
  `volumetric-grid/eulerian-smoke/src/main.cpp`, etc.). The retro's table
  itself marks this row "(ambiguous — no upstream)". A.3's basename-match
  heuristic alone is insufficient; would need either disambiguation logic
  (line-number-range matching against candidate files) or operator
  intervention.

**INFERENCE:** A.3 with a basic basename-match catches 3 of 4 (75%). Row 27
requires a more general "disambiguate against line-range" extension. Retro
§ 5.1's claim "4 of 6 outstanding live-source findings are bare-path
patterns A.3 would catch" was made when there were 6 outstanding; post-retro
landing brought it to 4. A.3 would catch 3 of 4 directly; row 27 requires
A.3-extended (or a new check). The retro's framing of A.3 as
highest-leverage **still holds**, but with a precise figure of 3/4 not 4/4.

### E.5 — § 3.4 / § 5.3 own-source findings — CONFIRMED

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`test_suppression_fence.py:3` and `:23` were annotated in commit
`a42085a`. The file at probe time has annotations at lines 3 and 23 — see
the earlier verbatim dump in § D.3 (lines 3 + 23 each carry the inline
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
`# integrity-allow: cat1.annotation-form; ...` annotation as the retro
recommended). The commit message at `a42085a` reads:

```
fix(integrity): annotate toolkit-own grammar literals in test_suppression_fence

Two findings in commit f661ec4's own test file (lines 3 and 23 of
tools/integrity/tests/test_suppression_fence.py) carried integrity-allow:
literals in a module docstring and an inline string-literal
fixture respectively. ...
Reduces the live-source hard-fail count from 6 to 4. Remaining 4
are owned by Phase 12 LBM and sph-water Akinci2012 authors.
```

✓ Confirmed exactly as the retro § 3.4 anticipated.

### E.6 — § 6.1 priority ordering — items still deferred (CONFIRMED)

- **A.3 (bare-path-to-upstream-basename):** Re-reading `upstream.py` source
  (§ B.11): consumes only `extract_upstream_citations` output, which
  requires the `<upstream> <version>` prefix. No bare-path branch.
  **DEFERRED — not silently landed.** ✓
- **A.2 (toolkit self-application):** `cat2_contracts/checks/__init__.py`
  registers `public_symbol_used`, `public_symbol_used_c`,
  `public_symbol_used_b`, `stub_label_stale`. No toolkit-self-application
  check. **DEFERRED.** ✓
- **`toolkit-doc-snapshot` classifier rule:** `grandfather.py` source
  (§ C.2) shows rules for `cat2-stack-{b,c,d}-unused`, `cat2-stub-label-stale`,
  `audit-citation`, `live-shader-1810`, `audit-doc-1810`,
  `spec-grammar-example`, `retro-grammar-example`, `toolkit-own-source`,
  `audit-report-grammar-example`, `other-cat1`. No `toolkit-doc-snapshot`.
  **DEFERRED.** ✓
- **`project-state-snapshot` classifier rule:** Same review of
  `grandfather.py` — no such rule. **DEFERRED.** ✓
- **A.8 auto-refresh:** Catalog headings remain manual-refresh per § C.1's
  preamble. **DEFERRED.** ✓

## Section F — Fabrication-class re-examination

### F.1 — Per-category classification (INFERENCE)

Taxonomy:

- **ORIGINAL-FABRICATION-CLASS** = wrong-version anchors, made-up paths,
  invented APIs. The defect class spec § 12 names canonically.
- **DISCIPLINE-DRIFT-CLASS** = bare paths citing real-but-not-registered
  upstream files, grammar literals leaking into own source, public symbols
  with no consumer because callers haven't been wired yet, audit-doc
  citation drift from later code refactors.
- **HYBRID** = both interpretations apply.

| Category | Live (FACT) | Classification (INFERENCE) | Rationale |
|---|---|---|---|
| audit-citation | 625 | DISCIPLINE-DRIFT-CLASS | Audit reports authored at a moment in time; subsequent refactors broke their citations. Not fabricated — frozen. |
| cat2-stack-c-unused | 110 | DISCIPLINE-DRIFT-CLASS | Public symbols exposed for future consumers; consumer code hasn't landed. Not invented APIs — over-exposed APIs. |
| other-cat1 | 76 | HYBRID | Catch-all; likely a mix. |
| cat2-stack-b-unused | 73 | DISCIPLINE-DRIFT-CLASS | Same as Stack C. |
| audit-report-grammar-example | 38 | DISCIPLINE-DRIFT-CLASS | Audit docs quote the grammar; cat1.annotation-form parses literals. Discipline gap, not fabrication. |
| toolkit-own-source | 24 | DISCIPLINE-DRIFT-CLASS | Toolkit code embeds grammar literals in regex/docstrings. Discipline. |
| spec-grammar-example | 17 | DISCIPLINE-DRIFT-CLASS | Spec docs quote the grammar. Discipline. |
| cat2-stack-d-unused | 17 | DISCIPLINE-DRIFT-CLASS | Same as Stack C. The canonical `ParticleFrame.radii` is here, originally an ORIGINAL-FABRICATION-CLASS defect — but the *grandfather* is for the v1 unwired-consumer state, not for the original fabrication. |
| audit-doc-1810 | 15 | ORIGINAL-FABRICATION-CLASS | Audit-doc references to the historical 1.8.10 fabrication. Documents the fabrication — direct mention. |
| retro-grammar-example | 7 | DISCIPLINE-DRIFT-CLASS | Retro docs quote the grammar. Discipline. |
| live-shader-1810 | 3 | ORIGINAL-FABRICATION-CLASS | Live shaders still carrying the 1.8.10 anchor — copied from pre-Setup-1 drafts, the canonical wrong-version anchor instance. |
| cat2-stub-label-stale | 2 | HYBRID | Stale stub labels are technically a "wrong claim about implementation state" (fabrication-shaped) AND a discipline-drift artifact (label wasn't updated when the impl landed). Could classify either way. |

#### Per-class tally (INFERENCE)

| Class | Suppressed entries |
|---|---|
| ORIGINAL-FABRICATION-CLASS | 18 (live-shader-1810 + audit-doc-1810) |
| HYBRID | 78 (other-cat1 + cat2-stub-label-stale) |
| DISCIPLINE-DRIFT-CLASS | 911 (audit-citation + stack-b/c/d + grammar examples + toolkit-own + retro-grammar) |

**Ratio:** ORIGINAL-FABRICATION : DISCIPLINE-DRIFT ≈ 1 : 50 (by suppressed
entry count). Even allocating all HYBRID to ORIGINAL-FABRICATION, the ratio
is 1 : 9.

### F.2 — Outstanding hard-fail classification (INFERENCE)

| Hard-fail | Citation | Class | Rationale |
|---|---|---|---|
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
| docs/phase12_lattice_boltzmann.md:203 | `chapter13/cpu/LBM.cpp:97` | HYBRID-leaning-DISCIPLINE | Real upstream file, real upstream is registered, citation lacks `Krueger book-companion-code-2016` prefix. Author knew of the upstream — discipline. Not invented. |
| docs/phase12_lattice_boltzmann.md:351 | (same) | HYBRID-leaning-DISCIPLINE | Same. |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| docs/phase12_lattice_boltzmann.md:1276 | `main.cpp:1168-1279` | DISCIPLINE-DRIFT-CLASS | Author elided context. Not fabricated — under-specified. |
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
| compute_boundary_volume.comp.glsl:7 | `SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75` | DISCIPLINE-DRIFT-CLASS | Real upstream + registered; missing `SPlisHSPlasH 2.16.1` prefix. Discipline. |

**4 of 4 outstanding hard-fails are DISCIPLINE-DRIFT or HYBRID-leaning. Zero
fit ORIGINAL-FABRICATION cleanly.**

### F.3 — Spec § 12 historical baseline (FACT, verbatim)

```markdown
# docs/integrity-toolkit-spec.md  § 12  (verbatim)
## 12. Existing fabrication cases the toolkit must catch

These are the named instances of Convention #8 the toolkit must mechanically detect. If a check is added and one of these cases would *not* be caught, that's a hole to fix in v1, not a v2 deferral.

| Case | Source | Cat | Check |
|------|--------|-----|-------|
| SPlisHSPlasH 1.8.10 anchor (Setup-1) | `particle-fluids/sph-water/docs/load-bearing-decisions.md:9` and 27 other citation sites | 1 | `cat1.upstream-citation` (wrong-version on every cite using `1.8.10`; `cat1.upstream-anchor` validates vendor HEAD against registry SHA, a separate concern) |
| LeniaNDK.py citation without vendoring | `continuous-ca/lenia-fft/python/lenia_fft/presets.py:11` | 1 | `cat1.intra-repo` (path doesn't resolve locally; bare-path form falls through upstream grammar — see § 6.4 note) |
| ParticleFrame::radii silent data-loss | `common/common-cpp/src/alembic_writer.cpp:51-82` | 2 | `cat2.public-symbol-used` |
| `vdb::writeVec3Grid` unexercised real impl | `common/common-cpp/src/vdb_writer.cpp:97-145` | 2 | `cat2.public-symbol-used` |
| Stale "stub" label on alembic_writer.hpp | `common/common-cpp/include/gpusims/alembic_writer.hpp` | 2 | `cat2.stub-label-stale` |
| kernel_gradW factor-of-6 (commit 1 fix) | `particle-fluids/sph-water/src/main.cpp:1349` (pre-fix) | 3 | `cat3.cubic-kernel` (catches formula-vs-implementation drift; v1 check transcribes the GLSL kernel to a host-side C++ driver and verifies driver output against analytical expected values; direct GLSL/WGSL shader-level verification is v2 candidate per § 13) |
```

#### Per-row classification (INFERENCE)

| Row | Defect | Class |
|---|---|---|
| 1 | SPlisHSPlasH 1.8.10 anchor — invented version | ORIGINAL-FABRICATION-CLASS |
| 2 | LeniaNDK.py — invented citation path | ORIGINAL-FABRICATION-CLASS |
| 3 | `ParticleFrame::radii` silent data-loss | HYBRID (the *symbol* was declared and unused — discipline; but the *field* was load-bearing per a fabricated belief — fabrication) |
| 4 | `vdb::writeVec3Grid` unexercised real impl | HYBRID (same shape) |
| 5 | Stale stub label on `alembic_writer.hpp` | HYBRID (label contradicts impl — fabrication-shaped; but happened by drift, not invention — discipline) |
| 6 | `kernel_gradW` factor-of-6 | ORIGINAL-FABRICATION-CLASS (wrong formula was the bug) |

**Spec § 12 baseline:** 3 of 6 named canonical defects are
ORIGINAL-FABRICATION-CLASS; 3 of 6 are HYBRID. None are pure
DISCIPLINE-DRIFT-CLASS.

### F.4 — Does the retro's "shift" framing hold? (INFERENCE)

The retro § 9 claims: "the fabrication class the toolkit was originally
built to catch (wrong-version anchors, made-up paths) is no longer the
dominant defect class on `main`."

Quantitative test:

- **Suppressed pool composition (F.1):** ORIGINAL-FABRICATION 1.8%,
  HYBRID 7.8%, DISCIPLINE-DRIFT 90.5%. Clear dominance of
  DISCIPLINE-DRIFT.
- **Outstanding hard-fails (F.2):** 0% ORIGINAL-FABRICATION, 100%
  HYBRID-leaning-DISCIPLINE or DISCIPLINE-DRIFT. Strongest evidence.
- **Spec § 12 baseline (F.3):** 50% ORIGINAL-FABRICATION, 50% HYBRID.
  Pre-toolkit baseline shape.

**INFERENCE: the retro's "shift" claim holds quantitatively.** Spec § 12
named defects were ~50% pure fabrication; current state is ~2-10%
fabrication. This is a 5×-25× reduction in the fabrication share, and the
outstanding-hard-fail evidence is the strongest: zero of the four current
gate-blockers fit the original fabrication shape. The retro is not
over-claiming.

The caveat worth surfacing: the toolkit's grandfather pool includes
~625 audit-citation entries that are FROZEN DISCIPLINE-DRIFT (audit reports
written at a moment in time; refactors after). These don't represent
"current discipline gaps" — they represent "discipline gaps preserved by
the append-only audit convention." If we filtered them out, the
DISCIPLINE-DRIFT live-stream picture would look very different: ~290
live discipline-drift entries (other-cat1 + stack-{b,c,d}-unused + active
grammar-leak channels), of which ~210 are public-symbol-used pre-wiring.
The "shift" is real, but the load-bearing measure for batch-2 prioritization
should be **live-stream discipline drift** (active leaks) rather than
**total suppressed** (which is inflated by audit-doc append-only state).

## Section G — A.3 leverage assessment

### G.1 — Bare-path enumeration (FACT)

Scope: all `.cpp/.hpp/.h/.py/.ts/.glsl/.wgsl/.cc/.cxx/.c/.md` files under
`docs/`, `common/`, `particle-fluids/`, `continuous-ca/`, `agent-based/`,
`closed-form/`, `hybrid-particle-grid/`, `volumetric-grid/`, `quantum/`,
`tools/integrity/docs/`, and `project-state.md`. Regex:
`(?:^|[^/\w])([A-Za-z_][A-Za-z_0-9\-]*\.(?:cpp|hpp|h|py|ts|glsl|wgsl|cc|cxx|c)):([0-9]+)(?:-([0-9]+))?`.

Total **bare-path-like matches: 924**.

### G.2 — Classification (FACT counts, INFERENCE classification)

Index built from `references/` for upstream basenames and from the rest
of the repo for intra-repo basenames (excluding `references/`).

| Class | Count | Definition |
|---|---|---|
| REGISTERED-UPSTREAM-BARE | 164 | Basename matches a file in `references/`, not in repo-non-references. |
| INTRA-REPO-BARE | 290 | Basename matches exactly one file in repo-non-references. |
| AMBIGUOUS | 226 | Basename matches multiple intra-repo files (e.g. `main.cpp` everywhere). |
| UNRESOLVABLE | 244 | Basename matches nothing tracked (e.g. invented names, regex-noise from line-wrapping like "nTimeStepDFSPH.cpp" where `\n` swallowed into the match). |

Samples:

**REGISTERED-UPSTREAM-BARE** (5):

```
docs/integrity-toolkit-spec.md:612 | SPHKernels.h | ground_truth = "SPlisHSPlasH 2.16.1 SPHKernels.h:14-91"
docs/diagnostics/_audits/phase11_5_commit2_verification_2026-05-14.md:93 | TimeStepDFSPH.cpp | `TimeStepDFSPH.cpp:1306-1307`:
docs/diagnostics/_audits/phase11_5_commit2_verification_2026-05-14.md:107 | TimeStepDFSPH.cpp | `TimeStepDFSPH.cpp:1383-1391`:
docs/diagnostics/_audits/phase11_5_probe3_2026-05-14_architect1.md:8 | TimeStepDFSPH.cpp | scope: scalar DFSPH reference functions; AVX twins (TimeStepDFSPH.cpp:735-1103) intentionally skipped per architect-1 instruction
docs/diagnostics/_audits/phase11_5_probe3_2026-05-14_architect1.md:305 | TimeStepDFSPH.cpp | The pressure-solve outer loop at `TimeStepDFSPH.cpp:324-341` references the following member variables and constants. Declaration sites listed:
```

**INTRA-REPO-BARE** (5):

```
docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md:97 | alembic_writer.hpp | (`alembic_writer.hpp:11` and `vdb_writer.hpp:12`, probe § G.1, G.2).
docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md:97 | vdb_writer.hpp | (...).
docs/diagnostics/_audits/commoncpp_consumers_2026-05-14_architect2.md:231 | vdb_writer.hpp | - `vdb_writer.hpp:33` — `writeVec3Grid`: ...
docs/diagnostics/_audits/commoncpp_consumers_2026-05-14_architect2.md:233 | vdb_writer.hpp | - `vdb_writer.hpp:41` — `writeFloatFrame` ...
docs/diagnostics/_audits/integrity_build_6_landing_2026-05-14.md:110 | vdb_writer.hpp | | `vdb::writeVec3Grid` unexercised real impl | HIT — `vdb_writer.hpp:33`, "no non-self consumer site" |
```

**AMBIGUOUS** (5):

```
docs/phase12_lattice_boltzmann.md:1153 | main.cpp | // Mirrors ES's pattern from main.cpp:1100-1158.
docs/phase12_lattice_boltzmann.md:1276 | main.cpp | Following ES's pattern from main.cpp:1168-1279, ...
docs/diagnostics/_audits/sims_prioritization_2026-05-14_triage.md:103 | main.cpp | - **Stack C sims (GLSL):** ...
docs/diagnostics/_audits/phase11_5_commit1_landing_2026-05-14.md:23 | main.cpp | the lambda body at `main.cpp:1349`. ...
docs/diagnostics/_audits/phase11_5_commit2a_landing_2026-05-14.md:332 | main.cpp | pattern already in `main.cpp:666-674`.
```

**UNRESOLVABLE** (5):

```
docs/integrity-toolkit-spec.md:441 | nTimeStepDFSPH.cpp | (regex noise — `\nTimeStepDFSPH.cpp`)
docs/integrity-toolkit-spec.md:443 | file.cpp | spec example: `[file.cpp:42]`
docs/integrity-toolkit-spec.md:488 | LeniaNDK.py | spec discussion of the canonical bare-path defect
docs/integrity-toolkit-spec.md:853 | LeniaNDK.py | spec § 13 v2-candidate paragraph
docs/diagnostics/_audits/phase11_5_commit2_verification_2026-05-14.md:17 | comp.glsl | regex noise — captured trailing `.comp.glsl` only
```

### G.3 — A.3 leverage estimate (INFERENCE)

If A.3 implements "bare basename → registered upstream basename"
auto-detection:

- **164 REGISTERED-UPSTREAM-BARE matches** would be candidate hits.
- Most live in audit-doc paths (under `docs/diagnostics/_audits/`) — those
  are append-only by convention; A.3 would HARD_FAIL them at run time
  unless explicitly excluded. Most would be grandfathered into the new
  `bare-upstream-citation` category.
- A small subset live in live-source paths (per § A.2, 3 of 4 outstanding
  hard-fails are exactly this shape — 2× `chapter13/cpu/LBM.cpp`, 1×
  `SPlisHSPlasH/BoundaryModel_Akinci2012.cpp`). A.3 would auto-rewrite
  these to the registered form OR hard-fail at write-time.

If A.3 also covers INTRA-REPO-BARE (basename matches exactly one intra-repo
file): another **290 candidate hits**. This is a substantive scope
expansion — many of these are audit-doc citations of intra-repo files that
moved, and would need careful grandfather treatment.

**INFERENCE — A.3 effective leverage:**

- **Live-source hard-fail prevention:** A.3 catches 3 of 4 current
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  outstanding hard-fails directly. The 4th (`main.cpp:1168-1279`) requires
  a more elaborate disambiguation pass.
- **Backlog cost:** 164 upstream-bare + 290 intra-repo-bare = 454 matches
  would need grandfather treatment if A.3 lands without exclusion of
  audit-doc paths. Probably ~80% of these would route to `audit-citation`
  via the existing classifier rule (which fires on `docs/diagnostics/_audits/`).
- **Edge-case risk:** `AMBIGUOUS` (226) requires policy: HARD_FAIL by
  default vs SOFT_WARN vs IGNORE. The retro doesn't bank a position. Worth
  resolving before A.3 implementation.

### G.4 — Edge cases A.3 would NOT catch (INFERENCE)

- **Line-range-only citations without colons.** Rare.
- **Line-spanning citations** where the path is wrapped onto a separate
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  line (`SPlisHSPlasH\nBoundaryModel.cpp:42`). Already a v2 deferral per
  spec § 13 ("multi-line citation grammar").
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **Citations to deleted files.** Bare path `gone.cpp:42` where no
  matching basename exists. Would fall into UNRESOLVABLE (currently 244
  matches). A.3 cannot rescue these without per-basename historical
  knowledge.
- **Citations to versioned upstreams not in the registry.** If a citation
  uses a bare path to a basename that *would* match an upstream if the
  upstream were registered (Chakazul Lenia is the existing case per
  spec § 12 row 2 + `ground-truth-sources.md`'s "Not yet registered"
  section): A.3 cannot auto-rewrite (no registry entry to rewrite to)
  and would HARD_FAIL. This is closer to spec § 12 row 2 territory
  (cat1.unregistered-upstream) and may be the right behavior.
- **Citations where the basename includes a directory-prefix that matches
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
  a non-existent path** (e.g., `Foo/bar.cpp:42` where neither `Foo/`
  nor `bar.cpp` resolves intra-repo or upstream). Falls into UNRESOLVABLE.

## Section H — Probe-template gap analysis

### H.1 — Pre-spec probes exist

```
$ wc -l docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md
(file present)
$ wc -l docs/diagnostics/_audits/integrity_v1_1_apispec_2026-05-15_architect1.md
(file present)
```

Both probes were dumped first-100-lines for the gap analysis.

### H.2 — Retro § 7.2 C+D probe-gap claims

#### § 7.2 C: "verbatim probe items enumerating 3-5 representative header→impl path pairs"

Grep results:

```
$ grep -n -E 'header.{0,5}impl|sibling-impl|path pair|path mirror|impl path|resolve.{0,10}impl' docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md
(no matches)

$ grep -n -E 'header.{0,5}impl|sibling-impl|path pair|path mirror|impl path|resolve.{0,10}impl' docs/diagnostics/_audits/integrity_v1_1_apispec_2026-05-15_architect1.md
1305:    """Return the per-stack public/impl path map rooted at `root`."""
```

Only one match in the apispec probe, and it's a docstring extract from
`stack_paths.py`, not a probe item enumerating header→impl path pairs.

**CONFIRMED-GAP:** Neither pre-spec probe enumerated representative
header→impl path pairs from the synced repo. The retro § 3.1 root-cause
claim ("the spec author filled in the convention from prior assumption
rather than from probe data") is supported. The retro § 7.2 C banked
mitigation is valid.

#### § 7.2 D: "verbatim probe items enumerating all modules that depend on the affected behavior"

Grep results:

```
$ grep -n -E 'cat1.*markdown|markdown.*cat1|scan.*\.md|markdown.*scan|extensions' docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md
(no matches)

$ grep -n -E 'cat1.*markdown|markdown.*cat1|scan.*\.md|markdown.*scan|extensions' docs/diagnostics/_audits/integrity_v1_1_apispec_2026-05-15_architect1.md
1178:# Scan extensions for files that can carry annotations.
```

One match in the apispec probe — a comment in dumped source. Not a probe
item enumerating which cat1 checks scan markdown.

**CONFIRMED-GAP:** Neither probe enumerated all cat1 check modules that
scan markdown content line-by-line. The retro § 3.2 root-cause claim is
supported. The retro § 7.2 D banked mitigation is valid.

### H.3 — Are the retro's root-cause claims fair? (INFERENCE)

Both probe-template gaps are real and probe-checkable.

The retro framing ("probe-template gap, not architect-1 discipline gap")
deserves a sharper take. A subjective reading: the probes were strong on
inventory (file trees, LOC, fixture inventory) but weak on **conventions
and dependencies**. The probe template tested *"what exists?"* but not
*"how does it relate to what already exists?"* — the two questions that the
two fabrications required.

INFERENCE: the retro is honestly self-critical here (and § 0 explicitly
flags author-bias). The probe gaps are real; the retro is not
under-attributing to architect-1 discipline. Both root-cause framings
could be true simultaneously: the probe gaps + the architect-1 habit of
filling in conventions from prior assumption are mutually reinforcing.
Mitigating either independently would have caught the issue. The
retro proposes a probe-template fix, which is the cheapest of the two
mitigations.

## Section I — Convention enforcement signal

### I.1 — Recent commit activity (FACT)

Computed via `git log --since='30 days ago' --name-only`:

- Total commits past 30 days: **139**
- Cat1-scannable-touching commits: **129** (92.8%)
- Commits with `grandfather` in subject: **3** (plus a few sweep companions)

### I.2 — Companion-pair rate (FACT, INFERENCE)

For each cat1-scannable-touching commit that is **not** itself a
grandfather-sweep commit, look ahead 1-2 commits for a `grandfather`-prefixed
companion:

- Non-grandfather cat1-touching commits: **126**
- Properly paired (followed within 2 commits by a grandfather companion): **4**
- Un-paired: **122**

**Un-paired rate: 96.8%.**

Per retro § 6.2 thresholds:

- <10% → soft (documentation)
- 10-40% → medium (CI check)
- >40% → hard (pre-commit hook)

### I.3 — Inferred enforcement level (INFERENCE, with strong caveat)

Mechanical reading: **96.8% > 40% → hard (pre-commit hook).**

**Caveat that changes the conclusion:** The retro's convention reads
"every commit that touches the cat1-scannable surface either runs the
sweep or lands a companion." But not every cat1-scannable-touching commit
*introduces findings*. Many touch files in cat1-scannable trees without
adding citations or grammar literals (e.g., `c1a257d` edits a shader and an
audit doc but doesn't change citation count — per § A.4, no new
unsuppressed findings landed in either of the two commits since `d772803`).

The 96.8% un-paired rate over-states the actual gap. A more accurate signal
is: of cat1-touching commits, how many **introduced new live findings**
that should have been swept? That measurement requires running the toolkit
at each historical SHA and diffing finding sets — not done in this probe.
The post-batch triage that motivated the retro identified 30 findings
across 4 sessions in one batch, suggesting the **introduction rate** is
modest (single-digit findings per session-day, most absorbed by existing
classifier rules).

**INFERENCE — re-stated:** Hard (pre-commit hook) is over-engineered for
the actual signal. **Medium (CI check that fails when live-source
finding count grows without a paired sweep commit)** is the right level:
- catches the real signal (live-source delta) without triggering on
  no-finding cat1-touches;
- modest implementation cost (~50 LOC CI script + a state file);
- matches the operating evidence (one author, modest finding-introduction
  rate, but enough triage churn to motivate the retro).

Soft (documentation) is insufficient — the retro itself documents that
"authors are expected to remember consistently" failed across 4 concurrent
sessions in batch 1.

## Section J — Test-suite growth velocity

### J.1 — Test LOC across landing SHAs (FACT)

```
SHA       test-files  total-LOC-tests
af248cf   12          868
f661ec4   16          1045
dbac051   17          1096
a71594a   17          1096
a28e1d7   17          1119
78e18d6   17          1119
bcba679   17          1119
f541557   17          1119
a42085a   17          1120
d772803   17          1120
c1a257d   17          1120
cdad2e2   17          1120
```

**FACT:** Test files grew by 5 across batch 1 (12 → 17). LOC grew by 252
(868 → 1120). Per the retro figure (74 → 96 tests), test count rose by 22
across the same SHA range — well-modeled by ~4-5 new tests per new test
file plus expansion within `test_cat1_annotation_fence.py` and
`test_snapshot.py`.

### J.2 — Maintenance-burden estimate (INFERENCE)

Current pytest wall-clock: **2:09** (129s) for 96 tests. Average ~1.3s per
test. The slow tests are the ones that spawn subprocess-mode integrity
runs (snapshot, grandfather_sweep, real-repo integration). Most pure-unit
tests run sub-millisecond.

Linear extrapolation:

- At 200 tests (double current): ~4 min wall-clock. Still well within
  retro § 1's "wall-clock CI: unchanged (~5 min)" budget.
- At 500 tests (5×): ~10 min. Past the comfort threshold for in-loop CI;
  begins to be a maintenance concern.

**INFERENCE:** Test suite hits maintenance-concern threshold around
**400-500 tests / 8-10 min wall-clock**. At the v1.1 batch-1 cadence
(+22 tests per batch), that's 18-19 more batches before hitting that
threshold. Plenty of runway. The retro § 8 question 4 is answerable now:
*not a concern for v1.2 or v1.3; revisit at v1.5 or batch 8, whichever
comes first.*

## Section K — Operating-condition evidence

### K.1 — Concurrent-commit frequency (FACT)

```
Total commits past 30 days: 139
Adjacent pairs within 1hr: 113
Percentage: 81.9%
```

**FACT:** 81.9% of consecutive commits land within an hour of each other.

### K.2 — Author distribution (FACT)

```
139  Steven Cohen
```

**FACT:** 100% of commits past 30 days are by a single human author.

### K.3 — Operating-condition framing (INFERENCE)

The retro § 7.1 calls the operating condition "concurrent multi-agent
landing." Strictly speaking, the data shows **single-human, multi-session
concurrent landing** — one human operating multiple Claude Code sessions in
parallel against the same `main`. The "multi-agent" framing refers to
the *Claude Code* agents being multiple, not the humans.

This is a meaningful but tractable framing distinction:

- The collision pattern (4 concurrent sessions, one human) is what the
  retro § 3.3 actually describes.
- The framing "multi-agent landing" is technically accurate (multiple AI
  agents committing on behalf of one human).
- The framing implies a different operating fingerprint than "multiple
  humans on parallel branches" — the latter would show distinct
  authorship in `git log`, the former does not.

**INFERENCE:** The retro § 7.3 "operating fingerprint as a positive
signal" claim is supportable, but the fingerprint to monitor is
**single-author + many-within-1hr commits** specifically. A future shift
to multi-human concurrent landing would change the operating shape and
the convention recommendations.

## Section L — Surface scope check

### L.1 — Retro claims not covered by sections A-K

Re-reading the retro for mechanically-checkable claims:

- **§ 2.3 "history file currently 1 entry":** Refuted in § C.5 (actual: 0
  entries). Already surfaced.
- **§ 2.3 "appends per run unless `--no-history-append`":** Code at
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
  `snapshot.py:178-181` does append on every run when invoked without
  `--no-history-append`. But the file is git-tracked, so appends
  remain uncommitted until an author commits them. **INFERENCE:** the
  mechanism works; the empty-array state reflects "no one has run
  `--grandfather-report` *and committed the result* since `dbac051`
  landed." Worth documenting: history appends are uncommitted side-effects.
- **§ 2.5 "13 sites swept across `tools/integrity/README.md`,
  `docs/integrity-toolkit-spec.md`, and `tools/integrity/integrity/__main__.py`":**
  Not directly verified in this probe (the 5.B sweep commit `a28e1d7`
  would need a specific diff inspection). Recommend follow-up if the
  exact-count claim is load-bearing.
- **§ 4.1 "Every pause-and-surface produced a correct resolution within
  minutes":** Subjective; not mechanically checkable.
- **§ 5.4 "24 of the 30 baseline hard-fails were audit-doc and
  toolkit-doc findings":** Cross-references the post-batch triage report;
  not directly recomputed in this probe.

### L.2 — Toolkit behavior not discussed in retro

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **CLI flag `--no-history-append`** is documented in `runner.py:63-64`
  but appears only briefly in retro § 2.3. Coverage is adequate.
- **`--state-snapshot` flag** is discussed in retro § 2.3; coverage
  adequate.
- **Cat 3 d3q19 verification machinery** (`d3q19_verify.py`, 281 LOC;
  `d3q19_equilibrium.expected.json`, 290 LOC; `algebraic/d3q19.md` and
  the `[Algebraic_D3Q19]` registry entry) **lands inside the v1.1
  batch-1 diff window but is not mentioned in the retro at all**. The
  retro scopes to "named v1.1 spec items (A.1, A.5, A.7, A.8, 5.B)" but
  this work clearly co-landed in the same SHA range (per § E.1 diff
  stat). The classifier (`grandfather.py`) doesn't have d3q19 rules
  either. **INFERENCE — surface scope gap:** the retro under-reports
  what landed in the batch window. Recommend banking d3q19 as a
  "co-landed concurrent setup item, not v1.1 batch 1" footnote in any
  next-batch inventory.
- **`cat3.d3q19-*` checks referenced in `ground-truth-sources.md`
  (`[Algebraic_D3Q19]` `used_by_checks`):** registry declares
  `cat3.d3q19-velocity-set`, `cat3.d3q19-weights`,
  `cat3.d3q19-equilibrium`. **None of these are registered in
  `cat3_numerical/checks/__init__.py`** — that file registers only
  `cat3.cubic-kernel`. So the registry-declared `used_by_checks` for
  Algebraic_D3Q19 names three checks that don't yet exist as registered
  toolkit checks. The verification harness (`d3q19_verify.py`) exists,
  but is not exposed via the toolkit's check-discovery surface.
  **INFERENCE — surface scope gap:** registry-declared checks are not
  yet registered. This is a real defect (registry declares ground-truth
  to be consumed by checks that don't run). Worth banking for batch 2.
- **Strict-mode human-renderer suppressed-stanza bug** (§ A.1 INFERENCE).
  Not discussed anywhere in the retro. Real defect.

### L.3 — Audit-prose freshness convention (FACT)

```
$ grep -rn -E 'audit-prose freshness|freshness check|disk before commit' docs/ tools/integrity/docs/
docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md:249:    F. Audit-prose freshness check. ...
docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md:314:6. **Audit-prose freshness check** (new v1.2 candidate banked in
docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md:325:(retro-doc-snapshot in item 4, audit-prose freshness in item 6).
```

**FACT:** The audit-prose freshness convention does NOT pre-exist in the
broader docs corpus; the only mentions are in the post-retro landing audit
itself, which banks it as a v1.2 candidate. The retro's framing of it as
a "new" convention is accurate.

## Summary

**Gate state (FACT):** 4 unsuppressed hard-fails, identical to the
post-retro audit's expected state (3 Phase 12 LBM + 1 sph-water Akinci2012).
Two new commits since `d772803` (`c1a257d`, `cdad2e2`) introduce zero new
unsuppressed findings.

**Retro claims tested:**

| Claim | Status |
|---|---|
| § 1 +6 modules new | REFUTED (3 new Python + 1 JSON; 11 modified) |
| § 1 ~750+200 LOC | REFUTED (1157 insertions per diff-stat; ~590 if excluding co-landed d3q19) |
| § 1 74→96 tests | CONFIRMED |
| § 1 1126 baseline → 967 → 1007 | partial-CONFIRMED (current 1007 matches; earlier baselines not re-run) |
| § 2.1 stub-label-stale catches alembic+vdb | CONFIRMED |
| § 2.2 A.5 fence-skip extended to intra-repo + upstream-citation | CONFIRMED |
| § 2.3 history file currently 1 entry | REFUTED (file is `[]`) |
| § 3.4 own-source findings annotated in a42085a | CONFIRMED |
| § 5.1 A.3 catches 4 of 6 outstanding | partial-CONFIRMED (catches 3 of 4 post-retro; row 27 is ambiguous-basename) |
| § 6.1 A.3, A.2, classifier rules deferred | CONFIRMED (all deferred, not silently landed) |
| § 9 fabrication-class shift | CONFIRMED quantitatively (~50% → ~2-10% fabrication share; 0/4 outstanding are pure fabrication) |

**New scope surfaced (not in retro):**

1. Strict-mode human-renderer emits HARD_FAIL stanzas for suppressed
   findings; summary line and stanza list are mutually inconsistent
   (§ A.1).
2. d3q19 verification machinery (290 LOC JSON + 281 LOC Python module)
   landed in the v1.1 batch-1 SHA range but is unmentioned in the retro
   (§ L.2).
3. Registry declares three `cat3.d3q19-*` checks as consumers of
   `[Algebraic_D3Q19]`, but none are registered in
   `cat3_numerical/checks/__init__.py`. Registry/check-registration
   mismatch (§ L.2).
4. Grandfather history file is empty, not seeded as the retro claims
   (§ C.5).
5. Catalog tally heading drift is +63 entries (+6.7%) over the post-retro
   window — empirical support for the retro § 5.5 prediction
   (§ C.4).

**Batch-2 prioritization recommendations (INFERENCE):**

1. **A.3 (bare-path-to-upstream-basename)** — confirmed highest leverage
   on outstanding hard-fails (3 of 4); 164 registered-upstream-bare
   candidates in the broader corpus; would need a policy for ambiguous
   basenames (226 candidates). Implementation cost likely modest (extend
   `cat1_citations/grammar.py` extractor + reuse registry).
2. **A.2 (toolkit self-application)** — confirmed deferred; would have
   caught the `test_suppression_fence.py` case. Higher complexity
   (recursive scanning).
3. **Grandfather-sweep medium-enforcement (CI check, not pre-commit
   hook)** — the un-paired rate of 96.8% overstates the actual finding-
   introduction rate; hard (pre-commit hook) is over-engineered. Medium
   is right.
4. **Register the three `cat3.d3q19-*` checks** — fix the registry/check
   mismatch surfaced in § L.2. Modest scope.
5. **Strict-mode human-renderer fix** — emit HARD_FAIL stanzas only for
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
   unsuppressed findings (§ A.1). One-line fix in `runner.py:141-145`.
6. **History file seeding** — either remove the "seed" framing from the
   retro/docs, OR have `dbac051`-equivalent commit actually seed one
   real entry. Either resolution is cheap; mismatch should not persist.

**Read-only constraint honored.** No files modified; no commits made; no
pushes.

## End of probe
