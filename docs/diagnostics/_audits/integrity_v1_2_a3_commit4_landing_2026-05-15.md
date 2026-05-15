---
title: "Integrity v1.2 A.3 — Commit 4 landing audit"
date: 2026-05-15
author: claude-code (executor)
status: complete
landed-as-sha: 908f619
sibling-docs:
  - /home/otacon/Downloads/integrity_v1_2_a3_spec.md
  - docs/diagnostics/_audits/integrity_v1_2_a3_commit3_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_2_commit1_landing_2026-05-15.md
companion-shas:
  - "v1.2 A.3 commit 1 (module + fixtures + tests): 6fc5884"
  - "v1.2 A.3 commit 2 (classifier rules): 77628b6"
  - "v1.2 A.3 commit 3 (registration + skip-guard): 880a400"
  - "v1.2 P1.8 (live-source protection): 5fe5e6b"
---

# Integrity v1.2 A.3 — Commit 4 landing audit

## A. Change summary

Commit 4 lands the grandfather-sweep companion: ~660 inline
`integrity-allow:` annotations across audit/retro/toolkit-doc files
and the 3 live-source deferred-upstream files. The strict-mode gate
drops from 707 hard-fails (commit 3) to **44 hard-fails** — the
live-source residue plus the pre-existing baseline.

Two small bug-fixes piggyback in this commit:

1. The `deferred-upstream-bare-path` classifier rule originally
   required both `LeniaNDK` and `Chakazul` in the cat1.bare-path
   finding's message. The finding message only contains the cited
   path, not the source file context — so `Chakazul` never appears in
   the message and the rule was effectively dead. Fixed by matching
   on `LeniaNDK.py` substring only.
2. The classifier `reason` text for `deferred-upstream-bare-path`
   originally used "deferred-upstream bare-path" (space). The
   grandfather-report's snapshot parser extracts categories by
   substring match against the reason, requiring the hyphenated form.
   Updated the reason text to use the hyphenated category name and
   back-patched the three live-source annotations that had already
   been written with the old text.

## B. File inventory

The sweep modified 35+ files, organized by classification:

- **audit-bare-path** (635 entries swept across 29 files in
  `docs/diagnostics/_audits/`)
- **retro-bare-path** (11 entries swept across 3 files in
  `docs/retro/`)
- **toolkit-doc-bare-path** (7 entries swept across
  `docs/integrity-toolkit-spec.md`, `project-state.md`, and entries
  in `tools/integrity/docs/grandfather-catalog.md`)
- **deferred-upstream-bare-path** (5 entries swept across 3
  live-source files: `continuous-ca/lenia-fft/python/lenia_fft/presets.py`,
  `docs/sim-specs/lenia-fft.md`, and
  `tools/integrity/integrity/cat1_citations/checks/unregistered_upstream.py`
  — these are intentionally swept per the deferred-upstream policy:
  the citations are known-pending until the Chakazul upstream is
  vendored)
- **other-cat1-bare-path** (0 swept; 44 live-source findings
  protected by the extended P1.8 live-source filter; these remain
  unsuppressed in strict mode and constitute the live-source residue
  attributed to introducing authors)

Modified files (other than sweep annotations):

- `tools/integrity/integrity/grandfather.py` — fixed the
  `deferred-upstream-bare-path` rule to match `LeniaNDK.py` and
  updated the reason text; **also extended the P1.8 live-source
  protection** in `apply_annotations` to skip
  `other-cat1-bare-path` (formerly only `other-cat1`). Without this
  extension the sweep would have annotated every live-source
  bare-path finding, violating Hard Rule 9. This extension is a
  natural enlargement of P1.8's intent: any classifier category
  that is not specifically targeted should not auto-annotate
  live-source.
- `tools/integrity/integrity/snapshot.py` — added the five new
  bare-path category names to `_KNOWN_CATEGORIES` so the
  grandfather-report's per-category aggregation recognizes them.
  Without this, the new categories collapsed into the `other`
  bucket in the report.
- `tools/integrity/docs/grandfather-catalog.md` — populated the
  five new section heading counts (`(?)` → actual counts).

New file:

- `docs/diagnostics/_audits/integrity_v1_2_a3_commit4_landing_2026-05-15.md`
  (this file).

## C. Verification block

`pytest tools/integrity/tests/ -q` → all tests pass.

`python3 -m integrity --mode strict --no-audit-log`:

```
integrity: 5 pass, 0 soft-warn, 44 hard-fail, 1084 suppressed
```

Pre-sweep classification breakdown (per spec § 7.1):

| Arm | Count |
|---|---|
| REGISTERED-UPSTREAM | 0 |
| INTRA-REPO | 383 |
| AMBIGUOUS | 148 |
| UNRESOLVABLE | 114 |
| **Total** | **645** |

Post-sweep grandfather-report:

```
audit-bare-path: 635
retro-bare-path: 11
toolkit-doc-bare-path: 7
deferred-upstream-bare-path: 5
other-cat1-bare-path: 0 (44 live-source skipped per P1.8 extension)
```

Idempotence check: re-running `python3 tools/integrity/scripts/grandfather_sweep.py`
after the initial sweep produces "0 files modified; 0 annotations
added".

## D. Behavioral notes

**P1.8 extension rationale.** The committed P1.8 (commit 5fe5e6b)
protects `other-cat1` only. After A.3 commit 3, the
`cat1.bare-path` check generates findings whose classifier category
is `other-cat1-bare-path` (the fall-through for paths not in the
audit/retro/toolkit-doc/deferred-upstream buckets). Without
extending the live-source filter to cover `other-cat1-bare-path`,
the sweep would have auto-annotated ~50 live-source bare-path
findings, violating Hard Rule 9. The 1-line condition change
generalizes P1.8's semantics: live-source protection now covers
both `other-cat1` *and* `other-cat1-bare-path` (both are
heterogeneous fall-through buckets).

**Live-source residue (44 findings).** Per the v1.1 batch-1 triage
hybrid policy, live-source bare-path citations are attributed to
introducing authors rather than swept. The 44 residue is at the
upper end of the spec's "<50, OK; >50, pause-and-surface" tolerance.
Sample distribution:

| File | Count |
|---|---|
| CHANGELOG.md | 3 |
| continuous-ca/lenia-fft/docs/*.md | 9 |
| continuous-ca/lenia-fft/python/lenia_fft/*.py | 3 |
| docs/phase12_lattice_boltzmann.md | 1 |
| docs/sim-specs/lenia-fft.md | 2 |
| particle-fluids/sph-water/shaders/*.glsl | 8 |
| tools/integrity/integrity/cat1_citations/grammar.py | 2 |
| tools/integrity/integrity/cat3_numerical/generate_expected.py | 3 |
| tools/integrity/tests/test_cat1_bare_path.py | 5 |
| (other) | 8 |

A v1.2 follow-up should attribute each residue entry to its
introducing author per the v1.1 triage policy, OR a separate
policy ticket should reclassify `tools/integrity/tests/` as
sweepable (mirroring `tools/integrity/docs/`).

**Coordination with P1.8 (commit 5fe5e6b).** The parallel session
landed P1.8 between A.3 commits 3 and 4. The extension applied
here (covering `other-cat1-bare-path`) was anticipated and is
backwards-compatible with P1.8's tests (which only verify
`other-cat1` skip behavior).

## E. Incidental findings during execution

1. **Spec rule 4 (deferred-upstream-bare-path) was unimplementable as
   written.** The spec said to match `"LeniaNDK" in msg AND "Chakazul"
   in msg`, but the `cat1.bare-path` finding message only contains
   the cited path. The fix (match `"LeniaNDK.py" in msg`) recovers
   the spec's intent: catch the 5 known-pending Chakazul/LeniaNDK
   citations.
2. **The `audit-citation` count dropped from 597 (pre-A.3) to 80
   (post-A.3).** Mostly because bare-basename citations that
   previously fired under cat1.intra-repo (and were classified
   `audit-citation`) now fire under cat1.bare-path (and are
   classified `audit-bare-path`). The total audit-doc suppressions
   are roughly conserved (audit-citation + audit-bare-path: 80 + 635
   ≈ 715 vs old audit-citation: 597 — an additional ~120 audit-doc
   findings surfaced because cat1.bare-path also catches AMBIGUOUS
   and UNRESOLVABLE bare paths that cat1.intra-repo silently
   resolved-then-passed).
3. **Test file annotations.** `tools/integrity/tests/test_cat1_bare_path.py`
   contains 5 literal bare-basename test strings that fire
   cat1.bare-path. These are NOT swept per the extended P1.8 filter
   (the test file is under `tools/integrity/tests/`, which is
   live-source). They contribute to the 44 residue and should be
   addressed by either annotating with inline `# integrity-allow:`
   comments OR adding `tools/integrity/tests/` to
   SWEEPABLE_PATH_PREFIXES.
