import React, { useState, useRef, useEffect } from 'react';
import { Box, Typography, CircularProgress, Divider, Alert, useTheme, IconButton, Tooltip, Chip, Button } from '@mui/material';
import { ImagePlus, Camera, Video, VideoOff, FlipHorizontal, RotateCcw, Play, Radio } from 'lucide-react';
import ThumbnailItem from './ThumbnailItem';
import CameraCapture from './CameraCapture';
import { createLiveCameraManager, getLiveCameraManager, destroyLiveCameraManager } from './LiveCameraManager';
import { useScene } from './SceneContext';
import { isMobileDevice } from '../utils/cameraUtils';
import Masonry from 'react-masonry-css';
import '../styles/MasonryImagePicker.css';

export const MasonryImagePicker = ({ json, width, onChange, setActivePane }) => {
    const { scenes, currentSceneIndex } = useScene();
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
    const [liveCameraInfo, setLiveCameraInfo] = useState({
        facingMode: 'user',
        isFrontCamera: true,
        isBackCamera: false,
        availableCameras: []
    });
    const [isSwitchingCamera, setIsSwitchingCamera] = useState(false);
    const [cachedLiveCameraSupported, setCachedLiveCameraSupported] = useState(false);
    const fileInputRef = useRef(null);
    const containerRef = useRef(null);
    const [containerWidth, setContainerWidth] = useState(0);
    const theme = useTheme();

    // Check if device is mobile
    const isMobile = isMobileDevice();

    // Get default camera facing mode - back camera for mobile, front for desktop
    const getDefaultFacingMode = () => {
        return isMobile ? 'environment' : 'user';
    };

    // The thumbnail size is 64px, and we'll add consistent spacing
    const THUMB_SIZE = 64;
    const THUMB_SPACING = 8;
    // Each thumbnail needs this much space with its margins
    const TOTAL_THUMB_WIDTH = THUMB_SIZE + (THUMB_SPACING * 2);

    // Check if current scene supports live camera (using cached value)
    const isLiveCameraSupported = () => {
        return cachedLiveCameraSupported;
    };

    // Fetch live camera support from backend when scene changes (once per scene)
    useEffect(() => {
        const fetchLiveCameraSupport = async () => {
            console.log('Checking live camera support for scene index:', currentSceneIndex);
            
            // Wait a bit for the scene to load in the backend
            await new Promise(resolve => setTimeout(resolve, 200));
            
            if (window.module && window.module.is_live_camera_supported) {
                const supported = window.module.is_live_camera_supported();
                // Only update if the support status has actually changed
                if (cachedLiveCameraSupported !== supported) {
                    setCachedLiveCameraSupported(supported);
                    console.log('Live camera support changed to:', supported, 'for scene index:', currentSceneIndex);
                }
            } else {
                if (cachedLiveCameraSupported !== false) {
                    setCachedLiveCameraSupported(false);
                }
            }
        };

        // Fetch when scene changes
        fetchLiveCameraSupport();
    }, [currentSceneIndex]);

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

    // Update live camera info periodically
    useEffect(() => {
        if (isLiveCameraActive) {
            const updateCameraInfo = () => {
                const cameraManager = getLiveCameraManager();
                if (cameraManager) {
                    const info = cameraManager.getCurrentCameraInfo();
                    // Only update if important info has actually changed
                    if (liveCameraInfo.facingMode !== info.facingMode || 
                        liveCameraInfo.availableCameras.length !== info.availableCameras.length) {
                        setLiveCameraInfo(info);
                    }
                }
            };

            updateCameraInfo();
            // Reduce frequency to prevent too many updates
            const interval = setInterval(updateCameraInfo, 3000); // Update every 3 seconds
            return () => clearInterval(interval);
        }
    }, [isLiveCameraActive]); // Removed liveCameraInfo dependency to prevent loops

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

                // Filter out ultra_camera as it's handled by our live camera button
                items = items.filter(item => item !== 'ultra_camera');

                // No longer add live camera to the menu items - it's now a separate button
                setMenuItems(items);
                console.log('Menu items set to:', items);

                let selectedIdx = -1;

                if (json.choice !== undefined && Number.isInteger(json.choice)) {
                    selectedIdx = json.choice;
                } else if (json.selected !== undefined && Number.isInteger(json.selected)) {
                    selectedIdx = json.selected;
                } else if (json.value !== undefined && Number.isInteger(json.value)) {
                    selectedIdx = json.value;
                }

                // Set selected image based on menu (no live camera offset needed)
                let targetSelectedImage = '';
                if (selectedIdx >= 0 && selectedIdx < items.length) {
                    targetSelectedImage = items[selectedIdx];
                    console.log('Setting selected image to:', targetSelectedImage);
                } else if (items.length > 0) {
                    // If live camera is currently active, keep it selected (but don't put it in menu)
                    if (isLiveCameraActive) {
                        targetSelectedImage = 'LIVE_CAMERA';
                    } else {
                        // Default to first regular image
                        targetSelectedImage = items[0];
                    }
                    console.log('Defaulting to selected image:', targetSelectedImage);
                }
                
                // Only update if different to prevent unnecessary re-renders
                if (selectedImage !== targetSelectedImage) {
                    setSelectedImage(targetSelectedImage);
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
    }, [json, isLiveCameraActive]); // Simplified dependencies

    // Stop live camera if scene changes to one that doesn't support it
    useEffect(() => {
        if (isLiveCameraActive && !isLiveCameraSupported()) {
            console.log('[ImagePicker] Scene changed to one that does not support live camera, stopping...');
            const cameraManager = getLiveCameraManager();
            if (cameraManager) {
                cameraManager.stop();
            }
            destroyLiveCameraManager();
            setIsLiveCameraActive(false);
            
            // Switch to first regular image if available
            if (menuItems.length > 0) {
                // Don't call handleImageSelect to avoid navigation, just update state
                setSelectedImage(menuItems[0]);
                if (window.module) {
                    window.module.update_source_name(menuItems[0]);
                    onChange(menuItems[0]);
                }
            }
        }
    }, [currentSceneIndex, isLiveCameraActive, menuItems, onChange]);

    // Handle live camera toggle
    const toggleLiveCamera = async (facingMode = getDefaultFacingMode()) => {
        try {
            setLiveCameraError(null);
            
            // Check if live camera is supported for current scene
            if (!isLiveCameraSupported()) {
                setLiveCameraError('Live camera is not supported for the current scene');
                return;
            }
            
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
                // Start live camera with mobile-specific default
                const cameraManager = createLiveCameraManager();
                await cameraManager.start(facingMode);
                setIsLiveCameraActive(true);
                setSelectedImage('LIVE_CAMERA');
                
                // Update camera info
                const info = cameraManager.getCurrentCameraInfo();
                setLiveCameraInfo(info);
                
                console.log('[ImagePicker] Live camera started with facing mode:', facingMode);
                
                // Navigate to home immediately when starting live camera
                setTimeout(() => {
                    setActivePane("home");
                    console.log("Navigating to home pane after starting live camera");
                }, 100);
            }
        } catch (error) {
            console.error('[ImagePicker] Live camera error:', error);
            setLiveCameraError(`Live camera failed: ${error.message}`);
            setIsLiveCameraActive(false);
        }
    };

    // Switch camera (front/back toggle)
    const switchLiveCamera = async () => {
        try {
            setIsSwitchingCamera(true);
            setLiveCameraError(null);
            
            const cameraManager = getLiveCameraManager();
            if (cameraManager && isLiveCameraActive) {
                const newFacingMode = await cameraManager.switchCamera();
                const info = cameraManager.getCurrentCameraInfo();
                setLiveCameraInfo(info);
                
                console.log('[ImagePicker] Camera switched to:', newFacingMode);
            }
        } catch (error) {
            console.error('[ImagePicker] Camera switch failed:', error);
            setLiveCameraError(`Camera switch failed: ${error.message}`);
        } finally {
            setIsSwitchingCamera(false);
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
                setIsLiveCameraActive(false);
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

    // Handle camera capture (still photo)
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
                setIsLiveCameraActive(false);
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
        
        // Live camera is no longer handled here since it's moved to a separate button
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

    // Show camera interface for still photos
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

            {/* Live Camera Status with Controls */}
            {isLiveCameraActive && (
                <Box sx={{ mb: 2, display: 'flex', alignItems: 'center', gap: 1, flexWrap: 'wrap' }}>
                    <Chip 
                        icon={<Radio size={16} />}
                        label={`Live Camera: ${liveCameraInfo.isFrontCamera ? 'Front' : 'Back'}`}
                        color="success"
                        variant="outlined"
                        size="small"
                        sx={{ 
                            fontWeight: 'bold',
                            transition: 'none' // Remove transition to prevent flashing
                        }}
                    />
                    
                    {/* Camera Switch Button - Only show on mobile */}
                    {isMobile && liveCameraInfo.availableCameras.length > 1 && (
                        <Button
                            size="small"
                            variant="outlined"
                            startIcon={isSwitchingCamera ? <CircularProgress size={16} /> : <RotateCcw size={16} />}
                            onClick={switchLiveCamera}
                            disabled={isSwitchingCamera}
                            sx={{ 
                                minWidth: 'auto', 
                                px: 1,
                                transition: 'opacity 0.2s ease' // Only animate opacity
                            }}
                        >
                            {isSwitchingCamera ? 'Switching...' : (liveCameraInfo.isFrontCamera ? 'Switch to Back' : 'Switch to Front')}
                        </Button>
                    )}
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
                        No images available. Upload an image or take a photo to get started.
                    </Typography>
                </Box>
            )}

            {/* Upload and Camera section */}
            <Divider sx={{ my: 2 }} />

            <Box sx={{ position: 'relative' }}>
                {/* Camera and Upload buttons */}
                <Box sx={{ display: 'flex', gap: 1, mb: 2 }}>
                    {/* Live Camera button - only show if supported */}
                    {isCameraSupported() && isLiveCameraSupported() && (
                        <Tooltip title={isLiveCameraActive ? "Stop Live Camera" : "Start Live Camera"}>
                            <IconButton
                                onClick={() => toggleLiveCamera()}
                                disabled={isLoading}
                                sx={{
                                    bgcolor: isLiveCameraActive ? '#4caf50' : '#ff5722',
                                    color: 'white',
                                    borderRadius: 1,
                                    width: 48,
                                    height: 48,
                                    '&:hover': {
                                        bgcolor: isLiveCameraActive ? '#45a049' : '#e64a19',
                                    },
                                    '&:disabled': {
                                        bgcolor: 'grey.300',
                                    }
                                }}
                            >
                                {isLiveCameraActive ? <Radio size={20} /> : <Play size={20} />}
                            </IconButton>
                        </Tooltip>
                    )}

                    {/* Still Camera button */}
                    {isCameraSupported() && (
                        <Tooltip title="Take Photo">
                            <IconButton
                                onClick={() => setShowCamera(true)}
                                disabled={isLoading}
                                sx={{
                                    bgcolor: 'primary.main',
                                    color: 'white',
                                    borderRadius: 1,
                                    width: 48,
                                    height: 48,
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
                                borderRadius: 1,
                                width: 48,
                                height: 48,
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
                        <Box sx={{ 
                            width: 24, 
                            height: 24, 
                            borderRadius: 0.5, 
                            bgcolor: 'action.disabled', 
                            display: 'flex', 
                            alignItems: 'center', 
                            justifyContent: 'center' 
                        }}>
                            <ImagePlus size={16} />
                        </Box>
                        {isCameraSupported() && (
                            <Box sx={{ 
                                width: 24, 
                                height: 24, 
                                borderRadius: 0.5, 
                                bgcolor: 'action.disabled', 
                                display: 'flex', 
                                alignItems: 'center', 
                                justifyContent: 'center' 
                            }}>
                                <Camera size={16} />
                            </Box>
                        )}
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