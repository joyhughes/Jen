// Modified version of your ImagePicker component
import React, { useState, useRef, useEffect, useCallback } from 'react';
import {
    Box,
    Typography,
    Button,
    CircularProgress,
    Paper,
    useTheme
} from '@mui/material';
import { PlusSquare, ImagePlus, Check, Upload } from 'lucide-react';

import ThumbnailItem, {THUMB_SIZE} from './ThumbnailItem.jsx';
import PropTypes from "prop-types";

function SectionTitle(props) {
    return null;
}

SectionTitle.propTypes = {label: PropTypes.any};

export const ImagePicker = ({ json, width}) => {
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

    // Fixed grid layout with 3 columns
    const columns = 3;

    // Calculate the minimum width needed for 3 thumbnails plus minimal gaps
    // THUMB_SIZE * 3 for the thumbnails + (columns - 1) * 4px for the gaps
    const gridMinWidth = (THUMB_SIZE * columns) + ((columns - 1) * 4);
    const containerHeight = THUMB_SIZE * 3 + 75;

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
            };
            reader.readAsArrayBuffer(file);
        } catch (error) {
            console.error('Failed to upload image:', error);
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

    // Main render
    return (
        <Paper
            ref={dropAreaRef}
            elevation={0}
            sx={{
                width: width || '100%', // Use the specified width or default to 100%
                minWidth: gridMinWidth + 4, // Add minimal padding to the minimum width
                padding: 0.5, // Minimal padding
                paddingBottom: 1,
                paddingTop: 0.5,
                height: containerHeight, // Fixed height container
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
                position: 'relative',
                backgroundColor: '#252525',
                display: 'flex',
                flexDirection: 'column'
            }}
        >
            {/* Header section */}
            <Box sx={{
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'space-between',
                marginBottom: 1,
                padding: '8px 0',
                flexShrink: 0 // Prevent header from shrinking
            }}>
                <SectionTitle
                    label={json?.label || 'Source Images'}
                    count={availableImages.length}
                />

                <Box sx={{
                    display: 'flex',
                    alignItems: 'center',
                    gap: 1
                }}>
                    {/* Add button */}
                    <Button
                        variant="outlined"
                        onClick={handleUploadClick}
                        disabled={isUploading}
                        startIcon={isUploading ? <CircularProgress size={16} /> : <PlusSquare size={16} />}
                        size="small"
                        sx={{
                            textTransform: 'none',
                            borderRadius: 1,
                            height: 32,
                            fontSize: '0.75rem',
                            borderStyle: 'dashed',
                            flexShrink: 0,
                            borderColor: 'rgba(255, 255, 255, 0.23)',
                            color: 'rgba(255, 255, 255, 0.8)',
                            '&:hover': {
                                borderColor: 'rgba(255, 255, 255, 0.4)',
                                backgroundColor: 'rgba(255, 255, 255, 0.05)'
                            }
                        }}
                    >
                        Add Image
                    </Button>

                    {/* Selected image label */}
                    {selectedImage && (
                        <Box
                            sx={{
                                display: 'flex',
                                alignItems: 'center',
                                backgroundColor: 'rgba(255, 255, 255, 0.08)',
                                borderRadius: 1,
                                py: 0.5,
                                px: 1,
                                maxWidth: '120px'
                            }}
                        >
                            <Check size={14} color={theme.palette.success.main} />
                            <Typography
                                variant="caption"
                                noWrap
                                sx={{
                                    ml: 0.5,
                                    color: 'rgba(255, 255, 255, 0.7)',
                                    fontWeight: 500,
                                    overflow: 'hidden',
                                    textOverflow: 'ellipsis'
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

            {/* Scrollable content container */}
            <Box sx={{
                overflowY: 'auto',
                overflowX: 'hidden',
                flexGrow: 1, // Fill remaining height
                // Custom scrollbar styling
                '&::-webkit-scrollbar': {
                    width: '4px', // Thinner scrollbar
                },
                '&::-webkit-scrollbar-track': {
                    background: 'rgba(255, 255, 255, 0.05)', // Subtle track
                    borderRadius: '2px',
                },
                '&::-webkit-scrollbar-thumb': {
                    background: 'rgba(255, 255, 255, 0.2)', // Subtle thumb
                    borderRadius: '2px',
                    '&:hover': {
                        background: 'rgba(255, 255, 255, 0.3)', // Slightly lighter on hover
                    },
                },
            }}>
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
                                : 'rgba(255, 255, 255, 0.04)', // More harmonious with dark mode
                            borderRadius: 1,
                            border: '1px dashed',
                            borderColor: isDragging
                                ? theme.palette.primary.main
                                : 'rgba(255, 255, 255, 0.1)', // More harmonious with dark mode
                            transition: 'all 0.2s ease-in-out',
                            height: '100%'
                        }}
                    >
                        <ImagePlus
                            size={32}
                            color={isDragging
                                ? theme.palette.primary.main
                                : 'rgba(255, 255, 255, 0.5)' // More harmonious with dark mode
                            }
                        />
                        <Typography
                            variant="body2"
                            sx={{
                                mt: 1.5,
                                color: isDragging
                                    ? theme.palette.primary.main
                                    : 'rgba(255, 255, 255, 0.7)', // More harmonious with dark mode
                                fontWeight: 500
                            }}
                        >
                            {isDragging
                                ? "Drop to upload image"
                                : "No images added yet"
                            }
                        </Typography>
                    </Box>
                ) : (
                    // Grid layout with fixed 3 columns
                    <Box
                        sx={{
                            display: 'grid',
                            gridTemplateColumns: `repeat(${columns}, ${THUMB_SIZE}px)`, // Fixed width columns
                            gap: 0.5, // Reduced gap between items
                            position: 'relative',
                            paddingRight: 0.25, // Minimal padding for scrollbar
                            justifyContent: 'space-between', // Distribute columns evenly
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
                                    backgroundColor: 'rgba(30, 30, 30, 0.9)', // More harmonious with dark mode
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

                        {/* Thumbnails - direct without extra Box wrapper */}
                        {availableImages.map((name) => (
                            <ThumbnailItem
                                key={name}
                                imageName={name}
                                isSelected={name === selectedImage}
                                onClick={handleThumbnailClick}
                            />
                        ))}
                    </Box>
                )}
            </Box>
        </Paper>
    );
};