// compute_density_change.comp.glsl — DFSPH divergence-solve source term.
//
// densityChange_i = V_i * Σ_j (v_i − v_j) · ∇W_ij
//
// Result written to da[gid].w. Other components (.x density, .y α/ρ²,
// .z density_adv) are preserved. No particle-deficiency clamp here —
// upstream applies it in the divergenceSolve driver / divergenceSolveIteration,
// not inside computeDensityChange.
//
// Reference: SPlisHSPlasH 2.16.1 SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1247-1295 (computeDensityChange).
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict readonly buffer Particles { vec4 p[]; };
layout(set=0, binding=1, std430) restrict buffer DensityAlpha { vec4 da[]; };
layout(set=0, binding=2, std430) restrict readonly buffer CellStarts { uint cell_starts[]; };
layout(set=0, binding=3, std430) restrict readonly buffer SortedIndex { uint sorted_index[]; };
layout(set=0, binding=4, std140) uniform U {
    uint  particleCount;
    uint  cellsPerAxisX;
    uint  cellsPerAxisY;
    uint  cellsPerAxisZ;
    float supportRadius;
    float particleMass;
    float density0;
    float kernelNorm3D;
    float gradKernelNorm3D;
    float dt;
    float viscosity;
    float cohesion;
    float vorticityStrength;
    float jacobiRelax;
    float _pad0;
    float _pad1;
    vec4  gravity_pad;
    vec4  domainMin_cellSize;
    vec4  domainMax_pad;
    uint  boundaryParticleCount;
    uint  _bpad0;
    uint  _bpad1;
    uint  _bpad2;
};
// Total: 128 bytes
layout(set=0, binding=5, std430) restrict readonly buffer BoundaryParticles {
    vec4 boundary_particles[];
};
layout(set=0, binding=6, std430) restrict readonly buffer BoundaryVolumes {
    float boundary_volumes[];
};
layout(set=0, binding=7, std430) restrict readonly buffer BoundaryCellStarts {
    uint boundary_cell_starts[];
};
layout(set=0, binding=8, std430) restrict readonly buffer BoundarySortedIndex {
    uint boundary_sorted_index[];
};

const float DFSPH_ALPHA_EPS = 1.0e-5;

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
vec3 kernel_gradW(vec3 r_ij, float r_mag, float q, float grad_kernel_norm) {
    float poly = 0.0;
    if (q < 0.5)      poly = 18.0*q*q - 12.0*q;
    else if (q < 1.0) { float omq = 1.0 - q; poly = -6.0 * omq * omq; }
    return (grad_kernel_norm * poly / r_mag) * r_ij;
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;

    vec3  pos_i     = p[gid * 8u + 0u].xyz;
    vec3  vel_i     = p[gid * 8u + 1u].xyz;
    float density_i = da[gid].x;

    vec3  rel_i  = (pos_i - domainMin_cellSize.xyz) / domainMin_cellSize.w;
    ivec3 cell_i = ivec3(clamp(rel_i, vec3(0.0),
        vec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ) - vec3(1.0)));

    float delta = 0.0;
    for (int dz = -1; dz <= 1; ++dz)
    for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx) {
        ivec3 ncell = cell_i + ivec3(dx, dy, dz);
        if (any(lessThan(ncell, ivec3(0))) ||
            any(greaterThanEqual(ncell, ivec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ))))
            continue;
        uint nmorton = morton_encode_3d(uvec3(ncell));
        uint nstart  = cell_starts[nmorton];
        uint nend    = cell_starts[nmorton + 1u];
        for (uint k = nstart; k < nend; ++k) {
            uint j = sorted_index[k];
            if (j == gid) continue;
            vec3  pos_j = p[j * 8u + 0u].xyz;
            vec3  vel_j = p[j * 8u + 1u].xyz;
            vec3  r_ij  = pos_i - pos_j;
            float r_mag = length(r_ij);
            float q     = r_mag / supportRadius;
            if (q >= 1.0 || r_mag < 1e-7) continue;
            vec3 grad_W = kernel_gradW(r_ij, r_mag, q, gradKernelNorm3D);
            delta += dot(vel_i - vel_j, grad_W);
        }
    }

    float V_i = particleMass / max(density_i, DFSPH_ALPHA_EPS);
    float densityChange = V_i * delta;

    // Akinci2012 boundary contribution at SPlisHSPlasH 2.16.1 SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1272-1278:
    // static boundary v_j = 0 reduces to V_b * vel_i · grad_W. NOT scaled
    // by V_i (upstream pattern).
    if (boundaryParticleCount > 0u) {
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
                uint j_b = boundary_sorted_index[k];
                vec3  pos_j = boundary_particles[j_b].xyz;
                float Vb_j  = boundary_volumes[j_b];
                vec3  r_ij  = pos_i - pos_j;
                float r_mag = length(r_ij);
                float q     = r_mag / supportRadius;
                if (q >= 1.0 || r_mag < 1e-7) continue;
                vec3 grad_W = kernel_gradW(r_ij, r_mag, q, gradKernelNorm3D);
                densityChange += Vb_j * dot(vel_i, grad_W);
            }
        }
    }

    da[gid].w = densityChange;
}
