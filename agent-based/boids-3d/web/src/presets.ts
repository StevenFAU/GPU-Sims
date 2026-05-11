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
        separationRadius: 1.0, separationWeight: 1.5,
        alignmentRadius:  1.8, alignmentWeight:  1.0,
        cohesionRadius:   2.5, cohesionWeight:   0.8,
        boidMaxSpeed:     3.5,
        leaderInfluenceRadius: 6.0, leaderStrength: 1.5,
        predatorMode: 'nearest-prey' as const,
        predatorFleeRadius:      2.0,
        predatorFleeStrength:    3.0,
        predatorDetectionRadius: 3.5,
        predatorRePickFrames:    90,
        predatorSpeedMul:        1.4,
    },
    'Loose Murmuration': {
        separationRadius: 0.8, separationWeight: 1.0,
        alignmentRadius:  3.5, alignmentWeight:  1.6,
        cohesionRadius:   3.0, cohesionWeight:   0.4,
        boidMaxSpeed:     4.0,
        leaderInfluenceRadius: 5.0, leaderStrength: 1.0,
        predatorMode: 'flock-center' as const,
        predatorFleeRadius:      2.5,
        predatorFleeStrength:    2.0,
        predatorDetectionRadius: 4.0,
        predatorRePickFrames:    120,
        predatorSpeedMul:        1.3,
    },
    'Tight Schooling': {
        separationRadius: 0.6, separationWeight: 2.2,
        alignmentRadius:  1.4, alignmentWeight:  1.8,
        cohesionRadius:   1.8, cohesionWeight:   1.5,
        boidMaxSpeed:     2.8,
        leaderInfluenceRadius: 4.0, leaderStrength: 1.2,
        predatorMode: 'nearest-prey' as const,
        predatorFleeRadius:      1.8,
        predatorFleeStrength:    4.0,
        predatorDetectionRadius: 3.0,
        predatorRePickFrames:    60,
        predatorSpeedMul:        1.6,
    },
    'Predator Spread': {
        separationRadius: 1.2, separationWeight: 1.8,
        alignmentRadius:  2.0, alignmentWeight:  1.2,
        cohesionRadius:   2.2, cohesionWeight:   0.6,
        boidMaxSpeed:     4.5,
        leaderInfluenceRadius: 5.0, leaderStrength: 1.0,
        predatorMode: 'stochastic-prey' as const,
        predatorFleeRadius:      3.0,
        predatorFleeStrength:    6.0,
        predatorDetectionRadius: 4.0,
        predatorRePickFrames:    90,
        predatorSpeedMul:        1.5,
    },
    'Waypoint Tour': {
        separationRadius: 1.0, separationWeight: 1.4,
        alignmentRadius:  1.5, alignmentWeight:  1.2,
        cohesionRadius:   1.8, cohesionWeight:   0.8,
        boidMaxSpeed:     3.0,
        leaderInfluenceRadius: 8.0, leaderStrength: 2.5,
        predatorMode: 'stochastic-prey' as const,
        predatorFleeRadius:      2.5,
        predatorFleeStrength:    3.5,
        predatorDetectionRadius: 3.5,
        predatorRePickFrames:    105,
        predatorSpeedMul:        1.3,
    },
    'Chaos': {
        separationRadius: 1.5, separationWeight: 2.5,
        alignmentRadius:  3.0, alignmentWeight:  2.0,
        cohesionRadius:   3.5, cohesionWeight:   2.0,
        boidMaxSpeed:     5.0,
        leaderInfluenceRadius: 6.0, leaderStrength: 2.0,
        predatorMode: 'nearest-prey' as const,
        predatorFleeRadius:      3.5,
        predatorFleeStrength:    5.0,
        predatorDetectionRadius: 4.0,
        predatorRePickFrames:    45,
        predatorSpeedMul:        2.0,
    },
} as const satisfies Record<string, Preset>;

export type PresetName = keyof typeof PRESETS;
