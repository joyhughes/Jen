import React from "react";
import ReactDOM from "react-dom/client";
import "./index.css";
import App from "./App.jsx";

/*
var factory = require('./lux.js');

factory().then((instance) => {
  window.module = instance; // Assign to global variable
  window.module.canvas = document.getElementById('imagePortCanvas');
});
*/

// Initialize WASM with integrated camera optimizations
(async () => {
  try {
    console.log(
      "[WASM] Loading lux.js with integrated camera optimizations..."
    );

    const module = await import("./lux.js");
    const factory = module.default ? module.default : module;

    console.log("[WASM] Factory loaded, initializing module...");

    factory()
      .then((instance) => {
        console.log("[WASM] Module instance created");
        window.module = instance; // Assign to global variable
        window.module.canvas = document.getElementById("imagePortCanvas");

        console.log("[WASM] Checking available functions...");
        const allFunctions = Object.keys(window.module).filter(
          (key) => typeof window.module[key] === "function"
        );
        console.log("[WASM] Total functions available:", allFunctions.length);
        console.log("[WASM] All functions:", allFunctions);

        // Verify ULTRA camera functions are available (new optimized names)
        const ultraCameraFunctions = [
          "ultra_update_camera_frame",
          "ultra_start_camera_stream",
          "ultra_stop_camera_stream",
          "ultra_process_camera_with_kaleidoscope",
          "ultra_get_camera_stats",
        ];

        console.log("[WASM] Checking ultra camera functions...");
        const missingUltraFunctions = ultraCameraFunctions.filter((fn) => {
          const available = typeof window.module[fn] === "function";
          console.log(`[WASM] ${fn}: ${available ? "✅" : "❌"}`);
          return !available;
        });

        // Also check basic required functions
        const basicFunctions = [
          "get_img_data",
          "get_buf_width",
          "get_buf_height",
          "set_frame_callback",
          "update_source_name",
          "get_widget_JSON",
        ];

        console.log("[WASM] Checking basic functions...");
        const missingBasicFunctions = basicFunctions.filter((fn) => {
          const available = typeof window.module[fn] === "function";
          console.log(`[WASM] ${fn}: ${available ? "✅" : "❌"}`);
          return !available;
        });

        if (
          missingUltraFunctions.length === 0 &&
          missingBasicFunctions.length === 0
        ) {
          console.log(
            "[WASM] ✅ Module initialized with ULTRA camera optimizations"
          );
          window.module.cameraReady = true;
          window.module.ultraCameraReady = true;
        } else {
          console.warn("[WASM] ⚠️ Some functions missing:");
          if (missingUltraFunctions.length > 0) {
            console.warn(
              "[WASM] Missing ultra camera functions:",
              missingUltraFunctions
            );
          }
          if (missingBasicFunctions.length > 0) {
            console.warn(
              "[WASM] Missing basic functions:",
              missingBasicFunctions
            );
          }
          window.module.cameraReady = false;
          window.module.ultraCameraReady = false;
        }

        console.log("[WASM] Module initialization complete");
        console.log("[WASM] Camera ready:", window.module.cameraReady);
        console.log(
          "[WASM] Ultra camera ready:",
          window.module.ultraCameraReady
        );

        // REMOVED: Problematic worker initialization that was causing errors
        // The video recording functionality will work directly through the main module
        // without requiring a separate worker for now
      })
      .catch((error) => {
        console.error("[WASM] Module factory error:", error);
      });
  } catch (error) {
    console.error("[WASM] Error loading module:", error);
    console.error("[WASM] Error stack:", error.stack);
  }
})();

const root = ReactDOM.createRoot(document.getElementById("root"));
root.render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);
