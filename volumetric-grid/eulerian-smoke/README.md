# Eulerian Smoke

**Status:** Implemented (Phase 8)
**Stack:** Stack C (Native C++ / Vulkan 1.3)
**Spec:** [`docs/sim-specs/eulerian-smoke.md`](../../docs/sim-specs/eulerian-smoke.md)

The first Tier-2 flagship sim in the GPU-Sims portfolio. Real-time Eulerian smoke simulation on a 3D grid via the full Fedkiw-2001 stack: MacCormack-corrected semi-Lagrangian advection, vorticity confinement, Jacobi pressure projection, buoyancy from temperature. Rendered via volume raymarch with Beer-Lambert absorption + single-scattering single-shadow-march and temperature-driven black-body emission. Six smoke-dynamics presets, sparse user-placed emitters, hot-reload, F5/F9 capture/load, optional per-frame OpenVDB export feeding a Blender Cycles hero-render pipeline.

## What you're seeing

A 256³ grid of velocity, density, temperature, and pressure fields. Each frame:

1. **Advect** every field along its own velocity (semi-Lagrangian + MacCormack correction).
2. **Apply buoyancy** — hot/dense fluid rises, cool/empty fluid falls.
3. **Apply vorticity confinement** — restore small-scale rotational detail that advection smoothes out.
4. **Inject sources** at each user-placed emitter (density + temperature + small upward velocity bias).
5. **Apply boundaries** — zero velocity at the five no-slip walls; zero-gradient at the open ceiling.
6. **Project to divergence-free** via 40 Jacobi pressure iterations + gradient subtraction.
7. **Volume raymarch** with self-shadowing for the canonical "smoke from a fire" look.

Six built-in presets demonstrate different visual dynamics:

| Preset | What you see |
|--------|--------------|
| **Plume** (default) | Slow buoyant rise, clear plume — the default "smoke from a small fire." |
| **Candle** | Narrow column with gentle wavering — candle-smoke at rest. |
| **Cigar** | Broad slow rise with fine surface detail — cigar smoke in still air. |
| **Smokestack** | Structured streamline with strong vertical motion — industrial chimney. |
| **Explosion-Puff** | One-shot burst, then watch the puff evolve under its own dynamics. |
| **Chimney-Down** | Inverted: cold smoke falling, "dry-ice fog spilling off a table." |

## Controls

| Input | Action |
|-------|--------|
| Mouse-left click in viewport | Place an emitter at the cursor's projected position on the y=0 ground plane (cap 8). |
| Mouse-right click in viewport (within 8 cells of an existing emitter) | Remove that emitter. |
| WASD | Free-fly camera movement (when "Auto-orbit" is off). |
| Mouse-right drag | Free-fly camera look. |
| Mouse scroll wheel | Free-fly camera zoom / move-forward. |
| Q / E | Free-fly camera move down / up (world space). |
| Shift held | Boost movement speed during free-fly. |
| F5 | Save full state to `captures/capture_NNNN/`. |
| F9 | Load most recent capture from `captures/`. |
| Esc | Close window. |

The ImGui panel (top-left) groups controls into six folders: **Simulation** (preset, tier, solver tunables), **Rendering** (raymarch + tonemap tunables), **Lighting** (light direction, color, background gradient), **Emitter** (per-emitter parameters), **Camera** (orbit speed / radius, auto-orbit toggle), **State** (VDB export toggle, F5 / F9 buttons).

## Build

The interactive demo (no OpenVDB needed):

```bash
# From the repo root.
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bin/eulerian_smoke
```

## Optional: OpenVDB export for hero rendering

The hero-render pipeline (see `render-pipelines/blender/render_smoke.py`) consumes OpenVDB density grids exported by this sim. To enable VDB export:

1. Install OpenVDB:
   - **Ubuntu 24.04:** `sudo apt install libopenvdb-dev`
   - **macOS (Homebrew):** `brew install openvdb`
   - **From source:** see the [OpenVDB project](https://www.openvdb.org/) for build instructions.

2. Re-configure with the flag:
   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPU_SIMS_USE_OPENVDB=ON
   cmake --build build
   ```

3. Run the sim, enable **"Export VDB density per frame"** in the State panel folder, run for some frames, disable the toggle. Output VDBs land in `vdb_export/density_NNNN.vdb` relative to the working directory.

4. The "Export current temperature" button writes `vdb_export/temperature_NNNN.vdb` at the current frame counter (one-shot snapshot). `render-pipelines/blender/render_smoke.py` consumes density + temperature pairs for the emissive smoke effect.

If OpenVDB is not enabled (default), the panel toggles are still visible but do nothing (the writer logs a single warning at first attempt and returns false). Rebuild with `-DGPU_SIMS_USE_OPENVDB=ON` to enable.

## Producing a hero render

Once you have a VDB export directory:

```bash
# From the repo root, with Blender 4.x installed:
blender -b -P render-pipelines/blender/render_smoke.py -- \
    --density-input volumetric-grid/eulerian-smoke/vdb_export/density_0060.vdb \
    --temperature-input volumetric-grid/eulerian-smoke/vdb_export/temperature_0060.vdb \
    --output renders/smoke_hero_v01.png \
    --resolution 1920 1080 \
    --samples 512 \
    --camera-pos -2 1 -2 --camera-target 0 0.5 0
```

The script auto-detects GPU (OptiX → HIP → CUDA → fail loud if no GPU in headless mode). See `render-pipelines/blender/render_smoke.py --help` for full argument list and animation-mode (`--frame-start` / `--frame-end`) usage. v1 deliverable is a single still; animation is v1.1 once A100 access is available.

## Mathematics

The sim solves the inviscid incompressible Euler equations with a temperature scalar and a passive smoke-density scalar. See [`docs/sim-specs/eulerian-smoke.md`](../../docs/sim-specs/eulerian-smoke.md) § 2 for the full mathematical formulation. References: Stam 1999 (semi-Lagrangian advection); Fedkiw 2001 (visual smoke + vorticity confinement); Selle/Fedkiw 2008 (MacCormack stability); Bridson 2008 (Fluid Simulation for Computer Graphics — the standard reference text).

## Performance

| Tier | Grid | VRAM | Target framerate (RX 6800 XT) |
|------|------|------|-------------------------------|
| Compact | 192³ | ~520 MB | 60 fps |
| Default | 256³ | ~1.0 GB | 60 fps |
| Stretch | 384³ | ~3.1 GB | 20–30 fps |

VDB export adds ~50–150 ms per frame at 256³ on typical NVMe storage, dropping framerate to ~5–10 fps while enabled. VDB export is an artist-mode batch operation, not a "leave it on while exploring" mode — the panel toggle label says so.

## Capture and load

F5 saves full state (velocity + density + temperature + pressure + camera + parameters + emitter array) to `captures/capture_NNNN/`. F9 loads the most recent capture. Round-trip is bit-exact-within-one-substep for all four fields at any tier (rgba16f and r32f survive the readback + upload cycle without quantization). See spec § 2.9 for details on the capture schema.

## See also

- [`docs/load-bearing-decisions.md`](docs/load-bearing-decisions.md) — what's expensive to revisit vs. what's slider-tunable.
- [`docs/notes.md`](docs/notes.md) — v1.1 polish backlog (obstacles, free-slip walls, MGPCG pressure, animation hero renders, etc.).
- [`render-pipelines/blender/README.md`](../../render-pipelines/blender/README.md) — the offline-render trajectory.

## License

MIT (matches the repo's root LICENSE).
