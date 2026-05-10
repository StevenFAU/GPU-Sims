# Physarum

**Multi-species slime-mold transport-network simulation. WebGPU agent system: ~10M agents, three-species mutual repulsion, persistent food-source pins.**

Live demo: <https://stevenfau.github.io/GPU-Sims/physarum/>

The Jones 2010 multi-species model: each agent senses chemoattractant trails ahead, steers toward the strongest gradient, deposits its own trail behind, and repels from the trails of other species. After enough iterations, the agents collectively form transport networks — Tokyo-subway-style routes between food sources, territorial boundaries between species, and emergent shortest-path solutions to nutrient-distribution problems.

## Stack

This simulation is **Stack B-only**. There is no Shadertoy counterpart — Shadertoy lacks storage buffers and compute shaders, making a 10M-agent physarum simulation architecturally impossible there. A "physarum-flavored" Shadertoy demo would simulate a fundamentally different system (continuum chemoattractant fields without discrete agents).

| Stack | Status | Path |
|-------|--------|------|
| A — Shadertoy | Not applicable | — |
| B — WebGPU/TypeScript | ✅ Live | `web/` |
| C — Native Vulkan/CUDA | Not planned | — |

## Controls

| Action | Input |
|--------|-------|
| Place food pin | Left-click on canvas |
| Remove food pin | Right-click within 8 cells of an existing pin |
| Clear all pins | Panel button |
| Save capture (.zip) | F5 |
| Load capture | F9 (file picker opens) |
| Reseed agents | R or panel button |
| Pick preset | Panel "Preset" dropdown |
| Tune any parameter | Panel sliders (flips preset to "Custom") |

## Presets

Six named starting points covering the parameter space:

- **Networks** (default) — transport networks between food sources
- **Snowflake** — tight crystalline patterns with high decay
- **Highways** — broad streams with low decay and long sense distance
- **Conflict** — strong territorial boundaries between species
- **Cooperation** — species merge into shared trails (zero repulsion)
- **Chaos** — diffuse, noisy patterns

Pick a preset, drop a few food pins, and watch the network form. Best demonstrations: place 3–5 pins in a triangle/pentagon, pick "Networks" preset, watch the slime route between them.

## Performance tiers

The "Agent count" panel dropdown controls discrete tiers:

| Tier | Agents | Target hardware |
|------|--------|-----------------|
| 256k | 262,144 | Integrated GPUs (Iris Xe, Vega 8) |
| 1M | 1,048,576 | Mobile dedicated (MX450, Apple M1) |
| **4M** | 4,194,304 | **Desktop default** — M-series MacBook, 6800 XT, 2080 Ti |
| 10M | 10,000,000 | Hero stretch — runs at 60 fps on 6800 XT, ~30 fps on lower dedicated |

The grid-size dropdown (512 / 1024 / 2048) controls the trail-texture resolution independently. Default 1024² works well at all agent counts.

## Capture / load behavior

`F5` saves a `.zip` containing the trail map (rgba16float bytes) plus all parameters and the RNG seed.

`F9` loads a capture: trail bytes restore directly; agents are **reseeded from the captured RNG seed**, not loaded literally. This is a deliberate design choice — the agent buffer at 10M is 160 MB, beyond reasonable browser-ZIP territory, and the trail map captures the more visually-meaningful state (the path-dependent history). Loading produces a visually-identical configuration within ~2 seconds as agents settle into the captured trail topology.

**Caveat:** loading a capture into a session with a different agent count produces a visually-similar but not literally-identical configuration. Same RNG seed + same agent count + same parameters = bit-identical run.

## Architecture

WebGPU compute pipeline graph per frame:

1. **clear-deposits** — zero three `atomic<u32>` deposit buffers (one per species)
2. **agent-move** — sense trails, steer (Jones rule), move, atomicAdd to deposits
3. **pin-deposit** (conditional) — scatter persistent food-pin contributions
4. **diffuse-decay** — 9-tap blur + multiplicative decay, write new trail texture
5. **visualize** — fullscreen blit, RGB-per-species tinted, pin outlines

The atomic deposit buffers are the load-bearing architectural choice: WebGPU baseline does not support atomic operations on storage textures (only on `atomic<u32>` storage-buffer arrays). The deposit buffers are indexed `cellY * gridSize + cellX` and read non-atomically by the diffuse-decay pass after the agent-move pass completes. See `docs/load-bearing-decisions.md` for full discussion.

## References consulted, no code lifted

- Jones, J. (2010) "Characteristics of pattern formation and evolution in approximations of physarum transport networks", *Artificial Life* 16(2).
- Sebastian Lague — "Coding Adventure: Ant and Slime Simulations" (Unity HLSL implementation; MIT-licensed but not source-portable to WGSL).
- Sage Jenson — "physarum" (TouchDesigner; reference for parameter-space exploration).

All shader code is independently authored. The Jones 2010 model is published math.

## Build

```bash
npm install
npm run dev --workspace=@gpusims/physarum-web      # http://127.0.0.1:5177
npm run build --workspace=@gpusims/physarum-web    # production build to dist/
npm run typecheck --workspace=@gpusims/physarum-web
```

Requires Node ≥22, npm workspace at the repo root.
