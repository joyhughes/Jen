import React, { useState, useEffect, useRef } from "react";
import {
    Slider,
    Input,
    Stack,
    Box,
    Typography,
    useTheme,
    IconButton,
    useMediaQuery
} from '@mui/material';
import { styled } from '@mui/system';
import { ChevronUp, ChevronDown } from 'lucide-react';

const StyledInput = styled(Input, {
    shouldForwardProp: (prop) => prop !== 'valueLength' && prop !== 'isMobile'
})(({ theme, valueLength, isMobile }) => ({
    width: isMobile
        ? `${Math.max(42, 36 + (valueLength > 2 ? (valueLength - 2) * 6 : 0))}px`
        : `${Math.max(40, 32 + (valueLength > 2 ? (valueLength - 2) * 6 : 0))}px`,
    height: isMobile ? 32 : 28,
    padding: '0px 4px',
    fontSize: isMobile ? '0.9rem' : '0.8rem',
    fontFamily: 'Roboto, Arial, sans-serif',
    borderRadius: theme.shape.borderRadius,
    backgroundColor: theme.palette.action.hover,
    transition: theme.transitions.create(['background-color', 'box-shadow', 'width']),
    '&:hover': {
        backgroundColor: theme.palette.action.selected,
    },
    '&.Mui-focused': {
        backgroundColor: theme.palette.action.selected,
        boxShadow: `0 0 0 1px ${theme.palette.primary.main}`,
    },
    '& input': {
        padding: isMobile ? '8px 0' : '4px 0',
        textAlign: 'center',
    },
    '& input[type=number]': {
        MozAppearance: 'textfield',
    },
    '& input::-webkit-outer-spin-button, & input::-webkit-inner-spin-button': {
        WebkitAppearance: 'none',
        margin: 0,
    },
}));

// Optimized slider with better size ratios
const StyledSlider = styled(Slider, {
    shouldForwardProp: (prop) => prop !== 'isMobile'
})(({ theme, isMobile }) => ({
    height: isMobile ? 6 : 4,
    padding: isMobile ? '13px 0' : '11px 0',
    '& .MuiSlider-thumb': {
        height: isMobile ? 20 : 16,
        width: isMobile ? 20 : 16,
        backgroundColor: theme.palette.common.white,
        border: `2px solid ${theme.palette.primary.main}`,
        boxShadow: 'none',
        '&:hover, &.Mui-focusVisible': {
            boxShadow: `0 0 0 4px ${theme.palette.primary.main}20`,
        },
        '&.Mui-active': {
            boxShadow: `0 0 0 6px ${theme.palette.primary.main}30`,
        },
    },
    '& .MuiSlider-rail': {
        height: isMobile ? 6 : 4,
        borderRadius: isMobile ? 3 : 2,
        backgroundColor: theme.palette.action.hover,
    },
    '& .MuiSlider-track': {
        height: isMobile ? 6 : 4,
        borderRadius: isMobile ? 3 : 2,
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

// Smaller increment/decrement buttons to save space
const ValueButton = styled(IconButton, {
    shouldForwardProp: prop => prop !== 'isMobile' && prop !== 'valueLength'
})(({ theme, isMobile }) => ({
    padding: 0,
    backgroundColor: theme.palette.action.hover,
    color: theme.palette.common.white,
    '&:hover': {
        backgroundColor: theme.palette.action.selected,
    },
    // Smaller buttons
    width: isMobile ? 28 : 24,
    height: isMobile ? 28 : 24,
    minWidth: isMobile ? 28 : 24,
    minHeight: isMobile ? 28 : 24,
}));

function JenSlider({ json, width }) {
    const theme = useTheme();
    const isMobile = useMediaQuery(theme.breakpoints.down('sm'));
    const [value, setValue] = useState(json.default_value ?? json.min ?? 0);
    const [minFocus, setMinFocus] = useState(true);
    const [isDragging, setIsDragging] = useState(false);
    const [showValueLabel, setShowValueLabel] = useState(false);
    const touchTimer = useRef(null);

    const isRange = json.type === 'range_slider_int' || json.type === 'range_slider_float';

    const formatDisplayValue = (val) => {
        if (json.type === 'slider_int' || json.type === 'range_slider_int') {
            return parseInt(val);
        } else {
            return parseFloat(val).toFixed(1);
        }
    };

    const formatModuleValue = (val) => {
        if (json.type === 'slider_int' || json.type === 'range_slider_int') {
            return parseInt(val);
        } else {
            return parseFloat(val);
        }
    };

    const getDisplayValue = () => {
        if (isRange) {
            return formatDisplayValue(minFocus ? value[0] : value[1]);
        } else {
            return formatDisplayValue(value);
        }
    };

    const getValueLength = () => {
        const displayVal = getDisplayValue();
        return displayVal ? displayVal.toString().length : 1;
    };

    const handleSliderChange = (event, newValue) => {
        if (isRange) {
            if (value[0] !== newValue[0]) setMinFocus(true);
            else if (value[1] !== newValue[1]) setMinFocus(false);

            if (window.module) {
                window.module.set_range_slider_value(
                    json.name,
                    formatModuleValue(newValue[0]),
                    formatModuleValue(newValue[1])
                );
            }
        } else {
            if (window.module) {
                window.module.set_slider_value(json.name, formatModuleValue(newValue));
            }
        }
        console.log("new value now bro: " + newValue);
        setValue(newValue);
    };

    const handleInputChange = (event) => {
        let newValue = event.target.value === '' ? 0 : Number(event.target.value);
        newValue = Math.max(json.min, Math.min(json.max, newValue));

        if (isRange) {
            const rangeValue = minFocus ? [newValue, value[1]] : [value[0], newValue];
            setValue(rangeValue);

            if (window.module) {
                window.module.set_range_slider_value(
                    json.name,
                    formatModuleValue(rangeValue[0]),
                    formatModuleValue(rangeValue[1])
                );
            }
        } else {
            setValue(newValue);

            if (window.module) {
                window.module.set_slider_value(json.name, formatModuleValue(newValue));
            }
        }
    };

    const handleIncrement = () => {
        if (isRange) {
            const currentVal = minFocus ? value[0] : value[1];
            const step = json.step || 1;
            let newVal = Math.min(json.max, currentVal + step);

            if (json.type === 'range_slider_int') {
                newVal = Math.floor(newVal);
            }

            const newValue = minFocus ? [newVal, value[1]] : [value[0], newVal];
            setValue(newValue);

            if (window.module) {
                window.module.set_range_slider_value(
                    json.name,
                    formatModuleValue(newValue[0]),
                    formatModuleValue(newValue[1])
                );
            }
        } else {
            const step = json.step || 1;
            let newVal = Math.min(json.max, value + step);

            if (json.type === 'slider_int') {
                newVal = Math.floor(newVal);
            }

            setValue(newVal);

            if (window.module) {
                window.module.set_slider_value(json.name, formatModuleValue(newVal));
            }
        }
    };

    const handleDecrement = () => {
        if (isRange) {
            const currentVal = minFocus ? value[0] : value[1];
            const step = json.step || 1;
            let newVal = Math.max(json.min, currentVal - step);

            if (json.type === 'range_slider_int') {
                newVal = Math.floor(newVal);
            }

            const newValue = minFocus ? [newVal, value[1]] : [value[0], newVal];
            setValue(newValue);

            if (window.module) {
                window.module.set_range_slider_value(
                    json.name,
                    formatModuleValue(newValue[0]),
                    formatModuleValue(newValue[1])
                );
            }
        } else {
            const step = json.step || 1;
            let newVal = Math.max(json.min, value - step);

            if (json.type === 'slider_int') {
                newVal = Math.floor(newVal);
            }

            setValue(newVal);

            if (window.module) {
                window.module.set_slider_value(json.name, formatModuleValue(newVal));
            }
        }
    };

    const handleTouchStart = () => {
        setShowValueLabel(true);
        setIsDragging(true);

        if (touchTimer.current) {
            clearTimeout(touchTimer.current);
        }
    };

    const handleTouchEnd = () => {
        touchTimer.current = setTimeout(() => {
            setShowValueLabel(false);
            setIsDragging(false);
        }, 1000);
    };

    const handleMouseDown = () => {
        setIsDragging(true);
        setShowValueLabel(true);
    };

    const handleMouseUp = () => {
        setIsDragging(false);
        setTimeout(() => {
            setShowValueLabel(false);
        }, 800);
    };

    useEffect(() => {
        if (isDragging) {
            document.addEventListener('mouseup', handleMouseUp);
            document.addEventListener('touchend', handleTouchEnd);

            return () => {
                document.removeEventListener('mouseup', handleMouseUp);
                document.removeEventListener('touchend', handleTouchEnd);
            };
        }
    }, [isDragging]);

    useEffect(() => {
        if (json) {
            setValue(json.default_value !== undefined ? json.default_value : (json.min ?? 0));
        }

        return () => {
            if (touchTimer.current) {
                clearTimeout(touchTimer.current);
            }
        };
    }, []);

    return (
        <Box
            sx={{
                width: '100%',
                // Reduced padding to save space
                px: 0.5,
                py: isMobile ? 0.5 : 0.25,
            }}
        >
            <Stack
                direction="row"
                spacing={0.5} // Reduced spacing
                alignItems="center"
                sx={{
                    width: '100%',
                    mb: isRange ? 0.5 : 0 // Reduced bottom margin
                }}
            >
                <StyledSlider
                    size={isMobile ? "medium" : "small"}
                    min={json.min}
                    max={json.max}
                    step={json.step}
                    value={value}
                    onChange={handleSliderChange}
                    valueLabelDisplay={(isDragging || showValueLabel) ? "auto" : "off"}
                    onMouseDown={handleMouseDown}
                    onTouchStart={handleTouchStart}
                    marks={json.markers ? [
                        { value: json.min, label: formatDisplayValue(json.min) },
                        { value: json.max, label: formatDisplayValue(json.max) }
                    ] : undefined}
                    sx={{
                        flexGrow: 1,
                        // Give more space to the slider
                        flexBasis: "85%",
                    }}
                    isMobile={isMobile}
                />

                <Stack
                    direction="row"
                    spacing={0.25} // Tighter spacing
                    alignItems="center"
                    // Contain the controls in a smaller area
                    sx={{
                        flexBasis: "15%",
                        flexShrink: 0,
                        justifyContent: "flex-end"
                    }}
                >
                    {/* Decrement button */}
                    <ValueButton
                        size="small"
                        onClick={handleDecrement}
                        isMobile={isMobile}
                    >
                        <ChevronDown size={isMobile ? 16 : 14} />
                    </ValueButton>

                    {/* Input field */}
                    <StyledInput
                        value={getDisplayValue()}
                        onChange={handleInputChange}
                        valueLength={getValueLength()}
                        type="number"
                        inputProps={{
                            min: json.min,
                            max: json.max,
                            step: json.step,
                            'aria-label': 'slider value',
                        }}
                        disableUnderline
                        isMobile={isMobile}
                    />

                    {/* Increment button */}
                    <ValueButton
                        size="small"
                        onClick={handleIncrement}
                        isMobile={isMobile}
                    >
                        <ChevronUp size={isMobile ? 16 : 14} />
                    </ValueButton>
                </Stack>
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
                            fontSize: isMobile ? '0.7rem' : '0.65rem', // Smaller text
                            fontFamily: 'Roboto, Arial, sans-serif',
                            padding: isMobile ? '4px 2px' : '2px', // Smaller padding
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
                            fontSize: isMobile ? '0.7rem' : '0.65rem', // Smaller text
                            fontFamily: 'Roboto, Arial, sans-serif',
                            padding: isMobile ? '4px 2px' : '2px', // Smaller padding
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