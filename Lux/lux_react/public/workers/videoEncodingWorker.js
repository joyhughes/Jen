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

// Remaining call sites are error paths, so log loudly.
const mobileLog = (message, data = null) => {
    console.error(`[Worker] ${message}`, data ?? '');
};

// Initialize WASM module
async function initWasm(wasmUrl) {
    if (!wasmUrl) {
        throw new Error('WASM URL not provided');
    }

    try {
        const moduleFactory = await import(wasmUrl);

        // Configure module without threading and with imported memory
        const moduleConfig = {
            print: (text) => mobileLog('[WASM Print]', text),
            printErr: (text) => mobileLog('[WASM Error]', text),
            locateFile: (path) => {
                return wasmUrl.replace('lux.js', path);
            },
            // Use imported memory configuration
            importMemory: true
        };

        CppModule = await moduleFactory.default(moduleConfig);
    } catch (error) {
        mobileLog('CRITICAL ERROR - Failed to initialize module:', error.message);
        throw new Error(`Module initialization failed: ${error.message}`);
    }

    // Verify critical recording functions exist
    const requiredFunctions = [
        'start_recording', 'stop_recording', 'get_recording_data',
        'get_recorded_frame_count', 'get_recording_state', 'get_recording_error',
        'is_recording', 'worker_add_frame'
    ];
    const missingFunctions = requiredFunctions.filter(fnName => !(CppModule && typeof CppModule[fnName] === 'function'));

    if (missingFunctions.length > 0) {
        mobileLog(`ERROR - Missing required recording functions: [${missingFunctions.join(', ')}]`);
        throw new Error(`Module recording functions missing: ${missingFunctions.join(', ')}`);
    }
}

function getCppError() {
    if (CppModule && typeof CppModule.get_recording_error === 'function') {
        try {
            return CppModule.get_recording_error();
        } catch (e) {
            console.error('[Worker Helper] Failed to get C++ error:', e);
            return 'Failed to get C++ error: ' + (e.message || e);
        }
    }
    return 'Module or get_recording_error not available';
}

// Process frames in batches
async function processFrameQueue() {
    
    if (isProcessingFrames || frameQueue.length === 0) {
        return;
    }
    
    isProcessingFrames = true;
    const batchSize = 5; // Process 5 frames at a time

    try {
        // Process frames while we have them and recording is still active (or we're in final processing)
        while (frameQueue.length > 0) {
            const batch = frameQueue.splice(0, batchSize);
            
            for (const frame of batch) {
                const frameStartTime = performance.now();
                try {
                    if (!CppModule || typeof CppModule.worker_add_frame !== 'function') {
                        console.error('[Worker] CppModule or worker_add_frame not available');
                        console.error('[Worker] - CppModule:', !!CppModule);
                        console.error('[Worker] - worker_add_frame type:', typeof (CppModule && CppModule.worker_add_frame));
                        continue;
                    }
                    
                    if (!frame.imageData || !frame.width || !frame.height) {
                        console.error('[Worker] Invalid frame data:', {
                            hasImageData: !!frame.imageData,
                            width: frame.width,
                            height: frame.height
                        });
                        continue;
                    }
                    
                    const success = CppModule.worker_add_frame(
                        frame.imageData,
                        frame.width,
                        frame.height
                    );
                    
                    const frameEndTime = performance.now();
                    const processingTime = frameEndTime - frameStartTime;
                    frameProcessingTimes.push(processingTime);
        
                    if (success) {
                        frameCount++;
                        
                        if (frameCount % 10 === 0) { // More frequent logging
                            const avgProcessingTime = frameProcessingTimes.reduce((a, b) => a + b, 0) / frameProcessingTimes.length;
                            const totalDuration = (performance.now() - recordingStartTime) / 1000;
                            const actualFps = frameCount / totalDuration;

                            self.postMessage({
                                type: 'recordingProgress',
                                frameCount: frameCount,
                                metrics: {
                                    avgProcessingTime,
                                    totalDuration,
                                    actualFps,
                                    queueSize: frameQueue.length
                                }
                            });
                            
                            // Reset metrics for next batch
                            frameProcessingTimes = [];
                        }
                    } else {
                        console.error(`[Worker] ✗ Failed to add frame ${frameCount + 1} - C++ function returned false`);
                        
                        // Get detailed error from C++
                        if (typeof CppModule.get_recording_error === 'function') {
                            const cppError = CppModule.get_recording_error();
                            console.error(`[Worker] C++ error: ${cppError}`);
                        }
                        
                        // Check recording state
                        if (typeof CppModule.get_recording_state === 'function') {
                            const state = CppModule.get_recording_state();
                            console.error(`[Worker] Recording state: ${state}`);
                        }
                        
                        // Check if still recording
                        if (typeof CppModule.is_recording === 'function') {
                            const isCppRecording = CppModule.is_recording();
                            console.error(`[Worker] C++ is_recording: ${isCppRecording}`);
                        }
                        
                        // Don't fail completely, continue with next frame
                        // self.postMessage({ type: 'error', error: 'Failed to add frame' });
                    }
                } catch (error) {
                    console.error(`[Worker] ✗ Exception adding frame ${frameCount + 1}:`, error);
                    console.error(`[Worker] Frame data:`, {
                        hasImageData: !!frame.imageData,
                        imageDataLength: frame.imageData ? frame.imageData.length : 0,
                        width: frame.width,
                        height: frame.height,
                        expectedLength: frame.width * frame.height * 4
                    });
                    // Don't fail completely, continue with next frame
                    // self.postMessage({ type: 'error', error: 'Error adding frame: ' + error.message });
                }
            }
            
            // Small delay to prevent blocking
            await new Promise(resolve => setTimeout(resolve, 0));
        }
    } finally {
        isProcessingFrames = false;
    }
}

let initPromise = null;

self.onmessage = async (event) => {
    const message = event.data;

    try {
        switch (message.type) {
            case 'init':
                if (initPromise) {
                    return;
                }

                initPromise = initWasm(message.wasmUrl);
                await initPromise;
                isInitialized = true;
                self.postMessage({ type: 'initialized' });
                break;

            case 'startRecording':

                // Messages posted right after 'init' arrive while the WASM
                // module is still loading; wait for it rather than failing.
                if (!isInitialized && initPromise) {
                    await initPromise;
                    isInitialized = true;
                }

                if (!isInitialized) {
                    mobileLog('ERROR: Worker not initialized');
                    throw new Error('Worker not initialized');
                }

                if (recordingInProgress) {
                    await stopCurrentRecording();
                }

                // CLEAN STATE: Reset all recording-related variables for fresh start
                frameCount = 0;
                frameQueue = []; // Clear any leftover frames from previous recording
                isProcessingFrames = false;
                recordingStartTime = performance.now();
                lastFrameTime = null;
                frameProcessingTimes = [];

                recordingInProgress = true;

                // ADAPTIVE DIMENSIONS: Use actual canvas dimensions for recording
                // This ensures compatibility with different image sizes and mobile devices
                let recordingWidth = message.options.width;
                let recordingHeight = message.options.height;
                
                // MOBILE COMPATIBILITY: Ensure dimensions are even numbers (required for H.264)
                if (recordingWidth % 2 !== 0) {
                    recordingWidth = recordingWidth - 1;
                }
                if (recordingHeight % 2 !== 0) {
                    recordingHeight = recordingHeight - 1;
                }
                
                // MOBILE COMPATIBILITY: Validate dimension limits for mobile devices
                const maxMobileDimension = 1920; // Safe limit for most mobile devices
                const minDimension = 16; // Minimum for H.264
                
                if (recordingWidth > maxMobileDimension || recordingHeight > maxMobileDimension) {
                }
                
                if (recordingWidth < minDimension || recordingHeight < minDimension) {
                    mobileLog(`ERROR: Dimensions too small (${recordingWidth}x${recordingHeight}), minimum is ${minDimension}x${minDimension}`);
                    recordingInProgress = false;
                    self.postMessage({ type: 'recordingStarted', success: false, error: 'Dimensions too small for H.264 encoding' });
                    return;
                }

                // Get buffer dimensions for comparison only (not for recording)
                const bufferWidth = CppModule.get_buf_width();
                const bufferHeight = CppModule.get_buf_height();

                const success = await CppModule.start_recording(
                    recordingWidth,  // Use canvas width (adaptive)
                    recordingHeight, // Use canvas height (adaptive)
                    message.options.fps,
                    message.options.bitrate,
                    message.options.codec,
                    message.options.format,
                    message.options.preset
                );

                if (success) {
                    // Verify recording state
                    if (typeof CppModule.get_recording_state === 'function') {
                        const state = CppModule.get_recording_state();
                    }
                    if (typeof CppModule.is_recording === 'function') {
                        const isRec = CppModule.is_recording();
                    }
                    
                    // Process any frames that were queued while waiting for backend to be ready
                    if (frameQueue.length > 0) {
                        processFrameQueue();
                    }
                    
                    self.postMessage({ type: 'recordingStarted', success: true });
                } else {
                    if (typeof CppModule.get_recording_error === 'function') {
                        const error = CppModule.get_recording_error();
                    }
                    recordingInProgress = false;
                    
                    // Clear any queued frames since recording failed
                    if (frameQueue.length > 0) {
                        frameQueue = [];
                    }
                    
                    self.postMessage({ type: 'recordingStarted', success: false, error: 'Failed to start H.264 recording' });
                }
                break;

            case 'addFrame':
                
                // SAFETY CHECK: Don't accept frames if recording is not active
                if (!recordingInProgress) {
                    return;
                }
                
                if (!CppModule || !CppModule.worker_add_frame) {
                    
                    // Queue the frame even if backend isn't ready yet
                    if (message.imageData && message.width && message.height) {
                        frameQueue.push({
                            imageData: message.imageData,
                            width: message.width,
                            height: message.height
                        });
                    }
                    return;
                }

                if (!message.imageData || !message.width || !message.height) {
                    return;
                }

                // Check current recording state
                if (typeof CppModule.get_recording_state === 'function') {
                    const currentState = CppModule.get_recording_state();
                }
                
                if (typeof CppModule.is_recording === 'function') {
                    const isCppRecording = CppModule.is_recording();
                    
                    // If C++ says it's not recording yet, queue the frame
                    if (!isCppRecording) {
                        frameQueue.push({
                            imageData: message.imageData,
                            width: message.width,
                            height: message.height
                        });
                        return;
                    }
                }

                frameQueue.push({
                    imageData: message.imageData,
                    width: message.width,
                    height: message.height
                });

                // Process frames if queue is getting large
                if (frameQueue.length >= MAX_QUEUE_SIZE) {
                    processFrameQueue();
                }
                break;

            case 'stopRecording':
                if (!recordingInProgress) {
                    return;
                }

                const stopTime = performance.now();
                const totalRecordingDuration = (stopTime - recordingStartTime) / 1000;

                // Immediately stop accepting new frames
                recordingInProgress = false;
                
                // Handle queue flushing based on message flag
                if (message.flushQueue) {
                    frameQueue = []; // Clear the queue - these frames won't be processed
                } else {
                    // Process any remaining frames that were already queued
                    if (frameQueue.length > 0) {
                        await processFrameQueue();
                    }
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
                    self.postMessage({ type: 'recorderState', state: 'error_getting_state', frameCount: 0, queueSize: 0 });
                }
                break;

            default:
        }
    } catch (error) {
        mobileLog('CRITICAL ERROR in message handler:', error.message);
        self.postMessage({ type: 'error', error: error.message });
    }
};

// Helper function to stop recording
async function stopCurrentRecording() {
    try {
        
        // Ensure recording is stopped
        recordingInProgress = false;
        
        // Ensure frame processing is stopped
        isProcessingFrames = false;

        // Process any remaining frames in queue (only if not already flushed)
        if (frameQueue.length > 0) {
            await processFrameQueue();
        }

        // Stop recording in C++
        const success = CppModule.stop_recording();

        if (success) {

            // Get the video data
            const videoDataArray = CppModule.get_recording_data();

            if (videoDataArray && videoDataArray.length > 0) {

                // Debug: Print first 16 bytes in hex
                const headerHex = Array.from(videoDataArray.slice(0, 16))
                    .map(b => b.toString(16).padStart(2, '0'))
                    .join(' ');

                // Verify video data integrity
                if (videoDataArray.length < 1000) { // Basic size check
                    throw new Error('Video data too small, likely corrupted');
                }

                // Create a proper MP4 container with metadata
                const videoDataBuffer = new ArrayBuffer(videoDataArray.length);
                const videoData = new Uint8Array(videoDataBuffer);
                videoData.set(videoDataArray);

                const finalFrameCount = CppModule.get_recorded_frame_count() || 0;

                // Verify MP4 header (ftyp box)
                const isMP4 = videoData[4] === 0x66 && videoData[5] === 0x74 && videoData[6] === 0x79 && videoData[7] === 0x70;
                if (!isMP4) {
                    // Don't fail - FFmpeg might use different container structure
                }

                // CLEAN STATE: Reset all variables for next recording
                frameQueue = []; // Ensure queue is empty
                isProcessingFrames = false;
                frameCount = 0; // Reset frame count for next recording
                recordingStartTime = null;
                lastFrameTime = null;
                frameProcessingTimes = [];

                // Send video data back to main thread with highest priority
                self.postMessage({
                    type: 'recordingStopped',
                    success: true,
                    videoData: videoData,
                    frameCount: finalFrameCount, // Use the final count from C++
                    mimeType: 'video/mp4; codecs="avc1.42E01E"', // H.264 baseline profile
                    duration: finalFrameCount / 30 // Approximate duration in seconds
                }, [videoDataBuffer]); // Transfer ownership for speed
            } else {
                self.postMessage({
                    type: 'recordingStopped',
                    success: false,
                    error: 'No video data available'
                });
            }
        } else {
            const error = getCppError();
            self.postMessage({
                type: 'recordingStopped',
                success: false,
                error: `Stop recording failed: ${error}`
            });
        }
    } catch (e) {
        self.postMessage({
            type: 'recordingStopped',
            success: false,
            error: 'Exception: ' + (e.message || e.toString())
        });
    } finally {
        // FINAL CLEANUP: Ensure state is completely reset regardless of success/failure
        recordingInProgress = false;
        isProcessingFrames = false;
        frameQueue = [];
        frameCount = 0;
        recordingStartTime = null;
        lastFrameTime = null;
        frameProcessingTimes = [];
        
    }
}

// Global error handlers
self.addEventListener('error', (event) => {
    mobileLog('CRITICAL ERROR in global error handler:', event.message, event.filename, event.lineno);
    if (self.postMessage) {
        try {
            self.postMessage({ type: 'error', error: 'Worker error: ' + event.message });
        } catch (e) {
            mobileLog('CRITICAL ERROR: Could not send error message');
        }
    }
});

self.addEventListener('unhandledrejection', (event) => {
    mobileLog('CRITICAL ERROR in global error handler:', event.reason);
    if (self.postMessage) {
        try {
            self.postMessage({ type: 'error', error: 'Worker promise rejection: ' + (event.reason.message || event.reason.toString()) });
        } catch (e) {
            mobileLog('CRITICAL ERROR: Could not send rejection message');
        }
    }
});

