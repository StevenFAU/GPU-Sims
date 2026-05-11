#version 450
#extension GL_GOOGLE_include_directive : require

// Single-pass MacCormack-corrected semi-Lagrangian advection of a scalar field.
// Same algorithm as advect_velocity.comp.glsl with vec3 -> float substitution.
// See advect_velocity.comp.glsl for the full single-pass MacCormack (Selle/Fedkiw 2008)
// algorithm description and the limiter rationale.
// Velocity for the trace is the post-velocity-advection field (passed by binding).

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(set = 0, binding = 0) uniform sampler3D u_scalar_old;
layout(set = 0, binding = 1) uniform sampler3D u_velocity;          // post-advection
layout(set = 0, binding = 2, r32f) uniform writeonly image3D u_scalar_new;

layout(set = 0, binding = 3) uniform AdvectScalarUniforms {
    float dt;
    float dissipation;
    float maccormackEpsilon;
    float _pad0;
    uint  gridSize;
    uint  _pad1;
    uint  _pad2;
    uint  _pad3;
} au;

float sampleScalar(vec3 pos) {
    return texture(u_scalar_old, pos).r;
}

vec3 sampleVelocity(vec3 pos) {
    return texture(u_velocity, pos).xyz;
}

void cornerMinMax(vec3 pos, out float outMin, out float outMax) {
    float invG = 1.0 / float(au.gridSize);
    vec3 cellPos = pos * float(au.gridSize) - 0.5;
    vec3 i0 = floor(cellPos) * invG + 0.5 * invG;
    vec3 i1 = i0 + invG;

    outMin =  1.0 / 0.0;  // +Inf
    outMax = -1.0 / 0.0;  // -Inf

    for (int dz = 0; dz <= 1; ++dz) {
        for (int dy = 0; dy <= 1; ++dy) {
            for (int dx = 0; dx <= 1; ++dx) {
                vec3 corner = vec3(
                    dx == 0 ? i0.x : i1.x,
                    dy == 0 ? i0.y : i1.y,
                    dz == 0 ? i0.z : i1.z
                );
                float s = texture(u_scalar_old, corner).r;
                outMin = min(outMin, s);
                outMax = max(outMax, s);
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
    vec3 pos = (vec3(coord) + 0.5) * invG;

    vec3 v_here = sampleVelocity(pos);
    vec3 back_pos = pos - au.dt * v_here * invG;

    float phi_hat = sampleScalar(back_pos);

    vec3 v_back = sampleVelocity(back_pos);
    vec3 fwd_pos = back_pos + au.dt * v_back * invG;
    float phi_back = sampleScalar(fwd_pos);

    float phi_old = sampleScalar(pos);
    float phi_new = phi_hat + 0.5 * (phi_old - phi_back);

    float corner_min, corner_max;
    cornerMinMax(back_pos, corner_min, corner_max);
    float widen = au.maccormackEpsilon;
    phi_new = clamp(phi_new, corner_min - widen, corner_max + widen);

    phi_new *= (1.0 - au.dt * au.dissipation);
    // Scalars (density, temperature) are positive-valued — clamp to >= 0 to guard
    // against rounding-noise undershoot on the lower bound.
    phi_new = max(phi_new, 0.0);

    imageStore(u_scalar_new, coord, vec4(phi_new, 0.0, 0.0, 0.0));
}
