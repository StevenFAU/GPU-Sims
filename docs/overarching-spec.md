# gpu-sims — Overarching Specification

> **Status:** Living document. Authoritative source for cross-cutting decisions across all simulation projects in this repo.
> **Audience:** The architect/coordinator chat, individual per-sim implementer chats, and any future contributor.
> **Companion document:** `root-context-distilled.md` — captures the reasoning and rejected alternatives behind the decisions in this spec.

---

## 1. Project goal and design philosophy

`gpu-sims` is a portfolio of GPU-accelerated physics and emergence simulations, each pushed toward the maximum scale and visual quality the hardware can sustain. The unifying philosophy:

**Scientific amazement through physical correctness at maximum scale.** Every simulation should be a real implementation of its underlying mathematics — not a stylized approximation, not a fakery. The "wow" comes from seeing 4M particles obey Navier-Stokes correctly in real time, not from post-processing tricks. When approximations are made, they are documented and justified.

**Interactive prototype, cinematic export.** Every sim runs in real time on consumer hardware at moderate scale, with full interactivity (sliders, mouse interactions, hot-reload). Every sim also exports its state to industry-standard formats (OpenVDB, Alembic) so it can be re-rendered offline at maximum scale on HPC hardware with path-traced lighting. The interactive build is for iteration and demonstration; the offline build is for hero renders and showcase material.

**Performance as a first-class concern.** The repo exists partly as an exercise in pushing GPU performance to the edge. Profiling is built into shared infrastructure. Every sim has explicit scale targets (laptop / desktop / HPC tiers) and treats hitting them as a deliverable.

**Visible engineering.** The repo is public from day one. Commit history, design documents, and per-sim READMEs are all part of the portfolio. The *process* is part of the asset, not just the result.

---

## 2. Hardware reality

The repo will be developed across three machines with different capabilities. Stack and library choices in this spec are constrained by the cross-platform reality below.

### Primary development desktop
- **GPU:** AMD Radeon RX 6800 XT (RDNA2, 16GB VRAM)
- **CPU:** Intel i7-12700KF (20 threads)
- **RAM:** 32GB
- **OS:** Ubuntu 24.04 LTS

**Implications:** No CUDA, no NVIDIA Warp, no Taichi-CUDA backend. The primary development environment is **Vulkan compute, OpenGL 4.6 compute, and WebGPU** (via dawn or wgpu-native). Taichi's Vulkan backend is available with caveats. ROCm/HIP is officially supported but historically finicky on consumer cards — usable for experiments, not the default.

### Lab PC (NVIDIA-native)
- **GPU:** 4× NVIDIA RTX 2080 Ti (11GB each, 44GB aggregate; first-gen RT cores, Tensor cores)
- **CPU:** Intel i9-9820X
- **Caveat:** SLI does not parallelize compute workloads. Each card is independently addressable; multi-GPU compute requires explicit domain partitioning written by hand.

**Implications:** This is the **CUDA development machine**. CUDA, Warp, Taichi-CUDA, OptiX path tracing, Tensor core acceleration are all available. Used for: MPM (Taichi), neural CA training, OptiX-based offline path tracing, anything wanting tensor cores or RT cores. Per-sim VRAM budget is 11GB per card unless explicitly partitioned.

### HPC with A100s (occasional)
- **GPU:** NVIDIA A100 (40 or 80GB VRAM, NVLink, FP64-capable, massive memory bandwidth).

**Implications:** Used for **hero runs only**. Develop and tune at moderate scale on consumer hardware; submit batch jobs at maximum scale to the HPC for offline-rendered showcase output. Not suitable for interactive iteration due to queueing.

### D-Wave quantum annealer (scheduled access)
- **Type:** Quantum annealer, not a universal quantum computer.
- **Suitable workload:** QUBO (quadratic unconstrained binary optimization) problems.
- **Not suitable for:** Navier-Stokes, SPH, MPM, or any continuous-PDE simulation.

**Implications:** D-Wave gets exactly one demo: a 2D/3D **Ising model simulation** (`quantum/ising-dwave/`). This is the natural workload for the hardware and is scientifically defensible. Other quantum-assisted ideas (parameter search for Lenia via QUBO, optimization sub-problems in classical sims) are noted as opportunistic stretch goals, not flagship targets. The repo does not pretend quantum hardware accelerates fluid simulation.

---

## 3. Stack categorization (the four reusable stacks)

Rather than a unique toolchain per sim, the repo standardizes on four stacks. Most sims pick one; a few span two.

### Stack A — Shadertoy
**Purpose:** Pure fragment-shader experiments. Zero local setup, instant iteration, shareable URLs.
**Use for:** 2D reaction-diffusion, fractal explorers, 2D Lenia prototypes, attractor renders.
**Limit:** One fragment shader, no real compute, no CPU-side logic.

### Stack B — Web/shareable (WebGPU)
**Purpose:** 3D, interactive, link-shareable demos. Portfolio website embedding.
**Tooling:** TypeScript + Vite + WebGPU + WGSL. Optional three.js for scene/camera glue.
**Use for:** Anything that should embed in a portfolio website, anything 3D that doesn't need to push hardware ceilings, anything where "send me a link" is more valuable than absolute performance.
**Browser baseline:** Chromium-based browsers (Chrome, Edge, Brave) on desktop. Safari and Firefox WebGPU support are progressing — verify current state per release rather than assuming.

### Stack C — Native performance (C++ / Vulkan / OpenGL)
**Purpose:** Maximum scale, maximum control, the path that compounds with existing C++/OpenGL skill.
**Tooling:** C++20, CMake, GLM for math, ImGui for UI, GLFW for windowing. Compute via OpenGL 4.6 compute shaders or Vulkan 1.3 compute. WebGPU via dawn or wgpu-native is an acceptable variant for cross-platform Stack C work.
**Shader language:** GLSL for OpenGL, GLSL→SPIR-V or HLSL→SPIR-V for Vulkan. WGSL if going through dawn/wgpu-native.
**Use for:** Eulerian smoke at 256³+, SPH water at 4M+ particles, 3D RD at 512³, Lattice Boltzmann, anything where the hardware ceiling is the goal.
**AMD-friendly:** Yes. Vulkan and OpenGL compute work identically on AMD as on NVIDIA. This is the primary reason Stack C is the flagship native path for this repo.

### Stack D — Algorithm-heavy prototyping (Python)
**Purpose:** When the algorithm is the hard part and rendering is secondary. Used surgically, not as a default.
**Tooling:** Python 3.11+, Taichi (Vulkan backend on AMD desktop, CUDA backend on lab PC), JAX or CuPy for research-mode CA work, PyTorch for neural CA training.
**Use for:** MPM (the canonical Taichi MLS-MPM demo and extensions), Lenia FFT-convolution research and parameter sweeps, neural CA training (deployment is in Stack B/C).

### Stack assignments are documented per sim (see §6). Stack overrides are allowed with cause; the cause goes in the per-sim spec

---

## 4. Interactive + offline rendering architecture

This is a cross-cutting architectural decision that shapes how every sim is structured. The same simulation code produces two output paths.

### The pattern

1. **Real-time interactive loop.** GPU compute step + simple rasterized rendering for interactivity. Targets: 60fps at moderate scale on the development hardware. UI, sliders, mouse interaction, hot-reload, profiling overlay. This is what runs during development and during in-person demonstrations.

2. **State export.** A "record" mode dumps full simulation state per frame to industry-standard cache formats. Volumetric grid sims export to **OpenVDB (.vdb)**. Particle and mesh sims export to **Alembic (.abc)**. These formats are the lingua franca of VFX and are read by Blender, Houdini, Arnold, Renderman, Karma, Embergen, etc.

3. **Offline render pipeline.** A separate `render-pipelines/` directory contains Blender Python scripts (and where appropriate, Houdini hip files or OptiX standalone renderers) that consume the cached VDB/ABC sequences and produce path-traced cinematic output. These scripts are headless-renderable and can be submitted as HPC batch jobs for A100 hero runs.

### Why this matters for design

**Adding VDB/ABC export to a finished simulation is painful; designing for it from day one is easy.** Per-sim specs MUST address how state is exported, even if export is implemented as a stretch goal. The data layout chosen for the simulation should be compatible with the export format from the start.

### Library choices

- **OpenVDB:** Use the official C++ library (Academy Software Foundation). For Stack D (Python), use `pyopenvdb` bindings or write VDB through Houdini's hou module if Houdini is available.
- **Alembic:** Official C++ library. `alembic` Python bindings for Stack D.
- **Blender as the default offline renderer:** Free, scriptable in Python, headless via `blender -b`, GPU-accelerated path tracing via Cycles. Works on both AMD (via HIP) and NVIDIA (via OptiX).

---

## 5. Repository structure

```
gpu-sims/
├── README.md                          # Top-level: what this is, demo gallery, links
├── LICENSE                            # MIT
├── docs/
│   ├── overarching-spec.md            # This document
│   ├── root-context-distilled.md      # Companion: reasoning and rejected alternatives
│   ├── stack-decisions.md             # Per-sim stack assignment with justification
│   ├── conventions.md                 # Coding conventions, shader conventions, naming
│   └── sim-specs/                     # Per-sim specification sheets
│       ├── _template.md               # Spec sheet template (see §7)
│       ├── strange-attractors.md
│       ├── mandelbulb-explorer.md
│       └── ...                        # One per sim
├── common/                            # Shared infrastructure
│   ├── camera/                        # Free-fly + arcball camera
│   ├── hot-reload/                    # Shader hot-reload (file-watcher based)
│   ├── profiling/                     # GPU timing wrappers (per-pass ms)
│   ├── state-capture/                 # F5/F9 save/load sim state to disk
│   ├── ui/                            # ImGui setup, common parameter panels
│   ├── export/
│   │   ├── vdb/                       # OpenVDB writers
│   │   └── alembic/                   # Alembic writers
│   └── cmake/                         # Shared CMake modules and toolchain files
├── volumetric-grid/                   # Eulerian sims
│   ├── eulerian-smoke/
│   ├── lattice-boltzmann/
│   └── reaction-diffusion-3d/
├── particle-fluids/                   # Lagrangian sims
│   ├── sph-water/
│   └── pic-flip/                      # Stretch
├── hybrid-particle-grid/              # Hybrid methods
│   └── mpm-multimaterial/
├── continuous-ca/                     # Continuous cellular automata
│   ├── reaction-diffusion-2d/
│   ├── lenia-fft/
│   └── neural-ca/
├── agent-based/                       # Emergent agent systems
│   ├── physarum/
│   └── boids-3d/
├── closed-form/                       # No simulation; pure rendering
│   ├── mandelbulb-explorer/
│   └── strange-attractors/
├── quantum/                           # D-Wave demos
│   └── ising-dwave/
├── render-pipelines/                  # Offline rendering (Blender Python, etc.)
│   ├── blender/                       # Blender scripts that consume VDB/ABC
│   ├── houdini/                       # If/when Houdini is involved
│   └── optix/                         # Standalone OptiX renderers if any
└── web/                               # Future portfolio website (see §10)
    └── (deferred — populated when multiple demos are shippable)
```

### Build system

- **Native (Stack C):** CMake 3.25+. Each sim has its own `CMakeLists.txt` that consumes shared modules from `common/cmake/`. Top-level `CMakeLists.txt` allows building the whole repo or individual sims.
- **Web (Stack B):** Per-sim `package.json` and Vite config. Each web sim is independently buildable and deployable.
- **Python (Stack D):** Per-sim `pyproject.toml` with `requirements.txt` or Poetry/uv lockfile. Virtual environments per-sim are recommended.

### Shared infrastructure conventions

The architect chat owns the API surface of `common/`. Per-sim chats consume it; they do not modify it without coordination. Specifically:

- **Camera class** must support free-fly, arcball, and orbit modes. Used by every native and web sim.
- **Hot-reload** must work for GLSL/WGSL/HLSL files. File-watcher based, with graceful fallback on shader compile errors (keep running with the last good shader).
- **Profiling** must produce per-pass GPU timings printable to overlay and dumpable to CSV for offline analysis.
- **State capture** must use a simple format (JSON metadata + binary blobs) that's restorable across runs and inspectable from Python for analysis.
- **Export (VDB/ABC)** is the cinematic-export path; design the API to be called from any sim's record mode.

---

## 6. Sim catalog

Each sim is listed with its category, primary stack, target machine, and a one-line interactive description. Detailed designs live in per-sim spec sheets in `docs/sim-specs/`.

### Closed-form (no simulation; pure rendering) — warm-up tier
- **Strange attractors** — Stack B (WebGPU) or Stack C — Desktop. 10M particles integrating Lorenz/Aizawa/Thomas ODEs, motion blur, additive blending, slow camera orbit. **First spinoff target.**
- **Mandelbulb explorer** — Stack A (Shadertoy) → Stack B (WebGPU) — Desktop. Free-fly DE ray marcher, soft shadows, orbit-trap coloring, parameter morph animations.

### Agent-based emergence
- **Physarum** — Stack B (WebGPU) — Desktop. 10M agents, 3 species with mutual repulsion, territorial boundary formation. 2D only by design.
- **Boids-3D** — Stack B (WebGPU) — Desktop. 100k boids + 1k predators in a volume, fish-school evasive emergence.

### Continuous cellular automata
- **Reaction-diffusion 2D** — Stack A (Shadertoy) — Anywhere. Gray-Scott pattern explorer, parameter sliders.
- **Reaction-diffusion 3D** — Stack C (native) — Desktop, A100 hero. 256³ to 512³ Gray-Scott "coral", ray-marched iso-surface, paint perturbations and parameter regions.
- **Lenia (FFT-convolution)** — Stack D (research) + Stack B (deploy) — Lab PC for FFT, anywhere for deploy. 2048² real-time with FFT convolution, automated parameter search for stable "creatures."
- **Neural CA** — Stack D (PyTorch training, lab PC) + Stack B (WebGPU deploy) — Train target image growth from seed pixel; deploy interactive damage-and-regenerate demo.

### Volumetric grid (Eulerian)
- **Eulerian smoke** — Stack C (native) — Desktop interactive, A100 hero. 256³ Stam stable fluids with vorticity confinement, single-scattering ray march, moving obstacles.
- **Lattice Boltzmann** — Stack C (native) — Desktop, A100 hero. 512×256×256 D3Q19 LBM around an airfoil, live streamlines.
- (3D RD is listed under continuous CA above; algorithmically related to volumetric grid but categorized by behavior.)

### Particle-based fluids (Lagrangian)
- **SPH water** — Stack C (native) — Desktop interactive, A100 hero. 2–4M particles, hand-rolled GPU spatial hash with Morton sort, screen-space rendering with refraction. **The most engineering-dense mid-tier project on the list.**
- **PIC/FLIP** — Stack C (native) — Stretch goal. Better looking than pure SPH but harder.

### Hybrid particle-grid
- **MPM multi-material** — Stack D (Taichi) — Lab PC primary, A100 hero. Multi-material sandbox: sand + jelly + water in the same scene, 1M particles + 256³ grid. **Sole Stack D primary commitment; the algorithm payoff justifies the side-trip.**

### Quantum
- **Ising on D-Wave** — D-Wave Leap + Stack B (WebGPU visualization) — Scheduled D-Wave time, anywhere for viz. 2D or 3D Ising lattice, real quantum annealing, real-time visualization of spin configurations and phase transitions.

---

## 7. Per-sim spec sheet template

Every sim gets a spec sheet at `docs/sim-specs/<sim-name>.md` before code is written. The architect chat drafts the initial version; the per-sim implementer chat refines it. Required sections:

1. **Goal and audience.** What does this sim demonstrate? Who is the imagined viewer? What feeling should it produce?

2. **Mathematical formulation.** The actual equations being solved, with citations to canonical papers. Approximations explicitly listed and justified.

3. **Stack assignment and rationale.** Which of A/B/C/D, and why this one over the alternatives. Rejected alternatives noted.

4. **Data structures and memory layout.** This is where 90% of GPU performance lives. Describe the buffers, textures, atlas formats, alignment requirements. Include estimated VRAM consumption at each scale tier.

5. **Per-frame compute pipeline.** Sequence of GPU dispatches. Synchronization points. Read/write hazards. For complex pipelines, a diagram.

6. **Interactive rendering approach.** What's drawn each frame, with what shader pipeline. Camera type, UI elements, sliders, mouse interactions, hotkeys.

7. **Offline export path.** How state is captured to VDB/ABC. What the offline render pipeline looks like (Blender? Houdini? OptiX standalone?). Which scenes/shots are the targeted hero renders.

8. **Scale tiers.**
   - *Laptop iteration scale:* what runs at 60fps on integrated graphics or low-end discrete.
   - *Desktop flagship scale:* what runs at 60fps on the RX 6800 XT (or 2080 Ti).
   - *HPC hero scale:* what's possible on an A100 batch run, even if not real-time.

9. **Stretch goals.** Optional improvements that would push performance, quality, or interactivity further. Listed with effort estimates.

10. **Engineering risks.** What's likely to be hard or slow. Where bugs typically hide. What you'll need to profile.

11. **References.** Canonical papers, reference implementations, prior-art demos. Includes license check on any reference code consulted.

---

## 8. Conventions

### Coding
- **C++:** C++20. `clang-format` with the project's `.clang-format`. RAII for GPU resources, no raw owning pointers.
- **Python:** Python 3.11+. `ruff` for linting and formatting. Type hints required for non-trivial functions.
- **TypeScript:** Strict mode. `prettier` + `eslint`. No `any` without justification.
- **Shaders:** GLSL 460 / WGSL / HLSL SM6 as appropriate per stack. Shader files live next to their host code, hot-reloaded.

### Naming
- Sim folders: `kebab-case`.
- C++ classes: `PascalCase`. Free functions: `camelCase`. Constants: `kPascalCase`.
- Shader uniforms/uniforms blocks: `u_camelCase`.
- WGSL/GLSL bind groups documented in shader headers.

### Profiling and performance
- Every native sim integrates `common/profiling/` and shows per-pass GPU times in the ImGui overlay.
- Every sim's spec sheet includes a "performance characterization" section in its README, populated after the implementation is done: actual frame times at each scale tier on the dev hardware.
- Performance regressions are caught by checking these numbers against the baseline before merging.

### Testing
- Native sims include at minimum a "does it launch and run for 60 seconds without crashing" smoke test.
- Algorithm-correctness tests (unit tests for math, conservation tests for fluids) where tractable. Not required, but good when present.

### Documentation
- Every sim folder has its own `README.md`: what it is, how to build/run, controls, screenshots/GIFs, performance numbers, references.
- Top-level `README.md` is a gallery linking to all sim READMEs.

---

## 9. Licensing and visibility

- **License:** MIT for all code in the repo. License header at the top of every source file.
- **Visibility:** Public from day one. The repo is a portfolio asset and the development *process* is part of that asset.
- **Attribution:** Per-sim READMEs credit foundational papers and reference implementations. Reference-implementation licenses are checked before any code is borrowed (most graphics-research code is MIT, but verify per case — Bert Chan's Lenia, Sebastian Lague's Physarum, Taichi's MLS-MPM example, etc.).
- **Commercial escape hatch:** If a specific demo reaches product-shape (a polished standalone app worth selling on Steam/itch.io/App Store), that demo is forked to a private repo for commercial development. The base research version remains in `gpu-sims/` under MIT. This preserves career-capital value of the public portfolio while keeping product upside open.

---

## 10. External presentation surface (deferred)

Long-term vision: a clean front-end portfolio website that showcases the demos.

- **Path:** Built after several demos are shipping. Not a day-one priority, but the repo structure anticipates it (the `web/` folder is reserved).
- **Implementation options:** Likely either (a) a hand-rolled static site (Astro, Next.js) with embedded WebGPU canvases for Stack B demos and GIF/video previews for Stack C/D demos, or (b) Claude-generated design with manual integration. Decision deferred until enough demos exist to motivate it.
- **Domain:** TBD. The website lives at a personal domain; the GitHub repo lives at `github.com/<user>/gpu-sims`.
- **Content per demo:** Embedded interactive (where Stack B), GIF or video preview (always), link to GitHub source, link to per-sim README, hero-render stills from the offline pipeline, brief explanation of the underlying math.
- **Implication for sim development:** Every sim should produce, as part of its "done" criteria, at least one GIF/video preview suitable for embedding and at least one offline-rendered hero still. This is folded into the per-sim spec template (§7, items 7 and 8).

---

## 11. Suggested implementation sequence

A recommended order (not binding; the architect chat may revise based on dependency analysis):

1. **Strange attractors** (warm-up; builds out `common/` against a forgiving sim)
2. **Mandelbulb explorer** (warm-up; exercises ray-marching infrastructure)
3. **Physarum** (first real compute-shader project; builds WebGPU patterns)
4. **Reaction-diffusion 2D** (Shadertoy; teaches fragment-shader iteration)
5. **Reaction-diffusion 3D** (first 3D volumetric; bridges to smoke/LBM)
6. **Eulerian smoke** (first serious native compute project)
7. **MPM multi-material** (Stack D side-trip; algorithmically dense)
8. **SPH water** (the engineering-dense mid-tier flagship)
9. **Lenia + Neural CA** (research-flavored CA work)
10. **Lattice Boltzmann + Boids + Ising-DWave** (parallel extensions)

Each sim's prerequisites are mostly infrastructure (`common/` matures over time) rather than algorithmic dependencies between sims. The order primarily serves as a learning curve.

---

## 12. Handoff notes for the architect chat

This document defines *what* the repo is and *what cross-cutting decisions have been made*. The next chat (architect/coordinator) is responsible for:

1. **Materializing the repo structure.** Creating the directory tree, top-level CMakeLists.txt, the `common/` library skeleton, and the `_template.md` spec sheet at `docs/sim-specs/_template.md`.
2. **Specifying `common/` API surfaces.** Camera class API, hot-reload API, profiling API, state-capture format, VDB/ABC export API. These are the contracts that all per-sim chats will consume.
3. **Drafting per-sim spec sheets.** Using the §7 template, drafting initial spec sheets for each sim in the catalog (§6). Implementer chats refine these but inherit the core decisions.
4. **Build system bring-up.** Getting CMake building a "hello world" native sim and Vite building a "hello world" web sim, both consuming `common/` where applicable.
5. **First sim handoff.** Producing a polished spec sheet for `strange-attractors` as the first implementer-chat target.

**Decisions in this spec are commitments.** Override only with explicit cause documented in the per-sim spec or a revision to this document. The companion `root-context-distilled.md` captures *why* each decision was made, including rejected alternatives, so the architect chat can re-evaluate with full context if needed.

**Decisions explicitly *not* made here, deferred to the architect:**
- Specific camera class API surface (free-fly mechanics, input handling).
- Specific hot-reload mechanism (inotify? polling? library choice?).
- Specific profiling library or hand-rolled wrapper.
- Whether `common/` is a single shared library or per-stack libraries (likely the latter — common-cpp, common-web, common-py — but architect's call).
- CMake module structure for cross-compiling shaders.
- CI/CD setup (GitHub Actions configuration).

Questions for the architect chat to flag back to the user before locking decisions:
- Preferred GitHub username / org for the repo.
- Domain name for the future portfolio website (can be deferred).
- Any existing `common/` code or conventions from Particle-Sandbox that should be ported in rather than rewritten.
