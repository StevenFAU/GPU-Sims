import { log } from '../log.js';
import { compileWgsl } from './shaderModule.js';
import type { Context } from './context.js';

export interface ComputePipelineDesc {
    /** WGSL source. Pass via `import src from './foo.wgsl?raw'`. */
    source: string;
    /**
     * Identifier used by the hot-reloader to match HMR updates against the
     * pipeline. Should be the same path the Vite WGSL plugin emits — usually
     * the value of `new URL('./shader.wgsl', import.meta.url).pathname`,
     * normalized.
     */
    shaderPath: string;
    /** Entry point name (default 'main'). */
    entryPoint?: string;
    /** Bind group layout entries. */
    bindings: GPUBindGroupLayoutEntry[];
    /** Workgroup size. Used only for documentation; WGSL declares its own. */
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    workgroupSize?: [number, number, number];
    /** Optional debug label. */
    label?: string;
}

/**
 * Compute pipeline wrapper. Mirrors gpusims::vk::ComputePipeline shape.
 *
 * Hot-reload: call pipeline.reload(newSource) at frame boundary (before
 * `renderer.beginFrame()`). On success the GPUShaderModule and underlying
 * GPUComputePipeline are replaced atomically. On failure, the previous
 * pipeline stays bound and the error message is returned.
 */
export class ComputePipeline {
    private ctx: Context;
    private desc: ComputePipelineDesc;
    private bindGroupLayout: GPUBindGroupLayout;
    private pipelineLayout: GPUPipelineLayout;
    private pipeline: GPUComputePipeline;
    /** Internal counter so we can label reloaded modules distinctly. */
    private generation = 0;

    private constructor(ctx: Context, desc: ComputePipelineDesc,
                        bindGroupLayout: GPUBindGroupLayout,
                        pipelineLayout: GPUPipelineLayout,
                        pipeline: GPUComputePipeline) {
        this.ctx = ctx;
        this.desc = desc;
        this.bindGroupLayout = bindGroupLayout;
        this.pipelineLayout = pipelineLayout;
        this.pipeline = pipeline;
    }

    static async create(ctx: Context, desc: ComputePipelineDesc): Promise<ComputePipeline> {
        const compiled = await compileWgsl(ctx, desc.source, 'compute', desc.label);
        if (!compiled.ok || !compiled.module) {
            throw new Error(`ComputePipeline.create("${desc.label ?? '<unlabeled>'}"): ${compiled.error}`);
        }

        const bglDesc: GPUBindGroupLayoutDescriptor = { entries: desc.bindings };
        if (desc.label) bglDesc.label = `${desc.label}-bgl`;
        const bgl = ctx.device.createBindGroupLayout(bglDesc);
        const layoutDesc: GPUPipelineLayoutDescriptor = { bindGroupLayouts: [bgl] };
        if (desc.label) layoutDesc.label = `${desc.label}-layout`;
        const layout = ctx.device.createPipelineLayout(layoutDesc);
        const pipelineDesc: GPUComputePipelineDescriptor = {
            layout,
            compute: {
                module: compiled.module,
                entryPoint: desc.entryPoint ?? 'main',
            },
        };
        if (desc.label) pipelineDesc.label = desc.label;
        const pipeline = ctx.device.createComputePipeline(pipelineDesc);

        return new ComputePipeline(ctx, desc, bgl, layout, pipeline);
    }

    /**
     * Recompile from new WGSL source. Returns null on success or the compiler
     * error string on failure (in which case the existing pipeline is unchanged).
     */
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    async reload(newSource: string): Promise<string | null> {
        const label = `${this.desc.label ?? 'compute'}#${++this.generation}`;
        const compiled = await compileWgsl(this.ctx, newSource, 'compute', label);
        if (!compiled.ok || !compiled.module) {
            this.generation--;  // don't bump if we're not swapping
            return compiled.error;
        }
        // Reuse existing layout — bindings haven't changed.
        const newPipeline = this.ctx.device.createComputePipeline({
            label,
            layout: this.pipelineLayout,
            compute: {
                module: compiled.module,
                entryPoint: this.desc.entryPoint ?? 'main',
            },
        });
        this.pipeline = newPipeline;
        this.desc = { ...this.desc, source: newSource };
        log.info(`compute pipeline reloaded: ${label}`);
        return null;
    }

    /**
     * Allocate a bind group from the wrapper's auto-derived layout.
     * Caller fills the entries with concrete buffer/texture/sampler bindings.
     */
    createBindGroup(entries: GPUBindGroupEntry[], label?: string): GPUBindGroup {
        const desc: GPUBindGroupDescriptor = { layout: this.bindGroupLayout, entries };
        if (label) desc.label = label;
        return this.ctx.device.createBindGroup(desc);
    }

    /** Set up a compute pass and dispatch. Caller provides bind group + workgroup count. */
    dispatch(encoder: GPUCommandEncoder, bindGroup: GPUBindGroup,
             gx: number, gy: number, gz: number,
             timestampWrites?: GPUComputePassTimestampWrites): void {
        const pass = encoder.beginComputePass(
            timestampWrites ? { timestampWrites } : undefined,
        );
        pass.setPipeline(this.pipeline);
        pass.setBindGroup(0, bindGroup);
        pass.dispatchWorkgroups(gx, gy, gz);
        pass.end();
    }

// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    get handle(): GPUComputePipeline { return this.pipeline; }
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    get layout(): GPUPipelineLayout { return this.pipelineLayout; }
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    get bindLayout(): GPUBindGroupLayout { return this.bindGroupLayout; }
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    get path(): string { return this.desc.shaderPath; }
}
