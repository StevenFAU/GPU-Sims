# Phase 11.5 boundary placement probe — upstream-anchored diagnostic

**Date:** 2026-05-15
**Author:** sph-water operator (Phase 11.5)
**Status:** Read-only diagnostic. No commits, no code edits.

The Akinci2012 boundary handling landed across commits 3 / 4 / 5 has been wrong in three different ways. Before guessing again, gather authoritative data from the upstream tree (`references/SPlisHSPlasH/` at registered anchor SHA `6bff55a6`).

---

## Section A — Upstream boundary sample placement

### A.1 — `BoundaryModel_Akinci2012::initModel` does no placement

`references/SPlisHSPlasH/SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:77-110`:

```cpp
void BoundaryModel_Akinci2012::initModel(RigidBodyObject *rbo, const unsigned int numBoundaryParticles, Vector3r *boundaryParticles)
{
    m_x0.resize(numBoundaryParticles);
    m_x.resize(numBoundaryParticles);
    m_v.resize(numBoundaryParticles);
    m_V.resize(numBoundaryParticles);
    …
    #pragma omp parallel default(shared)
    {
        #pragma omp for schedule(static)
        for (int i = 0; i < (int) numBoundaryParticles; i++)
        {
            m_x0[i] = boundaryParticles[i];
            m_x[i] = boundaryParticles[i];
            m_v[i].setZero();
            m_V[i] = 0.0;
        }
    }
    m_rigidBody = rbo;
    …
}
```

`initModel` is just storage. It receives a `Vector3r *boundaryParticles` array, copies the positions verbatim into `m_x0` and `m_x`, and registers the point set with the neighborhood search. **No offset, no inset, no transform.** The caller decides where each boundary particle goes.

### A.2 — Where the caller actually generates positions

`references/SPlisHSPlasH/Simulator/StaticBoundarySimulator.cpp:29-156` calls one of three samplers depending on `scene.boundaryModels[i]->samplingMode`. The samplers take a triangle mesh (the rigid body's `geo.getVertices() / geo.getFaces()`) and produce boundary particles on its surface:

```cpp
PoissonDiskSampling sampling;
sampling.sampleMesh(geo.numVertices(), geo.getVertices().data(),
                    geo.numFaces(),    geo.getFaces().data(),
                    scene.particleRadius, 10, 1, boundaryParticles);
…
RegularTriangleSampling sampling;
sampling.sampleMesh(geo.numVertices(), geo.getVertices().data(),
                    geo.numFaces(),    geo.getFaces().data(),
                    1.5f * scene.particleRadius, boundaryParticles);
```

Both samplers operate on the mesh's literal vertex/face coordinates with no thickness, no inset, no offset. After sampling, the boundary positions are then transformed by the rigid body's `translation` and `rotation` (lines 146-156) before being handed to `BoundaryModel_Akinci2012::initModel`.

The simplest of the two — `RegularTriangleSampling::sampleMesh` at `references/SPlisHSPlasH/SPlisHSPlasH/Utilities/RegularTriangleSampling.cpp:12-19`:

```cpp
void RegularTriangleSampling::sampleMesh(const unsigned int numVertices, const Vector3r * vertices, const unsigned int numFaces, const unsigned int * faces, const Real maxDistance, std::vector<Vector3r>& samples)
{
    const std::vector<Vector2ui> edges = uniqueEdges(numFaces, faces);

    appendVertexSamples(numVertices, vertices, samples);
    appendEdgeSamples(maxDistance, vertices, edges, samples);
    appendFaceSamples(maxDistance, vertices, numFaces, faces, samples);
}
```

`appendVertexSamples` (line 21-25) drops one sample at each mesh vertex literally:

```cpp
samples.insert(samples.end(), vertices, vertices + numVertices);
```

`appendEdgeSamples` (line 27-47) linearly interpolates between two vertices `v0 + α * (v1 - v0)` — exactly on the edge.

`appendFaceSamples` (line 49-95) parameterizes triangle interiors and emits samples at `lineStart + (i * lineLength) / (numSamples - 1) * bn`. Again exactly on the face plane.

**No code path in upstream offsets samples by `particleRadius`, `0.5 * particleRadius`, or any other inset.** Samples land on the mesh surface.

### A.3 — Example scene: `DamBreakModel.json`

`references/SPlisHSPlasH/data/Scenes/DamBreakModel.json`:

```json
{
  "Configuration": {
    "particleRadius": 0.025,
    …
    "boundaryHandlingMethod": 2,   // Akinci2012
    …
  },
  …
  "RigidBodies": [
    {
      "geometryFile": "../models/UnitBox.obj",
      "translation": [0, 1.5, 0],
      "rotationAxis": [1, 0, 0],
      "rotationAngle": 0,
      "scale": [4, 3, 1.5],
      …
      "isDynamic": false,
      "isWall": true,
      "mapInvert": true,
      …
    }
  ],
  "FluidBlocks": [
    {
      "denseMode": 0,
      "start": [-0.5, 0.0, -0.5],
      "end":   [0.5, 1.0, 0.5],
      "translation": [-1.45, 0.05, 0.0],
      "scale": [1, 1, 1]
    }
  ]
}
```

`UnitBox.obj` is a unit cube with vertices at ±0.5 on each axis (`references/SPlisHSPlasH/data/models/UnitBox.obj`):

```
v -0.500000 -0.500000  0.500000
v  0.500000 -0.500000  0.500000
v -0.500000  0.500000  0.500000
v  0.500000  0.500000  0.500000
v -0.500000  0.500000 -0.500000
v  0.500000  0.500000 -0.500000
v -0.500000 -0.500000 -0.500000
v  0.500000 -0.500000 -0.500000
```

After `scale: [4, 3, 1.5]` and `translation: [0, 1.5, 0]`, the box spans:
- x ∈ [-2.0, +2.0]
- y ∈ [0.0, +3.0]
- z ∈ [-0.75, +0.75]

The fluid block after `translation: [-1.45, 0.05, 0.0]`:
- x ∈ [-1.95, -0.95]
- y ∈ [+0.05, +1.05]
- z ∈ [-0.5, +0.5]

**The fluid initial position is offset from the container surfaces by:**
- 0.05 above the floor (= **2 × particleRadius** at r=0.025)
- 0.05 from the −x wall (= 2 × r)
- 0.25 from each z wall (= 10 × r)

`mapInvert: true` flips the box's normals so the surface becomes a *container* (fluid sees the inside). The boundary samples are produced from the (un-inverted) mesh triangles, which sit at the box's surface in object space and then get translated/scaled to world space — no inset is applied at any stage.

### A.4 — Direct answers to the placement questions

| Question | Upstream answer |
|---|---|
| Are particles on the box's inner surface? | They're on the **mesh surface** (the triangle plane itself). With `mapInvert: true`, the surface acts as the inner-facing wall, but the sample positions are at the surface, not on either side of it. |
| Are they on the outer surface? | Same answer — *on* the surface, no thickness. |
| Are they inset by any amount from the surface? | **No.** Neither the `RegularTriangleSampling` nor `PoissonDiskSampling` code paths apply any inset. |
| Margin/offset relative to `particleRadius`? | The boundary samples have **no offset**. What does scale with `particleRadius` is the **fluid initial position**: in `DamBreakModel.json` the fluid sits 2 × particleRadius from the floor and the closest side wall. |

**The upstream invariant is: boundary samples on the surface, fluid initial position ≥ 2 × particleRadius from any boundary surface.** That keeps fluid at q ≥ 0.5 from the nearest boundary sample at frame 0; equilibrium settling brings fluid to q ≈ 0.25 against the floor, where W is ~3.4× smaller than `W(0)` and the response is finite.

---

## Section B — 20-second runtime crash at HEAD (commit 5)

### B.1 — Long run

```
timeout 65 ./build-debug/bin/sph_water
EXIT=124   # SIGTERM from timeout, not a crash
```

Tail of `/tmp/sph_long.log`:

```
[12:17:51.995] [info] sph-water - Phase 11 (DFSPH + Morton-sort + screen-space fluid + Alembic first-exercise)
[12:17:52.019] [info] vk-context: subgroup-size-control enabled (min=32, max=64, stages=0xe0)
[12:17:52.019] [info] vk-context: ready (AMD Radeon RX 6800 XT (RADV NAVI21), 15966 MiB VRAM heap0)
[12:17:52.027] [info] vk-window: present mode = 1
[12:17:52.028] [info] [sph-water] Subgroup size: min=32, max=64, stages=0xe0
[12:17:52.031] [info] Alembic writer ready: /home/otacon/Projects/GPU-Sims/GPU-Sims/alembic_export/sph_water.abc
[12:17:52.211] [info] [sph-water] Akinci2012: 131458 boundary particles, ~2.51 MB
```

7 startup log lines, then nothing — no crash, no warning, no NaN-report path triggered, no Vulkan validation chatter. The 20-second crash reported during the commit-4 visual smoke does **not** reproduce at commit 5 in a non-interactive run.

### B.2 — gdb backtrace

Not run; the binary stayed up for 65 s and exited only to `timeout`'s SIGTERM. No fault to capture.

### B.3 — NaN instrumentation

Not run. The 20 s crash is no longer reproducing as of commit 5 under the test conditions here (no interactive input, headless-style operation through ImGui without user clicks). If the user reproduces it in a visual smoke against commit 5, the next step is to attach gdb live or add a `particles[gid].pos.x != particles[gid].pos.x` (NaN check) instrumentation pass.

Provisional conclusion: the 20-s crash was likely tied to the commit-4 wall-plastering geometry (e.g., a numerical NaN emerging from over-pressured fluid eventually corrupting buffer indexing) and may resolve once a correct boundary placement lands. Re-test after a correct commit.

---

## Section C — Current `generateBoundaryParticles` (commit 5 HEAD)

`particle-fluids/sph-water/src/main.cpp:1306-1414`:

- **Inset direction:** samples sit one `particle_radius` *outside* the AABB. Implementation: `imin = dmin - r; imax = dmax + r;` propagates through both the face-plane coordinates AND the in-plane axis bounds.
- **Dedup approach:** trim each face's hex grid by one `spacing` on every rim (per-face interior loop). 12 separate edge passes (one line of samples each, ends trimmed by one spacing). 8 explicit corner samples.
- **Count for default Dam-Break (1M tier):** 131,458 boundary particles, ~2.51 MB live.

The math (boundary V calculation, fluid-shader Akinci branches) was independently audited at the boundary-placement diagnostic step and matched upstream. The defect path is the geometry of `generateBoundaryParticles`, not the math.

---

## Section D — Commit-3 substantive (no inset)

`git show f9f2cb9:particle-fluids/sph-water/src/main.cpp`, function body:

```cpp
auto sample_face = [&](int normal_axis, float plane_coord,
                       int axis_u, int axis_v) {
    float u_min = dmin[axis_u];
    float u_max = dmax[axis_u];
    float v_min = dmin[axis_v];
    float v_max = dmax[axis_v];
    …
};

// Floor + ceiling (Y faces).
sample_face(1, dmin.y, 0, 2);
sample_face(1, dmax.y, 0, 2);
…
sample_face(0, dmin.x, 1, 2);
sample_face(0, dmax.x, 1, 2);
…
sample_face(2, dmin.z, 0, 1);
sample_face(2, dmax.z, 0, 1);
```

Commit 3 placed samples **at the AABB plane itself** — `plane_coord = dmin.y` for the floor, etc. No inset in either direction. This matches the upstream convention (samples on the mesh surface). What commit 3 did *not* match was the upstream **fluid-initial-position** invariant.

---

## Section E — Hypothesis-vs-data verdict

Recap of the three working hypotheses:

| H | Where boundary samples sit | Commit | Defect observed |
|---|---|---|---|
| H1 | At AABB plane (no inset) | 3 | Crashed on upload (separate defect); geometry never tested visually |
| H2 | One `particle_radius` *into* AABB (inside fluid domain) | 4 | Fluid plastered against walls / floor / ceiling |
| H3 | One `particle_radius` *outside* AABB (in wall material) | 5 | Banded standing column (weak boundary response) |

**Upstream convention from Section A is H1 (no inset, samples on the surface).** Neither H2 nor H3 matches. The commit-3 placement was actually correct upstream-style; the failure mode the diagnostic anticipated (`q→0` catastrophe when fluid touches the AABB plane) was a real concern only because our **fluid initial brick for Dam-Break is placed touching the AABB walls**, not respecting the 2 × particleRadius clearance upstream's `DamBreakModel.json` uses.

**Sub-finding (preset defect):** the Dam-Break preset at `particle-fluids/sph-water/src/main.cpp:188`:

```cpp
"Dam-Break",
glm::vec3(+0.5f, -1.0f, -1.0f), glm::vec3(+2.0f,  0.5f, +1.0f),  // initial brick
…
glm::vec3(-2.0f, -1.0f, -1.0f), glm::vec3(+2.0f, +2.0f, +1.0f),  // domain
```

The initial brick min is `(+0.5, -1.0, -1.0)` and max is `(+2.0, +0.5, +1.0)`. The domain min is `(-2.0, -1.0, -1.0)` and max is `(+2.0, +2.0, +1.0)`.

The fluid brick:
- Touches the AABB **floor**: brick y_min = -1.0 = domain y_min.
- Touches the AABB **right wall**: brick x_max = +2.0 = domain x_max.
- Touches **both z walls**: brick z spans the full domain z ∈ [-1, +1].

With particleRadius = 0.01, the brick should at minimum be inset by 0.02 from every wall (matching upstream's 2 × particleRadius clearance). Currently it has zero clearance on four of six faces. With samples sitting on the AABB plane (commit 3 / H1), the very first frame computes `q = 0` between a fluid particle and a boundary sample → kernel peak → ~398 kg/m³ density boost per neighbor → explosive over-pressure.

This is why commit 3's geometry "couldn't work" without a clearance fix in the preset, and why architect-1's commits 4 and 5 reached for inset adjustments. But inset is the wrong axis to fix. **The right fix is two-pronged:**

1. **Restore the upstream H1 geometry** — boundary samples on the AABB plane, no inset. Re-use the commit-3 substantive `generateBoundaryParticles` (perhaps with the seam dedup from commit 4 retained, since that defect is genuine: AABB seams *did* create double/triple-coincident samples).

2. **Fix the Dam-Break preset** to give the initial brick at least 2 × particleRadius clearance from every domain face — e.g., shift `initial_brick_min` to `(+0.52, -0.98, -0.98)` and `initial_brick_max` to `(+1.98, +0.5, +0.98)` (or whatever matches the visual intent while preserving the clearance invariant). Same audit pass on Central-Fountain, Droplet-Impact, and Pour-from-Source.

3. (Optional, lower priority) Re-examine the AABB fail-safe in `integrate_forces.comp.glsl`. If fluid is initialized with proper clearance, the boundary repulsion should keep particles inside without geometric clamping. The current fail-safe trigger margin `0.5 × particleRadius` outside the AABB may be tightenable to "any particle outside the AABB → reflect and zero velocity".

---

## Report path

`docs/diagnostics/_audits/phase11_5_boundary_placement_probe_2026-05-15.md`

(Not committed. Working tree unchanged. Sims code untouched.)
