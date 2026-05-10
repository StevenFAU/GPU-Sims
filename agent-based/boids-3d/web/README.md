# Boids 3D

Reynolds 1987 multi-species 3D flocking with leader attractors and dynamic predators, running entirely on the GPU via WebGPU. 50,000 boids and 500 predators by default; a 100k+1k hero tier is available in the dropdown for users with stronger hardware.

[Live demo](https://stevenfau.github.io/GPU-Sims/boids-3d/) (requires a WebGPU-enabled browser).

## What you're seeing

Boids are the small triangular shapes; predators are the larger red ones. Both species follow Reynolds' three rules — separation, alignment, cohesion — with a flee force when predators are nearby. Predators hunt boids using one of three runtime-switchable strategies (nearest prey, stochastic prey, or flock-center pursuit).

Place leader attractors (soft-white markers on the floor) by clicking. Boids within the leader's influence radius are pulled toward it. Combine multiple leaders to design routes the flock follows; predators will harass the flock along the route.

## Controls

| Action | Input | Notes |
|---|---|---|
| Move camera | WASD | Forward / left / back / right |
| Move up / down | Q / E | Vertical axis |
| Look around | RMB-drag | Free-fly orientation |
| Place leader | LMB-click | Drops a leader on the y=0 ground plane at the click point. Cap 32. |
| Remove leader | Shift+LMB-click | Removes the nearest leader within 4u of the click point. |
| Save snapshot | F5 | Downloads `capture_NNNN.zip` containing entity state + parameters. |
| Load snapshot | F9 | Opens a file picker for a previously-saved `.zip`. |
| Reseed | R | Re-randomize entities from the current init seed. |

**Why Shift+LMB instead of RMB for leader removal?** RMB is owned by the free-fly camera-look in this sim. Physarum (the other agent-system sim in this gallery) uses RMB for pin-removal because its 2D camera doesn't need RMB for look. Boids-3d is the first sim in the gallery with a free-fly 3D camera, so the input affordance shifted; Shift+LMB follows the convention used by 3D scene editors (Blender, Unity, Unreal).

## Six parameter presets

Each preset sets Reynolds-rule weights, leader-attraction strength, and predator parameters. Visualization (colors, scale) is independent and persists across preset changes.

| Preset | Character | Predator mode |
|---|---|---|
| **Cohesive Flock** | Default — balanced Reynolds, moderate leader pull, nearest-prey predators | Nearest prey |
| **Loose Murmuration** | Wide alignment, weak cohesion → thin streamers, flock-center predators | Flock center |
| **Tight Schooling** | Strong sep + align + cohesion → dense schools, fast predators | Nearest prey |
| **Predator Spread** | High flee strength, sustained chase arcs from stochastic predators | Stochastic prey |
| **Waypoint Tour** | Strong leader pull dominates the flock; stochastic predators harass the tour | Stochastic prey |
| **Chaos** | Conflicting weights, high speed, high flee — turbulent | Nearest prey |

## Scale tiers

The dropdown selects how many boids and predators run. Lower tiers are smoother on weaker hardware; the hero tier targets discrete GPUs.

| Tier | Boids | Predators | Hardware target |
|---|---|---|---|
| 25k | 25,000 | 250 | Integrated GPU floor (Intel Iris Xe, M-series MacBook Air) |
| **50k** (default) | 50,000 | 500 | Discrete GPU 60 fps target (RX 6800 XT, RTX 2080 Ti, M-series Pro/Max) |
| 75k | 75,000 | 750 | Mid stretch |
| 100k | 100,000 | 1,000 | Hero tier; 6800 XT comfortable at ~50 fps; sustained <30 fps logs a console warning |

If your hardware can't sustain 30 fps at the running tier for more than 1 second, the console emits a one-time warning suggesting a lower tier. The sim continues at whatever rate the hardware supports — no automatic tier change (would surprise mid-interaction).

## Capture and load

F5 downloads a `capture_NNNN.zip` with the full simulation state — entity buffer (positions + velocities + species), predator state buffer (target IDs and ages), all parameters, the camera pose, and the predator dropdown mode. F9 reloads it.

**Replay fidelity.** Loading a capture reproduces the captured frame **within one integration step** — one velocity-application worth of motion. The simulation state buffers are bit-exact restored; the visible frame after the load is one tick forward from the captured frame, which is well below the perceptibility threshold for any practical use. A captured snapshot followed by F9 reload then a second F5 capture produces nearly-identical entity bytes (differing by exactly one integration step) and nearly-identical JSON metadata (differing only in iteration count).

This is a stronger replay guarantee than seed-based replay (where reproducing a captured visual requires re-running the full simulation history): boids' literal-state capture works at any tier including 100k+1k where the entity buffer is just 3.2 MB.

## Build and run

```bash
npm install                                # from repo root
npm run dev --workspace=@gpusims/boids-3d-web  # opens http://127.0.0.1:5178
```

Or, from the `agent-based/boids-3d/web/` directory:

```bash
npm run dev
```

Production build:

```bash
npm run build --workspace=@gpusims/boids-3d-web
# Output: agent-based/boids-3d/web/dist/
```

Type-check:

```bash
npm run typecheck --workspace=@gpusims/boids-3d-web
```

## Browser requirements

- **Chromium / Chrome / Edge:** WebGPU is enabled by default on modern versions.
- **Firefox:** Set `dom.webgpu.enabled = true` in `about:config` (default-off as of May 2026).
- **Safari:** WebGPU is enabled by default on macOS 14.4+ / iOS 18+.

If WebGPU isn't available, the HUD reads "WebGPU not available in this browser" — there is no fallback path (per project-state.md § 4 decision #2, the repo doesn't maintain a WebGL2 fallback).

## See also

- The [physarum](../../physarum/) sim in the same gallery — the other agent-system Stack B sim, also using sparse user-placed sources to drive the simulation.
- The [strange-attractors](../../../closed-form/strange-attractors/) sim for the closed-form Stack B template.
