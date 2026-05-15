// compute_aij_pj.comp.glsl — DFSPH per-particle Σ_j (a_i − a_j) · ∇W stencil.
//
// aij_pj_i = V_i * Σ_j (a_i − a_j) · ∇W_ij
//
// `a_i` is the per-particle pressure acceleration produced by
// compute_pressure_accel.comp.glsl in the prior dispatch of the inner-loop.
// Result is scaled by dt² for density mode (pc.solver_mode==0) or dt for
// divergence mode (pc.solver_mode==1) before being written to
// aij_pj_scratch[gid], so the Jacobi pressure-update step can consume it as-is.
//
// Reference: SPlisHSPlasH 2.16.1 SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1370-1422 (compute_aij_pj scalar variant, fluid-only branch).
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict readonly buffer Particles { vec4 p[]; };
layout(set=0, binding=1, std430) restrict readonly buffer DensityAlpha { vec4 da[]; };
layout(set=0, binding=2, std430) restrict readonly buffer CellStarts { uint cell_starts[]; };
layout(set=0, binding=3, std430) restrict readonly buffer SortedIndex { uint sorted_index[]; };
layout(set=0, binding=4, std430) restrict readonly buffer PressureAccel { vec4 pressure_accel[]; };
layout(set=0, binding=5, std430) restrict writeonly buffer AijPjScratch { float aij_pj_scratch[]; };
layout(set=0, binding=6, std140) uniform U {
    // Integer counts                            offset
    uint  particleCount;                       //   0
    uint  cellsPerAxisX;                       //   4
    uint  cellsPerAxisY;                       //   8
    uint  cellsPerAxisZ;                       //  12
    // SPH kernel constants
    float supportRadius;                       //  16
    float particleMass;                        //  20
    float density0;                            //  24
    float kernelNorm3D;                        //  28
    float gradKernelNorm3D;                    //  32
    // Time integration
    float dt;                                  //  36
    // Force coefficients
    float viscosity;                           //  40
    float cohesion;                            //  44
    float vorticityStrength;                   //  48
    // Solver tuning
    float jacobiRelax;                         //  52
    // Padding to align next vec4 to 16 B
    float _pad0;                               //  56
    float _pad1;                               //  60
    // Vec4 block
    vec4  gravity_pad;                         //  64  (.xyz=gravity, .w=mode)
    vec4  domainMin_cellSize;                  //  80  (.xyz=domainMin, .w=cellSize)
    vec4  domainMax_pad;                       //  96  (.xyz=domainMax)
};
// Total: 112 bytes

layout(push_constant) uniform PC {
    uint solver_mode;   // 0 = density (*= dt²), 1 = divergence (*= dt)
} pc;

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
    vec3  a_i       = pressure_accel[gid].xyz;

    vec3  rel_i  = (pos_i - domainMin_cellSize.xyz) / domainMin_cellSize.w;
    ivec3 cell_i = ivec3(clamp(rel_i, vec3(0.0),
        vec3(cellsPerAxisX, cellsPerAxisY, cellsPerAxisZ) - vec3(1.0)));

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
            vec3  pos_j = p[j * 8u + 0u].xyz;
            vec3  a_j   = pressure_accel[j].xyz;
            vec3  r_ij  = pos_i - pos_j;
            float r_mag = length(r_ij);
            float q     = r_mag / supportRadius;
            if (q >= 1.0 || r_mag < 1e-7) continue;
            vec3 grad_W = kernel_gradW(r_ij, r_mag, q, gradKernelNorm3D);
            aij_pj_sum += dot(a_i - a_j, grad_W);
        }
    }

    float V_i = particleMass / max(density_i, DFSPH_ALPHA_EPS);
    aij_pj_sum *= V_i;
    if (pc.solver_mode == 0u) aij_pj_sum *= dt * dt;
    else                       aij_pj_sum *= dt;
    aij_pj_scratch[gid] = aij_pj_sum;
}
