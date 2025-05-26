import { useState, useRef, useCallback, useEffect } from 'react';

export const useCamera = () => {
    // Refs
    const videoRef = useRef(null);
    const canvasRef = useRef(null);
    const streamRef = useRef(null);

    // State
    const [isStreaming, setIsStreaming] = useState(false);
    const [isCapturing, setIsCapturing] = useState(false);
    const [error, setError] = useState(null);
    const [devices, setDevices] = useState([]);
    const [currentDeviceId, setCurrentDeviceId] = useState(null);
    const [constraints, setConstraints] = useState({
        video: {
            width: { ideal: 1280 },
            height: { ideal: 720 },
            facingMode: 'user' // Default to front camera on mobile
        },
        audio: false
    });

    // Enumerate available camera devices
    const enumerateDevices = useCallback(async () => {
        try {
            const devices = await navigator.mediaDevices.enumerateDevices();
            const videoDevices = devices.filter(device => device.kind === 'videoinput');
            setDevices(videoDevices);
            
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

    // Start camera stream
    const startCamera = useCallback(async (deviceId = null) => {
        try {
            setError(null);
            
            // Stop existing stream
            if (streamRef.current) {
                streamRef.current.getTracks().forEach(track => track.stop());
            }

            // Build constraints
            const videoConstraints = {
                ...constraints.video,
                ...(deviceId && { deviceId: { exact: deviceId } })
            };

            const streamConstraints = {
                ...constraints,
                video: videoConstraints
            };

            console.log('Starting camera with constraints:', streamConstraints);

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
                
                // Update current device ID
                if (deviceId) {
                    setCurrentDeviceId(deviceId);
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
                setError('Camera does not support the requested settings.');
            } else {
                setError(`Camera error: ${err.message || 'Unknown error'}`);
            }
            
            setIsStreaming(false);
        }
    }, [constraints, enumerateDevices]);

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

    // Switch to different camera
    const switchCamera = useCallback(async (deviceId) => {
        if (deviceId !== currentDeviceId) {
            await startCamera(deviceId);
        }
    }, [currentDeviceId, startCamera]);

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
        constraints,
        
        // Methods
        startCamera,
        stopCamera,
        switchCamera,
        capturePhoto,
        updateConstraints,
        enumerateDevices,
        isCameraSupported
    };
}; 