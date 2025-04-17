import React, { useState, useRef, useEffect, useCallback } from 'react';
import {
    Box,
    Typography,
    Button,
    CircularProgress,
    Paper,
    useTheme,
    Fade
} from '@mui/material';
import { Plus, Upload, Check, ImagePlus } from 'lucide-react';

import ThumbnailItem, { THUMB_SIZE } from './ThumbnailItem.jsx';

export const ImagePicker = ({ json }) => {
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
    const [menuItems, setMenuItems] = useState(json.items || []);

    // Calculate fixed height for 3 rows of thumbnails
    const fixedContainerHeight = THUMB_SIZE * 3;

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

    const handleFileUpload = async (file) => {
        setIsUploading(true);

        try {
            const reader = new FileReader();
            reader.onload = async (e) => {
                const uint8Array = new Uint8Array(e.target.result);
                const imagePath = `/lux_files/${file.name}`;
                const imageName = file.name.split('.')[0];

                if (window.module) {
                    console.log('Writing file:', imagePath);
                    window.module.FS.writeFile(imagePath, uint8Array);
                    window.module.add_image_to_scene(imageName, imagePath);
                    window.module.add_to_menu('source_image_menu', imageName);

                    const newItems = [...menuItems];
                    if (!newItems.includes(imageName)) {
                        newItems.push(imageName);
                        setMenuItems(newItems);
                    }

                    window.module.update_source_name(imageName);
                    setSelectedImage(imageName);
                }

                setIsUploading(false);
            };

            reader.readAsArrayBuffer(file);
        } catch (error) {
            console.error('Failed to upload image:', error);
            setIsUploading(false);
        }
    };

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

    // Calculate total height based on header + fixed thumbnail grid
    const headerHeight = 28; // Height of the header section
    const totalHeight = headerHeight + fixedContainerHeight;

    // Main render
    return (
        <Paper
            ref={dropAreaRef}
            elevation={0}
            sx={{
                width: THUMB_SIZE * 3, // Exact width of 3 thumbnails
                height: totalHeight, // Fixed height for 3 rows + header
                padding: 0, // Remove padding
                borderRadius: 1.5,
                border: '1px solid',
                borderColor: isDragging
                    ? theme.palette.primary.main
                    : theme.palette.divider,
                transition: 'all 0.2s ease-in-out',
                '&:hover': {
                    borderColor: isDragging
                        ? theme.palette.primary.main
                        : theme.palette.action.active,
                },
                position: 'relative',
                backgroundColor: theme.palette.background.paper,
                display: 'flex',
                flexDirection: 'column',
                overflow: 'hidden', // Prevent content from spilling out
            }}
        >
            {/* Header section - compact but visible */}
            <Box sx={{
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'space-between',
                paddingX: 0.5, // Minimal horizontal padding
                paddingY: 0.5, // Minimal vertical padding
                borderBottom: `1px solid ${theme.palette.divider}`,
                flexShrink: 0, // Prevent header from shrinking
                height: headerHeight + 'px', // Fixed height for header
            }}>
                <Box sx={{
                    display: 'flex',
                    alignItems: 'center',
                    gap: 0.5 // Reduce gap
                }}>
                    {/* Add button - more compact */}
                    <Button
                        variant="outlined"
                        onClick={handleUploadClick}
                        disabled={isUploading}
                        startIcon={isUploading ? <CircularProgress size={14} /> : <Plus size={14} />}
                        size="small"
                        sx={{
                            textTransform: 'none',
                            borderRadius: 1,
                            height: 24, // Smaller height
                            fontSize: '0.7rem', // Smaller font
                            borderStyle: 'dashed',
                            flexShrink: 0,
                            py: 0,
                            px: 0.5, // Reduce padding
                        }}
                    >
                        Add
                    </Button>

                    {/* Selected image label - more compact */}
                    {selectedImage && (
                        <Box
                            sx={{
                                display: 'flex',
                                alignItems: 'center',
                                borderRadius: 1,
                                py: 0.2, // Reduce padding
                                px: 0.5, // Reduce padding
                                maxWidth: '80px', // Reduce max width
                                bgcolor: theme.palette.action.selected
                            }}
                        >
                            <Check size={12} color={theme.palette.success.main} />
                            <Typography
                                variant="caption"
                                noWrap
                                sx={{
                                    ml: 0.2, // Reduce margin
                                    fontWeight: 500,
                                    overflow: 'hidden',
                                    textOverflow: 'ellipsis',
                                    fontSize: '0.65rem' // Smaller font
                                }}
                            >
                                {selectedImage}
                            </Typography>
                        </Box>
                    )}
                </Box>
            </Box>

            <input
                ref={fileInputRef}
                type="file"
                accept="image/*,.jpg,.jpeg,.png,.gif"
                onChange={(e) => e.target.files?.[0] && handleFileUpload(e.target.files[0])}
                style={{ display: 'none' }}
            />

            {/* Scrollable content container - fixed height for 3 rows */}
            <Box sx={{
                overflowY: 'auto', // Enable vertical scrolling
                overflowX: 'hidden',
                height: fixedContainerHeight, // Fixed height for 3 rows of thumbnails
                width: THUMB_SIZE * 3, // Exact width
                // Custom scrollbar styling
                '&::-webkit-scrollbar': {
                    width: '3px', // Thinner scrollbar
                },
                '&::-webkit-scrollbar-track': {
                    background: 'rgba(255, 255, 255, 0.05)',
                    borderRadius: '1px',
                },
                '&::-webkit-scrollbar-thumb': {
                    background: 'rgba(255, 255, 255, 0.2)',
                    borderRadius: '1px',
                    '&:hover': {
                        background: 'rgba(255, 255, 255, 0.3)',
                    },
                },
            }}>
                {availableImages.length === 0 ? (
                    // Empty state - sized to fit 3 rows
                    <Box
                        sx={{
                            display: 'flex',
                            flexDirection: 'column',
                            alignItems: 'center',
                            justifyContent: 'center',
                            height: fixedContainerHeight,
                            backgroundColor: isDragging
                                ? `${theme.palette.primary.main}10`
                                : theme.palette.action.hover,
                            borderRadius: 1,
                            border: '1px dashed',
                            borderColor: isDragging
                                ? theme.palette.primary.main
                                : theme.palette.divider,
                            transition: 'all 0.2s ease-in-out',
                        }}
                    >
                        <ImagePlus
                            size={24} // Smaller icon
                            color={isDragging
                                ? theme.palette.primary.main
                                : theme.palette.text.secondary
                            }
                        />
                        <Typography
                            variant="body2"
                            sx={{
                                mt: 1,
                                color: isDragging
                                    ? theme.palette.primary.main
                                    : theme.palette.text.secondary,
                                fontWeight: 500,
                                fontSize: '0.7rem' // Smaller font
                            }}
                        >
                            {isDragging
                                ? "Drop image"
                                : "No images"
                            }
                        </Typography>
                    </Box>
                ) : (
                    // Grid layout with fixed 3 columns - always showing 3 columns, vertical scroll for more rows
                    <Box
                        sx={{
                            display: 'grid',
                            gridTemplateColumns: `repeat(3, ${THUMB_SIZE}px)`, // Exact size columns
                            gap: 0, // No gap between thumbnails
                            position: 'relative',
                            width: THUMB_SIZE * 3, // Ensure full width
                            // Add overlay effect when dragging
                            '&::after': isDragging ? {
                                content: '""',
                                position: 'absolute',
                                top: 0,
                                left: 0,
                                right: 0,
                                bottom: 0,
                                backgroundColor: `${theme.palette.primary.main}10`,
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
                            <Fade in={isDragging}>
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
                                        backgroundColor: 'rgba(28, 28, 30, 0.85)',
                                        padding: 1, // Reduce padding
                                        borderRadius: 1,
                                        boxShadow: theme.shadows[4],
                                        pointerEvents: 'none'
                                    }}
                                >
                                    <Upload size={20} color={theme.palette.primary.main} />
                                    <Typography
                                        variant="caption" // Smaller variant
                                        fontWeight={500}
                                        color="primary.main"
                                        sx={{ mt: 0.5 }}
                                    >
                                        Drop to upload
                                    </Typography>
                                </Box>
                            </Fade>
                        )}

                        {/* Thumbnails */}
                        {availableImages.map((name) => (
                            <Box key={name} sx={{
                                display: 'flex',
                                justifyContent: 'center',
                                width: THUMB_SIZE, // Exact width
                                height: THUMB_SIZE, // Exact height
                                padding: 0, // No padding
                                margin: 0, // No margin
                            }}>
                                <ThumbnailItem
                                    imageName={name}
                                    isSelected={name === selectedImage}
                                    onClick={handleThumbnailClick}
                                />
                            </Box>
                        ))}
                    </Box>
                )}
            </Box>
        </Paper>
    );
};