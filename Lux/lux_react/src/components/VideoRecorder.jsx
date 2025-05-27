import React, { useEffect, useRef, useState } from 'react';
import {
  Box,
  Button,
  IconButton,
  Paper,
  Tooltip,
  Typography,
  Snackbar,
  Alert,
  CircularProgress
} from '@mui/material';
import { Camera, Pause, Play, RotateCcw, Save, SkipForward, Video, VideoOff } from 'lucide-react';

const VideoRecorder = ({ onStatusChange }) => {
  // Component state
  const [isRecording, setIsRecording] = useState(false);
  const [isProcessing, setIsProcessing] = useState(false);
  const [frameCount, setFrameCount] = useState(0);
  const [notification, setNotification] = useState({ open: false, message: '', severity: 'info' });
  const [recordingStartTime, setRecordingStartTime] = useState(null);
  const [elapsedTime, setElapsedTime] = useState(0);
  
  // Refs
  const workerRef = useRef(null);
  const animationFrameRef = useRef(null);
  const canvasRef = useRef(null);
  const statusCheckIntervalRef = useRef(null);
  const frameSequenceRef = useRef(0);

  // Initialize worker on component mount
  useEffect(() => {
    // Create worker
    const worker = new Worker(new URL('./videoEncodingWorker.js', import.meta.url), { type: 'module' });
    
    // Set up message listener
    worker.onmessage = handleWorkerMessage;
    worker.onerror = (error) => {
      console.error('Worker error:', error);
      showNotification(`Worker error: ${error.message}`, 'error');
    };
    
    // Save worker reference
    workerRef.current = worker;
    
    // Initialize worker with module
    initializeWorker();
    
    // Clean up on unmount
    return () => {
      if (isRecording) {
        stopRecording();
      }
      
      if (statusCheckIntervalRef.current) {
        clearInterval(statusCheckIntervalRef.current);
      }
      
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current);
      }
      
      if (workerRef.current) {
        workerRef.current.terminate();
      }
    };
  }, []);
  
  // Update elapsed time during recording
  useEffect(() => {
    if (isRecording && recordingStartTime) {
      const updateTimer = () => {
        setElapsedTime(Math.floor((Date.now() - recordingStartTime) / 1000));
        animationFrameRef.current = requestAnimationFrame(updateTimer);
      };
      
      animationFrameRef.current = requestAnimationFrame(updateTimer);
      return () => cancelAnimationFrame(animationFrameRef.current);
    }
  }, [isRecording, recordingStartTime]);

  // Handler for worker messages
  const handleWorkerMessage = (event) => {
    const message = event.data;
    
    switch (message.type) {
      case 'initialized':
        console.log('Worker initialized successfully');
        showNotification('Recording system ready', 'success');
        break;
        
      case 'recordingStarted':
        if (message.success) {
          console.log('Recording started successfully');
          setIsRecording(true);
          setRecordingStartTime(Date.now());
          setFrameCount(0);
          
          // Start capturing frames
          captureFrames();
          
          // Start status polling
          statusCheckIntervalRef.current = setInterval(pollRecordingStatus, 500);
          
          if (onStatusChange) {
            onStatusChange({ isRecording: true });
          }
        } else {
          console.error('Failed to start recording:', message.error);
          showNotification(`Failed to start recording: ${message.error}`, 'error');
          setIsRecording(false);
        }
        break;
        
      case 'recordingStopped':
        if (message.success) {
          console.log(`Recording completed: ${message.frameCount} frames`);
          
          // Create and download the video file
          const mimeType = getMimeType();
          const extension = getFileExtension();
          
          const blob = new Blob([message.videoData], { type: mimeType });
          const url = URL.createObjectURL(blob);
          const link = document.createElement('a');
          link.href = url;
          link.download = `recording-${new Date().toISOString().slice(0,19).replace(/:/g,'-')}.${extension}`;
          document.body.appendChild(link);
          link.click();
          document.body.removeChild(link);
          URL.revokeObjectURL(url);
          
          showNotification(`Recording saved: ${message.frameCount} frames`, 'success');
        } else {
          console.error('Recording failed:', message.error);
          showNotification(`Recording failed: ${message.error}`, 'error');
        }
        
        // Clean up
        if (statusCheckIntervalRef.current) {
          clearInterval(statusCheckIntervalRef.current);
          statusCheckIntervalRef.current = null;
        }
        
        setIsRecording(false);
        setIsProcessing(false);
        
        if (onStatusChange) {
          onStatusChange({ isRecording: false });
        }
        break;
        
      case 'recordingProgress':
        setFrameCount(message.frameCount);
        break;
        
      case 'recorderState':
        // Update UI based on state
        setFrameCount(message.frameCount);
        
        // Check for unexpected state changes
        if (isRecording && message.state !== 'recording') {
          console.warn(`Unexpected recording state: ${message.state}`);
          
          if (message.state === 'error') {
            stopRecording();
            showNotification('Recording error detected', 'error');
          }
        }
        break;
        
      case 'error':
        console.error('Worker error:', message.error);
        showNotification(`Error: ${message.error}`, 'error');
        
        // Attempt to recover
        if (isRecording) {
          stopRecording();
        }
        break;
        
      default:
        console.log('Unknown message from worker:', message);
    }
  };

  // Initialize the web worker with the WebAssembly module
  const initializeWorker = () => {
    if (!workerRef.current) return;
    
    // In a real implementation, this would use the module factory from the build
    workerRef.current.postMessage({
      type: 'init',
      // The actual module is already loaded in the main thread
      // and accessible via window.Module
    });
  };

  // Poll recording status to keep UI in sync
  const pollRecordingStatus = () => {
    if (workerRef.current && isRecording) {
      workerRef.current.postMessage({ type: 'getState' });
    }
  };

  // Start recording
  const startRecording = () => {
    if (!workerRef.current) {
      showNotification('Recording system not ready', 'error');
      return;
    }
    
    // Find the canvas
    const canvas = document.querySelector('canvas');
    if (!canvas) {
      showNotification('Canvas not found', 'error');
      return;
    }
    
    canvasRef.current = canvas;
    frameSequenceRef.current = 0;
    
    // Get dimensions from canvas
    const width = canvas.width;
    const height = canvas.height;
    
    // Ensure dimensions are even (required by most video codecs)
    const adjustedWidth = width % 2 === 0 ? width : width - 1;
    const adjustedHeight = height % 2 === 0 ? height : height - 1;
    
    // Setup recording options
    const options = {
      width: adjustedWidth,
      height: adjustedHeight,
      fps: 30,
      bitrate: 2000000,  // 2 Mbps
      codec: 'libx264',
      format: 'mp4',
      preset: 'medium'
    };
    
    // Send start recording message to worker
    workerRef.current.postMessage({
      type: 'startRecording',
      options
    });
    
    // Update UI
    showNotification('Starting recording...', 'info');
  };

  // Stop recording
  const stopRecording = () => {
    if (!workerRef.current || !isRecording) return;
    
    setIsProcessing(true);
    showNotification('Processing video...', 'info');
    
    // Stop the animation frame that's capturing frames
    if (animationFrameRef.current) {
      cancelAnimationFrame(animationFrameRef.current);
      animationFrameRef.current = null;
    }
    
    // Send stop recording message to worker
    workerRef.current.postMessage({ type: 'stopRecording' });
  };

  // Capture frames continuously during recording
  const captureFrames = () => {
    if (!isRecording || !canvasRef.current) return;
    
    // Get the canvas
    const canvas = canvasRef.current;
    
    // Capture the current frame
    const imageData = canvas.getContext('2d').getImageData(
      0, 0, canvas.width, canvas.height
    ).data;
    
    // Send frame to worker
    if (workerRef.current) {
      workerRef.current.postMessage({
        type: 'addFrame',
        imageData,
        width: canvas.width,
        height: canvas.height,
        sequence: frameSequenceRef.current++
      }, [imageData.buffer]);
    }
    
    // Schedule next frame capture
    animationFrameRef.current = requestAnimationFrame(captureFrames);
  };

  // Toggle recording state
  const handleToggleRecording = () => {
    if (isProcessing) return; // Prevent action while processing
    
    if (isRecording) {
      stopRecording();
    } else {
      startRecording();
    }
  };

  // Helper to determine the MIME type for video
  const getMimeType = () => {
    // Default to MP4
    return 'video/mp4';
  };

  // Helper to determine file extension
  const getFileExtension = () => {
    // Default to MP4
    return 'mp4';
  };

  // Show notification
  const showNotification = (message, severity = 'info') => {
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

  // Format time display (MM:SS)
  const formatTime = (seconds) => {
    const mins = Math.floor(seconds / 60).toString().padStart(2, '0');
    const secs = (seconds % 60).toString().padStart(2, '0');
    return `${mins}:${secs}`;
  };

  return (
    <Box sx={{ mb: 2 }}>
      <Paper sx={{ p: 2, display: 'flex', alignItems: 'center', gap: 2 }}>
        <Tooltip title={isRecording ? 'Stop Recording' : 'Start Recording'}>
          <IconButton
            color={isRecording ? 'error' : 'primary'}
            onClick={handleToggleRecording}
            disabled={isProcessing}
            sx={{ p: 1 }}
          >
            {isProcessing ? (
              <CircularProgress size={24} />
            ) : isRecording ? (
              <VideoOff />
            ) : (
              <Video />
            )}
          </IconButton>
        </Tooltip>
        
        <Box sx={{ flexGrow: 1 }}>
          {isRecording ? (
            <Typography variant="body2" color="error">
              Recording: {formatTime(elapsedTime)} ({frameCount} frames)
            </Typography>
          ) : (
            <Typography variant="body2" color="textSecondary">
              Click to start recording
            </Typography>
          )}
        </Box>
      </Paper>
      
      <Snackbar
        open={notification.open}
        autoHideDuration={4000}
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
    </Box>
  );
};

export default VideoRecorder;