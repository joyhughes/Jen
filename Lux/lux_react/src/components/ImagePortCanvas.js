import React, { useState, useRef, useEffect, useCallback } from 'react';

function ImagePortCanvas( { width, height } ) {
  const canvasRef = useRef(null);
  const [isModuleReady, setModuleReady] = useState(false);

const updateCanvas = useCallback(async () => {
    const ctx = canvasRef.current.getContext('2d');

    const imageDataArray = window.Module.get_img_data();
    const bufWidth  = window.Module.get_buf_width();
    const bufHeight = window.Module.get_buf_height();
    
    const imageData = new ImageData(
      new Uint8ClampedArray(imageDataArray.buffer, imageDataArray.byteOffset, imageDataArray.byteLength),
      bufWidth,
      bufHeight
    );
    const imageBitmap = await createImageBitmap(imageData);
    ctx.drawImage(imageBitmap, 0, 0, width, height);

  }, [width, height]);

  useEffect(() => {
    if (window.Module) {
      setModuleReady(true);
    } else {
      // Poll for the Module to be ready
      const intervalId = setInterval(() => {
        if (window.Module) {
          window.Module.set_frame_callback(updateCanvas);
          setModuleReady(true);
          clearInterval(intervalId);
        }
      }, 100); // Check every 100ms

      return () => clearInterval(intervalId);
    }
  }, [updateCanvas]);


  return (
      <canvas ref={canvasRef} width={width} height={height}></canvas>
  );
} 

export default ImagePortCanvas; 