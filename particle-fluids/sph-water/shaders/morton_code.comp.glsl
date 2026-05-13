// morton_code.comp.glsl — 30-bit Morton code (10 bits per axis) per particle.
// Standard "magic numbers" bit-interleave (Karras 2012 fast BVH paper).
#version 460
#extension GL_GOOGLE_include_directive : enable

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict readonly buffer Particles {
    vec4 p[];   // 8 vec4 per particle; offsets per shaders/_struct_layouts.txt
};
layout(set=0, binding=1, std430) restrict writeonly buffer MortonCodes {
    uint codes[];
};
layout(set=0, binding=2, std140) uniform U {
    uint  particleCount;
    uint  cellsPerAxis;       // unused; kept for layout parity with sort_ub host struct
    uint  totalCells;
    uint  _pad0;
    vec4  cellsXYZ_pad;       // .xyz = per-axis cell counts (rounded to pow2)
    vec4  domainMin_pad;      // .xyz = domain min; .w = cellSize
    vec4  domainMax_pad;
};

uint expand_bits_10(uint v) {
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

uint morton_encode_3d(uvec3 c) {
    return (expand_bits_10(c.x) << 2)
         | (expand_bits_10(c.y) << 1)
         |  expand_bits_10(c.z);
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;

    vec3  pos        = p[gid * 8u + 0u].xyz;
    vec3  domain_min = domainMin_pad.xyz;
    float cell_size  = domainMin_pad.w;
    uvec3 cells_axis = uvec3(cellsXYZ_pad.xyz);

    vec3  rel  = (pos - domain_min) / cell_size;
    uvec3 cell = uvec3(clamp(rel, vec3(0.0), vec3(cells_axis) - vec3(1.0)));

    codes[gid] = morton_encode_3d(cell);
}
