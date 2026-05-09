// RK4 integration of the selected attractor system.
// One thread per particle. Reads & writes positionsBuffer in place.
// Substep count is in the uniform; integrator runs N substeps of size simDt
// per dispatch.

struct SimUniforms {
    simDt: f32,
    substeps: u32,
    attractorId: u32,    // 0 = Lorenz, 1 = Aizawa, 2 = Thomas
    particleCount: u32,
    // attractor parameters — meaning depends on attractorId
    p0: f32, p1: f32, p2: f32, p3: f32,
    p4: f32, p5: f32, _pad0: f32, _pad1: f32,
};

@group(0) @binding(0) var<storage, read_write> positions: array<vec4<f32>>;
@group(0) @binding(1) var<uniform> sim: SimUniforms;

// ---------- attractor velocity functions ----------
// Each returns dState/dt at the given state point.

fn lorenz(s: vec3<f32>) -> vec3<f32> {
    // p0 = sigma, p1 = rho, p2 = beta
    let sigma = sim.p0;
    let rho = sim.p1;
    let beta = sim.p2;
    return vec3<f32>(
        sigma * (s.y - s.x),
        s.x * (rho - s.z) - s.y,
        s.x * s.y - beta * s.z
    );
}

fn aizawa(s: vec3<f32>) -> vec3<f32> {
    // p0 = a, p1 = b, p2 = c, p3 = d, p4 = e, p5 = f
    let a = sim.p0;
    let b = sim.p1;
    let c = sim.p2;
    let d = sim.p3;
    let e = sim.p4;
    let f = sim.p5;
    let r2 = s.x * s.x + s.y * s.y;
    return vec3<f32>(
        (s.z - b) * s.x - d * s.y,
        d * s.x + (s.z - b) * s.y,
        c + a * s.z - (s.z * s.z * s.z) / 3.0 - r2 * (1.0 + e * s.z) + f * s.z * s.x * s.x * s.x
    );
}

fn thomas(s: vec3<f32>) -> vec3<f32> {
    // p0 = b
    let b = sim.p0;
    return vec3<f32>(
        sin(s.y) - b * s.x,
        sin(s.z) - b * s.y,
        sin(s.x) - b * s.z
    );
}

fn velocity(s: vec3<f32>) -> vec3<f32> {
    switch sim.attractorId {
        case 0u: { return lorenz(s); }
        case 1u: { return aizawa(s); }
        case 2u: { return thomas(s); }
        default: { return vec3<f32>(0.0); }
    }
}

// ---------- RK4 single-step ----------

fn rk4_step(s: vec3<f32>, dt: f32) -> vec3<f32> {
    let k1 = velocity(s);
    let k2 = velocity(s + 0.5 * dt * k1);
    let k3 = velocity(s + 0.5 * dt * k2);
    let k4 = velocity(s + dt * k3);
    return s + (dt / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
}

// ---------- main ----------

@compute @workgroup_size(64)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let i = gid.x;
    if (i >= sim.particleCount) {
        return;
    }

    var s = positions[i].xyz;
    let dt = sim.simDt;

    for (var step: u32 = 0u; step < sim.substeps; step = step + 1u) {
        s = rk4_step(s, dt);
    }

    // Final velocity for color (post-integration speed magnitude).
    let v = velocity(s);
    let speed = length(v);

    positions[i] = vec4<f32>(s, speed);
}
