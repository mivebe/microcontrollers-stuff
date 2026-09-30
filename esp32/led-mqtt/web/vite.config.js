import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  plugins: [react()],
  // Relative paths so the build works from any folder (GitHub Pages, etc.)
  base: './',
})
