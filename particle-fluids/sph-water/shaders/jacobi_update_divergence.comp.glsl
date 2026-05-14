// jacobi_update_divergence.comp.glsl — DFSPH divergence-solve Jacobi-update step.
//
// Reads aij_pj_scratch (filled by compute_aij_pj with solver_mode==1, i.e.
// scaled by dt) plus per-particle density_change and alpha/rho^2, and writes
// the relaxed Jacobi update of pressure.
//
// Reference: SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:644 (source term s_i =
// -density_change) and :606 (Jacobi update with relaxation 0.5).
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict readonly buffer DensityAlpha {
    // .x = density, .y = alpha/rho^2, .z = density_adv (density solve), .w = density_change (div solve)
    vec4 da[];
};
layout(set=0, binding=1, std430) restrict readonly buffer AijPjScratch {
    float aij_pj_scratch[];
};
layout(set=0, binding=2, std430) restrict readonly buffer PressureRead {
    float p_read[];
};
layout(set=0, binding=3, std430) restrict writeonly buffer PressureWrite {
    float p_write[];
};
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
};

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;

    float alpha_over_rho2_i = da[gid].y;
    float density_change    = da[gid].w;
    float aij_pj            = aij_pj_scratch[gid];
    float p_i               = p_read[gid];

    float s_i   = -density_change;
    float p_new = max(p_i - jacobiRelax * (s_i - aij_pj) * alpha_over_rho2_i, 0.0);
    p_write[gid] = p_new;
}
