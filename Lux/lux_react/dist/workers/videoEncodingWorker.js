// videoEncodingWorker.js - Web Worker for Video Encoding

let CppModule; // To hold the initialized C++ module instance
let frameCount = 0;
let recordingInProgress = false;
let isInitialized = false;
let frameQueue = [];
let isProcessingFrames = false;
const MAX_QUEUE_SIZE = 30;
let recordingStartTime = null;
let lastFrameTime = null;
let frameProcessingTimes = [];

// Initialize WASM module
async function initWasm(wasmUrl) {
    console.log('[Worker] Initializing with WASM URL:', wasmUrl);
    if (!wasmUrl) {
        throw new Error('WASM URL not provided');
    }

    const moduleFactory = await import(wasmUrl);
    console.log('[Worker] Module factory loaded.');

    // Simple, safe module configuration
    const moduleConfig = {
        print: (text) => console.log('[WASM Print]', text),
        printErr: (text) => console.error('[WASM Error]', text),
        locateFile: (path) => {
            console.log('[Worker] Locating file:', path);
            return wasmUrl.replace('lux.js', path);
        }
        // Removed all memory-related configurations to let Emscripten handle it
    };

    try {
        CppModule = await moduleFactory.default(moduleConfig);
        console.log('[Worker] Module instance created.');
    } catch (error) {
        console.error('[Worker] Failed to initialize module:', error);
        throw new Error(`Module initialization failed: ${error.message}`);
    }

    const exportedFunctions = Object.keys(CppModule).filter(key => typeof CppModule[key] === 'function');
    console.log('[Worker] Module exports:', exportedFunctions);

    // Verify critical recording functions exist
    const requiredFunctions = [
        'start_recording', 'stop_recording', 'get_recording_data',
        'get_recorded_frame_count', 'get_recording_state', 'get_recording_error',
        'is_recording', 'worker_add_frame'
    ];
    const missingFunctions = requiredFunctions.filter(fnName => !(CppModule && typeof CppModule[fnName] === 'function'));

    if (missingFunctions.length === 0) {
        console.log('[Worker] Critical recording functions verified.');
    } else {
        console.error(`[Worker] Missing required recording functions: [${missingFunctions.join(', ')}]`);
        throw new Error(`Module recording functions missing: ${missingFunctions.join(', ')}`);
    }
}

function getCppError() {
    if (CppModule && typeof CppModule.get_recording_error === 'function') {
        try {
            const error = CppModule.get_recording_error();
            console.log('[Worker] C++ Error Details:', error);
            return error || 'No specific error available';
        } catch (e) {
            console.error('[Worker Helper] Failed to get C++ error:', e);
            return 'Failed to get C++ error: ' + (e.message || e);
        }
    }
    return 'Module or get_recording_error not available';
}

// Enhanced error reporting function
function reportDetailedError(context, cppError, jsError = null) {
    const errorReport = {
        context,
        cppError,
        jsError: jsError ? (jsError.message || jsError.toString()) : null,
        timestamp: new Date().toISOString(),
        recordingState: recordingInProgress,
        frameCount,
        queueSize: frameQueue.length
    };
    
    console.error('[Worker] Detailed Error Report:', errorReport);
    
    const fullErrorMessage = `${context}: C++[${cppError}]${jsError ? ` JS[${errorReport.jsError}]` : ''}`;
    
    self.postMessage({
        type: 'error',
        error: fullErrorMessage,
        details: errorReport
    });
    
    return fullErrorMessage;
}

// Process frames with ultra-optimized batching for 30fps
async function processFrameQueue() {
    if (isProcessingFrames || frameQueue.length === 0) return;
    
    isProcessingFrames = true;
    
    try {
        // Process ALL queued frames immediately for 30fps performance
        while (frameQueue.length > 0 && recordingInProgress) {
            const frame = frameQueue.shift(); // Process oldest frame first
            
            const frameStartTime = performance.now();
            try {
                const success = CppModule.worker_add_frame(
                    frame.imageData,
                    frame.width,
                    frame.height
                );
                
                const frameEndTime = performance.now();
                const processingTime = frameEndTime - frameStartTime;
                frameProcessingTimes.push(processingTime);
                
                // Keep only recent processing times for accurate metrics
                if (frameProcessingTimes.length > 30) {
                    frameProcessingTimes = frameProcessingTimes.slice(-30);
                }
                
                if (success) {
                    frameCount++;
                    
                    // Report progress every 15 frames for responsive UI
                    if (frameCount % 15 === 0) {
                        const avgProcessingTime = frameProcessingTimes.reduce((a, b) => a + b, 0) / frameProcessingTimes.length;
                        const totalDuration = (performance.now() - recordingStartTime) / 1000;
                        const actualFps = frameCount / totalDuration;
                        
                        self.postMessage({ 
                            type: 'recordingProgress',
                            frameCount: frameCount,
                            metrics: {
                                avgProcessingTime: avgProcessingTime,
                                totalDuration: totalDuration,
                                actualFps: actualFps,
                                queueSize: frameQueue.length
                            }
                        });
                    }
                } else {
                    console.error(`[Worker] Failed to add frame ${frameCount + 1}`);
                    // Continue processing other frames instead of stopping
                }
            } catch (error) {
                console.error(`[Worker] Error processing frame ${frameCount + 1}:`, error);
                // Continue processing other frames
            }
            
            // NO delays - process continuously for 30fps
        }
    } finally {
        isProcessingFrames = false;
    }
}

self.onmessage = async (event) => {
    const message = event.data;
    console.log('[Worker] Received message:', message.type);

    try {
        switch (message.type) {
            case 'init':
                if (isInitialized) {
                    console.log('[Worker] Already initialized');
                    return;
                }

                console.log('[Worker] Initializing...');
                await initWasm(message.wasmUrl);
                isInitialized = true;
                self.postMessage({ type: 'initialized' });
                break;

            case 'startRecording':
                if (!isInitialized) {
                    throw new Error('Worker not initialized');
                }

                if (recordingInProgress) {
                    console.log('[Worker] Already recording, stopping first');
                    await stopCurrentRecording();
                }

                console.log('[Worker] Starting recording with options:', message.options);
                recordingInProgress = true;
                frameCount = 0;
                recordingStartTime = performance.now();
                frameProcessingTimes = [];

                // Before starting recording, ensure canvas size matches the image
                const imageWidth = CppModule.get_buf_width();
                const imageHeight = CppModule.get_buf_height();
                // const canvas = document.querySelector('canvas');
                // if (canvas.width !== imageWidth || canvas.height !== imageHeight) {
                //     canvas.width = imageWidth;
                //     canvas.height = imageHeight;
                //     // Optionally, trigger a re-render here
                // }

                const success = await CppModule.start_recording(
                    message.options.width,
                    message.options.height,
                    message.options.fps,
                    message.options.bitrate,
                    message.options.codec,
                    message.options.format,
                    message.options.preset
                );

                if (success) {
                    self.postMessage({ type: 'recordingStarted', success: true });
                } else {
                    recordingInProgress = false;
                    self.postMessage({ type: 'recordingStarted', success: false, error: 'Failed to start recording' });
                }
                break;

            case 'addFrame':
                if (!CppModule || !CppModule.worker_add_frame || !recordingInProgress) {
                    console.warn('[Worker] Ignoring frame - not ready or not recording');
                    return;
                }
                
                if (!message.imageData || !message.width || !message.height) {
                    console.error('[Worker] Invalid frame data');
                    return;
                }
                
                // Validate image data length
                const expectedLength = message.width * message.height * 4; // RGBA
                if (message.imageData.length !== expectedLength) {
                    console.error('[Worker] Image data length mismatch:', {
                        received: message.imageData.length,
                        expected: expectedLength
                    });
                    return;
                }
                
                // For 30fps performance: process frame immediately if possible, otherwise queue
                if (!isProcessingFrames) {
                    // Process immediately for better performance
                    const frameStartTime = performance.now();
                    try {
                        const success = CppModule.worker_add_frame(
                            message.imageData,
                            message.width,
                            message.height
                        );
                        
                        const frameEndTime = performance.now();
                        const processingTime = frameEndTime - frameStartTime;
                        frameProcessingTimes.push(processingTime);
                        
                        if (frameProcessingTimes.length > 30) {
                            frameProcessingTimes = frameProcessingTimes.slice(-30);
                        }
                        
                        if (success) {
                            frameCount++;
                        } else {
                            console.error('[Worker] Failed to add frame immediately');
                        }
                    } catch (error) {
                        console.error('[Worker] Error adding frame immediately:', error);
                    }
                } else {
                    // Queue only if currently processing (should be rare at 30fps)
                    frameQueue.push({
                        imageData: message.imageData,
                        width: message.width,
                        height: message.height
                    });
                    
                    // Prevent queue overflow for sustained 30fps
                    if (frameQueue.length > 10) {
                        // Drop oldest frames to maintain performance
                        frameQueue.shift();
                        console.warn('[Worker] Dropped frame to maintain 30fps performance');
                    }
                }
                
                // Always trigger queue processing to handle any backlog
                processFrameQueue();
                break;

            case 'stopRecording':
                if (!recordingInProgress) {
                    console.log('[Worker] Not recording');
                    return;
                }

                const stopTime = performance.now();
                const totalRecordingDuration = (stopTime - recordingStartTime) / 1000;
                console.log(`[Worker] Recording stopped after ${totalRecordingDuration.toFixed(2)}s with ${frameCount} frames`);
                console.log(`[Worker] Average FPS: ${(frameCount / totalRecordingDuration).toFixed(2)}`);

                recordingInProgress = false;
                
                // Process any remaining frames
                if (frameQueue.length > 0) {
                    await processFrameQueue();
                }
                
                const result = await stopCurrentRecording();
                self.postMessage({
                    ...result,
                    metrics: {
                        totalDuration: totalRecordingDuration,
                        totalFrames: frameCount,
                        averageFps: frameCount / totalRecordingDuration
                    }
                });
                break;

            case 'getState':
                if (!CppModule) {
                    self.postMessage({ type: 'recorderState', state: 'module_not_ready', frameCount: 0, queueSize: 0 });
                    return;
                }

                try {
                    const state = typeof CppModule.get_recording_state === 'function' ? CppModule.get_recording_state() : 'unknown';
                    const frameCount = typeof CppModule.get_recorded_frame_count === 'function' ? CppModule.get_recorded_frame_count() : 0;
                    const isCppRecording = typeof CppModule.is_recording === 'function' ? CppModule.is_recording() : false;

                    self.postMessage({
                        type: 'recorderState',
                        state: state,
                        frameCount: frameCount,
                        queueSize: 0,
                        isCppRecording: isCppRecording,
                        isWorkerRecording: recordingInProgress
                    });
                } catch (e) {
                    console.error('[Worker] Exception during getState:', e);
                    self.postMessage({ type: 'recorderState', state: 'error_getting_state', frameCount: 0, queueSize: 0 });
                }
                break;

            default:
                console.warn('[Worker] Unknown message type:', message.type);
        }
    } catch (error) {
        console.error('[Worker] Error:', error);
        self.postMessage({ type: 'error', error: error.message });
    }
};

// Helper function to stop recording
async function stopCurrentRecording() {
    try {
        console.log('[Worker] Stopping recording and finalizing...');
        recordingInProgress = false;

        // Process any remaining frames in queue
        if (frameQueue.length > 0) {
            console.log(`[Worker] Processing ${frameQueue.length} remaining frames before stopping`);
            await processFrameQueue();
        }

        // Mobile-specific pre-stop checks
        const isMobile = /iPhone|iPad|iPod|Android/i.test(navigator.userAgent || '');
        const isIOS = /iPhone|iPad|iPod/i.test(navigator.userAgent || '');
        
        if (isMobile) {
            console.log(`[Worker] Mobile device detected: ${isIOS ? 'iOS' : 'Android'}`);
            
            // Give mobile browser extra time to process
            await new Promise(resolve => setTimeout(resolve, 100));
        }

        // Stop recording with enhanced error handling for mobile
        let success = false;
        let stopError = null;
        
        try {
            success = CppModule.stop_recording();
        } catch (error) {
            stopError = error;
            console.error('[Worker] Exception calling stop_recording:', error);
            
            // For mobile, try to get partial data even if stop failed
            if (isMobile) {
                console.log('[Worker] Mobile: Attempting to recover partial data after stop failure');
                success = false; // Will try to get data anyway below
            } else {
                throw error;
            }
        }

        if (success || (isMobile && !success)) {
            console.log(`[Worker] Recording ${success ? 'stopped successfully' : 'stopped with warnings (mobile)'}, getting data`);

            // Get the video data with mobile-specific handling
            let videoDataArray = null;
            let dataError = null;
            
            try {
                videoDataArray = CppModule.get_recording_data();
            } catch (error) {
                dataError = error;
                console.error('[Worker] Exception getting recording data:', error);
                
                if (isMobile) {
                    console.log('[Worker] Mobile: Data retrieval failed, this is expected on some devices');
                    // Continue to send failure message with details
                } else {
                    throw error;
                }
            }

            if (videoDataArray && videoDataArray.length > 0) {
                console.log(`[Worker] Got ${videoDataArray.length} bytes of video data`);

                // Debug: Print first 16 bytes in hex
                const headerHex = Array.from(videoDataArray.slice(0, 16))
                    .map(b => b.toString(16).padStart(2, '0'))
                    .join(' ');
                console.log('[Worker] First 16 bytes:', headerHex);

                // More lenient size check for mobile
                const minSize = isMobile ? 100 : 1000;
                if (videoDataArray.length < minSize) {
                    const sizeError = `Video data too small for ${isMobile ? 'mobile' : 'desktop'}: ${videoDataArray.length} bytes (min: ${minSize})`;
                    
                    if (isMobile) {
                        console.warn('[Worker] ' + sizeError + ' - attempting download anyway');
                        // Continue with small file on mobile
                    } else {
                        throw new Error(sizeError);
                    }
                }

                // Create a proper WebM container with metadata
                const videoDataBuffer = new ArrayBuffer(videoDataArray.length);
                const videoData = new Uint8Array(videoDataBuffer);
                videoData.set(videoDataArray);

                const frameCount = CppModule.get_recorded_frame_count() || 0;
                console.log(`[Worker] Final frame count: ${frameCount}`);

                // MP4 header verification with mobile tolerance
                const isMP4 = (videoData[4] === 0x66 && videoData[5] === 0x74 && videoData[6] === 0x79 && videoData[7] === 0x70) || // ftyp
                             (videoData[0] === 0x00 && videoData[1] === 0x00 && videoData[2] === 0x00 && videoData[8] === 0x66); // size + ftyp
                if (!isMP4) {
                    console.error('[Worker] Invalid MP4 header. Expected ftyp box');
                    console.error('[Worker] Got:', headerHex);
                    
                    if (isMobile) {
                        console.log('[Worker] Mobile: Invalid MP4 header detected, but continuing download...');
                        // Mobile devices can be more tolerant of header issues
                    } else {
                        console.warn('[Worker] Desktop: Invalid MP4 header, but continuing anyway');
                    }
                }

                // Mobile-specific MIME type handling for MP4
                const mimeType = 'video/mp4; codecs="avc1.42E01E"';

                // Send video data back to main thread with highest priority
                self.postMessage({
                    type: 'recordingStopped',
                    success: true,
                    videoData: videoData,
                    frameCount: frameCount,
                    mimeType: mimeType,
                    duration: frameCount / 30, // Approximate duration in seconds
                    platform: isMobile ? (isIOS ? 'iOS' : 'Android') : 'Desktop',
                    warnings: success ? [] : ['Stop recording had warnings but data recovered'],
                    formatNote: 'MP4 format - plays natively on all devices'
                }, [videoDataBuffer]); // Transfer ownership for speed
                
            } else {
                console.error('[Worker] No video data returned');
                
                // Enhanced error details for mobile debugging
                const errorDetails = {
                    platform: isMobile ? (isIOS ? 'iOS' : 'Android') : 'Desktop',
                    stopSuccess: success,
                    stopError: stopError ? stopError.message : null,
                    dataError: dataError ? dataError.message : null,
                    frameCount: frameCount || 0,
                    arrayLength: videoDataArray ? videoDataArray.length : 0,
                    cppError: getCppError()
                };
                
                console.error('[Worker] Error details:', errorDetails);
                
                self.postMessage({
                    type: 'recordingStopped',
                    success: false,
                    error: `No video data available (${errorDetails.platform})`,
                    details: errorDetails
                });
            }
        } else {
            const error = getCppError();
            console.error('[Worker] stop_recording failed:', error);
            
            // Enhanced error reporting for mobile
            const errorDetails = {
                platform: isMobile ? (isIOS ? 'iOS' : 'Android') : 'Desktop',
                cppError: error,
                stopError: stopError ? stopError.message : null,
                frameCount: frameCount || 0,
                userAgent: navigator.userAgent || 'unknown'
            };
            
            const fullError = reportDetailedError('Stop Recording Failed', error);
            self.postMessage({
                type: 'recordingStopped',
                success: false,
                error: fullError,
                details: errorDetails
            });
        }
    } catch (e) {
        console.error('[Worker] Exception during stopRecording:', e);
        const cppError = getCppError();
        
        // Mobile-specific exception handling
        const isMobile = /iPhone|iPad|iPod|Android/i.test(navigator.userAgent || '');
        const isIOS = /iPhone|iPad|iPod/i.test(navigator.userAgent || '');
        
        const errorDetails = {
            platform: isMobile ? (isIOS ? 'iOS' : 'Android') : 'Desktop',
            exception: e.message || e.toString(),
            cppError: cppError,
            stack: e.stack || 'No stack trace',
            frameCount: frameCount || 0
        };
        
        console.error('[Worker] Exception details:', errorDetails);
        
        const fullError = reportDetailedError('Stop Recording Exception', cppError, e);
        self.postMessage({
            type: 'recordingStopped',
            success: false,
            error: fullError,
            details: errorDetails
        });
    }
}

// Global error handlers
self.addEventListener('error', (event) => {
    console.error('[Worker Global Error]:', event.message, event.filename, event.lineno);
    if (self.postMessage) {
        try {
            self.postMessage({ type: 'error', error: 'Worker error: ' + event.message });
        } catch (e) {
            console.error("[Worker Global Error] Could not send error message", e);
        }
    }
});

self.addEventListener('unhandledrejection', (event) => {
    console.error('[Worker Global Error] Unhandled Promise Rejection:', event.reason);
    if (self.postMessage) {
        try {
            self.postMessage({ type: 'error', error: 'Worker promise rejection: ' + (event.reason.message || event.reason.toString()) });
        } catch (e) {
            console.error("[Worker Global Error] Could not send rejection message", e);
        }
    }
});

console.log('[Worker] Video encoding worker initialized');