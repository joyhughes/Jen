import React, { useState, useEffect } from 'react';
import {
    ButtonGroup,
    Button,
    Tooltip,
    Box,
    Typography,
    IconButton,
    Divider,
    useTheme
} from '@mui/material';
import {
    RotateCcw as RestartIcon,
    SkipForward as FrameIcon,
    Play as PlayIcon,
    Pause as PauseIcon,
    Download as DownloadIcon,
    Image as ScreenshotIcon
} from 'lucide-react';

function JenMediaController({ panelSize }) {
    const theme = useTheme();
    const [isRunning, setIsRunning] = useState(true);
    const [frameCount, setFrameCount] = useState(0);
    const [fps, setFps] = useState(0);

    // Update frame count and FPS from WebAssembly module
    useEffect(() => {
        if (!window.module) return;

        // Poll for frame count and FPS updates
        const intervalId = setInterval(() => {
            if (window.module) {
                try {
                    // Assuming these functions exist in your WebAssembly module
                    if (typeof window.module.get_frame_count === 'function') {
                        setFrameCount(window.module.get_frame_count());
                    }

                    if (typeof window.module.get_fps === 'function') {
                        setFps(window.module.get_fps());
                    }
                } catch (error) {
                    console.error("Error getting media stats:", error);
                }
            }
        }, 500); // Update every half second

        return () => clearInterval(intervalId);
    }, []);

    const handleRestart = () => {
        if (window.module) {
            try {
                window.module.restart();
                // Optionally reset frame count if your module doesn't do it automatically
                setFrameCount(0);
            } catch (error) {
                console.error("Error restarting:", error);
            }
        }
    };

    const handleAdvance = () => {
        if (window.module) {
            try {
                setIsRunning(false);
                window.module.advance_frame();
            } catch (error) {
                console.error("Error advancing frame:", error);
            }
        }
    };

    const handleRunPause = () => {
        if (window.module) {
            try {
                setIsRunning(!isRunning);
                window.module.run_pause();
            } catch (error) {
                console.error("Error toggling run/pause:", error);
            }
        }
    };

    const handleScreenshot = () => {
        if (window.module && typeof window.module.take_screenshot === 'function') {
            try {
                window.module.take_screenshot();
            } catch (error) {
                console.error("Error taking screenshot:", error);
            }
        }
    };

    const handleExportImage = () => {
        if (window.module && typeof window.module.export_high_res === 'function') {
            try {
                window.module.export_high_res();
            } catch (error) {
                console.error("Error exporting high-res image:", error);
            }
        }
    };

    return (
        <Box>
            <Typography
                variant="subtitle2"
                fontWeight={600}
                sx={{ mt: 2.5 }}
            >
                Playback Controls
            </Typography>

            <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
                {/* Main media controls */}
                <ButtonGroup
                    variant="contained"
                    size="small"
                    sx={{
                        '& .MuiButton-root': {
                            px: 1.5,
                            py: 0.75,
                            minWidth: 'auto'
                        }
                    }}
                >
                    <Tooltip title="Restart" arrow placement="top">
                        <Button
                            onClick={handleRestart}
                            color="secondary"
                        >
                            <RestartIcon size={18} />
                        </Button>
                    </Tooltip>

                    <Tooltip title="Advance One Frame" arrow placement="top">
                        <Button
                            onClick={handleAdvance}
                            variant="outlined"
                            color="primary"
                        >
                            <FrameIcon size={18} />
                        </Button>
                    </Tooltip>

                    <Tooltip title={isRunning ? "Pause" : "Play"} arrow placement="top">
                        <Button
                            onClick={handleRunPause}
                            color="primary"
                            variant={isRunning ? "contained" : "outlined"}
                        >
                            {isRunning ? <PauseIcon size={18} /> : <PlayIcon size={18} />}
                        </Button>
                    </Tooltip>
                </ButtonGroup>

                {/* Stats display */}
                <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                    <Tooltip title="Current frame" arrow placement="top">
                        <Typography
                            variant="body2"
                            sx={{
                                px: 1.5,
                                py: 0.5,
                                borderRadius: 1,
                                backgroundColor: theme.palette.action.hover,
                                fontFamily: 'monospace'
                            }}
                        >
                            Frame: {frameCount}
                        </Typography>
                    </Tooltip>

                    <Tooltip title="Frames per second" arrow placement="top">
                        <Typography
                            variant="body2"
                            sx={{
                                px: 1.5,
                                py: 0.5,
                                borderRadius: 1,
                                backgroundColor: theme.palette.action.hover,
                                fontFamily: 'monospace'
                            }}
                        >
                            {fps.toFixed(1)} FPS
                        </Typography>
                    </Tooltip>
                </Box>
            </Box>

            {/* Export controls */}
            <Box sx={{ display: 'flex', justifyContent: 'flex-end', mt: 2 }}>
                <ButtonGroup size="small" variant="outlined">
                    <Tooltip title="Take Screenshot" arrow placement="top">
                        <Button onClick={handleScreenshot}>
                            <ScreenshotIcon size={16} />
                            <Typography variant="caption" sx={{ ml: 0.5 }}>
                                Screenshot
                            </Typography>
                        </Button>
                    </Tooltip>

                    <Tooltip title="Export High Resolution" arrow placement="top">
                        <Button onClick={handleExportImage}>
                            <DownloadIcon size={16} />
                            <Typography variant="caption" sx={{ ml: 0.5 }}>
                                Export
                            </Typography>
                        </Button>
                    </Tooltip>
                </ButtonGroup>
            </Box>
        </Box>
    );
}

export default JenMediaController;