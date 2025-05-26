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

// Mobile logging helper for worker
const mobileLog = (message, data = null) => {
    const timestamp = new Date().toISOString().slice(11, 23);
    const logMessage = `[${timestamp}] [Worker] ${message}`;
    console.log(logMessage, data || '');
};

// Initialize WASM module
async function initWasm(wasmUrl) {
    mobileLog('Initializing with WASM URL:', wasmUrl);
    if (!wasmUrl) {
        throw new Error('WASM URL not provided');
    }

    try {
        mobileLog('Attempting to import module factory...');
        const moduleFactory = await import(wasmUrl);
        mobileLog('Module factory loaded successfully');

        // Configure module without threading and with imported memory
        const moduleConfig = {
            print: (text) => mobileLog('[WASM Print]', text),
            printErr: (text) => mobileLog('[WASM Error]', text),
            locateFile: (path) => {
                mobileLog('Locating file:', path);
                return wasmUrl.replace('lux.js', path);
            },
            // Use imported memory configuration
            importMemory: true
        };

        mobileLog('Creating module instance with config...');
        CppModule = await moduleFactory.default(moduleConfig);
        mobileLog('Module instance created successfully');
    } catch (error) {
        mobileLog('CRITICAL ERROR - Failed to initialize module:', error.message);
        mobileLog('Error stack:', error.stack);
        throw new Error(`Module initialization failed: ${error.message}`);
    }

    const exportedFunctions = Object.keys(CppModule).filter(key => typeof CppModule[key] === 'function');
    mobileLog('Module exports count:', exportedFunctions.length);
    mobileLog('First 10 exports:', exportedFunctions.slice(0, 10));

    // Verify critical recording functions exist
    const requiredFunctions = [
        'start_recording', 'stop_recording', 'get_recording_data',
        'get_recorded_frame_count', 'get_recording_state', 'get_recording_error',
        'is_recording', 'worker_add_frame'
    ];
    const missingFunctions = requiredFunctions.filter(fnName => !(CppModule && typeof CppModule[fnName] === 'function'));

    if (missingFunctions.length === 0) {
        mobileLog('✓ All critical recording functions verified');
    } else {
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
        // Process frames while we have them and recording is still active (or we're in final processing)
        while (frameQueue.length > 0) {
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
    mobileLog('Received message:', message.type);

    try {
        switch (message.type) {
            case 'init':
                if (isInitialized) {
                    mobileLog('Already initialized');
                    return;
                }

                mobileLog('Initializing worker...');
                await initWasm(message.wasmUrl);
                isInitialized = true;
                mobileLog('✓ Worker initialization complete');
                self.postMessage({ type: 'initialized' });
                break;

            case 'startRecording':
                mobileLog('=== START RECORDING REQUEST ===');
                mobileLog('isInitialized:', isInitialized);
                mobileLog('recordingInProgress:', recordingInProgress);
                
                if (!isInitialized) {
                    mobileLog('ERROR: Worker not initialized');
                    throw new Error('Worker not initialized');
                }

                if (recordingInProgress) {
                    mobileLog('Already recording, stopping first');
                    await stopCurrentRecording();
                }

                // CLEAN STATE: Reset all recording-related variables for fresh start
                mobileLog('Resetting recording state for fresh start...');
                frameCount = 0;
                frameQueue = []; // Clear any leftover frames from previous recording
                isProcessingFrames = false;
                recordingStartTime = performance.now();
                lastFrameTime = null;
                frameProcessingTimes = [];
                
                mobileLog('Recording state reset complete:', {
                    frameCount: frameCount,
                    queueLength: frameQueue.length,
                    isProcessingFrames: isProcessingFrames
                });

                mobileLog('Starting H.264/MP4 recording with options:', message.options);
                mobileLog('Original options dimensions:', message.options.width, 'x', message.options.height);
                
                recordingInProgress = true;

                // ADAPTIVE DIMENSIONS: Use actual canvas dimensions for recording
                // This ensures compatibility with different image sizes and mobile devices
                mobileLog('Using adaptive canvas dimensions for recording...');
                let recordingWidth = message.options.width;
                let recordingHeight = message.options.height;
                
                // MOBILE COMPATIBILITY: Ensure dimensions are even numbers (required for H.264)
                if (recordingWidth % 2 !== 0) {
                    recordingWidth = recordingWidth - 1;
                    mobileLog(`Adjusted width to even number: ${recordingWidth}`);
                }
                if (recordingHeight % 2 !== 0) {
                    recordingHeight = recordingHeight - 1;
                    mobileLog(`Adjusted height to even number: ${recordingHeight}`);
                }
                
                // MOBILE COMPATIBILITY: Validate dimension limits for mobile devices
                const maxMobileDimension = 1920; // Safe limit for most mobile devices
                const minDimension = 16; // Minimum for H.264
                
                if (recordingWidth > maxMobileDimension || recordingHeight > maxMobileDimension) {
                    mobileLog(`WARNING: Large dimensions (${recordingWidth}x${recordingHeight}) may cause issues on mobile devices`);
                }
                
                if (recordingWidth < minDimension || recordingHeight < minDimension) {
                    mobileLog(`ERROR: Dimensions too small (${recordingWidth}x${recordingHeight}), minimum is ${minDimension}x${minDimension}`);
                    recordingInProgress = false;
                    self.postMessage({ type: 'recordingStarted', success: false, error: 'Dimensions too small for H.264 encoding' });
                    return;
                }
                
                mobileLog(`Final recording dimensions: ${recordingWidth}x${recordingHeight}`);
                mobileLog(`Original canvas dimensions: ${message.options.width}x${message.options.height}`);
                
                // Get buffer dimensions for comparison only (not for recording)
                const bufferWidth = CppModule.get_buf_width();
                const bufferHeight = CppModule.get_buf_height();
                mobileLog(`Buffer dimensions (for reference): ${bufferWidth}x${bufferHeight}`);

                mobileLog('Calling C++ start_recording with CANVAS dimensions:');
                mobileLog('- Width:', recordingWidth);
                mobileLog('- Height:', recordingHeight);
                mobileLog('- FPS:', message.options.fps);
                mobileLog('- Bitrate:', message.options.bitrate);
                mobileLog('- Codec:', message.options.codec);
                mobileLog('- Format:', message.options.format);
                mobileLog('- Preset:', message.options.preset);

                const success = await CppModule.start_recording(
                    recordingWidth,  // Use canvas width (adaptive)
                    recordingHeight, // Use canvas height (adaptive)
                    message.options.fps,
                    message.options.bitrate,
                    message.options.codec,
                    message.options.format,
                    message.options.preset
                );

                mobileLog('start_recording returned:', success);
                
                if (success) {
                    mobileLog('✓ Recording started successfully');
                    // Verify recording state
                    if (typeof CppModule.get_recording_state === 'function') {
                        const state = CppModule.get_recording_state();
                        mobileLog('C++ recording state after start:', state);
                    }
                    if (typeof CppModule.is_recording === 'function') {
                        const isRec = CppModule.is_recording();
                        mobileLog('C++ is_recording after start:', isRec);
                    }
                    
                    // Process any frames that were queued while waiting for backend to be ready
                    if (frameQueue.length > 0) {
                        mobileLog(`Processing ${frameQueue.length} frames that were queued while waiting for backend`);
                        processFrameQueue();
                    }
                    
                    self.postMessage({ type: 'recordingStarted', success: true });
                } else {
                    mobileLog('✗ Failed to start H.264 recording');
                    if (typeof CppModule.get_recording_error === 'function') {
                        const error = CppModule.get_recording_error();
                        mobileLog('C++ error:', error);
                    }
                    recordingInProgress = false;
                    
                    // Clear any queued frames since recording failed
                    if (frameQueue.length > 0) {
                        mobileLog(`Clearing ${frameQueue.length} queued frames due to recording failure`);
                        frameQueue = [];
                    }
                    
                    self.postMessage({ type: 'recordingStarted', success: false, error: 'Failed to start H.264 recording' });
                }
                break;

            case 'addFrame':
                mobileLog('=== ADD FRAME REQUEST ===');
                mobileLog('CppModule available:', !!CppModule);
                mobileLog('worker_add_frame available:', !!(CppModule && CppModule.worker_add_frame));
                mobileLog('recordingInProgress:', recordingInProgress);
                
                // SAFETY CHECK: Don't accept frames if recording is not active
                if (!recordingInProgress) {
                    mobileLog('Rejecting frame - recording not in progress');
                    return;
                }
                
                if (!CppModule || !CppModule.worker_add_frame) {
                    mobileLog('Module not ready, queuing frame for when backend is ready');
                    mobileLog('- CppModule:', !!CppModule);
                    mobileLog('- worker_add_frame:', !!(CppModule && CppModule.worker_add_frame));
                    
                    // Queue the frame even if backend isn't ready yet
                    if (message.imageData && message.width && message.height) {
                        frameQueue.push({
                            imageData: message.imageData,
                            width: message.width,
                            height: message.height
                        });
                        mobileLog('Frame queued while waiting for backend. Queue size:', frameQueue.length);
                    }
                    return;
                }
                
                mobileLog('Frame data validation:');
                mobileLog('- hasImageData:', !!message.imageData);
                mobileLog('- width:', message.width);
                mobileLog('- height:', message.height);
                mobileLog('- imageData type:', message.imageData ? message.imageData.constructor.name : 'null');
                mobileLog('- imageData length:', message.imageData ? message.imageData.length : 0);
                mobileLog('- expected length:', message.width * message.height * 4);
                
                if (!message.imageData || !message.width || !message.height) {
                    mobileLog('Invalid frame data:', { 
                        hasImageData: !!message.imageData,
                        width: message.width,
                        height: message.height
                    });
                    return;
                }

                // Check current recording state
                if (typeof CppModule.get_recording_state === 'function') {
                    const currentState = CppModule.get_recording_state();
                    mobileLog('Current C++ recording state:', currentState);
                }
                
                if (typeof CppModule.is_recording === 'function') {
                    const isCppRecording = CppModule.is_recording();
                    mobileLog('C++ is_recording:', isCppRecording);
                    
                    // If C++ says it's not recording yet, queue the frame
                    if (!isCppRecording) {
                        mobileLog('C++ not ready yet, queuing frame');
                        frameQueue.push({
                            imageData: message.imageData,
                            width: message.width,
                            height: message.height
                        });
                        mobileLog('Frame queued waiting for C++ ready. Queue size:', frameQueue.length);
                        return;
                    }
                }

                mobileLog('Adding frame to queue. Current queue size:', frameQueue.length);
                frameQueue.push({
                    imageData: message.imageData,
                    width: message.width,
                    height: message.height
                });
                mobileLog('Frame added to queue. New queue size:', frameQueue.length);

                // Process frames if queue is getting large
                if (frameQueue.length >= MAX_QUEUE_SIZE) {
                    mobileLog('Queue size reached MAX_QUEUE_SIZE, processing frames');
                    processFrameQueue();
                }
                break;

            case 'stopRecording':
                if (!recordingInProgress) {
                    mobileLog('Not recording');
                    return;
                }

                const stopTime = performance.now();
                const totalRecordingDuration = (stopTime - recordingStartTime) / 1000;
                mobileLog(`Recording stopped after ${totalRecordingDuration.toFixed(2)}s with ${frameCount} frames`);
                mobileLog(`Average FPS: ${(frameCount / totalRecordingDuration).toFixed(2)}`);

                // Immediately stop accepting new frames
                recordingInProgress = false;
                
                // Handle queue flushing based on message flag
                if (message.flushQueue) {
                    mobileLog(`Flushing ${frameQueue.length} remaining frames from queue (will not be processed)`);
                    frameQueue = []; // Clear the queue - these frames won't be processed
                    mobileLog('Frame queue flushed - starting clean for next recording');
                } else {
                    // Process any remaining frames that were already queued
                    if (frameQueue.length > 0) {
                        mobileLog(`Processing ${frameQueue.length} remaining frames before stopping`);
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
                    mobileLog('Exception during getState:', e.message);
                    self.postMessage({ type: 'recorderState', state: 'error_getting_state', frameCount: 0, queueSize: 0 });
                }
                break;

            default:
                mobileLog('Unknown message type:', message.type);
        }
    } catch (error) {
        mobileLog('CRITICAL ERROR in message handler:', error.message);
        mobileLog('Error stack:', error.stack);
        self.postMessage({ type: 'error', error: error.message });
    }
};

// Helper function to stop recording
async function stopCurrentRecording() {
    try {
        mobileLog('Stopping recording and finalizing...');
        
        // Ensure recording is stopped
        recordingInProgress = false;
        
        // Ensure frame processing is stopped
        isProcessingFrames = false;

        // Process any remaining frames in queue (only if not already flushed)
        if (frameQueue.length > 0) {
            mobileLog(`Processing ${frameQueue.length} remaining frames before stopping`);
            await processFrameQueue();
        }

        // Stop recording in C++
        const success = CppModule.stop_recording();

        if (success) {
            mobileLog('Recording stopped successfully, getting data');

            // Get the video data
            const videoDataArray = CppModule.get_recording_data();

            if (videoDataArray && videoDataArray.length > 0) {
                mobileLog(`Got ${videoDataArray.length} bytes of video data`);

                // Debug: Print first 16 bytes in hex
                const headerHex = Array.from(videoDataArray.slice(0, 16))
                    .map(b => b.toString(16).padStart(2, '0'))
                    .join(' ');
                mobileLog('First 16 bytes:', headerHex);

                // Verify video data integrity
                if (videoDataArray.length < 1000) { // Basic size check
                    throw new Error('Video data too small, likely corrupted');
                }

                // Create a proper MP4 container with metadata
                const videoDataBuffer = new ArrayBuffer(videoDataArray.length);
                const videoData = new Uint8Array(videoDataBuffer);
                videoData.set(videoDataArray);

                const finalFrameCount = CppModule.get_recorded_frame_count() || 0;
                mobileLog(`Final frame count: ${finalFrameCount}`);

                // Verify MP4 header (ftyp box)
                const isMP4 = videoData[4] === 0x66 && videoData[5] === 0x74 && videoData[6] === 0x79 && videoData[7] === 0x70;
                if (!isMP4) {
                    mobileLog('Warning: MP4 header not detected. Expected ftyp box (66 74 79 70)');
                    mobileLog('Got:', headerHex);
                    // Don't fail - FFmpeg might use different container structure
                }

                // CLEAN STATE: Reset all variables for next recording
                mobileLog('Cleaning up state for next recording...');
                frameQueue = []; // Ensure queue is empty
                isProcessingFrames = false;
                frameCount = 0; // Reset frame count for next recording
                recordingStartTime = null;
                lastFrameTime = null;
                frameProcessingTimes = [];
                
                mobileLog('State cleanup complete:', {
                    frameQueue: frameQueue.length,
                    isProcessingFrames: isProcessingFrames,
                    frameCount: frameCount
                });

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
                mobileLog('No video data returned');
                self.postMessage({
                    type: 'recordingStopped',
                    success: false,
                    error: 'No video data available'
                });
            }
        } else {
            const error = getCppError();
            mobileLog('stop_recording failed:', error);
            self.postMessage({
                type: 'recordingStopped',
                success: false,
                error: `Stop recording failed: ${error}`
            });
        }
    } catch (e) {
        mobileLog('Exception during stopRecording:', e);
        self.postMessage({
            type: 'recordingStopped',
            success: false,
            error: 'Exception: ' + (e.message || e.toString())
        });
    } finally {
        // FINAL CLEANUP: Ensure state is completely reset regardless of success/failure
        mobileLog('Final cleanup - ensuring clean state...');
        recordingInProgress = false;
        isProcessingFrames = false;
        frameQueue = [];
        frameCount = 0;
        recordingStartTime = null;
        lastFrameTime = null;
        frameProcessingTimes = [];
        
        mobileLog('Final cleanup complete - ready for next recording');
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

mobileLog('H.264/MP4 video encoding worker initialized');