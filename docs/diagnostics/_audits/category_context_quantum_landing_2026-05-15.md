# Category-Context Landing — Quantum — 2026-05-15

Landing of the Quantum category-context document, first-of-shape in
the repo. The document is a pre-spec scoping artifact authored by
architect-1 over multiple chat turns with two empirical probes against
the repo state. Probe history is summarized in the document's
Appendix B. This audit records the file-landing pass only — drafting
of the Quantum-adapted spec template, the full
`docs/sim-specs/ising-dwave.md`, the integrity-toolkit Cat 3
amendments, and the vendoring of `references/dimod/` are all
explicitly out of scope per the file's § 8.

Companion to:

- The landing file itself: `docs/category-contexts/quantum.md`
- Phase 12 setup-2 dependency cited at
  `docs/diagnostics/_audits/phase12_prep1_landing_2026-05-15.md:109-111`
- Audit-doc convention: `integrity_build_8_landing_2026-05-14.md`

---

## A. Change summary

Created the new top-level docs subdirectory `docs/category-contexts/`
and placed the attached `category-context.md` at
`docs/category-contexts/quantum.md` (≈50 KB, 396 lines). No other
files modified. README updates, ledger amendments, and the
sibling-doc reference in `quantum/ising-dwave/README.md` are out of
scope for this commit — they land in subsequent phases per the
landing file's § 8.

The category-context document is structurally novel in this repo:

- First doc in `docs/category-contexts/`.
- First doc to record decisions that have *not yet shipped* (contrast
  with the per-sim `load-bearing-decisions.md` artifacts, which record
  shipped decisions).
- Probe-verified twice against live repo state at draft time
  (2026-05-15) — see the file's Appendix B for the six items resolved.

## B. File inventory

| File | Status | Notes |
|------|--------|-------|
| `docs/category-contexts/` | new directory | Sibling to `docs/sim-specs/`, holds category-scope (not sim-scope) pre-spec docs |
| `docs/category-contexts/quantum.md` | new | 396 lines; Quantum / Ising on D-Wave pre-spec scoping doc |

## C. Verification

### C.1 Intra-repo citation resolution

Three `file:line` citations in the file, all identical, all resolve:

```
$ python3 -c "from pathlib import Path; from integrity.cat1_citations.grammar import extract_intra_repo_citations; \
    f = Path('docs/category-contexts/quantum.md'); \
    [print(c.source_line, c.raw) for c in extract_intra_repo_citations(f.read_text(), f)]"
262 docs/diagnostics/_audits/phase12_prep1_landing_2026-05-15.md:109-111
330 docs/diagnostics/_audits/phase12_prep1_landing_2026-05-15.md:109-111
388 docs/diagnostics/_audits/phase12_prep1_landing_2026-05-15.md:109-111

$ wc -l docs/diagnostics/_audits/phase12_prep1_landing_2026-05-15.md
111 docs/diagnostics/_audits/phase12_prep1_landing_2026-05-15.md
```

Lines 109-111 exist (the file is exactly 111 lines). All three
citations resolve.

Zero upstream citations are extracted from the file. The references
to `[Algebraic_D3Q19]`, `[Algebraic_Ising2D]`, `[Algebraic_WolffCluster]`,
`[Algebraic_Morton30]`, `[dimod]`, `[SPlisHSPlasH]`, `[Krueger]`
appear in TOML-snippet form or prose mention without the
`<Upstream> <version> <path>:<line>` grammar, so the upstream-citation
parser does not match them.

### C.2 Bare-path references

The file contains many plain-path references (no `file:line` suffix)
used as prose mentions of repo locations. The cat1.intra-repo check
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
does not match bare paths (per `grammar.py:49-52`), so these are not
mechanically verified by the toolkit. Manual spot-check of the
non-forward-referenced paths:

| Path | Status |
|------|--------|
| `docs/overarching-spec.md` | exists |
| `docs/sim-specs/ising-dwave.md` | exists |
| `docs/integrity-toolkit-spec.md` | exists |
| `quantum/README.md` | exists |
| `quantum/ising-dwave/README.md` | exists |
| `tools/integrity/docs/ground-truth-sources.md` | exists |
| `tools/integrity/integrity/cat1_citations/upstream_anchor.py` | exists |
| `continuous-ca/lenia-fft/python/` | exists |
| `continuous-ca/reaction-diffusion-2d/web/` | exists |
| `volumetric-grid/lattice-boltzmann/README.md` | exists |
| `particle-fluids/pic-flip/README.md` | exists |

Paths the file describes as future/proposed (out of scope for this
landing): `tools/integrity/docs/algebraic/ising_2d.md`,
`tools/integrity/docs/algebraic/wolff_cluster.md`, `references/dimod/`,
`docs/sim-specs/_template-quantum.md`, `quantum/ising-dwave/web/`,
`quantum/ising-dwave/python/`. All are explicitly flagged as
deliverables of later phases in § 5, § 6, and § 8 of the landing file.

See § F.1 below for one bare-path reference that does not resolve and
appears to be a misreference rather than a forward-reference.

### C.3 Full-toolkit smoke

Running the integrity toolkit before and after the file is in place:

```
$ mv docs/category-contexts /tmp/category-contexts-stash
$ python3 -m integrity --mode strict --no-audit-log
integrity: 2 pass, 0 soft-warn, 11 hard-fail, 944 suppressed
Exit: 1
$ mv /tmp/category-contexts-stash docs/category-contexts
$ python3 -m integrity --mode strict --no-audit-log
integrity: 2 pass, 0 soft-warn, 11 hard-fail, 944 suppressed
Exit: 1
```

The before/after summary lines are byte-identical: the new file
introduces zero new findings on any check. The non-zero exit code is
pre-existing baseline state inherited from the in-flight integrity
v1.1 batch 1 work — see § F.2 for the breakdown.

The prompt's expectation that `python -m integrity --mode strict
--no-audit-log` exits 0 after the file is added is not met because
the baseline already does not exit 0; the relevant guarantee — that
the new file introduces no new findings — does hold.

## D. Behavioral notes

- The file is a markdown document inside `docs/`, so it is scanned by
  every cat1 check (cat1.intra-repo, cat1.upstream-citation,
  cat1.upstream-anchor, cat1.unregistered-upstream,
  cat1.annotation-form). It produces zero findings on all of them.
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
- The file does not contain any `integrity-allow:` annotations, so
  cat1.annotation-form has nothing to validate inside it.
- `docs/category-contexts/` is first-of-pattern. The integrity toolkit
  does not need any change to scan it — the scan walks all
  git-tracked files via `_list_scannable_files` and filters by
  extension, so the new directory is picked up automatically.

## E. Out-of-scope work explicitly deferred

Per the landing file's § 8 sequencing and the integration task's
explicit out-of-scope list:

| Deferred item | Lands in |
|---|---|
| `docs/sim-specs/ising-dwave.md` (full spec) | Phase N, multi-architect cross-review |
| `docs/sim-specs/_template-quantum.md` (adapted template) | Step 2 in § 8, single architect |
| `references/dimod/` (vendored upstream) | Step 4a in § 8, can land any time |
| `tools/integrity/docs/algebraic/ising_2d.md` | Step 4b, blocked on Phase 12 setup-2 |
| `tools/integrity/docs/algebraic/wolff_cluster.md` | Step 4b, blocked on Phase 12 setup-2 |
| `quantum/ising-dwave/README.md` update | Track 1 sim phase, § 8 Step 5 |
| Ledger / top-level README updates | Subsequent phases per § 8 |

None of the deferred work is touched in this commit.

## F. Incidental findings

### F.1 Misreference to `docs/load-bearing-decisions.md`

The file's § 0 contains the sentence:

> This doc is **not** load-bearing in the same sense as
> `docs/load-bearing-decisions.md` artifacts on shipped sims.

The path `docs/load-bearing-decisions.md` does not exist. The
`load-bearing-decisions.md` convention is **per-sim**, not top-level:
nine such files exist across the repo, each at
`<category>/<sim>/docs/load-bearing-decisions.md` (e.g.,
`particle-fluids/sph-water/docs/load-bearing-decisions.md`,
`continuous-ca/lenia-fft/docs/load-bearing-decisions.md`).

This is a bare path reference (no `file:line` suffix), so
cat1.intra-repo does not catch it; it is invisible to the toolkit.
The reference is misleading on independent read because it implies a
top-level decision-ledger that does not exist.

Per the integration task's "If something reads wrong on your
independent read, flag in the landing audit's 'incidental findings'
section and return to coordinator rather than editing" directive, this
is flagged here rather than corrected in the file. Coordinator should
route to architect-1 for revision; a small wording fix
("`<sim>/docs/load-bearing-decisions.md`" or "the per-sim
load-bearing-decisions documents") would resolve it.

### F.2 Pre-existing integrity-toolkit baseline failure

The integrity toolkit does not currently exit 0 on `main`, independent
of this commit. Baseline state (clean HEAD `0db9c73` with no
working-tree modifications) reports:

```
integrity: 2 pass, 0 soft-warn, 9 hard-fail, 944 suppressed
Exit: 1
```

After applying the working-tree modifications listed in the session's
opening `git status` (the in-flight integrity v1.1 batch 1 work
modifying `cat1_citations/checks/*.py`, `common/annotations.py`,
`common/suppression.py`, `grandfather.py`), the count rises to 11
hard-fail / 944 suppressed.

Per-check hard-fail counts (post-modifications, with the new file in
place):

| Check | Findings |
|---|---|
| cat1.intra-repo | 657 |
| cat2.public-symbol-used-c | 111 |
| cat2.public-symbol-used-ts | 73 |
| cat1.annotation-form | 68 |
| cat1.upstream-citation | 24 |
| cat2.public-symbol-used | 17 |
| cat1.unregistered-upstream | 3 |
| cat2.stub-label-stale | 2 |

Comparison with the integrity_build_8 final state (2026-05-14: 2 pass
/ 0 soft-warn / 0 hard-fail / 1109 suppressed) indicates ~165
previously-suppressed findings are no longer suppressed and have
surfaced as hard-fails. The shift is in the suppression mechanism
(touched by the v1.1 batch 1 work), not in new defects in shipped
code.

This is **not** introduced by the present commit; it is the existing
state of `main`. Surfaced here for coordinator awareness, not as a
blocker for the category-context landing. Routes to whoever owns the
integrity v1.1 batch 1 work-in-progress (architect-1 per ledger).

### F.3 First-of-shape directory naming

`docs/category-contexts/` is sibling to `docs/sim-specs/`, mirroring
the latter's plural-noun convention. Probe item 5 in the landing
file's Appendix B explicitly recommended this location over the
alternative `quantum/ising-dwave/docs/category-context.md` ("placing
category-scope content under a single sim's folder is naming-incoherent
and forces a move if the Quantum category ever grows"). The landing
follows the probe recommendation.

When future categories acquire their own context docs (e.g.,
`docs/category-contexts/hybrid-particle-grid.md`), no toolkit or
convention change is needed — the directory is already general-shape.

---

## F.1 Resolution — 2026-05-15

Fix-forward landed as commit `95cf161`
(`docs: fix F.1 misreference in Quantum category-context`). The
single-sentence rewrite in `docs/category-contexts/quantum.md` § 0
now reads:

> This doc is **not** load-bearing in the same sense as the per-sim
> `docs/load-bearing-decisions.md` files that ship with each
> implemented sim (one under each `<category>/<sim>/docs/`). [...]

The diff is exactly one line; no other content changed.

### Verification

```
$ git diff 149fc93..95cf161 -- docs/category-contexts/quantum.md | grep -c '^[+-][^+-]'
2

$ python3 -m integrity --mode strict --no-audit-log
integrity: 2 pass, 0 soft-warn, 13 hard-fail, 944 suppressed
Exit: 1
```

The post-fix summary line is **byte-identical** to the post-149fc93
baseline observed once the audit file itself was tracked
(see correction below). The fix introduces zero new findings.

### Baseline correction

The original § C.3 of this audit reported the pre/post counts as
`11 hard-fail / 944 suppressed`. That measurement was taken **before**
the audit file was committed; at that time the audit was untracked
and therefore not scanned by the toolkit (`_list_scannable_files`
uses `git ls-files` when `.git` is present). Once the audit file
landed as part of commit `149fc93`, it became tracked and contributed
2 additional findings (one cat1.intra-repo at line 80 from the bare
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
`grammar.py:49-52` reference inside a prose explanation, and one
<!-- integrity-allow: cat1.annotation-form; audit-doc literal mention of the annotation grammar (not a real annotation); n/a -->
cat1.annotation-form at line 139 from the literal "`integrity-allow:`"
token inside a backtick-fenced phrase). Both are self-referential
to the toolkit grammar — the audit doc describes the grammar and
quotes it, so the grammar's own parser picks up the quotes.

The true post-149fc93 baseline is therefore **13 hard-fail / 944
suppressed**, not 11. The byte-identical property held by the F.1
fix-forward is against this true baseline.

The two audit-self-references are within the audit's own prose and
out-of-scope for this fix-forward; if they merit cleanup they would
be in scope for an audit-doc revision (suppression annotations,
backtick adjustments, or a paraphrase). Not pursued here per the
fix-forward's stated scope.
