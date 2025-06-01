import React, { useState, useEffect, useRef } from "react";
import {
    Box,
    Paper,
    Typography,
    Fade,
    IconButton,
    Tooltip,
    useTheme,
    Zoom,
    Fab,
    Switch,
    FormControlLabel,
    Slider,
    Alert,
    Dialog
} from '@mui/material';
import {
    Maximize2,
    ZoomIn,
    ZoomOut,
    RotateCw,
    Download,
    RefreshCw,
    Info,
    X,
    Camera,
    CameraOff,
    Settings,
    Zap,
    ZapOff
} from 'lucide-react';

import ImagePortCanvas from './ImagePortCanvas.jsx';
import MediaController from "./MediaController.jsx";

function ImagePort({ dimensions, moduleReady }) {
    const theme = useTheme();
    const imagePortRef = useRef(null);
    const mouseTimerRef = useRef(null);

    // UI State
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

    // Toggle fullscreen mode with comprehensive mobile support
    const toggleFullscreen = async () => {
        if (!imagePortRef.current) return;

        try {
            if (!isFullscreen) {
                // Enter fullscreen
                const element = imagePortRef.current;
                
                if (element.requestFullscreen) {
                    await element.requestFullscreen();
                } else if (element.webkitRequestFullscreen) {
                    // Safari
                    await element.webkitRequestFullscreen();
                } else if (element.webkitRequestFullScreen) {
                    // Older Safari
                    await element.webkitRequestFullScreen();
                } else if (element.mozRequestFullScreen) {
                    // Firefox
                    await element.mozRequestFullScreen();
                } else if (element.msRequestFullscreen) {
                    // IE/Edge
                    await element.msRequestFullscreen();
                } else {
                    // Fallback: Try to maximize the element visually
                    console.warn('Fullscreen API not supported, using fallback');
                    setIsFullscreen(true);
                }
            } else {
                // Exit fullscreen
                if (document.exitFullscreen) {
                    await document.exitFullscreen();
                } else if (document.webkitExitFullscreen) {
                    await document.webkitExitFullscreen();
                } else if (document.webkitCancelFullScreen) {
                    await document.webkitCancelFullScreen();
                } else if (document.mozCancelFullScreen) {
                    await document.mozCancelFullScreen();
                } else if (document.msExitFullscreen) {
                    await document.msExitFullscreen();
                } else {
                    // Fallback
                    setIsFullscreen(false);
                }
            }
        } catch (error) {
            console.warn('Fullscreen operation failed:', error);
            // On mobile, some browsers may reject fullscreen requests
            // Try visual fullscreen as fallback
            setIsFullscreen(!isFullscreen);
        }
    };

    // Listen for fullscreen change events with comprehensive support
    useEffect(() => {
        const handleFullscreenChange = () => {
            const isCurrentlyFullscreen = !!(
                document.fullscreenElement ||
                document.webkitFullscreenElement ||
                document.webkitCurrentFullScreenElement ||
                document.mozFullScreenElement ||
                document.msFullscreenElement
            );
            setIsFullscreen(isCurrentlyFullscreen);
        };

        // Add all possible fullscreen event listeners
        const events = [
            'fullscreenchange',
            'webkitfullscreenchange',
            'mozfullscreenchange',
            'msfullscreenchange'
        ];

        events.forEach(event => {
            document.addEventListener(event, handleFullscreenChange);
        });

        return () => {
            events.forEach(event => {
                document.removeEventListener(event, handleFullscreenChange);
            });
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

    const handleMouseEnter = () => {
        setShowMediaControls(true);
    };

    const handleMouseLeave = () => {
        if (!mouseTimerRef.current) {
            setShowMediaControls(false);
        }
    };

    // Add touch handling for mobile
    const handleTouchStart = () => {
        setShowControls(true);
        if (mouseTimerRef.current) {
            clearTimeout(mouseTimerRef.current);
        }
        // Auto-hide controls after 4 seconds on mobile
        mouseTimerRef.current = setTimeout(() => {
            setShowControls(false);
        }, 4000);
    };

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

    // Handle window resize for fullscreen mode (important for mobile orientation changes)
    useEffect(() => {
        const handleResize = () => {
            if (isFullscreen) {
                // Force re-render of canvas with new dimensions
                // This is important for mobile when orientation changes
                const event = new CustomEvent('forceRedraw');
                const canvas = document.querySelector('[data-engine="true"]');
                if (canvas) {
                    canvas.dispatchEvent(event);
                }
            }
        };

        window.addEventListener('resize', handleResize);
        window.addEventListener('orientationchange', handleResize);

        return () => {
            window.removeEventListener('resize', handleResize);
            window.removeEventListener('orientationchange', handleResize);
        };
    }, [isFullscreen]);

    // Cleanup timer on unmount
    useEffect(() => {
        return () => {
            if (mouseTimerRef.current) {
                clearTimeout(mouseTimerRef.current);
            }
        };
    }, []);

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
                // Mobile fullscreen optimizations
                ...(isFullscreen && {
                    position: 'fixed !important',
                    top: 0,
                    left: 0,
                    width: '100vw !important',
                    height: '100vh !important',
                    zIndex: 9999,
                    backgroundColor: 'black',
                    // Ensure it takes full screen on mobile
                    WebkitTransform: 'translate3d(0,0,0)',
                    transform: 'translate3d(0,0,0)',
                }),
            }}
            onMouseMove={handleMouseMove}
            onMouseEnter={handleMouseEnter}
            onMouseLeave={handleMouseLeave}
            onTouchStart={handleTouchStart}
            onTouchEnd={handleMouseLeave}>
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
                <ImagePortCanvas 
                    width={isFullscreen ? (typeof window !== 'undefined' ? window.innerWidth : dimensions.width) : dimensions.width} 
                    height={isFullscreen ? (typeof window !== 'undefined' ? window.innerHeight : dimensions.height) : dimensions.height}
                />
            </Box>

            {/* Floating Controls - only basic fullscreen toggle */}
            <Fade in={showControls}>
                <Box
                    sx={{
                        position: 'absolute',
                        bottom: 16,
                        left: 16,
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
                        <Box sx={{ display: 'flex', flexDirection: 'column', p: 0.75 }}>
                            <Tooltip title="Toggle fullscreen" placement="left" arrow>
                                <IconButton
                                    onClick={toggleFullscreen}
                                    sx={{
                                        'color': 'white',
                                        'minWidth': { xs: 44, sm: 'auto' }, // Larger touch target on mobile
                                        'minHeight': { xs: 44, sm: 'auto' },
                                        'padding': { xs: '12px', sm: '8px' },
                                        '&:hover': { 
                                            color: theme.palette.primary.light,
                                            backgroundColor: 'rgba(255, 255, 255, 0.1)'
                                        },
                                        '&:active': {
                                            backgroundColor: 'rgba(255, 255, 255, 0.2)',
                                            transform: 'scale(0.95)'
                                        },
                                        // Prevent double-tap zoom on mobile
                                        touchAction: 'manipulation',
                                        transition: 'all 0.15s ease'
                                    }}
                                    size={isFullscreen ? "medium" : "small"}>
                                    {isFullscreen ? <X size={20} /> : <Maximize2 size={20} />}
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