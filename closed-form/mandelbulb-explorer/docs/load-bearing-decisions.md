# Mandelbulb Explorer — load-bearing decisions

These decisions are expensive to revisit. Read this before drafting v1.1.
Per-decision rationale is in `phase4_mandelbulb_explorer.md` § 2.

| Decision | Why load-bearing |
|----------|------------------|
| Math implemented from scratch; no third-party shader code lifted | License clarity; iq's site is mostly CC BY-NC-SA |
| Stack A artifact preserved at `shadertoy/` alongside Stack B at `web/` | Establishes the A → B port flow as a documented pattern |
| Single-pass raymarch + offscreen HDR + Reinhard tonemap | Required for the render-scale slider to actually reduce GPU cost |
| HDR offscreen RT format = `rgba16float` | Allows exposure > 1 without clipping; leaves bloom door open for v1.1 |
| Single uniform buffer for the raymarch pipeline (camera + DE params + coloring all in one struct) | Adding fields stays forward-compatible; splitting later is a re-plumb |
| F5 capture schema (camera + DE params + coloring + render-scale + auto-morph state, schemaVersion=1) | v1 captures must load in v2+; renames break replay |
| Stack B port lives at `closed-form/mandelbulb-explorer/web/` | Auto-resolves under the existing `closed-form/*/web` workspace glob |
| Default camera mode is free-fly, NOT auto-orbit | Mandelbulb is a fly-into fractal, not an orbit-around manifold |
| Auto-morph default OFF (slider is source-of-truth for `n`) | Avoids the lil-gui frozen-slider issue (project-state.md § 9) without code workarounds |

## Cheap to revisit (knobs, defaults, polish)

The full "cheap to revisit" list lives in the phase spec § 2.1 and isn't
duplicated here — defaults and slider ranges drift naturally as the user
plays with the sim, and this doc would go stale fast. The shader files
(`web/shaders/raymarch.frag.wgsl`, `web/shaders/tonemap.frag.wgsl`) and the
runtime defaults in `web/src/main.ts`'s `defaultRuntime()` are the
authoritative current state.
