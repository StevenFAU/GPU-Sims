# Strange Attractors — Specification

> **Status:** Specification pending — not yet drafted by the architect chat
> **Category:** Closed-form
> **Primary stack:** WebGPU (Stack B)
> **Secondary stack(s):** —
> **Target machine:** Desktop
> **Folder:** [`closed-form/strange-attractors`](../../closed-form/strange-attractors/)

---

## 1. Goal and audience

A portfolio piece demonstrating GPU-parallel ODE integration at scale, in the
browser. The audience is a recruiter or peer engineer landing on the GitHub
Pages site cold. The goal is for the visual to read as "real mathematics
running fast," not as a stylized graphic — every particle is a true RK4
trajectory of the chosen ODE, and the manifold structure they reveal is a
direct visualization of the attractor's invariant set.

The design philosophy is the repo's overarching one (`docs/overarching-spec.md`
§ 1): scientific amazement through physical correctness at maximum scale.
Bloom and tonemap are present but tasteful; the load-bearing aesthetic work is
done by HDR additive accumulation, RK4 integration accuracy, and
velocity-mapped color from a perceptually-uniform LUT.

## 2. Mathematical formulation

Three classic 3D dissipative dynamical systems with chaotic attractors:

**Lorenz** (Lorenz 1963). Standard parameters σ=10, ρ=28, β=8/3:
```
dx/dt = σ·(y − x)
dy/dt = x·(ρ − z) − y
dz/dt = x·y − β·z
```

**Aizawa** (Aizawa 1980s). Parameters a=0.95, b=0.7, c=0.6, d=3.5, e=0.25, f=0.1:
```
dx/dt = (z − b)·x − d·y
dy/dt = d·x + (z − b)·y
dz/dt = c + a·z − z³/3 − (x² + y²)·(1 + e·z) + f·z·x³
```

**Thomas** (Thomas 1999, cyclically symmetric). Parameter b=0.208186:
```
dx/dt = sin(y) − b·x
dy/dt = sin(z) − b·y
dz/dt = sin(x) − b·z
```

**Integrator: classical RK4 with fixed substep dt.** Each frame, every
particle runs N substeps of size `simDt`. N is per-attractor (Lorenz=8,
Aizawa=Thomas=16) and exposed in the panel. Approximation: `simDt` is fixed,
not adapted to local Lipschitz. For these systems at the documented parameter
values, fixed-step RK4 is well within the manifold's quasi-stable basin and
drift over thousands of frames is visually negligible. (Verified by sustained
runs at default settings; particles do not diverge from the attractor.)

Euler at frame rate is rejected — Aizawa and Thomas drift visibly off-manifold
under Euler at 60 Hz, which would violate the "physical correctness" stance.

## 3. Stack assignment and rationale

Stack B (WebGPU + TypeScript). The portfolio's deploy target is GitHub Pages,
and Stack B is the only stack that produces a "send me a link, click it,
demo runs in browser" deliverable. WebGPU's compute shaders are sufficient
for 2M-particle RK4 at 60 fps; the GPU is not the bottleneck at this scale.

Stack C (native Vulkan) is documented as a stretch path for 10M+ particles
where the browser's allocation limits become uncomfortable and the visitor
will install a binary. Phase 2 ships Stack B only.

Rejected stack alternatives:
- Stack A (Shadertoy) — single fragment shader, no compute, no parameter UI;
  unsuitable for million-scale particle ensembles or stateful integration.
- Stack D (Python/Taichi) — no browser deploy story; would only be a research
  prototype, which strange-attractors doesn't need.

## 4. Data structures and memory layout

Per particle: `vec4<f32>` = (x, y, z, speed). 16 B × 2,000,000 = 32 MB.
Storage buffer, `read_write` access in the integrate compute shader, `read`
access in the splat vertex shader.

Accumulation textures: 2× `rgba16float` at viewport × DPR resolution. Cap
DPR at 2.0; at 4K viewport that's 3840 × 2160 × 8 = ~63 MB per texture, ~126
MB combined.

Bloom buffers: 3× `rgba16float` at half-resolution. ~16 MB at 4K viewport.

Colormap LUT: 256×4 RGBA8 = 4 KB. Negligible.

Total VRAM at 4K viewport: ~180 MB. At default 1080p viewport: ~50 MB.

## 5. Per-frame compute pipeline

```
1. Integrate            compute    [positionsBuffer rw, simUniform uniform]
2. Decay                render     accumCurr → accumNext, multiplied by alpha
3. Splat                render     accumNext (load), additive, 6 verts × 2M instances
4. Bloom extract        compute    accumNext → bloomHalf (½ res, soft threshold)
5. Bloom blur H         compute    bloomHalf → bloomHalfH (5-tap separable)
6. Bloom blur V         compute    bloomHalfH → bloomHalfV
7. Tonemap              render     accumNext + bloomHalfV → swapchain (Reinhard)

Swap: pingPongIndex ^= 1; accumA ↔ accumB next frame.
```

Hazards:
- Integrate writes `positionsBuffer` before splat reads it; same encoder, no
  manual barrier needed (WebGPU's queue submission and pass boundaries
  serialize this).
- Decay reads `accumCurr` (the previous frame's output) and writes
  `accumNext`. These are distinct textures so no read-write hazard.
- Splat loads `accumNext` (the just-decayed texture) and additively blends
  on top. Same texture as decay's destination but next pass — pass boundary
  flushes.

## 6. Interactive rendering approach

- HDR accumulation: 2× `rgba16float` ping-pong, decay-then-splat each frame.
- Particle splats: 2 triangles per particle, billboarded around particle
  position, smoothstep-AA'd in fragment, additively blended.
- Color: velocity-magnitude-indexed LUT (256×4 RGBA8), four colormaps.
- Bloom: half-res, soft-threshold extract + 5-tap separable Gaussian.
- Tonemap: Reinhard, exposure-multiplied.
- Camera: auto-orbit by default, free-fly toggle hands to common-web's
  `Camera`.
- UI: lil-gui panel with folders for Attractor, Integration, Rendering, Trail,
  Color, Post, Camera, State. All settings persist via the `ParamPanel`'s
  localStorage backing.

## 7. Offline export path

No VDB or Alembic export — strange-attractors is not a volumetric or particle
fluid simulation; the attractor itself is fully reproducible from
(attractor index, parameters, initial seed, integrator settings).

The F5 capture writes a `strange_attractors_NNNN.zip` containing
`state.json` with the following schema (all fields documented for v1):

```json
{
  "frame": 0,
  "meta": {
    "strangeAttractors": {
      "schemaVersion": 1,
      "camera": { ... },
      "attractor": "lorenz" | "aizawa" | "thomas",
      "params": [/* parallel to attractor's params array */],
      "substeps": 8,
      "simDt": 0.005,
      "pointSize": 1.6,
      "depthAttenK": 0.02,
      "colorSpeedScale": 60,
      "colorExponent": 1.0,
      "colormap": 0,
      "bloomIntensity": 0.15,
      "bloomThreshold": 1.0,
      "bloomSoftKnee": 0.5,
      "exposure": 1.0,
      "alpha": 1.0,
      "decayPreset": "manifold" | "motion" | "custom",
      "autoOrbit": true,
      "orbitSpeedDegPerSec": 6,
      "orbitRadius": 60,
      "initSeed": 12648430,
      "orbitAngleDeg": 142.7
    }
  },
  "buffers": []
}
```

A future `render-pipelines/blender/render_strange_attractors.py` (deferred
to a later phase) consumes this JSON, re-integrates the same ODE with the
same seed, and renders the result via Blender Cycles for hero-still output.
The seed field is included specifically so this re-render is bit-identical
to the in-browser run, not just qualitatively similar.

## 8. Scale tiers

To be measured post-build. Add three rows here for: desktop (RX 6800 XT,
Chromium, default 2M particles, 1080p viewport), high-end (lab PC 2080 Ti),
HPC stretch (Stack C native variant — deferred).

## 9. Stretch goals

- AA line-segment quads in addition to point sprites (sharper edges, more
  CG-leaning aesthetic). v1.1 polish, ~100 lines of additional WGSL + a
  parallel render pipeline.
- More attractors: Halvorsen, Rössler, Chen, Sprott. Each is one new WGSL
  function + one parameter struct in `attractors.ts`.
- Stack C native variant pushing 10M+ particles with the same algorithm.
- Blender re-render script for offline hero stills (file in
  `render-pipelines/blender/`).
- ACES or Hejl-Burgess-Dawson tonemap as an alternative to Reinhard.

## 10. Engineering risks

- **Browser GPU memory limits.** WebGPU's per-allocation cap is roughly
  512 MB on most browsers as of mid-2026. Two 4K rgba16float ping-pong
  + bloom + 32 MB position buffer is well under that, but a future user
  on an exotic browser or display could hit it. Mitigation: DPR cap at 2.0
  is enforced; if needed, fall back to half-res rendering with a config flag.
- **Substep count mismatched to attractor stiffness.** Lorenz at 16 substeps
  is fine; Aizawa at 4 substeps drifts. Defaults are tuned per-attractor;
  user adjustments are exposed but not bounded.
- **Hot-reload of structural shader changes.** v1 logs reload events but
  doesn't rebuild pipelines on structural binding-layout changes — the
  page must be reloaded for those. Pure WGSL math edits work via Vite HMR
  text replacement, but binding additions or layout changes require reload.
  Acceptable for a v1; common-web's pipeline-wrapper layer eventually
  handles this declaratively.
- **Firefox WebGPU on Linux.** Behind `dom.webgpu.enabled` flag as of mid-2026
  (project-state.md § 9). Not a code fix; documented in the README.

## 11. References

- Lorenz, E. N. "Deterministic Nonperiodic Flow." JAS, 20(2), 1963.
- Aizawa, Y. "Symbolic Dynamics Approach to Intermittent Chaos." PTP, 1984.
- Thomas, R. "Deterministic chaos seen in terms of feedback circuits…" IJBC,
  9(10), 1999.
- Inigo Quilez, polynomial colormap fits, Shadertoy, public domain.
- Reference implementations consulted (no code copied): BrutPitt's glChAoS.P
  (MIT), Paul Bourke's strange-attractor pages (educational), merrypranxter's
  strange_attractors (license-checked at lift-time, none lifted).
