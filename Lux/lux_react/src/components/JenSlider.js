import React, { useState, useEffect } from "react";
import Paper from '@mui/material/Paper';

import Slider from '@mui/material/Slider';
import MuiInput from '@mui/material/Input';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Tooltip from '@mui/material/Tooltip';

import { useTheme } from '@mui/material/styles';
import { styled } from '@mui/system';

const Input = styled(MuiInput)`
  width: 50px;
`;

// Future: Slider props for logarithmic scale etc.

function JenSlider( { sliderName, panelSize } ) {  // sliderName is name of slider in scene file
    const theme = useTheme();

    const [sliderProps, setSliderProps] = useState({
      label: '',
      description: '',
      value: 0,
      min: 0,
      max: 100,
      step: 1,
      ready: false
    });
    
    useEffect(() => {
        if (window.Module) {
          setSliderProps({
            label: window.Module.get_slider_label(sliderName),
            description: window.Module.get_slider_description(sliderName),
            value: window.Module.get_slider_value(sliderName),
            min:   window.Module.get_slider_min(sliderName),
            max:   window.Module.get_slider_max(sliderName),
            step:  window.Module.get_slider_step(sliderName),
            ready: true
          });
          //console.log( "JenSlider useEffect mainSlider=" + JSON.stringify( sliderProps ) );
        } else {
          // Poll for the Module to be ready
          const intervalId = setInterval(() => {
            if (window.Module) {
              setSliderProps({
                label: window.Module.get_slider_label(sliderName),
                description: window.Module.get_slider_description(sliderName),
                value: window.Module.get_slider_value(sliderName),
                min:   window.Module.get_slider_min(sliderName),
                max:   window.Module.get_slider_max(sliderName),
                step:  window.Module.get_slider_step(sliderName),
                ready: true
              });
              //console.log( "JenSlider useEffect mainSlider=" + JSON.stringify( sliderProps ) );
    
              clearInterval(intervalId);
            }
          }, 100); // Check every 100ms
    
          return () => clearInterval(intervalId);
        }
    }, [] );

    const handleSliderChange = (event, newValue) => {
        //console.log( "JenSlider handleSliderChange newValue=" + newValue );
        setSliderProps(prevProps => ({
          ...prevProps,
          value: newValue
        }));
      
        if (window.Module) {
          window.Module.slider_value(newValue);
        }
      };
    
    // Handle input box to right of slider. Value constrained to slider min and max.
    const handleInputChange = (event) => {
        let newValue = (event.target.value === '' ? 0 : Number(event.target.value));
        if( newValue < sliderProps.min ) newValue = sliderProps.min;
        if( newValue > sliderProps.max ) newValue = sliderProps.max;
        //console.log( "JenSlider handleInputChange value=" + newValue );
        setSliderProps(prevProps => ({
          ...prevProps,
          value: newValue
        }));
        if (window.Module) {
          window.Module.slider_value(newValue);
        }
    };
    
    // Needed?
    const handleInputBlur = () => {
        if (sliderProps.value < sliderProps.min) {
            setSliderProps(prevProps => ({
                ...prevProps,
                value: sliderProps.min
            }));
        } else if (sliderProps.value > sliderProps.max) {
            setSliderProps(prevProps => ({
                ...prevProps,
                value: sliderProps.max
            }));
        }
    };
    
    return (
        <Paper 
        elevation={3}
        sx={{
          display: 'flex',
          justifyContent: 'center',
          alignItems: 'center',
          width: panelSize,
          height: 60
        }}
      >
        <Stack spacing={0} direction="column" alignItems="center">
          {sliderProps.label && (
            <Tooltip title={sliderProps.description}>
              <Typography style={{ textAlign: 'center', color: theme.palette.primary.main }}>
                {sliderProps.label}
              </Typography>
            </Tooltip>
          )}
          <Stack spacing={1} direction="row" alignItems="center">
            <Slider 
              size="small"
              style={{ width: panelSize - 80 }}
              min={sliderProps.min}
              max={sliderProps.max}
              step={sliderProps.step}
              value={sliderProps.value}
              onChange={handleSliderChange}
              aria-labelledby='input-slider'
              aria-label={sliderProps.label}
              valueLabelDisplay="off" // Shows the value label on hover or focus
            />
            <Input
              size="small"
              type="number"
              min={sliderProps.min}
              max={sliderProps.max}
              step={sliderProps.step}
              value={sliderProps.value}
              onChange={handleInputChange}
              onBlur={handleInputBlur}
              aria-labelledby="input-slider"
              aria-label="{sliderProps.label} input field"
            />
          </Stack>
        </Stack>
      </Paper>
    );
}

export default JenSlider;