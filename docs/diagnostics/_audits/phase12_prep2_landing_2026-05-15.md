# Phase 12 setup-2 landing audit: Algebraic_D3Q19 derivation + verifier

**Date:** 2026-05-15
**Phase:** 12 (lattice-boltzmann), setup commit 2

## Scope

Land the algebraic ground-truth derivation for the D3Q19 lattice
Boltzmann method paired with the [Krueger] anchor from setup-1, plus
an independent first-principles verification harness that re-derives
the 19 velocity vectors and 3 weight values from scratch and emits
the per-test-point expected `feq()` values.

## Files

| Path | Kind | Notes |
|---|---|---|
| `tools/integrity/docs/algebraic/d3q19.md` | new | Derivation contract (7 sections, ~250 lines) |
| `tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py` | new | Independent verifier; emits expected.json |
| `tools/integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.expected.json` | new (generated) | Per-test-point feq table |
| `tools/integrity/docs/ground-truth-sources.md` | modified | Added `[Algebraic_D3Q19]` stanza + notes paragraph |
| `tools/integrity/integrity/cat1_citations/upstream_anchor.py` | modified | Skip algebraic entries (no `vendor_root`) in loader |

## Verifier stdout (verbatim)

```
=== D3Q19 algebraic verification harness ===
  built 19 velocity vectors from {-1,0,1}^3 with |c|^2 <= 2
  canonical ordering matches d3q19.md section 2.2 table: OK
  sum(omega_i) = 1 (exact rational): OK
  second-moment tensor exactly (1/3) delta_{alpha,beta}: OK
  opposite-direction involution matches: [(0, 0), (1, 2), (3, 4), (5, 6), (7, 10), (8, 9), (11, 14), (12, 13), (15, 18), (16, 17)]

--- Hand-check: test point 2 (rho=1.0, u=(0.1, 0, 0)) ---
  feq[ 0] c=(0, 0, 0)      = 0.328333333333333  (want 0.328333333333333)
  feq[ 1] c=(1, 0, 0)      = 0.073888888888889  (want 0.073888888888889)
  feq[ 2] c=(-1, 0, 0)     = 0.040555555555556  (want 0.040555555555556)
  feq[ 3] c=(0, 1, 0)      = 0.054722222222222  (want 0.054722222222222)
  feq[ 4] c=(0, -1, 0)     = 0.054722222222222  (want 0.054722222222222)
  feq[ 5] c=(0, 0, 1)      = 0.054722222222222  (want 0.054722222222222)
  feq[ 6] c=(0, 0, -1)     = 0.054722222222222  (want 0.054722222222222)
  feq[ 7] c=(1, 1, 0)      = 0.036944444444444  (want 0.036944444444444)
  feq[ 8] c=(1, -1, 0)     = 0.036944444444444  (want 0.036944444444444)
  feq[ 9] c=(-1, 1, 0)     = 0.020277777777778  (want 0.020277777777778)
  feq[10] c=(-1, -1, 0)    = 0.020277777777778  (want 0.020277777777778)
  feq[11] c=(1, 0, 1)      = 0.036944444444444  (want 0.036944444444444)
  feq[12] c=(1, 0, -1)     = 0.036944444444444  (want 0.036944444444444)
  feq[13] c=(-1, 0, 1)     = 0.020277777777778  (want 0.020277777777778)
  feq[14] c=(-1, 0, -1)    = 0.020277777777778  (want 0.020277777777778)
  feq[15] c=(0, 1, 1)      = 0.027361111111111  (want 0.027361111111111)
  feq[16] c=(0, 1, -1)     = 0.027361111111111  (want 0.027361111111111)
  feq[17] c=(0, -1, 1)     = 0.027361111111111  (want 0.027361111111111)
  feq[18] c=(0, -1, -1)    = 0.027361111111111  (want 0.027361111111111)
  sum(feq)       = 1.000000000000000  (want 1.0)
  sum(feq*c_x)   = 0.100000000000000  (want 0.1)
  sum(feq*c_y)   = 0.000000000000000  (want 0.0)
  sum(feq*c_z)   = 0.000000000000000  (want 0.0)

  tp1_zero_velocity: rho=1.0 u=(0.0, 0.0, 0.0) mass=1.000000000000000 mom=(0.000000000000000, 0.000000000000000, 0.000000000000000)
  tp3_oblique_xy: rho=1.0 u=(0.05, 0.05, 0.0) mass=1.000000000000000 mom=(0.050000000000000, 0.050000000000000, 0.000000000000000)
  tp4_density_scaled: rho=2.5 u=(0.05, 0.0, 0.0) mass=2.500000000000000 mom=(0.125000000000000, 0.000000000000000, 0.000000000000000)
  tp4 density linear-scaling check (feq(2.5,u) == 2.5*feq(1.0,u)): OK

  wrote tools/integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.expected.json

=== ALL CHECKS PASS ===
```

## Verifier responsibilities (mapped to prompt step 2)

- **(a)** velocity-set enumeration from `{-1,0,1}^3` with `|c|^2 <= 2`,
  sorted into the canonical ordering from § 2.2 (rest, then six face
  neighbors `+x,-x,+y,-y,+z,-z`, then xy/xz/yz edges in `++,+-,-+,--`
  order). Compared against an explicit reference list — match exact.
- **(b)** weights assigned by squared norm: `1/3, 1/18, 1/36` for
  norms `0, 1, 2`.
- **(c)** sanity checks from § 3.3: `sum(ω_i) = 1` exactly (rational
  arithmetic via `fractions.Fraction`); second-moment tensor
  `Σ ω_i c_iα c_iβ = (1/3) δ_αβ` for ALL `(α, β) ∈ {x,y,z}^2`
  including yy, zz, and off-diagonals.
- **(d)** BGK `feq(ρ, u)` evaluator per § 4.1 formula.
- **(e)** evaluated at all four test points (zero, `u=(0.1,0,0)`,
  oblique `u=(0.05,0.05,0)`, density-scaled `ρ=2.5 u=(0.05,0,0)`).
- **(f)** hand-check table for test point 2 verified to 1e-12;
  conservation sums verified exactly to 1e-12.
- **(g)** opposite-direction involution derived by `-c_i` lookup and
  matched against the explicit pair list from § 2.2.
- **(h)** JSON expected blob written to
  `cat3_numerical/checks/d3q19_equilibrium.expected.json`.

## Toolkit gate

```
$ python3 -m integrity --check cat1.upstream-anchor
integrity: 1 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
```

The new `[Algebraic_D3Q19]` registry entry has no `vendor_root` and
is therefore not subject to upstream-anchor verification. The loader
in `cat1_citations/upstream_anchor.py` was updated to skip entries
missing `vendor_root` (algebraic registrations live alongside vendored
ones in the same TOML block, but only vendored ones get parsed into
the `UpstreamRegistration` dataclass).

## Diff stats (this commit only)

```
 tools/integrity/docs/algebraic/d3q19.md                           | + (new, ~250 lines)
 tools/integrity/docs/ground-truth-sources.md                      |  19 +++++++
 tools/integrity/integrity/cat1_citations/upstream_anchor.py       |   5 +
 tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py   | + (new, ~230 lines)
 tools/integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.expected.json | + (generated)
```

Other pre-existing working-tree modifications (cat1 checks tweaks,
suppression/grandfather work) are unrelated to setup-2 and not
included in this commit.

## References

- `docs/diagnostics/_audits/phase12_lbm_predraft_probe_2026-05-15_architect1.md`
  — Krüger D2Q9-only finding that motivated the algebraic route.
- `tools/integrity/docs/algebraic/d3q19.md` — the derivation itself.
- `tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py`
  — the independent verifier used at land time.
