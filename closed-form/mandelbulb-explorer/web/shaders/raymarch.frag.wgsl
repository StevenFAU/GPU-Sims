// Raymarch fragment shader for Mandelbulb Explorer.
//
// Math: Daniel White (2009) / Paul Nylander mandelbulb formulation.
// References: see ../shaders/../shadertoy/README.md for the full citation
// list. No code lifted.

struct Uniforms {
    // Camera (16-byte aligned). w-component carries auxiliary scalars to
    // keep the struct dense in std140-equivalent layout.
    camPos:        vec4<f32>,    // xyz = position, w = fovTan (tan(fov/2))
    camRight:      vec4<f32>,    // xyz = right basis vector
    camUp:         vec4<f32>,    // xyz = up basis vector
    camForward:    vec4<f32>,    // xyz = forward basis vector

    // DE / raymarch params
    deParams:      vec4<f32>,    // x = nPower, y = bailout, z = epsilonBase, w = epsilonGrow
    marchParams:   vec4<f32>,    // x = maxRayDist, y = iterCap (as f32), z = maxSteps (as f32), w = unused

    // Lighting / shadow
    lightDir:      vec4<f32>,    // xyz = normalized light direction, w = softShadowK
    lightFlags:    vec4<f32>,    // x = softShadowsEnabled (0/1), y = ambientStrength, z = unused, w = unused

    // Coloring
    colorHot:      vec4<f32>,    // xyz = hot color, w = trapNormalize (1 / trapRadius^2)
    colorCool:     vec4<f32>,    // xyz = cool color, w = orbitTrapMode (0=point, 1=planes, 2=mixed)
    bgColor:       vec4<f32>,    // xyz = background color, w = unused

    // Output sizing (the offscreen RT we render into).
    output:        vec4<f32>,    // xy = output size, zw = unused
};

@group(0) @binding(0) var<uniform> u: Uniforms;

// Mandelbulb distance estimator. Returns vec4(distance, orbitPoint, orbitPlanes, 0).
// orbitPoint  = squared minimum |z| during iteration (point-at-origin trap).
// orbitPlanes = squared minimum component of |z| (axis-plane trap).
fn mandelbulbDE(c: vec3<f32>) -> vec4<f32> {
    var z: vec3<f32> = c;
    var dr: f32 = 1.0;
    var r:  f32 = 0.0;
    var trapPoint:  f32 = 1e10;
    var trapPlanes: f32 = 1e10;

    let nPower:  f32 = u.deParams.x;
    let bailout: f32 = u.deParams.y;
    let iterCap: i32 = i32(u.marchParams.y);

    for (var i: i32 = 0; i < iterCap; i = i + 1) {
        r = length(z);
        if (r > bailout) { break; }

        let zsq = z * z;
        trapPoint  = min(trapPoint,  zsq.x + zsq.y + zsq.z);
        trapPlanes = min(trapPlanes, min(zsq.x, min(zsq.y, zsq.z)));

        let theta = atan2(sqrt(zsq.x + zsq.y), z.z);
        let phi   = atan2(z.y, z.x);
        let zr    = pow(r, nPower);
        dr        = pow(r, nPower - 1.0) * nPower * dr + 1.0;
        let st    = sin(nPower * theta);
        let ct    = cos(nPower * theta);
        let sp    = sin(nPower * phi);
        let cp    = cos(nPower * phi);
        z = zr * vec3<f32>(st * cp, st * sp, ct) + c;
    }

    let dist = 0.5 * log(max(r, 1e-10)) * r / max(dr, 1e-10);
    return vec4<f32>(dist, trapPoint, trapPlanes, 0.0);
}

// Finite-difference normal estimate around point p.
fn estimateNormal(p: vec3<f32>) -> vec3<f32> {
    let h: f32 = 0.001;
    let k = vec2<f32>(1.0, -1.0);
    return normalize(
        k.xyy * mandelbulbDE(p + k.xyy * h).x +
        k.yyx * mandelbulbDE(p + k.yyx * h).x +
        k.yxy * mandelbulbDE(p + k.yxy * h).x +
        k.xxx * mandelbulbDE(p + k.xxx * h).x
    );
}

// Cone-traced soft shadow toward u.lightDir. Returns 0..1 occlusion.
fn softShadow(ro: vec3<f32>, rd: vec3<f32>) -> f32 {
    var result: f32 = 1.0;
    var t: f32 = 0.02;
    let k: f32 = u.lightDir.w;  // softShadowK
    for (var i: i32 = 0; i < 32; i = i + 1) {
        if (t > 4.0) { break; }
        let d = mandelbulbDE(ro + rd * t).x;
        if (d < 0.0001) { return 0.0; }
        result = min(result, k * d / t);
        t = t + d;
    }
    return clamp(result, 0.0, 1.0);
}

@fragment
fn fs_main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
    // uv is 0..1 across the visible region, with uv.y = 0 at canvas TOP
    // (matches WebGPU's framebuffer-y origin / texture-sample convention).
    // Convert to NDC-style [-1, 1] with aspect correction; flip y so
    // ndc.y = +1 at top, matching the standard graphics-math screen-coord
    // convention that the ray-construction math expects.
    let aspect = u.output.x / max(u.output.y, 1.0);
    let ndc = vec2<f32>((uv.x * 2.0 - 1.0) * aspect, 1.0 - uv.y * 2.0);

    // Build ray direction from camera basis. fovTan = tan(fov/2).
    let fovTan = u.camPos.w;
    let dir = normalize(
        u.camRight.xyz * (ndc.x * fovTan) +
        u.camUp.xyz    * (ndc.y * fovTan) +
        u.camForward.xyz
    );
    let origin = u.camPos.xyz;

    // March.
    let maxRayDist:  f32 = u.marchParams.x;
    let maxSteps:    i32 = i32(u.marchParams.z);
    let epsilonBase: f32 = u.deParams.z;
    let epsilonGrow: f32 = u.deParams.w;

    var t: f32 = 0.0;
    var hit: bool = false;
    var trapPoint:  f32 = 1e10;
    var trapPlanes: f32 = 1e10;

    for (var i: i32 = 0; i < maxSteps; i = i + 1) {
        let p = origin + dir * t;
        let r = mandelbulbDE(p);
        let d = r.x;
        trapPoint  = min(trapPoint,  r.y);
        trapPlanes = min(trapPlanes, r.z);
        let eps = epsilonBase * (1.0 + epsilonGrow * t * 1000.0);
        if (d < eps) { hit = true; break; }
        t = t + d;
        if (t > maxRayDist) { break; }
    }

    var col: vec3<f32>;
    if (hit) {
        let p = origin + dir * t;
        let n = estimateNormal(p);
        let diff = max(0.0, dot(n, u.lightDir.xyz));
        var sh: f32 = 1.0;
        if (u.lightFlags.x > 0.5) {
            sh = softShadow(p + n * 0.002, u.lightDir.xyz);
        }

        // Orbit-trap coloring. Mode select between point-at-origin / planes / mixed.
        let mode = u.colorCool.w;
        let trapNorm = u.colorHot.w;
        let kPt   = clamp(sqrt(trapPoint)  * trapNorm * 1.4, 0.0, 1.0);
        let kPl   = clamp(sqrt(trapPlanes) * trapNorm * 2.0, 0.0, 1.0);
        var k: f32;
        if (mode < 0.5) {
            k = kPt;
        } else if (mode < 1.5) {
            k = kPl;
        } else {
            k = mix(kPt, kPl, 0.4);
        }

        let base = mix(u.colorHot.xyz, u.colorCool.xyz, k);
        let amb  = u.lightFlags.y;
        col = base * (amb + diff * sh);
    } else {
        col = u.bgColor.xyz;
    }

    return vec4<f32>(col, 1.0);
}
