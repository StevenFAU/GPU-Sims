// Vite plugin for WGSL: resolves `import './foo.wgsl?raw'` to the file's text,
// and emits a custom HMR event on .wgsl save so the page's HotReloader can
// re-create affected pipelines without a full reload.
//
// Per-sim usage (vite.config.ts):
//
//     import { defineConfig } from 'vite';
//     import { wgslPlugin } from '@gpusims/common-web/vite-plugin';
//
//     export default defineConfig({
//         plugins: [wgslPlugin()],
//     });

import type { Plugin, ViteDevServer } from 'vite';
import { promises as fs } from 'node:fs';
import { extname, relative, resolve } from 'node:path';

export interface WgslPluginOptions {
    /** Glob the plugin will watch. Default: '**\/*.wgsl' (all .wgsl files). */
    include?: string[];
}

/**
 * Vite plugin that handles `.wgsl` imports and HMR.
 *
 * The HMR event payload is `{ path: string, source: string }` where `path`
 * is repo-root-relative.
 */
export function wgslPlugin(_options: WgslPluginOptions = {}): Plugin {
    let server: ViteDevServer | undefined;
    let root: string = '.';

    return {
        name: 'gpusims:wgsl',
        configResolved(cfg) {
            root = cfg.root;
        },

        configureServer(s) {
            server = s;
        },

        // Allow `import wgslText from './foo.wgsl?raw'` to work in dev.
        // Vite handles ?raw natively for any text file — no transform needed.
        // We only intercept HMR events for files we recognize.

        async handleHotUpdate(ctx) {
            if (extname(ctx.file) !== '.wgsl') return undefined;

            try {
                const source = await fs.readFile(ctx.file, 'utf8');
                const rel = relative(root, ctx.file).split('\\').join('/');
                if (server) {
                    server.ws.send({
                        type: 'custom',
                        event: 'gpusims:wgsl-update',
                        data: { path: rel, source },
                    });
                }
            } catch (err) {
                // eslint-disable-next-line no-console
                console.error('[gpusims:wgsl] failed to read', ctx.file, err);
            }

            // Returning [] tells Vite not to do its default full-page reload —
            // we handle the update via the custom HMR event above.
            return [];
        },

        // Mark .wgsl files as assets that should be watched.
        load(id) {
            if (!id.endsWith('.wgsl') && !id.endsWith('.wgsl?raw')) return undefined;
            // Strip the ?raw query if present; let Vite's built-in raw loader handle it.
            return undefined;
        },

        resolveId(id, importer) {
            if (!id.endsWith('.wgsl') && !id.endsWith('.wgsl?raw')) return undefined;
            if (!importer) return undefined;
            // Resolve relative to importer; fall through to Vite's default resolver.
            const stripped = id.endsWith('?raw') ? id.slice(0, -'?raw'.length) : id;
            const resolved = resolve(importer, '..', stripped);
            // Re-attach the ?raw if it was there so Vite's raw loader fires.
            return id.endsWith('?raw') ? `${resolved}?raw` : resolved;
        },
    };
}

// CommonJS-friendly default export for tools that prefer it.
export default wgslPlugin;
