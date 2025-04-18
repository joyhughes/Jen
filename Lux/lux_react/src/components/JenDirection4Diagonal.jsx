import React, { useState, useEffect } from 'react';
import { Box, Button, useTheme, Tooltip } from '@mui/material';
import {
    ArrowUpLeft,
    ArrowUpRight,
    ArrowDownLeft,
    ArrowDownRight
} from 'lucide-react';

// Mapping from direction string to internal numeric representation
const directionToNumber = (directionString) => {
    const mapping = {
        "up_right": 0,
        "down_right": 1,
        "down_left": 2,
        "up_left": 3
    };

    return mapping[directionString] ?? 0; // Default to "up_right"
};

// Component for 4-directional diagonal picker
function JenDirection4Diagonal({ json }) {
    const theme = useTheme();
    const [direction, setDirection] = useState(() =>
        directionToNumber(json?.value ?? "up_right")
    );

    // Update module state when direction changes
    const handleDirectionChange = (newDirection) => {
        setDirection(newDirection);

        if (window.module) {
            window.module.pick_direction4_diagonal(json.name, newDirection);
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
                    value={3}
                    icon={<ArrowUpLeft size={16} />}
                    label="Up-Left"
                />
                <Box sx={{ width: 36 }} />
                <DirectionButton
                    value={0}
                    icon={<ArrowUpRight size={16} />}
                    label="Up-Right"
                />
            </Box>

            {/* Empty middle row for visual balance */}
            <Box sx={{ height: 36 }} />

            {/* Bottom row */}
            <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                <DirectionButton
                    value={2}
                    icon={<ArrowDownLeft size={16} />}
                    label="Down-Left"
                />
                <Box sx={{ width: 36 }} />
                <DirectionButton
                    value={1}
                    icon={<ArrowDownRight size={16} />}
                    label="Down-Right"
                />
            </Box>
        </Box>
    );
}

export default JenDirection4Diagonal;