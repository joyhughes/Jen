// ImagePort.js
import React, { useState, useEffect } from "react";
import {Box} from '@mui/material';
import Paper from '@mui/material/Paper';

import ImagePortCanvas from './ImagePortCanvas';

function ImagePort( { ratio, panelSize } ) {
  
  const imagePortRef = React.useRef();
/*
  // This useEffect hook will query the dimensions of ImagePortCanvas and MediaController
  // You need to implement the querying part based on your components
  React.useEffect(() => {
    if (imagePortRef.current) {
      // query the dimensions here and store them in state variables or context
    }
  }, [imagePortRef]);

  const height = 512;
  const width = 512;
  */

  const [dimensions, setDimensions] = useState({ width: 0, height: 0 });

  const resizeBox = () => {
    let windowRatio, width, height, availableWidth, availableHeight;

    windowRatio = window.innerWidth / window.innerHeight;
    if( windowRatio > ratio ) {
      availableWidth = window.innerWidth - parseInt(panelSize, 10);
      availableHeight = window.innerHeight;
    }
    else {
      availableWidth = window.innerWidth;
      availableHeight = window.innerHeight - ( parseInt(panelSize, 10) / ratio );
    }

    //console.log("resizeBox: innerWidth = " + window.innerWidth + ", innerHeight = " + window.innerHeight);
    //console.log("--------------------");
    //console.log("resizeBox: initial windowRatio = " + windowRatio);
    //console.log("resizeBox: availableWidth = " + availableWidth + ", availableHeight = " + availableHeight);

    windowRatio = availableWidth / availableHeight;
    if ( windowRatio > ratio ) {
      height = availableHeight; 
      width = height * ratio;
    } else {
      width = availableWidth; 
      height = width / ratio;
    }

    //console.log("resizeBox: ratio = " + ratio + ", panelSize = " + panelSize);
    //console.log("resizeBox: new windowRatio = " + windowRatio);
    //console.log("resizeBox: width = " + width + ", height = " + height);

    setDimensions({ width, height });
  };

  
  useEffect(() => {
    window.addEventListener("resize", resizeBox);
    resizeBox();
    return () => window.removeEventListener("resize", resizeBox);
  }, [ ratio, panelSize ]);


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