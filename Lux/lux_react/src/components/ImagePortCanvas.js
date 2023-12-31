import React, { useState, useRef, useEffect, useCallback } from 'react';

function ImagePortCanvas( { width, height } ) {
  const canvasRef = useRef(null);
  const [isModuleReady, setModuleReady] = useState(false);
/*
  const updateCanvas = useCallback(async () => {
    const Module = window.Module;
    if (!Module) {
      console.error('Emscripten module is not available.');
      return;
    }

    const bufWidth = Module.get_image_width();
    const bufHeight = Module.get_image_height();
    const imageDataArray = Module.get_image_data();

    const imageData = new ImageData(
      new Uint8ClampedArray(imageDataArray.buffer, imageDataArray.byteOffset, imageDataArray.byteLength),
      bufWidth,
      bufHeight
    );

    const imageBitmap = await createImageBitmap(imageData);
    const ctx = canvasRef.current.getContext('2d');
    ctx.drawImage(imageBitmap, 0, 0, width, height);
  }, [width, height]);
*/
const updateCanvas = useCallback(async () => {
    //console.log("updateCanvas called");
    const ctx = canvasRef.current.getContext('2d');

    // Assuming you have a function to get the current buffer data
    const imageDataArray = window.Module.get_img_data();
    //console.log("get_img_data called");
    const bufWidth  = window.Module.get_buf_width();
    //console.log("bufWidth: ", bufWidth);
    const bufHeight = window.Module.get_buf_height();
    //console.log("bufHeight: ", bufHeight);
    //console.log("Buffer length:", imageDataArray.buffer.byteLength);
    //console.log("Expected length:", bufWidth * bufHeight * 4);
    const imageData = new ImageData(
      new Uint8ClampedArray(imageDataArray.buffer, imageDataArray.byteOffset, imageDataArray.byteLength),
      bufWidth,
      bufHeight
    );
    //console.log("imageData created");
    const imageBitmap = await createImageBitmap(imageData);
    ctx.drawImage(imageBitmap, 0, 0, width, height);

  }, [width, height]);

  useEffect(() => {
    if (window.Module) {
      window.Module.set_frame_callback(updateCanvas);
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