import React, { useState, useRef, useEffect, useCallback } from 'react';
import { Box, CircularProgress } from '@mui/material';
import { getLiveCameraManager } from './LiveCameraManager';

function ImagePortCanvas({ width, height }) {
  const canvasRef = useRef(null);
  const [isModuleReady, setModuleReady] = useState(false);
  const [isInitializing, setIsInitializing] = useState(true);
  const prevSizeRef = useRef({ width, height });
  const lastFrameTimeRef = useRef(0);


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
    if (!window.module || !canvasRef.current) return;

    const canvas = canvasRef.current;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const currentTime = performance.now();

    const frameTime = performance.now();
    const deltaTime = lastFrameTimeRef.current > 0 ? 
        (frameTime - lastFrameTimeRef.current) / 1000 : 0.016667; // Default to 60fps
    
    if (window.audioUpdateFunction && typeof window.audioUpdateFunction === 'function') {
        window.audioUpdateFunction(deltaTime);
    } 
    
    lastFrameTimeRef.current = frameTime;
    

    try {
      let isLiveCameraSource = false;
      try {
        if (window.module && typeof window.module.get_widget_JSON === 'function') {
          const sourceMenuJSON = window.module.get_widget_JSON('source_image_menu');
          const sourceMenu = JSON.parse(sourceMenuJSON);
          const currentSource = sourceMenu.items[sourceMenu.choice];
          isLiveCameraSource = currentSource === 'ultra_camera';
          
        }
      } catch (sourceError) {
        isLiveCameraSource = false;
      }

      const imageDataArray = window.module.get_img_data();
      const bufWidth = window.module.get_buf_width();
      const bufHeight = window.module.get_buf_height();

      if (!imageDataArray || imageDataArray.byteLength === 0) {
        return;
      }

      if (isLiveCameraSource) {
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
          
          // Read pixel data in BGRA format 
          const b = imageDataArray[pixelStart + 0]; // Blue from position 0
          const g = imageDataArray[pixelStart + 1]; // Green from position 1
          const r = imageDataArray[pixelStart + 2]; // Red from position 2
          const a = imageDataArray[pixelStart + 3]; // Alpha from position 3
          
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

      if (isInitializing) {
        setIsInitializing(false);
      }
      
    } catch (error) {
      console.error('[Canvas] Rendering error:', error);
    }
  }, [width, height, isInitializing]);


  useEffect(() => {

    if (window.module) {
      window.module.set_frame_callback(updateCanvas);
      setModuleReady(true);

      // Force initial render
      setTimeout(() => {
        updateCanvas();
      }, 100);
    } else {
      const intervalId = setInterval(() => {
        if (window.module) {
          clearInterval(intervalId);
          window.module.set_frame_callback(updateCanvas);
          setModuleReady(true);
        }
      }, 100);

      return () => clearInterval(intervalId);
    }
  }, [updateCanvas]);

  useEffect(() => {
    const { width: prevWidth, height: prevHeight } = prevSizeRef.current;

    if (width !== prevWidth || height !== prevHeight) {
      prevSizeRef.current = { width, height };

      if (window.module && isModuleReady) {
        updateCanvas();
      }
    }
  }, [width, height, isModuleReady, updateCanvas]);

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