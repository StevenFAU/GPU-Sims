import { ParticleFrame } from '../../../../common/common-web/src/index.js';

export function useFrame(): number {
    const f = new ParticleFrame(10);
    return f.positions.length + f.velocities.length + f.ids.length;
    // f.radii is intentionally never read.
    // unusedFunction is intentionally never imported or called.
}
