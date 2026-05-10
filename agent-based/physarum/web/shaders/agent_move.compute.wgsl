// Multi-species Physarum agent kernel.
// Each agent: senses three points ahead, steers per Jones rule, moves, deposits.
// Per-species sensing weights cross-species trails by -repulsionStrength.

struct Params {
    gridSize:        u32,
    agentCount:      u32,
    iteration:       u32,
    depositScale:    u32,
    senseDistance:   f32,
    senseAngle:      f32,
    turnAngle:       f32,
    stepSize:        f32,
    decayRate:       f32,
    diffuseWeight:   f32,
    depositAmount:   f32,
    repulsionStrength: f32,
    simSpeed:        f32,
    pinCount:        u32,
    pinIntensity:    f32,
    pinRadius:       f32,
}

struct Agent {
    pos:     vec2<f32>,
    heading: f32,
    species: u32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read_write> agents: array<Agent>;
@group(0) @binding(2) var trailPrev: texture_2d<f32>;
@group(0) @binding(3) var trailSampler: sampler;
@group(0) @binding(4) var<storage, read_write> deposits0: array<atomic<u32>>;
@group(0) @binding(5) var<storage, read_write> deposits1: array<atomic<u32>>;
@group(0) @binding(6) var<storage, read_write> deposits2: array<atomic<u32>>;

// xorshift32 — deterministic per-(state) sequence.
fn xorshift32(state: ptr<function, u32>) -> u32 {
    var s = *state;
    s = s ^ (s << 13u);
    s = s ^ (s >> 17u);
    s = s ^ (s << 5u);
    *state = s;
    return s;
}

fn random01(state: ptr<function, u32>) -> f32 {
    return f32(xorshift32(state)) / 4294967296.0;
}

// Sample trail at a point in cell coordinates [0, gridSize].
// Per-species weighted: own species positive, others negative (repulsion).
fn senseAt(point: vec2<f32>, species: u32) -> f32 {
    let uv = point / vec2<f32>(f32(params.gridSize));
    let trail = textureSampleLevel(trailPrev, trailSampler, uv, 0.0).rgb;
    let lambda = params.repulsionStrength;
    switch (species) {
        case 0u: { return trail.r - lambda * (trail.g + trail.b); }
        case 1u: { return trail.g - lambda * (trail.r + trail.b); }
        default: { return trail.b - lambda * (trail.r + trail.g); }
    }
}

// Workgroup size 256 is load-bearing: at 64-wide, the 4M agent tier needs
// 4_194_304 / 64 = 65,536 workgroups in X — exceeds baseline
// maxComputeWorkgroupsPerDimension (65,535) by one. 256 brings 4M to 16,384
// and 10M to 39,063, both well inside the cap.
@compute @workgroup_size(256, 1, 1)
fn cs_main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let i = gid.x;
    if (i >= params.agentCount) { return; }

    var agent = agents[i];

    // Per-frame, per-agent RNG state from iteration count + agent index.
    var rng = (params.iteration * 2654435761u) ^ (i * 1597334677u);
    if (rng == 0u) { rng = 0xC0FFEEu; }

    let speed = params.stepSize * params.simSpeed;
    let turn  = params.turnAngle * params.simSpeed;
    let cosH  = cos(agent.heading);
    let sinH  = sin(agent.heading);

    // Sample three points: forward (F), left (L), right (R).
    let forward = agent.pos + params.senseDistance * vec2<f32>(cosH, sinH);
    let leftA   = agent.heading + params.senseAngle;
    let rightA  = agent.heading - params.senseAngle;
    let leftP   = agent.pos + params.senseDistance * vec2<f32>(cos(leftA),  sin(leftA));
    let rightP  = agent.pos + params.senseDistance * vec2<f32>(cos(rightA), sin(rightA));

    let F = senseAt(forward, agent.species);
    let L = senseAt(leftP,   agent.species);
    let R = senseAt(rightP,  agent.species);

    // Jones rule: steer toward strongest sensed gradient.
    if (F > L && F > R) {
        // continue forward
    } else if (F < L && F < R) {
        if (random01(&rng) > 0.5) {
            agent.heading = agent.heading + turn;
        } else {
            agent.heading = agent.heading - turn;
        }
    } else if (L > R) {
        agent.heading = agent.heading + turn;
    } else if (R > L) {
        agent.heading = agent.heading - turn;
    }

    // Heading jitter (~0.05 rad/step) to break symmetry.
    let jitter = (random01(&rng) - 0.5) * 0.1;
    agent.heading = agent.heading + jitter;

    // Move.
    let h = agent.heading;
    agent.pos = agent.pos + speed * vec2<f32>(cos(h), sin(h));

    // Wrap (periodic boundary).
    let g = f32(params.gridSize);
    agent.pos.x = agent.pos.x - g * floor(agent.pos.x / g);
    agent.pos.y = agent.pos.y - g * floor(agent.pos.y / g);

    // Deposit at floor(pos).
    let cellX = u32(clamp(floor(agent.pos.x), 0.0, g - 1.0));
    let cellY = u32(clamp(floor(agent.pos.y), 0.0, g - 1.0));
    let idx = cellY * params.gridSize + cellX;
    let scaled = u32(round(params.depositAmount * f32(params.depositScale)));
    switch (agent.species) {
        case 0u: { atomicAdd(&deposits0[idx], scaled); }
        case 1u: { atomicAdd(&deposits1[idx], scaled); }
        default: { atomicAdd(&deposits2[idx], scaled); }
    }

    agents[i] = agent;
}
