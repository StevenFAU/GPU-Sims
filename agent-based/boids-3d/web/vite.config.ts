import { defineConfig } from 'vite';
import { wgslPlugin } from '@gpusims/common-web/vite-plugin';

export default defineConfig({
    plugins: [wgslPlugin()],
    base: './',
    server: {
        port: 5178,
        host: '127.0.0.1',
    },
    build: {
        outDir: 'dist',
        emptyOutDir: true,
        target: 'es2022',
    },
});
