# Integrity Toolkit v2 — Specification

> **This is the toolkit specification.** It describes what the v2 toolkit
> is at completion: its architecture, categories of check, canonical
> reference bindings, implementation phases, and design decisions.
>
> The audit trail — methodology conventions, probe records, banking log,
> revision history — lives in `audit-trail.md`. The two documents are
> intended to be read independently: this one for "what is the toolkit",
> that one for "how we built the spec and verified it".

---

## § 0 — First-principles framing

### 0.1 What the toolkit is

The integrity toolkit is a **code verification framework** for the
GPU-Sims repository in the scientific-computing sense (Roy 2005;
Oberkampf-Roy 2010): a set of automated procedures that detect coding
mistakes affecting the numerical discretization of the published
algorithms each sim claims to implement. It is the **canonical
authority layer** for sim correctness.

The authority claim has four load-bearing pillars:

1. **Mechanical verification** — checks run automatically, find, and
   classify. Not human judgment.
2. **Anchored to a documented basis** — each correctness claim cites a
   peer-reviewed reference (canonical-reference principle, § 1.0).
3. **Reproducible** — same input + same seed + pinned environment →
   same output (hermetic builds § 1.7.L, Cat 4 goldens, FP determinism
   policy § 3.15).
4. **Independent of the system it verifies** — the verification is
   authored separately from the sim, by separate workflows
   (multi-Claude-chat orchestration + Claude Code dual-agent
   discipline; § 1.7.O).

Every category in § 1 supports one or more of these pillars. Loss of
any pillar compromises the authority claim.

### 0.2 What the toolkit is NOT

Per the V&V vocabulary (Roy 2005):

| Term | This toolkit |
|---|---|
| **Code verification** — "solving the equations right" | **In scope** (Cat 3). |
| **Solution verification** — quantifying numerical error in each specific run (Richardson extrapolation, GCI) | **Out of v2 scope.** Sims are graphics demos, not error-controlled prediction tools. |
| **Validation** — vs. experimental data | **Out of scope, indefinitely.** No per-sim experimental campaigns are planned. |
| **Uncertainty Quantification** | **Out of scope, indefinitely.** |

The toolkit verifies the implementation is faithful to the cited
paper. It does not claim the algorithm matches physical reality.

### 0.3 Category hierarchy

Of the seven check categories, **Cat 3 is load-bearing for the
authority claim**. The others are authority-supporting infrastructure:

| Layer | Category | Role |
|---|---|---|
| **Core** | Cat 3 — numerical correctness | The authority claim itself (MMS + invariants + algebraic). |
| **Basis** | Cat 1, Cat 2, Cat 5 | Citation integrity, contract verification, cross-source consistency. Protect the documented basis. |
| **Durability** | Cat 4 — runtime integration | Regression-locks the verified state. |
| **Tooling** | Cat 6, Cat 7 | Build hygiene, security. Protect the tooling that supports the authority. |
| **Meta** | § 1.7 — toolkit-itself standards | Protect the toolkit's own credibility. |

This hierarchy is informational, not gating. All seven run in CI; all
seven gate PRs. The hierarchy informs **priority of investment, depth
of review, and order of design discussion**: a Cat 3 defect directly
compromises the authority claim; a Cat 6 defect is one step removed.

### 0.4 TDD-restoration framing

The toolkit is being built after the sims it verifies. Test-driven
development would have written verification first, then code. The v2
design absorbs this discipline retroactively via the Phase 11
dependency graph (§ 4.X): each sim-conformance fix lands **before**
the Cat 3 invariant that depends on the canonical convention. The
invariant becomes the regression guard, not the diagnostic tool.

TDD restoration is complete when every shipped sim has at least one
Cat 3 invariant that goes red on a deliberate regression of the
canonical convention, and that invariant lives in the toolkit gate
at HEAD.

---

## Scope of v2

Six categories of check plus runtime infrastructure plus continued
hygiene. v2 closes the gaps in v1 categories (Cat 1, 2, 3), adds three
new categories (Cat 4, 5, 6), and adds sub-checks bridging categories
(determinism-pair, replay-determinism, visual regression).

Estimated total commit count: **~241 substantive + ~57 SHA back-fill
across 12 phases (Phase 0 + Phases 1-11)**. Estimated duration:
9-12 months. Direction: correctness over urgency.

### 0.4 CI baseline at HEAD `351c66e` (landed precedent)

Per Probe HHH (banking row 89): four closeout commits (4–8) landed
during v2 spec authoring; the spec under-acknowledges this. The
following CI capabilities are **already at HEAD** and v2.X batches
that reference them should treat them as starting state, not future
work:

- **Paired grandfather-sweep enforcement on PR** —
  `integrity.yml:56-68` runs
  `python3 tools/integrity/scripts/check_paired_sweep.py` on every PR
  (closeout commit 4, `c7e97bd`). Spec § 2.6.4 / § 1.7.R discussions
  of grandfather-sweep CI start from this baseline.
- **Cat 3 Stack C driver build in CI** — `integrity.yml:88-103`
  configures CMake with `-DGPU_SIMS_BUILD_INTEGRITY_CAT3=ON` and
  builds the `integrity_cat3_stack_c` ninja target on every run. § 1.3.E
  v2 work is "extend the existing driver to load GLSL via lavapipe",
  not "author the driver".
- **SPlisHSPlasH vendor-clone at pinned SHA**
  `6bff55a6eaf14083d34650f22a268ce156b62b54` — `integrity.yml:27-31`.
  § 1.5.F (`cat5.vendored-anchor-fresh`) check builds against this
  precedent.
- **Stack D combined-install isolation smoke** —
  `build-py.yml:75+` installs all Stack D sims into one venv and
  asserts cross-package class identity. § 1.5 cross-source consistency
  uses this as the precedent for symbol-isolation checks.
- **Stack C Release + Debug build matrix** — `build-native.yml` runs
  both. § 1.4.H Cat 4 memory work is "add `-fsanitize=address` to the
  existing Debug job", not "create a Debug matrix from scratch."
- **Concurrency-group cancellation on integrity.yml** —
  `integrity.yml:11-13` with `cancel-in-progress: true`. Cat 6
  precedent for CI-cost discipline.
- **Structural assertion workflow** — `structure.yml` checks 24 dirs,
  16 files, 14 sim stubs. Out-of-toolkit-scope today; v2 § 1.6 either
  folds into Cat 5/6 or lets it stand. Triage at v2.30.

---

## § 1 — Category structure (Cat 1 through Cat 7) + Canonical reference principle

### 1.0 Canonical reference wins

**Principle.** The toolkit encodes canonical conventions from peer-
reviewed literature. Sim implementations are scored against those
conventions, not the other way around. Where a sim diverges from
its canonical reference, the divergence must be either:

- **(a) Explicitly documented as defensible** — with rationale recorded
 in the sim's spec doc and cross-referenced in this design — for
 reasons such as: computational simplicity for educational/graphics
 purposes; hardware constraint (e.g., GPU-friendly memory layout);
 aesthetic intent (e.g., over-driven buoyancy for visual impact);
 legacy compatibility with prior versions.
- **(b) Scheduled for conformance-fix** in a dedicated v2 batch under
 Phase 11.

Divergences that are neither documented-defensible nor scheduled are
treated as live-source-red findings per the live-source-stays-red discipline.

**Rationale.** The toolkit is the canonical source for checking the
physics; sims are updated when load-bearing decisions need to be made
for the toolkit to be as strong as possible. The toolkit's *physics*
must be canonical — not a reflection of whatever the shader happened
to implement. Where the shader chose a non-canonical convention for
good reason, document it. Where the shader is just wrong, fix the
shader.

**What this is not.** Not a mandate that every sim use the latest
research-frontier algorithm. MLS-MPM is from 2018 and remains
canonical for graphics MPM despite newer variants (PolyPIC, CK-MPM,
Mapped MPM). DFSPH is from 2015/2017 and remains canonical despite
ongoing SPH research. The "canonical" choice is the published
reference that the sim claims to implement — typically the
most-cited paper or the SIGGRAPH-era textbook for the technique.

### 1.0.A How the toolkit encodes canonical references

Three places in the toolkit explicitly bind to canonical references:

1. **Cat 3 invariant authoring.** Every Cat 3 check spec opens with
 a "Canonical reference" header citing the paper / textbook /
 section that defines the convention being verified. Reviewers
 can fact-check the check against the reference without reading
 the sim source.

2. **Cat 5 cross-source consistency.** A new sub-check
 `cat5.canonical-reference-cited` verifies that every sim's spec
 doc (`docs/sim-specs/<sim>.md`) names its canonical reference
 in a structured front-matter block. Forces explicit declaration
 rather than implicit assumption.

3. **Sim-conformance batches (Phase 11).** Each scheduled sim-fix
 commit's audit report opens with "Canonical reference" and
 "Current divergence" sections. The fix is the diff that closes
 the divergence; the audit is the explanation.

### 1.0.B Per-sim canonical reference lock table

These are the canonical references claimed by each sim, sourced
from the sim's own `docs/sim-specs/<sim>.md` § 11 + per-sim README +
shader/kernel comments. **14 sims in repo inventory**: 11 with
shipping implementations (rows 1-3, 5-13), 3 README-only stubs
of varying maturity (rows 4, 12, 14).

All FACT-tagged unless noted; FACT means the cited reference appears
verbatim at the anchor; INFERENCE means implied by terminology /
parameter values / algorithm name; INFERENCE-FROM-INTENT means
inferred from the sim's stated goal in non-spec material (e.g.,
README) where no implementation exists yet to read.

| Sim path | Canonical reference | Tag | Maturity | In-tree anchor |
|---|---|---|---|---|
| `continuous-ca/lenia-fft/` | Chan 2019 "Lenia: Biology of Artificial Life" arXiv:1812.05433 + Chakazul/Lenia reference impl | FACT | shipping | `docs/sim-specs/lenia-fft.md:99-100`; `continuous-ca/lenia-fft/python/lenia_fft/kernels.py:21-22` |
| `continuous-ca/reaction-diffusion-2d/` | Pearson 1993 *Science* 261(5118):189-192 | FACT | shipping | `docs/sim-specs/reaction-diffusion-2d.md:54,178`; `BufA.glsl:4-5,15` |
| `continuous-ca/reaction-diffusion-3d/` | Pearson 1993 *Science* 261:189-192 (extended to 3D) | FACT | shipping (spec header stale) | `docs/sim-specs/reaction-diffusion-3d.md:208-209`; `continuous-ca/reaction-diffusion-3d/src/main.cpp:5,81-92` |
| `continuous-ca/neural-ca/` | Mordvintsev, Randazzo, Niklasson, Levin 2020 "Growing Neural Cellular Automata" *Distill* doi:10.23915/distill.00023 | **INFERENCE-FROM-INTENT** | **stub (README-only)** | `continuous-ca/README.md:9` paraphrase of Distill title |
| `agent-based/boids-3d/` | Reynolds 1987 "Flocks, herds and schools: A distributed behavioral model" *SIGGRAPH Comput. Graph.* 21(4):25-34 | FACT | shipping | `docs/sim-specs/boids-3d.md:231`; `web/shaders/flock_update.compute.wgsl:1-4,138` |
| `agent-based/physarum/` | Jones 2010 "Characteristics of pattern formation and evolution in approximations of physarum transport networks" *Artif. Life* 16(2) | FACT | shipping | `docs/sim-specs/physarum.md:49`; `web/shaders/agent_move.compute.wgsl:2,96` |
| `closed-form/strange-attractors/` | v1: Lorenz 1963 *J. Atmos. Sci.* + Aizawa 1984 *Prog. Theor. Phys.* + Thomas 1999 *IJBC*. v1.1 banked: Halvorsen (community-attributed; no primary publication), Rössler 1976, Sprott 1994, Chen-Ueta 1999 | FACT (v1); pending (v1.1) | shipping (spec header stale) | `docs/sim-specs/strange-attractors.md:221-224` |
| `closed-form/mandelbulb-explorer/` | White 2009 "The Mandelbulb: First 'true' 3D image of the famous fractal" (self-published; FractalForums.com). **Borrows iq's polynomial colormap from Shadertoy view/WlfXRN** (fragment-level dep). | FACT | shipping (spec header stale) | `docs/sim-specs/mandelbulb-explorer.md:182-186`; `closed-form/mandelbulb-explorer/shadertoy/mandelbulb.glsl:5` |
| `volumetric-grid/eulerian-smoke/` | Fedkiw, Stam, Jensen 2001 SIGGRAPH "Visual Simulation of Smoke" (primary). **Euler-plus features:** vorticity confinement (Fedkiw 2001 eq 14), Boussinesq buoyancy + density-downforce, scalar exponential decay. **Discretization:** MacCormack/Selle 2008 advection, Jacobi pressure. | FACT | shipping | `docs/sim-specs/eulerian-smoke.md:36-39,138-142`; `shaders/advect_velocity.comp.glsl:7,18,105` |
| `volumetric-grid/lattice-boltzmann/` | **Split attribution:** Krüger et al. 2016 *The Lattice Boltzmann Method: Principles and Practice* Springer (BGK pattern + halfway-BB convention, D2Q9). **D3Q19 velocity set + weights + equilibrium:** `tools/integrity/docs/algebraic/d3q19.md`. **Collision: SRT/BGK**, τ default 0.6. | FACT | shipping | `docs/sim-specs/lattice-boltzmann.md:63-66`; `volumetric-grid/lattice-boltzmann/src/main.cpp:7`; `tools/integrity/docs/ground-truth-sources.md:25-34` |
| `particle-fluids/sph-water/` | Bender & Koschier 2015 + 2017 "Divergence-Free SPH" — DFSPH solver. **Reference impl:** SPlisHSPlasH v2.16.1 at SHA `6bff55a6eaf14083d34650f22a268ce156b62b54` (upstream tag, verified against `github.com/InteractiveComputerGraphics/SPlisHSPlasH` master HEAD). In-tree at `references/SPlisHSPlasH/` as a detached-HEAD git checkout at the same SHA. Pressure storage R2 / stencil R1 currently mixed; v2.45 scheduled fix. | FACT | shipping (spec is template-stub) | `particle-fluids/sph-water/README.md:10`; `particle-fluids/sph-water/docs/load-bearing-decisions.md:8-9`; `particle-fluids/sph-water/shaders/compute_pressure_accel.comp.glsl:10` |
| `particle-fluids/pic-flip/` | Zhu & Bridson 2005 SIGGRAPH "Animating Sand as a Fluid" — FLIP particle-grid coupling | **INFERENCE-FROM-INTENT** | **stub (README-only)** | `particle-fluids/README.md:8` |
| `hybrid-particle-grid/mpm-multimaterial/` | **Three-layered:** Hu et al. 2018 SIGGRAPH "MLS-MPM" (transfer) + Jiang et al. 2015 SIGGRAPH "APIC" (affine matrix; **citation absent from `mpm-multimaterial.md` § 11 row 13 — recommended sim-spec edit out of rev-doc scope**) + **Stomakhin et al. 2013 SIGGRAPH "MPM for Snow"** (elasto-plastic constitutive — confirmed faithful for SNOW branch). **Parameter divergences from Stomakhin Table 2**: E=1000 vs 1.4e5; θ_s=4.5e-3 vs 7.5e-3 (both Taichi `mpm3d_ggui` upstream-derived). | FACT | shipping | `docs/sim-specs/mpm-multimaterial.md:38-39,71,297-301`; `hybrid-particle-grid/mpm-multimaterial/python/mpm_multimaterial/kernels.py:104-129` |
| `quantum/ising-dwave/` | **Two-tier.** **Q-track (D-Wave quantum-annealing):** Harris et al. 2018 *Science* 361:162-165 "Phase transitions in a programmable quantum spin glass simulator" → King et al. 2022 *Nat. Phys.* 18:1324-1328 "Coherent quantum annealing in a programmable 2,000 qubit Ising chain" → King et al. 2024/2025 *Science* 388:199-204 (DOI 10.1126/science.ado6285). "Harris-King 2018→2024" is shorthand for this lineage and does **not** assert co-authorship on Harris 2018. **C-track (classical dev-loop):** Metropolis et al. 1953 *J. Chem. Phys.* 21:1087-1092 + Wolff 1989 *Phys. Rev. Lett.* 62:361-364 (cluster algorithm) + Onsager 1944 *Phys. Rev.* 65:117-149 (2D Ising exact solution). **NOT Glauber 1963** — explicitly out of scope. | FACT | **stub (category-context doc only)** | `docs/category-contexts/quantum.md:124-138`; `quantum/README.md:3`; `docs/overarching-spec.md:47-52` |

**Spec doc hygiene** (v2.43 batch targets — 4 sims at template-stub class, not 1):
- `docs/sim-specs/sph-water.md` is still `_template.md` skeleton
- `docs/sim-specs/neural-ca.md` is template-stub
- `docs/sim-specs/pic-flip.md` is template-stub
- `docs/sim-specs/ising-dwave.md` is template-stub (but the
 `docs/category-contexts/quantum.md` 396-line category-context doc
 is substantively ahead — see)
- Stale "Specification pending" header on `reaction-diffusion-3d.md`,
 `strange-attractors.md`, `mandelbulb-explorer.md` — bodies complete
- Other 7 sims have full specs

**Stomakhin parameter divergence documentation**
(low-severity v2.0 closeout-polish target — 
verification against Stomakhin 2013 PDF):
- Add one-line citation comment at `kernels.py:118-119` citing
 Stomakhin 2013 **§ 4.1 (full method as 10 numbered steps; no
 formal "Algorithm 1" label in the paper)** + **§ 5 (constitutive
 model where the elasto-plastic energy density and Lamé-parameter
 functions are defined)**. Rev 10 said "§ 5 + Algorithm 1"; the
 algorithm label is not present in the paper — the 10-step "Full
 method" is in § 4.1 as numbered prose.
- Add note to `docs/sim-specs/mpm-multimaterial.md` § 2 documenting
 E=1000 (Taichi normalized-unit-cube convention, not Stomakhin Pa;
 canonical E_0 from **Stomakhin 2013 Table 2** is 1.4×10⁵ Pa — rev
 10 incorrectly said "Table 1"; Table 1 is the methods-comparison
 table, Table 2 is the parameter table) and θ_s=4.5e-3 (inherited
 from upstream `mpm3d_ggui`; canonical θ_s from Stomakhin 2013
 Table 2 is 7.5×10⁻³)
- Not a Phase 11 batch — folded into v2.0 closeout polish

**Vendoring policy**: 2 references vendored in-tree
(`references/lbm-principles-practice/` Krüger book code +
`references/SPlisHSPlasH/`). All other sims implement the math from
the published paper without lifting upstream code.



### 1.0.C Optimization and algorithm-choice considerations

Where a sim could legitimately choose between multiple
canonical-quality algorithms (not divergent implementations, but
genuinely different methods), rev 6 explicitly notes the choice
rather than ratchet our scope creep.

| Sim | Algorithm choice axis | Current | Alternatives | Recommendation |
|---|---|---|---|---|
| sph-water | Pressure solver | DFSPH | IISPH, PCISPH, PBF, WCSPH, PF | Keep DFSPH (state-of-the-art per SPlisHSPlasH, BK17 benchmarks) |
| lattice-boltzmann | Collision scheme | SRT/BGK (τ=0.6) | (alternatives: TRT, MRT, Cumulant, HRR) | Defensible for graphics use case; Path A planned at v2.47 to fix shifted-variant halfway-BB. |
| eulerian-smoke | Advection method | MacCormack-corrected SL (Selle/Fedkiw 2008) + reverse-Stam clamp | (alternatives: vanilla SL, BFECC) | Already optimal; v2.51 is documentation-only. No upgrade scheduled. |
| mpm-multimaterial | Transfer scheme | APIC | PIC, FLIP, APIC, PolyPIC, MLS | Keep APIC + MLS (state-of-the-art; CK-MPM 2024 is newer but adds dependency) |
| reaction-diffusion-{2d,3d} | Time integration | Forward Euler | Strang splitting, IMEX, exponential time differencing | Keep forward Euler for graphics; document stability condition |

The rule of thumb: don't change a working algorithm choice unless
a Cat 3 invariant cannot be expressed in the current convention.
Convention-mismatch (RD stencil normalization, LBM BB variant) is
*documentation* work, not *replacement* work.

### 1.0.D V&V framework alignment (Cat 3 deliverables under Roy 2005)

Per § 0.2: Cat 3 is code verification. This subsection states which
code-verification deliverables Cat 3 ships, and which Roy/Oberkampf
deliverables are deliberately not claimed.

**In-scope Cat 3 deliverables:**

- **Order-of-accuracy verification.** Each MMS check computes the
  *observed* order of accuracy across ≥ 3 grid resolutions and asserts
  it matches the *formal* order of the discretization within
  tolerance. § 1.3.B's slope clauses for LBM and MPM lift to a Cat
  3-wide convention at v2.5 spec authoring time.
- **Algebraic point evaluation.** Closed-form algorithms (D3Q19
  weights, cubic kernel, Wolff bond probability) verify
  bit-exact / tolerance-bounded agreement at canonical inputs.
- **Conservation-law invariants.** Implementations with provable
  conservation properties (MPM mass / momentum / energy non-increase)
  maintain them at every frame.
- **Bounded-state invariants.** Implementations with provable bounds
  (Lenia state ∈ [0,1], boids \|v\| ≤ max_speed) respect them.

**Out-of-scope:**

- **Solution verification** (per-run discretization error estimation).
  Out of v2; reopen if any sim transitions from graphics demo to
  error-controlled prediction.
- **Validation** (experimental comparison). Out indefinitely.
- **UQ** (uncertainty propagation). Out indefinitely.

---

### 1.1 Cat 1 — Citation integrity (v1, polished in v2)

**Defined as:** every `file:line` style citation in source, shader,
doc, or test resolves to a real file at a real line.

**v1 state:** four checks live (intra-repo, bare-path, upstream-citation,
upstream-anchor); annotation grammar and suppression mechanism mature.

**v2 polish:**

- *G.2 grammar fix* — extend annotation reason regex to allow balanced
 parentheses containing internal semicolons. One-time sweep to
 identify other affected annotations.
- *Multi-line citation grammar* — parser accepts citations split across
 lines.
- *Fenced-block awareness* — parser recognizes markdown fenced code
 blocks and suppresses `cat1.annotation-form` findings inside them
 natively.
- *Stack A coverage* — citations in Stack A shadertoy `.glsl` files
 (two sims, three files, 278 LOC) participate in
 cat1.intra-repo. Plus new `cat1.shadertoy-port-mapping` for
 README structure.

### 1.2 Cat 2 — Contract verification (v1, expanded in v2)

**Defined as:** every public API field, function, and declared behavior
in `common/common-cpp/`, `common/common-web/`, `common/common-py/`
has implementation matching its declaration.

**v1 state at HEAD `351c66e`** (per Probe III, banking row 88):
- Stack B: `tools/integrity/integrity/cat2_contracts/stack_b.py` (135 LOC; TS compiler API).
- Stack C: `tools/integrity/integrity/cat2_contracts/stack_c.py` (libclang with token-scan fallback). The unused `_find_matching_field_at_token` scaffold at `tools/integrity/integrity/cat2_contracts/stack_c.py:372-403` remains the obvious extension point for USR-aware resolution.
- Stack D: `tools/integrity/integrity/cat2_contracts/stack_d.py` (272 LOC; Python AST + Stack D self-application surface from § 3.6).

**Module / check-ID naming inconsistency** (per Probe III, banking row 88):
`stack_b.py` registers as `cat2.public-symbol-used-ts` while `stack_c.py` /
`stack_d.py` align with their stack letter (`-c` / `-d`). The `-ts` is
internally consistent with "TypeScript" but breaks the b/c/d module
naming pattern. **Resolution queued for v2.33**: either rename module
`stack_b.py` → `stack_ts.py` (matches check ID) or rename check ID
`cat2.public-symbol-used-ts` → `cat2.public-symbol-used-b` (matches
module). Recommend the former (TS is the public-facing name; the file
naming is the internal inconsistency).

**v2 expansion:**

- *Type-aware Stack C member access.* Wire in
 `_find_matching_field_at_token`. Resolves the name-collision
 false-MISS class. : this helper is currently unused;
 ~50 LOC of integration plus fixture tests.
- *Cross-stack capture-format contract.* 's finding that
 three writers (`state_writer.py`, `state_writer.cpp`,
 `stateWriter.ts` — total ~890 LOC) hand-maintain the same schema
 with drift (`rgba16f` vs `rgba16float`, `find_latest` by mtime vs
 by lexical order). New `cat2.capture-schema-consistent` check
 verifies the three writers emit byte-compatible files and the three
 readers parse them equivalently. Implementation: fixture-based —
 write canonical data from each writer, load from each reader, assert
 round-trip equivalence.

### 1.3 Cat 3 — Numerical correctness vs upstream (v1, dramatically expanded in v2)

**Defined as:** where the codebase claims to implement an upstream
algorithm, that implementation can be mechanically compared against
the upstream reference at chosen test inputs, agreeing within
documented tolerance.

**v1 state:** cubic-kernel check + four LBM D3Q19 algebraic checks
(velocity-set, weights, equilibrium, plus the kernel). Registry
supports vendored-upstream and algebraic-derivation ground-truth
sources. the algebraic-entry skip-branch is live and
`[Algebraic_D3Q19]` is the working precedent.

**v2 expansion** (seven work streams; numbered for cross-reference):

#### 1.3.A Quantum cat3 seed (existing scope)

Per `docs/category-contexts/quantum.md` § 6.1, four candidate checks:

- `cat3.ising-energy` — algebraic. `H = -Σ J_ij s_i s_j - Σ h_i s_i`.
- `cat3.qubo-ising-roundtrip` — vendored against `dimod`. `s = 2x - 1`
 and coefficient transformation.
- `cat3.wolff-bond-probability` — algebraic. `p = 1 - exp(-2βJ)`.
- `cat3.onsager-tc` — algebraic. `kT_c/J = 2/ln(1+√2) ≈ 2.269185`.

All four unblocked.

#### 1.3.B Method of Manufactured Solutions (MMS) — *new framing*

**Industry standard for PDE solver verification.** Used by NASA's
internal CFD verification (per NTRS 20150015494), Sandia, Stanford
Center for Turbulence Research, the MOOSE multiphysics framework
(`mooseframework.inl.gov/python/mms.html`), and MFiX (DOE,
`mfix.netl.doe.gov/doc/vvuq-manual/main/html/mms/`).

**The technique:**

1. Pick an analytic function `u_hat(x, t)`.
2. Substitute into the PDE the solver claims to solve. The residual is
 non-zero in general; call it `S(x, t) = PDE_residual(u_hat)`.
3. Modify the solver to add `S` to its right-hand side. The modified
 PDE has `u_hat` as its exact solution.
4. Run the solver. If correct, output matches `u_hat` to within
 discretization error.
5. Refine the grid; the discretization error decreases at the formal
 order of accuracy of the scheme (2nd order for typical CFD). Plot
 error-vs-grid-spacing on a log-log axis; the slope is the
 *observed* order of accuracy.
6. Compare observed-order to formal-order. Mismatch indicates bugs.

**Why this catches "subtle math wrong" bugs:** MMS doesn't depend on
real-world expected behavior; it depends on PDE correctness. A solver
with a wrong-signed pressure gradient or missed Laplacian term will
fail to reproduce `u_hat` regardless of the bug being "subtle." The
NASA report includes a deliberate-sabotage study where one author
introduced errors into a Navier-Stokes solver and the other detected
all of them via MMS.

**Apply to our PDE-based sims:**

- *eulerian-smoke* — `cat3.smoke-mms-divergence-projection`: verify
 the pressure projection step against a known divergence-free
 velocity field. `cat3.smoke-mms-advection-order`: verify
 semi-Lagrangian advection's order of accuracy via grid refinement.
 **Manufactured solution choice (rev 5 correction from 's
 2D recommendation):** Use the *3D* stream function
 `ψ(x,y,z) = sin²(πx) sin²(πy) sin²(πz)` with velocity defined as
 `u = ∇ × (ψ ẑ)`. Concretely:
 ```
 u_x = π sin²(πx) sin(2πy) sin²(πz)
 u_y = -π sin(2πx) sin²(πy) sin²(πz)
 u_z = 0
 ```
 Properties (verified algebraically from first principles, rev 5):
 1. *Divergence-free.* `∇·u = 0` everywhere by curl-of-vector-field
 identity.
 2. *Vanishes on all 6 cube faces.* On x=0 and x=1, `sin²(πx)=0`
 (in u_x) and `sin(2πx)=0` (in u_y), so all three components
 are zero. Same for y=0,1. On z=0 and z=1, `sin²(πz)=0` factors
 in u_x and u_y, and `u_z=0` always. Note: rev 4's 2D choice
 `ψ(x,y) = sin²(πx)sin²(πy)` only vanished on 4 faces — the
 z=0 and z=1 faces had non-zero tangential velocity. The
 `sin²(πz)` factor closes that gap.
 3. *Non-trivially 3D in terms of solver code paths.* All three
 velocity buffers carry signal: u_x and u_y are non-zero
 interior; u_z is identically zero. Preserving `u_z=0` through
 the 11-stage pipeline (advection, viscosity, projection, etc.)
 is itself a non-trivial correctness check — bugs that introduce
 spurious z-coupling would manifest as `u_z ≠ 0` at any frame.
 4. *BC-aligned with stage-8 zeroing.* re-anchored by
 (line 1945 not 1946), smoke's pipeline zeros velocity
 on 5 of 6 no-slip faces. The 3D ψ matches no-slip on all 6
 faces — if all 6 are no-slip, contamination is zero; if only
 5 are, the 6th face (likely outflow at top) sees `u_h = 0`
 trivially since the boundary condition coincides with the
 manufactured solution there.

 **Calibration ladder:** 48³ / 96³ / 192³ (factor-of-2 spacing);
 per-PR gate at 48³ (~5s wall-clock); full triplet on nightly
 schedule (~70s). 384³ deliberately excluded — at ~8min/run, the
 marginal observability gain isn't worth the CI cost.
 **Advection-order expectation:** the advection is
 single-pass MacCormack-corrected SL (Selle/Fedkiw 2008) with
 reverse-Stam corner-clamp limiter — NOT vanilla SL. **Expected
 slope in grid-refinement log-log plot: ≈ 2 in smooth regions, but
 the limiter can degrade order to ~1 near interior extrema.** Pass
 threshold for `cat3.smoke-mms-advection-order`: slope ≥ 1.7.
 Fail threshold: slope < 1.5. Choose MMS field without interior
 extrema in the advected quantity (e.g., a monotone Gaussian decay
 rather than a sin-wave with peaks) to avoid limiter activation
 masking the second-order signal. Do NOT set a slope-1 tolerance —
 that would silently accept a regression from MacCormack to
 vanilla SL.
 **Precision floor:** rgba16f storage caps observable convergence
 rate at ~10⁻³ error magnitude. Claims of "matches 2nd-order
 formal accuracy" only valid up to this floor; beyond it, fp16
 noise dominates and slope flattens. Document this explicitly in
 the check's pass criteria so we don't over-claim.

- *reaction-diffusion-2d / 3d* — `cat3.rd-mms-diffusion-order`:
 verify Laplacian discretization order. **`cat3.rd-stationary-pearson`
 :** verify known stationary
 patterns of the Gray-Scott reaction-diffusion model at canonical
 parameter sets from Pearson 1993 *Science* 261:5118. The Gray-Scott
 PDEs:
 ```
 ∂U/∂t = Du ∇²U − UV² + F(1−U)
 ∂V/∂t = Dv ∇²V + UV² − (F+k)V
 ```
 **Convention note (load-bearing + § 0.9 item 6):** our
 shaders use the unnormalized Laplacian convention with `Δx=1`
 (1/Δx² dropped). The conversion is `Du_ours = Du_Pearson / Δx²`;
 at Δx=1 these coincide. **In-tree presets `(Du=0.16, Dv=0.08)`
 ARE the canonical Pearson values in our graphics-port convention**
 — NOT a divergence from Pearson, and NOT a value to convert from.
 Stability headroom is 1.56× in 2D (dt=1.0 safe) and 1.04× in 3D
 (tight; explains the `clamp(_, 0, 2)` backstop in the 3D kernel).
 Canonical fixture points using our (Du=0.16, Dv=0.08) baseline:
 - **Spots** preset (in-tree named preset, F/k per `docs/sim-specs/
 reaction-diffusion-2d.md`) — stationary hexagonally-packed spots
 - **Stripes** preset — labyrinthine stationary patterns
 - **Spiral waves** preset — rotating spirals; not strictly
 stationary, periodic-stationary in co-rotating frame
 - **Turing-line boundary** — homogeneous decay; negative control
 Verification target: after long-time run from canonical
 initialization (U=1 everywhere except a small perturbation region;
 noise applied perturbation-local per Pearson, NOT globally —
 scheduled fix + v2.48), the L2 distance to a published
 reference image (stored as a Cat 4 golden under
 `<sim>/tests/integrity/goldens/rd-pearson-*.npy`) is within
 tolerance. **Note on test character:** closer to Cat 4 (snapshot
 regression against a golden) than pure Cat 3 (analytic invariant).
 Pearson patterns are emergent, not closed-form. The Cat 3 framing
 is appropriate because we're verifying the *PDE solver* produces
 *the same pattern* as the canonical reference, not that the
 simulation matches reality.

- *lattice-boltzmann* — already has algebraic checks; add
 `cat3.lbm-mms-poiseuille-flow`. The collision scheme is **SRT/BGK**
 (τ default 0.6, slider [0.51, 2.0]) with
 **shifted-variant halfway-BB** (reads `load_f` post-pull-stream
 instead of pre-pull). Two corrections layered
 on the textbook Poiseuille profile:
 1. **Scheme correction:** BGK with halfway-BB does NOT recover
 the continuous-limit parabolic profile
 `u(y) = g·h²/(8ν)·(1−4y²/h²)` exactly. Per He, Zou, Luo, Dembo
 1997 *J. Stat. Phys.* 87:115, "bounce-back actually mimics
 boundaries that move with a speed depending on the relaxation
 time τ" — i.e., wall slip is τ-dependent of order
 `(τ−0.5)²/H²` for canonical halfway-BB. **v2.7 check must pin
 τ=0.6 and compare against the BGK-corrected profile**, not the
 continuum parabola; otherwise τ becomes a hidden nuisance
 parameter masking actual bugs.
 2. **BC correction:** canonical halfway-BB is O(1/N²); our
 shifted variant is O(1/N)
 Tolerance recommendation: `rel_L2 ≤ 4e-4 at N=64`,
 `≤ 1e-4 at N=128`, with convergence-order slope ≥ 1.5 clause that
 surfaces the shifted-variant divergence as HARD_FAIL on first run
 if v2.47 takes the document-divergence path. If v2.47 takes the
 fix-path (refactor to canonical halfway-BB), tolerance becomes
 `≤ 1e-4 at N=64` with slope-2 expectation. **Path decision per
 :** Path A (fix) — no defensible reason for the shift was
 surfaced; SRT/BGK + canonical halfway-BB is the intent per
 `tools/integrity/docs/algebraic/d3q19.md` + Krüger reference.

- *sph-water* — `cat3.sph-mms-pressure-poisson`: verify the DFSPH
 pressure-Poisson solver convergence at canonical particle config.
 **
 and 's independent code re-read both
 identified a `~1e6×` pressure-coupling inconsistency in our shaders
 (storage of `α/ρ²` mixed with stencil dividing raw `p` by `ρ²`).
 Path A (fix-first): land the 6-line shader fix as
 **v2.45 commit 1** (not v2.7 — sim-conformance batch precedes the
 invariant per § 4.X dependency graph) *before* the MMS invariant;
 the invariant then guards against regression rather than being
 the diagnostic tool. **Validation oracle:** static hydrostatic
 equilibrium. 1D column of ~125k particles, gravity downward,
 no-flux side walls. Settled-state pressure profile should be
 `p(depth) = ρ_water · g · depth` (linear, slope ≈ 9810 Pa/m for
 water in Earth gravity). DFSPH-specific: this is the pressure
 *correction* that balances gravity to keep velocity divergence-
 free; not the absolute pressure. For weakly-compressible WCSPH
 variants the relationship is different (involves the Tait equation
 of state); for DFSPH the linear-in-depth result is direct. Per
 Bender & Koschier 2017 Figure 2 reference dam-break, 125k
 particles at h=0.04, r=0.01 is a well-tested resolution. Sweep
 iteration count `n_iter ∈ {1,2,4,8,16,32}` to verify convergence;
 default scene's 1-2 fixed iterations is
 (no convergence check; scheduled fix v2.46).

- *mpm-multimaterial* — **renamed from `cat3.mpm-mms-momentum-transfer`
 to four conservation-law checks + v2.50 batch:**
 - `cat3.mpm-mass-conservation` — Σ particle mass = Σ grid mass
 after P2G to MLS-MPM quadratic-weight epsilon (~1e-5 in f32)
 - `cat3.mpm-linear-momentum-conservation` — Σ particle momentum +
 APIC affine correction = Σ grid momentum (pre-gravity)
 - `cat3.mpm-angular-momentum-conservation` — exact for APIC +
 MLS-MPM per Jiang 2015 + Hu 2018
 - `cat3.mpm-energy-non-increase` — KE never amplifies through
 P2G→grid-update→G2P cycle (PIC monotone strict; APIC
 approximate; FLIP approximate with damping — our APIC is
 approximate, "never amplify" not "always decrease")
 Plus item 4 (`cat3.mpm-boundary-idempotence`) + item 7
 (`cat3.mpm-cfl-self-check`) — item 4 required
 for v2.50, item 7 nice-to-have. Substep refactor (v2.49 Option A) lands before v2.50. Classical MMS is structurally
 a poor fit for MPM (Lagrangian particles vs grid-based MMS);
 conservation laws are the right oracle

Each MMS check requires:

- A manufactured-solution doc under `tools/integrity/docs/algebraic/`
 or `tools/integrity/docs/mms/`.
- A canonical-input fixture defining initial condition + source term.
- Either a one-time sim modification adding the source term as an
 optional input (preferred: `--mms-source <fixture>` flag), or a
 build-time `-DMMS_VERIFICATION` mode.
- A comparator that loads the resulting capture and verifies against
 `u_hat` within tolerance.

**Industry-grounded MMS workflow:** Mirror MOOSE's pattern — Python
script (sympy-based) generates `S(x,t)` from `u_hat`; tests verify
the solver reproduces `u_hat` at multiple grid resolutions; report
emits observed-vs-formal order.

#### 1.3.C Per-sim invariant checks (non-MMS)

Closed-form facts the sim must satisfy at every frame, independent of
MMS. These catch *current* bugs (Cat 3 invariants are *the* primary
tool for catching the bugs Steven's "subtle math wrong everywhere"
framing points at; goldens lock in current state, invariants test
against absolute truth).

**Current shipped baseline (per Probe DD, banking row 80):** At HEAD
`351c66e`, the toolkit ships **4 Cat 3 checks** across **2 of 13 in-scope
sims** — `cat3.cubic-kernel` (sph-water; graceful-degrades when Stack-C
driver absent) and the d3q19 trio
(`velocity-set` / `weights` / `equilibrium`, all lattice-boltzmann).
**11 of 13 sims have zero Cat 3 coverage** at HEAD. The candidate table
below is forward-looking, not current; Probe GGG (per-sim Cat 3
candidate enumeration) lands the FACT-grounded version with full
columns.

**Per-sim candidate invariants (current draft — needs Probe GGG
enumeration for handoff):**

| Sim | Stack | Coverage at HEAD | Candidate invariants (draft) | Status |
|---|---|---|---|---|
| sph-water | C | 1 shipped (cubic-kernel) | `cat3.sph-density-zero-far-from-particles` (kernel support), `cat3.sph-momentum-conserved-symmetric` (symmetric IC → momentum stays zero in CM frame), `cat3.sph-frame-rate-budget` (perf, not invariant — surface as soft-warn). | **Phase 2 priority — has scaffolding** |
| lattice-boltzmann | C | 3 shipped (d3q19 trio) | Plus `cat3.lbm-mass-conservation` (Σρ = const), `cat3.lbm-equilibrium-positive` (feq ≥ 0 for valid macros). | **Phase 2 priority — has scaffolding** |
| boids-3d | B | 0 | `cat3.boids-distance-symmetric` (boid-i sees boid-j at distance = j sees i), `cat3.boids-velocity-bounded` (\|v\| ≤ max_speed), `cat3.boids-rule-symmetric-config` (symmetric IC → symmetric output). | Zero-coverage; enumerate first |
| eulerian-smoke | C | 0 | `cat3.smoke-velocity-bounded` (\|v\|_max < 50; *would have caught the v=1106 bug*), `cat3.smoke-divergence-after-projection` (‖∇·v‖ < ε), `cat3.smoke-pressure-gradient-sign` (applying ∇p reduces divergence), `cat3.smoke-mass-conserved` (Σ density = const). | Zero-coverage; enumerate first |
| mpm-multimaterial | D | 0 | **Resolved per Probe GGG § C + JJ (banking row 94):** Option C++ — **4 Cat 3 checks** decomposed as `cat3.mpm-particle-grid-momentum-transfer` (P2G then G2P preserves total linear momentum), `cat3.mpm-jacobian-water` (Jp ≈ 1 for water particles — currently holds), `cat3.mpm-plasticity-svd` (deformation gradient F = U·Σ·Vᵀ with bounded Σ), `cat3.mpm-lame-from-young-poisson` (Lamé parameters λ, μ derive correctly from E, ν). Mass-conservation check dropped as tautological (grid resampling preserves mass by construction). Angular-momentum-exactness dropped as tolerance-blocked → deferred to v2.50 (per GGG § E.3). **Authorities: Jiang 2015 (angular-momentum exactness criterion); Hu 2018 (MLS-MPM inheritance)** — not Stomakhin 2013 (snow-plastic-flow reference; correct for Jp check only). | **Resolved — enumerate per GGG § A** |
| physarum | B | 0 | `cat3.physarum-trail-decay-rate` (exponential at known params), `cat3.physarum-sensor-rotation-symmetry`. | Zero-coverage; enumerate first |
| reaction-diffusion-2d | B | 0 | `cat3.rd2d-mass-conservation` (Σu + Σv constant for chosen params), `cat3.rd2d-pearson-stationary` (verified against Pearson's published patterns). | Zero-coverage; enumerate first |
| reaction-diffusion-3d | C | 0 | Same as 2D in 3D. | Zero-coverage; enumerate first |
| lenia-fft | D | 0 | `cat3.lenia-state-bounded` (∈ [0,1]; *holds at frame 2900*), `cat3.lenia-no-nan`, `cat3.lenia-kernel-normalized` (Σ kernel = 1). | Zero-coverage; enumerate first |
| neural-ca | C | 0 | TBD per Probe GGG. | Zero-coverage; no draft candidates |
| pic-flip | C | 0 | TBD per Probe GGG. | Zero-coverage; no draft candidates |
| strange-attractors | B | 0 | **Resolved per Probe GGG § B (banking row 95):** BIFURCATE — 2 Cat 3 candidates + 1 Cat 4 deferred. Cat 3: `cat3.attractor-trajectory-bounded` (Lorenz/Rössler stays within attractor basin for canonical params), `cat3.attractor-step-symplectic` (symplectic integrator preserves invariants per timestep). Cat 4 deferred: `cat4.attractor-lyapunov` — Lyapunov exponent is runtime-integration-shaped (requires long-time integration + statistical convergence), better-suited to Cat 4 visual/runtime regression than Cat 3 per-frame invariant. | **Resolved — enumerate per GGG § A** |
| mandelbulb-explorer | B | 0 | `cat3.mandelbulb-de-correctness` (distance estimator returns ≤ true distance — fundamental DE correctness property). | Zero-coverage; enumerate first |

Each candidate is ~50–150 LOC + a derivation/ground-truth doc.

**Probe GGG output: 36-row enumeration table — see report § A.**

The FACT-grounded full enumeration landed at
`docs/diagnostics/_audits/probe_GGG_report_2026-05-17.md` § A. Headline
distribution: **33 Cat 3 + 1 Cat 4 deferred + 2 blocked-sim
placeholders**. Ground-truth source: **19 algebraic / 11 closed-form / 3
vendored**. Mode: **28 HARD_FAIL / 5 SOFT_WARN**.

Phase 2 sequencing (per GGG § E): v2.5–v2.11 land ~2400 LOC across 32
distinct check modules. Scaffolding-first ordering per Probe DD § E.3
— sph-water and lattice-boltzmann extensions land first, then
zero-coverage sims in parallel where the per-sim derivation work is
done.

Ground-truth sources fall into three classes:
- **Algebraic** — re-derivation in Python from spec equations (e.g.,
  d3q19 weights, ising-energy). Most defensible.
- **Vendored** — anchored to upstream reference implementation (e.g.,
  SPlisHSPlasH for cubic-kernel; Stomakhin Table 2 for MPM water).
- **Closed-form** — sim-specific invariants verifiable by symmetric
  initial conditions or analytical bounds (e.g., boids-distance-symmetric,
  smoke-velocity-bounded).

**Phase 2 sequencing (revised per Probe DD § E.3; finalized per Probe GGG § E):**

1. **Probe GGG output landed** at `docs/diagnostics/_audits/probe_GGG_report_2026-05-17.md`
   § A. Resolves the two open questions: **mpm-multimaterial → Option C++**
   (4 conservation-law checks; authorities Jiang 2015 + Hu 2018);
   **strange-attractors → BIFURCATE** (2 Cat 3 + 1 Cat 4 deferred).
2. **First Phase 2 batches deepen existing-scaffolding sims** —
   sph-water and lattice-boltzmann extensions amortize the per-invariant cost
   against existing Stack-C driver infrastructure + algebraic-derivation
   patterns.
3. **Subsequent Phase 2 batches** parallelize per zero-coverage sim
   per GGG's scaffolding-first ordering. Drives-sim default rule
   (§ 3.17.A) routes all Phase 2 batches through the v2.43 / v2.44
   infrastructure dependency before they can dispatch.

The "scaffolding-first" framing replaces the rev-13 "zero-coverage
sims first" assumption — Probe DD revealed without enumeration,
zero-coverage batches have no concrete contents. Existing-scaffolding
sims are where Phase 2 can produce code first; per GGG § E, ~2400 LOC
across 32 distinct check modules total.

#### 1.3.D GPU shader headless — Stack B

Per web search and finding (`webgpu` npm package publishes
Dawn as `dawn.node` for Node.js; VTK already uses this in CI for
Linux image regression; subito.it and many others use it for visual
regression):

- Add `webgpu` npm dependency to a CI-only `package.json`.
- Author Node-based harness that loads each sim's WGSL kernel,
 uploads canonical inputs, dispatches, reads back, pipes output to
 Python cat3 comparator.
- Port `cat3.cubic-kernel` from C++ driver to WGSL via dawn as proof.
- Expand to per-sim WGSL kernels.

CI cost: one `npm install` step (~30s). Plug-and-play.

#### 1.3.E GPU shader headless — Stack C

Two viable backends per web search:

- *Mesa lavapipe* (apt-installable via `mesa-vulkan-drivers`):
 CPU-based Vulkan ICD; faster than SwiftShader for many workloads
 per Phoronix benchmarks; supports compute shaders.
- *SwiftShader* (Google, CMake-buildable): more mature; misses
 transform feedback / tessellation but our compute workloads don't
 need either.

**Decision: lavapipe by default, SwiftShader fallback if a sim hits a
lavapipe limitation.**

- Add `mesa-vulkan-drivers` apt-install to integrity.yml workflow.
- Set `VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json`.
- Author C++ harness extending the existing cat3 C++ driver pattern.
- Port `cat3.d3q19-*` checks from algebraic-driver to GLSL-via-Vulkan
 under lavapipe as proof.
- Expand to per-sim GLSL/SPIR-V kernels.

CI cost: one apt-install command. Lowest-risk infra addition in v2.

#### 1.3.F Cross-stack equivalence

: mandelbulb-explorer and reaction-diffusion-2d have
Stack A→B ports. Cross-stack check verifies they produce numerically
equivalent output at canonical inputs.

`cat3.cross-stack-equivalence` — runs both stack implementations on
same canonical input, captures output, compares within
stack-difference tolerance.

#### 1.3.G Differentiability / gradient correctness

Forward-looking; no current sim uses gradients but the gap-analysis
doc identifies differentiability as a key frontier gap.

`cat3.gradient-finite-diff` — for each differentiable kernel,
evaluate analytic gradient (autodiff) and finite-difference gradient
at canonical input; assert agreement within FD tolerance.

Lands the harness in v2; per-sim adoption gates on differentiability
shipping.

### 1.4 Cat 4 — Runtime integration tests (new in v2)

**Defined as:** the compiled / interpreted sim binary, run with
canonical inputs and pinned seeds, produces output that matches
expected behavior. Multiple sub-checks per sim, configured for what
applies.

**v1 deferral hypothesis** (`integrity-toolkit-spec.md` § 1.2): "Cat 4
maintenance cost is high; doing it before structural layer is solid
means snapshots churn for the wrong reasons. Reconsidered after one
full project cycle on v1." v1 is solid; the hypothesis test concludes
that Cat 4 is in scope.

**Eight sub-check classes:**

#### 1.4.A `cat4.build-smoke` — universal cheap floor

Compile/build, run for N seconds with timeout, expect exit 0.
Catches: build breakage, immediate crashes, infinite hangs. Per
, no sim currently crashes; this is regression insurance.

#### 1.4.B `cat4.tolerance-snapshot` — primary state regression

Engine: `pytest-regtest` (mature; used by Tezos; supports NumPy
`atol`/`rtol`). Storage: Git LFS (decision § 3.1).

Same seed → output within ε. Per-buffer tolerance configured in
`<sim>/tests/integrity/config.toml`. Rebaseline via
`--regtest-reset`.

#### 1.4.C `cat4.bitwise-snapshot` — for CPU-deterministic captures

Same seed → byte-identical output. Special case of (B) with
tolerance=0. Applies where the sim runs CPU-deterministically:
Stack D Taichi-CPU backend, Stack C with `-ffp-contract=off` and
pinned thread count, any deterministic compute path.

#### 1.4.D `cat4.determinism-pair`

Run twice with same seed; outputs must match. Catches non-determinism
sources that ordinary snapshot wouldn't surface (uninitialized
memory accidentally consistent on most runs; thread races in
atomics).

#### 1.4.E `cat4.replay-determinism`

Load captured state at frame N, run M more frames, must equal frame
N+M of original. Catches state-serialization drift in the
StateWriter / StateReader cycle.

#### 1.4.F `cat4.vk-validation-clean` — Vulkan validation layers

**Standard Khronos validation layer with extended features.** Per
Vulkan registry, three feature flags catch progressively more bugs:

- Default validation: API misuse (wrong handles, missing required
 features, etc.).
- `VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT`: API misuse that
 isn't explicitly prohibited but is known-bad.
- `VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT`:
 resource access conflicts (read-after-write, layout transitions,
 etc.).
- `VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT`: shader-execution-time
 bounds violations.

Run each Stack C sim in CI with all four enabled for N frames;
capture validation-layer output to file; assert zero errors/warnings
(or explicit allow-list for known-safe noise).

This *almost certainly* catches several of Steven's "subtle bugs."
Stack C has substantial GPU synchronization surface; the current
integrity.yml already installs `vulkan-validationlayers` apt package
for the cat3 C++ driver but doesn't run with validation enabled as
a gate.

#### 1.4.G `cat4.visual-regression` — per-frame screenshot comparison

**Industry-standard pattern.** Per web search consensus
(jest-image-snapshot, BackstopJS, Percy, Chromatic, VisIt at LLNL,
VTK with Dawn, subito.it production):

- *Algorithm:* pixelmatch with anti-aliasing filter, 0.1% threshold
 default. SSIM secondary check for cases with high pixelmatch
 false-positive rate. PSNR tracked as canonical comparison number
 (logged over time, trend visible).
- *Storage:* PNG screenshots at canonical frame indices, in Git LFS
 under `<sim>/tests/integrity/screenshots/`.
- *Masking:* mask UI overlays (ImGui panels, Taichi GGUI sidebars)
 via per-sim mask config; only the simulated content participates
 in diff.
- *Cadence:* screenshot at canonical frame indices (e.g., frames 1,
 30, 100, 1000) per canonical input. Multi-frame coverage catches
 drift that single-frame doesn't (Steven's request: screenshots
 over time and compare).

**

three critical operational realities for v2.16:

- **Render dimensions vary by stack.** Stack B uses `viewport × DPR`
 (Playwright will be pinned to 1280×720, DPR 1 for determinism).
 Stack C is hardcoded 1920×1080 across all 4 sims. Stack D is
 hardcoded 1280×720 across both sims. Different stacks → different
 canvas resolutions in goldens; storage layout under
 `<sim>/tests/integrity/screenshots/` should encode this in
 filename or path.
- **No sim is overlay-free.** All 11 sims overlay UI on the sim
 surface: Stack B has lil-gui top-right ~245px + HTML HUD corners;
 Stack C has ImGui top-left (sizes 286–540px wide per `imgui.ini`);
 Stack D has Taichi GGUI panel column left ~22% of width
 (explicit `GUI_PANEL_RECTS` in source).
 **Decision (refined):** harness-side mask config (rect tables
 loaded by the comparator), NOT sim source changes. Mask rects live
 in `<sim>/tests/integrity/config.toml` keyed by render dimension.
 Sim source stays unchanged; pixelmatch ignores masked regions.
- **Canonical frame indices: 3 per sim** (early / mid / deep). Mid
 is the baseline; eulerian-smoke frame 120 anchored to 's
 empirical benchmark. Early frame for fast-feedback iteration;
 deep frame catches long-horizon drift.

**Screenshot acquisition mechanism:**

- **Stack B:** Playwright `page.screenshot` — trivial. No sim-side
 changes needed.
- **Stack C (interim):** `xwd` by window ID — works without sim code
 changes for v2.16 day-1 adoption.
- **Stack C (preferred long-term):** `vkCmdCopyImageToBuffer` +
 `stb_image_write` — adds one new offscreen-capture path in each of
 4 sims. Tracked as a Cat 4 follow-up (v2.20-ish).
- **Stack D (lenia-fft):** already has `window.write_image` /
 `ti.tools.imwrite` — wire to the headless harness.
- **Stack D (mpm-multimaterial):** add `ti.tools.imwrite` call
 conditional on `--headless` flag.

**Critical pre-baseline triage requirement:**

Eulerian-smoke and sph-water have known visual defects today (per
: v_max ≈ 1106 and 196ms/frame with banding). Baseline-locking
these sims now would encode the bugs as the reference, defeating the
entire purpose of regression detection. **Sequencing requirement:**
Cat 3 invariants (smoke MMS at v2.6, sph DFSPH residual at v2.7) must
pass — i.e., the underlying bug must be triaged and fixed — *before*
Cat 4 baseline-locks (v2.17 smoke, v2.18 sph). This is captured in
§ 3.13 below as a hard ordering constraint.

**Per-sim seed-pinning preconditions:**

- *physarum* (Stack B): uses random initial agent positions; needs
 `?seed=K` URL parameter wired through to RNG seed.
- *boids-3d* (Stack B): random initial boid positions; same wiring.
- *strange-attractors* (Stack B): random initial point positions;
 same wiring.
- *lenia-fft* (Stack D): CI must use Taichi real-space backend (not
 GPU backend) to match dev determinism; pinned in
 `<sim>/tests/integrity/config.toml` as `taichi_backend = "cpu"`.

**Comprehensive visual report (per Steven's request):**

The Cat 4 visual regression sub-check produces an output report
beyond just pass/fail. For each compared frame:

- Side-by-side composite image (baseline | current | diff).
- Pixelmatch percentage and pixel count.
- SSIM score (0-1; >0.99 = no visible change).
- PSNR score in dB.
- Worst-pixel coordinates and color values.
- Optional: heatmap overlay showing diff regions.
- Trend chart over time (last N runs) for the SSIM and PSNR.

This shipped as a multi-frame audit-report HTML page under
`docs/diagnostics/_audits/cat4_visual_<sim>_<date>.html`. Useful
even outside CI — running locally during development surfaces
visual drift between branches.

#### 1.4.H `cat4.memory-clean` and `cat4.no-strict-warnings`

- AddressSanitizer (Stack C) + Valgrind option. 's note
 that integrity.yml currently runs `cmake -DCMAKE_BUILD_TYPE=Release`
 — Cat 4 adds a parallel `Debug+ASAN` build matrix.
- Strict warning flags: `-Wall -Wextra -Werror` for Stack C; `tsc
 --strict` for Stack B; `mypy --strict` for Stack D.

**Required per-sim infrastructure (Cat 4-wide):**

+, three per-stack CLI stub patterns close the
gap. Updated for 11 sims (was 9):

- **Stack B (5 sims: boids-3d, physarum, reaction-diffusion-2d,
 strange-attractors, mandelbulb-explorer):** wrap rAF with frame
 counter; parse URL query string for `?seed=K&frames=N&headless=1`;
 drive via Playwright in CI.
- **Stack C (4 sims: eulerian-smoke, reaction-diffusion-3d,
 sph-water, lattice-boltzmann):** parse argc/argv (or for RD-3D,
 add the signature first divergence); short-circuit the
 main loop with `|| frame_count >= max_frames`; gate Window creation
 behind `--headless`. RD-3D uses `int main` without
 argc/argv at all (signature change needed); sph-water uses raw
 `glfwWindowShouldClose(window.glfwWindow)` not the
 `window.shouldClose` wrapper.
- **Stack D (2 sims: mpm-multimaterial, lenia-fft):** add `argparse`
 block; gate `ti.ui.Window` behind `--headless`; add frame counter.
 mpm already has shipped tests at
 `hybrid-particle-grid/mpm-multimaterial/python/tests/test_kernels.py` — Cat 4 work extends rather than
 replaces.

**Stack B CI automation precondition (new requirement):**

's critical finding: Stack B captures are manual-only
today. The triggerDownload pattern creates a Blob URL and
programmatically clicks `<a download>` — there's no programmatic path
to acquire a Stack B capture in CI. Cat 4 requires Stack B headless
acquisition. Three options:

- **Option A: Playwright + download handler.** `page.on('download',
...)` catches the ZIP download, saves to disk. Lowest-impact; no
 sim-side changes; matches existing manual flow.
- **Option B: Inject capture-now debug hook.** Add
 `window.__captureNow(frameIdx)` returning ZIP bytes synchronously
 for test code. Requires sim-side wiring.
- **Option C: File-system writer adapter.** Under `--mode=test`,
 swap triggerDownload for `fetch('/upload',...)` to a Vite dev
 endpoint. Tighter coupling; runs under Vite dev not prod build.

**Decision: Option A.** Matches the documented "browser triggers
download → user unzips" flow that the Tier-1 spec already accepts.
Cat 4 batch v2.13 lands the Playwright harness alongside the
per-stack stubs.

### 1.5 Cat 5 — Cross-source consistency (new in v2)

**Defined as:** when two separate sources claim the same fact, they
must agree.

**Originating motivation:** surfaced a concrete instance.
`quantum.md` § 6.2 asserts the parser would crash on algebraic
entries — wrong against current code (Phase 12 setup-2 landed the
skip-branch). Drift between doc and code went undetected because no
check verifies it.

**v1 precedent:** `tools/integrity/scripts/audit_prose_freshness.py`
(closeout commit 3 sibling tool, currently *untracked* on disk per
 but functionally complete).

**v2 sub-checks:**

#### 1.5.A `cat5.audit-prose-freshness` — gate-integrate the sibling tool

Same logic; register with runner. ~50 LOC integration.

#### 1.5.B `cat5.spec-claim-resolves` — verify spec claims against code

Extract structured claims from spec / category-context / sim-spec
documents:

- Backticked `path/to/file.py:42` citations → resolve in current tree.
- Backticked `function_name(arg1, arg2)` → match function signature
 at cited location (via Python AST / TS compiler API / libclang).
- Backticked `class ClassName` → match class declaration.
- Module-level constant claims in prose → match value in code.

Implementation: per-language AST walks. Suppress via
`integrity-allow: cat5.spec-claim-resolves` when prose is
intentionally narrative.

#### 1.5.C `cat5.changelog-tag-consistency`

Every `## [X.Y.Z]` header in CHANGELOG has a matching `vX.Y.Z` git
tag. Every compare-link parses and endpoints exist.

#### 1.5.D `cat5.sim-spec-divergence-noted`

For each shipped sim, compare `docs/sim-specs/<sim>.md` claims against
code; for each divergence, verify `<sim>/docs/load-bearing-decisions.md`
references the divergence (Phase 9 convention).

#### 1.5.E `cat5.conventions-doc-coverage`

Every Convention A–K in `tools/integrity/docs/conventions.md` is
referenced from at least one banking source (retro or audit).

#### 1.5.F `cat5.vendored-anchor-fresh`

For each vendored upstream registry entry, `anchor_sha` resolves to
a real commit in the vendor tree AND `anchor_doc` exists AND SHA
matches checked-out state.

#### 1.5.G `cat5.intra-doc-anchor-resolves`

Every markdown `[text](file.md#heading)` link resolves: file exists,
heading anchor matches a `## heading` in the file.

#### 1.5.H `cat5.cross-stack-divergence-noted`

For each A→B port sim (2 currently), verify shadertoy/README has
port-mapping table + "no code lifted" attribution clause.

#### 1.5.I `cat5.capture-format-reference-fresh`

's finding: tier1-capture-format-reference.md is stale
on Phase 9 (mpm), Phase 10 (lenia), and missing format strings
(r8uint, r32i, r32ui). New check verifies every shipped sim's
writer is represented in the reference doc.

### 1.6 Cat 6 — Build / CI hygiene (new in v2)

**Defined as:** build system and CI are themselves sane.

**v2 approach: integrate existing industry-standard linters rather
than reinventing.**

#### 1.6.A `cat6.workflow-lint` — actionlint integration

`actionlint` (`rhysd/actionlint`) is the standard. Used widely,
3700+ GitHub stars, type-checks workflow expressions, validates
runner labels, integrates `shellcheck` for `run:` blocks, integrates
`pyflakes` for Python steps. Free, Apache-licensed.

v2 adds actionlint to integrity.yml's own step list. finds existing issues — `ubuntu-latest` in markdown.yml and
structure.yml is a floating pin; integrity.yml uses `npm install
--silent` not `npm ci` (lockfile drift); SHA-pin discipline absent
from action references.

#### 1.6.B `cat6.shell-lint` — shellcheck via actionlint

Already integrated through actionlint when it runs.

#### 1.6.C `cat6.cmake-lint` — cmake-format / cmake-lint

`cmake-lint` from `cmakelang` package. Catches CMakeLists.txt issues
(missing `add_subdirectory`, orphan source files, deprecated
commands). Pip-installable.

#### 1.6.D `cat6.python-lint` — ruff + mypy unification

ruff + mypy already run in build-py.yml. v2 ensures
configuration consistent across packages (single `pyproject.toml`
ruleset or per-package override with rationale documented), and
that integrity.yml runs the same checks (currently doesn't).

#### 1.6.E `cat6.ts-lint` — tsc-strict + eslint

`npm run typecheck` runs in build-web.yml. Add eslint
+ prettier with shared config across Stack B sims.

#### 1.6.F `cat6.markdown-lint` — existing tooling

markdownlint-cli2 + lychee already run in markdown.yml.
Verify consistency across docs.

#### 1.6.G `cat6.deps-pinned`

Every dep manifest pins versions: Python `pyproject.toml` with
explicit `dependencies` (no `>=` without `<` upper bound), Node
`package-lock.json` present, Stack C vendored deps SHA-pinned.

#### 1.6.H `cat6.workflow-cache-discipline`

integrity.yml caches nothing despite installing both
pip and npm trees on every run. Cold install is dominant CI wallclock
cost. v2 adds `actions/setup-python@v5` with `cache: 'pip'` and
`actions/setup-node@v4` with `cache: 'npm'` to integrity.yml.

#### 1.6.I `cat6.workflow-os-pinned`

markdown.yml + structure.yml use `ubuntu-latest`
(floating pin); all others use `ubuntu-24.04`. Normalize to
`ubuntu-24.04` everywhere.

#### 1.6.J `cat6.lockfile-strict`

integrity.yml uses `npm install --silent` (non-strict);
build-web.yml + deploy-pages.yml use `npm ci` (strict). Standardize
to `npm ci` everywhere.

#### 1.6.K `cat6.codeowners-and-dependabot`

neither CODEOWNERS nor dependabot.yml exists. Both are
common hardening additions; v2 adds opinionated defaults.

#### 1.6.L `cat6.git-attributes-clean`

Already mostly clean. Add LFS filter directives when v2
introduces Git LFS for Cat 4 goldens.

#### 1.6.M `cat6.cmake-flags-consistent`

Strict warning flags configured in CMakeLists (not just CI), consistent
across Stack C. `-Wall -Wextra -Werror` minimum; `-Wpedantic` opt-in
per-sim.

### 1.7 Toolkit-itself standards compliance (new audit in rev 3)

We are building a tool to verify code; if the tool itself isn't built
to industry standard, our verification is fundamentally compromised.
This section audits our toolkit's architecture against mature
linter/static-analyzer conventions (Ruff, ESLint, Clippy, SonarQube,
CodeQL, GitHub Code Scanning) and identifies gaps.

#### 1.7.A SARIF 2.1.0 output mode (highest-leverage gap)

**Industry standard:** SARIF (Static Analysis Results Interchange
Format) is the OASIS standard for static analysis tool output. GitHub
Code Scanning consumes SARIF natively — findings appear as annotations
on PR diffs. SonarQube, CodeQL, every enterprise SAST/SCA tool
emits SARIF.

**Our state:** Custom stanza output (`HARD_FAIL: cat1.bare-path at
project-state.md:561`). No SARIF mode.

**v2 work:**

- New `--output sarif` mode emits SARIF 2.1.0 JSON.
- SARIF rules array auto-populated from check registry — one entry
 per check ID with shortDescription / fullDescription / help text.
- SARIF results array emits per-finding entries with ruleId, level
 (per check severity mapping), message, location (uri + region),
 partialFingerprints.
- Workflow integration: upload-sarif action posts to GitHub Code
 Scanning. Findings show in PR review UI natively.

**Mapping** (our severity → SARIF level):

- `HARD_FAIL` → `error`
- `SOFT_WARN` → `warning`
- `PASS` (informational) → not emitted (SARIF results are only for
 flagged items).
- `SUPPRESSED` → emitted with `suppressions` array entry per OASIS spec.

The custom stanza output stays as the default (audit-report-friendly);
SARIF is additive.

#### 1.7.B Standard exit-code convention

**Industry standard** (Ruff, ESLint, Prettier, RuboCop, Clippy):

- `0` = no violations
- `1` = violations found
- `2` = abnormal termination (bad CLI args, internal error)

**Our state:** `python -m integrity --mode strict
--no-audit-log` exits `1` on violations (HARD_FAIL count = 47).
Standard for state `0` and `1`. Behavior on bad CLI args needs
verification — argparse default is `2` but our runner may override.

**v2 work:** Probe at v2.32 spec time; assert standard `0/1/2`
convention; document in runner help text.

#### 1.7.C Rule ID stability + index

**Industry standard:** Each rule has a stable ID across versions.
Ruff: `F401`, `E501`, `B008`. ESLint: `no-unused-vars`, `prefer-const`.
Clippy: `needless_borrow`, `let_and_return`. SonarQube: `python:S1481`.

**Our state:** Stable semantic IDs (`cat1.intra-repo`, `cat2.public-symbol-used`,
`cat3.cubic-kernel`, etc.). Stability *contract* is not documented but
behaviorally maintained.

**v2 work:** Author `tools/integrity/docs/rules/INDEX.md` listing all
check IDs + their stability contract:

> "Check IDs are part of the public toolkit API. Renames cause an alias
> period of one minor version; removals require a deprecation note in
> the previous minor version. Adding new check IDs is a minor version
> bump."

Our semantic ID convention (`<category>.<check>`) is a defensible
divergence from Ruff/ESLint's prefix-NNN — more readable, less compact.
Document the choice.

#### 1.7.D Per-rule documentation pages

**Industry standard:** Each rule has a public documentation page
explaining what it catches, why, examples of triggering code, examples
of fixed code, related rules. Examples:
- Ruff: https://docs.astral.sh/ruff/rules/<rule>/
- ESLint: https://eslint.org/docs/rules/<rule>
- SonarQube: https://rules.sonarsource.com/python/RSPEC-<id>/
- CodeQL: https://codeql.github.com/codeql-query-help/python/<id>/

**Our state:** No per-rule docs. The check-implementation modules
have docstrings, but no user-facing rule reference.

**v2 work:** Author `tools/integrity/docs/rules/<check_id>.md` per
check. Template:

```
# <check_id>

**Category:** Cat N — <Category Name>
**Default severity:** HARD_FAIL / SOFT_WARN
**Stable since:** v<version>

## What this catches

<one paragraph>

## Why

<rationale>

## Triggering example

\`\`\`<language>
<code that triggers>
\`\`\`

## Fixed example

\`\`\`<language>
<code that passes>
\`\`\`

## Suppression

\`\`\`
<!-- integrity-allow: <check_id>; <reason>; <issue_ref> -->
\`\`\`

## Related

- <other check ids>

## Implementation

- `tools/integrity/integrity/cat<N>_<name>/checks/<file>.py`
```

Each new v2 check lands its rule doc in the same commit.

#### 1.7.E Auto-fix safety classification

**Industry standard:** Ruff distinguishes "safe" vs "unsafe" fixes.
Safe fixes preserve runtime behavior; unsafe fixes may not. ESLint has
`fixable: 'code' | 'whitespace'` plus suggestions vs fixes. Clippy
has machine-applicable suggestions vs hand-applicable.

**Our state:** The `--rewrite-stale-reasons` mode (landed in v1.3
closeout commit 1; not a convention — `conventions.md` lines 133–139
explicitly disown the prior letter-I attribution) is our first
auto-fix mechanism. It's described as opt-in and supports `--dry-run`
first. No formal safety classification.

**v2 work:**

- Classify the `--rewrite-stale-reasons` mode as **safe** (reason-text rewrite doesn't change
 check semantics; suppression decisions are unchanged).
- Document the safety convention in `tools/integrity/docs/conventions.md`:
 "Safe auto-fixes: applied by default in `--rewrite-stale-reasons` mode.
 Unsafe auto-fixes: not applied automatically; flagged in audit output
 with suggested patch text."
- Future auto-fixes follow the same classification.

#### 1.7.F Fingerprint discipline (SARIF partialFingerprints)

**Industry standard:** SARIF's `partialFingerprints` property lets a
static analysis tool emit a hash of the surrounding code context that
identifies a finding across line-shift edits. Without it, reformatting
a file causes "alert closed + new alert opened" noise on every commit.
GitHub deduplicates findings using these fingerprints.

**Our state:** No fingerprint computation. Per-finding identity is
implicit (`<check_id> at <path>:<line>`); line-shift edits create
false-positive churn in the audit trail.

**v2 work:**

- Compute fingerprint per finding: `sha256(check_id + normalized_path
 + ±3 lines of surrounding code, whitespace-normalized)`.
- Emit in SARIF mode as `partialFingerprints`.
- Emit in stanza mode as a comment `# fp: <hash>` for cross-run
 matching.
- Audit-report cross-references: when an audit report references a
 finding from a previous commit, use the fingerprint, not the
 `path:line` (which may have shifted).

This is the foundation for Convention F (audit-prose freshness) at
scale — fingerprints survive reformatting.

#### 1.7.G Doctest-based Cat 5 enhancement

**Industry standard:** Python's `doctest`, Sphinx's `sphinx.ext.doctest`,
Rust's `rustdoc` — all run code examples *embedded in documentation*
as actual tests. The doc and the test are the same artifact.

**Our state:** Cat 5 `cat5.spec-claim-resolves` (rev 2 design) uses
regex matching for backticked function signatures. Less rigorous than
doctest because regex doesn't actually *run* the code; just matches
shape.

**v2 enhancement:**

- New sub-check `cat5.doctest-spec` — for each spec doc with `>>> `
 blocks (Python REPL style), run them under doctest, assert success.
- Pairs with `cat5.spec-claim-resolves` (regex-based, broader coverage)
 vs `cat5.doctest-spec` (executable, narrower but more rigorous).
- For Stack C / Stack B spec docs without REPL syntax, fall back to
 regex; for Stack D / Python-side specs, prefer doctest.

#### 1.7.H Property-based testing for Cat 3 (Hypothesis)

**Industry standard:** Hypothesis is the standard Python property-based
testing library. Instead of testing invariants only at hand-picked
canonical inputs, Hypothesis generates random configurations and shrinks
failing cases to minimal reproducers. Used widely; standard for physics
simulation correctness work.

**Our state:** Cat 3 invariants (rev 2 design § 1.3.C) tested at
hand-picked canonical inputs only.

**v2 enhancement:**

- Each Cat 3 invariant gets a Hypothesis-property variant:
 - `cat3.smoke-divergence-after-projection` at canonical inputs (existing).
 - `cat3.smoke-divergence-after-projection-property` — Hypothesis
 generates random parameter sets within physical bounds, asserts
 invariant holds.
- Hypothesis's shrinking automatically finds minimal reproducers when
 invariants fail.
- Cat 3 property-based variants apply to all invariants where
 parameter randomization is meaningful (most of them).
- MMS extension: random manufactured solutions (Hypothesis generates
 `u_hat` from a parameterized family) instead of just hand-picked.

#### 1.7.I What our toolkit gets RIGHT against industry standards

Not all divergences are gaps. The following are deliberate or
incidentally good design choices that match or exceed industry norms:

- **Annotation-based suppression with mandatory reason + issue ref.**
 Mirrors `# noqa`, `# eslint-disable`, `# allow(...)` patterns but more
 rigorous — most tools accept bare suppressions; we require reason
 text and issue ref. Live-source-stays-red discipline is unusual in
 the industry but excellent.
- **Grandfather catalog as machine-readable migration TODO.** Closely
 matches how mature tools handle "we know this is wrong; tracking
 migration." Better than most because it's a single file vs. scattered
 per-rule disable comments.
- **Category-based organization.** Matches ESLint plugin / Ruff
 preset patterns. Our `cat1` through `cat6` (rev 2) maps cleanly to
 ESLint's `eslint:recommended`, `plugin:react/recommended`, etc.
- **Convention M (re-anchor before edit).** More rigorous than what
 most industry tools demand of their users — it's an internal-discipline
 convention at the linter-author level. Ruff maintainers re-verify
 rule line numbers against current parse output; we made it a global
 practice.
- **Audit-report-per-commit discipline.** Exceeds industry norm. Most
 linters emit findings and forget; we maintain a written trail with
 spec → execution → retro cadence.
- **Cross-stack design.** Few open-source tools span TS + C++ + Python
 uniformly. Most linters are language-bound; ours treats cross-stack
 consistency as a first-class concern. The capture-format-consistency
 Cat 2 work (rev 2 § 1.2) is a category we invented for our specific
 cross-stack-port reality; no industry analog needed.

#### 1.7.J Where our toolkit DIVERGES from standards (intentionally or not)

- **Custom output format vs SARIF.** Was intentional (audit-friendly
 prose stanzas); becomes a gap when interoperability matters. Fix:
 add SARIF as second output mode without removing stanzas. Lands in
 v2.32.
- **Semantic check IDs (`cat1.intra-repo`) vs prefix-NNN (`F401`).**
 Ours is more readable; industry convention is more compact. Defensible.
 Document the choice in v2.33.
- **No LSP server / IDE integration.** Major gap for daily-development
 friction but not for CI gating. Defer to v3 if ever needed.
- **No public per-rule documentation pages.** Industry standard.
 Lands in v2.34.
- **No fingerprint discipline.** Industry standard. Lands in v2.32.
- **No formal auto-fix safety classification.** Industry standard.
 Lands in v2.33.

#### 1.7.K Mutation testing for the toolkit itself

**Industry standard for verifying test effectiveness.** Coverage
percentage tells you which lines are *exercised* by tests; mutation
score tells you which lines have tests that *catch bugs*. The two are
not the same — a test that calls a function but doesn't assert
anything contributes to line coverage but kills no mutants.

**Tools:**

- `mutmut` (https://github.com/boxed/mutmut) — Python's most-active
 mutation tester per 2024 PyCon benchmarks; ~1200 mutants/min on
 AST-based generation.
- `cosmic-ray` (https://cosmic-ray.readthedocs.io) — alternative;
 supports more operators (~9 vs mutmut's 6); slower setup but
 more thorough.
- `pytest-gremlins` (newer alternative; 2026; tighter pytest integration).

**Industry benchmark:** 80% mutation score is "good" per IEEE Software
2024-2025 comparison (compared with PIT for Java which achieves
~88%).

** our toolkit at 80% *line* coverage means
roughly nothing about whether the tests catch the kinds of bugs Probe
F just found (rstrip char-set strip; ANNOTATION_RE/STRICT_RE
divergence). A mutation score baseline would tell us empirically.

**Especially relevant module:** `tools/integrity/integrity/common/audit_log.py` at 0% line
coverage. Even if we add line-coverage tests, mutation
testing would catch tests-that-don't-assert.

**Lands at v2.41.** Initial target: 80% kill rate; ratchet up over
time.

**Probe AA preconditions (banking row 73):**

- **Inner-loop test suite must be < 30s** before mutmut is viable.
  Current suite at HEAD is ~133s but 96.5% of that is two integration
  smokes; quarantining them behind `@pytest.mark.integration` yields a
  ~5s inner suite. Mutation testing perf ceiling is `inner_suite × N
  mutants`; at 5s the per-module mutmut run is feasible. § 2.9.7's
  integration-marker work (v2.0) is a prerequisite.

- **Scope mutmut per-category** (e.g., `mutmut run --paths-to-mutate
  integrity/cat1_citations`) rather than whole-toolkit. Per-run cost
  is bounded; mutation-score baseline per category is more
  actionable.

**Probe BB exclusion (banking row 74):** `tools/integrity/integrity/common/audit_log.py` is
**excluded from initial v2.41 scope**. Per Probe BB: the module is at
0% line coverage with 0 production callers (dead code in production).
Mutmut on this module would mark every mutant as both "survived" (no
test) and "unreachable from production" — vacuous on both axes. Re-add
to mutmut scope after § 2.9.7 wires the module into `runner.main()`
and tests are written.

#### 1.7.L Hermetic builds — Level 1-5 ladder

**Industry framework** (per multi-source 2025/2026 industry consensus,
e.g., `nemorize.com/roadmaps/hermetic-builds`, `beefed.ai`,
`oneuptime.com/blog/2026-02-08-how-to-build-reproducible-docker-images`):

| Level | Name | Properties |
|---|---|---|
| 1 | Basic | Some version pinning, ad-hoc |
| 2 | Loosely hermetic | Exact version pins, reproducible on same machine |
| 3 | Mostly hermetic | Hash-pinned deps, containerized builds, no network during build |
| 4 | Strongly hermetic | Content-addressable deps, isolated env, deterministic timestamps |
| 5 | Fully hermetic | Bit-for-bit reproducible, cryptographically signed, full SBOM |

**Current state:** GPU-Sims is Level 2 (+ — we have
package-lock.json, pyproject.toml dep pins, but the runner OS is
floating `ubuntu-latest`, apt-get fetches from upstream with no
hash pinning, and.

**Target for v2.42:** Level 3. Concretely:

- Pin Ubuntu runner OS by hash via container image (e.g., `container:
 ghcr.io/<owner>/gpu-sims-runner@sha256:abcdef...`).
- Bake `vulkan-validationlayers`, `libclang-dev`, `mesa-vulkan-drivers`
 etc. into a pre-built Docker image. Rebuild image on dependabot
 PR; CI consumes the digest, not the floating tag.
- Use `npm ci` (already in survey — currently using
 `npm install --silent`, which is divergent and a finding).
- Add SOURCE_DATE_EPOCH support if we ever want bit-for-bit reproducible
 artifact builds — but for now Level 3 is the right ambition.

**Why not Level 4-5:** Bazel/Nix rewrites are overkill for our scale
(11 sims, single repo). Level 3 closes the variance issues without
the engineering burden. Revisit at Phase 11 if ever.

#### 1.7.M Conformance testing analogy — Vulkan CTS framing

Khronos maintains `VK-GL-CTS` (Vulkan and OpenGL Conformance Tests).
The Vulkan Adopter Program requires drivers pass CTS before vendors
can claim "Vulkan-conformant." CTS verifies *implementations* against
the *API specification*.

**Direct analogy to our cat3 work:** the Cat 3 invariant and MMS
checks verify *our implementations* against *the algorithm spec*
(Navier-Stokes for smoke, DFSPH for sph-water, lattice-Boltzmann for
LBM, etc.). Same pattern, different layer.

**What this gives us framing-wise:**

- "GPU-Sims is conformant to MMS-verified Navier-Stokes" is a
 meaningful claim once v2.6 lands.
- "Conformant" is a useful word for retro docs to describe what passing
 Cat 3 means: the implementation matches the specification.
- Per-sim conformance fractions: like Vulkan CTS's "mustpass list,"
 we have a per-sim baseline of checks each sim must pass. Adding
 invariants is "extending the mustpass list."

**No machinery added** — this is framing only, but it makes Cat 3
discoverability and audit-prose clearer.

#### 1.7.N ACM Artifact Evaluation badges — alignment goal

**Industry framework** (ACM Artifact Review and Badging Policy,
ACM SIGSIM PADS, ACM MMSys, ACM MobiSys all use it):

| Badge | What it certifies |
|---|---|
| **Available** | Artifact is permanently archived (Zenodo, Software Heritage, etc.) |
| **Functional** | Artifact runs end-to-end as documented |
| **Reusable** | Artifact is engineered well enough that others can extend |
| **Reproduced** | Reviewers reproduced key results from scratch |
| **Replicated** | Independent team replicated results using their own implementation |

**Why this matters for GPU-Sims:**

- If GPU-Sims is ever cited in a paper, these badges become the
 discoverability marker industry-recognizes.
- The badges are decomposable — Available + Functional + Reusable
 are achievable via existing v2 infrastructure (Cat 4 reproducibility
 + Cat 7 license tracking + tools/integrity).
- Reproduced is achievable via Cat 4 pinned-seed bitwise-snapshot
 goldens (anyone with our repo + seed reproduces our PNG byte-for-byte).
- Replicated is independent — out of our control by design.

**No machinery added** — this is a framing goal and a retro-language
target. v2 infrastructure (Cat 4 goldens, Cat 7 licenses, Cat 6 CI
discipline) is already on the badge path; we just don't need to ship
"earn the badge" as a category.

#### 1.7.O Tool qualification and independence-of-verification

**Framing.** The integrity toolkit is a verification tool that
detects errors but doesn't generate certified code. In DO-178C/DO-330
vocabulary that maps to **TQL-5-equivalent discipline** (Criterion 3,
lowest qualification rigor — appropriate because the project does
not pursue aerospace certification). The *language* matters because
it gives the toolkit's own assurance work a name.

**TQL-5-equivalent deliverables mapped to v2 work:**

| DO-330 TQL-5 objective | v2 deliverable |
|---|---|
| Tool requirements specification | `toolkit-spec.md` (this document) |
| Tool operational requirements | `tools/integrity/docs/conventions.md` + per-rule docs (§ 1.7.D) |
| Tool installation procedures | Diátaxis tutorial (§ 1.7.P) |
| Tool verification | `tools/integrity/tests/` + mutation testing (§ 1.7.K) |
| Tool problem reporting | GitHub issues + `audit-trail.md` banking log |
| Configuration management | Git history + rule-ID stability (§ 1.7.C) |
| Tool quality assurance | Self-application (§ 2.9.6); CI runs the toolkit on the toolkit |

**Probe AA condition (banking row 73 FACT-converted):** TQL-5 framing
holds with two preconditions:

1. The two integration smokes — `test_emit_state_snapshot_smoke`
   (103s) and `test_driver_builds_and_runs` (25s) — are gated behind
   `@pytest.mark.integration` and registered in `pyproject.toml`. With
   these quarantined, the inner-loop test suite drops from ~133s to
   ~5s.
2. `tools/integrity/integrity/common/audit_log.py` is either wired to `runner.main()` and tested
   (Probe BB option (c)) or excised (Probe BB option (d)). The
   current state (0% coverage, 0 production callers, phantom
   `--no-audit-log` CLI flag) is a HIGH-severity TQL-5 surface —
   the tool advertises behavior that does not exist. See § 2.9.7
   for the new v2.0 batch addressing this.

**Independence-of-verification.** A canonical safety-software
principle: verification is performed by personnel independent of
those who developed the verified code. For an autonomous-agent
workflow this maps to:

- **Multi-Claude-chat orchestration.** The chat session that authors
  a check is not the same session that audits it. Probe charters
  dispatch fresh sessions; outputs land in `docs/diagnostics/_audits/`.
- **Claude Code dual-agent file verification.** File-level fact-checking
  is done by Claude Code reading the live tree at HEAD, separate from
  the chat session that authored the claim (audit-trail § 4.2, § 4.4).
- **No single session authors both a check and its tests.** Mutation
  testing (§ 1.7.K) is the trust-but-verify layer.

The Architect-2 framing (formally dropped in § 3.9) was a precursor of
this discipline that proved unnecessary because the multi-chat +
Claude Code workflow already achieves independence at finer
granularity than a single human reviewer would.

#### 1.7.P Documentation framework (Diátaxis)

**Decision: adopt the Diátaxis four-type structure** (tutorial,
how-to, reference, explanation) for toolkit docs. The current state
is "reference plus accreted explanation in audit-trail" — adequate
for solo-developer + Claude-chat workflow but fragile when new
contributors enter and the toolkit becomes load-bearing for shipped
sims.

**v2 deliverable: docs tree under `tools/integrity/docs/`:**

```
tools/integrity/docs/
  tutorials/
    getting-started.md          # "Run your first integrity check"
    your-first-cat3-invariant.md
  how-to/
    add-a-cat3-invariant.md
    add-a-canonical-reference.md
    suppress-a-finding.md
    rebaseline-a-cat4-golden.md
    interpret-sarif-output.md
    dispatch-an-audit-probe.md
  reference/
    rules/<check_id>.md          # per-rule docs (§ 1.7.D)
    INDEX.md                     # rule index (§ 1.7.C)
    conventions.md               # already exists
    cli.md
    configuration.md
  explanation/
    canonical-reference-principle.md
    live-source-stays-red.md
    auxiliary-vs-authoritative.md
    vv-framework.md              # links Cat 3 to V&V vocabulary
```

**Lands at v2.34 expanded.** Current v2.34 scopes per-rule docs only;
extend to land the full Diátaxis tree. Added scope: 6 how-tos
× ~200 LOC + 4 explanations × ~300 LOC ≈ 2400 LOC; +3 commits beyond
existing v2.34 scope.

#### 1.7.Q Findings UX standards

**Industry baseline** (Rust compiler ca. 2015, adopted by TypeScript,
Ruff, Clippy, ESLint flat-config): code frames, suggested fixes, ANSI
color, end-of-run summary.

**Toolkit state at HEAD per Probe ZZ (banking row 66 FACT-converted):**

| Element | State | Detail |
|---|---|---|
| Code frame | **absent (total gap)** | Stanza is two-line `<mode>: <check_id> at <file>:<line>` + message; no source quote, no caret. |
| ANSI color | **absent (total gap)** | Zero `color`/`tty`/`NO_COLOR` references in toolkit code. Piped output is byte-identical. |
| Suggested fixes | **partial** | `cat1.bare-path` emits `"…suggested rewrite: '<path>:<line>'"` inline in finding message (`bare_path.py:240-259`); no other check does. No `--fix` apply mode. |
| End-of-run summary | **partial — leading position** | One-line summary `integrity: <P> pass, <S> soft-warn, <H> hard-fail, <U> suppressed` exists at `runner.py:158-163`, but emitted *before* stanzas (inverted vs. industry baseline). No per-category breakdown, no fix-availability count, no elapsed-time. |
| `--output` flag | **present** | `--output {human,json,github}` (default `human`). JSON mode emits `{schema_version, commit, summary, findings}`; GitHub mode emits `::error file=…,line=…::` workflow commands. SARIF mode absent. |

**v2 deliverable shape (revised per Probe ZZ):**

**(A) Extend existing `--output` flag with `sarif` as a fourth
choice.** Do NOT introduce a parallel `--format` flag. The spec's
SARIF work in § 1.7.A lands as `--output sarif`.

**(B) Code frames in `--output human`.** Greenfield work. Read source
files at render time to emit 2-line context with caret pointing at
`<line>:<col>` (current `Finding` dataclass lacks column data;
extension required — add `column` / `end_column` fields to
`results.py:16-43`, populate at check time where available, fall
back to whole-line highlight where not).

**(C) ANSI color.** Greenfield work. Add `colorama` (Windows-safe);
auto-disable when stdout is not a TTY; honor `NO_COLOR=1`. Severity
color: red HARD_FAIL, yellow SOFT_WARN, dim suppressed.

**(D) End-of-run summary — move + enrich.** Move existing
`_emit_human_summary` line from leading to trailing position. Enrich
with: per-category breakdown (`cat1: 6 HARD_FAIL`), fix-availability
count (`46 HARD_FAIL, 8 with suggested fix`), optional elapsed-time
(`completed in 103s`).

**(E) Suggested-fix systematization + `--fix` mode.** Extend
`Finding` with optional `fix: Fix | None` field carrying a unified-diff
hunk. Migrate `cat1.bare-path`'s inline-text suggestion into the
structured field. Add `python3 -m integrity --fix` mode that applies
fixes; pair with `--dry-run` showing unified diff without applying.
Also: extend `--rewrite-stale-reasons` (the v1.3 stale-reason rewrite
feature; no convention attribution — `conventions.md` lines 133–139
disown the prior letter-I citation) to emit unified diff in addition to
its existing count summary.

**Renderer abstraction.** Current `emit_output()` at `runner.py:119-163`
is a single function with three hard-coded `elif` branches and no
abstraction. v2 has two viable paths: (a) factor out a renderer
interface (cleaner long-term; ~80 LOC refactor) or (b) accept the
elif chain and add a fourth branch for SARIF. Decision deferred to
v2.32 spec authoring; both are acceptable.

**Lands at v2.32–v2.33** alongside SARIF work. SARIF is the machine
surface; `--output human` with code frames + color + trailing summary
is the human surface; both share the same `Finding` model.

#### 1.7.R Type-strictness for toolkit code itself

**Policy.** Tooling that lints user code at strict-type level must
itself run at equal-or-stricter type checking. Per Probe YY (banking
rows 74 + 79 FACT-converted): `tools/integrity/pyproject.toml`
already declares `[tool.mypy] strict = true` and a curated 9-prefix
ruff selection (`E,F,W,I,UP,B,C4,PIE,RUF`). The gap is **not
configuration** — it's CI enforcement, hotspot remediation, suppression
hygiene, and a deliberate decision on which currently-unselected ruff
families to expand into.

**Precedent.** `.github/workflows/build-py.yml` already runs
`ruff check .` + `mypy --strict <pkg>` against sim-side packages
(`common/common-py`, `mpm-multimaterial`, `lenia-fft`). The pattern
exists; v2.28 extends it to `tools/integrity/`.

**v2 deliverable — four-part shape:**

**(1) CI enforcement.** Add `mypy --strict integrity/` + `ruff check
integrity/` steps to `.github/workflows/integrity.yml`. Per Probe YY:
neither tool is invoked against the toolkit in any workflow today;
the declared strictness is aspirational. The currently-selected
9-family ruff `select` alone would surface ≥113 findings if invoked
(70 E501 + 28 I001 + 10 B + 5 UP + uncounted F/W/C4/PIE/RUF). All
findings absorbed into grandfather catalog at baseline; CI gates fail
on any *new* finding above baseline.

**(2) Hotspot remediation — `tools/integrity/integrity/cat3_numerical/d3q19_verify.py`.** Probe
YY found 49 mypy errors in 6 files, of which 42 (86%) are in
`d3q19_verify.py`. The error mix (`type-arg`, `has-type`, `arg-type`,
`index`) is shape-of-data complaints typical of numpy-heavy code
without typed `ndarray` wrappers — not latent bugs. Fix: add
`numpy.typing.NDArray` parametric aliases. ~42 errors collapse to ~0
with one focused refactor. Estimated ~1 day; smaller than originally
scoped because the work concentrates in one module.

**(3) New Convention L: toolkit-source suppression hygiene.**

> **Convention L (toolkit-source suppression hygiene).** Every
> `# type: ignore` and `# noqa` annotation in `tools/integrity/`
> source carries:
>
> - **Required:** a rule code (e.g. `# noqa: F401`, never bare
>   `# noqa`). Probe YY baseline: 5/5 sites already rule-coded.
> - **Required:** a reason — a parseable comment after the rule code
>   explaining why the suppression exists.
> - **Required if reason is forward-looking:** a tracked issue
>   reference (`# noqa: F401  # waiting on #123`). A reason like
>   "used in later commits" without an issue ref is anti-pattern —
>   it has no expiry mechanism. Either the work landed (suppression
>   is stale) or it didn't (the suppressed line should be deleted).
> - **Recommended:** a review-by-date for any suppression older than
>   one minor version (`# noqa: F401  # waiting on #123 (review by 2026-08)`).

Empirical baseline at HEAD per Probe YY (banking row 79): 5
suppression sites total, all rule-coded. Migration cost is
**near-zero** — the toolkit is already a careful suppression user.
The interesting Convention L decision is whether `runner.py:19`'s
"used in later commits" aside (forward-looking, no issue ref) is
grandfathered or migrated to issue-ref form. Recommendation: migrate
to issue ref. The `stack_c.py:22` `# type: ignore[import-not-found]`
is already redundant (mypy's `warn_unused_ignores` flags it as
`[unused-ignore]`) and should be deleted, not retrofitted.

Convention L lands in `tools/integrity/docs/conventions.md` alongside
v2.28 commit 3.

**(4) Ruff selection expansion — staged.** Probe YY's recommended
sequencing: keep current 9-family selection at v2.28; add `S`
(security, 17 findings) and `B` is already in select (10 findings).
**Defer** `D` (107 docstring findings — wait on docstring conventions
decision) and `ANN` (annotations — wait on numpy-typing refactor).
**Defer** `--select ALL` as an aspirational ceiling rather than v2
target — 468 findings under ALL is a re-scoping problem, not a
grandfather-list problem.

| Family | Findings (Probe YY) | v2.28 disposition |
|---|---:|---|
| E (selected) | 70 (E501) | Adopt + `ruff format` auto-fix |
| I (selected) | 28 (I001) | Auto-fix |
| B (selected) | 10 | Manual review; small count |
| UP (selected) | 5 | Auto-fix |
| F, W, C4, PIE, RUF (selected) | unspecified small | Manual review |
| **S** | 17 | **Add at v2.28** (mostly subprocess-with-partial-path; substantive) |
| **COM** | 26 | **Add at v2.28** (auto-fix; mechanical) |
| D | 107 | Defer to v2.34 (Diátaxis docs) — docstring style decision required first |
| PLR | 27 | Per-rule review at v2.28 (PLR2004 most actionable) |
| C, SIM, TC, T, N, etc. | various | Per-category review deferred to v2.33+ |
| ANN | 5 | Defer until d3q19_verify numpy-typing lands |

**Lands at v2.28** (Cat 6 python-lint unification, expanded to 5 commits):

1. CI step adds `mypy --strict integrity/` + `ruff check integrity/`
   with grandfather-catalog absorbing baseline ~113 findings.
2. `d3q19_verify.py` numpy typing refactor (~42 errors → ~0).
3. Convention L lands in `conventions.md`; 5 existing annotations
   retrofitted (or 2 deleted as stale, 3 augmented).
4. Ruff selection expansion: add `S`, `COM`; document deferral
   rationale for the rest in `conventions.md`.
5. SHA back-fill.

**Verdict on the original v2.28 framing:** SHIFTED. Original wording
implied "grandfather-catalog migration is sufficient." Per Probe YY:
catalog covers the documentary portion; substantive work is the
`d3q19_verify.py` typing refactor + CI gate. Adoption cost for
Convention L itself is near-zero (already 0 bare suppressions).

### 1.8 Cat 7 — Security and dependency hygiene

**Defined as:** dependencies (vendored and transitive) are free of
known vulnerabilities, licenses are compatible, secrets aren't
committed, GitHub repo posture meets security baselines.

**v2 approach: integrate existing industry-standard tools, don't reinvent.**

#### 1.8.A `cat7.pip-audit` — Python dependency vulnerability scan

`pip-audit` (PyPA-maintained, https://pypi.org/project/pip-audit/) scans
installed Python packages against the PyPI Advisory Database. Industry
standard for Python supply-chain security.

v2 integration: install pip-audit in integrity.yml; run against each
Python package's resolved environment; emit findings via cat7 stanza
in main output.

#### 1.8.B `cat7.npm-audit` — Node dependency vulnerability scan

`npm audit` is built into npm. Scans `package-lock.json` against the
npm advisory database. Industry standard.

v2 integration: run `npm audit --json` in integrity.yml; parse and
emit cat7 findings.

#### 1.8.C `cat7.action-sha-pinned` — GitHub Actions pinned to SHA

**Industry standard** (OpenSSF Scorecard "Pinned-Dependencies" check):
GitHub Actions should be pinned to commit SHA, not floating version
tag. Floating tags (e.g., `actions/checkout@v4`) are subject to tag
spoofing attacks; SHA pins (e.g., `actions/checkout@b4ffde65f46336ab88eb53be808477a3936bae11`)
are immutable.

our workflows use floating tags (`actions/checkout@v4`,
`actions/setup-python@v5`, etc.). v2.36 migrates to SHA pins with
matching version-tag comments (`# v4.2.0`) for human readability.

dependabot keeps the SHA pins up to date — pairs with Cat 6
dependabot.yml addition.

#### 1.8.D `cat7.no-secrets-in-code` — gitleaks scan

`gitleaks` (https://github.com/gitleaks/gitleaks) scans repo content
and git history for secrets (API keys, tokens, passwords, certificates).
Industry standard; runs as pre-commit hook or CI step.

v2 integration: gitleaks as cat7 sub-check. Scan history once at v2.35
baseline; subsequent runs scan only the diff (cheap).

#### 1.8.E `cat7.license-compatible` — vendored dep license check

**Tool options:**
- `pip-licenses` (Python)
- `license-checker` (Node)
- `scancode-toolkit` (general, comprehensive)

v2 integration: verify each vendored upstream (SPlisHSPlasH, dimod when
landed, lbm-principles-practice) has a compatible license (MIT, Apache 2,
BSD-3-Clause). GPL-licensed deps would contaminate the MIT/Apache code
in the rest of the repo.

#### 1.8.F `cat7.openssf-scorecard` — overall repo posture

OpenSSF Scorecard (https://github.com/ossf/scorecard) evaluates a
repository against 16+ automated security checks:

- Token-Permissions: workflows declare read-only GITHUB_TOKEN by default.
- Branch-Protection: main branch is protected (requires PR + reviews).
- Pinned-Dependencies: actions pinned to SHA (Cat 7.C).
- Dependency-Update-Tool: dependabot or renovate configured.
- Code-Review: PRs reviewed before merge.
- Security-Policy: SECURITY.md exists.
- License: detected at repo root.
- CII-Best-Practices: Linux Foundation badge.
- Signed-Releases: releases signed.
-... and more.

Each check scored 0-10. Industry expectation: score > 7 for serious projects.

v2 integration: run Scorecard in scheduled workflow; fail if score
drops below threshold. The runner emits findings as cat7 stanzas
for any check below threshold.

#### 1.8.G `cat7.slsa-build-provenance` — SLSA Build Track alignment

**Target: SLSA Build L2 at v2.42; Build L3 as Phase 12 candidate.**

SLSA Build L2 requires hosted-build provenance with cryptographic
signing. The Hermetic Builds Level 3 work in § 1.7.L (v2.42)
produces a containerized, hash-pinned runner — SLSA Build L2 is
achieved by adding `slsa-github-generator` to the same workflow,
which emits signed in-toto attestations to Sigstore Rekor.

SLSA Build L3 requires a hardened build platform (isolated build
environment + non-falsifiable provenance). The current GitHub-hosted
runner is L2-capable; L3 requires either GitHub reusable-workflows
discipline (token scoping, secret isolation) or self-hosted runner
hardening. Defer to Phase 12.

Cross-reference: the Hermetic Builds L3 (§ 1.7.L) and SLSA Build L2
(this section) address overlapping concerns from different
vocabularies — reproducibility vs. provenance-attestation. They land
in the same batch (v2.42 expanded).

#### 1.8.H `cat7.sbom-generated` — SBOM artifact generation

**Decision: emit both CycloneDX 1.7 and SPDX 3.0.**

CycloneDX is security-focused (native VEX support); SPDX is
license-compliance-focused (ISO/IEC 5962:2021). For GPU-Sims the
relevant consumers are the academic reproducibility audience (ACM
Artifact Evaluation, § 1.7.N — SPDX-leaning) and security tooling
(CycloneDX-leaning). Generation cost is one CI step per format.

**v2 deliverable** (artifact filenames below are v2.38-produced; not extant at HEAD):

- `cyclonedx-py environment` → `sbom-python.cdx.json`
- `@cyclonedx/cdxgen` → `sbom-node.cdx.json`
- `spdx-tools` aggregated → `sbom.spdx.json`
- Attached as release artifacts on each v2.X tag in CI; included in
  SLSA provenance bundle (§ 1.8.G).
- `cat7.sbom-present` check verifies SBOM artifacts exist for the
  current tag.

**Lands at v2.38** (extending action-SHA-pinned + secrets-scan batch
with +2 commits for SBOM generation).

---

## § 2 — Inventory by category (item-level)

This section enumerates every item destined for v2 across all six
categories. Each entry: shape, source, scope estimate, dependencies,
proposed batch placement.

### 2.1 Cat 1 items

| # | Item | Source | Scope | Batch |
|---|---|---|---|---|
| 2.1.1 | G.2 grammar fix + sweep | root cause; closeout § 0.2 | ~30 LOC + tests; 1 commit | v2.0 |
| 2.1.2 | Multi-line citation grammar | v1 spec § 13 A.4 | ~50 LOC + tests; 1 commit | v2.1 |
| 2.1.3 | Fenced-block awareness + cleanup sweep | v1 retro § 4 | ~80 LOC + sweep; 2 commits | v2.1 |
| 2.1.4 | Stack A citation coverage | | ~60 LOC + tests; 1 commit | v2.2 |
| 2.1.5 | `cat1.shadertoy-port-mapping` | | ~80 LOC + tests; 1 commit | v2.2 |

### 2.2 Cat 2 items

| # | Item | Source | Scope | Batch |
|---|---|---|---|---|
| 2.2.1 | Type-aware Stack C member access | unused scaffold | ~80–120 LOC + tests; 2 commits | v2.3 |
| 2.2.2 | `cat2.capture-schema-consistent` | cross-stack drift | ~200 LOC + cross-stack fixtures; 2 commits | v2.3 |

### 2.3 Cat 3 items

| # | Item | Source | Scope | Batch |
|---|---|---|---|---|
| 2.3.1 | Quantum cat3 seed (4 checks + dimod vendor) | quantum.md § 6.1 + | ~600–800 LOC + 3 derivations + 4 checks; 5–7 commits | v2.4 |
| 2.3.2 | MMS harness foundation | new (§ 1.3.B) | ~300 LOC harness + sympy gen + verifier; 3–4 commits | v2.5 |
| 2.3.3 | Per-sim cat3 invariants — set A (3 sims: boids/strange-attractors/RD-2D) | new (§ 1.3.C) | ~150 LOC per check × ~3 checks per sim = ~1350 LOC; 4–6 commits | v2.6 |
| 2.3.4 | Per-sim cat3 invariants — set B (3 sims: physarum/RD-3D/sph-water) | new | similar; 4–6 commits | v2.7 |
| 2.3.5 | Per-sim cat3 invariants — set C (4 sims: smoke/MPM/LBM/lenia/mandelbulb) | new | similar; 4–6 commits | v2.8 |
| 2.3.6 | Smoke MMS check — high priority | bug | ~200 LOC + sim modification; 2 commits | v2.6 (priority) |
| 2.3.7 | RD-2D / RD-3D MMS check | new | ~200 LOC + sim modification; 2 commits | v2.7 |
| 2.3.8 | LBM Poiseuille MMS check | new | ~150 LOC; 1 commit | v2.7 |
| 2.3.9 | SPH pressure-Poisson MMS check | new | ~250 LOC; 2 commits | v2.8 |
| 2.3.10 | MPM particle-grid MMS check | new | ~250 LOC; 2 commits | v2.8 |
| 2.3.11 | Cat 3 GPU headless Stack B (dawn.node) | v1 spec § 13; finding | ~300 LOC harness + per-sim integration; 5–7 commits | v2.9 |
| 2.3.12 | Cat 3 GPU headless Stack C (lavapipe) | v1 spec § 13; finding | ~400 LOC harness + per-sim integration; 5–7 commits | v2.10 |
| 2.3.13 | Cross-stack equivalence | new | ~150 LOC harness + 2 port pairs; 3 commits | v2.11 |
| 2.3.14 | Differentiability foundations | new + gap-analysis | ~200 LOC harness; 2 commits | v2.12 |

### 2.4 Cat 4 items

| # | Item | Source | Scope | Batch |
|---|---|---|---|---|
| 2.4.1 | Per-stack CLI stubs (3 commits, rule-of-three) | + | Stack B ~80 LOC × 5 sims; Stack C ~120 LOC + RD-3D signature change × 4 sims; Stack D ~30 LOC × 2 sims; 3 commits | v2.13 |
| 2.4.2 | Stack B Playwright harness | finding | ~200 LOC + CI integration; 2 commits | v2.13 |
| 2.4.3 | Cat 4 harness core (build-smoke + tolerance-snapshot + bitwise + determinism-pair + replay) | new | ~600 LOC + tests; 5 commits | v2.14 |
| 2.4.4 | Git LFS setup for goldens | new | ~10 LOC `.gitattributes` + workflow LFS pull; 1 commit | v2.14 |
| 2.4.5 | Vulkan validation layer integration | new | ~150 LOC + CI + parser for validation output; 2 commits | v2.15 |
| 2.4.6 | Visual regression harness (pixelmatch + SSIM + PSNR + HTML report) | new | ~500 LOC + HTML template; 3–4 commits | v2.16 |
| 2.4.7 | Per-sim Cat 4 adoption — smoke (priority due to findings) | new | canonical inputs + goldens + tolerance + screenshots + MMS-source integration; 2–3 commits | v2.17 |
| 2.4.8 | Per-sim Cat 4 adoption — sph-water (priority due to findings) | new | similar; 2–3 commits | v2.18 |
| 2.4.9 | Per-sim Cat 4 adoption — MPM | new | 2 commits | v2.19 |
| 2.4.10 | Per-sim Cat 4 adoption — set A (boids, RD-2D, physarum) | new | 3 commits | v2.20 |
| 2.4.11 | Per-sim Cat 4 adoption — set B (RD-3D, LBM, lenia) | new | 3 commits | v2.21 |
| 2.4.12 | Per-sim Cat 4 adoption — set C (strange-attractors, mandelbulb) | new | 2 commits | v2.22 |
| 2.4.13 | Memory-clean (ASAN) for Stack C | new | ~150 LOC + per-sim adoption; 4 commits | v2.23 |
| 2.4.14 | Strict-warnings (all stacks) | new | ~100 LOC + per-stack adoption; 3 commits | v2.23 |

### 2.5 Cat 5 items

| # | Item | Source | Scope | Batch |
|---|---|---|---|---|
| 2.5.1 | Audit-prose freshness gate integration | closeout commit 3 promoted | ~50 LOC; 1 commit | v2.24 |
| 2.5.2 | Spec-claim resolution | motivating example | ~300–400 LOC + fixtures; 3–4 commits | v2.25 |
| 2.5.3 | CHANGELOG / git-tag consistency | phase11 retro | ~120 LOC + tests; 1 commit | v2.26 |
| 2.5.4 | Sim-spec divergence noted | Phase 9 banked | ~200 LOC + per-sim baseline; 2 commits | v2.26 |
| 2.5.5 | Conventions doc coverage | new | ~80 LOC; 1 commit | v2.26 |
| 2.5.6 | Vendored-anchor freshness | new | ~100 LOC; 1 commit | v2.26 |
| 2.5.7 | Intra-doc markdown anchor resolution | new | ~100 LOC + tests; 1 commit | v2.27 |
| 2.5.8 | Cross-stack divergence noted | new + | ~120 LOC + tests; 1 commit | v2.27 |
| 2.5.9 | Capture-format-reference freshness | stale-doc finding | ~80 LOC + reference doc update; 2 commits | v2.27 |

### 2.6 Cat 6 items

| # | Item | Source | Scope | Batch |
|---|---|---|---|---|
| 2.6.1 | actionlint integration | missing tool | ~30 LOC workflow + config; 1 commit | v2.28 |
| 2.6.2 | shellcheck (via actionlint) | included | 0 LOC marginal | v2.28 |
| 2.6.3 | cmake-lint integration | new | ~50 LOC; 1 commit | v2.28 |
| 2.6.4 | ruff + mypy unification across integrity.yml | divergent | ~30 LOC; 1 commit | v2.28 |
| 2.6.5 | tsc-strict + eslint + prettier shared config | partial | ~80 LOC; 2 commits | v2.29 |
| 2.6.6 | Deps pinning enforcement | new | ~80 LOC + tests; 1 commit | v2.30 |
| 2.6.7 | Workflow cache discipline | finding | ~30 LOC across workflows; 1 commit | v2.30 |
| 2.6.8 | Workflow OS pinned (ubuntu-24.04 everywhere) | | ~10 LOC; 1 commit | v2.30 |
| 2.6.9 | Lockfile-strict (`npm ci` everywhere) | | ~5 LOC; 1 commit | v2.30 |
| 2.6.10 | CODEOWNERS + dependabot.yml | missing | ~50 LOC; 1 commit | v2.31 |
| 2.6.11 | CMake flags consistent | new | ~80 LOC per CMakeLists; 2 commits | v2.31 |

### 2.7 Cat 7 items

| # | Item | Source | Scope | Batch |
|---|---|---|---|---|
| 2.7.1 | cat7.pip-audit integration | Industry standard (PyPA) | ~40 LOC + CI step; 1 commit | v2.37 |
| 2.7.2 | cat7.npm-audit integration | Industry standard (npm) | ~30 LOC + CI step; 1 commit | v2.37 |
| 2.7.3 | cat7.action-sha-pinned migration | OpenSSF Scorecard | ~50 LOC across all workflows + dependabot config tie-in; 1-2 commits | v2.38 |
| 2.7.4 | cat7.no-secrets-in-code (gitleaks) | Industry standard | ~30 LOC + CI step + baseline scan; 1 commit | v2.38 |
| 2.7.5 | cat7.license-compatible | Industry standard | ~80 LOC + scancode-toolkit or pip-licenses integration; 1 commit | v2.39 |
| 2.7.6 | cat7.openssf-scorecard | Industry standard | ~50 LOC + scheduled workflow + threshold config; 1 commit | v2.39 |

### 2.8 Toolkit-itself standards items

| # | Item | Source | Scope | Batch |
|---|---|---|---|---|
| 2.8.1 | SARIF 2.1.0 output mode | § 1.7.A | ~250 LOC emitter module + schema validation + tests; 2 commits | v2.32 |
| 2.8.2 | partialFingerprints computation | § 1.7.F | ~80 LOC + tests + audit-report integration; 1 commit | v2.32 |
| 2.8.3 | upload-sarif workflow integration | § 1.7.A | ~30 LOC workflow step; 1 commit | v2.32 |
| 2.8.4 | Exit-code convention verification | § 1.7.B | ~20 LOC verification + runner help text update; 1 commit | v2.33 |
| 2.8.5 | Auto-fix safety classification | § 1.7.E | ~30 LOC convention doc update; 1 commit | v2.33 |
| 2.8.6 | Per-rule documentation pages | § 1.7.D | ~30 LOC per rule template × ~20+ rules at v2.34 time = ~600 LOC + INDEX.md + stability contract; 3-4 commits | v2.34 |
| 2.8.7 | cat5.doctest-spec sub-check | § 1.7.G | ~100 LOC + Python-spec-doc harness; 1 commit | v2.35 |
| 2.8.8 | Hypothesis property-based Cat 3 variants | § 1.7.H | ~50 LOC harness + per-invariant property variants (~20 invariants × ~20 LOC each = ~400 LOC); 3 commits | v2.36 |

### 2.9 Toolkit hygiene (renumbered from rev 2's 2.7)

| # | Item | Source | Scope | Batch |
|---|---|---|---|---|
| 2.9.1 | G.4 KNOWN_CATEGORIES pinning test | closeout banked;.1 partially closed | ~15 LOC if not yet done | v2.0 |
| 2.9.2 | `_emit_human_summary` ordering | v1.2 bolt-ons § 5 | ~10 LOC | v2.0 |
| 2.9.3 | velocity.bin.bin bug fix (mechanism reframed in rev 11/QQ — literal doubled suffix arises at runtime in `state_writer.cpp:57`, not in source; spec picks state-writer fix Option A or call-site fix Option B at v2.0 spec time) | banked quirk; reframedlaim 4 + row 6 | ~5 LOC state-writer fix OR per-call-site cleanup; 1 commit | v2.0 |
| 2.9.4 | Reader API drift fix (Python mtime vs C++ lexical) | | ~50 LOC + tests; 1 commit | v2.1 |
| 2.9.5 | _FORMAT_TO_DTYPE for r8uint | | ~5 LOC + test; bundle | v2.1 |
| 2.9.6 | Self-application Cat 2 coverage | v1 retro § 4 | ~20 LOC if coverage gaps found + audit; otherwise no-op (cat2.public-symbol-used-toolkit is wired and emits 25 findings against toolkit modules) | v2.40 |
| 2.9.7 | **`tools/integrity/integrity/common/audit_log.py` wire-up + tests** (HIGH-severity TQL-5 fix per Probe BB row 74). Wire `runner.main()` to call `audit_log.append_findings(args.root, findings, git_head_sha(args.root))` under `not args.no_audit_log` when `findings` is non-empty; add unit tests for `audit_log_path` + `append_findings`; add end-to-end test asserting the file is written with expected front-matter. Closes the phantom-`--no-audit-log`-flag surface and unblocks `audit_log.py` from v2.41 mutation-testing exclusion. **Alternative (Probe BB option d):** demolition — delete `audit_log.py` + `--no-audit-log` flag + `no_audit_log` CliArgs field; re-anchor § 3.3 in legacy spec as "deferred / out of scope at v1.3." Recommendation: option (c) — wire-up. Demolition only if § 3.3's audit-log surface is genuinely abandoned. | ~40 LOC + 3 tests + 1 end-to-end; 2 commits | **v2.0** (priority bumped — HIGH-severity TQL-5 surface; must land before v2.41 mutation testing) |
| 2.9.8 | **Pytest integration-test markers** (Probe AA row 73 dependency). Add `@pytest.mark.integration` to the two slow tests (`test_emit_state_snapshot_smoke` 103s, `test_driver_builds_and_runs` 25s); register `slow` and `integration` markers in `pyproject.toml` `[tool.pytest.ini_options].markers`. Inner-loop suite drops from ~133s to ~5s; enables v2.41 mutmut. CI runs both `pytest -m "not integration"` (inner loop) and `pytest -m integration` (nightly / pre-release). | ~5 LOC + 1 pyproject.toml entry; 1 commit | **v2.0** (priority bumped — prerequisite for v2.41) |

---

## § 3 — Cross-cutting design decisions

### 3.1 Golden storage: Git LFS in-tree

**Decision: Git LFS, tracked under `<sim>/tests/integrity/{goldens,screenshots}/**`.**

Per web search consensus:

- **VisIt** (LLNL scientific visualization, closest analog to our use case): "test data and baselines stored using git lfs." Comparable scientific-viz project with regression suite.
- **ImageSharp** (image processing reference outputs): LFS recommended for binary regression assets.
- **subito.it production visual regression**: "To keep the repository lightweight and fast to clone, we use Git LFS."

Concrete numbers for GPU-Sims:

- Small sims (boids 1000 particles, RD-2D, strange-attractors): ~50–250KB per capture frame.
- Medium sims (eulerian-smoke 96³, LBM): ~10MB per capture frame.
- Large sims (MPM 250K particles, sph-water 1M particles): ~10–30MB per capture frame.
- Screenshots: ~500KB–2MB per frame.
- 11 sims × 3 canonical inputs × ~4 capture frames × averaged size ≈ 500MB–2GB total.

Plain in-tree would bloat the repo unacceptably; out-of-tree (DVC, S3) adds CI plumbing and auth complexity; **Git LFS is the right middle ground.** GitHub free tier (1GB storage + 1GB/month bandwidth) is borderline; paid tier ($5/mo for 50GB storage + 50GB bandwidth) is comfortable. Steven's plan to self-host eventually (Option C) doesn't change the v2 design — LFS pointers can target a self-hosted LFS server with no code changes when the time comes.

### 3.2 Per-sim test fixture location

**Decision: per-sim under `<sim>/tests/integrity/`.**

Structure:

```
<sim>/
 tests/
 integrity/
 canonical_inputs/
 small.json
 medium.json
 edge-case.json
 goldens/ # LFS-tracked
 small_frame_30.captureset/
 state.json
 *.bin
 screenshots/ # LFS-tracked
 small_frame_30.png
 small_frame_100.png
 masks/ # if needed for visual regression
 ui_overlay_mask.png
 config.toml # tolerances, frame indices, sub-check enables
```

Registry pointer at `tools/integrity/integrity/cat4_runtime/registry.toml`
enumerates participating sims.

### 3.3 Cat 4 fail semantics — separate snapshot mechanism (not `integrity-allow`)

**Decision: separate machinery, not shared with suppression grammar.**

Snapshots are versioned reference data; suppression is for grandfathered findings. The two have different lifecycles:

- *Suppression:* expected to disappear when the underlying issue is addressed.
- *Rebaseline:* expected to recur every time the sim is intentionally changed.

Cat 4 ships its own rebaseline CLI:

```
python3 -m integrity --rebaseline cat4.tolerance-snapshot <sim> <input>
python3 -m integrity --rebaseline cat4.visual-regression <sim> <input>
```

Updates the golden / screenshot and a sibling `golden_sha.txt` recording the SHA against which the golden was generated.

### 3.4 CI complexity tier

Per web search, the additions are all well-trodden:

| Addition | Cost | Precedent |
|---|---|---|
| Git LFS | apt-installable; lfs install in CI | VisIt, ImageSharp, subito.it |
| `webgpu` npm package (Dawn for Node) | ~30s `npm install` | VTK CI (Linux) |
| `mesa-vulkan-drivers` (lavapipe) | ~10s apt install | Chromium GPU-less CI |
| `pytest-regtest` | one `pip install` | Tezos integration testing |
| `pixelmatch` + `ssim.js` (Node) or `scikit-image` (Python) | one dep install | jest-image-snapshot, BackstopJS |
| `actionlint` | one binary download | Widely used, 3700+ stars |
| `cmake-lint` | one pip install | Standard for CMake projects |
| Playwright (Stack B headless) | npm install + headless Chromium download | Web visual regression standard |
| Vulkan validation layers extended features | apt package already present; just enable features | Khronos standard, GPU developer staple |

Total additional CI wallclock: ~3–5 minutes of setup + ~5–15 minutes of actual checks. Acceptable.

### 3.5 Stack A scope decision

**Decision: scope-bounded Stack A coverage via Cat 1 + Cat 5 only.**

: Stack A is exactly 278 LOC across 3 GLSL files in 2 sims. Frozen surface. No Cat 2/3/4 Stack A coverage. Cat 1 covers citation integrity for the GLSL files; Cat 5 covers port-mapping README presence and structure (cat5.cross-stack-divergence-noted).

### 3.6 Stack D self-application — MPM extends existing tests

`hybrid-particle-grid/mpm-multimaterial/python/tests/test_kernels.py` is the only sim with shipped tests. Cat 4 work for MPM extends this rather than introducing a fresh `tests/integrity/` directory. The Cat 4 harness must handle both patterns: `tests/integrity/` (new sims) and existing `tests/` augmentation (MPM only).

### 3.7 RD-3D entrypoint signature change

`continuous-ca/reaction-diffusion-3d/src/main.cpp` uses `int main` without argc/argv. v2.13 Stack C CLI stub must change the signature to `int main(int argc, char** argv)` first, then proceed with normal argv parsing. Per-stack stub commit identifies RD-3D as needing the extra signature change.

### 3.8 SPH-water context preservation

`particle-fluids/sph-water/src/main.cpp` uses non-default `gv::ContextCreateInfo` with `enable_subgroup_size_control = true`. Cat 4 `--headless` mode for sph-water must bypass Window creation while preserving the non-default context. The headless code path creates the Vulkan instance + device with the same context flags, just skips the windowing surface.

### 3.9 Architect framing dropped

Decisions in v2 are made when mechanical or clearly correct; decisions with real tradeoffs surface to Steven. No bookkeeping of "needs review by other chat." Historical conventions doc disclosure in `conventions.md` § "Decisions resolved without architect-2 review" remains the lineage record for v1.x decisions.

### 3.10 Closeout incorporation strategy

closeout is mid-flight at HEAD `a9b2aeb` with commits 1–2 landed. v2.0 design assumes some subset of closeout commits 3–8 will land before v2.0 ships. Specific incorporation points:

- If closeout commit 3 (`audit_prose_freshness.py`) lands: v2.24 promotes it to gate-integrated. If banked, v2.0 promotes a draft.
- If closeout commit 4 (T2.1 CI sweep step) lands: v2 inherits the discipline. If banked, v2.28 adds it as part of actionlint integration.
- If closeout commit 5 (conventions.md) lands: v2 references the file. If banked, v2.0 authors a stub.
- If closeout commit 6 (project-state.md cleanup) lands: G.2 remains v2's responsibility per closeout § 0.2.
- If closeout commit 7 (v1-closed marker) lands: v2 adds a v2-target marker; otherwise v2.0 lands both.

Each v2 batch's spec re-anchors against current HEAD at spec time.

### 3.11 Per-sim scope creep accepted

After v2 fully lands, the toolkit will have:

- 11 sims × ~3–5 cat3 invariants each = ~33–55 cat3 invariant checks
- ~5 cat3 MMS checks (PDE-based sims)
- ~5 cat3 cross-stack / GPU-headless / equivalence checks
- ~4 quantum cat3 checks
- 11 sims × ~6 cat4 sub-checks each = ~66 cat4 invocations
- ~9 cat5 checks
- ~13 cat6 checks
- Existing cat1 (4) + cat2 (3) = 7 baseline
- Plus toolkit-hygiene micro-items

**Total: ~140–160 active checks.** Tooling supports the volume: per-check modules are independently disableable; the runner emits stanza output that scales linearly; the catalog auto-refresh absorbs churn; the conventions doc remains stable in size.

### 3.12 Live-source-stays-red discipline continues

Per the live-source-stays-red discipline (not a banked convention at HEAD — Convention H in `conventions.md` is the filter-implementation rule, not this attribution policy), live-source findings get attributed in annotations, never swept. v2 doesn't change this. New Cat 4–6 categories add their own annotation grammar (Cat 4 separate per § 3.3); the live-source discipline applies to all of them.

### 3.13 Baseline-triage-before-lock (new,)

**Hard ordering constraint for Cat 4 visual goldens.** A sim with
known visual defects cannot have its goldens locked — the regression
mechanism would encode the bug as the reference. eulerian-smoke and sph-water have known defects today. For each
such sim, the Cat 3 invariant or MMS check that detects the defect
must land *first*, and the underlying bug must be fixed and held to
the new invariant for at least one full CI cycle, *before* the
corresponding Cat 4 baseline-lock batch runs.

Sequencing implication (reflected in § 4 phase plan): v2.6 (smoke
MMS) precedes v2.17 (smoke Cat 4 baseline). v2.7 (sph DFSPH residual)
precedes v2.18 (sph Cat 4 baseline). Other sims without known
defects can baseline immediately after their Cat 4 infrastructure
lands.

### 3.14 Probe-derived non-defensible divergences (new,)

The second probe round (F–M) surfaced concrete defects that are
*not* defensible divergences from industry standard — they are
real bugs or hygiene gaps to fix in v2. Catalogued here for the
single source of truth:

| Gap | Probe | Severity | Batch |
|---|---|---|---|
| cat1 annotation `body.rstrip("-->")` char-set strip bug | F | HIGH — parser bug | v2.0 |
| `EXIT_BAD_CLI=64` (BSD sysexits.h) vs Ruff/ESLint=2 | G | HIGH — convention violation | v2.0 |
| `EXIT_INTERNAL_FAIL=2` collides with expected bad-CLI exit | G | HIGH — exit-code collision | v2.0 |
| Fail-open on bad `--check cat99.nonexistent-check` | G | HIGH — silently green-lights CI | v2.0 |
| Fail-open on bad `--root /tmp/does-not-exist` | G | HIGH — silently green-lights CI | v2.0 |
| `velocity.bin.bin` on-disk filenames in eulerian-smoke captures (mechanism reframed in rev 11laim 4 + row 6 — literal not in source at HEAD; arises at runtime when `StateWriter::saveBuffer` at `state_writer.cpp:57` appends `.bin` to a name already ending in `.bin`) | K | HIGH — banked defect | v2.0 |
| `cat2.public-symbol-used-c` single-threaded ~95s | J → AA confirmed | HIGH — **91.7% of strict-run wallclock** (Probe AA re-anchored Probe J's ~60% estimate; bottleneck more concentrated than recorded) | **v2.-1 (Phase 0)** |
| `tools/integrity/integrity/common/audit_log.py` 0% coverage (load-bearing) | H | HIGH — risk surface | v2.0 |
| ANNOTATION_RE vs STRICT_RE parallel parsers | F | MEDIUM — duplication risk | v2.1 |
| `cmd.split` not `shlex.split` in Stack C | F | MEDIUM — breaks paths with spaces | v2.3 |
| `find_latest` C++/Python divergence (lexical vs mtime) | K | MEDIUM — F9 cross-session bug | v2.1 |
| `_FORMAT_TO_DTYPE` missing r8uint (silent dtype corruption) | K | MEDIUM — schema gap | v2.1 |
| C++/TS writers emit `bytes` field undocumented | K | LOW — schema undocumented | v2.3 or banked |
| 3 workflows missing `permissions:` block | I | MEDIUM — Scorecard hygiene | v2.37 |
| 0/17 actions SHA-pinned | I | MEDIUM — supply-chain hardening | v2.38 |
| No dependabot.yml | I | LOW — automated update gap | v2.38 |
| pytest 8.4.2 CVE-2025-71176 (dev-only) | I | LOW — bump to >=9.0.3 | v2.39 |

**Defensible divergences (preserved):**

| Divergence | Industry standard | Our choice | Why defensible |
|---|---|---|---|
| Semantic check IDs | Prefix-NNN (F401, S101) | Semantic (cat1.intra-repo) | Readability; ESLint also uses semantic names. |
| Custom stanza output as default | SARIF | Stanzas as default + SARIF additive | Audit-prose discipline load-bearing for retro. |
| Cross-stack scope | Language-bound | Single toolkit spans TS/C++/Py | Our reality is cross-stack ports; 1 toolkit > 3. |
| Live-source-stays-red discipline | `# noqa` allowed silently | Mandatory reason + issue ref | zero secret leaks / 0 high-sev — discipline pays off. |
| Grandfather catalog as single TODO file | Scattered disables | One structured file | Cleaner migration tracking. |
| Convention M (re-anchor before edit) | Linter authors handle internally | Documented user-discipline | Cross-Claude-chat workflow needs explicit. |
| Audit-report-per-commit retros | Emit-and-forget | Written trail per batch | Knowledge persistence across context windows. |

### 3.15 Floating-point determinism policy

Bitwise reproducibility (Cat 4 `bitwise-snapshot`, Cat 3 algebraic
exactness, FP-pillar of authority § 0.1.3) requires controlling
sources of FP non-determinism. Per Probe CC (banking row 75
FACT-converted): the toolkit currently has **no FP-determinism flags
applied anywhere** — neither in any `CMakeLists.txt`, nor in any CI
workflow, nor as `precise` qualifier on any GLSL/WGSL reduction.
GCC's `-O3 -ffp-contract=fast` default applies across all four Stack C
build trees. § 3.15 is therefore a fix-and-gate policy, not gate-only.

**Sources controlled (revised per Probe CC findings at HEAD):**

1. **Compiler FMA contraction.** GCC/Clang default `-ffp-contract=on`
   allows `a*b + c` to fuse; last bit differs from sequential ops.
   Confirmed unset across all build trees at HEAD.
2. **`-fast-math` / `-ffast-math`.** Reorders, drops infinity/NaN
   propagation. Never on for verification builds. Confirmed unset
   (also implicitly off — `-O3` does not enable `-ffast-math`).
3. **Scatter-slot ordering on GPU.** *Replaces* the original "GPU
   atomic-add float reduction" entry, which Probe CC found has zero
   instances in the repo (no float atomics anywhere). The actual
   Stack C/B non-determinism is particle-order-within-cell after
   scatter via `atomicAdd` on u32 slot counters:
   - `particle-fluids/sph-water/shaders/scatter.comp.glsl:31` —
     SPH-neighbour-loops then read particles in storage order →
     density/pressure/vorticity sums depend on dispatch-warp scheduling.
   - `agent-based/boids-3d/web/shaders/scatter.compute.wgsl:76` —
     same shape; affects steering force sums.

   The `atomicAdd` itself is bit-exact (integer add is associative);
   the slot allocation per particle is the non-deterministic part.
4. **Thread count for parallel reductions.** Different counts produce
   different reduction trees → different last-bit results. No
   `OMP_NUM_THREADS` / `TBB_NUM_THREADS` / `TAICHI_NUM_THREADS` pin
   in any CI workflow at HEAD.
5. **Driver-level FMA reordering** (Cat 4 visual-regression GPU
   jitter, § 7.3). Out of policy scope; tolerance-snapshot absorbs it.
6. **Backend / driver selection.** Stack D `ti.init(arch=...)` at sim
   entrypoints uses `ti.gpu`; `ti.cpu` is pinned only inside pytest
   fixtures. Stack B Dawn unpinned (only `@webgpu/types` is pinned —
   TS typedefs, not runtime). `lenia-fft/fft_backend.py:select_backend()`
   auto-probes — host-variant.

**Policy (revised — fix + gate per Probe CC):**

| Surface | v2 setting | HEAD state | Action |
|---|---|---|---|
| Stack C build flags | `-ffp-contract=off`, `-fno-fast-math`, `-frounding-math` | All absent | Add to root `CMakeLists.txt:29-46`. One-line fix; all four Stack C sims + common-cpp + Cat 3 driver inherit. |
| Stack C shaders | `precise` qualifier on reductions where supported | Zero usages | Defer; reductions are thread-local register sums anyway, so `precise` only constrains GLSL-side FMA. |
| Stack C reductions | Sequential tree (not atomic-add float) | No float atomics exist; u32 atomics commute | No-op for reductions; scatter-slot ordering below is the actual fix. |
| Stack C scatter | Stable-sort within cell after scatter, OR document particle-order-within-cell as out-of-scope for bitwise Cat 4 | sph-water + boids-3d affected | Per-sim Phase 11 decision. Bitwise Cat 4 only after fix lands. |
| Stack D verification | Taichi-CPU backend pinned via `--deterministic` flag at sim entrypoint | Only pinned in pytest fixtures | Add `--deterministic` CLI flag to mpm-multimaterial/main.py + lenia-fft/main.py; flips `ti.init(arch=ti.cpu)` and sets `default_fp=ti.f32` explicitly. lenia-fft also pins `fft_backend.select_backend()`. |
| Stack B verification | Dawn runtime version pinned (not just `@webgpu/types`) | Unpinned | Pin Dawn version in CI `webgpu` install; document "same-Dawn-version" as the determinism contract. |
| Thread counts | `OMP_NUM_THREADS=1` for verification | Unpinned everywhere | Add to `integrity.yml` + `build-py.yml` `env:` blocks. |

**Validation:** v2.13 / v2.14 / v2.15 land the per-stack
`--deterministic` flag plus build-flag fixes; Cat 4 bitwise-snapshot
gates on the flag's presence. **All three batches are fix + gate.**

**Cat 4 first-pilot candidates (per Probe CC § 5.4):**
**`reaction-diffusion-2d`**, **`mandelbulb-explorer`**, **`physarum`**.
Stack B WGSL stencils / no-reduction sims with no float atomics,
no scatter, no reductions; only Dawn-pin work blocks them. Stack C/D
sims need compiler-flag + scatter-order / backend-pin work first.

### 3.16 CI matrix scope

**Decision: Ubuntu 24.04 single-OS for v2.** Defensible rationale:

- Cat 4 visual regression on Stack C uses Mesa lavapipe (Linux-only).
- Cat 3 GPU-headless uses lavapipe on Linux; Dawn cross-platform is
  technically supported but adds CI surface without value to the
  single-developer workflow.
- The GPU-Sims development surface is single-developer Linux.
  Cross-platform Stack B portability is the sim's concern, not the
  toolkit's.

**Single Python version (3.11).** Pinned to match Taichi Stack D
compatibility. Multi-version matrix only opens if a contributor needs
it.

**Document in `tools/integrity/README.md`** as deliberate scope. If a
contributor on macOS / Windows wants to run the toolkit locally, the
toolkit MAY work but is not guaranteed to; CI is Linux-only.

**Reopen for v3 if:** GPU-Sims accepts external Stack B-only
contributions, or the diagnostic toolchain at `tools/diagnostics/`
ships and is expected to run on contributor laptops.

### 3.17 Batch contract — coding-readiness model

v2 execution model: this spec hands off to a single downstream
coordinator chat. That chat ingests the spec, generates Claude Code
prompts (one per parallel agent), dispatches code agents to execute
batches concurrently, and tracks landing back to coordinated merges.
For this to work without small-task clogging, **every batch in § 4
must be self-describable as a Claude Code prompt** — the batch
contract below states the minimum content per batch entry.

**Batch contract (required fields per v2.X entry):**

```
v2.X — <short title> (<N> commits)

  Purpose:        <one paragraph; why this batch exists>
  Inputs:         <repo state assumed at dispatch — usually "HEAD
                  after v2.Y" or "pre-spawn HEAD">
  Touches:
    CREATE:       <new files>
    MODIFY:       <existing files edited>
    DELETE:       <files removed>
    MOVE:         <source → destination relocations>
  Depends on:     <prior batches that must land first>
  Out of scope:   <what the batch must NOT touch — for parallel safety>
  Acceptance:     <mechanically checkable conditions>
    - Tests:      <which test files / cases must pass>
    - Findings:   <which integrity findings must appear/disappear>
    - Schema:     <output format / file existence checks>
    - Performance: <wallclock budget if relevant>
  Verification:   <what the executor runs locally before reporting
                  back to coordinator>
  Rollback:       <how to revert if downstream surfaces a break>
  Commit list:    <numbered breakdown; last commit always SHA back-fill>
```

**Field semantics:**

- **Touches**: load-bearing for parallel orchestration. Two batches
  with disjoint touch sets can dispatch concurrently. Inferred
  touches (path mentioned in prose but not declared) are flagged
  during the Probe EE matrix derivation; spec author confirms or
  corrects. Touches discovered mid-execution that weren't declared
  in the contract are coordinator-escalations.
- **Depends on**: graph edges into § 4.X dependency DAG. A batch may
  depend on multiple priors; "no depends" means dispatchable in any
  order after Phase 0.
- **Out of scope**: explicit so the executor knows what it's
  forbidden from touching even if "while you're there" temptations
  arise. Cross-batch scope discipline (Convention I) at the executor
  layer.
- **Acceptance**: this is what the executor checks before reporting
  done; it's also what the coordinator chat re-verifies on the
  merged branch. Acceptance criteria must be **mechanically
  checkable** — no "code is well-written"; yes "`pytest tests/test_X.py`
  passes" or "`python -m integrity --check cat1.bare-path` finds 0
  hits in `CHANGELOG.md`".
- **Verification** vs. **Acceptance**: acceptance is the contract;
  verification is the executor's local checklist for hitting it.
  Coordinator chat re-runs verification on merge to confirm.
- **Rollback**: usually `git revert <commit-range>`; sometimes
  requires a sequence (e.g., revert v2.0 commit 8 requires reverting
  v2.0 commits 9 & 10 first because of file dependencies). Spec
  author surfaces non-obvious revert sequences.

**Backfill plan.** Existing v2.X entries (v2.-1 through v2.55) carry
varying levels of contract detail. v2.0 through v2.4 are most
complete; later batches are one-line summaries. The pre-handoff work
in this rev includes per-batch backfill of touch sets and acceptance
criteria. Where the batch is too sparse to backfill, the per-sim Cat
3 enumeration probe (Probe GGG) or other pre-handoff probes fill in
the contract content first.

**Definition of done — batch level:**

A batch is "coding-ready" iff:
1. All seven contract fields are populated (or explicitly N/A).
2. Touch set is concrete (file paths, not "the relevant module").
3. Acceptance criteria are mechanically checkable.
4. Dependencies graph through to Phase 0 without unresolved edges.

The pre-handoff readiness audit (Probe BBB) walks all 57 batches and
flags any that fail the four conditions. Coordinator chat refuses to
dispatch a batch that fails the audit.

### 3.17.A Field defaults

When a batch entry in § 4 omits a contract field, the following
defaults apply unless the batch's prose contradicts them. These
defaults are binding on the coordinator chat at dispatch time:

- **Inputs:** "HEAD after the predecessor batch in dependency order
  landed" (i.e., the immediately-prior numbered v2.X by default;
  explicit `Depends on:` overrides).
- **Out of scope:** "any file not listed in `Touches:`."
- **Verification:** "executor runs (1) `pytest tests/` filtered to
  touched-module paths; (2) `python3 -m integrity --mode strict` and
  greps for the acceptance-criterion finding IDs; (3) `git diff
  --stat` to confirm touch set matches declared `Touches:`."
- **Rollback:** "`git revert <commit-range>` for the batch's commit
  list; for multi-file inter-dependent commits, revert in reverse
  order of landing."
- **Drives-sim:** when a batch's prose involves driving a sim from CI
  or per-sim shader/source changes, the batch implicitly carries
  **HARD `Depends on: v2.43, v2.44`** edges (per Probe EEE § F.1,
  banking row 93). `Drives-sim: yes` applies to: every Cat 3 batch
  (v2.5–v2.11), every Cat 4 per-sim adoption batch (v2.17–v2.22),
  every Phase-11 conformance batch driving a sim from CI
  (v2.45–v2.50). Other batches default to `Drives-sim: no` and incur
  no infrastructure edge. This single declaration collapses ~34
  otherwise-implicit MISSING dependency edges in § 4.X.

**Effect on coding-readiness verdicts.** Per Probe BBB § D.1
(banking row 84), applying these defaults retroactively converts the
verdict distribution from 0 READY / 0 CLOSE / 59 GAP to ≈ 12 READY /
18 CLOSE / 29 GAP without per-batch authoring. Backfill cost drops
from ~34 hours to ~6 hours of spec-author time concentrated on the
~17 batches where prose-level Touches specificity is genuinely
insufficient (per Probe BBB § D.4: v2.5 MMS harness API surface;
v2.7 8-commit touches; v2.9/v2.10 per-sim WGSL/GLSL paths; v2.16
mask config schema; v2.20–v2.22 Cat 4 per-sim sub-batches; v2.25/
v2.26/v2.27 Cat 5 check IDs + rule-doc paths; v2.32/v2.34 per-rule
doc pages; v2.44 per-stack smoke; v2.49 non-trivial rollback;
v2.53a.2/v2.53b BLOCKED-status marker).

**Status field for blocked batches.** Batches with prose-declared
blocking dependencies (currently v2.53a.2 "BLOCKED on Phase 12
setup-2 algebraic-derivation registry parser"; v2.53b "blocked until
implementations land") carry a formal `Status: BLOCKED` field so the
coordinator chat skips them in dispatch planning rather than
attempting and failing. Status returns to default (dispatchable) when
the named blocker lands.

### 3.18 Landing pipeline — start-to-finish

End-to-end workflow from spec-landed to v2-complete. Eight phases.

**Phase A — Pre-handoff (this chat session).**
- Spec authoring complete; all batches satisfy § 3.17 contract.
- Pre-handoff probes executed: II / JJ / AAA / BBB / CCC / DDD / EEE
  / FFF / GGG / HHH / III / JJJ (see § 6 probe registry).
- Probe outcomes folded back into spec.
- Spec lands at `docs/integrity-v2/toolkit-spec.md` and
  `docs/integrity-v2/audit-trail.md`.
- Hand-off brief at `docs/integrity-v2/HANDOFF_BRIEF.md` summarizes
  rev history and known LOW-confidence items.

**Phase B — Coordinator chat ingest.**
- ONE downstream claude.ai chat reads the spec (single context).
- Chat parses § 4 for batch contracts.
- Chat constructs the dispatch plan: dependency-resolved batch
  ordering, with concurrent groups identified (via Probe EE
  file-touch matrix).
- Chat reports the plan back to Steven for sign-off before any
  agent dispatch.

**Phase C — Pre-spawn snapshot.**
- For each parallel dispatch group, coordinator captures:
  - HEAD SHA at dispatch time.
  - Toolkit output snapshot:
    `python3 -m integrity --mode strict --output json > pre-spawn-G<N>.json`
  - Pytest output snapshot:
    `pytest tests/ -v --tb=no -q > pre-spawn-G<N>.pytest.log`
- Snapshots saved as the comparison baseline for that group.
- Audit report skeleton created at
  `docs/diagnostics/_audits/v2-group-<N>_dispatch_<date>.md`.

**Phase D — Parallel execution.**
- N Claude Code agents dispatched, one per batch in the group.
- Each prompt contains: batch contract verbatim, pre-spawn SHA,
  acceptance criteria as the verification step, Convention K
  re-anchor as the first step.
- Each agent:
  1. Confirms HEAD SHA matches pre-spawn SHA. Aborts if not.
  2. Creates branch `v2.X-<short>` off main.
  3. Implements the touch set per the contract.
  4. Runs verification locally (acceptance criteria).
  5. Stops when verification passes OR after 3 failed attempts;
     never silently proceeds with failing checks.

**Phase E — Reporting back to coordinator.**
- Each agent returns a structured report:
  - Touch set actually touched (vs. declared).
  - Tests added / modified / removed.
  - Acceptance criteria results (pass / partial / fail per criterion).
  - Unexpected findings (e.g., pre-existing toolkit failures in
    touched files; surface but don't fix unless in scope).
  - Branch name + last commit SHA.
- Coordinator chat aggregates reports; identifies any that need
  spec-author intervention (e.g., scope drift, premise inversion,
  unexpected dependency).

**Phase F — Landing (merge sequencing).**
- Coordinator chat sequences merges in dependency order.
- Within a parallel group (all depend only on prior groups), merge
  order is arbitrary but each subsequent merge rebases on the latest
  main before merging.
- After each merge, coordinator re-runs the toolkit on the merged
  state:
  - Expected findings disappear ✓ (per acceptance criteria)
  - No unexpected new findings ✓ (baseline-diff against pre-spawn-G<N>.json)
  - Pytest suite passes ✓

**Phase G — Post-merge audit.**
- For each landed batch, append audit report at
  `docs/diagnostics/_audits/v2.X_landing_<date>.md`.
- Required content: declared-vs-actual touch set; acceptance
  criteria results; any banked deviations; SHA range
  (`<pre-spawn-SHA>..<merged-SHA>`).
- Audit-trail § 5 absorbs any divergences as banking rows.

**Phase H — Iterate to next group.**
- Coordinator chat updates the dispatch plan: removes landed
  batches; re-evaluates downstream dependencies; identifies next
  parallel group.
- Returns to Phase C with new pre-spawn snapshot.
- Repeats until all 57 batches are landed.

**End condition (v2-complete):**
- All 57 batches landed.
- Toolkit running strict mode against the live repo is at 0
  HARD_FAIL findings (post-absorption baseline ≈ 1458, all
  grandfathered or fixed).
- All probes that were re-dispatchable executed and banked.
- HANDOFF_BRIEF updated with v2-complete summary; v3 candidate list
  surfaced from spec § 13 and audit-trail § 9 corrections queue.

**Coordinator chat discipline:**

- The coordinator chat never writes code directly. Its role is
  dispatch + sequencing + verification orchestration. Code agents
  do the work.
- The coordinator chat is single-session; if context fills up, a
  fresh session resumes with the HANDOFF_BRIEF + audit-trail as
  state.
- The coordinator chat tracks landing state in a structured ledger
  (recommended: `docs/integrity-v2/landing-ledger.md`) — one row per
  batch with dispatch-SHA, branch, merge-SHA, audit-report path.
  Append-only; mirrors audit-trail discipline.
- The coordinator chat surfaces blockers to Steven; never silently
  reschedules or invents scope.

---

## § 4 — Sequencing across batches

Numbering: `v2.N` for batch N. Total: 57 batches (Phase 0 + Phases 1-11).

**Pre-execution carry-over from rev-10 § 3.14 (per Probe QQ, banking
row 96):** **17 defects identified during rev-10 development remain
unfixed at HEAD `351c66e`.** Closeout commits 3–8 touched none of the
call sites. The "cleanup landed" framing in any inherited sequencing
prose is REFUTED — all 17 defects need real fixes (not just SHA
back-fills). Each defect maps to one of the v2.X batches below; v2.0
absorbs the cat1.bare-path subset (6 findings on `project-state.md`
per Probe WW row 61), v2.1 / v2.3 absorb the cat1/cat2 subsets;
remainder distribute across Phases 1 and 7. Phase 0 sequencing does
not assume any rev-10 cleanup landed.

### § 4.0 Explicit dependency edges (per Probe EEE, banking row 93)

The Drives-sim default in § 3.17.A.4 captures the ~34 critical-interlock
edges (any batch with `Drives-sim: yes` inherits HARD `Depends on:
v2.43, v2.44`). The 13 explicit edges below are NOT covered by that
default and must be declared:

| Downstream ← Upstream | Reason |
|---|---|
| v2.28 ← v2.0 | v2.0 commit 0 grandfather catalog cleanup is prerequisite for the "absorb baseline findings" pattern at v2.28. |
| v2.41 ← v2.0 | v2.0 commit 8 wires audit_log.py (unblocks v2.41 mutmut exclusion); v2.0 commit 9 splits integration markers (unblocks v2.41 perf budget). |
| v2.38 ← v2.31 | v2.38 pairs with dependabot config from v2.31 for SHA updates. |
| v2.42 ← v2.31 | v2.42 pins via dependabot's docker ecosystem (v2.31). |
| v2.27 ← v2.43 | v2.43 ships per-sim spec docs the `cat5.canonical-reference-cited` check (v2.27) consumes. |
| v2.53a.1 ← v2.4 | v2.53a.1 commit 1 cites the `[dimod]` registry entry; v2.4 ships the dimod vendor. |
| **v2.50 ← v2.8** | Per Probe EEE § F.3 — v2.8 ships placeholder `cat3.mpm-mms-momentum-transfer`; v2.50 renames it. (Resolution: HARD edge; v2.8 placeholder commit remains.) |
| v2.36 ← v2.5 | v2.36 property-based MMS extension requires MMS harness from v2.5. |
| v2.36 ← v2.6 | v2.36 invariant-property siblings consume invariants set A. |
| v2.36 ← v2.7 | v2.36 invariant-property siblings consume invariants set B. |
| v2.36 ← v2.8 | v2.36 invariant-property siblings consume invariants set C. |
| v2.46 ← v2.45 | v2.46 SPH solver hardening lands on same shader surface as v2.45 6-line fix; ordering required. |
| v2.14 ← v2.13 | Cat 4 sub-checks (v2.14) live inside the module skeleton (v2.13). |
| v2.16 ← v2.13 | Visual-regression goldens require LFS setup (v2.13). |

The v2.51/v2.52 → v2.6 edges declared in inherited spec prose are
relaxed to **informational** per EEE § F.4 (drops two edges, widens
v2.6 dispatch group; coordinator chat may re-tighten if parallel
constraint is binding).

The machine-readable enumeration (53 rows: 13 declared + 2 external +
38 audit-implied) is at `tools/integrity/docs/dep-graph/v2-deps.tsv`;
DOT visualization at `tools/integrity/docs/dep-graph/v2-deps.dot`.

### Phase 0: Perf-fix prerequisite (v2.-1)

- **v2.-1 — Parallelize cat2.public-symbol-used-c (2 commits).**
 ProcessPoolExecutor around `_parse_translation_units` at
 `tools/integrity/integrity/cat2_contracts/stack_c.py:506` (function
 definition (the call site inside
 `extract_and_find_references` at the same module); adds
 `--jobs N` config knob; defaults to `os.cpu_count`. **Re-anchored
 perf baseline per Probe AA (banking row 73):**
 - Pre-fix steady-state toolkit run: **103s** (not the originally
   projected ~275s; the legacy figure conflated pytest + toolkit).
 - `cat2.public-symbol-used-c` accounts for **94.92s (91.7%)** of
   that — far more concentrated than Probe J's earlier ~60% estimate.
 - Expected speedup at 4 cores: ~25s for the dominant check;
   combined steady-state toolkit run drops to **~30-35s**.
 - At 4-core parallelism the dominant check becomes comparable to
   the rest of cat2 (cat2.public-symbol-used at 3.1s); further
   parallelism shifts the bottleneck elsewhere and is not load-bearing.

 Plus SHA back-fill. Lands before v2.0 so every subsequent v2 batch
 benefits from faster CI iteration.

### Phase 1: Runner correctness + v1-banked items (v2.0–v2.4)

- **v2.0 — Catalog cleanup + runner correctness + closeout tail + TQL-5 fixes + Convention rename (12 commits).**

 **(0) Grandfather catalog cleanup pre-absorb sequence** — per Probe
 FF (banking row 82). Catalog at HEAD claims 1263 suppressed
 findings; live total is 1345 (drift +82). Auto-refresh tool exists
 (`refresh_catalog_counts.py`, commit `65a7685`) but **hard-errors on
 a missing `other` catalog heading** (4 orphan findings at
 `tools/integrity/tests/test_audit_prose_freshness.py:50,62,76,90`).
 Cleanup sequence — single commit:
 1. Resolve the `other`-category wedge — either (a) author a catalog
    `### other` H3 with the 4 orphan findings, or (b) rewrite the 4
    annotation reasons to match an existing category. Recommendation:
    (b), since `other` is the classifier's fallback bucket and
    promoting it to a first-class category creates a bad-incentive
    surface.
 2. Run `refresh_catalog_counts.py` to absorb the +82 drift across
    the 20 emitted categories. Largest absorptions: `audit-bare-path`
    747→800 (+53), `audit-report-grammar-example` 51→63 (+12),
    `toolkit-own-source` 26→32 (+6).
 3. Update the stale `project-state-snapshot` "Tracked observation"
    block (catalog lines 86–91) — the v1.3 part-C cleanup it
    describes already shipped in commit `ebbb743`.
 Post-cleanup catalog state: 1345 absorbed, no wedges, no stale
 prose. This unblocks the v2.0 / v2.28 "absorb baseline findings"
 pattern with FACT-grounded counts.

 **(1) Cat 1 annotation suffix-strip bug.** At
 `tools/integrity/integrity/cat1_citations/checks/annotation.py:81`,
 `body.rstrip("-->")` strips the character set `{ '-', '>' }` from the
 trailing edge, not the literal string `-->`. Replace with proper
 suffix strip (`if body.endswith("-->"): body = body[:-3]`).

 **(2) Exit-code convention.** Current `EXIT_BAD_CLI=64` and
 `EXIT_INTERNAL_FAIL=2` collide with the Ruff/ESLint convention of
 exit 0 = success, exit 1 = violations found, exit 2 = abnormal
 termination (invalid CLI, internal error). Adopt the convention:
 `EXIT_OK=0`, `EXIT_FAIL=1`, `EXIT_BAD_CLI=2`. Toolkit-specific
 extension: `EXIT_INTERNAL_FAIL=3` — kept distinct from BAD_CLI for
 diagnostic clarity (Ruff/ESLint lump both at 2; the toolkit
 separates them so CI can distinguish "operator error" from "toolkit
 bug"). Update tests.

 **(3) Fail-open fixes.** Unknown `--check <id>` and bad `--root
 <path>` both currently exit 0 with vacuous "1 pass" output. Both
 should exit 2 (BAD_CLI) with a clear diagnostic. The vacuous-pass
 behavior is hazardous because invocations that should report
 findings can silently report "clean".

 **(4) Fix `velocity.bin.bin` doubled-suffix on-disk filenames in
 eulerian-smoke captures.** Mechanism: `StateWriter::saveBuffer` at
 `common/common-cpp/src/state_writer.cpp:57` performs
 `current_dir_ / (name + ".bin")`. The four call sites at
 `volumetric-grid/eulerian-smoke/src/main.cpp:1439/1442/1445/1448`
 pass already-`.bin`-suffixed literals (`"velocity.bin"`,
 `"density.bin"`, etc.), so the on-disk filename and the JSON
 `desc["file"]` field (state_writer.cpp:70) both carry `.bin.bin`.
 The JSON `desc["name"]` field (state_writer.cpp:69) carries the
 caller-supplied un-doubled value. Read-back at `main.cpp:1490-1493`
 calls `cap->buffer("velocity.bin")` etc., which resolves through
 the JSON `name` field (state_reader.cpp:60) and opens via the
 JSON `file` field (state_reader.cpp:68-69) — so the bug is latent:
 the file loads, but the on-disk filename is ugly and leaks the
 writer's `.bin` convention into caller-visible paths.

 **Fix — Option A (preferred):** strip a trailing `.bin` from
 `name` at state_writer.cpp:57 if present (or stop appending `.bin`
 in the writer and document that callers supply the full filename).
 Touches one file. Preserves backward-compatibility with existing
 capture artifacts (e.g. `captures/capture_0094/`): old captures
 still load under the new reader because `desc["name"]` is the
 un-doubled value the new reader's lookup matches against, and
 `desc["file"]` continues to point at the doubled on-disk filename.

 **Alternative — Option B:** change the 8 main.cpp call-sites (4
 writer + 4 reader) to pass `"velocity"` / `"density"` /
 `"temperature"` / `"pressure"` (no `.bin`). Touches one file but
 breaks backward-compat with existing captures unless a migration
 shim is added in `bufferMeta`. Not recommended for v2.0.

 Adjacent cleanup under Option A: the `desc.value("file", name +
 ".bin")` fallback at state_reader.cpp:68 is dead code (every
 writer-emitted descriptor carries `"file"`). Either delete it or
 document the intent.

 **(5) Fix the 6 live `cat1.bare-path` findings on
 `project-state.md`.** At HEAD, `project-state.md` has 6 bare
 intra-repo citations that the cat1.bare-path check fires HARD_FAIL
 on:

 - L561 — `main.py:306-318` → rewrite to
 `continuous-ca/lenia-fft/python/lenia_fft/main.py:306-318` (or
 the appropriate candidate after context inspection).
 - L594 — `state_writer.cpp:57` → rewrite to
 `common/common-cpp/src/state_writer.cpp:57`.
 - L686 (×3) — rewrite the three context.{hpp,cpp} citations to
 the full `common/common-cpp/include/gpusims/vk/context.hpp:78`,
 `common/common-cpp/src/vk/context.cpp:116`,
 `common/common-cpp/src/vk/context.cpp:202`.
 - L741 — bare self-reference `project-state.md:559` → rewrite as
 a section anchor (e.g., `§ K.6 (project-state.md:559)`) or drop
 the self-ref entirely.

 Adjacent fix in the same commit: bare `project-state.md:561` at
 `docs/diagnostics/_audits/integrity_v1_3_closeout_commit6_landing_2026-05-17.md:31`
 (introduced by the v1.3 closeout commit 6 landing memo). Remove
 the now-inert `<!-- integrity-allow: cat1.bare-path;... -->`
 annotations preserved by that commit at lines 560/594/667-area
 of project-state.md.

 **Grammar hardening (same commit or follow-up):**
 - Regression test: `integrity-allow: cat1.intra-repo` annotations
 with no neighboring matching finding fire `SOFT_WARN` as
 "inert annotation".
 - Regression test: same-file self-references (filename matches
 containing file, e.g., `project-state.md:559` inside
 `project-state.md`) are explicitly covered by cat1.bare-path
 detection.

 **(6) Closeout tail.** One-line `sed` to replace `<COMMIT_8_SHA>`
 placeholder in `project-state.md:80` and at three locations
 (lines 21, 125, 185) of
 `docs/diagnostics/_audits/integrity_v1_3_closeout_commit7_landing_2026-05-17.md`
 with the closeout commit 8 SHA `351c66e`. One-line catalog
 section-add for the `other` category with count 4 (or rewrite the
 4 reason strings to fit `_KNOWN_CATEGORIES` substrings — simpler
 option preferred).

 **(7) Stomakhin citation polish.** Add one-line comment to
 `hybrid-particle-grid/mpm-multimaterial/python/mpm_multimaterial/kernels.py:118-119`
 citing **Stomakhin 2013 § 4.1 (10-step full method)** + **§ 5
 (constitutive model)**. The paper presents the full method as
 numbered steps in § 4.1 without an "Algorithm 1" label;
 constitutive model and Table 2 parameter values are in § 5. Add
 a note to `docs/sim-specs/mpm-multimaterial.md` § 2 documenting
 upstream parameter divergences (E=1000 vs Stomakhin canonical
 1.4e5; θ_s=4.5e-3 vs 7.5e-3 — both inherited from Taichi
 `mpm3d_ggui`; canonical values are **Stomakhin 2013 Table 2**,
 the parameter table — Table 1 is the methods-comparison table).
 Severity low; no behavior change.

 **(8) Wire `tools/integrity/integrity/common/audit_log.py` into `runner.main()`** — TQL-5
 fix per Probe BB (banking row 74). Module is at 0% line coverage
 with 0 production callers, and `--no-audit-log` is a phantom flag
 wired to dead code. Implement § 2.9.7 option (c): import
 `audit_log` from `runner.main()`, call
 `audit_log.append_findings(args.root, findings, git_head_sha(args.root))`
 under `not args.no_audit_log` when `findings` is non-empty. Add
 unit tests for `audit_log_path` + `append_findings`; add
 end-to-end test asserting the file is written with expected
 front-matter. Closes the HIGH-severity TQL-5 surface and unblocks
 audit_log.py from v2.41 mutation-testing exclusion.

 **(9) Pytest integration-test markers** — per Probe AA (banking
 row 73). Inner-loop test suite is at ~133s but 96.5% of that is
 two integration smokes. Add `@pytest.mark.integration` to
 `test_emit_state_snapshot_smoke` and `test_driver_builds_and_runs`;
 register `slow` and `integration` markers in `pyproject.toml`
 `[tool.pytest.ini_options].markers`; update
 `.github/workflows/integrity.yml` to run `pytest -m "not
 integration"` in the inner-loop step and `pytest -m integration`
 in a separate nightly step. Inner-loop suite drops to ~5s,
 unblocking v2.41 mutmut perf budget.

 **(10) Convention K → M rename sweep + bank Convention M into
 `conventions.md`.** Per Probe CCC § D.1 (banking row 84): the
 audit-trail's "Convention K — re-anchor before edit" rule collides
 with `conventions.md`'s actual Convention K (anchor-sketch labeling
 for spec content from inference). Rename audit-trail's rule to
 **Convention M** throughout: audit-trail.md § 2.1 heading +
 § 2.4 addendum heading; 7+ citation sites across spec + audit-trail
 (audit-trail.md:52→renamed, plus citation sites at audit-trail.md:84,
 324, 357, 472; toolkit-spec.md:1347, 2239, 2493). Bank Convention M
 in `tools/integrity/docs/conventions.md` with the canonical
 definition copied from audit-trail.md § 2.1. Add a one-line note in
 `conventions.md` § "Letter-collision note" recording the K↔M move.
 Acceptance: `grep -n "Convention K (re-anchor" docs/integrity-v2/ tools/integrity/docs/`
 returns 0 hits; `grep -n "Convention M" tools/integrity/docs/conventions.md`
 returns the canonical definition.

 **(11) SHA back-fill.** Per Convention #12, post-merge SHA
 back-fill is a separate follow-up commit, not `--amend`.
- **v2.1 — Cat 1 annotation polish + capture-schema gaps (5 commits).**
 Multi-line grammar + fenced-block awareness + fenced-block cleanup
 sweep + capture reader API drift fix + add `r8uint` to Python
 `_FORMAT_TO_DTYPE` + unify `find_latest` to
 Python's mtime semantics across C++ + SHA back-fill.
- **v2.2 — Cat 1 Stack A coverage (3 commits).** intra-repo extension
 + shadertoy-port-mapping check + SHA back-fill.
- **v2.3 — Cat 2 type-aware + cross-stack schema (5 commits).**
 USR-aware Stack C member access (wire up the existing scaffold at
 `stack_c.py:372-403`) + replace `cmd.split` with
 `shlex.split` + cat2.capture-schema-consistent +
 cross-stack fixture tests + document the `bytes` field
 (bank or remove from C++/TS writers) + SHA back-fill.
- **v2.4 — Cat 3 quantum seed (7 commits).** dimod vendor + 4 quantum
 checks + derivation docs + SHA back-fill.

### Phase 2: Cat 3 MMS + per-sim invariants (v2.5–v2.11)

- **v2.5 — MMS harness foundation (4 commits).** sympy-based source generator + verification module + golden manufactured solution fixtures + SHA back-fill.
  - *Touches (CREATE):* `tools/integrity/integrity/cat3_numerical/mms/__init__.py`,
    `tools/integrity/integrity/cat3_numerical/mms/source_generator.py`
    (exports `generate_source(pde_residual: sympy.Expr, u_hat: sympy.Expr) → sympy.Expr`),
    `tools/integrity/integrity/cat3_numerical/mms/verify.py`
    (exports `verify_convergence(solver_output: np.ndarray, exact: callable, dx: float, expected_order: float) → bool`),
    `tools/integrity/integrity/cat3_numerical/mms/fixtures/` (golden manufactured solutions, one per PDE family — initially heat, Burgers, advection-diffusion).
  - *Touches (MODIFY):* `tools/integrity/integrity/cat3_numerical/checks/__init__.py` (export MMS-using checks as they land in v2.6+).
  - *Acceptance:* `pytest tools/integrity/tests/cat3_numerical/test_mms_source_generator.py` passes (≥6 cases covering linear/nonlinear/coupled residuals); `pytest tools/integrity/tests/cat3_numerical/test_mms_verify.py` passes (≥4 cases at orders {1, 2, 4} ± 5% tolerance); imports `from tools.integrity.integrity.cat3_numerical.mms.source_generator import generate_source` and the symmetric `verify_convergence` import succeed.
- **v2.6 — Per-sim invariants set A + Smoke MMS (priority) (6 commits).** boids/RD-2D/strange-attractors invariants + smoke MMS (catches velocity ±1106 class bugs) + SHA back-fill.
- **v2.7 — Per-sim invariants set B + SPH pressure-coupling fix + RD MMS + LBM Poiseuille (8 commits, expanded).** Sequencing recommendation (Path A — fix first):
 (1) **SPH pressure-coupling fix.** 6 lines across 4 shaders sketch: drop the `/ρ²` from `alpha_stored` in `density_alpha.comp.glsl:174-178`; rename `alpha_over_rho2_i` → `alpha_i`; treat `p_read` as `p/ρ²` consistent with upstream Bender-Koschier 2017 Reading R1 convention. Predicted runtime: hydrostatic settling achieved; Dam-Break visibly more vigorous; possible small dt tightening needed; ≤1% perf delta. Visual verification against existing presets before landing.
 (2) **`cat3.sph-dfsph-residual` instrumentation.** avg_density_err residual is not currently materialized; add per-particle residual buffer + reduction kernel + sparse readback. ~80 LOC across compute pipeline.
 (3) **physarum / RD-3D invariants** (set B grouping).
 (4) **RD-2D / RD-3D MMS check** with Pearson canonical fixtures per § 1.3.B above.
 (5) **LBM Poiseuille MMS check** with resolution-dependent tolerance per § 1.3.B above (bounce-back wall slip scaling).
 (6) **SPH MMS pressure-Poisson check** against hydrostatic-column oracle. Iter-sweep n_iter ∈ {1,2,4,8,16,32}. Guards against regression of the v2.7-commit-1 fix.
 (7) **Add still-water canonical scene** to sph-water (currently has 4 presets; none are still-water). Needed for the hydrostatic oracle.
 (8) SHA back-fill.

  - *Touches (CREATE)* — per the 8-commit breakdown:
    `tools/integrity/integrity/cat3_numerical/checks/sph_dfsph_residual.py` (commit 2);
    `tools/integrity/integrity/cat3_numerical/checks/physarum_*.py` + `rd3d_*.py` (commit 3);
    `tools/integrity/integrity/cat3_numerical/checks/rd2d_mms.py` + `rd3d_mms.py` (commit 4);
    `tools/integrity/integrity/cat3_numerical/checks/lbm_poiseuille_mms.py` (commit 5);
    `tools/integrity/integrity/cat3_numerical/checks/sph_pressure_poisson_mms.py` (commit 6);
    `particle-fluids/sph-water/scenes/still_water.preset.json` (commit 7).
  - *Touches (MODIFY)* — per commit 1:
    `particle-fluids/sph-water/shaders/density_alpha.comp.glsl:174-178`,
    `particle-fluids/sph-water/shaders/compute_pressure_accel.comp.glsl:10` (`/ρ²` drop),
    plus 2 sibling shaders in the same dir for variable rename symmetry.
  - *Acceptance:* visual hydrostatic-settling test passes on still-water preset; `cat3.sph-dfsph-residual` emits per-particle residual buffer; all 5 new MMS checks pass convergence test at expected order ± 5% tolerance.
  - *Depends on:* v2.5 (MMS harness foundation); `Drives-sim: yes` (carries v2.43/v2.44 deps per § 3.17.A.4).
- **v2.8 — Per-sim invariants set C + SPH/MPM MMS (7 commits).** smoke/MPM/LBM/lenia/mandelbulb invariants + SPH pressure-Poisson MMS + MPM particle-grid MMS + SHA back-fill.
- **v2.9 — Cat 3 GPU headless Stack B (6 commits).** webgpu npm + Node harness + port cubic-kernel + per-sim WGSL kernels (smoke/MPM/LBM analog) + SHA back-fill.
  - *Touches (CREATE):* `tools/integrity/integrity/cat3_numerical/gpu_headless/stack_b_harness.ts`;
    `tools/integrity/integrity/cat3_numerical/gpu_headless/wgsl/cubic_kernel.wgsl`;
    `tools/integrity/integrity/cat3_numerical/gpu_headless/wgsl/smoke_diff.wgsl`;
    `tools/integrity/integrity/cat3_numerical/gpu_headless/wgsl/mpm_p2g.wgsl`;
    `tools/integrity/integrity/cat3_numerical/gpu_headless/wgsl/lbm_collide.wgsl`.
  - *Touches (MODIFY):* `tools/integrity/integrity/cat3_numerical/checks/cubic_kernel.py` (add Stack-B path); `package.json` (add @webgpu/types + webgpu runtime).
  - *Acceptance:* `node tools/integrity/integrity/cat3_numerical/gpu_headless/stack_b_harness.ts --check cubic-kernel` exits 0 with diff < 1e-6 against vendored ground-truth; CI workflow runs the harness against all 4 ported kernels.
  - *Depends on:* v2.43, v2.44 (Drives-sim); v2.7 (cubic-kernel reference values).
- **v2.10 — Cat 3 GPU headless Stack C (6 commits).** mesa-vulkan-drivers apt + C++ harness + port d3q19 checks + per-sim GLSL kernels + SHA back-fill.
  - *Touches (CREATE):* `tools/integrity/integrity/cat3_numerical/gpu_headless/stack_c_harness.cpp` + `CMakeLists.txt`;
    `tools/integrity/integrity/cat3_numerical/gpu_headless/glsl/d3q19_collide.comp.glsl`;
    `tools/integrity/integrity/cat3_numerical/gpu_headless/glsl/smoke_diffuse.comp.glsl`;
    `tools/integrity/integrity/cat3_numerical/gpu_headless/glsl/sph_density.comp.glsl`.
  - *Touches (MODIFY):* `tools/integrity/integrity/cat3_numerical/checks/d3q19_*.py` (add Stack-C paths); `.github/workflows/integrity.yml` (add `mesa-vulkan-drivers` + `VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json` per Probe HHH § B PARTIAL).
  - *Acceptance:* Stack C harness runs all 3 d3q19 checks + 2 ported per-sim kernels under lavapipe; bit-exact match against algebraic ground-truth for d3q19 trio; ≤1e-12 tolerance for sph density.
  - *Depends on:* v2.43, v2.44 (Drives-sim); v2.9 (cross-stack equivalence harness structure).
- **v2.11 — Cross-stack equivalence + differentiability foundations (4 commits).** Cross-stack harness for mandelbulb A↔B and RD-2D A↔B + gradient finite-diff harness + SHA back-fill.

### Phase 3: Cat 4 infrastructure + per-sim adoption (v2.12–v2.23)

- **v2.12 — Per-stack CLI stubs (4 commits).** Stack B (5 sims), Stack C (4 sims; includes RD-3D signature change), Stack D (2 sims) + SHA back-fill.
- **v2.13 — Stack B Playwright harness + Cat 4 module skeleton (5 commits).** Playwright + download handler + cat4_runtime module + Git LFS setup in `.gitattributes` + SHA back-fill.
- **v2.14 — Cat 4 core sub-checks (6 commits).** build-smoke + tolerance-snapshot via pytest-regtest + bitwise-snapshot + determinism-pair + replay-determinism + SHA back-fill.
- **v2.15 — Vulkan validation sub-check (3 commits).** vk-validation-clean integration with BEST_PRACTICES + SYNCHRONIZATION + GPU_ASSISTED features + parser for validation output + SHA back-fill.
- **v2.16 — Visual regression sub-check (6 commits, expanded).**
 pixelmatch + SSIM + PSNR + HTML report template + **harness-side
 per-sim mask config schema** (rect tables keyed by render dimension;
 decision — sim sources stay unchanged) +
 **canonical-frame definition for all 11 sims** (early/mid/deep
 triple) + Stack C `xwd`-by-window-id capture path
 (interim; `vkCmdCopyImageToBuffer` + `stb_image_write`
 follow-up tracked separately) + seed-pinning audit for
 physarum/boids-3d/strange-attractors + SHA
 back-fill.
  - *Touches (CREATE):* `tools/integrity/integrity/cat4_runtime/mask_config.schema.json`
    (rect-table schema); `tools/integrity/integrity/cat4_runtime/mask_configs/<sim>.json` × 11
    (one per shipping sim); `tools/integrity/integrity/cat4_runtime/canonical_frames.toml`
    (the early/mid/deep triple per sim); `tools/integrity/integrity/cat4_runtime/visual/{pixelmatch,ssim,psnr}.py`;
    `tools/integrity/integrity/cat4_runtime/visual/report.html.j2` (Jinja2 template).
  - *Touches (MODIFY):* `.github/workflows/integrity.yml` (new
    `cat4-visual.yml` per Probe HHH § E v2.16+).
  - *Acceptance:* `python3 -m integrity --cat 4 --sim <name>` produces SSIM ≥ 0.99 on canonical-frame triple for all 11 shipping sims; HTML report renders with side-by-side composite + heatmap.
  - *Depends on:* v2.43, v2.44 (Drives-sim); v2.13 (Cat 4 module skeleton); v2.14/.15 (FP-determinism + thread pins per § 3.15).
- **v2.17 — Cat 4 adoption: eulerian-smoke (5 commits).** *Requires
 v2.6 smoke MMS passing first per § 3.13 — baseline cannot be locked
 while v_max ≈ 1106 defect is live.* Canonical inputs + initial
 goldens (locked AFTER MMS-driven fix lands and holds for one CI
 cycle) + screenshots with per-sim mask config (ImGui top-left rect) + MMS-source integration + SHA back-fill.
- **v2.18 — Cat 4 adoption: sph-water (5 commits).** *Requires v2.7
 sph DFSPH residual passing first per § 3.13 — baseline cannot be
 locked while 196ms/frame banding defect is live.* Same shape +
 perf-budget tracking + ImGui mask config + SHA back-fill.
 **Budget note (per Probe FFF, banking row 90):** `particle-fluids/sph-water/src/main.cpp` is 3164 LOC, ~3× the RD-3D baseline assumed by adjacent Cat 4 adoption batches. v2.18 scope inherits the higher complexity — expect 5 commits with 100-200 LOC each rather than the 50-80 LOC shape of v2.17/v2.19/v2.20/v2.21/v2.22.
- **v2.19 — Cat 4 adoption: mpm-multimaterial (3 commits).** Extends existing `hybrid-particle-grid/mpm-multimaterial/python/tests/test_kernels.py` rather than fresh tests/integrity/.
- **v2.20 — Cat 4 portfolio set A (4 commits).** boids-3d, reaction-diffusion-2d, physarum.
  - *Sub-batch decomposition (parallelizable; disjoint touch sets):*
    Commit 1 → `agent-based/boids-3d/tests/integrity/` (canonical inputs + goldens + mask config);
    Commit 2 → `continuous-ca/reaction-diffusion-2d/tests/integrity/`;
    Commit 3 → `agent-based/physarum/tests/integrity/`;
    Commit 4 → SHA back-fill.
  - *Touches (CREATE):* `<sim>/tests/integrity/canonical_inputs.json` × 3;
    `<sim>/tests/integrity/goldens/` × 3 (PNG triples per canonical frame).
  - *Touches (MODIFY):* `tools/integrity/integrity/cat4_runtime/mask_configs/<sim>.json` × 3.
  - *Acceptance:* `python3 -m integrity --cat 4 --sim <each>` passes SSIM ≥ 0.99.
  - *Depends on:* v2.16; `Drives-sim: yes`.
- **v2.21 — Cat 4 portfolio set B (4 commits).** reaction-diffusion-3d, lattice-boltzmann, lenia-fft.
  - *Sub-batch decomposition (parallelizable; disjoint touch sets):*
    Commit 1 → `continuous-ca/reaction-diffusion-3d/tests/integrity/`;
    Commit 2 → `volumetric-grid/lattice-boltzmann/tests/integrity/`;
    Commit 3 → `continuous-ca/lenia-fft/python/tests/integrity/`;
    Commit 4 → SHA back-fill.
  - *Acceptance + Depends on:* same shape as v2.20.
- **v2.22 — Cat 4 portfolio set C (3 commits).** strange-attractors, mandelbulb-explorer.
  - *Sub-batch decomposition (parallelizable; disjoint touch sets):*
    Commit 1 → `closed-form/strange-attractors/tests/integrity/`;
    Commit 2 → `closed-form/mandelbulb-explorer/tests/integrity/`;
    Commit 3 → SHA back-fill.
  - *Acceptance + Depends on:* same shape as v2.20.
- **v2.23 — Cat 4 memory + strict-warnings (5 commits).** ASAN for Stack C + strict-warnings audits + cmake/tsconfig updates + SHA back-fill.

### Phase 4: Cat 5 cross-source consistency (v2.24–v2.27)

- **v2.24 — Audit-prose freshness promoted (2 commits).** Gate-integrate the sibling tool.
- **v2.25 — Spec-claim resolution (5 commits).** claim-extraction + claim-resolution + register + initial sweep + SHA back-fill.
  - *Touches (CREATE):* `tools/integrity/integrity/cat5_consistency/checks/spec_claim_resolution.py` (registers `cat5.spec-claim-resolution`); `tools/integrity/integrity/cat5_consistency/claim_extractor.py`; `tools/integrity/integrity/cat5_consistency/claim_register.toml`; `tools/integrity/docs/rules/cat5.spec-claim-resolution.md`.
  - *Acceptance:* `cat5.spec-claim-resolution` registers; initial sweep finds ≤ baseline claims; per-claim register accepts entries with shape `{claim_id, source_file, source_line, status}`.
- **v2.26 — Cat 5 small checks bundle 1 (6 commits).** changelog-tag-consistency + sim-spec-divergence-noted + conventions-doc-coverage + vendored-anchor-fresh + SHA back-fill.
  - *Touches (CREATE):* `tools/integrity/integrity/cat5_consistency/checks/changelog_tag_consistency.py` (`cat5.changelog-tag-consistency`); `tools/integrity/integrity/cat5_consistency/checks/sim_spec_divergence_noted.py` (`cat5.sim-spec-divergence-noted`); `tools/integrity/integrity/cat5_consistency/checks/conventions_doc_coverage.py` (`cat5.conventions-doc-coverage`); `tools/integrity/integrity/cat5_consistency/checks/vendored_anchor_fresh.py` (`cat5.vendored-anchor-fresh`); per-check rule-doc pages under `tools/integrity/docs/rules/`.
  - *Acceptance:* 4 new check IDs register; `python3 -m integrity --check cat5.<id>` exits 0 on clean repo.
- **v2.27 — Cat 5 small checks bundle 2 (5 commits).** intra-doc-anchor-resolves + cross-stack-divergence-noted + capture-format-reference-fresh + SHA back-fill.
  - *Touches (CREATE):* `tools/integrity/integrity/cat5_consistency/checks/intra_doc_anchor_resolves.py` (`cat5.intra-doc-anchor-resolves`); `tools/integrity/integrity/cat5_consistency/checks/cross_stack_divergence_noted.py` (`cat5.cross-stack-divergence-noted`); `tools/integrity/integrity/cat5_consistency/checks/capture_format_reference_fresh.py` (`cat5.capture-format-reference-fresh`); per-check rule-doc pages.
  - *Acceptance:* 3 new check IDs register and pass.

### Phase 5: Cat 6 build / CI hygiene (v2.28–v2.31)

- **v2.28 — actionlint + shellcheck + cmake-lint + python-lint unification (5 commits).** Add actionlint + cmake-lint to integrity.yml; unify ruff/mypy config; SHA back-fill.
- **v2.29 — ts-lint + eslint + prettier (3 commits).** Shared config across Stack B sims + SHA back-fill.
- **v2.30 — Workflow discipline (5 commits).** deps-pinned + cache discipline + OS pinned + lockfile-strict + SHA back-fill.
- **v2.31 — CODEOWNERS + dependabot + cmake-flags-consistent (4 commits).** Final hygiene + SHA back-fill.

### Phase 6: Toolkit-itself standards — SARIF + fingerprints (v2.32)

- **v2.32 — SARIF output + fingerprints (5 commits).** SARIF 2.1.0
 emitter as `--output sarif` mode + partialFingerprints computation
 + upload-sarif action integration in integrity.yml + workflow step
 posting to Code Scanning + SHA back-fill.
  - *Touches (CREATE):* `tools/integrity/integrity/output/sarif_emitter.py`
    (SARIF 2.1.0 spec-compliant; exports `emit_sarif(findings: list[Finding]) → dict`);
    `tools/integrity/tests/test_sarif_emitter.py` (validates against
    SARIF JSON schema).
  - *Touches (MODIFY):* `tools/integrity/integrity/runner.py:119-163`
    (add `sarif` branch to renderer alongside `human` / `json` /
    `github`); `.github/workflows/integrity.yml` (add upload-sarif
    step posting to Code Scanning).
  - *Acceptance:* `python3 -m integrity --output sarif > out.sarif` produces
    schema-valid SARIF 2.1.0; partialFingerprints stable across re-runs
    on identical findings; upload-sarif CI step succeeds.

### Phase 7: Toolkit-itself standards compliance (v2.33–v2.36)

- **v2.33 — Auto-fix safety + exit-code conventions + cat2 stack module rename (4 commits).**
 Classify the `--rewrite-stale-reasons` mode as safe; document exit-code convention 0/1/2
 in runner help; verify `2` on bad CLI args; **rename
 `tools/integrity/integrity/cat2_contracts/stack_b.py` →
 `stack_ts.py`** (per Probe III, banking row 88) to align the module
 name with its check ID (`cat2.public-symbol-used-ts`); update all
 imports + tests; SHA back-fill.
- **v2.34 — Per-rule documentation pages (5 commits).** Author
 `tools/integrity/docs/rules/<check_id>.md` per shipped check (~20+
 rules at v2.34 spec time); `INDEX.md` listing; rule-stability
 contract doc; SHA back-fill.
  - *Rule IDs enumerated* (per Probe III, banking row 88; 15
    registered at HEAD + ~5 added by v2.X batches before v2.34):
    Cat 1 (6): `cat1.intra-repo`, `cat1.shadertoy-port-mapping`,
    `cat1.bare-path`, `cat1.docstring-coverage`,
    `cat1.unregistered-upstream`, `cat1.spec-bare-prefix` (new at v2.X).
    Cat 2 (5): `cat2.public-symbol-used`, `cat2.public-symbol-used-c`,
    `cat2.public-symbol-used-ts` (renamed from `-b` at v2.33),
    `cat2.public-symbol-used-d`, `cat2.capture-schema-consistent`,
    `cat2.stub-label-stale`.
    Cat 3 (4 at HEAD + ~30 via v2.5–v2.11; per Probe GGG § A
    enumeration): `cat3.cubic-kernel`, `cat3.d3q19-velocity-set`,
    `cat3.d3q19-weights`, `cat3.d3q19-equilibrium`, plus 30+ new IDs.
    Cat 5 (~7 from v2.25–v2.27).
  - *Touches (CREATE):* `tools/integrity/docs/rules/<check_id>.md`
    × ~46 files; `tools/integrity/docs/rules/INDEX.md` (lists all
    rules with columns: check_id, category, mode, sim coverage,
    stability tier);
    `tools/integrity/docs/rules/_stability-contract.md` (defines
    stability tiers: stable / experimental / deprecated).
  - *INDEX.md schema:* `| Check ID | Category | Mode | Sims | Stability | Doc |`
    one row per check; sorted alphabetically; auto-generated by
    `tools/integrity/scripts/generate_rules_index.py` (CREATE in
    same commit).
  - *Acceptance:* `tools/integrity/docs/rules/INDEX.md` enumerates every registered check
    ID; `python3 tools/integrity/scripts/generate_rules_index.py --check`
    confirms INDEX matches registered checks; every check has a doc page
    with sections `## What` / `## Why` / `## Examples`.
- **v2.35 — Doctest-based Cat 5 enhancement (3 commits).** New
 `cat5.doctest-spec` sub-check; per-Python-spec-doc doctest harness
 via `python -m doctest`; SHA back-fill.
- **v2.36 — Property-based testing for Cat 3 via Hypothesis (4
 commits).** Hypothesis dep + per-invariant property-based variants
 (each `cat3.<sim>-<invariant>` gets a `-property` sibling) + MMS
 parameterized-family extension (random manufactured solutions) +
 SHA back-fill.

### Phase 8: Cat 7 security and dependency hygiene (v2.37–v2.39)

- **v2.37 — Dependency vulnerability scanning (4 commits).**
 cat7.pip-audit + cat7.npm-audit integration in integrity.yml +
 initial baseline triage (probably surfaces some advisories needing
 attention or grandfather suppression) + SHA back-fill.
- **v2.38 — Action SHA pinning + secrets scan (4 commits).**
 cat7.action-sha-pinned migration (replace floating tags with SHA
 pins across all workflows; pair with dependabot config from v2.31
 for SHA updates) + cat7.no-secrets-in-code (gitleaks) baseline
 scan + SHA back-fill.
- **v2.39 — License compatibility + OpenSSF Scorecard (4 commits).**
 cat7.license-compatible verification (vendored deps + transitive
 Python/Node licenses) + cat7.openssf-scorecard scheduled workflow
 + threshold tuning to current baseline + SHA back-fill.

### Phase 9: Toolkit self-application closure + v2-closed marker (v2.40)

- **v2.40 — Self-application closure + v2-closed marker (3 commits).**
 Extend cat2.public-symbol-used-toolkit if probe shows gaps;
 project-state.md v2-closed marker; tools/integrity/README.md Status
 update; Conventions A-K refresh if new conventions emerged during
 v2 retros; SHA back-fill.

### Phase 10: Toolkit-itself integrity enhancements (v2.41–v2.42)

- **v2.41 — Mutation testing for toolkit (4 commits).** Add mutmut or
 cosmic-ray as toolkit dev dependency; configure `mutmut.conf` or
 `cosmic-ray.toml` targeting `tools/integrity/integrity/` modules;
 baseline mutation score and ratchet (initial target: 80% kill rate,
 industry-standard for SAST tooling); document handling of equivalent
 mutants; SHA back-fill. **Addresses 's finding:** 80% line
 coverage tells us tests *exercise* the code, not that they *catch*
 bugs. Mutation score is the trust-but-verify layer. Especially
 relevant for `audit_log.py` (showed it's load-bearing but
 exercised only indirectly).
- **v2.42 — Hermetic CI Level 3 (5 commits).** Pin Ubuntu base image
 by SHA digest in integrity.yml (currently `ubuntu-latest` floating
 tag); pre-build Docker image with `vulkan-validationlayers`,
 `libclang-dev`, `mesa-vulkan-drivers` etc. baked in; switch
 integrity job to `runs-on: ubuntu-latest, container: ghcr.io/<repo>/integrity-runner@sha256:...`;
 pin via dependabot's docker ecosystem; document the
 hermetic-builds-Level-1-5 ladder placement (we move from Level 2
 → Level 3); SHA back-fill. **Addresses 's finding:** apt-get
 install variance 13s→176s tail. Pre-baked image kills that variance
 entirely. Industry term: "Level 3 — Mostly hermetic" (locked deps,
 containerized, no network during build).

### § 4.X — Phase 11 dependency graph

Per rev 6's flagged concern: the sequencing between Phase 11 sim-
conformance batches and the Cat 3 invariant batches needs explicit
formalization. Rule: sim-conformance commits land *before* the Cat 3
checks that depend on the canonical convention, so the check guards
against regression rather than diagnosing the existing bug.

```
INFRASTRUCTURE PREREQUISITES (must land first)
═══════════════════════════════════════════════
v2.43 [sim-spec docs baseline] ──┐
v2.44 [all-sims headless mode] ──┤
 │
 ▼
SPH CONFORMANCE BLOCK
═════════════════════
v2.45 [SPH pressure-coupling fix] ────────→ v2.7 commit 6
 [cat3.sph-mms-pressure-poisson]
v2.46 [SPH solver hardening] ────────────→ v2.7 commit 2
 [cat3.sph-dfsph-residual]
 v2.7 commit 7
 [still-water scene]

LBM CONFORMANCE BLOCK
═════════════════════
v2.47 [LBM BB fix-or-document] ──────────→ v2.7 commit 5
 [cat3.lbm-mms-poiseuille-flow]

RD CONFORMANCE BLOCK
════════════════════
v2.48 [RD stencil docs + noise fix] ─────→ v2.7 commit 4
 [cat3.rd-stationary-pearson]

MPM CONFORMANCE BLOCK
═════════════════════
v2.49 [MPM substep refactor] ────────────→ v2.50 [conservation invariants]
 (4 separate cat3 checks)

SMOKE CONFORMANCE BLOCK (mostly no-op)
═════════════════════════════════════════════════════
v2.51 [advection method docs only] ──────→ v2.6 [smoke MMS]
v2.52 [Euler-claim docstring polish] ────→ v2.6 [smoke MMS]

NEW SIM BLOCK (rev 7 additions; split per rev 8 + rev 9)
══════════════════════════════════════════════════════════
v2.43 ──→ v2.53a.1 [ising-dwave Step-4a Cat 3: [dimod] registry
 + cat3.qubo-ising-roundtrip — unblocked, can
 land before sim]
[Phase 12 setup-2 algebraic-derivation parser] ──→ v2.53a.2
 [ising-dwave Step-4b Cat 3: cat3.ising-energy
 + cat3.wolff-bond-probability + cat3.onsager-tc]
[neural-ca + pic-flip implementations land — out of v2 scope] ──→ v2.53b
 [neural-ca + pic-flip Cat 3 spec authoring]
v2.54 [Lenia upstream attribution] (independent — registry-only change)
v2.2 ──→ v2.55 [shadertoy fragment-level attribution
 — iq's polynomial colormap in strange-attractors]
```

**Critical interlock:** v2.43 (sim-spec docs) AND v2.44 (headless mode)
are prerequisite to almost every other Phase 11 batch and every Cat 3
+ Cat 4 batch that needs to drive a sim from CI. Both should land
*before* v2.5 (MMS harness foundation). Without v2.44 specifically,
v2.5 cannot exercise any of the 14 sims headlessly — meaning the v2.5
prototype work is constrained to CPU-only sympy generation, with the
GPU-side validation deferred to post-v2.44.

**v2.49 decision (Option A confirmed).** Vulkan re-bench
on RX 6800 XT / RADV NAVI21 showed +5.39% mean slowdown (3 invocations
× 5 trials; 's worst-case 8-15% projection did not materialize
— radv's command-buffer batching amortizes per-launch overhead).
**Item 4 (boundary idempotence) is required** for v2.50 conservation
invariants because none of the four invariants (mass / linear momentum
/ angular momentum / energy non-increase) robustly detects BOUND-cell
off-by-one bugs — mass is insensitive, momentum has defensive
tolerance bands, energy non-increase allows arbitrary loss. **Item 7
(CFL self-check) is nice-to-have** — energy non-increase catches
dt-too-large mechanically; item 7 adds diagnostic clarity only.
**v2.49 ships Option A** (4-phase kernel split + thin orchestration
wrapper), accepting ~5% Vulkan perf cost. The Option-B-with-cat2-
mirror-gate-and-@ti.func-factor contingency from is
dropped — both halves of its branch are closed.

**Within-batch ordering:** v2.7 LBM Poiseuille check (commit 5)
needs τ pinning at 0.6 — the spec docstring of the check
must record this pin, otherwise the Poiseuille profile depends on the
default-τ choice at check-run time. Similar pin recording for any
other τ-dependent / parameter-dependent canonical-reference choices.

---

### Phase 11: Sim conformance (v2.43–v2.55)

Per § 1.0 canonical-reference principle: bring sims into line with
their canonical references where divergences are not defensible.
These are sim-source changes informed by Cat 3 invariant authoring.
Each batch should land its conformance fix *before* the Cat 3 check
that depends on the canonical convention, so the check guards
against regression rather than diagnosing the existing bug.

- **v2.43 — Per-sim spec doc baseline (3 commits).** Create or extend
 `docs/sim-specs/<sim>.md` for all 11 sims with:
 - Canonical reference declaration (paper / textbook citation)
 - Documented defensible divergences (RD stencil normalization
 convention, LBM collision-scheme choice, smoke Euler-not-NS, etc.)
 - Scheduled-fix divergences with v2 batch numbers
 Required for the `cat5.canonical-reference-cited` check at v2.27.
 Findings from Probes V, BB, Z, AA feed this batch.

- **v2.44 — All-sims headless capture mode (8 commits).** no sim parses `--headless --steps N --out PATH` at HEAD. Land the minimum cross-sim CLI uniformly across **14 sims** (11 shipping + 3 stubs). **Total budget ~820 LOC arithmetic:** baseline ~520 LOC for 11 shipping sims (155 Stack B + 255 Stack C + 70 Stack D + 40 shared Playwright runner) + ~300 LOC for pic-flip (the only new sim with implementation work; neural-ca and ising-dwave are stubs without implementations to make headless). v2.44 lands the CLI framework; future v2.* batches absorb headless mode as the 3 stub-sims gain implementations.
  - *Touches (MODIFY)* — per-sim entrypoints (paths verified by Probe FFF):
    Stack B (5 sims): `<sim>/web/main.ts` × 5 (URL query parser +
    headless frame loop); Stack C (4 sims): `<sim>/src/main.cpp` × 4
    (`argc`/`argv` + `--headless` gate on `Window`; **RD-3D requires
    `int main` → `int main(int argc, char** argv)` signature change +
    constexpr→runtime promotion for `dt`/grid, +5 LOC**; **sph-water
    has non-default `ContextCreateInfo` for subgroup_size_control,
    +10 LOC**); Stack D (2 sims): `<sim>/main.py` × 2 (`argparse` +
    `ti.ui.Window` headless gate).
  - *Touches (CREATE):* `tools/integrity/runners/playwright_capture.py`
    (shared Stack B runner; ~40 LOC); `<sim>/tests/integrity/smoke_headless.sh`
    × 11 (one per shipping sim).
  - *Acceptance (per BBB § D.4 — per-stack headless smoke):* For each
    of 11 shipping sims, `bash <sim>/tests/integrity/smoke_headless.sh`
    runs to completion, produces 1 capture artifact at `--out PATH`,
    and exits 0. CI runs the 11 smokes in matrix.
  - *Depends on:* v2.43 (per-sim spec docs); `Drives-sim: yes` implicit
    via § 3.17.A.4.
 - **5 Stack B sims** (boids-3d, physarum, lenia-fft, reaction-diffusion-2d, mandelbulb-explorer; also strange-attractors and neural-ca if Stack B): URL query string parser → render-loop frame counter → Playwright download handler. ~155 LOC + shared Playwright runner ~40 LOC. **Commits 1-2.**
 - **4 Stack C sims** (eulerian-smoke, lattice-boltzmann, reaction-diffusion-3d, sph-water): argparse + `--headless` gate on `Window` creation + `if (frame >= max_frames) save_capture; break;` guard. **RD-3D needs `int main` → `(int argc, char** argv)` signature change + constexpr→runtime promotion for dt/grid (+5 LOC). sph-water has non-default ContextCreateInfo for subgroup_size_control (+10 LOC for headless verification).** ~255 LOC. **Commits 3-4.**
 - **1 Stack D sim** (mpm-multimaterial — "argparse exists" hypothesis disconfirmed by; physarum is Stack B row 6): argparse + `if not args.headless: ti.ui.Window(...)` + frame counter. ~70 LOC. **Commit 5.**
 - **pic-flip only** (neural-ca and ising-dwave have no implementations to make headless yet; + rev 9 § 0.9 #19): per-stack template applies (Stack C shape — shares with eulerian-smoke + LBM). ~300 LOC. **Commit 6.**
 - **Shared Playwright runner** at `tools/integrity/runners/playwright_capture.py` (or similar). ~40 LOC. **Commit 7.**
 - **Commit 8: SHA back-fill.**

 Uniform CLI per § 1.0.B principle: `--headless --steps N --out PATH --seed N` across all 14 sims. Stack B reads from URL query string and translates via the shared Playwright runner. Addresses every Cat 3 + Cat 4 capture workflow blocked by (all 6 RD fixture cells were BLOCKED).

- **v2.45 — sph-water pressure-coupling fix (3 commits).** Per Probe
 P/Q Path A. 6-line shader edit across 4 shaders: drop the `/ρ²`
 from `alpha_stored` in `density_alpha.comp.glsl`; rename
 `alpha_over_rho2_i` → `alpha_i`; treat `p_read` as `p/ρ²`
 consistent with BK17 Reading R1. Visual verification against
 4 existing presets before landing. Predicted runtime change:
 hydrostatic settling achieved; Dam-Break visibly more vigorous;
 possible small dt tightening; ≤1% perf delta. Lands *before*
 v2.7 (cat3.sph-mms-pressure-poisson) so the check guards against
 regression. SHA back-fill.

- **v2.46 — sph-water solver hardening (4 commits).** (1) add convergence check to Jacobi iterations (currently fixed
 1-2; BK17 prescribes iter-until-converged); (2) add particle-
 deficiency clamp for numNeighbors<20 (per BK17 § 4.2; currently
 missing despite shader comment claiming it); (3) add still-water
 canonical scene (— no current preset is hydrostatic-
 suitable); (4) SHA back-fill.

- **v2.47 — LBM bounce-back conformance OR documented-defense
 (3 commits, decision-clarified).** shifted-variant BB finding + identification of SRT/BGK
 (τ default 0.6). Two paths:
 - *Path A (fix):* Refactor halfway-BB to read load_f(i, cell)
 *before* pull-stream, matching Krüger D2Q9 convention extended
 algebraically to D3Q19 per `tools/integrity/docs/algebraic/d3q19.md`.
 Tolerance for v2.7 commit 5 becomes O(1/N²) per He 1997 / Zou-He
 1997 analytic results for canonical halfway-BB + BGK.
 - *Path B (document):* Keep "shifted variant" as defensible
 divergence with rationale in `docs/sim-specs/lattice-boltzmann.md`;
 `cat3.lbm-mms-poiseuille-flow` tolerance is O(1/N) with explicit
 cite to spec doc's divergence section.

 Per § 1.0: Path A unless there's a defensible reason for the shift.
 No defensible reason has been surfaced; the shifted variant
 appears to be an unintentional read-order bug rather than a
 deliberate choice. **Default recommendation: Path A.** 
 result (SRT/BGK + τ default 0.6 + halfway-BB intended at proper
 wall position) is consistent with canonical Krüger convention,
 reinforcing that Path A is the intent.

 v2.7 commit 5 (cat3.lbm-mms-poiseuille-flow) regardless of A/B:
 pin τ = 0.6 in the check (otherwise the Poiseuille profile depends
 on τ in a known way: `u(y) = (g·h²/(8ν))(1 − 4y²/h²)` + τ-dependent
 slip term per He 1997). Reference profile must be BGK-corrected,
 not continuous-limit parabolic.

- **v2.48 — RD stencil convention documented (2 commits).** Per
 : our shaders use unnormalized Laplacian (no 1/dx²).
 Defensible divergence — document in `docs/sim-specs/reaction-
 diffusion-2d.md` and `docs/sim-specs/reaction-diffusion-3d.md`
 with the conversion formula relating our (Du, Dv, F, k) to
 canonical Pearson 1993 coordinates. Also fix the noise-applied-
 globally divergence to match Pearson's
 perturbation-region-local noise — that one is a bug, not a
 defensible choice. SHA back-fill.

- **v2.49 — MPM substep refactor: Option A (4 commits, confirmed by).** Replace `substep` with 4 phase kernels (`p2g_phase`, `grid_update_phase`, `gravity_and_boundary_phase`, `g2p_phase`) + Python wrapper `substep_split`. +52 LOC, +5.39% Vulkan slowdown on RX 6800 XT (3 invocations × 5 trials each, mean ± 2.5% noise floor). **Item 4 (boundary idempotence) is REQUIRED** for v2.50 conservation invariants — none of the 4 invariants (mass / linear momentum / angular momentum / energy non-increase) robustly detects BOUND-cell off-by-one bugs because mass is insensitive to BOUND indexing, momentum tests have defensive tolerance bands that absorb the bug, and energy non-increase allows arbitrary loss. **Item 7 (CFL self-check)** is nice-to-have; energy non-increase catches dt-too-large mechanically. **Option B + cat2 mirror gate contingency is dropped** — both halves of that branch are closed (items 4/7 required → Option A forced regardless of perf gate). hidden recommendation (@ti.func factor) is therefore moot. Commits:
 - **Commit 1:** 4-phase kernel split + `substep_split` wrapper. Production `substep` becomes thin orchestration.
 - **Commit 2:** Update tests/test_kernels.py to invoke the wrapper or phase kernels directly. Existing NaN/Inf check preserved.
 - **Commit 3:** v2.49 changelog note documenting ~5% Vulkan perf cost + measured device (RX 6800 XT / RADV NAVI21) for future re-benchmarks.
 - **Commit 4:** SHA back-fill.

  - *Rollback (non-trivial — overrides § 3.17.A default per BBB § D.4):*
    Commits 1–3 form a load-bearing chain where commit 2's test edits
    depend on commit 1's kernel split. Reverting in `git revert` order
    (newest first: 3 → 2 → 1) is safe; reverting commit 1 alone leaves
    tests broken against missing `substep_split` symbol. Partial-revert
    options:
    - Revert 3 only: production safe; perf delta documentation lost. Acceptable.
    - Revert 3 + 2: production safe; tests reference removed symbol. Re-add stub `substep_split = substep` to restore test compile.
    - Revert 3 + 2 + 1: clean rollback to pre-v2.49 state. Preferred if v2.50 reveals an invariant the refactor breaks.

 **Risk note:** item 4's idempotence test must
 account for Phase 3's gravity delta. Naïve "Phase 3 applied twice
 equals once" is bit-unequal because each Phase 3 adds `dt × g`.
 Correct formulation in v2.50 spec: save `grid_v` after first
 Phase 3 + run Phase 3 again with `g_y = 0` (and any other external
 forces zeroed) + assert bit-equality against saved state. OR
 equivalently: track delta = (Phase-3-twice − Phase-3-once) and
 assert delta == `dt × g` element-wise.

- **v2.50 — MPM conservation invariants (4 commits).** After v2.49.
 `cat3.mpm-mass-conservation`, `cat3.mpm-linear-momentum-conservation`,
 `cat3.mpm-angular-momentum-conservation`,
 `cat3.mpm-energy-non-increase`. Renames the v2.8 placeholder
 `cat3.mpm-mms-momentum-transfer` — classical MMS doesn't fit
 Lagrangian MPM Conservation laws are the right
 oracle. gravity-impulse check is commit 1.

- **v2.51 — Smoke advection method documentation (1 commit, no-op).** our smoke uses single-pass MacCormack-corrected SL (Selle/Fedkiw 2008) with reverse-Stam corner-clamp limiter — NOT vanilla SL. Rev 6's "if vanilla SL: upgrade" branch does not fire. v2.51 reduced to:
 - Add to `docs/sim-specs/eulerian-smoke.md` the explicit citation: "Advection uses single-pass MacCormack-corrected semi-Lagrangian (Selle/Fedkiw 2008) with reverse-Stam corner-clamp limiter at extrema."
 - Document the implication for v2.6 cat3.smoke-mms-advection-order: pass-threshold ≥ 1.7, fail < 1.5 (corner-clamp can degrade order to ~1 near interior extrema, so the MMS source field should be chosen monotone in the advected quantity)

- **v2.52 — Smoke Euler-claim docstring polish (1 commit, no-op).** smoke spec already claims "inviscid incompressible Euler equations" unambiguously and consistently across three direct statements. Implementation agrees. No reconciliation needed. v2.52 reduced to:
 - Add the Euler-plus feature inventory to `docs/sim-specs/eulerian-smoke.md` § 1.0.B-style canonical-reference block: vorticity confinement (Fedkiw 2001 eq 14), Boussinesq buoyancy + density-downforce, scalar exponential decay
 - Cross-reference v1.1 banked features (MGPCG, moving obstacles, free-slip walls) so they don't accidentally land as undocumented features

- **v2.53a.1 — ising-dwave Step-4a Cat 3 spec authoring (3 commits, rev 9 split).** Unblocked at HEAD; can land immediately. - **Commit 1: [dimod] registry entry** — vendored convention-pinning reference for QUBO↔Ising mapping. dimod is the D-Wave Ocean SDK conversion library; the bijection `s = 2x − 1` (Ising spin from QUBO binary) needs a canonical implementation cited for the Cat 3 check to anchor against.
 - **Commit 2: `cat3.qubo-ising-roundtrip` spec** — verifies the bijection algebraically: for canonical (binary, coupling) inputs, `Ising(QUBO(x, Q)) = (x, Q)` exactly modulo the affine constant. Per quantum.md:255: covers host-side uniform-passing bugs (the actual bug class), not just identity of pure math.
 - **Commit 3:** SHA back-fill.

- **v2.53a.2 — ising-dwave Step-4b Cat 3 spec authoring (4 commits, BLOCKED on Phase 12 setup-2 algebraic-derivation registry parser).** **Status: BLOCKED** — coordinator chat skips this batch in dispatch planning per § 3.17.A field-defaults note; re-evaluate when the algebraic-derivation registry parser lands in Phase 12 setup-2. The three algebraic-derivation checks below require that parser before their specs can be authored.
 - **Commit 1: `cat3.ising-energy` spec** — *pure-function evaluation* on canonical configurations (all-aligned, alternating, fixed-seed random). Verifies the Hamiltonian function `H(s) = -Σ_{<i,j>} J_{ij} s_i s_j - Σ_i h_i s_i` on inputs with known closed-form energy. NOT time-conservation across substeps (that's Cat 4 statistical). Registry entry: [Algebraic_Ising2D].
 - **Commit 2: `cat3.wolff-bond-probability` spec** — *algebraic point-evaluation* of `p(β, J) = 1 − exp(−2βJ)` at canonical (β, J) points. Covers host-side uniform-passing bugs per quantum.md:255 (the actual bug class). NOT χ² sampling statistics (that's Cat 4). Registry entry: [Algebraic_WolffCluster].
 - **Commit 3: `cat3.onsager-tc` spec** — *single-value sanity check* that the sim's reported βc matches Onsager 1944 canonical `βc = ln(1+√2)/2 ≈ 0.4407` to 4 decimal places. Catches `J/k_B ↔ β` unit confusion (the actual bug class). NOT heat-capacity-peak recovery from β sweep (that would be Cat 4 per quantum.md:305).
 - **Commit 4:** SHA back-fill.

 Decision items (rev 9 defers both per probe recommendation):
 - Layer-(c) 5th check candidate: defer pending sim implementation
 - Embedding-correctness 6th check: defer (out of Cat 3 scope per
 quantum.md:228 line (d) — QPU-touching checks live in regular
 test suite, not integrity toolkit)

- **v2.53b — neural-ca + pic-flip Cat 3 spec authoring (4 commits, blocked until implementations land).** **Status: BLOCKED** — coordinator chat skips this batch in dispatch planning per § 3.17.A field-defaults note; re-evaluate when neural-ca and pic-flip leave README-only stub status. Both sims are README-only stubs at HEAD per Probe FFF. Cat 3 candidates-D.2:
 - **neural-ca:** morphogenesis stability after training (golden comparison after long-time run from seed — Cat-4-like more than Cat-3-like)
 - **pic-flip:** mass conservation across PIC→grid + grid→particle transfer; FLIP/PIC blending preserves total momentum to known tolerance
 This batch is BLOCKED-pending-implementation. Spec drafting can start once shader/kernel code exists.

- **v2.54 — Lenia upstream attribution (1 commit, rev 7 addition).** (banked): Chakazul/Lenia reference impl is cited in lenia-fft sources but not in the vendoring registry at `tools/integrity/docs/ground-truth-sources.md`. Add to the registry as an upstream attribution; no code lifting (per § E.7 vendoring policy — 9 of 11 (now 12 of 14) sims have canonicals NOT in the registry by design).

- **v2.55 — cat1.shadertoy-port-mapping extended for fragment-level deps (2 commits, rev 7 addition).** Depends on v2.2. strange-attractors borrows iq's polynomial colormap from Shadertoy `view/WlfXRN` as a fragment-level dependency. The v2.2 cat1.shadertoy-port-mapping check needs to handle "fragment-level attribution" as a sub-case (not the full-sim-port case). Spec update + check extension + test fixture. SHA back-fill.

### Total scope

**57 batches (rev 10, unchanged from rev 9), ~241 substantive commits
+ 57 SHA back-fill commits + ~297 audit reports** (adds one;
Probes MM/NN/OO/PP/QQ/RR add six more for rev 11 → ~304 audits as of
rev 11). Estimated 9-12 months of focused work at 1 batch per ~3-5
days. Per Steven's direction: correctness over urgency.

**Footnote on "57 batches"**: the canonical count is 57. Strict enumeration of unique
batch IDs in § 4 yields **59** distinct tokens (Phase 0: v2.-1;
Phase 1: v2.0-v2.4; Phase 2: v2.5-v2.11; Phase 3: v2.12-v2.23; Phase
4: v2.24-v2.27; Phase 5: v2.28-v2.31; Phase 6: v2.32; Phase 7:
v2.33-v2.36; Phase 8: v2.37-v2.39; Phase 9: v2.40; Phase 10:
v2.41-v2.42; Phase 11: v2.43-v2.55 including the v2.53a.1/a.2/b
split = 15). The +2 discrepancy is a legacy arithmetic carry from
the rev 7→rev 8→rev 9 v2.53 splits: v2.53 split into v2.53a + v2.53b
in rev 8, then v2.53a further split into v2.53a.1 + v2.53a.2 in
rev 9, with the parent counters not all retired in lockstep. Per
a and, the count "57" is rev-to-rev
internally consistent and load-bearing for cross-rev comparison; the
absolute enumeration is 59. **Treat "57 batches" as the canonical
defensible count; do not re-derive from § 4 enumeration without
applying this footnote.**

Rev 11 changes (append-zero, sweep-only):
- 14 single-line text edits sweeping Probes MM/NN/OO/PP/QQ/SS findings
 into live prose (anchor drifts at stack_c.py:578→506 and
 workflow-count 0/21→0/17; v2.0 commit 4 mechanism reframing for
 velocity.bin.bin; v2.0 commit 5 reconciliation since cat1.bare-path
 shows 0 HARD_FAILs at HEAD; v2.0 commit 6 path drift for
 commit7_landing; § 1.0.B rows 4/6/11/12/13 citation hygiene;
 Stomakhin Table 1→Table 2 and § 5→§ 4.1/§ 5 corrections; § 0.8
 stale 80-130/14-18 phase projection replaced with current
 ~241+57/12 phases; § 1.0.B row 14 + § 8 Refs Harris 2018 gloss
 re-disposition Option D1)
- 9 new banked corrections (§ 0.9 items 42-50); row 50 lands as
 REFUTED-AND-RESOLVED within the rev 11 cycle (resolved
 the quantum.md Harris-King fabrication; King 2022 + King 2024/2025
 spot-verified clean in the same cycle)
- 2 new discipline sub-sections (§ 0.7.B PP-pass; § 0.7.C
 canonical-reference-doc-vs-source audit) — is the first
 execution of § 0.7.C
- Zero new design content, zero new analysis, zero new batches
- Total scope unchanged at 57 batches (rev 11 changes are sweep-class)

Rev 10 changes (preserved for lineage):
- 10 single-line text edits closing 's findings catalog
 (items 32-41 banked into § 0.9)
- Zero new content, zero new probes, zero new sections
- Total scope unchanged at 57 batches (item 34 corrects stale "42"
 reference; the actual count was already 57 elsewhere)
- Discipline addition: grep-against-claim verification pass before
 submission,

Rev 9 changes (preserved for lineage):
- v2.53a split into v2.53a.1 (unblocked at HEAD, 3 commits — dimod
 registry + qubo-ising-roundtrip + SHA back-fill) and v2.53a.2
 (Phase-12-blocked, 4 commits — algebraic-derivation specs gated
 on Phase 12 setup-2 parser landing) — net +1 batch
- v2.0 header commit count corrected 7 → 8 (header/body
 reconciliation; Stomakhin polish addition from rev 8 was not
 reflected in the header)
- v2.44 LOC budget corrected ~700 → ~820 LOC (arithmetic reconciliation)
- v2.49 "3-5 commits, decision-pending" → "4 commits, Option A
 confirmed"
- § 1.3.B sweep: RD / LBM / SPH / MPM / smoke-advection paragraphs
 rewritten to reflect Probes T+AA / S+W / P+Q / U+EE / X findings
 in place rather than appending banked corrections
- § 4.X dep-graph: Probe-EE-blocked conditional clauses replaced
 with confirmed Option A decision
- v2.53a/b spec descriptions rewritten reframings
- Total scope expanded from 56 → 57 batches

Independent batches (v2.0–v2.4 mostly independent in file surface;
v2.5–v2.8 share Cat 3 infra and compose; v2.9–v2.10 independent of
each other; v2.12–v2.23 share Cat 4 infra and compose in order;
v2.24–v2.27 mostly independent; v2.28–v2.31 mostly independent;
v2.32–v2.36 build on each other (SARIF foundation, then
documentation, then property-testing); v2.37–v2.39 mostly independent
of each other) can be reordered based on Steven's priorities.

---

## § 5 — Open questions for Steven

Six items where the design depends on a Steven decision. Three have
been answered by Probes F/L/M (kept here for lineage with the answer
folded in); three remain genuinely open.

### 5.1 Q1 — MMS sim-modification approach

MMS requires temporarily modifying each PDE-based sim to accept an external source term `S(x,t)`. Two implementation patterns:

- **Option A: Runtime CLI flag** (`--mms-source <fixture.npy>`). Sim loads a precomputed source-term array, adds to PDE residual each frame.
- **Option B: Build-time flag** (`-DMMS_VERIFICATION=ON`). Source term injected via preprocessor; production builds compile it out.

Option A is more flexible (one binary, many manufactured solutions); Option B has zero production cost but requires per-sim CMake plumbing. Recommendation: **Option A.** The flag is opt-in; production runs without `--mms-source` are unchanged. ** the smoke injection slot at `main.cpp:1946` accepts Option A cleanly without restructuring the existing 11-stage pipeline.

### 5.2 Q2 — Cat 4 perf budget enforcement

 surfaced sph-water at 196ms/frame on 1M particles — far slower than expected for DFSPH on a 6800 XT. This is a perf bug, not a correctness bug.

- **Option A: Cat 4 perf sub-check as soft-warn.** `cat4.perf-budget` records frame time, soft-warns if > 1.5× expected, hard-fails if > 5× expected.
- **Option B: Bank perf as out-of-scope.** v2 focuses on correctness; perf gets its own phase.

Soft-warn (Option A) catches regressions cheaply and surfaces sph-water's current issue immediately. *Still open.*

### 5.3 Q3 — Visual regression baseline strategy

Previous framing: lock current state vs. wait for known-good.
** Cat 3 invariants must pass
*first* for sims with known visual defects (eulerian-smoke,
sph-water), *then* Cat 4 baselines lock. Pairing Cat 3 (catch current)
with Cat 4 (catch future) is still the model — but the *ordering*
matters because locking a baseline on a broken sim encodes the bug
as reference. Codified as § 3.13 (baseline-triage-before-lock). Other
sims without known defects can baseline immediately after Cat 4
infrastructure lands (v2.16). *Resolved.*

### 5.4 Q4 — Cat 4 ordering: smoke vs sph-water first

's MMS-foundation analysis settles this: **smoke first** is
correct, and identified the BC contamination issue early
enough that the stream-function manufactured solution is locked
in design before v2.6 spec authoring. Cat 3 smoke MMS at v2.6; Cat 4
smoke adoption at v2.17 with the baseline-triage interlock from
§ 3.13. *Resolved.*

### 5.5 Q5 — Cat 6 strictness — current vs aspirational?

Cat 6 checks (e.g., `cat6.deps-pinned`) will surface findings against current state. Soft-warn first, escalate to hard-fail after sweep? Or hard-fail with grandfather-catalog suppression for current state?

Recommendation: **hard-fail with grandfather sweep**, mirroring v1.0 launch pattern. Live-source-stays-red discipline says grandfather-catalog the current state, attribute, and live-source findings stay red until fixed. ** the only hard-fails would currently surface (3 permissions-missing workflows + 21 unpinned actions) total ~24 findings, which is well within the grandfather catalog's tolerance. *Still open.*

### 5.6 Q6 — Closeout completion coordination

The parallel chat is still working on closeout. v2.0 lands after closeout finishes. If closeout takes longer than expected (or pauses), should v2.0 land items unblocked by current HEAD?

Recommendation: **yes** — v2.0 items (cat1 rstrip bug, exit-code fixes, fail-open fixes, velocity.bin.bin, banked G.2/G.4) are independent of closeout state. confirmed at HEAD `a9b2aeb` that only commits 1-2 of 8 closeout commits have landed; v2.-1 (perf-fix) and v2.0 can begin in parallel with closeout finishing. *Still open.*

### 5.7 Q7 — New: Cat 4 long-term Stack C screenshot mechanism

 identified two paths for Stack C screenshot capture:
- *Interim:* `xwd` by window ID. Zero sim source changes. Slower per-frame; relies on X11 server running.
- *Long-term:* `vkCmdCopyImageToBuffer` + `stb_image_write` in each Stack C sim. ~30 LOC per sim; faster; no X11 dependency.

v2.16 uses the interim approach. Should the long-term approach be a single batch (v2.20-ish) or per-sim under existing Cat 4 adoption batches (v2.17/v2.18/v2.21)?

Recommendation: **single batch (v2.16.5 or v2.23.5).** Per-sim spread creates 4 nearly-identical commits across batches when one batch can land all 4 with shared review. *Still open.*

---

## § 7 — Risks and open design hazards

### 7.1 Cat 4 golden binary churn risk

Goldens are generated against current HEAD. Every intentional sim change requires rebaselining. **Mitigation:** rebaseline flow is one command (`python3 -m integrity --rebaseline <check> <sim>`); commits that intentionally change the sim include the rebaseline in the same commit; the diff in the PR is the review surface.

### 7.2 Cat 4 tolerance calibration

Per-buffer tolerance for tolerance-snapshot has to be set. Too tight: spurious failures. Too loose: misses real regressions. **Mitigation:** start permissive (~1% relative tolerance); tighten over time as confidence builds. Per-buffer config in `<sim>/tests/integrity/config.toml`. Each Cat 4 batch reports calibration choice and rationale in the audit.

### 7.3 Visual regression flakiness on GPU output

GPU rendering can produce frame-to-frame jitter from non-determinism (driver-level FMA reordering, etc.) — even with `cat4.determinism-pair` passing on the data side, the screenshot may differ by a few pixels. **Mitigation:** SSIM threshold > 0.99 (allows perceptually-imperceptible variance); pixelmatch threshold 0.5–1% (relaxed from typical web 0.1%); PSNR tracked for trend but not gated unless catastrophic. SwiftShader / lavapipe deterministic mode for full reproducibility if needed.

### 7.4 MMS sim modification risk

MMS requires adding a source term to the PDE solver. The modification itself could have bugs. **Mitigation:** the modification is gated behind `--mms-source` flag; production runs are unaffected; the MMS test verifies the *modification is correct* before relying on it to verify the rest of the solver (start with `u_hat` = constant, source term = 0; verify solver reproduces constant; then progress to `sin(x)cos(t)` etc.). This is the standard MOOSE/MFiX pattern.

### 7.5 GPU-headless flakiness

dawn.node and lavapipe are software implementations. Both can have subtle bugs producing different output than real GPU drivers. **Mitigation:** Cat 3 verifies against algebraic / MMS ground truth (not against real-GPU output). If the headless impl produces wrong output for the expected math, Cat 3 catches it regardless of which side is wrong.

### 7.6 Cat 5 spec-claim regex space

Extracting "claims" from spec prose is a heuristic. False-positives surface; false-negatives let drift slip through. **Mitigation:** start with narrowest claim shape (backticked inline-code `path:line` style + backticked function signatures). Expand as false-positive rate stabilizes.

### 7.7 Batch composition under concurrent sessions

v1 ran into concurrent multi-session friction (Conventions G, H, I,
J, K). v2 execution **is expected to be multi-agent and parallel**
(per § 3.17 — formalized as the parallel agent orchestration model).
The hazard from v1 remains real; the mitigation is now structural,
not advisory.

**Mitigations (all in § 3.17):**

- Each batch's spec entry names its `touches:` file set explicitly.
- File-touch matrix maintained in `audit-trail.md` § 10 (produced by
  Probe EE; updated when batches add or change file scope).
- Two parallel batches require disjoint touch sets; conflicts pause
  for coordinator re-sequencing.
- Spec changes serialize (single spec-author session at a time).
- Probe outputs append-only under `docs/diagnostics/_audits/`.
- Pre-spawn snapshot captures the comparison baseline before parallel
  dispatch.

### 7.8 The toolkit becoming load-bearing

After v2, the integrity toolkit gates every push and PR with ~150 checks. If the toolkit itself breaks, the whole pipeline stops. **Mitigation:** the toolkit's own gate runs its tests; v2 doesn't add a check without corresponding tests for both pass and fail cases; v2.32 reviews self-application coverage.

### 7.9 Visual regression masking complexity

UI overlays (ImGui, GGUI sidebars) change between runs. Mask everything around the simulated content. **Mitigation:** per-sim mask files; if a sim's UI is rendered into the same image as simulation content, switch sim to fullscreen-render mode for screenshot acquisition.

### 7.10 LFS bandwidth on free tier

GitHub free tier: 1GB storage + 1GB/mo bandwidth. v2 fully loaded: ~500MB–2GB goldens. Heavy CI activity could exceed bandwidth. **Mitigation:** Steven on paid LFS tier ($5/mo for 50GB storage + 50GB bandwidth) is the comfortable answer. Alternatively, sparse-checkout LFS in CI — pull only goldens for sims that changed in the diff.

---

## § 8 — References

### v1 source documents

- `docs/integrity-toolkit-spec.md` — v1 canonical spec.
- `docs/retro/integrity-toolkit-v1.md` — v1 retro.
- `docs/retro/integrity-toolkit-v1.1-batch1.md` — v1.1 retro.
- `docs/retro/integrity-toolkit-v1.1-batch1-addendum.md` — P-numbering.
- `docs/retro/integrity-toolkit-v1.2-bolt-ons.md` — v1.2 retro.
- `docs/retro/integrity-toolkit-v1.3-candidates.md` — v1.3 roadmap.
- `docs/retro/integrity-toolkit-v1.3-batch1-part-a.md` — v1.3 part-A retro.
- `docs/retro/integrity-toolkit-v1.3-batch1-part-b.md` — v1.3 part-B retro.
- `docs/diagnostics/_audits/integrity_v1_3_closeout_spec_2026-05-17_architect1.md` — v1.3 closeout spec.

### Probe data (this design intake)

- : v1.3 closeout end-state (2026-05-17, HEAD `a9b2aeb`).
 **Closeout completed: final HEAD `351c66e`, 8 commits landed, gate
 46 HARD_FAIL / 197 tests passing (down from 60 / up from 183 at
 probe-time). re-anchored every -M line citation
 against final HEAD — 11 CONFIRMED, 4 SHIFTED, 0 REFUTED, 1 DEFERRED.**
- : Capture format + writer call sites across 11 sims.
- : Per-stack entrypoint patterns.
- : `.github/workflows/` full inventory.
- : Broken-state characterization with visual inspection.
- : Cat 1/2/3 implementation audit (rev 4 intake; rev 5
 re-anchor confirmed: F.1 path now `tools/integrity/integrity/cat1_citations/checks/annotation.py:81`).
- : Runner CLI + exit code audit (rev 4 intake; rev 5 
 reconfirmed fail-open still present at HEAD `351c66e`).
- : Toolkit self-coverage report (rev 4 intake; rev 5 
 re-captured: 80% coverage, 3 zero-coverage modules unchanged).
- : Cat 7 security baseline (rev 4 intake, local-mode only —
 remote-mode Scorecard deferred per Steven).
- : Performance / CI wall-clock characterization (rev 4 intake;
 rev 5 re-timed cat2-c at 98.48s @ 1.57 GB RSS — closeout
 commit 2 was memory not time optimization; Phase 0 perf batch
 genuinely net-new).
- : Cross-stack StateWriter/StateReader byte-identity matrix
 (rev 4 intake).
- : Smoke MMS proof-of-concept + BC analysis (rev 4 intake;
 rev 5 re-anchor: injection slot at line 1945 not 1946).
- : Cat 4 visual regression per-sim setup catalog (rev 4 intake).
- **: Re-anchor every -M citation against closeout HEAD
 `351c66e` (rev 5 intake). 11/4/0/1 outcomes; no closeout commit
 silently fixed any -M bug.**
- **: DFSPH reference patterns + our shader cross-reference
 (rev 5 intake). Headline § B.2: pressure-coupling inconsistency
 (storage convention R2 mixed with stencil convention R1, no third
 self-consistent reading).**
- **: SPH hydrostatic empirical check (rev 5 intake). Empirical
 step BLOCKED (no still-water scene exists) but static code re-read
 CORROBORATED. Recommends Path A (fix-first); 6-line
 fix sketch in § E.**
- (deferred): Remote-mode OpenSSF Scorecard against
 `github.com/StevenFAU/GPU-Sims`. Skipped per Steven's call.
- ** MMS harness
 foundation prototype, LBM bounce-back convention probe, RD
 Pearson-fixture probe, MPM particle-grid oracle probe.

### Industry standard references (web search)

**Visual regression:**

- jest-image-snapshot: https://github.com/americanexpress/jest-image-snapshot — pixelmatch + ssim.js with 0.01 default threshold, used widely.
- BackstopJS, Percy, Chromatic: enterprise visual regression services.
- VisIt (LLNL): https://visit-sphinx-github-user-manual.readthedocs.io/ — scientific-viz regression with Git LFS.
- subito.it: visual regression in production with Playwright + Git LFS.
- VTK with Dawn: https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11323 — WebGPU regression in CI.

**Numerical verification:**

- Method of Manufactured Solutions: MOOSE (`mooseframework.inl.gov/python/mms.html`), MFiX (DOE `mfix.netl.doe.gov/doc/vvuq-manual/main/html/mms/`), Stanford CTR, NASA NTRS 20150015494.
- Roache 2004/2009/2016 — foundational MMS literature.
- "Testing for physical validity in molecular simulations" PMC6126824 — invariant testing for physics sims.

**GPU shader headless:**

- Dawn (Google): https://dawn.googlesource.com/dawn — WebGPU implementation; `webgpu` npm packages it for Node.js.
- Mesa lavapipe: apt-installable CPU Vulkan ICD.
- SwiftShader (Google): https://github.com/google/swiftshader — Vulkan 1.3 CPU implementation, Apache-licensed.

**Snapshot regression engines:**

- pytest-regtest: https://pytest-regtest.readthedocs.io/ — Python; supports NumPy `atol`/`rtol`; used by Tezos.
- pytest-regressions: alternative; documented at https://johal.in/pytest-regressions-data-golden-file-updates-2025/.

**Workflow / CI linting:**

- actionlint: https://github.com/rhysd/actionlint — GitHub Actions workflow linter, 3700+ stars; integrates shellcheck + pyflakes.
- yamllint, markdownlint-cli2 (already in use): general YAML/markdown linters.
- cmake-lint: from cmakelang; pip-installable.

**Vulkan validation:**

- Khronos Validation Layer: https://vulkan.lunarg.com/doc/view/1.3.283.0/windows/khronos_validation_layer.html
- VkValidationFeatureEnableEXT spec: https://registry.khronos.org/vulkan/specs/latest/man/html/VkValidationFeatureEnableEXT.html

**Image quality metrics:**

- SSIM: Wang & Bovik 2004 image quality assessment (the canonical paper).
- PSNR vs SSIM: https://www.testdevlab.com/blog/full-reference-quality-metrics-vmaf-psnr-and-ssim
- Limitations of both: https://videoprocessing.ai/metrics/ways-of-cheating-on-popular-objective-metrics.html

**Static analysis output format (toolkit-itself standards):**

- SARIF 2.1.0 OASIS standard: https://docs.oasis-open.org/sarif/sarif/v2.1.0/sarif-v2.1.0.html
- SARIF JSON schema: https://github.com/oasis-tcs/sarif-spec/blob/main/sarif-2.1/schema/sarif-schema-2.1.0.json
- GitHub Code Scanning SARIF support: https://docs.github.com/en/code-security/code-scanning/integrating-with-code-scanning/sarif-support-for-code-scanning
- Microsoft SARIF tutorials: https://github.com/microsoft/sarif-tutorials
- Sonar SARIF guide: https://www.sonarsource.com/resources/library/sarif/

**Linter architecture patterns (toolkit-itself standards):**

- Ruff (Python, Rust-implemented): https://github.com/astral-sh/ruff — drop-in
 Flake8 replacement; "safe vs unsafe" fix classification; prefix-NNN rule
 IDs; pyproject.toml config.
- ESLint architecture: https://eslint.org/docs/latest/contribute/architecture/ —
 plugin/preset model; flat-config; rule-as-data pattern.
- Ruff linter docs: https://docs.astral.sh/ruff/linter/ — exit-code convention
 (0/1/2); rule selection model; fix safety.
- Clippy rules: https://rust-lang.github.io/rust-clippy/ — per-rule docs;
 category-based organization (correctness/complexity/perf/style).
- SonarQube rule database: https://rules.sonarsource.com/ — per-rule
 documentation pattern reference.

**Documentation testing (Cat 5 doctest enhancement):**

- Python doctest: https://docs.python.org/3/library/doctest.html
- Sphinx doctest extension: https://www.sphinx-doc.org/en/master/usage/extensions/doctest.html
- Rust rustdoc tests: https://doc.rust-lang.org/rustdoc/write-documentation/documentation-tests.html

**Property-based testing (Cat 3 enhancement):**

- Hypothesis (Python): https://hypothesis.readthedocs.io/
- "Find more bugs with less work" — D. R. MacIver, Hypothesis maintainer.
- QuickCheck (Haskell origin): https://hackage.haskell.org/package/QuickCheck

**Security / dependency hygiene (Cat 7):**

- pip-audit (PyPA): https://pypi.org/project/pip-audit/
- npm audit (npm built-in): https://docs.npmjs.com/cli/v10/commands/npm-audit
- gitleaks: https://github.com/gitleaks/gitleaks
- OpenSSF Scorecard: https://github.com/ossf/scorecard — 16+ automated
 security checks scored 0-10.
- scancode-toolkit (license): https://github.com/aboutcode-org/scancode-toolkit
- dependabot: https://docs.github.com/en/code-security/dependabot

**External link checking (already in use):**

- lychee (Rust async link checker): https://github.com/lycheeverse/lychee — in use
 in markdown.yml.

**Mutation testing (new in rev 4, § 1.7.K):**

- mutmut: https://github.com/boxed/mutmut — Python's most-active mutation tester (2024-2025 IEEE Software).
- cosmic-ray: https://cosmic-ray.readthedocs.io — alternative; more operators, slower setup.
- pytest-gremlins: https://dev.to/mikelane/announcing-pytest-gremlins-v130-53ka — newer pytest-native (2026).
- "Static and Dynamic Comparison of Mutation Testing Tools for Python" — DOI 10.1145/3701625.3701659.

**Hermetic builds (new in rev 4, § 1.7.L):**

- Hermetic Builds roadmap (Nemorize): https://nemorize.com/roadmaps/hermetic-builds/ — Level 1-5 ladder.
- Hermetic build playbook (beefed.ai): https://beefed.ai/en/hermetic-build-playbook — Bazel patterns.
- Docker reproducibility (BuildKit + SOURCE_DATE_EPOCH): https://docs.docker.com/build/ci/github-actions/reproducible-builds/
- reproducible-builds.org: https://reproducible-builds.org/ — Debian, Fedora project on bit-for-bit reproducibility.

**ACM Artifact Evaluation badges (new in rev 4, § 1.7.N):**

- ACM Artifact Review and Badging Policy: https://www.acm.org/publications/policies/artifact-review-badging
- ACM SIGSIM PADS: https://sigsim.acm.org/conf/pads/2026/blog/artifact-evaluation/
- ctuning Artifact Evaluation (the unified appendix template): https://github.com/ctuning/artifact-evaluation

**Reproducibility vs replication framing (new in rev 4 patch):**

- ReScience C: http://rescience.github.io — peer-reviewed computational replication journal; Claerbout/Donoho/Peng vocabulary distinction. *Reproduction* = same code + same input → same result; what Cat 4 bitwise-snapshot gives us. *Replication* = independent reimplementation → same conclusion; what `cat3.cross-stack-equivalence` (v2.11) gives us between Stack B and Stack C ports.
- "Sustainable computational science: the ReScience initiative" PMC8530091.

**Supply-chain provenance (new in rev 4 patch — relevance: only at release time):**

- SLSA (Supply-chain Levels for Software Artifacts), v1.1: https://slsa.dev — 4 levels (0-3). L1 = recorded provenance; L2 = signed/tamper-resistant; L3 = isolated hardened build platform.
- in-toto attestation framework (CNCF): https://in-toto.io
- GitHub built-in artifact attestation: https://docs.github.com/en/actions/security-for-github-actions/using-artifact-attestations
- Relevant only if/when GPU-Sims cuts release tags. Not adding machinery in v2.

**Physics-correctness references (new in rev 5):**

- Stream-function formulation for incompressible Navier-Stokes (Wikipedia): https://en.wikipedia.org/wiki/Navier%E2%80%93Stokes_equations — "The stream function is constant on no-flow surfaces, with no-slip velocity conditions on surfaces." Confirms ψ-vanishes-on-face implies u-vanishes-on-face. Used for the smoke MMS 3D correction.
- García-Casado 2019 (arXiv 1707.09314) — exact solutions for restricted incompressible Navier-Stokes with Dirichlet BCs. Reference for analytic-solution existence in our MMS scope.
- Shirokoff & Rosales 2010 (arXiv 1011.3589) — efficient incompressible NS on irregular domains with no-slip BCs, high order up to the boundary. Reference for projection-method conventions.
- Bender & Koschier 2017 — Divergence-Free SPH for Incompressible and Viscous Fluids. Figure 2 dam-break with 125k particles is the canonical DFSPH validation case. https://discovery.ucl.ac.uk/10056699/1/BK17.pdf
- Becker & Teschner 2007 — Weakly compressible SPH for free surface flows (Tait equation). Reference for distinguishing WCSPH (Tait EOS) from DFSPH (Poisson). https://cg.informatik.uni-freiburg.de/publications/2007_SCA_SPH.pdf
- Meng/Gu/Emerson 2015 (arXiv 1508.02209) — slip velocity of LBM bounce-back boundary scheme. Establishes the `O(1/N)` slip scaling for full-way bounce-back; informs rev 5's LBM tolerance correction.
- Krastins 2020 — Moment-based BCs for 3D LBM. Establishes second-order convergence for moment-based BCs vs first-order for bounce-back. https://onlinelibrary.wiley.com/doi/full/10.1002/fld.4856
- Zou & He 1997 — On pressure and velocity flow BCs for LBM. Half-way bounce-back gives second-order accuracy for 2D Poiseuille. https://arxiv.org/abs/comp-gas/9611001
- Pearson 1993 — "Complex patterns in a simple system" *Science* 261:5118. Phase diagram in (F, k) parameter space for Gray-Scott reaction-diffusion. Canonical fixture coordinates: spots (0.035, 0.0625), stripes (0.035, 0.060), spiral waves (0.0118, 0.0475). https://www3.nd.edu/~powers/pearson.pdf
- MROB Gray-Scott catalog (Robert Munafo's expansion of Pearson's classification): http://www.mrob.com/pub/comp/xmorphia/index.html — pattern naming and extended phase-diagram coverage.

**MMS source-term generation (new in rev 5, § 1.3.B):**

- MASA: Manufactured Analytical Solution Abstraction library (open-source) — academia.edu/19827746. Canonical MMS reference repo; Maple and SymPy backends.
- MFiX VVUQ MMS chapter: https://mfix.netl.doe.gov/doc/vvuq-manual/main/html/mms/ — DOE-curated test-case bank; cited for canonical PDE forms.
- FDA tool: https://cdrh-rst.fda.gov/method-manufactured-solutions-mms-code-verification-source-term-generation-tool — Jupyter notebook template (Python + SymPy + NumPy + pandas + matplotlib).
- Sympy diff/simplify workflow for MMS source-term generation: standard pattern across MASA, MFiX, FDA, Stanford CTR. Used as v2.5 harness foundation.

**Per-sim canonical references (new in rev 6, § 1.0.B):**

*Eulerian smoke / fluid simulation:*
- Bridson, *Fluid Simulation for Computer Graphics* 2nd ed (2015), CRC Press / Routledge ISBN 9781482232837. THE canonical graphics-fluids textbook. MAC grid, semi-Lagrangian advection, pressure projection.
- Stam 1999, "Stable Fluids" SIGGRAPH — semi-Lagrangian advection origination.
- Fedkiw, Stam, Jensen 2001, "Visual Simulation of Smoke" SIGGRAPH — smoke specifically.
- Kim, Liu, Llamas, Rossignac 2005, "FlowFixer: Using BFECC for Fluid Simulation" — BFECC for advection. https://faculty.cc.gatech.edu/~jarek/papers/FlowFixer.pdf
- Selle et al. 2008 — MacCormack method for fluid simulation; second-order accurate, unconditionally stable.

*SPH:*
- Bender & Koschier 2017, "Divergence-Free SPH for Incompressible and Viscous Fluids" TVCG. https://discovery.ucl.ac.uk/10056699/1/BK17.pdf — DFSPH canonical paper.
- SPlisHSPlasH (Bender et al., open source) — reference implementation. https://github.com/InteractiveComputerGraphics/SPlisHSPlasH
- Ihmsen et al. 2014, "SPH Fluids in Computer Graphics" Eurographics state-of-the-art report — covers WCSPH/PCISPH/IISPH/DFSPH/PBF/PF comparison.
- Huang et al. 2024, "Journey into SPH Simulation: A Comprehensive Framework and Showcase" — recent (2024) survey confirming DFSPH state-of-the-art. https://arxiv.org/abs/2403.11156

*Lattice Boltzmann:*
- Krüger, Kusumaatmaja, Kuzmin, Shardt, Silva, Viggen 2016, *The Lattice Boltzmann Method: Principles and Practice* Springer. Canonical LBM textbook.
- Malaspinas 2015, "Increasing stability and accuracy of the lattice Boltzmann scheme: recursivity and regularization" arXiv:1505.06900 — HRR collision operator.
- Geier et al. 2015 — Cumulant collision operator.
- Lallemand & Luo 2000 — MRT collision operator origination.
- Bhatnagar, Gross, Krook 1954 — SRT/BGK collision operator origination.

*MPM:*
- Hu, Fang, Ge, Qu, Zhu, Pradhana, Jiang 2018, "A Moving Least Squares Material Point Method with Displacement Discontinuity and Two-Way Rigid Body Coupling" SIGGRAPH — MLS-MPM canonical. https://yuanming.taichi.graphics/publication/2018-mlsmpm/
- Jiang, Schroeder, Selle, Teran, Stomakhin 2015, "The Affine Particle-In-Cell Method" SIGGRAPH — APIC origination.
- Fu et al. 2017, "A Polynomial Particle-In-Cell Method" SIGGRAPH Asia — PolyPIC (higher-order than APIC).
- Sulsky, Chen, Schreyer 1994, "A particle method for history-dependent materials" — MPM origination paper.
- Niall Tippet, "MPM Guide" 2020+ — practical implementation reference. https://nialltl.neocities.org/articles/mpm_guide
- Fei et al. 2021, "Revisiting Integration in the Material Point Method: A Scheme for Easier Separation and Less Dissipation" SIGGRAPH — FLIP/APIC comparison.

*Reaction-diffusion (Gray-Scott):*
- Pearson 1993, "Complex patterns in a simple system" *Science* 261:5118. The phase-diagram paper.
- Gray & Scott 1983, "Autocatalytic reactions in the isothermal CSTR" *Chem Eng Sci* — Gray-Scott model origination.
- Turing 1952, "The chemical basis of morphogenesis" *Phil Trans R Soc B* — Turing pattern foundation.
- MROB Gray-Scott catalog (Robert Munafo): http://www.mrob.com/pub/comp/xmorphia/index.html

*Lenia:*
- Chan 2019, "Lenia: Biology of Artificial Life" *Complex Systems* — Lenia origination.
- Chan 2020, "Lenia and Expanded Universe" *ALIFE 2020*.

*Boids:*
- Reynolds 1987, "Flocks, Herds, and Schools: A Distributed Behavioral Model" SIGGRAPH — boids origination.
- Reynolds 1999, "Steering Behaviors for Autonomous Characters" — expanded rules.

*Physarum:*
- Jones 2010, "Characteristics of Pattern Formation and Evolution in Approximations of Physarum Transport Networks" *Artificial Life* 16:127-153.
- Sage Jensen 2021, community port and Sage's visual variants.

*Strange attractors / Mandelbulb:*
- Per-attractor references: Lorenz 1963 (Lorenz); Aizawa (Aizawa); Chen 1999 (Chen); etc.
- White, Daniel 2009 + Nylander 2009, Mandelbulb formula — first published Mandelbulb iso-surface formulation. (Originally on FractalForums; widely cited.)
- Hart 1996 — Sphere-tracing for implicit surfaces (distance-estimator raymarching foundation).

**Optimization considerations (new in rev 6, § 1.0.C):**

- Selle, Fedkiw, Kim, Liu, Rossignac 2008, "An Unconditionally Stable MacCormack Method" — second-order advection.
- Zhang, Ferguson, Bridson 2015, "Restoring the Missing Vorticity in Advection-Projection Fluid Solvers" — IVOCK; useful when standard semi-Lagrangian over-dissipates vorticity.
- Gagniere et al. 2020, "A Hybrid Lagrangian/Eulerian Collocated Advection and Projection Method for Fluid Simulation" — recent (2020) hybrid technique avoiding MAC-grid staggering. arXiv:2003.12227.

**Conformance testing framing (new in rev 4, § 1.7.M):**

- VK-GL-CTS (Khronos Vulkan/OpenGL/OpenGL ES CTS): https://github.com/KhronosGroup/VK-GL-CTS
- Vulkan CTS docs: https://docs.vulkan.org/guide/latest/vulkan_cts.html
- Android CTS integration: https://source.android.com/docs/core/graphics/cts-integration

**Coverage standards beyond line % (new in rev 4):**

- Parasoft on coverage standards (DO-178C / ISO 26262 / IEC 62304 contexts): https://www.parasoft.com/learning-center/code-coverage-guide/
- Qt on 70/80/90/100% framing: https://www.qt.io/quality-assurance/blog/is-70-80-90-or-100-code-coverage-good-enough
- Sonar on coverage % being misleading: https://www.sonarsource.com/resources/library/code-coverage-unit-tests/ (Google considers 75% commendable).

**Floating-point fuzzing (new in rev 4, deferred per § 0.2):**

- Erik Rigtorp on structure-aware libFuzzer for FP code: https://rigtorp.se/fuzzing-floating-point-code/
- Structure-Aware Fuzzing with libFuzzer (Google): https://github.com/google/fuzzing/blob/master/docs/structure-aware-fuzzing.md
- OSS-Fuzz: https://google.github.io/oss-fuzz/

**libclang + TypeScript compiler API confirmations (new in rev 4):**

- libclang Python bindings: https://libclang.readthedocs.io/en/latest/ — USR-based cross-TU traversal.
- TypeScript Compiler API + TypeChecker: https://www.satellytes.com/blog/post/typescript-ast-type-checker/ — canonical `ts.createProgram` + `getTypeChecker` pattern.
- typescript-eslint AST architecture: https://typescript-eslint.io/blog/asts-and-typescript-eslint/ — dual ESTree + TS AST.
- clang-tidy Contributing guide (custom checks): https://clang.llvm.org/extra/clang-tidy/Contributing.html

### Cross-references to project documents

- `docs/category-contexts/quantum.md` § 6.1 — quantum cat3 seed.
- `docs/tier1-capture-format-reference.md` — capture format contract (stale; pending v2.1 fix).
- `project-state.md` § 7 — convention index.
- `gpu-sims-frontier-gap-analysis.md` — substantive frontier gaps.

### bug-class evidence (motivating examples)

- Eulerian smoke velocity magnitude ±1106 against industry-standard
 ≤ 50 range — wildly out of spec; almost certainly missing or
 wrong-signed pressure projection / oversized buoyancy / vorticity
 confinement / wrong dt scaling. **** v2.6 smoke MMS
 with stream-function manufactured solution `ψ = sin²(πx)sin²(πy)`
 will catch this bug class. Per § 3.13: smoke Cat 4 baseline cannot
 lock until this is fixed.
- Sph-water 196 ms/frame on 1M particles on 6800 XT — 10× slower than
 expected DFSPH performance; perf-class bug separate from correctness
 but worth surfacing.
- Sph-water visible horizontal banding on water surface — screen-space
 fluid bilateral smoothing under-tuned; correctness-of-rendering bug.

