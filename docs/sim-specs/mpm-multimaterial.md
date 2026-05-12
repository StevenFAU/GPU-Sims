# MPM Multi-Material — Specification

> **Status:** Implemented (Phase 9)
> **Category:** Hybrid particle-grid
> **Primary stack:** D (Python / Taichi)
> **Secondary stack(s):** —
> **Target machine:** AMD RX 6800 XT desktop primary (Vulkan); RTX 2080 Ti lab PC
> (CUDA); A100 batch for v1.1 hero animation
> **Folder:** [`hybrid-particle-grid/mpm-multimaterial`](../../hybrid-particle-grid/mpm-multimaterial/)

---

## 1. Goal and audience

Demonstrate the canonical multi-material MLS-MPM sandbox — water, jelly, snow —
sharing the same grid in real time on commodity GPUs. The intended viewer recognizes
the Taichi MLS-MPM result from SIGGRAPH papers and wants to interact with it (place
material cubes, change gravity, swap materials) rather than just watch.

Three felt qualities the sim must produce:

1. **Material identity is visually obvious.** Water flows, jelly wobbles, snow
   compacts. The viewer should never have to read a UI label to identify which
   material is which.
2. **Interaction is immediate.** LMB places a material cube; the user sees it fall,
   collide, splash within milliseconds. No load times, no progress bars.
3. **Three distinct scale tiers are exposed transparently.** The user knows they
   can crank up to 1M particles but accepts that it'll be capture-mode-only;
   no surprise framerate cliff.

---

## 2. Mathematical formulation

Material-Point-Method (MPM) is a hybrid Lagrangian-Eulerian method: state lives on
particles, but updates use an intermediate background grid for spatial averaging and
stress-divergence approximation. The variant shipped is **MLS-MPM** (Moving Least
Squares MPM, Hu et al. 2018), which uses quadratic B-spline weights and an affine
particle-in-cell (APIC) velocity reconstruction.

**Per-substep:**

1. **Clear grid.** Set `grid_v = 0`, `grid_m = 0` everywhere.
2. **Particle-to-Grid (P2G).** For each particle:
   - Compute deformation gradient update `F = (I + dt·C) · F`.
   - Branch by material:
     - **Water (mu=0):** Compute SVD; reset `F` to identity with `F[0,0] = J`
       (volumetric-only deformation).
     - **Jelly (h=0.3):** Small Lamé multiplier; soft elastic response.
     - **Snow:** Plastic-clamp SVD singular values to `[1 - 2.5e-2, 1 + 4.5e-3]`;
       accumulate plastic Jacobian `Jp`; hardening multiplier `h = exp(10·(1 - Jp))`.
   - Compute stress `σ = 2μ(F - UV^T)F^T + λJ(J-1)I`.
   - Scatter momentum + stress contribution to the 3×3×3 surrounding grid cells
     via quadratic B-spline weights.
3. **Grid update.** Normalize velocity by mass; add `g·dt`; apply zero-velocity
   boundary at the 3-cell-thick boundary on each side.
4. **Grid-to-Particle (G2P).** Each particle gathers velocity + affine matrix from
   its 3×3×3 grid neighborhood; advect position by `dt · v`.

**Material parameters:**

- Young's modulus E = 1000 (dimensionless)
- Poisson's ratio ν = 0.2
- Particle density ρ = 1.0
- dt = 2 × 10⁻⁴ (25 substeps per visual frame at 60 fps)

References for the per-material plasticity:

- Water as J-only deformation: standard MPM water model; Hu et al. 2018 § 4.
- Jelly with constant low h: empirical Taichi mpm3d_ggui convention.
- Snow plastic clamping: Stomakhin et al. 2013 § 3.

**Sand (v1.1 banked):** Drucker-Prager plasticity — cone-projection of Cauchy
stress in principal-strain space. Reference: Klar et al. 2016. Not implemented
in v1; see `docs/load-bearing-decisions.md`.

---

## 3. Stack assignment and rationale

**Stack D (Python / Taichi).** Selected over the alternatives:

- **Stack C (native C++/Vulkan):** would require implementing MLS-MPM kernels in
  GLSL/HLSL compute shaders from scratch. Taichi's whole reason for existing is
  to dodge that work — Yuanming Hu (Taichi co-author) is the MLS-MPM paper's first
  author; Taichi's flagship demo is the canonical 88-line MPM. Re-implementing
  in Stack C would be original engineering work for no portfolio gain over the
  Taichi version.
- **Stack B (WebGPU):** technically feasible but adds the cross-stack-port cost
  of designing a Stack-B-equivalent of the common-py infrastructure simultaneously
  with shipping MPM. The interactive web-version is a v1.1 stretch (`lenia-fft`
  is the natural Stack B Python+WebGPU candidate, not MPM).

Stack D is also the natural first Stack D consumer for two reasons: (a) MLS-MPM is
Taichi's most-validated kernel pattern (canonical 322-line upstream example exists);
(b) it exercises the entire common-py surface (Camera, StateWriter/Reader, ParamPanel,
VdbWriter, AlembicWriter stub) in a single sim, validating the common-py shape against
a real consumer rather than against a hello-world.

---

## 4. Data structures and memory layout

**Particle SoA** (Vector / Matrix fields, AOS-of-vectors layout — Taichi default):

| Field | Type | Shape | Bytes per particle |
|---|---|---|---|
| `x` | `Vector.field(3, f32)` | (N,) | 12 |
| `v` | `Vector.field(3, f32)` | (N,) | 12 |
| `C` | `Matrix.field(3, 3, f32)` | (N,) | 36 |
| `F` | `Matrix.field(3, 3, f32)` | (N,) | 36 |
| `Jp` | `field(f32)` | (N,) | 4 |
| `materials` | `field(i32)` | (N,) | 4 |
| `colors` | `Vector.field(4, f32)` | (N,) | 16 |
| `used` | `field(i32)` | (N,) | 4 |

Per-particle total: 124 bytes. At 1M particles: 124 MB.

**Grid SoA** at resolution G³:

| Field | Type | Shape | Bytes per cell |
|---|---|---|---|
| `grid_v` | `Vector.field(3, f32)` | (G, G, G) | 12 |
| `grid_m` | `field(f32)` | (G, G, G) | 4 |

At G=192: 192³ × 16 ≈ 113 MB.

**Total VRAM by tier:**

| Tier | Particles | Grid | Particle bytes | Grid bytes | Total ≈ |
|---|---|---|---|---|---|
| Default | 250 000 | 96³ | 31 MB | 14 MB | **45 MB** |
| Mid | 500 000 | 128³ | 62 MB | 34 MB | **96 MB** |
| Stretch | 1 000 000 | 192³ | 124 MB | 113 MB | **237 MB** |

All tiers fit comfortably in the 16 GB AMD RX 6800 XT VRAM and 11 GB RTX 2080 Ti
VRAM. The dense grid representation is intentionally simple; a sparse grid is banked
v1.1 polish (would shrink the 192³ tier by ~5–10×).

---

## 5. Per-frame compute pipeline

One @ti.kernel `substep` per simulation substep; 25 substeps per visual frame at
60 fps (dt = 2 × 10⁻⁴, frame period ≈ 16.7 ms).

```
Phase 1: clear grid                       O(G³)
Phase 2: P2G (per-material plasticity)    O(N)
Phase 3: grid update (gravity, boundary)  O(G³)
Phase 4: G2P (advect particles)           O(N)
```

The four phases are fused into a single @ti.kernel for backend perf — splits
reduce clarity and (on CUDA) introduce extra grid-sync overhead between launches.

**Sync points:** Each substep's four phases run in serial Taichi for-loops; Taichi
auto-inserts grid syncs between them on GPU backends. No host-side sync per substep.
A `ti.sync()` is only needed before tier-change reallocation and at frame-end if
VDB/PLY export reads back grid_m / x to numpy.

**Read-write hazards:** P2G writes (scatter-add) to grid_v / grid_m; uses element-wise
scalar atomic adds (the `for d in ti.static(range(3))` shape). On Vulkan, this requires
no extension. On CUDA, native float atomics. No vector-atomic-float usage.

**CUDA hints:** `ti.loop_config(block_dim=n_grid)` at the start of P2G and G2P;
no-op on Vulkan.

---

## 6. Interactive rendering approach

Taichi GGUI (`ti.ui.Window` + `ti.ui.Scene` + `Canvas`) for the interactive
window. Per-frame draw:

1. Set camera from `Camera.ti_camera()` (wraps ti.ui.Camera with our conceptual surface).
2. Two point lights at fixed world positions for definition.
3. `scene.particles(state.x, per_vertex_color=state.colors, radius=0.005)` — one
   draw call for all particles.
4. Canvas.scene(scene).

Param panel via `gpusims_common.ParamPanel` (wraps `gui.sub_window`):

- **Presets** sub-window: 4 named presets as checkboxes.
- **Tier** sub-window: 3 tier checkboxes (250k / 500k / 1M).
- **Gravity** sub-window: 3 sliders (X/Y/Z, range [-20, 20] m/s²).
- **Materials** sub-window: 3 RGB color pickers + "place material" text + LMB-place
  status.
- **Export** sub-window: VDB checkbox + PLY checkbox + frame counter + reset button.

LMB-place UX: cursor-to-ground-plane unproject via `inverse(view_projection) @ NDC`,
ray-intersect at y=0.02. Cap 8 user emitters total; cycled material via M key.

Camera: free-fly (WASDQE + RMB-look) via `Camera.track_user_inputs(window)` which
delegates to `ti.ui.Camera.track_user_inputs`. F5 / F9 save / load full sim state
via `gpusims_common.StateWriter` / `StateReader`.

---

## 7. Offline export path

**Per-frame VDB density** via `gpusims_common.vdb_writer.write_float_frame`:
exports the post-P2G `grid_m` field as a single `"density"` channel. Real-or-stub
gated on `import pyopenvdb`; if absent, logs a one-time warning and is a no-op.

Multi-channel per-material VDB (water/jelly/snow as separate density grids) is
banked v1.1; v1 ships single-channel because the PLY particle export already
carries per-material discrimination via the per-vertex `material` int channel.

**Per-frame binary PLY particles** via `ti.tools.PLYWriter`: positions + per-vertex
`material` (int) channel. One PLY per export frame; filtered to used particles for
clean Blender import.

**Hero render** via `render-pipelines/blender/render_mpm.py`:

- Headless Blender Cycles with OptiX → HIP → CUDA → fail-loud GPU device chain.
- Three Cycles materials constructed via bpy API (no `.blend` dep): Water (Principled
  BSDF, transmission 1.0, IOR 1.33), Jelly (SSS), Snow (rough diffuse + slight emission).
- Geometry Nodes per-vertex `material` named-attribute drives a switch-tree across
  three Set Material nodes that assign the correct material to each instance.
- v1 deliverable: single still at `--frame 60`. v1.1: animation via `--frame-start` /
  `--frame-end` once A100 access is available.

**Alembic** export is NOT in scope. See `docs/load-bearing-decisions.md` for the
deferral rationale; the natural sph-water phase becomes Alembic consumer #1.

---

## 8. Scale tiers

| Tier | Particles | Grid | Target FPS | Interactive? |
|---|---|---|---|---|
| **Default** | 250 000 | 96³ | 60 fps on RX 6800 XT + Taichi Vulkan; 60 fps on RTX 2080 Ti + Taichi CUDA | yes |
| **Mid** | 500 000 | 128³ | 30–60 fps; expect dips with mixed-material scenes | yes |
| **Stretch** | 1 000 000 | 192³ | 5–15 fps | **no** — capture-mode-only |

The 1M tier shows a confirmation modal on selection ("This tier is for offline-render
frame capture only. The simulation will run at 5–15 fps and the UI may stutter.
Continue?") matching the eulerian-smoke 384³ stretch tier idiom.

Performance numbers will be added per-tier on dev hardware (AMD RX 6800 XT desktop
+ Vulkan; RTX 2080 Ti lab PC + CUDA) once user-driven visual verification ships.

---

## 9. Stretch goals (v1.1+)

- **Sand material** (Drucker-Prager plasticity; Klar et al. 2016): material index 3
  with cone-projected stress. ~1-2 days of work plus visual-verification iteration.
- **Multi-channel VDB per-material** density export: 3× disk usage. ~1 day, only
  worth shipping if hero renders demand it.
- **Sparse grid** (Taichi 1.7 `ti.root.bitmasked`): 5–10× memory shrinkage on the
  192³ tier at the cost of indirection overhead. ~2-3 days plus benchmarking.
- **A100 hero animation pass**: 120 frames @ 24 fps at 1M / 192³ tier, ~10 hours of
  Cycles render. Requires A100 access.
- **`watchfiles`-based process auto-restart wrapper**: small `gpusims_common.process_watcher`
  module skirting Taichi's no-in-process-reload constraint.
- **localStorage-equivalent slider persistence**: write to `~/.config/gpusims/<sim>.json`.
- **Vulkan `VK_EXT_shader_atomic_float` vector-atomic-add path**: ~2-3× faster grid
  scatter on capable drivers.

---

## 10. Engineering risks

- **Tier-change kernel recompile latency.** Taichi specializes kernels on field
  shapes; tier change triggers a 1–3 s recompile on first substep. UI mitigation:
  "Recompiling…" overlay (v1.1 polish item).
- **Vulkan vs. CUDA backend divergence.** Both backends are first-class via
  `ti.init(arch=ti.gpu)`. Risk: a kernel pattern that works on CUDA but not Vulkan
  (e.g., vector-atomic-add) silently degrades. Mitigation: dev-test on both
  backends during visual verification.
- **`@ti.kernel` AST inspection requires file-on-disk.** Tests passing kernel code
  as `python -c "…"` strings fail. Mitigation: tests live in `tests/test_kernels.py`
  files on disk; banked in `project-state.md` § 7.
- **`pyopenvdb` not pip-installable.** Install path is `sudo apt install python3-openvdb`
  on Ubuntu; conda-forge alternative. Mitigation: `vdb_writer.py` falls through to
  stub-mode gracefully if import fails; documented in common-py README.
- **`pyalembic` not pip-installable + same-name PyPI collision** (SQLAlchemy).
  Mitigation: Alembic permanent-stub; export to PLY instead in Phase 9.

---

## 11. References

- **Hu et al. 2018.** "A Moving Least Squares Material Point Method with
  Displacement Discontinuity and Two-Way Rigid Body Coupling." SIGGRAPH 2018.
  The MLS-MPM paper. Canonical reference.
- **Stomakhin et al. 2013.** "A Material Point Method for Snow Simulation."
  SIGGRAPH 2013. Snow plastic-clamp reference.
- **Klar et al. 2016.** "Drucker-Prager Elastoplasticity for Sand Animation."
  SIGGRAPH 2016. Sand v1.1 reference.
- **`taichi-dev/taichi` upstream.** `python/taichi/examples/ggui_examples/mpm3d_ggui.py`.
  MIT license. Phase 9 kernels.py is adapted from this 322-line example with the
  per-material plasticity branching preserved 1:1; structural change is parameterization
  of fields via `ti.template()` arguments to support runtime tier change. License
  attribution: see `docs/conventions.md` § "Reference-implementation licensing"
  and the comment block at the top of `kernels.py`.
- **Phase 9 spec & cross-review** (`phase9_common_py_mpm.md`): the full architect-1
  draft + architect-2 cross-review notes that produced this implementation.
