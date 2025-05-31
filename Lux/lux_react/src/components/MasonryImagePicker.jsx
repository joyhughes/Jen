import React, { useState, useRef, useEffect } from 'react';
import { Box, Typography, CircularProgress, Divider, Alert, useTheme, IconButton, Tooltip, Chip } from '@mui/material';
import { ImagePlus, Camera, Video, VideoOff } from 'lucide-react';
import ThumbnailItem from './ThumbnailItem';
import CameraCapture from './CameraCapture';
import { createLiveCameraManager, getLiveCameraManager, destroyLiveCameraManager } from './LiveCameraManager';
import Masonry from 'react-masonry-css';
import '../styles/MasonryImagePicker.css';

export const MasonryImagePicker = ({ json, width, onChange, setActivePane }) => {
    const [menuItems, setMenuItems] = useState([]);
    const [selectedImage, setSelectedImage] = useState('');
    const [isDragging, setIsDragging] = useState(false);
    const [isLoading, setIsLoading] = useState(false);
    const [isInitializing, setIsInitializing] = useState(true);
    const [newImageName, setNewImageName] = useState(null);
    const [error, setError] = useState(null);
    const [showCamera, setShowCamera] = useState(false);
    const [isLiveCameraActive, setIsLiveCameraActive] = useState(false);
    const [liveCameraError, setLiveCameraError] = useState(null);
    const fileInputRef = useRef(null);
    const containerRef = useRef(null);
    const [containerWidth, setContainerWidth] = useState(0);
    const theme = useTheme();

    // The thumbnail size is 64px, and we'll add consistent spacing
    const THUMB_SIZE = 64;
    const THUMB_SPACING = 8;
    // Each thumbnail needs this much space with its margins
    const TOTAL_THUMB_WIDTH = THUMB_SIZE + (THUMB_SPACING * 2);

    // Use ResizeObserver to detect actual width changes
    useEffect(() => {
        if (!containerRef.current) return;

        const resizeObserver = new ResizeObserver(entries => {
            for (let entry of entries) {
                const width = entry.contentRect.width;
                if (width > 0) {
                    setContainerWidth(width);
                }
            }
        });

        resizeObserver.observe(containerRef.current);

        return () => {
            if (containerRef.current) {
                resizeObserver.unobserve(containerRef.current);
            }
        };
    }, []);

    // Calculate breakpoints for image grid - ensuring we fit as many as possible
    const getBreakpointColumns = () => {
        // If we have the actual container width, use it, otherwise use the provided width or a default
        const availableWidth = containerWidth || (typeof width === 'number' ? width : 400);

        // Calculate how many thumbnails can fit in a row based on available width
        // Ensure at least 2 thumbnails per row
        const maxColumns = Math.max(2, Math.floor(availableWidth / TOTAL_THUMB_WIDTH));

        // Create a breakpoint object that fits as many thumbnails as possible
        const breakpoints = {
            default: maxColumns,         // Default to maximum possible thumbnails per row
            1200: Math.min(maxColumns, 6), // Large screens: up to 6 thumbnails per row
            900: Math.min(maxColumns, 5),  // Medium-large screens: up to 5 thumbnails per row
            600: Math.min(maxColumns, 4),  // Medium screens: up to 4 thumbnails per row
            450: Math.min(maxColumns, 3),  // Small screens: up to 3 thumbnails per row
            300: 2                        // Very small screens: 2 thumbnails per row
        };

        console.log(`ImagePicker: Available width ${availableWidth}px, max columns: ${maxColumns}`);
        return breakpoints;
    };

    // Log the incoming JSON for debugging
    useEffect(() => {
        console.log('MasonryImagePicker received JSON:', json);
    }, [json]);

    useEffect(() => {
        try {
            if (json) {
                let items = [];

                if (json.items && Array.isArray(json.items)) {
                    items = json.items;
                } else if (json.menu && Array.isArray(json.menu)) {
                    items = json.menu;
                } else if (typeof json.menu === 'string') {
                    try {
                        if (window.module && window.module.get_menu_items) {
                            const menuItemsStr = window.module.get_menu_items(json.menu);
                            items = JSON.parse(menuItemsStr);
                        }
                    } catch (menuErr) {
                        console.error('Error fetching menu items:', menuErr);
                    }
                }

                // Add live camera option to the beginning of the menu
                const allItems = ['LIVE_CAMERA', ...items];
                setMenuItems(allItems);
                console.log('Menu items with live camera:', allItems);

                let selectedIdx = -1;

                if (json.choice !== undefined && Number.isInteger(json.choice)) {
                    selectedIdx = json.choice + 1; // Offset by 1 due to live camera option
                } else if (json.selected !== undefined && Number.isInteger(json.selected)) {
                    selectedIdx = json.selected + 1;
                } else if (json.value !== undefined && Number.isInteger(json.value)) {
                    selectedIdx = json.value + 1;
                }

                if (selectedIdx >= 0 && selectedIdx < allItems.length) {
                    console.log('Setting selected image to:', allItems[selectedIdx]);
                    setSelectedImage(allItems[selectedIdx]);
                } else if (allItems.length > 1) {
                    // Default to first non-camera item if no selection
                    console.log('Defaulting to first image:', allItems[1]);
                    setSelectedImage(allItems[1]);
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

    // Handle live camera toggle
    const toggleLiveCamera = async () => {
        try {
            setLiveCameraError(null);
            
            if (isLiveCameraActive) {
                // Stop live camera
                const cameraManager = getLiveCameraManager();
                if (cameraManager) {
                    cameraManager.stop();
                }
                destroyLiveCameraManager();
                setIsLiveCameraActive(false);
                console.log('[ImagePicker] Live camera stopped');
                
                // Switch back to first regular image if available
                const regularImages = menuItems.filter(item => item !== 'LIVE_CAMERA');
                if (regularImages.length > 0) {
                    handleImageSelect(regularImages[0]);
                }
            } else {
                // Start live camera
                const cameraManager = createLiveCameraManager();
                await cameraManager.start();
                setIsLiveCameraActive(true);
                setSelectedImage('LIVE_CAMERA');
                console.log('[ImagePicker] Live camera started');
                
                // Navigate to home to show the live camera
                setTimeout(() => {
                    setActivePane("home");
                }, 100);
            }
        } catch (error) {
            console.error('[ImagePicker] Live camera error:', error);
            setLiveCameraError(`Live camera failed: ${error.message}`);
            setIsLiveCameraActive(false);
        }
    };

    // Handle file upload
    const handleFileUpload = async (file) => {
        try {
            setIsLoading(true);
            
            // Stop live camera if active
            if (isLiveCameraActive) {
                const cameraManager = getLiveCameraManager();
                if (cameraManager) {
                    cameraManager.stop();
                }
                destroyLiveCameraManager();
            }
            
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

            setTimeout(() => {
                setActivePane("home");
                console.log("Navigating to home pane after image uploading");
            }, 100)
        } catch (error) {
            console.error('Failed to upload image:', error);
            setError('Failed to upload image: ' + error.message);
            setIsLoading(false);
        }
    };

    // Handle camera capture
    const handleCameraCapture = async (captureData) => {
        try {
            setIsLoading(true);
            
            // Stop live camera if active
            if (isLiveCameraActive) {
                const cameraManager = getLiveCameraManager();
                if (cameraManager) {
                    cameraManager.stop();
                }
                destroyLiveCameraManager();
            }
            
            const { filename, imageName, imageData, blob } = captureData;
            
            // Camera capture now handles backend processing directly
            // Just update the UI state to reflect the new image
            if (imageName) {
                // Update local state with new camera image
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

                // Auto-select the newly captured image
                setSelectedImage(imageName);
                onChange(imageName);
                
                console.log('Camera capture processed successfully:', imageName);
            }
            
            setIsLoading(false);
            
            // Close camera and navigate to home
            setShowCamera(false);
            setTimeout(() => {
                setActivePane("home");
                console.log("Navigating to home pane after camera capture");
            }, 100);
            
        } catch (error) {
            console.error('Failed to process camera capture:', error);
            setError('Failed to process camera capture: ' + error.message);
            setIsLoading(false);
        }
    };

    const handleImageSelect = (imageName) => {
        console.log('Image selected:', imageName);
        
        if (imageName === 'LIVE_CAMERA') {
            if (!isLiveCameraActive) {
                toggleLiveCamera();
            }
            return;
        }
        
        // Stop live camera if switching to regular image
        if (isLiveCameraActive) {
            const cameraManager = getLiveCameraManager();
            if (cameraManager) {
                cameraManager.stop();
            }
            destroyLiveCameraManager();
            setIsLiveCameraActive(false);
        }
        
        setSelectedImage(imageName);

        if (window.module) {
            window.module.update_source_name(imageName);
            onChange(imageName);
        }
        setTimeout(() => {
            setActivePane("home");
            console.log("Navigating to home pane after image selection");
        }, 100);
    };

    // Cleanup live camera on unmount
    useEffect(() => {
        return () => {
            if (isLiveCameraActive) {
                const cameraManager = getLiveCameraManager();
                if (cameraManager) {
                    cameraManager.stop();
                }
                destroyLiveCameraManager();
            }
        };
    }, []);

    // Check if camera is supported
    const isCameraSupported = () => {
        return !!(navigator.mediaDevices && navigator.mediaDevices.getUserMedia);
    };

    // Custom thumbnail for live camera
    const LiveCameraThumbnail = ({ isSelected, onClick }) => (
        <Box
            onClick={() => onClick('LIVE_CAMERA')}
            sx={{
                width: THUMB_SIZE,
                height: THUMB_SIZE,
                borderRadius: 1,
                border: isSelected ? '3px solid' : '2px solid',
                borderColor: isSelected ? 'primary.main' : (isLiveCameraActive ? 'success.main' : 'grey.400'),
                cursor: 'pointer',
                display: 'flex',
                flexDirection: 'column',
                alignItems: 'center',
                justifyContent: 'center',
                background: isLiveCameraActive 
                    ? 'linear-gradient(45deg, rgba(76, 175, 80, 0.1), rgba(76, 175, 80, 0.2))'
                    : 'linear-gradient(45deg, rgba(33, 150, 243, 0.1), rgba(33, 150, 243, 0.2))',
                transition: 'all 0.2s ease-in-out',
                position: 'relative',
                '&:hover': {
                    borderColor: isLiveCameraActive ? 'success.light' : 'primary.light',
                    transform: 'scale(1.05)',
                }
            }}
        >
            {isLiveCameraActive ? (
                <>
                    <Video size={20} color={theme.palette.success.main} />
                    <Typography variant="caption" sx={{ 
                        fontSize: '9px', 
                        color: 'success.main', 
                        fontWeight: 'bold',
                        textAlign: 'center',
                        mt: 0.5
                    }}>
                        LIVE
                    </Typography>
                    {/* Live indicator dot */}
                    <Box
                        sx={{
                            position: 'absolute',
                            top: 4,
                            right: 4,
                            width: 8,
                            height: 8,
                            borderRadius: '50%',
                            backgroundColor: 'success.main',
                            animation: 'pulse 2s infinite',
                            '@keyframes pulse': {
                                '0%': { opacity: 1 },
                                '50%': { opacity: 0.5 },
                                '100%': { opacity: 1 },
                            }
                        }}
                    />
                </>
            ) : (
                <>
                    <VideoOff size={20} color={theme.palette.primary.main} />
                    <Typography variant="caption" sx={{ 
                        fontSize: '9px', 
                        color: 'primary.main', 
                        fontWeight: 'bold',
                        textAlign: 'center',
                        mt: 0.5
                    }}>
                        CAMERA
                    </Typography>
                </>
            )}
        </Box>
    );

    // Show camera interface
    if (showCamera) {
        return (
            <Box sx={{ width: width || '100%', height: 400 }}>
                <CameraCapture
                    onCapture={handleCameraCapture}
                    onClose={() => setShowCamera(false)}
                    width={width || '100%'}
                    height={400}
                    enableLivePreview={true}
                    autoSaveToGrid={true}
                />
            </Box>
        );
    }

    return (
        <Box sx={{ width: width || '100%' }} ref={containerRef}>
            {/* Error messages */}
            {error && (
                <Alert severity="error" sx={{ mb: 2 }}>
                    {error}
                </Alert>
            )}
            
            {liveCameraError && (
                <Alert severity="error" sx={{ mb: 2 }} onClose={() => setLiveCameraError(null)}>
                    {liveCameraError}
                </Alert>
            )}

            {/* Live Camera Status */}
            {isLiveCameraActive && (
                <Box sx={{ mb: 2 }}>
                    <Chip 
                        icon={<Video size={16} />}
                        label="Live Camera Active"
                        color="success"
                        variant="outlined"
                        size="small"
                        sx={{ fontWeight: 'bold' }}
                    />
                </Box>
            )}

            {/* Image grid using flex-based grid for consistent spacing */}
            {isInitializing ? (
                <Box sx={{ display: 'flex', justifyContent: 'center', py: 4 }}>
                    <CircularProgress size={24} />
                </Box>
            ) : menuItems.length > 0 ? (
                <Box className="image-grid-container">
                    {menuItems.map((imageName) => (
                        <Box
                            key={imageName}
                            className={`image-grid-item ${newImageName === imageName ? 'new-image' : ''}`}
                        >
                            {imageName === 'LIVE_CAMERA' ? (
                                <LiveCameraThumbnail
                                    isSelected={selectedImage === imageName}
                                    onClick={handleImageSelect}
                                />
                            ) : (
                                <ThumbnailItem
                                    imageName={imageName}
                                    isSelected={selectedImage === imageName}
                                    onClick={handleImageSelect}
                                />
                            )}
                        </Box>
                    ))}
                </Box>
            ) : (
                <Box sx={{ textAlign: 'center', py: 3, color: 'text.secondary' }}>
                    <ImagePlus size={24} style={{ opacity: 0.5, margin: '0 auto 8px' }} />
                    <Typography variant="body2">
                        No images available. Upload an image or take a photo to get started.
                    </Typography>
                </Box>
            )}

            {/* Upload and Camera section */}
            <Divider sx={{ my: 2 }} />

            <Box sx={{ position: 'relative' }}>
                {/* Camera and Upload buttons */}
                <Box sx={{ display: 'flex', gap: 1, mb: 2 }}>
                    {/* Still Camera button */}
                    {isCameraSupported() && (
                        <Tooltip title="Take Photo">
                            <IconButton
                                onClick={() => setShowCamera(true)}
                                disabled={isLoading}
                                sx={{
                                    bgcolor: 'primary.main',
                                    color: 'white',
                                    '&:hover': {
                                        bgcolor: 'primary.dark',
                                    },
                                    '&:disabled': {
                                        bgcolor: 'grey.300',
                                    }
                                }}
                            >
                                <Camera size={20} />
                            </IconButton>
                        </Tooltip>
                    )}
                    
                    {/* Upload button */}
                    <Tooltip title="Upload Image">
                        <IconButton
                            onClick={() => fileInputRef.current?.click()}
                            disabled={isLoading}
                            sx={{
                                bgcolor: 'secondary.main',
                                color: 'white',
                                '&:hover': {
                                    bgcolor: 'secondary.dark',
                                },
                                '&:disabled': {
                                    bgcolor: 'grey.300',
                                }
                            }}
                        >
                            <ImagePlus size={20} />
                        </IconButton>
                    </Tooltip>
                </Box>

                {/* Drag and drop area */}
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
                    <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 1, mb: 1 }}>
                        <ImagePlus size={24} />
                        {isCameraSupported() && <Camera size={24} />}
                    </Box>
                    <Typography sx={{ mt: 1 }}>
                        Drag & drop an image, tap to browse, or use camera
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