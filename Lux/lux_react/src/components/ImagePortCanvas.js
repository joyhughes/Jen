import { Paper } from '@mui/material';
import React from 'react';

function ImagePortCanvas( { width, height } ) {

/*  return (
    <canvas width={width} height={height}></canvas>
  );
*/
  return (
    <Paper elevation={3} sx={{ margin: 0 }} width={width} height={height}>
      <canvas width={width} height={height}></canvas>
    </Paper>
  );
} 

export default ImagePortCanvas; 