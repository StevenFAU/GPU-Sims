// cell_count.comp.glsl — atomicAdd 1 to per-cell count for each particle.
// Translated from agent-based/boids-3d/web/shaders/cell_count.compute.wgsl.
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict readonly buffer MortonCodes {
    uint codes[];
};
layout(set=0, binding=1, std430) restrict buffer CellCounts {
    uint counts[];
};
layout(set=0, binding=2, std140) uniform U {
    uint particleCount;
    uint _pad0; uint _pad1; uint _pad2;
    vec4 _pad3; vec4 _pad4; vec4 _pad5;
};

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;
    uint key = codes[gid];
    atomicAdd(counts[key], 1u);
}
