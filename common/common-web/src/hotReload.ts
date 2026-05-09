import { log } from './log.js';

export type ReloadCallback = (path: string, newSource: string) => void | Promise<void>;

export interface ReloadEvent {
    path: string;
    ok: boolean;
    message: string;
    /** ms since page load when the event happened. */
    t: number;
}

interface WgslUpdatePayload {
    path: string;
    source: string;
}

export class HotReloader {
    private callbacks = new Map<string, ReloadCallback>();
    private events: ReloadEvent[] = [];

    constructor() {
        if (import.meta.hot) {
            import.meta.hot.on('gpusims:wgsl-update', (data: WgslUpdatePayload) => {
                void this.dispatch(data);
            });
        } else {
            log.warn('HotReloader: Vite HMR client is not available; hot-reload disabled');
        }
    }

    /**
     * Register a callback for a shader path. Path format must match what the
     * Vite plugin emits — repo-root-relative with forward slashes.
     *
     * Tip: pass the path the same way you import the shader source. If you
     * `import src from './shaders/gradient.compute.wgsl?raw'`, register
     * `'common/common-web/examples/hello/shaders/gradient.compute.wgsl'`
     * (the path Vite resolves for HMR, not the import string).
     */
    watch(path: string, callback: ReloadCallback): void {
        const normalized = normalizePath(path);
        this.callbacks.set(normalized, callback);
        log.debug(`HotReloader: watching ${normalized}`);
    }

    unwatch(path: string): void {
        this.callbacks.delete(normalizePath(path));
    }

    /** Recently-fired events (up to ~3 seconds old). For UI overlays. */
    recentEvents(now = performance.now()): ReloadEvent[] {
        return this.events.filter((e) => now - e.t < 3000);
    }

    reportSuccess(path: string): void {
        this.events.push({ path, ok: true, message: '', t: performance.now() });
        this.gc();
    }

    reportFailure(path: string, message: string): void {
        this.events.push({ path, ok: false, message, t: performance.now() });
        this.gc();
    }

    private async dispatch(data: WgslUpdatePayload): Promise<void> {
        const cb = this.callbacks.get(normalizePath(data.path));
        if (!cb) {
            log.debug(`HotReloader: no watcher for ${data.path} — ignored`);
            return;
        }
        try {
            await cb(data.path, data.source);
        } catch (err) {
            const msg = err instanceof Error ? err.message : String(err);
            this.reportFailure(data.path, msg);
            log.error(`HotReloader: callback for ${data.path} threw: ${msg}`);
        }
    }

    private gc(): void {
        // Cap stored events so we don't leak.
        if (this.events.length > 32) this.events.splice(0, this.events.length - 32);
    }
}

function normalizePath(p: string): string {
    return p.replace(/\\/g, '/');
}
