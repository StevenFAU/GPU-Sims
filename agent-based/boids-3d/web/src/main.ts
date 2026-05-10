import {
    initLogger, log,
    HotReloader, ParamPanel, StateWriter, StateReader,
    Context, Renderer,
    Texture, TextureType,
    ComputePipeline, RenderPipeline,
    Buffer, MemoryUsage,
    Camera, snapshotInput,
    type JsonValue, type JsonObject,
} from '@gpusims/common-web';

import { PRESETS, type PresetName } from './presets.js';
import type {
    AgentCountTier, PredatorMode, Runtime,
} from './types.js';
import { unprojectToGroundPlane } from './unproject.js';

import cellCountWgsl       from '../shaders/cell_count.compute.wgsl?raw';
import prefixLocalWgsl     from '../shaders/prefix_sum_local.compute.wgsl?raw';
import prefixBlockWgsl     from '../shaders/prefix_sum_block.compute.wgsl?raw';
import prefixAddbackWgsl   from '../shaders/prefix_sum_addback.compute.wgsl?raw';
import scatterWgsl         from '../shaders/scatter.compute.wgsl?raw';
import flockUpdateWgsl     from '../shaders/flock_update.compute.wgsl?raw';
import predatorUpdateWgsl  from '../shaders/predator_update.compute.wgsl?raw';
import integrateWgsl       from '../shaders/integrate.compute.wgsl?raw';

import boidVert            from '../shaders/boid_render.vert.wgsl?raw';
import boidFrag            from '../shaders/boid_render.frag.wgsl?raw';
import wireframeVert       from '../shaders/wireframe.vert.wgsl?raw';
import wireframeFrag       from '../shaders/wireframe.frag.wgsl?raw';
import leaderVert          from '../shaders/leader_render.vert.wgsl?raw';
import leaderFrag          from '../shaders/leader_render.frag.wgsl?raw';
import bgVert              from '../shaders/background.vert.wgsl?raw';
import bgFrag              from '../shaders/background.frag.wgsl?raw';

// ---------------------------------------------------------------------------
// HMR-relative paths — match what the Vite WGSL plugin emits (relative to web/).
// ---------------------------------------------------------------------------
const HMR_CELL_COUNT       = 'shaders/cell_count.compute.wgsl';
const HMR_PREFIX_LOCAL     = 'shaders/prefix_sum_local.compute.wgsl';
const HMR_PREFIX_BLOCK     = 'shaders/prefix_sum_block.compute.wgsl';
const HMR_PREFIX_ADDBACK   = 'shaders/prefix_sum_addback.compute.wgsl';
const HMR_SCATTER          = 'shaders/scatter.compute.wgsl';
const HMR_FLOCK_UPDATE     = 'shaders/flock_update.compute.wgsl';
const HMR_PREDATOR_UPDATE  = 'shaders/predator_update.compute.wgsl';
const HMR_INTEGRATE        = 'shaders/integrate.compute.wgsl';
const HMR_BOID_VERT        = 'shaders/boid_render.vert.wgsl';
const HMR_BOID_FRAG        = 'shaders/boid_render.frag.wgsl';
const HMR_WIREFRAME_VERT   = 'shaders/wireframe.vert.wgsl';
const HMR_WIREFRAME_FRAG   = 'shaders/wireframe.frag.wgsl';
const HMR_LEADER_VERT      = 'shaders/leader_render.vert.wgsl';
const HMR_LEADER_FRAG      = 'shaders/leader_render.frag.wgsl';
const HMR_BG_VERT          = 'shaders/background.vert.wgsl';
const HMR_BG_FRAG          = 'shaders/background.frag.wgsl';

// ---------------------------------------------------------------------------
// World layout constants. CHANGES TO THESE PROPAGATE INTO EVERY SHADER —
// modify `params` uniform layout and shader constants together.
// ---------------------------------------------------------------------------
const BOX_HALF_EXTENT = 16.0;       // Box is [-16, 16]^3, edge length 32.
const CELL_SIZE       = 4.0;        // = max(allNeighborhoodRadii); see § 2.6.
const GRID_DIM        = 8;          // 32u / 4u = 8 cells per axis.
const CELL_COUNT      = GRID_DIM * GRID_DIM * GRID_DIM;     // 512
const CELL_PREFIX_BLOCK_SIZE = 256; // multi-block scan workgroup size

// ---------------------------------------------------------------------------
// Per-entity / per-buffer byte sizes. Match WGSL struct layouts byte-for-byte;
// changing any of these requires updating the corresponding shader struct.
// ---------------------------------------------------------------------------
const ENTITY_BYTES         = 32;    // vec3 pos + u32 species + vec3 vel + f32 _pad
const PREDATOR_STATE_BYTES = 16;    // u32 target_boid_id + u32 target_age_frames + 2 u32 pad
const LEADER_BYTES         = 16;    // vec3 position + f32 strength
const MAX_LEADERS          = 32;    // mirrors physarum MAX_PINS
const PARAMS_BYTES         = 160;   // see uploadParams() layout — three trailing vec3s (boidColor/predatorColor/leaderColor) each pad to 16 bytes per WGSL std-uniform rules; struct ends at byte 160
const CAMERA_UNIFORM_SIZE  = 96;    // see writeCameraUniform() layout
const MAX_DPR              = 2.0;
const DEFAULT_SEED         = 0xB01D5;

// ---------------------------------------------------------------------------
// Discrete agent-count tiers. 50k default sized for 60 fps on RX 6800 XT and
// 2080 Ti while keeping the 25k tier viable on integrated GPUs. 100k tier is
// the hero stretch with degradation contract per § 2.6.
// ---------------------------------------------------------------------------
const AGENT_COUNT_TIERS = {
    '25k':  { boids:  25_000, predators:  250 },
    '50k':  { boids:  50_000, predators:  500 },
    '75k':  { boids:  75_000, predators:  750 },
    '100k': { boids: 100_000, predators: 1000 },
} as const satisfies Record<string, { boids: number; predators: number }>;

const MAX_BOIDS     = AGENT_COUNT_TIERS['100k'].boids;
const MAX_PREDATORS = AGENT_COUNT_TIERS['100k'].predators;
const MAX_ENTITIES  = MAX_BOIDS + MAX_PREDATORS;

// ---------------------------------------------------------------------------
// Initial camera pose — looks at the box center from a slightly elevated
// vantage outside the box, giving the user an immediate aerial view of the
// scene on first load.
// ---------------------------------------------------------------------------
const INITIAL_CAMERA_POSITION: [number, number, number] = [22, 16, 22];
const INITIAL_CAMERA_TARGET:   [number, number, number] = [ 0,  0,  0];
const INITIAL_FOV_DEG = 55;

function defaultRuntime(): Runtime {
    const initial = PRESETS['Cohesive Flock'];
    return {
        presetName:        'Cohesive Flock',
        agentCountTier:    '50k',

        separationRadius:  initial.separationRadius,
        separationWeight:  initial.separationWeight,
        alignmentRadius:   initial.alignmentRadius,
        alignmentWeight:   initial.alignmentWeight,
        cohesionRadius:    initial.cohesionRadius,
        cohesionWeight:    initial.cohesionWeight,
        boidMaxSpeed:      initial.boidMaxSpeed,

        leaders:               [],
        leaderInfluenceRadius: initial.leaderInfluenceRadius,
        leaderStrength:        initial.leaderStrength,

        predatorMode:            initial.predatorMode,
        predatorFleeRadius:      initial.predatorFleeRadius,
        predatorFleeStrength:    initial.predatorFleeStrength,
        predatorDetectionRadius: initial.predatorDetectionRadius,
        predatorRePickFrames:    initial.predatorRePickFrames,
        predatorSpeedMul:        initial.predatorSpeedMul,

        boidColor:     [0.40, 0.85, 0.95],
        predatorColor: [0.95, 0.25, 0.20],
        leaderColor:   [0.95, 0.95, 0.85],
        boidScale:     0.20,
        lightYawDeg:   45,
        lightPitchDeg: 35,
        ambient:       0.25,

        initSeed:    DEFAULT_SEED,
        iteration:   0,
        autoResetOnPresetChange: true,
        autoResetOnTierChange:   true,
    };
}

async function main(): Promise<void> {
    initLogger();
    log.info('boids-3d: starting up');

    const canvas = document.getElementById('canvas') as HTMLCanvasElement | null;
    const hud    = document.getElementById('hud')    as HTMLDivElement    | null;
    if (!canvas) throw new Error('no #canvas element');
    if (!hud)    throw new Error('no #hud element');

    if (!('gpu' in navigator)) {
        hud.textContent = 'WebGPU not available in this browser.';
        return;
    }

    // Boids-3d's unified entity buffer at hero tier (101k × 32 B = 3.232 MB) is
    // well below baseline maxStorageBufferBindingSize (128 MiB). No requiredLimits
    // raise is needed — explicit divergence from physarum which needs 200 MB at
    // its 10M tier.
    const ctx = await Context.create({
        canvas,
        powerPreference: 'high-performance',
    });
    const renderer = new Renderer(ctx);
    const device = ctx.device;
    const presentationFormat = ctx.preferredFormat;

    // Disable browser context menu so RMB can drive camera look without being
    // hijacked. Same posture physarum took (where RMB drove pin removal); the
    // mechanism is identical, the consumer is different.
    canvas.addEventListener('contextmenu', (e) => { e.preventDefault(); });

    // ----- Canvas + DPR sizing ---------------------------------------------
    const targetDimensions = (): { width: number; height: number } => {
        const dpr = Math.min(window.devicePixelRatio || 1, MAX_DPR);
        return {
            width:  Math.max(1, Math.floor(window.innerWidth  * dpr)),
            height: Math.max(1, Math.floor(window.innerHeight * dpr)),
        };
    };
    {
        const dim = targetDimensions();
        canvas.width  = dim.width;
        canvas.height = dim.height;
    }

    // ----- Camera (3D free-fly) --------------------------------------------
    const camera = new Camera();
    camera.mode      = 'free-fly';
    camera.position  = INITIAL_CAMERA_POSITION;
    camera.fovDeg    = INITIAL_FOV_DEG;
    camera.near      = 0.1;
    camera.far       = 200.0;
    camera.moveSpeed = 8.0;
    camera.lookSpeed = 0.18;
    camera.lookAt(INITIAL_CAMERA_TARGET[0], INITIAL_CAMERA_TARGET[1], INITIAL_CAMERA_TARGET[2]);

    // snapshotInput(canvas) returns a thunk; invoke per-frame to get the current snapshot.
    const inputSnapshot = snapshotInput(canvas);

    const rt: Runtime = defaultRuntime();

    // ----- Persistent (tier-independent) buffers ---------------------------
    const leadersBuf = new Buffer(ctx, {
        size: MAX_LEADERS * LEADER_BYTES,
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'boids-3d-leaders',
    });
    const paramsBuf = new Buffer(ctx, {
        size: PARAMS_BYTES,
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'boids-3d-params',
    });
    const cameraUniformBuf = new Buffer(ctx, {
        size: CAMERA_UNIFORM_SIZE,
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'boids-3d-camera',
    });

    // ----- Depth attachment (recreated on canvas resize) --------------------
    let depthTexture: Texture = new Texture(ctx, {
        type: TextureType.e2D,
        extent: { width: canvas.width, height: canvas.height, depthOrArrayLayers: 1 },
        format: 'depth24plus',
        usage: GPUTextureUsage.RENDER_ATTACHMENT,
        label: 'boids-3d-depth',
    });
    const recreateDepth = (): void => {
        depthTexture.destroy();
        depthTexture = new Texture(ctx, {
            type: TextureType.e2D,
            extent: { width: canvas.width, height: canvas.height, depthOrArrayLayers: 1 },
            format: 'depth24plus',
            usage: GPUTextureUsage.RENDER_ATTACHMENT,
            label: 'boids-3d-depth',
        });
    };

    // ----- Tier-dependent resources ----------------------------------------
    const makeEntityBuf = (label: string): Buffer => new Buffer(ctx, {
        size: MAX_ENTITIES * ENTITY_BYTES,
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST | GPUBufferUsage.COPY_SRC,
        memory: MemoryUsage.DeviceLocal,
        label,
    });
    const makePredatorStateBuf = (): Buffer => new Buffer(ctx, {
        size: MAX_PREDATORS * PREDATOR_STATE_BYTES,
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST | GPUBufferUsage.COPY_SRC,
        memory: MemoryUsage.DeviceLocal,
        label: 'boids-3d-predator-state',
    });
    const makeCellBuf = (label: string, sizeU32: number): Buffer => new Buffer(ctx, {
        size: sizeU32 * 4,
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label,
    });

    // entityBufA / entityBufB are the ping-pong pair holding the unified entity
    // state ([0, boidCount) = boids, [boidCount, entityCount) = predators).
    let entityBufA = makeEntityBuf('boids-3d-entity-A');
    let entityBufB = makeEntityBuf('boids-3d-entity-B');
    let predatorStateBuf = makePredatorStateBuf();
    let cellCountsBuf    = makeCellBuf('boids-3d-cell-counts',     CELL_COUNT);
    let cellStartsBuf    = makeCellBuf('boids-3d-cell-starts',     CELL_COUNT + 1);   // +1 sentinel slot at [CELL_COUNT] holds entityCount
    let scratchCounterBuf = makeCellBuf('boids-3d-scratch-counter', CELL_COUNT);
    let sortedIndicesBuf = makeCellBuf('boids-3d-sorted-indices',  MAX_ENTITIES);
    let blockSumsBuf     = makeCellBuf('boids-3d-block-sums',      Math.ceil(CELL_COUNT / CELL_PREFIX_BLOCK_SIZE));

    // pingIsTarget == true: next frame's integrate writes into entityBufB; entityBufA
    // holds latest "new" state at frame start (read by flock_update / predator_update).
    // Initially false: integrate writes into entityBufA on the first frame, after
    // reseed has populated entityBufB as the initial "old" state.
    let pingIsTarget = false;

    // ----- Compute pipelines (8 total) -------------------------------------
    const cellCountPipe = await ComputePipeline.create(ctx, {
        source: cellCountWgsl,
        shaderPath: HMR_CELL_COUNT,
        entryPoint: 'cs_main',
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },
            { binding: 2, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
        ],
        label: 'boids-3d-cell-count',
    });

    const prefixLocalPipe = await ComputePipeline.create(ctx, {
        source: prefixLocalWgsl,
        shaderPath: HMR_PREFIX_LOCAL,
        entryPoint: 'cs_main',
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
        ],
        label: 'boids-3d-prefix-local',
    });

    const prefixBlockPipe = await ComputePipeline.create(ctx, {
        source: prefixBlockWgsl,
        shaderPath: HMR_PREFIX_BLOCK,
        entryPoint: 'cs_main',
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
        ],
        label: 'boids-3d-prefix-block',
    });

    const prefixAddbackPipe = await ComputePipeline.create(ctx, {
        source: prefixAddbackWgsl,
        shaderPath: HMR_PREFIX_ADDBACK,
        entryPoint: 'cs_main',
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },
            { binding: 2, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
        ],
        label: 'boids-3d-prefix-addback',
    });

    const scatterPipe = await ComputePipeline.create(ctx, {
        source: scatterWgsl,
        shaderPath: HMR_SCATTER,
        entryPoint: 'cs_main',
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },
            { binding: 2, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },
            { binding: 3, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 4, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
        ],
        label: 'boids-3d-scatter',
    });

    const flockUpdatePipe = await ComputePipeline.create(ctx, {
        source: flockUpdateWgsl,
        shaderPath: HMR_FLOCK_UPDATE,
        entryPoint: 'cs_main',
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },  // old entities
            { binding: 2, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },            // new entities
            { binding: 3, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },  // sorted_indices
            { binding: 4, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },  // cell_starts
            { binding: 5, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },  // leaders
        ],
        label: 'boids-3d-flock-update',
    });

    const predatorUpdatePipe = await ComputePipeline.create(ctx, {
        source: predatorUpdateWgsl,
        shaderPath: HMR_PREDATOR_UPDATE,
        entryPoint: 'cs_main',
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },  // old entities
            { binding: 2, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },            // new entities
            { binding: 3, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },  // sorted_indices
            { binding: 4, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },  // cell_starts
            { binding: 5, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },            // predator_state
        ],
        label: 'boids-3d-predator-update',
    });

    const integratePipe = await ComputePipeline.create(ctx, {
        source: integrateWgsl,
        shaderPath: HMR_INTEGRATE,
        entryPoint: 'cs_main',
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },  // read_write — entity buffer post-flock+predator, integrated in place per-slot
        ],
        label: 'boids-3d-integrate',
    });

    // ----- Render pipelines (4 total) --------------------------------------
    // depthStencil config: background draws first with depth disabled (and no
    // depth-write); the other three pipelines depth-test 'less' and depth-write.
    const depthStencilOpaque: GPUDepthStencilState = {
        format: 'depth24plus',
        depthCompare: 'less',
        depthWriteEnabled: true,
    };
    const depthStencilDisabled: GPUDepthStencilState = {
        format: 'depth24plus',
        depthCompare: 'always',
        depthWriteEnabled: false,
    };

    const backgroundPipe = await RenderPipeline.create(ctx, {
        vertexSource:   bgVert,
        fragmentSource: bgFrag,
        vertexPath:     HMR_BG_VERT,
        fragmentPath:   HMR_BG_FRAG,
        bindings: [],
        colorFormats: [presentationFormat],
        primitive:    { topology: 'triangle-list' },
        depthStencil: depthStencilDisabled,
        label: 'boids-3d-background',
    });

    const wireframePipe = await RenderPipeline.create(ctx, {
        vertexSource:   wireframeVert,
        fragmentSource: wireframeFrag,
        vertexPath:     HMR_WIREFRAME_VERT,
        fragmentPath:   HMR_WIREFRAME_FRAG,
        bindings: [
            { binding: 0, visibility: GPUShaderStage.VERTEX,   buffer: { type: 'uniform' } },
        ],
        colorFormats: [presentationFormat],
        primitive:    { topology: 'line-list' },
        depthStencil: depthStencilOpaque,
        label: 'boids-3d-wireframe',
    });

    const boidPipe = await RenderPipeline.create(ctx, {
        vertexSource:   boidVert,
        fragmentSource: boidFrag,
        vertexPath:     HMR_BOID_VERT,
        fragmentPath:   HMR_BOID_FRAG,
        bindings: [
            { binding: 0, visibility: GPUShaderStage.VERTEX | GPUShaderStage.FRAGMENT, buffer: { type: 'uniform' } },           // camera (vertex: viewProj; fragment: lightDir/ambient)
            { binding: 1, visibility: GPUShaderStage.VERTEX | GPUShaderStage.FRAGMENT, buffer: { type: 'uniform' } },           // params (boidScale, colors)
            { binding: 2, visibility: GPUShaderStage.VERTEX,                            buffer: { type: 'read-only-storage' } }, // entity buffer (post-integrate)
        ],
        colorFormats: [presentationFormat],
        primitive:    { topology: 'triangle-list', cullMode: 'none' },
        depthStencil: depthStencilOpaque,
        label: 'boids-3d-boid',
    });

    const leaderPipe = await RenderPipeline.create(ctx, {
        vertexSource:   leaderVert,
        fragmentSource: leaderFrag,
        vertexPath:     HMR_LEADER_VERT,
        fragmentPath:   HMR_LEADER_FRAG,
        bindings: [
            { binding: 0, visibility: GPUShaderStage.VERTEX | GPUShaderStage.FRAGMENT, buffer: { type: 'uniform' } },           // camera (vertex: viewProj; fragment: lightDir/ambient)
            { binding: 1, visibility: GPUShaderStage.VERTEX | GPUShaderStage.FRAGMENT, buffer: { type: 'uniform' } },           // params (leaderColor)
            { binding: 2, visibility: GPUShaderStage.VERTEX,                            buffer: { type: 'read-only-storage' } }, // leaders buffer
        ],
        colorFormats: [presentationFormat],
        primitive:    { topology: 'triangle-list', cullMode: 'back' },
        depthStencil: depthStencilOpaque,
        label: 'boids-3d-leader',
    });

    // ----- Bind groups (rebuilt on tier change for the entity-buffer-bound ones)
    let bgCellCountReadA:  GPUBindGroup;
    let bgCellCountReadB:  GPUBindGroup;
    let bgPrefixLocal:     GPUBindGroup;
    let bgPrefixBlock:     GPUBindGroup;
    let bgPrefixAddback:   GPUBindGroup;
    let bgScatterReadA:    GPUBindGroup;
    let bgScatterReadB:    GPUBindGroup;
    let bgFlockUpdateAB:   GPUBindGroup;  // read A, write B
    let bgFlockUpdateBA:   GPUBindGroup;  // read B, write A
    let bgPredatorUpdateAB: GPUBindGroup;
    let bgPredatorUpdateBA: GPUBindGroup;
    let bgIntegrateAB:     GPUBindGroup;  // read+write B (the just-written buffer)
    let bgIntegrateBA:     GPUBindGroup;  // read+write A
    let bgWireframe:       GPUBindGroup;
    let bgBoidA:           GPUBindGroup;  // boid render reads A as final entity buffer
    let bgBoidB:           GPUBindGroup;
    let bgLeader:          GPUBindGroup;

    // The three-step kernel chain per frame is:
    //   flock_update    : reads OLD, writes NEW
    //   predator_update : reads OLD, writes NEW (same NEW; per-slot writes only,
    //                     so flock writes [0,boidCount) and predator writes
    //                     [boidCount,entityCount) — non-overlapping)
    //   integrate       : reads the JUST-WRITTEN NEW (now treated as input),
    //                     writes back into the SAME slot of NEW with bounds
    //                     clamping applied. (No additional buffer needed —
    //                     integrate is a pure self-update on each entity's
    //                     velocity-and-position, no neighbor lookup.)
    //
    // Therefore:
    //   - flock_update / predator_update: bind (read=old, write=new)
    //   - integrate:                      bind (read=new-as-old, write=new)
    //                                     i.e. read and write the SAME buffer.
    //                                     Safe because integrate is pure
    //                                     self-update, no cross-slot reads.
    //
    // pingIsTarget == false: this frame's integrate writes into entityBufA;
    //   flock/predator: read B (old), write A (new); integrate: read+write A.
    // pingIsTarget == true: this frame's integrate writes into entityBufB;
    //   flock/predator: read A (old), write B (new); integrate: read+write B.

    const rebuildBindGroups = (): void => {
        bgCellCountReadA = cellCountPipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: entityBufA.handle } },
            { binding: 2, resource: { buffer: cellCountsBuf.handle } },
        ], 'boids-3d-bg-cell-count-A');
        bgCellCountReadB = cellCountPipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: entityBufB.handle } },
            { binding: 2, resource: { buffer: cellCountsBuf.handle } },
        ], 'boids-3d-bg-cell-count-B');

        bgPrefixLocal = prefixLocalPipe.createBindGroup([
            { binding: 0, resource: { buffer: cellCountsBuf.handle } },
            { binding: 1, resource: { buffer: blockSumsBuf.handle } },
        ], 'boids-3d-bg-prefix-local');

        bgPrefixBlock = prefixBlockPipe.createBindGroup([
            { binding: 0, resource: { buffer: blockSumsBuf.handle } },
        ], 'boids-3d-bg-prefix-block');

        bgPrefixAddback = prefixAddbackPipe.createBindGroup([
            { binding: 0, resource: { buffer: cellCountsBuf.handle } },
            { binding: 1, resource: { buffer: blockSumsBuf.handle } },
            { binding: 2, resource: { buffer: cellStartsBuf.handle } },
        ], 'boids-3d-bg-prefix-addback');

        bgScatterReadA = scatterPipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: entityBufA.handle } },
            { binding: 2, resource: { buffer: cellStartsBuf.handle } },
            { binding: 3, resource: { buffer: scratchCounterBuf.handle } },
            { binding: 4, resource: { buffer: sortedIndicesBuf.handle } },
        ], 'boids-3d-bg-scatter-A');
        bgScatterReadB = scatterPipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: entityBufB.handle } },
            { binding: 2, resource: { buffer: cellStartsBuf.handle } },
            { binding: 3, resource: { buffer: scratchCounterBuf.handle } },
            { binding: 4, resource: { buffer: sortedIndicesBuf.handle } },
        ], 'boids-3d-bg-scatter-B');

        // flock_update: read A, write B
        bgFlockUpdateAB = flockUpdatePipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: entityBufA.handle } },
            { binding: 2, resource: { buffer: entityBufB.handle } },
            { binding: 3, resource: { buffer: sortedIndicesBuf.handle } },
            { binding: 4, resource: { buffer: cellStartsBuf.handle } },
            { binding: 5, resource: { buffer: leadersBuf.handle } },
        ], 'boids-3d-bg-flock-AB');
        bgFlockUpdateBA = flockUpdatePipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: entityBufB.handle } },
            { binding: 2, resource: { buffer: entityBufA.handle } },
            { binding: 3, resource: { buffer: sortedIndicesBuf.handle } },
            { binding: 4, resource: { buffer: cellStartsBuf.handle } },
            { binding: 5, resource: { buffer: leadersBuf.handle } },
        ], 'boids-3d-bg-flock-BA');

        bgPredatorUpdateAB = predatorUpdatePipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: entityBufA.handle } },
            { binding: 2, resource: { buffer: entityBufB.handle } },
            { binding: 3, resource: { buffer: sortedIndicesBuf.handle } },
            { binding: 4, resource: { buffer: cellStartsBuf.handle } },
            { binding: 5, resource: { buffer: predatorStateBuf.handle } },
        ], 'boids-3d-bg-predator-AB');
        bgPredatorUpdateBA = predatorUpdatePipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: entityBufB.handle } },
            { binding: 2, resource: { buffer: entityBufA.handle } },
            { binding: 3, resource: { buffer: sortedIndicesBuf.handle } },
            { binding: 4, resource: { buffer: cellStartsBuf.handle } },
            { binding: 5, resource: { buffer: predatorStateBuf.handle } },
        ], 'boids-3d-bg-predator-BA');

        // integrate: single read_write binding on whichever buffer flock+predator just wrote to.
        // The kernel reads entities[i], applies clamping and step, writes back to entities[i].
        // No cross-slot reads, so reflexive read+write at one binding is safe.
        bgIntegrateAB = integratePipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: entityBufB.handle } },
        ], 'boids-3d-bg-integrate-B');
        bgIntegrateBA = integratePipe.createBindGroup([
            { binding: 0, resource: { buffer: paramsBuf.handle } },
            { binding: 1, resource: { buffer: entityBufA.handle } },
        ], 'boids-3d-bg-integrate-A');

        bgWireframe = wireframePipe.createBindGroup([
            { binding: 0, resource: { buffer: cameraUniformBuf.handle } },
        ], 'boids-3d-bg-wireframe');

        bgBoidA = boidPipe.createBindGroup([
            { binding: 0, resource: { buffer: cameraUniformBuf.handle } },
            { binding: 1, resource: { buffer: paramsBuf.handle } },
            { binding: 2, resource: { buffer: entityBufA.handle } },
        ], 'boids-3d-bg-boid-A');
        bgBoidB = boidPipe.createBindGroup([
            { binding: 0, resource: { buffer: cameraUniformBuf.handle } },
            { binding: 1, resource: { buffer: paramsBuf.handle } },
            { binding: 2, resource: { buffer: entityBufB.handle } },
        ], 'boids-3d-bg-boid-B');

        bgLeader = leaderPipe.createBindGroup([
            { binding: 0, resource: { buffer: cameraUniformBuf.handle } },
            { binding: 1, resource: { buffer: paramsBuf.handle } },
            { binding: 2, resource: { buffer: leadersBuf.handle } },
        ], 'boids-3d-bg-leader');
    };

    rebuildBindGroups();

    const recreateTierResources = (): void => {
        entityBufA.destroy();
        entityBufB.destroy();
        predatorStateBuf.destroy();
        cellCountsBuf.destroy();
        cellStartsBuf.destroy();
        scratchCounterBuf.destroy();
        sortedIndicesBuf.destroy();
        blockSumsBuf.destroy();

        entityBufA          = makeEntityBuf('boids-3d-entity-A');
        entityBufB          = makeEntityBuf('boids-3d-entity-B');
        predatorStateBuf    = makePredatorStateBuf();
        cellCountsBuf       = makeCellBuf('boids-3d-cell-counts',     CELL_COUNT);
        cellStartsBuf       = makeCellBuf('boids-3d-cell-starts',     CELL_COUNT + 1);
        scratchCounterBuf   = makeCellBuf('boids-3d-scratch-counter', CELL_COUNT);
        sortedIndicesBuf    = makeCellBuf('boids-3d-sorted-indices',  MAX_ENTITIES);
        blockSumsBuf        = makeCellBuf('boids-3d-block-sums',      Math.ceil(CELL_COUNT / CELL_PREFIX_BLOCK_SIZE));

        pingIsTarget = false;
        rebuildBindGroups();
    };

    // ----- Hot reload subscriptions ----------------------------------------
    const hot = new HotReloader();
    const watchCompute = (path: string, pipe: ComputePipeline): void => {
        hot.watch(path, async (filePath, src) => {
            const err = await pipe.reload(src);
            if (err) hot.reportFailure(filePath, err);
            else     hot.reportSuccess(filePath);
        });
    };
    const watchVert = (path: string, pipe: RenderPipeline): void => {
        hot.watch(path, async (filePath, src) => {
            const err = await pipe.reload('vertex', src);
            if (err) hot.reportFailure(filePath, err);
            else     hot.reportSuccess(filePath);
        });
    };
    const watchFrag = (path: string, pipe: RenderPipeline): void => {
        hot.watch(path, async (filePath, src) => {
            const err = await pipe.reload('fragment', src);
            if (err) hot.reportFailure(filePath, err);
            else     hot.reportSuccess(filePath);
        });
    };
    watchCompute(HMR_CELL_COUNT,      cellCountPipe);
    watchCompute(HMR_PREFIX_LOCAL,    prefixLocalPipe);
    watchCompute(HMR_PREFIX_BLOCK,    prefixBlockPipe);
    watchCompute(HMR_PREFIX_ADDBACK,  prefixAddbackPipe);
    watchCompute(HMR_SCATTER,         scatterPipe);
    watchCompute(HMR_FLOCK_UPDATE,    flockUpdatePipe);
    watchCompute(HMR_PREDATOR_UPDATE, predatorUpdatePipe);
    watchCompute(HMR_INTEGRATE,       integratePipe);
    watchVert(HMR_BG_VERT,        backgroundPipe);
    watchFrag(HMR_BG_FRAG,        backgroundPipe);
    watchVert(HMR_WIREFRAME_VERT, wireframePipe);
    watchFrag(HMR_WIREFRAME_FRAG, wireframePipe);
    watchVert(HMR_BOID_VERT,      boidPipe);
    watchFrag(HMR_BOID_FRAG,      boidPipe);
    watchVert(HMR_LEADER_VERT,    leaderPipe);
    watchFrag(HMR_LEADER_FRAG,    leaderPipe);

    // ----- Initial state upload --------------------------------------------
    // Reseed entities CPU-side using xorshift32(initSeed) so that
    // (initSeed, agentCountTier) → bit-identical entity layout. Predator state
    // is reset to sentinel values (target_boid_id = 0xFFFFFFFF, target_age = 0).
    // All entities are uploaded into BOTH entity buffers — the first frame's
    // flock_update reads from whichever is "old" (entityBufB when pingIsTarget
    // starts at false), so both must hold valid initial state to avoid a
    // first-frame zeroed-buffer read that would scatter entities arbitrarily.
    const reseedEntities = (): void => {
        const tier = AGENT_COUNT_TIERS[rt.agentCountTier];
        const total = tier.boids + tier.predators;
        const ab = new ArrayBuffer(MAX_ENTITIES * ENTITY_BYTES);
        const f32 = new Float32Array(ab);
        const u32 = new Uint32Array(ab);
        let s = (rt.initSeed >>> 0) || 1;
        const r01 = (): number => {
            s ^= (s << 13);
            s ^= (s >>> 17);
            s ^= (s << 5);
            return (s >>> 0) / 4294967296;
        };
        const r11 = (): number => r01() * 2 - 1;
        for (let i = 0; i < total; i++) {
            const off = i * 8;
            f32[off + 0] = r11() * BOX_HALF_EXTENT * 0.95;       // pos.x
            f32[off + 1] = r11() * BOX_HALF_EXTENT * 0.95;       // pos.y
            f32[off + 2] = r11() * BOX_HALF_EXTENT * 0.95;       // pos.z
            u32[off + 3] = (i < tier.boids) ? 0 : 1;             // species
            f32[off + 4] = r11() * 1.0;                           // vel.x
            f32[off + 5] = r11() * 1.0;                           // vel.y
            f32[off + 6] = r11() * 1.0;                           // vel.z
            f32[off + 7] = 0.0;                                   // _pad
        }
        // Tail (entries beyond `total` and up to MAX_ENTITIES) is zero-initialized
        // from ArrayBuffer construction. Species index 0 in the tail is harmless
        // since the dispatch only covers [0, total); kernels never read tail.
        entityBufA.uploadDirect(ab);
        entityBufB.uploadDirect(ab);
        rt.iteration = 0;
        pingIsTarget = false;
    };

    const resetPredatorState = (): void => {
        const ab = new ArrayBuffer(MAX_PREDATORS * PREDATOR_STATE_BYTES);
        const u32 = new Uint32Array(ab);
        for (let i = 0; i < MAX_PREDATORS; i++) {
            u32[i * 4 + 0] = 0xFFFFFFFF;   // target_boid_id sentinel
            u32[i * 4 + 1] = 0;            // target_age_frames
            u32[i * 4 + 2] = 0;            // _pad
            u32[i * 4 + 3] = 0;            // _pad
        }
        predatorStateBuf.uploadDirect(ab);
    };

    const uploadLeaders = (): void => {
        const ab = new ArrayBuffer(MAX_LEADERS * LEADER_BYTES);
        const f32 = new Float32Array(ab);
        for (let i = 0; i < rt.leaders.length; i++) {
            const l = rt.leaders[i]!;
            f32[i * 4 + 0] = l.position[0];
            f32[i * 4 + 1] = l.position[1];
            f32[i * 4 + 2] = l.position[2];
            f32[i * 4 + 3] = l.strength;
        }
        leadersBuf.uploadDirect(ab);
    };

    const paramsBytes = new ArrayBuffer(PARAMS_BYTES);
    const paramsU32 = new Uint32Array(paramsBytes);
    const paramsF32 = new Float32Array(paramsBytes);
    const PREDATOR_MODE_INDEX: Record<PredatorMode, number> = {
        'nearest-prey':    0,
        'stochastic-prey': 1,
        'flock-center':    2,
    };
    const uploadParams = (): void => {
        const tier = AGENT_COUNT_TIERS[rt.agentCountTier];
        const boidCount = tier.boids;
        const predCount = tier.predators;
        const totalCount = boidCount + predCount;

        // Layout (PARAMS_BYTES = 160; matches WGSL Params struct in shaders).
        // WGSL vec3<f32> in uniform memory has @align(16) @size(12) — three trailing
        // vec3 colors each pad to 16 bytes (boidColor, predatorColor, leaderColor),
        // pushing the total struct size to 160. f32 indices [27, 31, 35, 39] are
        // padding slots; written values would be ignored, so they're left zero.
        paramsU32[0]  = boidCount;
        paramsU32[1]  = predCount;
        paramsU32[2]  = totalCount;
        paramsU32[3]  = rt.iteration;
        paramsU32[4]  = CELL_COUNT;
        paramsU32[5]  = GRID_DIM;
        paramsF32[6]  = CELL_SIZE;
        paramsF32[7]  = BOX_HALF_EXTENT;
        paramsF32[8]  = rt.separationRadius;
        paramsF32[9]  = rt.separationWeight;
        paramsF32[10] = rt.alignmentRadius;
        paramsF32[11] = rt.alignmentWeight;
        paramsF32[12] = rt.cohesionRadius;
        paramsF32[13] = rt.cohesionWeight;
        paramsF32[14] = rt.boidMaxSpeed;
        paramsU32[15] = rt.leaders.length;
        paramsF32[16] = rt.leaderInfluenceRadius;
        paramsF32[17] = rt.leaderStrength;
        paramsU32[18] = PREDATOR_MODE_INDEX[rt.predatorMode];
        paramsF32[19] = rt.predatorFleeRadius;
        paramsF32[20] = rt.predatorFleeStrength;
        paramsF32[21] = rt.predatorDetectionRadius;
        paramsU32[22] = rt.predatorRePickFrames;
        paramsF32[23] = rt.predatorSpeedMul;
        paramsF32[24] = rt.boidScale;
        // f32[25..27] left zero
        paramsF32[28] = rt.boidColor[0];
        paramsF32[29] = rt.boidColor[1];
        paramsF32[30] = rt.boidColor[2];
        // f32[31] = _pad
        paramsF32[32] = rt.predatorColor[0];
        paramsF32[33] = rt.predatorColor[1];
        paramsF32[34] = rt.predatorColor[2];
        // f32[35] = _pad
        paramsF32[36] = rt.leaderColor[0];
        paramsF32[37] = rt.leaderColor[1];
        paramsF32[38] = rt.leaderColor[2];
        // f32[39] = _pad
        paramsBuf.uploadDirect(paramsBytes);
    };

    const camBytes = new ArrayBuffer(CAMERA_UNIFORM_SIZE);
    const camF32 = new Float32Array(camBytes);
    const writeCameraUniform = (): void => {
        camera.aspect = canvas.width / Math.max(canvas.height, 1);
        const vp = camera.viewProjection();
        for (let i = 0; i < 16; i++) camF32[i] = vp[i]!;
        camF32[16] = camera.position[0]!;
        camF32[17] = camera.position[1]!;
        camF32[18] = camera.position[2]!;
        // camF32[19] = _pad0
        const lyaw   = (rt.lightYawDeg   * Math.PI) / 180;
        const lpitch = (rt.lightPitchDeg * Math.PI) / 180;
        camF32[20] = Math.cos(lpitch) * Math.cos(lyaw);
        camF32[21] = Math.sin(lpitch);
        camF32[22] = Math.cos(lpitch) * Math.sin(lyaw);
        camF32[23] = rt.ambient;
        cameraUniformBuf.uploadDirect(camBytes);
    };

    reseedEntities();
    resetPredatorState();
    uploadLeaders();

    // ----- ParamPanel ------------------------------------------------------
    const panel = new ParamPanel({ title: 'Boids 3D', persistKey: 'boids-3d' });

    const presetCtrl = panel.addDropdown({
        label: 'Preset',
        getValue: () => rt.presetName,
        setValue: (v: string) => {
            rt.presetName = v as PresetName | 'Custom';
            if (v in PRESETS) {
                const p = PRESETS[v as PresetName];
                rt.separationRadius      = p.separationRadius;
                rt.separationWeight      = p.separationWeight;
                rt.alignmentRadius       = p.alignmentRadius;
                rt.alignmentWeight       = p.alignmentWeight;
                rt.cohesionRadius        = p.cohesionRadius;
                rt.cohesionWeight        = p.cohesionWeight;
                rt.boidMaxSpeed          = p.boidMaxSpeed;
                rt.leaderInfluenceRadius = p.leaderInfluenceRadius;
                rt.leaderStrength        = p.leaderStrength;
                rt.predatorMode            = p.predatorMode;
                rt.predatorFleeRadius      = p.predatorFleeRadius;
                rt.predatorFleeStrength    = p.predatorFleeStrength;
                rt.predatorDetectionRadius = p.predatorDetectionRadius;
                rt.predatorRePickFrames    = p.predatorRePickFrames;
                rt.predatorSpeedMul        = p.predatorSpeedMul;
                resetPredatorState();   // mode-switch reset on preset change
                if (rt.autoResetOnPresetChange) reseedEntities();
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
        label: 'Scale tier',
        getValue: () => rt.agentCountTier,
        setValue: (v: string) => {
            rt.agentCountTier = v as AgentCountTier;
            recreateTierResources();
            reseedEntities();
            resetPredatorState();
            panel.refreshDisplays();
        },
        options: Object.keys(AGENT_COUNT_TIERS).map((k) => ({
            label: `${k} (${AGENT_COUNT_TIERS[k as AgentCountTier].boids.toLocaleString()} boids + ${AGENT_COUNT_TIERS[k as AgentCountTier].predators} predators)`,
            value: k,
        })),
    });

    const reynoldsFolder = panel.addFolder('Reynolds rules');
    reynoldsFolder.addNumber(rt, 'separationRadius', 0.1, 4.0, 0.05).name('Separation radius').onChange(onCustomTouch);
    reynoldsFolder.addNumber(rt, 'separationWeight', 0.0, 5.0, 0.05).name('Separation weight').onChange(onCustomTouch);
    reynoldsFolder.addNumber(rt, 'alignmentRadius',  0.1, 4.0, 0.05).name('Alignment radius').onChange(onCustomTouch);
    reynoldsFolder.addNumber(rt, 'alignmentWeight',  0.0, 5.0, 0.05).name('Alignment weight').onChange(onCustomTouch);
    reynoldsFolder.addNumber(rt, 'cohesionRadius',   0.1, 4.0, 0.05).name('Cohesion radius').onChange(onCustomTouch);
    reynoldsFolder.addNumber(rt, 'cohesionWeight',   0.0, 5.0, 0.05).name('Cohesion weight').onChange(onCustomTouch);
    reynoldsFolder.addNumber(rt, 'boidMaxSpeed',     0.5, 6.0, 0.05).name('Max speed').onChange(onCustomTouch);

    const leaderFolder = panel.addFolder('Leaders');
    leaderFolder.addNumber(rt, 'leaderInfluenceRadius', 0.5, 4.0, 0.05).name('Influence radius').onChange(onCustomTouch);
    leaderFolder.addNumber(rt, 'leaderStrength',        0.0, 5.0, 0.05).name('Strength').onChange(onCustomTouch);
    leaderFolder.addButton('Clear all leaders', () => { rt.leaders = []; uploadLeaders(); });

    const predatorFolder = panel.addFolder('Predators');
    predatorFolder.addDropdown({
        label: 'Hunting mode',
        getValue: () => rt.predatorMode,
        setValue: (v: string) => {
            rt.predatorMode = v as PredatorMode;
            resetPredatorState();   // mode-switch reset on user dropdown change
            onCustomTouch();
            panel.refreshDisplays();
        },
        options: [
            { label: 'Nearest prey',    value: 'nearest-prey'    },
            { label: 'Stochastic prey', value: 'stochastic-prey' },
            { label: 'Flock center',    value: 'flock-center'    },
        ],
    });
    predatorFolder.addNumber(rt, 'predatorFleeRadius',      0.2, 4.0, 0.05).name('Flee radius').onChange(onCustomTouch);
    predatorFolder.addNumber(rt, 'predatorFleeStrength',    0.0, 8.0, 0.1).name('Flee strength').onChange(onCustomTouch);
    predatorFolder.addNumber(rt, 'predatorDetectionRadius', 0.2, 4.0, 0.05).name('Detection radius').onChange(onCustomTouch);
    predatorFolder.addNumber(rt, 'predatorRePickFrames',    30,  300, 1).name('Re-pick frames (stochastic)').onChange(onCustomTouch);
    predatorFolder.addNumber(rt, 'predatorSpeedMul',        0.8, 2.5, 0.05).name('Speed multiplier').onChange(onCustomTouch);

    const vizFolder = panel.addFolder('Visualization');
    vizFolder.addColor(rt,  'boidColor').name('Boid color');
    vizFolder.addColor(rt,  'predatorColor').name('Predator color');
    vizFolder.addColor(rt,  'leaderColor').name('Leader color');
    vizFolder.addNumber(rt, 'boidScale',     0.05, 0.6,  0.01).name('Boid scale');
    vizFolder.addNumber(rt, 'lightYawDeg',   0.0,  360.0, 1.0).name('Light yaw');
    vizFolder.addNumber(rt, 'lightPitchDeg', -89.0, 89.0, 1.0).name('Light pitch');
    vizFolder.addNumber(rt, 'ambient',       0.0,  1.0,  0.01).name('Ambient');

    const cameraFolder = panel.addFolder('Camera');
    cameraFolder.addNumber(camera, 'fovDeg',    20.0, 110.0, 1.0).name('FOV');
    cameraFolder.addNumber(camera, 'moveSpeed', 1.0,  30.0,  0.5).name('Move speed');
    cameraFolder.addNumber(camera, 'lookSpeed', 0.05, 0.5,   0.01).name('Look speed');
    cameraFolder.addButton('Reset camera', () => {
        camera.position = INITIAL_CAMERA_POSITION;
        camera.lookAt(INITIAL_CAMERA_TARGET[0], INITIAL_CAMERA_TARGET[1], INITIAL_CAMERA_TARGET[2]);
        panel.refreshDisplays();
    });

    const seedFolder = panel.addFolder('Seed / Reset');
    seedFolder.addNumber(rt, 'initSeed', 0, 0xFFFFFFFF, 1).name('Init seed');
    seedFolder.addBoolean(rt, 'autoResetOnPresetChange').name('Auto-reset on preset');
    seedFolder.addBoolean(rt, 'autoResetOnTierChange').name('Auto-reset on tier');
    seedFolder.addButton('Reseed', () => { reseedEntities(); resetPredatorState(); });

    panel.addButton('Save (F5)',    () => { void doSave(); });
    panel.addButton('Load... (F9)', () => { triggerFileLoad(); });

    // ----- Capture / Load --------------------------------------------------
    const stateWriter = new StateWriter('captures');
    let nextCapture = 0;

    interface CaptureMeta {
        presetName:           Runtime['presetName'];
        agentCountTier:       AgentCountTier;
        boidCount:            number;
        predatorCount:        number;
        iteration:            number;
        initSeed:             number;
        separationRadius:     number;
        separationWeight:     number;
        alignmentRadius:      number;
        alignmentWeight:      number;
        cohesionRadius:       number;
        cohesionWeight:       number;
        boidMaxSpeed:         number;
        leaders:              Array<{ position: [number, number, number]; strength: number }>;
        leaderInfluenceRadius: number;
        leaderStrength:       number;
        predatorMode:         PredatorMode;
        predatorFleeRadius:   number;
        predatorFleeStrength: number;
        predatorDetectionRadius: number;
        predatorRePickFrames: number;
        predatorSpeedMul:     number;
        boidColor:            [number, number, number];
        predatorColor:        [number, number, number];
        leaderColor:          [number, number, number];
        boidScale:            number;
        lightYawDeg:          number;
        lightPitchDeg:        number;
        ambient:              number;
        camera:               JsonObject;
    }

    const captureMeta = (): CaptureMeta => {
        const tier = AGENT_COUNT_TIERS[rt.agentCountTier];
        return {
            presetName:           rt.presetName,
            agentCountTier:       rt.agentCountTier,
            boidCount:            tier.boids,
            predatorCount:        tier.predators,
            iteration:            rt.iteration,
            initSeed:             rt.initSeed,
            separationRadius:     rt.separationRadius,
            separationWeight:     rt.separationWeight,
            alignmentRadius:      rt.alignmentRadius,
            alignmentWeight:      rt.alignmentWeight,
            cohesionRadius:       rt.cohesionRadius,
            cohesionWeight:       rt.cohesionWeight,
            boidMaxSpeed:         rt.boidMaxSpeed,
            leaders:              rt.leaders.map((l) => ({ position: [l.position[0], l.position[1], l.position[2]] as [number, number, number], strength: l.strength })),
            leaderInfluenceRadius: rt.leaderInfluenceRadius,
            leaderStrength:       rt.leaderStrength,
            predatorMode:         rt.predatorMode,
            predatorFleeRadius:   rt.predatorFleeRadius,
            predatorFleeStrength: rt.predatorFleeStrength,
            predatorDetectionRadius: rt.predatorDetectionRadius,
            predatorRePickFrames: rt.predatorRePickFrames,
            predatorSpeedMul:     rt.predatorSpeedMul,
            boidColor:            [rt.boidColor[0], rt.boidColor[1], rt.boidColor[2]],
            predatorColor:        [rt.predatorColor[0], rt.predatorColor[1], rt.predatorColor[2]],
            leaderColor:          [rt.leaderColor[0], rt.leaderColor[1], rt.leaderColor[2]],
            boidScale:            rt.boidScale,
            lightYawDeg:          rt.lightYawDeg,
            lightPitchDeg:        rt.lightPitchDeg,
            ambient:              rt.ambient,
            camera:               camera.toJson(),
        };
    };

    const doSave = async (): Promise<void> => {
        // Capture source = post-integrate buffer, i.e. the buffer that integrate
        // last wrote to. Per § 2.13's invariant, that is whichever buffer was the
        // "new" buffer during this frame. After end-of-frame pingIsTarget flip,
        // pingIsTarget==true means the post-integrate buffer is now treated as
        // entityBufA (it became "old for next frame"); pingIsTarget==false means
        // it's entityBufB.
        const captureSourceBuf = pingIsTarget ? entityBufA : entityBufB;
        const entityBytes = await captureSourceBuf.readback();
        const predatorBytes = await predatorStateBuf.readback();
        const tier = AGENT_COUNT_TIERS[rt.agentCountTier];
        const totalEntities = tier.boids + tier.predators;
        stateWriter.beginFrame(nextCapture);
        stateWriter.setMeta('boids3d', captureMeta() as unknown as JsonValue);
        stateWriter.saveBuffer('entities', entityBytes.subarray(0, totalEntities * ENTITY_BYTES), {
            count:  totalEntities,
            stride: ENTITY_BYTES,
        });
        stateWriter.saveBuffer('predator_state', predatorBytes.subarray(0, tier.predators * PREDATOR_STATE_BYTES), {
            count:  tier.predators,
            stride: PREDATOR_STATE_BYTES,
        });
        await stateWriter.endFrame();
        log.info(`captured capture_${nextCapture.toString().padStart(4, '0')}.zip`);
        nextCapture++;
    };

    const applyCapture = (
        m: CaptureMeta,
        entityBytes: Uint8Array,
        predatorBytes: Uint8Array,
    ): void => {
        const tierChanged = (rt.agentCountTier !== m.agentCountTier);

        rt.presetName            = m.presetName;
        rt.agentCountTier        = m.agentCountTier;
        rt.iteration             = m.iteration;
        rt.initSeed              = m.initSeed;
        rt.separationRadius      = m.separationRadius;
        rt.separationWeight      = m.separationWeight;
        rt.alignmentRadius       = m.alignmentRadius;
        rt.alignmentWeight       = m.alignmentWeight;
        rt.cohesionRadius        = m.cohesionRadius;
        rt.cohesionWeight        = m.cohesionWeight;
        rt.boidMaxSpeed          = m.boidMaxSpeed;
        rt.leaders               = m.leaders.map((l) => ({
            position: [l.position[0], l.position[1], l.position[2]] as [number, number, number],
            strength: l.strength,
        }));
        rt.leaderInfluenceRadius = m.leaderInfluenceRadius;
        rt.leaderStrength        = m.leaderStrength;
        rt.predatorMode          = m.predatorMode;
        rt.predatorFleeRadius    = m.predatorFleeRadius;
        rt.predatorFleeStrength  = m.predatorFleeStrength;
        rt.predatorDetectionRadius = m.predatorDetectionRadius;
        rt.predatorRePickFrames  = m.predatorRePickFrames;
        rt.predatorSpeedMul      = m.predatorSpeedMul;
        rt.boidColor             = [m.boidColor[0],     m.boidColor[1],     m.boidColor[2]];
        rt.predatorColor         = [m.predatorColor[0], m.predatorColor[1], m.predatorColor[2]];
        rt.leaderColor           = [m.leaderColor[0],   m.leaderColor[1],   m.leaderColor[2]];
        rt.boidScale             = m.boidScale;
        rt.lightYawDeg           = m.lightYawDeg;
        rt.lightPitchDeg         = m.lightPitchDeg;
        rt.ambient               = m.ambient;
        camera.fromJson(m.camera);

        if (tierChanged) {
            recreateTierResources();
        }

        // Upload loaded entity bytes into the buffer that will be the "old" buffer
        // for the next frame's flock_update. With pingIsTarget=false (next frame's
        // integrate writes into entityBufA), flock_update reads entityBufB as old
        // — so loaded entities go into entityBufB.
        // We force pingIsTarget=false on load to make the buffer choice deterministic
        // regardless of capture-time pingIsTarget value.
        pingIsTarget = false;
        const loadInto = entityBufB;
        // Pad the loaded bytes up to MAX_ENTITIES * ENTITY_BYTES — if the loaded
        // capture is at a smaller tier than MAX, the tail must be zeroed (already
        // is, in a fresh ArrayBuffer copy below).
        const padded = new Uint8Array(MAX_ENTITIES * ENTITY_BYTES);
        padded.set(entityBytes.subarray(0, Math.min(entityBytes.length, padded.length)));
        loadInto.uploadDirect(padded);
        // Also overwrite the OTHER buffer (entityBufA) with the same loaded bytes.
        // This frame's flock_update will read entityBufB and write entityBufA.
        // If we don't initialize entityBufA with valid data, the FIRST integrate
        // pass would read from entityBufA (which would still hold pre-load
        // post-integrate state from before the load) and write back to it —
        // that's stale state for one frame. Initializing both buffers ensures
        // the load is bit-exact from frame 1.
        entityBufA.uploadDirect(padded);

        // Predator state restored bit-exactly. NO sentinel reset on load.
        const predPadded = new Uint8Array(MAX_PREDATORS * PREDATOR_STATE_BYTES);
        predPadded.set(predatorBytes.subarray(0, Math.min(predatorBytes.length, predPadded.length)));
        predatorStateBuf.uploadDirect(predPadded);

        uploadLeaders();
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
            const m = cap.meta('boids3d') as unknown as CaptureMeta | undefined;
            if (!m) { log.error('capture missing boids3d meta'); return; }
            const entityBytes   = cap.buffer('entities');
            const predatorBytes = cap.buffer('predator_state');
            if (!entityBytes)   { log.error('capture missing entities buffer'); return; }
            if (!predatorBytes) { log.error('capture missing predator_state buffer'); return; }
            applyCapture(m, entityBytes, predatorBytes);
            log.info(`loaded ${cap.directoryName}`);
        };
        inp.click();
    };

    // ----- Hotkeys ---------------------------------------------------------
    let prevF5 = false, prevF9 = false, prevR = false;
    window.addEventListener('keydown', (e) => {
        if (e.code === 'F5')   { e.preventDefault(); if (!prevF5) { prevF5 = true; void doSave(); } }
        if (e.code === 'F9')   { e.preventDefault(); if (!prevF9) { prevF9 = true; triggerFileLoad(); } }
        if (e.code === 'KeyR') { if (!prevR) { prevR = true; reseedEntities(); resetPredatorState(); } }
    });
    window.addEventListener('keyup', (e) => {
        if (e.code === 'F5')   prevF5 = false;
        if (e.code === 'F9')   prevF9 = false;
        if (e.code === 'KeyR') prevR  = false;
    });

    // ----- Pointer (LMB place leader, Shift+LMB remove nearest) ------------
    const onPointerDown = (e: PointerEvent): void => {
        if (e.button !== 0) return;     // RMB owned by camera-look; middle button ignored
        const rect = canvas.getBoundingClientRect();
        const cx = e.clientX - rect.left;
        const cy = e.clientY - rect.top;
        const hit = unprojectToGroundPlane(cx, cy, rect.width, rect.height, camera);
        if (!hit.hit) return;

        if (e.shiftKey) {
            // Remove nearest leader within 4.0 world-space units.
            let bestIdx = -1, bestD = 4.0 * 4.0;
            for (let i = 0; i < rt.leaders.length; i++) {
                const l = rt.leaders[i]!;
                const dx = l.position[0] - hit.position[0];
                const dy = l.position[1] - hit.position[1];
                const dz = l.position[2] - hit.position[2];
                const d  = dx * dx + dy * dy + dz * dz;
                if (d < bestD) { bestD = d; bestIdx = i; }
            }
            if (bestIdx >= 0) {
                rt.leaders.splice(bestIdx, 1);
                uploadLeaders();
            }
        } else {
            if (rt.leaders.length >= MAX_LEADERS) {
                log.warn(`leader limit (${MAX_LEADERS}) reached`);
                return;
            }
            rt.leaders.push({ position: hit.position, strength: rt.leaderStrength });
            uploadLeaders();
        }
    };
    canvas.addEventListener('pointerdown', onPointerDown);

    // ----- Resize handling (debounced; also recreates depth attachment) ----
    let resizeTimer: number | null = null;
    window.addEventListener('resize', () => {
        if (resizeTimer !== null) clearTimeout(resizeTimer);
        resizeTimer = window.setTimeout(() => {
            const dim = targetDimensions();
            canvas.width  = dim.width;
            canvas.height = dim.height;
            recreateDepth();
            resizeTimer = null;
        }, 100);
    });

    // ----- Per-frame loop --------------------------------------------------
    let lastFrameTime = performance.now();
    let frameTimeMs = 16.7;
    let slowFrameStreak = 0;
    let warnedSlow = false;

    const frame = (): void => {
        const now = performance.now();
        // frameDtMs computed ONCE before lastFrameTime is updated; reused below
        // for both the rolling average and the slow-frame check. Re-reading
        // (now - lastFrameTime) AFTER the assignment would zero out — do not
        // refactor this without preserving the compute-once-reuse pattern.
        const frameDtMs = now - lastFrameTime;
        const dt = frameDtMs / 1000.0;
        lastFrameTime = now;
        frameTimeMs = frameTimeMs * 0.9 + frameDtMs * 0.1;

        // Drive camera from input snapshot.
        const input = inputSnapshot();
        camera.update(dt, input);

        rt.iteration++;
        uploadParams();
        writeCameraUniform();

        const tier = AGENT_COUNT_TIERS[rt.agentCountTier];
        const entityCount = tier.boids + tier.predators;

        const f = renderer.beginFrame();
        const enc = f.encoder;

        // Per-frame ping-pong walk-through (verify against § 2.13 invariants):
        //
        // pingIsTarget = T at frame start ⇒ A is "old", B is "new" target:
        //   cell_count    reads A         (bgCellCountReadA)
        //   scatter       reads A         (bgScatterReadA)
        //   flock_update  reads A, writes B (bgFlockUpdateAB)
        //   predator      reads A, writes B (bgPredatorUpdateAB)
        //   integrate     read_write B    (bgIntegrateAB binds B at binding 1)
        //   boid render   reads B         (bgBoidB)
        //
        // pingIsTarget = F at frame start ⇒ B is "old", A is "new" target:
        //   cell_count    reads B         (bgCellCountReadB)
        //   scatter       reads B         (bgScatterReadB)
        //   flock_update  reads B, writes A (bgFlockUpdateBA)
        //   predator      reads B, writes A (bgPredatorUpdateBA)
        //   integrate     read_write A    (bgIntegrateBA binds A at binding 1)
        //   boid render   reads A         (bgBoidA)
        //
        // End-of-frame flip: pingIsTarget = !pingIsTarget. After the flip,
        // capture-source = pingIsTarget ? A : B picks the just-written buffer.

        // Clear cell counts and scratch counters at start of each frame.
        // (cell_starts and sorted_indices are fully overwritten by the chain below;
        // the sentinel slot at cell_starts[CELL_COUNT] is written explicitly here
        // so the cell-walk loops can use cell_starts[cell + 1u] unconditionally
        // without special-case fallbacks at the last cell.)
        enc.clearBuffer(cellCountsBuf.handle,    0, CELL_COUNT * 4);
        enc.clearBuffer(scratchCounterBuf.handle, 0, CELL_COUNT * 4);
        device.queue.writeBuffer(
            cellStartsBuf.handle,
            CELL_COUNT * 4,
            new Uint32Array([entityCount]).buffer,
        );

        // 1. cell_count: histogram entities into cell_counts.
        const bgCellCount = pingIsTarget ? bgCellCountReadA : bgCellCountReadB;
        cellCountPipe.dispatch(enc, bgCellCount, Math.ceil(entityCount / 256), 1, 1);

        // 2a. prefix_sum_local: in-block exclusive scan over cell_counts.
        prefixLocalPipe.dispatch(enc, bgPrefixLocal, Math.ceil(CELL_COUNT / CELL_PREFIX_BLOCK_SIZE), 1, 1);

        // 2b. prefix_sum_block: scan over the per-block totals.
        prefixBlockPipe.dispatch(enc, bgPrefixBlock, 1, 1, 1);

        // 2c. prefix_sum_addback: add block totals back into per-element prefixes.
        prefixAddbackPipe.dispatch(enc, bgPrefixAddback, Math.ceil(CELL_COUNT / CELL_PREFIX_BLOCK_SIZE), 1, 1);

        // 3. scatter: each entity claims its slot via atomicAdd(scratch[cellIdx]).
        const bgScatter = pingIsTarget ? bgScatterReadA : bgScatterReadB;
        scatterPipe.dispatch(enc, bgScatter, Math.ceil(entityCount / 256), 1, 1);

        // 4. flock_update: per-boid Reynolds + leader pull + predator flee.
        const bgFlock = pingIsTarget ? bgFlockUpdateAB : bgFlockUpdateBA;
        flockUpdatePipe.dispatch(enc, bgFlock, Math.ceil(tier.boids / 256), 1, 1);

        // 5. predator_update: per-predator mode-specific kernel.
        const bgPredator = pingIsTarget ? bgPredatorUpdateAB : bgPredatorUpdateBA;
        predatorUpdatePipe.dispatch(enc, bgPredator, Math.ceil(tier.predators / 256), 1, 1);

        // 6. integrate: clamp velocity, advance position, enforce box bounds.
        // Bind the buffer that flock+predator just wrote to; integrate reads+writes
        // the same buffer (per-slot self-update only, no cross-slot reads).
        const bgIntegrate = pingIsTarget ? bgIntegrateAB : bgIntegrateBA;
        integratePipe.dispatch(enc, bgIntegrate, Math.ceil(entityCount / 256), 1, 1);

        // 7. Render pass: background → wireframe → leaders → boids+predators.
        // NOTE: bypassing Renderer.beginRendering because we need a depth attachment.
        const colorAttachment: GPURenderPassColorAttachment = {
            view: f.canvasView,
            clearValue: { r: 0, g: 0, b: 0, a: 1 },
            loadOp: 'clear',
            storeOp: 'store',
        };
        const depthAttachment: GPURenderPassDepthStencilAttachment = {
            view: depthTexture.view,
            depthClearValue: 1.0,
            depthLoadOp: 'clear',
            depthStoreOp: 'store',
        };
        const renderDesc: GPURenderPassDescriptor = {
            colorAttachments: [colorAttachment],
            depthStencilAttachment: depthAttachment,
        };
        const pass = enc.beginRenderPass(renderDesc);

        // Background: fullscreen triangle, no bind group, depth-disabled.
        // backgroundPipe has zero bindings (its WGSL declares no @group resources),
        // so RenderPipeline.bind() is not used — that helper requires a GPUBindGroup.
        // Instead we set the pipeline directly via the wrapper's public `handle`
        // getter and skip setBindGroup entirely.
        pass.setPipeline(backgroundPipe.handle);
        pass.draw(3, 1, 0, 0);

        // Wireframe: 12 edges × 2 vertices = 24 vertices, line-list topology.
        wireframePipe.bind(pass, bgWireframe);
        pass.draw(24, 1, 0, 0);

        // Leaders: octahedron (8 tris × 3 verts = 24 vertices) × leaderCount instances.
        if (rt.leaders.length > 0) {
            leaderPipe.bind(pass, bgLeader);
            pass.draw(24, rt.leaders.length, 0, 0);
        }

        // Boids + predators: 4-tri pyramid × 12 verts × entityCount instances.
        // After the integrate write, the post-integrate buffer is whichever
        // entity buffer the integrate kernel just wrote to:
        //   pingIsTarget == false: integrate wrote to entityBufA → bind boidBufA
        //   pingIsTarget == true:  integrate wrote to entityBufB → bind boidBufB
        const bgBoid = pingIsTarget ? bgBoidB : bgBoidA;
        boidPipe.bind(pass, bgBoid);
        pass.draw(12, entityCount, 0, 0);

        pass.end();

        renderer.endFrame(f);

        // Flip the ping-pong toggle. After the flip:
        //   - The buffer that integrate wrote to during this frame is now what
        //     next frame's flock_update will read as "old".
        //   - Capture-source = whichever buffer was just-written-to:
        //     pingIsTarget (after flip) == true  → entityBufA was written
        //     pingIsTarget (after flip) == false → entityBufB was written
        pingIsTarget = !pingIsTarget;

        // Slow-frame degradation contract (§ 2.6). 30 fps threshold = 33.3 ms.
        if (frameDtMs > 33.3) {
            slowFrameStreak++;
            if (slowFrameStreak >= 60 && !warnedSlow) {
                warnedSlow = true;
                log.warn(`boids-3d: tier '${rt.agentCountTier}' sustaining <30 fps; consider dropping to a lower tier`);
            }
        } else {
            slowFrameStreak = 0;
        }

        hud.textContent = `${frameTimeMs.toFixed(1)} ms · ${(1000 / Math.max(frameTimeMs, 0.1)).toFixed(0)} fps · `
                        + `${tier.boids.toLocaleString()} boids + ${tier.predators} predators · `
                        + `${rt.leaders.length} leaders`;

        requestAnimationFrame(frame);
    };
    requestAnimationFrame(frame);
    log.info('boids-3d: entered main loop');
}

main().catch((err) => {
    log.error(`boids-3d: failed to start: ${err instanceof Error ? err.message : err}`);
});
