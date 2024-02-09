import React, { useState } from "react";
import Slider from '@mui/material/Slider';
import MuiInput from '@mui/material/Input';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';

import { useTheme } from '@mui/material/styles';
import { styled } from '@mui/system';

import WidgetContainer from "./WidgetContainer";

const Input = styled(MuiInput)`
  width: 50px;
`;

function JenSlider({ json, width }) {
    const theme = useTheme();
    const [value, setValue] = useState(json.default_value || json.min || 0);

    const handleSliderChange = (event, newValue) => {
        setValue(newValue);
        window.Module.set_slider_value(json.name, newValue);
    };

    const handleInputChange = (event) => {
        let newValue = event.target.value === '' ? 0 : Number(event.target.value);
        setValue(newValue);
        window.Module.set_slider_value(json.name, newValue);
    };

    const handleInputBlur = () => {
        if (value < json.min) {
            setValue(json.min);
        } else if (value > json.max) {
            setValue(json.max);
        }
    };

    return (
      <Stack spacing={-0.5} direction="column" alignItems="center">
          <Typography style={{ textAlign: 'center', color: theme.palette.primary.main }}>
              {json.label}
          </Typography>
          <Stack spacing={1} direction="row" alignItems="center">
              <Slider
                  size="small"
                  style={{ width: width }}
                  min={json.min}
                  max={json.max}
                  step={json.step}
                  value={value}
                  onChange={handleSliderChange}
                  aria-labelledby='input-slider'
                  aria-label={json.label}
                  valueLabelDisplay="off"
              />
              <Input
                  size="small"
                  type="number"
                  min={json.min}
                  max={json.max}
                  step={json.step}
                  value={value}
                  onChange={handleInputChange}
                  onBlur={handleInputBlur}
                  aria-labelledby="input-slider"
                  aria-label={`${json.label} input field`}
              />
          </Stack>
      </Stack>
    );
}

export default JenSlider;


/*
import React, { useState, useEffect } from "react";
import Slider from '@mui/material/Slider';
import MuiInput from '@mui/material/Input';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';

import { useTheme } from '@mui/material/styles';
import { styled } from '@mui/system';

import WidgetContainer from "./WidgetContainer";

const Input = styled(MuiInput)`
  width: 50px;
`;

// Future: Slider props for logarithmic scale etc.

function JenSlider( { sliderName, width } ) {  // sliderName is name of slider in scene file
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
    const [isRange, setIsRange] = useState(false);  // Is this a range slider? [false, true]
    
    useEffect(() => {
      if (window.Module) {
        if( window.Module.isRangeSlider( sliderName ) ) setIsRange(true);
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
            if( window.Module.isRangeSlider( sliderName ) ) setIsRange(true);
            setSliderProps({
              label: window.Module.get_slider_label(sliderName),
              description: window.Module.get_slider_description(sliderName),
              value: isRange ?  [ window.Module.get_range_min(sliderName), window.Module.get_range_max(sliderName) ] : window.Module.get_slider_value(sliderName),
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
        if( isRange ) window.Module.set_range_slider_value(newValue[0], newValue[1]);
        else window.Module.set_slider_value(newValue);
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
    <Stack spacing={-0.5} direction="column" alignItems="center">
      <Typography style={{ textAlign: 'center', color: theme.palette.primary.main }}>
        {sliderProps.label}
      </Typography>
      <Stack spacing={1} direction="row" alignItems="center">
        <Slider 
          size="small"
          style={{ width: width }}
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
  );
}

export default JenSlider; */