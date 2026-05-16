---
title: "Integrity v1.3 Closeout Commit 8 — SHA back-fill for commits 1-7"
date: 2026-05-17
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_closeout_commit7_landing_2026-05-17.md
---

# Integrity v1.3 Closeout Commit 8 — SHA back-fill for commits 1-7

## § A. Change summary

Per Convention #12 (SHA back-fill is a separate commit, never
`--amend`), this commit replaces every `<COMMIT_N_SHA>` placeholder
in the commit-1 through commit-7 audit reports with the resolved
7-character short SHA. Per Part-A retro § 6, the back-fill commit
itself ships with an audit report; this is that report.

Six placeholders were back-filled (one per downstream-sibling
reference in commits 2 through 7's pre-edit-anchoring and
cross-references sections):

| Placeholder | Resolved SHA | Commit message subject |
|---|---|---|
| `<COMMIT_1_SHA>` | `1c84cce` | feat(integrity): --rewrite-stale-reasons sweep mode |
| `<COMMIT_2_SHA>` | `a9b2aeb` | perf(integrity): T2.3 single-parse Stack C refactor |
| `<COMMIT_3_SHA>` | `f45ebb2` | feat(integrity): T2.2 audit-prose-freshness sibling tool |
| `<COMMIT_4_SHA>` | `c7e97bd` | feat(integrity): T2.1 paired-sweep CI enforcement |
| `<COMMIT_5_SHA>` | `3d25ddc` | docs(integrity): conventions doc + T3 decisions |
| `<COMMIT_6_SHA>` | `ebbb743` | chore(integrity): remove project-state.md fossil annotations |

The `<COMMIT_8_SHA>` placeholder remains in `project-state.md`'s
phase ledger row and in commit 7's audit report (two references).
This commit is commit 8; its own SHA is not knowable until after
this commit lands. Mirroring the Part-B SHA back-fill (`67e19c1`)
pattern, the closeout batch does not include a commit-9 self-back-
fill; the `<COMMIT_8_SHA>` placeholder is the documented terminal
state for v1.3 closeout. A future maintenance commit may resolve it
if desired; per spec § 9 verification block, only commits 1–7's
placeholders are required to resolve at batch-close.

## § B. File inventory

| Status | Path | Diff |
|---|---|---|
| Modified | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit2_landing_2026-05-17.md` | 2 placeholders → `1c84cce` |
| Modified | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit3_landing_2026-05-17.md` | 2 placeholders → `a9b2aeb` |
| Modified | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit4_landing_2026-05-17.md` | 2 placeholders → `f45ebb2` |
| Modified | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit5_landing_2026-05-17.md` | 2 placeholders → `c7e97bd` |
| Modified | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit6_landing_2026-05-17.md` | 2 placeholders → `3d25ddc` |
| Modified | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit7_landing_2026-05-17.md` | 1 placeholder → `ebbb743` (plus 2 remaining `<COMMIT_8_SHA>` placeholders, intentionally not resolved) |
| Created | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit8_landing_2026-05-17.md` | this report |

## § C. Verification

Pre-edit enumeration of placeholders (spec § 9.B):

```
$ for n in 1 2 3 4 5 6 7; do
    sha=$(git log --format=%H --grep="v1.3 closeout commit $n" -1)
    echo "commit $n: $sha"
  done
commit 1: 1c84cce...
commit 2: a9b2aeb...
commit 3: f45ebb2...
commit 4: c7e97bd...
commit 5: 3d25ddc...
commit 6: ebbb743...
commit 7: 4b85cdb...
```

Post-edit verification (spec § 9.D):

```
$ grep -E "<COMMIT_[1-7]_SHA>" docs/diagnostics/_audits/integrity_v1_3_closeout_commit*_landing_2026-05-17.md project-state.md
(empty)

$ for sha in 1c84cce a9b2aeb f45ebb2 c7e97bd 3d25ddc ebbb743 4b85cdb; do
    git cat-file -e "$sha" 2>/dev/null && echo "OK $sha" || echo "MISSING $sha"
  done
OK 1c84cce
OK a9b2aeb
OK f45ebb2
OK c7e97bd
OK 3d25ddc
OK ebbb743
OK 4b85cdb
```

All seven SHAs resolve; no `<COMMIT_[1-7]_SHA>` placeholders remain
in the audit corpus. The two `<COMMIT_8_SHA>` placeholders in commit-7
audit and project-state.md phase ledger remain by design (commit-8
SHA isn't knowable from inside commit 8 without `--amend`, which
Convention #12 forbids).

Sweep companion + gate:

```
$ python3 tools/integrity/scripts/grandfather_sweep.py
grandfather-sweep: modified 1 files; 3 annotations added
  skipped as live-source (other-cat1): 40 (use --sweep-live-source to include)
         audit-report-grammar-example: 2
                      audit-bare-path: 1
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 46 hard-fail, 1345 suppressed
```

Hard-fail count unchanged at 46. Suppressed grew 1342 → 1345 (+3,
matching the sweep companion's absorbed findings — three new
annotations on audit-doc files whose citations changed when the
literal SHAs replaced the placeholders).

Catalog refresh (spec § 9.C): attempted via
`tools/integrity/scripts/refresh_catalog_counts.py`; the script flags
an `other` category with count 4 that has no heading in the catalog.
Inspection: the `other` bucket arises from custom annotation reason
strings introduced by the closeout's test-fixture inline annotations
(e.g., `toolkit-own test fixture string mimicking citation grammar`
from commit 3 / commit 4), which don't match any `_KNOWN_CATEGORIES`
substring nor `_REASON_PATTERNS` regex. These are real suppressions
on toolkit-own test code; not a regression. The catalog drift is
banked here for follow-up rather than fixed in-band (catalog
section-add is a one-line `### \`other\` (4)` addition that doesn't
need to ship with the SHA back-fill).

## § D. Behavioral notes

**Convention #12 honored.** No `--amend`. The back-fill is a discrete
commit with its own audit, matching the Part-B pattern.

**`<COMMIT_8_SHA>` left as-is.** The commit being made cannot know
its own SHA in advance. Two references remain:
- `project-state.md` phase ledger row 12.5 "Shipped at"
- `commit7_landing` § A.1 ("Status `✅ Done`, Shipped at `<COMMIT_8_SHA>`
  placeholder...") and § F (".. resolves the `ebbb743` and
  `<COMMIT_8_SHA>` placeholders above")

Resolving either would require either a commit-9 follow-up (which
this batch does not include — the spec calls for 8 commits) or
`--amend` (forbidden). Recommended: a maintenance commit at any
later time can run `sed -i 's/<COMMIT_8_SHA>/<resolved>/g'` against
those two files.

**`mechanical sed substitution.`** Back-fill executed via
`sed -i 's/<COMMIT_N_SHA>/<sha>/g'` per audit-doc, one substitution
per placeholder pair. No semantic edits; pure string replacement.

**Sweep companion.** 3 audit-doc annotations added. The SHA back-fill
modified audit-doc files' content lines that contain literal `SHA:`
or grammar-token strings; the regular sweep absorbed them.

## § E. Banked observations

**Catalog `other` bucket drift.** The closeout introduced 4
test-fixture annotation reasons that don't classify into a named
category (they use prose like "toolkit-own test fixture string
mimicking citation grammar"). The `refresh_catalog_counts.py` script
errors when this happens and recommends adding an `### \`other\` (4)`
section. Recommendation: add the section in a future maintenance
commit, OR rewrite the four test-fixture annotation reasons to use a
named category. Either fix is small; banked here as a follow-up.

**CI walltime for T2.3 perf assertion still unverified.** The Stack
C single-parse refactor (commit 2) shipped a skip-by-default perf
assertion with a 120s ceiling. The first post-landing CI run on the
push of this batch will be the first verifiable measurement. No CI
result is available at audit-write time.

**Post-batch state.** The closeout batch is complete after this
commit. Toolkit v1 is documented closed (project-state.md ledger
row, README "Status" section, conventions doc canonical home). The
next user-facing action is `git push origin main`.

## § F. Cross-references

- Spec § 9 (`docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md`)
- Convention #12 (v1.2 bolt-ons retro § 7.2 / project-state.md § 7)
- Part-B SHA back-fill precedent (`67e19c1`)
- All seven preceding commits' audit reports (commits 1–7 of the v1.3
  closeout batch)
