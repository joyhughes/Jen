import React, { useRef, useEffect, useState, useCallback } from 'react';
import { Box, CircularProgress, Typography, Tooltip, Fade } from '@mui/material';
import { styled, useTheme } from '@mui/material/styles';
import { AlertTriangle, Check, Info } from 'lucide-react';

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

// Generate a color based on the image name for placeholders
const getPlaceholderColor = (imageName) => {
    let hash = 0;
    for (let i = 0; i < imageName.length; i++) {
        hash = imageName.charCodeAt(i) + ((hash << 5) - hash);
    }
    const c = (hash & 0x00FFFFFF)
        .toString(16)
        .toUpperCase()
        .padStart(6, '0');
    return `#${c}`;
};

// Functional component for the thumbnail item
function ThumbnailItem({ imageName, isSelected, onClick }) {
    const theme = useTheme();
    const canvasRef = useRef(null);
    const [status, setStatus] = useState('loading'); // 'loading', 'loaded', 'error', 'debug'
    const [tooltipOpen, setTooltipOpen] = useState(false);
    const [retryCount, setRetryCount] = useState(0);
    const [debugMessage, setDebugMessage] = useState('');

    // Draw placeholder while loading or if there's an error
    const drawPlaceholder = useCallback(() => {
        if (!canvasRef.current) return;

        const ctx = canvasRef.current.getContext('2d');
        if (!ctx) return;

        // Fill with a color based on the image name
        const color = getPlaceholderColor(imageName);
        ctx.fillStyle = color;
        ctx.fillRect(0, 0, THUMB_SIZE, THUMB_SIZE);

        // Add text
        ctx.fillStyle = '#FFFFFF';
        ctx.font = '10px Arial';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';

        // Truncate long names
        let displayName = imageName.split('/').pop();
        if (displayName.length > 10) {
            displayName = displayName.substring(0, 8) + '...';
        }

        ctx.fillText(displayName, THUMB_SIZE / 2, THUMB_SIZE / 2);
    }, [imageName]);

    // Function to draw the thumbnail
    const drawThumbnail = useCallback(async () => {
        if (!canvasRef.current) return;

        setStatus('loading');
        let pixelDataVal = null;

        // Draw initial placeholder
        drawPlaceholder();

        // Check if WebAssembly module is available
        if (!window.module) {
            console.error('WebAssembly module not available');
            setDebugMessage('Module not loaded');
            setStatus('debug');
            return;
        }

        // Check if get_thumbnail function exists
        if (typeof window.module.get_thumbnail !== 'function') {
            console.error('get_thumbnail function not available in WebAssembly module');
            setDebugMessage('No thumbnail API');
            setStatus('debug');
            return;
        }

        try {
            // Get thumbnail from WebAssembly module
            pixelDataVal = window.module.get_thumbnail(imageName, THUMB_SIZE, THUMB_SIZE);

            // Check if we got valid data
            if (!pixelDataVal) {
                throw new Error('No pixel data returned');
            }

            const bufferLength = pixelDataVal.byteLength;

            // Validate buffer length
            if (!bufferLength || bufferLength !== THUMB_SIZE * THUMB_SIZE * 4) {
                console.error(`Thumbnail Error (${imageName}): Invalid pixel data length: ${bufferLength}`);

                setDebugMessage(`Invalid data: ${bufferLength} bytes`);
                setStatus('debug');

                // If it's the first try, retry once
                if (retryCount === 0) {
                    setRetryCount(1);
                    setTimeout(() => drawThumbnail(), 500);
                }
                return;
            }

            // Get clamped array view
            const pixelData = new Uint8ClampedArray(pixelDataVal.buffer, pixelDataVal.byteOffset, bufferLength);

            // Swizzle format if needed (assuming BGRA to RGBA)
            const rgbaPixelData = new Uint8ClampedArray(bufferLength);
            for (let i = 0; i < bufferLength; i += 4) {
                // Check the pattern of first few pixels to detect format
                if (i < 20) {
                    console.log(`Pixel ${i/4}: R=${pixelData[i]}, G=${pixelData[i+1]}, B=${pixelData[i+2]}, A=${pixelData[i+3]}`);
                }

                // Copy data (might need to swap R and B if colors look wrong)
                rgbaPixelData[i] = pixelData[i];       // R
                rgbaPixelData[i + 1] = pixelData[i + 1]; // G
                rgbaPixelData[i + 2] = pixelData[i + 2]; // B
                rgbaPixelData[i + 3] = 255;              // A (force full opacity)
            }

            // Create ImageData and get context
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

            // If it's the first try, retry once
            if (retryCount === 0) {
                setRetryCount(1);
                setTimeout(() => drawThumbnail(), 500);
                return;
            }

            setDebugMessage(err.message);
            setStatus('error');
            drawPlaceholder();
        }
    }, [imageName, retryCount, drawPlaceholder]);

    // Initialize thumbnail on mount or when imageName changes
    useEffect(() => {
        console.log(`Initializing thumbnail for ${imageName}, selected: ${isSelected}`);
        setRetryCount(0); // Reset retry count when imageName changes

        // Small delay to avoid blocking the UI thread during initial load
        const timer = setTimeout(() => {
            drawThumbnail();
        }, 50);

        return () => clearTimeout(timer);
    }, [drawThumbnail, imageName]);

    // Get filename for display (strip path if present)
    const displayName = imageName.split('/').pop();

    // Handle click with debounce to prevent double-clicks
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
                onTouchStart={() => setTooltipOpen(true)} // Better touch device support
                onTouchEnd={() => setTooltipOpen(false)}  // Better touch device support
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