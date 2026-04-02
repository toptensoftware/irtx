import { defineConfig } from 'vite';
import { viteStaticCopy } from 'vite-plugin-static-copy';

export default defineConfig({
  base: "/",
  publicDir: false,
  build: {
    emptyOutDir: true,
    outDir: '../out/dist',
  },
  plugins: [
    viteStaticCopy({
      targets: [
          { src: 'public/**/*', dest: './public/' },
          { src: 'stylish-theme.min.js', dest: './' },
          { src: '../firmware/binpack.js', dest: './' },
        ],
    }),
  ],
})