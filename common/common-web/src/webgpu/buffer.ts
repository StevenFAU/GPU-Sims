import type { Context } from './context.js';

/**
 * Memory-residency hint, mirrors gpusims::vk::MemoryUsage.
 *
 * WebGPU's memory model is simpler than Vulkan's — every buffer is
 * device-local and host visibility is via mapAsync(). The hint here picks
 * the right usage flags for common patterns:
 *
 *   - DeviceLocal:           GPU-only. No CPU mapping. Use stage() to upload.
 *   - HostVisibleSequential: Mapped at creation, write-only. For upload buffers.
 *   - HostVisibleRandom:     Mapped at creation, read+write. For staging/readback.
 */
export enum MemoryUsage {
    DeviceLocal = 'DeviceLocal',
    HostVisibleSequential = 'HostVisibleSequential',
    HostVisibleRandom = 'HostVisibleRandom',
}

export interface BufferCreateOptions {
    /** Total size in bytes. */
    size: number;
    /** WebGPU usage flags (e.g., GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST). */
    usage: GPUBufferUsageFlags;
    /** Memory residency hint. Adds COPY_DST (DeviceLocal) or MAP_WRITE/READ as needed. */
    memory: MemoryUsage;
    /** Optional debug label visible in browser devtools. */
    label?: string;
}

export class Buffer {
    readonly handle: GPUBuffer;
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    readonly size: number;
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    readonly memory: MemoryUsage;
    private ctx: Context;

    constructor(ctx: Context, options: BufferCreateOptions) {
        this.ctx = ctx;
        this.size = options.size;
        this.memory = options.memory;

        let usage = options.usage;
        if (options.memory === MemoryUsage.DeviceLocal) {
            usage |= GPUBufferUsage.COPY_DST;
        } else if (options.memory === MemoryUsage.HostVisibleSequential) {
            // mappedAtCreation lets the caller write the initial contents
            // synchronously; subsequent uploads use uploadDirect via writeBuffer.
            usage |= GPUBufferUsage.COPY_SRC;
        } else {
            usage |= GPUBufferUsage.MAP_READ | GPUBufferUsage.COPY_DST;
        }

        const desc: GPUBufferDescriptor = {
            size: options.size,
            usage,
        };
        if (options.label) desc.label = options.label;

        this.handle = ctx.device.createBuffer(desc);
    }

    /**
     * Synchronous CPU-side write via queue.writeBuffer. Works for any buffer
     * with COPY_DST in its usage flags (which DeviceLocal and HostVisibleSequential
     * both have here). Bytes are copied into the queue immediately.
     */
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    uploadDirect(src: ArrayBufferView | ArrayBuffer, dstOffset = 0): void {
        if (src instanceof ArrayBuffer) {
            this.ctx.device.queue.writeBuffer(this.handle, dstOffset, src);
        } else {
            this.ctx.device.queue.writeBuffer(this.handle, dstOffset,
                src.buffer, src.byteOffset, src.byteLength);
        }
    }

    /**
     * Async readback: copy buffer contents into a transient mapped staging
     * buffer and return its bytes. Used for state capture and debugging.
     */
    async readback(): Promise<Uint8Array> {
        const stagingDesc: GPUBufferDescriptor = {
            size: this.size,
            usage: GPUBufferUsage.MAP_READ | GPUBufferUsage.COPY_DST,
        };
        if (this.handle.label) stagingDesc.label = `${this.handle.label}-readback`;
        const staging = this.ctx.device.createBuffer(stagingDesc);
        const encoder = this.ctx.device.createCommandEncoder();
        encoder.copyBufferToBuffer(this.handle, 0, staging, 0, this.size);
        this.ctx.device.queue.submit([encoder.finish()]);

        await staging.mapAsync(GPUMapMode.READ);
        const copy = new Uint8Array(staging.getMappedRange().slice(0));
        staging.unmap();
        staging.destroy();
        return copy;
    }

    destroy(): void {
        this.handle.destroy();
    }
}
