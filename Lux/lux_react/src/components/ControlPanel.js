import React, { useState, useEffect } from "react";
import Paper from '@mui/material/Paper';

import WidgetGroup from './WidgetGroup'; 
import MediaController from "./MediaController";

//import Module from './useEmscripten';

function ControlPanel( { ratio, panelSize } ) {

  const [dimensions, setDimensions] = useState({ width: 0, height: 0, isRowDirection: true });
  const [ groupJSON, setGroupJSON ] = useState( [] );

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
  
  const setupPanel = () => {
    const panelJSONString = window.Module.get_panel_JSON();
    console.log( "ControlPanel setupPanel panelJSONString=" + panelJSONString );
  
    try {
      const parsedJSON = JSON.parse(panelJSONString);
      setGroupJSON(parsedJSON);
      console.log( "ControlPanel setupPanel groupJSON=" + JSON.stringify( groupJSON ) );
    } catch (error) {
      console.error("Error parsing JSON:", error);
      // Handle the error appropriately
    }
  };

  useEffect(() => {
    if (window.Module) {
        setupPanel();
    } else {
        // Poll for the Module to be ready
        const intervalId = setInterval(() => {
        if (window.Module) {
            setupPanel();
            clearInterval(intervalId);
        }
      }, 100); // Check every 100ms

      return () => clearInterval(intervalId);
    }
  }, [] );

  useEffect(() => {
    console.log("Updated groupJSON:", + JSON.stringify( groupJSON ));
  }, [groupJSON]);

  // Create list of widget groups from groupJSON
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
      < MediaController panelSize={panelSize} />
      {groupJSON.map((group, index) => (
        <WidgetGroup key={index} panelSize={panelSize} json={group} />
      ))}
    </Paper>
  );
}

export default ControlPanel; 

/*
import React, { useState, useEffect } from "react";
import Paper from '@mui/material/Paper';

import MediaController from './MediaController';
import JenSlider from './JenSlider';
import JenMenu from "./JenMenu";
import WidgetContainer from "./WidgetContainer";

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
      <WidgetContainer panelSize={panelSize}>
        <MediaController />
      </WidgetContainer>
      <WidgetContainer panelSize={panelSize}>
        <JenSlider sliderName="Main Slider" width={ panelSize - 80 } />
      </WidgetContainer>
      <WidgetContainer panelSize={panelSize}>
        <JenMenu menuName="Main Menu" panelSize={panelSize} />
      </WidgetContainer>
    </Paper>
  );
  
}

export default ControlPanel; 
*/
