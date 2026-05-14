import { log } from '../log.js';
import { compileWgsl } from './shaderModule.js';
import type { Context } from './context.js';

export interface RenderPipelineDesc {
    vertexSource: string;
    fragmentSource: string;
    /** Path to the vertex shader file (for hot-reload tracking). */
    vertexPath: string;
    /** Path to the fragment shader file (for hot-reload tracking). */
    fragmentPath: string;
    vertexEntryPoint?: string;
    fragmentEntryPoint?: string;

    bindings: GPUBindGroupLayoutEntry[];

    /** Color attachment target formats. Typically [ctx.preferredFormat]. */
    colorFormats: GPUTextureFormat[];
    /** Depth attachment format, omitted if no depth. */
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    depthFormat?: GPUTextureFormat;

    primitive?: GPUPrimitiveState;
    depthStencil?: GPUDepthStencilState;
    /** Empty by default — fullscreen-triangle pattern uses gl_VertexIndex equivalent. */
    vertexBuffers?: GPUVertexBufferLayout[];
    blend?: GPUBlendState;

    label?: string;
}

/**
 * Render pipeline wrapper using WebGPU. Hot-reload covers both vertex and
 * fragment shaders separately — pass which path changed and the new source.
 */
export class RenderPipeline {
    private ctx: Context;
    private desc: RenderPipelineDesc;
    private bindGroupLayout: GPUBindGroupLayout;
    private pipelineLayout: GPUPipelineLayout;
    private pipeline: GPURenderPipeline;
    private generation = 0;

    private constructor(ctx: Context, desc: RenderPipelineDesc,
                        bindGroupLayout: GPUBindGroupLayout,
                        pipelineLayout: GPUPipelineLayout,
                        pipeline: GPURenderPipeline) {
        this.ctx = ctx;
        this.desc = desc;
        this.bindGroupLayout = bindGroupLayout;
        this.pipelineLayout = pipelineLayout;
        this.pipeline = pipeline;
    }

    static async create(ctx: Context, desc: RenderPipelineDesc): Promise<RenderPipeline> {
        const vert = await compileWgsl(ctx, desc.vertexSource, 'vertex',
            desc.label ? `${desc.label}-vert` : undefined);
        const frag = await compileWgsl(ctx, desc.fragmentSource, 'fragment',
            desc.label ? `${desc.label}-frag` : undefined);
        if (!vert.ok || !vert.module) {
            throw new Error(`RenderPipeline vert: ${vert.error}`);
        }
        if (!frag.ok || !frag.module) {
            throw new Error(`RenderPipeline frag: ${frag.error}`);
        }

        const bglDesc: GPUBindGroupLayoutDescriptor = { entries: desc.bindings };
        if (desc.label) bglDesc.label = `${desc.label}-bgl`;
        const bgl = ctx.device.createBindGroupLayout(bglDesc);
        const layoutDesc: GPUPipelineLayoutDescriptor = { bindGroupLayouts: [bgl] };
        if (desc.label) layoutDesc.label = `${desc.label}-layout`;
        const layout = ctx.device.createPipelineLayout(layoutDesc);

        const pipeline = buildPipeline(ctx, desc, layout, vert.module, frag.module);

        return new RenderPipeline(ctx, desc, bgl, layout, pipeline);
    }

// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    async reload(which: 'vertex' | 'fragment', newSource: string): Promise<string | null> {
        const label = `${this.desc.label ?? 'render'}#${++this.generation}-${which}`;
        const compiled = await compileWgsl(this.ctx, newSource, which, label);
        if (!compiled.ok || !compiled.module) {
            this.generation--;
            return compiled.error;
        }

        // Recompile the *other* stage from its current source so we can
        // recreate the pipeline. This is fine — WGSL compile is fast.
        const otherSrc = which === 'vertex' ? this.desc.fragmentSource : this.desc.vertexSource;
        const otherStage = which === 'vertex' ? 'fragment' : 'vertex';
        const other = await compileWgsl(this.ctx, otherSrc, otherStage, `${label}-other`);
        if (!other.ok || !other.module) {
            this.generation--;
            return other.error;
        }

        const newPipeline = buildPipeline(
            this.ctx, this.desc, this.pipelineLayout,
            which === 'vertex' ? compiled.module : other.module,
            which === 'fragment' ? compiled.module : other.module,
        );

        this.pipeline = newPipeline;
        if (which === 'vertex')   this.desc = { ...this.desc, vertexSource: newSource };
        if (which === 'fragment') this.desc = { ...this.desc, fragmentSource: newSource };
        log.info(`render pipeline reloaded: ${label}`);
        return null;
    }

    createBindGroup(entries: GPUBindGroupEntry[], label?: string): GPUBindGroup {
        const bgDesc: GPUBindGroupDescriptor = { layout: this.bindGroupLayout, entries };
        if (label) bgDesc.label = label;
        return this.ctx.device.createBindGroup(bgDesc);
    }

    /** Bind onto a render pass. Caller follows with pass.draw(...) etc. */
    bind(pass: GPURenderPassEncoder, bindGroup: GPUBindGroup): void {
        pass.setPipeline(this.pipeline);
        pass.setBindGroup(0, bindGroup);
    }

    get handle(): GPURenderPipeline { return this.pipeline; }
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    get layout(): GPUPipelineLayout { return this.pipelineLayout; }
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    get bindLayout(): GPUBindGroupLayout { return this.bindGroupLayout; }
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    get vertexPath(): string { return this.desc.vertexPath; }
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    get fragmentPath(): string { return this.desc.fragmentPath; }
}

function buildPipeline(
    ctx: Context,
    desc: RenderPipelineDesc,
    layout: GPUPipelineLayout,
    vert: GPUShaderModule,
    frag: GPUShaderModule,
): GPURenderPipeline {
    const targets: GPUColorTargetState[] = desc.colorFormats.map((format) => {
        const t: GPUColorTargetState = { format };
        if (desc.blend) t.blend = desc.blend;
        return t;
    });

    const pipelineDesc: GPURenderPipelineDescriptor = {
        layout,
        vertex: {
            module: vert,
            entryPoint: desc.vertexEntryPoint ?? 'vs_main',
            buffers: desc.vertexBuffers ?? [],
        },
        fragment: {
            module: frag,
            entryPoint: desc.fragmentEntryPoint ?? 'fs_main',
            targets,
        },
        primitive: desc.primitive ?? { topology: 'triangle-list' },
    };
    if (desc.label) pipelineDesc.label = desc.label;
    if (desc.depthStencil) pipelineDesc.depthStencil = desc.depthStencil;

    return ctx.device.createRenderPipeline(pipelineDesc);
}
