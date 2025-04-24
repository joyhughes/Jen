import React, {useEffect, useRef, useState} from 'react';
import {Alert, Box, Divider, IconButton, Paper, Snackbar, Tooltip, useMediaQuery, useTheme} from '@mui/material';
import {Camera, Pause, Play, RotateCcw, Save, SkipForward, Video, VideoOff} from 'lucide-react';

function MediaController({ isOverlay = false }) {
    const theme = useTheme();
    const isMobile = useMediaQuery(theme.breakpoints.down('sm'));
    const isTablet = useMediaQuery(theme.breakpoints.down('md'));
    const [isRunning, setIsRunning] = useState(true);
    const [isRecording, setIsRecording] = useState(false);
    const [notification, setNotification] = useState({ open: false, message: '', severity: 'success' });
    const recordingInterval = useRef(null);
    const recordingFrames = useRef([]);

    const isIOS = useRef(/iPad|iPhone|iPod/.test(navigator.userAgent)) || (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
    const isAndroid = useRef(/Android/.test(navigator.userAgent));

    // Cleanup recording interval on unmount
    useEffect(() => {
        return () => {
            if (recordingInterval.current) {
                clearInterval(recordingInterval.current);
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
                showNotification("Cannot not found", "error");
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
                    })
                } catch (error) {
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
    }

    const handleToggleRecording = () => {
        if (isRecording) {
            stopRecording();
        } else {
            startRecording();
        }
    };

    const startRecording = () => {
        if (!window.module) return;

        // Clear any previous recording data
        recordingFrames.current = [];

        // Set up interval to capture frames
        recordingInterval.current = setInterval(() => {
            const canvas = document.querySelector('canvas');
            if (canvas) {
                recordingFrames.current.push(canvas.toDataURL('image/png'));
            }
        }, 100); // Capture 10 frames per second

        setIsRecording(true);
        showNotification('Recording started', 'info');
    };

    const stopRecording = async () => {
        if (recordingInterval.current) {
            clearInterval(recordingInterval.current);
            recordingInterval.current = null;
        }

        if (recordingFrames.current.length === 0) {
            setIsRecording(false);
            showNotification('No frames recorded', 'warning');
            return;
        }

        try {
            // Convert frames to video using canvas
            const canvas = document.createElement('canvas');
            const ctx = canvas.getContext('2d');
            const firstImg = new Image();

            // Set canvas dimensions based on first frame
            await new Promise(resolve => {
                firstImg.onload = resolve;
                firstImg.src = recordingFrames.current[0];
            });

            canvas.width = firstImg.width;
            canvas.height = firstImg.height;

            // Create video stream
            const stream = canvas.captureStream(30);
            const recorder = new MediaRecorder(stream, { mimeType: 'video/webm' });

            const chunks = [];
            recorder.ondataavailable = e => chunks.push(e.data);
            recorder.onstop = () => {
                const blob = new Blob(chunks, { type: 'video/webm' });
                const url = URL.createObjectURL(blob);
                const link = document.createElement('a');
                link.download = `jen-recording-${new Date().toISOString().slice(0, 19).replace(/:/g, '-')}.webm`;
                link.href = url;
                link.click();

                showNotification('Video saved', 'success');
            };

            // Start recording
            recorder.start();

            // Draw each frame to the canvas
            for (let i = 0; i < recordingFrames.current.length; i++) {
                const img = new Image();
                await new Promise(resolve => {
                    img.onload = resolve;
                    img.src = recordingFrames.current[i];
                });
                ctx.drawImage(img, 0, 0);

                // Add a small delay to ensure frames are captured
                await new Promise(resolve => setTimeout(resolve, 30));
            }

            // Stop recording
            recorder.stop();

        } catch (error) {
            console.error('Error creating video:', error);
            showNotification('Failed to create video', 'error');
        }

        setIsRecording(false);
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
        p: 1,
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

                <Tooltip title={isRecording ? "Stop Recording" : "Start Recording"} arrow>
                    <IconButton
                        onClick={handleToggleRecording}
                        sx={isRecording ? {
                            ...activeButtonStyles,
                            color: theme.palette.error.main,
                            '&:hover': {
                                bgcolor: theme.palette.error.main + '1A',
                            }
                        } : buttonStyles}
                        size="medium"
                    >
                        {isRecording ? <VideoOff size={iconSize} /> : <Video size={iconSize} />}
                    </IconButton>
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