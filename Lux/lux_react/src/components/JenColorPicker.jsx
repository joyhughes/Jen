import React, { useState, useEffect } from 'react';
import { Box, ButtonBase, Tooltip, useTheme, Typography } from '@mui/material';

function JenColorPicker({ json }) {
    const theme = useTheme();
    // Right now returns just a ucolor but can be extended to return frgb
    const [color, setColor] = useState(() => {
        try {
            if (json.value) {
                return typeof json.value === 'string' ? parseInt(json.value, 16) : json.value;
            }
            return (0xff808080); // Default value
        } catch (error) {
            console.error("Error parsing color value:", error);
            return (0xff808080); // Fallback default
        }
    });

    // Update module state when pattern changes
    useEffect(() => {
        if (window.module) {
            // Ensure color is treated as an unsigned 32-bit integer
            const unsignedColor = color >>> 0; // Convert to unsigned
            window.module.pick_ucolor(json.name, unsignedColor);
        }
    }, [color, json.name]);

    // Toggle a specific bit in the pattern
    const setBit = (index) => {
        setColor((prevColor) => prevColor ^ (1 << index));
    };

    // Render a single bit cell
    const renderCell = (index) => {
        const isBitSet = (color & (1 << index)) !== 0;
        const row = Math.floor(index / 8);
        const col = index % 8;

        // Create a tooltip label with row/column info
        const tooltipLabel = `Cell (${row}, ${col})`;

        return (
            <ButtonBase
                onClick={() => setBit(index)}
                sx={{
                    width: 16,
                    height: 16,
                    margin: 0.25,
                    borderRadius: '4px',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    backgroundColor: isBitSet
                        ? theme.palette.primary.main
                        : theme.palette.action.hover,
                    transition: 'all 0.15s ease',
                    border: `1px solid ${isBitSet
                        ? theme.palette.primary.dark
                        : theme.palette.divider}`,
                    boxShadow: isBitSet ? theme.shadows[1] : 'none',
                    opacity: isBitSet ? 1 : 0.6,
                    '&:hover': {
                        backgroundColor: isBitSet
                            ? theme.palette.primary.light
                            : theme.palette.action.selected,
                        transform: 'scale(1.05)',
                    },
                }}
            >
                <Box
                    sx={{
                        width: 10,
                        height: 10,
                        borderRadius: '50%',
                        backgroundColor: isBitSet
                            ? 'white'
                            : 'transparent',
                    }}
                />
            </ButtonBase>
        );
    };

    // Render control buttons for common patterns
    const renderPresetControls = () => {
        // Preset patterns
        const presets = [
            { name: "Default", value: 0xff808080 },
            { name: "Clear",   value: 0x00000000 },
            { name: "All",     value: 0xffffffff },
        ];

        return (
            <Box
                sx={{
                    display: 'flex',
                    justifyContent: 'space-between',
                    mt: 1.5,
                    px: 0.5
                }}
            >
                {presets.map((preset, index) => (
                    <Tooltip key={index} title={preset.name} arrow placement="top">
                        <ButtonBase
                            onClick={() => setColor(preset.value)}
                            sx={{
                                px: 1.5,
                                py: 0.5,
                                borderRadius: 1,
                                fontSize: '0.75rem',
                                color: theme.palette.text.secondary,
                                backgroundColor: theme.palette.action.hover,
                                '&:hover': {
                                    backgroundColor: theme.palette.action.selected,
                                }
                            }}
                        >
                            {preset.name}
                        </ButtonBase>
                    </Tooltip>
                ))}
            </Box>
        );
    };

    return (
        <Box
            sx={{
                width: '100%',
                border: `1px solid ${theme.palette.divider}`,
                borderRadius: 1,
                p: 0.5,
                backgroundColor: theme.palette.background.paper,
            }}
        >
            {/* Title */}
            <Typography
                variant="h6"
                sx={{
                    textAlign: 'center',
                    mb: 0, // Add some margin below the title
                    fontWeight: 500,
                    color: theme.palette.text.primary,
                    fontSize: '0.875rem', // Optional: explicitly set font size
                }}
            >
                {json.label}
            </Typography>

            {/* Grid container */}
            <Box
                sx={{
                    display: 'grid',
                    gridTemplateColumns: 'repeat(8, 1fr)',
                    gridTemplateRows: 'repeat(3, 1fr)',
                    gap: 0,
                    mx: 'auto',
                    width: 'fit-content',
                }}
            >
                {Array.from({ length: 24 }, (_, index) => (
                    <React.Fragment key={index}> {/* Add a unique key */}
                        {renderCell(23 - index)}
                    </React.Fragment>
                ))}
            </Box>

            {/* Preset patterns */}
            {renderPresetControls()}
        </Box>
    );
}

export default JenColorPicker;