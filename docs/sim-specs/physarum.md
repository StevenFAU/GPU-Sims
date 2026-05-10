# Physarum — Specification

> **Status:** Implemented (Phase 6)
> **Category:** Agent-based
> **Primary stack:** B (WebGPU)
> **Secondary stack(s):** None — Stack B-only (no Shadertoy counterpart)
> **Target machine:** Desktop default 4M agents; 10M hero stretch
> **Folder:** [`agent-based/physarum`](../../agent-based/physarum/)
> **Live:** <https://stevenfau.github.io/GPU-Sims/physarum/>

---

## 1. Overview

Multi-species slime-mold transport-network simulation based on Jones (2010). Three agent species lay down chemoattractant trails and steer toward the strongest gradient sensed ahead, with cross-species repulsion producing territorial-boundary formation. Persistent food-source pins (user-placed) provide an interactive moment that demonstrates the canonical "physarum solving the Tokyo subway" routing behavior.

Stack assignment: **Stack B-only.** No Shadertoy counterpart — Shadertoy lacks storage buffers and compute shaders, making a 10M-agent simulation architecturally impossible there.

## 2. Mathematical model

Jones 2010 multi-species Physarum:

- Each agent `i` has continuous position `p_i ∈ [0, gridSize)²`, heading `θ_i ∈ [0, 2π)`, and species index `s_i ∈ {0, 1, 2}`.
- Per-frame update: sense (three points ahead at angles `0, +senseAngle, -senseAngle`), steer (Jones rule: continue if F > L, R; turn ±turnAngle if uncertain or ambiguous), move (`step` cells in the heading direction), deposit (atomic add to species deposit buffer at `floor(p_i)`).
- Trail update: 9-tap diffuse + multiplicative decay; deposit values added before decay applied.

Per-species sensing weighted by:

```
sense(point, species) = trail_self - λ · (trail_other1 + trail_other2)
```

where `λ = repulsionStrength`. λ = 0 → species ignore each other; λ > 0 → species avoid each other's trails.

See `agent-based/physarum/docs/load-bearing-decisions.md` for full implementation discussion.

## 3. Numerical scheme

- **Discretization:** continuous-position agents (f32 pos), discrete-time simulation (one step per frame; no substepping).
- **Grid:** `gridSize × gridSize` cells (default 1024²); periodic boundary via REPEAT-mode sampler addressing for trail reads, and explicit `mod(p, gridSize)` wrap for agent positions before deposit-buffer indexing.
- **Atomic deposit:** integer-scaled (×100) `atomic<u32>` storage-buffer accumulation; non-atomic readback in the diffuse-decay pass (compute passes are serialized within an encoder).
- **Diffusion kernel:** 9-tap (4 cardinal × 1.0, 4 diagonal × 0.7, center × 4.0) / 10.8. Approximates a Gaussian σ ≈ 0.9 cells.
- **Decay:** per-frame multiplicative (`1 - decayRate`).

Stability: forward-Euler not a concern (agent kernel is discrete-time stochastic, not a PDE). Diffuse-decay is a stable smoothing operator at all reasonable parameter values.

## 4. Reference implementations consulted

- Jones, J. (2010) "Characteristics of pattern formation and evolution in approximations of physarum transport networks", *Artificial Life* 16(2). The mathematical model.
- Sebastian Lague — "Coding Adventure: Ant and Slime Simulations" (Unity HLSL implementation; MIT-licensed but not source-portable line-by-line to WGSL). Reference for parameter ranges.
- Sage Jenson — "physarum" (TouchDesigner network; reference for parameter-space exploration and visual aesthetics).
- Memo Akten — "Webcam Slime Mould" (openFrameworks + GLSL; reference for the multi-species extension shape).

**No code lifted.** All shader code in `web/shaders/` is independently authored from the published Jones 2010 model.

## 5. Visualization

RGB-per-species direct blit:

- R channel = species 0 trail concentration
- G channel = species 1 trail concentration
- B channel = species 2 trail concentration
- A channel = unused (reserved)

Per-species color tints (default red/green/blue) multiply the channel values; the three are summed and exposed via the trail-intensity slider before clamping to [0, 1]. Pin outlines (1-pixel white rings) overlay the trail at active food-pin positions, conditional on `pinCount > 0`.

No external colormap LUT — per-species color pickers in the panel replace it. Saturation / gamma controls are deferred to v1.1.

## 6. Capture format

JSON meta key: `'physarum'`.

Fields stored: `presetName`, `agentCountTier`, `gridSize`, all seven tunable shape parameters (sense-distance, sense-angle, turn-angle, step-size, decay-rate, diffuse-weight, deposit-amount, repulsion-strength), `simSpeed`, `initSeed`, `iteration`, `trailExposure`, three per-species color tints, plus the active pin array.

Binary buffers: one — `trail.bin` — containing the rgba16float trail texture bytes (8 bytes per cell, 1024² cells = 8 MB at default grid).

**Agent positions are NOT captured.** On load, agents are reseeded from the captured `initSeed` via the same xorshift32 sequence used at startup. Same seed + same agent count + same parameters = bit-identical agent layout. Loading into a session with a different agent count produces a visually-similar but not literally-identical configuration.

## 7. Offline export path

Not implemented in v1. The capture .zip is the canonical state-at-instant artifact; for offline rendering of the trail field, the rgba16float buffer can be loaded externally and treated as a 2D scalar field per channel. PNG snapshot of the rendered image is a v1.1 polish item (notes.md priority 1.4).

## 8. Scale tiers

| Tier | Memory | Per-frame BW | Target hardware | Default? |
|------|--------|--------------|-----------------|----------|
| 256k | 32 MB | ~5 GB/s | Integrated (Iris Xe, Vega 8) | — |
| 1M | 44 MB | ~20 GB/s | Mobile dedicated (MX450, M1) | — |
| 4M | 92 MB | ~75 GB/s | Desktop default (M-series, 6800 XT, 2080 Ti) | ✓ |
| 10M | 188 MB | ~190 GB/s | Hero stretch (6800 XT / 2080 Ti at 60 fps; lower dedicated at ~30 fps) | — |

## 9. Performance characteristics

Bandwidth-dominated at high agent counts; atomic-throughput-bound in dense regions.

Profiling: per-frame `performance.now()` deltas only (no `timestamp-query`). Per-pass breakdown is a v1.1 polish item, contingent on the `GpuProfiler` rework in common-web (project-state.md § 9 known issue 1).

## 10. Known limitations

- **UI freeze on 10M-tier reset** — ~200–500 ms while the 160 MB agent buffer is constructed CPU-side and uploaded. Documented in `agent-based/physarum/docs/load-bearing-decisions.md` § 4. v1.1 polish: GPU-side init kernel.
- **Agent positions not preserved on capture/load.** Trail map captures simulation history; agents reseed from the captured RNG seed. Same seed + same agent count = bit-identical reproduction; different agent count = visually-similar but not literally-identical.
- **Per-pass timing missing.** No `GpuProfiler` instantiated (timestamp-query is omitted to avoid the "Buffer is already mapped" warning per project-state.md § 9 known issue 1). HUD shows aggregate fps only.
- **Hard fail on adapters that can't grant `maxStorageBufferBindingSize: 200 MB`.** Canvas stays blank with a HUD error message. v1.1 priority 1.6: try/catch fallback that retries at baseline limits and caps the agent-count dropdown at 4M.
- **No Stack A counterpart.** Shadertoy can't host a 10M-agent simulation.

## 11. Post-build verification (interactive)

After Claude Code completes Phase 6, the user's interactive verification checklist:

1. `npm install` at repo root — succeeds without warnings beyond known issues.
2. `npm run typecheck --workspace=@gpusims/physarum-web` — exits 0.
3. `npm run dev --workspace=@gpusims/physarum-web` — starts dev server at http://127.0.0.1:5177.
4. Open in Chrome / Firefox — canvas renders within 2 seconds; HUD shows ~16.7 ms / 60 fps at default 4M tier on a typical desktop.
5. Cycle through all six presets via the "Preset" dropdown — each shows its named pattern type within ~10 seconds.
6. Place 5 food pins in a pentagon — the slime reroutes within ~5 seconds; transport network forms between pins.
7. Right-click a pin — pin removed; routes redistribute.
8. Hit F5 — capture .zip downloads. Hit F9 and load it back — trail and pins restored.
9. Switch to 10M-tier — accept ~500 ms reset freeze; sim continues at ~30 fps on lower-tier dedicated GPUs.
10. Resize browser window — canvas refits without artifacts.
11. Hit `R` — sim reseeds; transport network rebuilds within ~10 sec.

If any step fails, the failure is documented in the post-build report (project-state.md § 9 known issues) and a follow-up commit / patch is scoped.
