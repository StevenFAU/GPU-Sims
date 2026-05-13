// prefix_sum_addback.comp.glsl — Add each block's prefix into every element
// of that block's per-block-scanned data → final cell_starts.
// Translated from agent-based/boids-3d/web/shaders/prefix_sum_addback.compute.wgsl.
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict readonly buffer PerBlock {
    uint per_block[];
};
layout(set=0, binding=1, std430) restrict readonly buffer BlockPrefixes {
    uint block_prefixes[];
};
layout(set=0, binding=2, std430) restrict writeonly buffer CellStarts {
    uint cell_starts[];
};
layout(set=0, binding=3, std140) uniform U {
    uint particleCount;
    uint _pad0; uint totalCells; uint _pad1;
    vec4 _pad2; vec4 _pad3; vec4 _pad4;
};

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= totalCells) return;
    uint block = gl_WorkGroupID.x;
    cell_starts[gid] = per_block[gid] + block_prefixes[block];
}
