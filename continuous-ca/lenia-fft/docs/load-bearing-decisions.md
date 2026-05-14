# Lenia-FFT — Load-bearing decisions

Phase 10 decisions specific to this sim. The repo-wide architecture lives in
`/project-state.md` § 4; this file is the sim-local detail. Cross-references:

- Repo-wide § 7 (Conventions): `from __future__` × `@ti.kernel`, no `np.float64`,
  cross-stack capture schema (sim-namespaced meta wrapper), Stack D dual-backend
  posture (CUDA + Vulkan), runtime-only-surface convention, rule-of-three for
  `common-*` promotion.
- Phase 9 (`mpm-multimaterial`) — first `common-py` consumer; this sim is #2.
- Phase 8 (`eulerian-smoke`) — VDB writer first consumer; lenia-fft 3D tier
  is the second VDB consumer.

## Runtime FFT-backend selection (priority order)

Lenia's substance is `K ⊛ state` per-step. Taichi 1.7.4 has no built-in FFT
(verified at draft-time sandbox: `dir(ti)` / `ti.math` / `ti.tools` show no
fft entries). The overarching-spec's "2048² real-time Lenia" promise requires
GPU FFT on at least one of the user's hardware paths (NVIDIA CUDA lab PC or
AMD Vulkan/ROCm desktop).

**Decision:** Ship four backends in priority order; pick at runtime via
`select_backend()`; all four implement the same numpy-in/numpy-out
`LeniaConvolver` interface.

| Pri | Backend | Hardware | pip extra | Where exercised |
|---|---|---|---|---|
| 1 | `CuPyFFTConvolver` | NVIDIA CUDA | `.[cuda]` (cupy-cuda12x) | user lab PC |
| 2 | `TorchFFTConvolver` | NVIDIA CUDA or AMD ROCm | `.[cuda-torch]` or `.[rocm]` (torch>=2.1) | user AMD desktop |
| 3 | `TaichiRealSpaceConvolver` | universal (CUDA + Vulkan) | none — uses baseline Taichi | both user machines; CI Taichi-CPU |
| 4 | `NumpyFFTConvolver` | CPU | none — baseline numpy | CI smoke; headless dev |

**Probe-and-log discipline:** Each backend is import-guarded and smoke-tested
on a single-cell-impulse `(n_grid, n_grid)` field at init. The first that
passes is selected. Failures log at `log.info` (not warn — every run on AMD
without CuPy would otherwise warn). The selected backend is logged once at
sim start.

**Tier-label runtime mutability:** The 2048² stretch tier's interactivity
depends on which backend was selected. The Tier dropdown label is mutated
post-init: "2048² stretch (FFT)" if CuPy/Torch selected (interactive);
"2048² stretch (real-space, capture-mode)" if Taichi-real-space selected
(capture-mode triggers confirmation modal). This is a new convention
introduced this phase; documented in `/docs/conventions.md` "Runtime
backend selection (Stack D)".

**The naming awkwardness:** The sim is called `lenia-fft` but ships
real-space + numpy backends too. The `-fft` suffix denotes the Fourier-domain
*intent* of the sim's research thesis (convolution-as-fundamental-operation,
separable in frequency space), not a hard requirement that every backend
implement via FFT. Future-you reading this: the name is the thesis, not the
implementation.

**CI surface:** Only `NumpyFFTConvolver` and `TaichiRealSpaceConvolver`
(via Taichi CPU backend) are exercised in CI (`build-py.yml`). CuPy and
Torch are user-runtime-only verification — per the Phase 9
runtime-only-surface convention.

## 2D + 3D in one phase (2D load-bearing, 3D opt-in stretch)

**Why this is load-bearing:** Bert Chan's canonical Lenia is 2D; published
creature presets (Orbium unicaudatus, Vagorbium undulatus, Gyrorbium gyrans,
Discutium valvatus, etc. — 122 single-peak creatures enumerated upstream)
are all 2D. 3D Lenia exists in the research literature but is harder along
three dimensions: O(N³) convolution cost, volumetric visualization, smaller
parameter-stability window.

**Decision:** Ship 2D as the load-bearing path with the full UX (brush-paint,
parameter sliders, save/load, tier dropdown with three sizes, four named
creatures). Ship 3D as an opt-in fourth tier (128³ capture-mode with
confirmation modal, one generic preset, iso-cross-section-slice viewer,
VDB-density export). 3D shares the kernel structure but adds: cross-section
slice extraction kernel, 3D state allocation, VDB-export branch in the
main loop, `render_lenia_3d.py` hero render.

**Explicit scope-cut contingency:** If 3D proves too much scope at Claude Code
execution time, the documented escape is "ship 2D-only as Phase 10, bank 3D
as Phase 10.5." The file manifest is structured so a 3D cut is localized:

- Drop tier 3 from `TIERS_BASE` in main.py.
- Drop the 3D preset from `presets.py`.
- Drop `lenia_step_3d` / `swap_state_3d` / `extract_slice_3d` /
  `init_state_random_blob_3d` / `init_kernel_radial_3d` from kernels.py.
- Drop VDB-export branch from main.py.
- Drop `render_lenia_3d.py` from file manifest.
- Drop `vdb_export/.gitignore` placeholder.
- Drop the 3D-tier section from this file.

No `common-py` ripple. No cross-stack ripple. No CI surface change beyond
dropped sim-side tests. This contingency is documented so future-you knows
it's clean to execute if needed.

## 3D viewer in v1: iso-cross-section slice (NOT volumetric raymarch)

The natural visualization of a 3D scalar field is volumetric raymarch
(Beer-Lambert density-attenuation along view rays). Implementing it requires:

- Custom `@ti.kernel` writing into a 2D image
- Ray-step + density-accumulate + alpha-blend per pixel
- Camera setup + transfer function
- Likely sparse-ray-skipping to hit interactive frame rates at 128³

That's a significant shader effort — comparable in scope to a mini volume
renderer. **v1 ships iso-cross-section-slice display instead**: a 2D slice
of the 3D field along XY/XZ/YZ axes (radio button in panel) at a chosen
slice-position (slider 0–N), displayed via `canvas.set_image(slice_img)`.
The slice is computed each frame by `extract_slice_3d` kernel.

`common-py.Camera` is instantiated for the 3D tier (used for slice-orientation
UX) but the 3D viewer does NOT use `ti.ui.Scene`. The hero volumetric render
is offline via `render_lenia_3d.py` Blender Cycles.

Volumetric raymarch banked v1.1.

## 2D camera divergence from `common-py.Camera`

Phase 10 is the first Stack D sim where `common-py.Camera` does NOT apply
to the primary viewer. Lenia 2D is rendered via `canvas.set_image(state_field)`
with no `ti.ui.Scene`. The "camera" is a pan-zoom transform on the 2D field:
`pan_x, pan_y ∈ [-N/2, +N/2]`, `zoom ∈ [0.25, 8.0]`. The transform lives in
`composite_view_2d` (Taichi kernel) + its inverse in `cursor_to_field_cell`
(Python function). Both are sim-local.

The 3D tier reuses `common-py.Camera` (FreeFly mode) for slice-orientation
UX. The camera's pose is saved to F5/F9 state alongside slice_axis +
slice_idx.

**Promotion candidate at consumer #3?** `cursor_to_field_cell` is a 2D
pan-zoom inverse. MPM has `cursor_to_ground` (3D ray-plane unproject). Lenia
3D-slice has implicit `cursor_to_slice_cell` (2D-on-slice unproject). Three
different signatures, no common abstraction yet — KEEP sim-local at #3.

## Brush paint: LMB-drag, not edge-trigger

LMB-held-and-moving paints a Gaussian splat per frame; RMB-held erases with
the same brush profile but forced negative intensity. The Brush panel has
radius (1–80 px) + intensity (-1.0–+1.0) sliders.

This diverges from MPM Phase 9 LMB-place (edge-triggered, one click → one
cube). Lenia's UX is continuous strokes — drag detection (polled
`window.is_pressed(LMB)` every frame, no edge state).

GUI-occlusion gating inherits MPM:306–318's `_cursor_in_any_panel` pattern.
Sim-local copy; promotion candidate at consumer #3 (STRONG; see promotion
review below).

Save-creature ("RMB-snapshot named creature to library") banked v1.1.

## Hero render: two paths

- **`render_lenia_2d.py`** consumes the `frames_export/` PNG sequence
  (output of `ti.tools.imwrite` per-Nth-frame in the sim), applies Cycles
  compositor (RGB Curves color-grade + saturation boost + vignette via
  Mask multiply + optional Blur Time temporal smoothing), outputs MP4.
  Phase-10 first Cycles-compositor-without-3D-geometry script in the repo.

- **`render_lenia_3d.py`** consumes the `vdb_export/density/` VDB sequence
  (output of `gpusims_common.vdb_writer.write_float_frame` per-Nth-frame
  in the sim), loads into a Blender Volume domain, sets up Principled
  Volume shader with color-ramp transfer function + scatter + emission,
  GPU-device fallback chain (OptiX → HIP → CUDA → fail-loud). Same
  template as Phase 8 `render_smoke.py`.

- **Houdini path banked, not built.** When license access lands, the
  captured state formats are Houdini-compatible (VDB native, PNG sequence
  trivial). Integration is a future workstream; do not write a Houdini
  script speculatively in v1.

- **Fast-iteration video:** `ti.tools.VideoManager` encodes per-frame canvas
  captures to MP4 in-sim. End-of-session or on explicit Export Video toggle.
  Output to `frames_export/video_<frame>/...`. No color grading; this is
  the "raw" video. The hero scripts produce the "polished" video.

## Capture schema (extends Phase 9 sim-namespaced meta wrapper)

Per `/tier1-capture-format-reference.md` § 1 (sim-namespaced meta wrapper),
the lenia-fft meta wrapper key is `leniaFft`. Schema:

```json
{
  "dim": 2,
  "tier_idx": 0,
  "n_grid": 512,
  "kernel_radius": 13,
  "time_resolution": 10.0,
  "mu": 0.15,
  "sigma": 0.015,
  "preset_name": "Orbium unicaudatus",
  "view": {"pan_x": 0.0, "pan_y": 0.0, "zoom": 1.0},
  "camera": {<Camera.to_json() output, 3D tier only>},
  "slice_axis": "XY",
  "slice_idx": 64,
  "fft_backend_at_save": "CuPy FFT (CUDA)",
  "brush": {"radius": 8.0, "intensity": 0.5}
}
```

Per-buffer entries: `state` (r32f, shape `[N,N]` or `[N,N,N]`),
`kernel_lut` (r32f, shape `[2R+1, 2R+1]` or `[2R+1, 2R+1, 2R+1]`).

No new entries needed in the repo-wide format-string table — `r32f` is
already in `tier1-capture-format-reference.md` from Phase 9 + earlier.

The `fft_backend_at_save` field is informational only; the loader picks
its own backend at init (the save-side machine and load-side machine may
differ — common with the user's two-machine setup).

## Promotion-review for `common-py` (consumer #2)

Per the rule-of-three convention (`/project-state.md` § 7): patterns
identified as "this might want to be in `common-*`" during a sim phase
do not promote on the first consumer. They promote at the THIRD consumer
where the abstraction's shape is empirically validated by repeated use
rather than speculatively designed. The first two consumers keep per-sim
copies of the pattern; the third's spec includes the promotion review.

Phase 10 is consumer #2. This section documents promotion candidates with
rationale. **No code is extracted in Phase 10.** Phase 10's main.py includes
sim-local copies of all patterns identified here.

**Seven candidates inventoried** (synced source citations at
`b914892`):

1. **`_cursor_in_any_panel` GUI-occlusion test** (MPM
   `hybrid-particle-grid/mpm-multimaterial/python/mpm_multimaterial/main.py:306–318`)
   **— STRONG PROMOTE at #3.** Every interactive Stack D sim with sliders
   needs a "don't drop a thing on the canvas when the cursor is over a
   panel" test. The pattern is structurally identical between MPM and
   Lenia: panel rects list + bottom-y-to-top-y conversion. The
   abstraction's shape is clear: `cursor_in_any_panel(cursor, rects)
   -> bool`. Likely future API in common-py: `ParamPanel.is_cursor_over(window)`
   that wraps the panel-rect tracking internally so consumers don't
   manually maintain GUI_PANEL_RECTS lists.

<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
2. **Capture-mode confirmation modal** (MPM `main.py:608–618`)
   **— STRONG PROMOTE at #3.** Tier-dependent capture-mode appears in
   Phase 8 smoke (384³), Phase 9 MPM (500k tier), Phase 10 Lenia (2048²
   FFT-dependent + 3D 128³). The modal's shape is consistent:
   `pending_capture_mode_modal: int | None` sentinel + `with
   panel.folder("⚠ ...", ...)` rendering + Continue/Cancel buttons.
   Likely future API: a `ParamPanel.capture_mode_modal(label,
   on_continue, on_cancel)` helper, or a more general
   `ParamPanel.modal(...)` if other modal flows surface.

3. **Tier dropdown + deferred-change-after-`window.show()`** (MPM
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
   `main.py:560–567 + 624–642`)
   **— STRONG PROMOTE at #3.** The deferred-after-show idiom is a
   non-obvious UX subtlety: re-allocate Taichi fields BEFORE the next
   step starts, AFTER the current GUI frame finishes (otherwise the
   GUI's checkbox state visually flickers). This is exactly the kind
   of thing the package should encapsulate. Likely future API: a
   `TierDropdown` widget that takes a list of `(label, on_select)`
   tuples and defers the `on_select` callback to after `window.show()`.

<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
4. **F5/F9 save-load buttons UX pattern** (MPM `main.py:599–605`)
   **— MODERATE candidate.** The buttons live in each sim's GUI block
   (sim-specific labels, sim-specific side-effects) so the call sites
   stay sim-local. The pattern is consistent: button-tap → invoke
   save/load handler → log. If a third consumer (`neural-ca` Stack D
   variant likely) surfaces, promote a thin
   `ParamPanel.save_load_buttons(panel, on_save, on_load)` helper that
   renders the buttons and dispatches.

5. **`set_color_by_material` re-apply pattern** (MPM
   `hybrid-particle-grid/mpm-multimaterial/python/mpm_multimaterial/kernels.py`,
   `set_color_by_material`)
   **— KEEP SIM-LOCAL.** MPM-specific (Lenia has no discrete materials).
   The abstraction doesn't generalize to a continuous-CA sim.

<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
6. **Reserve-tail emitter allocation** (MPM `main.py:466–472` +
<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
   `EMITTER_RESERVE_SIZE` `main.py:96–100`)
   **— KEEP SIM-LOCAL.** MPM-specific physics-faithfulness move (preserve
   preset particles while LMB-place claims from a reserved tail region).
   Lenia's brush paints into the state field directly — no allocation
   needed. The abstraction doesn't generalize.

<!-- integrity-allow: cat1.intra-repo; grandfathered-pre-v1 (see grandfather-catalog other-cat1); n/a -->
7. **`cursor_to_ground` 3D ray-plane unproject** (MPM `main.py:187+`)
   **— KEEP SIM-LOCAL.** Lenia 2D uses `cursor_to_field_cell` (2D pan-zoom
   inverse). Lenia 3D-slice uses an implicit `cursor_to_slice_cell` (2D-
   on-slice unproject). MPM uses `cursor_to_ground` (3D ground-plane
   unproject). Three different signatures, no common abstraction yet.
   Likely future work: at consumer #4 (whenever a sim needs an
   arbitrary-plane unproject), revisit and promote if a coherent
   abstraction emerges.

**Candidates the consumer-#3 phase spec should re-review FIRST:** 1, 2, 3, 4.
The first three are STRONG promote; #4 is MODERATE. Consumer #3 should
allocate spec budget for the promotion work (architect-1 spec section
"Promote from `common-py`" with API design + migration plan for both
existing consumers).

## Stack B WebGPU port: deferred to a later phase

Phase 10 = Stack D only. The Stack B port is its own phase (likely Phase 11
or 12) and benefits from waiting until Phase 10's parameter space and known-
stable creatures settle — porting "this Lenia configuration with these
specific parameters" is a tighter scope than porting "Lenia in general."

## Tiers: 512² default (Chan canonical baseline, not 1024²)

Banking from Phase 9 polish-4 "default = canonical upstream baseline":
Chan's canonical Lenia experiments run at 256² or 512². 512² as default
matches the upstream; 1024² as default is a step above and risks the
"spec asserted X fps, reality was Y" pattern.

Tier table:

| Tier | Dim | Grid | Mem | Taichi-real-space | GPU-FFT (CuPy/Torch) |
|---|---|---|---|---|---|
| 0 default | 2 | 512² | ~5 MB | 60+ fps | 60+ fps |
| 1 mid | 2 | 1024² | ~12 MB | 30–60 fps | 60+ fps |
| 2 stretch | 2 | 2048² | ~50 MB | 5–15 fps capture | 30–60 fps |
| 3 3D stretch | 3 | 128³ | ~32 MB | 5–15 fps capture | N/A (3D FFT v1.1) |

Memory estimates: state (NxN or NxNxN) + state_next + kernel_lut (small) +
view_img (NxN) at f32 = 4 bytes/cell. 2048² = 16 MB × ~3 buffers ≈ 50 MB.
128³ = 8 MB × ~3 buffers + slice = 32 MB.

## Substeps per frame: 1 in v1; sub-stepping banked v1.1

Lenia time step is `dt = 1/T` where T is the per-preset time-resolution
constant. v1 ships **1 substep per frame**. Sub-stepping (multiple
internal substeps per displayed frame, used in MPM Phase 9 for stability
at large dt) is banked v1.1 polish.
