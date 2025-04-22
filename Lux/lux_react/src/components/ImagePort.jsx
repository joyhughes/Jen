import React, { useState, useEffect, useRef } from "react";
import {
    Box,
    Paper,
    Typography,
    Fade,
    IconButton,
    Tooltip,
    useTheme,
    Zoom
} from '@mui/material';
import {
    Maximize2,
    ZoomIn,
    ZoomOut,
    RotateCw,
    Download,
    RefreshCw,
    Info,
    X
} from 'lucide-react';

import ImagePortCanvas from './ImagePortCanvas.jsx';
import MediaController from "./MediaController.jsx";

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
    const [showMediaControls, setShowMediaControls] = useState(false);

    const mouseTimerRef = useRef(null);

    // Update image information periodically if functions are available
    useEffect(() => {
        if (!window.module || !moduleReady) return;

        const updateInfo = () => {
            const info = {};

            try {
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
                    info.fps = '60.0';
                }

                if (typeof window.module.get_frame_count === 'function') {
                    info.frameCount = window.module.get_frame_count().toString();
                } else {
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


    const handleMouseEnter = () => {
        setShowMediaControls(true);
    };


    const handleMouseLeave = () => {
        if (!mouseTimerRef.current) {
            setShowMediaControls(false);
        }
    };


    useEffect(() => {
        return () => {
            if (mouseTimerRef.current) {
                clearTimeout(mouseTimerRef.current);
            }
        };
    }, []);

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

    // Image manipulation functions
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

    const handleRotate = () => {
        if (window.module && typeof window.module.rotate_view === 'function') {
            window.module.rotate_view();
        }
    };

    const handleScreenshot = () => {
        if (window.module && typeof window.module.take_screenshot === 'function') {
            window.module.take_screenshot();
        }
    };

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
                borderRadius: isFullscreen ? 0 : 2,
                overflow: 'hidden',
                position: 'relative',
                backgroundColor: 'rgba(18, 18, 18, 0.8)',
                boxShadow: isFullscreen ? 'none' : theme.shadows[8],
                transition: 'all 0.3s ease-in-out',
            }}
            onMouseMove={handleMouseMove}
            onMouseEnter={handleMouseEnter}
            onMouseLeave={handleMouseLeave}>
            {/* Canvas Container */}
            <Box
                sx={{
                    width: '100%',
                    height: '100%',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    overflow: 'hidden',
                }}>
                <ImagePortCanvas width={dimensions.width} height={dimensions.height} />
            </Box>


            {/* Floating Controls - existing side controls */}
            <Fade in={showControls}>
                <Box
                    sx={{
                        position: 'absolute',
                        top: 16,
                        right: 16,
                        display: 'flex',
                        flexDirection: 'column',
                        gap: 1,
                        zIndex: 100,
                    }}>
                    <Paper
                        elevation={4}
                        sx={{
                            borderRadius: '12px',
                            overflow: 'hidden',
                            backgroundColor: 'rgba(28, 28, 30, 0.85)',
                            backdropFilter: 'blur(8px)',
                        }}>
                        <Box sx={{display: 'flex', flexDirection: 'column', p: 0.75}}>
                            <Tooltip title="Toggle fullscreen" placement="left" arrow>
                                <IconButton
                                    onClick={toggleFullscreen}
                                    sx={{
                                        'color': 'white',
                                        '&:hover': {color: theme.palette.primary.light},
                                    }}
                                    size="small">
                                    {isFullscreen ? <X size={16} /> : <Maximize2 size={16} />}
                                </IconButton>
                            </Tooltip>

                            <Tooltip title="Zoom in" placement="left" arrow>
                                <IconButton
                                    onClick={handleZoomIn}
                                    sx={{
                                        'color': 'white',
                                        '&:hover': {color: theme.palette.primary.light},
                                    }}
                                    size="small">
                                    <ZoomIn size={16} />
                                </IconButton>
                            </Tooltip>

                            <Tooltip title="Zoom out" placement="left" arrow>
                                <IconButton
                                    onClick={handleZoomOut}
                                    sx={{
                                        'color': 'white',
                                        '&:hover': {color: theme.palette.primary.light},
                                    }}
                                    size="small">
                                    <ZoomOut size={16} />
                                </IconButton>
                            </Tooltip>

                            <Tooltip title="Rotate view" placement="left" arrow>
                                <IconButton
                                    onClick={handleRotate}
                                    sx={{
                                        'color': 'white',
                                        '&:hover': {color: theme.palette.primary.light},
                                    }}
                                    size="small">
                                    <RotateCw size={16} />
                                </IconButton>
                            </Tooltip>

                            <Tooltip title="Reset view" placement="left" arrow>
                                <IconButton
                                    onClick={handleResetView}
                                    sx={{
                                        'color': 'white',
                                        '&:hover': {color: theme.palette.primary.light},
                                    }}
                                    size="small">
                                    <RefreshCw size={16} />
                                </IconButton>
                            </Tooltip>

                            <Box
                                sx={{
                                    mx: 0.5,
                                    my: 0.25,
                                    height: '1px',
                                    bgcolor: 'rgba(255, 255, 255, 0.2)',
                                }}
                            />

                            <Tooltip title="Take screenshot" placement="left" arrow>
                                <IconButton
                                    onClick={handleScreenshot}
                                    sx={{
                                        'color': 'white',
                                        '&:hover': {color: theme.palette.primary.light},
                                    }}
                                    size="small">
                                    <Download size={16} />
                                </IconButton>
                            </Tooltip>

                            <Tooltip title="Show info" placement="left" arrow>
                                <IconButton
                                    onClick={toggleInfo}
                                    sx={{
                                        'color': showInfo ? theme.palette.primary.main : 'white',
                                        '&:hover': {color: theme.palette.primary.light},
                                    }}
                                    size="small">
                                    <Info size={16} />
                                </IconButton>
                            </Tooltip>
                        </Box>
                    </Paper>
                </Box>
            </Fade>

            {/* Info panel */}
            <Zoom in={showInfo}>
                <Paper
                    elevation={4}
                    sx={{
                        position: 'absolute',
                        bottom: 16,
                        left: 16,
                        borderRadius: 1.5,
                        p: 1.75,
                        backgroundColor: 'rgba(28, 28, 30, 0.85)',
                        backdropFilter: 'blur(10px)',
                        color: 'white',
                        zIndex: 10,
                        maxWidth: 200,
                        boxShadow: '0 4px 20px rgba(0,0,0,0.3)'
                    }}
                >
                    <Typography variant="subtitle2" sx={{ mb: 1, color: theme.palette.primary.light, fontWeight: 600 }}>
                        Render Information
                    </Typography>

                    <Box sx={{ display: 'grid', gridTemplateColumns: 'auto 1fr', gap: '4px 12px' }}>
                        <Typography variant="caption" sx={{ color: 'rgba(255, 255, 255, 0.7)' }}>
                            Resolution:
                        </Typography>
                        <Typography variant="caption" sx={{ fontFamily: 'monospace', fontWeight: 500 }}>
                            {imageInfo.resolution}
                        </Typography>

                        <Typography variant="caption" sx={{ color: 'rgba(255, 255, 255, 0.7)' }}>
                            Render time:
                        </Typography>
                        <Typography variant="caption" sx={{ fontFamily: 'monospace', fontWeight: 500 }}>
                            {imageInfo.renderTime}
                        </Typography>

                        <Typography variant="caption" sx={{ color: 'rgba(255, 255, 255, 0.7)' }}>
                            FPS:
                        </Typography>
                        <Typography variant="caption" sx={{ fontFamily: 'monospace', fontWeight: 500 }}>
                            {imageInfo.fps}
                        </Typography>

                        <Typography variant="caption" sx={{ color: 'rgba(255, 255, 255, 0.7)' }}>
                            Frame:
                        </Typography>
                        <Typography variant="caption" sx={{ fontFamily: 'monospace', fontWeight: 500 }}>
                            {imageInfo.frameCount}
                        </Typography>
                    </Box>
                </Paper>


            </Zoom>
        </Paper>
    )

}

export default ImagePort;