#version 450
#extension GL_GOOGLE_include_directive : require

// Volumetric source-injection emitter pass.
// For each emitter in the array, find cells within its radius and add:
//   density_new[cell]     += rate * dt * gauss_falloff
//   temperature_new[cell] += temperature * dt * gauss_falloff
//   velocity_new[cell].y  += velocity_bias * dt * gauss_falloff
//
// Multiple overlapping emitters SUM (not last-write-wins), which is why this
// kernel does imageLoad + add + imageStore rather than imageStore alone.
// In-place writes on all three fields (no neighbor reads).

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(set = 0, binding = 0, rgba16f) uniform image3D u_velocity;     // in-place
layout(set = 0, binding = 1, r32f)    uniform image3D u_density;      // in-place
layout(set = 0, binding = 2, r32f)    uniform image3D u_temperature;  // in-place

// Emitter array; cap is 8, see EMITTER_CAP in main.cpp. count <= 8.
struct Emitter {
    vec4 pos_radius;          // .xyz = pos in [0,1]^3, .w = radius (in cells, not normalized)
    vec4 rate_temp_bias_pad;  // .x = density rate, .y = temperature, .z = velocity bias y, .w = pad
};

layout(set = 0, binding = 3) uniform EmitterUniforms {
    uint  count;
    uint  gridSize;
    float dt;
    float falloffPower;       // Gaussian falloff exponent (EMITTER_FALLOFF_POWER)
    Emitter emitters[8];
} eu;

void main() {
    ivec3 coord = ivec3(gl_GlobalInvocationID);
    if (any(greaterThanEqual(coord, ivec3(eu.gridSize)))) {
        return;
    }

    if (eu.count == 0u) {
        return;
    }

    float invG = 1.0 / float(eu.gridSize);
    vec3 cellPos = (vec3(coord) + 0.5) * invG;   // cell center in [0,1]^3

    float density_add = 0.0;
    float temp_add    = 0.0;
    float velY_add    = 0.0;

    for (uint i = 0u; i < eu.count; ++i) {
        Emitter e = eu.emitters[i];
        vec3 ePos = e.pos_radius.xyz;
        float eRadius = e.pos_radius.w * invG;    // convert cells -> normalized

        vec3 d = cellPos - ePos;
        float dist = length(d);
        if (dist > eRadius) continue;

        // Gaussian-falloff: smooth 1 at center, 0 at radius edge.
        // falloff = exp(-(dist/radius)^power * 2)
        float t = dist / max(eRadius, 1e-6);
        float falloff = exp(-pow(t, eu.falloffPower) * 2.0);

        density_add += e.rate_temp_bias_pad.x * eu.dt * falloff;
        temp_add    += e.rate_temp_bias_pad.y * eu.dt * falloff;
        velY_add    += e.rate_temp_bias_pad.z * eu.dt * falloff;
    }

    if (density_add > 0.0 || temp_add != 0.0 || velY_add != 0.0) {
        vec4 v = imageLoad(u_velocity, coord);
        v.y += velY_add;
        imageStore(u_velocity, coord, v);

        float rho = imageLoad(u_density, coord).r;
        rho += density_add;
        imageStore(u_density, coord, vec4(rho, 0.0, 0.0, 0.0));

        float T = imageLoad(u_temperature, coord).r;
        T += temp_add;
        imageStore(u_temperature, coord, vec4(T, 0.0, 0.0, 0.0));
    }
}
