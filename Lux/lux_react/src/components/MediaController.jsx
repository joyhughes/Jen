import React, {useCallback, useEffect, useRef, useState, useMemo} from 'react';
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
  Pause, 
  Play, 
  RotateCcw, 
  Save, 
  SkipForward, 
  Video,
  VideoOff,
  Camera
} from 'lucide-react';
import { ControlPanelContext } from './InterfaceContainer';
import { useScene } from './SceneContext';

function MediaController({ isOverlay = false }) {
  const theme = useTheme();
  const isMobile = useMediaQuery(theme.breakpoints.down('sm'));
  const [isRunning, setIsRunning] = useState(true); // Start playing by default to match backend
  
  const { triggerReset } = React.useContext(ControlPanelContext);
  
  const { sceneChangeTrigger } = useScene();

  const [isRecording, setIsRecording] = useState(false);
  const [isProcessing, setIsProcessing] = useState(false);
  const [frameCount, setFrameCount] = useState(0);
  const [recordingStartTime, setRecordingStartTime] = useState(null);
  const [elapsedTime, setElapsedTime] = useState(0);

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
  const recordingInterval = useRef(null);

  // Remaining call sites are error paths: log and surface a notification.
  const mobileLog = (message, data = null) => {
    console.error(`[MediaController] ${message}`, data ?? '');
    if (message.includes('ERROR') || message.includes('CRITICAL')) {
      showNotification(`Debug: ${message}`, 'error');
    }
  };

  // Enhanced mobile device detection with logging - MEMOIZED to prevent infinite re-renders
  const isMobileDevice = useMemo(() => {
    const userAgent = navigator.userAgent;
    const hasTouch = navigator.maxTouchPoints && navigator.maxTouchPoints > 2;
    const isMobileUA = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(userAgent);

    const result = isMobileUA || hasTouch;

    return result;
  }, []); // Empty deps - only calculate once

  // Safe feature detection for camera roll saving - MEMOIZED to prevent infinite re-renders
  const cameraRollSupport = useMemo(() => {
    try {

      // Check for Web Share API (iOS Safari, Android Chrome)
      const hasWebShare = typeof navigator.share === 'function' && typeof navigator.canShare === 'function';
      
      // Check for File System Access API (Android Chrome)
      const hasFileSystem = typeof window.showSaveFilePicker === 'function';
      
      // Check for iOS
      const isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent);
      
      // Check for Android
      const isAndroid = /Android/.test(navigator.userAgent);

      let result = null;
      if (hasWebShare) {
        result = 'webshare';
      } else if (hasFileSystem) {
        result = 'filesystem';
      } else if (isIOS) {
        result = 'ios-fallback';
      } else if (isAndroid) {
        result = 'android-fallback';
      }

      return result;
    } catch (error) {
      console.error('[MediaController] Error in camera roll detection:', error.message);
      return null;
    }
  }, []); // Empty deps - only calculate once

  // Update ref when state changes
  useEffect(() => {
    isRecordingRef.current = isRecording;
  }, [isRecording]);

  // Sync frontend running state with backend state on mount and when scenes change
  useEffect(() => {
    const syncWithBackend = () => {
      if (window.module && typeof window.module.get_animation_running === 'function') {
        const backendRunning = window.module.get_animation_running();
        setIsRunning(backendRunning);
      }
    };

    syncWithBackend();

    const syncInterval = setInterval(syncWithBackend, 1000);

    return () => {
      clearInterval(syncInterval);
    };
  }, []);

  useEffect(() => {
    if (window.module && typeof window.module.get_animation_running === 'function') {
      const backendRunning = window.module.get_animation_running();
      setIsRunning(backendRunning);
    }
  }, [sceneChangeTrigger]);

  // The worker is created lazily on first use (see ensureWorker) so page load
  // doesn't download a second copy of the WASM module just in case the user
  // records video. This effect only handles cleanup on unmount.
  useEffect(() => {

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
  }, []);

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

  // Create and initialize the video encoding worker on first use. The worker
  // queues messages sent right after 'init' itself, so callers may post
  // immediately after this returns.
  const ensureWorker = () => {
    if (workerRef.current) {
      return true;
    }

    try {
      // BASE_URL-relative so the worker also loads when the app is served
      // from a subpath (e.g. GitHub Pages /Jen/)
      const worker = new Worker(`${import.meta.env.BASE_URL}workers/videoEncodingWorker.js`, { type: 'module' });

      worker.onmessage = handleWorkerMessage;

      worker.onerror = (error) => {
        mobileLog('ERROR - Worker error:', error.message);
        showNotification('Recording system error: ' + error.message, 'error');
      };

      worker.onmessageerror = (error) => {
        mobileLog('ERROR - Worker message error:', error.message);
        showNotification('Recording system message error', 'error');
      };

      workerRef.current = worker;

      // Send init message to worker with correct WASM URL.
      // new URL(..., import.meta.url) lets Vite resolve the lux.js module both
      // in dev (/src/lux.js) and in production builds (hashed asset URL), so
      // the worker can import it from any deploy path.
      worker.postMessage({
        type: 'init',
        wasmUrl: new URL('../lux.js', import.meta.url).href
      });

      isInitializedRef.current = true;
      return true;
    } catch (error) {
      mobileLog('CRITICAL ERROR - Failed to create worker:', error.message);
      showNotification('Failed to initialize recording system: ' + error.message, 'error');
      return false;
    }
  };

  // Handle worker messages
  const handleWorkerMessage = (event) => {

    if (!event || !event.data) {
      mobileLog('ERROR: Invalid worker message received');
      return;
    }

    const message = event.data;

    try {
      if (!message || typeof message !== 'object') {
        throw new Error('Invalid message format received from worker');
      }

      switch (message.type) {
        case 'initialized':
          break;

        case 'recordingStarted':

          if (message.success) {
            // Frame capture and status polling are already running from startRecording()
            // No need to restart them here
          } else {

            // Stop the frame capture that was started immediately
            isRecordingRef.current = false;
            setIsRecording(false);

            // Stop status polling
            if (statusIntervalRef.current) {
              clearInterval(statusIntervalRef.current);
              statusIntervalRef.current = null;
            }

            showNotification(`Failed to start recording: ${message.error || 'Unknown error'}`, 'error');
          }
          break;

        case 'recordingStopped':

          isRecordingRef.current = false;
          setIsRecording(false);
          setIsProcessing(false);

          if (message.success) {

            // Create and download the video file with proper MIME type
            const blob = new Blob([message.videoData], {
              type: message.mimeType || 'video/mp4; codecs="avc1.42E01E"'
            });

            // Verify blob size
            if (blob.size < 1000) {
              mobileLog('ERROR: Video data too small - blob size:', blob.size);
              showNotification('Recording failed: Video data too small', 'error');
              return;
            }

            // Generate filename with timestamp
            const filename = `jen-recording-${new Date().toISOString().slice(0,19).replace(/:/g,'-')}.mp4`;

            // Try mobile camera roll save first, fallback to standard download
            saveToMobileCameraRoll(blob, filename).then((success) => {
              if (!success) {
                // This is already handled in the saveToMobileCameraRoll function
              }
            }).catch((error) => {
              // Fallback is already handled in the function
            });

            // Log final metrics
            if (message.metrics) {
            }
          } else {
            showNotification(`Recording failed: ${message.error || 'Unknown error'}`, 'error');
          }
          break;

        case 'recordingProgress':

          if (typeof message.frameCount === 'number') {
            setFrameCount(message.frameCount);

            if (message.metrics) {
              setPerformanceMetrics(message.metrics);
            }
          }
          break;

        case 'recorderState':

          if (typeof message.frameCount === 'number') {
            setFrameCount(message.frameCount);
          }
          break;

        case 'error':
          mobileLog('=== WORKER ERROR ===');

          showNotification(`Error: ${message.error || 'Unknown error'}`, 'error');

          if (isRecording) {
            stopRecording();
          }
          break;

        default:
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
    if (window.module && triggerReset) {
      // First restart the scene (resets time to 0.0f)
      window.module.restart();
      
      // Then reset all scene parameters to defaults (this will trigger integrator reset due to time change)
      if (typeof window.module.reset_scene_parameters === 'function') {
        //window.module.reset_scene_parameters();
      }
      
      // Trigger UI reset to refresh all widget displays
      triggerReset();
      
      showNotification('Scene reset to defaults', 'success');
    }
  };

  const handleAdvance = () => {
    if (window.module && typeof window.module.advance_frame === 'function') {
      window.module.advance_frame();
      
      // Sync with backend state after calling advance_frame
      if (typeof window.module.get_animation_running === 'function') {
        const backendRunning = window.module.get_animation_running();
        setIsRunning(backendRunning);
      } else {
        // Fallback: advance_frame should pause animation
        setIsRunning(false);
      }
    }
  };

  const handleRunPause = () => {
    if (window.module && typeof window.module.run_pause === 'function') {
      window.module.run_pause();
      
      // Sync with backend state after calling run_pause
      if (typeof window.module.get_animation_running === 'function') {
        const backendRunning = window.module.get_animation_running();
        setIsRunning(backendRunning);
      } else {
        // Fallback to toggle if get_animation_running is not available
        setIsRunning(!isRunning);
      }
    }
  };

  const handleTakeScreenshot = () => {
    if (window.module && typeof window.module.get_img_data === 'function') {
      try {
        // Get image data from the backend
        const imageDataVal = window.module.get_img_data();
        if (!imageDataVal) {
          showNotification('Failed to capture screenshot: No image data', 'error');
          return;
        }

        const width = window.module.get_buf_width();
        const height = window.module.get_buf_height();
        
        if (!width || !height) {
          showNotification('Failed to capture screenshot: Invalid dimensions', 'error');
          return;
        }

        // Create canvas to process the image data
        const canvas = document.createElement('canvas');
        canvas.width = width;
        canvas.height = height;
        const ctx = canvas.getContext('2d');

        // Get the image buffer
        const bufferLength = imageDataVal.byteLength;
        const pixelData = new Uint8ClampedArray(imageDataVal.buffer, imageDataVal.byteOffset, bufferLength);

        // Convert to RGBA format for canvas
        const rgbaData = new Uint8ClampedArray(bufferLength);
        for (let i = 0; i < bufferLength; i += 4) {
          rgbaData[i] = pixelData[i];         // R
          rgbaData[i + 1] = pixelData[i + 1]; // G
          rgbaData[i + 2] = pixelData[i + 2]; // B
          rgbaData[i + 3] = 255;              // A (full opacity)
        }

        // Create ImageData and render to canvas
        const imageData = new ImageData(rgbaData, width, height);
        ctx.putImageData(imageData, 0, 0);

        // Generate filename and download
        const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
        const filename = `jen-screenshot-${timestamp}.png`;

        canvas.toBlob((blob) => {
          if (blob) {
            const url = URL.createObjectURL(blob);
            const link = document.createElement('a');
            link.href = url;
            link.download = filename;
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
            URL.revokeObjectURL(url);
            
            showNotification('Screenshot saved!', 'success');
          } else {
            showNotification('Failed to create screenshot file', 'error');
          }
        }, 'image/png');

      } catch (error) {
        console.error('Screenshot error:', error);
        showNotification('Failed to take screenshot: ' + error.message, 'error');
      }
    } else {
      showNotification('Screenshot function not available', 'error');
    }
  };

  // Toggle recording
  const handleToggleRecording = () => {

    if (isProcessing) {
      return; // Prevent action while processing
    }

    if (isRecording) {
      stopRecording();
    } else {
      startRecording();
    }
  };

  // Capture frames from canvas and send to worker
  const captureFrame = useCallback(() => {

    if (!isRecordingRef.current) {
      return;
    }

    let frameNumber = 0;
    const captureStartTime = performance.now();

    const captureLoop = () => {
        // SAFETY CHECK: Always check recording state at the start of each loop
        if (!isRecordingRef.current) {
          return;
        }

        frameNumber++;
        const loopStartTime = performance.now();

        try {
            // Get the canvas element
            const canvas = document.querySelector('canvas');
            if (!canvas) {
                mobileLog('ERROR: Canvas not found for frame capture');
                setTimeout(captureLoop, 100);
                return;
            }

            // Get image data directly from the canvas
            const ctx = canvas.getContext('2d');
            if (!ctx) {
                mobileLog('ERROR: Canvas context not available');
                setTimeout(captureLoop, 100);
                return;
            }

            // Capture the current frame from the canvas
            const captureDataStart = performance.now();
            const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
            const captureDataTime = performance.now() - captureDataStart;

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

            if (!hasNonZeroData) {
            }

            // FINAL SAFETY CHECK: Verify recording is still active before sending
            if (!isRecordingRef.current) {
                return;
            }

            // Send to worker

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

                // Calculate FPS
                const elapsedSeconds = (performance.now() - captureStartTime) / 1000;
                const currentFps = frameNumber / elapsedSeconds;

            } else {
                mobileLog('ERROR: Cannot send frame to worker', {
                  workerAvailable: !!workerRef.current,
                  imageDataLength: imageData.data.length
                });
            }

            // Schedule next frame at 30fps (33.33ms interval) - but only if still recording
            if (isRecordingRef.current) {
                setTimeout(captureLoop, 33);
            } else {
            }
        } catch (error) {

            // Only continue if still recording
            if (isRecordingRef.current) {
                setTimeout(captureLoop, 100);
            }
        }
    };

    // Start the capture loop
    captureLoop();
  }, []);

  // Start recording
  const startRecording = () => {

    // Lazily create the encoding worker the first time recording starts
    ensureWorker();

    if (!workerRef.current || !isInitializedRef.current) {
      console.error('[MediaController] ERROR: Worker not ready');
      console.error('[MediaController] - Worker available:', !!workerRef.current);
      console.error('[MediaController] - Worker initialized:', isInitializedRef.current);
      showNotification('Recording system not ready', 'error');
      return;
    }

    const canvas = document.querySelector('canvas');
    if (!canvas) {
      console.error('[MediaController] ERROR: Canvas not found');
      showNotification('Canvas not found', 'error');
      return;
    }

    const width = canvas.width % 2 === 0 ? canvas.width : canvas.width - 1;
    const height = canvas.height % 2 === 0 ? canvas.height : canvas.height - 1;

    const options = {
      width,
      height,
      fps: 30,
      bitrate: 2500000,
      codec: 'libx264',
      format: 'mp4',
      preset: 'ultrafast'
    };

    recordingOptionsRef.current = options;

    // IMMEDIATE START: Set recording state and start frame capture immediately
    isRecordingRef.current = true;
    setIsRecording(true);
    setRecordingStartTime(Date.now());
    setFrameCount(0);

    // Start frame capture immediately - don't wait for worker response
    captureFrame();

    // Start status polling immediately
    startPollingStatus();

    try {
      workerRef.current.postMessage({
        type: 'startRecording',
        options
      });

      showNotification('Recording started!', 'success');
    } catch (error) {
      console.error('[MediaController] EXCEPTION sending start recording message:', error);
      console.error('[MediaController] - Error message:', error.message);
      console.error('[MediaController] - Error stack:', error.stack);

      // Rollback immediate start on error
      isRecordingRef.current = false;
      setIsRecording(false);
      showNotification('Failed to start recording: ' + error.message, 'error');
    }
  };

  // Stop recording
  const stopRecording = () => {

    if (!workerRef.current || !isRecordingRef.current) {
        return;
    }

    // Immediately stop recording state to prevent new frames from being captured
    isRecordingRef.current = false;
    setIsRecording(false);

    // Stop capturing frames immediately
    if (captureIntervalRef.current) {
        cancelAnimationFrame(captureIntervalRef.current);
        captureIntervalRef.current = null;
    }

    // Stop status checking
    if (statusIntervalRef.current) {
        clearInterval(statusIntervalRef.current);
        statusIntervalRef.current = null;
    }

    setIsProcessing(true);
    showNotification('Processing video...', 'info');

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
        const mobile = isMobileDevice;
        const saveType = cameraRollSupport;

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

    const supportType = cameraRollSupport;

    try {
      switch (supportType) {
        case 'webshare':

          try {
            // Create a File object for sharing
            const file = new File([blob], filename, { type: blob.type });

            // Check if we can share this file type
            if (navigator.canShare && navigator.canShare({ files: [file] })) {
              await navigator.share({
                title: 'Jen Recording',
                text: 'Video recorded with Jen',
                files: [file]
              });

              showNotification('Video shared to camera roll!', 'success');
              return true;
            } else {
              throw new Error('Cannot share this file type');
            }
          } catch (shareError) {
            mobileLog('ERROR in Web Share API:', shareError.message);
            throw shareError;
          }

        case 'filesystem':

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
            return true;
          } catch (fsError) {
            mobileLog('ERROR in File System Access API:', fsError.message);
            throw fsError;
          }

        case 'ios-fallback':

          try {
            // iOS Safari fallback - create a video element and prompt user
            const videoUrl = URL.createObjectURL(blob);

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
                } catch (cleanupError) {
                  mobileLog('ERROR cleaning up iOS overlay:', cleanupError.message);
                }
              }
            });

            document.body.appendChild(overlay);

            showNotification('Long-press video to save to Photos', 'info');
            return true;
          } catch (iosError) {
            mobileLog('ERROR in iOS fallback:', iosError.message);
            throw iosError;
          }

        case 'android-fallback':

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

            // Show instructions for moving to camera roll
            setTimeout(() => {
              showNotification('Video downloaded! Check Downloads folder or move to Gallery', 'info');
            }, 1000);

            setTimeout(() => {
              URL.revokeObjectURL(url);
            }, 5000);

            return true;
          } catch (androidError) {
            mobileLog('ERROR in Android fallback:', androidError.message);
            throw androidError;
          }

        default:
          throw new Error('No mobile camera roll support');
      }

    } catch (error) {
      mobileLog('ERROR saving to camera roll:', error.message);

      // Ultimate fallback - standard download with enhanced error handling
      try {
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
        }, 5000);

        if (isMobileDevice) {
          showNotification('Video downloaded! Check your Downloads folder', 'info');
        } else {
          showNotification('Video downloaded!', 'success');
        }

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
        <Tooltip title="Reset scene" arrow>
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
                              {!isRecording && !isProcessing && isMobileDevice && cameraRollSupport && (
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

        <Tooltip title="Take Screenshot" arrow>
          <IconButton
            onClick={handleTakeScreenshot}
            sx={buttonStyles}
            size="medium"
          >
            <Camera size={iconSize} />
          </IconButton>
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