import React, { useState, useEffect } from "react";
import Paper from '@mui/material/Paper';
import MediaController from './MediaController';

import Typography from '@mui/material/Typography';
import Stack from '@mui/material/Stack';
import Tooltip from '@mui/material/Tooltip';

import Slider from '@mui/material/Slider';
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import Radio from '@mui/material/Radio';
import RadioGroup from '@mui/material/RadioGroup';
import FormControlLabel from '@mui/material/FormControlLabel';
import Grid from '@mui/material/Grid';
import TextField from '@mui/material/TextField';
import {Box} from '@mui/material';

import { useTheme } from '@mui/material/styles';
import { styled } from '@mui/system';

//import Module from './useEmscripten';

function ControlPanel( { ratio, panelSize } ) {
  const theme = useTheme();

  const [sliderProps, setSliderProps] = useState({
    label: '',
    value: 0,
    min: 0,
    max: 100,
    step: 1,
    ready: false
  });

  const [radioValue, setRadioValue] = useState('option1');
  const [textValue, setTextValue] = useState('');
  const [menuChoices, setMenuChoices] = useState([]);
  const [selectedMenuChoice, setSelectedMenuChoice] = useState( '' );
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

  useEffect(() => {
    if (window.module) {
      setSliderProps({
        label: window.module.get_slider_label(),
        value: window.module.get_slider_value(),
        min:   window.module.get_slider_min(),
        max:   window.module.get_slider_max(),
        step:  window.module.get_slider_step(),
        ready: true
      });
      console.log( "ControlPanel useEffect mainSlider=" + JSON.stringify( sliderProps ) );

      const choices = window.module.get_menu_choices(); 
      console.log( "Menu choices=" + choices );
      const choicesArray = choices.split(','); // Splitting the string into an array
      setMenuChoices(choicesArray);
      setSelectedMenuChoice(window.module.get_initial_menu_choice());
    } else {
      // Poll for the Module to be ready
      const intervalId = setInterval(() => {
        if (window.module) {
          setSliderProps({
            label: window.module.get_slider_label(),
            value: window.module.get_slider_value(),
            min:   window.module.get_slider_min(),
            max:   window.module.get_slider_max(),
            step:  window.module.get_slider_step(),
            ready: true
          });
          console.log( "ControlPanel useEffect mainSlider=" + JSON.stringify( sliderProps ) );

          const choices = window.module.get_menu_choices(); 
          const choicesArray = choices.split(','); // Splitting the string into an array
          setMenuChoices(choicesArray);
          setSelectedMenuChoice(window.module.get_initial_menu_choice());

          clearInterval(intervalId);
        }
      }, 100); // Check every 100ms

      return () => clearInterval(intervalId);
    }
  }, [] );

  const handleSliderChange = (event, newValue) => {
    console.log( "ControlPanel handleSliderChange newValue=" + newValue );
    setSliderProps(prevProps => ({
      ...prevProps,
      value: newValue
    }));
  
    if (window.module) {
      window.module.slider_value(newValue);
    }
  };

  const handleMenuChange = (event) => {
    setSelectedMenuChoice(event.target.value);
    console.log( "ControlPanel handleMenuChange event.target.value=" + event.target.value );
    if (window.module) {
      window.module.handle_menu_choice(event.target.value); 
    }
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
        <Stack spacing={0} direction="column" alignItems="center">
          <Slider 
            size="small"
            style={{ width: panelSize - 40 }}
            min={sliderProps.min}
            max={sliderProps.max}
            step={sliderProps.step}
            value={sliderProps.value}
            onChange={handleSliderChange}
            aria-labelledby='input-slider'
            aria-label={sliderProps.label}
            valueLabelDisplay="auto" // Shows the value label on hover or focus
            />
          {sliderProps.label && (
            <Typography style={{ textAlign: 'center', color: theme.palette.primary.main }}>
              {sliderProps.label}
            </Typography>
          )}
        </Stack>
      </Paper>
      <Paper 
        elevation={3}
        sx={{
          display: 'flex',
          justifyContent: 'center',
          alignItems: 'center',
          width: panelSize,
          height: 60,
        }}
      >
        <Select
          value={selectedMenuChoice}
          onChange={handleMenuChange}
          displayEmpty
          inputProps={{ 'aria-label': 'Without label' }}
        >
          {menuChoices.map((choice, index) => (
            <MenuItem key={index} value={index}>
              {choice}
            </MenuItem>
          ))}
        </Select>
      </Paper>
    </Paper>
  );
  
}

export default ControlPanel; 
