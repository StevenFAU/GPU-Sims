---
title: "Integrity v1.2 Commit 4 — P1.7 stub_label_stale.py Module-Docstring Drift"
date: 2026-05-15
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_spec_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_commit1_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_2_commit2_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_2_commit3_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_commit1_landing_2026-05-15.md
---

# Integrity v1.2 Commit 4 — P1.7 stub_label_stale.py Module-Docstring Drift

## § A. Change summary

Cosmetic fix to the module docstring of
`tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py`.
The original module docstring (lines 15-18) described the
sibling-impl resolution as "`common-cpp/include/<sub>/<base>.hpp` ->
`common-cpp/src/<sub>/<base>.cpp` (relative path mirror)" — matching
the v1.1 batch-1 *spec* but not the actual code. The in-function
docstring at `_resolve_impl_path` (lines 100-107) already correctly
describes the namespace-strip convention adopted during
pause-and-surface #1 in `integrity_v1_1_commit1_landing_2026-05-15.md`
§ E.1. This commit aligns the module docstring with the working code.

No behavior change. No test changes. The fix is documentation-only.

## § B. File inventory

- `tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py` —
  modified lines 15-18 (replaced with 6 lines describing the
  namespace-strip convention).
- `docs/diagnostics/_audits/integrity_v1_2_commit4_landing_2026-05-15.md` —
  this landing report.

## § C. Verification

### C.1 Module imports cleanly

```
$ python3 -c "from integrity.cat2_contracts.checks import stub_label_stale; print('ok')"
ok
```

### C.2 Existing tests still pass

```
$ python3 -m pytest tests/test_cat2_stub_label_stale.py -v
6 passed in 0.02s
```

### C.3 Docstring/code consistency

```
$ grep -nA 5 "Sibling-impl resolution" tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py
15:Sibling-impl resolution (corrected post-batch-1 per commit-1 landing audit section E.1):
16-  - `.hpp`/`.h` in `common-cpp/include/<namespace>/<rest>.hpp` ->
17-    `common-cpp/src/<rest>.cpp` (first directory component after
18-    `include/` is the project namespace and is stripped)
19-  - `.py`: impl is the same file (Python does not separate
20-    declaration from implementation)
```

Both occurrences of the convention (module docstring at line 15,
`_resolve_impl_path` docstring at lines 100-107) now describe the
namespace-strip convention. No occurrence of the old "relative path
mirror" phrasing remains in the file.

## § D. Cross-references

- Probe § E.1 — docstring/code discrepancy verbatim.
- Probe § E.3 — replacement text.
- Probe § E.4 — confirms in-function docstring already describes
  the namespace-strip convention.
- `integrity_v1_1_commit1_landing_2026-05-15.md` § E.1 — original
  pause-and-surface #1 record that adopted the namespace-strip
  convention.

## § E. Banked observations

- None. Cosmetic one-paragraph fix.

## § F. Next commit

Commit 5 (SHA back-fill) — replace `<COMMIT_N_SHA>` placeholders
across the four commit-landing audit reports with actual SHAs.
