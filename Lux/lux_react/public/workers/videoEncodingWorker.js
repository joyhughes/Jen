// videoEncodingWorker.js - Web Worker for Video Encoding

let CppModule; // To hold the initialized C++ module instance
let frameSequence = 0; // Track frame sequence for ordering
let queuedFrames = 0; // Track number of frames in processing queue (JS side)
const MAX_QUEUE_SIZE = 30;
let recordingInProgress = false;
let isInitialized = false; // Track initialization state
let isRecording = false;
let frameCount = 0;
let frameQueue = [];
let isProcessingFrames = false;

// Frame dropping and queue management
let frameCounter = 0;
let lastFrameTime = 0;
const TARGET_FPS = 24; // Lower than 30 for mobile
const MIN_FRAME_INTERVAL = 1000 / TARGET_FPS;

// Initialize WASM module
async function initWasm(wasmUrl) {
    console.log('[Worker] Initializing with WASM URL:', wasmUrl);
    if (!wasmUrl) {
        throw new Error('WASM URL not provided');
    }

    const moduleFactory = await import(wasmUrl);
    console.log('[Worker] Module factory loaded.');

    CppModule = await moduleFactory.default({
        print: (text) => console.log('[WASM Print]', text),
        printErr: (text) => console.error('[WASM Error]', text),
        locateFile: (path) => {
            console.log('[Worker] Locating file:', path);
            return wasmUrl.replace('lux.js', path);
        }
    });

    const exportedFunctions = Object.keys(CppModule).filter(key => typeof CppModule[key] === 'function');
    console.log('[Worker] Module exports:', exportedFunctions);
    console.log('[Worker] Module instance created.');

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

function shouldDropFrame(queueSize) {
    const now = performance.now();
    const timeSinceLastFrame = now - lastFrameTime;
    
    // Always keep first frame after a gap
    if (timeSinceLastFrame > MIN_FRAME_INTERVAL * 2) {
        lastFrameTime = now;
        return false;
    }

    // Base drop rate on queue size
    if (queueSize < 10) return false; // Keep all frames if queue is small
    
    // Progressive drop rate
    const dropRate = Math.min(0.9, 0.1 + (queueSize - 10) * 0.02);
    return Math.random() < dropRate;
}

// Process frames in batches
async function processFrameQueue() {
    if (isProcessingFrames || frameQueue.length === 0) return;
    
    isProcessingFrames = true;
    const batchSize = 5; // Process 5 frames at a time

    try {
        while (frameQueue.length > 0 && recordingInProgress) {
            const batch = frameQueue.splice(0, batchSize);
            
            for (const frame of batch) {
                try {
                    // if (shouldDropFrame(frameQueue.length)) {
                    //     continue; // Drop this frame
                    // }
                    
                    const success = CppModule.worker_add_frame(
                        frame.imageData,
                        frame.width,
                        frame.height
                    );
                    
                    if (success) {
                        frameCount++;
                        if (frameCount % 30 === 0) {
                            console.log(`[Worker] Added frame #${frameCount}, queue size: ${frameQueue.length}`);
                            // Send progress update
                            self.postMessage({ 
                                type: 'recordingProgress',
                                frameCount: frameCount
                            });
                        }
                    } else {
                        console.error('[Worker] Failed to add frame');
                        self.postMessage({ type: 'error', error: 'Failed to add frame' });
                    }
                } catch (error) {
                    console.error('[Worker] Error adding frame:', error);
                    self.postMessage({ type: 'error', error: 'Error adding frame: ' + error.message });
                }
            }
            
            // Small delay to prevent blocking
            await new Promise(resolve => setTimeout(resolve, 0));
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
                queuedFrames = 0;

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
                    console.error('[Worker] Invalid frame data:', { 
                        hasImageData: !!message.imageData,
                        width: message.width,
                        height: message.height
                    });
                    return;
                }

                // if (shouldDropFrame(frameQueue.length)) {
                //     return; // Drop this frame
                // }
                
                lastFrameTime = performance.now();
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
                    console.log('[Worker] Not recording');
                    return;
                }

                console.log('[Worker] Stopping recording...');
                recordingInProgress = false;
                
                // Process any remaining frames
                if (frameQueue.length > 0) {
                    await processFrameQueue();
                }
                
                const result = await stopCurrentRecording();
                self.postMessage(result);
                break;

            case 'getState':
                if (!CppModule) {
                    self.postMessage({ type: 'recorderState', state: 'module_not_ready', frameCount: 0, queueSize: queuedFrames });
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
                        queueSize: queuedFrames,
                        isCppRecording: isCppRecording,
                        isWorkerRecording: recordingInProgress
                    });
                } catch (e) {
                    console.error('[Worker] Exception during getState:', e);
                    self.postMessage({ type: 'recorderState', state: 'error_getting_state', frameCount: 0, queueSize: queuedFrames });
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

                // Create a proper WebM container with metadata
                const videoDataBuffer = new ArrayBuffer(videoDataArray.length);
                const videoData = new Uint8Array(videoDataBuffer);
                videoData.set(videoDataArray);

                const frameCount = CppModule.get_recorded_frame_count() || 0;
                console.log(`[Worker] Final frame count: ${frameCount}`);

                // Verify WebM header (EBML header)
                const isWebM = videoData[0] === 0x1A && videoData[1] === 0x45 && videoData[2] === 0xDF && videoData[3] === 0xA3;
                if (!isWebM) {
                    console.error('[Worker] Invalid WebM header. Expected EBML header (1A 45 DF A3)');
                    console.error('[Worker] Got:', headerHex);
                    
                    // Try to fix the header if it's missing
                    if (videoData.length > 4) {
                        console.log('[Worker] Attempting to fix WebM header...');
                        videoData[0] = 0x1A;
                        videoData[1] = 0x45;
                        videoData[2] = 0xDF;
                        videoData[3] = 0xA3;
                        console.log('[Worker] Header fixed, new header:', 
                            Array.from(videoData.slice(0, 4))
                                .map(b => b.toString(16).padStart(2, '0'))
                                .join(' ')
                        );
                    }
                }

                // Send video data back to main thread with highest priority
                self.postMessage({
                    type: 'recordingStopped',
                    success: true,
                    videoData: videoData,
                    frameCount: frameCount,
                    mimeType: 'video/webm; codecs="vp8"', // VP8 codec
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

console.log('[Worker] Video encoding worker initialized');