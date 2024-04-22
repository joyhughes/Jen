
import './App.css';
//import './emscripten/lux.js';
import { BrowserRouter, Routes, Route } from 'react-router-dom';
//import Home from "./pages/Home";
//import Tour from "./pages/Tour";
//import SearchAppBar from './components/AppBar';
import Home from "./pages/Home";

/* Tour 
function App() {
  return <BrowserRouter>
    <SearchAppBar />
    <Routes>
      <Route path="/" element={<Home/>} />
     <Route path="/:id" element={<Tour/>} />
    </Routes>
  </BrowserRouter>;
}
*/

// Jen prototype
function App() {
  return <BrowserRouter>
    <Routes>
      <Route path="/" element={<Home/>} />
    </Routes>
  </BrowserRouter>;
}

export default App;
/*
import React, { useState, useEffect } from "react";
import { Container, Box } from "@mui/material";
import { css } from "@emotion/react";

const ImageBox = ({ ratio, box2Size }) => {  
  const [dimensions, setDimensions] = useState({ width: 0, height: 0 });

  const resizeBox = () => {
    let windowRatio, width, height, availableWidth, availableHeight;

    windowRatio = window.innerWidth / window.innerHeight;
    if( windowRatio > ratio ) {
      availableWidth = window.innerWidth - parseInt(box2Size, 10);
      availableHeight = window.innerHeight;
    }
    else {
      availableWidth = window.innerWidth;
      availableHeight = window.innerHeight - parseInt(box2Size, 10);
    }

    windowRatio = availableWidth / availableHeight;
    if ( windowRatio > ratio ) {
      height = availableHeight; // Assuming container height is 80% of the window
      width = height * ratio;
    } else {
      width = availableWidth; // Assuming container width is 80% of the window
      height = width / ratio;
    }

    setDimensions({ width, height });
  };

  useEffect(() => {
    window.addEventListener("resize", resizeBox);
    resizeBox();
    return () => window.removeEventListener("resize", resizeBox);
  }, [ ratio, box2Size ]);

  return (
    <Box
      sx={{
        width: `${dimensions.width}px`,
        height: `${dimensions.height}px`,
        backgroundColor: "lightblue",
        margin: "auto",
      }}
    />
  );
};

const App = () => {
  const imgWidth = 1600; // Replace with your image width
  const imgHeight = 900; // Replace with your image height
  const ratio = imgWidth / imgHeight;

  const flexDirection =
    window.innerWidth / window.innerHeight < ratio ? "row" : "column";
  const box2Size = flexDirection === "column" ? "100px" : "200px";

  const box2Style = css`
    flex-shrink: 0;
    width: ${flexDirection === "column" ? "100%" : box2Size};
    height: ${flexDirection === "column" ? box2Size : "100%"};
    background-color: lightgreen;
  `;

  return (
    <>
        <ImageBox ratio={ratio} box2Size={box2Size} />
        <Box css={box2Style}></Box>
    </>
  );
};

export default App;
*/
/*
      <Box
        display="flex"
        flexDirection={flexDirection}
        height="100vh"
        alignItems="center"
        justifyContent="center"
      >
*/