import React, { useState, useEffect } from "react";
import {
    Slider,
    Input,
    Stack,
    Box,
    Typography,
    useTheme
} from '@mui/material';
import { styled } from '@mui/system';

// Styled input for better aesthetics
const StyledInput = styled(Input)(({ theme }) => ({
    width: 50,
    height: 32,
    padding: '0px 8px',
    fontSize: '0.875rem',
    fontFamily: 'monospace',
    borderRadius: theme.shape.borderRadius,
    backgroundColor: theme.palette.action.hover,
    transition: theme.transitions.create(['background-color', 'box-shadow']),
    '&:hover': {
        backgroundColor: theme.palette.action.selected,
    },
    '&.Mui-focused': {
        backgroundColor: theme.palette.action.selected,
        boxShadow: `0 0 0 1px ${theme.palette.primary.main}`,
    },
    '& input': {
        padding: '6px 0',
        textAlign: 'center',
    },
    // For number inputs
    '& input[type=number]': {
        MozAppearance: 'textfield', // Firefox
    },
    '& input::-webkit-outer-spin-button, & input::-webkit-inner-spin-button': {
        WebkitAppearance: 'none',
        margin: 0,
    },
}));

// Custom slider style
const StyledSlider = styled(Slider)(({ theme }) => ({
    height: 6,
    padding: '13px 0',
    '& .MuiSlider-thumb': {
        height: 16,
        width: 16,
        backgroundColor: theme.palette.common.white,
        border: `2px solid ${theme.palette.primary.main}`,
        boxShadow: 'none',
        '&:hover, &.Mui-focusVisible': {
            boxShadow: `0 0 0 4px ${theme.palette.primary.main}20`,
        },
        '&.Mui-active': {
            boxShadow: `0 0 0 8px ${theme.palette.primary.main}30`,
        },
    },
    '& .MuiSlider-rail': {
        height: 6,
        borderRadius: 3,
        backgroundColor: theme.palette.action.hover,
    },
    '& .MuiSlider-track': {
        height: 6,
        borderRadius: 3,
    },
    '& .MuiSlider-valueLabel': {
        fontSize: '0.75rem',
        fontWeight: 500,
        background: 'unset',
        backgroundColor: theme.palette.primary.main,
        transformOrigin: 'bottom',
        transform: 'translate(50%, -30px) scale(0)',
        '&:before': { display: 'none' },
        '&.MuiSlider-valueLabelOpen': {
            transform: 'translate(50%, -30px) scale(1)',
        },
    },
}));

function JenSlider({ json }) {
    const theme = useTheme();
    const [value, setValue] = useState(json.value ?? json.min ?? 0);
    const [minFocus, setMinFocus] = useState(true);
    const [isDragging, setIsDragging] = useState(false);

    // Determine if slider is a range type
    const isRange = json.type === 'range_slider_int' || json.type === 'range_slider_float';

    // Format value for display based on type
    const formatDisplayValue = (val) => {
        if (json.type === 'slider_int' || json.type === 'range_slider_int') {
            return parseInt(val);
        } else {
            return parseFloat(val).toFixed(2);
        }
    };

    // Format value for module based on type
    const formatModuleValue = (val) => {
        if (json.type === 'slider_int' || json.type === 'range_slider_int') {
            return parseInt(val);
        } else {
            return parseFloat(val);
        }
    };

    const handleSliderChange = (event, newValue) => {
        if (isRange) {
            if (value[0] !== newValue[0]) setMinFocus(true);
            else if (value[1] !== newValue[1]) setMinFocus(false);
            window.module.set_range_slider_value(
                json.name,
                formatModuleValue(newValue[0]),
                formatModuleValue(newValue[1])
            );
        } else {
            window.module.set_slider_value(json.name, formatModuleValue(newValue));
        }
        setValue(newValue);
    };

    const handleInputChange = (event) => {
        let newValue = event.target.value === '' ? 0 : Number(event.target.value);

        // Clamp to min/max values
        newValue = Math.max(json.min, Math.min(json.max, newValue));

        if (isRange) {
            const rangeValue = minFocus ? [newValue, value[1]] : [value[0], newValue];
            setValue(rangeValue);
            window.module.set_range_slider_value(
                json.name,
                formatModuleValue(rangeValue[0]),
                formatModuleValue(rangeValue[1])
            );
        } else {
            setValue(newValue);
            window.module.set_slider_value(json.name, formatModuleValue(newValue));
        }
    };

    const handleMouseDown = () => setIsDragging(true);
    const handleMouseUp = () => setIsDragging(false);

    useEffect(() => {
        if (isDragging) {
            // Add global listeners to track dragging state
            document.addEventListener('mouseup', handleMouseUp);
            document.addEventListener('touchend', handleMouseUp);

            return () => {
                document.removeEventListener('mouseup', handleMouseUp);
                document.removeEventListener('touchend', handleMouseUp);
            };
        }
    }, [isDragging]);

    return (
        <Box
            sx={{
                width: '100%',
                px: 0.5
            }}
        >
            <Stack
                direction="row"
                spacing={2}
                alignItems="center"
                sx={{
                    width: '100%',
                    mb: isRange ? 1 : 0
                }}
            >
                <StyledSlider
                    size="small"
                    min={json.min}
                    max={json.max}
                    step={json.step}
                    value={value}
                    onChange={handleSliderChange}
                    valueLabelDisplay={isDragging ? "auto" : "off"}
                    onMouseDown={handleMouseDown}
                    onTouchStart={handleMouseDown}
                    marks={json.markers ? [
                        { value: json.min, label: formatDisplayValue(json.min) },
                        { value: json.max, label: formatDisplayValue(json.max) }
                    ] : undefined}
                    sx={{ flexGrow: 1 }}
                />

                <StyledInput
                    value={isRange ? formatDisplayValue(minFocus ? value[0] : value[1]) : formatDisplayValue(value)}
                    onChange={handleInputChange}
                    type="number"
                    inputProps={{
                        min: json.min,
                        max: json.max,
                        step: json.step,
                        'aria-label': 'slider value',
                    }}
                    disableUnderline
                />
            </Stack>

            {isRange && (
                <Box
                    sx={{
                        display: 'flex',
                        justifyContent: 'space-between',
                        px: 1,
                        mt: -0.5
                    }}
                >
                    <Typography
                        variant="caption"
                        sx={{
                            color: minFocus ? theme.palette.primary.main : theme.palette.text.secondary,
                            fontWeight: minFocus ? 600 : 400,
                            cursor: 'pointer',
                            transition: 'color 0.2s',
                            '&:hover': {
                                color: theme.palette.primary.main
                            }
                        }}
                        onClick={() => setMinFocus(true)}
                    >
                        Min: {formatDisplayValue(value[0])}
                    </Typography>

                    <Typography
                        variant="caption"
                        sx={{
                            color: !minFocus ? theme.palette.primary.main : theme.palette.text.secondary,
                            fontWeight: !minFocus ? 600 : 400,
                            cursor: 'pointer',
                            transition: 'color 0.2s',
                            '&:hover': {
                                color: theme.palette.primary.main
                            }
                        }}
                        onClick={() => setMinFocus(false)}
                    >
                        Max: {formatDisplayValue(value[1])}
                    </Typography>
                </Box>
            )}
        </Box>
    );
}

export default JenSlider;