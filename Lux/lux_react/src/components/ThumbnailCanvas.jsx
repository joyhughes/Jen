import { useEffect, useRef, useState } from 'react';
import { Box, CircularProgress, Fade } from '@mui/material';
import { fetchAndDrawThumbnail, drawPlaceholder } from '@/utils/utils.js';

const ThumbnailCanvas = ({
    imageName,
    width = 64,
    height = 64,
}) => {
    const canvasRef = useRef(null);
    const [retryCount, setRetryCount] = useState(0);
    const maxRetries = 1;

    useEffect(() => {
        let isMounted = true;
        let timeoutId;

        const loadThumbnail = async () => {
            if (!canvasRef.current || !imageName) return;
            drawPlaceholder(canvasRef.current, imageName, width);
            const result = await fetchAndDrawThumbnail(
                canvasRef.current,
                imageName,
                width,
                height
            );

            if (!isMounted) return;

            if (result.success) {
                console.log(`Thumbnail loaded successfully: ${imageName}`);
            } else {
                if (result.shouldRetry && retryCount < maxRetries) {
                    setRetryCount(prev => prev + 1);
                    timeoutId = setTimeout(loadThumbnail, 500);
                } else if (!result.success) {
                    drawPlaceholder(canvasRef.current, imageName, width);
                }
            }
        };

        loadThumbnail();

        return () => {
            isMounted = false;
            if (timeoutId) clearTimeout(timeoutId);
        };
    }, [imageName, width, height, retryCount]);

    return (
        <Box
            sx={{
                width,
                height,
                position: 'relative',
                borderRadius: '4px',
                overflow: 'hidden',
                bgcolor: 'rgba(0,0,0,0.2)'
            }}
        >
            <canvas
                ref={canvasRef}
                width={width}
                height={height}
                style={{ width: '100%', height: '100%' }}
            />

            {/* Use Fade transition to smoothly hide/show the loader */}
            <Fade in={status === 'loading'} timeout={300}>
                <Box
                    sx={{
                        position: 'absolute',
                        top: 0, left: 0, right: 0, bottom: 0,
                        display: status === 'loading' ? 'flex' : 'none', // Explicit display control
                        alignItems: 'center',
                        justifyContent: 'center',
                        backgroundColor: 'rgba(0,0,0,0.4)',
                        pointerEvents: 'none' // Prevent blocking interaction with the canvas
                    }}
                >
                    <CircularProgress size={width * 0.3} />
                </Box>
            </Fade>
        </Box>
    );
};

export default ThumbnailCanvas;