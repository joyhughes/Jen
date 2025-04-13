import React, { useState, useRef, useEffect, useCallback } from 'react';
import {
    Box,
    Typography,
    Button,
    CircularProgress,
    Paper,
    Grid,
    Divider,
    useTheme
} from '@mui/material';
import { PlusSquare, ImagePlus, Check, Upload } from 'lucide-react';

import ThumbnailItem from './ThumbnailItem.jsx';
import PropTypes from "prop-types";


function SectionTitle(props) {
    return null;
}

SectionTitle.propTypes = {label: PropTypes.any};
export const ImagePicker = ({ json, width, onChange }) => {
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
    const [menuItems, setMenuItems] = useState(json.items || []);


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
                width: width || '100%',
                marginTop: 5,
                padding: 2,
                paddingBottom: 3,
                paddingTop: 0,
                overflowY: 'auto',
                maxHeight: 190, // Reduced from 320 to take less space
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
                backgroundColor: '#252525'
            }}
        >
            <Box sx={{
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'space-between',
                marginBottom: 2
            }}>
                <SectionTitle
                    label={json?.label || 'Source Images'}
                    count={availableImages.length}
                />

                <Box sx={{
                    display: 'flex',
                    alignItems: 'center',
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
                            flexShrink: 0  // Prevent button from shrinking
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
                                backgroundColor: 'action.hover',
                                borderRadius: 1,
                                py: 0.5,
                                px: 1,
                                maxWidth: '120px'  // Limit width to prevent overflow
                            }}
                        >
                            <Check size={14} color={theme.palette.success.main} />
                            <Typography
                                variant="caption"
                                noWrap  // Prevents text wrapping
                                sx={{
                                    ml: 0.5,
                                    color: 'text.secondary',
                                    fontWeight: 500,
                                    overflow: 'hidden',
                                    textOverflow: 'ellipsis'  // Add ellipsis for long names
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
                            <ThumbnailItem
                                imageName={name}
                                isSelected={name === selectedImage}
                                onClick={handleThumbnailClick}
                            />
                        </Grid>
                    ))}

                    {/* Removed the Add button from the grid as it's now at the top */}
                </Grid>
            )}
        </Paper>
    );
};