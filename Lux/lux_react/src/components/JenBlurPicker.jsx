import React, { useState, useEffect } from 'react';
import { Box, ToggleButtonGroup, ToggleButton, Tooltip, useTheme } from '@mui/material';
import { Plus, X, Star } from 'lucide-react';

// Map blur method strings to numeric values for internal use
const blurMethodToNumber = (blurString) => {
    const mapping = {
        "orthogonal": 0,
        "diagonal": 1,
        "all": 2
    };

    return mapping[blurString] ?? 0; // Default to "orthogonal"
};

// Component for blur direction picker
function JenBlurPicker({ json }) {
    const theme = useTheme();
    const [blurMethod, setBlurMethod] = useState(() =>
        blurMethodToNumber(json?.value ?? "orthogonal")
    );

    // Update module state when blur method changes
    const handleBlurMethodChange = (event, newMethod) => {
        // Prevent deselection (null value)
        if (newMethod === null) return;

        setBlurMethod(newMethod);

        if (window.module) {
            window.module.pick_blur_method(json.name, newMethod);
        }
    };

    return (
        <Box>
            <ToggleButtonGroup
                value={blurMethod}
                exclusive
                onChange={handleBlurMethodChange}
                aria-label="blur method"
                size="small"
                sx={{
                    '& .MuiToggleButtonGroup-grouped': {
                        border: 1,
                        borderColor: theme.palette.divider,
                    },
                }}
            >
                <Tooltip title="Orthogonal (Plus)" arrow placement="top">
                    <ToggleButton
                        value={0}
                        aria-label="orthogonal"
                        sx={{
                            width: 32,
                            height: 32,
                            color: blurMethod === 0 ? 'white' : theme.palette.text.secondary,
                            bgcolor: blurMethod === 0 ? theme.palette.primary.main : 'transparent',
                            '&:hover': {
                                bgcolor: blurMethod === 0
                                    ? theme.palette.primary.dark
                                    : theme.palette.action.hover,
                            },
                            '&.Mui-selected': {
                                bgcolor: theme.palette.primary.main,
                                '&:hover': {
                                    bgcolor: theme.palette.primary.dark,
                                },
                            },
                        }}
                    >
                        <Plus size={16} />
                    </ToggleButton>
                </Tooltip>

                <Tooltip title="Diagonal (Cross)" arrow placement="top">
                    <ToggleButton
                        value={1}
                        aria-label="diagonal"
                        sx={{
                            width: 32,
                            height: 32,
                            color: blurMethod === 1 ? 'white' : theme.palette.text.secondary,
                            bgcolor: blurMethod === 1 ? theme.palette.primary.main : 'transparent',
                            '&:hover': {
                                bgcolor: blurMethod === 1
                                    ? theme.palette.primary.dark
                                    : theme.palette.action.hover,
                            },
                            '&.Mui-selected': {
                                bgcolor: theme.palette.primary.main,
                                '&:hover': {
                                    bgcolor: theme.palette.primary.dark,
                                },
                            },
                        }}
                    >
                        <X size={16} />
                    </ToggleButton>
                </Tooltip>

                <Tooltip title="All directions (Star)" arrow placement="top">
                    <ToggleButton
                        value={2}
                        aria-label="all"
                        sx={{
                            width: 32,
                            height: 32,
                            color: blurMethod === 2 ? 'white' : theme.palette.text.secondary,
                            bgcolor: blurMethod === 2 ? theme.palette.primary.main : 'transparent',
                            '&:hover': {
                                bgcolor: blurMethod === 2
                                    ? theme.palette.primary.dark
                                    : theme.palette.action.hover,
                            },
                            '&.Mui-selected': {
                                bgcolor: theme.palette.primary.main,
                                '&:hover': {
                                    bgcolor: theme.palette.primary.dark,
                                },
                            },
                        }}
                    >
                        <Star size={16} />
                    </ToggleButton>
                </Tooltip>
            </ToggleButtonGroup>
        </Box>
    );
}

export default JenBlurPicker;