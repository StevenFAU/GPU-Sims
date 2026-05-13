# Tier 1 Agent — Capture Format Reference

> Companion to `diagnostics-overview.md`. Hand this to the Tier 1 agent alongside the overview, project-state, and overarching-spec. This document is descriptive (what the repo actually emits today), not prescriptive.

---

## 1. Top-level `meta` keys observed across shipped sims (8 rows below)

Every shipped sim writes **exactly one** sim-namespaced top-level key in `state.json.meta`. The key is the activation signature for the Tier-3 module that diagnoses that sim.

> **Note (Phase 11):** This table currently lists sims through Phase 8 plus Phase 11 (sph-water). Phase 9 (mpm-multimaterial) and Phase 10 (lenia-fft) rows are scheduled for a separate ledger-backfill commit per `phase11_deferred_backfill.md` Item 1. Their top-level meta keys are `mpmMultimaterial` and `lenia` respectively; consult their CHANGELOG entries / project-state ledger for canonical confirmation.

| Sim | Phase | Stack | Top-level meta key |
|-----|-------|-------|--------------------|
| strange-attractors | 2 | B (TS) | `strangeAttractors` |
| reaction-diffusion-3d | 3 | C (C++) | `reactionDiffusion3d` |
| mandelbulb-explorer | 4 | B (TS) | `mandelbulbExplorer` |
| reaction-diffusion-2d | 5 | B (TS) | `reactionDiffusion2d` |
| physarum | 6 | B (TS) | `physarum` |
| boids-3d | 7 | B (TS) | `boids3d` |
| eulerian-smoke | 8 | C (C++) | `eulerianSmoke` |
| sph-water | 11 | C (C++) | `sphWater` |

Convention: camelCase, named after the sim. Future sims should follow this.

---

## 2. Complete `saveBuffer` enumeration across all 7 shipped sims

### Non-pixel-format buffer convention

For SSBOs whose contents don't map to a single Vulkan pixel format — packed-struct per-particle / per-entity buffers — the per-buffer `format` meta key takes one of:

- `"raw"` — the buffer is a contiguous byte sequence; consumers read per-field offsets from a sim-specific layout document. Phase 11 sph-water uses this for its 128-byte-per-particle struct; field offsets are at `particle-fluids/sph-water/shaders/_struct_layouts.txt`.
- `"packed_struct"` — synonym for `"raw"` when struct identity matters more than the byte layout. Phase 7 boids-3d's `entities` is the historical precedent.

In both cases, `"shape"` is a 2-element array `[count, stride]` rather than a 3-element pixel-format shape `[width, height, depth]`. Tier-3 diagnostics branch on `format == "raw" || format == "packed_struct"` to dispatch the right read path.

### strange-attractors (Phase 2)
**No `saveBuffer` calls.** Meta-only capture. The trajectory is parameter-driven; given the same seed and params it re-derives. Tier-3 has no buffer data to analyze unless this sim adds a diagnostic-mode dump.

### mandelbulb-explorer (Phase 4)
**No `saveBuffer` calls.** Meta-only capture. Render-only sim (raymarcher, no persistent state). No per-pixel escape data captured. Same coordinator decision needed as strange-attractors.

### reaction-diffusion-3d (Phase 3, Stack C)
```cpp
state_writer.saveBuffer("u", u_buf.data(), N * sizeof(float), {
    "count":  N,                     // GRID_SIZE^3
    "stride": 4,                     // sizeof(float)
    "format": "r32f",
    "shape":  [GRID_SIZE, GRID_SIZE, GRID_SIZE]
});
state_writer.saveBuffer("v", v_buf.data(), N * sizeof(float), { ...same... });
```
Two scalar fields. `GRID_SIZE = 256` by default → 67M elements/buffer → 256 MB/frame.

### reaction-diffusion-2d (Phase 5, Stack B)
```ts
stateWriter.saveBuffer('u', u, { count: cells, stride: 4, format: 'r32f', shape: [rt.gridSize, rt.gridSize] });
stateWriter.saveBuffer('v', v, { count: cells, stride: 4, format: 'r32f', shape: [rt.gridSize, rt.gridSize] });
```
Same `u`/`v` shape as RD-3D, 2D. Deliberate naming parity with the 3D sim.

### physarum (Phase 6, Stack B)
```ts
stateWriter.saveBuffer('trail', trailBytes, {
    count:  rt.gridSize * rt.gridSize,
    stride: TRAIL_BPP,               // = 8
    format: 'rgba16float',
    shape:  [rt.gridSize, rt.gridSize, 4]
});
```
Single trail buffer. Three species RGB + alpha = 4 channels at fp16. **No agent buffer is captured** — agents are re-seeded from the RNG seed on load (by design; literal positions are not preserved).

### boids-3d (Phase 7, Stack B) — **special case**
```ts
stateWriter.saveBuffer('entities', entityBytes, {
    count:  totalEntities,
    stride: ENTITY_BYTES             // = 32
    // NO format, NO shape
});
stateWriter.saveBuffer('predator_state', predatorBytes, {
    count:  tier.predators,
    stride: PREDATOR_STATE_BYTES     // = 16
    // NO format, NO shape
});
```
**Buffers are packed C-style structs, not single-dtype arrays.** Format and shape are deliberately absent because no single WebGPU format describes the layout. The Tier-1 loader's "liberal accept" must handle this — these buffers come back as `bytes` and Tier 2's particle module decodes via a struct-dtype.

Struct layouts (from WGSL):
```
Entity (32 bytes):
    pos: vec3<f32>   (12B)
    species: u32      (4B)  → 16B total, vec3-aligned
    velocity: vec3<f32> (12B)
    _pad: f32          (4B) → 32B total

PredatorState (16 bytes):
    target_boid_id: u32         (4B)
    target_age_frames: u32      (4B)
    _pad0: u32                  (4B)
    _pad1: u32                  (4B)
```
Recommended numpy dtype for `entities`:
```python
np.dtype([
    ('pos', '<f4', 3), ('species', '<u4'),
    ('velocity', '<f4', 3), ('_pad', '<f4'),
])
```

### eulerian-smoke (Phase 8, Stack C)
```cpp
saveBuffer("velocity.bin",    {count: N, stride: 8, format: "rgba16f",  shape: [G, G, G]});
saveBuffer("density.bin",     {count: N, stride: 4, format: "r32f",     shape: [G, G, G]});
saveBuffer("temperature.bin", {count: N, stride: 4, format: "r32f",     shape: [G, G, G]});
saveBuffer("pressure.bin",    {count: N, stride: 4, format: "r32f",     shape: [G, G, G]});
```
**Note:** buffer names include `.bin` extension here (`velocity.bin`, etc.), unlike the other sims which pass bare names (`u`, `v`, `trail`, `entities`). `StateWriter` appends `.bin` on bare names; passing `velocity.bin` produces a file literally named `velocity.bin.bin` on disk. Worth verifying against an actual capture — possible bug in Phase 8.

Velocity buffer uses `rgba16f` despite being a 3-component vector — the 4th component is unused. Tier-1 should expose vector fields as `[G, G, G, 4]` and let Tier-2 vector-field diagnostics slice off the unused channel.

---

## 3. Format-string normalization table (recommended)

The repo emits four distinct format strings today. Recommended numpy mapping:

| Format string | Emitted by | numpy dtype | Channels | Notes |
|---------------|-----------|-------------|----------|-------|
| `r32f` | RD-2D, RD-3D, smoke (density/temp/pressure) | `np.float32` | 1 | Standard fp32 scalar |
| `rgba16f` | smoke (velocity) | `np.float16` | 4 | Stack C naming |
| `rgba16float` | physarum (trail) | `np.float16` | 4 | Stack B naming — **same format as `rgba16f`** |
| *(absent)* | boids-3d (entities, predator_state) | `np.uint8` (raw bytes) | — | Decoded by Tier 2 particle module via struct-dtype |

Tier 1 should normalize `rgba16f` and `rgba16float` to the same internal representation. The inconsistency is a Stack B vs Stack C naming drift that's not worth pushing back onto shipped sims.

When `format` is absent, return raw bytes and require the caller to specify a struct dtype. When `shape` is absent, default to `[count]`.

---

## 4. GpuProfiler CSV reality — **this is bigger than the agent realized**

### The API exists in both stacks

Both `common-cpp/src/gpu_profiler.cpp` and `common-web/src/gpuProfiler.ts` define identical CSV emission:

```
frame,pass,gpu_ms,cpu_ms
```

Long-form, one row per (frame × pass). Header written on first call, appended thereafter.

C++ side: `void appendCsv(const std::filesystem::path& path)` — writes to a caller-supplied path.
TS side: `appendCsv()` accumulates in memory; `downloadCsv(filename)` triggers a browser download as `profile.csv`.

### But no shipped sim actually calls it

Across all 7 shipped sims, **zero call sites** for `appendCsv` or `downloadCsv`. The profiler exists, sims use it for the on-screen ImGui readout, but the CSV emission path is dormant.

### Implications for `perf.py`

The agent's Q1 ("where do CSVs land?") is really three questions:

1. **Do sims need to start emitting CSVs?** If yes, that's a coordinated change across all 7 shipped sims plus future ones. Default-on, opt-in, or hotkey-toggled?
2. **Where do they land?** Stack C writes to a caller-supplied path. Stack B downloads to browser default. No current convention for either.
3. **Does the capture format absorb them, or do they stay separate?** Two cleanish options:
   - **CSV alongside capture root:** sim emits `<root>/frame_times.csv` once per session; `perf.py` reads it independent of any particular `capture_NNNN/`.
   - **CSV inside each capture dir:** sim flushes the most recent N frames into `capture_NNNN/profile.csv` at capture time; `perf.py` aggregates across captures.

The first is simpler and matches what `appendCsv` already does. The second couples perf data to capture cadence, which has its own merits.

**Recommendation to coordinator:** decide between option 1 and option 2 *before* Tier 1 drafts the `perf.py` API. Otherwise the agent has to design for both. I'd lean option 1 (CSV at capture root, one CSV per session) — it's the lower-impact change to the shipped sims and matches the existing `appendCsv(path)` shape.

### Also worth noting

I claimed in the overview that `perf.py` parses "a frame-time CSV and a GPU pass-timing CSV." There's actually only one CSV in the GpuProfiler schema — pass timings, with frame as a column. A separate aggregate frame-time series can be derived by grouping; it doesn't need its own file. The agent should know this.

---

## 5. Stack B ZIP vs Stack C directory transport

Same logical schema, different physical container — the loader has to handle both.

**Stack C (native):** writes `<root>/capture_NNNN/state.json + *.bin` directly to disk.
**Stack B (browser):** triggers download of `capture_NNNN.zip` containing `capture_NNNN/state.json + *.bin`. The user unzips wherever they want.

The Tier 1 loader should accept either:
- A path to an unzipped `capture_NNNN/` directory.
- A path to a `capture_NNNN.zip` file (open with `zipfile` from stdlib, treat the inner directory as the capture).
- A path to a parent directory containing many `capture_NNNN/` or `capture_NNNN.zip` siblings (auto-discover the sequence).

`fflate` on the writer side is fine — the resulting ZIP is standard and `zipfile.ZipFile` reads it without trouble.

---

## 6. Buffer-name normalization edge case

Six of the seven sims pass bare buffer names (`u`, `v`, `trail`, `entities`, `predator_state`). Eulerian-smoke passes names with the `.bin` extension already attached (`velocity.bin`, `density.bin`, etc.). The C++ `StateWriter::saveBuffer` appends `.bin` unconditionally:

```cpp
const std::filesystem::path bin = current_dir_ / (name + ".bin");
```

So a smoke capture has files literally named `velocity.bin.bin` on disk and references `"file": "velocity.bin.bin"` in `state.json`. Either:
- This is a Phase 8 bug, fixable in a one-line PR to smoke.
- Or `StateWriter` should strip a `.bin` suffix if present.

The Tier 1 loader should match `file` from `state.json` literally and not assume a particular naming pattern. Surface this to the coordinator as a known issue, don't try to fix it from the diagnostics side.

---

## 7. Summary of decisions the coordinator should make before Tier 1 codes

In rough priority order (highest first):

1. **CSV emission:** option 1 (root-level session CSV) or option 2 (per-capture CSV)? Then a coordinated commit lands `appendCsv` calls in all 7 shipped sims.
2. **`velocity.bin.bin` double-extension:** fix smoke, or fix `StateWriter`? (Smoke fix is one-line; `StateWriter` fix is the more permanent answer.)
3. **Meta-only captures for closed-form sims (mandelbulb, strange-attractors):** add diagnostic-mode buffer dumps, accept meta-only Tier-3, or re-derive from params? Doesn't block Tier 1 but blocks Tier-3 closed-form.
4. **Spec-then-code vs end-to-end:** does the Tier 1 chat draft the spec, hand off, and have a separate chat implement? Or both in one chat? Architect-2 cross-review on the spec yes/no?
5. **Capture-cadence guidance:** is this a Tier 1 deliverable (recommended cadences per sim) or a coordinator policy doc?

Everything else in the agent's question list (caching policy, health.py shape, dependency envelope) the agent's instincts are correct and you can just confirm.

---

*End of reference. The Tier 1 agent now has the actual repo state, not paraphrases of it.*
