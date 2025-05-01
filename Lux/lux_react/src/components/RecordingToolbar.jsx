import React, { useState, useEffect } from 'react';
import {
    Box,
    Paper,
    Typography,
    IconButton,
    LinearProgress,
    Badge,
    Tooltip,
    useTheme,
    useMediaQuery
} from '@mui/material';
import {
    VideoOff,
    Timer,
    Circle,
    BarChart,
    Settings,
    Download
} from 'lucide-react';
import VideoRecordingSettings from './VideoRecordingSettings';

const RecordingToolbar = ({
                              isRecording,
                              onStopRecording,
                              recordingStats,
                              onSettingsChange
                          }) => {
    const theme = useTheme();
    const isMobile = useMediaQuery(theme.breakpoints.down('sm'));
    const isTablet = useMediaQuery(theme.breakpoints.down('md'));

    const [elapsedTime, setElapsedTime] = useState(0);
    const [settingsOpen, setSettingsOpen] = useState(false);
    const [recordingSettings, setRecordingSettings] = useState({
        fps: 30,
        bitrate: 2000000,
        codec: "libx264",
        format: "mp4",
        preset: "medium"
    });

    // Timer effect for recording duration
    useEffect(() => {
        let timer;
        if (isRecording) {
            setElapsedTime(0);
            timer = setInterval(() => {
                setElapsedTime(prev => prev + 1);
            }, 1000);
        }

        return () => {
            if (timer) clearInterval(timer);
        };
    }, [isRecording]);

    // Format seconds into MM:SS
    const formatTime = (seconds) => {
        const mins = Math.floor(seconds / 60);
        const secs = seconds % 60;
        return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
    };

    // Calculate file size estimate based on bitrate and elapsed time
    // This is a rough estimate: (bitrate * time in seconds) / 8 = bytes
    const calculateEstimatedSize = () => {
        const bytes = (recordingSettings.bitrate * elapsedTime) / 8;
        if (bytes < 1024 * 1024) {
            return `${(bytes / 1024).toFixed(2)} KB`;
        } else if (bytes < 1024 * 1024 * 1024) {
            return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
        } else {
            return `${(bytes / (1024 * 1024 * 1024)).toFixed(2)} GB`;
        }
    };

    const handleSettingsOpen = () => {
        setSettingsOpen(true);
    };

    const handleSettingsClose = () => {
        setSettingsOpen(false);
    };

    const handleSettingsApply = (newSettings) => {
        setRecordingSettings(newSettings);
        if (onSettingsChange) {
            onSettingsChange(newSettings);
        }
    };

    // Responsive design adaptations
    const containerStyles = {
        position: 'fixed',
        left: isMobile ? '50%' : theme.spacing(2),
        bottom: theme.spacing(2),
        transform: isMobile ? 'translateX(-50%)' : 'none',
        zIndex: 1000,
        borderRadius: theme.shape.borderRadius * 2,
        backgroundColor: 'rgba(0, 0, 0, 0.8)',
        backdropFilter: 'blur(10px)',
        display: 'flex',
        alignItems: 'center',
        padding: theme.spacing(1, 2),
        boxShadow: theme.shadows[10],
        color: 'white',
        minWidth: isMobile ? '90%' : 'auto',
        maxWidth: isMobile ? '90%' : '450px',
        transition: 'all 0.3s ease'
    };

    const recordingIndicatorPulse = {
        animation: 'pulse 1.5s infinite ease-in-out',
        '@keyframes pulse': {
            '0%': { opacity: 1 },
            '50%': { opacity: 0.5 },
            '100%': { opacity: 1 }
        }
    };

    return (
        <>
            {isRecording && (
                <Paper elevation={4} sx={containerStyles}>
                    {/* Recording Indicator */}
                    <Box sx={{
                        display: 'flex',
                        alignItems: 'center',
                        mr: isMobile ? 1 : 2
                    }}>
                        <Circle
                            size={12}
                            fill="red"
                            color="red"
                            style={recordingIndicatorPulse}
                        />
                        <Typography
                            variant="body2"
                            sx={{
                                ml: 1,
                                fontWeight: 'bold',
                                color: theme.palette.error.light,
                                display: isMobile ? 'none' : 'block'
                            }}
                        >
                            REC
                        </Typography>
                    </Box>

                    {/* Timer */}
                    <Tooltip title="Recording duration">
                        <Box sx={{
                            display: 'flex',
                            alignItems: 'center',
                            mr: 2
                        }}>
                            <Timer size={16} />
                            <Typography variant="body2" sx={{ ml: 1, minWidth: '45px' }}>
                                {formatTime(elapsedTime)}
                            </Typography>
                        </Box>
                    </Tooltip>

                    {/* Frames Counter */}
                    <Tooltip title="Frames recorded">
                        <Box sx={{
                            display: 'flex',
                            alignItems: 'center',
                            mr: 2
                        }}>
                            <BarChart size={16} />
                            <Typography variant="body2" sx={{ ml: 1 }}>
                                {recordingStats?.frames || 0}
                            </Typography>
                        </Box>
                    </Tooltip>

                    {/* Estimated Size (not shown on mobile) */}
                    {!isMobile && (
                        <Tooltip title="Estimated file size">
                            <Box sx={{
                                display: 'flex',
                                alignItems: 'center',
                                mr: 2
                            }}>
                                <Download size={16} />
                                <Typography variant="body2" sx={{ ml: 1 }}>
                                    {calculateEstimatedSize()}
                                </Typography>
                            </Box>
                        </Tooltip>
                    )}

                    {/* Spacer */}
                    <Box sx={{ flexGrow