// Camera utility functions

/**
 * Check if camera is supported in the current browser
 */
export const isCameraSupported = () => {
    return !!(navigator.mediaDevices && navigator.mediaDevices.getUserMedia);
};

/**
 * Check if the device is likely a mobile device
 */
export const isMobileDevice = () => {
    const userAgent = navigator.userAgent;
    const hasTouch = navigator.maxTouchPoints && navigator.maxTouchPoints > 2;
    const isMobileUA = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(userAgent);
    
    return isMobileUA || hasTouch;
};

/**
 * Get the default camera facing mode based on device type
 * Mobile devices default to back camera ('environment')
 * Desktop devices default to front camera ('user')
 */
export const getDefaultCameraFacingMode = () => {
    return isMobileDevice() ? 'environment' : 'user';
};

/**
 * Check if camera switching controls should be shown
 * Only show camera switching on mobile devices with multiple cameras
 */
export const shouldShowCameraSwitching = (hasMultipleCameras = false) => {
    return isMobileDevice() && hasMultipleCameras;
};

/**
 * Get optimal camera constraints for the device
 */
export const getOptimalConstraints = (preferredFacing = null) => {
    const isMobile = isMobileDevice();
    const facingMode = preferredFacing || getDefaultCameraFacingMode();
    
    const baseConstraints = {
        video: {
            facingMode: facingMode,
            width: { ideal: isMobile ? 1280 : 1920 },
            height: { ideal: isMobile ? 720 : 1080 },
            frameRate: { ideal: 30, max: 60 }
        },
        audio: false
    };

    // Add mobile-specific optimizations
    if (isMobile) {
        baseConstraints.video.aspectRatio = { ideal: 16/9 };
        baseConstraints.video.resizeMode = 'crop-and-scale';
    }

    return baseConstraints;
};

/**
 * Request camera permissions with user-friendly error handling
 */
export const requestCameraPermission = async () => {
    try {
        const stream = await navigator.mediaDevices.getUserMedia({ video: true });
        // Stop the stream immediately - we just wanted to check permissions
        stream.getTracks().forEach(track => track.stop());
        return { success: true };
    } catch (error) {
        let message = 'Camera access failed';
        
        switch (error.name) {
            case 'NotAllowedError':
                message = 'Camera access denied. Please allow camera permissions in your browser settings.';
                break;
            case 'NotFoundError':
                message = 'No camera found. Please connect a camera and try again.';
                break;
            case 'NotReadableError':
                message = 'Camera is already in use by another application.';
                break;
            case 'OverconstrainedError':
                message = 'Camera does not support the requested settings.';
                break;
            case 'SecurityError':
                message = 'Camera access blocked due to security restrictions.';
                break;
            default:
                message = `Camera error: ${error.message || 'Unknown error'}`;
        }
        
        return { success: false, error: message };
    }
};

/**
 * Enumerate available camera devices
 */
export const getCameraDevices = async () => {
    try {
        const devices = await navigator.mediaDevices.enumerateDevices();
        const videoDevices = devices.filter(device => device.kind === 'videoinput');
        
        return {
            success: true,
            devices: videoDevices.map(device => ({
                deviceId: device.deviceId,
                label: device.label || `Camera ${device.deviceId.slice(0, 8)}`,
                groupId: device.groupId
            }))
        };
    } catch (error) {
        return {
            success: false,
            error: `Failed to enumerate devices: ${error.message}`,
            devices: []
        };
    }
};

/**
 * Convert ImageData to various formats
 */
export const convertImageData = {
    /**
     * Convert ImageData to Blob
     */
    toBlob: (imageData, format = 'image/png', quality = 0.9) => {
        return new Promise((resolve) => {
            const canvas = document.createElement('canvas');
            const ctx = canvas.getContext('2d');
            canvas.width = imageData.width;
            canvas.height = imageData.height;
            ctx.putImageData(imageData, 0, 0);
            canvas.toBlob(resolve, format, quality);
        });
    },

    /**
     * Convert ImageData to Data URL
     */
    toDataURL: (imageData, format = 'image/png', quality = 0.9) => {
        const canvas = document.createElement('canvas');
        const ctx = canvas.getContext('2d');
        canvas.width = imageData.width;
        canvas.height = imageData.height;
        ctx.putImageData(imageData, 0, 0);
        return canvas.toDataURL(format, quality);
    },

    /**
     * Convert ImageData to Uint8Array (for backend processing)
     */
    toUint8Array: (imageData) => {
        return new Uint8Array(imageData.data.buffer);
    }
};

/**
 * Apply basic image transformations
 */
export const transformImage = {
    /**
     * Flip image horizontally (mirror)
     */
    flipHorizontal: (imageData) => {
        const canvas = document.createElement('canvas');
        const ctx = canvas.getContext('2d');
        canvas.width = imageData.width;
        canvas.height = imageData.height;
        
        ctx.scale(-1, 1);
        ctx.translate(-canvas.width, 0);
        ctx.putImageData(imageData, 0, 0);
        
        return ctx.getImageData(0, 0, canvas.width, canvas.height);
    },

    /**
     * Rotate image by degrees
     */
    rotate: (imageData, degrees) => {
        const canvas = document.createElement('canvas');
        const ctx = canvas.getContext('2d');
        
        // For 90 or 270 degree rotations, swap width/height
        const isVerticalRotation = Math.abs(degrees) === 90 || Math.abs(degrees) === 270;
        canvas.width = isVerticalRotation ? imageData.height : imageData.width;
        canvas.height = isVerticalRotation ? imageData.width : imageData.height;
        
        ctx.translate(canvas.width / 2, canvas.height / 2);
        ctx.rotate((degrees * Math.PI) / 180);
        ctx.translate(-imageData.width / 2, -imageData.height / 2);
        ctx.putImageData(imageData, 0, 0);
        
        return ctx.getImageData(0, 0, canvas.width, canvas.height);
    },

    /**
     * Resize image to target dimensions
     */
    resize: (imageData, targetWidth, targetHeight) => {
        const canvas = document.createElement('canvas');
        const ctx = canvas.getContext('2d');
        canvas.width = targetWidth;
        canvas.height = targetHeight;
        
        // Create temporary canvas with original image
        const tempCanvas = document.createElement('canvas');
        const tempCtx = tempCanvas.getContext('2d');
        tempCanvas.width = imageData.width;
        tempCanvas.height = imageData.height;
        tempCtx.putImageData(imageData, 0, 0);
        
        // Draw resized
        ctx.drawImage(tempCanvas, 0, 0, targetWidth, targetHeight);
        
        return ctx.getImageData(0, 0, targetWidth, targetHeight);
    }
};

/**
 * Generate unique filename for camera captures
 */
export const generateCameraFilename = (prefix = 'camera-capture', extension = 'png') => {
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
    return `${prefix}-${timestamp}.${extension}`;
};

/**
 * Validate image dimensions for processing
 */
export const validateImageDimensions = (width, height, maxDimension = 4096, minDimension = 16) => {
    const errors = [];
    
    if (width < minDimension || height < minDimension) {
        errors.push(`Dimensions too small: ${width}x${height} (minimum: ${minDimension}x${minDimension})`);
    }
    
    if (width > maxDimension || height > maxDimension) {
        errors.push(`Dimensions too large: ${width}x${height} (maximum: ${maxDimension}x${maxDimension})`);
    }
    
    if (width % 2 !== 0 || height % 2 !== 0) {
        errors.push(`Dimensions must be even numbers for video encoding: ${width}x${height}`);
    }
    
    return {
        valid: errors.length === 0,
        errors
    };
};

/**
 * Get camera capabilities for a specific device
 */
export const getCameraCapabilities = async (deviceId) => {
    try {
        const stream = await navigator.mediaDevices.getUserMedia({
            video: { deviceId: { exact: deviceId } }
        });
        
        const track = stream.getVideoTracks()[0];
        const capabilities = track.getCapabilities();
        const settings = track.getSettings();
        
        // Stop the stream
        stream.getTracks().forEach(track => track.stop());
        
        return {
            success: true,
            capabilities,
            settings
        };
    } catch (error) {
        return {
            success: false,
            error: error.message
        };
    }
};

/**
 * Performance monitoring for camera operations
 */
export class CameraPerformanceMonitor {
    constructor() {
        this.metrics = {
            frameCount: 0,
            totalProcessingTime: 0,
            averageProcessingTime: 0,
            lastFrameTime: 0,
            fps: 0
        };
        this.startTime = performance.now();
    }

    recordFrame(processingTime) {
        this.metrics.frameCount++;
        this.metrics.totalProcessingTime += processingTime;
        this.metrics.averageProcessingTime = this.metrics.totalProcessingTime / this.metrics.frameCount;
        this.metrics.lastFrameTime = processingTime;
        
        // Calculate FPS
        const elapsed = (performance.now() - this.startTime) / 1000;
        this.metrics.fps = this.metrics.frameCount / elapsed;
    }

    getMetrics() {
        return { ...this.metrics };
    }

    reset() {
        this.metrics = {
            frameCount: 0,
            totalProcessingTime: 0,
            averageProcessingTime: 0,
            lastFrameTime: 0,
            fps: 0
        };
        this.startTime = performance.now();
    }
}

/**
 * Debug utilities for camera development
 */
export const cameraDebug = {
    logDeviceInfo: async () => {
        console.group('Camera Device Information');
        
        const devices = await getCameraDevices();
        console.log('Available cameras:', devices);
        
        console.log('User agent:', navigator.userAgent);
        console.log('Is mobile:', isMobileDevice());
        console.log('Camera supported:', isCameraSupported());
        console.log('Max touch points:', navigator.maxTouchPoints);
        
        if (navigator.mediaDevices) {
            console.log('Media devices API available');
            try {
                const constraints = getOptimalConstraints();
                console.log('Optimal constraints:', constraints);
            } catch (e) {
                console.error('Error getting constraints:', e);
            }
        } else {
            console.warn('Media devices API not available');
        }
        
        console.groupEnd();
    },

    logStreamInfo: (stream) => {
        console.group('Camera Stream Information');
        
        const tracks = stream.getVideoTracks();
        tracks.forEach((track, index) => {
            console.log(`Track ${index}:`, {
                label: track.label,
                kind: track.kind,
                enabled: track.enabled,
                muted: track.muted,
                readyState: track.readyState,
                settings: track.getSettings(),
                capabilities: track.getCapabilities()
            });
        });
        
        console.groupEnd();
    }
}; 