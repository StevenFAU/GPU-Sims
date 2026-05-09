import { log } from '../log.js';
import type { Context } from './context.js';

export type ShaderStage = 'compute' | 'vertex' | 'fragment';

export interface CompileResult {
    ok: boolean;
    module: GPUShaderModule | null;
    error: string;
    /**
     * Files included transitively. Empty in WebGPU because WGSL doesn't have
     * #include — we keep the field for API parity with common-cpp.
     */
    includes: string[];
}

/**
 * Compile WGSL source into a GPUShaderModule. WebGPU compilation is
 * asynchronous-but-not-really: createShaderModule returns synchronously, but
 * the device only reports compilation messages via getCompilationInfo().
 *
 * compileAsync awaits getCompilationInfo() so callers see the full success/
 * error state in one go, matching common-cpp's CompileResult ergonomics.
 */
export async function compileWgsl(
    ctx: Context,
    source: string,
    stage: ShaderStage,
    label?: string,
): Promise<CompileResult> {
    const desc: GPUShaderModuleDescriptor = { code: source };
    if (label) desc.label = label;

    const module = ctx.device.createShaderModule(desc);
    const info = await module.getCompilationInfo();

    let hasError = false;
    const lines: string[] = [];
    for (const msg of info.messages) {
        if (msg.type === 'error') {
            hasError = true;
            lines.push(`error[${msg.lineNum}:${msg.linePos}]: ${msg.message}`);
        } else if (msg.type === 'warning') {
            lines.push(`warning[${msg.lineNum}:${msg.linePos}]: ${msg.message}`);
            log.warn(`wgsl ${label ?? 'anonymous'}: ${msg.message}`);
        }
    }

    if (hasError) {
        return {
            ok: false,
            module: null,
            error: lines.join('\n'),
            includes: [],
        };
    }
    return { ok: true, module, error: '', includes: [] };

    // Stage parameter is currently unused (WebGPU infers stage at pipeline
    // creation time from the entry point's @compute/@vertex/@fragment attribute).
    // It's kept in the signature for API parity with gpusims::vk::ShaderCompiler.
    // Accept the unused-parameter warning — TS sees it referenced via the JSDoc.
    void stage;
}
