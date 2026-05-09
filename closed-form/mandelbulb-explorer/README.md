# Mandelbulb Explorer

> **Stack:** A → B (Shadertoy → WebGPU) · **Status:** Implemented (Phase 4) · **Live:** [stevenfau.github.io/GPU-Sims/mandelbulb-explorer/](https://stevenfau.github.io/GPU-Sims/mandelbulb-explorer/)

The first GPU-Sims Stack A → B port. A Shadertoy-idiom GLSL implementation
of the Mandelbulb DE raymarcher (preserved at [`shadertoy/`](shadertoy/))
sits alongside a WebGPU port (at [`web/`](web/)). Both halves use the same
math and the same defaults, so a screenshot from one matches a screenshot
from the other under matching camera positions.

## What you're seeing

The Mandelbulb is a 3D analog of the Mandelbrot set, defined by iterating
`z ← z^n + c` with `z^n` extended to 3D via spherical coordinates (Daniel
White, 2009; Paul Nylander). For each pixel, a ray is cast from the camera
through that pixel; the **distance estimator** (DE) tells us how far we
can step along the ray before potentially hitting the surface, and we
sphere-trace until we converge or run out of steps.

The canonical exponent is `n = 8`. The lil-gui slider exposes `n` from 2
through 12; lower values are rounder and less detailed, higher values are
sharper and busier.

Coloring is by **orbit trap** — the closest approach of the iterated `z`
to a chosen geometric primitive during the iteration. Three trap presets
ship: point-at-origin (default, the warm-cool gradient most mandelbulb
images use), coordinate-planes (sharper "veined" coloring), and a 60/40
mix of the two.

A single directional light is cone-traced for soft shadows; toggling them
off roughly halves the GPU cost and is the obvious knob on weaker
hardware. Output is rendered into an HDR `rgba16float` offscreen target,
then linearly upsampled and Reinhard-tonemapped to the canvas; the
**render-scale** slider (0.5 – 1.0) trades resolution for cost.

## Controls

| Key / mouse | Action |
|-------------|--------|
| WASDQE | Move (free-fly always active) |
| RMB-drag | Look around |
| Shift (held) | Movement speed boost |
| F5 | Save capture (downloads `capture_NNNN.zip`) |
| F9 | Load capture (file picker) |
| Reset view (panel button) | Restore default camera position |

The lil-gui panel exposes:

- **DE:** n (power exponent), iteration cap, bailout
- **Raymarch:** max steps, epsilon base, epsilon growth, max ray distance
- **Lighting:** soft shadows toggle, shadow strength `k`, light yaw/pitch, ambient
- **Coloring:** orbit-trap preset, trap radius, hot tint, cool tint, background
- **Output:** render-scale, exposure
- **Camera:** FOV, move speed, look speed, Reset view button
- **Animation:** auto-morph toggle (default OFF), morph min/max, morph period

All settings persist to `localStorage` per browser. Refresh restores them.

## Stack A artifact

The Shadertoy-idiom reference implementation lives at
[`shadertoy/mandelbulb.glsl`](shadertoy/mandelbulb.glsl), with a port-mapping
note at [`shadertoy/README.md`](shadertoy/README.md). Paste the GLSL into
[shadertoy.com/new](https://www.shadertoy.com/new) and save to run; the
math and default parameters are identical to the Stack B side here.

## Mathematics

Distance-estimator raymarching of the Mandelbulb iterated map. See
[`docs/sim-specs/mandelbulb-explorer.md`](../../docs/sim-specs/mandelbulb-explorer.md)
for the equations, default parameters, and design rationale.

## References (consulted, no code lifted)

- Daniel White (2009), "The Mandelbulb: First 'true' 3D image of the famous fractal."
- Paul Nylander, hypercomplex fractals: <https://www.bugman123.com/Hypercomplex/>.
- Inigo Quilez, "Distance estimation": <https://iquilezles.org/articles/distfunctions/>.
  iq's website defaults to CC BY-NC-SA, so no shader code was lifted from
  there; the technique discussion was used as a general reference only.
- common-web (this repo): `Context`, `Renderer`, `Camera`, `RenderPipeline`,
  `Texture`, `Buffer`, `ParamPanel`, `HotReloader`, `StateWriter`,
  `StateReader` from [`common/common-web/`](../../common/common-web/).

## Build

From the repo root:

```sh
npm install                                          # if not done since last pull
npm run dev --workspace=@gpusims/mandelbulb-explorer-web
```

Open <http://127.0.0.1:5175> in a WebGPU-capable browser. (Port 5175 lets
this sim run side-by-side with hello-web on 5173 and strange-attractors on
5174.)

Production build:

```sh
npm run build --workspace=@gpusims/mandelbulb-explorer-web
```

Output is in `web/dist/`. The deploy workflow at
`.github/workflows/deploy-pages.yml` assembles this into the GitHub Pages
artifact at `/GPU-Sims/mandelbulb-explorer/`.

## Performance

To be characterized post-build. Three rows expected:

- **Desktop (RX 6800 XT, Chromium, default settings, 1080p, soft shadows on):** TBD ms/frame.
- **High-end (lab PC 2080 Ti, default settings, 1080p):** TBD ms/frame.
- **Render-scale 0.5 (same hardware/settings as above):** TBD ms/frame.

Per project-state.md § 9, GPU profiler timing runs CPU-only on Stack B until
the `mapAsync` issue lands a fix, so these numbers are CPU-side rAF deltas
rather than per-pass GPU times.
