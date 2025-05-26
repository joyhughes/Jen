import React, { useState, useRef, useEffect, useCallback } from 'react';
import { Box, CircularProgress } from '@mui/material';

function ImagePortCanvas({ width, height, isLiveCameraActive = false }) {
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

  // DUAL-MODE Canvas rendering function
  const updateCanvas = useCallback(async () => {
    if (!window.module || !canvasRef.current) return;

    const canvas = canvasRef.current;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // Controlled logging
    const logging = loggingRef.current;
    const currentTime = performance.now();
    const shouldLog = currentTime - logging.lastLogTime > logging.logInterval;
    
    if (shouldLog) {
      logging.lastLogTime = currentTime;
      logging.frameCount++;
    }

    try {
      if (isLiveCameraActive) {
        // CAMERA MODE: Live camera sends BGRA format - need to convert to RGBA
        if (shouldLog && logging.debugEnabled) {
          console.log('[Canvas-Camera] Processing live camera frame (BGRA format)...');
        }
        
        // Get image data from WebAssembly (camera processed data in BGRA format)
        const imageDataArray = window.module.get_img_data();
        const bufWidth = window.module.get_buf_width();
        const bufHeight = window.module.get_buf_height();

        if (shouldLog && logging.debugEnabled) {
          console.log(`[Canvas-Camera] Buffer: ${bufWidth}x${bufHeight}, Data length: ${imageDataArray?.byteLength}`);
          
          // Check camera processed data
          if (imageDataArray && imageDataArray.byteLength > 0) {
            let nonZeroCount = 0;
            const sampleSize = Math.min(1000, imageDataArray.byteLength);
            for (let i = 0; i < sampleSize; i++) {
              if (imageDataArray[i] > 0) nonZeroCount++;
            }
            console.log(`[Canvas-Camera] Processed pixels: ${nonZeroCount}/${sampleSize} (${((nonZeroCount/sampleSize)*100).toFixed(1)}%)`);
          }
        }
        
        if (!imageDataArray || imageDataArray.byteLength === 0) {
          if (shouldLog) {
            console.warn('[Canvas-Camera] No processed camera data available');
          }
          return;
        }

        // CRITICAL: Camera data is in BGRA format - convert to RGBA for Canvas
        const pixelCount = bufWidth * bufHeight;
        const expectedBytes = pixelCount * 4;
        
        if (imageDataArray.byteLength !== expectedBytes) {
          console.error(`[Canvas-Camera] Size mismatch: expected ${expectedBytes}, got ${imageDataArray.byteLength}`);
          return;
        }

        console.log('[Canvas-Camera] Converting BGRA → RGBA for camera display');
        
        // Create new array for RGBA data
        const rgbaData = new Uint8ClampedArray(expectedBytes);
        
        // Convert BGRA to RGBA pixel by pixel
        for (let i = 0; i < pixelCount; i++) {
          const pixelStart = i * 4;
          
          // Read pixel data in BGRA format (how camera backend stores it)
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
        // NORMAL MODE: Regular images are already in RGBA format - use directly
        if (shouldLog && logging.debugEnabled) {
          console.log('[Canvas-Normal] Displaying regular image (RGBA format)...');
        }
        
        // Get image data from WebAssembly (regular backend buffer in RGBA format)
        const imageDataArray = window.module.get_img_data();
        const bufWidth = window.module.get_buf_width();
        const bufHeight = window.module.get_buf_height();

        if (shouldLog && logging.debugEnabled) {
          console.log(`[Canvas-Normal] Buffer: ${bufWidth}x${bufHeight}, Data length: ${imageDataArray?.byteLength}`);
        }
        
        if (!imageDataArray || imageDataArray.byteLength === 0) {
          if (shouldLog) {
            console.warn('[Canvas-Normal] No image data available from backend');
          }
          return;
        }

        // Direct RGBA display (original behavior - no conversion needed)
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
        console.log(`[Canvas] Initialization complete - Mode: ${isLiveCameraActive ? 'Camera (BGRA→RGBA)' : 'Normal (RGBA)'}`);
        setIsInitializing(false);
      }
      
    } catch (error) {
      console.error(`[Canvas] Error in ${isLiveCameraActive ? 'camera' : 'normal'} mode:`, error);
    }
  }, [width, height, isInitializing, isLiveCameraActive]);

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