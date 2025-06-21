import React, { useState, useEffect, useRef, useCallback } from "react";
import {
    Slider,
    TextField,
    Stack,
    Box,
    Typography,
    useTheme,
    IconButton,
    useMediaQuery,
    InputAdornment,
    Fab
} from '@mui/material';
import { styled } from '@mui/system';
import { ChevronUp, ChevronDown, Edit, Check } from 'lucide-react';
import { ControlPanelContext } from './InterfaceContainer';

// Mobile-optimized slider with larger touch targets
const MobileSlider = styled(Slider)(({ theme }) => ({
    height: 8,
    padding: '16px 0',
    '& .MuiSlider-thumb': {
        height: 24,
        width: 24,
        backgroundColor: theme.palette.common.white,
        border: `3px solid ${theme.palette.primary.main}`,
        boxShadow: '0 2px 8px rgba(0,0,0,0.2)',
        '&:hover, &.Mui-focusVisible': {
            boxShadow: `0 0 0 8px ${theme.palette.primary.main}20, 0 2px 12px rgba(0,0,0,0.3)`,
        },
        '&.Mui-active': {
            boxShadow: `0 0 0 12px ${theme.palette.primary.main}30, 0 2px 16px rgba(0,0,0,0.4)`,
        },
        // Prevent zoom on double tap
        touchAction: 'manipulation',
    },
    '& .MuiSlider-rail': {
        height: 8,
        borderRadius: 4,
        backgroundColor: theme.palette.action.hover,
    },
    '& .MuiSlider-track': {
        height: 8,
        borderRadius: 4,
        border: 'none',
    },
    '& .MuiSlider-valueLabel': {
        fontSize: '0.85rem',
        fontWeight: 600,
        background: theme.palette.primary.main,
        borderRadius: 8,
        padding: '4px 8px',
        '&:before': { display: 'none' },
    },
    '& .MuiSlider-mark': {
        backgroundColor: theme.palette.text.disabled,
        height: 4,
        width: 2,
    },
    '& .MuiSlider-markActive': {
        backgroundColor: theme.palette.common.white,
    },
}));

// Desktop-optimized slider
const DesktopSlider = styled(Slider)(({ theme }) => ({
    height: 4,
    padding: '11px 0',
    '& .MuiSlider-thumb': {
        height: 18,
        width: 18,
        backgroundColor: theme.palette.common.white,
        border: `2px solid ${theme.palette.primary.main}`,
        boxShadow: '0 1px 4px rgba(0,0,0,0.2)',
        '&:hover, &.Mui-focusVisible': {
            boxShadow: `0 0 0 6px ${theme.palette.primary.main}20, 0 1px 6px rgba(0,0,0,0.3)`,
        },
        '&.Mui-active': {
            boxShadow: `0 0 0 8px ${theme.palette.primary.main}30, 0 1px 8px rgba(0,0,0,0.4)`,
        },
    },
    '& .MuiSlider-rail': {
        height: 4,
        borderRadius: 2,
        backgroundColor: theme.palette.action.hover,
    },
    '& .MuiSlider-track': {
        height: 4,
        borderRadius: 2,
        border: 'none',
    },
    '& .MuiSlider-valueLabel': {
        fontSize: '0.75rem',
        fontWeight: 500,
        background: theme.palette.primary.main,
        borderRadius: 6,
        padding: '2px 6px',
        '&:before': { display: 'none' },
    },
}));

// Mobile-optimized text field that prevents zooming
const MobileTextField = styled(TextField)(({ theme }) => ({
    '& .MuiInputBase-root': {
        fontSize: '16px', // Prevents zoom on iOS
        minHeight: '46px', // Standardized height
        height: '46px',    // Fixed height for consistency
        width: '100%',     // Full width of container
        borderRadius: 8,
        backgroundColor: theme.palette.action.hover,
        border: `1px solid ${theme.palette.divider}`,
        '&:hover': {
            backgroundColor: theme.palette.action.selected,
            borderColor: theme.palette.primary.main,
        },
        '&.Mui-focused': {
            backgroundColor: theme.palette.action.selected,
            borderColor: theme.palette.primary.main,
            boxShadow: `0 0 0 2px ${theme.palette.primary.main}20`,
        },
    },
    '& .MuiInputBase-input': {
        textAlign: 'center',
        padding: '12px 8px',
        fontWeight: 500,
        // Prevent zoom on mobile
        fontSize: '16px',
        // Disable browser zoom/scroll behavior
        touchAction: 'manipulation',
        height: 'auto',
        lineHeight: 'normal',
    },
    '& .MuiOutlinedInput-notchedOutline': {
        border: 'none',
    },
    '& input[type=number]': {
        MozAppearance: 'textfield',
    },
    '& input::-webkit-outer-spin-button, & input::-webkit-inner-spin-button': {
        WebkitAppearance: 'none',
        margin: 0,
    },
}));

// Desktop text field
const DesktopTextField = styled(TextField)(({ theme }) => ({
    '& .MuiInputBase-root': {
        fontSize: '0.875rem',
        minHeight: '34px',  // Standardized height
        height: '34px',     // Fixed height for consistency
        width: '100%',      // Full width of container
        borderRadius: 6,
        backgroundColor: theme.palette.action.hover,
        border: `1px solid ${theme.palette.divider}`,
        '&:hover': {
            backgroundColor: theme.palette.action.selected,
            borderColor: theme.palette.primary.main,
        },
        '&.Mui-focused': {
            backgroundColor: theme.palette.action.selected,
            borderColor: theme.palette.primary.main,
            boxShadow: `0 0 0 2px ${theme.palette.primary.main}20`,
        },
    },
    '& .MuiInputBase-input': {
        textAlign: 'center',
        padding: '8px 6px',
        fontWeight: 500,
        height: 'auto',
        lineHeight: 'normal',
    },
    '& .MuiOutlinedInput-notchedOutline': {
        border: 'none',
    },
    '& input[type=number]': {
        MozAppearance: 'textfield',
    },
    '& input::-webkit-outer-spin-button, & input::-webkit-inner-spin-button': {
        WebkitAppearance: 'none',
        margin: 0,
    },
}));

// Mobile-optimized control buttons
const MobileControlButton = styled(IconButton)(({ theme }) => ({
    width: 27,
    height: 27,
    backgroundColor: theme.palette.action.hover,
    border: `1px solid ${theme.palette.divider}`,
    borderRadius: 8,
    '&:hover': {
        backgroundColor: theme.palette.action.selected,
        borderColor: theme.palette.primary.main,
    },
    '&:active': {
        backgroundColor: theme.palette.primary.main,
        color: theme.palette.primary.contrastText,
        transform: 'scale(0.95)',
    },
    // Prevent zoom on double tap
    touchAction: 'manipulation',
    transition: 'all 0.15s ease',
}));

// Desktop control buttons
const DesktopControlButton = styled(IconButton)(({ theme }) => ({
    width: 28,
    height: 28,
    backgroundColor: theme.palette.action.hover,
    border: `1px solid ${theme.palette.divider}`,
    borderRadius: 4,
    '&:hover': {
        backgroundColor: theme.palette.action.selected,
        borderColor: theme.palette.primary.main,
    },
    '&:active': {
        backgroundColor: theme.palette.primary.main,
        color: theme.palette.primary.contrastText,
    },
    transition: 'all 0.15s ease',
}));

function JenSlider({ json, width }) {
    const theme = useTheme();
    const isMobile = useMediaQuery(theme.breakpoints.down('md'));
    
    // Initialize value properly for range vs single sliders
    const getInitialValue = () => {
        const isRange = json.type === 'range_slider_int' || json.type === 'range_slider_float';
        if (isRange) {
            // For range sliders, default_value should be an array [min, max]
            if (Array.isArray(json.default_value) && json.default_value.length === 2) {
                return json.default_value;
            }
            // Fallback: create array from min to max
            const minVal = json.min ?? 0;
            const maxVal = json.max ?? 100;
            return [minVal, maxVal];
        } else {
            // Single slider
            return json.default_value ?? json.min ?? 0;
        }
    };
    
    const [value, setValue] = useState(getInitialValue());
    const [minFocus, setMinFocus] = useState(true);
    const [isDragging, setIsDragging] = useState(false);
    const [showValueLabel, setShowValueLabel] = useState(false);
    const [inputValue, setInputValue] = useState('');
    const [isInputFocused, setIsInputFocused] = useState(false);
    const touchTimer = useRef(null);
    const inputRef = useRef(null);
    const { sliderValues, onSliderChange, resetTrigger } = React.useContext(ControlPanelContext);

    const isRange = json.type === 'range_slider_int' || json.type === 'range_slider_float';

    const formatDisplayValue = useCallback((val) => {
        if (json.type === 'slider_int' || json.type === 'range_slider_int') {
            return parseInt(val);
        } else {
            const rounded = parseFloat(val).toFixed(1);
            return rounded.endsWith('.0') ? rounded.slice(0, -2) : rounded;
        }
    }, [json.type]);

    const formatModuleValue = useCallback((val) => {
        if (json.type === 'slider_int' || json.type === 'range_slider_int') {
            return parseInt(val);
        } else {
            return parseFloat(val);
        }
    }, [json.type]);

    const getCurrentValue = useCallback(() => {
        if (isRange) {
            // Ensure value is an array before accessing
            if (Array.isArray(value) && value.length >= 2) {
                return minFocus ? value[0] : value[1];
            }
            // Fallback if value is not properly initialized
            return json.min ?? 0;
        } else {
            return value;
        }
    }, [isRange, minFocus, value, json.min]);

    const getDisplayValue = useCallback(() => {
        return formatDisplayValue(getCurrentValue());
    }, [getCurrentValue, formatDisplayValue]);

    // Reset to default value when resetTrigger changes
    /*useEffect(() => {
        if (resetTrigger > 0) {
            const resetValue = getInitialValue();
            setValue(resetValue);
            
            // Set input value based on current focus for range sliders
            if (isRange && Array.isArray(resetValue)) {
                const displayVal = minFocus ? resetValue[0] : resetValue[1];
                setInputValue(formatDisplayValue(displayVal).toString());
            } else {
                setInputValue(formatDisplayValue(resetValue).toString());
            }
        }
    }, [resetTrigger, json.default_value, json.min, json.max, formatDisplayValue, isRange, minFocus]);
*/
    useEffect(() => {
        console.log(`Initializing slider callback: ${json.name}`, json);
        // Register the callback via embind
        if (window.Module) {
            window.Module.set_slider_callback(json.name, (newValue) => {
                // Update the slider's internal value
                setValue(newValue);
    
                // Update the displayed input value
                if (isRange && Array.isArray(newValue)) {
                    const displayVal = minFocus ? newValue[0] : newValue[1];
                    setInputValue(formatDisplayValue(displayVal).toString());
                } else {
                    setInputValue(formatDisplayValue(newValue).toString());
                }
    
                // Notify the parent or context about the change
                onSliderChange(json.name, newValue);
                console.log(`Slider ${json.name} updated to:`, newValue);
            });
        }
    }, [json.name, onSliderChange, isRange, minFocus, formatDisplayValue]);

    // Update input value when slider value changes (but not when user is typing)
    useEffect(() => {
        if (!isInputFocused) {
            setInputValue(getDisplayValue().toString());
        }
    }, [getDisplayValue, isInputFocused]);

    const handleSliderChange = useCallback((event, newValue) => {
        if (isRange) {
            if (Array.isArray(newValue) && Array.isArray(value)) {
                if (value[0] !== newValue[0]) setMinFocus(true);
                else if (value[1] !== newValue[1]) setMinFocus(false);
            }
        }
        setValue(newValue);
        onSliderChange(json.name, newValue);
    }, [isRange, value, json.name, onSliderChange]);

    const handleInputChange = useCallback((e) => {
        const newInputValue = e.target.value;
        setInputValue(newInputValue);
        
        // Update slider value in real-time if it's a valid number
        const newValue = parseFloat(newInputValue);
        //if (!isNaN(newValue) && newValue >= json.min && newValue <= json.max) {
            if (isRange && Array.isArray(value)) {
                const rangeValue = minFocus ? [newValue, value[1]] : [value[0], newValue];
                setValue(rangeValue);
                onSliderChange(json.name, rangeValue);
            } else if (!isRange) {
                setValue(newValue);
                onSliderChange(json.name, newValue);
            }
        //}
    }, [json.min, json.max, json.name, isRange, minFocus, value, onSliderChange]);

    const handleInputFocus = useCallback(() => {
        setIsInputFocused(true);
        setInputValue(getDisplayValue().toString());
        // Select all text on focus for easy editing
        if (inputRef.current) {
            setTimeout(() => {
                inputRef.current.select();
            }, 0);
        }
    }, [getDisplayValue]);

    const handleInputBlur = useCallback(() => {
        setIsInputFocused(false);
        
        // Parse and validate input value
        let newValue = parseFloat(inputValue);
        if (isNaN(newValue)) {
            newValue = getCurrentValue();
        }
        
        // Clamp to min/max
        //newValue = Math.max(json.min, Math.min(json.max, newValue));
        
        if (isRange && Array.isArray(value)) {
            const rangeValue = minFocus ? [newValue, value[1]] : [value[0], newValue];
            setValue(rangeValue);
            onSliderChange(json.name, rangeValue);
        } else if (!isRange) {
            setValue(newValue);
            onSliderChange(json.name, newValue);
        }
        
        // Update input to show formatted value
        setInputValue(formatDisplayValue(newValue).toString());
    }, [inputValue, getCurrentValue, json.min, json.max, isRange, minFocus, value, json.name, onSliderChange, formatDisplayValue]);

    const handleInputKeyDown = useCallback((e) => {
        if (e.key === 'Enter') {
            // Parse and validate input value immediately
            let newValue = parseFloat(inputValue);
            if (isNaN(newValue)) {
                newValue = getCurrentValue();
            }
            
            // Clamp to min/max
            //newValue = Math.max(json.min, Math.min(json.max, newValue));
            
            if (isRange && Array.isArray(value)) {
                const rangeValue = minFocus ? [newValue, value[1]] : [value[0], newValue];
                setValue(rangeValue);
                onSliderChange(json.name, rangeValue);
            } else if (!isRange) {
                setValue(newValue);
                onSliderChange(json.name, newValue);
            }
            
            // Update input to show formatted value
            setInputValue(formatDisplayValue(newValue).toString());
            inputRef.current?.blur();
        } else if (e.key === 'Escape') {
            setInputValue(getDisplayValue().toString());
            inputRef.current?.blur();
        }
    }, [inputValue, getCurrentValue, json.min, json.max, isRange, minFocus, value, json.name, onSliderChange, formatDisplayValue, getDisplayValue]);

    const handleIncrement = useCallback(() => {
        const step = json.step || 1;
        if (isRange && Array.isArray(value)) {
            const currentVal = minFocus ? value[0] : value[1];
            let newVal = currentVal + step;
            if (json.type === 'range_slider_int') {
                newVal = Math.floor(newVal);
            }
            //newVal = Math.min(json.max, newVal);
            const newValue = minFocus ? [newVal, value[1]] : [value[0], newVal];
            setValue(newValue);
            onSliderChange(json.name, newValue);
        } else if (!isRange) {
            let newVal = value + step;
            if (json.type === 'slider_int') {
                newVal = Math.floor(newVal);
            }
            //newVal = Math.min(json.max, newVal);
            setValue(newVal);
            onSliderChange(json.name, newVal);
        }
    }, [json.step, json.max, json.type, isRange, minFocus, value, json.name, onSliderChange]);

    const handleDecrement = useCallback(() => {
        const step = json.step || 1;
        if (isRange && Array.isArray(value)) {
            const currentVal = minFocus ? value[0] : value[1];
            let newVal = currentVal - step;
            if (json.type === 'range_slider_int') {
                newVal = Math.floor(newVal);
            }
            //newVal = Math.max(json.min, newVal);
            const newValue = minFocus ? [newVal, value[1]] : [value[0], newVal];
            setValue(newValue);
            onSliderChange(json.name, newValue);
        } else if (!isRange) {
            let newVal = value - step;
            if (json.type === 'slider_int') {
                newVal = Math.floor(newVal);
            }
            //newVal = Math.max(json.min, newVal);
            setValue(newVal);
            onSliderChange(json.name, newVal);
        }
    }, [json.step, json.min, json.type, isRange, minFocus, value, json.name, onSliderChange]);

    const handleTouchStart = useCallback(() => {
        setShowValueLabel(true);
        setIsDragging(true);
        if (touchTimer.current) {
            clearTimeout(touchTimer.current);
        }
    }, []);

    const handleTouchEnd = useCallback(() => {
        touchTimer.current = setTimeout(() => {
            setShowValueLabel(false);
            setIsDragging(false);
        }, 1000);
    }, []);

    const handleMouseDown = useCallback(() => {
        setIsDragging(true);
        setShowValueLabel(true);
    }, []);

    const handleMouseUp = useCallback(() => {
        setIsDragging(false);
        setTimeout(() => {
            setShowValueLabel(false);
        }, 800);
    }, []);

    // Prevent viewport shifting on mobile
    const preventZoom = useCallback((e) => {
        if (isMobile && e.target.type === 'number') {
            e.target.style.fontSize = '16px';
        }
    }, [isMobile]);

    useEffect(() => {
        if (isDragging) {
            document.addEventListener('mouseup', handleMouseUp);
            document.addEventListener('touchend', handleTouchEnd);

            return () => {
                document.removeEventListener('mouseup', handleMouseUp);
                document.removeEventListener('touchend', handleTouchEnd);
            };
        }
    }, [isDragging, handleMouseUp, handleTouchEnd]);

    // Initialize value from context if available
    useEffect(() => {
        if (sliderValues[json.name] !== undefined) {
            setValue(sliderValues[json.name]);
        } else if (json.default_value !== undefined) {
            setValue(json.default_value);
        } else if (json.min !== undefined) {
            setValue(json.min);
        }

        return () => {
            if (touchTimer.current) {
                clearTimeout(touchTimer.current);
            }
        };
    }, [json.name, json.default_value, json.min, sliderValues]);

    // Choose components based on device
    const SliderComponent = isMobile ? MobileSlider : DesktopSlider;
    const TextFieldComponent = isMobile ? MobileTextField : DesktopTextField;
    const ControlButton = isMobile ? MobileControlButton : DesktopControlButton;

    return (
        <Box
            sx={{
                width: '100%',
                px: isMobile ? 1.25 : 0.5,
                py: isMobile ? 0.5 : 0.25,
                position: 'relative',
                overflow: 'hidden',
            }}
        >
            <Stack
                direction="row"
                spacing={isMobile ? 0.5 : 1}
                alignItems="center"
                sx={{
                    width: '100%',
                    mb: isRange ? (isMobile ? 0.25 : 0.125) : 0,
                    // Ensure consistent height to prevent shifts - match input field heights
                    minHeight: isMobile ? 46 : 34, // Match input field heights
                    height: isMobile ? 46 : 34,    // Fixed height for consistency
                }}
            >
                {/* Main Slider */}
                <Box sx={{ 
                    flexGrow: 1, 
                    minWidth: 0, 
                    pr: isMobile ? 0.5 : 0.25,
                    // Add padding for thumb space while maintaining alignment
                    px: isMobile ? 1.5 : 1.125, // Padding for thumb space (half of thumb width)
                }}>
                    <SliderComponent
                        min={json.min}
                        max={json.max}
                        step={json.step}
                        value={value}
                        onChange={handleSliderChange}
                        onMouseDown={handleMouseDown}
                        onTouchStart={handleTouchStart}
                        marks={json.markers ? [
                            { value: json.min, label: formatDisplayValue(json.min) },
                            { value: json.max, label: formatDisplayValue(json.max) }
                        ] : undefined}
                        className="jen-slider"
                        data-name={json.name}
                        data-value={value}
                        // Prevent zoom on mobile
                        style={{ touchAction: 'manipulation' }}
                    />
                </Box>

                {/* Control Buttons */}
                <Stack direction="column" spacing={isMobile ? 0.25 : 0.125}>
                    <ControlButton 
                        onClick={handleIncrement}
                        sx={{
                            minWidth: isMobile ? 30 : 28,
                            minHeight: isMobile ? 20 : 18,
                            padding: isMobile ? '2px' : '1px',
                        }}
                    >
                        <ChevronUp size={isMobile ? 16 : 14} />
                    </ControlButton>
                    <ControlButton 
                        onClick={handleDecrement}
                        sx={{
                            minWidth: isMobile ? 30 : 28,
                            minHeight: isMobile ? 20 : 18,
                            padding: isMobile ? '2px' : '1px',
                        }}
                    >
                        <ChevronDown size={isMobile ? 16 : 14} />
                    </ControlButton>
                </Stack>

                {/* Value Input - Always Editable */}
                <Box sx={{ width: isMobile ? 70 : 68, flexShrink: 0 }}>
                    <TextFieldComponent
                        ref={inputRef}
                        value={inputValue}
                        onChange={handleInputChange}
                        onFocus={handleInputFocus}
                        onBlur={handleInputBlur}
                        onKeyDown={handleInputKeyDown}
                        type="number"
                        inputProps={{
                            min: json.min,
                            max: json.max,
                            step: json.step,
                            'aria-label': 'slider value',
                            style: { 
                                fontSize: isMobile ? '16px' : '0.875rem',
                                cursor: 'text',
                                textAlign: 'center',
                                fontWeight: '500'
                            }
                        }}
                        sx={{
                            '& .MuiInputBase-root': {
                                cursor: 'text',
                                width: '100%',
                                minHeight: isMobile ? 46 : 34,
                                height: isMobile ? 46 : 34,
                                '&:hover': {
                                    cursor: 'text'
                                }
                            },
                            '& .MuiInputBase-input': {
                                padding: isMobile ? '12px 8px' : '8px 6px',
                                height: 'auto',
                                lineHeight: 'normal'
                            }
                        }}
                        variant="outlined"
                        size={isMobile ? "medium" : "small"}
                        placeholder={getDisplayValue().toString()}
                    />
                </Box>
            </Stack>

            {/* Range Slider Labels */}
            {isRange && (
                <Box
                    sx={{
                        display: 'flex',
                        justifyContent: 'space-between',
                        px: isMobile ? 0.5 : 1,
                        height: isMobile ? 24 : 20, // Reduced height
                        alignItems: 'center',
                        mt: isMobile ? 0.25 : 0.125, // Reduced top margin
                    }}
                >
                    <Typography
                        variant="caption"
                        sx={{
                            color: minFocus ? theme.palette.primary.main : theme.palette.text.secondary,
                            fontWeight: minFocus ? 600 : 400,
                            cursor: 'pointer',
                            transition: 'color 0.2s',
                            fontSize: isMobile ? '0.75rem' : '0.65rem', // Slightly smaller font
                            padding: isMobile ? '4px 6px' : '2px 4px', // Reduced padding
                            borderRadius: 1,
                            minHeight: isMobile ? 24 : 20, // Reduced touch target
                            display: 'flex',
                            alignItems: 'center',
                            // Minimum touch target on mobile
                            minWidth: isMobile ? 40 : 'auto', // Reduced min width
                            justifyContent: 'center',
                            '&:hover': {
                                color: theme.palette.primary.main,
                                backgroundColor: theme.palette.action.hover,
                            },
                            // Prevent zoom
                            touchAction: 'manipulation',
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
                            fontSize: isMobile ? '0.75rem' : '0.65rem', // Slightly smaller font
                            padding: isMobile ? '4px 6px' : '2px 4px', // Reduced padding
                            borderRadius: 1,
                            minHeight: isMobile ? 24 : 20, // Reduced touch target
                            display: 'flex',
                            alignItems: 'center',
                            // Minimum touch target on mobile
                            minWidth: isMobile ? 40 : 'auto', // Reduced min width
                            justifyContent: 'center',
                            '&:hover': {
                                color: theme.palette.primary.main,
                                backgroundColor: theme.palette.action.hover,
                            },
                            // Prevent zoom
                            touchAction: 'manipulation',
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