import React, { useState, useEffect } from "react";
import {Box} from '@mui/material';
import Paper from '@mui/material/Paper';

import ImagePortCanvas from './ImagePortCanvas';

function ImagePort( { dimensions } ) {
  
  const imagePortRef = React.useRef();

  return (
    <Paper 
      elevation={3} 
      sx={{ 
        margin: 0, 
        width: dimensions.width,
        height: dimensions.height,
      }}
    >
      <Box ref={imagePortRef} sx={{ padding: 0 }}>
        <Box mb={2}>
          <ImagePortCanvas width = {dimensions.width} height = {dimensions.height}/>
        </Box>
      </Box>
    </Paper>
  );


}

export default ImagePort;