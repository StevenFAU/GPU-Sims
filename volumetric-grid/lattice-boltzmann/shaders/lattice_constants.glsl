// Auto-derived from tools/integrity/docs/algebraic/d3q19.md § 2.2.
// DO NOT EDIT BY HAND. The 19 velocity vectors and 3 weights are pinned in
// the algebraic ground-truth derivation; this file's job is just to lift them
// into a form GLSL can use.
//
// cat3.upstream-citation: tools/integrity/docs/algebraic/d3q19.md § 2.2 (velocity set + weights)
// cat3.upstream-citation: tools/integrity/docs/algebraic/d3q19.md § 3   (sound speed)
// cat3.upstream-citation: tools/integrity/docs/algebraic/d3q19.md § 4.1 (equilibrium)
//
// Direction ordering (per d3q19.md § 2.2):
//   i = 0      : rest                         (0, 0, 0)
//   i = 1..6   : face neighbors  (+x,-x,+y,-y,+z,-z)
//   i = 7..10  : edge neighbors in xy-plane   (xy++, xy+-, xy-+, xy--)
//   i = 11..14 : edge neighbors in xz-plane   (xz++, xz+-, xz-+, xz--)
//   i = 15..18 : edge neighbors in yz-plane   (yz++, yz+-, yz-+, yz--)

#ifndef LATTICE_CONSTANTS_GLSL_INCLUDED
#define LATTICE_CONSTANTS_GLSL_INCLUDED

const int NUM_DIRS = 19;

// Velocity vectors. ivec3 stored in a constant array.
const ivec3 C_I[19] = ivec3[19](
    ivec3( 0,  0,  0),   // i=0  rest
    ivec3( 1,  0,  0),   // i=1  face +x
    ivec3(-1,  0,  0),   // i=2  face -x
    ivec3( 0,  1,  0),   // i=3  face +y
    ivec3( 0, -1,  0),   // i=4  face -y
    ivec3( 0,  0,  1),   // i=5  face +z
    ivec3( 0,  0, -1),   // i=6  face -z
    ivec3( 1,  1,  0),   // i=7  edge xy++
    ivec3( 1, -1,  0),   // i=8  edge xy+-
    ivec3(-1,  1,  0),   // i=9  edge xy-+
    ivec3(-1, -1,  0),   // i=10 edge xy--
    ivec3( 1,  0,  1),   // i=11 edge xz++
    ivec3( 1,  0, -1),   // i=12 edge xz+-
    ivec3(-1,  0,  1),   // i=13 edge xz-+
    ivec3(-1,  0, -1),   // i=14 edge xz--
    ivec3( 0,  1,  1),   // i=15 edge yz++
    ivec3( 0,  1, -1),   // i=16 edge yz+-
    ivec3( 0, -1,  1),   // i=17 edge yz-+
    ivec3( 0, -1, -1)    // i=18 edge yz--
);

// Weights. Three values: w_rest = 1/3, w_face = 1/18, w_edge = 1/36.
const float W_I[19] = float[19](
    1.0/3.0,                                                     // i=0
    1.0/18.0, 1.0/18.0, 1.0/18.0, 1.0/18.0, 1.0/18.0, 1.0/18.0,  // i=1..6
    1.0/36.0, 1.0/36.0, 1.0/36.0, 1.0/36.0,                       // i=7..10
    1.0/36.0, 1.0/36.0, 1.0/36.0, 1.0/36.0,                       // i=11..14
    1.0/36.0, 1.0/36.0, 1.0/36.0, 1.0/36.0                        // i=15..18
);

// Opposite-direction table (the involution c_i <-> -c_i, per d3q19.md § 2.2):
const int OPPOSITE_DIR[19] = int[19](
     0,  // i=0
     2,  // i=1  opp(+x) = -x = 2
     1,  // i=2  opp(-x) = +x = 1
     4,  // i=3  opp(+y) = -y = 4
     3,  // i=4
     6,  // i=5
     5,  // i=6
    10,  // i=7  opp(xy++) = xy-- = 10
     9,  // i=8  opp(xy+-) = xy-+ = 9
     8,  // i=9
     7,  // i=10
    14,  // i=11 opp(xz++) = xz-- = 14
    13,  // i=12 opp(xz+-) = xz-+ = 13
    12,  // i=13
    11,  // i=14
    18,  // i=15 opp(yz++) = yz-- = 18
    17,  // i=16
    16,  // i=17
    15   // i=18
);

// Lattice sound-speed squared. c_s^2 = 1/3 per d3q19.md § 3.
const float CS2          = 1.0 / 3.0;
const float CS2_INV      = 3.0;     // 1 / c_s^2
const float CS2_HALF_INV = 1.5;     // 1 / (2 c_s^2)
const float CS4_HALF_INV = 4.5;     // 1 / (2 c_s^4)

// Compute equilibrium f_i for a given (rho, u). Compressible Maxwell-Boltzmann:
//   feq_i = w_i * rho * [1 + 3*(c.u) + 4.5*(c.u)^2 - 1.5*(u.u)]
// Reference: tools/integrity/docs/algebraic/d3q19.md § 4.1.
float feq_i(int i, float rho, vec3 u) {
    vec3  ci    = vec3(C_I[i]);
    float cdotu = dot(ci, u);
    float udotu = dot(u, u);
    return W_I[i] * rho * (1.0 + CS2_INV * cdotu
                               + CS4_HALF_INV * cdotu * cdotu
                               - CS2_HALF_INV * udotu);
}

#endif  // LATTICE_CONSTANTS_GLSL_INCLUDED
