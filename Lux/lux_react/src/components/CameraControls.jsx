import React from 'react';
import { Box, IconButton, Fab, Tooltip, useTheme, useMediaQuery, CircularProgress, Chip } from '@mui/material';
import {
    Camera,
    CameraOff,
    FlipHorizontal,
    Settings,
    X,
    Zap,
    ZapOff,
    RotateCcw
} from 'lucide-react';

const CameraControls = ({
    isStreaming,
    isCapturing,
    onCapture,
    onClose,
    onToggleSettings,
    onSwitchCamera,
    showSwitchCamera,
    isSwitchingCamera = false,
    currentFacingMode = 'user',
    cameraInfo = {},
    livePreviewActive,
    onToggleLivePreview,
    moduleReady = true
}) => {
    const theme = useTheme();
    const isMobile = useMediaQuery(theme.breakpoints.down('sm'));

    // Button styles for mobile optimization
    const buttonStyles = {
        bgcolor: 'rgba(0,0,0,0.6)',
        color: 'white',
        '&:hover': {
            bgcolor: 'rgba(0,0,0,0.8)',
        },
        ...(isMobile && {
            width: 56,
            height: 56,
        })
    };

    const fabStyles = {
        bgcolor: theme.palette.primary.main,
        color: 'white',
        '&:hover': {
            bgcolor: theme.palette.primary.dark,
        },
        ...(isMobile && {
            width: 72,
            height: 72,
        })
    };

    return (
        <Box
            sx={{
                position: 'absolute',
                top: 0,
                left: 0,
                right: 0,
                bottom: 0,
                pointerEvents: 'none', // Allow clicks to pass through
                display: 'flex',
                flexDirection: 'column',
                justifyContent: 'space-between',
                p: 1
            }}
        >
            {/* Top controls */}
            <Box
                sx={{
                    display: 'flex',
                    justifyContent: 'space-between',
                    alignItems: 'flex-start'
                }}
            >
                {/* Close button */}
                <Tooltip title="Close Camera">
                    <IconButton
                        onClick={onClose}
                        sx={{ ...buttonStyles, pointerEvents: 'auto' }}
                    >
                        <X size={isMobile ? 24 : 20} />
                    </IconButton>
                </Tooltip>

                {/* Top right controls */}
                <Box sx={{ display: 'flex', gap: 1, flexDirection: 'column', alignItems: 'flex-end' }}>
                    {/* Camera status indicator */}
                    {isStreaming && (
                        <Chip
                            label={cameraInfo.isFrontCamera ? 'Front Camera' : 'Back Camera'}
                            size="small"
                            sx={{
                                bgcolor: 'rgba(0,0,0,0.7)',
                                color: 'white',
                                fontSize: '0.7rem',
                                height: 24,
                                pointerEvents: 'auto'
                            }}
                        />
                    )}

                    <Box sx={{ display: 'flex', gap: 1 }}>
                        {/* Live preview toggle */}
                        <Tooltip title={livePreviewActive ? "Disable Live Effects" : "Enable Live Effects"}>
                            <IconButton
                                onClick={onToggleLivePreview}
                                sx={{
                                    ...buttonStyles,
                                    pointerEvents: 'auto',
                                    color: livePreviewActive ? theme.palette.warning.main : 'white'
                                }}
                            >
                                {livePreviewActive ? <Zap size={isMobile ? 24 : 20} /> : <ZapOff size={isMobile ? 24 : 20} />}
                            </IconButton>
                        </Tooltip>

                        {/* Settings button */}
                        <Tooltip title="Camera Settings">
                            <IconButton
                                onClick={onToggleSettings}
                                sx={{ ...buttonStyles, pointerEvents: 'auto' }}
                            >
                                <Settings size={isMobile ? 24 : 20} />
                            </IconButton>
                        </Tooltip>
                    </Box>
                </Box>
            </Box>

            {/* Bottom controls */}
            <Box
                sx={{
                    display: 'flex',
                    justifyContent: 'center',
                    alignItems: 'center',
                    gap: 2,
                    pb: 2
                }}
            >
                {/* Switch camera button (if multiple cameras available) */}
                {showSwitchCamera && (
                    <Tooltip title={
                        isSwitchingCamera ? "Switching camera..." : 
                        `Switch to ${cameraInfo.isFrontCamera ? 'Back' : 'Front'} Camera`
                    }>
                        <IconButton
                            onClick={onSwitchCamera}
                            sx={{ 
                                ...buttonStyles, 
                                pointerEvents: 'auto',
                                opacity: isSwitchingCamera ? 0.6 : 1
                            }}
                            disabled={!isStreaming || isSwitchingCamera}
                        >
                            {isSwitchingCamera ? (
                                <CircularProgress size={isMobile ? 24 : 20} color="inherit" />
                            ) : (
                                <RotateCcw size={isMobile ? 24 : 20} />
                            )}
                        </IconButton>
                    </Tooltip>
                )}

                {/* Capture button */}
                <Tooltip title={
                    !moduleReady ? "Loading..." : 
                    isCapturing ? "Capturing..." : 
                    !isStreaming ? "Starting camera..." : 
                    "Take Photo"
                }>
                    <Fab
                        onClick={onCapture}
                        disabled={!isStreaming || isCapturing || !moduleReady}
                        sx={{
                            ...fabStyles,
                            pointerEvents: 'auto',
                            opacity: isCapturing ? 0.6 : 1,
                            transform: isCapturing ? 'scale(0.9)' : 'scale(1)',
                            transition: 'all 0.2s ease'
                        }}
                    >
                        <Camera size={isMobile ? 32 : 28} />
                    </Fab>
                </Tooltip>

                {/* Placeholder for symmetry if no switch camera */}
                {!showSwitchCamera && (
                    <Box sx={{ width: isMobile ? 56 : 48, height: isMobile ? 56 : 48 }} />
                )}
            </Box>
        </Box>
    );
};

export default CameraControls; 