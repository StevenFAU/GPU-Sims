# Physarum — Notes & v1.1 Polish Backlog

This file tracks features intentionally deferred from v1, ordered by polish-phase priority. Each item is sized to roughly one focused work session (one to four hours).

## Priority 1 — high-impact, low-risk

### 1.1 GPU-side reset compute kernel

**Why now:** the 10M-tier reset path uploads 160 MB via `queue.writeBuffer`, freezing the UI for ~200–500 ms. A GPU kernel that fills the agent buffer in-place using xorshift32 (same RNG sequence as the CPU path) would eliminate the freeze.

**Implementation sketch:** `agent_init.compute.wgsl`, single bind group (uniform Params + agent storage), `@workgroup_size(256, 1, 1)` (matches the production agent-move kernel; same `Math.ceil(agentCount / 256)` dispatch that keeps 4M and 10M tiers under `maxComputeWorkgroupsPerDimension`). Replicate the CPU-side xorshift32 + agent-index seed-derivation. Cross-platform determinism check: compare CPU-init vs GPU-init agent buffers byte-for-byte at the 256k tier.

**Risk:** RNG-determinism difference between CPU and GPU paths could break capture-replay reproducibility for captures saved on one path and loaded on the other. Mitigation: lock GPU init as the only path post-1.1.

### 1.2 Brush-attractant on Shift+LMB-drag

**Why:** complements pins for users who want to "paint" a region of attractant. ~30 lines: a small additional compute pass that adds painted attractant directly to the deposit buffers each frame while LMB is held, using the same falloff math as pins.

**Architectural fit:** uses the existing pin-deposit pattern; doesn't disturb the agent kernel.

**UX:** Shift+LMB-drag to disambiguate from pin-placement (which stays as plain LMB-click).

### 1.3 RMB-as-repellent

**Why:** dual to brush-attractant. While Shift+RMB is held, subtract deposit in a disk around cursor.

**Implementation:** new compute pass or extension of brush-attractant pass; conditional sign on the contribution. (Plain RMB stays as pin-remove.)

### 1.4 Snapshot to PNG ("Save image" button)

**Why:** capture flow saves simulation state, not rendered image. Visitors who want to share a poster shot want a PNG, not a `.zip`.

**Implementation:** read from the swap-chain texture or render to an offscreen RT first; encode via `canvas.toBlob('image/png')`. Exists in mandelbulb-explorer; adapt that helper.

### 1.5 Saturation / gamma on visualization

**Why:** the trail-intensity exposure slider only multiplies; saturation and gamma let visitors tune for poster shots without changing simulation parameters.

**Implementation:** two new sliders in the visualization folder; two extra fragment-shader operations (saturation via lerp(luminance, color); gamma via pow). Mirror mandelbulb's tonemap pattern.

### 1.6 Graceful fallback when 10M-tier limits aren't granted

**Why:** v1 boots fail hard if the adapter can't grant `maxStorageBufferBindingSize: 200_000_000`. On hardware that can only do baseline 128 MiB, the canvas stays blank with a HUD error and the panel never loads — visitor can't drop to a lower tier because the panel isn't there yet.

**Implementation:** wrap `Context.create` in a try/catch (already done in v1 for the error message). On failure, retry with no `requiredLimits`; if that succeeds, restrict the agent-count dropdown to `<= 4M` and show a console warning + a small banner near the HUD ("10M tier unavailable on this GPU; capped at 4M").

**Risk:** none — failure path is strictly better than current. Sized at maybe 30 lines.

## Priority 2 — medium-impact, contained

### 2.1 Per-pin species selection

**Why:** v1 places all-species pins. Per-species pin selection (Shift+click cycles through species masks 1/2/4/3/5/6/7) lets visitors construct asymmetric attractant configurations — e.g., "feed only the red species at this corner".

**Implementation:** the pin storage already has `speciesMask`; the placement UX needs a Shift-click handler and a panel display of "next pin species mask". Add a per-pin-color visualization tweak in the outline render.

### 2.2 Per-species sense angle / sense distance / turn angle

**Why:** v1 has global parameters across all three species. Per-species lets a "predator" species (long sense distance, sharp turn) be tuned distinctly from a "swarm" species (short sense distance, smooth turn).

**Implementation:** triple the sense parameters in the Params uniform (vec3 instead of f32); switch on species in the agent-move kernel. Panel folder gets nine additional sliders. Worth the panel-bloat trade-off because the resulting variety is striking.

### 2.3 Obstacle painting

**Why:** the second half of the Tokyo subway visual is "physarum routing around walls". Add an obstacle texture sampled in the agent-move kernel; agents that step onto an obstacle cell get deflected (heading reflected) or removed (deposited but not moved).

**Implementation:** new r8 storage texture; stamp via brush on Ctrl+LMB-drag; agent-move kernel samples and applies deflect-or-die rule. Architectural change to the agent kernel — bigger increment than the other v1.1 items, hence priority 2.

### 2.4 Autoplay through preset list

**Why:** hands-free demos for portfolio video capture. Cycle through the six presets at user-configurable interval (e.g., 30s each).

**Implementation:** `setInterval` in main.ts, switches dropdown value programmatically. ~20 lines.

## Priority 3 — speculative, may not ship

### 3.1 Bezier-path attractant (Lague-style draw-a-highway)

**Why:** user drags to lay down a polyline that becomes a persistent attractant trail. Produces "physarum reinforcing my drawn path" demo.

**Implementation:** ~80 lines. Polyline editing UX (drag to draw, click endpoints to delete), path rasterization in compute (or pre-rasterize on CPU), attractant deposit each frame along the rasterized path.

**Risk:** UX complexity. Worth doing only if pins + brush prove insufficient for portfolio storytelling.

### 3.2 Adaptive agent count at runtime

**Why:** active-agent count varies during a session — agents that wander into low-deposit regions get culled, new agents spawn near high-deposit regions. Research-grade variant of the model.

**Implementation:** ~150 lines. Indirect-dispatch with a count buffer; compaction pass to keep active agents at the front. Architectural change to the dispatch shape.

**Risk:** large surface area; deferred indefinitely unless a specific portfolio storytelling need surfaces.

## Priority 4 — common-web promotion candidates

### 4.1 Promote `PointEmitterArray` helper to common-web

**Trigger condition:** when boids-3d (Phase 8 likely) needs attractor pins **or** eulerian-smoke (Phase 9+ likely) needs persistent fluid sources.

**Promotion shape:**

```ts
class PointEmitterArray<T extends StructLayout> {
    constructor(ctx: Context, capacity: number, struct: T);
    upload(items: Array<T['fields']>): void;
    bindGroupEntry(binding: number): GPUBindGroupEntry;
}
```

**Out of scope for v1:** the food-pin pattern stays per-sim until the second consumer ships. Promoting too eagerly creates abstraction that constrains the second consumer's needs (a known anti-pattern from Phases 1.5 and 3.5 retrospectives).

### 4.2 Promote agent-buffer scaffolding to common-web

**Trigger condition:** when boids-3d (second agent sim) ships and the agent-buffer init/upload/destroy sequence demonstrably duplicates physarum's.

**Caveat:** boids-3d's agent struct will likely have different fields (velocity vector, neighbor index, etc.). The promotion shape needs to accept a struct-layout descriptor; this is non-trivial. Likely defer to Phase 8 retrospective.

### 4.3 Promote xorshift32 RNG helper to common-web

**Trigger condition:** when a third sim (beyond strange-attractors, rd-2d, physarum) needs deterministic pseudo-random initialization. Current threshold is 3/3 — already past the trigger but blocked on other items.

**Promotion shape:** module-level `createXorshift32(seed: number): () => number` and matching WGSL helper text constant.

## Performance / bandwidth notes

- **10M agents at default settings (1024² grid, 60 fps)** uses ~190 GB/s memory bandwidth. The 6800 XT has 512 GB/s; the 2080 Ti has 616 GB/s; the M2 Pro Max has 400 GB/s. Comfortable on all three.
- **Atomic contention** is the hidden bottleneck. At 10M agents on a 1024² grid, average ~10 agents per cell per step, peaks of 100+ in dense regions. Atomic throughput on Apple Silicon is ~50 GAtoms/s; should be non-binding. AMD RDNA2 atomic throughput is similar; NVIDIA Turing is ~3× higher.
- **Trail-decay computation** is the second hot path. The 9-tap diffuse + decay at 16×16 workgroups dispatches `(64 × 64) = 4096` workgroups, each doing 9 texture samples + 3 buffer reads + 1 storage-texture write. ~2.4 GB/s read, ~0.5 GB/s write at 60 fps. Negligible compared to agent move.

## Known issues / non-issues

### Non-issue: agent count not exactly divisible by 3

256k = 262,144; 262,144 / 3 = 87,381.333 — not exactly divisible. Same for 1M, 4M, 10M. Species split is off by ≤1 agent across species; visually irrelevant. Round-numbered tiers > exact 3-divisibility for portfolio readability.

### Non-issue: integer scaling in atomic deposits

`depositScale = 100` lets atomic ops carry 2 fractional digits of float depositAmount. Default depositAmount = 5.0 → atomic increments of 500. Maximum simultaneous u32 atomic value is 2³² ≈ 4.29 × 10⁹; at 500 units/agent/frame and ~10 agents/cell average, we'd need ~860,000 frames (~14,000 sec at 60 fps) before approaching overflow. The diffuse-decay pass clears the deposit each frame, so practical overflow risk is zero.

### Non-issue: capture-replay agent reproducibility caveat

Loading a capture into a session with a different agent count produces a visually-similar but not literally-identical agent configuration. This is a property of seed-based replay, not a bug. Same seed + same agent count + same parameters = bit-identical run.

### Issue (deferred): UI freeze during 10M-tier reset

~200–500 ms freeze during 10M-tier reseed. Documented in load-bearing-decisions.md § 4. v1.1 priority 1.1.

### Issue (deferred): per-pass timing missing

No `GpuProfiler` instantiated. Per-frame fps from `performance.now()` is available in the HUD, but per-pass breakdown isn't. v1.1 polish item, contingent on `GpuProfiler` rework in common-web (project-state.md § 9 known issue 1).
