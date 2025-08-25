import React, { useState, useEffect } from "react";
import {
    Select,
    MenuItem,
    FormControl,
    InputLabel,
    Radio,
    RadioGroup,
    FormControlLabel,
    Typography,
    Tooltip,
    Box,
    useTheme,
    Chip
} from '@mui/material';
import { CheckCircle } from 'lucide-react';
import { styled } from '@mui/system';
import { ControlPanelContext } from './InterfaceContainer';

const StyledSelect = styled(Select)(({ theme }) => ({
    borderRadius: theme.shape.borderRadius,
    fontFamily: 'Roboto, Arial, sans-serif', // Ensure consistent font family
    '& .MuiSelect-select': {
        paddingTop: 10,
        paddingBottom: 10,
        fontFamily: 'Roboto, Arial, sans-serif', // Apply font to selected text
    },
    '&.MuiOutlinedInput-root': {
        '& fieldset': {
            borderColor: theme.palette.divider,
        },
        '&:hover fieldset': {
            borderColor: theme.palette.action.active,
        },
        '&.Mui-focused fieldset': {
            borderColor: theme.palette.primary.main,
        },
    },
}));

const StyledRadio = styled(Radio)(({ theme }) => ({
    padding: 8,
    '&.Mui-checked': {
        color: theme.palette.primary.main,
    },
    '& .MuiSvgIcon-root': {
        fontSize: 20,
    },
}));

const StyledTypography = styled(Typography)(({ theme }) => ({
    fontFamily: 'Roboto, Arial, sans-serif',
}));

const StyledInputLabel = styled(InputLabel)(({ theme }) => ({
    fontFamily: 'Roboto, Arial, sans-serif',
}));

const StyledFormControlLabel = styled(FormControlLabel)(({ theme }) => ({
    '& .MuiFormControlLabel-label': {
        fontFamily: 'Roboto, Arial, sans-serif',
    }
}));

function JenMenu({ json, onChange }) {
    const theme = useTheme();
    
    // For saved scenes, the backend should have already assigned the runtime value
    // directly to json.choice, so we can use it directly
    const getInitialChoice = () => {
        if (json.choice !== undefined) {
            if (typeof json.choice === 'object' && json.choice.functions) {
                // This is a harness object - check for runtime value
                if (json.choice.value !== undefined) {
                    console.log(`Menu ${json.name}: Found harness object with runtime value:`, json.choice.value);
                    return json.choice.value;  // Use saved runtime value
                } else {
                    console.log(`Menu ${json.name}: Harness object without runtime value (default scene), using default_choice`);
                    // This is a default scene - use default_choice
                    return json.default_choice ?? 0;
                }
            } else if (typeof json.choice === 'number') {
                return json.choice;  // Use saved runtime value
            }
        }
        return json.default_choice ?? 0;  // Fallback to default
    };
    
    const [selectedMenuChoice, setSelectedMenuChoice] = useState(getInitialChoice());
    const [internalItems, setInternalItems] = useState(json.items || []);
    
    // Get reset trigger from context
    //const { resetTrigger } = React.useContext(ControlPanelContext);
    const { resetTrigger } = 0;

    // Handle menu change
    const handleMenuChange = (event) => {
        const newValue = event.target.value;
        setSelectedMenuChoice(newValue);

        if (window.module) {
            window.module.handle_menu_choice(json.name, newValue);

            if (json.affects_widget_groups && onChange) {
                onChange();
            }
        }
    };

    const renderPullDown = () => {
        return (
            <FormControl
                fullWidth
                size="small"
                variant="outlined"
                sx={{ minWidth: 120 }}
            >
                <StyledInputLabel id={`menu-label-${json.name}`}>{json.label}</StyledInputLabel>
                <StyledSelect
                    labelId={`menu-label-${json.name}`}
                    value={selectedMenuChoice}
                    label={json.label}
                    onChange={handleMenuChange}
                    MenuProps={{
                        PaperProps: {
                            style: {
                                maxHeight: 300,
                                overflowY: 'auto',
                                fontFamily: 'Roboto, Arial, sans-serif', // Apply font to menu container
                            },
                            elevation: 3,
                        },
                        anchorOrigin: {
                            vertical: 'bottom',
                            horizontal: 'left',
                        },
                        transformOrigin: {
                            vertical: 'top',
                            horizontal: 'left',
                        },
                    }}
                    renderValue={(selected) => (
                        <Box sx={{
                            display: 'flex',
                            alignItems: 'center',
                            gap: 0.5,
                            overflow: 'hidden',
                            fontFamily: 'Roboto, Arial, sans-serif' // Apply font to rendered value
                        }}>
                            {internalItems[selected]}
                        </Box>
                    )}
                >
                    {internalItems.map((choice, index) => (
                        <MenuItem
                            key={index}
                            value={index}
                            sx={{
                                minHeight: 40,
                                display: 'flex',
                                alignItems: 'center',
                                gap: 1,
                                px: 2,
                                fontFamily: 'Roboto, Arial, sans-serif' // Apply font to menu items
                            }}
                        >
                            {index === selectedMenuChoice && (
                                <CheckCircle
                                    size={16}
                                    color={theme.palette.primary.main}
                                />
                            )}
                            <StyledTypography
                                variant="body2"
                                sx={{
                                    ml: index === selectedMenuChoice ? 0 : 3.5,
                                    maxWidth: '100%',
                                    overflow: 'hidden',
                                    textOverflow: 'ellipsis'
                                }}
                            >
                                {choice}
                            </StyledTypography>
                        </MenuItem>
                    ))}
                </StyledSelect>
            </FormControl>
        );
    };

    const renderRadio = () => {
        return (
            <Box sx={{ width: '100%' }}>
                <StyledTypography
                    variant="body2"
                    sx={{
                        mb: 1,
                        fontWeight: 500,
                        color: theme.palette.text.primary
                    }}
                >
                    {json.label}
                </StyledTypography>
                <RadioGroup
                    value={selectedMenuChoice}
                    onChange={handleMenuChange}
                    sx={{ pl: 1 }}
                >
                    {internalItems.map((choice, index) => (
                        <StyledFormControlLabel
                            key={index}
                            value={index}
                            control={<StyledRadio />}
                            label={
                                <StyledTypography
                                    variant="body2"
                                    sx={{
                                        color: index === selectedMenuChoice ?
                                            theme.palette.text.primary :
                                            theme.palette.text.secondary
                                    }}
                                >
                                    {choice}
                                </StyledTypography>
                            }
                            sx={{
                                mb: 0.5,
                                '&:hover': {
                                    bgcolor: theme.palette.action.hover,
                                    borderRadius: 1
                                }
                            }}
                        />
                    ))}
                </RadioGroup>
            </Box>
        );
    };

    const renderMenu = () => {
        if (json.tool === 'pull_down' || json.tool === 'image') {
            return renderPullDown();
        } else if (json.tool === 'radio') {
            return renderRadio();
        } else {
            return (
                <StyledTypography color="error" variant="caption">
                    Unknown menu type: {json.tool}
                </StyledTypography>
            );
        }
    };

    // Reset to default choice when resetTrigger changes
    /*useEffect(() => {
        if (resetTrigger > 0) {
            const defaultChoice = json.default_choice ?? 0;
            setSelectedMenuChoice(defaultChoice);
        }
    }, [resetTrigger, json.default_choice]);*/

    useEffect(() => {
        setInternalItems(json.items || []);
    }, [json.items]);

    return (
        <Tooltip
            title={json.description ?? ''}
            placement="top"
            arrow
            enterDelay={700}
        >
            <Box sx={{ width: '100%' }}>
                {renderMenu()}
            </Box>
        </Tooltip>
    );
}

export default JenMenu;