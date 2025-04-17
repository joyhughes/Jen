import React, { useState, useEffect } from 'react';
import {
    Box,
    ButtonGroup,
    Button,
    Tooltip,
    Fade,
    useTheme
} from '@mui/material';
import {
    RotateCcw,
    SkipForward,
    Play,
    Pause
} from 'lucide-react';

// Add a prop to specify when it's being used as an overlay
function MediaController({ isOverlay = false }) {
    const theme = useTheme();
    const [isRunning, setIsRunning] = useState(true);

    const handleRestart = () => {
        if (window.module) {
            window.module.restart();
        }
    };

    const handleAdvance = () => {
        if (window.module) {
            setIsRunning(false);
            window.module.advance_frame();
        }
    };

    const handleRunPause = () => {
        if (window.module) {
            const newState = !isRunning;
            setIsRunning(newState);
            window.module.run_pause();
        }
    };

    // Determine styles based on whether it's an overlay or not
    const containerStyles = isOverlay ? {
        position: 'absolute',
        bottom: 16,
        left: '50%',
        transform: 'translateX(-50%)',
        borderRadius: 20,
        backgroundColor: 'rgba(0, 0, 0, 0.6)',
        backdropFilter: 'blur(4px)',
        padding: '4px 8px',
        boxShadow: theme.shadows[5],
        zIndex: 100,
        opacity: 1,
        transition: 'opacity 0.3s ease'
    } : {
        display: 'flex',
        flexDirection: 'row',
        alignItems: 'center',
        justifyContent: 'space-between',
        p: 1,
        borderRadius: 1.5,
        bgcolor: theme.palette.background.paper,
        border: `1px solid ${theme.palette.divider}`,
        width: '100%'
    };

    const buttonSize = isOverlay ? {
        width: '32px',
        height: '32px'
    } : {
        width: '40px',
        height: '40px'
    };

    return (
        <Box sx={containerStyles}>
            <ButtonGroup
                variant={isOverlay ? "text" : "outlined"}
                size="small"
                sx={{
                    '& .MuiButton-root': {
                        ...buttonSize,
                        minWidth: 0,
                        color: isOverlay ? 'white' : theme.palette.text.secondary,
                        borderColor: isOverlay ? 'transparent' : theme.palette.divider,
                        '&:hover': {
                            bgcolor: isOverlay ? 'rgba(255, 255, 255, 0.1)' : theme.palette.action.hover,
                            borderColor: isOverlay ? 'transparent' : theme.palette.divider,
                        },
                    }
                }}
            >
                <Tooltip title="Restart" arrow placement="top">
                    <Button onClick={handleRestart}>
                        <RotateCcw size={isOverlay ? 14 : 16} />
                    </Button>
                </Tooltip>

                <Tooltip title="Advance Frame" arrow placement="top">
                    <Button onClick={handleAdvance}>
                        <SkipForward size={isOverlay ? 14 : 16} />
                    </Button>
                </Tooltip>

                <Tooltip title={isRunning ? "Pause" : "Play"} arrow placement="top">
                    <Button
                        onClick={handleRunPause}
                        sx={{
                            color: isRunning ?
                                (isOverlay ? 'white' : theme.palette.primary.main) :
                                (isOverlay ? 'white' : theme.palette.text.secondary),
                            borderColor: isRunning ?
                                (isOverlay ? 'transparent' : theme.palette.primary.main) :
                                (isOverlay ? 'transparent' : theme.palette.divider),
                        }}
                    >
                        {isRunning ? <Pause size={isOverlay ? 14 : 16} /> : <Play size={isOverlay ? 14 : 16} />}
                    </Button>
                </Tooltip>
            </ButtonGroup>
        </Box>
    );
}

export default MediaController;