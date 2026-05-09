import { defineConfig } from 'vite';
import { wgslPlugin } from '@gpusims/common-web/vite-plugin';

export default defineConfig({
    base: '/GPU-Sims/mandelbulb-explorer/',
    plugins: [wgslPlugin()],
    server: {
        port: 5175,
        host: '127.0.0.1',
    },
    build: {
        target: 'esnext',
        sourcemap: true,
    },
});
