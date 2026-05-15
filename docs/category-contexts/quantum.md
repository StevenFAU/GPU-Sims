---
title: Quantum / Ising on D-Wave — Category Context
date: 2026-05-15
author: architect1 (pre-spec scoping)
status: draft, probe-verified 2026-05-15 (two passes; see Appendix B)
scope: pre-spec scoping document for the Quantum category, currently single-sim (ising-dwave)
audience: coordinator (decisions), repo-architect (eventual spec drafting), reviewer-architect (cross-review prep)
sibling-docs:
  - docs/overarching-spec.md
  - docs/sim-specs/ising-dwave.md
  - docs/integrity-toolkit-spec.md
  - quantum/README.md
  - quantum/ising-dwave/README.md
---

# Quantum / Ising on D-Wave — Category Context

## 0. Reader's note

This document is a **pre-spec scoping document**, not a spec sheet. It exists because the Quantum category does not transfer cleanly from the four-stack model that governs every other sim in the repo, and the existing `docs/sim-specs/ising-dwave.md` template was designed around GPU sims in ways that don't fit. Drafting the spec sheet directly would commit to template assumptions that need adapting first.

This doc does three things:

1. Surveys the 2018-2025 published landscape of Ising-on-D-Wave research so the eventual spec has a grounded set of canonical anchors, not the architect's recollection.
2. Lays out what D-Wave Leap and the Advantage2 hardware actually expose, so the Phase plan accounts for the real constraints (scheduled access, embedding limits, trial-tier QPU minutes) rather than imagined ones.
3. Surfaces the genuine *demo-shape question* this category forces — there are at least three honest framings of "an Ising sim on D-Wave," and they produce very different deliverables. The doc surfaces them rather than picking; the coordinator picks.

When this doc is approved by the coordinator, three follow-up artifacts land in order: (a) a Quantum-adapted spec template (small delta to the existing template), (b) the full `docs/sim-specs/ising-dwave.md` covering both tracks named in § 6, (c) the integrity-toolkit Cat 3 amendments in § 7. Code work begins only after that chain.

This doc is **not** load-bearing in the same sense as `docs/load-bearing-decisions.md` artifacts on shipped sims. Those record decisions that have shipped and are expensive to revisit. This doc records decisions that have *not yet shipped* and are being deliberated.

---

## 1. Why this category is structurally different

Every other simulation category in this repo shares four implicit assumptions that the Quantum category does not. Naming them explicitly heads off pattern-matching from sim phases that don't apply.

### 1.1 Compute is not local and not on-demand

SPH, MPM, eulerian smoke, Lenia, Neural CA — every other sim runs on hardware the owner controls: the RX 6800 XT, the lab PC, or the HPC node. The dev loop is "edit a shader, hit F5, see the result." D-Wave Leap is a remote cloud service accessed via a Python SDK; QPU access is metered in microseconds-of-anneal-time; trial-tier accounts get one minute of QPU access total (not per month) and that allotment expires one month after signup. Even a fully-funded academic account does not give "edit-and-rerun" iteration speed on the QPU itself.

**Implication:** every line of code that touches the D-Wave sampler must also work against a classical drop-in sampler (`dwave-samplers`' `SimulatedAnnealingSampler`, formerly known as `neal`). All development happens against the classical sampler. QPU runs are reserved for specific hero-shot moments — not used for iteration. This is not a preference; it is the budget constraint speaking.

### 1.2 The output is samples, not a trajectory

Every other sim produces a frame-by-frame state evolution that maps directly to "watch it go." A D-Wave anneal does not produce a trajectory. It produces an *ensemble of samples* — typically a few hundred to a few thousand spin configurations, each annotated with the energy of that configuration, the per-chain-break statistics, and (for advanced protocols) the anneal schedule that produced it.

There is no built-in notion of "frame N+1 after frame N" for a D-Wave anneal. A "phase transition" visualization in the D-Wave-native sense is not a movie of one configuration evolving across time; it is a *sweep across parameter space* (coupling strength, transverse-field schedule, effective temperature) with sample distributions at each parameter point.

This is the single most consequential difference. The category-README's current language about "real-time visualization of spin configurations and phase transitions" implicitly assumes the trajectory framing. It needs softening — see § 5 for the three honest framings this can resolve to.

### 1.3 Lattice geometry is constrained by hardware topology

Every other sim picks its grid resolution by "what fits in VRAM." D-Wave's qubits live on a fixed hardware graph with fixed connectivity. The Advantage2 system uses the Zephyr topology with 20-way per-qubit connectivity; the prior Advantage system used Pegasus with 15-way connectivity; the older 2000Q used Chimera with 6-way connectivity. A logical Ising lattice (a 2D square lattice has 4-way connectivity; a 3D cubic lattice has 6-way; etc.) only maps onto the physical graph through a **minor embedding** — chaining multiple physical qubits together with strong ferromagnetic couplings to represent a single logical lattice site.

The chaining is the binding constraint on lattice size. King et al.'s landmark 3D-Ising result on the Pegasus-era hardware reached 8×8×8 cubic lattices using thousands of physical qubits to represent 512 logical spins. Zephyr's higher connectivity reduces the chain length needed for an equivalent embedding but does not eliminate it. The headline "4400+ qubits" on Advantage2 is the *physical* count; the largest logical Ising lattice that fits is substantially smaller and depends on the lattice geometry.

**Implication for the spec:** the "scale tier" section of the standard template (laptop / desktop / HPC) does not translate. The Quantum-category replacement is a *graph-fit ladder*: what's the largest 2D square lattice that embeds cleanly on Zephyr, the largest 3D cubic, the largest frustrated triangular, the largest cubic-spin-glass with random disorder. Each comes with a specific minimum chain-length and a specific chain-break-rate budget. § 5 and § 6 propose where in this ladder the demo should target.

### 1.4 The canonical reference is a *physics* paper, not a *method* paper

Every other sim has a method-paper anchor: Stam 1999 for stable fluids, Bender 2017 for DFSPH, Hu 2018 for MLS-MPM, Reynolds 1987 for boids, Pearson 1993 for Gray-Scott. These are "here is how to compute X" papers. The architect copies the method and verifies the implementation matches.

The Ising-on-D-Wave literature is different in kind. The canonical anchors (Harris-King 2018, King 2022, King 2024/2025, Vodeb 2025 — see § 4) are **physics-using-the-apparatus** papers. The "method" is "submit a QUBO encoding the Hamiltonian; receive samples." The interesting choices are the physics question being asked (which Hamiltonian, which annealing schedule, which order parameters to measure) and the experimental design (parameter sweeps, sample sizes, statistical analysis). The choices live one level higher than they do for the other sims.

**Implication for the spec:** the "mathematical formulation" section of the standard template is two questions, not one. (a) What is the Hamiltonian being submitted? (b) What is the physics question that justifies submitting it? The second question is what gives the demo its narrative.

---

## 2. Hardware reality

### 2.1 The Advantage2 system

D-Wave's Advantage2 quantum annealer reached general availability on 2025-05-20 (Leap solver identifier `Advantage2_system1.1`). The relevant facts for this category:

| Property | Value | Source |
|---|---|---|
| Qubit count | 4,400+ | D-Wave product page |
| Coupler count | 40,000+ | D-Wave product page |
| Topology | Zephyr (20-way connectivity) | D-Wave product page |
| Energy scale increase vs Advantage | +40% | D-Wave press 2025-05-20 |
| Noise reduction vs Advantage | -75% | D-Wave press 2025-05-20 |
| Coherence gain vs Advantage | 2× | D-Wave press 2025-05-20 |
| Bias range | nominal [-6.0, 6.0] (was [-4.0, 4.0] on Advantage) | D-Wave SDK release notes |
| Anneal schedule features | Standard + "fast anneal" mode | D-Wave Advantage2 docs |

Specific to "what fits": the Advantage2 marketing emphasizes 3D lattice problems and spin-glass dynamics simulation as the canonical workload, with the King et al. 2024/2025 "Beyond-Classical Computation in Quantum Simulation" Science paper as the lead reference (arXiv 2403.00910). That work used the Advantage2 prototype — substantively similar topology to the production system but with fewer qubits.

### 2.2 Leap access tiers

D-Wave Leap is the cloud service through which the QPU is accessed.

| Tier | QPU time | Hybrid solver time | Duration |
|---|---|---|---|
| Trial (default on signup) | 1 minute total | 20 minutes total | Expires 1 month after signup |
| Open-source contribution path | Earn additional time per month | Earn additional time per month | Ongoing |
| Leap Quantum LaunchPad | 3-month free trial with expanded access | Same | 3 months |
| Academic / commercial paid | Per contract | Per contract | Per contract |

**Project-specific note:** the owner currently holds a Trial-tier account. The school's eventual Advantage2 access is on an unknown timeline (fall or spring semester). This window — from "trial tier" to "school hardware" — is what the architecture pre-build is sized against. Plan for an absolute minimum of QPU usage during the development window; most of Track 2 should be exercisable against the classical sampler without burning trial minutes.

Worth investigating separately: (a) whether Steven qualifies for the Leap Quantum LaunchPad 3-month program before school access lands, (b) the open-source-contribution monthly-credit mechanism applied to a public MIT-licensed repo of exactly this shape, and (c) what the school's access actually looks like in practice (dedicated time, shared queue, scheduled blocks). These three together determine the QPU-minute budget across the project lifespan.

### 2.3 Ocean SDK surface (the Python side)

The Ocean SDK is D-Wave's open-source Python toolkit for QUBO/Ising problem construction, sampling, and minor-embedding. Relevant module structure for this work:

- **`dimod`** — core types and sampler protocol. The central type is `dimod.BinaryQuadraticModel` (BQM), constructible via `BinaryQuadraticModel.from_ising(h, J)` where `h` is a dict of linear biases and `J` is a dict of quadratic biases keyed by edge tuples, or via `BinaryQuadraticModel.from_qubo(Q)`. Includes the reference `ExactSolver` for problems small enough to enumerate.
- **`dwave-samplers`** — classical samplers with `dimod`-compatible interface. Contains `SimulatedAnnealingSampler` (succeeding the older `neal` package), `SteepestDescentSampler`, `TabuSampler`, a `PlanarGraphSolver` for exact solutions on planar Ising problems without linear biases, and a simulated-quantum-annealing sampler. **These are the dev-loop substitute for the real QPU.**
- **`dwave.system`** — `DWaveSampler` (real-QPU sampler), `EmbeddingComposite` (auto-finds a minor-embedding on each call), `AutoEmbeddingComposite` (tries direct submission first), `FixedEmbeddingComposite` (reuses a precomputed embedding across repeated submissions — important for parameter sweeps).
- **`minorminer`** — the embedding-finding library. `minorminer.find_embedding(S, T)` takes a source graph `S` and target graph `T` and returns an embedding dict.
- **`dwave_networkx`** — `chimera_graph()`, `pegasus_graph()`, `zephyr_graph()` functions for constructing the hardware topology graphs as NetworkX objects. Used both for inspecting hardware structure and for offline embedding work.

The critical architectural point: **all of `DWaveSampler`, `SimulatedAnnealingSampler`, `ExactSolver`, and `PlanarGraphSolver` accept the same BQM and return `dimod.SampleSet`**. Code written against the `Sampler` interface is sampler-agnostic. Track 2's Python infrastructure must use this interface from day one, with the concrete sampler selected by a config flag. This makes the QPU-vs-classical-sampler swap a one-line change in the run script.

---

## 3. Literature survey

### 3.1 Canonical classical Ising

These are the anchors for the classical-Ising side of the work (Track 1 in § 6). They are method papers in the standard sense — they describe an algorithm and any implementation must verify against the math they describe.

- **Onsager, L. (1944).** "Crystal Statistics. I. A Two-Dimensional Model with an Order-Disorder Transition." *Physical Review* 65 (3–4), 117–149. The exact analytical solution of the 2D Ising model on a square lattice. Provides the critical temperature kT_c/J = 2/ln(1+√2) ≈ 2.269185, the universal critical exponents (β = 1/8, ν = 1, γ = 7/4 for 2D Ising), and the spontaneous magnetization curve below T_c. This is the verifiable ground truth against which any 2D Ising simulator can be calibrated.
- **Metropolis, N., Rosenbluth, A. W., Rosenbluth, M. N., Teller, A. H., Teller, E. (1953).** "Equation of State Calculations by Fast Computing Machines." *Journal of Chemical Physics* 21, 1087. The single-spin-flip Metropolis-Hastings algorithm. Acceptance probability for a proposed spin flip Δs at inverse temperature β is min(1, exp(-βΔE)). Foundational; cited universally.
- **Wolff, U. (1989).** "Collective Monte Carlo Updating for Spin Systems." *Physical Review Letters* 62 (4), 361–364. The single-cluster algorithm. A cluster is grown by adding bonded same-sign neighbors with probability p = 1 - exp(-2βJ), then the entire cluster is flipped. This eliminates the critical-slowing-down that plagues single-spin-flip Metropolis near T_c, where correlation lengths diverge.
- **Swendsen, R. H., Wang, J.-S. (1987).** "Nonuniversal Critical Dynamics in Monte Carlo Simulations." *Physical Review Letters* 58 (2), 86–88. The precursor multi-cluster algorithm to Wolff 1989. Wolff is more commonly used in practice because single-cluster is simpler and equally effective for visualization. Worth citing as background but not the implementation anchor.

The pedagogical value of Track 1 sits directly on top of these papers. Most introductory statistical mechanics curricula use the 2D Ising model with one of these algorithms as a worked example; a live WebGPU implementation near T_c is among the most photogenic introductions to phase transitions in physics.

### 3.2 Ising on D-Wave

These are the anchors for the quantum side of the work (Track 2 in § 6). They are physics-using-the-apparatus papers, not method papers.

- **Harris, R., Sato, Y., Berkley, A. J., Reis, M., Altomare, F., et al. (incl. King, A. D.) (2018).** "Phase transitions in a programmable quantum spin glass simulator." *Science* 361 (6398), 162–165. The canonical anchor. Simulated interacting Ising spins on 3D cubic lattices up to dimensions 8×8×8 on a D-Wave 2000Q (Chimera topology), with programmable disorder. Order parameters measured directly via per-spin readout. Identified paramagnetic, antiferromagnetic, and spin-glass phases by tuning disorder and effective transverse field.
- **King, A. D., Suzuki, S., Raymond, J., Zucca, A., Lanting, T., et al. (2022).** "Coherent quantum annealing in a programmable 2,000 qubit Ising chain." *Nature Physics* 18, 1324–1328. One-dimensional Ising chain at 2,000 qubits, demonstrating Kibble-Zurek scaling of the post-quench defect density and providing one of the first clear demonstrations that the anneal dynamics are coherent rather than thermal at sufficiently fast anneal rates. Important methodologically for the *quench-rate sweep* protocol that became the standard analysis pattern in subsequent work.
- **King, A. D., Nocera, A., Rams, M. M., Dziarmaga, J., Wiersema, R., et al. (2024/2025).** "Beyond-classical computation in quantum simulation." *Science* eado6285 (2025), arXiv:2403.00910 (preprint Mar 2024, peer-reviewed Science 2025). The state-of-the-art reference at this writing. Spin-glass dynamics simulation on the Advantage2 prototype across multiple lattice geometries and quench times, with claimed beyond-classical performance versus leading tensor-network and neural-network classical baselines. This is the paper any 2026 Ising-on-D-Wave demo benchmarks itself against.
- **Vodeb, J., Verstraelen, W., et al. (2025).** "Stirring the false vacuum via interacting quantized bubbles on a 5,564-qubit quantum annealer." *Nature Physics* 21, 386. A different physics question on similar hardware: false-vacuum decay in a programmable Ising model with non-equilibrium initial conditions. Cited as evidence of the breadth of physics questions the same apparatus addresses; not the primary methodological anchor.

### 3.3 What's settled vs what's still open

Three observations that bear on the demo design:

**Settled:** The D-Wave annealer at current generations produces samples consistent with finite-temperature equilibrium thermodynamics in a regime where the dynamics are quench-driven rather than thermalized — the "effective inverse temperature" framing of D-Wave samples is a useful approximation but not a precise statement. Order parameters measured from large sample ensembles (magnetization, structure factor, energy histograms) on lattices that fit the hardware graph cleanly are reproducible and consistent with classical numerical baselines wherever the classical baselines are computable.

**Settled but underappreciated:** The "phase transitions" the apparatus is good at demonstrating are primarily *quantum* phase transitions driven by varying the transverse-field schedule, not the *classical* finite-T phase transitions of the canonical 2D Ising model. The famous Onsager transition at T_c ≈ 2.269 J/k_B is a classical-thermal transition; D-Wave samples it indirectly through an effective-temperature mapping. The transition the apparatus shows naturally is the T = 0, varying-Γ quantum critical point — a structurally different beast.

**Open:** Where, precisely, the boundary lives between "the QPU computes something the classical baseline cannot" and "the classical baseline catches up." The King 2024/2025 Science paper claims beyond-classical performance on specific spin-glass dynamics simulations versus specific tensor-network and neural-network methods. The claim is hotly contested in the broader quantum-computing literature and the ground will continue to shift. Any portfolio demo should be deliberately scoped to *what the apparatus does well* rather than attempting to make the beyond-classical claim itself.

---

## 4. The framing question — three honest demos

The category README and the existing spec stub both describe the goal as "2D / 3D Ising model on D-Wave's quantum annealer, with real-time visualization of spin configurations and phase transitions." That sentence collapses three structurally different demos, each defensible on its own terms. The differences matter enough that committing to one before the spec drafts begin is appropriate.

### Framing A — Ground-state sampling demo

The demo is: pose Ising problems with various coupling-sign patterns (ferromagnetic, antiferromagnetic on bipartite lattices, frustrated triangular and cubic, programmable-disorder spin glass), submit each to the QPU, render the sample ensemble. The "phase transition" is across coupling-strength and disorder sweeps. Visually: spin-grid stills with histograms, structure factor plots, energy distributions. No motion.

**Strengths:** Honest about what D-Wave does. Tightest scope. Lowest QPU cost per parameter point (one anneal call per coupling configuration). Naturally aligned with the King-2018 methodology. Each sample call returns hundreds to thousands of independent samples, which is statistically rich.

**Weaknesses:** Visually static. Does not match the repo's "the demo is the dynamics" aesthetic from boids, smoke, water, MPM. The audience that responds to those sims may not respond to histograms of energy distributions.

### Framing B — Classical-MCMC for visuals, D-Wave for hero

The demo is: a Stack B WebGPU classical Ising simulation (Metropolis + Wolff) provides the live "watch domains form and dissolve at T_c" experience. The user sweeps T through T_c and sees the symmetry-breaking transition unfold in real time. The D-Wave half exists alongside it, displayed in a separate panel: pre-rendered sample ensembles from real QPU runs at the hardware graph's native geometry, with measured order parameters overlaid. The classical simulation provides the narrative; the D-Wave panel provides the legitimacy.

**Strengths:** Maximum visual impact. The Onsager phase transition is famously photogenic. The classical Ising sim is also a fully-credited portfolio piece in its own right; the WebGPU implementation alone is comparable in scope to the boids-3d phase. The D-Wave half is descoped to "we ran the real hardware and here's what came back" without depending on real-time QPU access for the demo to function.

**Weaknesses:** Largest engineering surface — the architect has to ship two sims that share a visualization layer. The classical and D-Wave halves are simulating different physics (classical thermal equilibrium versus programmable quantum dynamics), and the README must be explicit about this rather than blurring the distinction. The "scientific amazement through correctness" philosophy from the overarching spec requires that the two halves not be marketed as the same thing.

### Framing C — Pure D-Wave quench-dynamics replay

The demo is: reproduce a King-style experiment at portfolio scale. Pick a specific published result (Kibble-Zurek defect scaling, 3D spin-glass dynamics, the false-vacuum-decay experiment) and replicate it on a lattice that fits Advantage2 with the published quench protocol. The visualization shows the measured order parameters across the quench-time sweep, ideally side-by-side with the published curves. The "demo" is "I reproduced peer-reviewed physics on real quantum hardware."

**Strengths:** Maximum scientific credibility. The clearest case for "this is a real quantum computation." Tight scope: one Hamiltonian, one protocol, one measurement. Naturally extends from already-published literature, so the methodology is fully grounded.

**Weaknesses:** Lowest visual ambition — at minimum it's a parameter-sweep plot, possibly with sample-ensemble heatmaps annotated against the published curves. Heavy QPU dependency: a full quench-rate sweep at portfolio publication quality requires more QPU minutes than the trial tier provides, so this framing genuinely cannot proceed until school access lands. Carries the highest risk of "the reproduction doesn't quite reproduce" and needing follow-up debugging across multiple QPU sessions, which the access pattern may not support.

### Framing-comparison summary

| Axis | A: Ground-state sampling | B: Classical+D-Wave hero | C: Pure quench replay |
|---|---|---|---|
| Visual ambition | Low | Highest | Lowest |
| Scientific ambition | Moderate | Moderate | Highest |
| Engineering surface | Smallest | Largest | Medium |
| QPU dependency | Moderate | Low for the visuals; moderate for hero panel | Highest |
| Compatibility with trial tier alone | Marginal | Yes | No |
| Compatibility with school access | Yes | Yes | Yes |
| Risk of "doesn't match the published result" | Low | Low | Highest |

The coordinator's call. § 6 proposes a synthesis that captures most of B's strengths while keeping space for a C-style hero render once school access is in place.

---

## 5. The two-track structure (recommended)

This section proposes — but does not lock — a two-track structure under the single `quantum/ising-dwave/` folder. It is the synthesis the coordinator confirmed in the chat preceding this document (2026-05-15): pursue both philosophies in parallel, recognize the difference openly.

### 5.1 Track 1 — Classical Ising (Stack B WebGPU)

**Location proposal:** `quantum/ising-dwave/web/`, following the per-stack-subfolder convention in use across the catalog (e.g., `continuous-ca/lenia-fft/python/` and `continuous-ca/reaction-diffusion-2d/web/`). This sim would be the first to combine `web/` and `python/` subfolders inside a single sim folder; the sub-pattern itself is well-established.

**What ships:** A WebGPU classical Ising simulator on 2D square lattices. Both single-spin-flip Metropolis (good for showing critical slowing down near T_c) and Wolff single-cluster updates (good for showing fast equilibration). Side-by-side visualization mode comparing the two algorithms at the same T. Sliders for J, h (uniform external field), T. Live magnetization and energy traces. Default lattice 512² with a tier selector for 256² / 512² / 1024² / 2048².

**Why Stack B:** Web-deployable, embeddable in the portfolio site, runs on integrated graphics. The classical Ising kernels are simple compute shaders — no FFT, no spatial hashing, no neighborhood searches — well within WebGPU's comfort zone. Storage is one r8sint or r8snorm texture for spins; ping-pong updates. Workgroup size 256 dispatched in checkerboard sub-passes to avoid serial-update bias.

**Canonical anchors:** Metropolis 1953, Wolff 1989, Onsager 1944 (for verification). Algorithmically nothing novel; the contribution is correctness, scale, and visual quality.

**Verification surface:** the 2D Onsager solution gives exact closed-form ground truth for T_c, the critical exponents, and the spontaneous magnetization curve. A built-in calibration mode that sweeps T and measures the magnetization gives a falsifiable correctness check. § 7 proposes the integrity-toolkit Cat 3 entries that make this mechanical.

**Hero render question:** Track 1 does not have an obvious "offline render this in Blender" path because the simulation is 2D and the output is intrinsically rendered. The high-quality output is a recorded video traversal of T through T_c at 4K, which is a normal video-capture pass rather than a path-traced cinematic. Defer the hero-render question to spec-drafting time.

### 5.2 Track 2 — Quantum Ising (Python + Ocean + Leap)

**Location proposal:** `quantum/ising-dwave/python/`.

**What ships:** A Python package wrapping Ocean SDK with three responsibilities:

1. *Problem builders.* Functions that construct `BinaryQuadraticModel` objects for canonical lattice geometries: 2D square ferromagnet, 2D square antiferromagnet on a bipartite checkerboard, frustrated triangular, 3D cubic, programmable-disorder spin glass à la Harris-King 2018. Each builder is parameterized by lattice size, coupling magnitudes, and (where applicable) disorder realizations.
2. *Embedding utilities.* Wrappers around `minorminer.find_embedding` with caching (an embedding for a given (logical lattice, hardware graph) pair is reusable across parameter sweeps) and a fallback `FixedEmbeddingComposite` path. Hardware graph is fetched live from `DWaveSampler().to_networkx_graph()` so the code adapts if the school's specific Advantage2 has a different working-graph subset than the nominal 4400+ qubits.
3. *Sampler abstraction.* Single function that returns a `dimod.Sampler` based on a config flag: `'exact'` for tiny lattices (small enough to enumerate, ≤25 spins), `'simulated_annealing'` for the dev loop, `'qpu'` for the real hardware. The rest of the code treats the sampler as opaque. This is the single most important architectural decision in Track 2 and it is non-negotiable per § 1.1.

The package exports a CLI (`python -m ising_dwave run --geometry square_ferro --size 16 --sampler simulated_annealing --num-reads 1000 --output sampleset.json`) that writes sample sets in a documented JSON schema, plus a separate `analyze` subcommand that consumes a sample set and emits the visualization-layer data structures (magnetization histograms, energy distributions, structure factors, chain-break statistics).

**Canonical anchors:** Harris-King 2018, King 2022, King 2024/2025. Methodologically the closest match is the 2024/2025 paper's spin-glass-dynamics protocol, scaled down to a lattice that fits portfolio runtime constraints.

**Verification surface:** several layers, in order of how mechanically checkable they are. (a) QUBO↔Ising round-trip via `dimod` reference — closed-form, drops directly into the integrity toolkit. (b) Classical Hamiltonian evaluation matches `dimod.BinaryQuadraticModel.energy(sample)` for any sample — closed-form, integrity-toolkit-able. (c) For lattices small enough to enumerate, `ExactSolver` agrees with `SimulatedAnnealingSampler` on the ground state for any random Ising instance — statistical check, integrity-toolkit-able with `num_reads` set high. (d) `SimulatedAnnealingSampler` and `DWaveSampler` agree on ground-state energies for ferromagnetic 2D lattices that fit the hardware — this requires QPU access to verify and lives in the regular test suite, not the integrity toolkit. § 7 lays this out concretely.

**Hero render question:** Track 2's natural hero output is sample-ensemble heatmaps or quench-protocol order-parameter plots, both of which are matplotlib-style scientific renders rather than Blender path-traces. The closest equivalent to the rest of the repo's offline-render trajectory is a high-quality publication-style figure exported as SVG/PNG at print resolution.

### 5.3 How the tracks share viz

Track 1's WebGPU lattice viewer renders spin grids natively. Track 2's Python pipeline emits sample sets as JSON. The Track 1 frontend gains a "Load D-Wave samples" mode that reads Track 2's JSON output and renders the contained spin configurations through the same shader pipeline. This is a single additional input path on the Track 1 side — bytes in, render — and a single additional output path on the Track 2 side. They do not co-evolve; they meet at the JSON boundary.

The shared viz is the *only* shared infrastructure between the tracks. The simulations themselves remain fully decoupled; the README is explicit that the physics being simulated differs between the tracks.

---

## 6. Integrity-toolkit Cat 3 amendments

This section is the corrected version of the diagnostic-toolchain proposal from the chat preceding this document. The current `tools/integrity/` toolkit is v1-feature-complete with Cat 1 (citations), Cat 2 (contracts), and Cat 3 (numerical-vs-upstream); the v2 candidates per spec § 13 explicitly include "per-sim numerical checks beyond common-*", which is the shelf for this work.

### 6.1 Proposed new Cat 3 checks

Each is the same shape as the existing `cat3.cubic-kernel`: a small Python check that compares implementation output against expected within tolerance, with ground truth from either closed-form derivation or a vendored reference. None require Cat 4 (runtime integration) infrastructure.

| Check ID | Stack | Scope | Ground truth | Notes |
|---|---|---|---|---|
| `cat3.ising-energy` | D | Classical Hamiltonian H = −Σ J_ij s_i s_j − Σ h_i s_i evaluated on canonical configurations matches algebraic expected | Algebraic | Pure math; trivially closed-form |
| `cat3.qubo-ising-roundtrip` | D | s = 2x − 1 and the QUBO↔Ising coefficient transformation match `dimod.BinaryQuadraticModel` reference | Vendored: `dimod` | Catches sign-convention bugs (Ising couplings are conventionally J·s_i·s_j with sign meaning the opposite of what intuition suggests) |
| `cat3.wolff-bond-probability` | B (CPU-side) + D | Bond-add probability p = 1 − exp(−2βJ) at canonical (β, J) points | Algebraic | Closed-form; per spec § 8.5 Stack B Cat 3 v1 covers CPU-side host computations |
| `cat3.onsager-tc` | D | Reported critical temperature value matches kT_c/J = 2/ln(1+√2) ≈ 2.269185 | Algebraic | Single-value sanity check; specifically catches confusion between J/k_B and β |

The Stack B WebGPU kernel itself (the GPU-side Metropolis update step) cannot be verified by Cat 3 v1 per spec § 8.5 ("Stack B Cat 3 v1 tests CPU-side computations only") because there is no headless WebGPU driver in CI. GPU-kernel verification is the v2 candidate per § 13 ("Cat 3 GPU shader coverage via headless WebGPU/Vulkan"). This is a known limitation; the Wolff bond-probability check covers the CPU-side host code that prepares uniforms for the GPU kernel, which is where the most likely bug class lives (β being passed in the wrong units, J's sign being inverted at the host).

### 6.2 Registry entries for the new ground-truth sources

The integrity-toolkit registry at `tools/integrity/docs/ground-truth-sources.md` is *designed* to support two distinct ground-truth shapes, but only one is currently implemented in code:

- **Vendored-upstream entries** (`[SPlisHSPlasH]`, `[Krueger]`) point to source code cloned under `references/<UpstreamName>/` and pinned by SHA. The registry's live fielded schema has six fields, not the four shown in `docs/integrity-toolkit-spec.md` § 6.3 — the spec example is the minimum, but live entries carry `upstream_url` and `used_by_checks` in addition. The parser at `tools/integrity/integrity/cat1_citations/upstream_anchor.py` requires all six fields unconditionally.
- **Algebraic-derivation entries** are defined in `docs/integrity-toolkit-spec.md` Appendix A as a two-field shape: `derivation` (path to a derivation doc) plus `used_by_checks` (list). The spec shows `[Algebraic_Morton30]` as the skeleton example. **However: no algebraic entry currently exists in the live registry, no derivation doc has been authored, and the parser does not branch on entry shape — adding an algebraic entry today would crash `load_registry()` with `KeyError: 'anchor_version'`.** The Krueger entry's prose forward-references `[Algebraic_D3Q19]` as if it were live; it is not. Per `docs/diagnostics/_audits/phase12_prep1_landing_2026-05-15.md:109-111`, `[Algebraic_D3Q19]` is scheduled to land in Phase 12 setup-2, which will include the parser-fix work to support the algebraic-entry shape.

Three of the four proposed Cat 3 checks (`cat3.ising-energy`, `cat3.wolff-bond-probability`, `cat3.onsager-tc`) are closed-form and would naturally use the algebraic-derivation shape. Only `cat3.qubo-ising-roundtrip` needs a vendored upstream. The implication for sequencing is that the algebraic-entry parts of this category's integrity-toolkit amendments cannot land until Phase 12 setup-2 ships the parser fix. The vendored-upstream part (the `dimod` entry) can land any time. § 8 Step 4 has been adjusted accordingly.

**Proposed vendored-upstream entry, matching live six-field schema:**

```toml
[dimod]
anchor_version = "<TBD — pin to current Ocean release at vendoring time>"
anchor_sha     = "<TBD — commit on the anchor version's tag>"
vendor_root    = "references/dimod"
anchor_doc     = ".gitignore"
upstream_url   = "https://github.com/dwavesystems/dimod"
used_by_checks = ["cat1.upstream-citation", "cat1.upstream-anchor", "cat3.qubo-ising-roundtrip"]
```

The vendoring follows the SPlisHSPlasH precedent: shallow clone of the pinned commit, anchor-doc-validated. The Cat 3 check imports `dimod` from the vendored tree (not from the user's pip install) and runs canonical test inputs comparing implementation-side QUBO↔Ising conversions against `dimod`'s reference.

**Proposed algebraic-derivation entries (land after Phase 12 setup-2):**

```toml
[Algebraic_Ising2D]
derivation     = "tools/integrity/docs/algebraic/ising_2d.md"
used_by_checks = ["cat3.ising-energy", "cat3.onsager-tc"]

[Algebraic_WolffCluster]
derivation     = "tools/integrity/docs/algebraic/wolff_cluster.md"
used_by_checks = ["cat3.wolff-bond-probability"]
```

Naming follows the `[Algebraic_Morton30]` precedent from the spec skeleton: `Algebraic_<topic>` rather than `Algebraic_<paper>`. The two derivation docs would contain: (a) the Hamiltonian definition with explicit sign convention; (b) canonical test configurations (all-aligned, alternating, random with fixed seed); (c) expected energies and cluster-add probabilities at those configurations; (d) the Onsager exact value kT_c/J = 2/ln(1+√2) ≈ 2.269185 with derivation sketch, folded into `ising_2d.md` rather than getting its own file (it's part of the canonical 2D Ising story, not a separate subject).

**A subtle complication on the `dimod` vendoring itself.** `dimod` is a normal Python package, not a single-header library. Vendoring it consistently requires either (a) pinning a wheel that the integrity check installs into an isolated venv, or (b) cloning the source and installing it editably during integrity setup. Option (a) is cleaner and matches the SPlisHSPlasH pattern of "pinned binary blob"; option (b) is more flexible if the check ever needs source-level access. Recommend (a) for v1; revisit if needed.

**Two side observations on integrity-toolkit drift, scope: separate.**

- `docs/integrity-toolkit-spec.md` § 6.3 shows the four-field minimum for the registry TOML grammar, but the live registry uses six fields. The spec is out of date relative to its own implementation.
- The Krueger entry's prose comment forward-references `[Algebraic_D3Q19]` as if it were a live registry entry. It is not; it is Phase-12-pending. Harmless but misleading on first read.

Both are docs-drift items against the integrity toolkit, independent of this category-context work. Worth raising as a small `chore:` commit against the integrity toolkit's own maintenance backlog, possibly bundled with the Phase 12 setup-2 parser-fix commit since it touches the same registry surface.

### 6.3 What this is *not*

Statistical validation of D-Wave samples (e.g., "magnetization mean from N reads matches the Onsager analytical curve within 3σ") is **not** Cat 3 material in the current toolkit. It belongs to the deferred Cat 4 runtime-integration category per spec § 13. If at some point the runtime-integration shelf opens, statistical validation against published King-2018 ensemble results becomes the right kind of check; today it does not.

---

## 7. Decisions still outstanding

The coordinator owns these. They block spec-drafting but not category-context-document-drafting (the present doc).

1. **Framing.** A, B, or C from § 4 — or the B-with-C-stretch synthesis proposed in § 5. **Recommendation:** B-with-C-stretch.
2. **Sim-folder structure.** Single `quantum/ising-dwave/` with `web/` and `python/` subfolders (proposed), versus two sibling sims (`quantum/ising-classical/` and `quantum/ising-dwave/`). **Recommendation:** single-folder-with-subfolders, since the shared-viz boundary in § 5.3 means they are genuinely one sim with two implementations rather than two unrelated sims.
3. **Leap Quantum LaunchPad application.** Whether to apply for the 3-month expanded-access program in the window between now and school access landing. Independent of the technical work but bears on Track 2's testing depth before school hardware arrives.
4. **Open-source contribution credit path.** Whether to investigate the per-month earned-credit mechanism for trial accounts. Low priority but free QPU minutes are free QPU minutes.
5. **Hardware specificity.** The school confirmed Advantage2; uncertain whether their working-graph subset matches the nominal 4,400-qubit Zephyr or is a smaller subset (yield variation across QPU instances is real). The context doc is written graph-agnostic and will tighten once the specific solver identifier is known.
6. **Quantum-category-adapted spec template.** Whether to amend the existing `docs/sim-specs/<sim>.md` template globally with conditional sections, or fork a quantum-specific copy at `docs/sim-specs/_template-quantum.md`. **Recommendation:** fork. The existing template is well-suited to GPU sims; introducing conditional branches makes it harder to read for both audiences.

---

## 8. Sequencing recommendation

Conditional on coordinator approval of this document:

→ **Step 1.** Coordinator decides items 1, 2, 6 from § 7. (Items 3-5 can run in parallel and don't block.)
→ **Step 2.** Architect-1 drafts the Quantum-adapted spec template (`docs/sim-specs/_template-quantum.md`), small delta from the existing template per § 1.4, § 1.3, § 1.2. Single architect, no cross-review needed — it's a documentation-only commit.
→ **Step 3.** Architect-1 drafts the full `docs/sim-specs/ising-dwave.md` covering both tracks, using the adapted template. Multi-architect cross-review per the project-state convention for first-of-pattern phases — this is the first Quantum-category spec.
→ **Step 4a.** Architect-1 drafts the `dimod` vendored-upstream amendment to the integrity toolkit (the `[dimod]` entry plus `cat3.qubo-ising-roundtrip` check plus toolkit-spec edits to register it). Lands as a `chore:` commit chain against the integrity toolkit. **Can land any time** — the parser already supports the vendored-upstream shape; this is the smaller of the two integrity-toolkit pieces.
→ **Step 4b.** Architect-1 drafts the algebraic-derivation amendments (the three closed-form Cat 3 entries `cat3.ising-energy` / `cat3.wolff-bond-probability` / `cat3.onsager-tc`, the `[Algebraic_Ising2D]` and `[Algebraic_WolffCluster]` registry entries, the derivation docs at `tools/integrity/docs/algebraic/ising_2d.md` and `tools/integrity/docs/algebraic/wolff_cluster.md`). **Blocked on Phase 12 setup-2** landing the algebraic-entry parser support and the precedent `[Algebraic_D3Q19]` entry per `docs/diagnostics/_audits/phase12_prep1_landing_2026-05-15.md:109-111`. Once that lands, Step 4b is a `chore:` commit chain inheriting the parser fix. Estimated dependency wait: short; Phase 12 setup-2 is already on the active work plan.
→ **Step 5.** Phase N (TBD number per coordinator) — Track 1 ships. Standard Stack B sim phase shape; estimate ~2-3 weeks of architect-1 + reviewer + Claude Code work mirroring the boids-3d phase. The unimplemented-stub `quantum/ising-dwave/README.md` is amended as part of this phase's § 5 cross-cutting edits (status: Implemented, Stack: Stack B WebGPU classical + D-Wave Leap quantum, Build/Performance/References sections fleshed out for the classical side).
→ **Step 6.** Phase N+1 — Track 2 ships against `SimulatedAnnealingSampler`. No QPU dependency. Estimate ~3-4 weeks; the embedding utilities and sampler abstraction are first-of-pattern for Python-on-Leap in this repo. The README is amended again to add the quantum-side build instructions and the `--sampler simulated_annealing` invocation patterns.
→ **Step 7.** School Advantage2 access lands. First real QPU sessions validate Track 2's `--sampler qpu` path against the work it already did against `simulated_annealing`. Cheap if Step 6 was done right; the QPU swap is one line.
→ **Step 8.** Hero output. Either Framing-B's "D-Wave samples panel" rendering or Framing-C's quench-replay figure, depending on coordinator pick at Step 1 and available QPU minutes at Step 7.

Total runway estimate, end-to-end: 4-6 phase-equivalents of work, comfortably within two semesters of school-hardware lead time.

---

## 9. References

Web references retrieved 2026-05-15; full citations carried into the eventual `docs/sim-specs/ising-dwave.md` § 11 at spec-drafting time. License check on any reference code consulted will land at that time per the standard References-section convention.

### Canonical classical Ising

- Onsager, L. (1944). "Crystal Statistics. I. A Two-Dimensional Model with an Order-Disorder Transition." *Physical Review* 65 (3–4), 117–149.
- Metropolis, N., Rosenbluth, A. W., Rosenbluth, M. N., Teller, A. H., Teller, E. (1953). "Equation of State Calculations by Fast Computing Machines." *Journal of Chemical Physics* 21, 1087.
- Swendsen, R. H., Wang, J.-S. (1987). "Nonuniversal Critical Dynamics in Monte Carlo Simulations." *Physical Review Letters* 58 (2), 86–88.
- Wolff, U. (1989). "Collective Monte Carlo Updating for Spin Systems." *Physical Review Letters* 62 (4), 361–364.

### Ising on D-Wave

- Harris, R., et al. (2018). "Phase transitions in a programmable quantum spin glass simulator." *Science* 361 (6398), 162–165.
- King, A. D., et al. (2022). "Coherent quantum annealing in a programmable 2,000 qubit Ising chain." *Nature Physics* 18, 1324–1328.
- King, A. D., Nocera, A., et al. (2025). "Beyond-classical computation in quantum simulation." *Science* eado6285. arXiv:2403.00910 (preprint Mar 2024).
- Vodeb, J., Verstraelen, W., et al. (2025). "Stirring the false vacuum via interacting quantized bubbles on a 5,564-qubit quantum annealer." *Nature Physics* 21, 386.

### D-Wave hardware and SDK

- D-Wave Quantum Inc. (2025-05-20). "D-Wave Announces General Availability of Advantage2 Quantum Computer." Press release. <https://www.dwavequantum.com/company/newsroom/press-release/d-wave-announces-general-availability-of-advantage2-quantum-computer-its-most-advanced-and-performant-system/>
- D-Wave Support. "What Do I Get When I Sign Up for the Leap Service?" <https://support.dwavesys.com/hc/en-us/articles/360003680734>
- D-Wave Support. "D-Wave's Advantage2 Quantum Computer Now Generally Available." <https://support.dwavesys.com/hc/en-us/articles/32105885880087>
- Ocean SDK documentation. <https://docs.ocean.dwavesys.com/>
- `dimod` repository. <https://github.com/dwavesystems/dimod>
- `dwave-samplers` repository. <https://github.com/dwavesystems/dwave-samplers>

---

## Appendix A. Probe items resolved at draft time

These items were verified at draft time via empirical probe (web search dated 2026-05-15) rather than asserted from memory, per the fabrication-discipline convention. Recording here so future readers know what was probed versus inferred:

- **Advantage2 launch date and specs.** Verified via D-Wave press release dated 2025-05-20 and Quantum Computing Report coverage.
- **Zephyr topology connectivity (20-way) and Pegasus connectivity (15-way).** Verified via D-Wave product page and Advantage2 GA documentation.
- **Leap trial-tier QPU allotment (1 minute total, expires 1 month after signup).** Verified via D-Wave support article 360003680734.
- **Ocean SDK module structure (`dimod`, `dwave-samplers`, `dwave.system`, `minorminer`, `dwave_networkx`).** Verified via current Ocean documentation pages.
- **King 2024/2025 Science paper arXiv identifier (2403.00910).** Verified via arXiv listing; preprint dated 2024-03-01, peer-reviewed Science 2025.

## Appendix B. Repo-side probe items (resolved 2026-05-15)

These items required reading the actual repo state rather than the web; resolved via two Claude Code probes dated 2026-05-15.

1. **`docs/sim-specs/ising-dwave.md` exact content.** Resolved: unfilled 11-section template matching `docs/overarching-spec.md` § 7 byte-for-byte, zero substantive content. No change to § 8 sequencing needed.
2. **`quantum/ising-dwave/README.md` exact content.** Resolved: structural twin of `volumetric-grid/lattice-boltzmann/README.md` and `particle-fluids/pic-flip/README.md` — same five-block stub pattern, only Stack, description line, and spec path differ. The "real-time visualization of spin configurations and phase transitions" sentence § 1.2 critiques is an accurate quotation. § 8 sequencing amended to call out README updates explicitly in Steps 5 and 6.
3. **`tools/integrity/docs/ground-truth-sources.md` registry format (vendored-upstream).** Resolved with correction: live registry uses a six-field schema (`anchor_version`, `anchor_sha`, `vendor_root`, `anchor_doc`, `upstream_url`, `used_by_checks`), not the four-field minimum shown in `docs/integrity-toolkit-spec.md` § 6.3. § 6.2 rewritten to match live schema.
4. **Lenia/web+python subfolder precedent.** Resolved with correction: `continuous-ca/lenia-fft/` is python-only at the directory level; the `web/` and `python/` subfolder convention exists across the catalog but has not yet been combined within a single sim folder. § 5.1 rewritten to acknowledge the first-of-pattern status honestly.
5. **Category-context document precedent.** Resolved: no precedent exists; this doc is first-of-shape. Probe-recommended location is `docs/category-contexts/quantum.md` (parallel to `docs/sim-specs/`), not `quantum/ising-dwave/docs/category-context.md` — placing category-scope content under a single sim's folder is naming-incoherent and forces a move if the Quantum category ever grows. This document should land at `docs/category-contexts/quantum.md`.
6. **Algebraic-derivation registry shape (second probe).** Resolved with substantive finding: the algebraic-entry schema is defined in `docs/integrity-toolkit-spec.md` Appendix A as a two-field shape (`derivation` + `used_by_checks`), but no algebraic entry exists in the live registry, no derivation doc has been authored, and the parser at `tools/integrity/integrity/cat1_citations/upstream_anchor.py` hard-requires all six vendored-upstream fields. The Krueger entry's prose forward-reference to `[Algebraic_D3Q19]` is aspirational, not live. `[Algebraic_D3Q19]` is scheduled to land in Phase 12 setup-2 per `docs/diagnostics/_audits/phase12_prep1_landing_2026-05-15.md:109-111`, which will include the parser-fix work to support the algebraic-entry shape. § 6.2 rewritten with the corrected understanding; § 8 Step 4 split into 4a (vendored-upstream, can land any time) and 4b (algebraic-derivation, blocked on Phase 12 setup-2).

## Appendix C. Items surfaced but scope-separate

These were found during probe work but do not block this document. They are integrity-toolkit maintenance items, owned by whoever next touches the toolkit's registry surface.

7. **Integrity-toolkit spec docs-drift (registry schema).** `docs/integrity-toolkit-spec.md` § 6.3 documents the four-field registry minimum but the live registry uses six fields. Spec is out of date relative to its implementation. Worth a small `chore:` commit when convenient — natural bundle candidate with the Phase 12 setup-2 parser-fix commit.
8. **Integrity-toolkit spec/registry drift (forward-references).** The Krueger entry's prose comment in `tools/integrity/docs/ground-truth-sources.md` forward-references `[Algebraic_D3Q19]` as if it were a live registry entry. It is not; it is Phase-12-pending. Spec Appendix A's `[Algebraic_Morton30]` skeleton example similarly does not exist in the live registry. Harmless but misleading on first read — both worth a small `chore:` cleanup either alongside or after Phase 12 setup-2 lands the first real algebraic entry.
