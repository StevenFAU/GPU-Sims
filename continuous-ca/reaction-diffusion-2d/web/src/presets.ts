// Pearson 1993 named regions for Gray-Scott reaction-diffusion.
//
// Same six regions, same labels, same approximate (F, k) values as
// continuous-ca/reaction-diffusion-3d (Stack C, Phase 3) — cross-stack name
// parity is the convention.
//
// Values are *approximate region centers from the literature*. Pearson regions
// are not single points but neighborhoods in (F, k) space. If a preset doesn't
// show its named pattern type when running, cross-reference Robert Munafo's
// catalog at https://mrob.com/pub/comp/xmorphia and edit one row here.
//
// Reference: Pearson, J. E. (1993). "Complex Patterns in a Simple System."
// Science 261(5118), 189–192.

export interface PearsonPreset {
    /** Greek letter + short name, used as the dropdown label. */
    label: string;
    /** One-sentence pattern description, used in hover tooltips. */
    description: string;
    F: number;
    k: number;
}

export const PEARSON_PRESETS: PearsonPreset[] = [
    { label: 'λ — Irregular spots',  description: 'Static spots in random arrangement',           F: 0.026, k: 0.061 },
    { label: 'σ — Stripes',          description: 'Labyrinthine stripe patterns',                F: 0.037, k: 0.060 },
    { label: 'α — Chaotic',          description: 'Continuously rearranging chaos',              F: 0.014, k: 0.047 },
    { label: 'β — Uniform-ish',      description: 'Slow uniform blobs near the stability edge',  F: 0.026, k: 0.055 },
    { label: 'ξ — Moving spots',     description: 'Spots drift across the grid',                 F: 0.018, k: 0.051 },
    { label: 'τ — U-skate',          description: 'Spots replicate by mitosis',                  F: 0.020, k: 0.052 },
];
