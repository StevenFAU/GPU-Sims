// vite config for the common-web package itself. The library doesn't ship via
// Vite — it's consumed as TypeScript source by per-sim Vite projects. This
// config exists so `npm run dev` in this package launches the hello example.

import { defineConfig } from 'vite';

export default defineConfig({
    root: './examples/hello',
    server: {
        port: 5173,
        host: '127.0.0.1',
    },
});
