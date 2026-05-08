# Root Context — Distilled

> **Purpose:** Captures the substantive decisions, reasoning, and rejected alternatives from the founding conversation behind `gpu-sims`. Optimized for a future Claude chat to read in one pass before doing further work in this project.
> **Companion document:** `overarching-spec.md` — the authoritative spec. This document explains the *why* behind it.
> **Reading order:** This document first (15 min), then `overarching-spec.md` (20 min). Together they replace the need to read the full founding transcript.

---

## How this repo came to be

The user is building a game (Particle-Sandbox, C++17 + OpenGL 4.3 compute) and wanted to spin off a separate exploration of GPU-based physics and emergence simulations to learn deeper techniques and build a portfolio. The conversation that produced these documents was the second of two: a prior chat with a different Claude instance produced a per-sim toolchain breakdown that was substantively good but recommended Taichi as the unified accelerator across all sims. That recommendation was challenged in the second chat (this one) and refined to a four-stack model. The user agreed with the refinement and asked for an overarching spec to seed downstream chats.

The user's stated goals, in their own framing:
- "Visually stunning by virtue of the high performance simulations it's capable of."
- "Scientific amazement factor — real simulations at maximum scale."
- Cinematic experiences are also wanted, but performance-first is the design philosophy.
- All sims are "flagship" — none are throwaway.
- Long-term: a portfolio website showcasing the demos. Not day-one, but planned for.

---

## The fundamental design decisions, with reasoning

### Decision 1: Four reusable stacks, not a unique toolchain per sim

**The decision:** Standardize on four stacks (Shadertoy / WebGPU / Native C++ / Python) and assign each sim to one or two of them.

**Why:** The first Claude's per-sim toolchain churn would have meant re-learning APIs for every project and produced inconsistent codebases. The four-stack model means shared infrastructure (camera, UI, hot-reload, profiling, export) is reusable across most sims, and the user invests in a small number of skills that compound.

**Rejected alternative:** "Use Taichi for everything." Rejected because: (a) Taichi skills don't fully transfer to native compute or WebGPU, (b) Taichi gives up rendering control which matters for volumetric output, (c) the user already has C++/OpenGL skill from Particle-Sandbox that should compound rather than be set aside.

**Where Taichi *does* win:** MPM specifically. The 88-line MLS-MPM reference is pedagogically perfect; the algorithm is complex enough that fighting GPU APIs while learning it is the wrong battle. So MPM is the *one* primary Stack D commitment, not a default.

### Decision 2: AMD desktop is the primary dev machine, which constrains tooling

**The decision:** No CUDA-only tools as defaults. Vulkan compute, OpenGL compute, and WebGPU are the AMD-friendly paths. CUDA-specific work happens on the lab PC.

**Why:** The user's primary machine is RX 6800 XT on Ubuntu 24. Defaulting to CUDA would mean either rewriting on AMD later or developing only on the lab PC, both of which lose iteration speed.

**Implications that surprised me to think through:**
- NVIDIA Warp is out on the dev machine. (It's CUDA-only.)
- Taichi's Vulkan backend works but is less polished than its CUDA backend. So Taichi-on-AMD is a real but slightly degraded experience. MPM development may want to happen on the lab PC for that reason.
- ROCm/HIP is officially supported on RDNA2 but historically finicky. Available for experiments, not a default.

### Decision 3: SLI does not parallelize compute

**The decision:** Treat the lab PC as 4 independent 11GB cards, not a single 44GB card.

**Why:** This is a hardware fact often misunderstood. SLI (and DirectX/Vulkan multi-GPU) helps graphics under specific configurations; for compute workloads, each card is independently addressable and multi-GPU compute requires explicit domain partitioning written by hand. The user mentioned "4× 2080 Ti SLI" so it was worth flagging this directly so they don't budget against an imaginary 44GB.

**Per-sim implication:** Single-card VRAM budget on the lab PC is 11GB. Aggregate 44GB is only available with explicit partitioning, which is its own engineering project per sim.

### Decision 4: HPC is for hero runs, not iteration

**The decision:** Develop and tune at moderate scale on consumer hardware. Use HPC A100 access for batch jobs that produce maximum-scale offline-rendered showcase output.

**Why:** HPC queueing systems are anti-iteration. The user explicitly asked about both interactive and offline rendering — the answer is "both, but they live in different infrastructure." Consumer hardware for the interactive demo, HPC for the cinematic export.

### Decision 5: D-Wave gets exactly one honest demo, not a pretense of quantum acceleration

**The decision:** Plan an Ising model simulation on D-Wave as a single demo (`quantum/ising-dwave/`). Do not pretend D-Wave accelerates fluid simulations.

**Why:** D-Wave is a quantum *annealer*, not a universal quantum computer. It solves QUBO problems. It cannot accelerate Navier-Stokes, SPH, or MPM — those are continuous PDEs and the entirely wrong shape for the hardware. The honest use is Ising-model spin simulations, which are a direct match to annealer hardware and produce real, scientifically-defensible quantum simulations.

**Why this matters for credibility:** The repo's design philosophy is "scientific amazement through correctness." Forcing quantum hardware into demos where it doesn't help would undermine that philosophy. A real Ising simulation on D-Wave is a stronger demo than a forced quantum-flavored fluid sim.

**Rejected alternatives noted as opportunistic:** QUBO-formulated parameter search for Lenia (probably loses to classical methods, but defensible as a *demonstration* of quantum-assisted search). Optimization sub-problems inside classical sims (occasional, situational).

### Decision 6: Interactive + offline rendering via VDB/Alembic export

**The decision:** Every sim has two output paths from a single simulation codebase. Interactive rendering for development and demos; full simulation state exported to OpenVDB (volumetric) or Alembic (particle/mesh) for offline path-traced rendering in Blender/Houdini.

**Why:** The user asked "can I push the ceiling as high as possible with interactive stuff and then perform offline rendering to show what idealized capabilities it actually has?" The answer is yes, and the standard professional approach is exactly this: cache simulation state in industry formats, render offline at any quality. Adding export to a finished sim is painful; designing for it from day one is easy.

**Why VDB and Alembic specifically:** These are the industry standards. Blender, Houdini, Arnold, Renderman, Karma, Embergen all read them. This means the user gets path-traced multi-scatter cinematic rendering "for free" — the offline renderer is Blender Cycles or Houdini Karma, not a custom path tracer the user has to write.

**Architectural implication:** Per-sim spec sheets must address state export from day one. The data layout should be compatible with the export format from the start.

### Decision 7: MIT license, public from day one

**The decision:** Public GitHub repo, MIT license, with an explicit escape hatch for any single demo that reaches product-shape.

**Why:** The user asked whether this should be open source and whether there's commercial value here. The honest assessment:

- **Code-as-product has no market.** Nobody buys SPH solver source code; better free ones already exist (FluidX3D, SplishSplash, Houdini's free tier).
- **The portfolio as career capital is the dominant value.** Graphics/simulation engineering hires partly on portfolio. A public, polished `gpu-sims` is worth more in career terms than any licensing revenue would be.
- **Specific polished apps could sell.** A Mandelbulb explorer with VR support, an MPM teaching tool — these could go on Steam/itch.io for $5–20. Realistic revenue: a few thousand/year per polished app.
- **Educational content monetizes better than code.** YouTube, courses, blog posts. Public repo is a *feature* for this path.
- **Consulting is downstream of the portfolio.** Public repo lands the contracts.

**The escape hatch:** Any individual demo that becomes a product is forked to a private repo for commercial development. The base research version stays in `gpu-sims/` under MIT. This preserves both the public portfolio value and product optionality.

### Decision 8: Single repo with categorized subfolders, not eight repos

**The decision:** One `gpu-sims/` repo containing all demos as sibling folders under category groupings, with shared infrastructure in `common/`.

**Why:** Shared utilities (camera, hot-reload, ImGui, profiling, export) compound across sims. Eight separate repos would mean reimplementing or re-vendoring these every time. Single repo also means a single GitHub URL for the portfolio, single CI configuration, single license file.

**The category structure (volumetric-grid / particle-fluids / hybrid-particle-grid / continuous-ca / agent-based / closed-form / quantum / render-pipelines)** maps to the natural mathematical categories of the simulations rather than being arbitrary. It will read clearly to anyone browsing the repo and provides a sensible way to add new sims later (a new SPH variant goes under `particle-fluids/`, a new fractal goes under `closed-form/`).

### Decision 9: A handoff chain of three Claude chats

**The decision:** Three distinct chats, each with a clear scope:
1. **Root context chat (this one):** Establishes philosophy, hardware, stacks, sim catalog, cross-cutting architecture.
2. **Architect/coordinator chat (next):** Materializes the repo, defines `common/` API surfaces, drafts per-sim spec sheets, gets build systems running, hands off the first sim.
3. **Per-sim implementer chats (one each):** Build one sim end-to-end against the established conventions and its specific spec sheet.

**Why this structure:** Without an architect chat, each per-sim chat would re-invent shared infrastructure slightly differently and the demos would be inconsistent. Without a per-sim chat per sim, conversations would lose focus across multiple simulation domains. The three-tier structure matches the three distinct types of work: vision, infrastructure, implementation.

---

## Things I considered raising but chose not to push hard on

These are alternatives I noted but didn't recommend over the chosen path. Captured here so the architect chat can reconsider with full context if needed.

### Vulkan vs. OpenGL for native compute
The spec lists both. In practice the user's existing skill is OpenGL 4.3 compute (from Particle-Sandbox), so OpenGL 4.6 compute is the path of least friction. Vulkan compute offers better profiling and explicit synchronization but is significantly more code per dispatch. My recommendation if the architect needs to pick one default: **OpenGL 4.6 compute first**, with the option to migrate specific high-performance sims (SPH, smoke at 512³) to Vulkan if profiling shows the synchronization overhead matters.

### WebGPU via dawn vs. wgpu-native vs. browser-only
For Stack B (web-shareable), browser is the target and dawn-via-Chromium is the reference implementation. For Stack C native cross-platform work, dawn (Google's WebGPU implementation, C++) and wgpu-native (Mozilla's, Rust with C bindings) are both options. **Dawn is the more mature C++ option** and is what I'd suggest as default if the architect wants a unified WebGPU-everywhere story. But OpenGL 4.6 native is a perfectly fine alternative for Stack C and probably less code.

### Houdini in the offline render pipeline
Houdini is the industry standard for VFX simulation and rendering, and produces objectively better cinematic output than Blender for fluid sims. The free Apprentice version is non-commercial-use only with a watermark; the Indie license is $269/year. **Blender is the spec's default** because it's free and Python-scriptable, but if the user has Houdini access (or wants to spend $269/year for hero shots), Houdini's Karma renderer is a step up for cinematic output. Worth flagging as an option for the user to consider per-sim.

### Embergen / FluidX3D as references
Embergen is real-time GPU smoke at very high quality (commercial). FluidX3D is open-source LBM at extreme scale (research-grade). Both are worth studying as reference implementations and as comparisons for benchmarking the user's own work. Not dependencies, but inspiration sources.

### Multi-GPU on the lab PC
The 4× 2080 Ti is mostly under-utilized as a single-card-at-a-time machine. A dedicated multi-GPU project — say, distributing a 1024³ smoke sim across all four cards with halo exchange — would be an interesting flagship in its own right. Not in the current sim catalog because it's an order of magnitude more engineering than the others. **Worth flagging as a stretch project once the basics are working.**

---

## Things the user said that shaped my recommendations

Noted here verbatim or near-verbatim because they're useful for downstream chats to know:

- "I don't mind learning new stacks if it is best suited for the task."
- "I want things to be visually stunning by virtue of the high performance simulations it's capable of."
- "Scientific amazement factor — real simulations at maximum scale."
- "I want to focus on all of them as a flagship."
- "Not that important for the feedback loop [with Particle-Sandbox]; every project should be the best they can be and if overlap happens that's cool."
- On D-Wave: "We're actually going to have access to a quantum computer from D-Wave but that will be for a scheduled amount of time and not something I could iteratively test freely."
- On the website: "A longer term vision was to implement this into a clean front end website... after I had multiple demos to showcase."

These collectively justify the stance that:
- Stack choices are pragmatic per-sim, not ideological.
- Performance is the headline; everything is "flagship"; no half-effort demos.
- Interactive is the primary mode; offline is the showcase mode.
- The website is real but deferred; design for it now, build it later.

---

## What the architect chat should NOT relitigate

Decisions made above are commitments, not suggestions. The architect chat should override these only with explicit cause documented in the spec or a revision to `overarching-spec.md`:

- Four-stack model (A/B/C/D as defined).
- AMD desktop as primary dev machine; CUDA work on lab PC.
- D-Wave gets one Ising demo; not used for fluids.
- Interactive + offline via VDB/Alembic export.
- MIT license, public repo, day-one.
- Single `gpu-sims/` repo with categorized subfolders.
- Sim catalog (additions are fine; removals or recategorizations need cause).
- Per-sim spec template structure.

What the architect *does* own:
- `common/` API surfaces (camera, hot-reload, profiling, state-capture, export).
- Specific library choices (which CMake modules, which file-watcher, which WebGPU implementation, which OpenVDB binding).
- Build system bring-up (CMake structure, Vite config patterns, Python project layout).
- Per-sim spec sheet content (drafting initial drafts; implementers refine).
- Implementation sequencing (the §11 order in the spec is a suggestion; architect can revise).

---

## Open questions to surface back to the user

The architect chat should ask these before locking decisions:

1. **GitHub username/org** for the public repo.
2. **Domain name** for the eventual portfolio website (can defer).
3. **Particle-Sandbox port-in:** Are there existing utilities (camera class, ImGui setup, shader hot-reload) from Particle-Sandbox that should be ported into `common/` rather than rewritten? If yes, those become the baseline.
4. **Houdini access:** Does the user have Houdini Indie or Apprentice? If so, the offline render pipeline for fluid sims gets a meaningful quality upgrade.
5. **Real name vs. handle:** The repo is part of a portfolio; portfolios usually attach to a real name. Is the user comfortable with that, or is there a separation reason?
6. **Existing infrastructure** — is the user working from a fresh repo, or is there a starter scaffold to build on?

None of these block starting work; all of them affect details of the architect's deliverable.
