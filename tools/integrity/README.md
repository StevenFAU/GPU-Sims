# GPU-Sims Integrity Toolkit

Cross-stack verification toolkit per `docs/integrity-toolkit-spec.md`.

## What it checks

- **Category 1: Citation integrity** — every `file:line` citation resolves; upstream citations match vendored references
- **Category 2: Contract verification** — every public API field, function, and declared behavior has matching implementation
- **Category 3: Numerical correctness** — implementations of upstream algorithms match the upstream reference at canonical test points

## Running locally

```bash
# Install (editable, with dev deps):
pip install -e tools/integrity[dev]

# Run all checks (strict — honors HARD_FAIL):
python -m integrity

# Local-development mode (downgrades HARD_FAIL to warnings):
python -m integrity --mode warn-only

# Run a single category or check:
python -m integrity --cat 1
python -m integrity --check cat1.upstream-anchor

# JSON output:
python -m integrity --output json

# GitHub Actions annotation output:
python -m integrity --output github
```

## Running the toolkit's own tests

```bash
pytest tools/integrity/tests/ -v
```

## On failure

CI failures appear as:

1. GitHub Actions inline PR annotations
2. Entries in `docs/diagnostics/_audits/integrity_failures_<YYYY-MM-DD>.md`

To suppress a finding with an inline annotation (per spec § 3.2):

```cpp
// integrity-allow: cat1.upstream-anchor; SPlisHSPlasH 1.8.10 anchor pre-v1; #117
```

See `tools/integrity/docs/failure-modes.md` and `tools/integrity/docs/grandfather-catalog.md` for details.

## Implementation status

- [x] Commit 1: scaffold (this commit)
- [ ] Commit 2: Cat 1 citation parsing + intra-repo resolution
- [ ] Commit 3: Cat 1 upstream-citation + anchor verification
- [ ] Commit 4: grandfather sweep + CI integration
- [ ] Commit 5: Cat 2 Stack D
- [ ] Commit 6: Cat 2 Stack C
- [ ] Commit 7: Cat 2 Stack B
- [ ] Commit 8: Cat 3 cubic-kernel
