// videoEncodingWorker.js - Web Worker for Video Encoding

let module; // To hold the initialized C++ module instance
let frameSequence = 0; // Track frame sequence for ordering
let queuedFrames = 0; // Track number of frames in processing queue (JS side)
const MAX_QUEUE_SIZE = 30;
let recordingInProgress = false;

function getCppError() {
    if (module && typeof module.get_recording_error === 'function') {
        try {
            return module.get_recording_error();
        } catch (e) {
            console.error('[Worker] Failed to get C++ error:', e);
            return 'Failed to get C++ error: ' + (e.message || e);
        }
    }
    return 'Module or get_recording_error not available';
}

self.onmessage = async (event) => {
    const message = event.data;

    switch (message.type) {
        case 'init':
            try {
                console.log('[Worker] Initializing with WASM module');
                // The module is passed directly as a factory function
                module = await self.Module();
                
                console.log('[Worker] Module instance created');
                
                // Verify critical recording functions exist
                const requiredFunctions = [
                    'start_recording', 'stop_recording', 'get_recording_data',
                    'get_recorded_frame_count', 'get_recording_state', 'get_recording_error'
                ];
                
                const missingFunctions = requiredFunctions.filter(
                    fnName => !(module && typeof module[fnName] === 'function')
                );

                if (missingFunctions.length === 0) {
                    console.log('[Worker] Critical recording functions verified');
                    self.postMessage({type: 'initialized'});
                } else {
                    console.error(`[Worker] Missing required recording functions: [${missingFunctions.join(', ')}]`);
                    throw new Error(`Module recording functions missing: ${missingFunctions.join(', ')}`);
                }
            } catch (e) {
                console.error('[Worker] Initialization failed:', e);
                self.postMessage({ 
                    type: 'error', 
                    error: 'Worker init failed: ' + (e.message || e.toString()) 
                });
            }
            break;

        case 'startRecording':
            if (!module) {
                self.postMessage({ 
                    type: 'recordingStarted', 
                    success: false, 
                    error: 'Module not initialized' 
                });
                return;
            }

            try {
                const opts = message.options;
                console.log(`[Worker] Starting recording with options:`, opts);
                
                // Reset counters
                frameSequence = 0;
                queuedFrames = 0;
                recordingInProgress = true;
                
                // Start recording
                const success = module.start_recording(
                    opts.width,
                    opts.height,
                    opts.fps,
                    opts.bitrate,
                    opts.codec,
                    opts.format,
                    opts.preset
                );

                if (success) {
                    console.log('[Worker] Recording started successfully');
                    self.postMessage({type: 'recordingStarted', success: true});
                } else {
                    const error = getCppError();
                    console.error('[Worker] start_recording failed:', error);
                    recordingInProgress = false;
                    self.postMessage({ 
                        type: 'recordingStarted', 
                        success: false, 
                        error: `Recording failed: ${error}` 
                    });
                }
            } catch (e) {
                console.error('[Worker] Exception during startRecording:', e);
                recordingInProgress = false;
                self.postMessage({ 
                    type: 'recordingStarted', 
                    success: false, 
                    error: 'Exception: ' + (e.message || e.toString()) 
                });
            }
            break;

        case 'addFrame':
            if (!module || !recordingInProgress) {
                return; // Silently ignore frames if not ready or not recording
            }

            try {
                const {imageData, width, height, sequence} = message;
                
                // Only log occasionally for better performance
                if (sequence % 30 === 0) {
                    console.log(`[Worker] Processing frame #${sequence}: ${width}x${height}`);
                }

                // Safety checks
                if (!imageData || !width || !height) {
                    console.error('[Worker] Invalid frame data received');
                    return;
                }

                queuedFrames++;
                
                // Add frame to encoder
                const success = module.add_frame(imageData);

                queuedFrames--;

                // Log progress periodically
                if (sequence % 30 === 0) {
                    const frameCount = module.get_recorded_frame_count();
                    if (success) {
                        console.log(`[Worker] Added frame #${sequence}, total frames: ${frameCount}`);
                        
                        // Update UI with progress
                        self.postMessage({
                            type: 'recordingProgress',
                            frameCount: frameCount
                        });
                    } else {
                        console.error(`[Worker] Failed to add frame #${sequence}: ${getCppError()}`);
                    }
                }
            } catch (e) {
                console.error('[Worker] Exception processing frame:', e);
            }
            break;

        case 'stopRecording':
            if (!module) {
                self.postMessage({ 
                    type: 'recordingStopped', 
                    success: false, 
                    error: 'Module not initialized' 
                });
                return;
            }

            try {
                console.log('[Worker] Stopping recording and finalizing...');
                recordingInProgress = false;
                
                // Stop recording
                const success = module.stop_recording();

                if (success) {
                    console.log('[Worker] Recording stopped successfully, getting data');
                    
                    // Get the video data
                    const videoDataArray = module.get_recording_data();

                    if (videoDataArray && videoDataArray.length > 0) {
                        console.log(`[Worker] Got ${videoDataArray.length} bytes of video data`);
                        
                        // Transfer data directly without additional processing
                        const videoDataBuffer = new ArrayBuffer(videoDataArray.length);
                        const videoData = new Uint8Array(videoDataBuffer);
                        videoData.set(videoDataArray);

                        const frameCount = module.get_recorded_frame_count() || 0;
                        
                        // Send video data back to main thread with highest priority
                        self.postMessage({
                            type: 'recordingStopped',
                            success: true,
                            videoData: videoData, 
                            frameCount: frameCount
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
            break;

        case 'getState':
            if (!module) {
                self.postMessage({ 
                    type: 'recorderState', 
                    state: 'module_not_ready', 
                    frameCount: 0, 
                    queueSize: queuedFrames 
                });
                return;
            }

            try {
                const state = typeof module.get_recording_state === 'function' ? 
                    module.get_recording_state() : 'unknown';
                const frameCount = typeof module.get_recorded_frame_count === 'function' ? 
                    module.get_recorded_frame_count() : 0;
                
                self.postMessage({
                    type: 'recorderState',
                    state: state,
                    frameCount: frameCount,
                    queueSize: queuedFrames,
                    isWorkerRecording: recordingInProgress
                });
            } catch (e) {
                console.error('[Worker] Exception during getState:', e);
                self.postMessage({ 
                    type: 'recorderState', 
                    state: 'error_getting_state', 
                    frameCount: 0, 
                    queueSize: queuedFrames 
                });
            }
            break;

        default:
            console.warn('[Worker] Unknown message type received:', message.type);
    }
};

// Global error handlers
self.addEventListener('error', (event) => {
    console.error('[Worker Global Error]:', event.message, event.filename, event.lineno);
    self.postMessage({ type: 'error', error: 'Worker error: ' + event.message });
});

self.addEventListener('unhandledrejection', (event) => {
    console.error('[Worker Global Error] Unhandled Promise Rejection:', event.reason);
    self.postMessage({ 
        type: 'error', 
        error: 'Worker promise rejection: ' + (event.reason.message || event.reason.toString()) 
    });
});

console.log('[Worker] Video encoding worker initialized');