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

    // Configure module without threading and with imported memory
    const moduleConfig = {
        print: (text) => console.log('[WASM Print]', text),
        printErr: (text) => console.error('[WASM Error]', text),
        locateFile: (path) => {
            console.log('[Worker] Locating file:', path);
            return wasmUrl.replace('lux.js', path);
        },
        // Use imported memory configuration
        importMemory: true
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
    console.log('[Worker] === PROCESS FRAME QUEUE START ===');
    console.log('[Worker] isProcessingFrames:', isProcessingFrames);
    console.log('[Worker] frameQueue.length:', frameQueue.length);
    console.log('[Worker] recordingInProgress:', recordingInProgress);
    
    if (isProcessingFrames || frameQueue.length === 0) {
        console.log('[Worker] Skipping frame processing - already processing or empty queue');
        return;
    }
    
    isProcessingFrames = true;
    const batchSize = 5; // Process 5 frames at a time
    console.log('[Worker] Starting frame processing with batch size:', batchSize);

    try {
        while (frameQueue.length > 0 && recordingInProgress) {
            const batch = frameQueue.splice(0, batchSize);
            console.log('[Worker] Processing batch of', batch.length, 'frames. Remaining in queue:', frameQueue.length);
            
            for (const frame of batch) {
                const frameStartTime = performance.now();
                try {
                    // CRITICAL: Add detailed logging for frame processing
                    console.log(`[Worker] === PROCESSING FRAME ${frameCount + 1} ===`);
                    console.log(`[Worker] Frame dimensions: ${frame.width}x${frame.height}`);
                    console.log(`[Worker] Frame imageData type:`, frame.imageData.constructor.name);
                    console.log(`[Worker] Frame imageData length:`, frame.imageData.length);
                    console.log(`[Worker] Expected length:`, frame.width * frame.height * 4);
                    console.log(`[Worker] Queue remaining: ${frameQueue.length}, recording: ${recordingInProgress}`);
                    
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
                    
                    console.log('[Worker] Calling CppModule.worker_add_frame...');
                    const success = CppModule.worker_add_frame(
                        frame.imageData,
                        frame.width,
                        frame.height
                    );
                    console.log('[Worker] worker_add_frame returned:', success);
                    
                    const frameEndTime = performance.now();
                    const processingTime = frameEndTime - frameStartTime;
                    frameProcessingTimes.push(processingTime);
                    
                    if (success) {
                        frameCount++;
                        console.log(`[Worker] ✓ Frame ${frameCount} added successfully in ${processingTime.toFixed(2)}ms`);
                        
                        if (frameCount % 10 === 0) { // More frequent logging
                            const avgProcessingTime = frameProcessingTimes.reduce((a, b) => a + b, 0) / frameProcessingTimes.length;
                            const totalDuration = (performance.now() - recordingStartTime) / 1000;
                            const actualFps = frameCount / totalDuration;
                            
                            console.log(`[Worker] Performance metrics:
                                Frame #${frameCount}
                                Queue size: ${frameQueue.length}
                                Avg processing time: ${avgProcessingTime.toFixed(2)}ms
                                Total duration: ${totalDuration.toFixed(2)}s
                                Actual FPS: ${actualFps.toFixed(2)}
                                Target FPS: 30`);
                            
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
        console.log('[Worker] === PROCESS FRAME QUEUE END ===');
        console.log('[Worker] Final queue size:', frameQueue.length);
        console.log('[Worker] Total frames processed:', frameCount);
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
                console.log('[Worker] === START RECORDING REQUEST ===');
                console.log('[Worker] isInitialized:', isInitialized);
                console.log('[Worker] recordingInProgress:', recordingInProgress);
                
                if (!isInitialized) {
                    console.error('[Worker] ERROR: Worker not initialized');
                    throw new Error('Worker not initialized');
                }

                if (recordingInProgress) {
                    console.log('[Worker] Already recording, stopping first');
                    await stopCurrentRecording();
                }

                console.log('[Worker] Starting H.264/MP4 recording with options:', message.options);
                console.log('[Worker] Original options dimensions:', message.options.width, 'x', message.options.height);
                
                recordingInProgress = true;
                frameCount = 0;
                recordingStartTime = performance.now();
                frameProcessingTimes = [];

                // ADAPTIVE DIMENSIONS: Use actual canvas dimensions for recording
                // This ensures compatibility with different image sizes and mobile devices
                console.log('[Worker] Using adaptive canvas dimensions for recording...');
                let recordingWidth = message.options.width;
                let recordingHeight = message.options.height;
                
                // MOBILE COMPATIBILITY: Ensure dimensions are even numbers (required for H.264)
                if (recordingWidth % 2 !== 0) {
                    recordingWidth = recordingWidth - 1;
                    console.log(`[Worker] Adjusted width to even number: ${recordingWidth}`);
                }
                if (recordingHeight % 2 !== 0) {
                    recordingHeight = recordingHeight - 1;
                    console.log(`[Worker] Adjusted height to even number: ${recordingHeight}`);
                }
                
                // MOBILE COMPATIBILITY: Validate dimension limits for mobile devices
                const maxMobileDimension = 1920; // Safe limit for most mobile devices
                const minDimension = 16; // Minimum for H.264
                
                if (recordingWidth > maxMobileDimension || recordingHeight > maxMobileDimension) {
                    console.warn(`[Worker] WARNING: Large dimensions (${recordingWidth}x${recordingHeight}) may cause issues on mobile devices`);
                }
                
                if (recordingWidth < minDimension || recordingHeight < minDimension) {
                    console.error(`[Worker] ERROR: Dimensions too small (${recordingWidth}x${recordingHeight}), minimum is ${minDimension}x${minDimension}`);
                    recordingInProgress = false;
                    self.postMessage({ type: 'recordingStarted', success: false, error: 'Dimensions too small for H.264 encoding' });
                    return;
                }
                
                console.log(`[Worker] Final recording dimensions: ${recordingWidth}x${recordingHeight}`);
                console.log(`[Worker] Original canvas dimensions: ${message.options.width}x${message.options.height}`);
                
                // Get buffer dimensions for comparison only (not for recording)
                const bufferWidth = CppModule.get_buf_width();
                const bufferHeight = CppModule.get_buf_height();
                console.log(`[Worker] Buffer dimensions (for reference): ${bufferWidth}x${bufferHeight}`);

                console.log('[Worker] Calling C++ start_recording with CANVAS dimensions:');
                console.log('[Worker] - Width:', recordingWidth);
                console.log('[Worker] - Height:', recordingHeight);
                console.log('[Worker] - FPS:', message.options.fps);
                console.log('[Worker] - Bitrate:', message.options.bitrate);
                console.log('[Worker] - Codec:', message.options.codec);
                console.log('[Worker] - Format:', message.options.format);
                console.log('[Worker] - Preset:', message.options.preset);

                const success = await CppModule.start_recording(
                    recordingWidth,  // Use canvas width (adaptive)
                    recordingHeight, // Use canvas height (adaptive)
                    message.options.fps,
                    message.options.bitrate,
                    message.options.codec,
                    message.options.format,
                    message.options.preset
                );

                console.log('[Worker] start_recording returned:', success);
                
                if (success) {
                    console.log('[Worker] ✓ Recording started successfully');
                    // Verify recording state
                    if (typeof CppModule.get_recording_state === 'function') {
                        const state = CppModule.get_recording_state();
                        console.log('[Worker] C++ recording state after start:', state);
                    }
                    if (typeof CppModule.is_recording === 'function') {
                        const isRec = CppModule.is_recording();
                        console.log('[Worker] C++ is_recording after start:', isRec);
                    }
                    self.postMessage({ type: 'recordingStarted', success: true });
                } else {
                    console.error('[Worker] ✗ Failed to start H.264 recording');
                    if (typeof CppModule.get_recording_error === 'function') {
                        const error = CppModule.get_recording_error();
                        console.error('[Worker] C++ error:', error);
                    }
                    recordingInProgress = false;
                    self.postMessage({ type: 'recordingStarted', success: false, error: 'Failed to start H.264 recording' });
                }
                break;

            case 'addFrame':
                console.log('[Worker] === ADD FRAME REQUEST ===');
                console.log('[Worker] CppModule available:', !!CppModule);
                console.log('[Worker] worker_add_frame available:', !!(CppModule && CppModule.worker_add_frame));
                console.log('[Worker] recordingInProgress:', recordingInProgress);
                
                if (!CppModule || !CppModule.worker_add_frame || !recordingInProgress) {
                    console.warn('[Worker] Ignoring frame - not ready or not recording');
                    console.warn('[Worker] - CppModule:', !!CppModule);
                    console.warn('[Worker] - worker_add_frame:', !!(CppModule && CppModule.worker_add_frame));
                    console.warn('[Worker] - recordingInProgress:', recordingInProgress);
                    return;
                }
                
                console.log('[Worker] Frame data validation:');
                console.log('[Worker] - hasImageData:', !!message.imageData);
                console.log('[Worker] - width:', message.width);
                console.log('[Worker] - height:', message.height);
                console.log('[Worker] - imageData type:', message.imageData ? message.imageData.constructor.name : 'null');
                console.log('[Worker] - imageData length:', message.imageData ? message.imageData.length : 0);
                console.log('[Worker] - expected length:', message.width * message.height * 4);
                
                if (!message.imageData || !message.width || !message.height) {
                    console.error('[Worker] Invalid frame data:', { 
                        hasImageData: !!message.imageData,
                        width: message.width,
                        height: message.height
                    });
                    return;
                }

                // Check current recording state
                if (typeof CppModule.get_recording_state === 'function') {
                    const currentState = CppModule.get_recording_state();
                    console.log('[Worker] Current C++ recording state:', currentState);
                }
                
                if (typeof CppModule.is_recording === 'function') {
                    const isCppRecording = CppModule.is_recording();
                    console.log('[Worker] C++ is_recording:', isCppRecording);
                }

                // if (shouldDropFrame(frameQueue.length)) {
                //     return; // Drop this frame
                // }
                
                console.log('[Worker] Adding frame to queue. Current queue size:', frameQueue.length);
                frameQueue.push({
                    imageData: message.imageData,
                    width: message.width,
                    height: message.height
                });
                console.log('[Worker] Frame added to queue. New queue size:', frameQueue.length);

                // Process frames if queue is getting large
                if (frameQueue.length >= MAX_QUEUE_SIZE) {
                    console.log('[Worker] Queue size reached MAX_QUEUE_SIZE, processing frames');
                    processFrameQueue();
                }
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

        // Stop recording
        const success = CppModule.stop_recording();

        if (success) {
            console.log('[Worker] Recording stopped successfully, getting data');

            // Get the video data
            const videoDataArray = CppModule.get_recording_data();

            if (videoDataArray && videoDataArray.length > 0) {
                console.log(`[Worker] Got ${videoDataArray.length} bytes of video data`);

                // Debug: Print first 16 bytes in hex
                const headerHex = Array.from(videoDataArray.slice(0, 16))
                    .map(b => b.toString(16).padStart(2, '0'))
                    .join(' ');
                console.log('[Worker] First 16 bytes:', headerHex);

                // Verify video data integrity
                if (videoDataArray.length < 1000) { // Basic size check
                    throw new Error('Video data too small, likely corrupted');
                }

                // Create a proper MP4 container with metadata
                const videoDataBuffer = new ArrayBuffer(videoDataArray.length);
                const videoData = new Uint8Array(videoDataBuffer);
                videoData.set(videoDataArray);

                const frameCount = CppModule.get_recorded_frame_count() || 0;
                console.log(`[Worker] Final frame count: ${frameCount}`);

                // Verify MP4 header (ftyp box)
                const isMP4 = videoData[4] === 0x66 && videoData[5] === 0x74 && videoData[6] === 0x79 && videoData[7] === 0x70;
                if (!isMP4) {
                    console.warn('[Worker] Warning: MP4 header not detected. Expected ftyp box (66 74 79 70)');
                    console.warn('[Worker] Got:', headerHex);
                    // Don't fail - FFmpeg might use different container structure
                }

                // Send video data back to main thread with highest priority
                self.postMessage({
                    type: 'recordingStopped',
                    success: true,
                    videoData: videoData,
                    frameCount: frameCount,
                    mimeType: 'video/mp4; codecs="avc1.42E01E"', // H.264 baseline profile
                    duration: frameCount / 30 // Approximate duration in seconds
                }, [videoDataBuffer]); // Transfer ownership for speed
            } else {
                console.error('[Worker] No video data returned');
                self.postMessage({
                    type: 'recordingStopped',
                    success: false,
                    error: 'No video data available'
                });
            }
        } else {
            const error = getCppError();
            console.error('[Worker] stop_recording failed:', error);
            self.postMessage({
                type: 'recordingStopped',
                success: false,
                error: `Stop recording failed: ${error}`
            });
        }
    } catch (e) {
        console.error('[Worker] Exception during stopRecording:', e);
        self.postMessage({
            type: 'recordingStopped',
            success: false,
            error: 'Exception: ' + (e.message || e.toString())
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

console.log('[Worker] H.264/MP4 video encoding worker initialized');