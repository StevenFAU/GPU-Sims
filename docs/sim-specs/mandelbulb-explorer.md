# Mandelbulb Explorer — Specification

> **Status:** Specification pending — not yet drafted by the architect chat
> **Category:** Closed-form
> **Primary stack:** A (Shadertoy) → B (WebGPU)
> **Secondary stack(s):** —
> **Target machine:** Desktop
> **Folder:** [`closed-form/mandelbulb-explorer`](../../closed-form/mandelbulb-explorer/)

---

## 1. Goal and audience

Demonstrate that a real distance-estimator raymarcher of the Mandelbulb
fractal — Daniel White's 2009 3D analog of the Mandelbrot set — is
runnable in real time in a current browser via WebGPU, with full
free-fly camera control, soft shadows, orbit-trap coloring, and tunable
exponent morph. The imagined viewer is a recruiter or peer who has seen
fractal flythroughs on YouTube and assumes they're rendered offline; the
"wow" comes from being able to move through one in real time on their
own machine.

This is also the first GPU-Sims Stack A &rarr; B port. The `shadertoy/`
folder preserves a paste-runnable Shadertoy implementation we wrote
ourselves; the `web/` folder is the WebGPU port of it. Both halves are in
the repo so the port flow is a documented pattern future Stack A &rarr; B
sims (notably reaction-diffusion-2d) can copy.

## 2. Mathematical formulation

The Mandelbulb is the 3D iterated map `z ← z^n + c` where `z^n` is
defined via spherical coordinates: cube the radius, scale the polar
angles by `n`. For each pixel, the world-space sample point along the
camera ray is the constant `c`, and we iterate `z` (starting at `z = c`)
until `|z| > bailout` or we hit the iteration cap.

```
r     = length(z)
theta = atan2(sqrt(z.x^2 + z.y^2), z.z)
phi   = atan2(z.y, z.x)
z_new = r^n * (sin(n*theta)*cos(n*phi),
               sin(n*theta)*sin(n*phi),
               cos(n*theta))           + c
```

The Hubbard-Douady distance estimate at the escape point:

```
DE(c) = 0.5 * log(r) * r / dr
```

where `dr` is the running derivative magnitude updated each iteration as
`dr ← n * r^(n-1) * dr + 1`.

**Defaults:** `n = 8` (canonical), iteration cap 8, bailout 2.0, max
ray-march steps 96, epsilon base 0.001, max ray distance 8.0.

**Approximations:** the DE function is the standard Hubbard-Douady
estimate. It's an over-estimate near surface detail and slightly wrong
inside thin features; the renderer mitigates with `epsilon` scaling
(linear-in-distance growth) and a max-step cap. No code lifted from any
specific reference shader — see § 11 for the reference list.

## 3. Stack assignment and rationale

**Stack A &rarr; Stack B.** The Stack A artifact at
`closed-form/mandelbulb-explorer/shadertoy/mandelbulb.glsl` is a
self-contained Shadertoy-idiom GLSL fragment shader, paste-runnable on
shadertoy.com. The Stack B port at `closed-form/mandelbulb-explorer/web/`
is a real WebGPU port of the same math.

Rejected: pure Stack A (Shadertoy is one fragment shader, no real
camera state across frames, can't capture/restore parameters); pure
Stack C (overkill for a closed-form renderer that fits in a single
fragment shader, and the deploy-to-portfolio target is web).

## 4. Data structures and memory layout

Per-frame data is tiny.

- **Raymarch uniform buffer:** 192 bytes (12 × `vec4<f32>`). Camera basis
  (4 vec4s) + DE/march params (2 vec4s) + lighting (2 vec4s) + coloring
  (3 vec4s) + output sizing (1 vec4). Rewritten every frame.
- **Tonemap uniform buffer:** 16 bytes (1 × `vec4<f32>`). Just the
  exposure scalar in the v1; padded to 16 bytes per WebGPU's uniform
  alignment requirements.
- **Offscreen HDR render target:** one `rgba16float` 2D texture sized
  `(canvasWidth * renderScale) × (canvasHeight * renderScale)`. Memory
  footprint at 4K canvas + DPR 2 + render-scale 1.0 worst-case is roughly
  3840 × 2160 × 8 bytes = 63 MB. At render-scale 0.5: 16 MB. Recreated
  on canvas resize and on render-scale slider commit.

No simulation state, no particles, no time-evolved volumes, no scratch
buffers. Total VRAM: under 100 MB even at the largest practical canvas.

## 5. Per-frame compute pipeline

There is no compute pipeline. The pipeline is two render passes:

```
[Render]  raymarch.frag.wgsl: fullscreen triangle into offscreen RT
            input:  raymarch uniform buffer (camera + DE params + coloring)
            output: rgba16float texture, sized canvas * renderScale
            ↓
[Render]  tonemap.frag.wgsl: fullscreen triangle into swap-chain image
            input:  tonemap uniform (exposure) + offscreen RT (linear sampler)
            output: ctx.preferredFormat to canvas
```

No synchronization concerns: the two passes happen in encoder order,
WebGPU's pass boundaries flush all writes from the prior pass before the
next pass reads them.

## 6. Interactive rendering approach

- **Raymarch:** distance-estimator sphere-tracing in a single fragment
  shader. Iteration cap, bailout, max steps, and epsilon scaling all
  live in lil-gui sliders.
- **Soft shadows:** cone-traced from a single directional light
  (toggleable). Default ON. Adds roughly 2× to per-pixel cost when on.
- **Orbit-trap coloring:** three presets (point-at-origin, coordinate
  planes, mixed). Hot/cool tint colors are user-pickable.
- **Camera:** free-fly via common-web `Camera`. Initial position is
  off-axis (1.5, 0.8, 2.5) so the first frame shows fractal structure
  rather than a symmetric blob.
- **n-power morph:** lil-gui slider drives `n` directly by default.
  Optional "Auto-morph" toggle (default OFF) animates `n` on a slow sine
  wave between user-chosen min/max.
- **UI:** lil-gui panel with folders for DE, Raymarch, Lighting,
  Coloring, Output, Camera, Animation. All settings persist via
  `ParamPanel`'s localStorage backing.

## 7. Offline export path

No VDB or Alembic export — the mandelbulb is a closed-form fractal, fully
reproducible from the parameter set. The F5 capture writes a
`mandelbulb_explorer_NNNN.zip` containing `state.json` with the runtime
parameters and camera state. A future
`render-pipelines/blender/render_mandelbulb_explorer.py` (deferred) could
consume the JSON and re-render via Blender Cycles for cinematic stills,
but the current sim's screenshot-via-browser output already serves the
hero-still use case.

## 8. Scale tiers

To be measured post-build. Add three rows here for: desktop (RX 6800 XT,
Chromium, default settings, 1080p viewport, soft shadows on); high-end
(lab PC 2080 Ti, same settings); and render-scale-0.5 (same as desktop
row but with `renderScale = 0.5`). HPC stretch is not applicable —
mandelbulb's interactive build is the deliverable.

## 9. Stretch goals

- **AO and bloom polish.** The HDR offscreen RT leaves both doors open;
  bloom is the standard "extract → blur → composite in tonemap pass"
  pattern strange-attractors uses, ~150 lines of WGSL + a small TS
  rebind.
- **Mandelbox / other DE fractals.** Selectable via dropdown; shares all
  the raymarch / tonemap infrastructure. Each new fractal is one
  additional WGSL function.
- **Cinematic-orbit camera mode.** The common-web `Camera` class already
  supports orbit mode; expose a toggle so the user gets a hands-free
  camera path for screen-capture videos.

## 10. Engineering risks

- **DE math correctness.** One wrong sign in the spherical-coordinate
  conversion produces a fractal that's recognizable but subtly off. The
  Stack A artifact is the verification reference: a side-by-side at
  matching parameters and camera position should look identical.
- **Soft-shadow ringing.** The cone-trace approximation can produce
  banding when `softShadowK` is high and the iteration cap is low.
  Defaults are tuned to avoid it; if it appears in v1.1, increasing
  iteration cap is the first knob to turn.
- **WebGPU uniform-buffer alignment.** The 12 × `vec4<f32>` packing in
  raymarch.frag.wgsl is deliberate — every slot is 16-byte aligned.
  Adding a `f32` field "in between" two vec4s without padding will
  silently corrupt subsequent fields.

## 11. References

- Daniel White (2009), "The Mandelbulb: First 'true' 3D image of the
  famous fractal." Math, no code. Self-published / blog post.
- Paul Nylander, hypercomplex-fractals gallery and refinements:
  <https://www.bugman123.com/Hypercomplex/>. Math and reference renders;
  no code lifted.
- Inigo Quilez, "Distance estimation":
  <https://iquilezles.org/articles/distfunctions/>. General DE-raymarching
  technique; iq's website defaults to CC BY-NC-SA, so no shader code was
  lifted from there.
- common-web infrastructure (this repo): see
  [`common/common-web/`](../../common/common-web/) for the shared API
  surface (`Context`, `Renderer`, `Camera`, `RenderPipeline`, `Texture`,
  `ParamPanel`, `HotReloader`, `StateWriter`, `StateReader`).
