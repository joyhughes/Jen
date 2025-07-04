import React, { useState, useRef, useEffect, useCallback } from 'react';
import { Box, CircularProgress } from '@mui/material';
import { getLiveCameraManager } from './LiveCameraManager';

function ImagePortCanvas({ width, height }) {
  const canvasRef = useRef(null);
  const [isModuleReady, setModuleReady] = useState(false);
  const [isInitializing, setIsInitializing] = useState(true);
  const prevSizeRef = useRef({ width, height });
  const lastFrameTimeRef = useRef(0);

  // Add logging control to prevent spam
  const loggingRef = useRef({
    frameCount: 0,
    lastLogTime: 0,
    logInterval: 1000, // Log every 1000ms instead of every frame
    debugEnabled: false, // Disable for better performance
    audioCallCount: 0,
    lastAudioLog: 0
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

  // DUAL-MODE Canvas rendering function with color format detection + Audio Integration
  const updateCanvas = useCallback(async () => {
    if (!window.module || !canvasRef.current) return;

    const canvas = canvasRef.current;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // Controlled logging - MUST BE FIRST
    const logging = loggingRef.current;
    const currentTime = performance.now();
    const shouldLog = currentTime - logging.lastLogTime > logging.logInterval;

    // AUDIO INTEGRATION: Update audio parameters with main animation loop
    const frameTime = performance.now();
    const deltaTime = lastFrameTimeRef.current > 0 ? 
        (frameTime - lastFrameTimeRef.current) / 1000 : 0.016667; // Default to 60fps
    
    // Call audio update function if available
    if (window.audioUpdateFunction && typeof window.audioUpdateFunction === 'function') {
        window.audioUpdateFunction(deltaTime);
        logging.audioCallCount++;
        
        // Log audio calls every 2 seconds
        if (frameTime - logging.lastAudioLog > 2000) {
            console.log(`[Canvas] 🎵 Audio update called ${logging.audioCallCount} times, deltaTime: ${deltaTime.toFixed(4)}s`);
            logging.lastAudioLog = frameTime;
            logging.audioCallCount = 0;
        }
    } else if (shouldLog && logging.debugEnabled) {
        console.log('[Canvas] 🎵 Audio update function not available:', {
            exists: !!window.audioUpdateFunction,
            type: typeof window.audioUpdateFunction
        });
    }
    
    lastFrameTimeRef.current = frameTime;
    
    if (shouldLog) {
      logging.lastLogTime = currentTime;
      logging.frameCount++;
    }

    try {
      // CORRECT DETECTION: Check the actual current source from backend
      let isLiveCameraSource = false;
      try {
        if (window.module && typeof window.module.get_widget_JSON === 'function') {
          const sourceMenuJSON = window.module.get_widget_JSON('source_image_menu');
          const sourceMenu = JSON.parse(sourceMenuJSON);
          const currentSource = sourceMenu.items[sourceMenu.choice];
          isLiveCameraSource = currentSource === 'ultra_camera';
          
          if (shouldLog && logging.debugEnabled) {
            console.log(`[Canvas] Current source: "${currentSource}", isLiveCamera: ${isLiveCameraSource}`);
          }
        }
      } catch (sourceError) {
        // Fallback: if we can't get source info, assume regular image
        isLiveCameraSource = false;
        if (shouldLog && logging.debugEnabled) {
          console.log('[Canvas] Could not detect source, assuming regular image');
        }
      }

      // Get image data from WebAssembly backend
      const imageDataArray = window.module.get_img_data();
      const bufWidth = window.module.get_buf_width();
      const bufHeight = window.module.get_buf_height();

      if (shouldLog && logging.debugEnabled) {
        console.log(`[Canvas] Mode: ${isLiveCameraSource ? 'Live Camera (BGRA)' : 'Regular Image (RGBA)'}`);
        console.log(`[Canvas] Buffer: ${bufWidth}x${bufHeight}, Data length: ${imageDataArray?.byteLength}`);
      }
      
      if (!imageDataArray || imageDataArray.byteLength === 0) {
        if (shouldLog) {
          console.warn('[Canvas] No image data available from backend');
        }
        return;
      }

      if (isLiveCameraSource) {
        // LIVE CAMERA MODE: Backend sends BGRA format - convert to RGBA for display
        if (shouldLog && logging.debugEnabled) {
          console.log('[Canvas-Camera] Converting BGRA → RGBA for live camera display');
        }

        const pixelCount = bufWidth * bufHeight;
        const expectedBytes = pixelCount * 4;
        
        if (imageDataArray.byteLength !== expectedBytes) {
          console.error(`[Canvas-Camera] Size mismatch: expected ${expectedBytes}, got ${imageDataArray.byteLength}`);
          return;
        }

        // Create new array for RGBA data
        const rgbaData = new Uint8ClampedArray(expectedBytes);
        
        // Convert BGRA to RGBA pixel by pixel
        for (let i = 0; i < pixelCount; i++) {
          const pixelStart = i * 4;
          
          // Read pixel data in BGRA format (how camera backend sends it)
          const b = imageDataArray[pixelStart + 0]; // Blue from position 0
          const g = imageDataArray[pixelStart + 1]; // Green from position 1
          const r = imageDataArray[pixelStart + 2]; // Red from position 2
          const a = imageDataArray[pixelStart + 3]; // Alpha from position 3
          
          // Write pixel data in RGBA format (how Canvas expects it)
          rgbaData[pixelStart + 0] = r; // Red moves to position 0
          rgbaData[pixelStart + 1] = g; // Green stays in position 1
          rgbaData[pixelStart + 2] = b; // Blue moves to position 2
          rgbaData[pixelStart + 3] = a; // Alpha stays in position 3
        }

        // Create ImageData from the converted RGBA data
        const imageData = new ImageData(rgbaData, bufWidth, bufHeight);

        // Render converted camera data
        const imageBitmap = await createImageBitmap(imageData);
        ctx.drawImage(imageBitmap, 0, 0, width, height);
        imageBitmap.close();

      } else {
        // REGULAR IMAGE MODE: Still images are already in RGBA format - use directly
        if (shouldLog && logging.debugEnabled) {
          console.log('[Canvas-Regular] Displaying regular image (RGBA format, no conversion)');
        }

        // Display image data directly (original behavior - no conversion needed)
        const imageData = new ImageData(
            new Uint8ClampedArray(imageDataArray.buffer, imageDataArray.byteOffset, imageDataArray.byteLength),
            bufWidth,
            bufHeight
        );

        // Render regular image
        const imageBitmap = await createImageBitmap(imageData);
        ctx.drawImage(imageBitmap, 0, 0, width, height);
        imageBitmap.close();
      }

      // Mark initialization complete after first render
      if (isInitializing) {
        console.log(`[Canvas] Initialization complete - Mode: ${isLiveCameraSource ? 'Camera (BGRA→RGBA)' : 'Regular (RGBA)'}`);
        setIsInitializing(false);
      }
      
    } catch (error) {
      console.error('[Canvas] Rendering error:', error);
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