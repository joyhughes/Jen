import React, { useState, useRef, useEffect, useCallback } from 'react';
import { Box, CircularProgress } from '@mui/material';

function ImagePortCanvas({ width, height }) {
  const canvasRef = useRef(null);
  const [isModuleReady, setModuleReady] = useState(false);
  const [isInitializing, setIsInitializing] = useState(true);
  const prevSizeRef = useRef({ width, height });

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

  // Canvas rendering function
  const updateCanvas = useCallback(async () => {
    if (!canvasRef.current || !window.module) return;

    const ctx = canvasRef.current.getContext('2d', {
      alpha: false, // Optimization: no alpha needed for full coverage
      desynchronized: true // Potential optimization on supported browsers
    });

    if (!ctx) return;

    try {
      // Get image data from WebAssembly
      const imageDataArray = window.module.get_img_data();
      const bufWidth = window.module.get_buf_width();
      const bufHeight = window.module.get_buf_height();

      // Create ImageData from the raw buffer
      const imageData = new ImageData(
          new Uint8ClampedArray(imageDataArray.buffer, imageDataArray.byteOffset, imageDataArray.byteLength),
          bufWidth,
          bufHeight
      );

      // Create and render bitmap (faster than putImageData)
      const imageBitmap = await createImageBitmap(imageData);
      ctx.drawImage(imageBitmap, 0, 0, width, height);
      imageBitmap.close(); // Clean up bitmap resource

      // Mark initialization complete after first render
      if (isInitializing) {
        setIsInitializing(false);
      }
    } catch (error) {
      console.error('Canvas update error:', error);
    }
  }, [width, height, isInitializing]);

  // Set up WebAssembly callback
  useEffect(() => {
    if (window.module) {
      window.module.set_frame_callback(updateCanvas);
      setModuleReady(true);

      // Force a render after a brief delay to ensure we get the initial frame
      setTimeout(() => {
        updateCanvas();
      }, 100);
    } else {
      // Poll for the Module to be ready
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

  // Update canvas size when dimensions change
  useEffect(() => {
    const { width: prevWidth, height: prevHeight } = prevSizeRef.current;

    if (width !== prevWidth || height !== prevHeight) {
      prevSizeRef.current = { width, height };

      // Request a new frame when size changes
      if (window.module && isModuleReady) {
        updateCanvas();
      }
    }
  }, [width, height, isModuleReady, updateCanvas]);

  // Set up event listeners
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    canvas.addEventListener('mousemove', handleMouseMove);
    canvas.addEventListener('click', handleMouseClick);
    canvas.addEventListener('mousedown', handleMouseDown);
    canvas.addEventListener('mouseup', handleMouseUp);
    canvas.addEventListener('mouseenter', handleMouseEnter);
    canvas.addEventListener('mouseleave', handleMouseLeave);

    return () => {
      canvas.removeEventListener('mousemove', handleMouseMove);
      canvas.removeEventListener('click', handleMouseClick);
      canvas.removeEventListener('mousedown', handleMouseDown);
      canvas.removeEventListener('mouseup', handleMouseUp);
      canvas.removeEventListener('mouseenter', handleMouseEnter);
      canvas.removeEventListener('mouseleave', handleMouseLeave);
    };
  }, [
    handleMouseMove,
    handleMouseClick,
    handleMouseDown,
    handleMouseUp,
    handleMouseEnter,
    handleMouseLeave
  ]);

  return (
      <Box sx={{ position: 'relative', width, height }}>
        <canvas
            ref={canvasRef}
            width={width}
            height={height}
            style={{
              display: 'block',
              cursor: 'crosshair',
              imageRendering: 'pixelated', // Better for pixel art rendering
            }}
        />

        {/* Loading indicator */}
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