// Mandelbulb Explorer — Stack A (Shadertoy) reference implementation.
// Part of GPU-Sims (https://github.com/StevenFAU/GPU-Sims).
// MIT licensed. Author: Steven Cohen.
//
// Math: Daniel White (2009) / Paul Nylander mandelbulb formulation.
// References consulted (no code lifted):
//   - Daniel White, "The Mandelbulb: First 'true' 3D image of the famous fractal" (2009).
//   - Paul Nylander, https://www.bugman123.com/Hypercomplex/
//   - Inigo Quilez, "Distance estimation" (general DE-raymarching technique).
//
// Controls: hold left mouse and drag to orbit. Click without dragging: zoom in/out.
//           No keyboard input — Shadertoy doesn't expose one cleanly.
//
// To use: copy this file's contents into a new Shadertoy (https://shadertoy.com/new),
//         paste into the Image tab, save. Runs at full Shadertoy resolution.

// ---------- Tunables (mirror the Stack B defaults) ----------
const float N_POWER       = 8.0;     // mandelbulb power exponent (canonical = 8)
const int   ITER_CAP      = 8;       // DE iteration cap
const float BAILOUT       = 2.0;     // escape radius
const int   MAX_STEPS     = 96;      // raymarch step cap
const float EPSILON_BASE  = 0.001;   // surface-hit threshold at t=0
const float EPSILON_GROW  = 0.001;   // anti-aliases distant detail
const float MAX_RAY_DIST  = 8.0;     // bounding distance
const float SOFT_SHADOW_K = 16.0;    // higher = harder shadow edge
const vec3  LIGHT_DIR     = normalize(vec3(0.5, 0.7, 0.5));
const vec3  COLOR_HOT     = vec3(1.00, 0.38, 0.25);  // warm tint
const vec3  COLOR_COOL    = vec3(0.13, 0.31, 0.63);  // cool tint
const vec3  BG_COLOR      = vec3(0.02, 0.02, 0.04);
const float EXPOSURE      = 1.0;

// ---------- Mandelbulb DE ----------
// Returns vec4(distance, orbit_trap_sq_min, _, _)  — orbit-trap is the squared
// minimum |z| during iteration, which the shading step uses for coloring.
vec4 mandelbulbDE(vec3 c) {
    vec3 z = c;
    float dr = 1.0;
    float r = 0.0;
    float orbitTrap = 1e10;

    for (int i = 0; i < ITER_CAP; i++) {
        r = length(z);
        if (r > BAILOUT) break;

        // Track closest approach to origin (point-at-origin orbit trap).
        orbitTrap = min(orbitTrap, dot(z, z));

        float theta = atan(sqrt(z.x*z.x + z.y*z.y), z.z);   // polar angle from +Z
        float phi   = atan(z.y, z.x);                       // azimuth in XY
        float zr    = pow(r, N_POWER);
        dr          = pow(r, N_POWER - 1.0) * N_POWER * dr + 1.0;
        float st    = sin(N_POWER * theta);
        float ct    = cos(N_POWER * theta);
        float sp    = sin(N_POWER * phi);
        float cp    = cos(N_POWER * phi);
        z = zr * vec3(st * cp, st * sp, ct) + c;
    }

    float dist = 0.5 * log(max(r, 1e-10)) * r / dr;
    return vec4(dist, orbitTrap, 0.0, 0.0);
}

// Cheap finite-difference normal at a surface point.
vec3 estimateNormal(vec3 p) {
    const float h = 0.001;
    vec2 k = vec2(1.0, -1.0);
    return normalize(
        k.xyy * mandelbulbDE(p + k.xyy * h).x +
        k.yyx * mandelbulbDE(p + k.yyx * h).x +
        k.yxy * mandelbulbDE(p + k.yxy * h).x +
        k.xxx * mandelbulbDE(p + k.xxx * h).x
    );
}

// Cone-traced soft shadow toward LIGHT_DIR.
float softShadow(vec3 ro, vec3 rd) {
    float result = 1.0;
    float t = 0.02;  // start a bit off the surface
    for (int i = 0; i < 32; i++) {
        if (t > 4.0) break;
        float d = mandelbulbDE(ro + rd * t).x;
        if (d < 0.0001) return 0.0;
        result = min(result, SOFT_SHADOW_K * d / t);
        t += d;
    }
    return clamp(result, 0.0, 1.0);
}

// Camera basis from yaw/pitch around target=origin at given orbit radius.
mat3 cameraFrame(float yaw, float pitch) {
    vec3 fwd = normalize(vec3(
        cos(pitch) * sin(yaw),
        sin(pitch),
        cos(pitch) * cos(yaw)
    ));
    vec3 right = normalize(cross(fwd, vec3(0.0, 1.0, 0.0)));
    vec3 up    = cross(right, fwd);
    return mat3(right, up, fwd);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

    // Mouse drives camera orbit when LMB held; otherwise default slow auto-orbit.
    float yaw, pitch;
    if (iMouse.z > 0.0) {
        yaw   =  ((iMouse.x / iResolution.x) - 0.5) * 6.28318;
        pitch = -((iMouse.y / iResolution.y) - 0.5) * 1.5;
    } else {
        yaw   = 0.4 * iTime;
        pitch = 0.35;
    }
    pitch = clamp(pitch, -1.4, 1.4);

    float orbitRadius = 2.7;
    vec3 target = vec3(0.0);
    vec3 camPos = target - cameraFrame(yaw, pitch)[2] * orbitRadius;
    mat3 frame  = cameraFrame(yaw, pitch);
    vec3 rd     = normalize(frame * vec3(uv.x, uv.y, 1.2));   // 1.2 ~= cot(half-fov)

    // March.
    float t = 0.0;
    bool  hit = false;
    float trap = 1e10;
    for (int i = 0; i < MAX_STEPS; i++) {
        vec3 p = camPos + rd * t;
        vec4 r = mandelbulbDE(p);
        float d = r.x;
        trap = min(trap, r.y);
        float eps = EPSILON_BASE * (1.0 + EPSILON_GROW * t * 1000.0);
        if (d < eps) { hit = true; break; }
        t += d;
        if (t > MAX_RAY_DIST) break;
    }

    vec3 col;
    if (hit) {
        vec3 p = camPos + rd * t;
        vec3 n = estimateNormal(p);
        float diff = max(0.0, dot(n, LIGHT_DIR));
        float sh   = softShadow(p + n * 0.002, LIGHT_DIR);
        // Color by orbit-trap distance: closer to origin = hot, farther = cool.
        float k = clamp(sqrt(trap) * 1.4, 0.0, 1.0);
        vec3 base = mix(COLOR_HOT, COLOR_COOL, k);
        col = base * (0.05 + diff * sh) + 0.05 * base;  // ambient + lit
    } else {
        col = BG_COLOR;
    }

    // Reinhard tonemap + exposure.
    vec3 exposed = col * EXPOSURE;
    vec3 mapped  = exposed / (vec3(1.0) + exposed);
    fragColor = vec4(mapped, 1.0);
}
