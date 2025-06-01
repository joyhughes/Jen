import React, { useState, useEffect } from 'react';
import { Box, Button, useTheme, Tooltip } from '@mui/material';
import { ArrowUp, ArrowDown, ArrowLeft, ArrowRight } from 'lucide-react';

// Mapping from direction string to internal numeric representation
const directionToNumber = (directionString) => {
    const mapping = {
        "up": 0,
        "right": 1,
        "down": 2,
        "left": 3
    };

    return mapping[directionString] ?? 2; // Default to "down"
};

// Component for 4-directional picker (cardinal directions)
function JenDirection4({ json }) {
    const theme = useTheme();
    const [direction, setDirection] = useState(() =>
        directionToNumber(json?.value ?? "down")
    );

    // Update module state when direction changes
    const handleDirectionChange = (newDirection) => {
        setDirection(newDirection);

        if (window.module) {
            window.module.pick_direction4(json.name, newDirection);
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
            {/* Top button */}
            <Box sx={{ display: 'flex', justifyContent: 'center' }}>
                <DirectionButton
                    value={0}
                    icon={<ArrowUp size={16} />}
                    label="Up"
                />
            </Box>

            {/* Middle row with left & right */}
            <Box sx={{ display: 'flex', justifyContent: 'space-between', my: 0.5 }}>
                <DirectionButton
                    value={3}
                    icon={<ArrowLeft size={16} />}
                    label="Left"
                />
                <Box sx={{ width: 36, height: 36 }} /> {/* Empty center */}
                <DirectionButton
                    value={1}
                    icon={<ArrowRight size={16} />}
                    label="Right"
                />
            </Box>

            {/* Bottom button */}
            <Box sx={{ display: 'flex', justifyContent: 'center' }}>
                <DirectionButton
                    value={2}
                    icon={<ArrowDown size={16} />}
                    label="Down"
                />
            </Box>
        </Box>
    );
}

export default JenDirection4;