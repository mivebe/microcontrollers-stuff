import path from 'node:path'
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'

export default defineConfig({
  plugins: [react(), tailwindcss()],
  resolve: {
    alias: { '@': path.resolve(import.meta.dirname, './src') },
  },
  // Relative paths so the build works from any folder (GitHub Pages, etc.)
  base: './',
  // The MQTT client library is most of the bundle; fine for a hosted page
  build: { chunkSizeWarningLimit: 1000 },
})
