# GPU-Sims

A portfolio of GPU-accelerated physics and emergence simulations — every sim a real implementation of its underlying mathematics, pushed toward the maximum scale and visual quality the hardware can sustain. Interactive prototype, cinematic export.

**Status:** Architecture phase complete. Skeleton in place. Implementation in progress.

> Authoritative spec: [`docs/overarching-spec.md`](docs/overarching-spec.md). Reasoning and rejected alternatives: [`docs/root-context-distilled.md`](docs/root-context-distilled.md).

---

## Gallery

| Sim | Category | Stack | Status |
|---|---|---|---|
| [Strange attractors](closed-form/strange-attractors/) | Closed-form | WebGPU | [Live](https://stevenfau.github.io/GPU-Sims/strange-attractors/) (Phase 2) |
| [Mandelbulb explorer](closed-form/mandelbulb-explorer/) | Closed-form | Shadertoy → WebGPU | [Live](https://stevenfau.github.io/GPU-Sims/mandelbulb-explorer/) (Phase 4) |
| [Physarum](agent-based/physarum/) | Agent-based | WebGPU | [Live](https://stevenfau.github.io/GPU-Sims/physarum/) (Phase 6) |
| [Boids-3D](agent-based/boids-3d/) | Agent-based | WebGPU | [Live](https://stevenfau.github.io/GPU-Sims/boids-3d/) (Phase 7) |
| [Reaction-diffusion 2D](continuous-ca/reaction-diffusion-2d/) | Continuous CA | Shadertoy → WebGPU | [Live](https://stevenfau.github.io/GPU-Sims/reaction-diffusion-2d/) (Phase 5) |
| [Reaction-diffusion 3D](continuous-ca/reaction-diffusion-3d/) | Continuous CA | Native C++ | Implemented (Phase 3) |
| [Lenia (FFT)](continuous-ca/lenia-fft/) | Continuous CA | Python (Taichi) + WebGPU | Not started |
| [Neural CA](continuous-ca/neural-ca/) | Continuous CA | Python (PyTorch) + WebGPU | Not started |
| [Eulerian smoke](volumetric-grid/eulerian-smoke/) | Volumetric grid | Native C++ | Implemented (Phase 8) |
| [Lattice Boltzmann](volumetric-grid/lattice-boltzmann/) | Volumetric grid | Native C++ | Not started |
| [SPH water](particle-fluids/sph-water/) | Particle fluids | Native C++ | Not started |
| [PIC/FLIP](particle-fluids/pic-flip/) | Particle fluids | Native C++ | Stretch |
| [MPM multi-material](hybrid-particle-grid/mpm-multimaterial/) | Hybrid particle-grid | Python (Taichi) | Not started |
| [Ising on D-Wave](quantum/ising-dwave/) | Quantum | D-Wave + WebGPU | Not started |

> Gallery preview images and offline-rendered hero stills will be added per sim as they ship. See [`docs/sim-specs/`](docs/sim-specs/) for per-sim specifications.

---

## The four-stack model

Rather than a unique toolchain per sim, this repo standardizes on four reusable stacks. Most sims pick one; a few span two. See [`docs/stack-decisions.md`](docs/stack-decisions.md) for per-sim assignments.

- **Stack A — Shadertoy.** Pure fragment-shader experiments. Zero local setup, instant iteration, shareable URLs.
- **Stack B — Web/shareable (WebGPU).** TypeScript + Vite + WGSL. Link-shareable 3D demos, embeddable in the future portfolio site.
- **Stack C — Native performance (C++/Vulkan/OpenGL).** Maximum scale and control. The flagship native path. AMD-friendly via Vulkan and OpenGL 4.6 compute.
- **Stack D — Algorithm-heavy prototyping (Python).** Used surgically for MPM (Taichi), Lenia research, and neural CA training.

---

## Hardware tiers

Designed to scale across three machines. See [`docs/hardware.md`](docs/hardware.md) for full details and constraints.

- **Primary dev desktop:** AMD RX 6800 XT (16GB) on Ubuntu 24.04. Vulkan/OpenGL/WebGPU. No CUDA.
- **Lab PC (CUDA):** 4× NVIDIA RTX 2080 Ti (11GB each, *not* aggregated). CUDA, Warp, Taichi-CUDA, OptiX.
- **HPC (hero runs):** A100 batch jobs for offline-rendered showcase output.
- **D-Wave (scheduled):** Quantum annealer, used exclusively for the Ising-model demo.

---

## Build instructions

Build instructions are per-stack and per-sim. See each sim's `README.md` for specifics. Common patterns:

**Native sims (Stack C):**
```bash
cd <sim-folder>
cmake -B build -S .
cmake --build build -j
./build/<sim-name>
```

**Web sims (Stack B):**
```bash
cd <sim-folder>
npm install
npm run dev
```

**Python sims (Stack D):**
```bash
cd <sim-folder>
python -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
python main.py
```

Shared infrastructure under [`common/`](common/) is consumed by native and web sims as a CMake module / TypeScript package. Phase 1 (forthcoming) populates `common/` with camera, hot-reload, profiling, state-capture, and VDB/Alembic export utilities.

---

## Offline rendering

Every simulation exports its state to industry-standard formats (`.vdb` for volumetric grids, `.abc` for particles and meshes). The same simulation code feeds two output paths:

1. **Interactive renderer** for development and live demos (60fps target on the dev desktop).
2. **Offline path-traced renderer** for cinematic hero stills and clips, consuming the cached files. Default: Blender Cycles. With student-tier Houdini Education access, Karma is added for smoke and SPH hero shots — the simulations themselves are renderer-agnostic.

See [`render-pipelines/`](render-pipelines/) for the offline pipeline.

---

## Documentation

- [`docs/overarching-spec.md`](docs/overarching-spec.md) — authoritative cross-cutting spec
- [`docs/root-context-distilled.md`](docs/root-context-distilled.md) — reasoning and rejected alternatives
- [`docs/hardware.md`](docs/hardware.md) — hardware reality and per-machine constraints
- [`docs/stack-decisions.md`](docs/stack-decisions.md) — per-sim stack assignments
- [`docs/conventions.md`](docs/conventions.md) — coding, shader, naming, and profiling conventions
- [`docs/sim-specs/`](docs/sim-specs/) — per-sim specification sheets
- [`CHANGELOG.md`](CHANGELOG.md) — version history
- [`CONTRIBUTING.md`](CONTRIBUTING.md) — how this repo is structured and how to add a sim

---

## How to run

### Native sims (Stack C — Vulkan)

```
sudo apt install libvulkan-dev vulkan-validationlayers spirv-tools glslang-tools \
    libgl1-mesa-dev libxinerama-dev libxcursor-dev libxi-dev libxrandr-dev \
    libwayland-dev libxkbcommon-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPU_SIMS_BUILD_EXAMPLES=ON
cmake --build build
./build/bin/gpu_sims_hello
```

See `common/common-cpp/README.md` for details.

### Web sims (Stack B — WebGPU)

```
node --version    # must be 22 LTS or newer
npm install
npm run dev:hello-web
```

Open http://127.0.0.1:5173. See `common/README.md` for details.

## License

MIT. See [`LICENSE`](LICENSE). Per-sim references and reference-implementation licenses are tracked in each sim's README.

## Citation

If you use or reference this work, see [`CITATION.cff`](CITATION.cff) for citation metadata.

## Author

Steven Cohen — [github.com/StevenFAU](https://github.com/StevenFAU)
