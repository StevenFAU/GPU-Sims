#version 460
#extension GL_GOOGLE_include_directive : require

// Apply boundary conditions to the post-stream f-state. Four regimes,
// applied in one dispatch. Run AFTER stream and BEFORE compute_moments.
//
//   1. Solid cells (interior obstacle): zero out all populations. They
//      don't participate in the dynamics; the moment-reconstruction guard
//      will read rho == 0 and set u = 0 there.
//   2. Halfway bounce-back at fluid cells whose +c_i neighbor is solid.
//      Reflects f_i back into f_OPPOSITE[i] at the same cell.
//      Reference: tools/integrity/docs/algebraic/d3q19.md § 5
//                 references/lbm-principles-practice/chapter5/poiseuille_BB.m:123
//                 (D2Q9 pattern; 3D D3Q19 generalization per d3q19.md § 2.2)
//      cat1.upstream-citation: chapter5/poiseuille_BB.m:123 (D2Q9 pattern)
//   3. -X face: velocity inlet. +X face: pressure outlet.
//      v1 implementation: EQUILIBRIUM boundary (f_i = feq(rho_0, u_inlet))
//      for the unknown populations at each face. First-order accurate;
//      self-consistent with init_equilibrium. The Phase 12 spec § 4.G
//      called for full Zou-He second-order; that derivation is not
//      anchored in d3q19.md and Claude Code's first-principles re-derivation
//      surfaced demonstrable spec-internal errors (D2Q9 vs D3Q19 face-weight
//      coefficient on f_1; partition swap of f_16<->f_17 in the f_7 closure).
//      Banked as v1.1 polish: full Zou-He upgrade with derivation-doc anchor.
//   4. +-Y, +-Z faces: free-slip (specular reflection). The wall-normal
//      component of velocity is reflected; tangential components pass.
//      Equivalent on populations: swap f_i with the population whose c_i
//      differs only in the sign of the wall-normal axis.

#include "lattice_constants.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 4) in;

layout(set = 0, binding = 0, r32f) uniform image3D    f_nonrest[18];
layout(set = 0, binding = 1, r32f) uniform image3D    f_rest;
layout(set = 0, binding = 2)       uniform usampler3D obstacle_mask;

layout(set = 0, binding = 3) uniform BoundariesUniforms {
    ivec4 dims;
    vec4  u_inf;        // (u_x, u_y, u_z, rho_0)
    ivec4 inlet_axis;   // reserved
} U;

float load_f(int i, ivec3 cell) {
    if (i == 0) return imageLoad(f_rest, cell).r;
    return imageLoad(f_nonrest[i - 1], cell).r;
}
void store_f(int i, ivec3 cell, float v) {
    if (i == 0) imageStore(f_rest, cell, vec4(v, 0.0, 0.0, 0.0));
    else        imageStore(f_nonrest[i - 1], cell, vec4(v, 0.0, 0.0, 0.0));
}
uint mask_at(ivec3 c) {
    if (any(lessThan(c, ivec3(0))) || any(greaterThanEqual(c, U.dims.xyz))) return 1u;
    return texelFetch(obstacle_mask, c, 0).r;
}

void main() {
    ivec3 cell = ivec3(gl_GlobalInvocationID.xyz);
    if (any(greaterThanEqual(cell, U.dims.xyz))) return;

    // -- (1) Solid cells: zero out f. They don't participate in dynamics.
    if (mask_at(cell) == 1u) {
        store_f(0, cell, 0.0);
        for (int i = 1; i < NUM_DIRS; ++i) store_f(i, cell, 0.0);
        return;
    }

    // -- (2) Halfway bounce-back at fluid cells adjacent to solid.
    // For each direction i with cell + c_i solid, the would-be-streamed-into-
    // solid population is reflected back into f_OPPOSITE[i] at this cell.
    // Note this is performed BEFORE the inlet/outlet/free-slip overwrites
    // below so the ±X-face / ±Y±Z-face logic operates on a halfway-BB-
    // corrected state at edge-of-obstacle cells (solid airfoils don't
    // touch the box walls in any preset, so corner-case ordering doesn't
    // matter for v1).
    for (int i = 1; i < NUM_DIRS; ++i) {
        ivec3 neigh = cell + C_I[i];
        if (mask_at(neigh) == 1u) {
            float fi = load_f(i, cell);
            store_f(OPPOSITE_DIR[i], cell, fi);
        }
    }

    float rho0 = U.u_inf.w;

    // -- (3a) -X face: velocity inlet (equilibrium boundary, v1).
    // Sets every population at the boundary cell to its equilibrium value
    // for (rho_0, u_inf). This is over-determining (the c_x<=0 populations
    // technically come from the streamed-in interior values), but the
    // first-order error is acceptable for v1 wind-tunnel visualization.
    if (cell.x == 0) {
        vec3 u = U.u_inf.xyz;
        store_f(0, cell, feq_i(0, rho0, u));
        for (int i = 1; i < NUM_DIRS; ++i) {
            store_f(i, cell, feq_i(i, rho0, u));
        }
    }

    // -- (3b) +X face: pressure outlet (equilibrium boundary, v1).
    // Density fixed at rho_0; velocity copied from one cell upstream
    // (cell - (1,0,0)) so the outflow inherits the local interior velocity.
    // First-order accurate; banked for v1.1 upgrade to Zou-He.
    if (cell.x == U.dims.x - 1) {
        // Sample upstream interior velocity from local f-state.
        ivec3 up_cell = ivec3(cell.x - 1, cell.y, cell.z);
        // If upstream is solid (shouldn't happen at outlet face in any
        // sane preset, but defensive), fall back to u_inf.
        vec3 u;
        if (mask_at(up_cell) == 1u) {
            u = U.u_inf.xyz;
        } else {
            float rho_local = load_f(0, up_cell);
            vec3  rho_u     = vec3(0.0);
            for (int i = 1; i < NUM_DIRS; ++i) {
                float f = load_f(i, up_cell);
                rho_local += f;
                rho_u     += f * vec3(C_I[i]);
            }
            u = (rho_local > 1e-12) ? (rho_u / rho_local) : U.u_inf.xyz;
        }
        store_f(0, cell, feq_i(0, rho0, u));
        for (int i = 1; i < NUM_DIRS; ++i) {
            store_f(i, cell, feq_i(i, rho0, u));
        }
    }

    // -- (4) +-Y, +-Z faces: free-slip (specular reflection).
    // For -Y wall: the incoming populations (c_iy = -1) are overwritten
    // with the corresponding outgoing populations whose c differs only
    // in the sign of c_y. Mapping pairs derived from § 2.2:
    //   i=4  (0,-1,0)  <-  i=3  (0,+1,0)
    //   i=8  (+1,-1,0) <-  i=7  (+1,+1,0)
    //   i=10 (-1,-1,0) <-  i=9  (-1,+1,0)
    //   i=17 (0,-1,+1) <-  i=15 (0,+1,+1)
    //   i=18 (0,-1,-1) <-  i=16 (0,+1,-1)
    if (cell.y == 0) {
        store_f( 4, cell, load_f( 3, cell));
        store_f( 8, cell, load_f( 7, cell));
        store_f(10, cell, load_f( 9, cell));
        store_f(17, cell, load_f(15, cell));
        store_f(18, cell, load_f(16, cell));
    }
    if (cell.y == U.dims.y - 1) {
        store_f( 3, cell, load_f( 4, cell));
        store_f( 7, cell, load_f( 8, cell));
        store_f( 9, cell, load_f(10, cell));
        store_f(15, cell, load_f(17, cell));
        store_f(16, cell, load_f(18, cell));
    }
    // -Z wall: incoming c_iz = -1.
    //   i=6  (0,0,-1)  <-  i=5  (0,0,+1)
    //   i=12 (+1,0,-1) <-  i=11 (+1,0,+1)
    //   i=14 (-1,0,-1) <-  i=13 (-1,0,+1)
    //   i=16 (0,+1,-1) <-  i=15 (0,+1,+1)
    //   i=18 (0,-1,-1) <-  i=17 (0,-1,+1)
    if (cell.z == 0) {
        store_f( 6, cell, load_f( 5, cell));
        store_f(12, cell, load_f(11, cell));
        store_f(14, cell, load_f(13, cell));
        store_f(16, cell, load_f(15, cell));
        store_f(18, cell, load_f(17, cell));
    }
    if (cell.z == U.dims.z - 1) {
        store_f( 5, cell, load_f( 6, cell));
        store_f(11, cell, load_f(12, cell));
        store_f(13, cell, load_f(14, cell));
        store_f(15, cell, load_f(16, cell));
        store_f(17, cell, load_f(18, cell));
    }
}
