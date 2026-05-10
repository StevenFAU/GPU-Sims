// Four colormap LUTs baked into a single 256×4 RGBA8 texture at startup.
// Rows: 0 = magma, 1 = inferno, 2 = viridis, 3 = HSV.
//
// Magma/inferno/viridis use Inigo Quilez's well-known 7-coefficient
// polynomial fits to matplotlib's perceptual colormaps
// (https://www.shadertoy.com/view/WlfXRN — public domain on Shadertoy).
// HSV is the standard hsv → rgb conversion at full saturation and value.

export type ColormapName = 'magma' | 'inferno' | 'viridis' | 'hsv';
export const COLORMAP_ORDER: ColormapName[] = ['magma', 'inferno', 'viridis', 'hsv'];
export const COLORMAP_INDEX: Record<ColormapName, number> = {
    magma: 0,
    inferno: 1,
    viridis: 2,
    hsv: 3,
};

interface Vec3 { x: number; y: number; z: number; }

const magmaCoefs: Vec3[] = [
    { x: -0.002136485053939582, y: -0.000749655052795221, z: -0.005386127855323933 },
    { x:  0.2516605407371642,   y:  0.6775232436837668,   z:  2.494026599312351 },
    { x:  8.353717279216625,    y: -3.577719514958484,    z:  0.3144679030132573 },
    { x: -27.66873308576866,    y:  14.26473078096533,    z: -13.64921318813922 },
    { x:  52.17613981234068,    y: -27.94360607168351,    z:  12.94416944238394 },
    { x: -50.76852536473588,    y:  29.04658282127291,    z:  4.23415299384598 },
    { x:  18.65570506591883,    y: -11.48977351997711,    z: -5.601961508734096 },
];

const infernoCoefs: Vec3[] = [
    { x:  0.0002189403691192265, y:  0.001651004631001012, z: -0.01948089843709184 },
    { x:  0.1065134194856116,    y:  0.5639564367884091,   z:  3.932712388889277 },
    { x:  11.60249308247187,     y: -3.972853965665698,    z: -15.9423941062914 },
    { x: -41.70399613139459,     y:  17.43639888205313,    z:  44.35414519872813 },
    { x:  77.162935699427,       y: -33.40235894210092,    z: -81.80730925738993 },
    { x: -71.31942824499214,     y:  32.62606426397723,    z:  73.20951985803202 },
    { x:  25.13112622477341,     y: -12.24266895238567,    z: -23.07032500287172 },
];

const viridisCoefs: Vec3[] = [
    { x:  0.2777273272234177,  y:  0.005407344544966578, z:  0.3340998053353061 },
    { x:  0.1050930431085774,  y:  1.404613529898575,    z:  1.384590162594685 },
    { x: -0.3308618287255563,  y:  0.214847559468213,    z:  0.09509516302823659 },
    { x: -4.634230498983486,   y: -5.799100973351585,    z: -19.33244095627987 },
    { x:  6.228269936347081,   y:  14.17993336680509,    z:  56.69055260068105 },
    { x:  4.776384997670288,   y: -13.74514537774601,    z: -65.35303263337234 },
    { x: -5.435455855934631,   y:  4.645852612178535,    z:  26.3124352495832 },
];

function evalPolyColormap(coefs: Vec3[], t: number): [number, number, number] {
    // Horner's method: c0 + t*(c1 + t*(c2 + t*(c3 + t*(c4 + t*(c5 + t*c6)))))
    let r = coefs[6]!.x; let g = coefs[6]!.y; let b = coefs[6]!.z;
    for (let i = 5; i >= 0; i--) {
        const c = coefs[i]!;
        r = c.x + t * r;
        g = c.y + t * g;
        b = c.z + t * b;
    }
    return [r, g, b];
}

function hsv01(t: number): [number, number, number] {
    // h in [0,1), s = 1, v = 1 → standard rainbow
    const h = (t % 1 + 1) % 1;
    const i = Math.floor(h * 6);
    const f = h * 6 - i;
    const p = 0;
    const q = 1 - f;
    const u = f;
    switch (i % 6) {
        case 0: return [1, u, p];
        case 1: return [q, 1, p];
        case 2: return [p, 1, u];
        case 3: return [p, q, 1];
        case 4: return [u, p, 1];
        case 5: return [1, p, q];
    }
    return [0, 0, 0];
}

/** Build a 256×4 RGBA8 texture; one colormap per row. */
export function buildColormapTextureData(): Uint8Array<ArrayBuffer> {
    // Construct the underlying ArrayBuffer explicitly so the Uint8Array's
    // buffer type narrows to ArrayBuffer (not ArrayBufferLike). WebGPU's
    // writeTexture requires GPUAllowSharedBufferSource which excludes the
    // SharedArrayBuffer branch.
    const buffer = new ArrayBuffer(256 * 4 * 4);
    const data = new Uint8Array(buffer);

    function writeRow(rowIndex: number, sampler: (t: number) => [number, number, number]): void {
        for (let i = 0; i < 256; i++) {
            const t = i / 255;
            const [r, g, b] = sampler(t);
            const pixelOffset = (rowIndex * 256 + i) * 4;
            data[pixelOffset + 0] = clamp01(r) * 255;
            data[pixelOffset + 1] = clamp01(g) * 255;
            data[pixelOffset + 2] = clamp01(b) * 255;
            data[pixelOffset + 3] = 255;
        }
    }

    writeRow(0, (t) => evalPolyColormap(magmaCoefs, t));
    writeRow(1, (t) => evalPolyColormap(infernoCoefs, t));
    writeRow(2, (t) => evalPolyColormap(viridisCoefs, t));
    writeRow(3, (t) => hsv01(t));

    return data;
}

function clamp01(x: number): number {
    if (x < 0) return 0;
    if (x > 1) return 1;
    return x;
}
