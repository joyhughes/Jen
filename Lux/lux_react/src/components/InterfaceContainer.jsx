import {Box} from "@mui/material";
import React, { useEffect, useState, useCallback } from "react";
import ControlPanel from "./ControlPanel";
import ImagePort from "./ImagePort";

function InterfaceContainer({ panelSize }) {
  const [moduleReady, setModuleReady] = useState(false);
  const [isRowDirection, setIsRowDirection] = useState(true);
  const [imagePortDimensions, setImagePortDimensions] = useState({ width: 0, height: 0 });
  const [controlPanelDimensions, setControlPanelDimensions] = useState({ width: 0, height: 0 });
  const [activePane, setActivePane] = useState("home");

  // Calculate maximum panel width to prevent it from taking over the screen
  const getMaxPanelWidth = () => {
    // Limit control panel to at most 30% of screen width on desktop
    // and 100% when below the image (on mobile)
    return isRowDirection ? window.innerWidth : Math.min(panelSize, window.innerWidth * 0.3);
  };

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

    // Determine layout direction based on screen aspect ratio
    if (windowRatio > 1.2) { // Wider screens - panel on right
      isRowDirection = false;

      // Limit control panel width
      const maxPanelWidth = Math.min(panelSize, window.innerWidth * 0.3);

      // Calculate available space for image port
      availableWidth = window.innerWidth - maxPanelWidth;
      availableHeight = window.innerHeight;

      // Calculate image port dimensions to maintain aspect ratio
      if ((availableWidth / availableHeight) > ratio) {
        // Height constrained
        imagePortHeight = availableHeight;
        imagePortWidth = imagePortHeight * ratio;
      } else {
        // Width constrained
        imagePortWidth = availableWidth;
        imagePortHeight = imagePortWidth / ratio;
      }

      // Set control panel dimensions
      controlPanelWidth = maxPanelWidth;
      controlPanelHeight = window.innerHeight;
    } else { // Taller/square screens - panel on bottom
      isRowDirection = true;

      // Limit control panel height
      const maxPanelHeight = Math.min(panelSize, window.innerHeight * 0.4);

      // Calculate available space for image port
      availableWidth = window.innerWidth;
      availableHeight = window.innerHeight - maxPanelHeight;

      // Calculate image port dimensions to maintain aspect ratio
      if ((availableWidth / availableHeight) > ratio) {
        // Height constrained
        imagePortHeight = availableHeight;
        imagePortWidth = imagePortHeight * ratio;
      } else {
        // Width constrained
        imagePortWidth = availableWidth;
        imagePortHeight = imagePortWidth / ratio;
      }

      // Set control panel dimensions
      controlPanelWidth = window.innerWidth;
      controlPanelHeight = maxPanelHeight;
    }

    // Update state with calculated dimensions
    setImagePortDimensions({ width: imagePortWidth, height: imagePortHeight });
    setControlPanelDimensions({ width: controlPanelWidth, height: controlPanelHeight });
    setIsRowDirection(isRowDirection);
  }, [panelSize]);

  // Set up resize callback when module is ready
  useEffect(() => {
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
      }, 100);

      return () => clearInterval(intervalId);
    }
  }, [resizeBox]);

  // Handle window resize events
  useEffect(() => {
    window.addEventListener("resize", resizeBox);
    resizeBox();
    return () => window.removeEventListener("resize", resizeBox);
  }, [panelSize, resizeBox]);

  // Handle pane changes from the ControlPanel
  const handlePaneChange = (pane) => {
    setActivePane(pane);
    // Trigger resize when changing panes to ensure proper layout
    setTimeout(resizeBox, 0);
  };

  return (
      <Box
          sx={{
            display: "flex",
            flexDirection: isRowDirection ? "column" : "row",
            width: "100vw",
            height: "100vh",
            top: 0,
            left: 0,
            justifyContent: "center",
            alignItems: "center",
            overflow: "hidden",
            background: "#121212"
          }}
      >
        <Box
            sx={{
              display: "flex",
              justifyContent: "center",
              alignItems: "center",
              width: isRowDirection ? "100%" : "auto",
              height: isRowDirection ? "auto" : "100%"
            }}
        >
          <ImagePort dimensions={imagePortDimensions} />
        </Box>

        <ControlPanel
            dimensions={controlPanelDimensions}
            panelSize={panelSize}
            activePane={activePane}
            onPaneChange={handlePaneChange}
        />
      </Box>
  );
}

export default InterfaceContainer;