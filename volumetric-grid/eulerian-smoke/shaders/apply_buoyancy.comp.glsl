#version 450
#extension GL_GOOGLE_include_directive : require

// Boussinesq buoyancy: f_buoy = (alpha * T - beta * rho) * world_up.
// Reads density + temperature at the cell, modifies velocity at the cell.
// In-place write (no neighbor reads on velocity), per spec § 2.6.

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(set = 0, binding = 0, rgba16f) uniform image3D u_velocity;    // in-place
layout(set = 0, binding = 1) uniform sampler3D u_density;
layout(set = 0, binding = 2) uniform sampler3D u_temperature;

layout(set = 0, binding = 3) uniform BuoyancyUniforms {
    float dt;
    float alpha;             // temperature -> upward gain
    float beta;              // density -> downward gain (smoke weighs something)
    float referenceTemp;     // T_0; subtracted from T before alpha-multiplying
    uint  gridSize;
    uint  _pad0;
    uint  _pad1;
    uint  _pad2;
} bu;

void main() {
    ivec3 coord = ivec3(gl_GlobalInvocationID);
    if (any(greaterThanEqual(coord, ivec3(bu.gridSize)))) {
        return;
    }

    float invG = 1.0 / float(bu.gridSize);
    vec3 pos = (vec3(coord) + 0.5) * invG;

    float rho = texture(u_density, pos).r;
    float T   = texture(u_temperature, pos).r;

    vec4 v = imageLoad(u_velocity, coord);
    // Boussinesq buoyancy force in the +y direction.
    float buoy = bu.dt * (bu.alpha * (T - bu.referenceTemp) - bu.beta * rho);
    v.y += buoy;

    imageStore(u_velocity, coord, v);
}
