import { unzipSync, strFromU8 } from 'fflate';

import { log } from './log.js';
import type { JsonObject, JsonValue } from './types.js';

/**
 * Read a capture written by StateWriter. Created from a user-selected File
 * (via <input type="file">):
 *
 *     <input type="file" accept=".zip" onchange={async (e) => {
 *         const file = e.target.files[0];
 *         const cap = await StateReader.fromFile(file);
 *         camera.fromJson(cap.meta('camera'));
 *     }}/>
 */
export class StateReader {
    private state: { frame: number; meta: JsonObject; buffers: JsonObject[] };
    private buffers: Map<string, Uint8Array>;
    private dirName: string;

    private constructor(state: typeof StateReader.prototype.state,
                        buffers: Map<string, Uint8Array>,
                        dirName: string) {
        this.state = state;
        this.buffers = buffers;
        this.dirName = dirName;
    }

    static async fromFile(file: File): Promise<StateReader | null> {
        try {
            const ab = await file.arrayBuffer();
            const entries = unzipSync(new Uint8Array(ab));

            // Find the capture_NNNN/ prefix.
            const stateKey = Object.keys(entries).find((k) => k.endsWith('/state.json'));
            if (!stateKey) {
                log.error('StateReader: no state.json in archive');
                return null;
            }
            const dirName = stateKey.slice(0, -'/state.json'.length);

            const stateBytes = entries[stateKey];
            if (!stateBytes) {
                log.error('StateReader: no state.json bytes in archive');
                return null;
            }
            const stateText = strFromU8(stateBytes);
            const state = JSON.parse(stateText) as {
                frame: number; meta: JsonObject; buffers: JsonObject[];
            };

            const buffers = new Map<string, Uint8Array>();
            for (const desc of state.buffers) {
                const name = String(desc['name'] ?? '');
                const fileName = String(desc['file'] ?? `${name}.bin`);
                const blobKey = `${dirName}/${fileName}`;
                if (entries[blobKey]) buffers.set(name, entries[blobKey]);
            }

            return new StateReader(state, buffers, dirName);
        } catch (err) {
            log.error(`StateReader: failed to load ${file.name}: ${err instanceof Error ? err.message : err}`);
            return null;
        }
    }

    /** Top-level metadata. */
    meta(key: string): JsonValue | null {
        const v = this.state.meta[key];
        return v === undefined ? null : v;
    }

    /** Per-buffer descriptor. */
    bufferMeta(name: string): JsonObject | null {
        for (const b of this.state.buffers) {
            if (b['name'] === name) return b;
        }
        return null;
    }

    /** Raw bytes for a saved buffer. */
    buffer(name: string): Uint8Array | null {
        return this.buffers.get(name) ?? null;
    }

    get frameIndex(): number { return this.state.frame; }
    get directoryName(): string { return this.dirName; }
}
