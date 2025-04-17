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

// Styled Select component for better aesthetics
const StyledSelect = styled(Select)(({ theme }) => ({
    borderRadius: theme.shape.borderRadius,
    '& .MuiSelect-select': {
        paddingTop: 10,
        paddingBottom: 10,
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

// Styled Radio component
const StyledRadio = styled(Radio)(({ theme }) => ({
    padding: 8,
    '&.Mui-checked': {
        color: theme.palette.primary.main,
    },
    '& .MuiSvgIcon-root': {
        fontSize: 20,
    },
}));

function JenMenu({ json, onChange }) {
    const theme = useTheme();
    const [selectedMenuChoice, setSelectedMenuChoice] = useState(json.choice ?? 0);
    const [internalItems, setInternalItems] = useState(json.items || []);

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

    // For pull-down or image menu types
    const renderPullDown = () => {
        return (
            <FormControl
                fullWidth
                size="small"
                variant="outlined"
                sx={{ minWidth: 120 }}
            >
                <InputLabel id={`menu-label-${json.name}`}>{json.label}</InputLabel>
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
                            overflow: 'hidden'
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
                                px: 2
                            }}
                        >
                            {index === selectedMenuChoice && (
                                <CheckCircle
                                    size={16}
                                    color={theme.palette.primary.main}
                                />
                            )}
                            <Typography
                                variant="body2"
                                sx={{
                                    ml: index === selectedMenuChoice ? 0 : 3.5,
                                    maxWidth: '100%',
                                    overflow: 'hidden',
                                    textOverflow: 'ellipsis'
                                }}
                            >
                                {choice}
                            </Typography>
                        </MenuItem>
                    ))}
                </StyledSelect>
            </FormControl>
        );
    };

    // For radio button menu type
    const renderRadio = () => {
        return (
            <Box sx={{ width: '100%' }}>
                <Typography
                    variant="body2"
                    sx={{
                        mb: 1,
                        fontWeight: 500,
                        color: theme.palette.text.primary
                    }}
                >
                    {json.label}
                </Typography>
                <RadioGroup
                    value={selectedMenuChoice}
                    onChange={handleMenuChange}
                    sx={{ pl: 1 }}
                >
                    {internalItems.map((choice, index) => (
                        <FormControlLabel
                            key={index}
                            value={index}
                            control={<StyledRadio />}
                            label={
                                <Typography
                                    variant="body2"
                                    sx={{
                                        color: index === selectedMenuChoice ?
                                            theme.palette.text.primary :
                                            theme.palette.text.secondary
                                    }}
                                >
                                    {choice}
                                </Typography>
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

    // Determine which rendering method to use
    const renderMenu = () => {
        if (json.tool === 'pull_down' || json.tool === 'image') {
            return renderPullDown();
        } else if (json.tool === 'radio') {
            return renderRadio();
        } else {
            return (
                <Typography color="error" variant="caption">
                    Unknown menu type: {json.tool}
                </Typography>
            );
        }
    };

    // Observe changes to menu items
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