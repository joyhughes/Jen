import React, { useState } from "react";
import Slider from '@mui/material/Slider';
import MuiInput from '@mui/material/Input';
import Stack from '@mui/material/Stack';
import FormControl from '@mui/material/FormControl';
import InputLabel from '@mui/material/InputLabel';

import { useTheme } from '@mui/material/styles';
import { styled } from '@mui/system';

const Input = styled(MuiInput)`
  width: 50px;
`;

function JenSlider({ json, width }) {
    const theme = useTheme();
    const [value, setValue] = useState( json.value ?? json.min ?? 0 );
    const [minFocus, setMinFocus] = useState(true);

    const handleSliderChange = (event, newValue) => {
      if( json.type === 'range_slider_int' || json.type === 'range_slider_float' ) {
        if( value[0] !== newValue[0] ) setMinFocus(true);
        else if( value[1] !== newValue[1] ) setMinFocus(false);
        window.Module.set_range_slider_value(json.name, newValue[0], newValue[1]);
      } 
      else  { 
        window.Module.set_slider_value(json.name, newValue);
      }
      setValue(newValue);
    };

    const handleInputChange = (event) => {
        let newValue = event.target.value === '' ? 0 : Number(event.target.value);
        if( json.type === 'range_slider_int' || json.type === 'range_slider_float' ) {
          if( minFocus ) setValue([newValue, value[1]]);
          else setValue([value[0], newValue]);
          window.Module.set_range_slider_value(json.name, value[0], value[1]);
        }
        else {
          setValue(newValue);
          window.Module.set_slider_value(json.name, newValue);
        }
    };

    const handleInputBlur = () => {
        if (value < json.min) {
            setValue(json.min);
        } else if (value > json.max) {
            setValue(json.max);
        }
    };

    return (
      <Stack spacing={1} direction="row" alignItems="center">
          <Slider
              size="small"
              style={{ width: width }}
              min={json.min}
              max={json.max}
              step={json.step}
              value={value}
              onChange={handleSliderChange}
              valueLabelDisplay="off"
          />
          <Input
              style={{ width: '50px', height: '30px' }}
              margin="none"
              size="small"
              type="number"
              min={json.min}
              max={json.max}
              step={json.step}
              value={(json.type === 'range_slider_int' || json.type === 'range_slider_float') ? (minFocus ? value[0] : value[1]) : value}
              onChange={handleInputChange}
              onBlur={handleInputBlur}
          />
      </Stack>
  );
}

export default JenSlider;