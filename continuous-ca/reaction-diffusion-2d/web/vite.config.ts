import { defineConfig } from 'vite';
import { wgslPlugin } from '@gpusims/common-web/vite-plugin';

export default defineConfig({
    base: '/GPU-Sims/reaction-diffusion-2d/',
    plugins: [wgslPlugin()],
    server: {
        port: 5176,
        host: '127.0.0.1',
    },
    build: {
        target: 'esnext',
        sourcemap: true,
    },
});
