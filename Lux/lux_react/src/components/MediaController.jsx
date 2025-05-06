import React, {useEffect, useRef, useState} from 'react';
import {Alert, Box, Divider, IconButton, Paper, Snackbar, Tooltip, useMediaQuery, useTheme} from '@mui/material';
import {Camera, Pause, Play, RotateCcw, Save, SkipForward, Video, VideoOff} from 'lucide-react';

function MediaController({ isOverlay = false }) {
    const theme = useTheme();
    const isMobile = useMediaQuery(theme.breakpoints.down('sm'));
    const isTablet = useMediaQuery(theme.breakpoints.down('md'));
    const [isRunning, setIsRunning] = useState(true);
    const [isRecording, setIsRecording] = useState(false);
    const [recordingState, setRecordingState] = useState('idle');
    const [recordedFrames, setRecordedFrames] = useState(0);
    const [notification, setNotification] = useState({ open: false, message: '', severity: 'success' });

    const recordingCheckInterval = useRef(null);
    const isIOS = useRef(/iPad|iPhone|iPod/.test(navigator.userAgent)) || (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
    const isAndroid = useRef(/Android/.test(navigator.userAgent));

    // Check if WASM module is ready and has recording functions
    const hasRecordingFeature = () => {
        return window.module &&
            typeof window.module.start_recording_adaptive === 'function' &&
            typeof window.module.stop_recording === 'function';
    };

    // Poll recording status from C++ when recording is active
    useEffect(() => {
        if (isRecording && hasRecordingFeature()) {
            // Set up interval to check recording status
            recordingCheckInterval.current = setInterval(() => {
                if (window.module) {
                    // Update recording state from C++
                    const state = window.module.get_recording_state();
                    setRecordingState(state);

                    // Update frame count
                    const frames = window.module.get_recorded_frame_count();
                    setRecordedFrames(frames);

                    // If recording ended or errored out, update UI
                    if (state === 'error') {
                        const errorMsg = window.module.get_recording_error();
                        showNotification(`Recording error: ${errorMsg}`, 'error');
                        setIsRecording(false);
                        clearInterval(recordingCheckInterval.current);
                    } else if (state === 'idle' && isRecording) {
                        // Recording stopped but we didn't trigger it
                        showNotification('Recording stopped unexpectedly', 'warning');
                        setIsRecording(false);
                        clearInterval(recordingCheckInterval.current);
                    }
                }
            }, 500); // Check every half second

            return () => {
                if (recordingCheckInterval.current) {
                    clearInterval(recordingCheckInterval.current);
                    recordingCheckInterval.current = null;
                }
            };
        }
    }, [isRecording]);

    // Cleanup recording interval on unmount
    useEffect(() => {
        return () => {
            if (recordingCheckInterval.current) {
                clearInterval(recordingCheckInterval.current);
            }
            // Make sure to stop any active recording when component unmounts
            if (hasRecordingFeature() && window.module.is_recording()) {
                window.module.stop_recording();
            }
        };
    }, []);

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

            const isMobileDevice = isIOS.current || isAndroid.current;
            if (isMobileDevice && navigator.share) {
                try {
                    // convert raw data to blob for sharing
                    const blob = await (await fetch(dataUrl)).blob();
                    const file = new File([blob], filename, { type: 'image/png' });
                    await navigator.share({
                        title: "Jen Snapshot",
                        files: [file]
                    });
                } catch (shareError) {
                    console.warn('Share failed, falling back to download:', shareError);
                    downloadImage(dataUrl, filename);
                }
            } else {
                downloadImage(dataUrl, filename);
            }
        } catch (error) {
            console.error('Error taking snapshot:', error);
            showNotification('Failed to save snapshot', 'error');
        }
    };

    const downloadImage = (dataUrl, fileName) => {
        const link = document.createElement('a');
        link.download = fileName;
        link.href = dataUrl;
        link.click();
        showNotification("Snapshot saved", "success");
    };

    const handleToggleRecording = () => {
        if (!hasRecordingFeature()) {
            showNotification('Recording feature not available', 'error');
            return;
        }

        if (isRecording) {
            stopRecording();
        } else {
            startRecording();
        }
    };

    const startRecording = () => {
        if (!hasRecordingFeature()) return;

        try {
            // Get current canvas dimensions
            const canvas = document.querySelector('canvas');
            if (!canvas) {
                showNotification("Canvas not found for recording", "error");
                return;
            }

            // Print WASM module methods to check if they're properly exposed
            console.log("Available module methods:", Object.keys(window.module).filter(key => typeof window.module[key] === 'function'));

            // Log recording functions specifically
            console.log("Recording functions:", {
                start_recording: typeof window.module.start_recording,
                stop_recording: typeof window.module.stop_recording,
                is_recording: typeof window.module.is_recording,
                get_recording_state: typeof window.module.get_recording_state
            });

            const width = 512;
            const height = 512;
            const fps = 30;
            const bitrate = 2000000;
            const codec = "libx264";
            const format = "mp4";
            const preset = "medium";

            console.log("Starting recording with params:", {
                width, height, fps, bitrate, codec, format, preset
            });

            try {
                const success = window.module.start_recording_adaptive(fps, bitrate, codec, format, preset);
                console.log("Recording start result:", success);

                if (success) {
                    setIsRecording(true);
                    setRecordingState('recording');
                    setRecordedFrames(0);
                    showNotification('Recording started', 'info');
                } else {
                    const errorMsg = window.module.get_recording_error ? window.module.get_recording_error() : "Unknown error";
                    showNotification(`Failed to start recording: ${errorMsg}`, 'error');
                }
            } catch (wasmError) {
                console.error("WASM specific error:", wasmError);
                showNotification(`WASM error: ${wasmError.message || wasmError}`, 'error');
            }
        } catch (error) {
            console.error('Error in startRecording function:', error);
            showNotification(`Recording error: ${error.message || error}`, 'error');
        }
    };

    const stopRecording = async () => {
        if (!hasRecordingFeature() || !isRecording) return;

        try {
            setRecordingState('encoding');
            showNotification('Processing video...', 'info');

            // Stop recording via WASM
            const success = window.module.stop_recording();
            console.log("Stop recording result:", success);

            if (success) {
                // Wait for encoding to complete (up to 5 seconds)
                let encodingCompleted = false;
                for (let i = 0; i < 20; i++) {
                    await new Promise(resolve => setTimeout(resolve, 250));
                    const state = window.module.get_recording_state();
                    console.log(`Encoding check ${i+1}/20: State = ${state}`);
                    if (state === 'idle') {
                        encodingCompleted = true;
                        break;
                    }
                }

                if (encodingCompleted) {
                    try {
                        // Save the video file
                        const videoData = window.module.get_recording_data();
                        console.log("Video data received:", videoData ? "Yes (with data)" : "No (null)");

                        if (videoData) {
                            // Determine MIME type based on current format
                            let mimeType = 'video/webm';
                            let extension = 'webm';

                            // If we can get the options, use the format from there
                            if (window.module.get_options && typeof window.module.get_options === 'function') {
                                const options = window.module.get_options();
                                if (options && options.format) {
                                    if (options.format === 'mpeg') {
                                        mimeType = 'video/mpeg';
                                        extension = 'mpg';
                                    } else if (options.format === 'mp4') {
                                        mimeType = 'video/mp4';
                                        extension = 'mp4';
                                    } else {
                                        mimeType = `video/${options.format}`;
                                        extension = options.format;
                                    }
                                }
                            }

                            console.log(`Creating blob with MIME type: ${mimeType}`);
                            const blob = new Blob([videoData], { type: mimeType });
                            const url = URL.createObjectURL(blob);
                            const link = document.createElement('a');
                            link.download = `jen-recording-${new Date().toISOString().slice(0, 19).replace(/:/g, '-')}.${extension}`;
                            link.href = url;
                            link.click();
                            URL.revokeObjectURL(url);
                            showNotification(`Video saved (${recordedFrames} frames)`, 'success');
                        } else {
                            console.error("No video data returned from get_recording_data()");
                            showNotification('No video data available', 'error');
                        }
                    } catch (dataError) {
                        console.error("Error processing video data:", dataError);
                        showNotification(`Error processing video: ${dataError.message || dataError}`, 'error');
                    }
                } else {
                    console.error("Video encoding timed out - state never reached 'idle'");
                    showNotification('Video encoding timed out', 'error');
                }
            } else {
                const errorMsg = window.module.get_recording_error ? window.module.get_recording_error() : "Unknown error";
                console.error("Failed to stop recording:", errorMsg);
                showNotification(`Failed to stop recording: ${errorMsg}`, 'error');
            }
        } catch (error) {
            console.error('Error stopping recording:', error);
            showNotification('Failed to process video', 'error');
        } finally {
            setIsRecording(false);
            setRecordingState('idle');
        }
    };

    const handleSaveScene = () => {
        try {
            if (window.module && typeof window.module.get_scene_json === 'function') {
                const sceneData = window.module.get_scene_json();
                const blob = new Blob([sceneData], { type: 'application/json' });
                const url = URL.createObjectURL(blob);
                const link = document.createElement('a');
                link.download = `jen-scene-${new Date().toISOString().slice(0, 19).replace(/:/g, '-')}.json`;
                link.href = url;
                link.click();
                URL.revokeObjectURL(url);
                showNotification('Scene saved', 'success');
            } else {
                throw new Error('Scene saving not supported');
            }
        } catch (error) {
            console.error('Error saving scene:', error);
            showNotification('Failed to save scene', 'error');
        }
    };

    const showNotification = (message, severity = 'success') => {
        setNotification({
            open: true,
            message,
            severity
        });
    };

    const handleCloseNotification = () => {
        setNotification(prev => ({ ...prev, open: false }));
    };

    // Determine styles based on whether it's an overlay or not
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
        justifyContent: 'center', // Center controls
        p: 0.5,
        borderRadius: 2,
        bgcolor: theme.palette.background.paper,
        border: `1px solid ${theme.palette.divider}`,
        width: '100%',
        maxWidth: '100%',
        overflowX: 'auto',
        boxShadow: theme.shadows[3]
    };

    // Adjusted button sizes for all controls to fit in one row
    // Smaller on mobile to fit everything
    const buttonSize = isMobile ? 36 : (isOverlay ? 40 : 44);

    // Icons size based on button size
    const iconSize = buttonSize * 0.5;

    // Reduced margins for mobile to fit all controls
    const buttonMargin = isMobile ? 0.25 : 0.5;

    // Button styling with adjusted margins for mobile
    const buttonStyles = {
        width: buttonSize,
        height: buttonSize,
        m: buttonMargin,
        color: isOverlay ? 'white' : theme.palette.text.secondary,
        '&:hover': {
            bgcolor: isOverlay ? 'rgba(255, 255, 255, 0.1)' : theme.palette.action.hover,
        },
    };

    // Active button styling (for play/pause, record)
    const activeButtonStyles = {
        ...buttonStyles,
        color: theme.palette.primary.main,
        '&:hover': {
            bgcolor: theme.palette.primary.main + '1A', // 10% opacity
        },
    };

    // Define recording button styles based on state
    const getRecordingButtonStyles = () => {
        if (!isRecording) return buttonStyles;

        // For recording state
        if (recordingState === 'encoding') {
            // Yellow for encoding
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

    // Get recording button tooltip based on state
    const getRecordingTooltip = () => {
        if (!isRecording) return "Start Recording";
        if (recordingState === 'encoding') return "Encoding... Please wait";
        return `Stop Recording (${recordedFrames} frames)`;
    };

    return (
        <Paper elevation={isOverlay ? 0 : 3} sx={containerStyles}>
            <Box sx={{
                display: 'flex',
                flexWrap: 'nowrap',  // Never wrap to keep all in one row
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

                {/* Use smaller divider on mobile */}
                <Divider orientation="vertical" flexItem sx={{
                    mx: isMobile ? 0.25 : 0.5,
                    my: 0.5,
                    height: isMobile ? '70%' : '80%'
                }} />

                {/* Secondary Controls - Always visible */}
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
                            disabled={recordingState === 'encoding'}
                        >
                            {isRecording ? <VideoOff size={iconSize} /> : <Video size={iconSize} />}
                        </IconButton>
                    </span>
                </Tooltip>

                <Tooltip title="Save Scene" arrow>
                    <IconButton
                        onClick={handleSaveScene}
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