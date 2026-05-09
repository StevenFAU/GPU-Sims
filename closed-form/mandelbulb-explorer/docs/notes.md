# Mandelbulb Explorer — design notes

Free-form notes from design iteration. Architects writing v1.1+ append
here as they work; nothing in this file is authoritative.

## v1 — first interactive session observations (post-Phase 4 ship)

These are observations from the first hands-on session with the running
sim, recorded for v1.1+ work. Nothing here is a defect — the v1 math is
dimensionally correct and the slider directions are intuitive. These are
visual-tuning notes.

### Trap-coloring saturates at default parameters

At default `trapRadius = 1.0` and `iterCap = 8`, `|z|_min` for most
surface points clamps the per-pixel `kPt = clamp(|z|_min / r, 0, 1)` to
near 1, so the fractal renders dominated by `colorCool` with the warm
`colorHot` barely mixing in. The README's "warm-cool gradient most
mandelbulb images use" reads aspirationally rather than descriptively at
these defaults.

Sliding `trapRadius` up to ~2.5 brings `colorHot` in as a strong global
tilt — the surface goes coral instead of deep blue — but the look is a
two-tone tilt across the whole fractal, not per-pixel variegation where
some surface features render hot and others cool side-by-side.

### What's needed for per-pixel variegation (v1.1 candidates)

1. **Higher iterCap default.** At n=8 power, ≥12 iterations are needed
   for orbit trajectories to differentiate enough that adjacent surface
   points fall into meaningfully different `|z|_min` bins. The current
   default of 8 is on the cusp.
2. **Smoothstep ramp with adjustable lo/hi bounds** instead of the
   current linear `clamp(0, 1)`. A `smoothstep(trapLo, trapHi, |z|_min)`
   with both bounds exposed as sliders would let the user shape the
   gradient's response curve directly. Default suggestion: lo=0.2, hi=1.0
   gives a softer transition than the current hard linear ramp.
3. **Perceptual gamma on the ramp** — a `pow(k, 0.45)` or similar before
   the `mix()` would push more pixels into the "interesting middle"
   range without changing the endpoints.
4. **Per-trap-mode independent radius.** Right now `trapRadius` is shared
   between point and planes traps. The planes trap concentrates `|z|_min`
   in a different range; a separate slider would let each preset look
   correct simultaneously.

### Coloring v1.1 sketch (one possible shape)

Add to lil-gui Coloring folder:
- `trapLo` slider, [0, 1], default 0.2
- `trapHi` slider, [0, 4], default 1.0
- `trapGamma` slider, [0.2, 2.5], default 0.45
- Drop the current single `trapRadius` and `1.4` / `2.0` mode-specific
  multipliers; replace with the smoothstep + gamma combo.

This is a v1.1 polish item, not a v1 defect. The v1 math is right; v1.1
makes the slider feel more like a paintbrush.

### Other observations

- Soft shadows produce excellent contact shading on the bumpy outer
  iterates; turning them off makes the surface read as much flatter (as
  expected). Default ON was the right call.
- Free-fly + RMB-drag camera works as designed; off-axis initial position
  immediately shows fractal structure. Reset view button is useful when
  flying into interior pockets.
- Auto-morph slider continuity (the v3-pass fix from architect cross-review)
  works correctly — toggling on/off doesn't snap n discontinuously.
- Render-scale slider visibly reduces resolution at 0.5 with no
  perceptible UI lag (recreation on `onFinishChange` was the right call).

### Spec defects encountered during Phase 4 execution

For future architect chats, banking the lesson:

- § 12.1 anchor was wrong. Spec expected a 10-leading-space command line
  (`          npm run build --workspace=@gpusims/strange-attractors-web`),
  but the as-built `deploy-pages.yml` uses per-step `name:` / `run:`
  blocks where each workspace is its own step.
- § 12.2 anchor was wrong. Spec expected
  `cp -r closed-form/strange-attractors/web/dist _site/strange-attractors`,
  but the as-built line is
  `cp -r closed-form/strange-attractors/web/dist/* _site/strange-attractors/`
  (glob form + trailing slash) and is preceded by
  `mkdir -p _site/strange-attractors`.

Both were adapted on the fly during execution; the resulting CI behavior
matches what the spec intends. The pattern is the same drift class that
caught Camera.lookAt, StateWriter API, and JsonValue imports in earlier
review rounds: spec memory drifts from synced source, only re-reading
catches it. The architect-side rule "read the actual source, the spec is
not authoritative" applies equally to CI YAML as to common-web TypeScript.
