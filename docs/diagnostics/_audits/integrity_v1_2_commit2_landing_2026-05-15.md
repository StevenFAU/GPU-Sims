---
title: "Integrity v1.2 Commit 2 — P1.5 Register cat3.d3q19-* Checks"
date: 2026-05-15
author: architect1-via-claude-code
status: landed
sibling-docs:
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_bolt_ons_spec_2026-05-15_architect1.md
  - docs/diagnostics/_audits/integrity_v1_2_commit1_landing_2026-05-15.md
---

# Integrity v1.2 Commit 2 — P1.5 Register cat3.d3q19-* Checks

## § A. Change summary

P1.5 wires the existing `d3q19_verify.py` algebraic harness into the
check-discovery surface as three independent CHECK_IDs:
`cat3.d3q19-velocity-set`, `cat3.d3q19-weights`, `cat3.d3q19-equilibrium`.
Closes the registry-vs-implementation drift surfaced as
Convention #8 firing #9 in
`docs/diagnostics/_audits/phase12_substantive_landing_2026-05-15.md`
(probe § B.5 verified the three CHECK_IDs returned zero registry
matches at probe SHA `9add149`).

The harness was relocated from `cat3_numerical/checks/d3q19_verify.py`
to `cat3_numerical/d3q19_verify.py` to mirror the `cat3.cubic-kernel`
algorithmic/check split (probe § C.2 + § C.3). The expected-values
JSON moved alongside. The harness gained three new top-level functions
(`verify_velocity_set`, `verify_weights`, `verify_equilibrium`) plus
a JSON loader (`load_expected_payload`) — these are the integration
surface the check modules consume.

## § B. File inventory

- `tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py` →
  `tools/integrity/integrity/cat3_numerical/d3q19_verify.py` — moved
  (git mv preserves history).
- `tools/integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.expected.json` →
  `tools/integrity/integrity/cat3_numerical/d3q19_equilibrium.expected.json` —
  moved.
- `tools/integrity/integrity/cat3_numerical/d3q19_verify.py` — added
  `load_expected_payload()`, `verify_velocity_set()`, `verify_weights()`,
  `verify_equilibrium()` top-level helpers; updated `"source"` field in
  the JSON-write block.
- `tools/integrity/integrity/cat3_numerical/checks/d3q19_velocity_set.py` —
  new, ~38 LOC.
- `tools/integrity/integrity/cat3_numerical/checks/d3q19_weights.py` —
  new, ~38 LOC.
- `tools/integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.py` —
  new, ~44 LOC.
- `tools/integrity/integrity/cat3_numerical/checks/__init__.py` —
  modified; registers the three new check modules.
- `tools/integrity/tests/test_cat3_d3q19.py` — new, ~150 LOC; 12 test
  functions covering harness re-derivation, check-module pass on the
  real payload, and check-module fail on deliberately corrupted
  payloads.
- `docs/diagnostics/_audits/integrity_v1_2_commit2_landing_2026-05-15.md` —
  this landing report.

## § C. Verification

### C.1 Standalone harness

```
$ python3 tools/integrity/integrity/cat3_numerical/d3q19_verify.py
=== D3Q19 algebraic verification harness ===
  built 19 velocity vectors from {-1,0,1}^3 with |c|^2 <= 2
  canonical ordering matches d3q19.md section 2.2 table: OK
  sum(omega_i) = 1 (exact rational): OK
  second-moment tensor exactly (1/3) delta_{alpha,beta}: OK
  opposite-direction involution matches: [...]
  ...
  tp4 density linear-scaling check (feq(2.5,u) == 2.5*feq(1.0,u)): OK
  wrote tools/integrity/integrity/cat3_numerical/d3q19_equilibrium.expected.json
=== ALL CHECKS PASS ===
```

### C.2 Tests

```
$ cd tools/integrity && python3 -m pytest tests/test_cat3_cubic_kernel.py tests/test_cat3_d3q19.py -v
21 passed in 24.13s
```

9 existing cubic-kernel tests + 12 new d3q19 tests = 21 total.

### C.3 Check discoverability

```
$ python3 -m integrity --check cat3.d3q19-velocity-set --no-audit-log
integrity: 1 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
$ python3 -m integrity --check cat3.d3q19-weights --no-audit-log
integrity: 1 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
$ python3 -m integrity --check cat3.d3q19-equilibrium --no-audit-log
integrity: 1 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
```

All three checks discoverable and passing.

### C.4 Gate state

Pre-commit and post-commit gate state is unchanged from the
post-A.3 / pre-A.3-commit-4 baseline (652 hard-fail) — the new
checks only add to the `pass` count (now +3) and don't introduce
new findings.

## § D. Cross-references

- Probe § B.1 — current `d3q19_verify.py` content (relocated &
  refactored here).
- Probe § B.4 — `[Algebraic_D3Q19]` `used_by_checks` entries in
  `ground-truth-sources.md` confirming the three target CHECK_IDs.
- Probe § B.5 — registry gap (CHECK_IDs not yet wired); now closed.
- Probe § C.1–§ C.7 — `cat3.cubic-kernel` precedent pattern.
- Addendum § 4.3 — load-bearing decision for three separate check
  modules instead of one module-per-three-CHECK_IDs.

## § E. Banked observations

- **Different serialization formats per cat3 check.** Cubic-kernel
  uses `expected_values.toml`; d3q19 uses `d3q19_equilibrium.expected.json`.
  Probe § C.4 flagged this; no action recommended here, but future
  cat3 checks should decide on a unified format. **Bank as v1.3
  candidate.**
- **No pause-and-surface fired during commit 2 execution.**

## § F. Next commit

Commit 3 (P1.6) — strict-mode human-renderer suppressed-stanza
filter. SHA back-fill at commit 5.
