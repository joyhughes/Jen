import React, { useEffect, useRef, useState, useCallback } from 'react';
import {
  Alert, 
  Box, 
  CircularProgress,
  Divider, 
  IconButton, 
  Paper, 
  Snackbar, 
  Tooltip, 
  useMediaQuery, 
  useTheme
} from '@mui/material';
import { 
  Camera, 
  Pause, 
  Play, 
  RotateCcw, 
  Save, 
  SkipForward, 
  Video, 
  VideoOff 
} from 'lucide-react';

function MediaController({ isOverlay = false }) {
  const theme = useTheme();
  const isMobile = useMediaQuery(theme.breakpoints.down('sm'));
  const isTablet = useMediaQuery(theme.breakpoints.down('md'));
  const [isRunning, setIsRunning] = useState(true);
  
  // Video recording states
  const [isRecording, setIsRecording] = useState(false);
  const [isProcessing, setIsProcessing] = useState(false);
  const [frameCount, setFrameCount] = useState(0);
  const [recordingStartTime, setRecordingStartTime] = useState(null);
  const [elapsedTime, setElapsedTime] = useState(0);
  
  // Refs
  const workerRef = useRef(null);
  const captureIntervalRef = useRef(null);
  const statusIntervalRef = useRef(null);
  const isInitializedRef = useRef(false);
  const isRecordingRef = useRef(false);
  const recordingOptionsRef = useRef(null);

  const [notification, setNotification] = useState({ open: false, message: '', severity: 'success' });
  const [performanceMetrics, setPerformanceMetrics] = useState({
    avgProcessingTime: 0,
    actualFps: 0,
    queueSize: 0
  });

  // Mobile logging helper
  const mobileLog = (message, data = null) => {
    const timestamp = new Date().toISOString().slice(11, 23);
    const logMessage = `[${timestamp}] [Mobile] ${message}`;
    console.log(logMessage, data || '');
    
    // Also show critical errors as notifications on mobile
    if (message.includes('ERROR') || message.includes('CRITICAL')) {
      showNotification(`Debug: ${message}`, 'error');
    }
  };

  // Enhanced mobile device detection with logging
  const isMobileDevice = () => {
    const userAgent = navigator.userAgent;
    const hasTouch = navigator.maxTouchPoints && navigator.maxTouchPoints > 2;
    const isMobileUA = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(userAgent);
    
    mobileLog('Device detection:', {
      userAgent: userAgent,
      maxTouchPoints: navigator.maxTouchPoints,
      hasTouch: hasTouch,
      isMobileUA: isMobileUA,
      result: isMobileUA || hasTouch
    });
    
    return isMobileUA || hasTouch;
  };

  // Safe feature detection for camera roll saving
  const supportsCameraRollSave = () => {
    try {
      mobileLog('Checking camera roll save support...');
      
      // Check for Web Share API (iOS Safari, Android Chrome)
      const hasWebShare = typeof navigator.share === 'function' && typeof navigator.canShare === 'function';
      mobileLog('Web Share API available:', hasWebShare);
      
      // Check for File System Access API (Android Chrome)
      const hasFileSystem = typeof window.showSaveFilePicker === 'function';
      mobileLog('File System Access API available:', hasFileSystem);
      
      // Check for iOS
      const isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent);
      mobileLog('iOS detected:', isIOS);
      
      // Check for Android
      const isAndroid = /Android/.test(navigator.userAgent);
      mobileLog('Android detected:', isAndroid);
      
      if (hasWebShare) {
        mobileLog('Using Web Share API');
        return 'webshare';
      }
      
      if (hasFileSystem) {
        mobileLog('Using File System Access API');
        return 'filesystem';
      }
      
      if (isIOS) {
        mobileLog('Using iOS fallback');
        return 'ios-fallback';
      }
      
      if (isAndroid) {
        mobileLog('Using Android fallback');
        return 'android-fallback';
      }
      
      mobileLog('No camera roll support detected');
      return null;
    } catch (error) {
      mobileLog('ERROR in supportsCameraRollSave:', error.message);
      return null;
    }
  };

  // Update ref when state changes
  useEffect(() => {
    isRecordingRef.current = isRecording;
  }, [isRecording]);

  // Initialize worker on component mount
  useEffect(() => {
    mobileLog('MediaController initializing...');
    mobileLog('Component mount - device info:', {
      isMobile: isMobileDevice(),
      userAgent: navigator.userAgent,
      viewport: `${window.innerWidth}x${window.innerHeight}`,
      devicePixelRatio: window.devicePixelRatio
    });

    try {
      // Create worker with enhanced error handling
      mobileLog('Creating video encoding worker...');
      const worker = new Worker(new URL('/workers/videoEncodingWorker.js', import.meta.url), { type: 'module' });
      
      // Listen to messages from worker
      worker.onmessage = handleWorkerMessage;
      
      // Add error handler
      worker.onerror = (error) => {
        mobileLog('ERROR - Worker error:', error.message);
        showNotification('Recording system error: ' + error.message, 'error');
      };

      // Add message error handler
      worker.onmessageerror = (error) => {
        mobileLog('ERROR - Worker message error:', error.message);
        showNotification('Recording system message error', 'error');
      };
      
      // Save worker reference
      workerRef.current = worker;
      mobileLog('Worker created successfully');
      
      // Initialize worker
      initializeWorker();
      
      isInitializedRef.current = true;
      mobileLog('MediaController initialization complete');
      
    } catch (error) {
      mobileLog('CRITICAL ERROR - Failed to create worker:', error.message);
      showNotification('Failed to initialize recording system: ' + error.message, 'error');
    }
    
    // Clean up on unmount
    return () => {
      mobileLog('MediaController cleanup...');
      
      if (isRecording) {
        stopRecording();
      }
      
      if (statusIntervalRef.current) {
        clearInterval(statusIntervalRef.current);
      }
      
      if (captureIntervalRef.current) {
        cancelAnimationFrame(captureIntervalRef.current);
      }
      
      if (workerRef.current) {
        workerRef.current.terminate();
      }
      
      mobileLog('MediaController cleanup complete');
    };
  }, []); // Empty dependency array since we only want to initialize once

  // Update elapsed time during recording
  useEffect(() => {
    let timeInterval;
    if (isRecording && recordingStartTime) {
      timeInterval = setInterval(() => {
        setElapsedTime(Math.floor((Date.now() - recordingStartTime) / 1000));
      }, 1000);
    }
    
    return () => {
      if (timeInterval) {
        clearInterval(timeInterval);
      }
    };
  }, [isRecording, recordingStartTime]);

  // Initialize the web worker
  const initializeWorker = () => {
    try {
      if (!workerRef.current) {
        mobileLog('ERROR - Worker not available for initialization');
        return;
      }
      
      mobileLog('Sending init message to worker...');
      
      // Send init message to worker with correct WASM URL
      workerRef.current.postMessage({
        type: 'init',
        wasmUrl: '/src/lux.js'  // This matches where the Makefile outputs the file
      });
      
      mobileLog('Init message sent to worker');
      
      // Check for mobile camera roll support and notify user
      if (isMobileDevice()) {
        mobileLog('Mobile device detected, checking camera roll support...');
        const saveType = supportsCameraRollSave();
        mobileLog('Camera roll save type:', saveType);
        
        if (saveType) {
          setTimeout(() => {
            switch (saveType) {
              case 'webshare':
                showNotification('📱 Mobile detected: Videos will save to camera roll via share', 'info');
                break;
              case 'filesystem':
                showNotification('📱 Mobile detected: Videos will save to device storage', 'info');
                break;
              case 'ios-fallback':
                showNotification('📱 iOS detected: Long-press videos to save to Photos', 'info');
                break;
              case 'android-fallback':
                showNotification('📱 Android detected: Videos will download to Gallery', 'info');
                break;
            }
          }, 2000); // Delay to avoid overwhelming user with notifications
        }
      }
    } catch (error) {
      mobileLog('CRITICAL ERROR - Failed to initialize worker:', error.message);
      showNotification('Failed to initialize recording: ' + error.message, 'error');
    }
  };

  // Handle worker messages
  const handleWorkerMessage = (event) => {
    mobileLog('=== WORKER MESSAGE RECEIVED ===');
    mobileLog('Raw event received:', !!event);
    
    if (!event || !event.data) {
      mobileLog('ERROR: Invalid worker message received');
      mobileLog('- Event:', !!event);
      mobileLog('- Event.data:', !!event?.data);
      return;
    }

    const message = event.data;
    mobileLog('Message type:', message.type);
    mobileLog('Full message keys:', Object.keys(message));
    
    try {
      if (!message || typeof message !== 'object') {
        throw new Error('Invalid message format received from worker');
      }

      switch (message.type) {
        case 'initialized':
          mobileLog('✓ Recording system initialized successfully');
          break;
          
        case 'recordingStarted':
          mobileLog('=== RECORDING STARTED RESPONSE ===');
          mobileLog('Success:', message.success);
          mobileLog('Error (if any):', message.error);
          
          if (message.success) {
            mobileLog('✓ Recording started successfully');
            mobileLog('Setting recording state...');
            mobileLog('- Before: isRecordingRef.current =', isRecordingRef.current);
            
            isRecordingRef.current = true; // Set ref first
            setIsRecording(true);
            setRecordingStartTime(Date.now());
            setFrameCount(0); // Reset frame count
            
            mobileLog('- After: isRecordingRef.current =', isRecordingRef.current);
            mobileLog('- Recording start time set:', new Date().toISOString());
            
            // Start frame capture immediately after recording starts
            mobileLog('Starting frame capture...');
            captureFrame();
            
            mobileLog('Starting status polling...');
            startPollingStatus();
          } else {
            mobileLog('✗ Recording failed to start');
            mobileLog('- Error:', message.error || 'Unknown error');
            
            isRecordingRef.current = false;
            setIsRecording(false);
            showNotification(`Failed to start recording: ${message.error || 'Unknown error'}`, 'error');
          }
          break;
          
        case 'recordingStopped':
          mobileLog('=== RECORDING STOPPED RESPONSE ===');
          mobileLog('Success:', message.success);
          mobileLog('Frame count:', message.frameCount);
          mobileLog('Video data size:', message.videoData ? message.videoData.length : 0);
          mobileLog('MIME type:', message.mimeType);
          
          isRecordingRef.current = false;
          setIsRecording(false);
          setIsProcessing(false);
          
          if (message.success) {
            mobileLog('✓ Recording completed successfully');
            
            // Create and download the video file with proper MIME type
            mobileLog('Creating video blob...');
            const blob = new Blob([message.videoData], { 
              type: message.mimeType || 'video/mp4; codecs="avc1.42E01E"'
            });
            
            mobileLog('Blob created:', {
              size: blob.size,
              type: blob.type
            });
            
            // Verify blob size
            if (blob.size < 1000) {
              mobileLog('ERROR: Video data too small - blob size:', blob.size);
              showNotification('Recording failed: Video data too small', 'error');
              return;
            }
            
            mobileLog(`✓ Video blob created successfully: ${blob.size} bytes, ${message.frameCount} frames`);
            
            // Generate filename with timestamp
            const filename = `jen-recording-${new Date().toISOString().slice(0,19).replace(/:/g,'-')}.mp4`;
            
            // Try mobile camera roll save first, fallback to standard download
            mobileLog('Attempting mobile camera roll save...');
            saveToMobileCameraRoll(blob, filename).then((success) => {
              if (!success) {
                mobileLog('Mobile camera roll save failed, used standard download');
                // This is already handled in the saveToMobileCameraRoll function
              }
            }).catch((error) => {
              mobileLog('Error in camera roll save:', error.message);
              // Fallback is already handled in the function
            });

            // Log final metrics
            if (message.metrics) {
              mobileLog(`Final recording metrics:
                Total duration: ${message.metrics.totalDuration.toFixed(2)}s
                Total frames: ${message.metrics.totalFrames}
                Average FPS: ${message.metrics.averageFps.toFixed(2)}`);
            }
          } else {
            showNotification(`Recording failed: ${message.error || 'Unknown error'}`, 'error');
          }
          break;
          
        case 'recordingProgress':
          mobileLog('=== RECORDING PROGRESS UPDATE ===');
          mobileLog('Frame count:', message.frameCount);
          
          if (typeof message.frameCount === 'number') {
            setFrameCount(message.frameCount);
            mobileLog('Frame count updated to:', message.frameCount);
            
            if (message.metrics) {
              setPerformanceMetrics(message.metrics);
              mobileLog('Performance metrics updated:', {
                avgProcessingTime: message.metrics.avgProcessingTime?.toFixed(2),
                actualFps: message.metrics.actualFps?.toFixed(2),
                queueSize: message.metrics.queueSize
              });
            }
          }
          break;

        case 'recorderState':
          mobileLog('=== RECORDER STATE UPDATE ===');
          mobileLog('State:', message.state);
          mobileLog('Frame count:', message.frameCount);
          mobileLog('Is C++ recording:', message.isCppRecording);
          mobileLog('Is worker recording:', message.isWorkerRecording);
          
          if (typeof message.frameCount === 'number') {
            setFrameCount(message.frameCount);
            mobileLog('Frame count updated from state to:', message.frameCount);
          }
          break;
          
        case 'error':
          mobileLog('=== WORKER ERROR ===');
          mobileLog('Error message:', message.error);
          mobileLog('Current recording state:', isRecording);
          mobileLog('Current processing state:', isProcessing);
          
          showNotification(`Error: ${message.error || 'Unknown error'}`, 'error');
          
          if (isRecording) {
            mobileLog('Stopping recording due to error...');
            stopRecording();
          }
          break;

        default:
          mobileLog('=== UNKNOWN MESSAGE TYPE ===');
          mobileLog('Unknown message type:', message.type);
          mobileLog('Full message:', message);
      }
    } catch (error) {
      mobileLog('CRITICAL ERROR handling worker message:', error.message);
      showNotification('Error processing recording message: ' + error.message, 'error');
      
      // Reset recording state on error
      if (isRecording) {
        isRecordingRef.current = false;
        setIsRecording(false);
        setIsProcessing(false);
      }
    }
  };

  // Handle media controls
  const handleRestart = () => {
    if (window.module) {
      window.module.restart();
      showNotification('Scene restarted', 'info');
    }
  };

  const handleAdvance = () => {
    if (window.module) {
      setIsRunning(false);
      window.module.advance_frame();
    }
  };

  const handleRunPause = () => {
    if (window.module) {
      const newState = !isRunning;
      setIsRunning(newState);
      window.module.run_pause();
    }
  };

  const handleSnapshot = async () => {
    try {
      const canvas = document.querySelector('canvas');
      if (!canvas) {
        showNotification("Canvas not found", "error");
        return;
      }
      const filename = `jen-snapshot-${new Date().toISOString().slice(0, 19).replace(/:/g, '-')}.png`;

      const dataUrl = canvas.toDataURL('image/png');
      const link = document.createElement('a');
      link.download = filename;
      link.href = dataUrl;
      link.click();
      
      showNotification("Snapshot saved", "success");
    } catch (error) {
      console.error('Error taking snapshot:', error);
      showNotification('Failed to save snapshot', 'error');
    }
  };

  // Toggle recording
  const handleToggleRecording = () => {
    console.log('[MediaController] Toggle recording clicked, current state:', { isRecording, isProcessing });
    
    if (isProcessing) {
      console.log('[MediaController] Currently processing, ignoring click');
      return; // Prevent action while processing
    }
    
    if (isRecording) {
      console.log('[MediaController] Stopping recording...');
      stopRecording();
    } else {
      console.log('[MediaController] Starting recording...');
      startRecording();
    }
  };

  // Capture frames from canvas and send to worker
  const captureFrame = useCallback(() => {
    mobileLog('=== CAPTURE FRAME INIT ===');
    mobileLog('isRecordingRef.current:', isRecordingRef.current);
    
    if (!isRecordingRef.current) {
      mobileLog('Not recording, skipping frame capture');
      return;
    }

    let frameNumber = 0;
    const captureStartTime = performance.now();

    const captureLoop = () => {
        // SAFETY CHECK: Always check recording state at the start of each loop
        if (!isRecordingRef.current) {
          mobileLog('Recording stopped, ending capture loop');
          return;
        }

        frameNumber++;
        const loopStartTime = performance.now();
        mobileLog(`=== CAPTURING FRAME ${frameNumber} ===`);

        try {
            // Get the canvas element
            mobileLog('Looking for canvas element...');
            const canvas = document.querySelector('canvas');
            if (!canvas) {
                mobileLog('ERROR: Canvas not found for frame capture');
                setTimeout(captureLoop, 100);
                return;
            }
            
            mobileLog('Canvas found:', {
              width: canvas.width,
              height: canvas.height,
              styleWidth: canvas.style.width,
              styleHeight: canvas.style.height
            });

            // Get image data directly from the canvas
            mobileLog('Getting canvas context...');
            const ctx = canvas.getContext('2d');
            if (!ctx) {
                mobileLog('ERROR: Canvas context not available');
                setTimeout(captureLoop, 100);
                return;
            }
            mobileLog('Canvas context obtained successfully');

            // Capture the current frame from the canvas
            mobileLog('Capturing image data...');
            const captureDataStart = performance.now();
            const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
            const captureDataTime = performance.now() - captureDataStart;
            
            mobileLog('Image data captured:', {
              dataLength: imageData.data.length,
              expectedLength: canvas.width * canvas.height * 4,
              dataType: imageData.data.constructor.name,
              captureTime: captureDataTime.toFixed(2) + 'ms'
            });
            
            // Validate image data
            if (imageData.data.length === 0) {
                mobileLog('ERROR: Empty image data captured');
                setTimeout(captureLoop, 100);
                return;
            }
            
            if (imageData.data.length !== canvas.width * canvas.height * 4) {
                mobileLog('ERROR: Image data length mismatch', {
                  expected: canvas.width * canvas.height * 4,
                  actual: imageData.data.length
                });
                setTimeout(captureLoop, 100);
                return;
            }
            
            // Check for non-zero data (detect blank frames)
            let hasNonZeroData = false;
            for (let i = 0; i < Math.min(1000, imageData.data.length); i += 4) {
                if (imageData.data[i] !== 0 || imageData.data[i+1] !== 0 || imageData.data[i+2] !== 0) {
                    hasNonZeroData = true;
                    break;
                }
            }
            
            mobileLog('Frame content validation:', {
              hasNonZeroPixels: hasNonZeroData,
              firstPixelRGBA: [
                imageData.data[0], imageData.data[1], imageData.data[2], imageData.data[3]
              ]
            });
            
            if (!hasNonZeroData) {
                mobileLog('WARNING: Frame appears to be blank (all black pixels)');
            }
            
            // FINAL SAFETY CHECK: Verify recording is still active before sending
            if (!isRecordingRef.current) {
                mobileLog('Recording stopped during frame capture, discarding frame');
                return;
            }
            
            // Send to worker
            mobileLog('Sending frame to worker...');
            mobileLog('Worker available:', !!workerRef.current);
            mobileLog('Frame dimensions:', canvas.width, 'x', canvas.height);
            
            if (workerRef.current && imageData.data.length > 0) {
                const sendStartTime = performance.now();
                
                workerRef.current.postMessage({
                    type: 'addFrame',
                    imageData: imageData.data,
                    width: canvas.width,
                    height: canvas.height
                }, [imageData.data.buffer]);
                
                const sendTime = performance.now() - sendStartTime;
                const totalFrameTime = performance.now() - loopStartTime;
                
                mobileLog('Frame sent to worker successfully', {
                  sendTime: sendTime.toFixed(2) + 'ms',
                  totalFrameTime: totalFrameTime.toFixed(2) + 'ms',
                  framesCaptured: frameNumber
                });
                
                // Calculate FPS
                const elapsedSeconds = (performance.now() - captureStartTime) / 1000;
                const currentFps = frameNumber / elapsedSeconds;
                mobileLog('Current capture FPS:', currentFps.toFixed(2));
                
            } else {
                mobileLog('ERROR: Cannot send frame to worker', {
                  workerAvailable: !!workerRef.current,
                  imageDataLength: imageData.data.length
                });
            }
            
            // Schedule next frame at 30fps (33.33ms interval) - but only if still recording
            if (isRecordingRef.current) {
                mobileLog('Scheduling next frame in 33ms...');
                setTimeout(captureLoop, 33);
            } else {
                mobileLog('Recording stopped, not scheduling next frame');
            }
        } catch (error) {
            mobileLog('EXCEPTION in frame capture:', error.message);
            mobileLog('Error stack:', error.stack);
            
            // Only continue if still recording
            if (isRecordingRef.current) {
                setTimeout(captureLoop, 100);
            }
        }
    };

    // Start the capture loop
    mobileLog('Starting capture loop...');
    captureLoop();
  }, []);

  // Start recording
  const startRecording = () => {
    console.log('[MediaController] === START RECORDING PROCESS ===');
    console.log('[MediaController] Worker state check:');
    console.log('[MediaController] - workerRef.current:', !!workerRef.current);
    console.log('[MediaController] - isInitializedRef.current:', isInitializedRef.current);
    
    if (!workerRef.current || !isInitializedRef.current) {
      console.error('[MediaController] ERROR: Worker not ready');
      console.error('[MediaController] - Worker available:', !!workerRef.current);
      console.error('[MediaController] - Worker initialized:', isInitializedRef.current);
      showNotification('Recording system not ready', 'error');
      return;
    }
    
    console.log('[MediaController] Looking for canvas element...');
    const canvas = document.querySelector('canvas');
    if (!canvas) {
      console.error('[MediaController] ERROR: Canvas not found');
      showNotification('Canvas not found', 'error');
      return;
    }

    console.log('[MediaController] Canvas found for recording:');
    console.log('[MediaController] - Original canvas width:', canvas.width);
    console.log('[MediaController] - Original canvas height:', canvas.height);
    console.log('[MediaController] - Canvas client width:', canvas.clientWidth);
    console.log('[MediaController] - Canvas client height:', canvas.clientHeight);
    console.log('[MediaController] - Canvas offset width:', canvas.offsetWidth);
    console.log('[MediaController] - Canvas offset height:', canvas.offsetHeight);
    
    const width = canvas.width % 2 === 0 ? canvas.width : canvas.width - 1;
    const height = canvas.height % 2 === 0 ? canvas.height : canvas.height - 1;
    
    console.log('[MediaController] Adjusted dimensions for encoding:');
    console.log('[MediaController] - Adjusted width:', width);
    console.log('[MediaController] - Adjusted height:', height);
    console.log('[MediaController] - Width adjustment needed:', canvas.width !== width);
    console.log('[MediaController] - Height adjustment needed:', canvas.height !== height);
    
    const options = {
      width,
      height,
      fps: 30,
      bitrate: 2500000,
      codec: 'libx264',
      format: 'mp4',
      preset: 'ultrafast'
    };
    
    console.log('[MediaController] Recording options prepared:');
    console.log('[MediaController] - Width:', options.width);
    console.log('[MediaController] - Height:', options.height);
    console.log('[MediaController] - FPS:', options.fps);
    console.log('[MediaController] - Bitrate:', options.bitrate);
    console.log('[MediaController] - Codec:', options.codec);
    console.log('[MediaController] - Format:', options.format);
    console.log('[MediaController] - Preset:', options.preset);
    
    recordingOptionsRef.current = options;
    
    try {
      console.log('[MediaController] Sending startRecording message to worker...');
      workerRef.current.postMessage({
        type: 'startRecording',
        options
      });
      
      console.log('[MediaController] Start recording message sent successfully');
      showNotification('Starting recording...', 'info');
    } catch (error) {
      console.error('[MediaController] EXCEPTION sending start recording message:', error);
      console.error('[MediaController] - Error message:', error.message);
      console.error('[MediaController] - Error stack:', error.stack);
      showNotification('Failed to start recording: ' + error.message, 'error');
    }
  };

  // Stop recording
  const stopRecording = () => {
    mobileLog('Stopping recording process...');
    
    if (!workerRef.current || !isRecordingRef.current) {
        mobileLog('Cannot stop - worker not ready or not recording');
        return;
    }
    
    // Immediately stop recording state to prevent new frames from being captured
    isRecordingRef.current = false;
    setIsRecording(false);
    
    // Stop capturing frames immediately
    if (captureIntervalRef.current) {
        mobileLog('Cancelling frame capture');
        cancelAnimationFrame(captureIntervalRef.current);
        captureIntervalRef.current = null;
    }
    
    // Stop status checking
    if (statusIntervalRef.current) {
        mobileLog('Clearing status interval');
        clearInterval(statusIntervalRef.current);
        statusIntervalRef.current = null;
    }

    mobileLog('Frame capture stopped');
    
    setIsProcessing(true);
    showNotification('Processing video...', 'info');
    
    mobileLog('Sending stop recording message to worker');
    
    // Tell worker to stop recording and process remaining frames
    workerRef.current.postMessage({ 
        type: 'stopRecording',
        flushQueue: true // Add flag to flush any remaining frames
    });
  };

  // Poll status from worker
  const startPollingStatus = () => {
    statusIntervalRef.current = setInterval(() => {
      if (workerRef.current && isRecording) {
        workerRef.current.postMessage({ type: 'getState' });
      }
    }, 1000); // Check once per second
  };

  // Format time (MM:SS)
  const formatTime = (seconds) => {
    const mins = Math.floor(seconds / 60).toString().padStart(2, '0');
    const secs = (seconds % 60).toString().padStart(2, '0');
    return `${mins}:${secs}`;
  };

  // Show notification
  const showNotification = (message, severity = 'success') => {
    setNotification({
      open: true,
      message,
      severity
    });
  };

  // Close notification
  const handleCloseNotification = () => {
    setNotification(prev => ({ ...prev, open: false }));
  };

  // Styling
  const containerStyles = isOverlay ? {
    position: 'relative',
    borderRadius: 28,
    backgroundColor: 'rgba(0, 0, 0, 0.7)',
    backdropFilter: 'blur(8px)',
    padding: '8px 12px',
    boxShadow: theme.shadows[8],
    opacity: 1,
    transition: 'opacity 0.3s ease',
    width: 'auto',
    maxWidth: '100%'
  } : {
    display: 'flex',
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    p: 0.5,
    borderRadius: 2,
    bgcolor: theme.palette.background.paper,
    border: `1px solid ${theme.palette.divider}`,
    width: '100%',
    maxWidth: '100%',
    overflowX: 'auto',
    boxShadow: theme.shadows[3]
  };

  // Button sizing
  const buttonSize = isMobile ? 36 : (isOverlay ? 40 : 44);
  const iconSize = buttonSize * 0.5;
  const buttonMargin = isMobile ? 0.25 : 0.5;

  // Button styling
  const buttonStyles = {
    width: buttonSize,
    height: buttonSize,
    m: buttonMargin,
    color: isOverlay ? 'white' : theme.palette.text.secondary,
    '&:hover': {
      bgcolor: isOverlay ? 'rgba(255, 255, 255, 0.1)' : theme.palette.action.hover,
    },
  };

  // Active button styling
  const activeButtonStyles = {
    ...buttonStyles,
    color: theme.palette.primary.main,
    '&:hover': {
      bgcolor: theme.palette.primary.main + '1A', // 10% opacity
    },
  };

  // Recording button style
  const getRecordingButtonStyles = () => {
    if (!isRecording) return buttonStyles;

    // For recording state
    if (isProcessing) {
      // Yellow for processing
      return {
        ...buttonStyles,
        color: theme.palette.warning.main,
        '&:hover': {
          bgcolor: theme.palette.warning.main + '1A',
        }
      };
    } else {
      // Red for active recording
      return {
        ...buttonStyles,
        color: theme.palette.error.main,
        '&:hover': {
          bgcolor: theme.palette.error.main + '1A',
        }
      };
    }
  };

  // Recording tooltip
  const getRecordingTooltip = () => {
    if (!isRecording) {
      const mobile = isMobileDevice();
      const saveType = supportsCameraRollSave();
      
      if (mobile && saveType) {
        switch (saveType) {
          case 'webshare':
            return "Start Recording (saves to camera roll via share)";
          case 'filesystem':
            return "Start Recording (saves to device storage)";
          case 'ios-fallback':
            return "Start Recording (long-press video to save to Photos)";
          case 'android-fallback':
            return "Start Recording (saves to Downloads/Gallery)";
          default:
            return "Start Recording";
        }
      }
      return "Start Recording";
    }
    
    if (isProcessing) return "Processing... Please wait";
    return `Stop Recording (${formatTime(elapsedTime)}, ${frameCount} frames)`;
  };

  // Save video to camera roll (mobile-optimized)
  const saveToMobileCameraRoll = async (blob, filename) => {
    mobileLog('=== SAVING TO MOBILE CAMERA ROLL ===');
    mobileLog('Device detection:', {
      isMobile: isMobileDevice(),
      userAgent: navigator.userAgent,
      supportType: supportsCameraRollSave()
    });
    
    const supportType = supportsCameraRollSave();
    
    try {
      switch (supportType) {
        case 'webshare':
          mobileLog('Using Web Share API for camera roll save...');
          
          try {
            // Create a File object for sharing
            const file = new File([blob], filename, { type: blob.type });
            mobileLog('File created for sharing:', {
              name: file.name,
              size: file.size,
              type: file.type
            });
            
            // Check if we can share this file type
            if (navigator.canShare && navigator.canShare({ files: [file] })) {
              mobileLog('File can be shared, attempting share...');
              await navigator.share({
                title: 'Jen Recording',
                text: 'Video recorded with Jen',
                files: [file]
              });
              
              showNotification('Video shared to camera roll!', 'success');
              mobileLog('✓ Video shared successfully via Web Share API');
              return true;
            } else {
              mobileLog('Web Share API cannot share this file type, falling back...');
              throw new Error('Cannot share this file type');
            }
          } catch (shareError) {
            mobileLog('ERROR in Web Share API:', shareError.message);
            throw shareError;
          }
          
        case 'filesystem':
          mobileLog('Using File System Access API for camera roll save...');
          
          try {
            // Use File System Access API (Android Chrome)
            const fileHandle = await window.showSaveFilePicker({
              suggestedName: filename,
              types: [{
                description: 'MP4 Videos',
                accept: {
                  'video/mp4': ['.mp4']
                }
              }]
            });
            
            const writable = await fileHandle.createWritable();
            await writable.write(blob);
            await writable.close();
            
            showNotification('Video saved to device storage!', 'success');
            mobileLog('✓ Video saved successfully via File System Access API');
            return true;
          } catch (fsError) {
            mobileLog('ERROR in File System Access API:', fsError.message);
            throw fsError;
          }
          
        case 'ios-fallback':
          mobileLog('Using iOS fallback method...');
          
          try {
            // iOS Safari fallback - create a video element and prompt user
            const videoUrl = URL.createObjectURL(blob);
            mobileLog('Video URL created for iOS fallback');
            
            const videoElement = document.createElement('video');
            videoElement.src = videoUrl;
            videoElement.controls = true;
            videoElement.style.width = '100%';
            videoElement.style.maxWidth = '400px';
            videoElement.style.height = 'auto';
            
            // Create a modal-like overlay with safer styling
            const overlay = document.createElement('div');
            overlay.style.position = 'fixed';
            overlay.style.top = '0';
            overlay.style.left = '0';
            overlay.style.width = '100%';
            overlay.style.height = '100%';
            overlay.style.background = 'rgba(0, 0, 0, 0.9)';
            overlay.style.display = 'flex';
            overlay.style.flexDirection = 'column';
            overlay.style.justifyContent = 'center';
            overlay.style.alignItems = 'center';
            overlay.style.zIndex = '10000';
            overlay.style.padding = '20px';
            overlay.style.boxSizing = 'border-box';
            
            const instructions = document.createElement('div');
            instructions.style.color = 'white';
            instructions.style.textAlign = 'center';
            instructions.style.marginBottom = '20px';
            instructions.style.fontFamily = '-apple-system, BlinkMacSystemFont, sans-serif';
            instructions.style.fontSize = '16px';
            instructions.style.lineHeight = '1.4';
            
            instructions.innerHTML = `
              <h3 style="margin: 0 0 10px 0;">Save to Camera Roll</h3>
              <p style="margin: 0 0 10px 0;">1. Tap and hold the video below</p>
              <p style="margin: 0 0 10px 0;">2. Select "Save to Photos" from the menu</p>
              <p style="margin: 0;">Tap outside to close</p>
            `;
            
            overlay.appendChild(instructions);
            overlay.appendChild(videoElement);
            
            // Close overlay when clicking outside video
            overlay.addEventListener('click', (e) => {
              if (e.target === overlay) {
                try {
                  document.body.removeChild(overlay);
                  URL.revokeObjectURL(videoUrl);
                  mobileLog('iOS overlay closed and cleaned up');
                } catch (cleanupError) {
                  mobileLog('ERROR cleaning up iOS overlay:', cleanupError.message);
                }
              }
            });
            
            document.body.appendChild(overlay);
            mobileLog('iOS overlay created and displayed');
            
            showNotification('Long-press video to save to Photos', 'info');
            mobileLog('✓ iOS fallback method activated');
            return true;
          } catch (iosError) {
            mobileLog('ERROR in iOS fallback:', iosError.message);
            throw iosError;
          }
          
        case 'android-fallback':
          mobileLog('Using Android fallback method...');
          
          try {
            // Android fallback - trigger download and show instructions
            const url = URL.createObjectURL(blob);
            const link = document.createElement('a');
            link.href = url;
            link.download = filename;
            link.style.display = 'none';
            
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
            
            mobileLog('Android download link clicked');
            
            // Show instructions for moving to camera roll
            setTimeout(() => {
              showNotification('Video downloaded! Check Downloads folder or move to Gallery', 'info');
            }, 1000);
            
            setTimeout(() => {
              URL.revokeObjectURL(url);
              mobileLog('Android download URL cleaned up');
            }, 5000);
            
            mobileLog('✓ Android fallback download triggered');
            return true;
          } catch (androidError) {
            mobileLog('ERROR in Android fallback:', androidError.message);
            throw androidError;
          }
          
        default:
          mobileLog('No mobile camera roll support detected, using standard download...');
          throw new Error('No mobile camera roll support');
      }
      
    } catch (error) {
      mobileLog('ERROR saving to camera roll:', error.message);
      
      // Ultimate fallback - standard download with enhanced error handling
      try {
        mobileLog('Using ultimate fallback - standard download...');
        const url = URL.createObjectURL(blob);
        const link = document.createElement('a');
        link.href = url;
        link.download = filename;
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        
        // Clean up URL after a delay
        setTimeout(() => {
          URL.revokeObjectURL(url);
          mobileLog('Standard download URL cleaned up');
        }, 5000);
        
        if (isMobileDevice()) {
          showNotification('Video downloaded! Check your Downloads folder', 'info');
        } else {
          showNotification('Video downloaded!', 'success');
        }
        
        mobileLog('✓ Standard download completed');
        return false;
      } catch (fallbackError) {
        mobileLog('CRITICAL ERROR - Even standard download failed:', fallbackError.message);
        showNotification('Failed to save video: ' + fallbackError.message, 'error');
        return false;
      }
    }
  };

  return (
    <Paper elevation={isOverlay ? 0 : 3} sx={containerStyles}>
      <Box sx={{
        display: 'flex',
        flexWrap: 'nowrap',
        alignItems: 'center',
        justifyContent: 'center',
        width: '100%'
      }}>
        {/* Primary Controls */}
        <Tooltip title="Restart" arrow>
          <IconButton
            onClick={handleRestart}
            sx={buttonStyles}
            size="medium"
          >
            <RotateCcw size={iconSize} />
          </IconButton>
        </Tooltip>

        <Tooltip title="Advance Frame" arrow>
          <IconButton
            onClick={handleAdvance}
            sx={buttonStyles}
            size="medium"
          >
            <SkipForward size={iconSize} />
          </IconButton>
        </Tooltip>

        <Tooltip title={isRunning ? "Pause" : "Play"} arrow>
          <IconButton
            onClick={handleRunPause}
            sx={isRunning ? activeButtonStyles : buttonStyles}
            size="medium"
          >
            {isRunning ? <Pause size={iconSize} /> : <Play size={iconSize} />}
          </IconButton>
        </Tooltip>

        <Divider orientation="vertical" flexItem sx={{
          mx: isMobile ? 0.25 : 0.5,
          my: 0.5,
          height: isMobile ? '70%' : '80%'
        }} />

        {/* Secondary Controls */}
        <Tooltip title="Take Snapshot" arrow>
          <IconButton
            onClick={handleSnapshot}
            sx={buttonStyles}
            size="medium"
          >
            <Camera size={iconSize} />
          </IconButton>
        </Tooltip>

        <Tooltip title={getRecordingTooltip()} arrow>
          <span> {/* Wrapper to allow tooltip on disabled button */}
            <IconButton
              onClick={handleToggleRecording}
              sx={{
                ...getRecordingButtonStyles(),
                position: 'relative' // For positioning the mobile indicator
              }}
              size="medium"
              disabled={isProcessing}
            >
              {isProcessing ? (
                <CircularProgress size={iconSize} />
              ) : isRecording ? (
                <VideoOff size={iconSize} />
              ) : (
                <Video size={iconSize} />
              )}
              
              {/* Mobile camera roll indicator */}
              {!isRecording && !isProcessing && isMobileDevice() && supportsCameraRollSave() && (
                <Box
                  sx={{
                    position: 'absolute',
                    top: 2,
                    right: 2,
                    width: 8,
                    height: 8,
                    borderRadius: '50%',
                    bgcolor: theme.palette.success.main,
                    border: `1px solid ${isOverlay ? 'white' : theme.palette.background.paper}`,
                    zIndex: 1
                  }}
                />
              )}
            </IconButton>
          </span>
        </Tooltip>

        <Tooltip title="Save Scene" arrow>
          <IconButton
            onClick={() => {/* Save scene logic */}}
            sx={buttonStyles}
            size="medium"
          >
            <Save size={iconSize} />
          </IconButton>
        </Tooltip>
      </Box>

      {/* Display recording info if recording and not mobile */}
      {isRecording && !isMobile && !isProcessing && (
        <Box sx={{ 
          position: 'absolute', 
          bottom: '-20px', 
          left: '50%', 
          transform: 'translateX(-50%)',
          bgcolor: theme.palette.error.dark,
          color: 'white',
          px: 1,
          py: 0.2,
          borderRadius: 1,
          fontSize: '0.7rem',
          fontWeight: 'bold',
          display: 'flex',
          flexDirection: 'column',
          alignItems: 'center'
        }}>
          <Box>REC {formatTime(elapsedTime)} • {frameCount} frames</Box>
          <Box sx={{ fontSize: '0.6rem', opacity: 0.8 }}>
            FPS: {performanceMetrics.actualFps.toFixed(1)} • 
            Queue: {performanceMetrics.queueSize} • 
            Process: {performanceMetrics.avgProcessingTime.toFixed(0)}ms
          </Box>
        </Box>
      )}

      <Snackbar
        open={notification.open}
        autoHideDuration={3000}
        onClose={handleCloseNotification}
        anchorOrigin={{ vertical: 'bottom', horizontal: 'center' }}
      >
        <Alert
          onClose={handleCloseNotification}
          severity={notification.severity}
          variant="filled"
          sx={{ width: '100%' }}
        >
          {notification.message}
        </Alert>
      </Snackbar>
    </Paper>
  );
}

export default MediaController;