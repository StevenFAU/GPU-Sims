# Reaction-Diffusion 2D

Gray-Scott pattern explorer with six Pearson 1993 named presets, mouse-paint
brush, and full-state capture. Live at
<https://stevenfau.github.io/GPU-Sims/reaction-diffusion-2d/>.

This is the third Stack B sim and the second Stack A → B port — it preserves
the Steven-original Shadertoy reference at
[`shadertoy/`](shadertoy/) alongside the WebGPU port at [`web/`](web/).

## What you're seeing

Two chemical concentrations `u` and `v` diffusing across a periodic 2D grid
and reacting via the Gray-Scott model. The visible color is `v` (the minority
species that forms the patterns) mapped through a perceptual colormap. The
six built-in presets — λ, σ, α, β, ξ, τ — are points in the Pearson 1993
parameter space where distinct stable pattern types form: spots, stripes,
chaos, drifting blobs, moving spots, and self-replicating spots.

The grid wraps toroidally (left edge meets right; top meets bottom), so
patterns drifting in one direction reappear on the opposite side.

## Controls

- **Left-click and drag** in the canvas to paint `v` material (the canonical
  RD interaction). Drag across the canvas to seed pattern formation faster.
- **Right side panel** — pick a Pearson preset, tune `(F, k)`, change grid
  size, swap colormaps, adjust brush radius/strength.
- **F5** — save full simulation state to `capture_NNNN.zip`. The download
  fires immediately. Includes camera + parameter block + raw `u.bin` + `v.bin`
  for the live grid.
- **F9** — load a saved capture. File picker opens; pick a previously-saved
  ZIP.

## Stack A artifact

A standalone Shadertoy implementation lives at
[`shadertoy/`](shadertoy/) — two GLSL files (`BufA.glsl` + `Image.glsl`) plus
a setup README. Same Gray-Scott math; smaller and more readable. Paste into
shadertoy.com per the instructions there.

## Mathematics

Gray-Scott reaction-diffusion with Forward Euler integration:

```
∂u/∂t = Du · ∇²u  −  u·v²  +  F · (1 − u)
∂v/∂t = Dv · ∇²v  +  u·v²  −  (F + k) · v
```

5-point Laplacian in space, normalized `dx = 1, dt = 1`, periodic boundaries.
Default canonical diffusion `Du = 0.16, Dv = 0.08`. The six presets set
distinct `(F, k)` values; the literature places these in approximate regions,
not single points, so the panel exposes both the dropdown and per-parameter
sliders.

References (consulted, no code lifted):

- Pearson, J. E. (1993). "Complex Patterns in a Simple System." *Science*
  261(5118), 189–192.
- Robert Munafo, "Reaction-diffusion by the Gray-Scott model" —
  <https://mrob.com/pub/comp/xmorphia>.
- Inigo Quilez's polynomial colormap fits, public-domain via
  <https://www.shadertoy.com/view/WlfXRN>.

## Build

```
node --version    # must be 22 LTS or newer
npm install       # from repo root, picks up workspaces
npm run dev --workspace=@gpusims/reaction-diffusion-2d-web
```

Then open <http://127.0.0.1:5176> in a WebGPU-enabled browser. Production
build:

```
npm run build --workspace=@gpusims/reaction-diffusion-2d-web
```

Outputs `dist/` for deploy.

## Performance

| Grid | Substeps/frame | Frame time target |
|------|----------------|-------------------|
| 256² | 4 | < 1 ms (60 fps trivially) |
| 512² | 4 | TBD (measure post-build) |
| 1024² | 4 | TBD |
| 1024² | 32 | TBD |

Bandwidth-bound, not compute-bound. The 6800 XT should handle 1024² × 32
substeps × 60 fps comfortably; integrated GPUs may need to drop to 256² or
512².

## License

MIT. See [`LICENSE`](../../LICENSE) at the repo root. Per-reference license
notes are in [`shadertoy/README.md`](shadertoy/README.md).
