# Hardware reality and per-machine constraints

This document expands [`overarching-spec.md`](overarching-spec.md) § 2 with practical detail. Stack and library choices throughout the repo are constrained by what these machines actually can and cannot do.

---

## Primary development desktop

| Component | Spec |
|---|---|
| GPU | AMD Radeon RX 6800 XT (RDNA2, 16 GB VRAM) |
| CPU | Intel i7-12700KF (20 threads) |
| RAM | 32 GB |
| OS | Ubuntu 24.04 LTS |

**What works here:**
- Vulkan 1.3 compute (primary native compute path)
- OpenGL 4.6 compute (alternative, less code per dispatch)
- WebGPU via dawn or wgpu-native
- Taichi with the Vulkan backend
- ROCm/HIP (officially supported on RDNA2 but historically finicky — usable for experiments, not the default)

**What does NOT work here:**
- CUDA (NVIDIA-only). Anything CUDA-specific must run on the lab PC.
- NVIDIA Warp (CUDA-only).
- Taichi with the CUDA backend (use Vulkan backend instead, slightly degraded).
- OptiX path tracing.

**Implication:** This machine is the daily driver. Sims that target maximum performance on AMD use OpenGL 4.6 or Vulkan compute. Sims that need CUDA-specific tooling (OptiX, NVIDIA Warp) develop on the lab PC instead.

---

## Lab PC (NVIDIA-native)

| Component | Spec |
|---|---|
| GPU | 4× NVIDIA RTX 2080 Ti (11 GB VRAM each, 44 GB aggregate) |
| CPU | Intel i9-9820X |
| Notable hardware | First-gen RT cores, Tensor cores |

**Critical caveat — SLI does not parallelize compute.** Each card is independently addressable by CUDA / Vulkan / OpenGL. Multi-GPU compute requires explicit domain partitioning written by hand. For most sims, treat the lab PC as a single 11 GB card. The 44 GB aggregate is only available with explicit multi-GPU engineering, which is its own project per sim.

**What works here (uniquely):**
- CUDA development
- NVIDIA Warp
- Taichi with the CUDA backend (more polished than Vulkan backend)
- OptiX path tracing
- Tensor-core acceleration (FP16 / mixed-precision)
- RT-core ray tracing

**Use cases:**
- MPM development with Taichi (the CUDA backend is the smoother experience)
- Neural CA training (PyTorch on Tensor cores)
- OptiX-based standalone offline path tracers
- Anything wanting CUDA-specific profiling tools (Nsight Systems, Nsight Compute)

**Per-sim VRAM budget:** 11 GB per card unless multi-GPU partitioning is implemented.

---

## HPC with A100s (occasional)

| Component | Spec |
|---|---|
| GPU | NVIDIA A100 (40 or 80 GB) |
| Interconnect | NVLink |
| Notable | FP64-capable, massive memory bandwidth |

**Use exclusively for hero runs.** HPC queueing systems are anti-iteration. Develop and tune at moderate scale on consumer hardware; submit batch jobs to the HPC for offline-rendered showcase output at maximum scale.

**Workflow:**
1. Run sim on dev desktop or lab PC at moderate scale; verify correctness and aesthetics.
2. Export simulation state to `.vdb` or `.abc` cache files at the highest scale that fits in interactive memory.
3. For hero scale, package the simulation as a headless batch job, submit to HPC, retrieve cache files.
4. Render the cache files offline with Blender (or Houdini/OptiX where applicable). The renderer can also run on HPC as a separate batch job.

---

## D-Wave quantum annealer (scheduled access)

| Property | Detail |
|---|---|
| Type | Quantum annealer (NOT a universal quantum computer) |
| Suitable workload | QUBO (quadratic unconstrained binary optimization) problems |
| Not suitable for | Continuous PDEs (Navier-Stokes, SPH, MPM, smoke, water) |

**The repo's only D-Wave demo is the Ising model** (`quantum/ising-dwave/`). This is the natural workload for an annealer and is scientifically defensible. The repo does not pretend quantum hardware accelerates fluid simulation.

**Opportunistic stretch goals (not flagship):**
- QUBO-formulated parameter search for Lenia (probably loses to classical methods, but a defensible *demonstration* of quantum-assisted search).
- Optimization sub-problems inside classical sims (situational, case-by-case).

Access is scheduled (not on-demand), so iterative testing on D-Wave is limited. Develop and test the QUBO formulation classically first; submit to D-Wave when scheduled time is available.
