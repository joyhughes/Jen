import { useState, useRef, useCallback, useEffect } from 'react';
import { isMobileDevice } from '../utils/cameraUtils';

export const useCamera = () => {
    // Refs
    const videoRef = useRef(null);
    const canvasRef = useRef(null);
    const streamRef = useRef(null);

    // Get mobile-specific default facing mode
    const isMobile = isMobileDevice();
    const defaultFacingMode = isMobile ? 'environment' : 'user'; // Back camera for mobile, front for desktop

    // State
    const [isStreaming, setIsStreaming] = useState(false);
    const [isCapturing, setIsCapturing] = useState(false);
    const [error, setError] = useState(null);
    const [devices, setDevices] = useState([]);
    const [currentDeviceId, setCurrentDeviceId] = useState(null);
    const [currentFacingMode, setCurrentFacingMode] = useState(defaultFacingMode);
    const [constraints, setConstraints] = useState({
        video: {
            width: { ideal: 1280 },
            height: { ideal: 720 },
            facingMode: defaultFacingMode // Use mobile-specific default
        },
        audio: false
    });

    console.log(`[useCamera] Initialized with default facing mode: ${defaultFacingMode} (mobile: ${isMobile})`);

    // Real-time processing state
    const [isLiveProcessing, setIsLiveProcessing] = useState(false);
    const [processingStats, setProcessingStats] = useState({ fps: 0, active: false });
    const liveProcessingRef = useRef(null);

    // Enumerate available camera devices
    const enumerateDevices = useCallback(async () => {
        try {
            const devices = await navigator.mediaDevices.enumerateDevices();
            const videoDevices = devices.filter(device => device.kind === 'videoinput');
            setDevices(videoDevices);
            
            console.log('[useCamera] Available cameras:', videoDevices.map(d => ({ 
                id: d.deviceId, 
                label: d.label,
                facingMode: d.label.toLowerCase().includes('back') ? 'environment' : 'user'
            })));
            
            // Auto-select first device if none selected
            if (videoDevices.length > 0 && !currentDeviceId) {
                setCurrentDeviceId(videoDevices[0].deviceId);
            }
            
            return videoDevices;
        } catch (err) {
            console.error('Error enumerating devices:', err);
            setError('Failed to access camera devices');
            return [];
        }
    }, [currentDeviceId]);

    // Get camera constraints based on facing mode or device ID
    const getCameraConstraints = useCallback((facingMode = null, deviceId = null) => {
        const baseConstraints = {
            width: { ideal: 1280 },
            height: { ideal: 720 }
        };

        // If we have a specific device ID, use it
        if (deviceId) {
            return {
                ...baseConstraints,
                deviceId: { exact: deviceId }
            };
        }

        // Otherwise use facing mode
        return {
            ...baseConstraints,
            facingMode: facingMode || currentFacingMode
        };
    }, [currentFacingMode]);

    // Start camera stream with enhanced mobile support
    const startCamera = useCallback(async (deviceId = null, facingMode = null) => {
        try {
            setError(null);
            
            // Stop existing stream
            if (streamRef.current) {
                streamRef.current.getTracks().forEach(track => track.stop());
            }

            // Build constraints with mobile support
            const videoConstraints = getCameraConstraints(facingMode, deviceId);
            const streamConstraints = {
                video: videoConstraints,
                audio: false
            };

            console.log(`[useCamera] Starting camera with constraints:`, streamConstraints);

            // Get user media
            const stream = await navigator.mediaDevices.getUserMedia(streamConstraints);
            streamRef.current = stream;

            // Set video source
            if (videoRef.current) {
                videoRef.current.srcObject = stream;
                
                // Wait for video to be ready
                await new Promise((resolve, reject) => {
                    const video = videoRef.current;
                    
                    const onLoadedMetadata = () => {
                        video.removeEventListener('loadedmetadata', onLoadedMetadata);
                        video.removeEventListener('error', onError);
                        
                        // Detect actual facing mode from stream
                        const track = stream.getVideoTracks()[0];
                        if (track && track.getSettings) {
                            const settings = track.getSettings();
                            if (settings.facingMode) {
                                setCurrentFacingMode(settings.facingMode);
                                console.log(`[useCamera] Camera started with facing mode: ${settings.facingMode}`);
                            }
                        }
                        
                        resolve();
                    };
                    
                    const onError = (err) => {
                        video.removeEventListener('loadedmetadata', onLoadedMetadata);
                        video.removeEventListener('error', onError);
                        reject(err);
                    };
                    
                    video.addEventListener('loadedmetadata', onLoadedMetadata);
                    video.addEventListener('error', onError);
                });

                setIsStreaming(true);
                
                // Update current device ID and facing mode
                if (deviceId) {
                    setCurrentDeviceId(deviceId);
                }
                if (facingMode) {
                    setCurrentFacingMode(facingMode);
                }
            }

            // Enumerate devices after successful stream
            await enumerateDevices();

        } catch (err) {
            console.error('Error starting camera:', err);
            
            // Provide user-friendly error messages
            if (err.name === 'NotAllowedError') {
                setError('Camera access denied. Please allow camera permissions and try again.');
            } else if (err.name === 'NotFoundError') {
                setError('No camera found. Please connect a camera and try again.');
            } else if (err.name === 'NotReadableError') {
                setError('Camera is already in use by another application.');
            } else if (err.name === 'OverconstrainedError') {
                setError('Camera does not support the requested settings. Trying fallback...');
                
                // Try fallback with basic constraints
                try {
                    const fallbackStream = await navigator.mediaDevices.getUserMedia({
                        video: { facingMode: facingMode || currentFacingMode },
                        audio: false
                    });
                    streamRef.current = fallbackStream;
                    if (videoRef.current) {
                        videoRef.current.srcObject = fallbackStream;
                        setIsStreaming(true);
                        setError(null);
                    }
                } catch (fallbackErr) {
                    setError(`Camera error: ${err.message || 'Unknown error'}`);
                }
            } else {
                setError(`Camera error: ${err.message || 'Unknown error'}`);
            }
            
            if (error) {
                setIsStreaming(false);
            }
        }
    }, [getCameraConstraints, currentFacingMode, enumerateDevices, error]);

    // Stop camera stream
    const stopCamera = useCallback(() => {
        if (streamRef.current) {
            streamRef.current.getTracks().forEach(track => track.stop());
            streamRef.current = null;
        }
        
        if (videoRef.current) {
            videoRef.current.srcObject = null;
        }
        
        setIsStreaming(false);
    }, []);

    // Switch to different camera by device ID
    const switchCamera = useCallback(async (deviceId) => {
        if (deviceId !== currentDeviceId) {
            await startCamera(deviceId);
        }
    }, [currentDeviceId, startCamera]);

    // Toggle between front and back camera (mobile-friendly)
    const toggleCameraFacing = useCallback(async () => {
        const newFacingMode = currentFacingMode === 'user' ? 'environment' : 'user';
        console.log(`[useCamera] Toggling camera from ${currentFacingMode} to ${newFacingMode}`);
        
        try {
            await startCamera(null, newFacingMode);
            return newFacingMode;
        } catch (error) {
            console.error('[useCamera] Failed to toggle camera facing:', error);
            throw error;
        }
    }, [currentFacingMode, startCamera]);

    // Switch to front camera
    const switchToFrontCamera = useCallback(async () => {
        if (currentFacingMode !== 'user') {
            await startCamera(null, 'user');
        }
    }, [currentFacingMode, startCamera]);

    // Switch to back camera
    const switchToBackCamera = useCallback(async () => {
        if (currentFacingMode !== 'environment') {
            await startCamera(null, 'environment');
        }
    }, [currentFacingMode, startCamera]);

    // Get current camera info
    const getCameraInfo = useCallback(() => {
        return {
            facingMode: currentFacingMode,
            deviceId: currentDeviceId,
            availableCameras: devices,
            isFrontCamera: currentFacingMode === 'user',
            isBackCamera: currentFacingMode === 'environment',
            hasMultipleCameras: devices.length > 1
        };
    }, [currentFacingMode, currentDeviceId, devices]);

    // Update camera constraints
    const updateConstraints = useCallback(async (newConstraints) => {
        setConstraints(newConstraints);
        
        // Restart camera with new constraints if currently streaming
        if (isStreaming) {
            await startCamera(currentDeviceId);
        }
    }, [isStreaming, currentDeviceId, startCamera]);

    // Capture photo from video stream
    const capturePhoto = useCallback(async () => {
        if (!videoRef.current || !isStreaming) {
            throw new Error('Camera not ready for capture');
        }

        setIsCapturing(true);

        try {
            const video = videoRef.current;
            
            // Create canvas for capture
            const canvas = canvasRef.current || document.createElement('canvas');
            const ctx = canvas.getContext('2d');
            
            // Set canvas size to match video
            canvas.width = video.videoWidth;
            canvas.height = video.videoHeight;
            
            // Draw video frame to canvas
            ctx.drawImage(video, 0, 0, canvas.width, canvas.height);
            
            // Get image data
            const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
            
            // Add flash effect (visual feedback)
            if (videoRef.current) {
                const flashOverlay = document.createElement('div');
                flashOverlay.style.cssText = `
                    position: absolute;
                    top: 0;
                    left: 0;
                    right: 0;
                    bottom: 0;
                    background: white;
                    opacity: 0.8;
                    pointer-events: none;
                    z-index: 9999;
                `;
                
                const container = videoRef.current.parentElement;
                if (container) {
                    container.appendChild(flashOverlay);
                    
                    // Remove flash after animation
                    setTimeout(() => {
                        if (container.contains(flashOverlay)) {
                            container.removeChild(flashOverlay);
                        }
                    }, 150);
                }
            }
            
            return imageData;
            
        } catch (err) {
            console.error('Error capturing photo:', err);
            throw err;
        } finally {
            setIsCapturing(false);
        }
    }, [isStreaming]);

    // Initialize devices on mount
    useEffect(() => {
        enumerateDevices();
        
        // Listen for device changes
        const handleDeviceChange = () => {
            enumerateDevices();
        };
        
        navigator.mediaDevices.addEventListener('devicechange', handleDeviceChange);
        
        return () => {
            navigator.mediaDevices.removeEventListener('devicechange', handleDeviceChange);
            stopCamera();
        };
    }, [enumerateDevices, stopCamera]);

    // Check for camera support
    const isCameraSupported = useCallback(() => {
        return !!(navigator.mediaDevices && navigator.mediaDevices.getUserMedia);
    }, []);

    // Start real-time processing
    const startLiveProcessing = useCallback(async () => {
        if (!window.module || isLiveProcessing) return false;
        
        try {
            const success = window.module.start_camera_stream();
            if (success) {
                setIsLiveProcessing(true);
                
                // Start stats monitoring
                const updateStats = () => {
                    if (window.module && window.module.get_camera_stream_stats) {
                        try {
                            const stats = JSON.parse(window.module.get_camera_stream_stats());
                            setProcessingStats(stats);
                        } catch (e) {
                            console.warn('Failed to get camera stats:', e);
                        }
                    }
                };
                
                liveProcessingRef.current = setInterval(updateStats, 1000); // Update every second
                return true;
            }
        } catch (error) {
            console.error('Failed to start live processing:', error);
        }
        return false;
    }, [isLiveProcessing]);

    // Stop real-time processing
    const stopLiveProcessing = useCallback(() => {
        if (!isLiveProcessing) return;
        
        try {
            if (window.module && window.module.stop_camera_stream) {
                window.module.stop_camera_stream();
            }
            
            if (liveProcessingRef.current) {
                clearInterval(liveProcessingRef.current);
                liveProcessingRef.current = null;
            }
            
            setIsLiveProcessing(false);
            setProcessingStats({ fps: 0, active: false });
        } catch (error) {
            console.error('Failed to stop live processing:', error);
        }
    }, [isLiveProcessing]);

    // Process single frame with optimized backend
    const processFrame = useCallback((imageData, width, height) => {
        if (!window.module || !window.module.update_camera_frame_optimized) {
            return false;
        }
        
        try {
            const success = window.module.update_camera_frame_optimized(
                imageData.data,
                width,
                height
            );
            
            if (success && window.module.process_camera_frame_with_effects) {
                window.module.process_camera_frame_with_effects();
            }
            
            return success;
        } catch (error) {
            console.error('Failed to process frame:', error);
            return false;
        }
    }, []);

    // Cleanup on unmount
    useEffect(() => {
        return () => {
            stopLiveProcessing();
        };
    }, [stopLiveProcessing]);

    return {
        // Refs
        videoRef,
        canvasRef,
        
        // State
        isStreaming,
        isCapturing,
        error,
        devices,
        currentDeviceId,
        currentFacingMode,
        constraints,
        
        // Methods
        startCamera,
        stopCamera,
        switchCamera,
        toggleCameraFacing,
        switchToFrontCamera,
        switchToBackCamera,
        getCameraInfo,
        capturePhoto,
        updateConstraints,
        enumerateDevices,
        isCameraSupported,
        startLiveProcessing,
        stopLiveProcessing,
        processFrame
    };
}; 