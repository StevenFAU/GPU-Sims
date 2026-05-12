# mpm-multimaterial — implementation notes

Polish-pass items and v1.1 stretch goals, banked here so they don't get lost
between phases. Distinct from `load-bearing-decisions.md` which captures
shape-locking choices; `notes.md` captures non-load-bearing followups.

## v1.1 stretch goals

- **Sand material (Drucker-Prager plasticity).** Adds material index 3 (SAND);
  per-material plasticity branch in `kernels.py` substep replaces SVD plastic
  clamping with cone-projected Cauchy stress. Reference: Klar et al. 2016
  "Drucker-Prager Elastoplasticity for Sand Animation" (SIGGRAPH). Visual demo:
  granular pile vs. jelly column collision; sand-on-water mixing.
- **Multi-channel VDB per-material density export.** Three density grids per
  frame for hero renders that need per-material volumetric treatment.
- **`watchfiles`-based process auto-restart wrapper.** Small
  `gpusims_common.process_watcher` module that monitors `*.py` files and
  re-execs `python main.py` on change. Banked until a concrete sim demands it.
- **Per-slider localStorage-equivalent persistence.**
  `gpusims_common.ParamPanel` constructor accepts a `persist_key` arg that's
  currently a no-op; v1.1 would write/read slider state to
  `~/.config/gpusims/<persist_key>.json` on change/load.
- **A100 hero-render animation pass.** Once A100 access is available, render
  the Mixed Sandbox preset at the 1M / 192^3 tier as a 4-second animation
  (120 frames @ 24 fps; ~10 hours of A100 Cycles rendering at 256 spp).

## Performance polish

- **Particle data SOA vs AOS.** Current `ti.Vector.field(3, ti.f32, N)` is AOS.
  Taichi 1.7's SNode layout API can re-arrange into SOA for cache-friendlier
  access at the substep level. Worth benchmarking against upstream.
- **Sparse grid representation.** The 192^3 = 7M-cell dense grid is mostly
  empty for most simulation states. Taichi 1.7 supports sparse SNodes
  (`ti.root.bitmasked(...)`); could shrink memory footprint by 5–10× at the
  cost of indirection overhead.
- **Vulkan vector-atomic-float path.** Current grid scatter uses scalar
  atomic-adds for portability. If the Vulkan runtime has
  `VK_EXT_shader_atomic_float`, the vector-atomic path is ~2–3× faster on
  dense scatter.

## UX polish

- **Visual feedback for LMB-place emitters.** A pre-place ghost cube
  (transparent wireframe) under the cursor would make placement intent visible.
- **Tier-change progress indicator.** Tier change locks the UI for ~1–3 s
  while Taichi recompiles. A "Recompiling…" overlay would clarify the wait.
- **F5 / F9 toast notifications.** On-screen 1-second toast on save/load.
- **Camera presets.** "Top-down" / "Side-view" / "Hero-angle" buttons matching
  the Blender render setup.
- **F-key save/load keybinding investigation.** Phase 9's `49c0559` polish
  pass added "save state" / "load latest" buttons in both hello and MPM
  Export panels because the spec's `F5`/`F9` keybindings did not fire on
  the AMD RX 6800 XT + Taichi Vulkan + X11 dev setup — captures dir
  empty when only F-keys pressed, populated when buttons clicked. Root
  cause unverified: either Taichi GGUI doesn't route F-keys through its
  event system on this backend (no F-key constants in
  `taichi/ui/constants.py`; zero F-key references in Taichi's own
  examples), or the event-string is uppercase `"F5"` while the spec used
  lowercase `"f5"`. The polish-3 commit kept the F-key handlers with
  case-insensitive matching plus added a `log.info("key event: %r", ev.key)`
  diagnostic. Next-session investigation: run either sim, press
  Fn+5/Fn+9/regular letters/space, copy the `key event:` lines from
  stderr, and decide between (a) one-line keybinding fix if Taichi
  reports a known string, or (b) drop the F-key handlers entirely and
  demote the diagnostic if Taichi doesn't fire F-key events at all.
  Banked Phase 9.

## Banked CI workflow items

- **GPU runner for build-py.yml.** CI currently exercises only the Taichi CPU
  backend. GPU-equipped runner would let CI exercise CUDA / Vulkan paths.
- **Headless Blender render in CI.** `render_mpm.py` is currently exercised
  manually. CI could install Blender (apt: `blender`) and render a 2-frame
  smoke at low resolution.

## Known minor issues

- `ti.tools.PLYWriter.add_vertex_channel` is undocumented in Taichi 1.7 API
  docs but works correctly per direct source inspection at
  `taichi/tools/np2ply.py`. If Taichi 1.8+ removes this API, fallback to
  numpy-direct PLY writing.
- Material 0 / 1 / 2 in `set_color_by_material` uses positional ordering
  matching `WATER / JELLY / SNOW` constants; baked into both `material_colors`
  indexing and `set_color_by_material`. If v1.1 material reordering ships,
  both sites must update together.
