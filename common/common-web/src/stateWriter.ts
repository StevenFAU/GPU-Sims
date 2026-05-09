import { zipSync, strToU8 } from 'fflate';

import { log } from './log.js';
import type { JsonObject, JsonValue } from './types.js';

interface BufferEntry {
    name: string;
    file: string;
    bytes: number;
    meta: JsonObject;
    data: Uint8Array;
}

/**
 * Pack simulation state into a downloadable ZIP. Schema mirrors common-cpp's
 * StateWriter exactly:
 *
 *     capture_<NNNN>/
 *         state.json    {frame, meta: {...}, buffers: [{name, file, bytes, ...}]}
 *         <name>.bin    raw binary blobs
 *
 * Triggered F5-press flow:
 *
 *     const w = new StateWriter('captures');
 *     w.beginFrame(frameIdx);
 *     w.setMeta('camera', camera.toJson());
 *     w.saveBuffer('particles', particlesGpuBuffer, {count: N, stride: 16});
 *     await w.endFrame();   // triggers download as capture_NNNN.zip
 */
export class StateWriter {
    private root: string;
    private state: { frame: number; meta: JsonObject; buffers: JsonObject[] } | null = null;
    private buffers: BufferEntry[] = [];
    private currentFrameIdx = 0;

    constructor(root = 'captures') {
        this.root = root;
    }

    beginFrame(frameIdx: number): void {
        if (this.state) {
            log.warn('StateWriter: beginFrame called while already in a frame; flushing first');
            void this.endFrame();
        }
        this.currentFrameIdx = frameIdx;
        this.state = {
            frame: frameIdx,
            meta: {},
            buffers: [],
        };
        this.buffers = [];
    }

    setMeta(key: string, value: JsonValue): void {
        if (!this.state) {
            log.warn(`StateWriter: setMeta('${key}') called outside a frame`);
            return;
        }
        this.state.meta[key] = value;
    }

    /**
     * Record a binary blob. `data` is consumed (a copy is taken); pass any
     * Uint8Array/ArrayBuffer derived from a mapped GPU buffer or CPU array.
     */
    saveBuffer(name: string, data: ArrayBufferView | ArrayBuffer, meta: JsonObject = {}): void {
        if (!this.state) {
            log.warn(`StateWriter: saveBuffer('${name}') called outside a frame`);
            return;
        }
        const u8 = data instanceof ArrayBuffer
            ? new Uint8Array(data.slice(0))
            : new Uint8Array(data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength));

        const fileName = `${name}.bin`;
        const desc: JsonObject = { ...meta, name, file: fileName, bytes: u8.byteLength };
        this.state.buffers.push(desc);
        this.buffers.push({
            name, file: fileName, bytes: u8.byteLength, meta: { ...meta }, data: u8,
        });
    }

    /**
     * Pack everything into a ZIP and trigger a browser download.
     * Returns the byte length of the generated ZIP.
     */
    async endFrame(): Promise<number> {
        if (!this.state) return 0;

        const dirName = `capture_${pad4(this.currentFrameIdx)}`;
        const stateJson = JSON.stringify(this.state, null, 2);

        const zipEntries: Record<string, Uint8Array> = {};
        zipEntries[`${dirName}/state.json`] = strToU8(stateJson);
        for (const b of this.buffers) {
            zipEntries[`${dirName}/${b.file}`] = b.data;
        }

        const zipped = zipSync(zipEntries);
        const blob = new Blob([zipped as BlobPart], { type: 'application/zip' });
        triggerDownload(blob, `${dirName}.zip`);

        log.info(`StateWriter: wrote ${dirName}.zip (${zipped.byteLength} bytes, ${this.buffers.length} buffer(s))`);

        this.state = null;
        this.buffers = [];
        return zipped.byteLength;
    }

    get rootName(): string { return this.root; }
}

function pad4(n: number): string { return String(n).padStart(4, '0'); }

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
