# Integrity Toolkit v1.1 — Commit 1 Landing — 2026-05-15

First commit in the integrity-toolkit v1.1 batch-1 sequence per
`docs/diagnostics/_audits/integrity_v1_1_batch1_spec.md`. Implements
item **A.1**: `cat2.stub-label-stale`. Closes the only spec § 12
canonical defect that v1 detection did not cover, and grandfathers the
two confirmed pre-v1.1 cases.

Companion to:

- Batch-1 execution spec: `docs/diagnostics/_audits/integrity_v1_1_batch1_spec_2026-05-15_architect1.md`
- v1.1 spec draft probe: `docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md`
- v1.1 spec draft apispec: `docs/diagnostics/_audits/integrity_v1_1_apispec_2026-05-15_architect1.md`
- Prior commit's audit: `integrity_build_8_landing_2026-05-14.md`
- Next commit's audit: `integrity_v1_1_commit2_landing_2026-05-15.md` (SHA back-filled separately)

---

## A. Change summary

`cat2.stub-label-stale` flags occurrences of the canonical phrasing
`In Phase <N>, this is a stub:` on header files whose corresponding
implementation is no longer a stub (impl has more than 10 non-comment
LOC). Scope: C++ headers under `common/common-cpp/include/` and Python
modules under `common/common-py/gpusims_common/`. Stack D files
carrying the discriminator phrasings `permanent stub` or `real-or-stub`
in the top 40 lines are exempt.

Anchored on the exact label phrasing per probe § G (rather than
`\bstub\b`) to avoid false positives on `permanent stub for Phase 9`
and `real-or-stub` discriminator patterns. Threshold 10 non-comment LOC
mirrors probe § D.2.

Two cases detected on the synced `main` HEAD `447ebf0`:

- `common/common-cpp/include/gpusims/alembic_writer.hpp:13` —
  impl `common/common-cpp/src/alembic_writer.cpp` has 82 non-comment LOC.
- `common/common-cpp/include/gpusims/vdb_writer.hpp:12` —
  impl `common/common-cpp/src/vdb_writer.cpp` has 114 non-comment LOC.

Both are grandfathered into the new `cat2-stub-label-stale` category.
Migration target: replace the "Phase 1 stub" framing with the
runtime-mode discriminator shape used in the Stack D twins
(e.g., `Real-or-stub depending on GPU_SIMS_HAVE_ALEMBIC`) when the
headers are next edited.

## B. File inventory

**New:**

- `tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py` —
  check implementation (~170 LOC).
- `tools/integrity/tests/test_cat2_stub_label_stale.py` — 6 tests covering
  bad/good fixtures, discriminator gating, smoke, and the empty-repo case.
- `tools/integrity/tests/fixtures/bad_stub_label/` — fixture tree mirroring
  production layout, with stale label + 14-LOC impl (C++) and
  stale label without discriminator (Python).
- `tools/integrity/tests/fixtures/good_stub_label/` — fixture tree with
  stale label + 5-LOC impl (C++ real stub) and stale label with
  `permanent stub` discriminator (Python).
- `docs/diagnostics/_audits/integrity_v1_1_probe_2026-05-15_architect1.md` —
  v1.1 probe report (untracked at start of commit; bundled in per spec
  § 1 sibling-docs front-matter).
- `docs/diagnostics/_audits/integrity_v1_1_apispec_2026-05-15_architect1.md` —
  v1.1 API-surface probe report (untracked at start of commit; bundled
  in per spec § 1 sibling-docs front-matter).
- This audit report.

**Modified:**

- `tools/integrity/integrity/cat2_contracts/checks/__init__.py` —
  register `stub_label_stale` in `REGISTERED_CHECKS`.
- `tools/integrity/integrity/grandfather.py` — classifier rule for
  `cat2.stub-label-stale` → `cat2-stub-label-stale`. Reason text uses
  ` -- ` instead of `;` to satisfy the annotation grammar (E.2 below).
- `tools/integrity/docs/grandfather-catalog.md` — new
  `cat2-stub-label-stale` category section between `cat2-stack-b-unused`
  and "Suppression-annotation discipline".
- `common/common-cpp/include/gpusims/alembic_writer.hpp` — grandfather
  annotation inserted above line 13.
- `common/common-cpp/include/gpusims/vdb_writer.hpp` — grandfather
  annotation inserted above line 12.

## C. Verification

All steps run from repo root unless noted.

### C.1 New-check unit tests

```
$ cd tools/integrity && python3 -m pytest tests/test_cat2_stub_label_stale.py -v
...
tests/test_cat2_stub_label_stale.py::test_bad_cpp_header_with_real_impl_flags PASSED
tests/test_cat2_stub_label_stale.py::test_good_cpp_header_with_stub_impl_does_not_flag PASSED
tests/test_cat2_stub_label_stale.py::test_bad_python_stub_label_flags PASSED
tests/test_cat2_stub_label_stale.py::test_python_permanent_stub_discriminator_does_not_flag PASSED
tests/test_cat2_stub_label_stale.py::test_check_id_and_mode PASSED
tests/test_cat2_stub_label_stale.py::test_repo_root_with_no_common_dir_returns_empty PASSED
============================== 6 passed in 0.02s ===============================
```

### C.2 Full test suite

```
$ cd tools/integrity && python3 -m pytest tests/
============================= 80 passed in 26.13s ==============================
```

80 = 74 baseline + 6 new.

### C.3 Real-repo strict-mode run

```
$ python3 -m integrity --mode strict --no-audit-log
integrity: 2 pass, 0 soft-warn, 0 hard-fail, 1128 suppressed
EXIT=0
```

1128 = 1126 baseline (probe § A.1) + 2 new stub-label-stale findings
now grandfathered.

### C.4 Check-id smoke

```
$ python3 -m integrity --check cat2.stub-label-stale --output human --no-audit-log
integrity: 1 pass, 0 soft-warn, 0 hard-fail, 2 suppressed
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
Both findings on `alembic_writer.hpp:13` and `vdb_writer.hpp:12` are
suppressed by the grandfather annotations inserted by the sweep.

### C.5 Sweep idempotence

```
$ python3 tools/integrity/scripts/grandfather_sweep.py
grandfather-sweep: modified 0 files; 0 annotations added
```

No further annotations needed.

## D. Behavioral notes

- **Canonical case coverage:** Both spec § 12 row 5 cases caught.
  `vdb_writer.hpp` is a bonus catch beyond what spec § 12 named — the
  v1.1 probe identified it as a parallel case during § G.
- **Stack D files:** No Stack D `.py` files currently carry the literal
  "In Phase N, this is a stub:" phrasing without a `permanent stub` or
  `real-or-stub` discriminator, so the Python-side check has zero
  current findings on `main`. The discriminator guard is exercised by
  the `good_stub_label/permanent.py` fixture.
- **Stack B:** Excluded by Decision 3 — TypeScript "stub" usage is rare
  and probe § D.1 found zero hits.
- **Grandfather treatment:** `cat2-stub-label-stale` is a migration
  category, not a permanent suppression. When either header is next
  edited for unrelated reasons, the suppression should be removed and
  the framing replaced with the runtime-mode discriminator shape used
  in the Stack D twins. See catalog `cat2-stub-label-stale` section.

## E. Incidental findings

### E.1 Spec fabrication caught by Hard Rule 2 — `include`/`src` layout mirror

The spec's Decision 2 asserted a 1:1 mirror between
`include/<sub>/<base>.hpp` and `src/<sub>/<base>.cpp`. Synced repo
state has `include/gpusims/...` stripping the `gpusims/` namespace
component in the `src` tree — i.e., `include/gpusims/alembic_writer.hpp`
maps to `src/alembic_writer.cpp`, not `src/gpusims/alembic_writer.cpp`.
The check's first execution attempt resolved zero impl paths and would
have missed both canonical target cases.

Resolved by replacing the path-mirror logic with a first-component-strip
rule: the first directory component after `include/` is treated as the
project namespace and stripped. Verified against three production
header/impl pairs (`alembic_writer`, `vk/context`, `vdb_writer`).

This is exactly the fabrication class the toolkit exists to catch — the
spec author (architect-1) is the same fabrication source the toolkit
defends against, and the apispec probe did not include a probe of the
`include`→`src` convention. Future v1.1+ spec drafts should add a
convention-verification line item to every probe template.

**Decision 2 (corrected, in-effect):** A.1 staleness heuristic is
"impl file has > 10 non-comment LOC". Implementation path resolution
mirrors the synced common-cpp convention:
`include/<namespace>/<rest>.hpp` → `src/<rest>.cpp` (the first directory
component after `include/` is the project namespace, stripped).
Verified against `alembic_writer.hpp`, `vk/context.hpp`, and
`vdb_writer.hpp` on 2026-05-15.

### E.2 Annotation grammar reason cannot contain semicolons

The spec's drafted classifier reason string contained a semicolon
inside parentheses (`...canonical spec § 12 row 5; tracked for
migration...`). The annotation grammar regex in
`tools/integrity/integrity/common/annotations.py:30-35` treats `;`
as the field separator for `<check_id>; <reason>; <issue_ref>`, so
the inserted annotations did not parse and `apply_suppressions` did
not suppress the new findings.

Resolved by replacing the embedded `;` with ` -- ` in the classifier
reason. Verified by re-running the sweep and observing 2 suppressed
findings. The grammar has no escape syntax for embedded semicolons;
future classifier rules need to avoid them in reason strings.
Candidate v1.2 item: grammar extension for semicolon escape (or a
linter rule against embedded `;` in classifier reasons).

### E.3 Fixture path layout

Decision 2's correction (E.1) implies the fixture trees also follow the
strip-namespace convention. The fixture impl file at
`bad_stub_label/common/common-cpp/src/widget.cpp` (not
`.../src/widget/widget.cpp`) corresponds to the header at
`bad_stub_label/common/common-cpp/include/widget/widget.hpp` — the
`widget/` directory after `include/` plays the role of `gpusims/` in
production and is stripped during impl resolution. Same shape applied
to the good fixture.

---

## F. Numbers at a glance

| Metric | Pre-commit | Post-commit |
|---|---|---|
| Total tests | 74 | 80 |
| `cat2.stub-label-stale` findings (hard-fail) | n/a | 0 |
| `cat2.stub-label-stale` findings (suppressed) | n/a | 2 |
| Total suppressed | 1126 | 1128 |
| Integrity strict-mode exit | 0 | 0 |

## G. Next commits

- **Commit 2** — A.5 markdown fenced-block awareness in the annotation
  parser and suppressor. Expected to drop `cat1.annotation-form`
  findings from ~69 to ~28.
- **Commit 3** — A.7 (`--grandfather-report` and `--state-snapshot`),
  A.8 (per-category live tallies in the catalog headings), and 5.B
  (`python` → `python3` docs sweep).
- **SHA back-fill** — after all three commits land, update each audit
  report's "Companion to:" lines with the actual SHAs of the other two
  landing commits.

End of commit 1 audit report.
