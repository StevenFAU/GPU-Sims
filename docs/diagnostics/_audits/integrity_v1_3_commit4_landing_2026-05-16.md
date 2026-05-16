---
title: "Integrity v1.3 Commit 4 — SHA Back-Fill"
date: 2026-05-16
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_commit1_landing_2026-05-16.md
  - docs/diagnostics/_audits/integrity_v1_3_commit2_landing_2026-05-16.md
  - docs/diagnostics/_audits/integrity_v1_3_commit3_landing_2026-05-16.md
  - docs/diagnostics/_audits/integrity_v1_3_t1_3_5_spec_2026-05-16_architect1.md
---

# Integrity v1.3 Commit 4 — SHA Back-Fill

## § A. Change summary

Commit 4 of the v1.3 batch-1 part-A landed the SHA back-fill required
by Convention #12 (retro § 7.2): the three substantive commits in
this batch were authored with `<COMMIT_N_SHA>` placeholders in their
§ G "Next commit" pointers because the downstream SHAs were not known
at draft time. Commit 4 replaces every placeholder with the resolved
SHA.

The spec at `docs/diagnostics/_audits/integrity_v1_3_t1_3_5_spec_2026-05-16_architect1.md`
§ 6 did not require a landing audit report for this commit (only § 6.1
through § 6.6 covering purpose, pre-commit, file modifications,
verification, commit message, push). This minimal landing audit is
authored post-hoc to (a) keep the four-commits-per-batch audit-doc
inventory complete and (b) resolve the sibling-doc citation in the
v1.3 batch-1 part-A retro's front-matter. Per Convention F
(audit-prose freshness), this gap is documented as § E rather than
silently filled.

## § B. File inventory

| Status | Path | Diff |
|---|---|---|
| Modified | `docs/diagnostics/_audits/integrity_v1_3_commit1_landing_2026-05-16.md` | `<COMMIT_2_SHA>` → `72a2d26` (1 line) |
| Modified | `docs/diagnostics/_audits/integrity_v1_3_commit2_landing_2026-05-16.md` | `<COMMIT_3_SHA>` → `9e3afa9` (1 line) |
| Modified | `docs/diagnostics/_audits/integrity_v1_3_commit3_landing_2026-05-16.md` | `<COMMIT_1_SHA>` / `<COMMIT_2_SHA>` / `<COMMIT_3_SHA>` → `65a7685` / `72a2d26` / `9e3afa9` (3 lines) |
| Modified | `tools/integrity/docs/grandfather-catalog.md` | `audit-bare-path` count: 745 → 747 (post-sweep refresh) |

Per Convention #12: separate follow-up commit, never `--amend`.

## § C. Verification

All four cited SHAs resolve:

```
$ for sha in 65a7685 72a2d26 9e3afa9; do git cat-file -e "$sha" 2>/dev/null && echo "OK $sha"; done
OK 65a7685
OK 72a2d26
OK 9e3afa9
```

No `<COMMIT_N_SHA>` placeholders remain:

```
$ grep -l "<COMMIT_[1-3]_SHA>" docs/diagnostics/_audits/integrity_v1_3_commit*_landing_2026-05-16.md
(empty output)
```

Gate state post-sweep:

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 5 pass, 0 soft-warn, 44 hard-fail, 1262 suppressed
```

Hard-fail count unchanged from commit-3's post-sweep state (44).
Suppressed count grew by 2 from the inline sweep companion picking up
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
two new bare-path findings in commit-3's audit report (`runner.py:148`
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
and `runner.py:154` line-number-drift citations).

## § D. Design decisions applied

**Convention #12** (retro § 7.2) — SHA back-fill is a separate commit,
never `--amend`. Rationale: amending would rewrite history for commits
already pushed; the back-fill is a small textual change that doesn't
warrant the disruption.

## § E. Banked observations

1. **Spec § 6 omitted the audit-report deliverable for this commit.**
   Spec § 6 covers purpose / pre-commit / file modifications /
   verification / commit message / commit and push — no audit-report
   subsection (cf. § 3.6 / § 4.4 / § 5.4 for commits 1–3). This
   landing audit was authored post-hoc when the v1.3 batch-1 part-A
   retro's front-matter cited an `integrity_v1_3_commit4_landing_2026-05-16.md`
   sibling-doc and the freshness check surfaced the missing file.
   Recorded here rather than silently editing the spec.

2. **Audit-doc growth from the back-fill itself is minimal.** The
   actual back-fill changes were five 1-line SHA substitutions plus
   the catalog refresh. No new audit-doc bare-paths from the SHA
   substitutions themselves; the +2 suppressed count came from
   commit-3's audit-report content rather than commit-4's changes.

## § F. Cross-references

- Spec § 6 — Commit 4 plan.
- Commit 1 audit (`65a7685`) — `<COMMIT_2_SHA>` placeholder origin.
- Commit 2 audit (`72a2d26`) — `<COMMIT_3_SHA>` placeholder origin.
- Commit 3 audit (`9e3afa9`) — three `<COMMIT_N_SHA>` placeholders.
- Retro § 7.2 Convention #12 — Back-fill as separate commit.

## § G. Closing batch

Commit 4 closes v1.3 batch-1 part-A. The retro at
`docs/retro/integrity-toolkit-v1.3-batch1-part-a.md` follows in the
next commit.
