// Six Jones-1993-/Lague-style named presets covering the parameter space.
// Values are informed starting points, not literature-canonical.
// Adjust one row of this file post-build if a preset doesn't show its
// named pattern type at the default agent count + grid size.

export type PresetName = 'Networks' | 'Snowflake' | 'Highways' | 'Conflict' | 'Cooperation' | 'Chaos';

export interface PhysarumPreset {
    senseDistance:     number;   // cells
    senseAngle:        number;   // radians
    turnAngle:         number;   // radians
    stepSize:          number;   // cells / frame
    decayRate:         number;   // per frame, multiplicative
    depositAmount:     number;   // per agent per frame, raw f32 (×depositScale internally)
    repulsionStrength: number;   // λ in sense(point, species) = self - λ × (other1 + other2)
}

export const PRESETS: Record<PresetName, PhysarumPreset> = {
    Networks:    { senseDistance:  9.0, senseAngle: Math.PI / 4, turnAngle: Math.PI / 4, stepSize: 1.0, decayRate: 0.030, depositAmount: 5.0, repulsionStrength: 1.0 },
    Snowflake:   { senseDistance:  3.0, senseAngle: Math.PI / 3, turnAngle: Math.PI / 3, stepSize: 0.5, decayRate: 0.100, depositAmount: 8.0, repulsionStrength: 0.5 },
    Highways:    { senseDistance: 15.0, senseAngle: Math.PI / 6, turnAngle: Math.PI / 8, stepSize: 2.0, decayRate: 0.015, depositAmount: 3.0, repulsionStrength: 1.5 },
    Conflict:    { senseDistance:  9.0, senseAngle: Math.PI / 4, turnAngle: Math.PI / 4, stepSize: 1.0, decayRate: 0.030, depositAmount: 5.0, repulsionStrength: 3.0 },
    Cooperation: { senseDistance:  9.0, senseAngle: Math.PI / 4, turnAngle: Math.PI / 4, stepSize: 1.0, decayRate: 0.030, depositAmount: 5.0, repulsionStrength: 0.0 },
    Chaos:       { senseDistance:  5.0, senseAngle: Math.PI / 2, turnAngle: Math.PI / 2, stepSize: 1.5, decayRate: 0.050, depositAmount: 4.0, repulsionStrength: 0.5 },
};
