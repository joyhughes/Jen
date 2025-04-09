import React, { useRef, useEffect, useState, useCallback } from 'react';
import { Box, CircularProgress, Typography, Tooltip, Fade } from '@mui/material';
import { styled, useTheme } from '@mui/material/styles';
import { AlertTriangle, Check } from 'lucide-react';

// Consistent thumbnail size in pixels
export const THUMB_SIZE = 80;

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
        ? `0 0 0 2px ${theme.palette.primary.main}, 0 3px 6px rgba(0,0,0,0.1)`
        : '0 1px 3px rgba(0,0,0,0.08)',
    '&:hover': {
        transform: 'translateY(-2px)',
        boxShadow: selected
            ? `0 0 0 2px ${theme.palette.primary.main}, 0 6px 10px rgba(0,0,0,0.15)`
            : '0 4px 8px rgba(0,0,0,0.12)'
    },
    '&:active': {
        transform: 'translateY(0)',
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
        maxWidth: '100%',
        maxHeight: '100%',
        objectFit: 'contain',
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
        ? 'rgba(180, 0, 0, 0.75)'
        : 'rgba(0, 0, 0, 0.6)',
    color: 'white',
    textAlign: 'center',
    zIndex: 2,
}));

// Selection indicator
const SelectionIndicator = styled(Box)(({ theme }) => ({
    position: 'absolute',
    top: 4,
    right: 4,
    width: 16,
    height: 16,
    borderRadius: '50%',
    backgroundColor: theme.palette.primary.main,
    color: theme.palette.primary.contrastText,
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    zIndex: 3,
    boxShadow: '0 1px 2px rgba(0,0,0,0.2)',
}));

// Functional component for the thumbnail item
function JenThumbnailItem({ imageName, isSelected, onClick }) {
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
            const ctx = canvasRef.current.getContext('2d');

            if (!ctx) {
                throw new Error("Could not get canvas context");
            }

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
                    ctx.fillStyle = '#500';
                    ctx.fillRect(0, 0, THUMB_SIZE, THUMB_SIZE);
                }
            }
        }
    }, [imageName]);

    // Initialize thumbnail on mount or imageName change
    useEffect(() => {
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
                        <AlertTriangle size={20} />
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

export default JenThumbnailItem;