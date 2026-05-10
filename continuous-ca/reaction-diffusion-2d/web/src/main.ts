import {
    initLogger, log,
    HotReloader, ParamPanel, StateWriter, StateReader,
    Context, Renderer,
    Texture, TextureType,
    ComputePipeline, RenderPipeline,
    Buffer, MemoryUsage,
    type JsonValue,
} from '@gpusims/common-web';

import { COLORMAP_ORDER, COLORMAP_INDEX, buildColormapTextureData } from './colormaps';
import { PEARSON_PRESETS } from './presets';

import fullscreenVert from '../shaders/fullscreen.vert.wgsl?raw';
import rdUpdateWgsl   from '../shaders/rd_update.compute.wgsl?raw';
import brushStampWgsl from '../shaders/brush_stamp.compute.wgsl?raw';
import visualizeFrag  from '../shaders/visualize.frag.wgsl?raw';

// HMR-relative paths. MUST match what the Vite WGSL plugin emits, which is
// `relative(viteRoot, file)` where viteRoot is this sim's `web/` directory.
// Convention is paths relative to web/, e.g. 'shaders/rd_update.compute.wgsl'
// (matches strange-attractors and the mandelbulb fix landed in this phase).
const VERT_PATH        = 'shaders/fullscreen.vert.wgsl';
const RD_UPDATE_PATH   = 'shaders/rd_update.compute.wgsl';
const BRUSH_STAMP_PATH = 'shaders/brush_stamp.compute.wgsl';
const VISUALIZE_PATH   = 'shaders/visualize.frag.wgsl';

const MAX_DPR        = 2.0;
const STATE_FORMAT: GPUTextureFormat = 'rg32float';
const STATE_BPP      = 8;   // bytes per pixel for rg32float
const SUBSTEPS_DEFAULT = 4;
const DEFAULT_SEED   = 0xC0FFEE;

const GRID_SIZE_OPTIONS = [256, 512, 768, 1024] as const;
type GridSize = typeof GRID_SIZE_OPTIONS[number];

interface RuntimeState {
    schemaVersion: number;
    presetIndex: number;     // 0..5 = λ σ α β ξ τ; -1 = custom
    F: number;
    k: number;
    Du: number;
    Dv: number;
    dt: number;
    gridSize: GridSize;
    substeps: number;
    iteration: number;
    initSeed: number;
    seedBlockSize: number;
    noiseAmp: number;
    brushRadius: number;
    brushStrength: number;
    colormap: number;
    minValue: number;
    maxValue: number;
}

function defaultRuntime(): RuntimeState {
    const lambda = PEARSON_PRESETS[0]!;
    return {
        schemaVersion: 1,
        presetIndex: 0,
        F: lambda.F, k: lambda.k,
        Du: 0.16, Dv: 0.08, dt: 1.0,
        gridSize: 512,
        substeps: SUBSTEPS_DEFAULT,
        iteration: 0,
        initSeed: DEFAULT_SEED,
        seedBlockSize: 16,
        noiseAmp: 0.05,
        brushRadius: 12,
        brushStrength: 1.0,
        colormap: COLORMAP_INDEX.magma,
        minValue: 0.0, maxValue: 1.0,
    };
}

function xorshift32(state: { s: number }): number {
    let x = state.s | 0;
    x ^= x << 13;
    x ^= x >>> 17;
    x ^= x << 5;
    state.s = x | 0;
    return ((x >>> 0) / 4294967296);
}

function buildInitialState(gridSize: number, blockSize: number,
                           amp: number, seed: number): Float32Array {
    const data = new Float32Array(gridSize * gridSize * 2);
    const half = Math.floor(blockSize / 2);
    const cx = Math.floor(gridSize / 2);
    const cy = Math.floor(gridSize / 2);
    const rng = { s: (seed | 0) || 1 };

    for (let y = 0; y < gridSize; y++) {
        for (let x = 0; x < gridSize; x++) {
            const idx = (y * gridSize + x) * 2;
            const inBlock = (Math.abs(x - cx) < half) && (Math.abs(y - cy) < half);
            const noiseU = (xorshift32(rng) - 0.5) * amp;
            const noiseV = (xorshift32(rng) - 0.5) * amp;
            if (inBlock) {
                data[idx]     = 0.5  + noiseU;
                data[idx + 1] = 0.25 + noiseV;
            } else {
                data[idx]     = 1.0  + noiseU;
                data[idx + 1] = 0.0  + noiseV;
            }
        }
    }
    return data;
}

function makeStateTexture(ctx: Context, gridSize: number, label: string): Texture {
    return new Texture(ctx, {
        type: TextureType.e2D,
        extent: { width: gridSize, height: gridSize, depthOrArrayLayers: 1 },
        format: STATE_FORMAT,
        usage: GPUTextureUsage.STORAGE_BINDING
             | GPUTextureUsage.TEXTURE_BINDING
             | GPUTextureUsage.COPY_SRC
             | GPUTextureUsage.COPY_DST,
        label,
    });
}

async function main(): Promise<void> {
    initLogger();
    log.info('reaction-diffusion-2d: starting up');

    const canvas = document.getElementById('canvas') as HTMLCanvasElement | null;
    const unsupported = document.getElementById('unsupported');
    if (!canvas) throw new Error('no #canvas element');

    if (!('gpu' in navigator)) {
        if (unsupported) unsupported.style.display = 'grid';
        canvas.style.display = 'none';
        return;
    }

    let ctx: Context;
    try {
        ctx = await Context.create({ canvas, powerPreference: 'high-performance' });
    } catch (err) {
        log.error(`Context.create failed: ${err instanceof Error ? err.message : err}`);
        if (unsupported) unsupported.style.display = 'grid';
        canvas.style.display = 'none';
        return;
    }
    const renderer = new Renderer(ctx);
    const device = ctx.device;

    function targetDimensions(): { width: number; height: number } {
        const dpr = Math.min(window.devicePixelRatio || 1, MAX_DPR);
        return {
            width:  Math.max(1, Math.floor(window.innerWidth  * dpr)),
            height: Math.max(1, Math.floor(window.innerHeight * dpr)),
        };
    }
    let { width: canvasW, height: canvasH } = targetDimensions();
    canvas.width = canvasW;
    canvas.height = canvasH;

    const rt: RuntimeState = defaultRuntime();

    // ----- State textures (ping-pong) -----
    let texPing = makeStateTexture(ctx, rt.gridSize, 'rd-state-ping');
    let texPong = makeStateTexture(ctx, rt.gridSize, 'rd-state-pong');
    let pingIsLatest = true;

    function reseedTextures(): void {
        const data = buildInitialState(rt.gridSize, rt.seedBlockSize, rt.noiseAmp, rt.initSeed);
        texPing.uploadDirect2D(data, STATE_BPP);
        texPong.uploadDirect2D(data, STATE_BPP);
        pingIsLatest = true;
        rt.iteration = 0;
    }
    reseedTextures();

    function recreateStateTextures(): void {
        texPing.destroy();
        texPong.destroy();
        texPing = makeStateTexture(ctx, rt.gridSize, 'rd-state-ping');
        texPong = makeStateTexture(ctx, rt.gridSize, 'rd-state-pong');
        reseedTextures();
        rebuildBindGroups();
    }

    // ----- Samplers -----
    const stateSampler = device.createSampler({
        label: 'rd-state-sampler',
        addressModeU: 'repeat', addressModeV: 'repeat',
        magFilter: 'nearest', minFilter: 'nearest',
    });
    const lutSampler = device.createSampler({
        label: 'rd-lut-sampler',
        magFilter: 'linear', minFilter: 'linear',
    });

    // ----- Colormap LUT texture (256×4 RGBA8) -----
    const lutData = buildColormapTextureData();
    const lutTexture = new Texture(ctx, {
        type: TextureType.e2D,
        extent: { width: 256, height: 4, depthOrArrayLayers: 1 },
        format: 'rgba8unorm',
        usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST,
        label: 'rd-colormap-lut',
    });
    lutTexture.uploadDirect2D(lutData, 4);

    // ----- Uniforms -----
    const RD_UNIFORM_SIZE = 32;
    const rdUniform = new Buffer(ctx, {
        size: RD_UNIFORM_SIZE,
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'rd-uniform',
    });
    const rdUniformBytes = new ArrayBuffer(RD_UNIFORM_SIZE);
    const rdU32 = new Uint32Array(rdUniformBytes);
    const rdF32 = new Float32Array(rdUniformBytes);
    function uploadRDUniform(): void {
        rdU32[0] = rt.gridSize; rdU32[1] = rt.gridSize;
        rdF32[2] = rt.F;        rdF32[3] = rt.k;
        rdF32[4] = rt.Du;       rdF32[5] = rt.Dv;
        rdF32[6] = rt.dt;       rdF32[7] = 0;
        rdUniform.uploadDirect(new Uint8Array(rdUniformBytes));
    }

    const BRUSH_UNIFORM_SIZE = 32;
    const brushUniform = new Buffer(ctx, {
        size: BRUSH_UNIFORM_SIZE,
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'brush-uniform',
    });
    const brushUniformBytes = new ArrayBuffer(BRUSH_UNIFORM_SIZE);
    const brushU32 = new Uint32Array(brushUniformBytes);
    const brushF32 = new Float32Array(brushUniformBytes);
    function uploadBrushUniform(centerX: number, centerY: number): void {
        brushU32[0] = rt.gridSize; brushU32[1] = rt.gridSize;
        brushF32[2] = centerX;     brushF32[3] = centerY;
        brushF32[4] = rt.brushRadius;
        brushF32[5] = rt.brushStrength;
        brushF32[6] = 0; brushF32[7] = 0;
        brushUniform.uploadDirect(new Uint8Array(brushUniformBytes));
    }

    const VIZ_UNIFORM_SIZE = 32;
    const vizUniform = new Buffer(ctx, {
        size: VIZ_UNIFORM_SIZE,
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'viz-uniform',
    });
    const vizUniformBytes = new ArrayBuffer(VIZ_UNIFORM_SIZE);
    const vizF32 = new Float32Array(vizUniformBytes);
    function uploadVizUniform(): void {
        vizF32[0] = rt.gridSize; vizF32[1] = rt.gridSize;
        vizF32[2] = rt.minValue; vizF32[3] = rt.maxValue;
        vizF32[4] = rt.colormap; vizF32[5] = 0;
        vizF32[6] = 0;           vizF32[7] = 0;
        vizUniform.uploadDirect(new Uint8Array(vizUniformBytes));
    }

    // ----- Compute pipelines -----
    const rdPipeline = await ComputePipeline.create(ctx, {
        source: rdUpdateWgsl,
        shaderPath: RD_UPDATE_PATH,
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, texture: { sampleType: 'unfilterable-float', viewDimension: '2d' } },
            { binding: 2, visibility: GPUShaderStage.COMPUTE, sampler: { type: 'non-filtering' } },
            { binding: 3, visibility: GPUShaderStage.COMPUTE, storageTexture: { access: 'write-only', format: STATE_FORMAT, viewDimension: '2d' } },
        ],
        label: 'rd-update',
    });

    const brushPipeline = await ComputePipeline.create(ctx, {
        source: brushStampWgsl,
        shaderPath: BRUSH_STAMP_PATH,
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, texture: { sampleType: 'unfilterable-float', viewDimension: '2d' } },
            { binding: 2, visibility: GPUShaderStage.COMPUTE, sampler: { type: 'non-filtering' } },
            { binding: 3, visibility: GPUShaderStage.COMPUTE, storageTexture: { access: 'write-only', format: STATE_FORMAT, viewDimension: '2d' } },
        ],
        label: 'brush-stamp',
    });

    const vizPipeline = await RenderPipeline.create(ctx, {
        vertexSource: fullscreenVert,
        fragmentSource: visualizeFrag,
        vertexPath: VERT_PATH,
        fragmentPath: VISUALIZE_PATH,
        bindings: [
            { binding: 0, visibility: GPUShaderStage.FRAGMENT, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'unfilterable-float', viewDimension: '2d' } },
            { binding: 2, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float', viewDimension: '2d' } },
            { binding: 3, visibility: GPUShaderStage.FRAGMENT, sampler: { type: 'filtering' } },
        ],
        colorFormats: [ctx.preferredFormat],
        primitive: { topology: 'triangle-list' },
        label: 'rd-visualize',
    });

    // ----- Bind groups (rebuilt on texture recreate) -----
    let rdBgPingToPong:    GPUBindGroup;
    let rdBgPongToPing:    GPUBindGroup;
    let brushBgPingToPong: GPUBindGroup;
    let brushBgPongToPing: GPUBindGroup;
    let vizBgPing:         GPUBindGroup;
    let vizBgPong:         GPUBindGroup;

    function rebuildBindGroups(): void {
        rdBgPingToPong = rdPipeline.createBindGroup([
            { binding: 0, resource: { buffer: rdUniform.handle } },
            { binding: 1, resource: texPing.view },
            { binding: 2, resource: stateSampler },
            { binding: 3, resource: texPong.view },
        ], 'rd-bg-ping-to-pong');
        rdBgPongToPing = rdPipeline.createBindGroup([
            { binding: 0, resource: { buffer: rdUniform.handle } },
            { binding: 1, resource: texPong.view },
            { binding: 2, resource: stateSampler },
            { binding: 3, resource: texPing.view },
        ], 'rd-bg-pong-to-ping');

        brushBgPingToPong = brushPipeline.createBindGroup([
            { binding: 0, resource: { buffer: brushUniform.handle } },
            { binding: 1, resource: texPing.view },
            { binding: 2, resource: stateSampler },
            { binding: 3, resource: texPong.view },
        ], 'brush-bg-ping-to-pong');
        brushBgPongToPing = brushPipeline.createBindGroup([
            { binding: 0, resource: { buffer: brushUniform.handle } },
            { binding: 1, resource: texPong.view },
            { binding: 2, resource: stateSampler },
            { binding: 3, resource: texPing.view },
        ], 'brush-bg-pong-to-ping');

        vizBgPing = vizPipeline.createBindGroup([
            { binding: 0, resource: { buffer: vizUniform.handle } },
            { binding: 1, resource: texPing.view },
            { binding: 2, resource: lutTexture.view },
            { binding: 3, resource: lutSampler },
        ], 'viz-bg-ping');
        vizBgPong = vizPipeline.createBindGroup([
            { binding: 0, resource: { buffer: vizUniform.handle } },
            { binding: 1, resource: texPong.view },
            { binding: 2, resource: lutTexture.view },
            { binding: 3, resource: lutSampler },
        ], 'viz-bg-pong');
    }
    rebuildBindGroups();

    // ----- Hot reload -----
    const hot = new HotReloader();
    hot.watch(RD_UPDATE_PATH, async (path, src) => {
        const err = await rdPipeline.reload(src);
        if (err) hot.reportFailure(path, err); else hot.reportSuccess(path);
    });
    hot.watch(BRUSH_STAMP_PATH, async (path, src) => {
        const err = await brushPipeline.reload(src);
        if (err) hot.reportFailure(path, err); else hot.reportSuccess(path);
    });
    hot.watch(VERT_PATH, async (path, src) => {
        const err = await vizPipeline.reload('vertex', src);
        if (err) hot.reportFailure(path, err); else hot.reportSuccess(path);
    });
    hot.watch(VISUALIZE_PATH, async (path, src) => {
        const err = await vizPipeline.reload('fragment', src);
        if (err) hot.reportFailure(path, err); else hot.reportSuccess(path);
    });

    // ----- Brush input tracking -----
    let brushDown = false;
    let brushCellX = 0;
    let brushCellY = 0;
    let brushHasPosition = false;

    function pointerToCell(e: PointerEvent): void {
        const rect = canvas.getBoundingClientRect();
        const u = (e.clientX - rect.left) / Math.max(rect.width,  1);
        const v = (e.clientY - rect.top)  / Math.max(rect.height, 1);
        brushCellX = u * rt.gridSize;
        brushCellY = v * rt.gridSize;
        brushHasPosition = true;
    }
    canvas.addEventListener('pointermove',  pointerToCell);
    canvas.addEventListener('pointerdown', (e) => {
        if (e.button !== 0) return;
        canvas.setPointerCapture(e.pointerId);
        brushDown = true;
        pointerToCell(e);
    });
    canvas.addEventListener('pointerup',   (e) => {
        if (e.button !== 0) return;
        canvas.releasePointerCapture(e.pointerId);
        brushDown = false;
    });
    canvas.addEventListener('pointercancel', () => { brushDown = false; });
    canvas.addEventListener('contextmenu',   (e) => e.preventDefault());

    // ----- ParamPanel -----
    const panel = new ParamPanel({ title: 'Reaction-Diffusion 2D', persistKey: 'reaction-diffusion-2d' });

    const fPreset = panel.addFolder('Preset');
    fPreset.addDropdown({
        label: 'Pearson region',
        options: PEARSON_PRESETS.map((p, i) => ({ value: String(i), label: p.label })),
        getValue: () => String(rt.presetIndex < 0 ? 0 : rt.presetIndex),
        setValue: (v) => {
            const idx = parseInt(v, 10);
            const preset = PEARSON_PRESETS[idx];
            if (!preset) return;
            rt.presetIndex = idx;
            rt.F = preset.F;
            rt.k = preset.k;
            reseedTextures();
            panel.refreshDisplays();
        },
    });
    fPreset.addNumber(rt, 'F', 0.0, 0.1, 0.0005)
        .onChange(() => { rt.presetIndex = -1; });
    fPreset.addNumber(rt, 'k', 0.0, 0.1, 0.0005)
        .onChange(() => { rt.presetIndex = -1; });
    fPreset.addButton('Reseed (current params)', () => { reseedTextures(); });

    const fDiff = panel.addFolder('Diffusion');
    fDiff.addNumber(rt, 'Du', 0.0, 1.0, 0.01);
    fDiff.addNumber(rt, 'Dv', 0.0, 1.0, 0.01);
    fDiff.addNumber(rt, 'dt', 0.1, 1.5, 0.05);
    fDiff.close();

    const fSim = panel.addFolder('Simulation');
    fSim.addDropdown({
        label: 'grid size',
        options: GRID_SIZE_OPTIONS.map((g) => ({ value: String(g), label: `${g} × ${g}` })),
        getValue: () => String(rt.gridSize),
        setValue: (v) => {
            const next = parseInt(v, 10) as GridSize;
            if (!(GRID_SIZE_OPTIONS as readonly number[]).includes(next)) return;
            rt.gridSize = next;
            recreateStateTextures();
        },
    });
    fSim.addNumber(rt, 'substeps', 1, 32, 1).name('substeps/frame');
    fSim.addNumber(rt, 'noiseAmp',      0.0, 0.5, 0.005).name('noise amp');
    fSim.addNumber(rt, 'seedBlockSize', 4, 64, 2).name('seed block');
    fSim.addNumber(rt, 'initSeed', 0, 0xFFFFFFFF, 1).name('init seed');

    const fBrush = panel.addFolder('Brush');
    fBrush.addNumber(rt, 'brushRadius',   2, 64, 1).name('radius (cells)');
    fBrush.addNumber(rt, 'brushStrength', 0, 4,  0.05).name('strength');

    const fDisp = panel.addFolder('Display');
    fDisp.addDropdown({
        label: 'colormap',
        options: COLORMAP_ORDER.map((name, i) => ({ value: String(i), label: name })),
        getValue: () => String(rt.colormap),
        setValue: (v) => { rt.colormap = parseInt(v, 10); },
    });
    fDisp.addNumber(rt, 'minValue', 0.0, 1.0, 0.01).name('V min');
    fDisp.addNumber(rt, 'maxValue', 0.0, 1.0, 0.01).name('V max');

    panel.addButton('Save (F5)',    () => { void doSave(); });
    panel.addButton('Load... (F9)', () => { triggerFileLoad(); });

    // ----- Capture (F5) and load (F9) -----
    const stateWriter = new StateWriter('captures');
    let nextCapture = 0;

    interface CaptureMeta {
        schemaVersion: 1;
        presetIndex: number;
        presetName: string;
        F: number; k: number;
        Du: number; Dv: number; dt: number;
        gridSize: number;
        substeps: number;
        iteration: number;
        colormap: number;
        minValue: number;
        maxValue: number;
        brushRadius: number;
        brushStrength: number;
        initSeed: number;
        seedBlockSize: number;
        noiseAmp: number;
    }

    function captureMeta(): CaptureMeta {
        const presetName = rt.presetIndex >= 0
            ? (PEARSON_PRESETS[rt.presetIndex]?.label ?? 'Custom')
            : 'Custom';
        return {
            schemaVersion: 1,
            presetIndex: rt.presetIndex,
            presetName,
            F: rt.F, k: rt.k, Du: rt.Du, Dv: rt.Dv, dt: rt.dt,
            gridSize: rt.gridSize,
            substeps: rt.substeps,
            iteration: rt.iteration,
            colormap: rt.colormap,
            minValue: rt.minValue, maxValue: rt.maxValue,
            brushRadius: rt.brushRadius, brushStrength: rt.brushStrength,
            initSeed: rt.initSeed, seedBlockSize: rt.seedBlockSize, noiseAmp: rt.noiseAmp,
        };
    }

    async function doSave(): Promise<void> {
        const latest = pingIsLatest ? texPing : texPong;
        const raw = await latest.readback2D(STATE_BPP);
        const cells = rt.gridSize * rt.gridSize;
        const u = new Float32Array(cells);
        const v = new Float32Array(cells);
        const view = new Float32Array(raw.buffer, raw.byteOffset, cells * 2);
        for (let i = 0; i < cells; i++) {
            u[i] = view[i * 2]!;
            v[i] = view[i * 2 + 1]!;
        }

        stateWriter.beginFrame(nextCapture);
        stateWriter.setMeta('reactionDiffusion2d', captureMeta() as unknown as JsonValue);
        stateWriter.saveBuffer('u', u, { count: cells, stride: 4, format: 'r32f', shape: [rt.gridSize, rt.gridSize] });
        stateWriter.saveBuffer('v', v, { count: cells, stride: 4, format: 'r32f', shape: [rt.gridSize, rt.gridSize] });
        await stateWriter.endFrame();
        log.info(`captured capture_${nextCapture.toString().padStart(4, '0')}.zip`);
        nextCapture++;
    }

    function triggerFileLoad(): void {
        const inp = document.createElement('input');
        inp.type = 'file';
        inp.accept = '.zip';
        inp.onchange = async (): Promise<void> => {
            const f = inp.files?.[0];
            if (!f) return;
            const cap = await StateReader.fromFile(f);
            if (!cap) return;
            const m = cap.meta('reactionDiffusion2d') as unknown as CaptureMeta | undefined;
            if (!m) { log.error('capture missing reactionDiffusion2d meta'); return; }
            const uBytes = cap.buffer('u');
            const vBytes = cap.buffer('v');
            if (!uBytes || !vBytes) { log.error('capture missing u/v buffer data'); return; }
            applyCapture(m, uBytes, vBytes);
            log.info(`loaded ${cap.directoryName}`);
        };
        inp.click();
    }

    function applyCapture(m: CaptureMeta, uBytes: Uint8Array, vBytes: Uint8Array): void {
        const gridChanged = (rt.gridSize !== m.gridSize);
        rt.presetIndex = m.presetIndex;
        rt.F  = m.F;  rt.k  = m.k;
        rt.Du = m.Du; rt.Dv = m.Dv; rt.dt = m.dt;
        rt.gridSize = m.gridSize as GridSize;
        rt.substeps = m.substeps;
        rt.iteration = m.iteration;
        rt.colormap = m.colormap;
        rt.minValue = m.minValue;
        rt.maxValue = m.maxValue;
        rt.brushRadius   = m.brushRadius;
        rt.brushStrength = m.brushStrength;
        rt.initSeed      = m.initSeed;
        rt.seedBlockSize = m.seedBlockSize;
        rt.noiseAmp      = m.noiseAmp;

        if (gridChanged) {
            texPing.destroy();
            texPong.destroy();
            texPing = makeStateTexture(ctx, rt.gridSize, 'rd-state-ping');
            texPong = makeStateTexture(ctx, rt.gridSize, 'rd-state-pong');
            rebuildBindGroups();
        }

        // Re-interleave u/v back into a Float32Array of shape (gridSize², 2).
        const cells = rt.gridSize * rt.gridSize;
        const uF = new Float32Array(uBytes.buffer, uBytes.byteOffset, cells);
        const vF = new Float32Array(vBytes.buffer, vBytes.byteOffset, cells);
        const interleaved = new Float32Array(cells * 2);
        for (let i = 0; i < cells; i++) {
            interleaved[i * 2]     = uF[i]!;
            interleaved[i * 2 + 1] = vF[i]!;
        }
        // Upload to both ping and pong so subsequent substeps are valid
        // regardless of which side `pingIsLatest` points to.
        texPing.uploadDirect2D(interleaved, STATE_BPP);
        texPong.uploadDirect2D(interleaved, STATE_BPP);
        pingIsLatest = true;

        panel.refreshDisplays();
    }

    // ----- Keyboard hotkeys (F5 / F9) -----
    let prevF5 = false, prevF9 = false;
    window.addEventListener('keydown', (e) => {
        if (e.code === 'F5') { e.preventDefault(); if (!prevF5) { prevF5 = true; void doSave(); } }
        if (e.code === 'F9') { e.preventDefault(); if (!prevF9) { prevF9 = true; triggerFileLoad(); } }
    });
    window.addEventListener('keyup', (e) => {
        if (e.code === 'F5') prevF5 = false;
        if (e.code === 'F9') prevF9 = false;
    });

    // ----- Resize handling (debounced) -----
    let resizeTimer: number | null = null;
    window.addEventListener('resize', () => {
        if (resizeTimer !== null) clearTimeout(resizeTimer);
        resizeTimer = window.setTimeout(() => {
            const dim = targetDimensions();
            canvasW = dim.width;
            canvasH = dim.height;
            canvas.width  = canvasW;
            canvas.height = canvasH;
            resizeTimer = null;
        }, 100);
    });

    // ----- Main loop -----
    function frame(): void {
        uploadRDUniform();
        uploadVizUniform();

        const f = renderer.beginFrame();

        // 1. Substep loop (compute ping-pong)
        for (let s = 0; s < rt.substeps; s++) {
            const bg = pingIsLatest ? rdBgPingToPong : rdBgPongToPing;
            const gx = Math.ceil(rt.gridSize / 16);
            const gy = Math.ceil(rt.gridSize / 16);
            rdPipeline.dispatch(f.encoder, bg, gx, gy, 1);
            pingIsLatest = !pingIsLatest;
            rt.iteration++;
        }

        // 2. Brush stamp (one extra ping-pong tick if active)
        if (brushDown && brushHasPosition) {
            uploadBrushUniform(brushCellX, brushCellY);
            const bg = pingIsLatest ? brushBgPingToPong : brushBgPongToPing;
            const gx = Math.ceil(rt.gridSize / 16);
            const gy = Math.ceil(rt.gridSize / 16);
            brushPipeline.dispatch(f.encoder, bg, gx, gy, 1);
            pingIsLatest = !pingIsLatest;
        }

        // 3. Visualize the latest state into the swapchain image
        const vizBg = pingIsLatest ? vizBgPing : vizBgPong;
        const pass = renderer.beginRendering(f, [0, 0, 0, 1]);
        vizPipeline.bind(pass, vizBg);
        pass.draw(3, 1, 0, 0);
        renderer.endRendering(pass);

        renderer.endFrame(f);

        requestAnimationFrame(frame);
    }

    requestAnimationFrame(frame);
    log.info('reaction-diffusion-2d: entered main loop');
}

void main();
