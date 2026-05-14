import { MAX_FRAMES_IN_FLIGHT } from '../index.js';
import type { Context } from './context.js';

export interface Frame {
    /** 0..MAX_FRAMES_IN_FLIGHT-1 */
    inFlightIndex: number;
    encoder: GPUCommandEncoder;
    /** Texture view for this frame's swap-chain image. */
    canvasView: GPUTextureView;
}

/**
 * Frame lifecycle:
 *
 *     const frame = renderer.beginFrame();
 *
 *     // Compute work (no encoder.finish() yet)
 *     compute_pipe.dispatch(frame.encoder, ...);
 *
 *     // Render pass
 *     const pass = renderer.beginRendering(frame, [r,g,b,a]);
 *     graphics_pipe.bind(pass, ...);
 *     pass.draw(3, 1, 0, 0);
 *     renderer.endRendering(pass);
 *
 *     renderer.endFrame(frame);   // submits encoder.finish() to queue
 *
 * The renderer does NOT call requestAnimationFrame — sims own their main loop
 * so they can choose between rAF (for visual sims), fixed-step (for sim
 * predictability), or run-as-fast-as-possible (for benchmarks).
 */
export class Renderer {
    private ctx: Context;
    private currentFrame = 0;

    constructor(ctx: Context) {
        this.ctx = ctx;
    }

    beginFrame(): Frame {
        // Sync canvas size to display CSS so the swapchain matches the visible
        // area. No-op when the canvas hasn't resized.
        this.ctx.resizeToDisplay();

        const view = this.ctx.canvasContext.getCurrentTexture().createView();
        const encoder = this.ctx.device.createCommandEncoder({
            label: `frame-${this.currentFrame}`,
        });
        const frame: Frame = {
            inFlightIndex: this.currentFrame,
            encoder,
            canvasView: view,
        };
        return frame;
    }

    /**
     * Begin a render pass that writes into the swap-chain image.
     * Returns the GPURenderPassEncoder so callers can issue draws against it.
     *
     * Use `clear` to set the background color (RGBA, 0..1). Pass null to load
     * existing contents instead of clearing.
     */
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    beginRendering(frame: Frame,
                   clear: [number, number, number, number] | null = [0.05, 0.05, 0.07, 1.0],
                   timestampWrites?: GPURenderPassTimestampWrites): GPURenderPassEncoder {
        const colorAttachment: GPURenderPassColorAttachment = {
            view: frame.canvasView,
            storeOp: 'store',
            ...(clear === null
                ? { loadOp: 'load' as const }
                : { loadOp: 'clear' as const, clearValue: { r: clear[0], g: clear[1], b: clear[2], a: clear[3] } }),
        };

        const desc: GPURenderPassDescriptor = {
            colorAttachments: [colorAttachment],
        };
        if (timestampWrites) desc.timestampWrites = timestampWrites;

        return frame.encoder.beginRenderPass(desc);
    }

    endRendering(pass: GPURenderPassEncoder): void {
        pass.end();
    }

    endFrame(frame: Frame): void {
        const buf = frame.encoder.finish();
        this.ctx.device.queue.submit([buf]);
        this.currentFrame = (this.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    /** Wait until all submitted GPU work has completed. Use sparingly (shutdown). */
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    async waitIdle(): Promise<void> {
        await this.ctx.device.queue.onSubmittedWorkDone();
    }
}
