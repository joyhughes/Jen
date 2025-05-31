import React, { useRef, useEffect, useState } from 'react';

class LiveCameraManager {
    constructor() {
        this.videoRef = null;
        this.canvasRef = null;
        this.animationFrameRef = null;
        this.lastFrameTimeRef = 0;
        this.isActive = false;
        this.cameraReady = false;
        
        // Performance tracking
        this.performanceRef = {
            frameCount: 0,
            lastFpsUpdate: 0,
            currentFps: 0,
            avgFrameTime: 0,
            droppedFrames: 0,
            skipCounter: 0,
            targetFrameTime: 16.67, // 60fps = 16.67ms per frame
            performanceMode: 'auto'
        };

        // Camera settings
        this.settings = {
            fps: 60,
            quality: 0.25,
            mirrorMode: true,
            adaptiveQuality: true,
            bufferReuse: true,
            skipFrames: 0,
            lowLatencyMode: true,
            fastScaling: true
        };

        this.FRAME_INTERVAL = 1000 / this.settings.fps;
        
        // Create hidden video and canvas elements
        this.createElements();
    }

    createElements() {
        // Create hidden video element
        this.videoRef = document.createElement('video');
        this.videoRef.autoplay = true;
        this.videoRef.playsInline = true;
        this.videoRef.muted = true;
        this.videoRef.style.display = 'none';
        document.body.appendChild(this.videoRef);

        // Create hidden canvas element
        this.canvasRef = document.createElement('canvas');
        this.canvasRef.style.display = 'none';
        document.body.appendChild(this.canvasRef);
    }

    async start() {
        try {
            console.log('[LiveCamera] Starting camera with backend integration...');

            // Backend initialization
            if (window.module && typeof window.module.ultra_start_camera_stream === 'function') {
                console.log('[LiveCamera] Starting backend camera stream...');
                const backendStarted = window.module.ultra_start_camera_stream();
                
                if (!backendStarted) {
                    throw new Error('Backend camera stream failed to start');
                }

                // Switch source to camera
                if (typeof window.module.update_source_name === 'function') {
                    window.module.update_source_name('ultra_camera');
                    console.log('[LiveCamera] Switched source to ultra_camera');
                }
            }

            // Start camera stream
            const stream = await navigator.mediaDevices.getUserMedia({
                video: {
                    width: { ideal: 640, max: 1280 },
                    height: { ideal: 480, max: 720 },
                    frameRate: { ideal: 60, min: 30 },
                    facingMode: 'user',
                    aspectRatio: { ideal: 4 / 3 },
                    resizeMode: 'crop-and-scale'
                },
                audio: false
            });

            this.videoRef.srcObject = stream;
            
            // Configure video for performance
            this.videoRef.playsInline = true;
            this.videoRef.muted = true;
            this.videoRef.controls = false;
            this.videoRef.preload = 'none';

            return new Promise((resolve, reject) => {
                this.videoRef.onloadedmetadata = () => {
                    console.log(`[LiveCamera] Video loaded: ${this.videoRef.videoWidth}x${this.videoRef.videoHeight}`);
                    
                    // Reset performance counters
                    this.performanceRef = {
                        frameCount: 0,
                        lastFpsUpdate: performance.now(),
                        currentFps: 0,
                        avgFrameTime: 0,
                        droppedFrames: 0,
                        skipCounter: 0,
                        targetFrameTime: 16.67,
                        performanceMode: 'auto'
                    };

                    this.cameraReady = true;
                    this.isActive = true;

                    // Start frame processing
                    setTimeout(() => {
                        this.animationFrameRef = requestAnimationFrame(() => this.processFrame());
                    }, 50);

                    resolve();
                };

                this.videoRef.onerror = (error) => {
                    console.error('[LiveCamera] Video error:', error);
                    reject(error);
                };

                this.videoRef.play().catch(reject);
            });

        } catch (error) {
            console.error('[LiveCamera] Failed to start:', error);
            throw error;
        }
    }

    stop() {
        console.log('[LiveCamera] Stopping camera...');

        // Backend cleanup
        if (window.module && typeof window.module.ultra_stop_camera_stream === 'function') {
            const backendStopped = window.module.ultra_stop_camera_stream();
            if (backendStopped) {
                console.log('[LiveCamera] Backend camera stream stopped');
            }
        }

        // Frontend cleanup
        if (this.animationFrameRef) {
            cancelAnimationFrame(this.animationFrameRef);
            this.animationFrameRef = null;
        }

        if (this.videoRef && this.videoRef.srcObject) {
            const tracks = this.videoRef.srcObject.getTracks();
            tracks.forEach(track => track.stop());
            this.videoRef.srcObject = null;
        }

        this.isActive = false;
        this.cameraReady = false;

        console.log('[LiveCamera] Camera stopped');
    }

    processFrame() {
        const perf = this.performanceRef;

        // Basic state checks
        if (!this.isActive || !this.videoRef || !this.canvasRef || !this.cameraReady) {
            return;
        }

        const currentTime = performance.now();

        // Frame rate limiting
        let dynamicInterval = this.FRAME_INTERVAL;
        if (this.settings.adaptiveQuality && perf.avgFrameTime > perf.targetFrameTime * 1.5) {
            dynamicInterval = Math.max(this.FRAME_INTERVAL, perf.avgFrameTime * 2);
        }

        if (currentTime - this.lastFrameTimeRef < dynamicInterval) {
            this.animationFrameRef = requestAnimationFrame(() => this.processFrame());
            return;
        }

        const frameStartTime = currentTime;
        const video = this.videoRef;
        const canvas = this.canvasRef;

        // Check video readiness
        if (video.readyState < 2) {
            perf.droppedFrames++;
            this.animationFrameRef = requestAnimationFrame(() => this.processFrame());
            return;
        }

        const ctx = canvas.getContext('2d');
        const BACKEND_WIDTH = 256;
        const BACKEND_HEIGHT = 256;

        // Configure canvas
        if (canvas.width !== BACKEND_WIDTH || canvas.height !== BACKEND_HEIGHT) {
            canvas.width = BACKEND_WIDTH;
            canvas.height = BACKEND_HEIGHT;
            ctx.imageSmoothingEnabled = true;
            ctx.imageSmoothingQuality = 'high';
            ctx.globalCompositeOperation = 'source-over';
        }

        try {
            const videoWidth = video.videoWidth;
            const videoHeight = video.videoHeight;

            if (videoWidth === 0 || videoHeight === 0) {
                perf.droppedFrames++;
                this.animationFrameRef = requestAnimationFrame(() => this.processFrame());
                return;
            }

            // Square crop for kaleidoscope effects
            const minDim = Math.min(videoWidth, videoHeight);
            const sourceX = Math.round((videoWidth - minDim) * 0.5);
            const sourceY = Math.round((videoHeight - minDim) * 0.5);

            // Mirror mode handling
            if (this.settings.mirrorMode) {
                ctx.setTransform(-1, 0, 0, 1, BACKEND_WIDTH, 0);
            } else {
                ctx.setTransform(1, 0, 0, 1, 0, 0);
            }

            // Draw video frame to canvas
            ctx.drawImage(
                video,
                sourceX, sourceY, minDim, minDim,
                0, 0, BACKEND_WIDTH, BACKEND_HEIGHT
            );

            // Backend processing
            if (window.module && typeof window.module.ultra_update_camera_frame === 'function') {
                try {
                    const imageData = ctx.getImageData(0, 0, BACKEND_WIDTH, BACKEND_HEIGHT);

                    // Convert RGBA to ARGB format
                    const convertedData = new Uint8ClampedArray(imageData.data.length);
                    
                    for (let i = 0; i < imageData.data.length; i += 4) {
                        const r = imageData.data[i];
                        const g = imageData.data[i + 1];
                        const b = imageData.data[i + 2];
                        const a = imageData.data[i + 3];
                        
                        convertedData[i] = a;     // Alpha
                        convertedData[i + 1] = r; // Red
                        convertedData[i + 2] = g; // Green
                        convertedData[i + 3] = b; // Blue
                    }

                    // Send to backend
                    const success = window.module.ultra_update_camera_frame(
                        convertedData,
                        BACKEND_WIDTH,
                        BACKEND_HEIGHT
                    );

                    if (success && typeof window.module.ultra_process_camera_with_kaleidoscope === 'function') {
                        window.module.ultra_process_camera_with_kaleidoscope();
                    }

                } catch (backendError) {
                    console.warn('[LiveCamera] Backend error:', backendError);
                    perf.droppedFrames++;
                }
            }

            // Performance tracking
            const frameTime = performance.now() - frameStartTime;
            perf.frameCount++;

            if (perf.frameCount % 3 === 0) {
                perf.avgFrameTime = (perf.avgFrameTime * 0.8) + (frameTime * 0.2);
            }

            // FPS reporting
            if (perf.frameCount % 60 === 0) {
                const elapsed = currentTime - perf.lastFpsUpdate;
                if (elapsed > 0) {
                    perf.currentFps = Math.round(60000 / elapsed);
                    perf.lastFpsUpdate = currentTime;
                    console.log(`[LiveCamera] Performance: ${perf.currentFps}fps, ${perf.avgFrameTime.toFixed(1)}ms avg`);
                }
            }

        } catch (error) {
            console.warn('[LiveCamera] Frame error:', error);
            perf.droppedFrames++;
        }

        this.lastFrameTimeRef = currentTime;

        // Schedule next frame
        if (this.settings.lowLatencyMode && perf.avgFrameTime < perf.targetFrameTime) {
            setTimeout(() => {
                this.animationFrameRef = requestAnimationFrame(() => this.processFrame());
            }, 1);
        } else {
            this.animationFrameRef = requestAnimationFrame(() => this.processFrame());
        }
    }

    cleanup() {
        this.stop();
        
        // Remove elements
        if (this.videoRef && this.videoRef.parentNode) {
            this.videoRef.parentNode.removeChild(this.videoRef);
        }
        if (this.canvasRef && this.canvasRef.parentNode) {
            this.canvasRef.parentNode.removeChild(this.canvasRef);
        }
    }
}

// Global instance
let liveCameraManager = null;

export const createLiveCameraManager = () => {
    if (liveCameraManager) {
        liveCameraManager.cleanup();
    }
    liveCameraManager = new LiveCameraManager();
    return liveCameraManager;
};

export const getLiveCameraManager = () => {
    return liveCameraManager;
};

export const destroyLiveCameraManager = () => {
    if (liveCameraManager) {
        liveCameraManager.cleanup();
        liveCameraManager = null;
    }
};

export default LiveCameraManager; 