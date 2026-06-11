import path from "path";
import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";
import { VitePWA } from "vite-plugin-pwa";
import type { Plugin } from "vite";

// The CSP meta tag in index.html is production-only: the dev server
// needs inline scripts (react-refresh preamble) and a ws: connection
// (HMR), so serve mode strips the tag.
function stripCspForDev(): Plugin {
  return {
    name: "strip-csp-for-dev",
    apply: "serve",
    transformIndexHtml(html) {
      return html.replace(/<meta[^>]*http-equiv="Content-Security-Policy"[^>]*>\s*/i, "");
    },
  };
}

// https://vitejs.dev/config/
export default defineConfig({
  test: {
    environment: "jsdom",
    globals: true,
    setupFiles: "./src/test/setup.ts",
  },
  resolve: {
    alias: {
      "@": path.resolve(__dirname, "./src"),
    },
  },
  plugins: [
    stripCspForDev(),
    tailwindcss(),
    react(),
    VitePWA({
      registerType: "autoUpdate",
      workbox: {
        globPatterns: ["**/*.{js,css,html,ico,png,svg}"],
      },
      devOptions: {
        enabled: false,
      },
      manifest: {
        name: "Beatled Console",
        short_name: "Beatled",
        description: "Beatled App",
        theme_color: "#f96332",
        background_color: "#ffffff",
        icons: [
          {
            src: "/android-chrome-192x192.png",
            sizes: "192x192",
            type: "image/png",
          },
          {
            src: "/android-chrome-512x512.png",
            sizes: "512x512",
            type: "image/png",
          },
        ],
      },
    }),
  ],
  server: {
    proxy: {
      "/api": {
        target: "https://127.0.0.1:8443",
        changeOrigin: true,
        secure: false,
      },
    },
  },
  build: {
    sourcemap: false,
  },
});
