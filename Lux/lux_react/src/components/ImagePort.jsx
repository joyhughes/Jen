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
    const videoRef = useRef(null);
    const canvasRef = useRef(null);
    const animationFrameRef = useRef(null);
    const lastFrameTimeRef = useRef(0);
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

    // Camera state - SIMPLIFIED
    const [isLiveCameraActive, setIsLiveCameraActive] = useState(false);
    const [cameraReady, setCameraReady] = useState(false);
    const [error, setError] = useState(null);

    // ULTRA-OPTIMIZED Camera settings for maximum FPS
    const [cameraSettings, setCameraSettings] = useState({
        fps: 60,                    // Target 60fps
        quality: 0.25,              // Start at 25% quality for maximum performance
        mirrorMode: true,
        adaptiveQuality: true,      // Auto-adjust quality based on performance
        bufferReuse: true,          // Reuse canvas buffers
        skipFrames: 0,              // Skip frames when performance drops
        lowLatencyMode: true,       // Minimize processing delays
        fastScaling: true           // Use fastest scaling algorithms
    });

    const FRAME_INTERVAL = 1000 / cameraSettings.fps;
    const { mirrorMode, quality, adaptiveQuality, bufferReuse, skipFrames, lowLatencyMode, fastScaling } = cameraSettings;

    // ULTRA-PERFORMANCE tracking with frame skipping
    const performanceRef = useRef({
        frameCount: 0,
        lastFpsUpdate: 0,
        currentFps: 0,
        avgFrameTime: 0,
        droppedFrames: 0,
        skipCounter: 0,             // Frame skipping counter
        targetFrameTime: 16.67,     // 60fps = 16.67ms per frame
        performanceMode: 'auto'     // auto, performance, quality
    });

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

    // ULTRA-OPTIMIZED camera startup with backend integration
    const startLiveCamera = async () => {
        try {
            console.log('=== CAMERA START DEBUG ===');
            console.log('[Camera] Starting ULTRA-OPTIMIZED camera with backend kaleidoscope...');
            console.log('[Camera] moduleReady:', moduleReady);
            console.log('[Camera] window.module available:', !!window.module);

            // BACKEND INITIALIZATION: Start ultra camera stream
            if (window.module && typeof window.module.ultra_start_camera_stream === 'function') {
                console.log('[Camera] Calling ultra_start_camera_stream...');
                const backendStarted = window.module.ultra_start_camera_stream();
                console.log('[Camera] ultra_start_camera_stream result:', backendStarted);

                if (!backendStarted) {
                    throw new Error('Backend camera stream failed to start');
                }
                console.log('[Camera] Backend ultra camera stream started');

                // CRITICAL FIX: Switch source menu to camera (like image picker does)
                if (typeof window.module.update_source_name === 'function') {
                    console.log('[Camera] Calling update_source_name with ultra_camera...');
                    window.module.update_source_name('ultra_camera');
                    console.log('[Camera] Switched source menu to ultra_camera');

                    // VERIFY: Check if source was actually switched
                    if (typeof window.module.get_widget_JSON === 'function') {
                        try {
                            const sourceMenuJSON = window.module.get_widget_JSON('source_image_menu');
                            const sourceMenu = JSON.parse(sourceMenuJSON);
                            console.log('[Camera] Current source menu state:', sourceMenu);
                            console.log('[Camera] Current choice index:', sourceMenu.choice);
                            console.log('[Camera] Current choice item:', sourceMenu.items[sourceMenu.choice]);
                        } catch (e) {
                            console.warn('[Camera] Could not verify source menu state:', e);
                        }
                    }
                } else {
                    console.warn('[Camera] update_source_name function not available');
                }

                // VERIFY: Check backend camera stats
                if (typeof window.module.ultra_get_camera_stats === 'function') {
                    const stats = window.module.ultra_get_camera_stats();
                    console.log('[Camera] Backend camera stats:', stats);
                } else {
                    console.warn('[Camera] ultra_get_camera_stats not available');
                }

                // VERIFY: Test all ultra camera functions
                console.log('[Camera] Testing backend function availability:');
                console.log('  - ultra_start_camera_stream:', typeof window.module.ultra_start_camera_stream);
                console.log('  - ultra_update_camera_frame:', typeof window.module.ultra_update_camera_frame);
                console.log('  - ultra_process_camera_with_kaleidoscope:', typeof window.module.ultra_process_camera_with_kaleidoscope);
                console.log('  - ultra_stop_camera_stream:', typeof window.module.ultra_stop_camera_stream);
                console.log('  - ultra_get_camera_stats:', typeof window.module.ultra_get_camera_stats);

            } else {
                console.warn('[Camera] Backend ultra functions not available, using frontend only');
                console.log('[Camera] Available module functions:', Object.keys(window.module || {}));
            }

            console.log('[Camera] Starting getUserMedia...');
            // OPTIMAL PERFORMANCE: Lower resolution for maximum smooth 60fps
            const stream = await navigator.mediaDevices.getUserMedia({
                video: {
                    width: { ideal: 640, max: 1280 },        // Smooth: optimal for 256x256
                    height: { ideal: 480, max: 720 },        // Smooth: 4:3 ratio, less demanding
                    frameRate: { ideal: 60, min: 30 },       // Prioritize high frame rate
                    facingMode: 'user',
                    // Performance optimizations
                    aspectRatio: { ideal: 4 / 3 },           // Standard aspect ratio
                    resizeMode: 'crop-and-scale'             // Let browser optimize
                },
                audio: false  // Explicitly disable audio for performance
            });
            console.log('[Camera] getUserMedia successful, stream:', stream);

            if (videoRef.current) {
                console.log('[Camera] Setting video srcObject...');
                videoRef.current.srcObject = stream;

                // OPTIMIZATION: Configure video element for performance
                videoRef.current.playsInline = true;
                videoRef.current.muted = true;
                videoRef.current.controls = false;
                videoRef.current.preload = 'none';

                videoRef.current.onloadedmetadata = () => {
                    const actualWidth = videoRef.current.videoWidth;
                    const actualHeight = videoRef.current.videoHeight;
                    console.log(`[Camera] Video metadata loaded: ${actualWidth}x${actualHeight}`);
                    console.log(`[Camera] Video readyState: ${videoRef.current.readyState}`);

                    // Reset performance counters
                    performanceRef.current = {
                        frameCount: 0,
                        lastFpsUpdate: performance.now(),
                        currentFps: 0,
                        avgFrameTime: 0,
                        droppedFrames: 0,
                        skipCounter: 0,
                        targetFrameTime: 16.67,
                        performanceMode: 'auto'
                    };

                    console.log('[Camera] Setting cameraReady to true...');
                    setCameraReady(true);
                    console.log('[Camera] Setting isLiveCameraActive to true...');
                    setIsLiveCameraActive(true);

                    // Start processing with minimal delay
                    console.log('[Camera] Starting frame processing in 50ms...');
                    setTimeout(() => {
                        console.log('[Camera] Requesting first animation frame...');
                        animationFrameRef.current = requestAnimationFrame(processLiveCameraFrame);
                    }, 50);
                };

                videoRef.current.onerror = (error) => {
                    console.error('[Camera] Video error:', error);
                };

                console.log('[Camera] Calling video.play()...');
                await videoRef.current.play();
                console.log('[Camera] Video play() successful');
                console.log('[Camera] ULTRA-OPTIMIZED stream with backend kaleidoscope started successfully');
            } else {
                console.error('[Camera] videoRef.current is null!');
            }
            console.log('=== CAMERA START DEBUG END ===');
        } catch (error) {
            console.error('=== CAMERA START ERROR ===');
            console.error('[Camera] Failed to start ULTRA camera with backend:', error);
            console.error('[Camera] Error stack:', error.stack);
            setError(`Ultra camera with backend failed: ${error.message}`);
            console.error('=== CAMERA START ERROR END ===');
        }
    };

    // ULTRA-OPTIMIZED camera shutdown with backend cleanup
    const stopLiveCamera = () => {
        console.log('[Camera] Stopping ULTRA-OPTIMIZED camera with backend cleanup...');

        // BACKEND CLEANUP: Stop ultra camera stream and restore source
        if (window.module && typeof window.module.ultra_stop_camera_stream === 'function') {
            const backendStopped = window.module.ultra_stop_camera_stream();
            if (backendStopped) {
                console.log('[Camera] Backend ultra camera stream stopped and source restored');
            } else {
                console.warn('[Camera] Backend camera stream stop failed');
            }
        }

        // FRONTEND CLEANUP
        if (animationFrameRef.current) {
            cancelAnimationFrame(animationFrameRef.current);
            animationFrameRef.current = null;
        }

        if (videoRef.current && videoRef.current.srcObject) {
            const tracks = videoRef.current.srcObject.getTracks();
            tracks.forEach(track => track.stop());
            videoRef.current.srcObject = null;
        }

        // Reset all states
        setIsLiveCameraActive(false);
        setCameraReady(false);
        setError(null);

        // Reset performance tracking
        performanceRef.current = {
            frameCount: 0,
            lastFpsUpdate: performance.now(),
            currentFps: 0,
            avgFrameTime: 0,
            droppedFrames: 0,
            skipCounter: 0,
            targetFrameTime: 16.67,
            performanceMode: 'auto'
        };

        console.log('[Camera] Ultra camera with backend stopped');
    };


    useEffect(() => {
        if (isLiveCameraActive && cameraReady && moduleReady && videoRef.current && canvasRef.current) {
            console.log('[Camera] All states ready, starting frame processing...');
            const timeoutId = setTimeout(() => {
                animationFrameRef.current = requestAnimationFrame(processLiveCameraFrame);
            }, 100);
            
            return () => clearTimeout(timeoutId);
        }
    }, [isLiveCameraActive, cameraReady, moduleReady])

    // processLiveCameraFrame function with CONTROLLED LOGGING
    const processLiveCameraFrame = () => {
        const perf = performanceRef.current;

        // CONTROLLED LOGGING: Only log first 3 frames and every 60th frame
        const shouldLog = perf.frameCount < 3 || perf.frameCount % 60 === 0;
        const shouldDetailLog = perf.frameCount < 2; // Detailed logs only for first 2 frames

        // Basic state checks - removed excessive logging
        // if (!isLiveCameraActive || !videoRef.current || !canvasRef.current || !cameraReady || !moduleReady) {
        //     if (shouldDetailLog) {
        //         console.log(`[Frame ${perf.frameCount}] Skipping: state check failed`);
        //     }
        //     return;
        // }

        const currentTime = performance.now();

        // Frame rate limiting with minimal logging
        let dynamicInterval = FRAME_INTERVAL;
        if (adaptiveQuality && perf.avgFrameTime > perf.targetFrameTime * 1.5) {
            dynamicInterval = Math.max(FRAME_INTERVAL, perf.avgFrameTime * 2);
        }

        if (currentTime - lastFrameTimeRef.current < dynamicInterval) {
            animationFrameRef.current = requestAnimationFrame(processLiveCameraFrame);
            return;
        }

        const frameStartTime = currentTime;
        const video = videoRef.current;
        const canvas = canvasRef.current;

        // Fast video readiness check
        if (video.readyState < 2) {
            perf.droppedFrames++;
            animationFrameRef.current = requestAnimationFrame(processLiveCameraFrame);
            return;
        }

        const ctx = canvas.getContext('2d');
        const BACKEND_WIDTH = 256;   // OPTIMAL: Maximum smooth performance  
        const BACKEND_HEIGHT = 256;  // OPTIMAL: Original smooth 60fps

        // Configure canvas for backend processing
        if (canvas.width !== BACKEND_WIDTH || canvas.height !== BACKEND_HEIGHT) {
            canvas.width = BACKEND_WIDTH;
            canvas.height = BACKEND_HEIGHT;
            ctx.imageSmoothingEnabled = true;  // Enable smoothing for better quality
            ctx.imageSmoothingQuality = 'high';  // Use high quality interpolation
            ctx.globalCompositeOperation = 'source-over';
            if (shouldDetailLog) {
                console.log(`[Frame ${perf.frameCount}] Canvas configured: ${BACKEND_WIDTH}x${BACKEND_HEIGHT} (BALANCED QUALITY)`);
            }
        }

        try {
            const videoWidth = video.videoWidth;
            const videoHeight = video.videoHeight;

            if (videoWidth === 0 || videoHeight === 0) {
                perf.droppedFrames++;
                animationFrameRef.current = requestAnimationFrame(processLiveCameraFrame);
                return;
            }

            // Fast square crop for kaleidoscope effects with HIGH QUALITY scaling
            const minDim = Math.min(videoWidth, videoHeight);
            const sourceX = Math.round((videoWidth - minDim) * 0.5);
            const sourceY = Math.round((videoHeight - minDim) * 0.5);

            // Mirror mode handling
            if (mirrorMode) {
                ctx.setTransform(-1, 0, 0, 1, BACKEND_WIDTH, 0);
            } else {
                ctx.setTransform(1, 0, 0, 1, 0, 0);
            }

            // HIGH QUALITY draw operation with superior interpolation
            ctx.drawImage(
                video,
                sourceX, sourceY, minDim, minDim,      // Source: square crop from center
                0, 0, BACKEND_WIDTH, BACKEND_HEIGHT    // Dest: 512x512 with high quality scaling
            );

            // BACKEND PROCESSING with minimal logging
            if (window.module && typeof window.module.ultra_update_camera_frame === 'function') {
                try {
                    const imageData = ctx.getImageData(0, 0, BACKEND_WIDTH, BACKEND_HEIGHT);

                    if (shouldDetailLog) {
                        // Check if we have actual image data - only for first 2 frames
                        let nonZeroCount = 0;
                        for (let i = 0; i < Math.min(1000, imageData.data.length); i++) {
                            if (imageData.data[i] > 0) nonZeroCount++;
                        }
                        console.log(`[Frame ${perf.frameCount}] Camera data: ${nonZeroCount}/1000 non-zero pixels`);
                    }

                    // CRITICAL COLOR FIX: Convert RGBA to ARGB format like successful still image capture
                    // The still image capture does this conversion in frontend, not backend
                    const convertedData = new Uint8ClampedArray(imageData.data.length);
                    
                    // Convert RGBA → ARGB pixel by pixel (same as still image capture)
                    for (let i = 0; i < imageData.data.length; i += 4) {
                        const r = imageData.data[i];     // Red
                        const g = imageData.data[i + 1]; // Green  
                        const b = imageData.data[i + 2]; // Blue
                        const a = imageData.data[i + 3]; // Alpha
                        
                        // Backend expects ARGB format (ucolor bit layout) - same as still capture
                        convertedData[i]     = a; // Alpha → position 0
                        convertedData[i + 1] = r; // Red → position 1
                        convertedData[i + 2] = g; // Green → position 2
                        convertedData[i + 3] = b; // Blue → position 3
                    }

                    if (shouldDetailLog) {
                        console.log(`[Frame ${perf.frameCount}] Applied RGBA→ARGB conversion (matching still image capture)`);
                    }

                    // Send converted ARGB data to backend (backend now expects this format)
                    const success = window.module.ultra_update_camera_frame(
                        convertedData,  // Use converted ARGB data instead of raw RGBA
                        BACKEND_WIDTH,
                        BACKEND_HEIGHT
                    );

                    if (success && typeof window.module.ultra_process_camera_with_kaleidoscope === 'function') {
                        const processResult = window.module.ultra_process_camera_with_kaleidoscope();

                        if (shouldDetailLog) {
                            console.log(`[Frame ${perf.frameCount}] Backend processing: update=${success}, process=${processResult}`);

                            // Debug main buffer only for first frame
                            if (perf.frameCount === 0 && typeof window.module.get_img_data === 'function') {
                                const mainBufferData = window.module.get_img_data();
                                if (mainBufferData) {
                                    const mainArray = new Uint8Array(mainBufferData);
                                    let mainNonZero = 0;
                                    for (let i = 0; i < Math.min(1000, mainArray.length); i++) {
                                        if (mainArray[i] > 0) mainNonZero++;
                                    }
                                    console.log(`[Frame ${perf.frameCount}] Main buffer result: ${mainNonZero}/1000 non-zero pixels`);
                                }
                            }
                        }
                    } else if (shouldDetailLog) {
                        console.warn(`[Frame ${perf.frameCount}] Backend processing failed or unavailable`);
                    }

                } catch (backendError) {
                    if (shouldDetailLog) {
                        console.warn(`[Frame ${perf.frameCount}] Backend error:`, backendError);
                    }
                    perf.droppedFrames++;
                }
            } else if (shouldDetailLog) {
                console.warn(`[Frame ${perf.frameCount}] Backend functions not available`);
            }

            // Minimal performance tracking
            const frameTime = performance.now() - frameStartTime;
            perf.frameCount++;

            // Ultra-fast moving average (only update every few frames)
            if (perf.frameCount % 3 === 0) {
                perf.avgFrameTime = (perf.avgFrameTime * 0.8) + (frameTime * 0.2);
            }

            // Adaptive quality adjustment (every 30 frames)
            if (perf.frameCount % 30 === 0 && adaptiveQuality) {
                if (perf.avgFrameTime > perf.targetFrameTime * 1.5 && quality > 0.125) {
                    setCameraSettings(prev => ({
                        ...prev,
                        quality: Math.max(0.125, prev.quality * 0.7),
                        skipFrames: Math.min(3, prev.skipFrames + 1)
                    }));
                    if (shouldLog) {
                        console.log(`[Frame ${perf.frameCount}] Performance: reducing quality to ${Math.max(0.125, quality * 0.7)}`);
                    }
                } else if (perf.avgFrameTime < perf.targetFrameTime * 0.7 && quality < 0.5) {
                    setCameraSettings(prev => ({
                        ...prev,
                        quality: Math.min(0.5, prev.quality * 1.1),
                        skipFrames: Math.max(0, prev.skipFrames - 1)
                    }));
                }
            }

            // FPS reporting (every 60 frames)
            if (shouldLog && perf.frameCount % 60 === 0) {
                const elapsed = currentTime - perf.lastFpsUpdate;
                if (elapsed > 0) {
                    perf.currentFps = Math.round(60000 / elapsed);
                    perf.lastFpsUpdate = currentTime;
                    console.log(`[Frame ${perf.frameCount}] Performance: ${perf.currentFps}fps, ${perf.avgFrameTime.toFixed(1)}ms avg, ${perf.droppedFrames} dropped`);
                }
            }

        } catch (error) {
            if (shouldLog) {
                console.warn(`[Frame ${perf.frameCount}] Frame error:`, error);
            }
            perf.droppedFrames++;
        }

        lastFrameTimeRef.current = currentTime;

        // Schedule next frame
        if (lowLatencyMode && perf.avgFrameTime < perf.targetFrameTime) {
            setTimeout(() => {
                animationFrameRef.current = requestAnimationFrame(processLiveCameraFrame);
            }, 1);
        } else {
            animationFrameRef.current = requestAnimationFrame(processLiveCameraFrame);
        }
    };


    // Toggle live camera mode - SIMPLIFIED
    const toggleLiveCamera = () => {
        if (isLiveCameraActive) {
            stopLiveCamera();
        } else {
            startLiveCamera();
        }
    };

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
                <ImagePortCanvas 
                    width={dimensions.width} 
                    height={dimensions.height}
                    isLiveCameraActive={isLiveCameraActive}
                />
            </Box>

            {/* Hidden video element for camera stream */}
            <video
                ref={videoRef}
                autoPlay
                playsInline
                muted
                style={{ display: 'none' }}
            />

            {/* Hidden canvas for camera processing */}
            <canvas
                ref={canvasRef}
                style={{ display: 'none' }}
            />

            {/* Camera Error Alert */}
            {error && (
                <Alert
                    severity="error"
                    sx={{
                        position: 'absolute',
                        top: 16,
                        left: 16,
                        right: 16,
                        zIndex: 160
                    }}
                    onClose={() => setError(null)}
                >
                    Camera Error: {error}
                </Alert>
            )}

            {/* Floating Controls - existing side controls */}
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
                                        '&:hover': { color: theme.palette.primary.light },
                                    }}
                                    size="small">
                                    {isFullscreen ? <X size={16} /> : <Maximize2 size={16} />}
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

            {/* High Quality Camera Indicator - Shows when camera is active */}
            {isLiveCameraActive && (
                <Box
                    sx={{
                        position: 'absolute',
                        top: 8,
                        left: 8,
                        background: 'linear-gradient(45deg, rgba(255, 0, 0, 0.95), rgba(255, 69, 0, 0.95))',
                        color: 'white',
                        padding: '8px 12px',
                        borderRadius: '8px',
                        fontSize: '11px',
                        fontWeight: 'bold',
                        zIndex: 1000,
                        fontFamily: 'monospace',
                        lineHeight: 1.3,
                        boxShadow: '0 3px 12px rgba(255,0,0,0.4)',
                        border: '1px solid rgba(255,255,255,0.2)'
                    }}
                >
                    <div style={{ display: 'flex', alignItems: 'center', gap: '6px' }}>
                        <span style={{ fontSize: '12px' }}>●</span>
                        <span>SMOOTH CAMERA</span>
                    </div>
                    <div style={{ fontSize: '9px', opacity: 0.95, marginTop: '3px', display: 'flex', flexDirection: 'column', gap: '1px' }}>
                        <div>
                            Frontend: {performanceRef.current.currentFps}fps | Q:{Math.round(quality * 100)}% | {performanceRef.current.avgFrameTime.toFixed(1)}ms
                        </div>
                        <div style={{ fontSize: '8px', opacity: 0.8 }}>
                            Backend: 256×256 | Skip:{skipFrames} | Drop:{performanceRef.current.droppedFrames}
                        </div>
                        <div style={{ fontSize: '8px', opacity: 0.7, color: '#90EE90' }}>
                            ✓ Scene Effects Active (Frontend Controlled)
                        </div>
                    </div>
                </Box>
            )}

            {/* Floating Camera Button */}
            <Box
                sx={{
                    position: 'absolute',
                    top: 16,
                    right: 16,
                    display: 'flex',
                    flexDirection: 'column',
                    gap: 1,
                    zIndex: 200
                }}
            >
                {/* Live Camera Toggle */}
                <Zoom in={true}>
                    <Fab
                        color={isLiveCameraActive ? "secondary" : "primary"}
                        size="medium"
                        onClick={toggleLiveCamera}
                        sx={{
                            background: isLiveCameraActive
                                ? 'linear-gradient(45deg, #f44336 30%, #ff5722 90%)'
                                : 'linear-gradient(45deg, #2196F3 30%, #21CBF3 90%)',
                            boxShadow: isLiveCameraActive
                                ? '0 3px 15px rgba(244, 67, 54, 0.4)'
                                : '0 3px 15px rgba(33, 150, 243, 0.4)',
                            '&:hover': {
                                background: isLiveCameraActive
                                    ? 'linear-gradient(45deg, #d32f2f 30%, #f57c00 90%)'
                                    : 'linear-gradient(45deg, #1976D2 30%, #1CB5E0 90%)',
                                transform: 'scale(1.05)',
                            },
                            transition: 'all 0.2s ease-in-out'
                        }}
                    >
                        {isLiveCameraActive ? <CameraOff size={24} /> : <Camera size={24} />}
                    </Fab>
                </Zoom>
            </Box>
        </Paper>
    )
}

export default ImagePort;