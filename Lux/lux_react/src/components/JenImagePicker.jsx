import React, { useState, useRef, useEffect, useCallback } from 'react';
import {
    Box,
    Typography,
    Button,
    CircularProgress,
    Paper,
    Fade,
    Grid,
    Divider,
    useTheme
} from '@mui/material';
import { PlusSquare, ImagePlus, Check, Upload } from 'lucide-react';

// Import the thumbnail item component
import JenThumbnailItem from './JenThumbnailItem.jsx';

// Custom component for the section title with an optional counter
const SectionTitle = ({ label, count }) => (
    <Box sx={{
        display: 'flex',
        alignItems: 'center',
        mb: 1.5,
        mt: 1
    }}>
        <Typography
            variant="subtitle2"
            sx={{
                fontWeight: 600,
                color: 'text.primary',
                letterSpacing: '0.02em'
            }}
        >
            {label || 'Source Image'}
        </Typography>
        {count > 0 && (
            <Typography
                variant="caption"
                sx={{
                    ml: 1,
                    bgcolor: 'action.selected',
                    px: 1,
                    py: 0.25,
                    borderRadius: 1,
                    fontWeight: 500
                }}
            >
                {count}
            </Typography>
        )}
    </Box>
);

export const JenImagePicker = ({ json, width, onChange }) => {
    const theme = useTheme();
    const fileInputRef = useRef(null);
    const dropAreaRef = useRef(null);
    const [availableImages, setAvailableImages] = useState(json?.items || []);
    const [initialLoadDone, setInitialLoadDone] = useState(false);
    const [selectedImage, setSelectedImage] = useState(() => {
        const items = json?.items || [];
        const defaultIndex = json?.default_choice !== undefined ? json.default_choice : 0;
        const validIndex = Math.min(Math.max(0, defaultIndex), items.length - 1);
        return items[validIndex] || (items.length > 0 ? items[0] : null);
    });
    const [isUploading, setIsUploading] = useState(false);
    const [isDragging, setIsDragging] = useState(false);
    const [dropAreaActive, setDropAreaActive] = useState(false);

    // Calculate the grid columns based on width
    const calculateColumns = () => {
        const baseSize = 88; // Base size of each thumbnail + margins
        return Math.max(2, Math.floor((width - 32) / baseSize));
    };

    const [columns, setColumns] = useState(calculateColumns());

    // Update columns when width changes
    useEffect(() => {
        setColumns(calculateColumns());
    }, [width]);

    useEffect(() => {
        const newItems = json?.items || [];
        setAvailableImages(newItems);

        if (!initialLoadDone && newItems.length > 0) {
            const defaultIndex = json?.default_choice !== undefined ? json.default_choice : 0;
            const validIndex = Math.min(Math.max(0, defaultIndex), newItems.length - 1);
            const initialSelected = newItems[validIndex];
            setSelectedImage(initialSelected);
            setInitialLoadDone(true);
        } else if (newItems.length === 0) {
            setSelectedImage(null);
            setInitialLoadDone(false);
        }
    }, [json]);

    const handleFileUpload = useCallback(async (file) => {
        if (!file || !file.type.startsWith('image/')) {
            console.error("Invalid file type uploaded.");
            return;
        }
        setIsUploading(true);
        let baseName = file.name.split('.')[0].replace(/[^a-zA-Z0-9_]/g, '_');
        let uniqueName = baseName;
        let counter = 1;

        while (availableImages.includes(uniqueName)) {
            uniqueName = `${baseName}_${counter++}`;
        }
        const imageName = uniqueName;

        try {
            const bitmap = await createImageBitmap(file);
            const offscreenCanvas = document.createElement('canvas');
            offscreenCanvas.width = bitmap.width;
            offscreenCanvas.height = bitmap.height;
            const ctx = offscreenCanvas.getContext('2d');
            if (!ctx) throw new Error("Could not create offscreen canvas context");
            ctx.drawImage(bitmap, 0, 0);
            const imageData = ctx.getImageData(0, 0, bitmap.width, bitmap.height);
            const pixelData = imageData.data;
            bitmap.close();

            if (window.module && typeof window.module.add_image_to_scene === 'function') {
                window.module.add_image_to_scene(imageName, bitmap.width, bitmap.height, pixelData);

                setAvailableImages(prev => {
                    if (!prev.includes(imageName)) {
                        return [...prev, imageName];
                    }
                    return prev;
                });

                setSelectedImage(imageName);

                if (typeof window.module.add_to_menu === 'function') {
                    window.module.add_to_menu("source_image_menu", imageName);
                }
            } else {
                console.error("Wasm module or add_image_to_scene not available.");
            }
        } catch (error) {
            console.error(`Failed to process or upload image '${imageName}':`, error);
        } finally {
            setIsUploading(false);
            setDropAreaActive(false);
            if (fileInputRef.current) {
                fileInputRef.current.value = '';
            }
        }
    }, [availableImages]);

    const handleThumbnailClick = useCallback((imageName) => {
        if (imageName !== selectedImage) {
            setSelectedImage(imageName);
            if (window.module) {
                window.module.update_source_name(imageName);
            }
        }
    }, [selectedImage]);

    const handleUploadClick = () => {
        fileInputRef.current?.click();
    };

    // Handle drag events
    const handleDragOver = (e) => {
        e.preventDefault();
        e.stopPropagation();
        setIsDragging(true);
    };

    const handleDragEnter = (e) => {
        e.preventDefault();
        e.stopPropagation();
        setIsDragging(true);
    };

    const handleDragLeave = (e) => {
        e.preventDefault();
        e.stopPropagation();

        // Only set to false if actually leaving the container
        const rect = e.currentTarget.getBoundingClientRect();
        const x = e.clientX;
        const y = e.clientY;

        if (
            x <= rect.left ||
            x >= rect.right ||
            y <= rect.top ||
            y >= rect.bottom
        ) {
            setIsDragging(false);
        }
    };

    const handleDrop = (e) => {
        e.preventDefault();
        e.stopPropagation();
        setIsDragging(false);

        if (e.dataTransfer.files && e.dataTransfer.files.length > 0) {
            const file = e.dataTransfer.files[0];
            handleFileUpload(file);
        }
    };

    // Set up drag and drop listeners
    useEffect(() => {
        const element = dropAreaRef.current;
        if (!element) return;

        element.addEventListener('dragover', handleDragOver);
        element.addEventListener('dragenter', handleDragEnter);
        element.addEventListener('dragleave', handleDragLeave);
        element.addEventListener('drop', handleDrop);

        return () => {
            element.removeEventListener('dragover', handleDragOver);
            element.removeEventListener('dragenter', handleDragEnter);
            element.removeEventListener('dragleave', handleDragLeave);
            element.removeEventListener('drop', handleDrop);
        };
    }, [dropAreaRef.current]);

    // Main render
    return (
        <Paper
            ref={dropAreaRef}
            elevation={0}
            sx={{
                width: width || '100%',
                padding: 2,
                paddingBottom: 3,
                paddingTop: 0,
                overflowY: 'auto',
                maxHeight: 320,
                borderRadius: 1.5,
                borderColor: isDragging
                    ? theme.palette.primary.main
                    : theme.palette.divider,
                transition: 'all 0.2s ease-in-out',
                '&:hover': {
                    borderColor: isDragging
                        ? theme.palette.primary.main
                        : theme.palette.action.active,
                },
                position: 'relative'
            }}
        >
            <Box sx={{
                display: 'flex',
                justifyContent: 'space-between',
                alignItems: 'center'
            }}>
                <SectionTitle
                    label={json?.label || 'Source Images'}
                    count={availableImages.length}
                />
            </Box>

            <input
                ref={fileInputRef}
                type="file"
                accept="image/*,.jpg,.jpeg,.png,.gif"
                onChange={(e) => e.target.files?.[0] && handleFileUpload(e.target.files[0])}
                style={{ display: 'none' }}
            />

            {availableImages.length === 0 ? (
                // Empty state
                <Box
                    sx={{
                        display: 'flex',
                        flexDirection: 'column',
                        alignItems: 'center',
                        justifyContent: 'center',
                        py: 4,
                        backgroundColor: isDragging
                            ? `${theme.palette.primary.main}10`
                            : 'action.hover',
                        borderRadius: 1,
                        border: '1px dashed',
                        borderColor: isDragging
                            ? theme.palette.primary.main
                            : 'action.disabled',
                        transition: 'all 0.2s ease-in-out'
                    }}
                >
                    <ImagePlus
                        size={32}
                        color={isDragging
                            ? theme.palette.primary.main
                            : theme.palette.text.secondary
                        }
                    />
                    <Typography
                        variant="body2"
                        sx={{
                            mt: 1.5,
                            color: isDragging
                                ? theme.palette.primary.main
                                : 'text.secondary',
                            fontWeight: 500
                        }}
                    >
                        {isDragging
                            ? "Drop to upload image"
                            : "No images added yet"
                        }
                    </Typography>
                    <Button
                        variant="outlined"
                        onClick={handleUploadClick}
                        disabled={isUploading}
                        startIcon={isUploading ? <CircularProgress size={16} /> : null}
                        sx={{
                            mt: 2,
                            textTransform: 'none',
                            borderRadius: 1,
                            px: 2
                        }}
                    >
                        Upload first image
                    </Button>
                </Box>
            ) : (
                // Grid layout
                <Grid
                    container
                    spacing={1}
                    sx={{
                        mb: 1,
                        position: 'relative',
                        // Add overlay effect when dragging
                        '&::after': isDragging ? {
                            content: '""',
                            position: 'absolute',
                            top: 0,
                            left: 0,
                            right: 0,
                            bottom: 0,
                            backgroundColor: `${theme.palette.primary.main}20`,
                            border: '2px dashed',
                            borderColor: theme.palette.primary.main,
                            borderRadius: 1,
                            zIndex: 10,
                            pointerEvents: 'none'
                        } : {}
                    }}
                >
                    {/* Show drop indicator when dragging */}
                    {isDragging && (
                        <Box
                            sx={{
                                position: 'absolute',
                                top: '50%',
                                left: '50%',
                                transform: 'translate(-50%, -50%)',
                                zIndex: 20,
                                display: 'flex',
                                flexDirection: 'column',
                                alignItems: 'center',
                                justifyContent: 'center',
                                backgroundColor: 'rgba(255, 255, 255, 0.9)',
                                padding: 2,
                                borderRadius: 1,
                                boxShadow: theme.shadows[4],
                                pointerEvents: 'none'
                            }}
                        >
                            <Upload size={24} color={theme.palette.primary.main} />
                            <Typography
                                variant="body2"
                                fontWeight={500}
                                color="primary.main"
                                sx={{ mt: 1 }}
                            >
                                Drop to upload
                            </Typography>
                        </Box>
                    )}

                    {/* Thumbnails */}
                    {availableImages.map((name) => (
                        <Grid item key={name}>
                            <JenThumbnailItem
                                imageName={name}
                                isSelected={name === selectedImage}
                                onClick={handleThumbnailClick}
                            />
                        </Grid>
                    ))}

                    {/* Add button */}
                    <Grid item>
                        <Button
                            variant="outlined"
                            onClick={handleUploadClick}
                            disabled={isUploading}
                            sx={{
                                width: 80,
                                height: 80,
                                display: 'flex',
                                flexDirection: 'column',
                                alignItems: 'center',
                                justifyContent: 'center',
                                borderColor: 'divider',
                                color: 'text.secondary',
                                borderStyle: 'dashed',
                                borderRadius: 1,
                                '&:hover': {
                                    borderColor: 'primary.main',
                                    backgroundColor: 'action.hover'
                                }
                            }}
                        >
                            {isUploading ? (
                                <CircularProgress size={24} color="inherit"/>
                            ) : (
                                <>
                                    <PlusSquare size={24} />
                                    <Typography variant="caption" sx={{ mt: 0.5 }}>
                                        Add
                                    </Typography>
                                </>
                            )}
                        </Button>
                    </Grid>
                </Grid>
            )}

            {selectedImage && (
                <>
                    <Divider sx={{ my: 1 }} />
                    <Box
                        sx={{
                            display: 'flex',
                            alignItems: 'center',
                            mt: 1
                        }}
                    >
                        <Check size={16} color={theme.palette.success.main} />
                        <Typography
                            variant="caption"
                            sx={{
                                ml: 0.5,
                                color: 'text.secondary',
                                fontWeight: 500
                            }}
                        >
                            Selected: <span style={{ color: theme.palette.text.primary }}>{selectedImage}</span>
                        </Typography>
                    </Box>
                </>
            )}
        </Paper>
    );
};