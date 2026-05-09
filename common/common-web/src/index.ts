// Public API surface of @gpusims/common-web.
// Per-sim consumers import from this file:
//     import { Context, Renderer, Camera, ParamPanel } from '@gpusims/common-web';

// Generic
export { initLogger, log } from './log.js';
export { Camera } from './camera.js';
export type { CameraInputState, CameraMode } from './camera.js';
export { HotReloader } from './hotReload.js';
export type { ReloadCallback, ReloadEvent } from './hotReload.js';
export { GpuProfiler } from './gpuProfiler.js';
export type { PassResult } from './gpuProfiler.js';
export { StateWriter } from './stateWriter.js';
export { StateReader } from './stateReader.js';
export { ParamPanel } from './paramPanel.js';
export type { ParamFolder } from './paramPanel.js';
export { snapshotInput } from './input.js';
export type { InputSnapshot } from './input.js';

// WebGPU-specific
export { Context } from './webgpu/context.js';
export type { ContextOptions } from './webgpu/context.js';
export { Buffer, MemoryUsage } from './webgpu/buffer.js';
export { Texture, TextureType } from './webgpu/texture.js';
export type { TextureCreateInfo } from './webgpu/texture.js';
export { compileWgsl } from './webgpu/shaderModule.js';
export type { CompileResult, ShaderStage } from './webgpu/shaderModule.js';
export { ComputePipeline } from './webgpu/computePipeline.js';
export type { ComputePipelineDesc } from './webgpu/computePipeline.js';
export { RenderPipeline } from './webgpu/renderPipeline.js';
export type { RenderPipelineDesc } from './webgpu/renderPipeline.js';
export { Renderer } from './webgpu/renderer.js';
export type { Frame } from './webgpu/renderer.js';

// Frame-in-flight count. Must match the renderer's count exactly.
export const MAX_FRAMES_IN_FLIGHT = 2;
