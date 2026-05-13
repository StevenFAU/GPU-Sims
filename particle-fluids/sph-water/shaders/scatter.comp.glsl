// scatter.comp.glsl — Each particle looks up its Morton-cell start, atomically
// claims a slot via cell_counts_atomic, writes its original index to sorted.
// Translated from agent-based/boids-3d/web/shaders/scatter.compute.wgsl.
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict readonly buffer MortonCodes {
    uint codes[];
};
layout(set=0, binding=1, std430) restrict readonly buffer CellStarts {
    uint cell_starts[];
};
layout(set=0, binding=2, std430) restrict writeonly buffer SortedIndex {
    uint sorted[];
};
layout(set=0, binding=3, std430) restrict buffer CellCountsAtomic {
    uint atomic_counts[];
};
layout(set=0, binding=4, std140) uniform U {
    uint particleCount;
    uint _pad0; uint _pad1; uint _pad2;
    vec4 _pad3; vec4 _pad4; vec4 _pad5;
};

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;

    uint key          = codes[gid];
    uint slot_in_cell = atomicAdd(atomic_counts[key], 1u);
    sorted[cell_starts[key] + slot_in_cell] = gid;
}
