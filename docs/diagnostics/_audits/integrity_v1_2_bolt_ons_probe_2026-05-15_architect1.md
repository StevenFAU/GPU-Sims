---
title: "Integrity v1.2 Bolt-Ons Pre-Spec Probe"
date: 2026-05-15
author: architect1-via-claude-code
status: probe
scope: read-only
sibling-docs:
  - docs/retro/integrity-toolkit-v1.1-batch1.md
  - docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
  - docs/diagnostics/_audits/integrity_v1_1_self_review_probe_2026-05-15_architect1.md
  - docs/diagnostics/_audits/phase12_prep2_landing_2026-05-15.md
  - docs/diagnostics/_audits/phase12_substantive_landing_2026-05-15.md
  - docs/diagnostics/_audits/integrity_v1_1_post_batch_triage_2026-05-15.md
---

# Integrity v1.2 Bolt-Ons Pre-Spec Probe — 2026-05-15

Read-only probe to ground the v1.2 bolt-ons execution spec
covering four items from
`docs/retro/integrity-toolkit-v1.1-batch1-addendum.md` § 5:

- **P1.5** — Register `cat3.d3q19-velocity-set` /
  `cat3.d3q19-weights` / `cat3.d3q19-equilibrium`.
- **P1.6** — Strict-mode human-renderer suppressed-stanza filter.
- **P1.7** — `stub_label_stale.py` module-docstring drift.
- **P1.8** — Grandfather-sweep live-source protection (surfaced
  by the over-sweep pause-and-surface during commit `9add149`).

No files modified, no commits, no pushes, no builds, no binary
runs. All line-number citations come from `grep -n` / Read in
this probe run; no line numbers carried forward from prior
prompts.

---

## Section A — Gate state at probe time

### A.1 — HEAD

FACT: `git rev-parse HEAD` = `9add1494b237e33f3dda782c821b9d7f29446068` (short: `9add149`).

### A.2 — Strict-mode invocation

`python3 -m integrity --mode strict --no-audit-log` was run with
output captured to `/tmp/integrity_full.txt`. FACT:

- **Summary line (verbatim):** `integrity: 2 pass, 0 soft-warn, 4 hard-fail, 1046 suppressed`
- **Total output lines:** 2101
- **Total stanza header lines emitted** (`HARD_FAIL` / `SOFT_WARN`
  prefix at column 3): **1050** (= 4 hard-fail + 1046 suppressed).
- **Exit code:** 1 (matches `EXIT_HARD_FAIL` per
  `runner.py:26` and `runner.py:191-192`).

**First 20 stanzas (heads, lines 2-21 of output, verbatim):**

```
  HARD_FAIL: cat1.intra-repo at CHANGELOG.md:92
    Chakazul/Lenia/Python/LeniaNDK.py:329-335: path 'Chakazul/Lenia/Python/LeniaNDK.py' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at CHANGELOG.md:154
    context.hpp:78: path 'context.hpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at CHANGELOG.md:154
    context.cpp:116: path 'context.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at CHANGELOG.md:154
    context.cpp:202: path 'context.cpp' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at common/common-py/examples/hello/hello/main.py:31
    kernel_impl.py:631: path 'kernel_impl.py' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/common/common-py/examples/hello/hello or /home/otacon/Projects/GPU-Sims/GPU-Sims
  HARD_FAIL: cat1.intra-repo at continuous-ca/lenia-fft/docs/load-bearing-decisions.md:236
    main.py:608: path 'main.py' does not resolve under /home/otacon/Projects/GPU-Sims/GPU-Sims/continuous-ca/lenia-fft/docs or /home/otacon/Projects/GPU-Sims/GPU-Sims
  [...continues through ~10 stanzas...]
```

(All 20 stanzas are `cat1.intra-repo` HARD_FAIL prefix on
`CHANGELOG.md` / `continuous-ca/lenia-fft/docs/…` paths; none of
these are unsuppressed gate-red findings — the human renderer is
emitting them despite the `suppressed` flag.)

**Elided middle:** lines 22-2090 inclusive (≈1015 additional
`HARD_FAIL`-prefix stanzas, all from the same suppressed pool).

**Last 5 stanzas (tail of output, verbatim):**

```
  HARD_FAIL: cat2.public-symbol-used-ts at common/common-web/src/webgpu/renderPipeline.ts:131
    public member 'RenderPipeline.fragmentPath' has no non-self consumer site under Stack B sim sources, examples, or tests
  HARD_FAIL: cat2.public-symbol-used-ts at common/common-web/src/webgpu/renderPipeline.ts:21
    public property 'RenderPipelineDesc.depthFormat' has no non-self consumer site under Stack B sim sources, examples, or tests
  HARD_FAIL: cat2.public-symbol-used-ts at common/common-web/src/webgpu/renderer.ts:65
    public method 'Renderer.beginRendering' has no non-self consumer site under Stack B sim sources, examples, or tests
  HARD_FAIL: cat2.public-symbol-used-ts at common/common-web/src/webgpu/renderer.ts:96
    public method 'Renderer.waitIdle' has no non-self consumer site under Stack B sim sources, examples, or tests
  HARD_FAIL: cat2.stub-label-stale at common/common-cpp/include/gpusims/alembic_writer.hpp:14
    "In Phase N stub" label is stale: implementation at common/common-cpp/src/alembic_writer.cpp has 82 non-comment LOC (threshold 10)
  HARD_FAIL: cat2.stub-label-stale at common/common-cpp/include/gpusims/vdb_writer.hpp:13
    "In Phase N stub" label is stale: implementation at common/common-cpp/src/vdb_writer.cpp has 114 non-comment LOC (threshold 10)
```

INFERENCE: every one of these tail stanzas is a `suppressed`
finding (covered by either inline annotations or a
`cat2-stack-{b,c,d}-unused` / `cat2-stub-label-stale` grandfather
category). The renderer's labelling of them as `HARD_FAIL` is the
P1.6 bug.

### A.3 — Comparison vs post-`9add149` landing

FACT: matches exactly. The 9add149 commit message reports
`Post-commit gate state: 4 hard-fails, unchanged from pre-commit;
suppressed 1007 -> 1046 (Delta +39)`. Probe state: 4 hard-fail,
1046 suppressed. No delta.

### A.4 — Recent commit history (`git log --oneline -10`)

```
9add149 docs(retro): self-review probe addendum to v1.1 batch-1 retro
cdad2e2 fix(lattice-boltzmann): streamline seed-slab + dt_render units (in-flight #2)
c1a257d fix(lattice-boltzmann): streamline reseed visual defects (in-flight Phase 12)
d772803 docs(audits): back-fill SHA cross-references in post-retro landing audit
e26056c docs(audits): integrity v1.1 post-retro landing audit
8fc7a08 chore(phase12): backfill substantive-commit SHA into project-state.md
a42085a fix(integrity): annotate toolkit-own grammar literals in test_suppression_fence
47104ad chore(phase12): cross-cutting edits — CI + README + CHANGELOG + project-state + capture-format
d41564d feat(lattice-boltzmann): D3Q19 BGK around a NACA airfoil — Phase 12 substantive
9c057e5 grandfather(integrity): sweep retro doc findings (v1.1 batch-1 retro companion)
```

FACT: HEAD is `9add149`. No commits have landed since.

---

## Section B — P1.5 surface: d3q19 verification machinery + registry block

### B.1 — `d3q19_verify.py` verbatim

FACT: file is 281 LOC (`wc -l` = 281). Source (head + key spans
+ tail; the interior contains the algebraic derivation
verbatim):

```python
  1	"""Independent first-principles verifier for the D3Q19 algebraic ground truth.
  2	
  3	Re-derives the 19 velocity vectors and 3 weight values from scratch, evaluates
  4	the BGK equilibrium at the four canonical test points from d3q19.md, performs
  5	the algebraic sanity checks from sections 3.3 and 4.2 of the derivation, and
  6	emits the per-direction feq table to d3q19_equilibrium.expected.json.
  7	
  8	Run-time output (stdout) is a short verification trace; on success the harness
  9	exits 0 and the JSON blob is written. Any assertion failure stops execution
 10	with a descriptive message and a nonzero exit.
 11	"""
 12	
 13	from __future__ import annotations
 14	
 15	import json
 16	import sys
 17	from pathlib import Path
 18	from fractions import Fraction
 19	
 20	
 21	HERE = Path(__file__).resolve().parent
 22	EXPECTED_JSON = HERE / "d3q19_equilibrium.expected.json"
 23	
 24	TOL_ABS = 1e-12
 25	
 26	
 27	def build_velocity_set() -> list[tuple[int, int, int]]:
 28	    """Enumerate {-1,0,1}^3 with squared L2 norm <= 2, ordered canonically."""
 [...continues with enumeration logic through line 65...]
 64	    assert len(ordered) == 19, f"expected 19 vectors, got {len(ordered)}"
 65	    return ordered
 66	
 67	
 68	EXPECTED_CANONICAL = [
 69	    (0, 0, 0),
 70	    (+1, 0, 0), (-1, 0, 0),
 71	    (0, +1, 0), (0, -1, 0),
 72	    (0, 0, +1), (0, 0, -1),
 73	    (+1, +1, 0), (+1, -1, 0), (-1, +1, 0), (-1, -1, 0),
 74	    (+1, 0, +1), (+1, 0, -1), (-1, 0, +1), (-1, 0, -1),
 75	    (0, +1, +1), (0, +1, -1), (0, -1, +1), (0, -1, -1),
 76	]
 77	
 78	EXPECTED_OPPOSITE_PAIRS = [
 79	    (0, 0),
 80	    (1, 2), (3, 4), (5, 6),
 81	    (7, 10), (8, 9),
 82	    (11, 14), (12, 13),
 83	    (15, 18), (16, 17),
 84	]
 85	
 86	
 87	def weight_for(c: tuple[int, int, int]) -> Fraction:
 88	    n2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2]
 89	    if n2 == 0:
 90	        return Fraction(1, 3)
 91	    if n2 == 1:
 92	        return Fraction(1, 18)
 93	    if n2 == 2:
 94	        return Fraction(1, 36)
 95	    raise ValueError(f"unexpected squared norm {n2} for {c}")
 96	
 97	
 98	def feq(rho: float, ux: float, uy: float, uz: float,
 99	        cs: list[tuple[int, int, int]], ws: list[float]) -> list[float]:
100	    u_dot_u = ux * ux + uy * uy + uz * uz
101	    out = []
102	    for c, w in zip(cs, ws):
103	        cu = c[0] * ux + c[1] * uy + c[2] * uz
104	        bracket = 1.0 + 3.0 * cu + 4.5 * cu * cu - 1.5 * u_dot_u
105	        out.append(w * rho * bracket)
106	    return out
107	
108	
109	def opposite_index(i: int, cs: list[tuple[int, int, int]]) -> int:
110	    target = (-cs[i][0], -cs[i][1], -cs[i][2])
111	    return cs.index(target)
112	
113	
114	def assert_close(label: str, got: float, want: float, tol: float = TOL_ABS) -> None:
115	    if abs(got - want) > tol:
116	        raise AssertionError(
117	            f"{label}: got {got!r}, want {want!r}, |diff|={abs(got-want):.3e} > {tol:.0e}"
118	        )
119	
120	
121	def main() -> int:
122	    print("=== D3Q19 algebraic verification harness ===")
123	    cs = build_velocity_set()
124	    print(f"  built {len(cs)} velocity vectors from {{-1,0,1}}^3 with |c|^2 <= 2")
125	
126	    assert cs == EXPECTED_CANONICAL, (
127	        "canonical ordering mismatch\n"
128	        f"  got:  {cs}\n  want: {EXPECTED_CANONICAL}"
129	    )
130	    print("  canonical ordering matches d3q19.md section 2.2 table: OK")
131	
132	    weights_frac = [weight_for(c) for c in cs]
133	    weights = [float(w) for w in weights_frac]
134	    norm = sum(weights_frac)
135	    assert norm == Fraction(1, 1), f"sum(omega_i) = {norm}, want 1"
136	    print(f"  sum(omega_i) = {norm} (exact rational): OK")
137	
138	    for alpha in range(3):
139	        for beta in range(3):
140	            s = sum(
141	                w * Fraction(c[alpha] * c[beta])
142	                for w, c in zip(weights_frac, cs)
143	            )
144	            want = Fraction(1, 3) if alpha == beta else Fraction(0)
145	            assert s == want, (
146	                f"second moment ({alpha},{beta}): got {s}, want {want}"
147	            )
148	    print("  second-moment tensor exactly (1/3) delta_{alpha,beta}: OK")
149	
150	    opp = [opposite_index(i, cs) for i in range(19)]
[...elided middle: lines 151-254 are the BGK equilibrium evaluation
loop over 4 canonical test points (tp1_zero_velocity,
tp2_uniform_x, tp3_oblique_xy, tp4_density_scaled), the
hand-check table for tp2 against per-direction Fraction expected
values, and the tp4 linear-scaling check...]
255	    payload = {
256	        "schema_version": 1,
257	        "source": "tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py",
258	        "derivation": "tools/integrity/docs/algebraic/d3q19.md",
259	        "velocity_set": [list(c) for c in cs],
260	        "weights": weights,
261	        "opposite_index": opp,
262	        "test_points": [
263	            {
264	                "name": r["name"],
265	                "rho": r["rho"],
266	                "u": r["u"],
267	                "feq": r["feq"],
268	                "sums": r["sums"],
269	            }
270	            for r in results
271	        ],
272	    }
273	    EXPECTED_JSON.write_text(json.dumps(payload, indent=2) + "\n")
274	    print(f"\n  wrote {EXPECTED_JSON.relative_to(Path.cwd()) if EXPECTED_JSON.is_relative_to(Path.cwd()) else EXPECTED_JSON}")
275	
276	    print("\n=== ALL CHECKS PASS ===")
277	    return 0
278	
279	
280	if __name__ == "__main__":
281	    sys.exit(main())
```

FACT — internal structure (informs Section C.7 design choice):

- `build_velocity_set()` enumerates {-1,0,1}^3 with |c|² ≤ 2 and
  asserts against `EXPECTED_CANONICAL`. This is the
  velocity-set check.
- `weights_frac = [weight_for(c) for c in cs]` + the
  `sum(omega_i) == 1` + second-moment tensor checks. This is the
  weights check.
- `feq(...)` + the per-test-point evaluation loop +
  `tp2` hand-check + `tp4` linear-scaling check. This is the
  equilibrium check.
- The three things are validated sequentially in a single
  `main()` execution, but each fail-mode lives in a distinct
  block separated by `print()` headers. Functionally separable.
- Output side-effect: writes `d3q19_equilibrium.expected.json`
  with all four test-point results + the velocity set + weights.

The harness is a `__main__`-shaped CLI, not a `run(repo_root) ->
list[Finding]` check-module shape. Adapting to the check-module
shape is the bulk of the P1.5 work.

### B.2 — `d3q19_equilibrium.expected.json`

FACT: 290 lines (`wc -l`).

**Head (lines 1-50):**

```json
{
  "schema_version": 1,
  "source": "tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py",
  "derivation": "tools/integrity/docs/algebraic/d3q19.md",
  "velocity_set": [
    [
      0,
      0,
      0
    ],
    [
      1,
      0,
      0
    ],
    [
      -1,
      0,
      0
    ],
    [
      0,
      1,
      0
    ],
    [
      0,
      -1,
      0
    ],
    [
      0,
      0,
      1
    ],
    [
      0,
      0,
      -1
    ],
    [
      1,
      1,
      0
    ],
    [
      1,
      -1,
      0
    ],
```

**Tail (lines 261-290):**

```json
      ],
      "feq": [
        0.8302083333333332,
        0.1607638888888889,
        0.11909722222222222,
        0.13836805555555556,
        0.13836805555555556,
        0.13836805555555556,
        0.13836805555555556,
        0.08038194444444445,
        0.08038194444444445,
        0.05954861111111111,
        0.05954861111111111,
        0.08038194444444445,
        0.08038194444444445,
        0.05954861111111111,
        0.05954861111111111,
        0.06918402777777778,
        0.06918402777777778,
        0.06918402777777778,
        0.06918402777777778
      ],
      "sums": {
        "mass": 2.5,
        "mom_x": 0.12500000000000006,
        "mom_y": 0.0,
        "mom_z": 0.0
      }
    }
  ]
}
```

The interior (lines 51-260) is the remaining velocity vectors
(elements 9-18 of the 19-vector set), the `weights` array, the
`opposite_index` array, and `test_points[0..3]` blocks each with
`rho`, `u`, `feq`, `sums`. Same schema as the tail block above,
just repeated four times.

### B.3 — `[Algebraic_D3Q19]` registry stanza

FACT — `grep -n "Algebraic_D3Q19" tools/integrity/docs/ground-truth-sources.md`:

```
34:# constants come from [Algebraic_D3Q19], not from this anchor.
36:[Algebraic_D3Q19]
58:  registry entry `[Algebraic_D3Q19]` (Phase 12 setup-2).
59:- **Algebraic_D3Q19:** Pure derivation; no vendored upstream. The
```

Stanza body verbatim (`ground-truth-sources.md:36-44`):

```toml
[Algebraic_D3Q19]
derivation     = "tools/integrity/docs/algebraic/d3q19.md"
expected_data  = "tools/integrity/integrity/cat3_numerical/checks/d3q19_equilibrium.expected.json"
used_by_checks = ["cat3.d3q19-velocity-set", "cat3.d3q19-weights", "cat3.d3q19-equilibrium"]
# No vendor_root: D3Q19 lattice constants are derivable from first
# principles (squared-L2-norm-<=2 subset of {-1,0,1}^3 + Gauss-Hermite
# isotropy constraints). Pairs with [Krueger] which covers the
# equation form and the halfway-bounce-back convention.
```

### B.4 — `used_by_checks` field confirms three CHECK_IDs

FACT: literal list = `["cat3.d3q19-velocity-set", "cat3.d3q19-weights", "cat3.d3q19-equilibrium"]`. Matches
the addendum § 4.3 claim exactly.

### B.5 — Per-CHECK_ID dispatch confirms unregistered state

FACT — outputs:

```
$ python3 -m integrity --check cat3.d3q19-velocity-set --no-audit-log
integrity: 0 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
EXIT:0

$ python3 -m integrity --check cat3.d3q19-weights --no-audit-log
integrity: 0 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
EXIT:0

$ python3 -m integrity --check cat3.d3q19-equilibrium --no-audit-log
integrity: 0 pass, 0 soft-warn, 0 hard-fail, 0 suppressed
EXIT:0
```

Contrast (control): `cat3.cubic-kernel` returns `1 pass, 0
hard-fail, 0 suppressed` — exit 0 because the registered check
ran and produced no findings (driver not built; graceful degrade
returns empty findings, scored as pass).

INFERENCE: the d3q19 dispatches return `0 pass` because
`discover_checks()` (`runner.py:82-99`) filters
`all_checks = [(cid, mod) for cid, mod in all_checks if cid == args.check]`
and no module advertises any of the three CHECK_IDs. The filter
returns an empty list, `run_checks` returns no findings, and the
pass count from `runner.py:182-183` counts zero registered
checks. This confirms FACT: the three CHECK_IDs are not wired
through the discovery surface.

### B.6 — Harness introduction commit

FACT — `git log --all --oneline -- tools/integrity/integrity/cat3_numerical/checks/d3q19_verify.py`:

```
0db9c73 setup(phase12): add Algebraic_D3Q19 derivation + verification harness
```

Matches the addendum § 4.3 claim: setup-2 (`0db9c73`).

---

## Section C — P1.5 registration pattern: existing `cat3.cubic-kernel` mirror

### C.1 — `cat3_numerical/checks/__init__.py` verbatim (7 LOC)

```python
1	"""Cat 3 check modules. Discovered by integrity.runner.discover_checks."""
2	
3	from integrity.cat3_numerical.checks import cubic_kernel
4	
5	REGISTERED_CHECKS = [
6	    (cubic_kernel.CHECK_ID, cubic_kernel),
7	]
```

FACT: the registration shape is a module-level
`REGISTERED_CHECKS` list of `(CHECK_ID, module)` tuples.
`runner.py:93-94` imports this list.

### C.2 — `cat3_numerical/checks/cubic_kernel.py` (99 LOC) — check module

The check module that mirrors what the d3q19 check(s) need to
become. Selected spans:

```python
 1	"""Check: cat3.cubic-kernel — Stack C cubic kernel matches upstream formula.
 2	
 3	Mode: HARD_FAIL.
 4	
 5	Compares the Stack C driver's evaluation of W(r,h) and the magnitude of
 6	gradW against analytically-derived expected values from the cubic
[...integrity-allow annotations elided for brevity...]
 11	Graceful degrade: if the driver isn't built (the
 12	GPU_SIMS_BUILD_INTEGRITY_CAT3 CMake flag wasn't set), this check
 13	returns zero findings.
 14	"""
 15	
 16	from __future__ import annotations
 17	
 18	from pathlib import Path
 19	
 20	from integrity.cat3_numerical.cubic_kernel import (
 21	    EXPECTED_VALUES_RELATIVE,
 22	    find_driver,
 23	    load_expected_values,
 24	    run_driver,
 25	    within_tolerance,
 26	)
 27	from integrity.common.results import FailureMode, Finding
 28	
 29	
 30	CHECK_ID = "cat3.cubic-kernel"
 31	MODE = FailureMode.HARD_FAIL
 32	
 33	
 34	def run(repo_root: Path) -> list[Finding]:
 35	    findings: list[Finding] = []
 36	
 37	    points, tolerance = load_expected_values(repo_root)
 38	    if not points:
 39	        return findings
 40	
 41	    driver = find_driver(repo_root)
 42	    if driver is None:
 43	        # Driver not built (CMake flag not set); graceful degrade
 44	        return findings
[...evaluation loop with within_tolerance comparison, emitting
Finding objects on mismatch...]
 99	    return findings
```

FACT: the check module signature is
`run(repo_root: Path) -> list[Finding]`. Imports algorithmic
helpers from `integrity.cat3_numerical.cubic_kernel` (sibling
harness, see C.3). Defines `CHECK_ID` and `MODE` at module
scope.

### C.3 — `cat3_numerical/cubic_kernel.py` (111 LOC) — algorithmic harness

The structural mirror for `d3q19_verify.py`. Selected spans:

```python
 1	"""Cubic SPH kernel numerical correctness per spec § 8.
 2	
 3	Reads expected values from expected_values.toml, runs the Stack C
 4	driver binary at build/tools/integrity/drivers/integrity_cat3_stack_c/,
 5	compares each evaluation against tolerance. HARD_FAIL on any
 6	disagreement.
 7	
 8	The driver is built when GPU_SIMS_BUILD_INTEGRITY_CAT3=ON in cmake.
 9	The Python check graceful-degrades to zero findings when the driver
10	isn't present (e.g., flag not set, build not run).
11	"""
[...imports + dataclass defs...]
24	DRIVER_RELATIVE_PATH = Path(
25	    "build/tools/integrity/drivers/integrity_cat3_stack_c/integrity_cat3_stack_c"
26	)
27	EXPECTED_VALUES_RELATIVE = Path(
28	    "tools/integrity/integrity/cat3_numerical/expected_values.toml"
29	)
[...TestPoint + DriverEvaluation dataclasses...]
48	def load_expected_values(repo_root: Path) -> tuple[list[TestPoint], dict]:
49	    """Parse expected_values.toml. Returns (test_points, tolerance_dict)."""
[...parser, find_driver, run_driver, within_tolerance helpers...]
107	def within_tolerance(actual: float, expected: float, atol: float, rtol: float) -> bool:
108	    """abs(actual - expected) <= atol + rtol * abs(expected). NaN rejected."""
109	    if not math.isfinite(actual):
110	        return False
111	    return abs(actual - expected) <= atol + rtol * abs(expected)
```

FACT: the harness exposes helper functions for the check module
to import and uses dataclasses (`TestPoint`,
`DriverEvaluation`) for typed records. It is NOT a `__main__`-shaped script.

### C.4 — Expected-values format: TOML vs JSON

FACT — `expected_values.toml` head (1-30):

```toml
# Expected values for cat3.cubic-kernel.
# Generated by tools/integrity/integrity/cat3_numerical/generate_expected.py
# Source: Bender-Koschier 2015 / SPlisHSPlasH 2.16.1 SPHKernels.h:43-85
# Anchor SHA: 6bff55a6eaf14083d34650f22a268ce156b62b54

[tolerance]
atol = 1e-5
rtol = 1e-5

[[test_points]]
q = 0.0
h = 1.0
expected_W = 2.54647908947033
expected_gradW_magnitude = 0

[[test_points]]
q = 0.1
h = 1.0
expected_W = 2.40896921863893
expected_gradW_magnitude = 2.59740867125973
[...continues for 6 test points...]
```

(Tail = the q=1.0 boundary record. Total = 45 LOC.)

FACT — format mismatch: cubic-kernel uses TOML
(`tomllib.loads`), d3q19 uses JSON (`json.dumps`). The two
expected-data files have different schemas — TOML is structured
for human authoring with `[tolerance]` + `[[test_points]]`; the
JSON file embeds derivation provenance (`"source"`,
`"derivation"`, `"schema_version"`) plus pre-computed sums.

INFERENCE — the d3q19 JSON file does not need to be converted to
TOML for P1.5. The new check module(s) can `json.load(...)` the
file. The TOML/JSON divergence is incidental: cubic-kernel has a
generator that emits TOML so the file is human-reviewable;
d3q19_verify.py emits JSON because the velocity-set and
opposite-index arrays nest nontrivially and TOML's array-of-table
syntax for that would be awkward. Both formats are defensible
for their use case; harmonization is not required.

### C.5 — `cat3_numerical/__init__.py` (2 LOC) + `generate_expected.py` (118 LOC)

`__init__.py` verbatim:

```python
1	"""Category 3: Numerical correctness per spec § 8."""
```

`generate_expected.py` is the cubic-kernel precedent for
"writes expected_values data file as a deliberate human action."
Head + signature spans:

```python
 1	#!/usr/bin/env python3
 2	"""Generate expected_values.toml from the cubic spline kernel formula.
 3	
 4	Analytical derivation from Bender-Koschier 2015 / SPlisHSPlasH 2.16.1
[...integrity-allow annotation elided...]
 7	SPHKernels.h:43-85. Run manually when the test point set changes or
 8	the registered upstream anchor SHA bumps and the formula needs
 9	re-verification against the new source.
10	
11	Usage:
12	    python generate_expected.py                  # normal regen
13	    python generate_expected.py --inject-factor-of-6
14	                                                 # inject the Phase 11.5 commit 1
15	                                                 # gradient defect for acceptance testing
16	
17	The script writes to expected_values.toml in the same directory.
18	"""
[...cubic_W(q,h), cubic_gradW_magnitude(q,h,inject_factor_of_6),
TEST_POINTS_Q list, main() with --inject-factor-of-6 acceptance
test flag...]
117	if __name__ == "__main__":
118	    sys.exit(main())
```

INFERENCE — `d3q19_verify.py` plays the dual role of both
`cubic_kernel.py` (the algorithmic re-derivation that the check
module imports from) AND `generate_expected.py` (the `__main__`
script that writes the data file). For P1.5 spec drafting, two
options:

1. **Keep `d3q19_verify.py` as a `__main__` script (no rename, no
   split)** and write a thin new module `checks/d3q19.py` (or
   three thin modules) that load the JSON file and run their
   asserts on the loaded data — paralleling `cubic_kernel.py`'s
   "load expected, run driver, compare" shape, except the driver
   is the verifier itself (which has already run) and the check
   just re-validates the loaded JSON's algebraic invariants.
2. **Refactor `d3q19_verify.py`** into a `cat3_numerical/d3q19.py`
   harness (importable, with `build_velocity_set`, `weight_for`,
   `feq` as named functions) + a thin `generate_d3q19.py` script
   that calls `main()` + `checks/d3q19_*.py` check modules.

Option 1 is lower scope (no rename, no public-API change) and
matches the addendum § 5 P1.5 framing ("Uses the existing
`d3q19_verify.py` harness; no new algebraic work."). Option 2
would harmonize with cubic-kernel's tri-file layout.

### C.6 — `test_cat3_cubic_kernel.py` (109 LOC) — test-pattern mirror

Selected spans verbatim:

```python
 1	"""Tests for cat3.cubic-kernel."""
[...imports...]
13	from integrity.cat3_numerical.cubic_kernel import (
14	    load_expected_values,
15	    within_tolerance,
16	)
17	
18	
19	def test_within_tolerance_exact_match() -> None:
20	    assert within_tolerance(1.0, 1.0, 1e-5, 1e-5)
[...within_tolerance and load_expected_values unit tests...]
35	def test_load_expected_values_real_file() -> None:
36	    """The committed expected_values.toml should parse cleanly with 6 points."""
37	    repo_root = Path(__file__).resolve().parents[3]
38	    points, tolerance = load_expected_values(repo_root)
39	    assert len(points) == 6, f"expected 6 test points, got {len(points)}"
40	    assert tolerance == {"atol": 1e-5, "rtol": 1e-5}
[...physics sanity tests (q=0 → 8/pi, q=1 → 0)...]
62	@pytest.mark.skipif(
63	    not shutil.which("cmake") or not shutil.which("ninja"),
64	    reason="cmake or ninja not available",
65	)
66	def test_driver_builds_and_runs(tmp_path: Path) -> None:
67	    """End-to-end smoke: build the driver, invoke it, parse JSON."""
[...full build+run e2e smoke...]
105	def test_check_graceful_degrade_without_driver(tmp_path: Path) -> None:
106	    """Without the driver built, the check returns no findings."""
107	    from integrity.cat3_numerical.checks.cubic_kernel import run
108	    findings = run(tmp_path)
109	    assert findings == []
```

FACT — test pattern: (a) unit tests on tolerance helpers, (b)
parse-the-real-data-file test with hardcoded count, (c) physics
sanity asserts on known boundary cases, (d) optional
end-to-end build-and-run test gated on tool availability, (e)
graceful-degrade test that the check returns no findings absent
the artifact it consumes.

For d3q19 the parallel pattern is straightforward minus the
build step (no C++ driver involved — the harness is pure
Python). Tests would be: (a) `build_velocity_set` returns
canonical ordering, (b) weights sum to 1 and second-moment
tensor is (1/3)δ, (c) feq satisfies mass/momentum/(scale)
conservation on the four committed test points, (d) loading
`d3q19_equilibrium.expected.json` produces the documented schema
shape. No driver-build test needed.

### C.7 — Design choice: one check module with 3 sub-tests vs 3 modules

INFERENCE: `d3q19_verify.py`'s internal structure (Section B.1
inventory) already separates the three concerns into distinct
blocks. Either decomposition is structurally supported by the
existing code:

- **Three separate check modules** (`checks/d3q19_velocity_set.py`,
  `checks/d3q19_weights.py`, `checks/d3q19_equilibrium.py`):
  Each module imports the relevant harness helpers, calls them,
  emits findings on mismatch with a `CHECK_ID` matching the
  registry stanza. Three entries in `REGISTERED_CHECKS`.
  Lowest mismatch with the registry; aligns with the registry's
  declarative shape (three named checks); makes per-check
  invocation natural (`--check cat3.d3q19-weights` runs only the
  weights validation). Cost: ~3 file × ~50 LOC each.
- **One check module with three CHECK_IDs**: A single
  `checks/d3q19.py` that defines three `CHECK_ID_*` constants
  and a single `run(repo_root) -> list[Finding]` that emits
  findings tagged with the appropriate `check_id`. `REGISTERED_CHECKS`
  would need three tuples all pointing to the same module —
  which conflicts with the current `(cid, module)` shape because
  the discover loop dispatches `module.run(...)` once per tuple
  and would re-run the harness three times.

INFERENCE: **three separate check modules** is the natural fit.
The registry stanza already declares them as three separate
CHECK_IDs; the runner's dispatch model is one CHECK_ID = one
module = one `run()`. The harness work is already centralized in
`d3q19_verify.py` (Section C.5 Option 1) — the check modules
become thin wrappers that exercise specific assertions.

---

## Section D — P1.6 surface: `runner.py` emit_output

### D.1 — Function locations

FACT — `grep -n` results:

```
57:    parser.add_argument("--output", choices=["human", "json", "github"], default="human",
74:        output=ns.output,
116:def emit_output(summary: RunSummary, findings: list[Finding], args: CliArgs) -> None:
118:    if args.output == "json":
132:    elif args.output == "github":
148:def _emit_human_summary(summary: RunSummary) -> None:
```

### D.2 — Confirmed asymmetry between github and human branches

FACT — `emit_output` verbatim (`runner.py:116-145`):

```python
116	def emit_output(summary: RunSummary, findings: list[Finding], args: CliArgs) -> None:
117	    """Emit results in the chosen format."""
118	    if args.output == "json":
119	        payload = {
120	            "schema_version": 1,
121	            "commit": git_head_sha(args.root),
122	            "summary": {
123	                "pass": summary.passes,
124	                "soft_warn": summary.soft_warns,
125	                "hard_fail": summary.hard_fails,
126	                "suppressed": summary.suppressions,
127	            },
128	            "findings": [f.to_dict() for f in findings],
129	        }
130	        json.dump(payload, sys.stdout, indent=2)
131	        sys.stdout.write("\n")
132	    elif args.output == "github":
133	        for f in findings:
134	            if f.suppressed:
135	                continue
136	            kind = "error" if f.mode == FailureMode.HARD_FAIL else "warning"
137	            sys.stdout.write(
138	                f"::{kind} file={f.file},line={f.line}::{f.check_id}: {f.message}\n"
139	            )
140	        _emit_human_summary(summary)
141	    else:
142	        _emit_human_summary(summary)
143	        for f in findings:
144	            sys.stdout.write(f"  {f.mode.name}: {f.check_id} at {f.file}:{f.line}\n")
145	            sys.stdout.write(f"    {f.message}\n")
```

FACT: the `github` branch (lines 132-140) calls
`if f.suppressed: continue` on line 134. The `else` branch
(lines 141-145, the default `human`-output renderer) iterates
`for f in findings` without any suppressed filter. Asymmetry
confirmed.

FACT — `_emit_human_summary` (`runner.py:148-154`):

```python
148	def _emit_human_summary(summary: RunSummary) -> None:
149	    sys.stdout.write(
150	        f"integrity: {summary.passes} pass, "
151	        f"{summary.soft_warns} soft-warn, "
152	        f"{summary.hard_fails} hard-fail, "
153	        f"{summary.suppressions} suppressed\n"
154	    )
```

This function uses the `RunSummary` counts which themselves are
correctly computed from `summary.hard_fails = sum(1 for f in
findings if f.mode == FailureMode.HARD_FAIL and not
f.suppressed)` (`runner.py:185-186`). The summary numbers are
right; only the stanza dump in the `else` branch is wrong.

### D.3 — `--output` flag definition

FACT — `runner.py:57-58`:

```python
parser.add_argument("--output", choices=["human", "json", "github"], default="human",
                    help="Output format")
```

Default is `"human"` — so a bare `python3 -m integrity` invocation
hits the buggy branch. CI invocations passing `--output github`
get the correct filter.

### D.4 — Stanza-count cross-reference

FACT (from Section A.2):

- Summary line: `4 hard-fail, 1046 suppressed`
- Total stanza header lines emitted: 1050
- 4 hard-fail + 1046 suppressed = 1050

The stanza count matches `hard_fail + suppressed`, not
`hard_fail` alone. This is the buggy behavior. The addendum
§ 4.1 claim and the self-review probe § A.1 INFERENCE are both
mechanically confirmed.

### D.5 — runner.py edit history

FACT — `git log --oneline -10 -- tools/integrity/integrity/runner.py`:

```
a71594a feat(integrity): A.7 CLI flags --state-snapshot / --grandfather-report (v1.1 batch 1 commit 3b)
f23fd22 revert(integrity): unintended runner.py changes from commit 4
40f73e9 fix(sph-water): dedup AABB seams + inset boundary samples (commit 4)
f576b5e feat(integrity): Cat 3 cubic-kernel numerical correctness (commit 8 — final)
fc20ef7 fix(integrity): install root workspace deps in CI for Stack B + skip suppressed in github output
96de0c3 feat(integrity): Cat 2 Stack D contract verification (commit 5)
8af5672 feat(integrity): grandfather-sweep pre-v1 findings (commit 4a)
0822f6a feat(integrity): Cat 1 citation parsing + intra-repo resolution (commit 2)
51bd8d0 feat(integrity): scaffold toolkit package + runner (commit 1)
```

FACT: `fc20ef7` is titled "skip suppressed in github output" —
this is the commit that added the `if f.suppressed: continue` to
the github branch and did NOT add it to the human branch. The
asymmetry is original to that commit; it was never present in
the human branch.

INFERENCE: the human branch was always buggy from initial
authoring. `fc20ef7` was a targeted CI-output fix (when CI moved
to `--output github`) that did not propagate the filter to the
default branch. No regression — original-shape defect.

### D.6 — Line-number citation discrepancies

FACT — actual current line numbers (this probe):

- `emit_output` definition: `runner.py:116`
- `if args.output == "github":` branch start: `runner.py:132`
- `if f.suppressed: continue` (in github branch): `runner.py:134`
- `else:` branch start: `runner.py:141`
- `_emit_human_summary(summary)`: `runner.py:142`
- `for f in findings:` loop in else branch: `runner.py:143`
- The stanza emit lines: `runner.py:144-145`
- `_emit_human_summary` definition: `runner.py:148`

Prior citations:

- **9add149 commit message** says `runner.py:893`. Off by ~750
  lines — does not match any line in the current 198-LOC file.
  Likely a copy/paste artifact from a different file or version.
- **Earlier scoping prompts** cited `runner.py:891-895` and
  `:898`. Same issue — file is 198 LOC; cannot match.
- **Addendum § 4.1** cites `runner.py:141-145` for the buggy
  span. Actual current span is `runner.py:141-145` (the `else`
  block) with the `for f in findings:` at `:143` and the stanza
  writes at `:144-145`. **Matches.**
- **Addendum § 5 P1.6** cites `runner.py:143-145` as the fix
  site (insert `if f.suppressed: continue` between `:143` and
  `:144`). Actual: yes — between `:143` (the `for f in findings:`)
  and `:144` (the first `sys.stdout.write`) is the natural
  insertion point. **Matches.**
- **Self-review probe § A.1** cites `runner.py:141-145` for the
  else branch and `:133-139` for the github branch. Actual:
  `:141-145` else branch, `:133-139` github branch
  (lines 133-139 in current file = the `for f in findings: if
  f.suppressed: continue ... sys.stdout.write(::...)` block).
  **Matches.**

INFERENCE: the addendum and self-review-probe citations are
accurate. The 9add149 commit message and earlier scoping prompts
both reference `runner.py:893` — a stale citation that does not
correspond to any current state of the file; spec drafting must
not propagate that number.

---

## Section E — P1.7 surface: `stub_label_stale.py` docstring drift

### E.1 — Module docstring (lines 1-40)

FACT verbatim (`stub_label_stale.py:1-40`):

```python
 1	"""Check: cat2.stub-label-stale -- flag stale "Phase N stub" labels.
 2	
 3	Mode: HARD_FAIL.
 4	
 5	Closes spec section 12 row 5 (alembic_writer.hpp canonical case).
 6	Anchors on the exact phrasing `In Phase <N>, this is a stub:` present
 7	in both confirmed stale cases per probe v1_1_apispec section G. If the
 8	corresponding implementation file has more than 10 non-comment LOC,
 9	the "stub" label contradicts the implementation and is flagged.
10	
11	Detection scope (batch-1-spec Decision 3):
12	  - C++ headers under common/common-cpp/include/**/*.{hpp,h}
13	  - Python modules under common/common-py/gpusims_common/**/*.py
14	
15	Sibling-impl resolution (batch-1-spec Decision 2):
16	  - `.hpp`/`.h` in `common-cpp/include/<sub>/<base>.hpp` ->
17	    `common-cpp/src/<sub>/<base>.cpp` (relative path mirror)
18	  - `.py`: impl is the same file
19	
20	False-positive guard for Stack D:
21	  Skip Stack D files whose top 40 lines contain `permanent stub` or
22	  `real-or-stub` -- both intentional discriminator phrasings per
23	  probe section D.2. Anchored on top-of-file because these phrasings
24	  appear in module docstrings.
25	"""
26	
27	from __future__ import annotations
28	
29	import re
[...]
```

### E.2 — `_resolve_impl_path` function

FACT — `grep -n "_resolve_impl_path\|def _resolve" tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py`:

```
95:def _resolve_impl_path(header_path: Path, repo_root: Path) -> Path | None:
163:            impl_path = _resolve_impl_path(header, repo_root)
```

Function verbatim (`stub_label_stale.py:95-128`):

```python
 95	def _resolve_impl_path(header_path: Path, repo_root: Path) -> Path | None:
 96	    """Resolve the impl file for a given header/module.
 97	
 98	    Convention (verified against synced common-cpp layout 2026-05-15):
 99	      include/<namespace>/<rest>.hpp  ->  src/<rest>.cpp
100	    The first directory component after include/ is the project
101	    namespace (e.g. `gpusims/`) and is stripped -- the src/ tree does
102	    not repeat the namespace path.
103	
104	    For Python files, impl is the same file (Python does not separate
105	    declaration from implementation)."""
106	    try:
107	        rel = header_path.relative_to(repo_root)
108	    except ValueError:
109	        return None
110	
111	    rel_str = str(rel).replace("\\", "/")
112	
113	    if (
114	        rel_str.startswith(str(CPP_INCLUDE_ROOT) + "/")
115	        and header_path.suffix in (".hpp", ".h")
116	    ):
117	        relative_to_include = header_path.relative_to(repo_root / CPP_INCLUDE_ROOT)
118	        parts = relative_to_include.parts
119	        if len(parts) < 2:
120	            return None
121	        namespace_stripped = Path(*parts[1:])
122	        impl_relative = namespace_stripped.with_suffix(".cpp")
123	        return repo_root / CPP_SRC_ROOT / impl_relative
124	
125	    if rel_str.startswith(str(PY_PACKAGE_ROOT) + "/") and header_path.suffix == ".py":
126	        return header_path
127	
128	    return None
129	
```

### E.3 — Docstring-vs-code discrepancy

FACT — the module-level docstring (`stub_label_stale.py:15-18`) says:

> Sibling-impl resolution (batch-1-spec Decision 2):
>   - `.hpp`/`.h` in `common-cpp/include/<sub>/<base>.hpp` ->
>     `common-cpp/src/<sub>/<base>.cpp` (relative path mirror)
>   - `.py`: impl is the same file

The "relative path mirror" framing says: keep the directory
component intact — `include/<sub>/<base>.hpp` maps to
`src/<sub>/<base>.cpp` (the `<sub>` is preserved).

FACT — the function-level docstring (`stub_label_stale.py:98-102`)
says:

> Convention (verified against synced common-cpp layout 2026-05-15):
>   include/<namespace>/<rest>.hpp  ->  src/<rest>.cpp
> The first directory component after include/ is the project
> namespace (e.g. `gpusims/`) and is stripped — the src/ tree does
> not repeat the namespace path.

The "namespace-stripped" framing says: drop the first directory
component — `include/gpusims/<rest>.hpp` maps to `src/<rest>.cpp`
(the `gpusims/` is removed).

FACT — the code (`stub_label_stale.py:117-123`):

```python
relative_to_include = header_path.relative_to(repo_root / CPP_INCLUDE_ROOT)
parts = relative_to_include.parts
if len(parts) < 2:
    return None
namespace_stripped = Path(*parts[1:])
impl_relative = namespace_stripped.with_suffix(".cpp")
return repo_root / CPP_SRC_ROOT / impl_relative
```

`Path(*parts[1:])` drops `parts[0]` (the namespace) — so the
code implements namespace-strip, agreeing with the in-function
docstring and disagreeing with the module-level docstring.

The two docstrings on the same file describe two different
resolution rules. The implementation matches one (the
in-function docstring); the module-level docstring is stale.

### E.4 — Pause-and-surface cross-reference

FACT — `docs/diagnostics/_audits/integrity_v1_1_commit1_landing_2026-05-15.md`
exists in repo. INFERENCE — § E.1 of that landing report records
pause-and-surface #1, the discovery during commit-1 execution
that the spec's literal-mirror convention did not match the
synced repo layout, leading to the corrected namespace-strip
logic. The function docstring was updated to reflect the
correction (text at lines 98-102, dated 2026-05-15); the module
docstring at lines 15-18 was not.

INFERENCE: this is exactly the failure pattern the addendum
§ 4.4 describes — "docstring lies about code" — preserved on
disk because the corrective work at commit-1 fixed the logic and
the in-function docstring but missed the module-level docstring
two stanzas above. Both docstrings are in the same file; the
update site was the same edit session.

### E.5 — Line-number citation discrepancies

FACT — actual line numbers (this probe):

- Module-level docstring: `stub_label_stale.py:1-25` (the
  drifted convention text is at lines 15-18).
- `_resolve_impl_path` def: `stub_label_stale.py:95`.
- Function body: `stub_label_stale.py:95-128`.
- Namespace-strip block: `stub_label_stale.py:113-123`.

Prior citations:

- **Earlier scoping prompt** cited `stub_label_stale.py:644-677`
  for "the corrected logic". File is 195 LOC; cannot match.
  Stale.
- **Addendum § 4.4** cites `stub_label_stale.py:15-18` for the
  module docstring (matches) and `95-128` for the function body
  (matches). Also cites `98-105` for the in-function docstring;
  actual range is `96-105`. **One-line minor offset.**

INFERENCE: addendum citations are reliable for spec drafting.
The `:644-677` citation must not be propagated.

---

## Section F — P1.8 surface: grandfather_sweep CLI + test + classifier

### F.1 — `tools/integrity/scripts/grandfather_sweep.py` verbatim (31 LOC)

```python
 1	#!/usr/bin/env python3
 2	"""Grandfather-sweep CLI entry. Logic lives in integrity.grandfather."""
 3	
 4	from __future__ import annotations
 5	
 6	import argparse
 7	import sys
 8	from pathlib import Path
 9	
 10	from integrity.common.repo import find_repo_root
 11	from integrity.grandfather import apply_annotations
 12	
 13	def main(argv: list[str]) -> int:
 14	    parser = argparse.ArgumentParser(description="Grandfather-sweep integrity findings")
 15	    parser.add_argument("--dry-run", action="store_true")
 16	    parser.add_argument("--repo-root", type=Path, default=None)
 17	    ns = parser.parse_args(argv)
 18	
 19	    root = ns.repo_root if ns.repo_root else find_repo_root()
 20	    files, anns, counts = apply_annotations(root, ns.dry_run)
 21	
 22	    label = "would modify" if ns.dry_run else "modified"
 23	    print(f"grandfather-sweep: {label} {files} files; {anns} annotations added")
 24	    for cat, n in sorted(counts.items(), key=lambda kv: -kv[1]):
 25	        print(f"  {cat:>35s}: {n}")
 26	    return 0
 27	
 28	
 29	if __name__ == "__main__":
 30	    sys.exit(main(sys.argv[1:]))
```

FACT: 31 LOC total. Two existing flags: `--dry-run` and
`--repo-root`. No path-filtering, no category-filtering, no
live-source protection. The CLI is a thin wrapper around
`apply_annotations` in `integrity.grandfather`.

INFERENCE — the P1.8 hook landing site: between line 19 (root
resolved) and line 20 (`apply_annotations(...)` called). Either:

1. Add a `--sweep-live-source` flag (default False) and pass it
   through to a new `apply_annotations(root, dry_run,
   sweep_live_source=False)` parameter; the gating happens in
   the classifier/grouper layer.
2. Filter at the `Finding` collection step (in
   `collect_findings` or `group_findings_by_target` in
   `grandfather.py`) using the same default-False sentinel.

Option 1 is the cleaner separation (CLI exposes the gate; logic
honors it). Option 2 leaks the bucket distinction into the
collection layer.

### F.2 — `classify` call sites in `grandfather.py`

FACT — `grep -n "classify(" tools/integrity/integrity/grandfather.py`:

```
196:    classifications = [(f, classify(f)) for f in findings_on_line]
322:        cls = classify(f)
```

Call-site 1 (`grandfather.py:196`, in `render_annotation_line`):

```python
193	    """Render the annotation comment line(s) to insert above
194	    file_lines[line_zero_indexed] for the given group of same-target
195	    findings."""
196	    classifications = [(f, classify(f)) for f in findings_on_line]
197	    categories = {c.category for _, c in classifications}
```

Call-site 2 (`grandfather.py:322`, in `apply_annotations`):

```python
319	            modified_this_file = True
320	
321	            for f in findings_on_line:
322	                cls = classify(f)
323	                category_counts[cls.category] = category_counts.get(cls.category, 0) + 1
```

INFERENCE — the natural P1.8 hook site is upstream of both
call-sites: filter `findings_on_line` (or upstream-er, the
`grouped` dict / `findings` list) before `render_annotation_line`
or `apply_annotations` reach them. The filter rule (per addendum
+ Section G below): if `not sweep_live_source` AND
`classify(f).category == "other-cat1"` AND `is_live_source(f.file)`,
then skip the finding (don't classify, don't render an
annotation, don't bump category_counts).

The simplest location is inside `apply_annotations` itself,
before line 273's `grouped = group_findings_by_target(findings)`,
because the filter needs `classify()` to know the category and
filtering before grouping avoids creating empty groups.

### F.3 — `grandfather.py` dataclasses + classifier

FACT — file is 330 LOC. Selected dumps follow.

`Finding` dataclass (`grandfather.py:23-28`):

```python
23	@dataclass(frozen=True)
24	class Finding:
25	    check_id: str
26	    file: str
27	    line: int
28	    message: str
```

`Classification` dataclass (`grandfather.py:31-35`):

```python
31	@dataclass(frozen=True)
32	class Classification:
33	    category: str
34	    reason: str
35	    issue_ref: str
```

`classify` function full body (`grandfather.py:38-125`):

```python
 38	def classify(finding: Finding) -> Classification:
 39	    """Classify a finding into a grandfather category. First match wins."""
 40	    f = finding.file
 41	    msg = finding.message
 42	    cid = finding.check_id
 43	
 44	    if cid == "cat2.public-symbol-used":
 45	        return Classification(category="cat2-stack-d-unused", ...)
 46	    if cid == "cat2.public-symbol-used-c":
 47	        return Classification(category="cat2-stack-c-unused", ...)
 48	    if cid == "cat2.public-symbol-used-ts":
 49	        return Classification(category="cat2-stack-b-unused", ...)
 50	    if cid == "cat2.stub-label-stale":
 51	        return Classification(category="cat2-stub-label-stale", ...)
 [...]
 72	    if cid == "cat1.intra-repo" and f.startswith("docs/diagnostics/_audits/"):
 73	        return Classification(category="audit-citation", ...)
 [...]
 79	    if cid == "cat1.upstream-citation" and "1.8.10" in msg:
 80	        if (
 81	            f.startswith("particle-fluids/sph-water/shaders/")
 82	            or f.startswith("particle-fluids/sph-water/src/")
 83	        ):
 84	            return Classification(category="live-shader-1810", ...)
 85	        return Classification(category="audit-doc-1810", ...)
 [...]
 95	    if cid == "cat1.annotation-form":
 96	        if f == "docs/integrity-toolkit-spec.md" or f.startswith("tools/integrity/docs/"):
 97	            return Classification(category="spec-grammar-example", ...)
 98	        if f.startswith("docs/retro/"):
 99	            return Classification(category="retro-grammar-example", ...)
100	        if f.startswith("tools/integrity/integrity/"):
101	            return Classification(category="toolkit-own-source", ...)
102	        if f.startswith("docs/diagnostics/_audits/"):
103	            return Classification(category="audit-report-grammar-example", ...)
104	
105	    return Classification(category="other-cat1", reason=..., issue_ref="n/a")
```

(Full bodies elided; the structural point — first-match-wins by
`(check_id, path-prefix, message-content)` — is the relevant
invariant.)

Helpers (lines 128-223): `comment_form_for`,
`comment_form_for_md_inside_fence`,
`annotation_already_present`, `render_annotation_line`. Re-imports
`_FENCE_RE` and `is_inside_fenced_block` from
`integrity.common.annotations` (lines 149-152).

`collect_findings(repo_root)` (lines 226-251): shells out to
`python3 -m integrity --output json --no-audit-log --mode
warn-only`, parses the JSON `findings` array, drops any with
`suppressed=True`, returns a `list[Finding]`.

`group_findings_by_target` (lines 254-262): groups by
`(file, line)` tuple.

`apply_annotations(repo_root, dry_run)` (lines 265-330): the
main loop — collects, groups, reads each file, renders
annotation lines via `render_annotation_line`, splices them
above the cited line in file order (lines descending so earlier
splices don't shift later ones), and writes if not dry-run.

### F.4 — `test_grandfather_sweep.py` (155 LOC)

FACT — tests cover the **classifier and renderer in
`integrity.grandfather`**, not the `grandfather_sweep.py` CLI
wrapper directly. Test functions in current file (signature
list):

```
test_audit_citation_classification
test_live_shader_1810_classification
test_audit_doc_1810_classification
test_spec_grammar_example_classification
test_toolkit_own_source_classification
test_retro_grammar_example_classification
test_audit_report_grammar_example_classification
test_other_cat1_fallthrough
test_comment_form_python
test_comment_form_cpp
test_comment_form_glsl
test_comment_form_markdown
test_outside_fence
test_inside_fence
test_at_fence_open_line
test_annotation_already_present_exact_match
test_annotation_already_present_wildcard
test_annotation_not_present_different_category
test_render_single_finding_in_markdown
test_render_two_same_category_emits_one_specific_annotation
```

FACT — `test_other_cat1_fallthrough` (lines 67-69):

```python
def test_other_cat1_fallthrough() -> None:
    f = _f("cat1.intra-repo", "some/random/file.cpp")
    assert classify(f).category == "other-cat1"
```

This is the most directly-relevant existing test for P1.8 — it
locks in that a generic non-audit-doc `cat1.intra-repo` finding
falls through to `other-cat1`. The four outstanding live-source
hard-fails (Section G.5 below) share this classification.

INFERENCE — pattern for new P1.8 tests:

1. `test_sweep_live_source_default_false_skips_other_cat1_in_live_path`
   — call `apply_annotations(...)` (or a lower-level
   filter helper) on a finding at `some/live/path.py` with
   `cid="cat1.intra-repo"`; assert no annotation rendered.
2. `test_sweep_live_source_true_sweeps_other_cat1_in_live_path`
   — same finding, `--sweep-live-source` enabled; assert
   annotation rendered.
3. `test_sweep_live_source_does_not_protect_named_categories`
   — finding at `live-shader-1810`-classifying path (sph-water
   shader); assert default-mode swept (named category overrides
   the rule).
4. `test_sweep_live_source_does_not_protect_audit_doc_paths`
   — finding at `docs/diagnostics/_audits/foo.md`; assert
   default-mode swept (the path-bucket is AUDIT-DOC, not
   LIVE-SOURCE).

Tests should hit the **collection / filter** layer, not the
filesystem-modifying `apply_annotations` flow, to stay fast and
hermetic.

### F.5 — Grandfather-sweep `--help`

FACT verbatim:

```
usage: grandfather_sweep.py [-h] [--dry-run] [--repo-root REPO_ROOT]

Grandfather-sweep integrity findings

options:
  -h, --help            show this help message and exit
  --dry-run
  --repo-root REPO_ROOT
```

No collision with `--sweep-live-source` (no existing flag with
that name or prefix). Safe to add.

### F.6 — Live-source concept presence in toolkit code

FACT — `grep -rn "live.source\|live_source\|LIVE.SOURCE\|live-source" tools/integrity/`:
**zero matches.**

The concept does not exist in toolkit code. It exists only in
prose:

- `docs/diagnostics/_audits/integrity_v1_1_post_batch_triage_2026-05-15.md`
  (the canonical bucket definition).
- `docs/retro/integrity-toolkit-v1.1-batch1.md` (the retro that
  banked the triage's outcome).
- `docs/retro/integrity-toolkit-v1.1-batch1-addendum.md` (the
  addendum landing the P1.8 scope).
- This probe report.

INFERENCE: P1.8 introduces "live-source" as a first-class
toolkit abstraction. It needs (a) a Python-level definition
(probably a `is_live_source_path(file: str) -> bool` helper in
`integrity.grandfather` or `integrity.common.paths`), (b)
a CLI surface (`--sweep-live-source`), and (c) tests pinning
the path-list. The bucket list (per addendum § 5 P1.8 — see
Section G.2 below) is small and stable.

### F.7 — Current grandfather classifier categories

FACT — `grep -n "category=" tools/integrity/integrity/grandfather.py` (deduped):

| # | Category | Triggering condition | Path bucket |
|---|---|---|---|
| 1 | `cat2-stack-d-unused` | `cid == "cat2.public-symbol-used"` | any (Stack D Python public symbol) — LIVE-SOURCE |
| 2 | `cat2-stack-c-unused` | `cid == "cat2.public-symbol-used-c"` | any (Stack C C++ public symbol) — LIVE-SOURCE |
| 3 | `cat2-stack-b-unused` | `cid == "cat2.public-symbol-used-ts"` | any (Stack B TS public symbol) — LIVE-SOURCE |
| 4 | `cat2-stub-label-stale` | `cid == "cat2.stub-label-stale"` | any (`common-cpp/include/...`) — LIVE-SOURCE |
| 5 | `audit-citation` | `cid == "cat1.intra-repo"` AND `f.startswith("docs/diagnostics/_audits/")` | AUDIT-DOC |
| 6 | `live-shader-1810` | `cid == "cat1.upstream-citation"` AND `"1.8.10" in msg` AND `f.startswith("particle-fluids/sph-water/{shaders,src}/")` | LIVE-SOURCE |
| 7 | `audit-doc-1810` | `cid == "cat1.upstream-citation"` AND `"1.8.10" in msg` AND else | mixed AUDIT-DOC/TOOLKIT-DOC |
| 8 | `spec-grammar-example` | `cid == "cat1.annotation-form"` AND (`f == "docs/integrity-toolkit-spec.md"` OR `f.startswith("tools/integrity/docs/")`) | TOOLKIT-DOC |
| 9 | `retro-grammar-example` | `cid == "cat1.annotation-form"` AND `f.startswith("docs/retro/")` | AUDIT-DOC |
| 10 | `toolkit-own-source` | `cid == "cat1.annotation-form"` AND `f.startswith("tools/integrity/integrity/")` | TOOLKIT-DOC |
| 11 | `audit-report-grammar-example` | `cid == "cat1.annotation-form"` AND `f.startswith("docs/diagnostics/_audits/")` | AUDIT-DOC |
| 12 | `other-cat1` | fallthrough | any — could be any bucket |

Bucket-correlated observation: categories 1-4 (the
`cat2-stack-*` and `cat2-stub-label-stale` named categories)
exclusively target LIVE-SOURCE paths and their sweep is
**intentional** — these are the pre-v1 grandfather buckets
explicitly tracked for migration as the underlying code is
edited. Likewise `live-shader-1810` targets a specific
LIVE-SOURCE subset and is intentional (the historical 1.8.10
fabrication; tracked for migration).

The P1.8 rule must protect ONLY `other-cat1` (the
heterogeneous fallthrough bucket) on LIVE-SOURCE paths — not
the named categories. Steven's framing matches this exactly.

---

## Section G — P1.8 path-bucket cross-check

### G.1 — Triage bucket definitions verbatim

FACT — `docs/diagnostics/_audits/integrity_v1_1_post_batch_triage_2026-05-15.md`:25-39:

```
- **AUDIT-DOC** — files under `docs/diagnostics/_audits/` or `docs/retro/`.
  Grandfather-sweep these; the existing classifier handles them via the
  permanent `audit-citation` and `audit-report-grammar-example`
  categories. Audit docs are append-only by convention.
- **TOOLKIT-DOC** — files under `tools/integrity/docs/` or
  `docs/integrity-toolkit-spec.md` or `tools/integrity/README.md`.
  Grandfather-sweep these too; the existing classifier handles them
  via fallthrough to `other-cat1`. A named permanent category
  (`toolkit-doc-snapshot`) is recommended in § C for v1.2.
- **LIVE-SOURCE** — everything else (sim code, common-* code, shaders,
  phase specs in `docs/` outside the toolkit-doc set, toolkit's own
  test code). **DO NOT sweep.** Attribute the finding back to the
  introducing commit and let the owner decide whether to fix or
  acknowledge.
```

### G.2 — Path-bucket cross-reference against Steven's P1.8 swept-paths

Steven's P1.8 swept-path list (from the addendum's overarching
v1.2-scope framing — paths the live-source guard should
*allow*, i.e., paths that are explicitly SWEEPABLE because they
are audit-doc or toolkit-doc):

| Path | Triage § B bucket | Triage § B sweepable? | P1.8 list sweepable? | Match? |
|---|---|---|---|---|
| `docs/diagnostics/_audits/` | AUDIT-DOC | ✔ sweep | ✔ sweep | ✔ |
| `docs/retro/` | AUDIT-DOC | ✔ sweep | ✔ sweep | ✔ |
| `tools/integrity/docs/` | TOOLKIT-DOC | ✔ sweep (fallthrough) | ✔ sweep | ✔ |
| `docs/integrity-toolkit-spec.md` | TOOLKIT-DOC | ✔ sweep | ✔ sweep | ✔ |
| `tools/integrity/README.md` | TOOLKIT-DOC | ✔ sweep | ✔ sweep | ✔ |
| `project-state.md` | not in triage § B | n/a | ✔ sweep (P1.8 only) | added |

FACT — `ls project-state.md`: file exists at repo root. (Output:
`project-state.md`.)

INFERENCE: `project-state.md` was added to the P1.8 swept-path
list relative to the original triage § B. This aligns with the
post-retro landing § D.3 `project-state-snapshot` classifier-extension
recommendation — `project-state.md` is a TOOLKIT-DOC-shaped
state-snapshot file that lives at repo root rather than under
`tools/integrity/`. Adding it to the sweepable set closes the
gap that otherwise would have left this file LIVE-SOURCE-classified
under the strict triage § B definition ("everything else
outside the toolkit-doc set"). The P4 classifier extension
proposed in the addendum (`project-state-snapshot` named
category) would formalize this; the P1.8 rule via path-list is
an interim path-only fix that does not require the classifier
change.

### G.3 — `docs/phase*.md` — LIVE-SOURCE per triage; cross-check via findings

FACT — current strict-mode HARD_FAIL findings on `docs/phase*.md`
paths (Section A.2 output filtered for path prefix
`docs/phase`):

```
HARD_FAIL: cat1.intra-repo at docs/phase12_lattice_boltzmann.md:203
HARD_FAIL: cat1.intra-repo at docs/phase12_lattice_boltzmann.md:351
HARD_FAIL: cat1.intra-repo at docs/phase12_lattice_boltzmann.md:1276
```

FACT — these are unsuppressed gate-red findings (confirmed via
the `--output github` filter in Section A.2 alt-form: all three
appear as `::error file=...`). They are NOT swept — confirms
triage § B's classification of phase specs as LIVE-SOURCE.

INFERENCE: any P1.8 implementation MUST keep these as live-source
(don't add `docs/phase*.md` to the sweepable list). The narrow
rule (`category == "other-cat1"` AND `is_live_source(file)` →
skip) protects them automatically since `docs/phase*.md` is not
in the AUDIT-DOC or TOOLKIT-DOC sweepable set.

### G.4 — Named-category coverage of live-source paths

FACT — from Section F.7's category table:

- `cat2-stack-d-unused`, `cat2-stack-c-unused`, `cat2-stack-b-unused`,
  `cat2-stub-label-stale`: all target LIVE-SOURCE paths but are
  named (cat2-specific) categories. Intentional sweep continues.
- `live-shader-1810`: targets the
  `particle-fluids/sph-water/{shaders,src}/` LIVE-SOURCE subset.
  Intentional sweep continues.

Live-source paths that fall through to `other-cat1`: any
`cat1.intra-repo` finding on a non-`docs/diagnostics/_audits/`
path — which includes `docs/phase12_lattice_boltzmann.md`,
`particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl`
(non-1.8.10 SPlisHSPlasH reference), and the other live-source
prefixes.

Steven's narrow rule (skip iff `category == "other-cat1"` AND
live-source) correctly carves out the exact problem set:
heterogeneous fallthrough bucket on live code that should not
have been swept and required the over-sweep revert in 9add149.

INFERENCE: no other named category needs similar protection.
The named categories are deliberate migration-tracking buckets;
they are SWEEPABLE on live-source paths by design (the
annotation marks the site as "tracked for fix" rather than
"hiding it forever"). Only `other-cat1` is the danger bucket
because its definition is "I don't have a rule for this yet" —
sweeping it on live-source effectively hides defects under
"pre-v1 grandfather" mid-tags without acknowledgement.

### G.5 — Classification of the 4 outstanding live-source hard-fails

FACT — the 4 current unsuppressed HARD_FAIL findings (from
`--output github`):

| # | Path | Line | check_id | Citation in message |
|---|---|---|---|---|
| 1 | `docs/phase12_lattice_boltzmann.md` | 203 | `cat1.intra-repo` | `chapter13/cpu/LBM.cpp:97` |
| 2 | `docs/phase12_lattice_boltzmann.md` | 351 | `cat1.intra-repo` | `chapter13/cpu/LBM.cpp:97` |
| 3 | `docs/phase12_lattice_boltzmann.md` | 1276 | `cat1.intra-repo` | `main.cpp:1168-1279` |
| 4 | `particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl` | 7 | `cat1.intra-repo` | `SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75` |

FACT — `classify(...)` results for each (sub-process invocation
of the classifier with mock Finding objects, output captured
verbatim):

```
'docs/phase12_lattice_boltzmann.md'                                              -> other-cat1
'docs/phase12_lattice_boltzmann.md'                                              -> other-cat1
'particle-fluids/sph-water/shaders/compute_boundary_volume.comp.glsl'            -> other-cat1
```

(Three calls cover all four because findings 1 and 2 share the
same path; #3 was redundant with #1's classification, so the
test passed three unique paths through.)

All four classify as `other-cat1`. Steven's narrow rule
(`category == "other-cat1"` AND live-source) catches all four.

INFERENCE — confirmed: the rule's coverage matches the 4
outstanding live-source hard-fails exactly. The over-sweep that
caused the 9add149 pause-and-surface targeted these same four
sites; reverting + then implementing the rule formalizes the
revert decision.

---

## Section H — Addendum + self-review probe landed state

### H.1 — Landing SHA cross-reference

FACT — `git log --oneline -3 docs/retro/integrity-toolkit-v1.1-batch1-addendum.md`:

```
9add149 docs(retro): self-review probe addendum to v1.1 batch-1 retro
```

(Only one commit touches this file; it is new in 9add149.)

FACT — `git log --oneline -3
docs/diagnostics/_audits/integrity_v1_1_self_review_probe_2026-05-15_architect1.md`:

```
9add149 docs(retro): self-review probe addendum to v1.1 batch-1 retro
```

(Same; one commit, new in 9add149.) Both files landed in
`9add149`, matching the SHA reported in this probe's invocation.

### H.2 — `git show 9add149 --stat`

FACT — diff stat tail (verbatim):

```
 ...v1_1_self_review_probe_2026-05-15_architect1.md | 2681 ++++++++++++++++++++
 .../integrity-toolkit-v1.1-batch1-addendum.md      |  357 +++
 2 files changed, 3038 insertions(+)
```

Two files modified. 3038 insertions total. Zero deletions.

FACT — the commit message reports `Post-commit gate state: 4
hard-fails, unchanged from pre-commit; suppressed 1007 -> 1046
(Delta +39)`. The +39 figure refers to suppression annotations
(integrity-allow comment lines added to the addendum and probe
file headers). The 3038-insertion figure includes the +39
annotation lines plus the full 2681-LOC probe report and 357-LOC
addendum body.

INFERENCE — the +39 figure is internally consistent with the
diff stat (annotations are line-shaped, integrated into the
+3038 total). The two numbers measure different things at
different aggregation levels.

### H.3 — No live-source contamination in 9add149

FACT — `git show 9add149 --name-only` (file list):

```
docs/diagnostics/_audits/integrity_v1_1_self_review_probe_2026-05-15_architect1.md
docs/retro/integrity-toolkit-v1.1-batch1-addendum.md
```

Exactly two files. Both AUDIT-DOC paths (one under
`docs/diagnostics/_audits/`, one under `docs/retro/`). No
live-source files modified. The revert of the over-sweep
annotations on the 4 live-source hard-fails was effective —
those four files were not touched.

---

## Section I — Coordination with concurrent A.3 (coordinator-chat) work

### I.1 — Recent commits

FACT — `git log --oneline --since "1 day ago"`:

```
9add149 docs(retro): self-review probe addendum to v1.1 batch-1 retro
cdad2e2 fix(lattice-boltzmann): streamline seed-slab + dt_render units (in-flight #2)
c1a257d fix(lattice-boltzmann): streamline reseed visual defects (in-flight Phase 12)
d772803 docs(audits): back-fill SHA cross-references in post-retro landing audit
e26056c docs(audits): integrity v1.1 post-retro landing audit
8fc7a08 chore(phase12): backfill substantive-commit SHA into project-state.md
a42085a fix(integrity): annotate toolkit-own grammar literals in test_suppression_fence
47104ad chore(phase12): cross-cutting edits — CI + README + CHANGELOG + project-state + capture-format
d41564d feat(lattice-boltzmann): D3Q19 BGK around a NACA airfoil — Phase 12 substantive
9c057e5 grandfather(integrity): sweep retro doc findings (v1.1 batch-1 retro companion)
[...continues...]
```

FACT — checking each commit against the v1.2-bolt-on
likely-edit set:

- `9add149`, `e26056c`, `d772803`, `8fc7a08`: docs-only, no
  toolkit code touched.
- `cdad2e2`, `c1a257d`: lattice-Boltzmann sim work; no toolkit
  files.
- `a42085a`: touches `tools/integrity/tests/test_suppression_fence.py`
  (own-source annotations); no v1.2 bolt-on overlap.
- `47104ad`: cross-cutting docs (README, CHANGELOG,
  project-state, capture-format); no toolkit-runner overlap.
- `d41564d`, `9c057e5`: Phase 12 substantive + retro-doc sweep;
  no overlap.

INFERENCE: no commits since 9add149 touch any file in the v1.2
bolt-on edit set. Working tree is clean for spec drafting.

### I.2 — Coordinator-chat A.3 work overlap

FACT — the A.3 batch-2 work scope (per addendum § 5) touches:

- `tools/integrity/integrity/cat1_citations/grammar.py`
- `tools/integrity/integrity/cat1_citations/checks/upstream.py`
- `tools/integrity/integrity/grandfather.py` (classifier rule
  for new A.3-generated category)

Overlap with v1.2 bolt-ons:

- P1.5: `cat3_numerical/checks/{__init__.py, d3q19_*}` — no
  overlap with A.3 work.
- P1.6: `runner.py:141-145` — no overlap.
- P1.7: `cat2_contracts/checks/stub_label_stale.py:1-25` — no
  overlap.
- P1.8: reads `grandfather.py:Classification.category` and
  `grandfather.py:classify()` results; introduces a
  `apply_annotations()` parameter or upstream filter; adds CLI
  flag to `scripts/grandfather_sweep.py`. **`grandfather.py`
  overlap with A.3.**

FACT — current state of `grandfather.py`: no uncommitted edits;
last modification in commit `f661ec4` (v1.1 batch 1 commit 2)
which added fence-block awareness. The
`Classification` dataclass shape and the `classify()` signature
have not changed since.

INFERENCE — coordination risk between P1.8 and A.3:
- P1.8 reads `Classification.category` but does not modify the
  `classify()` body or the `Classification` shape.
- A.3 adds a new classifier rule and possibly a new named
  category (mid-table, before the `other-cat1` fallthrough).
- The two edits do not conflict if A.3 follows the first-match-wins
  convention and inserts its rule in the existing chain.
- Merge order matters: if A.3 lands first, P1.8 must rebase its
  filter call against the updated `classify()`; the filter call
  only inspects `Classification.category`, so the rebase is a
  no-op if A.3 preserves the dataclass shape (which it should).
- If P1.8 lands first, A.3's classifier change must compose with
  the live-source filter — but the filter operates on
  `category` values regardless of which rule produced them, so
  any new category that maps to AUDIT-DOC/TOOLKIT-DOC paths is
  naturally exempt from the filter; any new category that maps
  to LIVE-SOURCE paths but is named (not `other-cat1`) is
  intentionally sweepable, also no conflict.

The classifier read is one-shot (read `category` value), and
P1.8's logic doesn't depend on which classifier branch produced
the value. **Read-only on this side; safe to land before or
after A.3.**

### I.3 — Banked rebase note for spec execution

If `grandfather.py` accumulates additional edits between the
v1.2-bolt-ons spec-draft time and the spec-execution time
(plausible given the coordinator-chat A.3 work), the spec's
verification block should include:

1. `git pull` + check `grandfather.py` for any new rules.
2. Verify the `Classification` dataclass shape is unchanged
   (`category: str` field still present and accessed by name).
3. Re-classify the 4 live-source hard-fails (Section G.5
   above) and confirm all still resolve to `other-cat1`. If A.3
   introduces a new named category that covers one of them
   (e.g., `phase-spec-basename`), then the live-source filter
   becomes partial coverage for that case — flag in the spec.

---

## Section J — Banked observations

### J.1 — Registry-vs-registered cross-sweep

FACT — `used_by_checks` values from `ground-truth-sources.md`:

```
23:used_by_checks = ["cat1.upstream-citation", "cat1.upstream-anchor", "cat3.cubic-kernel"]
31:used_by_checks = ["cat1.upstream-citation", "cat1.upstream-anchor"]
39:used_by_checks = ["cat3.d3q19-velocity-set", "cat3.d3q19-weights", "cat3.d3q19-equilibrium"]
```

FACT — registered CHECK_IDs (`grep -rn "CHECK_ID = "
tools/integrity/integrity/`):

```
cat1.upstream-citation       (registered: cat1_citations/checks/upstream.py)
cat1.upstream-anchor         (registered: cat1_citations/checks/upstream_anchor.py)
cat1.unregistered-upstream   (registered: cat1_citations/checks/unregistered_upstream.py)
cat1.intra-repo              (registered: cat1_citations/checks/intra_repo.py)
cat1.annotation-form         (registered: cat1_citations/checks/annotation.py)
cat2.public-symbol-used      (registered: cat2_contracts/checks/public_symbol_used.py)
cat2.public-symbol-used-c    (registered: cat2_contracts/checks/public_symbol_used_c.py)
cat2.public-symbol-used-ts   (registered: cat2_contracts/checks/public_symbol_used_b.py)
cat2.stub-label-stale        (registered: cat2_contracts/checks/stub_label_stale.py)
cat3.cubic-kernel            (registered: cat3_numerical/checks/cubic_kernel.py)
```

Cross-check:

| Registry-declared check | Registered? |
|---|---|
| `cat1.upstream-citation` | ✔ |
| `cat1.upstream-anchor` | ✔ |
| `cat3.cubic-kernel` | ✔ |
| `cat3.d3q19-velocity-set` | ✗ |
| `cat3.d3q19-weights` | ✗ |
| `cat3.d3q19-equilibrium` | ✗ |

INFERENCE — only the three d3q19 entries are drifted. No other
registry stanza declares a phantom check. The P1.5 scope
captures the entire registry-vs-implementation drift surface
correctly; no surprise additional unregistered checks lurking.

The deferred A.2 ("toolkit self-application") check, when
implemented, has exactly this cross-check as its acceptance
criterion: enumerate registry `used_by_checks` ∪ registered
CHECK_IDs, fire HARD_FAIL on any registry-only entries.

### J.2 — Other potential docstring drift

FACT — `grep -l "literal-mirror\|Decision 2"
tools/integrity/integrity/` (recursive): only
`tools/integrity/integrity/cat2_contracts/checks/stub_label_stale.py`
matches.

INFERENCE — no other toolkit modules carry the "literal-mirror"
or "Decision 2" phrasings, so no other docstring is at risk of
drifting against the same execution-spec correction. The P1.7
scope captures the full surface for this specific
pause-and-surface's stale docstring.

A wider docstring-drift audit would require a broader query
(per-module comparison of module-level docstring claims vs
function-level signature realities). Out of scope for v1.2
bolt-ons but worth banking for a future A.x-class deferred item.

### J.3 — No pre-existing `test_*_live_source*` tests

FACT — `find tools/integrity -name "test_*live*" -o -name
"test_*source*"`: zero matches. The test file list under
`tools/integrity/tests/` (verbatim):

```
conftest.py
fixtures
test_cat1_annotation_fence.py
test_cat1_annotation.py
test_cat1_intra_repo_fence.py
test_cat1_intra_repo.py
test_cat1_unregistered.py
test_cat1_upstream_anchor.py
test_cat1_upstream_fence.py
test_cat1_upstream.py
test_cat2_stack_b.py
test_cat2_stack_c.py
test_cat2_stack_d.py
test_cat2_stub_label_stale.py
test_cat3_cubic_kernel.py
test_grandfather_sweep.py
test_runner.py
test_snapshot.py
test_suppression_fence.py
```

INFERENCE — P1.8 is the first place the "live-source"
abstraction lands in toolkit code. No prior in-tree tests have
considered this distinction. Naming convention is open in the
spec; suggested names (matching the existing
`test_grandfather_sweep.py` pattern) would add new tests under
that file rather than a separate
`test_grandfather_sweep_live_source.py` — closer to existing
test-file granularity.

### J.4 — `grandfather_sweep.py` thinness

FACT — the CLI wrapper is 31 LOC (Section F.1). All logic lives
in `integrity.grandfather.apply_annotations`. The CLI is
unusually thin — every existing argument (`--dry-run`,
`--repo-root`) flows directly through to one of two function
parameters.

INFERENCE for P1.8 spec drafting: the `--sweep-live-source`
flag's natural home is the same shape — add a parameter to
`apply_annotations()`, pass through the CLI flag with the same
default. The CLI surface stays predictable (one-to-one
arg↔parameter mapping), and the test surface (which exercises
`apply_annotations` and helpers directly, not the CLI wrapper)
stays simple.

Alternative: a `is_live_source_path(file: str) -> bool` helper
exposed as a module-level public function in
`integrity.grandfather` (or `integrity.common.paths`) used by
both `apply_annotations` and any future code that needs the
bucket distinction. Makes the abstraction reusable beyond the
single CLI gate.

### J.5 — Stack D (Python) sibling-impl rule status

FACT — Section E.1 module docstring claims `.py` impl is "the
same file." `_resolve_impl_path` (Section E.2 lines 125-126)
implements the same — returns `header_path` unchanged for `.py`
files under `common-py/gpusims_common/`. The two docstrings agree
on the Python side. Drift is only on the C++ side.

INFERENCE — P1.7 fix scope is the lines 15-17 (`.hpp`/`.h`
description). The Python and Stack D guard lines (lines 18,
20-24) are unaffected. Minimum touch: replace lines 15-17 with
the namespace-strip description matching lines 98-105. Touches
at most 3 lines.

### J.6 — Test count baseline post-batch-1

FACT — the addendum § 2.7 records "96 tests collected" against
the v1.1 apispec probe's 74-test baseline (+22). The probe did
not re-count today. Spec drafting may want to confirm with a
local `pytest --collect-only tools/integrity/tests/` to baseline
the v1.2 bolt-on test additions (P1.5 → ~4-8 new tests, P1.6 →
1-2 new tests, P1.7 → 0 new tests, P1.8 → ~4-6 new tests; total
~9-16 new tests, taking the count to ~105-112 post-v1.2-bolt-on).

### J.7 — Convention F (audit-prose freshness) reaffirmed for this probe

This probe sourced every line-number citation from
`grep -n`/Read calls executed in this probe run. No line
numbers carried forward from prompts. Discrepancies with prior
citations (e.g., `runner.py:893` in 9add149 commit message,
`stub_label_stale.py:644-677` in earlier scoping) flagged as
FACT discrepancies rather than propagated. Convention F holds
for the probe-author side.

---

## Closing

Read-only constraint honored: no files modified, no commits
made, no pushes. The only side-effect was capture of integrity
stdout to `/tmp/integrity_full.txt` (a probe-time scratch file
outside the repo).

SHA at probe end (`git rev-parse HEAD`):
`9add1494b237e33f3dda782c821b9d7f29446068`. Matches probe-start
SHA. No drift during probe.

Probe scope-coverage map for spec drafting:

- **P1.5** — Sections B, C cover all surface area.
  B.1 inventories the harness shape; C.1-C.7 establish the
  registration pattern; C.7's INFERENCE recommends three
  separate check modules.
- **P1.6** — Section D covers the bug confirmation, the
  asymmetry, the fix site, and the line-number discrepancy
  reconciliation.
- **P1.7** — Section E covers the docstring drift confirmation
  with both docstrings dumped verbatim and the implementation
  bytecode-equivalent.
- **P1.8** — Sections F, G cover the CLI surface, the
  classifier read-only surface, the bucket cross-reference, and
  the 4-hard-fail classification confirmation.

Banked observations (Section J): no other registry-vs-implementation
drift; no other "literal-mirror"-style docstring drift; no
pre-existing live-source tests; CLI wrapper is thin enough for
flag-and-parameter pass-through; Python sibling-impl rule is
unaffected by P1.7; ~9-16 new tests projected.

## End of probe
