import React, { useState, useRef, useEffect, useCallback } from 'react';
import {
    Box,
    Button,
    IconButton,
    Paper,
    Typography,
    Alert,
    CircularProgress,
    Switch,
    FormControlLabel,
    Slider,
    useTheme,
    useMediaQuery
} from '@mui/material';
import {
    Camera,
    CameraOff,
    Settings,
    Zap,
    ZapOff,
    RotateCw,
    FlipHorizontal
} from 'lucide-react';
import { useCamera } from '../hooks/useCamera';

const RealTimeCameraKaleidoscope = ({ 
    width = 512, 
    height = 512,
    onClose 
}) => {
    const theme = useTheme();
    const isMobile = useMediaQuery(theme.breakpoints.down('sm'));
    
    // Camera hook
    const {
        videoRef,
        canvasRef,
        isStreaming,
        error,
        devices,
        currentDeviceId,
        startCamera,
        stopCamera,
        switchCamera,
        startLiveProcessing,
        stopLiveProcessing,
        processFrame
    } = useCamera();

    // Real-time processing state
    const [isProcessing, setIsProcessing] = useState(false);
    const [moduleReady, setModuleReady] = useState(false);
    const [stats, setStats] = useState({ fps: 0, active: false, frame_count: 0 });
    const [showSettings, setShowSettings] = useState(false);
    const [mirrorMode, setMirrorMode] = useState(true);
    
    // Performance settings
    const [targetFPS, setTargetFPS] = useState(30);
    const [processingQuality, setProcessingQuality] = useState(1.0); // 1.0 = full quality, 0.5 = half res, etc.
    
    // Animation frame management
    const animationFrameRef = useRef(null);
    const lastFrameTimeRef = useRef(0);
    const frameCountRef = useRef(0);
    const statsUpdateRef = useRef(null);

    // Check module readiness
    useEffect(() => {
        const checkModuleReady = () => {
            // Check if module exists and has camera functions
            if (window.module && window.module.cameraReady === true) {
                setModuleReady(true);
                return true;
            }
            
            // Fallback check for individual functions
            if (window.module && 
                window.module.start_camera_stream && 
                window.module.update_camera_frame_optimized &&
                window.module.process_camera_frame_with_effects) {
                setModuleReady(true);
                window.module.cameraReady = true; // Set flag for future checks
                return true;
            }
            
            return false;
        };

        if (checkModuleReady()) {
            console.log('[Camera] Module ready for camera processing');
            return;
        }

        console.log('[Camera] Waiting for camera-optimized module...');
        const interval = setInterval(() => {
            if (checkModuleReady()) {
                console.log('[Camera] Module became ready');
                clearInterval(interval);
            }
        }, 100);

        // Timeout after 10 seconds
        const timeout = setTimeout(() => {
            clearInterval(interval);
            if (!moduleReady) {
                console.error('[Camera] Module failed to load camera functions within timeout');
                setModuleReady(false);
            }
        }, 10000);

        return () => {
            clearInterval(interval);
            clearTimeout(timeout);
        };
    }, [moduleReady]);

    // Start camera on mount
    useEffect(() => {
        if (!isStreaming) {
            startCamera();
        }
        return () => {
            stopProcessing();
            stopCamera();
        };
    }, []);

    // Real-time processing loop
    const processVideoFrame = useCallback(() => {
        if (!isProcessing || !videoRef.current || !canvasRef.current || !moduleReady) {
            return;
        }

        const currentTime = performance.now();
        const frameInterval = 1000 / targetFPS;
        
        // Frame rate limiting
        if (currentTime - lastFrameTimeRef.current < frameInterval) {
            animationFrameRef.current = requestAnimationFrame(processVideoFrame);
            return;
        }

        const video = videoRef.current;
        const canvas = canvasRef.current;
        const ctx = canvas.getContext('2d');

        // Calculate processing dimensions based on quality setting
        const processWidth = Math.floor(video.videoWidth * processingQuality);
        const processHeight = Math.floor(video.videoHeight * processingQuality);

        // Set canvas size
        canvas.width = processWidth;
        canvas.height = processHeight;

        // Draw and process frame
        ctx.save();
        
        if (mirrorMode) {
            ctx.scale(-1, 1);
            ctx.translate(-canvas.width, 0);
        }
        
        ctx.drawImage(video, 0, 0, processWidth, processHeight);
        ctx.restore();

        // Send to backend for kaleidoscope processing
        try {
            const imageData = ctx.getImageData(0, 0, processWidth, processHeight);
            const success = processFrame(imageData, processWidth, processHeight);
            
            if (success) {
                frameCountRef.current++;
                
                // Update stats every 30 frames
                if (frameCountRef.current % 30 === 0) {
                    updateStats();
                }
            }
        } catch (err) {
            console.warn('Frame processing error:', err);
        }

        lastFrameTimeRef.current = currentTime;
        animationFrameRef.current = requestAnimationFrame(processVideoFrame);
    }, [isProcessing, moduleReady, targetFPS, processingQuality, mirrorMode, processFrame]);

    // Update performance stats
    const updateStats = useCallback(() => {
        if (window.module && window.module.get_camera_stream_stats) {
            try {
                const newStats = JSON.parse(window.module.get_camera_stream_stats());
                setStats(newStats);
            } catch (e) {
                console.warn('Failed to get stats:', e);
            }
        }
    }, []);

    // Start processing
    const startProcessing = useCallback(async () => {
        if (!moduleReady || isProcessing) return;
        
        try {
            const success = await startLiveProcessing();
            if (success) {
                setIsProcessing(true);
                frameCountRef.current = 0;
                processVideoFrame();
                
                // Start stats updates
                statsUpdateRef.current = setInterval(updateStats, 1000);
            }
        } catch (error) {
            console.error('Failed to start processing:', error);
        }
    }, [moduleReady, isProcessing, startLiveProcessing, processVideoFrame, updateStats]);

    // Stop processing
    const stopProcessing = useCallback(() => {
        if (!isProcessing) return;
        
        setIsProcessing(false);
        
        if (animationFrameRef.current) {
            cancelAnimationFrame(animationFrameRef.current);
        }
        
        if (statsUpdateRef.current) {
            clearInterval(statsUpdateRef.current);
        }
        
        stopLiveProcessing();
        setStats({ fps: 0, active: false, frame_count: 0 });
    }, [isProcessing, stopLiveProcessing]);

    // Toggle processing
    const toggleProcessing = () => {
        if (isProcessing) {
            stopProcessing();
        } else {
            startProcessing();
        }
    };

    // Settings panel
    const renderSettings = () => (
        <Paper 
            sx={{ 
                position: 'absolute', 
                top: 60, 
                right: 10, 
                p: 2, 
                minWidth: 280,
                zIndex: 1000
            }}
        >
            <Typography variant="h6" gutterBottom>Real-time Settings</Typography>
            
            {/* Target FPS */}
            <Typography gutterBottom>Target FPS: {targetFPS}</Typography>
            <Slider
                value={targetFPS}
                onChange={(e, value) => setTargetFPS(value)}
                min={10}
                max={60}
                step={5}
                marks={[
                    { value: 15, label: '15' },
                    { value: 30, label: '30' },
                    { value: 60, label: '60' }
                ]}
                sx={{ mb: 2 }}
            />

            {/* Processing Quality */}
            <Typography gutterBottom>
                Processing Quality: {Math.round(processingQuality * 100)}%
            </Typography>
            <Slider
                value={processingQuality}
                onChange={(e, value) => setProcessingQuality(value)}
                min={0.25}
                max={1.0}
                step={0.25}
                marks={[
                    { value: 0.25, label: '25%' },
                    { value: 0.5, label: '50%' },
                    { value: 0.75, label: '75%' },
                    { value: 1.0, label: '100%' }
                ]}
                sx={{ mb: 2 }}
            />

            {/* Mirror mode */}
            <FormControlLabel
                control={
                    <Switch
                        checked={mirrorMode}
                        onChange={(e) => setMirrorMode(e.target.checked)}
                    />
                }
                label="Mirror Mode"
                sx={{ mb: 1 }}
            />

            {/* Camera selection */}
            {devices.length > 1 && (
                <Button
                    fullWidth
                    variant="outlined"
                    onClick={() => {
                        const nextIndex = (devices.findIndex(d => d.deviceId === currentDeviceId) + 1) % devices.length;
                        if (devices[nextIndex]) {
                            switchCamera(devices[nextIndex].deviceId);
                        }
                    }}
                    sx={{ mt: 1 }}
                >
                    Switch Camera
                </Button>
            )}
        </Paper>
    );

    if (error) {
        return (
            <Box sx={{ p: 2, textAlign: 'center' }}>
                <Alert severity="error" sx={{ mb: 2 }}>
                    {error}
                </Alert>
                <Button onClick={startCamera} variant="contained">
                    Retry Camera Access
                </Button>
            </Box>
        );
    }

    return (
        <Box sx={{ position: 'relative', width, height, bgcolor: 'black', borderRadius: 1 }}>
            {/* Video preview */}
            <video
                ref={videoRef}
                autoPlay
                playsInline
                muted
                style={{
                    width: '100%',
                    height: '100%',
                    objectFit: 'cover',
                    transform: mirrorMode ? 'scaleX(-1)' : 'none',
                    borderRadius: theme.shape.borderRadius
                }}
            />

            {/* Hidden processing canvas */}
            <canvas
                ref={canvasRef}
                style={{ display: 'none' }}
            />

            {/* Loading overlay */}
            {!isStreaming && (
                <Box
                    sx={{
                        position: 'absolute',
                        top: 0,
                        left: 0,
                        right: 0,
                        bottom: 0,
                        display: 'flex',
                        alignItems: 'center',
                        justifyContent: 'center',
                        bgcolor: 'rgba(0,0,0,0.7)',
                        color: 'white'
                    }}
                >
                    <Box sx={{ textAlign: 'center' }}>
                        <CircularProgress color="inherit" sx={{ mb: 2 }} />
                        <Typography>Starting camera...</Typography>
                    </Box>
                </Box>
            )}

            {/* Module loading overlay */}
            {isStreaming && !moduleReady && (
                <Box
                    sx={{
                        position: 'absolute',
                        top: 0,
                        left: 0,
                        right: 0,
                        bottom: 0,
                        display: 'flex',
                        alignItems: 'center',
                        justifyContent: 'center',
                        bgcolor: 'rgba(0,0,0,0.5)',
                        color: 'white'
                    }}
                >
                    <Box sx={{ textAlign: 'center' }}>
                        <CircularProgress color="inherit" sx={{ mb: 2 }} />
                        <Typography>Loading kaleidoscope engine...</Typography>
                    </Box>
                </Box>
            )}

            {/* Control buttons */}
            <Box
                sx={{
                    position: 'absolute',
                    bottom: 10,
                    left: '50%',
                    transform: 'translateX(-50%)',
                    display: 'flex',
                    gap: 1
                }}
            >
                {/* Start/Stop processing */}
                <IconButton
                    onClick={toggleProcessing}
                    disabled={!moduleReady || !isStreaming}
                    sx={{
                        bgcolor: isProcessing ? 'error.main' : 'primary.main',
                        color: 'white',
                        '&:hover': {
                            bgcolor: isProcessing ? 'error.dark' : 'primary.dark'
                        }
                    }}
                >
                    {isProcessing ? <ZapOff size={24} /> : <Zap size={24} />}
                </IconButton>

                {/* Settings */}
                <IconButton
                    onClick={() => setShowSettings(!showSettings)}
                    sx={{ bgcolor: 'rgba(0,0,0,0.7)', color: 'white' }}
                >
                    <Settings size={24} />
                </IconButton>

                {/* Close */}
                {onClose && (
                    <IconButton
                        onClick={onClose}
                        sx={{ bgcolor: 'rgba(0,0,0,0.7)', color: 'white' }}
                    >
                        <CameraOff size={24} />
                    </IconButton>
                )}
            </Box>

            {/* Processing indicator */}
            {isProcessing && (
                <Box
                    sx={{
                        position: 'absolute',
                        top: 10,
                        left: 10,
                        display: 'flex',
                        alignItems: 'center',
                        bgcolor: 'rgba(0,255,0,0.8)',
                        color: 'black',
                        px: 1,
                        py: 0.5,
                        borderRadius: 1,
                        fontSize: '0.75rem',
                        fontWeight: 'bold'
                    }}
                >
                    <Zap size={16} style={{ marginRight: 4 }} />
                    LIVE KALEIDOSCOPE
                    {stats.fps > 0 && (
                        <Typography variant="caption" sx={{ ml: 1 }}>
                            {stats.fps.toFixed(1)} FPS
                        </Typography>
                    )}
                </Box>
            )}

            {/* Performance stats */}
            {isProcessing && stats.active && (
                <Box
                    sx={{
                        position: 'absolute',
                        top: 10,
                        right: 10,
                        bgcolor: 'rgba(0,0,0,0.8)',
                        color: 'white',
                        px: 1,
                        py: 0.5,
                        borderRadius: 1,
                        fontSize: '0.7rem',
                        fontFamily: 'monospace'
                    }}
                >
                    <div>FPS: {stats.fps?.toFixed(1) || 0}</div>
                    <div>Frames: {stats.frame_count || 0}</div>
                    <div>Quality: {Math.round(processingQuality * 100)}%</div>
                </Box>
            )}

            {/* Settings panel */}
            {showSettings && renderSettings()}
        </Box>
    );
};

export default RealTimeCameraKaleidoscope; 