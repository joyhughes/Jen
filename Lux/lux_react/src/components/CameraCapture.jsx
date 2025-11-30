import React, { useState, useRef, useEffect, useCallback } from 'react';
import {
    Box,
    Button,
    IconButton,
    Paper,
    Typography,
    Alert,
    CircularProgress,
    Fab,
    Slider,
    FormControl,
    InputLabel,
    Select,
    MenuItem,
    Switch,
    FormControlLabel,
    useTheme,
    useMediaQuery
} from '@mui/material';
import {
    Camera,
    CameraOff,
    FlipHorizontal,
    FlipVertical,
    RotateCw,
    Settings,
    X,
    Check,
    Zap,
    ZapOff
} from 'lucide-react';
import { useCamera } from '../hooks/useCamera';
import { isMobileDevice } from '../utils/cameraUtils';
import CameraControls from './CameraControls';
import '../styles/CameraCapture.css';

const CameraCapture = ({ 
    onCapture, 
    onClose, 
    width = 400, 
    height = 300,
    enableLivePreview = true,
    autoSaveToGrid = true 
}) => {
    const theme = useTheme();
    const isMobile = useMediaQuery(theme.breakpoints.down('sm'));
    const isMobileDevice_ = isMobileDevice(); // Use utility function for mobile detection
    
    // Module ready state
    const [moduleReady, setModuleReady] = useState(false);
    
    // Get default camera facing mode - back camera for mobile, front for desktop
    const getDefaultFacingMode = () => {
        return isMobileDevice_ ? 'environment' : 'user';
    };
    
    // Camera hook for all camera logic
    const {
        videoRef,
        canvasRef,
        isStreaming,
        isCapturing,
        error,
        devices,
        currentDeviceId,
        currentFacingMode,
        constraints,
        startCamera,
        stopCamera,
        switchCamera,
        toggleCameraFacing,
        switchToFrontCamera,
        switchToBackCamera,
        getCameraInfo,
        capturePhoto,
        updateConstraints
    } = useCamera();

    // UI state
    const [showSettings, setShowSettings] = useState(false);
    const [flashEnabled, setFlashEnabled] = useState(false);
    const [mirrorMode, setMirrorMode] = useState(true);
    const [captureMode, setCaptureMode] = useState('photo'); // 'photo' | 'live'
    const [livePreviewActive, setLivePreviewActive] = useState(false);
    const [isSwitchingCamera, setIsSwitchingCamera] = useState(false);

    // Live preview state
    const livePreviewRef = useRef(null);
    const animationFrameRef = useRef(null);
    const [streamStats, setStreamStats] = useState({ fps: 0, active: false });

    // Performance optimization refs
    const lastFrameTimeRef = useRef(0);
    const frameSkipCountRef = useRef(0);
    const TARGET_FPS = 30; // Target FPS for live processing
    const FRAME_INTERVAL = 1000 / TARGET_FPS;

    // Auto-update mirror mode based on camera facing
    useEffect(() => {
        // Only mirror front camera, not back camera
        setMirrorMode(currentFacingMode === 'user');
    }, [currentFacingMode]);

    // Enhanced camera switching for mobile
    const handleCameraSwitch = async () => {
        try {
            setIsSwitchingCamera(true);
            const newFacingMode = await toggleCameraFacing();
            console.log('[CameraCapture] Camera switched to:', newFacingMode);
        } catch (error) {
            console.error('[CameraCapture] Camera switch failed:', error);
        } finally {
            setIsSwitchingCamera(false);
        }
    };

    // Get camera info for UI display
    const cameraInfo = getCameraInfo();

    // Check for module readiness
    useEffect(() => {
        const checkModuleReady = () => {
            if (window.module && window.module.FS && typeof window.module.FS.writeFile === 'function') {
                setModuleReady(true);
                return true;
            }
            return false;
        };

        // Check immediately
        if (checkModuleReady()) {
            return;
        }

        // Poll for module readiness
        const interval = setInterval(() => {
            if (checkModuleReady()) {
                clearInterval(interval);
            }
        }, 100);

        return () => clearInterval(interval);
    }, []);

    // Initialize camera on component mount with mobile-specific default
    useEffect(() => {
        const initCamera = async () => {
            try {
                console.log('[CameraCapture] Starting camera with default facing mode for mobile:', getDefaultFacingMode());
                await startCamera(null, getDefaultFacingMode());
            } catch (err) {
                console.error('[CameraCapture] Failed to start camera:', err);
            }
        };

        if (moduleReady) {
            initCamera();
        }

        return () => {
            stopCamera();
            if (animationFrameRef.current) {
                cancelAnimationFrame(animationFrameRef.current);
            }
        };
    }, [moduleReady]);

    // Enhanced live preview with optimized processing
    const processLivePreview = useCallback(() => {
        if (!livePreviewActive || !videoRef.current || !canvasRef.current || !moduleReady) {
            return;
        }

        const currentTime = performance.now();
        
        // Frame rate limiting - only process if enough time has passed
        if (currentTime - lastFrameTimeRef.current < FRAME_INTERVAL) {
            animationFrameRef.current = requestAnimationFrame(processLivePreview);
            return;
        }

        const video = videoRef.current;
        const canvas = canvasRef.current;
        const ctx = canvas.getContext('2d');

        // Set canvas size to match video
        canvas.width = video.videoWidth;
        canvas.height = video.videoHeight;

        // Draw video frame to canvas
        ctx.save();
        
        // Apply mirror effect if enabled
        if (mirrorMode) {
            ctx.scale(-1, 1);
            ctx.translate(-canvas.width, 0);
        }
        
        ctx.drawImage(video, 0, 0, canvas.width, canvas.height);
        ctx.restore();

        // Send frame to backend for optimized processing
        if (enableLivePreview && window.module) {
            try {
                const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
                
                // Use optimized backend function
                const success = window.module.update_camera_frame_optimized(
                    imageData.data, 
                    canvas.width, 
                    canvas.height
                );
                
                if (success) {
                    // Process frame with current effects
                    window.module.process_camera_frame_with_effects();
                    
                    // Update stats periodically
                    if (frameSkipCountRef.current % 30 === 0) { // Every 30 frames
                        const stats = JSON.parse(window.module.get_camera_stream_stats());
                        setStreamStats(stats);
                    }
                    frameSkipCountRef.current++;
                }
                
                lastFrameTimeRef.current = currentTime;
            } catch (err) {
                console.warn('Live preview processing error:', err);
            }
        }

        // Continue animation loop
        animationFrameRef.current = requestAnimationFrame(processLivePreview);
    }, [livePreviewActive, mirrorMode, enableLivePreview, moduleReady]);

    // Start/stop live preview with backend stream control
    const toggleLivePreview = () => {
        if (livePreviewActive) {
            setLivePreviewActive(false);
            if (animationFrameRef.current) {
                cancelAnimationFrame(animationFrameRef.current);
            }
            // Stop backend camera stream
            if (window.module && window.module.stop_camera_stream) {
                window.module.stop_camera_stream();
            }
        } else {
            setLivePreviewActive(true);
            // Start backend camera stream
            if (window.module && window.module.start_camera_stream) {
                window.module.start_camera_stream();
            }
            processLivePreview();
        }
    };

    // Handle photo capture
    const handleCapture = async () => {
        try {
            // Check if module is ready
            if (!moduleReady) {
                throw new Error('WebAssembly module not ready - please wait a moment and try again');
            }
            
            const imageData = await capturePhoto();
            
            if (imageData && autoSaveToGrid) {
                // Generate unique filename
                const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
                const filename = `camera-capture-${timestamp}.png`;
                
                // CRITICAL FIX: Convert RGBA to ARGB format for backend compatibility
                const canvas = document.createElement('canvas');
                const ctx = canvas.getContext('2d');
                canvas.width = imageData.width;
                canvas.height = imageData.height;
                
                // Create new ImageData with ARGB format (backend ucolor format)
                const convertedData = new Uint8ClampedArray(imageData.data.length);
                
                // Convert RGBA → ARGB pixel by pixel
                for (let i = 0; i < imageData.data.length; i += 4) {
                    const r = imageData.data[i];     // Red
                    const g = imageData.data[i + 1]; // Green  
                    const b = imageData.data[i + 2]; // Blue
                    const a = imageData.data[i + 3]; // Alpha
                    
                    // Backend expects ARGB format (ucolor bit layout)
                    convertedData[i]     = a; // Alpha → position 0
                    convertedData[i + 1] = r; // Red → position 1
                    convertedData[i + 2] = g; // Green → position 2
                    convertedData[i + 3] = b; // Blue → position 3
                }
                
                // Create ImageData with converted format
                const convertedImageData = new ImageData(convertedData, imageData.width, imageData.height);
                ctx.putImageData(convertedImageData, 0, 0);
                
                // Convert to blob with PNG format
                canvas.toBlob(async (blob) => {
                    if (blob) {
                        try {
                            // Convert blob to ArrayBuffer exactly like file uploads
                            const arrayBuffer = await blob.arrayBuffer();
                            const uint8Array = new Uint8Array(arrayBuffer);
                            const imagePath = `/lux_files/${filename}`;
                            const imageName = filename.split('.')[0];

                            // Save to backend exactly like file uploads
                            if (window.module && window.module.FS) {
                                console.log('Writing camera capture as ARGB PNG file:', imagePath);
                                window.module.FS.writeFile(imagePath, uint8Array);
                                
                                // Add to scene and menu (same as upload)
                                if (typeof window.module.add_image_to_scene === 'function') {
                                    window.module.add_image_to_scene(imageName, imagePath);
                                }
                                if (typeof window.module.add_to_menu === 'function') {
                                    window.module.add_to_menu('source_image_menu', imageName);
                                }
                                if (typeof window.module.update_chosen_image === 'function') {
                                    window.module.update_chose_image("source_menu_image", imageName);
                                }
                                
                                // Notify parent component
                                if (onCapture) {
                                    onCapture({
                                        filename,
                                        imageName: imageName,
                                        imageData: convertedImageData, // Pass converted data
                                        blob
                                    });
                                }
                            } else {
                                throw new Error('WebAssembly module or filesystem not available');
                            }
                        } catch (saveError) {
                            console.error('Failed to save camera capture:', saveError);
                            throw saveError;
                        }
                    }
                }, 'image/png', 1.0); // Maximum quality PNG
            }
        } catch (err) {
            console.error('Capture failed:', err);
            // Show user-friendly error
            if (err.message.includes('module not ready')) {
                alert('Please wait for the app to finish loading before taking photos.');
            } else {
                alert('Failed to capture photo: ' + err.message);
            }
        }
    };

    // Camera settings panel
    const renderSettings = () => (
        <Paper 
            sx={{ 
                position: 'absolute', 
                top: 60, 
                right: 10, 
                p: 2, 
                minWidth: 250,
                zIndex: 1000
            }}
        >
            <Typography variant="h6" gutterBottom>Camera Settings</Typography>
            
            {/* Camera selection */}
            {devices.length > 1 && (
                <FormControl fullWidth sx={{ mb: 2 }}>
                    <InputLabel>Camera</InputLabel>
                    <Select
                        value={currentDeviceId || ''}
                        onChange={(e) => switchCamera(e.target.value)}
                    >
                        {devices.map((device) => (
                            <MenuItem key={device.deviceId} value={device.deviceId}>
                                {device.label || `Camera ${device.deviceId.slice(0, 8)}`}
                            </MenuItem>
                        ))}
                    </Select>
                </FormControl>
            )}

            {/* Resolution */}
            <Typography gutterBottom>Resolution</Typography>
            <Select
                fullWidth
                value={`${constraints.video.width?.ideal || 1280}x${constraints.video.height?.ideal || 720}`}
                onChange={(e) => {
                    const [w, h] = e.target.value.split('x').map(Number);
                    updateConstraints({
                        video: {
                            ...constraints.video,
                            width: { ideal: w },
                            height: { ideal: h }
                        }
                    });
                }}
                sx={{ mb: 2 }}
            >
                <MenuItem value="640x480">640x480</MenuItem>
                <MenuItem value="1280x720">1280x720 (HD)</MenuItem>
                <MenuItem value="1920x1080">1920x1080 (Full HD)</MenuItem>
                <MenuItem value="3840x2160">3840x2160 (4K)</MenuItem>
            </Select>

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

            {/* Live preview */}
            <FormControlLabel
                control={
                    <Switch
                        checked={livePreviewActive}
                        onChange={toggleLivePreview}
                    />
                }
                label="Live Effects Preview"
            />
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
        <Box className="camera-capture-container" sx={{ position: 'relative', width, height }}>
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

            {/* Hidden canvas for processing */}
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
                        <Typography>Loading image processing...</Typography>
                    </Box>
                </Box>
            )}

            {/* Camera controls overlay */}
            <CameraControls
                isStreaming={isStreaming}
                isCapturing={isCapturing}
                onCapture={handleCapture}
                onClose={onClose}
                onToggleSettings={() => setShowSettings(!showSettings)}
                onSwitchCamera={handleCameraSwitch}
                showSwitchCamera={isMobileDevice_ && cameraInfo.hasMultipleCameras} // Only show on mobile
                isSwitchingCamera={isSwitchingCamera}
                currentFacingMode={currentFacingMode}
                cameraInfo={cameraInfo}
                livePreviewActive={livePreviewActive}
                onToggleLivePreview={toggleLivePreview}
                moduleReady={moduleReady}
            />

            {/* Settings panel */}
            {showSettings && renderSettings()}

            {/* Live preview indicator */}
            {livePreviewActive && (
                <Box
                    sx={{
                        position: 'absolute',
                        top: 10,
                        left: 10,
                        display: 'flex',
                        alignItems: 'center',
                        bgcolor: 'rgba(0,0,0,0.7)',
                        color: 'white',
                        px: 1,
                        py: 0.5,
                        borderRadius: 1,
                        fontSize: '0.75rem'
                    }}
                >
                    <Zap size={16} style={{ marginRight: 4 }} />
                    Live Effects
                    {streamStats.fps > 0 && (
                        <Typography variant="caption" sx={{ ml: 1, color: 'lightgreen' }}>
                            {streamStats.fps.toFixed(1)} FPS
                        </Typography>
                    )}
                </Box>
            )}

            {/* Performance stats overlay (debug mode) */}
            {livePreviewActive && streamStats.active && (
                <Box
                    sx={{
                        position: 'absolute',
                        bottom: 10,
                        left: 10,
                        bgcolor: 'rgba(0,0,0,0.8)',
                        color: 'white',
                        px: 1,
                        py: 0.5,
                        borderRadius: 1,
                        fontSize: '0.7rem',
                        fontFamily: 'monospace'
                    }}
                >
                    <div>FPS: {streamStats.fps?.toFixed(1) || 0}</div>
                    <div>Frames: {streamStats.frame_count || 0}</div>
                    <div>Size: {streamStats.width}×{streamStats.height}</div>
                </Box>
            )}
        </Box>
    );
};

export default CameraCapture; 