import React, { useState } from 'react';
import { Box, CircularProgress, Typography, Tooltip, Fade } from '@mui/material';
import { styled } from '@mui/material/styles';
import { AlertTriangle, Check, Info } from 'lucide-react';
import ThumbnailCanvas from "./ThumbnailCanvas.jsx";

export const THUMB_SIZE = 64; // Thumbnail size

const ThumbnailContainer = styled(Box)(({ theme, selected }) => ({
    position: 'relative',
    width: THUMB_SIZE,
    height: THUMB_SIZE,
    borderRadius: theme.shape.borderRadius,
    overflow: 'hidden',
    cursor: 'pointer',
    transition: 'all 0.2s ease-in-out',
    boxShadow: selected
        ? `0 0 0 2px ${theme.palette.primary.main}, ${theme.shadows[2]}`
        : theme.shadows[1],
    '&:hover': {
        transform: 'translateY(-2px)',
        boxShadow: selected
            ? `0 0 0 2px ${theme.palette.primary.main}, ${theme.shadows[4]}`
            : theme.shadows[3]
    },
    '&:active': {
        transform: 'scale(0.98)',
    }
}));

// Overlay for loading/error states
const StatusOverlay = styled(Box)(({ theme, status }) => ({
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: status === 'error'
        ? 'rgba(211, 47, 47, 0.75)'
        : status === 'debug'
            ? 'rgba(25, 118, 210, 0.75)'
            : 'rgba(0, 0, 0, 0.5)',
    backdropFilter: 'blur(2px)',
    color: 'white',
    textAlign: 'center',
    zIndex: 2,
}));

// Selection indicator
const SelectionIndicator = styled(Box)(({ theme }) => ({
    position: 'absolute',
    top: 6,
    right: 6,
    width: 18,
    height: 18,
    borderRadius: '50%',
    backgroundColor: theme.palette.primary.main,
    color: theme.palette.common.white,
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    zIndex: 3,
    boxShadow: theme.shadows[2],
}));

function ThumbnailItem({ imageName, isSelected, onClick }) {
    const [tooltipOpen, setTooltipOpen] = useState(false);



    const displayName = imageName.split('/').pop();

    const handleClick = () => {
        if (onClick) onClick(imageName);
    };

    return (
        <Tooltip
            title={
                <Box>
                    <Typography variant="body2">{displayName}</Typography>
                    {status === 'error' && (
                        <Typography variant="caption" color="error">
                            Error: {debugMessage}
                        </Typography>
                    )}
                    {status === 'debug' && (
                        <Typography variant="caption">
                            Debug: {debugMessage}
                        </Typography>
                    )}
                </Box>
            }
            placement="top"
            arrow
            open={tooltipOpen}
            onOpen={() => setTooltipOpen(true)}
            onClose={() => setTooltipOpen(false)}
            enterDelay={700}
            leaveDelay={100}
        >
            <ThumbnailContainer
                selected={isSelected}
                onClick={handleClick}
                onMouseEnter={() => setTooltipOpen(true)}
                onMouseLeave={() => setTooltipOpen(false)}
                onTouchStart={() => setTooltipOpen(true)} 
                onTouchEnd={() => setTooltipOpen(false)}  
            >
                <ThumbnailCanvas
                    imageName={imageName}
                    width={THUMB_SIZE}
                    height={THUMB_SIZE}

                />

                {/* Loading overlay */}
                {status === 'loading' && (
                    <Fade in={true}>
                        <StatusOverlay status="loading">
                            <CircularProgress size={24} color="inherit" />
                        </StatusOverlay>
                    </Fade>
                )}

                {/* Error overlay */}
                {status === 'error' && (
                    <StatusOverlay status="error">
                        <AlertTriangle size={18} />
                        <Typography variant="caption" sx={{ mt: 0.5, fontSize: '0.65rem' }}>
                            Error
                        </Typography>
                    </StatusOverlay>
                )}

                {/* Debug overlay */}
                {status === 'debug' && (
                    <StatusOverlay status="debug">
                        <Info size={18} />
                        <Typography variant="caption" sx={{ mt: 0.5, fontSize: '0.65rem' }}>
                            Debug
                        </Typography>
                    </StatusOverlay>
                )}

                {/* Selection indicator */}
                {isSelected && (
                    <Fade in={true}>
                        <SelectionIndicator>
                            <Check size={12} />
                        </SelectionIndicator>
                    </Fade>
                )}
            </ThumbnailContainer>
        </Tooltip>
    );
}

export default ThumbnailItem;