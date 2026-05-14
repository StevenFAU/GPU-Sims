import { log } from './log.js';
import { MAX_FRAMES_IN_FLIGHT } from './index.js';

const MAX_PASSES = 64;
const QUERIES_PER_FRAME = 2 * MAX_PASSES;

export interface PassResult {
    name: string;
    cpuMs: number;
    gpuMs: number;  // 0 when timestamp-query unavailable
}

type ReadbackState = 'idle' | 'pending' | 'ready';

interface FrameData {
    passNames: string[];
    cpuBegin: number[];
    cpuEnd: number[];
    passCount: number;
    submitted: boolean;
    /** Buffer that receives query results via copyQueryResultsToBuffer. */
    resultBuffer: GPUBuffer | null;
    /** Mapped readback buffer. */
    readbackBuffer: GPUBuffer | null;
    /** Tracks whether readbackBuffer has a pending mapAsync in flight. */
    readbackState: ReadbackState;
}

export class GpuProfiler {
    private hasTimestamp: boolean;
    private querySets: Array<GPUQuerySet | null> = [];
    private frames: FrameData[] = [];
    private currentIdx = 0;
    private frameCounter = 0;
    private latestResults: PassResult[] = [];
    private csvHeaderWritten = false;
    private csvLines: string[] = [];

    constructor(device: GPUDevice) {
        this.hasTimestamp = device.features.has('timestamp-query');

        for (let i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            const frame: FrameData = {
                passNames: new Array<string>(MAX_PASSES).fill(''),
                cpuBegin: new Array<number>(MAX_PASSES).fill(0),
                cpuEnd: new Array<number>(MAX_PASSES).fill(0),
                passCount: 0,
                submitted: false,
                resultBuffer: null,
                readbackBuffer: null,
                readbackState: 'idle',
            };

            if (this.hasTimestamp) {
                const qs = device.createQuerySet({
                    type: 'timestamp',
                    count: QUERIES_PER_FRAME,
                });
                this.querySets.push(qs);
                frame.resultBuffer = device.createBuffer({
                    size: QUERIES_PER_FRAME * 8,
                    usage: GPUBufferUsage.QUERY_RESOLVE | GPUBufferUsage.COPY_SRC,
                });
                frame.readbackBuffer = device.createBuffer({
                    size: QUERIES_PER_FRAME * 8,
                    usage: GPUBufferUsage.MAP_READ | GPUBufferUsage.COPY_DST,
                });
            } else {
                this.querySets.push(null);
            }

            this.frames.push(frame);
        }

        if (!this.hasTimestamp) {
            log.warn('GpuProfiler: timestamp-query feature unavailable; CPU-only timing');
        }
    }

// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    destroy(): void {
        for (const qs of this.querySets) qs?.destroy();
        for (const f of this.frames) {
            f.resultBuffer?.destroy();
            f.readbackBuffer?.destroy();
        }
    }

    /**
     * Begin a frame. Reads back the query results from MAX_FRAMES_IN_FLIGHT
     * frames ago (which the GPU has long since completed), then clears state
     * for the new frame.
     */
    beginFrame(frameInFlightIdx: number): void {
        this.currentIdx = frameInFlightIdx;
        const f = this.frames[this.currentIdx];
        if (!f) {
            log.warn(`GpuProfiler: beginFrame idx ${this.currentIdx} out of bounds`);
            return;
        }
        f.passCount = 0;

        // Only kick off a new readback if no prior readback is still pending.
        // If still pending, skip — we'll get this frame's results on a future cycle.
        if (f.submitted && f.readbackState === 'idle') {
            f.readbackState = 'pending';
            void this.readBack(this.currentIdx);
        }

        f.submitted = true;
        this.frameCounter++;
    }

    endFrame(): void {
        // Reads happen in next frame's beginFrame (delayed-read pattern).
    }

    /**
     * Returns a disposable that issues begin/end timestamps. Use with
     * `using _ = profiler.scope(encoder, 'name')` (TypeScript 5.2+).
     * For older callers, do `const s = profiler.scope(...); ...; s.end();`.
     */
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    scope(passEncoder: GPUComputePassEncoder | GPURenderPassEncoder | null,
          name: string): { end(): void } {
        void passEncoder;
        const slot = this.beginPass(name);
        return {
            end: (): void => { this.endPass(slot); },
        };
    }

    /**
     * Configure descriptors with the timestamp writes. Pass these into
     * encoder.beginComputePass / beginRenderPass instead of using scope().
     * Returns null if timestamp-query is unavailable.
     */
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    timestampWritesFor(name: string): GPURenderPassTimestampWrites | null {
        if (!this.hasTimestamp) return null;
        const slot = this.beginPass(name);
        const qs = this.querySets[this.currentIdx];
        if (!qs) return null;
        return {
            querySet: qs,
            beginningOfPassWriteIndex: 2 * slot,
            endOfPassWriteIndex: 2 * slot + 1,
        };
    }

    /** Resolve queries into the result buffer; queue copy to the readback. */
    resolveQueries(encoder: GPUCommandEncoder): void {
        if (!this.hasTimestamp) return;
        const f = this.frames[this.currentIdx];
        const qs = this.querySets[this.currentIdx];
        if (!f || !qs || !f.resultBuffer || !f.readbackBuffer) return;
        if (f.passCount === 0) return;
        encoder.resolveQuerySet(qs, 0, 2 * f.passCount, f.resultBuffer, 0);
        encoder.copyBufferToBuffer(f.resultBuffer, 0, f.readbackBuffer, 0, 2 * f.passCount * 8);
    }

// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    get latest(): PassResult[] { return this.latestResults; }

// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    appendCsv(): void {
        if (this.latestResults.length === 0) return;
        if (!this.csvHeaderWritten) {
            this.csvLines.push('frame,pass,gpu_ms,cpu_ms');
            this.csvHeaderWritten = true;
        }
        for (const r of this.latestResults) {
            this.csvLines.push(`${this.frameCounter},${r.name},${r.gpuMs},${r.cpuMs}`);
        }
    }

    /** Trigger a download of accumulated CSV rows. */
// integrity-allow: cat2.public-symbol-used-ts; pre-v1 Stack B public symbol with no current consumer (tracked for v1.1 review per grandfather-catalog cat2-stack-b-unused); n/a
    downloadCsv(filename = 'profile.csv'): void {
        if (this.csvLines.length === 0) return;
        const blob = new Blob([this.csvLines.join('\n') + '\n'], { type: 'text/csv' });
        triggerDownload(blob, filename);
    }

    private beginPass(name: string): number {
        const f = this.frames[this.currentIdx];
        if (!f) {
            log.warn(`GpuProfiler: beginPass idx ${this.currentIdx} out of bounds`);
            return -1;
        }
        if (f.passCount >= MAX_PASSES) {
            log.warn(`GpuProfiler: pass count exceeded; '${name}' ignored`);
            return -1;
        }
        const slot = f.passCount++;
        f.passNames[slot] = name;
        f.cpuBegin[slot] = performance.now();
        return slot;
    }

    private endPass(slot: number): void {
        if (slot < 0) return;
        const f = this.frames[this.currentIdx];
        if (!f) {
            log.warn(`GpuProfiler: endPass idx ${this.currentIdx} out of bounds`);
            return;
        }
        f.cpuEnd[slot] = performance.now();
    }

    private async readBack(idx: number): Promise<void> {
        const f = this.frames[idx];
        if (!f) {
            log.warn(`GpuProfiler: readBack idx ${idx} out of bounds`);
            return;
        }
        if (f.readbackState !== 'pending') return;
        if (!this.hasTimestamp || !f.readbackBuffer) {
            this.publishCpuOnly(f);
            f.readbackState = 'idle';
            return;
        }
        try {
            await f.readbackBuffer.mapAsync(GPUMapMode.READ, 0, 2 * f.passCount * 8);
            const view = new BigUint64Array(f.readbackBuffer.getMappedRange().slice(0));
            f.readbackBuffer.unmap();
            this.publishWithGpu(f, view);
            f.readbackState = 'idle';
        } catch (err) {
            log.warn(`GpuProfiler: readback failed (${err instanceof Error ? err.message : err})`);
            this.publishCpuOnly(f);
            f.readbackState = 'idle';
        }
    }

    private publishWithGpu(f: FrameData, view: BigUint64Array): void {
        const out: PassResult[] = [];
        for (let i = 0; i < f.passCount; i++) {
            const t0 = view[2 * i + 0];
            const t1 = view[2 * i + 1];
            const name = f.passNames[i];
            const cpuBegin = f.cpuBegin[i];
            const cpuEnd = f.cpuEnd[i];
            if (t0 === undefined || t1 === undefined || name === undefined
                || cpuBegin === undefined || cpuEnd === undefined) continue;
            const gpuNs = t1 > t0 ? Number(t1 - t0) : 0;  // ns per WebGPU spec
            out.push({
                name,
                cpuMs: cpuEnd - cpuBegin,
                gpuMs: gpuNs / 1e6,
            });
        }
        this.latestResults = out;
    }

    private publishCpuOnly(f: FrameData): void {
        const out: PassResult[] = [];
        for (let i = 0; i < f.passCount; i++) {
            const name = f.passNames[i];
            const cpuBegin = f.cpuBegin[i];
            const cpuEnd = f.cpuEnd[i];
            if (name === undefined || cpuBegin === undefined || cpuEnd === undefined) continue;
            out.push({
                name,
                cpuMs: cpuEnd - cpuBegin,
                gpuMs: 0,
            });
        }
        this.latestResults = out;
    }
}

function triggerDownload(blob: Blob, filename: string): void {
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
}
