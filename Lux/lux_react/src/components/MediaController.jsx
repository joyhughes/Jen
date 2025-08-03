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
  useTheme,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Typography,
  List,
  ListItem,
  ListItemText,
  ListItemSecondaryAction,
  Chip,
  Badge
} from '@mui/material';
import { 
  Pause, 
  Play, 
  RotateCcw, 
  Save, 
  SkipForward, 
  Video,
  VideoOff,
  Camera,
  FolderOpen,
  Delete,
  CloudDownload,
  CloudUpload,
  Settings,
  Check,
  Download,
  Upload
} from 'lucide-react';
import { ControlPanelContext } from './InterfaceContainer';
import { useScene } from './SceneContext';
import { SceneStorage } from '../utils/sceneStorage.js';

const MediaController = ({ isOverlay = false, saveSceneConfig, hasUnsavedChanges, currentSceneName, exportSceneState, importSceneState }) => {
  const theme = useTheme();
  const isMobile = useMediaQuery(theme.breakpoints.down('sm'));
  const [isRunning, setIsRunning] = useState(true); // Start playing by default to match backend
  
  // Get reset functionality from context
  const { triggerReset, sliderValues, onSliderChange } = React.useContext(ControlPanelContext);
  
  // Get scene context to listen for scene changes
  const { sceneChangeTrigger } = useScene();

  // Scene management states
  const [showSaveDialog, setShowSaveDialog] = useState(false);
  const [savedConfigs, setSavedConfigs] = useState([]);
  const [storageInfo, setStorageInfo] = useState({ configCount: 0, formattedSize: '0 Bytes' });
  const [isSaving, setIsSaving] = useState(false);
  const [saveSuccess, setSaveSuccess] = useState(false);

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
  const recordingInterval = useRef(null);

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


  // Show notification
  const showNotification = (message, severity = 'success') => {
    setNotification({
      open: true,
      message,
      severity
    });
  };

  // Enhanced mobile device detection with logging - MEMOIZED to prevent infinite re-renders
  const isMobileDevice = useMemo(() => {
    const userAgent = navigator.userAgent;
    const hasTouch = navigator.maxTouchPoints && navigator.maxTouchPoints > 2;
    const isMobileUA = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(userAgent);

    const result = isMobileUA || hasTouch;
    console.log('[MediaController] Device detection (memoized):', { result, userAgent: userAgent.slice(0, 50) + '...' });

    return result;
  }, []); // Empty deps - only calculate once

  // Safe feature detection for camera roll saving - MEMOIZED to prevent infinite re-renders
  const cameraRollSupport = useMemo(() => {
    try {
      console.log('[MediaController] Checking camera roll support (memoized)...');

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
        result = 'android-fallback'
      }

      console.log('[MediaController] Camera roll support (memoized):', result);
      return result;
    } catch (error) {
      console.error('[MediaController] Error in camera roll detection:', error.message);
      return null;
    }
  }, []); // Empty deps - only calculate once

  // Scene Storage Functions
  const loadSavedConfigs = useCallback(() => {
    const configs = SceneStorage.listSavedConfigs();
    const configsWithMetadata = configs.map(sceneName => ({
      sceneName,
      metadata: SceneStorage.getConfigMetadata(sceneName)
    }));
    setSavedConfigs(configsWithMetadata);
    setStorageInfo(SceneStorage.getStorageInfo());
  }, []);

  const handleOpenSaveDialog = useCallback(() => {
    loadSavedConfigs();
    setShowSaveDialog(true);
  }, [loadSavedConfigs]);

  const handleCloseSaveDialog = useCallback(() => {
    setShowSaveDialog(false);
  }, []);

  const handleSaveConfig = useCallback(async () => {
    if (saveSceneConfig) {
      setIsSaving(true);
      setSaveSuccess(false);
      try {
        const success = await saveSceneConfig();
        if (success) {
          setSaveSuccess(true);
          loadSavedConfigs(); // Refresh the list
          // Reset success state after 2 seconds
          setTimeout(() => setSaveSuccess(false), 2000);
        }
        // Note: Notification is handled by the ControlPanel's saveSceneConfig function
      } catch (error) {
        console.error('Error saving config:', error);
        showNotification(`Failed to save: ${error.message}`, 'error');
      } finally {
        setIsSaving(false);
      }
    }
    handleCloseSaveDialog();
  }, [saveSceneConfig, loadSavedConfigs, handleCloseSaveDialog, showNotification]);

  const handleDeleteConfig = useCallback((sceneName) => {
    try {
      SceneStorage.deleteSceneConfig(sceneName);
      loadSavedConfigs(); // Refresh the list
      showNotification(`Deleted configuration for ${sceneName}`, 'success');
    } catch (error) {
      console.error('Error deleting config:', error);
      showNotification(`Failed to delete configuration: ${error.message}`, 'error');
    }
  }, [loadSavedConfigs]);

  const handleLoadConfig = useCallback(async (sceneName) => {
    try {
      // Load the saved configuration
      const savedConfig = SceneStorage.loadSceneConfig(sceneName);
      if (savedConfig && savedConfig.sliderValues) {
        // Apply the saved slider values
        for (const [name, value] of Object.entries(savedConfig.sliderValues)) {
          try {
            if (window.module && typeof window.module.set_slider_value === 'function') {
              window.module.set_slider_value(name, value);
            }
          } catch (error) {
            console.error(`Error loading slider value for ${name}:`, error);
          }
        }
        
        // Update the context with the loaded values
        onSliderChange(savedConfig.sliderValues);
        
        showNotification(`Loaded configuration for ${sceneName}`, 'success');
        console.log(`Loaded configuration for scene: ${sceneName}`);
      } else {
        showNotification(`No saved configuration found for ${sceneName}`, 'warning');
      }
      handleCloseSaveDialog();
    } catch (error) {
      console.error('Error loading config:', error);
      showNotification(`Failed to load configuration: ${error.message}`, 'error');
    }
  }, [handleCloseSaveDialog, onSliderChange]);

  const handleExportConfigs = useCallback(() => {
    try {
      const configs = SceneStorage.exportAllConfigs();
      const dataStr = JSON.stringify(configs, null, 2);
      const dataBlob = new Blob([dataStr], { type: 'application/json' });
      
      const url = URL.createObjectURL(dataBlob);
      const link = document.createElement('a');
      link.href = url;
      link.download = `jen_scene_configs_${new Date().toISOString().split('T')[0]}.json`;
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
      URL.revokeObjectURL(url);
      
      showNotification('Configurations exported successfully', 'success');
    } catch (error) {
      console.error('Error exporting configs:', error);
      showNotification(`Failed to export configurations: ${error.message}`, 'error');
    }
  }, []);

  const handleImportConfigs = useCallback(() => {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.json';
    input.onchange = (event) => {
      const file = event.target.files[0];
      if (file) {
        const reader = new FileReader();
        reader.onload = (e) => {
          try {
            const configs = JSON.parse(e.target.result);
            const result = SceneStorage.importConfigs(configs, false);
            showNotification(`Imported ${result.imported} configs, skipped ${result.skipped} existing`, 'success');
            loadSavedConfigs(); // Refresh the list
          } catch (error) {
            console.error('Error importing configs:', error);
            showNotification(`Failed to import configurations: ${error.message}`, 'error');
          }
        };
        reader.readAsText(file);
      }
    };
    input.click();
  }, [loadSavedConfigs]);

  const handleClearAllConfigs = useCallback(() => {
    if (window.confirm('Are you sure you want to delete all saved configurations? This cannot be undone.')) {
      try {
        SceneStorage.clearAllConfigs();
        loadSavedConfigs(); // Refresh the list
        showNotification('All configurations cleared', 'success');
      } catch (error) {
        console.error('Error clearing all configs:', error);
        showNotification(`Failed to clear configurations: ${error.message}`, 'error');
      }
    }
  }, [loadSavedConfigs]);

  // Export complete scene state
  const handleExportCompleteState = useCallback(async () => {
    if (exportSceneState) {
      try {
        await exportSceneState();
      } catch (error) {
        console.error('Error exporting scene state:', error);
        showNotification(`Failed to export scene state: ${error.message}`, 'error');
      }
    } else {
      showNotification('Scene state export not available', 'error');
    }
  }, [exportSceneState, showNotification]);

  // Import complete scene state
  const handleImportCompleteState = useCallback(() => {
    if (importSceneState) {
      const input = document.createElement('input');
      input.type = 'file';
      input.accept = '.json';
      input.onchange = async (event) => {
        const file = event.target.files[0];
        if (file) {
          try {
            await importSceneState(file);
          } catch (error) {
            console.error('Error importing scene state:', error);
            showNotification(`Failed to import scene state: ${error.message}`, 'error');
          }
        }
      };
      input.click();
    } else {
      showNotification('Scene state import not available', 'error');
    }
  }, [importSceneState, showNotification]);

  // Wrapper for main save button with loading state
  const handleMainSaveConfig = useCallback(async () => {
    if (saveSceneConfig) {
      setIsSaving(true);
      setSaveSuccess(false);
      try {
        const success = await saveSceneConfig();
        if (success) {
          setSaveSuccess(true);
          // Reset success state after 2 seconds
          setTimeout(() => setSaveSuccess(false), 2000);
        }
        // Note: Notification is handled by the ControlPanel's saveSceneConfig function
      } catch (error) {
        console.error('Error saving config:', error);
        showNotification(`Failed to save: ${error.message}`, 'error');
      } finally {
        setIsSaving(false);
      }
    }
  }, [saveSceneConfig, showNotification]);

  // Update ref when state changes
  useEffect(() => {
    isRecordingRef.current = isRecording;
  }, [isRecording]);

  // Sync frontend running state with backend state on mount and when scenes change
  useEffect(() => {
    const syncWithBackend = () => {
      if (window.module && typeof window.module.get_animation_running === 'function') {
        const backendRunning = window.module.get_animation_running();
        console.log('[MediaController] Syncing with backend animation state:', backendRunning);
        setIsRunning(backendRunning);
      }
    };

    // Sync immediately
    syncWithBackend();

    // Set up interval to sync periodically (in case scene changes externally)
    const syncInterval = setInterval(syncWithBackend, 1000);

    return () => {
      clearInterval(syncInterval);
    };
  }, []);

  // Sync when scene changes
  useEffect(() => {
    if (window.module && typeof window.module.get_animation_running === 'function') {
      const backendRunning = window.module.get_animation_running();
      console.log('[MediaController] Scene changed - syncing animation state:', backendRunning);
      setIsRunning(backendRunning);
    }
  }, [sceneChangeTrigger]);

  // Initialize worker on component mount
  useEffect(() => {
    mobileLog('MediaController initializing...');
    mobileLog('Component mount - device info:', {
      isMobile: isMobileDevice,
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
            mobileLog('✓ Recording backend started successfully');
            // Frame capture and status polling are already running from startRecording()
            // No need to restart them here
            mobileLog('Frame capture already running, backend now ready');
          } else {
            mobileLog('✗ Recording backend failed to start');
            mobileLog('- Error:', message.error || 'Unknown error');

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

  // Video recording functions (keeping all the existing implementation)
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
    console.log('[MediaController] Setting recording state immediately...');
    isRecordingRef.current = true;
    setIsRecording(true);
    setRecordingStartTime(Date.now());
    setFrameCount(0);

    // Start frame capture immediately - don't wait for worker response
    console.log('[MediaController] Starting frame capture immediately...');
    captureFrame();

    // Start status polling immediately
    console.log('[MediaController] Starting status polling...');
    startPollingStatus();

    try {
      console.log('[MediaController] Sending startRecording message to worker...');
      workerRef.current.postMessage({
        type: 'startRecording',
        options
      });

      console.log('[MediaController] Start recording message sent successfully');
      showNotification('Recording started!', 'success');
    } catch (error) {
      console.error('[MediaController] EXCEPTION sending start recording message:', error);
      
      // Rollback immediate start on error
      isRecordingRef.current = false;
      setIsRecording(false);
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

  // Save video to camera roll (mobile-optimized)
  const saveToMobileCameraRoll = async (blob, filename) => {
    mobileLog('=== SAVING TO MOBILE CAMERA ROLL ===');
    mobileLog('Device detection:', {
      isMobile: isMobileDevice,
      userAgent: navigator.userAgent,
      supportType: cameraRollSupport
    });

    const supportType = cameraRollSupport;

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

        if (isMobileDevice) {
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

  // Format time (MM:SS)
  const formatTime = (seconds) => {
    const mins = Math.floor(seconds / 60).toString().padStart(2, '0');
    const secs = (seconds % 60).toString().padStart(2, '0');
    return `${mins}:${secs}`;
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

  return (
    <>
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

          {/* Scene Save Controls */}
          <Tooltip title={
            isSaving ? "Saving..." : 
            saveSuccess ? "Saved successfully!" : 
            "Save Scene Configuration"
          }>
            <Badge
              color="warning"
              variant="dot"
              invisible={!hasUnsavedChanges}
            >
              <IconButton
                onClick={handleMainSaveConfig}
                sx={{
                  ...buttonStyles,
                  color: saveSuccess ? theme.palette.success.main : 
                         hasUnsavedChanges ? theme.palette.warning.main : 
                         buttonStyles.color,
                  ...(isSaving && {
                    animation: 'spin 1s linear infinite',
                    '@keyframes spin': {
                      '0%': { transform: 'rotate(0deg)' },
                      '100%': { transform: 'rotate(360deg)' }
                    }
                  }),
                  ...(saveSuccess && {
                    animation: 'pulse 0.5s ease-in-out',
                    '@keyframes pulse': {
                      '0%': { transform: 'scale(1)' },
                      '50%': { transform: 'scale(1.1)' },
                      '100%': { transform: 'scale(1)' }
                    }
                  })
                }}
                size="medium"
                disabled={!currentSceneName || isSaving}
              >
                {isSaving ? (
                  <CircularProgress size={iconSize - 4} color="inherit" />
                ) : saveSuccess ? (
                  <Check size={iconSize} />
                ) : (
                  <Save size={iconSize} />
                )}
              </IconButton>
            </Badge>
          </Tooltip>

          <Tooltip title="Manage Saved Configurations" arrow>
            <IconButton
              onClick={handleOpenSaveDialog}
              sx={buttonStyles}
              size="medium"
            >
              <FolderOpen size={iconSize} />
            </IconButton>
          </Tooltip>

          {/* Export/Import Complete Scene State */}
          <Tooltip title="Export Complete Scene State" arrow>
            <IconButton
              onClick={handleExportCompleteState}
              sx={buttonStyles}
              size="medium"
            >
              <Download size={iconSize} />
            </IconButton>
          </Tooltip>

          <Tooltip title="Import Complete Scene State" arrow>
            <IconButton
              onClick={handleImportCompleteState}
              sx={buttonStyles}
              size="medium"
            >
              <Upload size={iconSize} />
            </IconButton>
          </Tooltip>

        </Box>
      </Paper>

      {/* Scene Management Dialog */}
      <Dialog
        open={showSaveDialog}
        onClose={handleCloseSaveDialog}
        maxWidth="md"
        fullWidth
        PaperProps={{
          sx: { maxHeight: '80vh' }
        }}
      >
        <DialogTitle>
          <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
            <Typography variant="h6">
              Scene Configuration Manager
            </Typography>
            <Box sx={{ display: 'flex', gap: 1 }}>
              <Tooltip title="Export All Configs">
                <IconButton onClick={handleExportConfigs} size="small">
                  <CloudDownload size={16} />
                </IconButton>
              </Tooltip>
              <Tooltip title="Import Configs">
                <IconButton onClick={handleImportConfigs} size="small">
                  <CloudUpload size={16} />
                </IconButton>
              </Tooltip>
              <Tooltip title="Clear All Configs">
                <IconButton onClick={handleClearAllConfigs} size="small" color="error">
                  <Delete size={16} />
                </IconButton>
              </Tooltip>
            </Box>
          </Box>
        </DialogTitle>
        
        <DialogContent dividers>
          {/* Current Scene Section */}
          {currentSceneName && (
            <Box sx={{ mb: 3 }}>
              <Typography variant="h6" gutterBottom>
                Current Scene: {currentSceneName}
              </Typography>
              <Box sx={{ display: 'flex', gap: 1, alignItems: 'center', flexWrap: 'wrap' }}>
                <Button
                  variant="contained"
                  startIcon={
                    isSaving ? <CircularProgress size={16} /> : 
                    saveSuccess ? <Check size={16} /> : 
                    <Save size={16} />
                  }
                  onClick={handleSaveConfig}
                  disabled={!hasUnsavedChanges || isSaving}
                  color={
                    saveSuccess ? "success" :
                    hasUnsavedChanges ? "warning" : 
                    "primary"
                  }
                  size="small"
                >
                  {isSaving ? 'Saving...' : 
                   saveSuccess ? 'Saved!' : 
                   (hasUnsavedChanges ? 'Save Changes' : 'Saved')}
                </Button>
                {SceneStorage.hasConfigForScene(currentSceneName) && (
                  <Chip
                    label="Has saved configuration"
                    size="small"
                    color="success"
                    variant="outlined"
                  />
                )}
              </Box>
            </Box>
          )}

          {/* Storage Info */}
          <Alert severity="info" sx={{ mb: 2 }}>
            <Typography variant="body2">
              <strong>{storageInfo.configCount}</strong> saved configurations using <strong>{storageInfo.formattedSize}</strong>
            </Typography>
          </Alert>

          {/* Saved Configurations List */}
          {savedConfigs.length > 0 ? (
            <List sx={{ maxHeight: '400px', overflow: 'auto' }}>
              {savedConfigs.map(({ sceneName, metadata }) => (
                <ListItem key={sceneName} divider>
                  <ListItemText
                    primary={
                      <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                        <Typography variant="subtitle1">
                          {sceneName}
                        </Typography>
                        {sceneName === currentSceneName && (
                          <Chip 
                            label="Current" 
                            size="small" 
                            color="primary" 
                            variant="outlined" 
                          />
                        )}
                      </Box>
                    }
                    secondary={
                      metadata ? (
                        <Box>
                          <Typography variant="body2" color="text.secondary">
                            Saved: {metadata.lastModified}
                          </Typography>
                          <Typography variant="caption" color="text.secondary">
                            Size: {SceneStorage.formatBytes(metadata.size)} • Version: {metadata.version}
                          </Typography>
                        </Box>
                      ) : (
                        <Typography variant="body2" color="text.secondary">
                          No metadata available
                        </Typography>
                      )
                    }
                  />
                  <ListItemSecondaryAction>
                    <Box sx={{ display: 'flex', gap: 0.5 }}>
                      <Tooltip title="Load Configuration">
                        <IconButton
                          edge="end"
                          onClick={() => handleLoadConfig(sceneName)}
                          size="small"
                          color="primary"
                        >
                          <FolderOpen size={16} />
                        </IconButton>
                      </Tooltip>
                      <Tooltip title="Delete Configuration">
                        <IconButton
                          edge="end"
                          onClick={() => handleDeleteConfig(sceneName)}
                          size="small"
                          color="error"
                        >
                          <Delete size={16} />
                        </IconButton>
                      </Tooltip>
                    </Box>
                  </ListItemSecondaryAction>
                </ListItem>
              ))}
            </List>
          ) : (
            <Box sx={{ textAlign: 'center', py: 4 }}>
              <Settings size={48} color={theme.palette.text.disabled} style={{ marginBottom: 8 }} />
              <Typography variant="body1" color="text.secondary">
                No saved configurations found
              </Typography>
              <Typography variant="body2" color="text.disabled">
                Save your first configuration to see it here
              </Typography>
            </Box>
          )}
        </DialogContent>

        <DialogActions sx={{ p: 2 }}>
          <Button onClick={handleCloseSaveDialog} color="primary">
            Close
          </Button>
        </DialogActions>
      </Dialog>

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
    </>
  );
}

export default MediaController;