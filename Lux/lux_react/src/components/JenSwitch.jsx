import React, { useState, useEffect } from "react";
import {
    Switch,
    Checkbox,
    FormControlLabel,
    FormGroup,
    Typography,
    Tooltip,
    Box,
    Paper
} from '@mui/material';
import { styled, useTheme } from '@mui/material/styles';
import { Info, ToggleLeft, ToggleRight } from 'lucide-react';

// Styled Switch with custom colors and animation
const StyledSwitch = styled(Switch)(({ theme }) => ({
    width: 36,
    height: 20,
    padding: 0,
    '& .MuiSwitch-switchBase': {
        padding: 2,
        '&.Mui-checked': {
            transform: 'translateX(16px)',
            color: '#fff',
            '& + .MuiSwitch-track': {
                backgroundColor: theme.palette.primary.main,
                opacity: 1,
            },
        },
    },
    '& .MuiSwitch-thumb': {
        width: 16,
        height: 16,
        borderRadius: '50%',
    },
    '& .MuiSwitch-track': {
        borderRadius: 10,
        opacity: 0.5,
        backgroundColor: theme.palette.mode === 'dark' ? '#8796A5' : '#aab4be',
    },
}));

// Styled Checkbox with custom colors and animation
const StyledCheckbox = styled(Checkbox)(({ theme }) => ({
    padding: 8,
    '& .MuiSvgIcon-root': {
        fontSize: 20,
    },
    '&.Mui-checked': {
        color: theme.palette.primary.main,
    },
}));

// Info tooltip component for description
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

function JenSwitch({ json, onChange }) {
    const theme = useTheme();
    const [switchValue, setSwitchValue] = useState(json.value ?? false);

    // Update local state if JSON value changes
    useEffect(() => {
        setSwitchValue(json.value ?? false);
    }, [json.value]);

    const handleSwitchChange = (event) => {
        const newValue = event.target.checked;
        setSwitchValue(newValue);

        // Call C++ function to update backend state
        if (window.module && typeof window.module.handle_switch_value === 'function') {
            window.module.handle_switch_value(json.name, newValue);

            // If this switch affects widget groups, call the onChange callback
            if (json.affects_widget_groups && onChange) {
                onChange();
            }
        }
    };

    // Get appropriate icon based on tool type and state
    const getIconComponent = () => {
        if (json.tool === 'switch') {
            return switchValue ? <ToggleRight size={16} /> : <ToggleLeft size={16} />;
        }
        return null;
    };

    // Render different types of toggle controls
    const renderToggleControl = () => {
        const label = (
            <Box sx={{ display: 'flex', alignItems: 'center' }}>
                <Typography variant="body2" fontWeight={500}>
                    {json.label || json.name}
                </Typography>
                <InfoTooltip description={json.description} />
            </Box>
        );

        switch (json.tool) {
            case 'checkbox':
                return (
                    <FormControlLabel
                        control={
                            <StyledCheckbox
                                checked={switchValue}
                                onChange={handleSwitchChange}
                                size="small"
                            />
                        }
                        label={label}
                        sx={{ marginLeft: -0.5 }}
                    />
                );

            case 'switch':
            default:
                return (
                    <FormControlLabel
                        control={
                            <StyledSwitch
                                checked={switchValue}
                                onChange={handleSwitchChange}
                                size="small"
                            />
                        }
                        label={label}
                        labelPlacement="start"
                        sx={{
                            ml: 0,
                            mr: 0,
                            justifyContent: 'space-between',
                            width: '100%'
                        }}
                    />
                );
        }
    };

    return (
        <Paper
            variant="outlined"
            sx={{
                p: 0.75,
                px: 1.5,
                borderRadius: 1,
                backgroundColor: theme.palette.background.paper,
                borderColor: switchValue
                    ? `${theme.palette.primary.main}50` // Semi-transparent primary color when active
                    : theme.palette.divider,
                '&:hover': {
                    borderColor: switchValue
                        ? theme.palette.primary.main
                        : theme.palette.action.active,
                },
                transition: 'all 0.2s ease-in-out',
            }}
        >
            <FormGroup sx={{ m: 0 }}>
                {renderToggleControl()}
            </FormGroup>
        </Paper>
    );
}

export default JenSwitch;