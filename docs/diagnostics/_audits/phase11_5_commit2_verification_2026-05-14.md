# Phase 11.5 Commit-2 Verification Probe — 2026-05-14

Read-only verification of three claims in the architect-1 commit-2 spec.
No file modifications, no commits, no builds, no binary runs.

---

## A. Claim 1 — GPU gradient kernel is 6× too large vs upstream

**Claim.** Our shader's `kernel_gradW` produces a result 6× larger than upstream
`CubicKernel::gradW` at any non-trivial input.

### Numerical evaluation

Both upstream and shader symbolic logic transcribed from:
- `references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h:62-85`
- `particle-fluids/sph-water/shaders/density_alpha.comp.glsl:73-78`

With `h = 0.04`:
- `m_l       = 48 / (π · h³) = 48 / (π · 6.4e-5) = 2.38732e+05`
- `grad_kern_norm = 48 / (π · h⁴) = 48 / (π · 2.56e-6) = 5.96831e+06`

#### Test point 1: r = (0.02, 0, 0), q = 0.5

- Upstream (`q <= 0.5` branch): `res = m_l · q · (3q-2) · gradq`
  = `m_l · 0.5 · (-0.5) · (25, 0, 0)`
  = `m_l · (-6.25)` along x
  = **-1.49208e+06** along x
- Shader (`q < 0.5` is false at q=0.5; takes `q < 1.0` branch):
  `omq = 0.5`, `poly = -6 · 0.5 · 0.5 = -1.5`
  return `(grad_kern_norm · -1.5 / 0.02) · (0.02, 0, 0)`
  = `grad_kern_norm · -1.5` along x
  = **-8.95247e+06** along x
- **Ratio shader/upstream = 6.00000.**

#### Test point 2: r = (0.01, 0, 0), q = 0.25

- Upstream (`q <= 0.5` branch): `res = m_l · 0.25 · (0.75-2) · gradq`
  = `m_l · 0.25 · (-1.25) · (25, 0, 0)`
  = `m_l · (-7.8125)` along x
  = **-1.86510e+06** along x
- Shader (`q < 0.5` branch): `poly = 18q² - 12q = 1.125 - 3 = -1.875`
  return `(grad_kern_norm · -1.875 / 0.01) · (0.01, 0, 0)`
  = `grad_kern_norm · -1.875` along x
  = **-1.11906e+07** along x
- **Ratio shader/upstream = 6.00000.**

### Algebraic confirmation

Shader factor / upstream factor at both q:

```
(48/(π·h⁴) · -1.5)  /  (48/(π·h³) · -6.25)   =  (1.5 / 6.25) / h  =  0.24 · 25  =  6
(48/(π·h⁴) · -1.875) /  (48/(π·h³) · -7.8125) =  (1.875/7.8125)/ h =  0.24 · 25  =  6
```

The ratio is exactly `1/h · (shader_poly / upstream_poly_with_gradq)` and the
shader has effectively folded `1/h` into the norm (`h⁴` vs `h³`) but failed to
divide it out elsewhere — i.e. the host should be passing `8/(π·h⁴)` (so the
shader's `poly · norm` equals upstream's `m_l · q · (3q-2) · 1/h`), or the
shader should be using `h³` and dividing by `r_mag` and multiplying by `gradq`
separately. The cleaner host-side fix is the factor-of-6 reduction:
`48/(π·h⁴) → 8/(π·h⁴)`.

### Verdict

**CLAIM_CONFIRMED.** Ratio is exactly 6 at both test points (6.00000 to 6 sig
figs). Commit 1 is a one-line host fix.

---

## B. Claim 2 — upstream `compute_aij_pj` reads pressure-accel that's updated each iteration

**Claim.** Inside `pressureSolveIteration` / `divergenceSolveIteration`, the call
to `computePressureAccel` writes to the same storage that `compute_aij_pj`
subsequently reads.

### (a) Call site inside `pressureSolveIteration`

`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:566-571`:

```cpp
		#pragma omp for schedule(static) 
		for (int i = 0; i < numParticles; i++)
		{
			computePressureAccel(fluidModelIndex, i, density0, m_simulationData.getPressureRho2Data());
		}
```

### (b) `computePressureAccel` write target

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`TimeStepDFSPH.cpp:1306-1307`:

```cpp
	Vector3r& ai = m_simulationData.getPressureAccel(fluidModelIndex, i);
	ai.setZero();
```

Followed by `ai += pSum * grad_p_j;` (line 1326) and three boundary-mode
`ai += a;` accumulations (lines 1341, 1350, 1360). All writes target the same
`m_simulationData.getPressureAccel(fluidModelIndex, i)` reference.

### (c) `compute_aij_pj` read source

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`TimeStepDFSPH.cpp:1383-1391`:

```cpp
	const Vector3r& xi = model->getPosition(i);
	const Vector3r& ai = m_simulationData.getPressureAccel(fluidModelIndex, i);
	...
	forall_fluid_neighbors(
		const Vector3r & aj = m_simulationData.getPressureAccel(pid, neighborIndex);
```

Both the self (`ai`) and neighbor (`aj`) accelerations come from the identical
accessor — same `m_simulationData.getPressureAccel(...)` signature with the
same `(fluidIndex, particleIndex)` arguments.

### (d) Convergence of references

- Writer: `Vector3r& ai = m_simulationData.getPressureAccel(fluidModelIndex, i);` (1306)
- Reader self: `const Vector3r& ai = m_simulationData.getPressureAccel(fluidModelIndex, i);` (1384)
- Reader neighbor: `const Vector3r& aj = m_simulationData.getPressureAccel(pid, neighborIndex);` (1391)

All three reference the same per-particle storage, indexed identically by
`(fluidModelIndex, i)` (or `(pid, neighborIndex)` for the neighbor read).

### (e) Storage declaration

`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/SimulationDataDFSPH.h`:

```cpp
33:  std::vector<std::vector<Vector3r>> m_pressureAccel;
118: FORCE_INLINE Vector3r& getPressureAccel(const unsigned int fluidIndex, const unsigned int i)
120:   return m_pressureAccel[fluidIndex][i];
123: FORCE_INLINE const Vector3r& getPressureAccel(const unsigned int fluidIndex, const unsigned int i) const
125:   return m_pressureAccel[fluidIndex][i];
```

Single per-particle `Vector3r` per fluid, with both mutable and const accessors
returning references into that same `m_pressureAccel` buffer.

### Verdict

**CLAIM_CONFIRMED.** `computePressureAccel` writes and `compute_aij_pj` reads
both target `m_simulationData.m_pressureAccel[fluidModelIndex][i]`. Because
both calls are issued inside the same parallel block in
`pressureSolveIteration`, the pressure-accel is refreshed per inner iteration.
The algorithmic-mismatch foundation of the commit-2 restructure is sound.

---

## C. Claim 3 — dispatch-count delta is ~25 vs ~17 per substep

**Claim.** Architect-1's commit-2 spec Section 5 enumerates ~25 dispatches at
default `minIterDivergence=1`, `minIterDensity=2`. Probe-1 Section A counted 17
in the current chain.

### Re-counting the spec's enumerated chain

Using the prompt's enumeration verbatim:

| Group | Dispatches | Count |
|---|---|---|
| `apply_emitter` (conditional) | 1 | 1 |
| Sort/scan pipeline (morton, cell_count, prefix_sum_local, prefix_sum_block ×2, prefix_sum_block_l2, prefix_sum_addback, scatter) | 8 | 8 |
| `density_alpha` | 1 | 1 |
| **Pre-solve subtotal** | | **10** |
| `compute_density_change` | 1 | 1 |
| Divergence iter × 1: (compute_pressure_accel, compute_aij_pj_divergence, jacobi_update_divergence) | 3 | 3 |
| `compute_pressure_accel` (final, divergence) | 1 | 1 |
| `apply_velocity` (divergence) | 1 | 1 |
| **Divergence solve subtotal** | | **6** |
| Integrate forces (FORCES mode) | 1 | 1 |
| `compute_density_adv` | 1 | 1 |
| Density iter × 2: (compute_pressure_accel, compute_aij_pj_density, jacobi_update_density) | 6 | 6 |
| `compute_pressure_accel` (final, density) | 1 | 1 |
| `apply_velocity` (density) | 1 | 1 |
| **Density solve subtotal** | | **9** |
| Integrate forces (POSITION mode) | 1 | 1 |
| **TOTAL** | | **27** |

`1 + 8 + 1 + 1 + 3 + 1 + 1 + 1 + 1 + 6 + 1 + 1 + 1 = 27`

### Verdict

**CLAIM_REFUTED (minor correction).** Spec's "~25" is off by 2 — actual count
from the spec's own enumeration is 27. This is a small correction. The
~1.5× current frame-time expectation should be re-derived from 27/17 ≈ 1.59×,
which is still consistent with the spec's "~1.5×" qualitative estimate but
nudges the upper bound slightly. Nothing structurally wrong with the proposed
chain.

---

## Summary

| Claim | Verdict |
|---|---|
| 1. GPU gradient kernel is 6× too large | **CLAIM_CONFIRMED** (ratio = 6.00000 at both q=0.5 and q=0.25) |
| 2. Upstream compute_aij_pj reads per-iteration pressure-accel | **CLAIM_CONFIRMED** (single `m_pressureAccel` storage, both call sites use identical accessor) |
| 3. Dispatch count is ~25 | **CLAIM_REFUTED — minor correction** (actual count = 27, off by 2; spec's ~1.5× frame-time estimate becomes 1.59×, still qualitatively consistent) |
