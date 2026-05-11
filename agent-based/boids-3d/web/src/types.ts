import type { PresetName } from './presets.js';

/** Discrete agent-count tier identifier. */
export type AgentCountTier = '5k' | '10k' | '25k' | '50k' | '75k' | '100k';

/** Predator hunting strategy. Mutually exclusive runtime mode. */
export type PredatorMode = 'nearest-prey' | 'stochastic-prey' | 'flock-center';

/** A user-placed leader attractor. Position is in world-space; y is constrained
 *  to 0 in v1 by the click-to-place ground-plane intersection. */
export interface Leader {
    position: [number, number, number];
    strength: number;
}

/** Top-level per-sim runtime state. Mutated by panel callbacks, preset dropdown,
 *  capture-load, and click handlers. Not mutated by the per-frame compute loop
 *  (sim state lives on the GPU; this struct holds the CPU-side params + UI state). */
export interface Runtime {
    presetName: PresetName | 'Custom';
    agentCountTier: AgentCountTier;

    // Reynolds parameters
    separationRadius: number;
    separationWeight: number;
    alignmentRadius:  number;
    alignmentWeight:  number;
    cohesionRadius:   number;
    cohesionWeight:   number;
    boidMaxSpeed:     number;

    // Leaders
    leaders: Leader[];
    leaderInfluenceRadius: number;
    leaderStrength:        number;

    // Predators
    predatorMode: PredatorMode;
    predatorFleeRadius:      number;
    predatorFleeStrength:    number;
    predatorDetectionRadius: number;
    predatorRePickFrames:    number;
    predatorSpeedMul:        number;

    // Visualization (panel-tunable, NOT preset-driven)
    boidColor:     [number, number, number];
    predatorColor: [number, number, number];
    leaderColor:   [number, number, number];
    boidScale:     number;
    lightYawDeg:   number;
    lightPitchDeg: number;
    ambient:       number;

    // Init / control
    initSeed:    number;
    iteration:   number;
    autoResetOnPresetChange: boolean;
    autoResetOnTierChange:   boolean;
}
