import React, { useRef, useEffect, useState, useCallback } from 'react';
import { Box, CircularProgress, Typography, Tooltip, Fade } from '@mui/material';
import { styled, useTheme } from '@mui/material/styles';
import { AlertTriangle, Check } from 'lucide-react';

export const THUMB_SIZE = 64; // Slightly smaller for better fit

// Styled container for each thumbnail
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

// Canvas wrapper with consistent styling
const CanvasWrapper = styled(Box)({
    width: '100%',
    height: '100%',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    '& canvas': {
        display: 'block',
        width: '100%',
        height: '100%',
        objectFit: 'cover',
    },
});

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

// Functional component for the thumbnail item
function ThumbnailItem({ imageName, isSelected, onClick }) {
    const theme = useTheme();
    const canvasRef = useRef(null);
    const [status, setStatus] = useState('loading'); // 'loading', 'loaded', 'error'
    const [tooltipOpen, setTooltipOpen] = useState(false);

    // Function to draw the thumbnail
    const drawThumbnail = useCallback(async () => {
        if (!window.module || !canvasRef.current) return;

        setStatus('loading');
        let pixelDataVal = null;

        try {
            pixelDataVal = window.module.get_thumbnail(imageName, THUMB_SIZE, THUMB_SIZE);
            const bufferLength = pixelDataVal?.byteLength;

            if (!bufferLength || bufferLength !== THUMB_SIZE * THUMB_SIZE * 4) {
                console.error(`Thumbnail Error (${imageName}): Invalid pixel data length: ${bufferLength}`);
                setStatus('error');
                return;
            }

            // Get clamped array view
            const pixelData = new Uint8ClampedArray(pixelDataVal.buffer, pixelDataVal.byteOffset, bufferLength);

            // Swizzle BGRA to RGBA (assuming C++ format is 0xAARRGGBB)
            const rgbaPixelData = new Uint8ClampedArray(bufferLength);
            for (let i = 0; i < bufferLength; i += 4) {
                const r_val = pixelData[i];
                const g_val = pixelData[i + 1];
                const b_val = pixelData[i + 2];

                // Write in RGBA order for ImageData
                rgbaPixelData[i] = r_val;
                rgbaPixelData[i + 1] = g_val;
                rgbaPixelData[i + 2] = b_val;
                rgbaPixelData[i + 3] = 255;
            }

            const imageData = new ImageData(rgbaPixelData, THUMB_SIZE, THUMB_SIZE);
            const ctx = canvasRef.current.getContext('2d', { alpha: false });

            if (!ctx) {
                throw new Error("Could not get canvas context");
            }

            // Create and render the bitmap
            const imageBitmap = await createImageBitmap(imageData);
            ctx.clearRect(0, 0, THUMB_SIZE, THUMB_SIZE);
            ctx.drawImage(imageBitmap, 0, 0, THUMB_SIZE, THUMB_SIZE);
            imageBitmap.close();

            setStatus('loaded');
        } catch (err) {
            console.error(`Thumbnail Error (${imageName}):`, err);
            setStatus('error');

            // Draw error state
            if (canvasRef.current) {
                const ctx = canvasRef.current.getContext('2d');
                if (ctx) {
                    ctx.fillStyle = theme.palette.error.dark;
                    ctx.fillRect(0, 0, THUMB_SIZE, THUMB_SIZE);
                }
            }
        }
    }, [imageName, theme.palette.error.dark]);

    // Initialize thumbnail on mount or imageName change
    useEffect(() => {
        // Small delay to avoid blocking the UI thread
        const timer = setTimeout(() => {
            drawThumbnail();
        }, 50);
        return () => clearTimeout(timer);
    }, [drawThumbnail]);

    // Get filename for display (strip path if present)
    const displayName = imageName.split('/').pop();

    return (
        <Tooltip
            title={displayName}
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
                onClick={() => onClick(imageName)}
                onMouseEnter={() => setTooltipOpen(true)}
                onMouseLeave={() => setTooltipOpen(false)}
            >
                <CanvasWrapper>
                    <canvas
                        ref={canvasRef}
                        width={THUMB_SIZE}
                        height={THUMB_SIZE}
                    />
                </CanvasWrapper>

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

                {/* Selection indicator */}
                {isSelected && status === 'loaded' && (
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