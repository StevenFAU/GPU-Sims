// apply_velocity.comp.glsl — apply per-particle pressure acceleration to velocity.
//
// vel_i += dt * pressure_accel[gid].xyz
//
// Dispatched once per inner-loop after pressure_accel has been resolved (DFSPH
// integrity-allow: cat1.upstream-citation; pre-v1 SPlisHSPlasH 1.8.10 anchor in live code (migration target tracked in grandfather-catalog live-shader-1810); n/a
// density / divergence loops). Mirrors SPlisHSPlasH 1.8.10 TimeStepDFSPH.cpp:514-515
// (divergence) / :359-360 (density), where the velocity correction is the only
// per-particle write after the inner iteration converges.
#version 460

layout(local_size_x = 256) in;

layout(set=0, binding=0, std430) restrict buffer Particles { vec4 p[]; };
layout(set=0, binding=1, std430) restrict readonly buffer PressureAccel { vec4 pressure_accel[]; };
layout(set=0, binding=2, std140) uniform U {
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

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= particleCount) return;
    vec3 vel_i   = p[gid * 8u + 1u].xyz;
    vec3 a_press = pressure_accel[gid].xyz;
    p[gid * 8u + 1u].xyz = vel_i + dt * a_press;
}
