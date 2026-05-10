import {
    initLogger, log,
    HotReloader, ParamPanel, StateWriter, StateReader,
    Context, Renderer,
    Texture, TextureType,
    ComputePipeline, RenderPipeline,
    Buffer, MemoryUsage,
    type JsonValue,
} from '@gpusims/common-web';

import { PRESETS, type PresetName } from './presets';

import fullscreenVert    from '../shaders/fullscreen.vert.wgsl?raw';
import clearWgsl         from '../shaders/clear_deposits.compute.wgsl?raw';
import agentMoveWgsl     from '../shaders/agent_move.compute.wgsl?raw';
import pinDepositWgsl    from '../shaders/pin_deposit.compute.wgsl?raw';
import diffuseDecayWgsl  from '../shaders/diffuse_decay.compute.wgsl?raw';
import visualizeFrag     from '../shaders/visualize.frag.wgsl?raw';

// HMR-relative paths — match what the Vite WGSL plugin emits (relative to web/).
const HMR_FULLSCREEN_VERT = 'shaders/fullscreen.vert.wgsl';
const HMR_CLEAR           = 'shaders/clear_deposits.compute.wgsl';
const HMR_AGENT_MOVE      = 'shaders/agent_move.compute.wgsl';
const HMR_PIN_DEPOSIT     = 'shaders/pin_deposit.compute.wgsl';
const HMR_DIFFUSE_DECAY   = 'shaders/diffuse_decay.compute.wgsl';
const HMR_VISUALIZE_FRAG  = 'shaders/visualize.frag.wgsl';

const MAX_PINS = 32;
const DEPOSIT_SCALE = 100;
const TRAIL_FORMAT: GPUTextureFormat = 'rgba16float';
const TRAIL_BPP = 8;                      // rgba16float = 8 bytes per pixel
const AGENT_BYTES = 16;
const PIN_BYTES = 16;
const VIZ_PARAMS_BYTES = 80;              // see VizParams struct layout
const PARAMS_BYTES = 64;                  // 4 u32 + 12 f32 = 64 B
const MAX_DPR = 2.0;
const DEFAULT_SEED = 0xC0FFEE;

// Discrete agent-count tiers. 4M default sized to hit ~60 fps on a typical
// desktop while keeping the 256k tier viable on integrated GPUs.
const AGENT_COUNT_TIERS = {
    '256k': 262_144,
    '1M':   1_048_576,
    '4M':   4_194_304,
    '10M': 10_000_000,
} as const satisfies Record<string, number>;
type AgentCountTier = keyof typeof AGENT_COUNT_TIERS;

const GRID_SIZES = [512, 1024, 2048] as const;
type GridSize = (typeof GRID_SIZES)[number];

interface Pin {
    posX: number;       // cell coords
    posY: number;
    intensity: number;
    speciesMask: number;
}

interface Runtime {
    presetName: PresetName | 'Custom';
    agentCountTier: AgentCountTier;
    gridSize: GridSize;

    // Sense / steer / move
    senseDistance: number;
    senseAngle: number;       // radians
    turnAngle: number;        // radians
    stepSize: number;
    simSpeed: number;

    // Trail
    decayRate: number;
    diffuseWeight: number;
    depositAmount: number;
    repulsionStrength: number;

    // Init
    initSeed: number;
    iteration: number;

    // Viz
    trailExposure: number;
    colorSpecies0: [number, number, number];
    colorSpecies1: [number, number, number];
    colorSpecies2: [number, number, number];

    // Pins
    pins: Pin[];
    pinIntensity: number;
    pinRadius: number;

    autoResetOnPresetChange: boolean;
}

function defaultRuntime(): Runtime {
    const initial = PRESETS['Networks'];
    return {
        presetName:        'Networks',
        agentCountTier:    '4M',
        gridSize:          1024,
        senseDistance:     initial.senseDistance,
        senseAngle:        initial.senseAngle,
        turnAngle:         initial.turnAngle,
        stepSize:          initial.stepSize,
        simSpeed:          1.0,
        decayRate:         initial.decayRate,
        diffuseWeight:     0.3,
        depositAmount:     initial.depositAmount,
        repulsionStrength: initial.repulsionStrength,
        initSeed:          DEFAULT_SEED,
        iteration:         0,
        trailExposure:     1.0,
        colorSpecies0:     [1.0, 0.2, 0.2],
        colorSpecies1:     [0.2, 1.0, 0.2],
        colorSpecies2:     [0.2, 0.2, 1.0],
        pins:              [],
        pinIntensity:      50.0,
        pinRadius:         4.0,
        autoResetOnPresetChange: true,
    };
}

async function main(): Promise<void> {
    initLogger();
    log.info('physarum: starting up');

    const canvas = document.getElementById('canvas') as HTMLCanvasElement | null;
    const hud = document.getElementById('hud') as HTMLDivElement | null;
    if (!canvas) throw new Error('no #canvas element');
    if (!hud)    throw new Error('no #hud element');

    if (!('gpu' in navigator)) {
        hud.textContent = 'WebGPU not available in this browser.';
        return;
    }

    // 10M-tier agent buffer is 160 MB; baseline maxStorageBufferBindingSize is 128 MiB.
    // Request the higher limit — most desktop GPUs support it. Adapters that
    // can't grant it will fail Context.create; the v1.1 graceful-fallback path
    // (notes.md priority 1.6) wraps this in a try/catch.
    let ctx: Context;
    try {
        ctx = await Context.create({
            canvas,
            powerPreference: 'high-performance',
            requiredLimits: {
                maxStorageBufferBindingSize: 200_000_000,
                maxBufferSize:               200_000_000,
            },
        });
    } catch (err) {
        log.error(`Context.create failed: ${err instanceof Error ? err.message : err}`);
        hud.textContent = 'WebGPU device unavailable (10M-tier limits not granted? See console).';
        return;
    }
    const renderer = new Renderer(ctx);
    const device = ctx.device;

    // Disable browser context menu so RMB can be used to remove pins.
    canvas.addEventListener('contextmenu', (e) => { e.preventDefault(); });

    function targetDimensions(): { width: number; height: number } {
        const dpr = Math.min(window.devicePixelRatio || 1, MAX_DPR);
        return {
            width:  Math.max(1, Math.floor(window.innerWidth  * dpr)),
            height: Math.max(1, Math.floor(window.innerHeight * dpr)),
        };
    }
    {
        const dim = targetDimensions();
        canvas.width = dim.width;
        canvas.height = dim.height;
    }

    const rt: Runtime = defaultRuntime();

    // ----- Persistent (grid-size-independent) buffers ---------------------
    const pinBuf = new Buffer(ctx, {
        size: MAX_PINS * PIN_BYTES,
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'physarum-pins',
    });
    const paramsBuf = new Buffer(ctx, {
        size: PARAMS_BYTES,
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'physarum-params',
    });
    const vizParamsBuf = new Buffer(ctx, {
        size: VIZ_PARAMS_BYTES,
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'physarum-viz-params',
    });
    const trailSampler = device.createSampler({
        label: 'physarum-trail-sampler',
        addressModeU: 'repeat',
        addressModeV: 'repeat',
        magFilter: 'nearest',
        minFilter: 'nearest',
    });

    // ----- Resources that depend on grid + agent count --------------------
    function makeTrail(gridSize: number, label: string): Texture {
        return new Texture(ctx, {
            type: TextureType.e2D,
            extent: { width: gridSize, height: gridSize, depthOrArrayLayers: 1 },
            format: TRAIL_FORMAT,
            usage: GPUTextureUsage.STORAGE_BINDING | GPUTextureUsage.TEXTURE_BINDING
                 | GPUTextureUsage.COPY_SRC          | GPUTextureUsage.COPY_DST,
            label,
        });
    }
    function makeAgentBuf(agentCount: number): Buffer {
        return new Buffer(ctx, {
            size: agentCount * AGENT_BYTES,
            usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST | GPUBufferUsage.COPY_SRC,
            memory: MemoryUsage.DeviceLocal,
            label: 'physarum-agents',
        });
    }
    function makeDepositBuf(gridSize: number, label: string): Buffer {
        return new Buffer(ctx, {
            size: gridSize * gridSize * 4,
            usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
            memory: MemoryUsage.DeviceLocal,
            label,
        });
    }

    let agentBuf:    Buffer  = makeAgentBuf(AGENT_COUNT_TIERS[rt.agentCountTier]);
    let deposit0Buf: Buffer  = makeDepositBuf(rt.gridSize, 'physarum-deposit0');
    let deposit1Buf: Buffer  = makeDepositBuf(rt.gridSize, 'physarum-deposit1');
    let deposit2Buf: Buffer  = makeDepositBuf(rt.gridSize, 'physarum-deposit2');
    let trailPing:   Texture = makeTrail(rt.gridSize, 'physarum-trail-ping');
    let trailPong:   Texture = makeTrail(rt.gridSize, 'physarum-trail-pong');
    // pingIsTarget == true means: the *next* diffuse-decay pass writes into trailPing,
    // so trailPong holds the latest visible state at the start of this frame.
    // Initially false: trailPing is the empty source we'll diffuse from on frame 0.
    let pingIsTarget = false;

    function recreateGridResources(): void {
        agentBuf.destroy();
        deposit0Buf.destroy();
        deposit1Buf.destroy();
        deposit2Buf.destroy();
        trailPing.destroy();
        trailPong.destroy();

        agentBuf = makeAgentBuf(AGENT_COUNT_TIERS[rt.agentCountTier]);
        deposit0Buf = makeDepositBuf(rt.gridSize, 'physarum-deposit0');
        deposit1Buf = makeDepositBuf(rt.gridSize, 'physarum-deposit1');
        deposit2Buf = makeDepositBuf(rt.gridSize, 'physarum-deposit2');
        trailPing = makeTrail(rt.gridSize, 'physarum-trail-ping');
        trailPong = makeTrail(rt.gridSize, 'physarum-trail-pong');
        pingIsTarget = false;
        rebuildBindGroups();
    }

    // ----- Pipelines -------------------------------------------------------
    const clearPipe = await ComputePipeline.create(ctx, {
        source: clearWgsl,
        shaderPath: HMR_CLEAR,
        entryPoint: 'cs_main',
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 2, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 3, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
        ],
        label: 'physarum-clear-deposits',
    });

    const agentMovePipe = await ComputePipeline.create(ctx, {
        source: agentMoveWgsl,
        shaderPath: HMR_AGENT_MOVE,
        entryPoint: 'cs_main',
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 2, visibility: GPUShaderStage.COMPUTE, texture: { sampleType: 'unfilterable-float', viewDimension: '2d' } },
            { binding: 3, visibility: GPUShaderStage.COMPUTE, sampler: { type: 'non-filtering' } },
            { binding: 4, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 5, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 6, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
        ],
        label: 'physarum-agent-move',
    });

    const pinDepositPipe = await ComputePipeline.create(ctx, {
        source: pinDepositWgsl,
        shaderPath: HMR_PIN_DEPOSIT,
        entryPoint: 'cs_main',
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },
            { binding: 2, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 3, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 4, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
        ],
        label: 'physarum-pin-deposit',
    });

    const diffuseDecayPipe = await ComputePipeline.create(ctx, {
        source: diffuseDecayWgsl,
        shaderPath: HMR_DIFFUSE_DECAY,
        entryPoint: 'cs_main',
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, texture: { sampleType: 'unfilterable-float', viewDimension: '2d' } },
            { binding: 2, visibility: GPUShaderStage.COMPUTE, sampler: { type: 'non-filtering' } },
            { binding: 3, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },
            { binding: 4, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },
            { binding: 5, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },
            { binding: 6, visibility: GPUShaderStage.COMPUTE, storageTexture: { access: 'write-only', format: TRAIL_FORMAT, viewDimension: '2d' } },
        ],
        label: 'physarum-diffuse-decay',
    });

    const visualizePipe = await RenderPipeline.create(ctx, {
        vertexSource:   fullscreenVert,
        fragmentSource: visualizeFrag,
        vertexPath:     HMR_FULLSCREEN_VERT,
        fragmentPath:   HMR_VISUALIZE_FRAG,
        bindings: [
            { binding: 0, visibility: GPUShaderStage.FRAGMENT, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'unfilterable-float', viewDimension: '2d' } },
            { binding: 2, visibility: GPUShaderStage.FRAGMENT, sampler: { type: 'non-filtering' } },
            { binding: 3, visibility: GPUShaderStage.FRAGMENT, buffer: { type: 'read-only-storage' } },
        ],
        colorFormats: [ctx.preferredFormat],
        primitive: { topology: 'triangle-list' },
        label: 'physarum-visualize',
    });

    // ----- Bind groups (rebuilt on grid/agent resize) ----------------------
    let bgClear:                  GPUBindGroup;
    let bgAgentMoveReadPing:      GPUBindGroup;
    let bgAgentMoveReadPong:      GPUBindGroup;
    let bgPinDeposit:             GPUBindGroup;
    let bgDiffuseDecayWritePong:  GPUBindGroup;
    let bgDiffuseDecayWritePing:  GPUBindGroup;
    let bgVisualizeFromPing:      GPUBindGroup;
    let bgVisualizeFromPong:      GPUBindGroup;

    function rebuildBindGroups(): void {
        bgClear = clearPipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: deposit0Buf.handle } },
            { binding: 2, resource: { buffer: deposit1Buf.handle } },
            { binding: 3, resource: { buffer: deposit2Buf.handle } },
        ], 'physarum-bg-clear');

        bgAgentMoveReadPing = agentMovePipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: agentBuf.handle } },
            { binding: 2, resource: trailPing.view },
            { binding: 3, resource: trailSampler },
            { binding: 4, resource: { buffer: deposit0Buf.handle } },
            { binding: 5, resource: { buffer: deposit1Buf.handle } },
            { binding: 6, resource: { buffer: deposit2Buf.handle } },
        ], 'physarum-bg-agent-move-from-ping');
        bgAgentMoveReadPong = agentMovePipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: agentBuf.handle } },
            { binding: 2, resource: trailPong.view },
            { binding: 3, resource: trailSampler },
            { binding: 4, resource: { buffer: deposit0Buf.handle } },
            { binding: 5, resource: { buffer: deposit1Buf.handle } },
            { binding: 6, resource: { buffer: deposit2Buf.handle } },
        ], 'physarum-bg-agent-move-from-pong');

        bgPinDeposit = pinDepositPipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: pinBuf.handle } },
            { binding: 2, resource: { buffer: deposit0Buf.handle } },
            { binding: 3, resource: { buffer: deposit1Buf.handle } },
            { binding: 4, resource: { buffer: deposit2Buf.handle } },
        ], 'physarum-bg-pin-deposit');

        bgDiffuseDecayWritePong = diffuseDecayPipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: trailPing.view },
            { binding: 2, resource: trailSampler },
            { binding: 3, resource: { buffer: deposit0Buf.handle } },
            { binding: 4, resource: { buffer: deposit1Buf.handle } },
            { binding: 5, resource: { buffer: deposit2Buf.handle } },
            { binding: 6, resource: trailPong.view },
        ], 'physarum-bg-diffuse-write-pong');
        bgDiffuseDecayWritePing = diffuseDecayPipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: trailPong.view },
            { binding: 2, resource: trailSampler },
            { binding: 3, resource: { buffer: deposit0Buf.handle } },
            { binding: 4, resource: { buffer: deposit1Buf.handle } },
            { binding: 5, resource: { buffer: deposit2Buf.handle } },
            { binding: 6, resource: trailPing.view },
        ], 'physarum-bg-diffuse-write-ping');

        bgVisualizeFromPing = visualizePipe.createBindGroup([
            { binding: 0, resource: { buffer: vizParamsBuf.handle } },
            { binding: 1, resource: trailPing.view },
            { binding: 2, resource: trailSampler },
            { binding: 3, resource: { buffer: pinBuf.handle } },
        ], 'physarum-bg-visualize-from-ping');
        bgVisualizeFromPong = visualizePipe.createBindGroup([
            { binding: 0, resource: { buffer: vizParamsBuf.handle } },
            { binding: 1, resource: trailPong.view },
            { binding: 2, resource: trailSampler },
            { binding: 3, resource: { buffer: pinBuf.handle } },
        ], 'physarum-bg-visualize-from-pong');
    }
    rebuildBindGroups();

    // ----- Hot reload subscriptions ---------------------------------------
    // Bind groups stay valid across pipeline reloads (the layout is reused),
    // so we do NOT rebuild them here.
    const hot = new HotReloader();
    const watchCompute = (path: string, pipe: ComputePipeline): void => {
        hot.watch(path, async (filePath, src) => {
            const err = await pipe.reload(src);
            if (err) hot.reportFailure(filePath, err);
            else     hot.reportSuccess(filePath);
        });
    };
    watchCompute(HMR_CLEAR,         clearPipe);
    watchCompute(HMR_AGENT_MOVE,    agentMovePipe);
    watchCompute(HMR_PIN_DEPOSIT,   pinDepositPipe);
    watchCompute(HMR_DIFFUSE_DECAY, diffuseDecayPipe);
    hot.watch(HMR_FULLSCREEN_VERT, async (filePath, src) => {
        const err = await visualizePipe.reload('vertex', src);
        if (err) hot.reportFailure(filePath, err);
        else     hot.reportSuccess(filePath);
    });
    hot.watch(HMR_VISUALIZE_FRAG, async (filePath, src) => {
        const err = await visualizePipe.reload('fragment', src);
        if (err) hot.reportFailure(filePath, err);
        else     hot.reportSuccess(filePath);
    });

    // ----- Initial state upload -------------------------------------------
    // Reseed builds the agent buffer CPU-side using xorshift32(initSeed) so that
    // (initSeed, agentCount, gridSize) → bit-identical agent layout. At the 10M
    // tier this uploads 160 MB; the UI freezes briefly. v1.1 priority 1.1 is a
    // GPU-side init kernel that eliminates the freeze.
    const reseedAgents = (): void => {
        const ac = AGENT_COUNT_TIERS[rt.agentCountTier];
        const ab = new ArrayBuffer(ac * AGENT_BYTES);
        const f32 = new Float32Array(ab);
        const u32 = new Uint32Array(ab);
        let s = (rt.initSeed >>> 0) || 1;
        const r01 = (): number => {
            s ^= (s << 13);
            s ^= (s >>> 17);
            s ^= (s << 5);
            return (s >>> 0) / 4294967296;
        };
        const g = rt.gridSize;
        for (let i = 0; i < ac; i++) {
            f32[i * 4 + 0] = r01() * g;
            f32[i * 4 + 1] = r01() * g;
            f32[i * 4 + 2] = r01() * Math.PI * 2;
            u32[i * 4 + 3] = i % 3;
        }
        agentBuf.uploadDirect(ab);
        rt.iteration = 0;

        const zeros = new Uint8Array(rt.gridSize * rt.gridSize * TRAIL_BPP);
        trailPing.uploadDirect2D(zeros, TRAIL_BPP);
        trailPong.uploadDirect2D(zeros, TRAIL_BPP);
        // Pins not cleared by reseed — visitor may want them across reseeds.
    };

    const uploadPins = (): void => {
        const ab = new ArrayBuffer(MAX_PINS * PIN_BYTES);
        const f32 = new Float32Array(ab);
        const u32 = new Uint32Array(ab);
        for (let i = 0; i < rt.pins.length; i++) {
            const p = rt.pins[i]!;
            f32[i * 4 + 0] = p.posX;
            f32[i * 4 + 1] = p.posY;
            f32[i * 4 + 2] = p.intensity;
            u32[i * 4 + 3] = p.speciesMask;
        }
        // Tail already zero from ArrayBuffer construction.
        pinBuf.uploadDirect(ab);
    };

    const paramsBytes = new ArrayBuffer(PARAMS_BYTES);
    const paramsU32 = new Uint32Array(paramsBytes);
    const paramsF32 = new Float32Array(paramsBytes);
    const uploadParams = (): void => {
        paramsU32[0] = rt.gridSize;
        paramsU32[1] = AGENT_COUNT_TIERS[rt.agentCountTier];
        paramsU32[2] = rt.iteration;
        paramsU32[3] = DEPOSIT_SCALE;
        paramsF32[4] = rt.senseDistance;
        paramsF32[5] = rt.senseAngle;
        paramsF32[6] = rt.turnAngle;
        paramsF32[7] = rt.stepSize;
        paramsF32[8] = rt.decayRate;
        paramsF32[9] = rt.diffuseWeight;
        paramsF32[10] = rt.depositAmount;
        paramsF32[11] = rt.repulsionStrength;
        paramsF32[12] = rt.simSpeed;
        paramsU32[13] = rt.pins.length;
        paramsF32[14] = rt.pinIntensity;
        paramsF32[15] = rt.pinRadius;
        paramsBuf.uploadDirect(paramsBytes);
    };

    const vizBytes = new ArrayBuffer(VIZ_PARAMS_BYTES);
    const vizF32 = new Float32Array(vizBytes);
    const vizU32 = new Uint32Array(vizBytes);
    const uploadVizParams = (): void => {
        // Layout (5 vec4-equivalent slots, 80 bytes total):
        //   slot 0: vec2 canvasSize
        //   slot 1: u32 gridSize, u32 pinCount, f32 pinRadius, f32 trailExposure
        //   slot 2: vec2 _pad0 (followed by start of vec3 colorSpecies0)
        //          actually vec3 needs 16-byte alignment, so:
        //   slot 2: vec2 _pad0, then padding to slot 3
        //   slot 3: vec3 colorSpecies0, f32 _pad1
        //   slot 4: vec3 colorSpecies1, f32 _pad2
        //   slot 5: vec3 colorSpecies2, f32 _pad3
        // Wait — that's 6 slots = 96 B, not 80. The struct definition packs more
        // tightly: canvasSize (8) + gridSize (4) + pinCount (4) + pinRadius (4)
        // + trailExposure (4) + _pad0 (8) + colorSpecies0 (12) + _pad1 (4) +
        // colorSpecies1 (12) + _pad2 (4) + colorSpecies2 (12) + _pad3 (4) = 80.
        // The _pad0 vec2 sits between trailExposure and colorSpecies0 to push
        // colorSpecies0 to a 16-byte-aligned offset (32).
        vizF32[0] = canvas.width;
        vizF32[1] = canvas.height;
        vizU32[2] = rt.gridSize;
        vizU32[3] = rt.pins.length;
        vizF32[4] = rt.pinRadius;
        vizF32[5] = rt.trailExposure;
        // f32[6], f32[7] = _pad0 (left zero)
        vizF32[8]  = rt.colorSpecies0[0];
        vizF32[9]  = rt.colorSpecies0[1];
        vizF32[10] = rt.colorSpecies0[2];
        // f32[11] = _pad1
        vizF32[12] = rt.colorSpecies1[0];
        vizF32[13] = rt.colorSpecies1[1];
        vizF32[14] = rt.colorSpecies1[2];
        // f32[15] = _pad2
        vizF32[16] = rt.colorSpecies2[0];
        vizF32[17] = rt.colorSpecies2[1];
        vizF32[18] = rt.colorSpecies2[2];
        // f32[19] = _pad3
        vizParamsBuf.uploadDirect(vizBytes);
    };

    reseedAgents();
    uploadPins();

    // ----- ParamPanel -----------------------------------------------------
    const panel = new ParamPanel({ title: 'Physarum', persistKey: 'physarum' });

    const presetCtrl = panel.addDropdown({
        label: 'Preset',
        getValue: () => rt.presetName,
        setValue: (v: string) => {
            rt.presetName = v as PresetName | 'Custom';
            if (v in PRESETS) {
                const p = PRESETS[v as PresetName];
                rt.senseDistance     = p.senseDistance;
                rt.senseAngle        = p.senseAngle;
                rt.turnAngle         = p.turnAngle;
                rt.stepSize          = p.stepSize;
                rt.decayRate         = p.decayRate;
                rt.depositAmount     = p.depositAmount;
                rt.repulsionStrength = p.repulsionStrength;
                if (rt.autoResetOnPresetChange) reseedAgents();
            }
            panel.refreshDisplays();
        },
        options: [
            ...(Object.keys(PRESETS).map((k) => ({ label: k, value: k }))),
            { label: 'Custom', value: 'Custom' },
        ],
    });

    const onCustomTouch = (): void => {
        rt.presetName = 'Custom';
        presetCtrl.updateDisplay();
    };

    const sceneFolder = panel.addFolder('Scene');
    sceneFolder.addDropdown({
        label: 'Agent count',
        getValue: () => rt.agentCountTier,
        setValue: (v: string) => {
            rt.agentCountTier = v as AgentCountTier;
            recreateGridResources();
            reseedAgents();
            panel.refreshDisplays();
        },
        options: Object.keys(AGENT_COUNT_TIERS).map((k) => ({ label: k, value: k })),
    });
    sceneFolder.addDropdown({
        label: 'Grid size',
        getValue: () => String(rt.gridSize),
        setValue: (v: string) => {
            rt.gridSize = Number(v) as GridSize;
            recreateGridResources();
            reseedAgents();
            panel.refreshDisplays();
        },
        options: GRID_SIZES.map((g) => ({ label: String(g), value: String(g) })),
    });
    sceneFolder.addNumber(rt, 'simSpeed', 0.25, 4.0, 0.05).name('Sim speed');

    const senseFolder = panel.addFolder('Sense / Steer');
    senseFolder.addNumber(rt, 'senseDistance', 1.0, 30.0, 0.5).name('Sense distance').onChange(onCustomTouch);
    senseFolder.addNumber(rt, 'senseAngle',    0.05, Math.PI / 2, 0.01).name('Sense angle').onChange(onCustomTouch);
    senseFolder.addNumber(rt, 'turnAngle',     0.05, Math.PI / 2, 0.01).name('Turn angle').onChange(onCustomTouch);
    senseFolder.addNumber(rt, 'stepSize',      0.1, 4.0, 0.05).name('Step size').onChange(onCustomTouch);

    const trailFolder = panel.addFolder('Trail');
    trailFolder.addNumber(rt, 'decayRate',          0.001, 0.20, 0.001).name('Decay rate').onChange(onCustomTouch);
    trailFolder.addNumber(rt, 'diffuseWeight',      0.0,   1.0,  0.01).name('Diffuse weight').onChange(onCustomTouch);
    trailFolder.addNumber(rt, 'depositAmount',      0.0,   20.0, 0.1).name('Deposit amount').onChange(onCustomTouch);
    trailFolder.addNumber(rt, 'repulsionStrength',  0.0,   5.0,  0.05).name('Cross-species repulsion').onChange(onCustomTouch);

    const vizFolder = panel.addFolder('Visualization');
    vizFolder.addNumber(rt, 'trailExposure', 0.1, 5.0, 0.05).name('Trail intensity');
    vizFolder.addColor(rt,  'colorSpecies0').name('Species 0 color');
    vizFolder.addColor(rt,  'colorSpecies1').name('Species 1 color');
    vizFolder.addColor(rt,  'colorSpecies2').name('Species 2 color');

    const pinFolder = panel.addFolder('Food pins');
    pinFolder.addNumber(rt, 'pinIntensity', 1.0,  500.0, 1.0).name('Pin intensity');
    pinFolder.addNumber(rt, 'pinRadius',    1.0,  32.0,  0.5).name('Pin radius');
    pinFolder.addButton('Clear all pins', () => { rt.pins = []; uploadPins(); });

    const seedFolder = panel.addFolder('Seed / Reset');
    seedFolder.addNumber(rt, 'initSeed', 0, 0xFFFFFFFF, 1).name('Init seed');
    seedFolder.addBoolean(rt, 'autoResetOnPresetChange').name('Auto-reset on preset');
    seedFolder.addButton('Reseed', () => reseedAgents());

    panel.addButton('Save (F5)',    () => { void doSave(); });
    panel.addButton('Load... (F9)', () => { triggerFileLoad(); });

    // ----- Capture / Load --------------------------------------------------
    const stateWriter = new StateWriter('captures');
    let nextCapture = 0;

    interface CaptureMeta {
        presetName:        Runtime['presetName'];
        agentCountTier:    Runtime['agentCountTier'];
        gridSize:          number;
        senseDistance:     number;
        senseAngle:        number;
        turnAngle:         number;
        stepSize:          number;
        decayRate:         number;
        diffuseWeight:     number;
        depositAmount:     number;
        repulsionStrength: number;
        simSpeed:          number;
        initSeed:          number;
        iteration:         number;
        trailExposure:     number;
        colorSpecies0:     [number, number, number];
        colorSpecies1:     [number, number, number];
        colorSpecies2:     [number, number, number];
        pinCount:          number;
        pinIntensity:      number;
        pinRadius:         number;
        pins:              Array<{ pos: [number, number]; speciesMask: number }>;
    }

    const captureMeta = (): CaptureMeta => ({
        presetName:        rt.presetName,
        agentCountTier:    rt.agentCountTier,
        gridSize:          rt.gridSize,
        senseDistance:     rt.senseDistance,
        senseAngle:        rt.senseAngle,
        turnAngle:         rt.turnAngle,
        stepSize:          rt.stepSize,
        decayRate:         rt.decayRate,
        diffuseWeight:     rt.diffuseWeight,
        depositAmount:     rt.depositAmount,
        repulsionStrength: rt.repulsionStrength,
        simSpeed:          rt.simSpeed,
        initSeed:          rt.initSeed,
        iteration:         rt.iteration,
        trailExposure:     rt.trailExposure,
        colorSpecies0:     rt.colorSpecies0,
        colorSpecies1:     rt.colorSpecies1,
        colorSpecies2:     rt.colorSpecies2,
        pinCount:          rt.pins.length,
        pinIntensity:      rt.pinIntensity,
        pinRadius:         rt.pinRadius,
        pins:              rt.pins.map((p) => ({ pos: [p.posX, p.posY], speciesMask: p.speciesMask })),
    });

    const doSave = async (): Promise<void> => {
        // The texture most recently written by diffuse-decay holds the latest
        // visible state. After a frame, pingIsTarget has been flipped; the
        // last write target is the one ping/pong is now reading from for viz.
        const latest = pingIsTarget ? trailPong : trailPing;
        const trailBytes = await latest.readback2D(TRAIL_BPP);
        stateWriter.beginFrame(nextCapture);
        stateWriter.setMeta('physarum', captureMeta() as unknown as JsonValue);
        stateWriter.saveBuffer('trail', trailBytes, {
            count: rt.gridSize * rt.gridSize,
            stride: TRAIL_BPP,
            format: 'rgba16float',
            shape: [rt.gridSize, rt.gridSize, 4],
        });
        await stateWriter.endFrame();
        log.info(`captured capture_${nextCapture.toString().padStart(4, '0')}.zip`);
        nextCapture++;
    };

    const applyCapture = (m: CaptureMeta, trailBytes: Uint8Array): void => {
        const gridChanged = (rt.gridSize !== m.gridSize);
        const tierChanged = (rt.agentCountTier !== m.agentCountTier);

        rt.presetName        = m.presetName;
        rt.gridSize          = m.gridSize as GridSize;
        rt.agentCountTier    = m.agentCountTier;
        rt.senseDistance     = m.senseDistance;
        rt.senseAngle        = m.senseAngle;
        rt.turnAngle         = m.turnAngle;
        rt.stepSize          = m.stepSize;
        rt.decayRate         = m.decayRate;
        rt.diffuseWeight     = m.diffuseWeight;
        rt.depositAmount     = m.depositAmount;
        rt.repulsionStrength = m.repulsionStrength;
        rt.simSpeed          = m.simSpeed;
        rt.initSeed          = m.initSeed;
        rt.iteration         = m.iteration;
        rt.trailExposure     = m.trailExposure;
        rt.colorSpecies0     = m.colorSpecies0;
        rt.colorSpecies1     = m.colorSpecies1;
        rt.colorSpecies2     = m.colorSpecies2;
        rt.pinIntensity      = m.pinIntensity;
        rt.pinRadius         = m.pinRadius;
        rt.pins              = m.pins.map((p) => ({
            posX: p.pos[0], posY: p.pos[1], intensity: rt.pinIntensity, speciesMask: p.speciesMask,
        }));

        if (gridChanged || tierChanged) {
            recreateGridResources();
        }

        // Reseed agents (NOT loaded literally — see notes.md and load-bearing-decisions.md § 6).
        reseedAgents();
        uploadPins();

        // Restore trail bytes onto trailPong; trailPing stays zero (will be the
        // next diffuse-decay write target). Set pingIsTarget = true so the
        // visualize pass reads from trailPong (the loaded state).
        trailPong.uploadDirect2D(trailBytes, TRAIL_BPP);
        const zeros = new Uint8Array(rt.gridSize * rt.gridSize * TRAIL_BPP);
        trailPing.uploadDirect2D(zeros, TRAIL_BPP);
        pingIsTarget = true;

        panel.refreshDisplays();
    };

    const triggerFileLoad = (): void => {
        const inp = document.createElement('input');
        inp.type = 'file';
        inp.accept = '.zip';
        inp.onchange = async (): Promise<void> => {
            const f = inp.files?.[0];
            if (!f) return;
            const cap = await StateReader.fromFile(f);
            if (!cap) return;
            const m = cap.meta('physarum') as unknown as CaptureMeta | undefined;
            if (!m) { log.error('capture missing physarum meta'); return; }
            const trailBytes = cap.buffer('trail');
            if (!trailBytes) { log.error('capture missing trail buffer'); return; }
            applyCapture(m, trailBytes);
            log.info(`loaded ${cap.directoryName}`);
        };
        inp.click();
    };

    // ----- Hotkeys --------------------------------------------------------
    let prevF5 = false, prevF9 = false, prevR = false;
    window.addEventListener('keydown', (e) => {
        if (e.code === 'F5') { e.preventDefault(); if (!prevF5) { prevF5 = true; void doSave(); } }
        if (e.code === 'F9') { e.preventDefault(); if (!prevF9) { prevF9 = true; triggerFileLoad(); } }
        if (e.code === 'KeyR') { if (!prevR) { prevR = true; reseedAgents(); } }
    });
    window.addEventListener('keyup', (e) => {
        if (e.code === 'F5')   prevF5 = false;
        if (e.code === 'F9')   prevF9 = false;
        if (e.code === 'KeyR') prevR  = false;
    });

    // ----- Pointer (pin placement / removal) ------------------------------
    const cursorToCell = (e: PointerEvent): { x: number; y: number } => {
        const rect = canvas.getBoundingClientRect();
        const u = (e.clientX - rect.left) / Math.max(rect.width,  1);
        const v = (e.clientY - rect.top)  / Math.max(rect.height, 1);
        return {
            x: u * rt.gridSize,
            y: v * rt.gridSize,
        };
    };

    const onPointerDown = (e: PointerEvent): void => {
        const cell = cursorToCell(e);
        if (e.button === 0) {
            if (rt.pins.length >= MAX_PINS) {
                log.warn(`pin limit (${MAX_PINS}) reached`);
                return;
            }
            rt.pins.push({ posX: cell.x, posY: cell.y, intensity: rt.pinIntensity, speciesMask: 7 });
            uploadPins();
        } else if (e.button === 2) {
            // Find nearest pin within 8-cell radius.
            let bestIdx = -1, bestD = 8.0 * 8.0;
            for (let i = 0; i < rt.pins.length; i++) {
                const p = rt.pins[i]!;
                const d = (p.posX - cell.x) ** 2 + (p.posY - cell.y) ** 2;
                if (d < bestD) { bestD = d; bestIdx = i; }
            }
            if (bestIdx >= 0) {
                rt.pins.splice(bestIdx, 1);
                uploadPins();
            }
        }
    };
    canvas.addEventListener('pointerdown', onPointerDown);

    // ----- Resize handling (debounced; CSS sizes are 100vw/100vh in HTML) -
    let resizeTimer: number | null = null;
    window.addEventListener('resize', () => {
        if (resizeTimer !== null) clearTimeout(resizeTimer);
        resizeTimer = window.setTimeout(() => {
            const dim = targetDimensions();
            canvas.width  = dim.width;
            canvas.height = dim.height;
            resizeTimer = null;
        }, 100);
    });

    // ----- Per-frame loop -------------------------------------------------
    let lastFrameTime = performance.now();
    let frameTimeMs = 16.7;

    const frame = (): void => {
        const now = performance.now();
        const dt = now - lastFrameTime;
        lastFrameTime = now;
        frameTimeMs = frameTimeMs * 0.9 + dt * 0.1;

        rt.iteration++;
        uploadParams();
        uploadVizParams();

        const f = renderer.beginFrame();
        const enc = f.encoder;

        const total = rt.gridSize * rt.gridSize;
        const ac = AGENT_COUNT_TIERS[rt.agentCountTier];

        // 1. Clear deposits (workgroup_size 256).
        clearPipe.dispatch(enc, bgClear, Math.ceil(total / 256), 1, 1);

        // 2. Agent move (workgroup_size 256). Read trailPing if pingIsTarget,
        //    else trailPong (the latest written by the previous frame).
        const bgAgentMove = pingIsTarget ? bgAgentMoveReadPing : bgAgentMoveReadPong;
        agentMovePipe.dispatch(enc, bgAgentMove, Math.ceil(ac / 256), 1, 1);

        // 3. Pin deposit (conditional).
        if (rt.pins.length > 0) {
            const wg = Math.ceil(rt.gridSize / 8);
            pinDepositPipe.dispatch(enc, bgPinDeposit, wg, wg, 1);
        }

        // 4. Diffuse + decay. Write into the target ping/pong.
        const bgDiff = pingIsTarget ? bgDiffuseDecayWritePing : bgDiffuseDecayWritePong;
        const wgDiff = Math.ceil(rt.gridSize / 16);
        diffuseDecayPipe.dispatch(enc, bgDiff, wgDiff, wgDiff, 1);

        // 5. Visualize the texture we just wrote.
        const visBg = pingIsTarget ? bgVisualizeFromPing : bgVisualizeFromPong;
        const renderPass = renderer.beginRendering(f, [0.04, 0.04, 0.04, 1.0]);
        visualizePipe.bind(renderPass, visBg);
        renderPass.draw(3, 1, 0, 0);
        renderer.endRendering(renderPass);

        renderer.endFrame(f);
        pingIsTarget = !pingIsTarget;

        hud.textContent = `${frameTimeMs.toFixed(1)} ms · ${(1000 / frameTimeMs).toFixed(0)} fps · `
                        + `${ac.toLocaleString()} agents · `
                        + `${rt.gridSize}² grid · ${rt.pins.length} pins`;

        requestAnimationFrame(frame);
    };
    requestAnimationFrame(frame);
    log.info('physarum: entered main loop');
}

main().catch((err) => {
    log.error(`physarum: failed to start: ${err instanceof Error ? err.message : err}`);
});
