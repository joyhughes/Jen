import React, { useState, useEffect } from 'react';
import { Box, ButtonGroup, Button, useTheme, Tooltip } from '@mui/material';
import {
    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,
    ArrowUpLeft,
    ArrowUpRight,
    ArrowDownLeft,
    ArrowDownRight
} from 'lucide-react';

// Mapping from direction string to internal numeric representation
const directionToNumber = (directionString) => {
    const mapping = {
        "up": 0,
        "up_right": 1,
        "right": 2,
        "down_right": 3,
        "down": 4,
        "down_left": 5,
        "left": 6,
        "up_left": 7
    };

    return mapping[directionString] ?? 0; // Default to "up"
};

// Component for 8-directional picker
function JenDirection8({ json }) {
    const theme = useTheme();
    const [direction, setDirection] = useState(() =>
        directionToNumber(json?.value ?? "down")
    );

    // Update module state when direction changes
    const handleDirectionChange = (newDirection) => {
        setDirection(newDirection);

        if (window.module) {
            window.module.pick_direction8(json.name, newDirection);
        }
    };

    // Direction button component with tooltips
    const DirectionButton = ({ value, icon, label }) => {
        const isSelected = direction === value;

        return (
            <Tooltip title={label} arrow placement="top">
                <Button
                    variant={isSelected ? "contained" : "outlined"}
                    onClick={() => handleDirectionChange(value)}
                    sx={{
                        minWidth: 36,
                        width: 36,
                        height: 36,
                        padding: 0,
                        borderColor: isSelected ? 'transparent' : theme.palette.divider,
                        color: isSelected ? 'white' : theme.palette.text.secondary,
                        boxShadow: isSelected ? 2 : 0,
                        transition: 'all 0.2s',
                        '&:hover': {
                            backgroundColor: isSelected
                                ? theme.palette.primary.main
                                : theme.palette.action.hover,
                            borderColor: isSelected
                                ? 'transparent'
                                : theme.palette.primary.light,
                        }
                    }}
                >
                    {icon}
                </Button>
            </Tooltip>
        );
    };

    return (
        <Box sx={{ display: 'inline-flex', flexDirection: 'column' }}>
            {/* Top row */}
            <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                <DirectionButton
                    value={7}
                    icon={<ArrowUpLeft size={16} />}
                    label="Up-Left"
                />
                <DirectionButton
                    value={0}
                    icon={<ArrowUp size={16} />}
                    label="Up"
                />
                <DirectionButton
                    value={1}
                    icon={<ArrowUpRight size={16} />}
                    label="Up-Right"
                />
            </Box>

            {/* Middle row */}
            <Box sx={{ display: 'flex', justifyContent: 'space-between', my: 0.5 }}>
                <DirectionButton
                    value={6}
                    icon={<ArrowLeft size={16} />}
                    label="Left"
                />
                <Box sx={{ width: 36, height: 36 }} /> {/* Empty center */}
                <DirectionButton
                    value={2}
                    icon={<ArrowRight size={16} />}
                    label="Right"
                />
            </Box>

            {/* Bottom row */}
            <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                <DirectionButton
                    value={5}
                    icon={<ArrowDownLeft size={16} />}
                    label="Down-Left"
                />
                <DirectionButton
                    value={4}
                    icon={<ArrowDown size={16} />}
                    label="Down"
                />
                <DirectionButton
                    value={3}
                    icon={<ArrowDownRight size={16} />}
                    label="Down-Right"
                />
            </Box>
        </Box>
    );
}

export default JenDirection8;