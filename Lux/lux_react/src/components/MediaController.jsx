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

  // Update ref when state changes
  useEffect(() => {
    isRecordingRef.current = isRecording;
  }, [isRecording]);

  // Initialize worker on component mount
  useEffect(() => {
    console.log('[MediaController] Initializing worker...');

    // Create worker
    const worker = new Worker(new URL('/workers/videoEncodingWorker.js', import.meta.url), { type: 'module' });
    
    // Listen to messages from worker
    worker.onmessage = handleWorkerMessage;
    
    // Add error handler
    worker.onerror = (error) => {
      console.error('[MediaController] Worker error:', error);
      showNotification('Recording system error: ' + error.message, 'error');
    };

    // Add message error handler
    worker.onmessageerror = (error) => {
      console.error('[MediaController] Worker message error:', error);
      showNotification('Recording system message error', 'error');
    };
    
    // Save worker reference
    workerRef.current = worker;
    
    // Initialize worker
    initializeWorker();
    
    isInitializedRef.current = true;
    
    // Clean up on unmount
    return () => {
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
    if (!workerRef.current) {
      console.error('[MediaController] Worker not available for initialization');
      return;
    }
    
    console.log('[MediaController] Sending init message to worker...');
    
    // Send init message to worker with correct WASM URL
    workerRef.current.postMessage({
      type: 'init',
      wasmUrl: '/src/lux.js'  // This matches where the Makefile outputs the file
    });
  };

  // Handle worker messages
  const handleWorkerMessage = (event) => {
    console.log('[MediaController] === WORKER MESSAGE RECEIVED ===');
    console.log('[MediaController] Raw event:', event);
    
    if (!event || !event.data) {
      console.error('[MediaController] ERROR: Invalid worker message received');
      console.error('[MediaController] - Event:', event);
      console.error('[MediaController] - Event.data:', event?.data);
      return;
    }

    const message = event.data;
    console.log('[MediaController] Message type:', message.type);
    console.log('[MediaController] Full message:', message);
    
    try {
      if (!message || typeof message !== 'object') {
        throw new Error('Invalid message format received from worker');
      }

      switch (message.type) {
        case 'initialized':
          console.log('[MediaController] ✓ Recording system initialized successfully');
          break;
          
        case 'recordingStarted':
          console.log('[MediaController] === RECORDING STARTED RESPONSE ===');
          console.log('[MediaController] Success:', message.success);
          console.log('[MediaController] Error (if any):', message.error);
          
          if (message.success) {
            console.log('[MediaController] ✓ Recording started successfully');
            console.log('[MediaController] Setting recording state...');
            console.log('[MediaController] - Before: isRecordingRef.current =', isRecordingRef.current);
            
            isRecordingRef.current = true; // Set ref first
            setIsRecording(true);
            setRecordingStartTime(Date.now());
            setFrameCount(0); // Reset frame count
            
            console.log('[MediaController] - After: isRecordingRef.current =', isRecordingRef.current);
            console.log('[MediaController] - Recording start time set:', new Date().toISOString());
            
            // Start frame capture immediately after recording starts
            console.log('[MediaController] Starting frame capture...');
            captureFrame();
            
            console.log('[MediaController] Starting status polling...');
            startPollingStatus();
          } else {
            console.error('[MediaController] ✗ Recording failed to start');
            console.error('[MediaController] - Error:', message.error || 'Unknown error');
            
            isRecordingRef.current = false;
            setIsRecording(false);
            showNotification(`Failed to start recording: ${message.error || 'Unknown error'}`, 'error');
          }
          break;
          
        case 'recordingStopped':
          console.log('[MediaController] === RECORDING STOPPED RESPONSE ===');
          console.log('[MediaController] Success:', message.success);
          console.log('[MediaController] Frame count:', message.frameCount);
          console.log('[MediaController] Metrics:', message.metrics);
          console.log('[MediaController] Video data size:', message.videoData ? message.videoData.length : 0);
          console.log('[MediaController] MIME type:', message.mimeType);
          
          isRecordingRef.current = false;
          setIsRecording(false);
          setIsProcessing(false);
          
          if (message.success) {
            console.log('[MediaController] ✓ Recording completed successfully');
            
            // Create and download the video file with proper MIME type
            console.log('[MediaController] Creating video blob...');
            const blob = new Blob([message.videoData], { 
              type: message.mimeType || 'video/mp4; codecs="avc1.42E01E"'
            });
            
            console.log('[MediaController] Blob created:');
            console.log('[MediaController] - Blob size:', blob.size, 'bytes');
            console.log('[MediaController] - Blob type:', blob.type);
            
            // Verify blob size
            if (blob.size < 1000) {
              console.error('[MediaController] ERROR: Video data too small');
              console.error('[MediaController] - Blob size:', blob.size);
              showNotification('Recording failed: Video data too small', 'error');
              return;
            }
            
            console.log(`[MediaController] ✓ Video blob created successfully: ${blob.size} bytes, ${message.frameCount} frames`);
            
            // Create download link
            console.log('[MediaController] Creating download link...');
            const url = URL.createObjectURL(blob);
            const link = document.createElement('a');
            link.href = url;
            link.download = `jen-recording-${new Date().toISOString().slice(0,19).replace(/:/g,'-')}.mp4`;
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
            URL.revokeObjectURL(url);
            
            showNotification(`Recording saved: ${message.frameCount || 0} frames`, 'success');

            // Log final metrics
            if (message.metrics) {
              console.log(`[MediaController] Final recording metrics:
                Total duration: ${message.metrics.totalDuration.toFixed(2)}s
                Total frames: ${message.metrics.totalFrames}
                Average FPS: ${message.metrics.averageFps.toFixed(2)}`);
            }
          } else {
            showNotification(`Recording failed: ${message.error || 'Unknown error'}`, 'error');
          }
          break;
          
        case 'recordingProgress':
          console.log('[MediaController] === RECORDING PROGRESS UPDATE ===');
          console.log('[MediaController] Frame count:', message.frameCount);
          console.log('[MediaController] Metrics:', message.metrics);
          
          if (typeof message.frameCount === 'number') {
            setFrameCount(message.frameCount);
            console.log('[MediaController] Frame count updated to:', message.frameCount);
            
            if (message.metrics) {
              setPerformanceMetrics(message.metrics);
              console.log('[MediaController] Performance metrics updated:');
              console.log('[MediaController] - Avg processing time:', message.metrics.avgProcessingTime?.toFixed(2), 'ms');
              console.log('[MediaController] - Actual FPS:', message.metrics.actualFps?.toFixed(2));
              console.log('[MediaController] - Queue size:', message.metrics.queueSize);
            }
          }
          break;

        case 'recorderState':
          console.log('[MediaController] === RECORDER STATE UPDATE ===');
          console.log('[MediaController] State:', message.state);
          console.log('[MediaController] Frame count:', message.frameCount);
          console.log('[MediaController] Queue size:', message.queueSize);
          console.log('[MediaController] Is C++ recording:', message.isCppRecording);
          console.log('[MediaController] Is worker recording:', message.isWorkerRecording);
          
          if (typeof message.frameCount === 'number') {
            setFrameCount(message.frameCount);
            console.log('[MediaController] Frame count updated from state to:', message.frameCount);
          }
          break;
          
        case 'error':
          console.error('[MediaController] === WORKER ERROR ===');
          console.error('[MediaController] Error message:', message.error);
          console.error('[MediaController] Current recording state:', isRecording);
          console.error('[MediaController] Current processing state:', isProcessing);
          
          showNotification(`Error: ${message.error || 'Unknown error'}`, 'error');
          
          if (isRecording) {
            console.log('[MediaController] Stopping recording due to error...');
            stopRecording();
          }
          break;

        default:
          console.warn('[MediaController] === UNKNOWN MESSAGE TYPE ===');
          console.warn('[MediaController] Unknown message type:', message.type);
          console.warn('[MediaController] Full message:', message);
      }
    } catch (error) {
      console.error('[MediaController] Error handling worker message:', error);
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
    console.log('[MediaController] === CAPTURE FRAME INIT ===');
    console.log('[MediaController] isRecordingRef.current:', isRecordingRef.current);
    
    if (!isRecordingRef.current) {
      console.log('[MediaController] Not recording, skipping frame capture');
      return;
    }

    let frameNumber = 0;
    const captureStartTime = performance.now();

    const captureLoop = () => {
        if (!isRecordingRef.current) {
          console.log('[MediaController] Recording stopped, ending capture loop');
          return;
        }

        frameNumber++;
        const loopStartTime = performance.now();
        console.log(`[MediaController] === CAPTURING FRAME ${frameNumber} ===`);

        try {
            // Get the canvas element
            console.log('[MediaController] Looking for canvas element...');
            const canvas = document.querySelector('canvas');
            if (!canvas) {
                console.error('[MediaController] ERROR: Canvas not found for frame capture');
                setTimeout(captureLoop, 100);
                return;
            }
            
            console.log('[MediaController] Canvas found:');
            console.log('[MediaController] - Canvas width:', canvas.width);
            console.log('[MediaController] - Canvas height:', canvas.height);
            console.log('[MediaController] - Canvas style width:', canvas.style.width);
            console.log('[MediaController] - Canvas style height:', canvas.style.height);

            // Get image data directly from the canvas
            console.log('[MediaController] Getting canvas context...');
            const ctx = canvas.getContext('2d');
            if (!ctx) {
                console.error('[MediaController] ERROR: Canvas context not available');
                setTimeout(captureLoop, 100);
                return;
            }
            console.log('[MediaController] Canvas context obtained successfully');

            // Capture the current frame from the canvas
            console.log('[MediaController] Capturing image data...');
            const captureDataStart = performance.now();
            const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
            const captureDataTime = performance.now() - captureDataStart;
            
            console.log('[MediaController] Image data captured:');
            console.log('[MediaController] - Data length:', imageData.data.length);
            console.log('[MediaController] - Expected length:', canvas.width * canvas.height * 4);
            console.log('[MediaController] - Data type:', imageData.data.constructor.name);
            console.log('[MediaController] - Capture time:', captureDataTime.toFixed(2), 'ms');
            
            // Validate image data
            if (imageData.data.length === 0) {
                console.error('[MediaController] ERROR: Empty image data captured');
                setTimeout(captureLoop, 100);
                return;
            }
            
            if (imageData.data.length !== canvas.width * canvas.height * 4) {
                console.error('[MediaController] ERROR: Image data length mismatch');
                console.error('[MediaController] - Expected:', canvas.width * canvas.height * 4);
                console.error('[MediaController] - Actual:', imageData.data.length);
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
            
            console.log('[MediaController] Frame content validation:');
            console.log('[MediaController] - Has non-zero pixels:', hasNonZeroData);
            console.log('[MediaController] - First 4 pixels RGBA:', [
                imageData.data[0], imageData.data[1], imageData.data[2], imageData.data[3],
                imageData.data[4], imageData.data[5], imageData.data[6], imageData.data[7],
                imageData.data[8], imageData.data[9], imageData.data[10], imageData.data[11],
                imageData.data[12], imageData.data[13], imageData.data[14], imageData.data[15]
            ]);
            
            if (!hasNonZeroData) {
                console.warn('[MediaController] WARNING: Frame appears to be blank (all black pixels)');
            }
            
            // Send to worker
            console.log('[MediaController] Sending frame to worker...');
            console.log('[MediaController] - Worker available:', !!workerRef.current);
            console.log('[MediaController] - Frame dimensions:', canvas.width, 'x', canvas.height);
            
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
                
                console.log('[MediaController] Frame sent to worker successfully');
                console.log('[MediaController] - Send time:', sendTime.toFixed(2), 'ms');
                console.log('[MediaController] - Total frame time:', totalFrameTime.toFixed(2), 'ms');
                console.log('[MediaController] - Frames captured so far:', frameNumber);
                
                // Calculate FPS
                const elapsedSeconds = (performance.now() - captureStartTime) / 1000;
                const currentFps = frameNumber / elapsedSeconds;
                console.log('[MediaController] - Current capture FPS:', currentFps.toFixed(2));
                
            } else {
                console.error('[MediaController] ERROR: Cannot send frame to worker');
                console.error('[MediaController] - Worker available:', !!workerRef.current);
                console.error('[MediaController] - Image data length:', imageData.data.length);
            }
            
            // Schedule next frame at 30fps (33.33ms interval)
            console.log('[MediaController] Scheduling next frame in 33ms...');
            setTimeout(captureLoop, 33);
        } catch (error) {
            console.error('[MediaController] EXCEPTION in frame capture:', error);
            console.error('[MediaController] - Error message:', error.message);
            console.error('[MediaController] - Error stack:', error.stack);
            setTimeout(captureLoop, 100);
        }
    };

    // Start the capture loop
    console.log('[MediaController] Starting capture loop...');
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
    console.log('[MediaController] Stopping recording process...');
    
    if (!workerRef.current || !isRecordingRef.current) {
        console.log('[MediaController] Cannot stop - worker not ready or not recording');
        return;
    }
    
    // Immediately stop recording state
    isRecordingRef.current = false;
    setIsRecording(false);
    
    // Stop capturing frames immediately
    if (captureIntervalRef.current) {
        console.log('[MediaController] Cancelling frame capture');
        cancelAnimationFrame(captureIntervalRef.current);
        captureIntervalRef.current = null;
    }
    
    // Stop status checking
    if (statusIntervalRef.current) {
        console.log('[MediaController] Clearing status interval');
        clearInterval(statusIntervalRef.current);
        statusIntervalRef.current = null;
    }

    console.log('[MediaController] Frame capture stopped');
    
    setIsProcessing(true);
    showNotification('Processing video...', 'info');
    
    console.log('[MediaController] Sending stop recording message to worker');
    
    // Tell worker to stop recording and wait for response
    workerRef.current.postMessage({ 
        type: 'stopRecording',
        forceStop: true // Add flag to force stop processing frames
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
    if (!isRecording) return "Start Recording";
    if (isProcessing) return "Processing... Please wait";
    return `Stop Recording (${formatTime(elapsedTime)}, ${frameCount} frames)`;
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
              sx={getRecordingButtonStyles()}
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