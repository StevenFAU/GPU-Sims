---
title: "Integrity v1.3 Closeout Commit 5 — Conventions doc + T3 decisions"
date: 2026-05-17
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md
  - docs/diagnostics/_audits/integrity_v1_3_closeout_commit4_landing_2026-05-17.md
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.2-bolt-ons.md
  - docs/retro/integrity-toolkit-v1.3-batch1-part-a.md
  - docs/diagnostics/_audits/integrity_v1_1_post_retro_landing_2026-05-15.md
---

# Integrity v1.3 Closeout Commit 5 — Conventions doc + T3 decisions

## § A. Change summary

Lands `tools/integrity/docs/conventions.md` as the canonical home for
the toolkit's banked conventions (resolves T3.2 / Decision D5).
Conventions A through K are migrated from retros into one navigable
surface, regrouped under the four-bucket taxonomy per Decision D6
(spec-time discipline / execution-time discipline / batch
coordination / design taste). Letters preserved as historical
anchors; section headings drive navigation.

The doc also banks the long-stalled T3 decisions with explicit
"architect-1-decided, no architect-2 review obtained" disclosure:

- D4 (T3.1 audit-citation exclusion): **rejected** — keep-and-bucket
  preserves audit attribution.
- D5 (T3.2 conventions doc home): **`tools/integrity/docs/conventions.md`**
  (this doc).
- D6 (T3.3 numbering taxonomy): **four-bucket taxonomy**.
- D7 (T3.4 architect-2 backlog): items 1–3 **formally banked
  unresolved**; item 4 resolved by D2 in this batch's commit 4.

Convention F is normalized to the standard `> **F.**` blockquote
shape during migration (it was originally banked as indented prose in
the v1.1 post-retro landing audit; the prose body is preserved
verbatim, only the surrounding fence shape changes). The letter-I
collision between Convention I (cross-batch scope discipline,
v1.2 § 4.3) and the Part-B § 4.1 "Convention I" reference (rewrite-
stale-reasons feature, landed as a feature in v1.3 closeout commit 1)
is documented explicitly in the Convention I section.

README updated with a Conventions section pointing at the new doc.

## § B. File inventory

| Status | Path | Diff |
|---|---|---|
| Created | `tools/integrity/docs/conventions.md` | ~250 LOC (11 conventions + 4 decisions + disclosure) |
| Modified | `tools/integrity/README.md` | +6 LOC (Conventions pointer section) |
| Created | `docs/diagnostics/_audits/integrity_v1_3_closeout_commit5_landing_2026-05-17.md` | this report |

## § C. Verification

Pre-edit anchoring (HEAD `c7e97bd`, after closeout commit 4 landed):

```
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 45 hard-fail, 1339 suppressed
```

Audit-prose-freshness sanity check on the new doc:

```
$ python3 tools/integrity/scripts/audit_prose_freshness.py tools/integrity/docs/conventions.md
audit-prose-freshness: checked 0 citations across 1 files (0 skipped as non-repo-local)
audit-prose-freshness: all citations resolve
```

The conventions doc itself has zero `path:line`-shaped citations
under repo-local prefixes (it cites retros and audit reports by
section name only, plus one relative-path link to the spec doc),
which is the right shape for a reference doc.

Post-edit gate:

```
$ python3 -m integrity --mode strict --no-audit-log 2>&1 | head -1
integrity: 5 pass, 0 soft-warn, 45 hard-fail, 1339 suppressed
```

Gate unchanged from pre-edit; doc-only commit with no new citations
or grammar literals.

Full pytest suite: unchanged (no test additions in this commit).

## § D. Behavioral notes

**Convention text provenance.** All eleven convention blockquote
bodies are copied verbatim from the source retros (probe § C.1–C.4
captured the exact text and confirmed location alignment). Where two
sources of a convention exist (e.g., the inline prose vs the
canonical retro entry), the canonical retro version is used; for F
specifically, the indented-prose form from the v1.1 post-retro
landing audit § D.2.1 was re-formatted as a blockquote, preserving
the prose body word-for-word.

**Taxonomy assignment.** Per Decision D6 / Part-A retro § 5.2:

- **Spec-time discipline:** C, D, K
- **Execution-time discipline:** A, F
- **Batch coordination:** G, I, J
- **Design taste:** B, E, H

The spec assigned the buckets at draft time; this commit follows
that assignment verbatim. Future readers debating a re-assignment
(e.g., "B belongs under batch coordination" is an arguable claim)
should bank that observation as a v2 reconsideration item rather
than silently changing the doc.

**Letter-I collision documentation.** Convention I in this doc is the
v1.2 bolt-ons retro § 4.3 entry (cross-batch scope discipline). The
Part-B retro § 4.1 reference to "Convention I" for the rewrite-stale-
reasons feature is documented inline in the Convention I section as
a numbering collision; the rewrite-stale-reasons capability is a
feature, not a convention, and is documented in the v1.3 closeout
spec § 2 (now landed in commit 1).

**Disclosure block.** The top-of-doc disclosure names the
architect-2-opt-out and warns readers that the conventions, while
load-bearing in practice, have not had independent review. The
"Decisions resolved without architect-2 review" section provides per-
decision disclosure as a forward audit trail for any future
architect-2 pass.

## § E. Banked observations

**Convention B / E taxonomy might be debatable.** Convention B
("Grandfather-sweep companion") arguably fits batch coordination
better than design taste; Convention E ("Spec-author-self-test
review") arguably fits spec-time discipline. The spec drafted these
under design taste, and per Hard Rule 1 ("execute every file
creation, modification, and removal specified") the landed doc
follows the spec verbatim. A future architect-2 review (or a
follow-up retro) might revisit.

**No new code.** This commit ships documentation only. No
classifier, sweep, or gate behavior changes. Pytest count and gate
hard-fail count are both unchanged.

**Future-mirror discipline.** The "Living document" section at the
end of conventions.md instructs the next batch to mirror any
newly-banked conventions from retros into this doc. Per the
single-source-of-truth principle, this doc becomes the canonical
reference and the retros remain the per-origination forensic record.

## § F. Cross-references

- Spec § 6 (`docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md`)
- Probe § C.1–C.4 (verbatim convention text), § G.7 (Convention F
  formatting normalization)
- Decisions D4 / D5 / D6 / D7 (spec § 0.3) — landed with disclosure
  in `tools/integrity/docs/conventions.md`
- Part-A retro § 5.2 (taxonomy proposal)
- Part-B retro § 5.4 (architect-2 opt-out formal-bank recommendation)
- Convention #12 — commit 8 of this batch resolves the
  `c7e97bd` placeholder above
