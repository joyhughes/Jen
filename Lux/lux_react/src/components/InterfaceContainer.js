import Box from "@mui/material/Box";
import React, { useEffect, useState } from "react";
import ControlPanel from "./ControlPanel";
import ImagePort from "./ImagePort";

function InterfaceContainer({ ratio, panelSize }) {
  const [isRowDirection, setIsRowDirection] = useState(true);

  const resizeBox = () => {
    let windowRatio, isRowDirection;

    windowRatio = window.innerWidth / window.innerHeight;
    if (windowRatio > ratio) {
      isRowDirection = true;
    } else {
      isRowDirection = false;
    }

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
      <ImagePort ratio={ratio} panelSize={panelSize} />

      <ControlPanel ratio={ratio} panelSize={panelSize} />
    </Box>
  );
}

export default InterfaceContainer;
