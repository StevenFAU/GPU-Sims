import type { Context } from './context.js';

export enum TextureType {
    e2D = '2d',
    e3D = '3d',
}

export interface TextureCreateInfo {
    type: TextureType;
    /** Width, height, depth (depth=1 for 2D). */
    extent: { width: number; height: number; depthOrArrayLayers?: number };
    format: GPUTextureFormat;
    usage: GPUTextureUsageFlags;
    mipLevelCount?: number;
    sampleCount?: number;
    label?: string;
}

export class Texture {
    readonly handle: GPUTexture;
    readonly view: GPUTextureView;
    readonly format: GPUTextureFormat;
    readonly type: TextureType;
    readonly width: number;
    readonly height: number;
    readonly depth: number;
    private ctx: Context;

    constructor(ctx: Context, info: TextureCreateInfo) {
        this.ctx = ctx;
        this.type = info.type;
        this.format = info.format;
        this.width  = info.extent.width;
        this.height = info.extent.height;
        this.depth  = info.extent.depthOrArrayLayers ?? 1;

        const desc: GPUTextureDescriptor = {
            size: {
                width: info.extent.width,
                height: info.extent.height,
                depthOrArrayLayers: info.extent.depthOrArrayLayers ?? 1,
            },
            format: info.format,
            usage: info.usage,
            dimension: info.type === TextureType.e3D ? '3d' : '2d',
        };
        if (info.mipLevelCount !== undefined) desc.mipLevelCount = info.mipLevelCount;
        if (info.sampleCount !== undefined) desc.sampleCount = info.sampleCount;
        if (info.label) desc.label = info.label;

        this.handle = ctx.device.createTexture(desc);
        const viewDesc: GPUTextureViewDescriptor = {
            dimension: info.type === TextureType.e3D ? '3d' : '2d',
        };
        if (info.label) viewDesc.label = `${info.label}-view`;
        this.view = this.handle.createView(viewDesc);
    }

    /**
     * Upload pixel data into a 2D texture at mip level 0. Pitch (bytesPerRow)
     * is computed from the format; pass `bytesPerPixel` if WebGPU's default
     * doesn't match (for non-standard formats). For most use cases the
     * default rgba8unorm / r32float / etc. work fine.
     */
    uploadDirect2D(src: ArrayBufferView, bytesPerPixel: number): void {
        if (this.type !== TextureType.e2D) {
            throw new Error('Texture: uploadDirect2D requires a 2D texture');
        }
        this.ctx.device.queue.writeTexture(
            { texture: this.handle, mipLevel: 0 },
            src.buffer,
            {
                offset: src.byteOffset,
                bytesPerRow: this.width * bytesPerPixel,
                rowsPerImage: this.height,
            },
            { width: this.width, height: this.height, depthOrArrayLayers: 1 },
        );
    }

    destroy(): void {
        this.handle.destroy();
    }
}
