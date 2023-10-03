import React, { useState, useEffect } from "react";
import Paper from '@mui/material/Paper';
import MediaController from './MediaController';

import Slider from '@mui/material/Slider';
import Radio from '@mui/material/Radio';
import RadioGroup from '@mui/material/RadioGroup';
import FormControlLabel from '@mui/material/FormControlLabel';
import Grid from '@mui/material/Grid';
import TextField from '@mui/material/TextField';
import Box from '@mui/material/Box';

function ControlPanel( { ratio, panelSize } ) {
  const [sliderValue, setSliderValue] = useState(30);
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


  const handleSliderChange = (event, newValue) => {
    setSliderValue(newValue);
  };

  const handleRadioChange = (event) => {
    setRadioValue(event.target.value);
  };

  const handleTextChange = (event) => {
    setTextValue(event.target.value);
  };

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
      <Paper 
        elevation={3}
        sx={{
          display: 'flex',
          justifyContent: 'center',
          alignItems: 'center',
          width: panelSize,
          height: 60,  // Or any desired height for the enclosing container
        }}
      >
        <MediaController />
      </Paper>
      <Paper 
        elevation={3}
        sx={{
          display: 'flex',
          justifyContent: 'center',
          alignItems: 'center',
          width: panelSize,
          height: 60,  // Or any desired height for the enclosing container
        }}
      >
        <Slider 
          style={{ width: panelSize - 40 }}
          value={ sliderValue } 
          onChange={ handleSliderChange }
          aria-labelledby = "input-slider"
        /> 
      </Paper>
    </Paper>

  );
}

export default ControlPanel;

/*
    <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
      <Slider 
        style={{ width: 200 }}
        value={sliderValue} 
        onChange={handleSliderChange}
        aria-labelledby="input-slider" 
      />
      <RadioGroup 
        value={radioValue} 
        onChange={handleRadioChange}
        row
      >
        <Grid container spacing={1}>
          <Grid item xs={6}>
            <FormControlLabel value="option1" control={<Radio />} label="Option 1" />
          </Grid>
          <Grid item xs={6}>
            <FormControlLabel value="option2" control={<Radio />} label="Option 2" />
          </Grid>
          <Grid item xs={6}>
            <FormControlLabel value="option3" control={<Radio />} label="Option 3" />
          </Grid>
          <Grid item xs={6}>
            <FormControlLabel value="option4" control={<Radio />} label="Option 4" />
          </Grid>
        </Grid>
      </RadioGroup>

      
      <TextField 
        label="Input"
        variant="outlined"
        value={textValue}
        onChange={handleTextChange}
      />
    </Box>
      */
