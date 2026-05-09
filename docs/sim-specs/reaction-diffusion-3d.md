# Reaction-Diffusion 3D — Specification

> **Status:** Specification pending — not yet drafted by the architect chat
> **Category:** Continuous CA
> **Primary stack:** C (Native C++)
> **Secondary stack(s):** —
> **Target machine:** Desktop interactive, A100 hero
> **Folder:** [`continuous-ca/reaction-diffusion-3d`](../../continuous-ca/reaction-diffusion-3d/)

---

## 1. Goal and audience

A portfolio piece demonstrating GPU-parallel volumetric reaction-diffusion at
real-time framerates in 3D, with a UX that gets a non-expert visitor to a
recognizable pattern type within seconds. The audience is a recruiter or peer
engineer who has heard of reaction-diffusion in 2D (familiar from
Turing-pattern demos) and wants to see what it looks like in 3D when the math
is honored and the scale is real.

The design philosophy mirrors the strange-attractors sim:
scientific amazement through physical correctness at substantial scale. The
patterns visible at default settings are real Gray-Scott trajectories on a
genuinely-periodic 3D grid, not a stylized look-alike. Pearson's 1993 regions
are the headline because they're the literature-canonical way to talk about
this parameter space; visitors who know the references see them in the panel.

## 2. Mathematical formulation

Gray-Scott model on `[0, 1]³` with periodic boundary conditions:

```
∂u/∂t = Du · ∇²u − u·v² + F · (1 − u)
∂v/∂t = Dv · ∇²v + u·v² − (F + k) · v
```

`u` and `v` are dimensionless concentrations of two reacting chemicals. `Du`,
`Dv` are diffusion coefficients (canonical: `Du = 0.16`, `Dv = 0.08`). `F` is
the feed rate of `u`; `k` is the kill rate of `v`. The Pearson 1993 named
regions are particular (F, k) values where the system stably produces distinct
pattern types.

**Discretization:**
- Time: Forward Euler with normalized `dt = 1`. Stable for canonical Du/Dv at
  Pearson region parameters.
- Space: 7-point Laplacian stencil (center + 6 face neighbors) with normalized
  `dx = 1`. Periodic boundary at the sampler level
  (`VK_SAMPLER_ADDRESS_MODE_REPEAT` + NEAREST filter).
- Per-frame substepping: `SUBSTEPS_DEFAULT = 4`; slider 1–32. Higher = pattern
  formation faster.

ADI / implicit / IMEX schemes are textbook overkill for the Pearson regions.
RK4 is unnecessary — Forward Euler is the standard choice for Gray-Scott.

## 3. Stack assignment and rationale

Stack C (native Vulkan 1.3 + C++20). The portfolio's "first real Stack C sim"
slot. Reasons Stack C beats Stack B for this:

- 256³ field memory at FP32 = 64 MB per buffer × 4 buffers = 256 MB. Fine on
  desktop WebGPU but uncomfortably close to browser allocation ceilings if a
  visitor is on a tablet GPU. Native lifts that.
- 60 fps × 4 substeps × 256³ is ~257 GB/s memory bandwidth — well below
  6800 XT's 512 GB/s. Native Vulkan's lower abstraction overhead matters less
  than the bandwidth headroom; both stacks would handle this fine in
  principle, but Stack C is what we have to validate next anyway.
- 512³ stretch at ~2 GB+ field memory clears the "comfortably native" bar and
  starts to bump web-sim ceilings.

Stack A (Shadertoy) — single fragment shader, no compute kernel, can't do RD
update without absurd ping-pong-fragment-shader contortions; rejected.

Stack D (Python/Taichi) — possible alternative path; Taichi handles 3D RD well
and the JIT runs respectably. Not chosen because Stack D infrastructure
(common-py) doesn't exist yet (Phase 6) and reaction-diffusion-3d's job is
specifically to exercise common-cpp.

## 4. Data structures and memory layout

**Fields:** two scalar concentrations `u`, `v`, both stored as
`VK_FORMAT_R32_SFLOAT` 3D images at 256³. Each field has a ping-pong pair
(`u_ping` / `u_pong`, `v_ping` / `v_pong`) for read-write hazard isolation.
Total: 4 × 64 MB = 256 MB of field memory.

**Storage usage:** `STORAGE_BIT | SAMPLED_BIT | TRANSFER_SRC_BIT |
TRANSFER_DST_BIT`. Sampled binding for the compute kernel's neighbor reads
(REPEAT + NEAREST sampler), storage binding for writes (no sampler), transfer
bits for F5 readback and F9 upload.

**Uniform buffers:** `RdUniforms` (32 B; Du, Dv, F, k, dt, gridSize, padding)
and `RaymarchUniforms` (~144 B; invViewProj, cameraPos, volumeMin/Max, step
count, density transfer, colormap index, exposure). One per in-flight slot to
avoid host/GPU race when overwriting per-frame.

**Colormap LUT:** 256×4 `VK_FORMAT_R8G8B8A8_UNORM` 2D image, one row per
colormap. 4 KB. Same Inigo Quilez polynomial fits as Phase 2's
strange-attractors, baked at startup.

**Per-frame heap traffic:** zero — all per-frame work writes through the
existing storage textures. The compute kernel is bandwidth-bound on the
field reads/writes; the raymarch kernel is bandwidth-bound on volume sampling
plus the small LUT lookup.

## 5. Per-frame compute pipeline

```
1. Integrate substeps × N    compute    rd_update.comp
                                        Reads u_curr/v_curr, writes u_next/v_next.
                                        Swap ping/pong each substep.
                                        N = 4 default; slider 1–32.
                                        Profiler scope: "substep" (one per substep).

2. Raymarch                  graphics   fullscreen.vert + raymarch.frag
                                        Reads v field (post-substep), LUT.
                                        Writes swapchain (HDR -> Reinhard tonemap inline).
                                        Profiler scope: "raymarch".

3. ImGui                     graphics   ImGui glue inside same render pass.
                                        Profiler scope: "imgui".
```

Hazards (Vulkan requires explicit pipeline barriers — see § 2.15 of the phase
spec for the full rationale and main.cpp for the call sites):
- Inter-substep: each substep writes u_next/v_next via storage-image stores;
  the next substep reads those same images via combined-image-sampler. A
  pipeline barrier with src `COMPUTE_SHADER` / `SHADER_STORAGE_WRITE` and dst
  `COMPUTE_SHADER` / `SHADER_SAMPLED_READ` is required between substeps.
- Substep-to-raymarch: after the final substep, the fragment shader reads the
  v field. A barrier with src `COMPUTE_SHADER` / `SHADER_STORAGE_WRITE` and
  dst `FRAGMENT_SHADER` / `SHADER_SAMPLED_READ` is required before the
  raymarch draw.
- Field images stay in `VK_IMAGE_LAYOUT_GENERAL` for the lifetime of the
  program (legal for both storage and sampled bindings; minor perf delta
  vs optimal layouts that we accept for simplicity). Memory barriers
  (`VkMemoryBarrier2`), not image-memory-barriers, suffice; main.cpp uses
  `gv::memoryBarrier` (added to common-cpp during Phase 3).

## 6. Interactive rendering approach

- Volume raymarch on a fullscreen triangle, ray-AABB intersection, fixed-step
  DDA (96 steps default).
- Front-to-back compositing: closer features occlude farther ones.
- Density transfer: `smoothstep(threshold, threshold + 0.1, v) * intensity`.
- Color: 256-row LUT sampled by `v` (not by density), four colormaps shipped.
- HDR + Reinhard tonemap inline in the raymarch fragment shader.
- Bloom is reserved as a uniform but disabled by default (intensity = 0); v1.1
  may add Phase 2-style HDR ping-pong + half-res bloom if the look needs it.
- Camera: auto-orbit by default at 1.5 world units radius around the
  unit-cube volume center; free-fly toggle hands input to common-cpp's Camera.
- UI: ImGui panel with sections for Preset, Parameters, Integration,
  Rendering, Color, Camera, State, Profiler.

## 7. Offline export path

F5 captures a complete state snapshot to `captures/capture_NNNN/`:

```
state.json    (camera + meta + parameters + schema version)
u.bin         (256³ × 4 bytes raw float, x-major)
v.bin         (256³ × 4 bytes raw float, x-major)
```

Total ~134 MB per capture. Heavy but worth it: the capture is a full state
snapshot any external tool can consume without re-implementing the solver.

A future `render-pipelines/blender/render_reaction_diffusion.py` (deferred to
a later phase) consumes `state.json` + `v.bin` and renders a Cycles
volumetric still. The Blender script is not part of Phase 3.

## 8. Scale tiers

To be measured post-build. Add three rows here for: desktop default (RX 6800
XT, 256³, 4 substeps, 1080p target), 256³ stretch (16 substeps), 512³ stretch
(4 substeps, "is it still 60 fps?").

## 9. Stretch goals

- GPU isosurface extraction (marching cubes) as an alternative to raymarch.
  Hundreds of lines of scan/compaction work; v1.1 candidate if the look needs
  sharper edges.
- Phase 2-style HDR ping-pong + half-res bloom + decoupled tonemap pass.
- More named presets (Munafo's RD parameter map has dozens of labeled regions).
- Stack D (Taichi) port for benchmarking against the same algorithm in a
  different runtime.
- VDB export — when eulerian-smoke (Phase 5) lands the real OpenVDB writer,
  reaction-diffusion-3d gets a "save as VDB" alongside its raw-binary capture.
- A 1024³ A100-only run, with measured numbers.

## 10. Engineering risks

- **Forward Euler stability at slider extremes.** Within the documented
  Pearson regions and at canonical Du/Dv = (0.16, 0.08), Forward Euler is
  stable. Push F or k far outside the documented ranges and the integrator
  can blow up. Mitigation: per-cell clamp `[0, 2]` on u and v in the kernel
  catches the worst extremes; sliders bound to literature-typical ranges.
- **Boundary-condition gotcha.** Periodic via REPEAT sampler is correct for
  the kernel's sampled reads, but if a future architect adds a non-sampled
  read path (e.g., `imageLoad` with explicit modulo), the boundary handling
  must match. Documented in `docs/load-bearing-decisions.md`.
- **GpuProfiler ring-buffer correctness.** First real consumer of common-cpp's
  GpuProfiler. If timestamp readback is mis-aligned, scope timings will look
  noisy or off-by-frames. Documented as a Phase 3 verification target.
- **Linux + GLFW + X11.** Per project-state.md § 4 row 9, common-cpp pins X11
  even on Wayland sessions. Phase 3 inherits this; no change needed here.

## 11. References

- Pearson, J. E. "Complex Patterns in a Simple System." *Science* 261:189–192,
  1993.
- Munafo, R. "Xmorphia: Reaction-Diffusion explorer."
  https://mrob.com/pub/comp/xmorphia
- Quilez, I. "GPU colormap polynomial fits." Shadertoy WlfXRN, public domain.
- Crane, K. "Discrete Differential Geometry." (Discretization references.)
