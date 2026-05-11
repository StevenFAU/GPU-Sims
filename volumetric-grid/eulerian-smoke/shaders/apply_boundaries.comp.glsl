#version 450
#extension GL_GOOGLE_include_directive : require

// Boundary enforcement: zero velocity at no-slip walls.
//   x = 0, x = N-1, z = 0, z = N-1, y = 0 -> velocity = 0
//   y = N-1 (ceiling) -> unchanged (sampler addressing handles zero-gradient outflow)
// Density and temperature at the boundary are not touched (the open-ceiling
// boundary's zero-gradient is encoded by sampler addressing CLAMP_TO_EDGE).

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(set = 0, binding = 0, rgba16f) uniform image3D u_velocity;    // in-place

layout(set = 0, binding = 1) uniform BoundariesUniforms {
    uint  gridSize;
    uint  _pad0;
    uint  _pad1;
    uint  _pad2;
} bu;

void main() {
    ivec3 coord = ivec3(gl_GlobalInvocationID);
    int N = int(bu.gridSize);
    if (coord.x < 0 || coord.x >= N || coord.y < 0 || coord.y >= N || coord.z < 0 || coord.z >= N) {
        return;
    }

    bool wall_x = (coord.x == 0) || (coord.x == N - 1);
    bool wall_z = (coord.z == 0) || (coord.z == N - 1);
    bool wall_y = (coord.y == 0);   // floor only; ceiling is open
    if (wall_x || wall_z || wall_y) {
        imageStore(u_velocity, coord, vec4(0.0));
    }
}
