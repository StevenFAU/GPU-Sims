# Physarum — Load-bearing decisions

This file enumerates the architectural decisions that shape the physarum simulation. Each is **load-bearing** — changing it requires non-trivial refactoring and may break the capture/load schema, the deployed-URL convention, or downstream sim implementations that inherit from this one's patterns.

For decisions that are cheap to revisit (slider defaults, preset values), see the panel; for the v1.1 polish backlog (features intentionally deferred), see `notes.md`.

## 1. Stack B-only — no Shadertoy counterpart

Physarum is the **first sim in the repo to ship without a Stack A artifact**. The `agent-based/physarum/` folder contains `web/` and `docs/` but no `shadertoy/` subdirectory.

Reasoning:
- Shadertoy supports neither storage buffers nor compute shaders. A 10M-agent simulation cannot exist in Shadertoy.
- A "physarum-flavored" Shadertoy demo would simulate a continuum chemoattractant field without discrete agents — a fundamentally different system, not a Stack A version of this one.
- This convention extension applies to future Stack B-originated sims (boids-3d, neural-CA web variant, lenia-fft web variant) that have no clean Stack A expression.

Per project-state.md § 7 (file-layout convention), this is an explicit extension of the existing convention. Captured in the post-Phase-6 project-state.md update.

## 2. Atomic deposits use storage BUFFERS, not storage textures

WebGPU baseline restricts atomic operations to `atomic<u32>` or `atomic<i32>` declared in storage-buffer types. Storage-texture atomics are a proposed extension (gpuweb#4329, native-only in wgpu's `TEXTURE_ATOMIC` feature flag) and are **not portable** to current Chrome/Firefox/Safari.

Three storage buffers, each `gridSize² × 4 bytes`:

```wgsl
@group(0) @binding(N) var<storage, read_write> depositsK: array<atomic<u32>>;
```

Indexed by `cellY * gridSize + cellX`. The agent-move pass writes via `atomicAdd`; the diffuse-decay pass reads non-atomically (compute passes are serialized within an encoder, so the prior write has completed by the time the read runs).

Integer scaling: agents deposit a float `depositAmount` (default 5.0); the value is multiplied by `depositScale = 100` before atomicAdd, giving 500-unit integer increments. The diffuse-decay pass divides by `depositScale` when reading back. This is necessary because `atomic<u32>` requires integer types.

**If WebGPU's `TEXTURE_ATOMIC` proposal lands as baseline in a future revision**, switching to `r32uint` storage textures would eliminate the indexing arithmetic and allow direct cell-address access. Until then, storage buffers are the correct path. v2.x consideration, not v1.1.

## 3. Three species, RGB-per-channel trail texture

The trail texture is `rgba16float`: R = species 0, G = species 1, B = species 2, A = unused. This encoding is load-bearing because:

- Multi-species territorial boundaries become natural color transitions.
- Channel-overlap regions become natural mid-tones.
- Per-species color tints (in the visualize fragment) let users restyle without re-rendering simulation state.
- No external colormap LUT is needed — the per-species color pickers replace it.

Adding a fourth species would require either expanding to two ping-pong texture pairs or switching to a 3D texture; both are architectural increments, not v1 features.

## 4. CPU-side reset path (no GPU init kernel in v1)

The `reseedAgents` function constructs the agent buffer on CPU using xorshift32 with `initSeed`, then uploads via `Buffer.uploadDirect`. At the 10M tier this is a 160 MB upload that takes ~200–500 ms; the UI freezes briefly during reset.

Acceptable for a one-shot operation. Replacing this with a GPU compute kernel that fills the agent buffer in-place is the priority-1 v1.1 polish item.

The CPU-side path is also what makes capture-replay reproducibility work: same `initSeed` + same agent count + same parameters = bit-identical agent layout. A GPU kernel would need to use the same xorshift32 sequence with the same seed-derivation, which is doable but not free; the v1.1 implementation will need to verify cross-platform determinism.

## 5. Persistent food-source pins as the headline interactive moment

Brush-attractant (the rd-2d pattern) was rejected for physarum because trail decay is *definitional* — at default decay 0.03/frame and 60 fps, painted attractant evaporates in ~370 ms. Pins survive decay because they re-deposit each frame from the same location.

Architectural shape:
- Fixed-size 32-pin storage buffer (512 bytes total).
- One additional compute pass (pin-deposit) that scatters contributions into the deposit buffers each frame.
- Conditional dispatch — the encoder records nothing for the pin-deposit pass when `pinCount === 0`.
- Pin visualization (1-pixel white outline rings) is in the visualize fragment, conditional on `pinCount > 0`.

This is the **first sim to use the sparse-source-array compute pattern**. Boids-3D's "leader attractor" feature, eulerian-smoke's fluid emitters, and sph-water's particle emitters will all reuse this shape. Pearson's-law-of-three threshold for promoting `PointEmitterArray` to common-web fires at the third consumer — until then, each sim ships its own per-sim copy.

## 6. Capture stores trail map + RNG seed, NOT literal agent positions

At 10M agents (160 MB), the agent buffer is beyond reasonable browser-ZIP territory. Trail maps are smaller (8 MB at 1024²) and capture the path-dependent history that makes physarum visually interesting.

The capture flow:
- Save: `Texture.readback2D(8)` → ZIP with `trail.bin` + JSON meta containing all params + `initSeed` + pin array.
- Load: `StateReader.fromFile`, restore params, reseed agents from captured `initSeed`, upload trail bytes to ping texture.

**Honest limitation documented in README and notes.md:** loading a capture into a session with a different agent count won't produce literal-identical configuration (different RNG draw count). Same agent count + same seed = bit-identical agent layout.

## 7. Single step per frame (no substepping)

Deliberate departure from rd-2d's substep slider. Agent step at default 1.0 cell/frame × 60 fps = 60 cell/sec; on 1024² that's 6 %/sec per agent — alive but smooth. Substepping would compound bandwidth without compounding visual quality.

The "Sim speed" slider (multiplies step size and turn-angle uniformly, range [0.25, 4.0]) achieves "visit faster" without re-clearing deposits. Effective up to ~4× before visual smoothness degrades.

## 8. Profiler omitted (no `timestamp-query`)

Same posture as Phases 2 / 4 / 5. Per project-state.md § 9 known issue 1, requesting `timestamp-query` would re-introduce the "Buffer is already mapped" warning that the current `GpuProfiler` triggers. CPU-side `performance.now()` deltas suffice for the "is this 60 fps" inspection that the HUD shows. Detailed per-pass timing is a v1.1 polish item, contingent on the `GpuProfiler` rework landing in common-web.

## 9. JSON meta key: `'physarum'`

Cross-stack name parity with `'reactionDiffusion3d'` (Phase 3) and `'reactionDiffusion2d'` (Phase 5). Single-word lowercase no-hyphens for sims with multi-word names follows the same pattern. Future agent sims: `'boidsThreeDimensional'` or just `'boids3d'`? — TBD when boids-3d ships; will be locked then.

## 10. Vite port: 5177

Sequential allocation per project-state.md § 11. Phase 2 = 5173, Phase 4 = 5175, Phase 5 = 5176, Phase 6 = 5177. Phase 7+ continues from 5178.

## Alignment-change-breaks-silently flag (Agent struct)

The `Agent` struct in WGSL is exactly 16 bytes (vec2 pos + f32 heading + u32 species). The TypeScript side uses `ArrayBuffer` with dual `Float32Array` + `Uint32Array` views to write the species field as a `u32` bit pattern at slot index 3.

If a future revision adds a `vec3<f32>` field (alignment 16), the struct alignment becomes 16 and the size becomes 32 bytes (with padding). The TS upload would silently corrupt unless updated. **Audit this when adding any new field to the Agent struct.**

## 11. Workgroup size 256 and raised storage-buffer / buffer-size limits

The 1D-dispatch compute kernels (agent-move and clear-deposits) use `@workgroup_size(256, 1, 1)`, not the more conventional 64. This is load-bearing because of the agent-count tier ceiling.

**Baseline WebGPU limit:** `maxComputeWorkgroupsPerDimension = 65,535`.

**Dispatch math at workgroup size 64:**

| Tier | Workgroups (X) | Within 65,535? |
|------|---------------:|----------------|
| 256k |  4,096 | ✓ |
| 1M   | 16,384 | ✓ |
| 4M   | 65,536 | ✗ — exceeds by 1 |
| 10M  | 156,250 | ✗ |

The 4M default tier would silently fail validation at workgroup size 64. Bumping to 256:

| Tier | Workgroups (X) at 256 | Within 65,535? |
|------|----------------------:|----------------|
| 256k |  1,024 | ✓ |
| 1M   |  4,096 | ✓ |
| 4M   | 16,384 | ✓ |
| 10M  | 39,063 | ✓ |

256 is within the baseline `maxComputeWorkgroupSizeX = 256`. Pin-deposit and diffuse-decay use 2D dispatches `(8,8,1)` and `(16,16,1)` over `gridSize × gridSize` cells; their workgroup-count caps are not in play.

**Storage-buffer binding size.** At 10M agents × 16 B/agent = 160 MB. Baseline `maxStorageBufferBindingSize = 134,217,728 bytes (128 MiB)` and `maxBufferSize = 268,435,456 bytes (256 MiB)`. The 10M-tier agent buffer exceeds the binding-size limit; without raising it, the storage-binding decoration on the agent buffer fails validation.

**`Context.create` requests `requiredLimits: { maxStorageBufferBindingSize: 200_000_000, maxBufferSize: 200_000_000 }`.** Most desktop dedicated GPUs support these (the actual hardware limits are typically 2 GiB+ for storage-buffer binding on RDNA2 / Ada / Apple-Silicon-Pro). Integrated GPUs typically support this too on current drivers; older integrated parts may fall short.

**Failure mode and v1.1 fallback.** If the adapter doesn't grant the requested limits, `Context.create` throws. v1 surfaces this as a HUD message ("WebGPU device unavailable…") and the canvas stays blank. This is acceptable because:

1. The 4M-and-below tiers fit under baseline limits cleanly. A user on hardware that can't handle 10M would notice the failure but can't currently use the panel to drop to a lower tier — the panel never loads on the failure path.
2. The graceful fallback (try 200 MB, fall back to 134 MB if denied; restrict the agent-count dropdown to ≤4M in that case; show a banner near the HUD) is straightforward but adds branching to the boot path.

Flagged as v1.1 priority 1.6 in `notes.md`. The choice of 4M as the default tier was partly motivated by this — a user whose adapter can't host 10M never hits the failure path on first load.

**If the WebGPU spec ever raises baseline `maxStorageBufferBindingSize` to 256 MB or higher**, this discussion becomes obsolete and `requiredLimits` can be dropped. Tracked as a v2.x consideration.

## Convention extensions to track in project-state.md

Phase 6 introduces these conventions that should be reflected in project-state.md after Phase 6 completes:

- **No-Stack-A pattern** (§ 1) — first sim without `shadertoy/`.
- **Atomic-buffer compute idiom** (§ 2) — first sim using `atomic<u32>` storage buffers; will be referenced by future agent and sparse-source sims.
- **Discrete agent-count tiers** — first sim with a tier-dropdown rather than a continuous slider for buffer-size-affecting parameters.
- **CPU-side reset path** (§ 4) — first sim with a buffer-size large enough to cause perceptible UI freeze on reset; documents the polish backlog priority.
- **Sparse-source point-emitter pattern** (§ 5) — first consumer of the food-pin / fluid-emitter / particle-emitter pattern. Promotion threshold to common-web at third consumer.
- **Workgroup size 256 + raised storage-buffer limits** (§ 11) — first sim to hit baseline-WebGPU dispatch and binding-size limits; tier sizing is constrained by these.
