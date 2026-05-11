#version 450
#extension GL_GOOGLE_include_directive : require

// Single-pass MacCormack-corrected semi-Lagrangian advection of the velocity field.
// Self-advection: velocity is advected along itself.
//
// Algorithm (single-pass MacCormack — Selle/Fedkiw 2008 reverse-Stam framework):
//   back_pos  = pos - dt * v(pos)                    backward trace
//   phi_hat   = phi_old(back_pos)                    forward semi-Lagrangian sample
//   round_pos = back_pos + dt * v(back_pos)          re-advance from source position
//   phi_back  = phi_old(round_pos)                   round-trip sample (single-pass approximation)
//   phi_new   = phi_hat + 0.5 * (phi_old(pos) - phi_back)            correction
//   phi_new   = clamp(phi_new, min8, max8)                            reverse-Stam limiter
// where min8/max8 are the min and max of the 8 corner samples of phi_old
// at the BACKWARD-TRACED position (NOT the forward-traced position; this
// is the common bug callout in phase8_eulerian_smoke.md § 0.5 #1).
//
// The "single-pass" qualifier: textbook MacCormack (Fedkiw 2001 eq 8-11) defines
// phi_back as SemiLagrangian(phi_hat, v, -dt), which requires materializing
// phi_hat as a temporary field. The single-pass approximation samples phi_old at
// round_pos instead — well-trodden in real-time fluid sims; numerical-stability
// and convergence properties documented in Selle/Fedkiw 2008.
//
// Boundary conditions: sampler addressing mode CLAMP_TO_EDGE (set on the
// CPU side) gives zero-gradient at the field edges. The explicit boundary
// pass that follows in the dispatch chain zeros the velocity at the five
// no-slip wall faces.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

// Old velocity (read via sampler with LINEAR filter + CLAMP_TO_EDGE addressing).
layout(set = 0, binding = 0) uniform sampler3D u_velocity_old;

// New velocity (write).
layout(set = 0, binding = 1, rgba16f) uniform writeonly image3D u_velocity_new;

layout(set = 0, binding = 2) uniform AdvectUniforms {
    float dt;
    float dissipation;       // velocity dissipation rate (per substep)
    float maccormackEpsilon; // limiter relaxation (0 = pure clamp; ignored at 0)
    float _pad0;
    uint  gridSize;
    uint  _pad1;
    uint  _pad2;
    uint  _pad3;
} au;

// Trilinear sample of velocity at a normalized [0,1]³ position.
vec3 sampleVelocity(vec3 pos) {
    return texture(u_velocity_old, pos).xyz;
}

// 8-corner min/max of the OLD velocity field around a normalized [0,1]³ position.
// Returns (min_x, min_y, min_z) and (max_x, max_y, max_z) componentwise — used by
// the reverse-Stam limiter to bound the corrected output.
void cornerMinMax(vec3 pos, out vec3 outMin, out vec3 outMax) {
    float invG = 1.0 / float(au.gridSize);
    // Cell-relative position: shift so corner sampling lands on integer texel centers.
    vec3 cellPos = pos * float(au.gridSize) - 0.5;
    vec3 i0 = floor(cellPos) * invG + 0.5 * invG; // texel-center of lower corner
    vec3 i1 = i0 + invG;                          // texel-center of upper corner

    outMin =  vec3( 1.0 / 0.0);  // +Inf
    outMax = -vec3( 1.0 / 0.0);  // -Inf

    for (int dz = 0; dz <= 1; ++dz) {
        for (int dy = 0; dy <= 1; ++dy) {
            for (int dx = 0; dx <= 1; ++dx) {
                vec3 corner = vec3(
                    dx == 0 ? i0.x : i1.x,
                    dy == 0 ? i0.y : i1.y,
                    dz == 0 ? i0.z : i1.z
                );
                // Sample with NEAREST behavior via textureLod at the exact texel center.
                vec3 v = texture(u_velocity_old, corner).xyz;
                outMin = min(outMin, v);
                outMax = max(outMax, v);
            }
        }
    }
}

void main() {
    ivec3 coord = ivec3(gl_GlobalInvocationID);
    if (any(greaterThanEqual(coord, ivec3(au.gridSize)))) {
        return;
    }

    float invG = 1.0 / float(au.gridSize);
    vec3 pos = (vec3(coord) + 0.5) * invG;                       // cell center in [0,1]³

    // Forward semi-Lagrangian trace: where did the velocity at `pos` come from dt seconds ago?
    vec3 v_here = sampleVelocity(pos);
    vec3 back_pos = pos - au.dt * v_here * invG;                  // dt in cell-units via invG

    // First half of MacCormack: forward trace (this is phi_hat in the math).
    vec3 phi_hat = sampleVelocity(back_pos);

    // Second half: backward trace from phi_hat by +dt (in shader-time, this is "go forward").
    // Sample velocity at phi_hat's position to get the velocity that would have moved phi_hat back to pos.
    vec3 v_back = sampleVelocity(back_pos);
    vec3 fwd_pos = back_pos + au.dt * v_back * invG;
    vec3 phi_back = sampleVelocity(fwd_pos);

    // Correction (Fedkiw 2001 eq 11): phi_new = phi_hat + 0.5 * (phi_old - phi_back).
    vec3 phi_old = sampleVelocity(pos);    // velocity that USED to be at `pos`
    vec3 phi_new = phi_hat + 0.5 * (phi_old - phi_back);

    // Reverse-Stam clamping limiter: clamp phi_new to the 8-corner extrema of the OLD
    // field at the BACKWARD-traced position. This is the load-bearing detail.
    vec3 corner_min, corner_max;
    cornerMinMax(back_pos, corner_min, corner_max);
    // Optional epsilon relaxation: widen the clamp interval by ±epsilon. At epsilon=0 this is the
    // pure clamp; positive epsilon lets MacCormack overshoots through, which can help retain
    // sharp features at the cost of stability.
    vec3 widen = vec3(au.maccormackEpsilon);
    phi_new = clamp(phi_new, corner_min - widen, corner_max + widen);

    // Apply per-substep dissipation.
    phi_new *= (1.0 - au.dt * au.dissipation);

    imageStore(u_velocity_new, coord, vec4(phi_new, 0.0));
}
