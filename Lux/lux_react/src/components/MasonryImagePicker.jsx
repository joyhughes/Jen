import React, { useState, useRef, useEffect } from 'react';
import { Box, Typography, CircularProgress, Divider, Alert } from '@mui/material';
import { ImagePlus } from 'lucide-react';
import ThumbnailItem from './ThumbnailItem';
import './MasonryImagePicker.css';

export const MasonryImagePicker = ({ json, width, onChange }) => {
    const [menuItems, setMenuItems] = useState([]);
    const [selectedImage, setSelectedImage] = useState('');
    const [isDragging, setIsDragging] = useState(false);
    const [isLoading, setIsLoading] = useState(false);
    const [isInitializing, setIsInitializing] = useState(true);
    const [newImageName, setNewImageName] = useState(null);
    const [error, setError] = useState(null);
    const fileInputRef = useRef(null);

    // Log the incoming JSON for debugging
    useEffect(() => {
        console.log('MasonryImagePicker received JSON:', json);
    }, [json]);

    // Set up selected image and menu items based on json
    useEffect(() => {
        try {
            if (json) {
                // Handle different formats to extract items
                let items = [];

                if (json.items && Array.isArray(json.items)) {
                    // Standard format
                    items = json.items;
                } else if (json.menu && Array.isArray(json.menu)) {
                    // Alternative format
                    items = json.menu;
                } else if (typeof json.menu === 'string') {
                    // Menu name format - need to fetch items from module
                    try {
                        if (window.module && window.module.get_menu_items) {
                            const menuItemsStr = window.module.get_menu_items(json.menu);
                            items = JSON.parse(menuItemsStr);
                        }
                    } catch (menuErr) {
                        console.error('Error fetching menu items:', menuErr);
                    }
                }

                setMenuItems(items);
                console.log('Extracted menu items:', items);

                // Set initially selected image
                let selectedIdx = -1;

                if (json.choice !== undefined && Number.isInteger(json.choice)) {
                    selectedIdx = json.choice;
                } else if (json.selected !== undefined && Number.isInteger(json.selected)) {
                    selectedIdx = json.selected;
                } else if (json.value !== undefined && Number.isInteger(json.value)) {
                    selectedIdx = json.value;
                }

                if (selectedIdx >= 0 && selectedIdx < items.length) {
                    console.log('Setting selected image to:', items[selectedIdx]);
                    setSelectedImage(items[selectedIdx]);
                } else if (items.length > 0) {
                    // Default to first item if no selection
                    console.log('Defaulting to first image:', items[0]);
                    setSelectedImage(items[0]);
                }

                setError(null);
            } else {
                setError('No JSON data provided to MasonryImagePicker');
            }
        } catch (err) {
            console.error('Error initializing MasonryImagePicker:', err);
            setError('Failed to initialize image picker: ' + err.message);
        } finally {
            setIsInitializing(false);
        }
    }, [json]);

    // Handle file upload
    const handleFileUpload = async (file) => {
        try {
            setIsLoading(true);
            const reader = new FileReader();
            reader.onload = async (e) => {
                const uint8Array = new Uint8Array(e.target.result);
                const imagePath = `/lux_files/${file.name}`;
                const imageName = file.name.split('.')[0];

                if (window.module) {
                    console.log('Writing file:', imagePath);
                    window.module.FS.writeFile(imagePath, uint8Array);
                    window.module.add_image_to_scene(imageName, imagePath);

                    // Determine which menu to add to
                    let menuName = 'source_image_menu';
                    if (json && json.menu && typeof json.menu === 'string') {
                        menuName = json.menu;
                    }

                    console.log('Adding image to menu:', menuName);
                    window.module.add_to_menu(menuName, imageName);

                    // Update local state with new menu items
                    const newItems = [...menuItems];
                    if (!newItems.includes(imageName)) {
                        newItems.push(imageName);
                        setMenuItems(newItems);
                        setNewImageName(imageName); // Mark as new for animation

                        // Reset new image name after animation duration
                        setTimeout(() => {
                            setNewImageName(null);
                        }, 1000);
                    }

                    // Auto-select the newly added image
                    setSelectedImage(imageName);
                    window.module.update_source_name(imageName);
                    onChange(imageName);
                }
                setIsLoading(false);
            };
            reader.readAsArrayBuffer(file);
        } catch (error) {
            console.error('Failed to upload image:', error);
            setError('Failed to upload image: ' + error.message);
            setIsLoading(false);
        }
    };

    // Handle image selection
    const handleImageSelect = (imageName) => {
        console.log('Image selected:', imageName);
        setSelectedImage(imageName);

        // Update the selected image in the C++ backend
        if (window.module) {
            window.module.update_source_name(imageName);
            onChange(imageName);
        }
    };

    return (
        <Box sx={{ width: width || '100%' }}>
            <Typography variant="subtitle1" gutterBottom>
                Source Images
            </Typography>

            {error && (
                <Alert severity="error" sx={{ mb: 2 }}>
                    {error}
                </Alert>
            )}

            {/* Thumbnails grid */}
            {isInitializing ? (
                <Box sx={{ display: 'flex', justifyContent: 'center', py: 4 }}>
                    <CircularProgress size={24} />
                </Box>
            ) : menuItems.length > 0 ? (
                <Box className="thumbnails-container">
                    {menuItems.map((imageName) => (
                        <Box
                            key={imageName}
                            className={newImageName === imageName ? 'new-thumbnail' : ''}
                        >
                            <ThumbnailItem
                                imageName={imageName}
                                isSelected={selectedImage === imageName}
                                onClick={handleImageSelect}
                            />
                        </Box>
                    ))}
                </Box>
            ) : (
                <Box sx={{ textAlign: 'center', py: 3, color: 'text.secondary' }}>
                    <ImagePlus size={24} style={{ opacity: 0.5, margin: '0 auto 8px' }} />
                    <Typography variant="body2">
                        No images available. Upload an image to get started.
                    </Typography>
                </Box>
            )}

            {/* Upload section with loading overlay */}
            <Divider sx={{ my: 2 }} />

            <Box sx={{ position: 'relative' }}>
                <Box
                    sx={{
                        p: 2,
                        border: 2,
                        borderStyle: 'dashed',
                        borderColor: isDragging ? 'primary.main' : 'grey.300',
                        borderRadius: 1,
                        textAlign: 'center',
                        bgcolor: isDragging ? 'action.hover' : 'transparent',
                        transition: 'all 0.2s',
                        cursor: 'pointer',
                        opacity: isLoading ? 0.5 : 1,
                        pointerEvents: isLoading ? 'none' : 'auto'
                    }}
                    onDragOver={(e) => { e.preventDefault(); setIsDragging(true); }}
                    onDragLeave={() => setIsDragging(false)}
                    onDrop={(e) => {
                        e.preventDefault();
                        setIsDragging(false);
                        const file = e.dataTransfer.files[0];
                        if (file?.type.startsWith('image/')) handleFileUpload(file);
                    }}
                    onClick={() => fileInputRef.current?.click()}
                >
                    <ImagePlus size={24} />
                    <Typography sx={{ mt: 1 }}>
                        Drag & drop an image or tap to browse
                    </Typography>
                    <Typography variant="caption" color="text.secondary">
                        Supports JPG, PNG, and other common formats
                    </Typography>
                    <input
                        ref={fileInputRef}
                        type="file"
                        accept="image/*"
                        onChange={(e) => e.target.files?.[0] && handleFileUpload(e.target.files[0])}
                        style={{ display: 'none' }}
                    />
                </Box>

                {isLoading && (
                    <Box
                        sx={{
                            position: 'absolute',
                            top: 0,
                            left: 0,
                            right: 0,
                            bottom: 0,
                            display: 'flex',
                            alignItems: 'center',
                            justifyContent: 'center',
                        }}
                    >
                        <CircularProgress size={24} />
                    </Box>
                )}
            </Box>
        </Box>
    );
};

export default MasonryImagePicker;