import React, { useState, useRef, useEffect, useCallback } from 'react';

function ImagePortCanvas({ width, height } ) {
  const canvasRef = useRef(null);
  const [isModuleReady, setModuleReady] = useState( false );
  // const [ frameTime, setFrameTime ] = useState( 0 );

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
    if (window.module) {
      const rect = canvasRef.current.getBoundingClientRect();
      const x = event.clientX - rect.left;
      const y = event.clientY - rect.top;
      window.module.mouse_move(x, y, width, height );
      window.module.mouse_over( true );
    }
  }, [ width, height ] );

  const handleMouseClick = useCallback(() => {
    if (window.module && canvasRef.current) {
      console.log( "handleMouseClick" );
      window.module.mouse_click( true );
      window.module.mouse_over(  true );
    }
  }, [] );

  const updateCanvas = useCallback(async () => {
    const ctx = canvasRef.current.getContext('2d');

    let t0 = performance.now();
    const imageDataArray = window.module.get_img_data();
    const bufWidth  = window.module.get_buf_width();
    const bufHeight = window.module.get_buf_height();

    //let t1, t2, t3, t4;
    //t1 = performance.now();
    //console.log( "updateCanvas get_img_data time = " + (t1 - t0) );
    const imageData = new ImageData(
        new Uint8ClampedArray(imageDataArray.buffer, imageDataArray.byteOffset, imageDataArray.byteLength),
        bufWidth,
        bufHeight
    );
    //t2 = performance.now();
    //console.log( "updateCanvas new ImageData time = " + (t2 - t1) );
    const imageBitmap = await createImageBitmap(imageData);
    //t3 = performance.now();
    //console.log( "updateCanvas createImageBitmap time = " + (t3 - t2) );
    //console.log( "width=" + width + " height=" + height );
    ctx.drawImage(imageBitmap, 0, 0, width, height);
    //ctx.putImageData(imageData, 0, 0);
    //t4 = performance.now();
    //console.log( "updateCanvas drawImage time = " + (t4 - t3) );
    //console.log( "frame time = " + (t4 - frameTime) );
    //setFrameTime( t4 );

  }, [width, height]);

  useEffect(() => {
    if (window.module) {
      window.module.set_frame_callback(updateCanvas);
      setModuleReady(true);
    } else {
      // Poll for the Module to be ready
      const intervalId = setInterval(() => {
        if (window.module) {
          clearInterval(intervalId);
          window.module.set_frame_callback(updateCanvas);
          setModuleReady(true);
        }
      }, 100); // Check every 100ms

      return () => clearInterval(intervalId);
    }
  }, [updateCanvas]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (canvas) {
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
    }
  }, [handleMouseMove, handleMouseClick, handleMouseDown, handleMouseUp, handleMouseEnter, handleMouseLeave]);

  return (
      <canvas ref={canvasRef} width={width} height={height}></canvas>
  );
}

export default ImagePortCanvas;