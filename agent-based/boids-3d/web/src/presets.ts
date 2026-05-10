// Six parameter regimes for boids-3d. Each preset sets Reynolds-rule weights,
// leader-attraction strength, and predator parameters. Visualization
// parameters (colors, scale) are not preset-driven — they're independent
// panel state that persists across preset changes.

export interface Preset {
    // Reynolds rules
    separationRadius: number;
    separationWeight: number;
    alignmentRadius:  number;
    alignmentWeight:  number;
    cohesionRadius:   number;
    cohesionWeight:   number;
    boidMaxSpeed:     number;

    // Leaders
    leaderInfluenceRadius: number;
    leaderStrength:        number;

    // Predators
    predatorMode:            'nearest-prey' | 'stochastic-prey' | 'flock-center';
    predatorFleeRadius:      number;
    predatorFleeStrength:    number;
    predatorDetectionRadius: number;
    predatorRePickFrames:    number;   // used by stochastic-prey mode only
    predatorSpeedMul:        number;
}

export const PRESETS = {
    'Cohesive Flock': {
        separationRadius: 0.5, separationWeight: 1.5,
        alignmentRadius:  0.7, alignmentWeight:  1.0,
        cohesionRadius:   0.8, cohesionWeight:   1.0,
        boidMaxSpeed:     2.5,
        leaderInfluenceRadius: 3.0, leaderStrength: 1.2,
        predatorMode: 'nearest-prey' as const,
        predatorFleeRadius:      1.0,
        predatorFleeStrength:    3.0,
        predatorDetectionRadius: 1.5,
        predatorRePickFrames:    90,
        predatorSpeedMul:        1.4,
    },
    'Loose Murmuration': {
        separationRadius: 0.3, separationWeight: 0.9,
        alignmentRadius:  1.2, alignmentWeight:  1.4,
        cohesionRadius:   1.5, cohesionWeight:   0.6,
        boidMaxSpeed:     2.2,
        leaderInfluenceRadius: 3.5, leaderStrength: 0.7,
        predatorMode: 'flock-center' as const,
        predatorFleeRadius:      1.2,
        predatorFleeStrength:    2.0,
        predatorDetectionRadius: 2.0,
        predatorRePickFrames:    120,
        predatorSpeedMul:        1.2,
    },
    'Tight Schooling': {
        separationRadius: 0.6, separationWeight: 2.5,
        alignmentRadius:  0.5, alignmentWeight:  1.8,
        cohesionRadius:   0.6, cohesionWeight:   1.6,
        boidMaxSpeed:     2.8,
        leaderInfluenceRadius: 2.5, leaderStrength: 1.5,
        predatorMode: 'nearest-prey' as const,
        predatorFleeRadius:      0.9,
        predatorFleeStrength:    4.0,
        predatorDetectionRadius: 1.3,
        predatorRePickFrames:    60,
        predatorSpeedMul:        1.5,
    },
    'Predator Spread': {
        separationRadius: 0.8, separationWeight: 1.8,
        alignmentRadius:  0.6, alignmentWeight:  0.7,
        cohesionRadius:   0.7, cohesionWeight:   0.4,
        boidMaxSpeed:     3.0,
        leaderInfluenceRadius: 2.0, leaderStrength: 0.5,
        predatorMode: 'stochastic-prey' as const,
        predatorFleeRadius:      1.5,
        predatorFleeStrength:    5.0,
        predatorDetectionRadius: 2.0,
        predatorRePickFrames:    150,
        predatorSpeedMul:        1.6,
    },
    'Waypoint Tour': {
        separationRadius: 0.4, separationWeight: 1.4,
        alignmentRadius:  0.6, alignmentWeight:  1.2,
        cohesionRadius:   0.7, cohesionWeight:   0.8,
        boidMaxSpeed:     2.4,
        leaderInfluenceRadius: 3.5, leaderStrength: 2.0,
        predatorMode: 'stochastic-prey' as const,
        predatorFleeRadius:      1.1,
        predatorFleeStrength:    3.5,
        predatorDetectionRadius: 1.7,
        predatorRePickFrames:    105,
        predatorSpeedMul:        1.3,
    },
    'Chaos': {
        separationRadius: 1.0, separationWeight: 0.6,
        alignmentRadius:  0.3, alignmentWeight:  0.3,
        cohesionRadius:   1.2, cohesionWeight:   0.3,
        boidMaxSpeed:     3.5,
        leaderInfluenceRadius: 3.0, leaderStrength: 0.8,
        predatorMode: 'nearest-prey' as const,
        predatorFleeRadius:      1.3,
        predatorFleeStrength:    3.5,
        predatorDetectionRadius: 1.8,
        predatorRePickFrames:    45,
        predatorSpeedMul:        1.7,
    },
} as const satisfies Record<string, Preset>;

export type PresetName = keyof typeof PRESETS;
