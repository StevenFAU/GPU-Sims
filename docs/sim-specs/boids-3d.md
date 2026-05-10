# Boids 3D — Specification

> **Status:** Implemented (Phase 7)
> **Category:** Agent-based
> **Primary stack:** B (WebGPU)
> **Secondary stack(s):** —
> **Target machine:** Desktop interactive
> **Folder:** [`agent-based/boids-3d`](../../agent-based/boids-3d/)

---

## 1. Goal and audience

Multi-species 3D Reynolds flocking with two distinct sparse-source mechanics — persistent leader attractors and dynamic predators — as the headline interactive moments. The visitor sets up a route via leader waypoints, predators harass the flock along the route, and the user watches how the flock adapts.

**Audience:** recruiters and reviewers viewing the GPU-Sims portfolio. The demo demonstrates: (a) GPU-accelerated 3D simulation with non-trivial neighbor queries (spatial hash), (b) a free-fly 3D camera as a portfolio differentiator vs. flat 2D sims like physarum, (c) instanced low-poly rendering with depth-correct visibility, (d) bit-exact-within-one-step capture/load round-trip as a fidelity claim that physarum cannot make.

**Feeling:** organic emergence under pressure. The flock is recognizable as Reynolds boids; the leader-pull adds intentionality (the user is shaping the simulation rather than just watching it); the predator harassment adds dramatic tension (the flock is being *acted upon*, not just acting).

## 2. Mathematical formulation

### 2.1 Reynolds three rules (Reynolds 1987)

For each boid `i` and each boid neighbor `j` within the relevant radius:

- **Separation:** `force_i += (pos_i - pos_j) / max((pos_i - pos_j)², ε)` where `ε = 0.01u²` prevents the singularity at zero distance. Sum over neighbors with `||pos_i - pos_j|| < separationRadius`. Final separation force scaled by `separationWeight / count`.
- **Alignment:** `align_i = average(velocity_j) - velocity_i` over neighbors with `||pos_i - pos_j|| < alignmentRadius`. Scaled by `alignmentWeight`.
- **Cohesion:** `cohesion_i = (average(pos_j) - pos_i)` over neighbors with `||pos_i - pos_j|| < cohesionRadius`. Scaled by `cohesionWeight`.

### 2.2 Leader attraction (custom extension)

For each boid `i` and each leader `L`:

- `to_L = pos_L - pos_i`, `r = ||to_L||`
- If `r < leaderInfluenceRadius`: `leader_force_i += (to_L / r) * leaderStrength * cos((π/2) * r / leaderInfluenceRadius)`
- Cosine-envelope falloff peaks at `r = 0` (force = leaderStrength) and zeros at `r = leaderInfluenceRadius`.

The cosine envelope provides smooth approach behavior — boids passing the influence boundary don't feel a force discontinuity. Applied to all leaders unconditionally (no spatial-hash filter); cap of 32 leaders × 100k boids = 3.2M ops/frame which is negligible.

### 2.3 Predator-flee force (custom extension)

For each boid `i` and each predator `P` within `predatorFleeRadius`:

- `from_P = pos_i - pos_P`, `r = ||from_P||`
- If `0 < r < predatorFleeRadius`: `flee_force_i += (from_P / r) * predatorFleeStrength * (1 - r / predatorFleeRadius)`
- **Linear falloff** (sharper edge than the cosine used for leaders) — produces a more dramatic visible "edge" at the flee-radius boundary, which reads as more striking in the demo.

### 2.4 Velocity update

```
new_velocity_i = clamp_to_max_speed(
    old_velocity_i
    + separation_i + alignment_i + cohesion_i
    + leader_force_i + flee_force_i
)
```

Max speed = `boidMaxSpeed` for boids, `boidMaxSpeed × predatorSpeedMul` for predators (default 1.4× faster).

### 2.5 Predator dynamics — three modes

Predators do not flock with each other. Each predator independently follows one of three runtime-switchable strategies (single-select dropdown):

- **Nearest-prey pursuit (mode 0, default):** target = nearest boid within `predatorDetectionRadius`. Hysteresis: if previous target is still within the radius and within `√4.0u² = 2.0u` of the new nearest distance, keep previous target.
- **Stochastic-prey pursuit (mode 1):** maintain target for `predatorRePickFrames` frames (default 90 ≈ 1.5s @ 60fps), then re-pick a new target via deterministic-modulo selection over candidates within the detection radius.
- **Flock-center pursuit (mode 2):** target = local centroid of all boids within the detection radius (computed from the 27-cell walk).

Predator steer: `velocity += (to_target / dist) * 0.4` per frame, clamped to predator max speed.

### 2.6 Box bounds

Simulation domain is `[-16, 16]³` (32-unit edge length). Bounds enforcement: bounce-back. When position exits a face, position is clamped to the face and the corresponding velocity component is flipped (sign reversed, magnitude preserved). Conservative with respect to kinetic energy.

### 2.7 Approximations

- **27-cell walk for neighbor queries.** Cell size 4.0u (= max neighborhood radius). Every neighborhood-radius slider clamped to ≤ 4.0u so the 3×3×3 cell walk is guaranteed to find every neighbor in the relevant range. Wider walks (e.g., for radii > cell size) are not supported in v1.
- **Deterministic-modulo random-target selection in stochastic-prey mode.** Not a true uniform sample, but visually plausible at the demo scale (1k predators × 90-frame re-pick interval). Banked v1.1 upgrade to reservoir-1.
- **Single integration step per frame, no substepping.** Reynolds dynamics at default parameters are stable at unit time-step; integration error is acceptable for visual purposes. Adding substeps would slow the visible motion proportionally without improving quality.

## 3. Stack assignment and rationale

**Stack B (WebGPU + TypeScript).** The headline differentiator is the *interactive* free-fly 3D camera + click-to-place leaders + dropdown-switchable predator modes — all of which require a hosted-in-browser interactive sim with parameter-panel and click-handler infrastructure. Stack B is exactly this: WebGPU provides the storage-buffer-with-atomic-add primitives needed for the spatial hash; TypeScript provides type-safe panel/click/capture wiring; the `@gpusims/common-web` package provides the Camera + ParamPanel + StateWriter/Reader infrastructure.

**Why not Stack A (Shadertoy):** Shadertoy can't host the spatial-hash compute pipeline (no storage buffers, no compute shaders, no instanced rendering). A "boids-flavored Shadertoy" would be a different sim (low-entity-count, brute-force-neighbors, render-only).

**Why not Stack C (native C++ + Vulkan):** the demo's value is *interactive* with mouse/keyboard/dropdown affordances in a hosted-on-Pages context. Native C++ would require platform-specific binary distribution and add no quality on visuals or simulation depth at this entity count. Stack C remains the right home for the Tier 2 flagship sims (eulerian-smoke, sph-water) where 256³ grids and 4M particles genuinely exceed WebGPU's reasonable limits.

**Why not Stack D (Python + Taichi):** sim-time interactivity in Python is awkward; the value of Stack D is research-mode parameter-sweep work where rendering is secondary. Boids' headline IS the interactive rendering, not the parameter-sweep.

## 4. Data structures and memory layout

### 4.1 Unified entity buffer (ping-ponged)

Two `entityBufA` / `entityBufB` storage buffers, ping-ponged each frame. Each entry is 32 bytes:

```
struct Entity {
    pos:      vec3<f32>   // 0..12
    species:  u32         // 12..16   (0 = boid, 1 = predator)
    velocity: vec3<f32>   // 16..28
    _pad:     f32         // 28..32
}
```

Layout: `[boids in 0..tier.boids, predators in tier.boids..tier.boids + tier.predators]`. Indexing range maps directly to species (`i < tier.boids` ⇒ species 0; otherwise species 1). Buffer size at hero tier 100k+1k: 101k × 32 B = 3.232 MB per ping-pong slot, 6.464 MB total.

### 4.2 Predator state buffer (not ping-ponged)

```
struct PredatorState {
    target_boid_id:    u32   // 0..4    (0xFFFFFFFF = sentinel "no target")
    target_age_frames: u32   // 4..8
    _pad0:             u32   // 8..12
    _pad1:             u32   // 12..16
}
```

Per-predator self-update only (no cross-predator reads), so no ping-pong needed. Size at hero tier: 1000 × 16 B = 16 KB.

### 4.3 Spatial-hash buffers

- `cell_counts[CELL_COUNT]` = 512 × 4 B = 2 KB. Per-frame histogram of entities per cell.
- `cell_starts[CELL_COUNT + 1]` = 513 × 4 B = 2.05 KB. Exclusive prefix-sum (with sentinel slot at index `CELL_COUNT` holding `entityCount`).
- `scratch_counter[CELL_COUNT]` = 512 × 4 B = 2 KB. Per-frame scatter scratch (cleared at frame start).
- `sorted_entity_indices[MAX_ENTITIES]` = 101k × 4 B = 404 KB. After scatter, holds entity indices grouped contiguously by home cell.
- `block_sums[2]` = 8 B. Per-block totals for the multi-block prefix scan.

### 4.4 Leader buffer (CPU-managed)

```
struct Leader {
    position: vec3<f32>   // 0..12
    strength: f32         // 12..16
}
```

Fixed-size 32-entry buffer. Mouse-managed (LMB places, Shift+LMB removes). 32 × 16 B = 512 B.

### 4.5 Total VRAM at hero tier

| Buffer | Bytes |
|---|---|
| Entity ping-pong pair | 6.46 MB |
| Predator state | 16 KB |
| Sorted indices | 404 KB |
| Leaders + cell counts/starts/scratch + block sums | ~7 KB |
| Camera uniform + Params uniform | ~256 B |
| Depth attachment (`depth24plus` at 1080p × DPR 2 = 2160p) | ~33 MB |
| **Total** | **~40 MB** |

Comfortably below baseline `maxStorageBufferBindingSize = 128 MiB` and `maxBufferSize` ceilings. No `requiredLimits` raise needed (notable simplification vs. physarum's 200 MB raise for the 10M tier).

## 5. Per-frame compute pipeline

```
[ CPU ] write entityCount sentinel into cell_starts[CELL_COUNT]
[ CPU ] clearBuffer cell_counts, scratch_counter
[ GPU ] cell_count       -- ceil(entityCount / 256) wg, atomicAdd histogram
[ GPU ] prefix_sum_local -- 2 wg of 256, Blelloch up-sweep + down-sweep
[ GPU ] prefix_sum_block -- 1 wg of 256, scan over per-block totals
[ GPU ] prefix_sum_addback -- 2 wg of 256, write final cell_starts[c]
[ GPU ] scatter          -- ceil(entityCount / 256) wg, atomicAdd scratch + write sorted index
[ GPU ] flock_update     -- ceil(boidCount / 256) wg, Reynolds + leader + predator-flee
[ GPU ] predator_update  -- ceil(predatorCount / 256) wg, 3-mode kernel
[ GPU ] integrate        -- ceil(entityCount / 256) wg, advance + bounce
[ GPU ] render pass --
            background gradient (no depth)
            wireframe box (depth-test, depth-write)
            leaders (instanced 24-vert octahedron, depth-test, depth-write)
            boids+predators (instanced 12-vert pyramid, depth-test, depth-write)
[ CPU ] flip pingIsTarget; check slow-frame degradation streak
```

Total per-frame: 1 buffer write (CPU), 2 buffer clears (GPU encoder), 8 compute dispatches, 1 render pass with 4 draws, 1 boolean flip.

**Synchronization:** WebGPU dispatch ordering provides implicit ordering between compute passes — each pass sees the writes of all prior passes in the same encoder. No explicit barriers needed.

**Read/write hazards:** the read-old-write-new invariant (§ 2.13 of the phase spec) keeps every neighbor-querying kernel race-free. The integrate pass's single read_write binding is the documented exception (per-thread reflexive access only).

## 6. Interactive rendering approach

- **Camera:** common-web's `Camera` in `'free-fly'` mode. WASD move + RMB-drag look + Q/E vertical. Initial pose `[22, 16, 22]` looking at the origin — gives an immediate aerial view of the scene on first load.
- **Render-pass construction:** manual `frame.encoder.beginRenderPass(desc)` with `colorAttachments` + `depthStencilAttachment`. Bypasses `Renderer.beginRendering` which doesn't support depth attachments (deferred to rule-of-three; mandelbulb is consumer #1 of 3D rendering, boids is #2, eulerian-smoke or sph-water will be #3). Depth attachment recreated on canvas resize.
- **Camera uniform:** `viewProj: mat4 (64B) + cameraPos: vec3 (16B) + lightDir: vec3 (12B) + ambient: f32 (4B)` = 96 bytes total. Distinct shape from mandelbulb's basis-vector raymarch packing (boids uses rasterization, not raymarching).
- **Mesh strategy:** instanced low-poly. 4-tri pyramid for boids/predators (4 vertices, 12 indices, apex at +Z) with vertex shader reading per-instance velocity from the entity buffer to derive orientation via Gram-Schmidt with singularity fallback (forward ‖ world_up uses world_forward as up-seed). 8-tri octahedron for leaders.
- **Shader pipelines:** 4 render pipelines (background, wireframe, boid, leader). Boids and predators share one pipeline with per-instance species attribute driving color and scale.
- **Parameter panel:** lil-gui via common-web's `ParamPanel`. Six folders (Scene, Reynolds rules, Leaders, Predators, Visualization, Camera, Seed/Reset) plus top-level Preset dropdown, Save/Load buttons.
- **Click handlers:** LMB places leader on y=0 ground plane via canvas-pixel → world-ray unproject (helper in `web/src/unproject.ts`); Shift+LMB removes nearest within 4.0u. RMB owned by camera-look.
- **Hotkeys:** F5 save, F9 load, R reseed (rising-edge detection mirrors physarum).

## 7. Offline export path

Not in scope for v1. Stack B sims in this gallery do not currently feed the offline render pipeline (`render-pipelines/`). If a hero-render of boids-3d ever becomes desirable, the path is:

1. Capture entity state via F5 → JSON + bin in a ZIP archive.
2. Read the entity buffer in Blender via a Python addon (parses the same schema, instantiates a particle system at the captured positions with the captured velocities driving orientation).
3. Render with Cycles using the same per-species color scheme and 4-tri pyramid mesh.

Implementation deferred. The Stack B live demo is the canonical "this is boids-3d" view.

## 8. Scale tiers

| Tier | Boids | Predators | Total | Avg per-cell | Hardware target |
|---|---|---|---|---|---|
| 25k | 25,000 | 250 | 25,250 | ~49 | Integrated GPU floor |
| 50k (default) | 50,000 | 500 | 50,500 | ~99 | RX 6800 XT, 2080 Ti, M-series Pro/Max — 60 fps comfortable |
| 75k | 75,000 | 750 | 75,750 | ~148 | Mid stretch — 6800 XT 60 fps, lower-tier dedicated GPUs ~45 fps |
| 100k (hero) | 100,000 | 1,000 | 101,000 | ~197 | Overarching-spec ambition; 6800 XT ~50 fps; degradation warning <30 fps |

Tier dropdown shape mirrors physarum's `256k / 1M / 4M / 10M` four-tier pattern. Tier change triggers buffer recreation + reseed (50–100ms hitch; banked v1.1 polish to make this a one-frame uniform write).

## 9. Stretch goals

These are deferred-but-tracked v1.1 polish items — see [`agent-based/boids-3d/web/docs/notes.md`](../../agent-based/boids-3d/web/docs/notes.md) for the prioritized backlog. Highlights:

- Render-style toggle (aquarium ↔ void) — render-pipeline-additive, no simulation changes.
- Vertical leader placement via Shift+LMB+scroll — extends the click-unproject helper.
- GPU-side reseed kernel — eliminates the tier-change UI hitch; cross-references physarum's same-shape polish item.
- Predator user-placement and predator-attracted-to-leaders variant.
- Conditional sort skipping for the per-frame compute graph.

## 10. Engineering risks

- **Spatial-hash early-out hit rate is estimated, not measured pre-execution.** § 2.6 of the phase spec assumes ~95% of cell-walk candidates reject on the cheap distance² check. If the real rate is materially lower (say 80%), default-tier ops/frame roughly doubles. Still fits 60 fps on dev hardware but tighter; v1.1 polish 1.6 instruments and tunes.
- **Workgroup-order non-determinism in stochastic-prey mode** (deterministic-modulo selection chosen specifically to avoid this; reservoir-1 sampling banked as v1.1 if uniformity becomes load-bearing).
- **The radix-sort prefix-sum chain is the highest first-of-pattern-risk module** — three passes (local + block + addback) with explicit pass-by-pass invariants. Off-by-one on block boundaries produces silent neighbor-query failures (sim runs and looks vaguely correct, neighbor queries return scrambled state). Verified during phase execution via architect-2 cross-review of each pass's post-condition.
- **Hero tier (100k+1k) is marginal on dev hardware** — degradation contract logs a console warning if frame rate drops below 30 fps for >60 consecutive frames, but no auto-tier-change. User picks an appropriate tier for their hardware.

## 11. References

- Reynolds, Craig W. "Flocks, herds and schools: A distributed behavioral model." *SIGGRAPH Computer Graphics* 21.4 (1987): 25–34. The canonical boids paper. Math implemented from scratch from the published equations; no third-party code consulted (license incompatibility with the repo's MIT).
- Reynolds' original boids page: <https://www.red3d.com/cwr/boids/>. Reference for the three-rule formulation and visual character.
- Sebastian Lague's "Coding Adventure: Boids" (Unity HLSL implementation) — referenced for general structural inspiration (multi-species + predator concept). License: MIT but Unity-specific; no code lifted to WGSL.
- "Memo Akten — Forms" (Reynolds-derived predator-prey installations) — referenced for the visual character of predator-flee dynamics. No code; aesthetic reference only.
- The WebGPU Specification, W3C Editor's Draft. Specifically: storage-buffer atomicAdd semantics, depth-stencil-attachment requirements, and the constraint that same-buffer aliased read/write decorations within one bind group are rejected (the load-bearing reason integrate uses a single `read_write` binding).
- CUB / RocPRIM radix-sort reference implementations — referenced for the multi-block prefix-scan pattern (3 passes with N+1 sentinel allocation). Implementation written from the published pattern; no code lifted.
