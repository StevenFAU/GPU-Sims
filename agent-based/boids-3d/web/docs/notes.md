# boids-3d notes

Per-sim v1.1 polish backlog. Items are not pre-scheduled — they ship when the trade-off becomes worth it relative to the next sim's priorities.

## Priority 1.0 — render-style toggle (aquarium ↔ void)

v1 ships the aquarium look (visible 12-edge wireframe + subtle vertical gradient background). The "void" alternative (no wireframe + pure-black background + emissive boids) is a different visual identity that some users may prefer.

Implementation: add a panel checkbox "Show bounds wireframe", a checkbox "Solid background (vs gradient)", or a single "Render style" dropdown with two options. The wireframe pipeline becomes conditional on the checkbox in the per-frame render-pass code; the background fragment shader takes a uniform that selects between gradient and solid. ~30 lines of WGSL + ~40 lines of TS pipeline glue. No simulation, capture, or load-bearing structure changes.

## Priority 1.1 — vertical leader placement via Shift+LMB+scroll

v1 places leaders on the y=0 ground plane via LMB-click. Advanced users designing 3D paths may want to place leaders at altitude.

Implementation: hold Shift+LMB-down, scroll wheel adjusts placement depth from an 8u-default-from-camera distance, click releases the leader at that depth. The Shift modifier preserves the place-leader semantic while disambiguating from camera-zoom (which currently isn't bound, but if it were, scroll would mean zoom). Same shape as Blender's place-3d-cursor with depth scrolling.

## Priority 1.2 — instanced thin cylinders for wireframe

v1 renders the wireframe as `topology: 'line-list'` at 1px (WebGPU clamps line width to 1.0 across all browsers). At 100k tightly-clustered boids the 1px wireframe may read as too thin against the dense flock.

Implementation: replace the line-list pipeline with instanced cylinders (one cylinder per edge × 12 edges = 12 instances; each cylinder ~24 tris = 288 tris total). Cylinder thickness becomes a panel slider. Adds normal vectors and corner-mitering at the 8 box corners (otherwise cylinders end at angles and look like loose pipes). ~100 lines of WGSL + ~30 lines of TS.

## Priority 1.3 — conditional sort skipping

v1 runs the spatial-hash sort every frame. Most entities stay in the same cell for many frames at default Reynolds parameters; skipping the sort on stable frames is a real ~20–30% throughput win on the per-frame compute graph.

Implementation: measure max per-entity displacement during `integrate.compute.wgsl` (atomicMax over a small displacement-tracking buffer); next frame's CPU code reads this back via `Buffer.readback` (with 1-frame latency, acceptable) and gates the cell-count + prefix-sum + scatter dispatches on `max_displacement < CELL_SIZE`. The latency means the gating is one frame behind ground truth, which means a single fast-displacement frame still triggers a sort but the next frame's check might miss an early movement — acceptable since the sort would happen on the frame after anyway.

The silent-failure mode of stale cell-membership doesn't earn v1 risk; this polish ships when measurement shows it's worth the bookkeeping.

## Priority 1.4 — GPU-side reseed kernel

v1 reseeds entities CPU-side via xorshift32 then uploads via `Buffer.uploadDirect` — at the 100k tier this is a 3.2 MB upload, fast but visible as a UI hitch on tier change or R-press. Tier change is currently ~50–100ms hitch; would drop to <1ms with a GPU kernel.

Implementation: a `reseed.compute.wgsl` kernel takes `initSeed` and `entityCount` as uniforms, generates per-entity position+velocity via xorshift32 inline, writes directly to both `entityBufA` and `entityBufB` (so first-frame flock_update reads valid initial state).

**Cross-reference:** physarum's `notes.md` priority 1.1 is the same shape problem at smaller surface (physarum's 10M-tier reseed has a 200–500ms hitch). When this polish work happens — likely at the third sparse-source consumer's promotion review (eulerian-smoke / sph-water / lattice-boltzmann) — boids and physarum should adopt the same pattern. Promote the shared GPU-init pattern to `common-web` as part of that review.

## Priority 1.5 — stochastic-mode reservoir-1 sampling

v1 uses deterministic-modulo target selection in stochastic-prey mode (each predator's `pick_idx = rng_u32(predatorIdx ^ iteration) % 1024`, picks the candidate at that index in cell-walk order). This isn't a uniform sample — candidates earlier in the walk order are more likely to be picked, biasing toward boids in cells with lower linear cell index.

Implementation: replace with reservoir-1 sampling: maintain `picked_id` and `seen_count`; on each candidate, replace `picked_id` with probability `1.0 / f32(seen_count + 1)` (uses the per-predator RNG that already runs for jitter). True uniform sample. ~5 line change in `predator_update.compute.wgsl`.

Trade-off: the reservoir-1 result depends on cell-walk iteration order, which depends on workgroup execution order, which is non-deterministic in WebGPU. Bit-exact replay (currently "within one integration step" per § 2.8) loses determinism guarantees in stochastic mode. v1 deterministic-modulo preserves replay determinism at the cost of the technical-uniformity claim. Ship reservoir only if a portfolio claim ever depends on the uniformity.

## Priority 1.6 — instrument early-out hit rate, tune cell size

§ 2.6's dimensional-analysis block estimates ~95% early-out rate for cell-walk candidates (most are outside the actual neighborhood radius even though they're in one of the 27 cells). Banked-but-unmeasured.

Implementation: add a per-frame counter `early_out_hits` and `total_candidates` (atomicAdd inside `flock_update.compute.wgsl`'s neighbor loop). Read back periodically via `Buffer.readback`. If the actual rate is materially below 95% (say <85%), tune cell size: smaller cell size → fewer candidates per walk → higher early-out rate, at the cost of more cells in the hash. Currently CELL_SIZE = 4.0 with neighborhood radii up to ~3.0; reducing CELL_SIZE to 3.5 or 3.0 (with corresponding clamp on neighborhood-radius slider max) tightens the walk volume.

## Priority 1.7 — predator user-placement

v1 auto-spawns predators at sim init and lets them roam. Some users may want to place predators interactively (e.g., to set up a specific harassment scenario along a leader-tour route).

Implementation: a panel toggle "Predator placement mode" that re-uses the LMB click handler. When enabled, LMB places a predator at the unproject point with zero initial velocity; the predator immediately begins hunting per the active mode. Cap could mirror the leader cap (32) or be unlimited up to the predator-tier count. Removes auto-spawn behavior on tier change while predator-placement-mode is active.

## Priority 1.8 — bit-exact-from-frame-1 capture replay

v1 reproduces a captured frame within one integration step (per § 2.8, README capture/load section). Some portfolio framings may want literal pixel-for-pixel bit-exact replay.

Implementation: `applyCapture` sets a `suppressIntegrateNextFrame: bool` flag; the frame loop checks this flag, skips the `integratePipe.dispatch(...)` call on the next frame, then clears the flag. The render reads the just-loaded entity buffer directly, producing the exact captured frame. ~5 lines of logic.

Trade-off: adds a load-only code path that diverges from the canonical compute graph for one frame; minor architectural noise. Skip until a recruiter or reviewer asks specifically about literal bit-exact replay.

## Priority 1.9 — predators attracted to leaders

v1 has predators ignoring leaders entirely. A v1.1 variant: predators are attracted to leaders too (with a different falloff than boids; e.g., square-falloff over a longer radius). Produces a "predators stalk leader hubs" visual where predators concentrate at leader positions and the boids that approach.

Implementation: add a `predatorLeaderAttractStrength: f32` panel slider (default 0.0 = off in v1), modify `predator_update.compute.wgsl` to walk the leader array and accumulate an attraction force when strength > 0. ~15 lines of WGSL.

## Priority 1.10 — smooth tier transitions

v1 destroys + recreates 8 buffers + 16 bind groups on tier change, producing a 50–100ms hitch. Acceptable but not great.

Implementation: allocate all entity-related buffers at MAX tier (100k+1k = 101k) at sim init; never destroy. The `params.entityCount` uniform drives the active range — kernels iterate up to entityCount, dispatch grids size to entityCount. Tail entries beyond entityCount are ignored. Tier change becomes a one-frame uniform write + reseed via GPU kernel (after priority 1.4 ships). Hitch drops to <1ms.

## Priority 1.11 — separation force epsilon tuning

v1 ships canonical Reynolds 1/d² separation force with `epsilon = 0.01u²` to prevent the d → 0 singularity. The epsilon may need tuning if visible behavior shows either: (a) boids passing through each other at very small distances (epsilon too large), or (b) numerical instability / NaN propagation at near-collision (epsilon too small).

Implementation: expose `separationEpsilon: f32` as a panel slider in the Reynolds folder, default 0.01, range [0.001, 0.1]. ~3 line change in `flock_update.compute.wgsl` to read from the params uniform instead of the hardcoded constant. Banked rather than tuned pre-ship because the visible behavior depends on the actual flock dynamics at default Reynolds weights, which haven't been observed yet.
