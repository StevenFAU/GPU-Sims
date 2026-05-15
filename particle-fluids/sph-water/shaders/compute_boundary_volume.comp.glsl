// compute_boundary_volume.comp.glsl — Akinci2012 boundary volume kernel.
//
// One thread per boundary particle. Computes
//   delta_i = W_zero + Σ_{j ∈ boundary} W(x_i - x_j)
//   V_i     = 1 / delta_i
// Boundary-only neighbor scan; mirrors the upstream pattern at SPlisHSPlasH
// 2.16.1 SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48-75 (computeBoundaryVolume).
// Dispatched ONCE at preset load — the result is stored permanently in
// boundary_volumes[] and read by the five fluid shaders' boundary branches.
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict readonly buffer BoundaryParticles {
    vec4 boundary_particles[];
};
layout(set=0, binding=1, std430) restrict readonly buffer BoundaryCellStarts {
    uint boundary_cell_starts[];
};
layout(set=0, binding=2, std430) restrict readonly buffer BoundarySortedIndex {
    uint boundary_sorted_index[];
};
layout(set=0, binding=3, std430) restrict writeonly buffer BoundaryVolumes {
    float boundary_volumes[];
};
layout(set=0, binding=4, std140) uniform U {
    uint  boundaryParticleCount;
    uint  cellsPerAxisX;
    uint  cellsPerAxisY;
    uint  cellsPerAxisZ;
    float supportRadius;
    float kernelNorm3D;
    float gradKernelNorm3D;
    float _pad0;
    vec4  domainMin_cellSize;        // .xyz = domainMin, .w = cellSize
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
    return (expand_bits_10(c.x) << 2) | (expand_bits_10(c.y) << 1) | expand_bits_10(c.z);
}
float kernel_W(float q, float kernel_norm) {
    if (q < 0.5)      return kernel_norm * (6.0*q*q*q - 6.0*q*q + 1.0);
    else if (q < 1.0) { float omq = 1.0 - q; return kernel_norm * 2.0 * omq*omq*omq; }
    else              return 0.0;
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= boundaryParticleCount) return;

    vec3 pos_i = boundary_particles[gid].xyz;

    // W(0) — self contribution. Mirrors `Real delta = sim->W_zero();` at
    // SPlisHSPlasH 2.16.1 SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:61.
    float delta = kernel_W(0.0, kernelNorm3D);

    vec3  rel_i  = (pos_i - domainMin_cellSize.xyz) / domainMin_cellSize.w;
    ivec3 cell_i = ivec3(clamp(rel_i, vec3(0.0),
        vec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ) - vec3(1.0)));

    for (int dz = -1; dz <= 1; ++dz)
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx) {
        ivec3 ncell = cell_i + ivec3(dx, dy, dz);
        if (any(lessThan(ncell, ivec3(0))) ||
            any(greaterThanEqual(ncell, ivec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ))))
            continue;
        uint nmorton = morton_encode_3d(uvec3(ncell));
        uint nstart  = boundary_cell_starts[nmorton];
        uint nend    = boundary_cell_starts[nmorton + 1u];
        for (uint k = nstart; k < nend; ++k) {
            uint j = boundary_sorted_index[k];
            if (j == gid) continue;
            vec3  pos_j = boundary_particles[j].xyz;
            vec3  r_ij  = pos_i - pos_j;
            float r_mag = length(r_ij);
            float q     = r_mag / supportRadius;
            if (q >= 1.0) continue;
            delta += kernel_W(q, kernelNorm3D);
        }
    }

    boundary_volumes[gid] = 1.0 / max(delta, 1.0e-9);
}
