import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'
import path from 'path'

export default defineConfig({
  base: '', // مهم برای electron
  build: {
    outDir: 'dist' // مطمئن می‌شیم خروجی همونی باشه که Electron لود می‌کنه
  },
  plugins: [
    react(),
    tailwindcss(),
  ],
  server: {
    watch: {
      ignored: ['**/hls_output/**', '**/node_modules/**']
    }
  },
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src')
    }
  }
})
