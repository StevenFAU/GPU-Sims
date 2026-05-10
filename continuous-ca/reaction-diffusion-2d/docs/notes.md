# Reaction-Diffusion 2D — design notes

Free-form running notes from implementation and live tuning. v1.1 backlog
items live here until they earn a ticket.

## v1.1 polish backlog

- **U-channel display toggle.** Visualize V only in v1; toggle V|U|U+V in v1.1.
  One uniform field + one shader branch.
- **Per-channel min/max contrast.** Single (min, max) pair maps V in v1; split
  into per-channel pairs once U-channel display lands.
- **Adjustable brush profile.** Soft-disk falloff in v1; gaussian, hard-edge,
  tapered presets in v1.1. One uniform enum + one shader branch.
- **Right-click brush** for inverting (remove V, add U). Same kernel + a sign flag.
- **PNG snapshot button.** Reads the swap-chain image post-tonemap and saves
  as PNG. Independent of the simulation-state ZIP capture.
- **Autoplay through preset list** for hands-free demos. ~15 lines + a folder
  in the panel.
- **GPU-side reset kernel.** CPU reseed works at all v1 grid sizes; revisit
  if grid sizes ever scale past 4096².
- **Adaptive substepping** based on observed `dt·∇²f` magnitudes. Research-grade;
  not pursued.
- **Localized brush dispatch.** Currently dispatches over the full grid; could
  limit to a 32×32 region around the cursor. Saves ~99% of the brush kernel's
  cells; trivial cost overall, but the dispatch pattern would be cleaner.

## Pearson preset tuning observations

(To be filled in after first run-through.)

After running each preset for ~60 seconds at default `(F, k)`:
- λ (Irregular spots): TBD
- σ (Stripes): TBD
- α (Chaotic): TBD
- β (Uniform-ish): TBD — likely fragile per Phase 3's spec note (k = 0.055 sits
  near the stability edge).
- ξ (Moving spots): TBD
- τ (U-skate): TBD — Phase 3's spec flagged this as suspect (canonical U-skate
  is reportedly nearer F ≈ 0.062). Cross-check Munafo's catalog if patterns
  don't match expectation.

If any preset misses, edit one row of `presets.ts` and re-test. Polish-level;
not blocking on v1 ship.
