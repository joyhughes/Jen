import {Box} from "@mui/material";
import React, { useEffect, useState, useCallback } from "react";
import ControlPanel from "./ControlPanel";
import ImagePort from "./ImagePort";

function InterfaceContainer({ panelSize }) {
  const [moduleReady, setModuleReady] = useState(false);
  const [isRowDirection, setIsRowDirection] = useState(true);
  const [imagePortDimensions, setImagePortDimensions] = useState({ width: 0, height: 0 });
  const [controlPanelDimensions, setControlPanelDimensions] = useState({ width: 0, height: 0 });

  const resizeBox = useCallback(() => {
    let windowRatio, isRowDirection;
    let availableWidth, availableHeight;
    let imagePortWidth, imagePortHeight;
    let controlPanelWidth, controlPanelHeight;
    let bufWidth, bufHeight;

    if (window.module) {
      bufWidth = window.module.get_buf_width();
      bufHeight = window.module.get_buf_height();
    } else {
      bufWidth = 512;
      bufHeight = 512;
    }
    let ratio = bufWidth / bufHeight;

    windowRatio = window.innerWidth / window.innerHeight;

    if (windowRatio > ratio) { // panel on right
      isRowDirection = false;

      // Calculate dimensions for the control panel
      controlPanelWidth = parseInt(panelSize, 10);
      controlPanelHeight = window.innerHeight;

      // Use all remaining width for the image
      availableWidth = window.innerWidth - controlPanelWidth;
      availableHeight = window.innerHeight;

      // Maintain aspect ratio for the image
      let availableWindowRatio = availableWidth / availableHeight;
      if (availableWindowRatio > ratio) {
        imagePortHeight = availableHeight;
        imagePortWidth = imagePortHeight * ratio;
      } else {
        imagePortWidth = availableWidth;
        imagePortHeight = imagePortWidth / ratio;
      }
    } else { // panel on bottom
      isRowDirection = true;

      // Calculate dimensions for the control panel
      controlPanelHeight = parseInt(panelSize, 10);
      controlPanelWidth = window.innerWidth;

      // Use all remaining height for the image
      availableWidth = window.innerWidth;
      availableHeight = window.innerHeight - controlPanelHeight;

      // Maintain aspect ratio for the image
      let availableWindowRatio = availableWidth / availableHeight;
      if (availableWindowRatio > ratio) {
        imagePortHeight = availableHeight;
        imagePortWidth = imagePortHeight * ratio;
      } else {
        imagePortWidth = availableWidth;
        imagePortHeight = imagePortWidth / ratio;
      }
    }

    setImagePortDimensions({ width: imagePortWidth, height: imagePortHeight });
    setControlPanelDimensions({ width: controlPanelWidth, height: controlPanelHeight });
    setIsRowDirection(isRowDirection);
  }, [moduleReady, panelSize]);

  // Accesses C++ module to get callback for resizing image
  useEffect(() => {
    console.log( "InterfaceContainer useEffect" );
    if (window.module) {
      window.module.set_resize_callback(resizeBox);
      setModuleReady(true);
    } else {
      // Poll for the Module to be ready
      const intervalId = setInterval(() => {
        if (window.module) {
          clearInterval(intervalId);
          window.module.set_resize_callback(resizeBox);
          setModuleReady(true);
        }
      }, 100); // Check every 100ms

      return () => clearInterval(intervalId);
    }
  }, [resizeBox]);

  // resize container
  useEffect(() => {
    window.addEventListener("resize", resizeBox);
    resizeBox();
    return () => window.removeEventListener("resize", resizeBox);
  }, [panelSize]);
/*
  return (
    <Box
      sx={{
        display: "flex",
        flexDirection: "column",
        flexWrap: "wrap",
        width: "100vw",
        height: "100vh",
        top: 0,
        left: 0,
        justifyContent: "center",
      }}
    >
      
      <ImagePort dimensions={imagePortDimensions} />

      <ControlPanel dimensions={controlPanelDimensions} panelSize={panelSize} />

    </Box>
  );
*/
return (
  <Box
    sx={{
      display: "flex",
      flexDirection: isRowDirection ? "row" : "column",
      flexWrap: "wrap",
      width: "100vw",
      height: "100vh",
      top: 0,
      left: 0,
      justifyContent: "center",
    }}
  >
    
    <ImagePort dimensions={imagePortDimensions} />

    <ControlPanel dimensions={controlPanelDimensions} panelSize={panelSize} />

  </Box>
);

}

export default InterfaceContainer;
