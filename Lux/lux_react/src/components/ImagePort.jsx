import React, { useState, useEffect, useRef } from "react";
import {
    Box,
    Paper,
    Typography,
    CircularProgress,
    Fade,
    IconButton,
    Tooltip,
    Zoom,
    useTheme
} from '@mui/material';
import {
    Maximize2,
    ZoomIn,
    ZoomOut,
    RotateCw,
    Download,
    RefreshCw,
    Info
} from 'lucide-react';

import ImagePortCanvas from './ImagePortCanvas.jsx';

function ImagePort({ dimensions, moduleReady }) {
    const theme = useTheme();
    const imagePortRef = useRef(null);
    const [isFullscreen, setIsFullscreen] = useState(false);
    const [showControls, setShowControls] = useState(false);
    const [showInfo, setShowInfo] = useState(false);
    const [imageInfo, setImageInfo] = useState({
        resolution: '512×512',
        renderTime: '0ms',
        fps: '60',
        frameCount: '0'
    });

    // Track mouse movement to show/hide controls
    const mouseTimerRef = useRef(null);

    // Update image information periodically if functions are available
    useEffect(() => {
        if (!window.module || !moduleReady) return;

        const updateInfo = () => {
            const info = {};

            try {
                // These are hypothetical functions your module might provide
                if (typeof window.module.get_buf_width === 'function' &&
                    typeof window.module.get_buf_height === 'function') {
                    const width = window.module.get_buf_width();
                    const height = window.module.get_buf_height();
                    info.resolution = `${width}×${height}`;
                }

                if (typeof window.module.get_render_time === 'function') {
                    info.renderTime = `${window.module.get_render_time().toFixed(2)}ms`;
                }

                if (typeof window.module.get_fps === 'function') {
                    info.fps = window.module.get_fps().toFixed(1);
                } else {
                    // Fallback if get_fps doesn't exist
                    info.fps = '60.0';
                }

                if (typeof window.module.get_frame_count === 'function') {
                    info.frameCount = window.module.get_frame_count().toString();
                } else {
                    // Fallback if get_frame_count doesn't exist
                    const frameCountElement = document.querySelector('.frame-count');
                    if (frameCountElement) {
                        info.frameCount = frameCountElement.textContent.trim();
                    }
                }

                setImageInfo(prevInfo => ({ ...prevInfo, ...info }));
            } catch (error) {
                console.error("Error updating image info:", error);
            }
        };

        // Update info immediately and set up interval
        updateInfo();
        const intervalId = setInterval(updateInfo, 1000);

        return () => clearInterval(intervalId);
    }, [moduleReady]);

    // Handle mouse movement to show/hide controls
    const handleMouseMove = () => {
        setShowControls(true);

        // Clear existing timer
        if (mouseTimerRef.current) {
            clearTimeout(mouseTimerRef.current);
        }

        // Set new timer to hide controls after inactivity
        mouseTimerRef.current = setTimeout(() => {
            setShowControls(false);
        }, 3000);
    };

    // Cleanup timer on unmount
    useEffect(() => {
        return () => {
            if (mouseTimerRef.current) {
                clearTimeout(mouseTimerRef.current);
            }
        };
    }, []);

    // Toggle fullscreen mode
    const toggleFullscreen = () => {
        if (!imagePortRef.current) return;

        if (!isFullscreen) {
            if (imagePortRef.current.requestFullscreen) {
                imagePortRef.current.requestFullscreen();
            } else if (imagePortRef.current.webkitRequestFullscreen) {
                imagePortRef.current.webkitRequestFullscreen();
            } else if (imagePortRef.current.msRequestFullscreen) {
                imagePortRef.current.msRequestFullscreen();
            }
        } else {
            if (document.exitFullscreen) {
                document.exitFullscreen();
            } else if (document.webkitExitFullscreen) {
                document.webkitExitFullscreen();
            } else if (document.msExitFullscreen) {
                document.msExitFullscreen();
            }
        }
    };

    // Listen for fullscreen change events
    useEffect(() => {
        const handleFullscreenChange = () => {
            setIsFullscreen(
                document.fullscreenElement ||
                document.webkitFullscreenElement ||
                document.msFullscreenElement
            );
        };

        document.addEventListener('fullscreenchange', handleFullscreenChange);
        document.addEventListener('webkitfullscreenchange', handleFullscreenChange);
        document.addEventListener('msfullscreenchange', handleFullscreenChange);

        return () => {
            document.removeEventListener('fullscreenchange', handleFullscreenChange);
            document.removeEventListener('webkitfullscreenchange', handleFullscreenChange);
            document.removeEventListener('msfullscreenchange', handleFullscreenChange);
        };
    }, []);

    // Zoom in/out functions
    const handleZoomIn = () => {
        if (window.module && typeof window.module.zoom_in === 'function') {
            window.module.zoom_in();
        }
    };

    const handleZoomOut = () => {
        if (window.module && typeof window.module.zoom_out === 'function') {
            window.module.zoom_out();
        }
    };

    // Rotation function
    const handleRotate = () => {
        if (window.module && typeof window.module.rotate_view === 'function') {
            window.module.rotate_view();
        }
    };

    // Screenshot/export function
    const handleScreenshot = () => {
        if (window.module && typeof window.module.take_screenshot === 'function') {
            window.module.take_screenshot();
        }
    };

    // Reset view function
    const handleResetView = () => {
        if (window.module && typeof window.module.reset_view === 'function') {
            window.module.reset_view();
        }
    };

    // Toggle info display
    const toggleInfo = () => {
        setShowInfo(!showInfo);
    };

    return (
        <Paper
            ref={imagePortRef}
            elevation={0}
            sx={{
                width: dimensions.width,
                height: dimensions.height,
                borderRadius: 2,
                overflow: 'hidden',
                position: 'relative',
                boxShadow: theme.shadows[4],
                backgroundColor: theme.palette.background.paper,
                transition: 'all 0.3s ease-in-out',
                '&:hover': {
                    boxShadow: theme.shadows[8]
                }
            }}
            onMouseMove={handleMouseMove}
        >
            {/* Use your existing ImagePortCanvas component */}
            <Box
                sx={{
                    width: '100%',
                    height: '100%',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    overflow: 'hidden'
                }}
            >
                <ImagePortCanvas
                    width={dimensions.width}
                    height={dimensions.height}
                />
            </Box>

            {/* Floating controls */}
            <Fade in={showControls}>
                <Box
                    sx={{
                        position: 'absolute',
                        top: 16,
                        right: 16,
                        display: 'flex',
                        flexDirection: 'column',
                        gap: 1,
                        zIndex: 10
                    }}
                >
                    <Paper
                        elevation={3}
                        sx={{
                            borderRadius: 5,
                            overflow: 'hidden',
                            backgroundColor: 'rgba(33, 33, 33, 0.75)',
                            backdropFilter: 'blur(8px)',
                        }}
                    >
                        <Box sx={{ display: 'flex', flexDirection: 'column', p: 0.5 }}>
                            <Tooltip title="Toggle fullscreen" placement="left" arrow>
                                <IconButton
                                    onClick={toggleFullscreen}
                                    sx={{ color: 'white', '&:hover': { color: theme.palette.primary.light } }}
                                    size="small"
                                >
                                    <Maximize2 size={18} />
                                </IconButton>
                            </Tooltip>

                            <Tooltip title="Zoom in" placement="left" arrow>
                                <IconButton
                                    onClick={handleZoomIn}
                                    sx={{ color: 'white', '&:hover': { color: theme.palette.primary.light } }}
                                    size="small"
                                >
                                    <ZoomIn size={18} />
                                </IconButton>
                            </Tooltip>

                            <Tooltip title="Zoom out" placement="left" arrow>
                                <IconButton
                                    onClick={handleZoomOut}
                                    sx={{ color: 'white', '&:hover': { color: theme.palette.primary.light } }}
                                    size="small"
                                >
                                    <ZoomOut size={18} />
                                </IconButton>
                            </Tooltip>

                            <Tooltip title="Rotate view" placement="left" arrow>
                                <IconButton
                                    onClick={handleRotate}
                                    sx={{ color: 'white', '&:hover': { color: theme.palette.primary.light } }}
                                    size="small"
                                >
                                    <RotateCw size={18} />
                                </IconButton>
                            </Tooltip>

                            <Tooltip title="Reset view" placement="left" arrow>
                                <IconButton
                                    onClick={handleResetView}
                                    sx={{ color: 'white', '&:hover': { color: theme.palette.primary.light } }}
                                    size="small"
                                >
                                    <RefreshCw size={18} />
                                </IconButton>
                            </Tooltip>

                            <Box sx={{ mx: 0.5, my: 0.25, height: '1px', bgcolor: 'rgba(255, 255, 255, 0.2)' }} />

                            <Tooltip title="Take screenshot" placement="left" arrow>
                                <IconButton
                                    onClick={handleScreenshot}
                                    sx={{ color: 'white', '&:hover': { color: theme.palette.primary.light } }}
                                    size="small"
                                >
                                    <Download size={18} />
                                </IconButton>
                            </Tooltip>

                            <Tooltip title="Show info" placement="left" arrow>
                                <IconButton
                                    onClick={toggleInfo}
                                    sx={{
                                        color: showInfo ? theme.palette.primary.main : 'white',
                                        '&:hover': { color: theme.palette.primary.light }
                                    }}
                                    size="small"
                                >
                                    <Info size={18} />
                                </IconButton>
                            </Tooltip>
                        </Box>
                    </Paper>
                </Box>
            </Fade>

            {/* Info panel */}
            <Zoom in={showInfo}>
                <Paper
                    elevation={3}
                    sx={{
                        position: 'absolute',
                        bottom: 16,
                        left: 16,
                        borderRadius: 1,
                        p: 1.5,
                        backgroundColor: 'rgba(33, 33, 33, 0.75)',
                        backdropFilter: 'blur(8px)',
                        color: 'white',
                        zIndex: 10,
                        maxWidth: 200
                    }}
                >
                    <Typography variant="subtitle2" sx={{ mb: 1, color: theme.palette.primary.light }}>
                        Render Information
                    </Typography>

                    <Box sx={{ display: 'grid', gridTemplateColumns: 'auto 1fr', gap: '4px 8px' }}>
                        <Typography variant="caption" sx={{ color: 'rgba(255, 255, 255, 0.7)' }}>
                            Resolution:
                        </Typography>
                        <Typography variant="caption" sx={{ fontFamily: 'monospace' }}>
                            {imageInfo.resolution}
                        </Typography>

                        <Typography variant="caption" sx={{ color: 'rgba(255, 255, 255, 0.7)' }}>
                            Render time:
                        </Typography>
                        <Typography variant="caption" sx={{ fontFamily: 'monospace' }}>
                            {imageInfo.renderTime}
                        </Typography>

                        <Typography variant="caption" sx={{ color: 'rgba(255, 255, 255, 0.7)' }}>
                            FPS:
                        </Typography>
                        <Typography variant="caption" sx={{ fontFamily: 'monospace' }}>
                            {imageInfo.fps}
                        </Typography>

                        <Typography variant="caption" sx={{ color: 'rgba(255, 255, 255, 0.7)' }}>
                            Frame:
                        </Typography>
                        <Typography variant="caption" sx={{ fontFamily: 'monospace' }}>
                            {imageInfo.frameCount}
                        </Typography>
                    </Box>
                </Paper>
            </Zoom>
        </Paper>
    );
}

export default ImagePort;