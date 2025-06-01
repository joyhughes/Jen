import React, { useState } from 'react';
import { Box, Button, useTheme, Tooltip } from '@mui/material';
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

// Component for 8-directional multiple selection picker
function JenMultiDirection8({ name, value, code }) {
    const theme = useTheme();
    const [directionsByte, setDirectionsByte] = useState(value ?? 0);

    // Toggle a particular direction bit
    const handleDirectionChange = (dirIndex) => {
        // Toggles the selected direction's bit
        const updatedDirectionsByte = directionsByte ^ (1 << dirIndex);
        setDirectionsByte(updatedDirectionsByte);

        // Call the external function with the updated byte value
        if (window.module) {
            window.module.pick_multi_direction8(name, updatedDirectionsByte, code ?? 0);
        }
    };

    // Check if a direction is active
    const isDirectionActive = (dirIndex) => {
        return (directionsByte & (1 << dirIndex)) !== 0;
    };

    // Direction button component
    const DirectionButton = ({ index, icon, label }) => {
        const isActive = isDirectionActive(index);

        return (
            <Tooltip title={label} arrow placement="top">
                <Button
                    variant={isActive ? "contained" : "outlined"}
                    onClick={() => handleDirectionChange(index)}
                    sx={{
                        minWidth: 26,
                        width: 26,
                        height: 26,
                        padding: 0,
                        borderColor: isActive ? 'transparent' : theme.palette.divider,
                        color: isActive ? 'white' : theme.palette.text.secondary,
                        boxShadow: isActive ? 1 : 0,
                        transition: 'all 0.15s',
                        '&:hover': {
                            backgroundColor: isActive
                                ? theme.palette.primary.main
                                : theme.palette.action.hover,
                            borderColor: isActive
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
        <Box
            sx={{
                border: `1px solid ${theme.palette.divider}`,
                borderRadius: 1,
                p: 0.5,
                background: theme.palette.background.paper,
            }}
        >
            <Box sx={{ display: 'flex', flexDirection: 'column' }}>
                {/* Top row */}
                <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                    <DirectionButton
                        index={7}
                        icon={<ArrowUpLeft size={14} />}
                        label="Up-Left"
                    />
                    <DirectionButton
                        index={0}
                        icon={<ArrowUp size={14} />}
                        label="Up"
                    />
                    <DirectionButton
                        index={1}
                        icon={<ArrowUpRight size={14} />}
                        label="Up-Right"
                    />
                </Box>

                {/* Middle row */}
                <Box sx={{ display: 'flex', justifyContent: 'space-between', my: 0.5 }}>
                    <DirectionButton
                        index={6}
                        icon={<ArrowLeft size={14} />}
                        label="Left"
                    />
                    <Box sx={{ width: 26, height: 26 }} /> {/* Empty center */}
                    <DirectionButton
                        index={2}
                        icon={<ArrowRight size={14} />}
                        label="Right"
                    />
                </Box>

                {/* Bottom row */}
                <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                    <DirectionButton
                        index={5}
                        icon={<ArrowDownLeft size={14} />}
                        label="Down-Left"
                    />
                    <DirectionButton
                        index={4}
                        icon={<ArrowDown size={14} />}
                        label="Down"
                    />
                    <DirectionButton
                        index={3}
                        icon={<ArrowDownRight size={14} />}
                        label="Down-Right"
                    />
                </Box>
            </Box>
        </Box>
    );
}

export default JenMultiDirection8;