---
title: "Integrity v1.3 Commit 3 — T1.4 Probe Template Conventions Doc"
date: 2026-05-16
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_t1_3_5_probe_2026-05-16_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_t1_3_5_spec_2026-05-16_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_commit1_landing_2026-05-16.md
  - docs/diagnostics/_audits/integrity_v1_3_commit2_landing_2026-05-16.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
---

# Integrity v1.3 Commit 3 — T1.4 Probe Template Conventions Doc

## § A. Change summary

T1.4 lands `tools/integrity/docs/probe-template-conventions.md`, the
canonical home for Convention C (path-resolution enumeration) and
Convention D (call-site enumeration) banked by v1.1 batch-1 retro
§ 7.2. The doc embeds three worked examples per convention drawn
from existing audit and probe reports — the violation case from v1.1
batch-1 (pause-and-surface #1 / #2) and two followed-correctly cases
each from v1.2 batch work (A.3 probe, A.2 probe, bolt-ons probe).

Originating record: v1.1 batch-1 retro § 7.2 C+D. Worked-example
sources per probe § C.5 / § C.6. The doc serves as the canonical home
for both conventions until roadmap § 5 T3.2 (permanent CONVENTIONS
doc) lands.

## § B. File inventory

| Status | Path | Diff |
|---|---|---|
| Created | `tools/integrity/docs/probe-template-conventions.md` | +171 LOC |
| Created | `docs/diagnostics/_audits/integrity_v1_3_commit3_landing_2026-05-16.md` | this report |

## § C. Verification

Doc presence + light validation:

```
$ wc -l tools/integrity/docs/probe-template-conventions.md
171 tools/integrity/docs/probe-template-conventions.md

$ python3 -c "
content = open('tools/integrity/docs/probe-template-conventions.md').read()
assert content.startswith('# Probe Template Conventions')
assert 'Convention C' in content
assert 'Convention D' in content
assert content.count('## Convention') == 2
print('basic structure OK')
"
basic structure OK
```

Note: the spec § 5.3's validation block included
`assert content.count('### Examples') == 0` ("no orphan example
headers"), but the spec's own doc body in § 5.2 uses
`### Examples of Convention C followed` / `### Examples of
Convention D followed` H3 subheadings. The doc body is authoritative
per Hard Rule 2; the validation rule conflicts with it and was
relaxed. Recorded as banked observation § E.1.

Gate state (no sweep needed):

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 5 pass, 0 soft-warn, 44 hard-fail, 1260 suppressed
```

Gate unchanged from post-commit-2 baseline. The new doc's intra-repo
citations are all path-prefixed
(`docs/diagnostics/_audits/integrity_...`) and do not trigger
`cat1.bare-path`. No findings surface from the new doc.

## § D. Cross-references

- Probe § C.5 — Convention C worked-example sources.
- Probe § C.6 — Convention D worked-example sources.
- Probe § C.7 — Worked-example scope decision (embed all six).
- Probe § F.6 — Line-number drift precedent for the bolt-ons § D.1 example.
- v1.1 batch-1 retro § 7.2 C + D — Originating conventions.
- v1.1 batch-1 retro § 3.1 — Convention C violation source.
- v1.1 batch-1 retro § 3.2 — Convention D violation source.

## § E. Banked observations

1. **Spec § 5.3 validation rule conflicts with spec § 5.2 doc body.**
   The validation block in spec § 5.3 asserts
   `content.count('### Examples') == 0`, but the doc body the spec
   provides in § 5.2 uses two `### Examples of Convention [C|D]
   followed` subheadings as the lead-in to the worked examples. The
   doc body is the structurally correct content (the H3 subheadings
   separate convention-text from worked-example payload); the
   validation rule was a drafting oversight. Recorded here per
   Convention F (audit-prose freshness) rather than rewriting either
   the spec or the doc to match the rule.

2. **Line-number-drift precedent already validated in commit 1.** The
   bolt-ons probe § D.1's `_emit_human_summary` line citation
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
   (`runner.py:148`) is stale relative to current disk
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
   (`runner.py:154` per probe § F.6). The doc's "How to apply"
   guidance to tag with anchor SHA rather than line numbers is exactly
   the methodology that prevents this drift; the doc's own bolt-ons
   example follows the guidance (omits line numbers from the citation
   payload). The probe's § F.6 framing is reinforced rather than
   contradicted by execution-time inspection.

## § F. Next commit

Commit 4 — SHA back-fill. Commits 1, 2, 3 used `<COMMIT_N_SHA>`
placeholders in their § G "Next commit" pointers; commit 4 will
replace those placeholders with the actual SHAs.

- Commit 1: `65a7685` (T1.3)
- Commit 2: `72a2d26` (T1.5)
- Commit 3: this commit — `9e3afa9`
