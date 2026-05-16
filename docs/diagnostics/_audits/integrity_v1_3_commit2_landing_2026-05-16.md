---
title: "Integrity v1.3 Commit 2 — T1.5 Cat3 Expected-Values TOML → JSON"
date: 2026-05-16
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_t1_3_5_probe_2026-05-16_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_t1_3_5_spec_2026-05-16_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_commit1_landing_2026-05-16.md
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md
  - docs/retro/integrity-toolkit-v1.3-candidates.md
---

# Integrity v1.3 Commit 2 — T1.5 Cat3 Expected-Values TOML → JSON

## § A. Change summary

T1.5 converges the cat3 expected-values format to JSON per probe § C.4
/ roadmap § 4 T1.5. Originating observation: v1.2 bolt-ons probe § C.4
noted the asymmetry — `d3q19_equilibrium.expected.json` (already JSON,
schema_version 1, derivation/anchor_sha keys) was the precedent;
`expected_values.toml` (cubic-kernel) was the outlier. T1.5 removes
the outlier.

The harness's `load_expected_values()` in `cubic_kernel.py` swaps
`tomllib.loads` for `json.loads`. Function signature, return type, and
TestPoint construction are unchanged — the format swap is internal to
this function. The generator (`generate_expected.py`) emits JSON via
`json.dumps`. The four TOML header comments (anchor provenance,
generator pointer, upstream-source pointer, anchor SHA) map to
top-level keys (`source`, `derivation`, `anchor_sha`).

A new doc at `tools/integrity/docs/cat3-conventions.md` records the
JSON-as-canonical-format convention and lists existing cat3
expected-values files. The doc is positioned as a growth path for
future cat3 conventions.

## § B. File inventory

| Status | Path | Diff |
|---|---|---|
| Created | `tools/integrity/integrity/cat3_numerical/expected_values.json` | +37 LOC |
| Created | `tools/integrity/docs/cat3-conventions.md` | +71 LOC |
| Created | `docs/diagnostics/_audits/integrity_v1_3_commit2_landing_2026-05-16.md` | this report |
| Removed | `tools/integrity/integrity/cat3_numerical/expected_values.toml` | −45 LOC |
| Modified | `tools/integrity/integrity/cat3_numerical/cubic_kernel.py` | docstring (.toml → .json) / `tomllib` → `json` / EXPECTED_VALUES_RELATIVE / `load_expected_values` body (−1/+5 net) |
| Modified | `tools/integrity/integrity/cat3_numerical/generate_expected.py` | docstring / OUTPUT_PATH / `main()` body fully rewritten (TOML literal-text → `json.dumps`) |
| Modified | `tools/integrity/tests/test_cat3_cubic_kernel.py` | docstring update only (`expected_values.toml` → `expected_values.json`) |
| Modified | `tools/integrity/docs/grandfather-catalog.md` | catalog refresh post-sweep (audit-citation +1, audit-report-grammar-example +1, audit-bare-path +10) |

Per Decision 6 (probe § D.8): `git rm` + `git add` (not `git mv`),
since content-type change makes rename detection misleading.

## § C. Verification

```
$ cd tools/integrity && python3 -m pytest tests/test_cat3_cubic_kernel.py -v
============================== 9 passed in 25.63s ==============================
```

All 9 cat3 tests pass with the JSON file in place (no test code
changes other than docstring text; the format swap is internal to
`load_expected_values`).

Generator round-trip byte-for-byte:

```
$ python3 tools/integrity/integrity/cat3_numerical/generate_expected.py
Wrote .../tools/integrity/integrity/cat3_numerical/expected_values.json
$ git diff --quiet tools/integrity/integrity/cat3_numerical/expected_values.json
$ echo $?
0
```

No `.toml` references in toolkit code/tests proper:

```
$ grep -rn "expected_values\.toml" tools/integrity/
tools/integrity/docs/cat3-conventions.md:70:The v1 TOML format (`expected_values.toml`) was removed in v1.3 batch-1
```

The single remaining match is the new cat3-conventions doc's
historical note documenting the removal. No matches in code/tests.

Cat3 check still runs (graceful-degrade unchanged when driver absent):

```
$ python3 -m integrity --check cat3.cubic-kernel --no-audit-log
integrity: 1 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
```

Gate state pre-sweep / post-sweep:

```
pre-sweep:  integrity: 5 pass, 0 soft-warn, 56 hard-fail, 1248 suppressed
post-sweep: integrity: 5 pass, 0 soft-warn, 44 hard-fail, 1260 suppressed
```

Pre-sweep hard-fail count rose by 12 due to audit-doc bare-path
findings in this commit's audit report and the v1.3 spec/probe siblings
landed in commit 1 (the probe doc carried 10 audit-bare-path findings
that were not all swept in commit 1's sweep — banked observation #2).
Post-sweep returns to 44, matching commit 1's post-sweep baseline.

## § D. Design decisions applied

**Decision 6 — File-operation sequence (per probe § D.8):** `git rm` +
`git add` rather than `git mv`. Rationale: content-type change has
near-zero practical value from rename detection; explicit delete+add
is cleaner in review.

**Decision 7 — JSON schema (per probe § D.8 + § D.5):** Top-level
shape mirrors d3q19's schema. Keys: `schema_version`, `source`,
`derivation`, `anchor_sha`, `tolerance`, `test_points`. The four TOML
header comments map to top-level keys.

## § E. Banked observations

1. **Spec § 4.2.1 numeric literals diverged from generator output.**
   The spec's § 4.2.1 had 15-significant-figure float literals
   transcribed via `{:.15g}` from the old TOML emission (e.g.
   `2.54647908947033`) and integer `0` literals at q=0 / q=1 for
   `expected_W` / `expected_gradW_magnitude`. Python's `json.dumps`
   emits floats via Python's `repr` (shortest round-trip-safe form,
   often 17 digits) and emits `0.0`, not `0`, for `float` values
   returned by `cubic_W` / `cubic_gradW_magnitude`. The execution
   prompt's friction-point #1 anticipated this exact case.

   **Resolution applied:** generator output is authoritative. The
   committed `expected_values.json` matches the generator's natural
   output (full Python `repr` precision, `0.0` literals). Mathematically
   the values are equivalent within float precision; the generator
   round-trip check passes byte-for-byte. The spec's § 4.2.1 literals
   are now an artifact of the spec's drafting, not a constraint.

   Recorded here per Convention F (audit-prose freshness) rather than
   silently editing § 4.2.1.

2. **Pre-sweep gate spike attributable to incomplete commit-1 sweep.**
   This commit's pre-sweep gate was 56 hard-fail (vs 44 post-commit-1).
   The +12 delta breaks down as: 10 cat1.bare-path findings in the
   commit-1-landed spec/probe sibling docs (`integrity_v1_3_t1_3_5_probe`,
   `integrity_v1_3_t1_3_5_spec`); 1 cat1.intra-repo finding; 1
   cat1.annotation-form finding. The commit-1 inline sweep companion
   only added 1 annotation because it ran before the spec/probe
   siblings were staged. The commit-2 sweep companion picked these
   up; gate returned to baseline.

   This is a Convention B execution-time observation worth banking:
   the inline sweep companion runs against the staged set, so files
   added to the same commit but not yet sweep-anchored are sometimes
   left for the next commit's sweep companion.

## § F. Cross-references

- Probe § D.1 — TOML file structure (six test points, tolerance block,
  header comments).
- Probe § D.2 — `cubic_kernel.py` touch points (module docstring,
  `tomllib` import, `EXPECTED_VALUES_RELATIVE`, `load_expected_values`).
- Probe § D.4 — `generate_expected.py` main() emission logic.
- Probe § D.5 — d3q19 schema as precedent.
- Probe § D.6 — Test docstring references to TOML filename.
- Probe § D.7 — Combined `cubic_kernel.py` + `generate_expected.py` touch points.
- Probe § D.8 — File-operation sequence + JSON-schema-shape decisions.
- Probe § D.9–§ D.10 — Doc-home decision (new file vs adding to ground-truth-sources.md).
- Bolt-ons probe § C.4 — Originating observation.

## § G. Next commit

Commit 3 — T1.4 probe template conventions doc. SHA cross-reference
will be filled in by commit 4 (SHA back-fill): `9e3afa9`.
