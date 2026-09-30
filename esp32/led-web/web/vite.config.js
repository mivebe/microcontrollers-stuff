import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  plugins: [react()],
  build: {
    // Output straight into the PlatformIO filesystem folder
    outDir: '../data',
    emptyOutDir: true,
    // Keep file names short and stable (LittleFS has a filename length limit)
    rollupOptions: {
      output: {
        entryFileNames: 'app.js',
        chunkFileNames: '[name].js',
        assetFileNames: '[name][extname]',
      },
    },
    modulePreload: { polyfill: false },
  },
})
