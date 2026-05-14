// integrate_forces.comp.glsl — Two-mode kernel.
//
//   gravity_pad.w == 0.0: FORCES_ONLY — v += dt · (gravity + viscosity + cohesion)
//   gravity_pad.w == 1.0: POSITION_ONLY — x += dt · v with AABB box clamp
//
// Dispatched twice per substep.
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict buffer Particles { vec4 p[]; };
layout(set=0, binding=1, std430) restrict readonly buffer DensityAlpha { vec4 da[]; };
layout(set=0, binding=2, std430) restrict readonly buffer CellStarts { uint cell_starts[]; };
layout(set=0, binding=3, std430) restrict readonly buffer SortedIndex { uint sorted_index[]; };
layout(set=0, binding=4, std140) uniform U {
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
    uint mode;
} pc;

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
    if (gid >= particleCount) return;

    vec3 pos_i = p[gid * 8u + 0u].xyz;
    vec3 vel_i = p[gid * 8u + 1u].xyz;
    int  mode  = int(pc.mode);

    if (mode == 1) {
        // POSITION_ONLY mode.
        vec3 pos_new = pos_i + dt * vel_i;
        vec3 vel_new = vel_i;
        vec3 dmin    = domainMin_cellSize.xyz;
        vec3 dmax    = domainMax_pad.xyz;
        for (int ax = 0; ax < 3; ++ax) {
            if (pos_new[ax] < dmin[ax]) { pos_new[ax] = dmin[ax]; vel_new[ax] = max(vel_new[ax], 0.0); }
            if (pos_new[ax] > dmax[ax]) { pos_new[ax] = dmax[ax]; vel_new[ax] = min(vel_new[ax], 0.0); }
        }
        p[gid * 8u + 0u].xyz = pos_new;
        p[gid * 8u + 1u].xyz = vel_new;
        return;
    }

    // FORCES_ONLY mode.
    vec3 a_visc     = vec3(0.0);
    vec3 a_cohesion = vec3(0.0);

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
        uint nstart  = cell_starts[nmorton];
        uint nend    = cell_starts[nmorton + 1u];
        for (uint k = nstart; k < nend; ++k) {
            uint j = sorted_index[k];
            if (j == gid) continue;
            vec3  pos_j     = p[j * 8u + 0u].xyz;
            vec3  vel_j     = p[j * 8u + 1u].xyz;
            float density_j = max(da[j].x, 1e-3);
            vec3  r_ij      = pos_i - pos_j;
            float r_mag     = length(r_ij);
            float q         = r_mag / supportRadius;
            if (q >= 1.0 || r_mag < 1e-7) continue;

            float W = kernel_W(q, kernelNorm3D);
            a_visc     += viscosity * (particleMass / density_j) * (vel_j - vel_i) * W;
            a_cohesion -= cohesion  * particleMass * W * (r_ij / r_mag);
        }
    }

    // Vorticity confinement: simple curl-approximation skeleton (v1 placeholder).
    // Full Stam-style banked v1.1; for now leave as no-op weighted by vorticityStrength.
    vec3 a_vorticity = vec3(0.0) * vorticityStrength;

    vec3 a_total = gravity_pad.xyz + a_visc + a_cohesion + a_vorticity;
    p[gid * 8u + 1u].xyz = vel_i + dt * a_total;
}
