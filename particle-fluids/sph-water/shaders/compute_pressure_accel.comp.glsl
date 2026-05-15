// compute_pressure_accel.comp.glsl — DFSPH per-particle pressure acceleration.
//
// a_i = -Σ_j m_j (p_i/ρ_i² + p_j/ρ_j²) ∇W_ij
//
// Single-fluid GPU port: the multiphase pSum = p_rho2_i + (ρ0_n/ρ0_s) · p_rho2_j
// collapses to p_rho2_i + p_rho2_j because ρ0_n == ρ0_s. Result is the per-
// particle pressure acceleration; it is *not* applied to velocity here —
// apply_velocity.comp.glsl integrates it after the inner loop converges.
//
// Reference: SPlisHSPlasH 2.16.1 SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1299-1367 (computePressureAccel, fluid-only branch).
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict readonly buffer Particles { vec4 p[]; };
layout(set=0, binding=1, std430) restrict readonly buffer DensityAlpha { vec4 da[]; };
layout(set=0, binding=2, std430) restrict readonly buffer PressureRead { float p_read[]; };
layout(set=0, binding=3, std430) restrict readonly buffer CellStarts { uint cell_starts[]; };
layout(set=0, binding=4, std430) restrict readonly buffer SortedIndex { uint sorted_index[]; };
layout(set=0, binding=5, std430) restrict writeonly buffer PressureAccel { vec4 pressure_accel[]; };
layout(set=0, binding=6, std140) uniform U {
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
layout(set=0, binding=7, std430) restrict readonly buffer BoundaryParticles {
    vec4 boundary_particles[];
};
layout(set=0, binding=8, std430) restrict readonly buffer BoundaryVolumes {
    float boundary_volumes[];
};
layout(set=0, binding=9, std430) restrict readonly buffer BoundaryCellStarts {
    uint boundary_cell_starts[];
};
layout(set=0, binding=10, std430) restrict readonly buffer BoundarySortedIndex {
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
    float density_i = da[gid].x;
    float p_i       = p_read[gid];
    float rho_i2    = max(density_i * density_i, DFSPH_ALPHA_EPS);
    float p_rho2_i  = p_i / rho_i2;

    vec3  rel_i  = (pos_i - domainMin_cellSize.xyz) / domainMin_cellSize.w;
    ivec3 cell_i = ivec3(clamp(rel_i, vec3(0.0),
        vec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ) - vec3(1.0)));

    vec3 a_press = vec3(0.0);
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
            vec3  pos_j     = p[j * 8u + 0u].xyz;
            float density_j = da[j].x;
            float p_j       = p_read[j];
            vec3  r_ij      = pos_i - pos_j;
            float r_mag     = length(r_ij);
            float q         = r_mag / supportRadius;
            if (q >= 1.0 || r_mag < 1e-7) continue;
            vec3  grad_W    = kernel_gradW(r_ij, r_mag, q, gradKernelNorm3D);
            float rho_j2    = max(density_j * density_j, DFSPH_ALPHA_EPS);
            float p_rho2_j  = p_j / rho_j2;
            a_press -= particleMass * (p_rho2_i + p_rho2_j) * grad_W;
        }
    }

    // Akinci2012 boundary contribution at SPlisHSPlasH 2.16.1 SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1335-1344:
    // boundary has no pressure, so pSum reduces to p_rho2_i alone and
    // grad_p_j = -V_b * grad_W. Skip when |p_rho2_i| is below eps
    // (upstream condition at line 1333).
    if (boundaryParticleCount > 0u && abs(p_rho2_i) > DFSPH_ALPHA_EPS) {
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
                // grad_p_j = -V_b * grad_W; a = p_rho2_i * grad_p_j
                a_press -= p_rho2_i * Vb_j * grad_W;
            }
        }
    }

    pressure_accel[gid] = vec4(a_press, 0.0);
}
