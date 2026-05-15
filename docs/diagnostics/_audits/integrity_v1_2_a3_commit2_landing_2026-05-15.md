---
title: "Integrity v1.2 A.3 — Commit 2 landing audit"
date: 2026-05-15
author: claude-code (executor)
status: complete
sibling-docs:
  - /home/otacon/Downloads/integrity_v1_2_a3_spec.md
  - docs/diagnostics/_audits/integrity_v1_2_a3_commit1_landing_2026-05-15.md
---

# Integrity v1.2 A.3 — Commit 2 landing audit

## A. Change summary

Commit 2 lands the five `cat1.bare-path` classifier rules in
`grandfather.py` and adds five corresponding category sections to
`grandfather-catalog.md`. Both additions are append-only with respect
to existing rules; classifier order preserves "first match wins" so
existing categories are unaffected.

The check is still not registered (registration is commit 3), so these
rules are dormant: there are no live `cat1.bare-path` findings to
classify yet.

## B. File inventory

Modified files:

- `tools/integrity/integrity/grandfather.py` — five new classifier
  rules appended before the final fall-through `return
  Classification(category="other-cat1", ...)`. +39 LOC.
- `tools/integrity/docs/grandfather-catalog.md` — five new category
  sections (`audit-bare-path`, `retro-bare-path`,
  `toolkit-doc-bare-path`, `deferred-upstream-bare-path`,
  `other-cat1-bare-path`) appended before the "Suppression-annotation
  discipline" section. Counts placeholder-stamped `(?)` pending
  commit 4 sweep. +69 LOC.

New files:

- `docs/diagnostics/_audits/integrity_v1_2_a3_commit2_landing_2026-05-15.md`
  (this file).

## C. Verification block

`pytest tools/integrity/tests/ -q` → 119 passed (unchanged from
commit 1).

Routing verification (per spec § 5.3 step 3):

```
python3 -c "from integrity.grandfather import classify, Finding; ..."
```

| Finding shape | Expected category | Observed |
|---|---|---|
| `cat1.bare-path` @ `docs/diagnostics/_audits/test.md` | `audit-bare-path` | OK |
| `cat1.bare-path` @ `docs/retro/test.md` | `retro-bare-path` | OK |
| `cat1.bare-path` @ `docs/integrity-toolkit-spec.md` | `toolkit-doc-bare-path` | OK |
| `cat1.bare-path` @ `tools/integrity/docs/foo.md` | `toolkit-doc-bare-path` | OK |
| `cat1.bare-path` @ `tools/integrity/README.md` | `toolkit-doc-bare-path` | OK |
| `cat1.bare-path` w/ message containing "LeniaNDK" + "Chakazul" | `deferred-upstream-bare-path` | OK |
| `cat1.bare-path` @ `common/foo.cpp` (fall-through) | `other-cat1-bare-path` | OK |

`python3 -m integrity --mode strict --no-audit-log` → exit 1, gate
unchanged (5 hard-fails, 1046 suppressed); the new rules are dormant.

## D. Behavioral notes

The rule order is deliberately specific-to-general so that
`audit-bare-path` and `retro-bare-path` (which are path-prefix matches)
take precedence over `other-cat1-bare-path` (the fall-through). The
`deferred-upstream-bare-path` rule is positioned after the
toolkit-doc rule so that the cited `LeniaNDK.py` references in
`continuous-ca/lenia-fft/...` route to the deferred-upstream bucket
rather than the generic fall-through (the toolkit-doc rule would not
match because the file is not under `tools/integrity/docs/`).

Catalog counts will be filled in by commit 4 after the grandfather
sweep runs.

## E. Incidental findings during execution

1. The parallel session's P1.8 (live-source-protection) work is in
   the working tree but was reset before this commit was staged so
   that only the classifier-rule diff is in commit 2. P1.8 will be
   re-applied to the working tree after this commit and remains
   uncommitted pending the parallel session's own commit cycle. The
   coordination protocol (spec § 8.1) anticipated this race surface.
2. The classifier change is independently testable without P1.8:
   `classify()` is pure-function and does not depend on
   `apply_annotations()` plumbing.
