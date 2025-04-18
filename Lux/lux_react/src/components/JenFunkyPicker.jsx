import React, { useState, useEffect } from 'react';
import { Box, ButtonBase, Tooltip, useTheme, Typography } from '@mui/material';

function JenFunkyPicker({ json }) {
    const theme = useTheme();
    const [funky, setFunky] = useState(() => {
        try {
            if (json.value) {
                if (typeof json.value === 'string' && json.value.startsWith('0x')) {
                    return BigInt(json.value);
                } else {
                    return BigInt(json.value);
                }
            }
            return BigInt("0xffffffffaa00aa00"); // Default value
        } catch (error) {
            console.error("Error parsing funky value:", error);
            return BigInt("0xffffffffaa00aa00"); // Fallback default
        }
    });

    // Update module state when pattern changes
    useEffect(() => {
        if (window.module) {
            // Pass value as hexadecimal string
            window.module.pick_funk_factor(json.name, '0x' + funky.toString(16));
        }
    }, [funky, json.name]);

    // Toggle a specific bit in the pattern
    const funkItUp = (index) => {
        setFunky((prevFunky) => prevFunky ^ (BigInt(1) << BigInt(index)));
    };

    // Render a single bit cell
    const renderCell = (index) => {
        const isBitSet = (funky & (BigInt(1) << BigInt(index))) !== BigInt(0);
        const row = Math.floor(index / 8);
        const col = index % 8;

        // Create a tooltip label with row/column info
        const tooltipLabel = `Cell (${row}, ${col})`;

        return (
            <Tooltip key={index} title={tooltipLabel} arrow placement="top">
                <ButtonBase
                    onClick={() => funkItUp(index)}
                    sx={{
                        width: 24,
                        height: 24,
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
            </Tooltip>
        );
    };

    // Render control buttons for common patterns
    const renderPresetControls = () => {
        // Preset patterns
        const presets = [
            { name: "Clear", value: BigInt(0) },
            { name: "All", value: BigInt("0xFFFFFFFFFFFFFFFF") },
            { name: "Checkered", value: BigInt("0xAA55AA55AA55AA55") },
            { name: "Stripes", value: BigInt("0xFF00FF00FF00FF00") }
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
                            onClick={() => setFunky(preset.value)}
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
                p: 1.5,
                backgroundColor: theme.palette.background.paper,
            }}
        >
            {/* Grid container */}
            <Box
                sx={{
                    display: 'grid',
                    gridTemplateColumns: 'repeat(8, 1fr)',
                    gridTemplateRows: 'repeat(8, 1fr)',
                    gap: 0,
                    mx: 'auto',
                    width: 'fit-content',
                }}
            >
                {Array.from({ length: 64 }, (_, index) => renderCell(index))}
            </Box>

            {/* Preset patterns */}
            {renderPresetControls()}
        </Box>
    );
}

export default JenFunkyPicker;