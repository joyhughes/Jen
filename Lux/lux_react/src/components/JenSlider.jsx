import React, { useState, useEffect } from "react";
import {
  Slider,
  Input,
  Typography,
  Stack,
  Tooltip,
  Paper,
  Box,
  Fade
} from '@mui/material';
import { styled, useTheme } from '@mui/material/styles';
import { Info } from 'lucide-react';

// Styled Input component for number field
const StyledInput = styled(Input)(({ theme }) => ({
  width: 50,
  fontSize: '0.875rem',
  '& input': {
    padding: '4px 6px',
    textAlign: 'center',
  },
  '&.Mui-focused': {
    boxShadow: `0 0 0 2px ${theme.palette.primary.main}`,
    borderRadius: theme.shape.borderRadius,
  },
}));

// Info icon with tooltip
const InfoTooltip = ({ description }) => {
  if (!description) return null;

  return (
    <Tooltip
      title={description}
      arrow
      placement="top"
    >
      <Box
        component="span"
        sx={{
          display: 'inline-flex',
          ml: 0.75,
          color: 'text.secondary',
          cursor: 'help'
        }}
      >
        <Info size={14} />
      </Box>
    </Tooltip>
  );
};

// Value indicator for slider
const SliderValueIndicator = ({ value, visible, min, max }) => {
  const theme = useTheme();

  // Calculate position as percentage of slider width
  const position = ((value - min) / (max - min)) * 100;

  return (
    <Fade in={visible}>
      <Box
        sx={{
          position: 'absolute',
          bottom: '100%',
          left: `${position}%`,
          transform: 'translateX(-50%)',
          marginBottom: '4px',
          backgroundColor: theme.palette.primary.main,
          color: 'white',
          padding: '2px 6px',
          borderRadius: '4px',
          fontSize: '0.7rem',
          fontWeight: 'bold',
          pointerEvents: 'none',
          zIndex: 10,
          boxShadow: '0 1px 2px rgba(0,0,0,0.2)',
          '&::after': {
            content: '""',
            position: 'absolute',
            top: '100%',
            left: '50%',
            transform: 'translateX(-50%)',
            width: 0,
            height: 0,
            borderLeft: '4px solid transparent',
            borderRight: '4px solid transparent',
            borderTop: `4px solid ${theme.palette.primary.main}`,
          }
        }}
      >
        {value}
      </Box>
    </Fade>
  );
};

function JenSlider({ json, width }) {
  const theme = useTheme();
  const [value, setValue] = useState(json.value ?? json.min ?? 0);
  const [minFocus, setMinFocus] = useState(true);
  const [showTooltip, setShowTooltip] = useState(false);
  const isRangeSlider = json.type === 'range_slider_int' || json.type === 'range_slider_float';

  // Make sure we have the latest value from props
  useEffect(() => {
    setValue(json.value ?? json.min ?? 0);
  }, [json.value, json.min]);

  const handleSliderChange = (event, newValue) => {
    if (isRangeSlider) {
      if (value[0] !== newValue[0]) setMinFocus(true);
      else if (value[1] !== newValue[1]) setMinFocus(false);

      window.module.set_range_slider_value(json.name, newValue[0], newValue[1]);
    } else {
      window.module.set_slider_value(json.name, newValue);
    }

    setValue(newValue);
  };

  const handleInputChange = (event) => {
    let newValue = event.target.value === '' ? 0 : Number(event.target.value);

    if (isRangeSlider) {
      if (minFocus) {
        setValue([newValue, value[1]]);
        window.module.set_range_slider_value(json.name, newValue, value[1]);
      } else {
        setValue([value[0], newValue]);
        window.module.set_range_slider_value(json.name, value[0], newValue);
      }
    } else {
      setValue(newValue);
      window.module.set_slider_value(json.name, newValue);
    }
  };

  const handleInputBlur = () => {
    if (isRangeSlider) {
      // Clamp values for range slider
      let newMinValue = Math.max(json.min, Math.min(value[0], json.max));
      let newMaxValue = Math.max(json.min, Math.min(value[1], json.max));

      // Ensure min <= max
      if (newMinValue > newMaxValue) {
        if (minFocus) {
          newMinValue = newMaxValue;
        } else {
          newMaxValue = newMinValue;
        }
      }

      setValue([newMinValue, newMaxValue]);
      window.module.set_range_slider_value(json.name, newMinValue, newMaxValue);
    } else {
      // Clamp value for regular slider
      const newValue = Math.max(json.min, Math.min(value, json.max));
      setValue(newValue);
      window.module.set_slider_value(json.name, newValue);
    }
  };

  // Handle keyboard accessibility
  const handleKeyDown = (event) => {
    if (event.key === 'ArrowUp' || event.key === 'ArrowRight') {
      event.preventDefault();

      if (isRangeSlider) {
        const step = json.step || 1;
        if (minFocus) {
          const newValue = Math.min(value[0] + step, value[1]);
          setValue([newValue, value[1]]);
          window.module.set_range_slider_value(json.name, newValue, value[1]);
        } else {
          const newValue = Math.min(value[1] + step, json.max);
          setValue([value[0], newValue]);
          window.module.set_range_slider_value(json.name, value[0], newValue);
        }
      } else {
        const newValue = Math.min(value + (json.step || 1), json.max);
        setValue(newValue);
        window.module.set_slider_value(json.name, newValue);
      }
    } else if (event.key === 'ArrowDown' || event.key === 'ArrowLeft') {
      event.preventDefault();

      if (isRangeSlider) {
        const step = json.step || 1;
        if (minFocus) {
          const newValue = Math.max(value[0] - step, json.min);
          setValue([newValue, value[1]]);
          window.module.set_range_slider_value(json.name, newValue, value[1]);
        } else {
          const newValue = Math.max(value[1] - step, value[0]);
          setValue([value[0], newValue]);
          window.module.set_range_slider_value(json.name, value[0], newValue);
        }
      } else {
        const newValue = Math.max(value - (json.step || 1), json.min);
        setValue(newValue);
        window.module.set_slider_value(json.name, newValue);
      }
    }
  };

  return (
    <Paper
      variant="outlined"
      sx={{
        p: 1.5,
        pt: 1,
        borderRadius: 1,
        backgroundColor: theme.palette.background.paper,
        borderColor: theme.palette.divider,
        width: width ? width + 16 : '100%',
        '&:hover': {
          borderColor: theme.palette.action.active,
        },
      }}
    >
      <Box sx={{ mb: 1.5, display: 'flex', alignItems: 'center' }}>
        <Typography variant="body2" fontWeight={500}>
          {json.label || json.name}
        </Typography>
        <InfoTooltip description={json.description} />
      </Box>

      <Stack spacing={1} direction="row" alignItems="center" sx={{ position: 'relative' }}>
        <Box sx={{ position: 'relative', width: '100%' }}>
          <Slider
            size="small"
            min={json.min}
            max={json.max}
            step={json.step || 1}
            value={value}
            onChange={handleSliderChange}
            valueLabelDisplay="off"
            onMouseEnter={() => setShowTooltip(true)}
            onMouseLeave={() => setShowTooltip(false)}
            sx={{
              mx: 0.5,
              '& .MuiSlider-thumb': {
                width: 14,
                height: 14,
                '&:hover, &.Mui-focusVisible': {
                  boxShadow: `0 0 0 8px ${theme.palette.primary.main}30`,
                },
              },
            }}
          />

          {!isRangeSlider && (
            <SliderValueIndicator
              value={value}
              visible={showTooltip}
              min={json.min}
              max={json.max}
            />
          )}
        </Box>

        <StyledInput
          margin="none"
          size="small"
          type="number"
          min={json.min}
          max={json.max}
          step={json.step || 1}
          value={isRangeSlider ? (minFocus ? value[0] : value[1]) : value}
          onChange={handleInputChange}
          onBlur={handleInputBlur}
          onKeyDown={handleKeyDown}
          inputProps={{
            step: json.step || 1,
            min: json.min,
            max: json.max,
            type: 'number',
            'aria-labelledby': 'input-slider',
          }}
        />
      </Stack>

      {isRangeSlider && (
        <Box sx={{ mt: 0.5, display: 'flex', justifyContent: 'space-between' }}>
          <Typography variant="caption" color="text.secondary">
            Min: {value[0]}
          </Typography>
          <Typography variant="caption" color="text.secondary">
            Max: {value[1]}
          </Typography>
        </Box>
      )}
    </Paper>
  );
}

export default JenSlider;