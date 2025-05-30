import React, { useState, useRef, useEffect, useCallback } from 'react';
import { Box, CircularProgress } from '@mui/material';

function ImagePortCanvas({ width, height, isLiveCameraActive }) {
  const canvasRef = useRef(null);
  const [isModuleReady, setModuleReady] = useState(false);
  const [isInitializing, setIsInitializing] = useState(true);
  const prevSizeRef = useRef({ width, height });

  // Add logging control to prevent spam
  const loggingRef = useRef({
    frameCount: 0,
    lastLogTime: 0,
    logInterval: 1000, // Log every 1000ms instead of every frame
    debugEnabled: false // Set to true only for debugging
  });

  // Mouse event handlers
  const handleMouseDown = useCallback(() => {
    if (window.module) {
      window.module.mouse_down(true);
    }
  }, []);

  const handleMouseUp = useCallback(() => {
    if (window.module) {
      window.module.mouse_down(false);
    }
  }, []);

  const handleMouseEnter = useCallback(() => {
    if (window.module) {
      window.module.mouse_over(true);
    }
  }, []);

  const handleMouseLeave = useCallback(() => {
    if (window.module) {
      window.module.mouse_over(false);
    }
  }, []);


  const handleMouseMove = useCallback((event) => {
    if (!window.module || !canvasRef.current) return;

    const rect = canvasRef.current.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;

    window.module.mouse_move(x, y, width, height);
    window.module.mouse_over(true);
  }, [width, height]);

  const handleMouseClick = useCallback(() => {
    if (window.module && canvasRef.current) {
      window.module.mouse_click(true);
      window.module.mouse_over(true);
    }
  }, []);

  const updateCanvas = useCallback(async () => {
    const now = performance.now();
    const logging = loggingRef.current;
    logging.frameCount++;

    // Intelligent logging control - this prevents console spam while still giving us
    // the diagnostic information we need during development and debugging
    const shouldLog = logging.debugEnabled && (now - logging.lastLogTime > logging.logInterval);
    const shouldAnalyzeColors = logging.frameCount <= 3; // Deep color analysis for first 3 frames only

    if (shouldLog) {
      console.log(`[Canvas] Frame ${logging.frameCount}: updateCanvas called`);
      logging.lastLogTime = now;
    }

    // Early validation - we check for essential dependencies before doing any work
    // This pattern prevents unnecessary processing and provides clear error messages
    if (!canvasRef.current || !window.module) {
      if (shouldLog) {
        console.log('[Canvas] Early return: missing canvas or module');
      }
      return;
    }

    // Get the 2D rendering context with performance optimizations
    // These context options are specifically chosen for maximum rendering speed
    const ctx = canvasRef.current.getContext('2d', {
      alpha: false,        // We don't need alpha compositing - saves processing time
      desynchronized: true // Allows the browser to optimize rendering timing
    });

    if (!ctx) {
      if (shouldLog) {
        console.log('[Canvas] Failed to get 2d context');
      }
      return;
    }

    try {
      // STEP 1: Retrieve processed image data from WebAssembly backend
      // This is where your beautifully processed kaleidoscope data comes from
      const backendImageData = window.module.get_img_data();
      const bufWidth = window.module.get_buf_width();
      const bufHeight = window.module.get_buf_height();

      // Validate that we actually received image data from the backend
      if (!backendImageData || backendImageData.byteLength === 0) {
        if (shouldLog) {
          console.warn('[Canvas] No image data available from module');
        }
        return;
      }

      const pixelCount = bufWidth * bufHeight;
      const expectedBytes = pixelCount * 4; // 4 bytes per pixel (RGBA/BGRA)

      // Data integrity check - ensures our backend and frontend agree on data size
      if (backendImageData.byteLength !== expectedBytes) {
        console.error(`[Canvas] Size mismatch: expected ${expectedBytes}, got ${backendImageData.byteLength}`);
        return;
      }

      if (shouldLog || shouldAnalyzeColors) {
        console.log(`[Canvas] Processing ${bufWidth}x${bufHeight} image (${expectedBytes} bytes)`);
      }

      // STEP 2: Advanced Color Format Analysis (for debugging and understanding)
      // This section helps us understand what format our backend is actually returning
      if (shouldAnalyzeColors) {
        console.log(`=== FRAME ${logging.frameCount} COLOR FORMAT ANALYSIS ===`);

        // Sample pixels from different regions to understand color distribution
        const sampleIndices = [
          0,                                    // Top-left corner
          Math.floor(pixelCount * 0.25),       // First quarter
          Math.floor(pixelCount * 0.5),        // Center
          Math.floor(pixelCount * 0.75),       // Third quarter
          pixelCount - 1                       // Bottom-right corner
        ];

        console.log('Backend raw data samples:');
        sampleIndices.forEach((pixelIndex, i) => {
          const byteIndex = pixelIndex * 4;
          if (byteIndex + 3 < backendImageData.byteLength) {
            const byte0 = backendImageData[byteIndex + 0];
            const byte1 = backendImageData[byteIndex + 1];
            const byte2 = backendImageData[byteIndex + 2];
            const byte3 = backendImageData[byteIndex + 3];

            console.log(`  Sample ${i} (pixel ${pixelIndex}): [${byte0}, ${byte1}, ${byte2}, ${byte3}]`);
          }
        });

        // Statistical analysis to determine the most likely color format
        // This helps us verify our BGRA assumption through mathematical analysis
        let totalChannel0 = 0, totalChannel1 = 0, totalChannel2 = 0, totalChannel3 = 0;
        const analysisSize = Math.min(1000, pixelCount); // Sample first 1000 pixels

        for (let i = 0; i < analysisSize; i++) {
          const pixelStart = i * 4;
          totalChannel0 += backendImageData[pixelStart + 0];
          totalChannel1 += backendImageData[pixelStart + 1];
          totalChannel2 += backendImageData[pixelStart + 2];
          totalChannel3 += backendImageData[pixelStart + 3];
        }

        const avgCh0 = Math.round(totalChannel0 / analysisSize);
        const avgCh1 = Math.round(totalChannel1 / analysisSize);
        const avgCh2 = Math.round(totalChannel2 / analysisSize);
        const avgCh3 = Math.round(totalChannel3 / analysisSize);

        console.log(`Channel averages: [${avgCh0}, ${avgCh1}, ${avgCh2}, ${avgCh3}]`);
        console.log('Interpreting as BGRA: B=' + avgCh0 + ' G=' + avgCh1 + ' R=' + avgCh2 + ' A=' + avgCh3);
      }

      // STEP 3: CRITICAL COLOR CONVERSION - BGRA to RGBA
      // This is the heart of our solution! Based on our analysis, your backend
      // returns data in BGRA format, but HTML5 Canvas expects RGBA format
      const canvasImageData = new Uint8ClampedArray(expectedBytes);

      console.log('[Canvas] Applying BGRA → RGBA conversion (corrected format)');

      // Efficient pixel-by-pixel conversion loop
      // We process all pixels in a single pass for optimal performance
      for (let i = 0; i < pixelCount; i++) {
        const pixelStart = i * 4;

        // Read pixel data in BGRA format (how your backend stores it)
        // Your get_img_data() function returns bytes in this specific order
        let b, g, r, a;
        if (isLiveCameraActive) {
          b = backendImageData[pixelStart + 0]; // Blue from position 0
          g = backendImageData[pixelStart + 1]; // Green from position 1
          r = backendImageData[pixelStart + 2]; // Red from position 2
          a = backendImageData[pixelStart + 3]; // Alpha from position 3
        } else {
          r = backendImageData[pixelStart + 0]; // Blue from position 0
          g = backendImageData[pixelStart + 1]; // Green from position 1
          b = backendImageData[pixelStart + 2]; // Red from position 2
          a = backendImageData[pixelStart + 3]; // Alpha from position 3
        }

        // Write pixel data in RGBA format (how Canvas expects it)
        // We're essentially reordering the color channels to match web standards
        canvasImageData[pixelStart + 0] = r; // Red moves to position 0
        canvasImageData[pixelStart + 1] = g; // Green stays in position 1
        canvasImageData[pixelStart + 2] = b; // Blue moves to position 2
        canvasImageData[pixelStart + 3] = a; // Alpha stays in position 3
      }

      // STEP 5: Create HTML5 ImageData Object
      // This object encapsulates our converted pixel data in a format that
      // the Canvas API can understand and render efficiently
      const imageData = new ImageData(canvasImageData, bufWidth, bufHeight);

      // STEP 6: High-Performance Rendering
      // We use createImageBitmap for optimal performance - it's faster than putImageData
      // for most use cases and handles hardware acceleration better
      const imageBitmap = await createImageBitmap(imageData);

      // Render the processed and color-corrected image to the canvas
      // The drawImage call scales our kaleidoscope to fit the display dimensions
      ctx.drawImage(imageBitmap, 0, 0, width, height);

      // Clean up the ImageBitmap immediately to free memory
      // This is important for performance in real-time applications
      imageBitmap.close();

      // STEP 7: Success Logging and State Management
      if (shouldLog) {
        console.log('[Canvas] ✓ Frame rendered successfully with BGRA→RGBA conversion');
      }

      // Mark initialization as complete after the first successful render
      // This removes any loading indicators and enables full functionality
      if (isInitializing) {
        console.log('[Canvas] ✓ Initialization complete - color conversion working perfectly');
        setIsInitializing(false);
      }

    } catch (error) {
      // Comprehensive error handling with detailed diagnostics
      console.error('[Canvas] Canvas update error:', error);
      console.error('[Canvas] Error details:', error.stack);

      // Log the current state for debugging
      if (window.module) {
        console.error('[Canvas] Module state: available');
        console.error('[Canvas] Buffer dimensions:', window.module.get_buf_width?.(), 'x', window.module.get_buf_height?.());
      } else {
        console.error('[Canvas] Module state: not available');
      }
    }
  }, [width, height, isInitializing]);

  // This dependency array ensures the callback is recreated only when these values change
  // This is important for performance - we don't want to recreate the function unnecessarily

  // Set up WebAssembly callback
  useEffect(() => {
    console.log('[Canvas] Setting up WebAssembly callback...');

    if (window.module) {
      console.log('[Canvas] Module ready, setting frame callback...');
      window.module.set_frame_callback(updateCanvas);
      setModuleReady(true);

      // Force initial render
      setTimeout(() => {
        console.log('[Canvas] Calling initial updateCanvas...');
        updateCanvas();
      }, 100);
    } else {
      console.log('[Canvas] Module not ready, polling...');
      const intervalId = setInterval(() => {
        if (window.module) {
          console.log('[Canvas] Module became available, setting up...');
          clearInterval(intervalId);
          window.module.set_frame_callback(updateCanvas);
          setModuleReady(true);
        }
      }, 100);

      return () => clearInterval(intervalId);
    }
  }, [updateCanvas]);

  // Update canvas size when dimensions change
  useEffect(() => {
    const { width: prevWidth, height: prevHeight } = prevSizeRef.current;

    if (width !== prevWidth || height !== prevHeight) {
      prevSizeRef.current = { width, height };

      if (window.module && isModuleReady) {
        updateCanvas();
      }
    }
  }, [width, height, isModuleReady, updateCanvas]);

  // Set up event listeners
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const handleForceRedraw = () => {
      updateCanvas();
    };

    canvas.addEventListener('mousemove', handleMouseMove);
    canvas.addEventListener('click', handleMouseClick);
    canvas.addEventListener('mousedown', handleMouseDown);
    canvas.addEventListener('mouseup', handleMouseUp);
    canvas.addEventListener('mouseenter', handleMouseEnter);
    canvas.addEventListener('mouseleave', handleMouseLeave);
    canvas.addEventListener('forceRedraw', handleForceRedraw);

    return () => {
      canvas.removeEventListener('mousemove', handleMouseMove);
      canvas.removeEventListener('click', handleMouseClick);
      canvas.removeEventListener('mousedown', handleMouseDown);
      canvas.removeEventListener('mouseup', handleMouseUp);
      canvas.removeEventListener('mouseenter', handleMouseEnter);
      canvas.removeEventListener('mouseleave', handleMouseLeave);
      canvas.removeEventListener('forceRedraw', handleForceRedraw);
    };
  }, [
    handleMouseMove,
    handleMouseClick,
    handleMouseDown,
    handleMouseUp,
    handleMouseEnter,
    handleMouseLeave,
    updateCanvas
  ]);

  return (
    <Box sx={{ position: 'relative', width, height }}>
      <canvas
        ref={canvasRef}
        width={width}
        height={height}
        data-engine="true"
        style={{
          display: 'block',
          cursor: 'crosshair',
          imageRendering: 'pixelated',
        }}
      />

      {(isInitializing || !isModuleReady) && (
        <Box
          sx={{
            position: 'absolute',
            top: 0,
            left: 0,
            right: 0,
            bottom: 0,
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            background: 'rgba(0, 0, 0, 0.5)',
            backdropFilter: 'blur(4px)',
            zIndex: 10,
          }}
        >
          <CircularProgress color="primary" size={48} />
        </Box>
      )}
    </Box>
  );
}

export default ImagePortCanvas;