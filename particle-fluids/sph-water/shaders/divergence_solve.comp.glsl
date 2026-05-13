// divergence_solve.comp.glsl — DFSPH divergence-free pressure inner-loop.
//
// References:
//   Source s_i = -ρ̇_i:           SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:662
//   aij_pj scales by h:           TimeStepDFSPH.cpp:656
//   Pressure update (Jacobi 0.5): TimeStepDFSPH.cpp:692
//   factor scales by 1/h:         TimeStepDFSPH.cpp:442
//
// NOTE: the precise a_ij pair-coupling formula left as a skeleton; the canonical
// upstream form involves the symmetric (factor_i + factor_j) pressure-gradient
// term scaled by particleMass·∇W. See § 4.D.2 architect-2 verification item 1.
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict buffer Particles { vec4 p[]; };
layout(set=0, binding=1, std430) restrict readonly buffer DensityAlpha { vec4 da[]; };
layout(set=0, binding=2, std430) restrict readonly buffer CellStarts { uint cell_starts[]; };
layout(set=0, binding=3, std430) restrict readonly buffer SortedIndex { uint sorted_index[]; };
layout(set=0, binding=4, std430) restrict readonly buffer PressureRead { float p_read[]; };
layout(set=0, binding=5, std430) restrict writeonly buffer PressureWrite { float p_write[]; };
layout(set=0, binding=6, std140) uniform U {
    uint  particleCount;
    uint  cellsPerAxisX;
    uint  cellsPerAxisY;
    uint  cellsPerAxisZ;
    float supportRadius;
    float supportRadius2;
    float particleMass;
    float density0;
    float dt;
    float kernelNorm3D;
    float gradKernelNorm3D;
    float jacobiRelax;
    vec4  domainMin_cellSize;
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
    float factor_i  = da[gid].y;
    float p_v_i     = p_read[gid];

    vec3  rel_i  = (pos_i - domainMin_cellSize.xyz) / domainMin_cellSize.w;
    ivec3 cell_i = ivec3(clamp(rel_i, vec3(0.0),
        vec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ) - vec3(1.0)));

    // Pass 1: ρ̇_i = Σ_j m_j (v_i − v_j) · ∇W_ij  →  s_i = -ρ̇_i (clamped ≥ 0).
    float rho_dot = 0.0;
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
            rho_dot += particleMass * dot(vel_i - vel_j, grad_W);
        }
    }
    float s_i = max(-rho_dot, 0.0);

    // Pass 2: aij_pj_sum = Σ_{j≠i} a_ij · p̃_v_j;  Jacobi update with relax = 0.5.
    // SKELETON COUPLING — replace with upstream-exact form per Callout 1.
    float aij_pj_sum = 0.0;
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
            float p_v_j     = p_read[j];
            vec3  r_ij      = pos_i - pos_j;
            float r_mag     = length(r_ij);
            float q         = r_mag / supportRadius;
            if (q >= 1.0 || r_mag < 1e-7) continue;
            vec3 grad_W = kernel_gradW(r_ij, r_mag, q, gradKernelNorm3D);
            // Placeholder coupling: m_j · |∇W|² scaled by h. Replace with the
            // symmetric (factor_i + factor_j)·m_j·∇W form from upstream.
            float coupling = particleMass * dot(grad_W, grad_W);
            aij_pj_sum += supportRadius * coupling * p_v_j;
        }
    }

    float factor_with_h_inv = factor_i / max(supportRadius, 1e-7);
    float new_p_v = max(p_v_i - jacobiRelax * (s_i - aij_pj_sum) * factor_with_h_inv, 0.0);
    p_write[gid]  = new_p_v;
}
