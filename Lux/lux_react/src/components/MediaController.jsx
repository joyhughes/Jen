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
  const animationFrameRef = useRef(null);
  const captureIntervalRef = useRef(null);
  const statusIntervalRef = useRef(null);
  const frameSequenceRef = useRef(0);
  const isInitializedRef = useRef(false);
  const isRecordingRef = useRef(false);
  const recordingOptionsRef = useRef(null);
  const mediaStreamRef = useRef(null);
  
  const [notification, setNotification] = useState({ open: false, message: '', severity: 'success' });

  // Update ref when state changes
  useEffect(() => {
    isRecordingRef.current = isRecording;
  }, [isRecording]);

  // Initialize worker on component mount
  useEffect(() => {
    // if (isInitializedRef.current) {
    //   return; // Already initialized
    // }

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
    if (!event || !event.data) {
      console.error('[MediaController] Invalid worker message received');
      return;
    }

    const message = event.data;
    console.log('[MediaController] Received worker message:', message);
    
    try {
      if (!message || typeof message !== 'object') {
        throw new Error('Invalid message format received from worker');
      }

      switch (message.type) {
        case 'initialized':
          console.log('[MediaController] Recording system initialized');
          break;
          
        case 'recordingStarted':
          console.log('[MediaController] Recording started response:', message);
          if (message.success) {
            isRecordingRef.current = true; // Set ref first
            setIsRecording(true);
            setRecordingStartTime(Date.now());
            setFrameCount(0); // Reset frame count
            // Start frame capture immediately after recording starts
            captureFrame();
            startPollingStatus();
          } else {
            isRecordingRef.current = false;
            setIsRecording(false);
            showNotification(`Failed to start recording: ${message.error || 'Unknown error'}`, 'error');
          }
          break;
          
        case 'recordingStopped':
          console.log('[MediaController] Recording stopped response:', message);
          isRecordingRef.current = false; // Set ref first
          setIsRecording(false);
          setIsProcessing(false);
          
          if (message.success) {
            // Create and download the video file with proper MIME type
            const blob = new Blob([message.videoData], { 
              type: message.mimeType || 'video/webm; codecs="vp8"'
            });
            
            // Verify blob size
            if (blob.size < 1000) {
              showNotification('Recording failed: Video data too small', 'error');
              return;
            }
            
            console.log(`[MediaController] Video blob created: ${blob.size} bytes, ${message.frameCount} frames`);
            
            // Create a temporary video element to verify the video
            const video = document.createElement('video');
            video.preload = 'metadata';
            
            // Create object URL
            const url = URL.createObjectURL(blob);
            video.src = url;
            
            // Set up event handlers
            const cleanup = () => {
              URL.revokeObjectURL(url);
              video.remove();
            };
            
            video.onloadedmetadata = () => {
              console.log(`[MediaController] Video metadata loaded: ${video.duration}s, ${video.videoWidth}x${video.videoHeight}`);
              
              // Download the video
              const link = document.createElement('a');
              link.href = url;
              link.download = `jen-recording-${new Date().toISOString().slice(0,19).replace(/:/g,'-')}.webm`;
              document.body.appendChild(link);
              link.click();
              document.body.removeChild(link);
              
              showNotification(`Recording saved: ${message.frameCount || 0} frames`, 'success');
              cleanup();
            };
            
            video.onerror = (e) => {
              console.error('[MediaController] Video verification failed:', e);
              console.error('[MediaController] Video error code:', video.error?.code);
              console.error('[MediaController] Video error message:', video.error?.message);
              showNotification('Recording failed: Video data is invalid', 'error');
              cleanup();
            };
            
            // Set a timeout in case the video never loads
            setTimeout(() => {
              if (video.readyState === 0) { // HAVE_NOTHING
                console.error('[MediaController] Video verification timed out');
                showNotification('Recording failed: Video verification timed out', 'error');
                cleanup();
              }
            }, 5000);
          } else {
            showNotification(`Recording failed: ${message.error || 'Unknown error'}`, 'error');
          }
          break;
          
        case 'recordingProgress':
          if (typeof message.frameCount === 'number') {
            setFrameCount(message.frameCount);
          }
          break;

        case 'recorderState':
          if (typeof message.frameCount === 'number') {
            setFrameCount(message.frameCount);
          }
          break;
          
        case 'error':
          console.error('[MediaController] Worker error:', message.error);
          showNotification(`Error: ${message.error || 'Unknown error'}`, 'error');
          
          if (isRecording) {
            stopRecording();
          }
          break;

        default:
          console.warn('[MediaController] Unknown message type:', message.type);
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
    if (!isRecordingRef.current || !mediaStreamRef.current) return;

    const videoTrack = mediaStreamRef.current.getVideoTracks()[0];
    if (!videoTrack) return;

    const imageCapture = new ImageCapture(videoTrack);
    
    // Use requestAnimationFrame for smoother timing
    const captureLoop = () => {
        if (!isRecordingRef.current) return;

        imageCapture.grabFrame()
            .then(frame => {
                if (!isRecordingRef.current) return;
                
                const canvas = document.createElement('canvas');
                canvas.width = frame.width;
                canvas.height = frame.height;
                const ctx = canvas.getContext('2d');
                ctx.drawImage(frame, 0, 0);
                
                // Get image data instead of sending canvas
                const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
                
                // Verify frame data
                if (!imageData || !imageData.data || imageData.data.length === 0) {
                    throw new Error('Invalid frame data');
                }
                
                // Send to worker
                workerRef.current?.postMessage({
                    type: 'addFrame',
                    imageData: imageData.data,
                    width: canvas.width,
                    height: canvas.height,
                    timestamp: performance.now()
                }, [imageData.data.buffer]); // Transfer the buffer ownership
                
                // Schedule next frame
                requestAnimationFrame(captureLoop);
            })
            .catch(err => {
                console.error('Frame capture error:', err);
                // Retry after a short delay
                setTimeout(captureLoop, 100);
            });
    };

    // Start capture loop
    requestAnimationFrame(captureLoop);
  }, []);

  // Start recording
  const startRecording = () => {
    console.log('[MediaController] Starting recording process...');
    
    if (!workerRef.current) {
      console.error('[MediaController] Worker not initialized');
      showNotification('Recording system not ready', 'error');
      return;
    }

    // Check if worker is ready
    if (!isInitializedRef.current) {
      console.error('[MediaController] Worker not fully initialized');
      showNotification('Recording system not ready', 'error');
      return;
    }
    
    // Get canvas for dimensions
    const canvas = document.querySelector('canvas');
    if (!canvas) {
      console.error('[MediaController] Canvas not found');
      showNotification('Canvas not found', 'error');
      return;
    }

    // Set up media stream from canvas
    try {
      mediaStreamRef.current = canvas.captureStream(30); // 30fps
      console.log('[MediaController] Media stream set up from canvas');
    } catch (error) {
      console.error('[MediaController] Failed to set up media stream:', error);
      showNotification('Failed to set up recording stream', 'error');
      return;
    }
    
    // Reset frame counter
    frameSequenceRef.current = 0;
    
    // Get dimensions (ensure even numbers)
    const width = canvas.width % 2 === 0 ? canvas.width : canvas.width - 1;
    const height = canvas.height % 2 === 0 ? canvas.height : canvas.height - 1;
    
    console.log('[MediaController] Canvas dimensions:', { width, height });
    
    // Create recording options with current dimensions
    const options = {
      width,
      height,
      fps: 30, 
      bitrate: 1000000, // Lower bitrate for mobile
      codec: 'libvpx',
      format: 'webm',
      preset: 'realtime' // Always use realtime preset for mobile
    };
    
    // Store options in ref for use in other functions
    recordingOptionsRef.current = options;
    
    console.log('[MediaController] Sending start recording message with options:', options);
    
    try {
      // Send start message to worker
      workerRef.current.postMessage({
        type: 'startRecording',
        options
      });
      
      showNotification('Starting recording...', 'info');
    } catch (error) {
      console.error('[MediaController] Error sending start recording message:', error);
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

    // Clean up media stream
    if (mediaStreamRef.current) {
        mediaStreamRef.current.getTracks().forEach(track => track.stop());
        mediaStreamRef.current = null;
        console.log('[MediaController] Media stream cleaned up');
    }
    
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
          fontWeight: 'bold'
        }}>
          REC {formatTime(elapsedTime)} • {frameCount} frames
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