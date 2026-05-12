# Reaction-Diffusion 2D — load-bearing decisions

Load-bearing for the v1 implementation; locked in Phase 5. See the Phase 5
spec's § 2.1 for full reasoning. Summary here for quick reference during
follow-up polish work.

## Locked

- **Stack A artifact is multi-file.** `shadertoy/BufA.glsl` + `shadertoy/Image.glsl` + `shadertoy/README.md` together. **First multi-buffer Stack A artifact in
  the repo** — extends Phase 4's single-file convention to sims with persistent
  state. Future multi-buffer ports (physarum, neural-CA) inherit this layout.
- **Compute pipeline for the RD update step.** `ComputePipeline.create` from
  common-web, single pipeline, two alternating bind groups (ping→pong, pong→ping).
  First Stack B sim with compute ping-pong on persistent 2D state.
- **Storage textures `rg32float` (R = u, G = v).** Two ping-pong textures.
  FP32 precision; matches Phase 3's per-field FP32 in C++.
- **Read access via sampled-texture binding** (`unfilterable-float`) + NEAREST + REPEAT sampler. Periodic BCs come for free from REPEAT addressing.
- **Manual bilinear in the visualize fragment** (4 textureLoad + lerp). `rg32float`
  is not in baseline filterable; manual bilinear avoids the `'float32-filterable'`
  optional feature and keeps display smooth at any canvas scale.
- **Capture is full state.** RD is path-dependent; params-only would be a
  preset bookmark. Captures save deinterleaved `u.bin` + `v.bin` (separate
  `r32f` files) under JSON meta key `'reactionDiffusion2d'` for cross-stack
  parity with Phase 3's `'reactionDiffusion3d'`.
- **`Texture.readback2D` added to common-web** (in-flight). 15 lines of
  texture-readback orchestration that future sims (physarum, neural-CA, …)
  will reuse.
- **Six Pearson presets with same Greek-letter labels and approximate F/k
  values as Phase 3.** Cross-stack vocabulary parity. Same post-build tuning
  caveat: cross-reference Munafo's catalog and edit one row of `presets.ts`
  if a preset misses its pattern type.
- **Discrete grid sizes 256/512/768/1024 via dropdown, default 512².** All
  multiples of 32 to satisfy WebGPU's `bytesPerRow % 256 == 0` requirement
  for `copyTextureToBuffer` at `rg32float` (8 bpp). Above 1024² is hero-render
  territory — out of scope for v1 per the rollout brief.
- **Brush interaction in v1.** Separate compute kernel; one extra ping-pong
  tick per frame the brush is active. Establishes the input → compute-dispatch
  pattern that physarum, neural-CA, and lenia-fft web variants will reuse.
- **Camera not used.** 2D sim, fullscreen blit. ~50 lines saved.
- **HMR shader paths follow the strange-attractors convention** (relative to
  the per-sim `web/` directory: `'shaders/rd_update.compute.wgsl'` etc.). This
  matches what the `viteWgslPlugin` actually emits. Phase 4's mandelbulb used
  full repo-relative paths and silently no-ops on hot-reload as a result —
  out of scope for this phase, flagged for the next hardening pass.

## Cheap to revisit

| Decision | Where |
|----------|-------|
| Default Pearson preset (λ) | `presets.ts` index 0 |
| Default substeps (4) | Constant in `main.ts`; lil-gui slider 1–32 |
| Default grid size (512²) | Default in `main.ts`; dropdown 256/512/768/1024 |
| Default colormap (magma) | Constant; dropdown over 4 LUTs |
| Default brush radius / strength (12 cells / 1.0) | lil-gui sliders |
| Default min/max display range (0.0 / 1.0) | lil-gui sliders |
| Default seed block size (16) and noise amplitude (0.05) | lil-gui sliders |
| Default `initSeed` (0xC0FFEE) | Constant; matches Phase 2 / Phase 3 |
| Whether the brush dispatch covers the full grid or only the brush rect | One-line change in `main.ts` |
