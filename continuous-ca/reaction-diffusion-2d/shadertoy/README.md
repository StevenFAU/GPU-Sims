# Stack A — Shadertoy reference

Steven-original Shadertoy implementation of the Reaction-Diffusion 2D sim.
This is the Stack A half of the Stack A → B port; the Stack B WebGPU port
lives at `../web/`.

## What's here

This is a **multi-buffer Shadertoy artifact** — two GLSL files that together
implement the simulation:

- `BufA.glsl` — the simulation pass. Reads the previous frame's state via
  `iChannel0` (self-referenced), runs one Forward Euler substep, and writes
  the new state. Frame 0 writes the initial conditions instead of stepping.
- `Image.glsl` — the visualization pass. Reads `BufA` via `iChannel0`,
  applies a magma colormap, displays.

The "multi-buffer idiom" is Shadertoy's standard pattern for sims with
persistent state — Shadertoy's `Image` pass cannot ping-pong with itself
because it writes directly to the canvas. `BufferA` is the persistent
state texture; `Image` is the read-only display layer.

This is the **first multi-file Shadertoy artifact in the repo**. Phase 4's
`mandelbulb-explorer/shadertoy/` was a single `mandelbulb.glsl` because
mandelbulb is stateless. RD-2D needs persistent state, so it gets two
files. The convention extension is documented in
`../docs/load-bearing-decisions.md`.

## To run

1. Open <https://www.shadertoy.com/new>. Sign in.
2. In the bottom toolbar, click `+` next to `Image` and add a `Buffer A` tab.
3. Paste the contents of `BufA.glsl` into the **Buffer A** tab.
4. In Buffer A's `iChannel0` input slot, click and select `Misc → Buffer A`
   (self-reference). Set the channel's filter to `Nearest` and wrap to `Clamp`.
5. Paste the contents of `Image.glsl` into the **Image** tab.
6. In Image's `iChannel0` input slot, select `Misc → Buffer A`. Filter `Linear`,
   wrap `Clamp`.
7. Hit Play. Pattern formation takes ~30 seconds at 60 fps.

## Controls

- **LMB-drag in the canvas:** paint `v` material onto the grid (the canonical
  RD interaction). Drag across the canvas to seed pattern formation faster.
- **`(F, k)` constants in `BufA.glsl` line 24–25:** edit to switch Pearson
  presets. Values for all six are listed in the comment header.

## Port mapping (Stack A → Stack B)

The Stack B port at `../web/` keeps the same Gray-Scott math, the same
six Pearson presets, and the same brush interaction. Differences:

| Concern | Stack A (Shadertoy) | Stack B (WebGPU) |
|---|---|---|
| Update language | GLSL fragment | WGSL compute kernel |
| State texture | `Buffer A` (auto-managed) | Two explicit storage textures, ping-pong via bind groups |
| Substeps per frame | 1 (Shadertoy model) | 4 default, slider 1–32 |
| Visualization | Image pass + magma | `visualize.frag.wgsl` + 4-LUT colormap dropdown |
| Brush splat | Inlined into BufA | Separate `brush_stamp.compute.wgsl` |
| Preset switching | Edit constants | lil-gui dropdown, six presets |
| Capture / load | None | F5 saves full state to ZIP, F9 loads |
| Periodic BCs | Manual `wrap()` in BufA | REPEAT sampler on the read binding |
| Initial conditions | Inlined in BufA at `iFrame == 0` | CPU-side seed with deterministic xorshift, uploaded via `Texture.uploadDirect2D` |

The Stack B port runs the simulation faster (multi-substep), supports
parameter sweeps without recompiling, and captures replayable state — the
trade-off is more code. The Shadertoy version is the design's reference
sketch and stays maintained as the readable single-page snapshot.

## License

MIT, matching the rest of the GPU-Sims repository. References consulted:

- Pearson, J. E. (1993). "Complex Patterns in a Simple System." *Science*
  261(5118), 189–192. The original (F, k) regime catalog.
- Robert Munafo, "Reaction-diffusion by the Gray-Scott model" — the
  authoritative online catalog at <https://mrob.com/pub/comp/xmorphia>.
- Inigo Quilez's polynomial colormap fits, public-domain via
  <https://www.shadertoy.com/view/WlfXRN>. Used inline in `Image.glsl`.

No GLSL code was lifted from any specific reference shader.
