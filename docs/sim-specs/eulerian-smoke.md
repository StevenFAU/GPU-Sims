# Eulerian Smoke — Specification

> **Status:** Implemented (Phase 8)
> **Category:** Volumetric grid
> **Primary stack:** C (Native C++)
> **Secondary stack(s):** —
> **Target machine:** Desktop interactive, A100 hero
> **Folder:** [`volumetric-grid/eulerian-smoke`](../../volumetric-grid/eulerian-smoke/)

---

## 1. Goal

Real-time interactive Eulerian smoke simulation on a 3D grid, with a portfolio-load-bearing offline hero render via Blender Cycles. The interactive demo must:

- Run at 60 fps at 256³ on the dev hardware (RX 6800 XT), 1920×1080 windowed.
- Produce visually recognizable "smoke from a fire" output — buoyant rising plumes, fine-scale turbulent detail preserved through advection, temperature-driven emission for the canonical glowing hot base.
- Support live user interaction: place / remove emitters via mouse, tune solver and render parameters via ImGui sliders, save and load state via F5 / F9.
- Demonstrate solver flexibility via six built-in presets spanning the visual dynamics range.

The hero-render path consumes per-frame OpenVDB density exports and produces a single still in Blender Cycles via `render-pipelines/blender/render_smoke.py`. The script supports animation mode for v1.1 once compute capacity is available.

## 2. Mathematics

The sim solves the inviscid incompressible Euler equations with a temperature scalar (drives buoyancy) and a passive density scalar (the smoke itself, advected by velocity):

```
∂v/∂t  +  (v · ∇)v  =  -∇p / ρ_0  +  f_buoy  +  f_vc           (momentum)
∇ · v  =  0                                                       (incompressibility)
∂T/∂t  +  (v · ∇)T  =  -κ_T * T                                 (temperature)
∂ρ/∂t  +  (v · ∇)ρ  =  -κ_ρ * ρ                                 (smoke density)
```

Where:
- `v` is velocity, `p` pressure, `T` temperature, `ρ` smoke density (not reference density `ρ_0`).
- `f_buoy = (α·T - β·ρ) * ĵ` is Boussinesq buoyancy (Fedkiw 2001 eq 2).
- `f_vc = ε·h·(N×ω)` is vorticity confinement (Fedkiw 2001 eq 14); `N = ∇|ω|/max(|∇|ω||, 1e-5)`.

Discretization per substep follows the full Fedkiw-2001 stack with MacCormack advection + reverse-Stam clamping, vorticity confinement, and Jacobi-iteration pressure projection. See `phase8_eulerian_smoke.md` § 2.3 for the full per-step discretization.

## 3. Solver-stack rationale

**Why Stam-tradition semi-Lagrangian advection?** Unconditional stability allows large timesteps without CFL violations. The price (over-dissipation) is offset by the MacCormack correction.

**Why MacCormack with reverse-Stam clamping?** MacCormack restores detail lost to pure semi-Lagrangian advection. The reverse-Stam limiter prevents the corrected output from overshooting the local field extrema (which would produce negative density values and broken visuals).

**Why vorticity confinement?** Counteracts the numerical dissipation that semi-Lagrangian advection introduces in small-scale rotational motion, restoring the "turbulent" look that makes smoke recognizable as smoke rather than fog.

**Why Jacobi pressure projection?** GPU-friendly parallelism (unlike Gauss-Seidel which requires sequential sweep). Jacobi is more iterations but the per-iteration cost is dominated by parallelism rather than arithmetic. MGPCG (multigrid preconditioned CG) is the v1.1 upgrade.

**Why temperature as a separately-advected field rather than render-only?** Temperature couples to velocity via the buoyancy term — hot fluid rises, cool fluid falls. Without advecting temperature, the buoyancy force is decoupled from the smoke's history.

**Why density-driven downward force `β·ρ`?** "Smoke has mass" — the canonical Stam-tradition trick that makes denser smoke regions fall slightly relative to less-dense regions, producing the layered-plume look.

## 4. Data structures

Ten 3D images:

| Field | Format | Ping-pong? | Purpose |
|-------|--------|-----------|---------|
| `velocity_ping/pong` | rgba16f | yes | Velocity field (vec3 in xyz, w=0 padding) |
| `density_ping/pong`  | r32f    | yes | Smoke density |
| `temperature_ping/pong` | r32f | yes | Temperature |
| `pressure_ping/pong` | r32f    | yes | Pressure (ping-pong within Jacobi loop) |
| `curl`               | rgba16f | no  | Curl scratch field (write-then-immediately-consume) |
| `divergence`         | r32f    | no  | Divergence scratch field |

Plus emitter array (cap 8, CPU-managed, uploaded each frame).

## 5. Per-frame pipeline

Per substep, 11 compute dispatches in this order:

1. Advect velocity (MacCormack)
2. Advect density (MacCormack)
3. Advect temperature (MacCormack)
4. Apply buoyancy (in-place on velocity)
5. Compute curl
6. Apply vorticity confinement (in-place on velocity)
7. Emit sources (in-place on velocity, density, temperature)
8. Apply boundaries (in-place on velocity)
9. Compute divergence
10. Jacobi pressure solve (× `pressureIters`)
11. Project velocity to divergence-free (in-place on velocity)

Followed by the volume raymarch fragment shader producing the final framebuffer image.

See `phase8_eulerian_smoke.md` § 2.6 for the full dispatch-chain table, in-place safety analysis, and memory-barrier placement.

## 6. Interactive rendering

The live render is a volume raymarch fragment shader with:

- Beer-Lambert absorption from the density field
- Single-scattering single-shadow-march per primary-ray sample toward the key light
- Temperature-driven emission via a 256×4 RGBA8 LUT (4 ramps: blackbody/sunset/cold/mono)
- Blue-noise jitter on primary-ray start to break slice-banding
- Inline Reinhard tonemap

Default budget: 96 primary samples × (1 + 16 shadow samples) = ~1632 texture samples/pixel. Comfortable at 1920×1080 on the dev hardware.

## 7. Offline export path

When OpenVDB is enabled at build time (`-DGPU_SIMS_USE_OPENVDB=ON`), the sim can write per-frame density VDB files (`vdb_export/density_NNNN.vdb`) and one-shot temperature VDB files (`vdb_export/temperature_NNNN.vdb`) consumed by the Blender script.

The hero render is produced by `render-pipelines/blender/render_smoke.py`, which runs headless via `blender -b -P render_smoke.py -- <args>` and selects GPU (OptiX → HIP → CUDA → fail loud).

## 8. Scale tiers

| Tier | Grid | VRAM | Target framerate |
|------|------|------|-------------------|
| Compact | 192³ | ~520 MB | 60 fps |
| Default | 256³ | ~1.0 GB | 60 fps |
| Stretch | 384³ | ~3.1 GB | 20–30 fps |

The 384³ tier is the "look how big" stretch with the degradation-warning pattern from boids-3d's 100k tier — accessible via the tier dropdown with a panel-warning when selected.

## 9. Stretch goals (v1.1+ scope)

See `volumetric-grid/eulerian-smoke/docs/notes.md` for the full prioritized backlog. Highlights:

- Moving obstacles (priority 1.0; the v1 omission per Phase 8's coordinator-banked scope-trim).
- MGPCG pressure solver (better convergence; matters for water more than smoke).
- Animation hero render once A100 access is available.
- Free-slip wall boundaries (visually invisible for free plumes; matters with obstacles).
- HDR bloom + ACES tonemap pass.

## 10. Engineering risks

1. **First real OpenVDB exercise.** Phase 8 is consumer #1 of `gpusims::vdb::writeFloatFrame`; any latent bugs in `vdb_writer.cpp`'s `writeFloatGrid` or `frameSequencePath` paths surface here. Hard-rule-8 in `phase8_eulerian_smoke.md § 0` says "pause and report" if anything surfaces.
2. **MacCormack clamping correctness.** Architect-2 callout #1; the reverse-Stam limiter must clamp to the corner-extrema of the *original* field at the *backward-traced* position (NOT the forward-traced position).
3. **Vorticity-confinement NaN propagation.** Architect-2 callout #2; the zero-guard `max(|∇|ω||, 1e-5)` is non-negotiable.
4. **Tier-change use-after-free.** Architect-2 callout #7; all descriptor sets must be re-written after image recreation.
5. **VDB readback frame-budget impact.** Architect-2 callout #6; ~50–150 ms/frame at 256³. Documented as artist-mode batch operation in the panel toggle label.

## 11. References

- Stam, J. (1999). "Stable Fluids." SIGGRAPH 1999.
- Fedkiw, R., Stam, J., Jensen, H.W. (2001). "Visual Simulation of Smoke." SIGGRAPH 2001.
- Selle, A., Fedkiw, R., Kim, B., Liu, Y., Rossignac, J. (2008). "An Unconditionally Stable MacCormack Method." J. Sci. Comput.
- Bridson, R. (2008). "Fluid Simulation for Computer Graphics." 1st ed. — the standard graphics-fluids reference.
- McAdams, A., Sifakis, E., Teran, J. (2010). "A parallel multigrid Poisson solver for fluids simulation on large grids." SCA 2010.
