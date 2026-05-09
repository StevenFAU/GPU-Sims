import { defineConfig } from 'vite';
import { wgslPlugin } from '@gpusims/common-web/vite-plugin';

export default defineConfig({
    plugins: [wgslPlugin()],
    server: {
        port: 5173,
        host: '127.0.0.1',
    },
    build: {
        target: 'esnext',
        sourcemap: true,
    },
});
