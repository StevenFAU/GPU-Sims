import { log } from '../log.js';

export interface ContextOptions {
    /** Canvas to render into. Required. */
    canvas: HTMLCanvasElement;
    /** Power preference for adapter selection. Default: 'high-performance'. */
    powerPreference?: GPUPowerPreference;
    /** Required device features. Empty array = no extras beyond defaults. */
    requiredFeatures?: GPUFeatureName[];
    /** Optional device features (silently dropped if unavailable). */
    optionalFeatures?: GPUFeatureName[];
    /** Required device limits override. */
    requiredLimits?: Record<string, number>;
}

/**
 * Initialize WebGPU. Must be called before any other common-web WebGPU
 * function. Resolves once the device is ready and the canvas is configured.
 *
 *     const ctx = await Context.create({ canvas });
 *     const renderer = new Renderer(ctx);
 */
export class Context {
    readonly canvas: HTMLCanvasElement;
    readonly adapter: GPUAdapter;
    readonly device: GPUDevice;
    readonly canvasContext: GPUCanvasContext;
    readonly preferredFormat: GPUTextureFormat;
    /** Active features — what was actually granted by the device. */
    readonly features: ReadonlySet<string>;

    private constructor(canvas: HTMLCanvasElement, adapter: GPUAdapter, device: GPUDevice,
                        canvasContext: GPUCanvasContext, preferredFormat: GPUTextureFormat) {
        this.canvas = canvas;
        this.adapter = adapter;
        this.device = device;
        this.canvasContext = canvasContext;
        this.preferredFormat = preferredFormat;
        this.features = new Set(device.features as unknown as Iterable<string>);

        // Configure the canvas surface.
        canvasContext.configure({
            device,
            format: preferredFormat,
            alphaMode: 'opaque',
        });

        // Forward device-loss / uncaptured errors to the log.
        device.lost.then((info) => {
            log.error(`webgpu device lost: reason=${info.reason} message=${info.message}`);
        }).catch(() => { /* ignore */ });

        device.addEventListener('uncapturederror', (event) => {
            const e = event as GPUUncapturedErrorEvent;
            log.error(`webgpu uncaptured error: ${e.error.message}`);
        });
    }

    static async create(options: ContextOptions): Promise<Context> {
        if (!('gpu' in navigator) || !navigator.gpu) {
            throw new Error(
                'WebGPU is not available in this browser. ' +
                'See https://caniuse.com/webgpu for support; this site requires a current browser.'
            );
        }

        const adapter = await navigator.gpu.requestAdapter({
            powerPreference: options.powerPreference ?? 'high-performance',
        });
        if (!adapter) {
            throw new Error('WebGPU: no compatible adapter found');
        }

        // Filter optional features by what the adapter supports.
        const required = options.requiredFeatures ?? [];
        const optional = (options.optionalFeatures ?? []).filter((f) => adapter.features.has(f));
        const features: GPUFeatureName[] = [...required];
        for (const f of optional) {
            if (!features.includes(f)) features.push(f);
        }
        const deviceDescriptor: GPUDeviceDescriptor = { requiredFeatures: features };
        if (options.requiredLimits) {
            deviceDescriptor.requiredLimits = options.requiredLimits;
        }

        const device = await adapter.requestDevice(deviceDescriptor);

        const canvasContext = options.canvas.getContext('webgpu');
        if (!canvasContext) {
            throw new Error('WebGPU: failed to get canvas WebGPU context');
        }

        const preferredFormat = navigator.gpu.getPreferredCanvasFormat();
        const ctx = new Context(options.canvas, adapter, device, canvasContext, preferredFormat);

        log.info(`webgpu: ready (adapter "${adapter.info?.description ?? '<unknown>'}", `
               + `format=${preferredFormat}, features=${[...features].join(',')})`);

        return ctx;
    }

    /** Match canvas drawing-buffer size to its CSS size and current devicePixelRatio. */
    resizeToDisplay(): boolean {
        const dpr = window.devicePixelRatio || 1;
        const cssWidth  = this.canvas.clientWidth;
        const cssHeight = this.canvas.clientHeight;
        const desiredW  = Math.max(1, Math.floor(cssWidth  * dpr));
        const desiredH  = Math.max(1, Math.floor(cssHeight * dpr));
        if (this.canvas.width !== desiredW || this.canvas.height !== desiredH) {
            this.canvas.width  = desiredW;
            this.canvas.height = desiredH;
            return true;
        }
        return false;
    }

    /** Aspect ratio (width / height) of the current canvas drawing buffer. */
    get aspect(): number {
        return this.canvas.height === 0 ? 1 : this.canvas.width / this.canvas.height;
    }

    /**
     * Run a one-shot command (analogous to gpusims::vk::Context::runOneShot).
     * Submits and awaits a queue.onSubmittedWorkDone() so the call returns
     * once the GPU has finished.
     */
    async runOneShot(fn: (encoder: GPUCommandEncoder) => void): Promise<void> {
        const encoder = this.device.createCommandEncoder({ label: 'one-shot' });
        fn(encoder);
        const buf = encoder.finish();
        this.device.queue.submit([buf]);
        await this.device.queue.onSubmittedWorkDone();
    }
}
