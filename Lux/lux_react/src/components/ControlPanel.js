import React, { useState, useEffect } from "react";
import Paper from '@mui/material/Paper';

import WidgetGroup from './WidgetGroup'; 
import MediaController from "./MediaController";

//import Module from './useEmscripten';

function ControlPanel( { ratio, panelSize } ) {

  const [ dimensions,   setDimensions ]  = useState({ width: 0, height: 0, isRowDirection: true });
  const [ panelJSON,    setPanelJSON ]   = useState( [] );
  const [ activeGroups, setActiveGroups] = useState( [] );

  // Callback function to handle state changes in widget groups
  const handleWidgetGroupChange = () => {
    // Logic to update the control panel's state or perform other actions
    const active = panelJSON.filter(group => 
      window.Module.is_widget_group_active(group.name)
    );
    setActiveGroups(active);
    //console.log("ControlPanel handleWidgetGroupChange activeGroups=" + JSON.stringify( activeGroups ) + " panelJSON = " + panelJSON );
  };

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
    //console.log( "ControlPanel setupPanel panelJSONString=" + panelJSONString );
  
    try {
      const parsedJSON = JSON.parse(panelJSONString);
      setPanelJSON(parsedJSON);
      //console.log( "ControlPanel setupPanel panelJSON=" + JSON.stringify( panelJSON ) );
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
    //console.log("Updated panelJSON:", + JSON.stringify( panelJSON ));
    handleWidgetGroupChange();
  }, [ panelJSON ] );

  // Create list of widget groups from panelJSON
  
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
      {activeGroups.map((group) => (
        // Passing handleWidgetGroupChange callback to each WidgetGroup component
        <WidgetGroup key={group.name} panelSize={panelSize} json={group} onChange={handleWidgetGroupChange} />
      ))}
    </Paper>
  );
}

export default ControlPanel; 
