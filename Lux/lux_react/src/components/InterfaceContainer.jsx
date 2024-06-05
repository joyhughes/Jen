import {Box} from "@mui/material";
import React, { useEffect, useState } from "react";
import ControlPanel from "./ControlPanel";
import ImagePort from "./ImagePort";

function InterfaceContainer({ ratio, panelSize }) {
  const [isRowDirection, setIsRowDirection] = useState(true);
  const [imagePortDimensions, setImagePortDimensions] = useState({ width: 0, height: 0 });
  const [controlPanelDimensions, setControlPanelDimensions] = useState({ width: 0, height: 0 });

  const resizeBox = () => {
    let windowRatio, isRowDirection;
    let availableWidth, availableHeight;
    let imagePortWidth, imagePortHeight;
    let controlPanelWidth, controlPanelHeight;

    windowRatio = window.innerWidth / window.innerHeight;
    if( windowRatio > ratio ) {
      availableWidth = window.innerWidth - parseInt(panelSize, 10);
      availableHeight = window.innerHeight;
      isRowDirection = true;
    }
    else {
      availableWidth = window.innerWidth;
      availableHeight = window.innerHeight - ( parseInt(panelSize, 10) / ratio );
      isRowDirection = false;
    }

    let availableWindowRatio = availableWidth / availableHeight;
    if ( availableWindowRatio > ratio ) {
      imagePortHeight = availableHeight; 
      imagePortWidth = imagePortHeight * ratio;
    } else {
      imagePortWidth = availableWidth; 
      imagePortHeight = imagePortWidth / ratio;
    }

    if( windowRatio > ratio ) {
      controlPanelWidth = window.innerWidth - imagePortWidth;
      controlPanelHeight = window.innerHeight;
    }
    else {
      controlPanelWidth = window.innerWidth;
      controlPanelHeight = window.innerHeight - imagePortHeight;
    }

    setImagePortDimensions({ width: imagePortWidth, height: imagePortHeight });
    setControlPanelDimensions({ width: controlPanelWidth, height: controlPanelHeight });
    setIsRowDirection(isRowDirection);
  };

  useEffect(() => {
    window.addEventListener("resize", resizeBox);
    resizeBox();
    return () => window.removeEventListener("resize", resizeBox);
  }, [ratio, panelSize]);

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
