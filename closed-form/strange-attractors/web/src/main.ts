import {
    initLogger, log,
    Camera, HotReloader, ParamPanel, StateWriter, StateReader,
    snapshotInput,
    Context, Renderer,
    Buffer, MemoryUsage,
    type JsonValue, type JsonObject,
} from '@gpusims/common-web';

import {
    ATTRACTORS, ATTRACTOR_ORDER, packParams, seedParticles,
    type AttractorId,
} from './attractors';
import { COLORMAP_ORDER, COLORMAP_INDEX, buildColormapTextureData } from './colormaps';

import integrateWgsl     from '../shaders/integrate.compute.wgsl?raw';
import decayWgsl         from '../shaders/decay.frag.wgsl?raw';
import fullscreenVert    from '../shaders/fullscreen.vert.wgsl?raw';
import splatVert         from '../shaders/splat.vert.wgsl?raw';
import splatFrag         from '../shaders/splat.frag.wgsl?raw';
import bloomExtractWgsl  from '../shaders/bloom_extract.compute.wgsl?raw';
import bloomBlurWgsl     from '../shaders/bloom_blur.compute.wgsl?raw';
import tonemapWgsl       from '../shaders/tonemap.frag.wgsl?raw';

const PARTICLE_COUNT = 2_000_000;
const HDR_FORMAT: GPUTextureFormat = 'rgba16float';
const DEFAULT_SEED = 0xC0FFEE;
const MAX_DPR = 2.0;
const DEFAULT_BLOOM_INTENSITY = 0.15;
const DEFAULT_BLOOM_THRESHOLD = 1.0;
const DEFAULT_BLOOM_SOFT_KNEE = 0.5;
const DEFAULT_EXPOSURE = 1.0;
const ORBIT_DEFAULT_DEG_PER_SEC = 6;

// Shader-relative paths used as keys for hot-reload registration.
const PATH_INTEGRATE      = 'shaders/integrate.compute.wgsl';
const PATH_DECAY          = 'shaders/decay.frag.wgsl';
const PATH_FULLSCREEN_VS  = 'shaders/fullscreen.vert.wgsl';
const PATH_SPLAT_VS       = 'shaders/splat.vert.wgsl';
const PATH_SPLAT_FS       = 'shaders/splat.frag.wgsl';
const PATH_BLOOM_EXTRACT  = 'shaders/bloom_extract.compute.wgsl';
const PATH_BLOOM_BLUR     = 'shaders/bloom_blur.compute.wgsl';
const PATH_TONEMAP        = 'shaders/tonemap.frag.wgsl';

interface RuntimeState {
    attractorId: AttractorId;
    paramValues: number[];           // current values, parallel to def.params
    substeps: number;
    simDt: number;
    pointSize: number;
    depthAttenK: number;
    colorSpeedScale: number;
    colorExponent: number;
    colormap: number;                // 0..3
    bloomIntensity: number;
    bloomThreshold: number;
    bloomSoftKnee: number;
    exposure: number;
    alpha: number;
    decayPreset: 'manifold' | 'motion' | 'custom';
    autoOrbit: boolean;
    orbitSpeedDegPerSec: number;
    orbitRadius: number;
    initSeed: number;
}

function defaultRuntime(): RuntimeState {
    const def = ATTRACTORS.lorenz;
    return {
        attractorId: 'lorenz',
        paramValues: def.params.map((p) => p.default),
        substeps: def.defaultSubsteps,
        simDt: def.defaultSimDt,
        pointSize: def.defaultPointSize,
        depthAttenK: 0.02,
        colorSpeedScale: def.defaultColorSpeedScale,
        colorExponent: 1.0,
        colormap: COLORMAP_INDEX.magma,
        bloomIntensity: DEFAULT_BLOOM_INTENSITY,
        bloomThreshold: DEFAULT_BLOOM_THRESHOLD,
        bloomSoftKnee: DEFAULT_BLOOM_SOFT_KNEE,
        exposure: DEFAULT_EXPOSURE,
        alpha: 1.0,
        decayPreset: 'manifold',
        autoOrbit: true,
        orbitSpeedDegPerSec: ORBIT_DEFAULT_DEG_PER_SEC,
        orbitRadius: def.orbitRadius,
        initSeed: DEFAULT_SEED,
    };
}

async function main(): Promise<void> {
    initLogger();
    log.info('strange-attractors: starting up');

    const canvasEl = document.getElementById('canvas');
    const unsupported = document.getElementById('unsupported');
    if (!(canvasEl instanceof HTMLCanvasElement)) throw new Error('no #canvas element');
    const canvas: HTMLCanvasElement = canvasEl;

    if (!('gpu' in navigator)) {
        if (unsupported) unsupported.style.display = 'grid';
        canvas.style.display = 'none';
        return;
    }

    let ctx: Context;
    try {
        // Per project-state.md known issue 1, do NOT request timestamp-query
        // — profiler degrades to CPU-only timing. Phase 2 doesn't need GPU
        // timestamps for v1.
        ctx = await Context.create({
            canvas,
            powerPreference: 'high-performance',
        });
    } catch (err) {
        log.error(`Context.create failed: ${err instanceof Error ? err.message : err}`);
        if (unsupported) unsupported.style.display = 'grid';
        canvas.style.display = 'none';
        return;
    }
    const renderer = new Renderer(ctx);
    const device = ctx.device;

    // ----- Canvas + DPR sizing -----
    function targetDimensions(): { width: number; height: number } {
        const dpr = Math.min(window.devicePixelRatio || 1, MAX_DPR);
        const w = Math.max(1, Math.floor(window.innerWidth * dpr));
        const h = Math.max(1, Math.floor(window.innerHeight * dpr));
        return { width: w, height: h };
    }
    let { width: rtWidth, height: rtHeight } = targetDimensions();
    canvas.width = rtWidth;
    canvas.height = rtHeight;

    // ----- Runtime state -----
    const rt: RuntimeState = defaultRuntime();

    // ----- Particle storage buffer -----
    const positions = new Buffer(ctx, {
        size: PARTICLE_COUNT * 16,         // vec4<f32> per particle
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'positions',
    });

    function reseed(): void {
        const data = new Float32Array(PARTICLE_COUNT * 4);
        seedParticles(data, ATTRACTORS[rt.attractorId], PARTICLE_COUNT, rt.initSeed);
        positions.uploadDirect(data);
    }
    reseed();

    // ----- Sim uniform (integrate.compute.wgsl) -----
    const simUniform = new Buffer(ctx, {
        size: 64,                          // 1 vec4<f32> + 1 vec4<u32> + 8 floats
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'sim-uniform',
    });
    const simBytes = new ArrayBuffer(64);
    const simF32 = new Float32Array(simBytes);
    const simU32 = new Uint32Array(simBytes);
    function uploadSim(): void {
        const def = ATTRACTORS[rt.attractorId];
        const packed = packParams({ ...def, params: def.params.map((p, i) => ({ ...p, default: rt.paramValues[i] ?? p.default })) });
        simF32[0] = rt.simDt;
        simU32[1] = rt.substeps;
        simU32[2] = def.index;
        simU32[3] = PARTICLE_COUNT;
        simF32[4] = packed[0];
        simF32[5] = packed[1];
        simF32[6] = packed[2];
        simF32[7] = packed[3];
        simF32[8] = packed[4];
        simF32[9] = packed[5];
        // remaining bytes are padding; leave at zero
        simUniform.uploadDirect(new Uint8Array(simBytes));
    }
    uploadSim();

    // ----- Render uniform (splat shaders) -----
    // Layout matches RenderUniforms struct in splat.vert.wgsl:
    //   mat4x4<f32> viewProj   (64 B)
    //   vec2<f32>   viewportSize (8 B)
    //   f32 pointSizePx, depthAttenK, colorSpeedScale, colorExponent, colormapIndex, _pad (24 B)
    // Total 96 B.
    const renderUniform = new Buffer(ctx, {
        size: 96,
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'render-uniform',
    });
    const renderBytes = new ArrayBuffer(96);
    const renderF32 = new Float32Array(renderBytes);
    function uploadRenderUniform(viewProj: Float32Array, viewportW: number, viewportH: number): void {
        renderF32.set(viewProj, 0);                 // 16 floats
        renderF32[16] = viewportW;
        renderF32[17] = viewportH;
        renderF32[18] = rt.pointSize;
        renderF32[19] = rt.depthAttenK;
        renderF32[20] = rt.colorSpeedScale;
        renderF32[21] = rt.colorExponent;
        renderF32[22] = rt.colormap;
        // [23] is _pad
        renderUniform.uploadDirect(new Uint8Array(renderBytes));
    }

    // ----- Decay / bloom / tonemap uniforms -----
    const decayUniform = new Buffer(ctx, {
        size: 16, usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal, label: 'decay-uniform',
    });
    const bloomExtractUniform = new Buffer(ctx, {
        size: 16, usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal, label: 'bloom-extract-uniform',
    });
    const bloomBlurUniformH = new Buffer(ctx, {
        size: 16, usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal, label: 'bloom-blur-h-uniform',
    });
    const bloomBlurUniformV = new Buffer(ctx, {
        size: 16, usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal, label: 'bloom-blur-v-uniform',
    });
    const tonemapUniform = new Buffer(ctx, {
        size: 16, usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal, label: 'tonemap-uniform',
    });
    function uploadScalarUniform(buf: Buffer, values: [number, number, number, number]): void {
        const bytes = new Float32Array(values);
        buf.uploadDirect(new Uint8Array(bytes.buffer));
    }
    function uploadVec2Uniform(buf: Buffer, v: [number, number]): void {
        const bytes = new Float32Array([v[0], v[1], 0, 0]);
        buf.uploadDirect(new Uint8Array(bytes.buffer));
    }
    uploadVec2Uniform(bloomBlurUniformH, [1, 0]);
    uploadVec2Uniform(bloomBlurUniformV, [0, 1]);

    // ----- Colormap LUT texture (256 × 4 RGBA8) -----
    const lutData = buildColormapTextureData();
    const lutTexture = device.createTexture({
        label: 'colormap-lut',
        size: { width: 256, height: 4, depthOrArrayLayers: 1 },
        format: 'rgba8unorm',
        usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST,
    });
    device.queue.writeTexture(
        { texture: lutTexture },
        lutData,
        { bytesPerRow: 256 * 4, rowsPerImage: 4 },
        { width: 256, height: 4, depthOrArrayLayers: 1 },
    );
    const lutSampler = device.createSampler({ label: 'lut-sampler', magFilter: 'linear', minFilter: 'linear' });
    const linearSampler = device.createSampler({ label: 'linear', magFilter: 'linear', minFilter: 'linear' });

    // ----- Per-resolution textures (recreated on resize) -----
    interface SizedResources {
        accumA: GPUTexture;
        accumB: GPUTexture;
        bloomHalf: GPUTexture;       // post-extract
        bloomHalfH: GPUTexture;      // post-horizontal blur
        bloomHalfV: GPUTexture;      // post-vertical blur (final composited)
    }
    let sized: SizedResources = createSizedResources(rtWidth, rtHeight);

    function createSizedResources(w: number, h: number): SizedResources {
        const halfW = Math.max(1, w >> 1);
        const halfH = Math.max(1, h >> 1);
        const mkAccum = (label: string): GPUTexture => device.createTexture({
            label,
            size: { width: w, height: h, depthOrArrayLayers: 1 },
            format: HDR_FORMAT,
            usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING,
        });
        const mkBloom = (label: string): GPUTexture => device.createTexture({
            label,
            size: { width: halfW, height: halfH, depthOrArrayLayers: 1 },
            format: HDR_FORMAT,
            usage: GPUTextureUsage.STORAGE_BINDING | GPUTextureUsage.TEXTURE_BINDING,
        });
        return {
            accumA: mkAccum('accum-A'),
            accumB: mkAccum('accum-B'),
            bloomHalf: mkBloom('bloom-half'),
            bloomHalfH: mkBloom('bloom-half-h'),
            bloomHalfV: mkBloom('bloom-half-v'),
        };
    }

    function destroySizedResources(s: SizedResources): void {
        s.accumA.destroy();
        s.accumB.destroy();
        s.bloomHalf.destroy();
        s.bloomHalfH.destroy();
        s.bloomHalfV.destroy();
    }

    // ----- Pipelines -----
    // For brevity: each pipeline created via raw device.create* — common-web's
    // ComputePipeline / RenderPipeline wrappers don't yet model the multi-pass
    // layouts this sim needs. If a wrapper would help, that's a follow-up
    // common-web addition; current path is verbose-but-correct.

    const integrateModule = device.createShaderModule({ label: 'integrate', code: integrateWgsl });
    const decayModule     = device.createShaderModule({ label: 'decay-fs', code: decayWgsl });
    const fullscreenVsModule = device.createShaderModule({ label: 'fullscreen-vs', code: fullscreenVert });
    const splatVsModule   = device.createShaderModule({ label: 'splat-vs', code: splatVert });
    const splatFsModule   = device.createShaderModule({ label: 'splat-fs', code: splatFrag });
    const bloomExtractModule = device.createShaderModule({ label: 'bloom-extract', code: bloomExtractWgsl });
    const bloomBlurModule    = device.createShaderModule({ label: 'bloom-blur', code: bloomBlurWgsl });
    const tonemapModule   = device.createShaderModule({ label: 'tonemap-fs', code: tonemapWgsl });

    const integratePipeline = device.createComputePipeline({
        label: 'integrate-pipeline',
        layout: 'auto',
        compute: { module: integrateModule, entryPoint: 'main' },
    });
    const integrateBg = device.createBindGroup({
        label: 'integrate-bg',
        layout: integratePipeline.getBindGroupLayout(0),
        entries: [
            { binding: 0, resource: { buffer: positions.handle } },
            { binding: 1, resource: { buffer: simUniform.handle } },
        ],
    });

    const decayPipeline = device.createRenderPipeline({
        label: 'decay-pipeline',
        layout: 'auto',
        vertex: { module: fullscreenVsModule, entryPoint: 'vs_main' },
        fragment: { module: decayModule, entryPoint: 'fs_main', targets: [{ format: HDR_FORMAT }] },
        primitive: { topology: 'triangle-list' },
    });
    function makeDecayBg(srcView: GPUTextureView): GPUBindGroup {
        return device.createBindGroup({
            label: 'decay-bg',
            layout: decayPipeline.getBindGroupLayout(0),
            entries: [
                { binding: 0, resource: linearSampler },
                { binding: 1, resource: srcView },
                { binding: 2, resource: { buffer: decayUniform.handle } },
            ],
        });
    }

    const splatPipeline = device.createRenderPipeline({
        label: 'splat-pipeline',
        layout: 'auto',
        vertex: { module: splatVsModule, entryPoint: 'vs_main' },
        fragment: {
            module: splatFsModule,
            entryPoint: 'fs_main',
            targets: [{
                format: HDR_FORMAT,
                blend: {
                    color: { srcFactor: 'one', dstFactor: 'one', operation: 'add' },
                    alpha: { srcFactor: 'one', dstFactor: 'one', operation: 'add' },
                },
            }],
        },
        primitive: { topology: 'triangle-list' },
    });
    const splatBg = device.createBindGroup({
        label: 'splat-bg',
        layout: splatPipeline.getBindGroupLayout(0),
        entries: [
            { binding: 0, resource: { buffer: positions.handle } },
            { binding: 1, resource: { buffer: renderUniform.handle } },
            { binding: 2, resource: lutSampler },
            { binding: 3, resource: lutTexture.createView() },
        ],
    });

    const bloomExtractPipeline = device.createComputePipeline({
        label: 'bloom-extract-pipeline',
        layout: 'auto',
        compute: { module: bloomExtractModule, entryPoint: 'main' },
    });
    const bloomBlurPipeline = device.createComputePipeline({
        label: 'bloom-blur-pipeline',
        layout: 'auto',
        compute: { module: bloomBlurModule, entryPoint: 'main' },
    });

    function makeBloomExtractBg(srcView: GPUTextureView, dstView: GPUTextureView): GPUBindGroup {
        return device.createBindGroup({
            label: 'bloom-extract-bg',
            layout: bloomExtractPipeline.getBindGroupLayout(0),
            entries: [
                { binding: 0, resource: linearSampler },
                { binding: 1, resource: srcView },
                { binding: 2, resource: dstView },
                { binding: 3, resource: { buffer: bloomExtractUniform.handle } },
            ],
        });
    }
    function makeBloomBlurBg(srcView: GPUTextureView, dstView: GPUTextureView, uniformBuf: Buffer): GPUBindGroup {
        return device.createBindGroup({
            label: 'bloom-blur-bg',
            layout: bloomBlurPipeline.getBindGroupLayout(0),
            entries: [
                { binding: 0, resource: linearSampler },
                { binding: 1, resource: srcView },
                { binding: 2, resource: dstView },
                { binding: 3, resource: { buffer: uniformBuf.handle } },
            ],
        });
    }

    const tonemapPipeline = device.createRenderPipeline({
        label: 'tonemap-pipeline',
        layout: 'auto',
        vertex: { module: fullscreenVsModule, entryPoint: 'vs_main' },
        fragment: {
            module: tonemapModule,
            entryPoint: 'fs_main',
            targets: [{ format: ctx.preferredFormat }],
        },
        primitive: { topology: 'triangle-list' },
    });
    function makeTonemapBg(hdrView: GPUTextureView, bloomView: GPUTextureView): GPUBindGroup {
        return device.createBindGroup({
            label: 'tonemap-bg',
            layout: tonemapPipeline.getBindGroupLayout(0),
            entries: [
                { binding: 0, resource: linearSampler },
                { binding: 1, resource: hdrView },
                { binding: 2, resource: bloomView },
                { binding: 3, resource: { buffer: tonemapUniform.handle } },
            ],
        });
    }

    // ----- Camera -----
    const camera = new Camera();
    camera.aspect = ctx.aspect;
    camera.fovDeg = 50;
    camera.near = 0.1;
    camera.far = 1000;

    let orbitAngleDeg = 0;
    function applyOrbit(dt: number): void {
        if (!rt.autoOrbit) return;
        const def = ATTRACTORS[rt.attractorId];
        orbitAngleDeg = (orbitAngleDeg + rt.orbitSpeedDegPerSec * dt) % 360;
        const a = (orbitAngleDeg * Math.PI) / 180;
        const cx = def.orbitCenter[0] + Math.cos(a) * rt.orbitRadius;
        const cy = def.orbitCenter[1] + 0.4 * rt.orbitRadius;
        const cz = def.orbitCenter[2] + Math.sin(a) * rt.orbitRadius;
        camera.position = [cx, cy, cz];
        camera.lookAt(def.orbitCenter[0], def.orbitCenter[1], def.orbitCenter[2]);
    }

    // ----- lil-gui panel -----
    const panel = new ParamPanel({ title: 'strange-attractors', persistKey: 'strange-attractors' });

    const attractorFolder = panel.addFolder('Attractor');
    attractorFolder.addDropdown({
        getValue: () => rt.attractorId,
        setValue: (v: string) => switchAttractor(v as AttractorId),
        label: 'System',
        options: ATTRACTOR_ORDER.map((id) => ({ value: id, label: ATTRACTORS[id].label })),
    });
    let paramsFolder = attractorFolder.addFolder('Parameters');
    rebuildParamsFolder();

    function rebuildParamsFolder(): void {
        paramsFolder.clear();
        const def = ATTRACTORS[rt.attractorId];
        for (let i = 0; i < def.params.length; i++) {
            const p = def.params[i]!;
            paramsFolder.addNumber(
                {
                    get [p.name]() { return rt.paramValues[i] ?? p.default; },
                    set [p.name](v: number) { rt.paramValues[i] = v; uploadSim(); },
                } as Record<string, number>,
                p.name, p.min, p.max, p.step,
            ).label = p.label;
        }
    }

    function switchAttractor(id: AttractorId): void {
        rt.attractorId = id;
        const def = ATTRACTORS[id];
        rt.paramValues = def.params.map((p) => p.default);
        rt.substeps = def.defaultSubsteps;
        rt.simDt = def.defaultSimDt;
        rt.pointSize = def.defaultPointSize;
        rt.colorSpeedScale = def.defaultColorSpeedScale;
        rt.orbitRadius = def.orbitRadius;
        orbitAngleDeg = 0;
        rebuildParamsFolder();
        reseed();
        uploadSim();
        clearAccumulators = true;     // request a one-frame full clear
        log.info(`switched to ${def.label}`);
    }

    const integrationFolder = panel.addFolder('Integration');
    integrationFolder.addNumber(rt, 'substeps', 1, 64, 1).onChange(() => uploadSim());
    integrationFolder.addNumber(rt, 'simDt', 0.0005, 0.05, 0.0005).onChange(() => uploadSim());
    integrationFolder.addNumber(rt, 'initSeed', 0, 0xffffffff, 1).onChange(() => reseed());
    integrationFolder.addButton('Reseed particles', () => reseed());

    const renderFolder = panel.addFolder('Rendering');
    renderFolder.addNumber(rt, 'pointSize', 0.5, 8, 0.05);
    renderFolder.addNumber(rt, 'depthAttenK', 0, 0.2, 0.001);

    const trailFolder = panel.addFolder('Trail');
    trailFolder.addDropdown({
        getValue: () => rt.decayPreset,
        setValue: (v: string) => {
            rt.decayPreset = v as RuntimeState['decayPreset'];
            if (v === 'manifold') rt.alpha = 1.0;
            else if (v === 'motion') rt.alpha = 0.97;
            // 'custom' keeps current alpha
        },
        label: 'Preset',
        options: [
            { value: 'manifold', label: 'Manifold (persistent)' },
            { value: 'motion',   label: 'Motion (decay 0.97)' },
            { value: 'custom',   label: 'Custom' },
        ],
    });
    trailFolder.addNumber(rt, 'alpha', 0.5, 1.0, 0.001).onChange(() => {
        rt.decayPreset = 'custom';
    });

    const colorFolder = panel.addFolder('Color');
    colorFolder.addDropdown({
        getValue: () => COLORMAP_ORDER[rt.colormap]!,
        setValue: (v: string) => { rt.colormap = COLORMAP_INDEX[v as keyof typeof COLORMAP_INDEX]; },
        label: 'Colormap',
        options: COLORMAP_ORDER.map((c) => ({ value: c, label: c })),
    });
    colorFolder.addNumber(rt, 'colorSpeedScale', 0.1, 200, 0.1);
    colorFolder.addNumber(rt, 'colorExponent', 0.1, 4, 0.01);

    const postFolder = panel.addFolder('Post');
    postFolder.addNumber(rt, 'bloomIntensity', 0, 2, 0.01);
    postFolder.addNumber(rt, 'bloomThreshold', 0, 5, 0.01);
    postFolder.addNumber(rt, 'bloomSoftKnee', 0, 2, 0.01);
    postFolder.addNumber(rt, 'exposure', 0.1, 4, 0.01);

    const cameraFolder = panel.addFolder('Camera');
    cameraFolder.addBoolean(rt, 'autoOrbit').label = 'Auto-orbit';
    cameraFolder.addNumber(rt, 'orbitSpeedDegPerSec', 0, 60, 0.1);
    cameraFolder.addNumber(rt, 'orbitRadius', 0.1, 200, 0.1);

    const stateFolder = panel.addFolder('State');
    stateFolder.addButton('Save (F5)', () => { void doSave(); });
    stateFolder.addButton('Load... (F9)', () => { triggerFileLoad(); });

    // ----- Hot-reload -----
    const hot = new HotReloader();
    function logReload(path: string, ok: boolean, err?: string): void {
        if (ok) { hot.reportSuccess(path); log.info(`${path} reloaded ✓`); }
        else { hot.reportFailure(path, err ?? 'reload failed'); }
    }
    // For Phase 2 v1, hot-reload of compute/render pipelines is logged but does
    // not currently rebuild the pipelines (the existing common-web Pipeline
    // wrappers aren't used here — see pipelines section). A future common-web
    // enhancement will make this declarative; for now, hot-reloads only the
    // shader text (page reload picks up structural changes).
    [PATH_INTEGRATE, PATH_DECAY, PATH_FULLSCREEN_VS, PATH_SPLAT_VS, PATH_SPLAT_FS,
     PATH_BLOOM_EXTRACT, PATH_BLOOM_BLUR, PATH_TONEMAP].forEach((p) => {
        hot.watch(p, async (path) => {
            log.info(`${path} changed (page reload to apply structural changes)`);
            logReload(path, true);
        });
    });

    // ----- Resize handling -----
    let resizeTimer: number | null = null;
    let pendingResize = false;
    window.addEventListener('resize', () => {
        if (resizeTimer !== null) window.clearTimeout(resizeTimer);
        resizeTimer = window.setTimeout(() => {
            pendingResize = true;
            resizeTimer = null;
        }, 100);
    });

    // ----- F5 / F9 capture -----
    const stateWriter = new StateWriter('captures');
    const stateReader = StateReader;
    let nextCapture = 0;

    interface CaptureMeta {
        camera: JsonObject;
        attractor: AttractorId;
        params: number[];
        substeps: number;
        simDt: number;
        pointSize: number;
        depthAttenK: number;
        colorSpeedScale: number;
        colorExponent: number;
        colormap: number;
        bloomIntensity: number;
        bloomThreshold: number;
        bloomSoftKnee: number;
        exposure: number;
        alpha: number;
        decayPreset: RuntimeState['decayPreset'];
        autoOrbit: boolean;
        orbitSpeedDegPerSec: number;
        orbitRadius: number;
        initSeed: number;
        orbitAngleDeg: number;
        schemaVersion: 1;
    }

    async function doSave(): Promise<void> {
        stateWriter.beginFrame(nextCapture);
        const meta: CaptureMeta = {
            camera: camera.toJson(),
            attractor: rt.attractorId,
            params: [...rt.paramValues],
            substeps: rt.substeps,
            simDt: rt.simDt,
            pointSize: rt.pointSize,
            depthAttenK: rt.depthAttenK,
            colorSpeedScale: rt.colorSpeedScale,
            colorExponent: rt.colorExponent,
            colormap: rt.colormap,
            bloomIntensity: rt.bloomIntensity,
            bloomThreshold: rt.bloomThreshold,
            bloomSoftKnee: rt.bloomSoftKnee,
            exposure: rt.exposure,
            alpha: rt.alpha,
            decayPreset: rt.decayPreset,
            autoOrbit: rt.autoOrbit,
            orbitSpeedDegPerSec: rt.orbitSpeedDegPerSec,
            orbitRadius: rt.orbitRadius,
            initSeed: rt.initSeed,
            orbitAngleDeg,
            schemaVersion: 1,
        };
        stateWriter.setMeta('strangeAttractors', meta as unknown as JsonValue);
        await stateWriter.endFrame();
        log.info(`captured strange_attractors_${nextCapture.toString().padStart(4, '0')}.zip`);
        nextCapture++;
    }

    function triggerFileLoad(): void {
        const inp = document.createElement('input');
        inp.type = 'file';
        inp.accept = '.zip';
        inp.onchange = async (): Promise<void> => {
            const f = inp.files?.[0];
            if (!f) return;
            const cap = await stateReader.fromFile(f);
            if (!cap) return;
            const m = cap.meta('strangeAttractors') as unknown as CaptureMeta | undefined;
            if (!m) {
                log.error('capture missing strangeAttractors meta');
                return;
            }
            applyCapture(m);
            log.info(`loaded ${cap.directoryName}`);
        };
        inp.click();
    }

    function applyCapture(m: CaptureMeta): void {
        rt.attractorId = m.attractor;
        rt.paramValues = [...m.params];
        rt.substeps = m.substeps;
        rt.simDt = m.simDt;
        rt.pointSize = m.pointSize;
        rt.depthAttenK = m.depthAttenK;
        rt.colorSpeedScale = m.colorSpeedScale;
        rt.colorExponent = m.colorExponent;
        rt.colormap = m.colormap;
        rt.bloomIntensity = m.bloomIntensity;
        rt.bloomThreshold = m.bloomThreshold;
        rt.bloomSoftKnee = m.bloomSoftKnee;
        rt.exposure = m.exposure;
        rt.alpha = m.alpha;
        rt.decayPreset = m.decayPreset;
        rt.autoOrbit = m.autoOrbit;
        rt.orbitSpeedDegPerSec = m.orbitSpeedDegPerSec;
        rt.orbitRadius = m.orbitRadius;
        rt.initSeed = m.initSeed;
        orbitAngleDeg = m.orbitAngleDeg;
        if (m.camera) camera.fromJson(m.camera);
        rebuildParamsFolder();
        reseed();
        uploadSim();
        clearAccumulators = true;
    }

    // F5 / F9 keyboard
    let prevF5 = false, prevF9 = false;
    window.addEventListener('keydown', (e) => {
        if (e.code === 'F5') { e.preventDefault(); if (!prevF5) { prevF5 = true; void doSave(); } }
        if (e.code === 'F9') { e.preventDefault(); if (!prevF9) { prevF9 = true; triggerFileLoad(); } }
    });
    window.addEventListener('keyup', (e) => {
        if (e.code === 'F5') prevF5 = false;
        if (e.code === 'F9') prevF9 = false;
    });

    // ----- Input -----
    const readInput = snapshotInput(canvas);

    // ----- Main loop -----
    let last = performance.now();
    let pingPongIndex = 0;
    let clearAccumulators = true;     // request one-frame full clear

    function frame(): void {
        const now = performance.now();
        const dt = (now - last) / 1000;
        last = now;

        if (pendingResize) {
            const dim = targetDimensions();
            rtWidth = dim.width; rtHeight = dim.height;
            canvas.width = rtWidth; canvas.height = rtHeight;
            destroySizedResources(sized);
            sized = createSizedResources(rtWidth, rtHeight);
            pendingResize = false;
            clearAccumulators = true;
        }

        // Camera
        camera.aspect = rtWidth / rtHeight;
        if (rt.autoOrbit) applyOrbit(dt);
        else camera.update(dt, readInput());

        // Upload uniforms
        uploadRenderUniform(camera.viewProjection() as Float32Array, rtWidth, rtHeight);
        uploadScalarUniform(decayUniform, [rt.alpha, 0, 0, 0]);
        uploadScalarUniform(bloomExtractUniform, [rt.bloomThreshold, rt.bloomSoftKnee, rt.bloomIntensity, 0]);
        uploadScalarUniform(tonemapUniform, [rt.bloomIntensity, rt.exposure, 0, 0]);

        // Identify ping-pong roles for this frame.
        const accumCurr = pingPongIndex === 0 ? sized.accumA : sized.accumB;
        const accumNext = pingPongIndex === 0 ? sized.accumB : sized.accumA;
        const accumCurrView = accumCurr.createView();
        const accumNextView = accumNext.createView();
        const bloomHalfView = sized.bloomHalf.createView();
        const bloomHalfHView = sized.bloomHalfH.createView();
        const bloomHalfVView = sized.bloomHalfV.createView();

        // Begin GPU frame.
        const fr = renderer.beginFrame();

        // 1. Integrate (compute).
        const integratePass = fr.encoder.beginComputePass({ label: 'integrate' });
        integratePass.setPipeline(integratePipeline);
        integratePass.setBindGroup(0, integrateBg);
        integratePass.dispatchWorkgroups(Math.ceil(PARTICLE_COUNT / 64));
        integratePass.end();

        // 2. Decay pass — render to accumNext from accumCurr * alpha.
        // If clearAccumulators is set, force alpha=0 for this pass to wipe history.
        if (clearAccumulators) {
            uploadScalarUniform(decayUniform, [0, 0, 0, 0]);
            clearAccumulators = false;
        }
        const decayBg = makeDecayBg(accumCurrView);
        {
            const pass = fr.encoder.beginRenderPass({
                label: 'decay',
                colorAttachments: [{
                    view: accumNextView,
                    clearValue: { r: 0, g: 0, b: 0, a: 0 },
                    loadOp: 'clear',
                    storeOp: 'store',
                }],
            });
            pass.setPipeline(decayPipeline);
            pass.setBindGroup(0, decayBg);
            pass.draw(3, 1, 0, 0);
            pass.end();
        }

        // 3. Splat pass — additive over accumNext.
        {
            const pass = fr.encoder.beginRenderPass({
                label: 'splat',
                colorAttachments: [{
                    view: accumNextView,
                    loadOp: 'load',
                    storeOp: 'store',
                }],
            });
            pass.setPipeline(splatPipeline);
            pass.setBindGroup(0, splatBg);
            pass.draw(6, PARTICLE_COUNT, 0, 0);
            pass.end();
        }

        // 4. Bloom extract — accumNext (half-res) → bloomHalf.
        {
            const bg = makeBloomExtractBg(accumNextView, bloomHalfView);
            const pass = fr.encoder.beginComputePass({ label: 'bloom-extract' });
            pass.setPipeline(bloomExtractPipeline);
            pass.setBindGroup(0, bg);
            const halfW = Math.max(1, rtWidth >> 1);
            const halfH = Math.max(1, rtHeight >> 1);
            pass.dispatchWorkgroups(Math.ceil(halfW / 8), Math.ceil(halfH / 8), 1);
            pass.end();
        }

        // 5. Bloom blur horizontal — bloomHalf → bloomHalfH.
        {
            const bg = makeBloomBlurBg(bloomHalfView, bloomHalfHView, bloomBlurUniformH);
            const pass = fr.encoder.beginComputePass({ label: 'bloom-blur-h' });
            pass.setPipeline(bloomBlurPipeline);
            pass.setBindGroup(0, bg);
            const halfW = Math.max(1, rtWidth >> 1);
            const halfH = Math.max(1, rtHeight >> 1);
            pass.dispatchWorkgroups(Math.ceil(halfW / 8), Math.ceil(halfH / 8), 1);
            pass.end();
        }

        // 6. Bloom blur vertical — bloomHalfH → bloomHalfV.
        {
            const bg = makeBloomBlurBg(bloomHalfHView, bloomHalfVView, bloomBlurUniformV);
            const pass = fr.encoder.beginComputePass({ label: 'bloom-blur-v' });
            pass.setPipeline(bloomBlurPipeline);
            pass.setBindGroup(0, bg);
            const halfW = Math.max(1, rtWidth >> 1);
            const halfH = Math.max(1, rtHeight >> 1);
            pass.dispatchWorkgroups(Math.ceil(halfW / 8), Math.ceil(halfH / 8), 1);
            pass.end();
        }

        // 7. Tonemap to swapchain.
        {
            const tonemapBg = makeTonemapBg(accumNextView, bloomHalfVView);
            const pass = renderer.beginRendering(fr, [0, 0, 0, 1.0]);
            pass.setPipeline(tonemapPipeline);
            pass.setBindGroup(0, tonemapBg);
            pass.draw(3, 1, 0, 0);
            renderer.endRendering(pass);
        }

        renderer.endFrame(fr);

        // Swap.
        pingPongIndex = 1 - pingPongIndex;

        requestAnimationFrame(frame);
    }

    requestAnimationFrame(frame);
    log.info('strange-attractors: entered main loop');
}

void main();
