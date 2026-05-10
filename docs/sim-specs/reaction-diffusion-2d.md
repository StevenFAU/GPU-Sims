# Reaction-Diffusion 2D — Specification

> **Status:** Implemented (Phase 5) — <https://stevenfau.github.io/GPU-Sims/reaction-diffusion-2d/>
> **Category:** Continuous CA
> **Primary stack:** A (Shadertoy) → B (WebGPU)
> **Secondary stack(s):** B (WebGPU) — see `continuous-ca/reaction-diffusion-2d/web/`
> **Target machine:** Anywhere
> **Folder:** [`continuous-ca/reaction-diffusion-2d`](../../continuous-ca/reaction-diffusion-2d/)

---

## 1. Goal and audience

A live, mouse-paintable explorer of the Gray-Scott reaction-diffusion model on
a 2D periodic grid. The visitor lands on the page, picks one of six Pearson
1993 named presets from a dropdown, and within seconds sees the named pattern
type forming. Dragging the mouse paints `v` material into the grid, seeding
new pattern formation in real time. F5 captures full simulation state to a
ZIP; F9 reloads it.

The imagined viewer is a recruiter or peer reviewer browsing a portfolio. The
sim should produce its headline pattern (λ — irregular spots) within ~30
seconds at the default substep rate, and respond visibly to brush input within
one frame. The feeling is "you can play with this," not "watch this video."

## 2. Mathematical formulation

Gray-Scott reaction-diffusion:

```
∂u/∂t = Du · ∇²u  −  u·v²  +  F · (1 − u)
∂v/∂t = Dv · ∇²v  +  u·v²  −  (F + k) · v
```

`u` and `v` are concentrations of two chemicals. `Du`, `Dv` are diffusion
coefficients; canonical values `Du = 0.16, Dv = 0.08`. `F` (feed rate) and
`k` (kill rate) are the regime-determining parameters; the Pearson 1993
named regions are points in (F, k) space where distinct stable patterns form.

Discretization: Forward Euler in time, second-order central differences in
space. Normalized `dx = 1, dt = 1`. Stability bound for the diffusion term
in 2D is `Du · dt / dx² ≤ 1/4`; canonical settings sit at 0.16, well inside.

5-point Laplacian stencil:

```
L(f)[i,j] = f[i±1,j] + f[i,j±1] − 4·f[i,j]
```

Periodic boundary conditions implemented at the sampler level (REPEAT
addressing on the read binding).

References:
- Pearson, J. E. (1993). "Complex Patterns in a Simple System." *Science* 261(5118), 189–192.
- Munafo, R., "Reaction-diffusion by the Gray-Scott model." <https://mrob.com/pub/comp/xmorphia>.

## 3. Stack assignment and rationale

Stack A → Stack B port. The Stack A artifact (`shadertoy/BufA.glsl` +
`shadertoy/Image.glsl`) is a Steven-original Shadertoy implementation in the
multi-buffer idiom — first multi-file Stack A artifact in the repo,
extending Phase 4's single-file convention to sims with persistent state.

The Stack B port (`web/`) consumes `@gpusims/common-web` for context, renderer,
compute pipeline wrapper, render pipeline wrapper, lil-gui parameter panel,
state writer/reader, and hot-reload. **First Stack B sim with compute
ping-pong on persistent 2D state** — Phase 4 was render-only, hello-world's
compute kernel is a trivial gradient, and strange-attractors uses compute for
particle integration but does not ping-pong general 2D grid state. This phase
exercises the wrapper at the canonical pattern.

Stack C (Vulkan) is occupied by the 3D sibling `reaction-diffusion-3d`; a
Stack C 2D version would duplicate without learning anything new. Stack D
(Python/Taichi) is the right home for any future "RD with adaptive mesh
refinement" or "RD on irregular topology" research extensions; not pursued.

## 4. Data structures and memory layout

Two `rg32float` storage textures (ping + pong). R channel is `u`, G channel
is `v`. Eight bytes per cell.

| Grid | Cells | Per-texture | Total (ping+pong) |
|------|-------|-------------|-------------------|
| 256² |  65,536 | 0.5 MB | 1 MB |
| 512² | 262,144 | 2 MB | 4 MB |
| 768² | 589,824 | 4.5 MB | 9 MB |
| 1024² | 1,048,576 | 8 MB | 16 MB |

Texture usage flags: `STORAGE_BINDING | TEXTURE_BINDING | COPY_SRC | COPY_DST`.
Each texture must be sampleable (read), storage-writable, capture-readable,
and reset-uploadable.

Three uniform buffers (32 B each): RD parameters, brush parameters, viz
parameters. One 256×4 RGBA8 texture for the colormap LUT.

## 5. Per-frame compute pipeline

Per render frame:
1. Substep loop, `N` iterations (default `N = 4`):
   - Dispatch the RD update compute kernel: read latest texture, write the other.
   - Swap latest pointer.
2. If LMB is held: dispatch the brush stamp compute kernel (one extra ping-pong tick).
3. Render the latest texture through the visualize fragment shader to the swapchain.

Workgroup size 16×16×1 (= 256, exactly at WebGPU's baseline limit). Dispatch
shape `⌈gridSize/16⌉ × ⌈gridSize/16⌉ × 1`.

No synchronization overhead — WebGPU command-encoder semantics insert the
necessary barriers between dispatches automatically. The substep loop is a
single `GPUCommandEncoder` and submitted once per frame.

## 6. Interactive rendering approach

Direct-to-canvas; no offscreen RT. RD's visualization is a 2D grid lookup
through a colormap, no exposure/HDR adjustment to make HDR offscreen worth it.

The visualize fragment does manual bilinear interpolation across the four
nearest cells (because `rg32float` is not in WebGPU's baseline filterable
list). Smooth display at any canvas scale without paying for the
`'float32-filterable'` optional feature.

UI: lil-gui panel via `common-web`'s `ParamPanel`. Folders: Preset, Diffusion,
Simulation, Brush, Display. Buttons for save/load. localStorage persistence
via the `persistKey: 'reaction-diffusion-2d'` option.

Brush: `pointermove`/`pointerdown`/`pointerup` listeners on the canvas drive
a separate compute dispatch when LMB is held.

## 7. Offline export path

F5 captures full simulation state — JSON parameter block + raw `u.bin` + `v.bin`
in a ZIP. F9 loads. The `Texture.readback2D` helper added to common-web in
this phase orchestrates the `copyTextureToBuffer` + staging-buffer round-trip;
the sim deinterleaves the `rg32float` bytes into separate `r32f` files so the
disk format matches Phase 3's `reaction-diffusion-3d` per-field shape.

A future Blender script (deferred) would read `u.bin`/`v.bin` as a NumPy
array and render the field as a 2D height-map or 3D extrusion — out of scope
for v1, in scope for the eventual cross-stack capture replay tool.

## 8. Scale tiers

- **Laptop iteration scale:** 256² × 4 substeps × 60 fps. Trivially fast on
  any WebGPU-capable device.
- **Desktop flagship scale:** 512² × 4 substeps × 60 fps default. RX 6800 XT
  comfortably hits this; integrated graphics on a recent MacBook should also
  manage.
- **HPC hero scale:** 1024² × 32 substeps × 60 fps. Within the 6800 XT's
  bandwidth headroom; useful for accelerated pattern-formation captures.

## 9. Stretch goals

Documented in `continuous-ca/reaction-diffusion-2d/docs/notes.md` v1.1 polish
backlog. Headlines: U-channel display toggle, per-channel display range,
brush profile presets (gaussian/hard-edge), right-click brush for inverted
splat, PNG snapshot, preset autoplay.

## 10. Engineering risks

- **Pearson preset values approximate.** Literature places each region as a
  neighborhood; the dropdown uses approximate centers. If a preset doesn't
  show its named pattern, cross-reference Munafo's catalog and edit one row
  of `presets.ts`. Polish-level; not blocking ship. Three of the six values
  (λ, β, τ) were flagged as suspect during Phase 3 cross-review — same caveats
  carry forward here.
- **`Texture.readback2D` requires `bytesPerRow % 256 == 0`.** Discrete grid
  sizes 256/512/768/1024 all satisfy this at `rg32float` (8 bpp). Any future
  non-aligned grid size would need row padding on capture — explicit error
  thrown by the helper makes the failure mode obvious.
- **Brush full-grid dispatch is wasteful.** Cost is ~one substep equivalent
  per active-brush frame. Negligible in practice but a v1.1 polish item.
- **Forward Euler instability** at extreme parameter excursions. The clamp to
  `[0, 1]` after each substep is a defensive guard; under canonical settings
  the system stays in `[0, 1]` analytically.

## 11. References

- Pearson, J. E. (1993). "Complex Patterns in a Simple System." *Science* 261(5118), 189–192.
- Munafo, R. P. — <https://mrob.com/pub/comp/xmorphia>.
- Inigo Quilez, polynomial colormap fits — <https://www.shadertoy.com/view/WlfXRN>.
- White, D. (2009) — Mandelbulb formulation (peer reference for the Stack A → B convention).

No code lifted; references consulted for math and pattern catalogs only.
