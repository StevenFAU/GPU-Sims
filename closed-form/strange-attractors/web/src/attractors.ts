// Per-attractor definitions: parameters, defaults, initial-condition spawn boxes,
// recommended camera orbit, and default substep count.

export type AttractorId = 'lorenz' | 'aizawa' | 'thomas';

export interface AttractorParam {
    name: string;
    label: string;
    min: number;
    max: number;
    step: number;
    default: number;
}

export interface AttractorDef {
    id: AttractorId;
    index: number;                   // matches the u32 attractorId in the WGSL switch
    label: string;
    params: AttractorParam[];        // in WGSL p0..p5 order
    initBox: { min: [number, number, number]; max: [number, number, number] };
    orbitCenter: [number, number, number];
    orbitRadius: number;
    defaultSubsteps: number;
    defaultSimDt: number;
    defaultPointSize: number;
    defaultColorSpeedScale: number;
}

export const ATTRACTORS: Record<AttractorId, AttractorDef> = {
    lorenz: {
        id: 'lorenz',
        index: 0,
        label: 'Lorenz',
        params: [
            { name: 'sigma', label: 'σ', min: 0.1, max: 30, step: 0.01, default: 10 },
            { name: 'rho',   label: 'ρ', min: 0.1, max: 60, step: 0.01, default: 28 },
            { name: 'beta',  label: 'β', min: 0.1, max: 10, step: 0.01, default: 8 / 3 },
        ],
        initBox: { min: [-15, -20, 0], max: [15, 20, 40] },
        orbitCenter: [0, 0, 25],
        orbitRadius: 60,
        defaultSubsteps: 8,
        defaultSimDt: 0.005,
        defaultPointSize: 1.6,
        defaultColorSpeedScale: 60,
    },
    aizawa: {
        id: 'aizawa',
        index: 1,
        label: 'Aizawa',
        params: [
            { name: 'a', label: 'a', min: 0.1, max: 2,    step: 0.001, default: 0.95 },
            { name: 'b', label: 'b', min: 0.1, max: 2,    step: 0.001, default: 0.7 },
            { name: 'c', label: 'c', min: 0.1, max: 2,    step: 0.001, default: 0.6 },
            { name: 'd', label: 'd', min: 0.1, max: 6,    step: 0.001, default: 3.5 },
            { name: 'e', label: 'e', min: 0,   max: 2,    step: 0.001, default: 0.25 },
            { name: 'f', label: 'f', min: 0,   max: 1,    step: 0.001, default: 0.1 },
        ],
        initBox: { min: [-1.5, -1.5, -1.5], max: [1.5, 1.5, 1.5] },
        orbitCenter: [0, 0, 0],
        orbitRadius: 3,
        defaultSubsteps: 16,
        defaultSimDt: 0.005,
        defaultPointSize: 1.6,
        defaultColorSpeedScale: 4,
    },
    thomas: {
        id: 'thomas',
        index: 2,
        label: 'Thomas',
        params: [
            { name: 'b', label: 'b', min: 0.05, max: 1, step: 0.0001, default: 0.208186 },
        ],
        initBox: { min: [-4, -4, -4], max: [4, 4, 4] },
        orbitCenter: [0, 0, 0],
        orbitRadius: 8,
        defaultSubsteps: 16,
        defaultSimDt: 0.01,
        defaultPointSize: 1.6,
        defaultColorSpeedScale: 1.5,
    },
};

export const ATTRACTOR_ORDER: AttractorId[] = ['lorenz', 'aizawa', 'thomas'];

/** Pack 6 params into a fixed-length array for the WGSL uniform (defaults to 0 for missing). */
export function packParams(def: AttractorDef): [number, number, number, number, number, number] {
    const out: [number, number, number, number, number, number] = [0, 0, 0, 0, 0, 0];
    for (let i = 0; i < def.params.length && i < 6; i++) {
        out[i] = def.params[i]!.default;
    }
    return out;
}

/** Deterministic xorshift32 — used to seed initial particle positions reproducibly. */
export class Xorshift32 {
    private state: number;
    constructor(seed: number) {
        // Avoid zero — xorshift on zero is degenerate.
        this.state = (seed | 0) || 0x9e3779b9;
    }
    next(): number {
        let x = this.state;
        x ^= x << 13;
        x ^= x >>> 17;
        x ^= x << 5;
        this.state = x | 0;
        // Map to [0, 1).
        return ((x >>> 0) / 4294967296);
    }
}

/**
 * Fill a Float32Array (size = 4 * particleCount) with initial positions
 * uniformly inside the attractor's spawn box. The .w channel is initialized
 * to zero; the integrate shader fills it with speed on first dispatch.
 */
export function seedParticles(
    target: Float32Array,
    def: AttractorDef,
    count: number,
    seed: number,
): void {
    if (target.length < count * 4) {
        throw new Error(`seedParticles: buffer too small (need ${count * 4}, got ${target.length})`);
    }
    const rng = new Xorshift32(seed);
    const [minX, minY, minZ] = def.initBox.min;
    const [maxX, maxY, maxZ] = def.initBox.max;
    const dx = maxX - minX;
    const dy = maxY - minY;
    const dz = maxZ - minZ;
    for (let i = 0; i < count; i++) {
        const j = i * 4;
        target[j + 0] = minX + rng.next() * dx;
        target[j + 1] = minY + rng.next() * dy;
        target[j + 2] = minZ + rng.next() * dz;
        target[j + 3] = 0;
    }
}
