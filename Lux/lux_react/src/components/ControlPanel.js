import React, { useState, useEffect } from "react";
import Paper from '@mui/material/Paper';

import MediaController from './MediaController';
import JenSlider from './JenSlider';
import JenMenu from "./JenMenu";

//import Module from './useEmscripten';

function ControlPanel( { ratio, panelSize } ) {

  const [radioValue, setRadioValue] = useState('option1');
  const [textValue, setTextValue] = useState('');
  const [dimensions, setDimensions] = useState({ width: 0, height: 0, isRowDirection: true });
  const resizeBox = () => {
    let windowRatio, width, height, isRowDirection;

    windowRatio = window.innerWidth / window.innerHeight;
    if( windowRatio > ratio ) {
      width = parseInt(panelSize, 10);
      height = window.innerHeight;
      isRowDirection = false;
    }
    else {
      width = window.innerWidth;
      height = parseInt(panelSize, 10) / ratio;
      isRowDirection = true;
    }

    setDimensions({ width, height, isRowDirection });
  };

  useEffect(() => {
    window.addEventListener("resize", resizeBox);
    resizeBox();
    return () => window.removeEventListener("resize", resizeBox);
  }, [ ratio, panelSize ]);

  const handleRadioChange = (event) => {
    setRadioValue(event.target.value);
  };

  const handleTextChange = (event) => {
    setTextValue(event.target.value);
  };
  
  // Future: generate this list from the scene file
  return (
    <Paper elevation={3} 
      sx={{ 
        minWidth: dimensions.width,
        minHeight: dimensions.height,
        display: 'flex',
        flexDirection: dimensions.isRowDirection ? 'row' : 'column',
        flexWrap: 'wrap',
        flexGrow: 1,
        alignItems: 'flex-start',
        alignSelf: 'stretch',
      }}
    >
      <MediaController panelSize={panelSize} />
      <JenSlider sliderName="Main Slider" panelSize={panelSize} />
      <JenMenu   menuName="Main Menu"     panelSize={panelSize} />
    </Paper>
  );
  
}

export default ControlPanel; 
