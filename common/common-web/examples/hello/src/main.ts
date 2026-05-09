import {
    initLogger, log,
    Camera, GpuProfiler, HotReloader, ParamPanel, StateWriter, StateReader,
    snapshotInput,
    Context, Renderer,
    Texture, TextureType,
    ComputePipeline, RenderPipeline,
    Buffer, MemoryUsage,
} from '@gpusims/common-web';

import gradientWgsl   from '../shaders/gradient.compute.wgsl?raw';
import fullscreenVert from '../shaders/fullscreen.vert.wgsl?raw';
import fullscreenFrag from '../shaders/fullscreen.frag.wgsl?raw';

// HMR-relative paths (must match Vite plugin's emitted `data.path`).
const GRADIENT_PATH = 'shaders/gradient.compute.wgsl';
const VERT_PATH     = 'shaders/fullscreen.vert.wgsl';
const FRAG_PATH     = 'shaders/fullscreen.frag.wgsl';

const RT_WIDTH = 1280;
const RT_HEIGHT = 720;

interface PushConstants {
    resolutionX: number;
    resolutionY: number;
    time: number;
    pad: number;
}

async function main(): Promise<void> {
    initLogger();
    log.info('hello: starting up');

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

    // ----- Resources -----
    const gradientTexture = new Texture(ctx, {
        type: TextureType.e2D,
        extent: { width: RT_WIDTH, height: RT_HEIGHT, depthOrArrayLayers: 1 },
        format: 'rgba8unorm',
        usage: GPUTextureUsage.STORAGE_BINDING |
               GPUTextureUsage.TEXTURE_BINDING |
               GPUTextureUsage.COPY_DST,
        label: 'gradient-image',
    });

    const sampler = ctx.device.createSampler({
        label: 'gradient-sampler',
        magFilter: 'linear',
        minFilter: 'linear',
    });

    // Push-constant-equivalent: a small uniform buffer rewritten each frame.
    const pushBuffer = new Buffer(ctx, {
        size: 16,  // 4 floats
        usage: GPUBufferUsage.UNIFORM,
        memory: MemoryUsage.DeviceLocal,
        label: 'gradient-push',
    });
    const pushBytes = new Float32Array(4);
    function uploadPush(p: PushConstants): void {
        pushBytes[0] = p.resolutionX;
        pushBytes[1] = p.resolutionY;
        pushBytes[2] = p.time;
        pushBytes[3] = p.pad;
        pushBuffer.uploadDirect(pushBytes);
    }

    // ----- Compute pipeline -----
    const computePipe = await ComputePipeline.create(ctx, {
        source: gradientWgsl,
        shaderPath: GRADIENT_PATH,
        bindings: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE,
              storageTexture: { access: 'write-only', format: 'rgba8unorm', viewDimension: '2d' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE,
              buffer: { type: 'uniform' } },
        ],
        label: 'gradient-compute',
    });
    let computeBg = computePipe.createBindGroup([
        { binding: 0, resource: gradientTexture.view },
        { binding: 1, resource: { buffer: pushBuffer.handle } },
    ], 'gradient-compute-bg');

    // ----- Render pipeline -----
    const renderPipe = await RenderPipeline.create(ctx, {
        vertexSource: fullscreenVert,
        fragmentSource: fullscreenFrag,
        vertexPath: VERT_PATH,
        fragmentPath: FRAG_PATH,
        vertexEntryPoint: 'vs_main',
        fragmentEntryPoint: 'fs_main',
        bindings: [
            { binding: 0, visibility: GPUShaderStage.FRAGMENT, sampler: { type: 'filtering' } },
            { binding: 1, visibility: GPUShaderStage.FRAGMENT,
              texture: { sampleType: 'float', viewDimension: '2d' } },
        ],
        colorFormats: [ctx.preferredFormat],
        label: 'fullscreen-render',
    });
    let renderBg = renderPipe.createBindGroup([
        { binding: 0, resource: sampler },
        { binding: 1, resource: gradientTexture.view },
    ], 'fullscreen-render-bg');

    // ----- Camera, profiler, hot-reload, state I/O -----
    const camera = new Camera();
    camera.aspect = ctx.aspect;

    const profiler = new GpuProfiler(ctx.device);
    const stateWriter = new StateWriter('captures');
    let nextCapture = 0;

    const hot = new HotReloader();
    hot.watch(GRADIENT_PATH, async (path, src) => {
        const err = await computePipe.reload(src);
        if (err) hot.reportFailure(path, err);
        else {
            hot.reportSuccess(path);
            // Bind group references the pipeline-derived layout, which is unchanged.
            log.info('compute reloaded ✓');
        }
    });
    hot.watch(VERT_PATH, async (path, src) => {
        const err = await renderPipe.reload('vertex', src);
        if (err) hot.reportFailure(path, err);
        else {
            hot.reportSuccess(path);
            log.info('vertex reloaded ✓');
        }
    });
    hot.watch(FRAG_PATH, async (path, src) => {
        const err = await renderPipe.reload('fragment', src);
        if (err) hot.reportFailure(path, err);
        else {
            hot.reportSuccess(path);
            log.info('fragment reloaded ✓');
        }
    });

    // ----- lil-gui parameter panel -----
    const panel = new ParamPanel({ title: 'hello-web', persistKey: 'hello-web' });

    const lensFolder = panel.addFolder('Lens');
    lensFolder.addNumber(camera, 'fovDeg', 10, 120, 0.1);
    lensFolder.addNumber(camera, 'near', 0.001, 10, 0.001);
    lensFolder.addNumber(camera, 'far',  1, 100000, 1);

    const flyFolder = panel.addFolder('Free-fly');
    flyFolder.addNumber(camera, 'moveSpeed', 0.1, 100, 0.1);
    flyFolder.addNumber(camera, 'lookSpeed', 0.01, 2, 0.01);
    flyFolder.addNumber(camera, 'boostMultiplier', 1, 50, 0.5);

    const stateFolder = panel.addFolder('State');
    stateFolder.addButton('Save (F5)', () => { void doSave(); });
    stateFolder.addButton('Load... (F9)', () => { triggerFileLoad(); });

    // ----- Input -----
    const readInput = snapshotInput(canvas);

    // F5 = save, F9 = load (file picker)
    let prevF5 = false, prevF9 = false;
    window.addEventListener('keydown', (e) => {
        if (e.code === 'F5') { e.preventDefault(); }
        if (e.code === 'F9') { e.preventDefault(); }
    });

    async function doSave(): Promise<void> {
        stateWriter.beginFrame(nextCapture);
        stateWriter.setMeta('camera', camera.toJson());
        stateWriter.setMeta('frame_time_ms', lastDtMs);
        const status = new Uint32Array([nextCapture]);
        stateWriter.saveBuffer('status', status, { description: 'frame counter at save time' });
        await stateWriter.endFrame();
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
            const cam = cap.meta('camera');
            if (cam && typeof cam === 'object' && !Array.isArray(cam)) {
                camera.fromJson(cam);
                log.info(`loaded ${cap.directoryName}`);
            }
        };
        inp.click();
    }

    // ----- Main loop -----
    let last = performance.now();
    let lastDtMs = 0;
    let t = 0;

    function frame(): void {
        const now = performance.now();
        const dt = (now - last) / 1000;
        last = now;
        lastDtMs = dt * 1000;
        t += dt;

        // F5 / F9 (rising-edge)
        const input = readInput();
        const f5 = input.keyW || false;  // we read F5/F9 separately
        const f9 = input.keyS || false;
        // Note: snapshotInput doesn't expose F5/F9 by default; check window-level keys
        // via an auxiliary pair of booleans tracked in the keydown listener if needed.
        void f5; void f9; void prevF5; void prevF9;

        camera.aspect = ctx.aspect;
        camera.update(dt, input);

        // Begin frame
        const fr = renderer.beginFrame();
        profiler.beginFrame(fr.inFlightIndex);

        // Compute pass: write gradient into the storage texture.
        uploadPush({ resolutionX: RT_WIDTH, resolutionY: RT_HEIGHT, time: t, pad: 0 });
        const gx = Math.ceil(RT_WIDTH / 16);
        const gy = Math.ceil(RT_HEIGHT / 16);
        const computeTs = profiler.timestampWritesFor('compute_gradient');
        if (computeTs) {
            computePipe.dispatch(fr.encoder, computeBg, gx, gy, 1, computeTs);
        } else {
            const s = profiler.scope(null, 'compute_gradient');
            computePipe.dispatch(fr.encoder, computeBg, gx, gy, 1);
            s.end();
        }

        // Render pass: sample the texture onto the canvas.
        const renderTs = profiler.timestampWritesFor('render');
        const pass = renderer.beginRendering(fr, [0.05, 0.05, 0.07, 1.0], renderTs ?? undefined);
        renderPipe.bind(pass, renderBg);
        pass.draw(3, 1, 0, 0);
        renderer.endRendering(pass);

        // Resolve queries before submit.
        profiler.resolveQueries(fr.encoder);
        profiler.endFrame();

        renderer.endFrame(fr);

        requestAnimationFrame(frame);
    }

    // F5/F9 listeners (separate from snapshotInput which doesn't track them).
    window.addEventListener('keydown', (e) => {
        if (e.code === 'F5' && !prevF5) { prevF5 = true; void doSave(); }
        if (e.code === 'F9' && !prevF9) { prevF9 = true; triggerFileLoad(); }
    });
    window.addEventListener('keyup', (e) => {
        if (e.code === 'F5') prevF5 = false;
        if (e.code === 'F9') prevF9 = false;
    });

    // Bind groups never need recreation because their resource references
    // (gradientTexture.view, sampler, pushBuffer.handle) are stable; mark them
    // as touched so the linter sees their use.
    void computeBg; void renderBg;

    requestAnimationFrame(frame);
    log.info('hello: entered main loop');
}

void main();
