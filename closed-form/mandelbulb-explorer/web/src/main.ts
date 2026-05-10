import {
    initLogger, log,
    Camera, HotReloader, ParamPanel, StateWriter, StateReader,
    snapshotInput,
    Context, Renderer,
    Texture, TextureType,
    RenderPipeline,
    Buffer, MemoryUsage,
    type JsonValue, type JsonObject,
} from '@gpusims/common-web';

import fullscreenVert from '../shaders/fullscreen.vert.wgsl?raw';
import raymarchFrag   from '../shaders/raymarch.frag.wgsl?raw';
import tonemapFrag    from '../shaders/tonemap.frag.wgsl?raw';

// HMR-relative paths. Must match the path Vite's plugin emits.
const VERT_PATH     = 'shaders/fullscreen.vert.wgsl';
const RAYMARCH_PATH = 'shaders/raymarch.frag.wgsl';
const TONEMAP_PATH  = 'shaders/tonemap.frag.wgsl';

const MAX_DPR = 2.0;
const HDR_FORMAT: GPUTextureFormat = 'rgba16float';

// Default initial camera placement. Off-axis to immediately show fractal
// structure (a dead-on (0,0,2.5) view reads as a featureless blob).
const INITIAL_POSITION: [number, number, number] = [1.5, 0.8, 2.5];
const INITIAL_TARGET:   [number, number, number] = [0.0, 0.0, 0.0];
const INITIAL_FOV_DEG = 60;

interface RuntimeState {
    schemaVersion: number;

    // DE / mandelbulb math
    nPower: number;             // [2, 12], step 1
    iterCap: number;            // [1, 24]
    bailout: number;            // [1.5, 8]

    // Raymarch
    maxSteps: number;           // [16, 256]
    epsilonBase: number;        // [0.00005, 0.005]
    epsilonGrow: number;        // [0, 0.01]
    maxRayDist: number;         // [2, 32]

    // Lighting
    softShadowsEnabled: boolean;
    softShadowK: number;        // [2, 64]
    lightYawDeg: number;        // [-180, 180]
    lightPitchDeg: number;      // [-89, 89]
    ambient: number;            // [0, 0.3]

    // Coloring
    orbitTrapMode: 0 | 1 | 2;   // 0=point, 1=planes, 2=mixed
    trapRadius: number;         // [0.1, 4.0] — visual sliding range
    colorHot: string;           // hex
    colorCool: string;          // hex
    bgColor: string;            // hex

    // Output
    renderScale: number;        // [0.5, 1.0]
    exposure: number;           // [0.1, 4.0]

    // Animation
    autoMorphEnabled: boolean;
    autoMorphMin: number;       // [2, 11]
    autoMorphMax: number;       // [3, 12]
    autoMorphPeriodSec: number; // [4, 120]
    autoMorphPhaseAtT0: number; // tracks where the wave was on toggle, so it doesn't jump
}

function defaultRuntime(): RuntimeState {
    return {
        schemaVersion: 1,
        nPower: 8,
        iterCap: 8,
        bailout: 2.0,
        maxSteps: 96,
        epsilonBase: 0.001,
        epsilonGrow: 0.001,
        maxRayDist: 8.0,
        softShadowsEnabled: true,
        softShadowK: 16.0,
        lightYawDeg: 50,
        lightPitchDeg: 35,
        ambient: 0.05,
        orbitTrapMode: 0,
        trapRadius: 1.0,
        colorHot: '#ff6040',
        colorCool: '#2050a0',
        bgColor: '#040408',
        renderScale: 1.0,
        exposure: 1.0,
        autoMorphEnabled: false,
        autoMorphMin: 4,
        autoMorphMax: 10,
        autoMorphPeriodSec: 24,
        autoMorphPhaseAtT0: 0,
    };
}

function hexToRgb01(h: string): [number, number, number] {
    const m = h.replace('#', '').match(/.{2}/g);
    if (!m || m.length < 3) return [1, 1, 1];
    return [
        parseInt(m[0]!, 16) / 255,
        parseInt(m[1]!, 16) / 255,
        parseInt(m[2]!, 16) / 255,
    ];
}

function degToRad(d: number): number { return (d * Math.PI) / 180; }

async function main(): Promise<void> {
    initLogger();
    log.info('mandelbulb-explorer: starting up');

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
        // Per project-state.md known issue 1, do NOT request timestamp-query
        // — profiler degrades to CPU-only timing. Phase 4 doesn't need GPU
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
    let { width: canvasW, height: canvasH } = targetDimensions();
    canvas.width = canvasW;
    canvas.height = canvasH;

    // ----- Runtime state -----
    const rt: RuntimeState = defaultRuntime();

    // ----- Offscreen HDR RT (recreated when canvas/render-scale changes) -----
    let offscreen: Texture | null = null;
    let offscreenView: GPUTextureView | null = null;
    function recreateOffscreen(): void {
        const w = Math.max(1, Math.floor(canvasW * rt.renderScale));
        const h = Math.max(1, Math.floor(canvasH * rt.renderScale));
        offscreen = new Texture(ctx, {
            type: TextureType.e2D,
            extent: { width: w, height: h, depthOrArrayLayers: 1 },
            format: HDR_FORMAT,
            usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING,
            label: 'mandelbulb-offscreen',
        });
        offscreenView = offscreen.handle.createView({ label: 'mandelbulb-offscreen-view' });
        log.info(`offscreen RT: ${w}x${h} ${HDR_FORMAT}`);
    }
    recreateOffscreen();

    const linearSampler = device.createSampler({
        label: 'mandelbulb-linear',
        magFilter: 'linear',
        minFilter: 'linear',
    });

    // ----- Uniforms -----
    // Raymarch uniform: 12 vec4<f32> = 192 bytes. See Uniforms struct in
    // raymarch.frag.wgsl for the exact field mapping.
    const RAYMARCH_UNIFORM_SIZE = 192;
    const raymarchUniform = new Buffer(ctx, {
        size: RAYMARCH_UNIFORM_SIZE,
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'raymarch-uniform',
    });
    const rmBytes = new ArrayBuffer(RAYMARCH_UNIFORM_SIZE);
    const rmF32 = new Float32Array(rmBytes);

    // Tonemap uniform: 1 vec4<f32> = 16 bytes.
    const tonemapUniform = new Buffer(ctx, {
        size: 16,
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        memory: MemoryUsage.DeviceLocal,
        label: 'tonemap-uniform',
    });
    const tmBytes = new ArrayBuffer(16);
    const tmF32 = new Float32Array(tmBytes);

    // ----- Camera -----
    const camera = new Camera();
    camera.mode = 'free-fly';
    camera.position = INITIAL_POSITION;
    camera.fovDeg = INITIAL_FOV_DEG;
    camera.near = 0.001;
    camera.far = 32.0;
    camera.moveSpeed = 0.6;
    camera.lookSpeed = 0.2;

    camera.lookAt(INITIAL_TARGET[0], INITIAL_TARGET[1], INITIAL_TARGET[2]);

    function resetView(): void {
        camera.position = INITIAL_POSITION;
        camera.lookAt(INITIAL_TARGET[0], INITIAL_TARGET[1], INITIAL_TARGET[2]);
    }

    // ----- Render pipelines -----
    const raymarchPipeline = await RenderPipeline.create(ctx, {
        vertexSource: fullscreenVert,
        fragmentSource: raymarchFrag,
        vertexPath: VERT_PATH,
        fragmentPath: RAYMARCH_PATH,
        bindings: [
            // group(0) binding(0): uniform buffer
            { binding: 0, visibility: GPUShaderStage.FRAGMENT, buffer: { type: 'uniform' } },
        ],
        colorFormats: [HDR_FORMAT],
        primitive: { topology: 'triangle-list' },
        label: 'raymarch',
    });

    const tonemapPipeline = await RenderPipeline.create(ctx, {
        vertexSource: fullscreenVert,
        fragmentSource: tonemapFrag,
        vertexPath: VERT_PATH,
        fragmentPath: TONEMAP_PATH,
        bindings: [
            { binding: 0, visibility: GPUShaderStage.FRAGMENT, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.FRAGMENT, sampler: { type: 'filtering' } },
            { binding: 2, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float' } },
        ],
        colorFormats: [ctx.preferredFormat],
        primitive: { topology: 'triangle-list' },
        label: 'tonemap',
    });

    const raymarchBindGroup = raymarchPipeline.createBindGroup(
        [{ binding: 0, resource: { buffer: raymarchUniform.handle } }],
        'raymarch-bg',
    );
    let tonemapBindGroup = tonemapPipeline.createBindGroup(
        [
            { binding: 0, resource: { buffer: tonemapUniform.handle } },
            { binding: 1, resource: linearSampler },
            { binding: 2, resource: offscreenView! },
        ],
        'tonemap-bg',
    );
    function rebuildTonemapBindGroup(): void {
        tonemapBindGroup = tonemapPipeline.createBindGroup(
            [
                { binding: 0, resource: { buffer: tonemapUniform.handle } },
                { binding: 1, resource: linearSampler },
                { binding: 2, resource: offscreenView! },
            ],
            'tonemap-bg',
        );
    }

    // ----- Hot reload -----
    const hot = new HotReloader();
    hot.watch(VERT_PATH, async (_p, src) => {
        const e1 = await raymarchPipeline.reload('vertex', src);
        const e2 = await tonemapPipeline.reload('vertex', src);
        const err = e1 ?? e2;
        if (err) hot.reportFailure(VERT_PATH, err); else hot.reportSuccess(VERT_PATH);
    });
    hot.watch(RAYMARCH_PATH, async (_p, src) => {
        const err = await raymarchPipeline.reload('fragment', src);
        if (err) hot.reportFailure(RAYMARCH_PATH, err); else hot.reportSuccess(RAYMARCH_PATH);
    });
    hot.watch(TONEMAP_PATH, async (_p, src) => {
        const err = await tonemapPipeline.reload('fragment', src);
        if (err) hot.reportFailure(TONEMAP_PATH, err); else hot.reportSuccess(TONEMAP_PATH);
    });

    // ----- Profiler omitted for v1 -----
    // Strange-attractors doesn't use one either; mandelbulb v1 follows suit.
    // CPU-only frame timing is available via performance.now() deltas in the
    // main loop if needed for in-page debugging.

    // ----- ParamPanel -----
    const panel = new ParamPanel({ title: 'Mandelbulb Explorer', persistKey: 'mandelbulb-explorer' });

    const fDE = panel.addFolder('DE');
    fDE.addNumber(rt, 'nPower', 2, 12, 1).name('n (power)');
    fDE.addNumber(rt, 'iterCap', 1, 24, 1).name('iter cap');
    fDE.addNumber(rt, 'bailout', 1.5, 8.0, 0.1);

    const fMarch = panel.addFolder('Raymarch');
    fMarch.addNumber(rt, 'maxSteps', 16, 256, 8);
    fMarch.addNumber(rt, 'epsilonBase', 0.00005, 0.005, 0.00005);
    fMarch.addNumber(rt, 'epsilonGrow', 0.0, 0.01, 0.0005);
    fMarch.addNumber(rt, 'maxRayDist', 2.0, 32.0, 0.5);

    const fLight = panel.addFolder('Lighting');
    fLight.addBoolean(rt, 'softShadowsEnabled').name('soft shadows');
    fLight.addNumber(rt, 'softShadowK', 2, 64, 1).name('shadow strength k');
    fLight.addNumber(rt, 'lightYawDeg',   -180, 180, 1).name('light yaw');
    fLight.addNumber(rt, 'lightPitchDeg', -89,  89,  1).name('light pitch');
    fLight.addNumber(rt, 'ambient', 0, 0.3, 0.005);

    const fColor = panel.addFolder('Coloring');
    fColor.addDropdown({
        label: 'orbit trap',
        options: [
            { label: 'Point at origin', value: '0' },
            { label: 'Coordinate planes', value: '1' },
            { label: 'Mixed', value: '2' },
        ],
        getValue: () => String(rt.orbitTrapMode),
        setValue: (v: string) => { rt.orbitTrapMode = (parseInt(v, 10) as 0 | 1 | 2); },
    });
    fColor.addNumber(rt, 'trapRadius', 0.1, 4.0, 0.05).name('trap radius');
    fColor.addColor(rt, 'colorHot').name('hot tint');
    fColor.addColor(rt, 'colorCool').name('cool tint');
    fColor.addColor(rt, 'bgColor').name('background');

    const fOut = panel.addFolder('Output');
    fOut.addNumber(rt, 'renderScale', 0.5, 1.0, 0.05).name('render scale')
        .onFinishChange(() => { recreateOffscreen(); rebuildTonemapBindGroup(); });
    fOut.addNumber(rt, 'exposure', 0.1, 4.0, 0.01);

    const fCam = panel.addFolder('Camera');
    fCam.addNumber(camera, 'fovDeg', 20, 110, 1).name('FOV');
    fCam.addNumber(camera, 'moveSpeed', 0.05, 4.0, 0.05).name('move speed');
    fCam.addNumber(camera, 'lookSpeed', 0.05, 1.0, 0.01).name('look speed');
    fCam.addButton('Reset view', resetView);

    const fAnim = panel.addFolder('Animation');
    fAnim.addBoolean(rt, 'autoMorphEnabled').name('auto-morph n')
        .onChange(() => {
            // Keep the wave continuous — record the equivalent phase shift
            // that maps t = performance.now() to the current rt.nPower.
            if (rt.autoMorphEnabled) {
                rt.autoMorphPhaseAtT0 = performance.now() / 1000.0;
            }
        });
    fAnim.addNumber(rt, 'autoMorphMin', 2, 11, 1).name('morph min');
    fAnim.addNumber(rt, 'autoMorphMax', 3, 12, 1).name('morph max');
    fAnim.addNumber(rt, 'autoMorphPeriodSec', 4, 120, 1).name('period (s)');

    panel.addButton('Save (F5)', () => { void doSave(); });
    panel.addButton('Load... (F9)', () => { triggerFileLoad(); });

    // ----- Capture (F5) and load (F9) -----
    const stateWriter = new StateWriter('captures');
    let nextCapture = 0;

    interface CaptureMeta {
        schemaVersion: 1;
        camera: JsonObject;

        // DE
        nPower: number;
        iterCap: number;
        bailout: number;

        // Raymarch
        maxSteps: number;
        epsilonBase: number;
        epsilonGrow: number;
        maxRayDist: number;

        // Lighting
        softShadowsEnabled: boolean;
        softShadowK: number;
        lightYawDeg: number;
        lightPitchDeg: number;
        ambient: number;

        // Coloring
        orbitTrapMode: 0 | 1 | 2;
        trapRadius: number;
        colorHot: string;
        colorCool: string;
        bgColor: string;

        // Output
        renderScale: number;
        exposure: number;

        // Animation
        autoMorphEnabled: boolean;
        autoMorphMin: number;
        autoMorphMax: number;
        autoMorphPeriodSec: number;
        autoMorphPhaseAtT0: number;
    }

    async function doSave(): Promise<void> {
        stateWriter.beginFrame(nextCapture);
        const meta: CaptureMeta = {
            schemaVersion: 1,
            camera: camera.toJson(),
            nPower: rt.nPower,
            iterCap: rt.iterCap,
            bailout: rt.bailout,
            maxSteps: rt.maxSteps,
            epsilonBase: rt.epsilonBase,
            epsilonGrow: rt.epsilonGrow,
            maxRayDist: rt.maxRayDist,
            softShadowsEnabled: rt.softShadowsEnabled,
            softShadowK: rt.softShadowK,
            lightYawDeg: rt.lightYawDeg,
            lightPitchDeg: rt.lightPitchDeg,
            ambient: rt.ambient,
            orbitTrapMode: rt.orbitTrapMode,
            trapRadius: rt.trapRadius,
            colorHot: rt.colorHot,
            colorCool: rt.colorCool,
            bgColor: rt.bgColor,
            renderScale: rt.renderScale,
            exposure: rt.exposure,
            autoMorphEnabled: rt.autoMorphEnabled,
            autoMorphMin: rt.autoMorphMin,
            autoMorphMax: rt.autoMorphMax,
            autoMorphPeriodSec: rt.autoMorphPeriodSec,
            autoMorphPhaseAtT0: rt.autoMorphPhaseAtT0,
        };
        stateWriter.setMeta('mandelbulbExplorer', meta as unknown as JsonValue);
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
            const m = cap.meta('mandelbulbExplorer') as unknown as CaptureMeta | undefined;
            if (!m) {
                log.error('capture missing mandelbulbExplorer meta');
                return;
            }
            applyCapture(m);
            log.info(`loaded ${cap.directoryName}`);
        };
        inp.click();
    }

    function applyCapture(m: CaptureMeta): void {
        rt.nPower = m.nPower;
        rt.iterCap = m.iterCap;
        rt.bailout = m.bailout;
        rt.maxSteps = m.maxSteps;
        rt.epsilonBase = m.epsilonBase;
        rt.epsilonGrow = m.epsilonGrow;
        rt.maxRayDist = m.maxRayDist;
        rt.softShadowsEnabled = m.softShadowsEnabled;
        rt.softShadowK = m.softShadowK;
        rt.lightYawDeg = m.lightYawDeg;
        rt.lightPitchDeg = m.lightPitchDeg;
        rt.ambient = m.ambient;
        rt.orbitTrapMode = m.orbitTrapMode;
        rt.trapRadius = m.trapRadius;
        rt.colorHot = m.colorHot;
        rt.colorCool = m.colorCool;
        rt.bgColor = m.bgColor;
        rt.renderScale = m.renderScale;
        rt.exposure = m.exposure;
        rt.autoMorphEnabled = m.autoMorphEnabled;
        rt.autoMorphMin = m.autoMorphMin;
        rt.autoMorphMax = m.autoMorphMax;
        rt.autoMorphPeriodSec = m.autoMorphPeriodSec;
        rt.autoMorphPhaseAtT0 = m.autoMorphPhaseAtT0;
        if (m.camera) camera.fromJson(m.camera);
        recreateOffscreen();
        rebuildTonemapBindGroup();
    }

    // ----- Input + keyboard hotkeys -----
    const readInput = snapshotInput(canvas);
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
            canvas.width = canvasW;
            canvas.height = canvasH;
            recreateOffscreen();
            rebuildTonemapBindGroup();
            resizeTimer = null;
        }, 100);
    });

    // ----- Auto-morph helper -----
    function effectiveN(nowSec: number): number {
        if (!rt.autoMorphEnabled) return rt.nPower;
        const tNorm = ((nowSec - rt.autoMorphPhaseAtT0) % rt.autoMorphPeriodSec) / rt.autoMorphPeriodSec;
        const wave = 0.5 - 0.5 * Math.cos(tNorm * Math.PI * 2);   // 0..1
        const lo = Math.min(rt.autoMorphMin, rt.autoMorphMax);
        const hi = Math.max(rt.autoMorphMin, rt.autoMorphMax);
        return lo + wave * (hi - lo);
    }

    // ----- Uniform packing -----
    function writeRaymarchUniform(): void {
        camera.aspect = (canvasW / Math.max(canvasH, 1));
        const fwd  = camera.forward();
        const rgt  = camera.right();
        const upv  = camera.up();
        const fovTan = Math.tan(degToRad(camera.fovDeg) * 0.5);

        const lightYaw   = degToRad(rt.lightYawDeg);
        const lightPitch = degToRad(rt.lightPitchDeg);
        const lightDir = [
            Math.cos(lightPitch) * Math.cos(lightYaw),
            Math.sin(lightPitch),
            Math.cos(lightPitch) * Math.sin(lightYaw),
        ];

        const hot  = hexToRgb01(rt.colorHot);
        const cool = hexToRgb01(rt.colorCool);
        const bg   = hexToRgb01(rt.bgColor);

        const trapNorm = 1.0 / Math.max(rt.trapRadius * rt.trapRadius, 1e-6);

        const offW = Math.max(1, Math.floor(canvasW * rt.renderScale));
        const offH = Math.max(1, Math.floor(canvasH * rt.renderScale));

        // Field-by-field. Each row is one vec4.
        rmF32[ 0] = camera.position[0]; rmF32[ 1] = camera.position[1]; rmF32[ 2] = camera.position[2]; rmF32[ 3] = fovTan;
        rmF32[ 4] = rgt[0];             rmF32[ 5] = rgt[1];             rmF32[ 6] = rgt[2];             rmF32[ 7] = 0;
        rmF32[ 8] = upv[0];             rmF32[ 9] = upv[1];             rmF32[10] = upv[2];             rmF32[11] = 0;
        rmF32[12] = fwd[0];             rmF32[13] = fwd[1];             rmF32[14] = fwd[2];             rmF32[15] = 0;

        const nNow = effectiveN(performance.now() / 1000.0);
        rmF32[16] = nNow;               rmF32[17] = rt.bailout;         rmF32[18] = rt.epsilonBase;     rmF32[19] = rt.epsilonGrow;
        rmF32[20] = rt.maxRayDist;      rmF32[21] = rt.iterCap;         rmF32[22] = rt.maxSteps;        rmF32[23] = 0;

        rmF32[24] = lightDir[0]!;       rmF32[25] = lightDir[1]!;       rmF32[26] = lightDir[2]!;       rmF32[27] = rt.softShadowK;
        rmF32[28] = rt.softShadowsEnabled ? 1 : 0;
        rmF32[29] = rt.ambient;         rmF32[30] = 0;                   rmF32[31] = 0;

        rmF32[32] = hot[0];             rmF32[33] = hot[1];             rmF32[34] = hot[2];             rmF32[35] = trapNorm;
        rmF32[36] = cool[0];            rmF32[37] = cool[1];            rmF32[38] = cool[2];            rmF32[39] = rt.orbitTrapMode;
        rmF32[40] = bg[0];              rmF32[41] = bg[1];              rmF32[42] = bg[2];              rmF32[43] = 0;

        rmF32[44] = offW;               rmF32[45] = offH;               rmF32[46] = 0;                   rmF32[47] = 0;

        device.queue.writeBuffer(raymarchUniform.handle, 0, rmBytes);
    }

    function writeTonemapUniform(): void {
        tmF32[0] = rt.exposure; tmF32[1] = 0; tmF32[2] = 0; tmF32[3] = 0;
        device.queue.writeBuffer(tonemapUniform.handle, 0, tmBytes);
    }

    // ----- Main loop -----
    let lastT = performance.now();
    function frame(): void {
        const nowMs = performance.now();
        const dt = Math.min(0.1, (nowMs - lastT) / 1000.0);
        lastT = nowMs;

        const input = readInput();
        camera.update(dt, input);

        writeRaymarchUniform();
        writeTonemapUniform();

        const f = renderer.beginFrame();

        // Pass 1: raymarch into offscreen RT.
        const pass1Desc: GPURenderPassDescriptor = {
            colorAttachments: [{
                view: offscreenView!,
                loadOp: 'clear',
                storeOp: 'store',
                clearValue: { r: 0, g: 0, b: 0, a: 1 },
            }],
        };
        const pass1 = f.encoder.beginRenderPass(pass1Desc);
        raymarchPipeline.bind(pass1, raymarchBindGroup);
        pass1.draw(3, 1, 0, 0);
        pass1.end();

        // Pass 2: tonemap into the swap-chain image.
        const pass2 = renderer.beginRendering(f, [0, 0, 0, 1]);
        tonemapPipeline.bind(pass2, tonemapBindGroup);
        pass2.draw(3, 1, 0, 0);
        renderer.endRendering(pass2);

        renderer.endFrame(f);

        requestAnimationFrame(frame);
    }

    requestAnimationFrame(frame);
    log.info('mandelbulb-explorer: entered main loop');
}

void main();
