import React, { useState, useEffect } from "react";
import {
    Switch,
    Checkbox,
    FormControlLabel,
    Typography,
    Box,
    Tooltip
} from '@mui/material';
import { styled, useTheme } from '@mui/material/styles';
import { ControlPanelContext } from './InterfaceContainer';

const StyledSwitch = styled(Switch)(({ theme }) => ({
    width: 38,
    height: 20,
    padding: 0,
    '& .MuiSwitch-switchBase': {
        padding: 2,
        margin: 0,
        transitionDuration: '300ms',
        '&.Mui-checked': {
            transform: 'translateX(18px)',
            color: '#fff',
            '& + .MuiSwitch-track': {
                backgroundColor: theme.palette.primary.main,
                opacity: 1,
                border: 0,
            },
            '&.Mui-disabled + .MuiSwitch-track': {
                opacity: 0.5,
            },
        },
        '&.Mui-disabled + .MuiSwitch-track': {
            opacity: theme.palette.mode === 'light' ? 0.7 : 0.3,
        },
    },
    '& .MuiSwitch-thumb': {
        boxSizing: 'border-box',
        width: 16,
        height: 16,
        boxShadow: 'none',
    },
    '& .MuiSwitch-track': {
        borderRadius: 26 / 2,
        backgroundColor: theme.palette.mode === 'light' ? 'rgba(0,0,0,.25)' : 'rgba(255,255,255,.25)',
        opacity: 1,
        transition: theme.transitions.create(['background-color'], {
            duration: 500,
        }),
    },
}));

// Styled Checkbox for better aesthetics
const StyledCheckbox = styled(Checkbox)(({ theme }) => ({
    '&.MuiCheckbox-root': {
        padding: 6,
    },
    '& .MuiSvgIcon-root': {
        fontSize: 20,
    },
    '&.Mui-checked': {
        color: theme.palette.primary.main,
    },
}));

function JenSwitch({ json, onChange }) {
    const theme = useTheme();
    const [switchValue, setSwitchValue] = useState(json.value ?? false);
    
    // Get reset trigger from context
    const { resetTrigger } = React.useContext(ControlPanelContext);

    // Handle switch change
    const handleSwitchChange = (event) => {
        const newValue = event.target.checked;
        setSwitchValue(newValue);

        if (window.module) {
            window.module.handle_switch_value(json.name, newValue);

            if (json.affects_widget_groups && onChange) {
                onChange();
            }
        }
    };

    // For checkbox type
    const renderCheckbox = () => {
        return (
            <FormControlLabel
                control={
                    <StyledCheckbox
                        checked={switchValue}
                        onChange={handleSwitchChange}
                        inputProps={{ 'aria-label': json.label }}
                    />
                }
                label={
                    <Typography
                        variant="body2"
                        sx={{
                            color: switchValue ?
                                theme.palette.text.primary :
                                theme.palette.text.secondary,
                            transition: 'color 0.2s',
                            fontWeight: switchValue ? 500 : 400
                        }}
                    >
                        {json.label}
                    </Typography>
                }
                sx={{
                    m: 0,
                    '&:hover': {
                        bgcolor: theme.palette.action.hover,
                        borderRadius: 1
                    }
                }}
            />
        );
    };

    // For switch type
    const renderSwitch = () => {
        return (
            <StyledSwitch
                checked={switchValue}
                onChange={handleSwitchChange}
                inputProps={{ 'aria-label': json.label }}
            />
        );
    };

    // Determine which type to render
    const renderControl = () => {
        switch (json.tool) {
            case 'checkbox':
                return renderCheckbox();
            case 'switch':
                return renderSwitch();
            default:
                return (
                    <Typography color="error" variant="caption">
                        Unknown switch type: {json.tool}
                    </Typography>
                );
        }
    };

    // Reset to default value when resetTrigger changes
    useEffect(() => {
        if (resetTrigger > 0) {
            const defaultValue = json.default_value ?? false;
            setSwitchValue(defaultValue);
        }
    }, [resetTrigger, json.default_value]);

    // Update state if json value changes
    useEffect(() => {
        if (json.value !== undefined && json.value !== switchValue) {
            setSwitchValue(json.value);
        }
    }, [json.value]);

    return (
        <Tooltip
            title={json.description ?? ''}
            placement="top"
            arrow
            enterDelay={700}
        >
            <Box>
                {renderControl()}
            </Box>
        </Tooltip>
    );
}

export default JenSwitch;