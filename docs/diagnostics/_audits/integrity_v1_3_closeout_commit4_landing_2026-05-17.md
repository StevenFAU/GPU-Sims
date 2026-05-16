---
title: "Integrity v1.3 Closeout Commit 4 — T2.1 paired-sweep CI enforcement"
date: 2026-05-17
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_closeout_commit3_landing_2026-05-17.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
---

# Integrity v1.3 Closeout Commit 4 — T2.1 paired-sweep CI enforcement

## § A. Change summary

Lands `tools/integrity/scripts/check_paired_sweep.py` and wires it
into `.github/workflows/integrity.yml` as a PR-only job step. The
check fails when cat1-scannable live-source files changed in the PR
diff without a paired grandfather-sweep commit (or an explicit
`[skip-paired-sweep]` escape-hatch tag in any commit body in the
range).

Per Decision D2 (closeout spec § 0.3): medium enforcement level — CI
check, not pre-commit hook. Pre-commit hooks slow every edit; the
soft-discipline-only convention (Convention G) has failed to hold
across three retros. CI-gated enforcement is the right ergonomic
trade-off: late enough that local iteration stays fast, early enough
that the violation is caught before merge.

Heuristic: a commit is a paired sweep if (a) any commit body in the
range contains `grandfather-sweep`, `grandfather sweep`, or
`sweep-companion`, OR (b) the diff touches
`tools/integrity/docs/grandfather-catalog.md` (the catalog refresh is
the canonical artifact of a sweep run). Either signal suffices.

Three new tests pin the behavior. README pointer omitted (this is a
CI-only tool with no direct operator workflow).

## § B. File inventory

| Status | Path | Diff |
|---|---|---|
| Created | `tools/integrity/scripts/check_paired_sweep.py` | ~125 LOC (executable) |
| Created | `tools/integrity/tests/test_check_paired_sweep.py` | ~95 LOC (3 tests) |
| Modified | `.github/workflows/integrity.yml` | +16 LOC (new PR-only step + base-ref fetch) |
| Created | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit4_landing_2026-05-17.md` | this report |

## § C. Verification

Pre-edit anchoring (HEAD `f45ebb2`, after closeout commit 3 landed):

```
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 44 hard-fail, 1332 suppressed
```

New tests:

```
$ python3 -m pytest tools/integrity/tests/test_check_paired_sweep.py -v
test_no_live_source_changes_passes PASSED
test_live_source_change_without_sweep_fails PASSED
test_skip_tag_overrides_check PASSED
============================== 3 passed in 0.17s ===============================
```

Local smoke against the current branch (HEAD~5..HEAD spans this
batch's commits 1, 2, 3 + part-B retro + part-B SHA back-fill, all of
which include sweep companions or touch the catalog):

```
$ python3 tools/integrity/scripts/check_paired_sweep.py --base-ref HEAD~5 --head-ref HEAD
check-paired-sweep: tools/integrity/docs/grandfather-catalog.md touched; treating as paired-sweep; OK
$ echo $?
0
```

Workflow YAML sanity-check: new step is inserted after the
`Install integrity toolkit` step (the script depends on
`integrity.grandfather.is_live_source_path`, which the editable install
exposes). The step is guarded by `if: github.event_name ==
'pull_request'` so push-to-main runs (which typically merge a
previously-approved PR) do not double-fire. Base-ref fetch uses
option 2 from probe § E.1 (`git fetch --no-tags --depth=1 origin
$BASE_REF`); the workflow's checkout `fetch-depth: 1` is unchanged.

## § D. Behavioral notes

**D2 medium enforcement.** The CI check fails the workflow when the
violation occurs; it does not amend or rewrite. The PR author resolves
by either running the sweep and amending the PR, or adding
`[skip-paired-sweep]` to a commit body in the PR range (for
docs-only live-source edits that legitimately don't add findings).

**PR-only guard.** `if: github.event_name == 'pull_request'`. Rationale:
push-to-main is typically the merge of a PR that already passed the
check; firing again on the push-to-main is redundant and would
double-spend the protective signal. PR is the right semantic surface.

**Fetch pattern (probe § E.1 option 2).** The new step runs `git fetch
--no-tags --depth=1 origin "$BASE_REF"` before computing the diff.
This keeps the workflow's checkout step's `fetch-depth: 1` (fast
clone for non-diff steps) and pays the deepening cost only for the
diff range. Option 1 (`fetch-depth: 0`) was considered and rejected
as too expensive on every workflow run for the marginal simplicity
gain.

**Heuristic and escape hatches.** A commit qualifies as a paired
sweep if its body contains any of three markers (`grandfather-sweep`,
`grandfather sweep`, `sweep-companion`), OR if the diff touches
`tools/integrity/docs/grandfather-catalog.md`. The escape-hatch
`[skip-paired-sweep]` is intentionally documented inside the failure
message so the affected operator sees it inline without consulting
the docs.

**No README pointer.** The script is wired only through the CI
workflow; an operator who hits the failure follows the inline
instructions in the workflow log. No direct human-driven invocation
is expected outside CI. Distinct from the audit-prose-freshness
sibling tool (commit 3), which has a README pointer because spec
authors invoke it directly.

## § E. Banked observations

**First post-landing PR will exercise the check live.** The first PR
opened after this batch lands will be the first runtime test of the
workflow step. The closeout batch itself lands as a sequence of
direct main commits (each with a sweep companion or catalog touch),
so the merge into origin/main does not trigger the PR path. Commit 8
or a follow-up addendum should capture the first PR's behavior.

**Workflow placement.** Inserted between `Install integrity toolkit`
and `Install build dependencies (for compile_commands.json)` so the
check fires early in the job. If the build deps install fails for
unrelated reasons, the paired-sweep result is still recorded as a
separate step status — useful for triage.

**No false-paired risk on this batch.** Each closeout commit either
explicitly mentions `sweep-companion` / `grandfather-sweep` in its
body or touches the catalog file. The heuristic would not flag the
closeout commits if they were submitted as a PR.

## § F. Cross-references

- Spec § 5 (`docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md`)
- Probe § B.5, § E.1, § G.6
- Decision D2 (spec § 0.3) — medium-enforcement rationale
- Convention G (v1.2 bolt-ons retro § 4.1) — sweep-side protection
  before check-side scope expansion; resolved into mechanical
  enforcement by this commit
- Convention #12 — commit 8 of this batch resolves the
  `f45ebb2` placeholder above
