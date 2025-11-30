import React, { useState, useRef, useEffect } from 'react';
import { Box, Typography, CircularProgress, Divider, Alert, useTheme, IconButton, Tooltip, Chip, Button } from '@mui/material';
import { ImagePlus, Camera, Video, VideoOff, FlipHorizontal, RotateCcw, Play, Radio } from 'lucide-react';
import ThumbnailItem from './ThumbnailItem';
import CameraCapture from './CameraCapture';
import { createLiveCameraManager, getLiveCameraManager, destroyLiveCameraManager } from './LiveCameraManager';
import { useScene } from './SceneContext';
import { isMobileDevice } from '../utils/cameraUtils';
import '../styles/MasonryImagePicker.css';

export const MasonryImagePicker = ({ json, updateFuncName, width, onChange}) => {
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

    const isMobile = isMobileDevice();

    const getDefaultFacingMode = () => {
        return isMobile ? 'environment' : 'user';
    };


    const isLiveCameraSupported = () => {
        return cachedLiveCameraSupported;
    };

    useEffect(() => {
        const fetchLiveCameraSupport = async () => {
            console.log('Checking live camera support for scene index:', currentSceneIndex);
            
            await new Promise(resolve => setTimeout(resolve, 200));
            
            if (window.module && window.module.is_live_camera_supported) {
                const supported = window.module.is_live_camera_supported();
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

        fetchLiveCameraSupport();
    }, [currentSceneIndex]);

    useEffect(() => {
        if (isLiveCameraActive) {
            const updateCameraInfo = () => {
                const cameraManager = getLiveCameraManager();
                if (cameraManager) {
                    const info = cameraManager.getCurrentCameraInfo();
                    if (liveCameraInfo.facingMode !== info.facingMode || 
                        liveCameraInfo.availableCameras.length !== info.availableCameras.length) {
                        setLiveCameraInfo(info);
                    }
                }
            };

            updateCameraInfo();
            const interval = setInterval(updateCameraInfo, 3000); 
            return () => clearInterval(interval);
        }
    }, [isLiveCameraActive]);

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

                items = items.filter(item => item !== 'ultra_camera');

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

                let targetSelectedImage = '';
                if (selectedIdx >= 0 && selectedIdx < items.length) {
                    targetSelectedImage = items[selectedIdx];
                    console.log('Setting selected image to:', targetSelectedImage);
                } else if (items.length > 0) {
                    if (isLiveCameraActive) {
                        targetSelectedImage = 'LIVE_CAMERA';
                    } else {
                        targetSelectedImage = items[0];
                    }
                    console.log('Defaulting to selected image:', targetSelectedImage);
                }
                
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
    }, [json, isLiveCameraActive]); 

    useEffect(() => {
        if (isLiveCameraActive && !isLiveCameraSupported()) {
            console.log('[ImagePicker] Scene changed to one that does not support live camera, stopping...');
            const cameraManager = getLiveCameraManager();
            if (cameraManager) {
                cameraManager.stop();
            }
            destroyLiveCameraManager();
            setIsLiveCameraActive(false);
            
            if (menuItems.length > 0) {
                setSelectedImage(menuItems[0]);
                if (window.module) {
                    window.module.update_chosen_image(updateFuncName, menuItems[0]);
                    onChange(menuItems[0]);
                }
            }
        }
    }, [currentSceneIndex, isLiveCameraActive, menuItems, onChange]);

    const toggleLiveCamera = async (facingMode = getDefaultFacingMode()) => {
        try {
            setLiveCameraError(null);
            
            if (!isLiveCameraSupported()) {
                setLiveCameraError('Live camera is not supported for the current scene');
                return;
            }
            
            if (isLiveCameraActive) {
                const cameraManager = getLiveCameraManager();
                if (cameraManager) {
                    cameraManager.stop();
                }
                destroyLiveCameraManager();
                setIsLiveCameraActive(false);
                console.log('[ImagePicker] Live camera stopped');
                
                const regularImages = menuItems.filter(item => item !== 'LIVE_CAMERA');
                if (regularImages.length > 0) {
                    handleImageSelect(regularImages[0]);
                }
            } else {
                const cameraManager = createLiveCameraManager();
                await cameraManager.start(facingMode);
                setIsLiveCameraActive(true);
                setSelectedImage('LIVE_CAMERA');
                
                const info = cameraManager.getCurrentCameraInfo();
                setLiveCameraInfo(info);
                
                console.log('[ImagePicker] Live camera started with facing mode:', facingMode);
                
                console.log("Live camera started, staying on current pane");
            }
        } catch (error) {
            console.error('[ImagePicker] Live camera error:', error);
            setLiveCameraError(`Live camera failed: ${error.message}`);
            setIsLiveCameraActive(false);
        }
    };

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

    const handleFileUpload = async (file) => {
        try {
            setIsLoading(true);
            
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

                    const newItems = [...menuItems];
                    if (!newItems.includes(imageName)) {
                        newItems.push(imageName);
                        setMenuItems(newItems);
                        setNewImageName(imageName); 

                        setTimeout(() => {
                            setNewImageName(null);
                        }, 1000);
                    }

                    setSelectedImage(imageName);
                    window.module.update_chosen_image(updateFuncName, imageName);
                    onChange(imageName);
                }
                setIsLoading(false);
            };
            reader.readAsArrayBuffer(file);

            console.log("Image uploaded, staying on current pane");
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
            
            const { imageName  } = captureData;
            
            if (imageName) {
                const newItems = [...menuItems];
                if (!newItems.includes(imageName)) {
                    newItems.push(imageName);
                    setMenuItems(newItems);
                    setNewImageName(imageName); 

                    setTimeout(() => {
                        setNewImageName(null);
                    }, 1000);
                }

                setSelectedImage(imageName);
                onChange(imageName);
                
                console.log('Camera capture processed successfully:', imageName);
            }
            
            setIsLoading(false);
            
            setShowCamera(false);
            console.log("Camera capture completed, staying on current pane");
            
        } catch (error) {
            console.error('Failed to process camera capture:', error);
            setError('Failed to process camera capture: ' + error.message);
            setIsLoading(false);
        }
    };

    const handleImageSelect = (imageName) => {
        console.log('Image selected:', imageName);
        
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
            console.log('select new image: ' + imageName)
            window.module.update_chosen_image(updateFuncName, imageName);
            onChange(imageName);
        }
        console.log("Image selected, staying on current pane");
    };

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

    const isCameraSupported = () => {
        return !!(navigator.mediaDevices && navigator.mediaDevices.getUserMedia);
    };

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
                <Box sx={{ 
                    mb: 2, 
                    p: 2,
                    bgcolor: 'rgba(244, 67, 54, 0.1)',
                    borderRadius: 2,
                    border: '2px solid #f44336',
                    display: 'flex', 
                    alignItems: 'center', 
                    justifyContent: 'space-between',
                    flexWrap: 'wrap',
                    gap: 1.5
                }}>
                    <Chip 
                        icon={<Radio size={18} />}
                        label={`LIVE ${liveCameraInfo.isFrontCamera ? 'FRONT' : 'BACK'} CAMERA`}
                        color="error"
                        variant="filled"
                        size="medium"
                        sx={{ 
                            fontWeight: 'bold',
                            fontSize: '0.85rem',
                            letterSpacing: '0.5px',
                            animation: 'pulse 2s infinite',
                            '@keyframes pulse': {
                                '0%': { opacity: 1 },
                                '50%': { opacity: 0.7 },
                                '100%': { opacity: 1 }
                            },
                            transition: 'none'
                        }}
                    />
                    
                    <Box sx={{ display: 'flex', gap: 1 }}>
                        {/* Camera Switch Button - Show when multiple cameras available */}
                        {liveCameraInfo.availableCameras.length > 1 && (
                            <Button
                                size="medium"
                                variant="contained"
                                color="primary"
                                startIcon={isSwitchingCamera ? <CircularProgress size={18} /> : <FlipHorizontal size={18} />}
                                onClick={switchLiveCamera}
                                disabled={isSwitchingCamera}
                                sx={{ 
                                    minWidth: { xs: 120, sm: 'auto' },
                                    px: { xs: 2, sm: 2 },
                                    py: { xs: 1, sm: 0.75 },
                                    fontSize: { xs: '0.9rem', sm: '0.875rem' },
                                    borderRadius: 2,
                                    textTransform: 'none',
                                    fontWeight: 600,
                                    boxShadow: 2,
                                    '&:hover': {
                                        boxShadow: 3,
                                        transform: 'translateY(-1px)'
                                    },
                                    transition: 'all 0.2s ease'
                                }}
                            >
                                {isSwitchingCamera ? 'Switching...' : `${liveCameraInfo.isFrontCamera ? '📱' : '📷'} Switch`}
                            </Button>
                        )}
                        
                        {/* Stop Live Camera Button */}
                        <Button
                            size="medium"
                            variant="contained"
                            color="error"
                            startIcon={<VideoOff size={18} />}
                            onClick={() => toggleLiveCamera()}
                            sx={{ 
                                minWidth: { xs: 100, sm: 'auto' },
                                px: { xs: 2, sm: 2 },
                                py: { xs: 1, sm: 0.75 },
                                fontSize: { xs: '0.9rem', sm: '0.875rem' },
                                borderRadius: 2,
                                textTransform: 'none',
                                fontWeight: 600,
                                boxShadow: 2,
                                '&:hover': {
                                    boxShadow: 3,
                                    transform: 'translateY(-1px)',
                                    bgcolor: '#d32f2f'
                                },
                                transition: 'all 0.2s ease'
                            }}
                        >
                            Stop
                        </Button>
                    </Box>
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
                <Box sx={{ display: 'flex', gap: 1.5, mb: 2, alignItems: 'center', flexWrap: 'wrap' }}>
                    {/* Live Camera button - only show if supported */}
                    {isCameraSupported() && isLiveCameraSupported() && (
                        <Tooltip title={isLiveCameraActive ? "Live Camera Active - Click to Stop" : `Start Live Camera (${isMobile ? 'Back' : 'Front'} Camera)`}>
                            <IconButton
                                onClick={() => toggleLiveCamera()}
                                disabled={isLoading}
                                sx={{
                                    bgcolor: isLiveCameraActive ? '#4caf50' : '#ff5722',
                                    color: 'white',
                                    borderRadius: 2,
                                    width: { xs: 56, sm: 48 },
                                    height: { xs: 56, sm: 48 },
                                    position: 'relative',
                                    boxShadow: 3,
                                    '&:hover': {
                                        bgcolor: isLiveCameraActive ? '#45a049' : '#e64a19',
                                        transform: 'translateY(-2px)',
                                        boxShadow: 4,
                                    },
                                    '&:disabled': {
                                        bgcolor: 'grey.300',
                                        color: 'grey.500',
                                    },
                                    '&:active': {
                                        transform: 'translateY(0)',
                                    },
                                    transition: 'all 0.2s ease',
                                    // Add pulsing animation when active
                                    ...(isLiveCameraActive && {
                                        animation: 'livePulse 2s infinite',
                                        '@keyframes livePulse': {
                                            '0%': { boxShadow: '0 4px 8px rgba(76, 175, 80, 0.3)' },
                                            '50%': { boxShadow: '0 4px 20px rgba(76, 175, 80, 0.6)' },
                                            '100%': { boxShadow: '0 4px 8px rgba(76, 175, 80, 0.3)' }
                                        }
                                    })
                                }}
                            >
                                {isLiveCameraActive ? <Radio size={24} /> : <Video size={24} />}
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
                                    borderRadius: 2,
                                    width: { xs: 56, sm: 48 },
                                    height: { xs: 56, sm: 48 },
                                    boxShadow: 2,
                                    '&:hover': {
                                        bgcolor: 'primary.dark',
                                        transform: 'translateY(-1px)',
                                        boxShadow: 3,
                                    },
                                    '&:disabled': {
                                        bgcolor: 'grey.300',
                                        color: 'grey.500',
                                    },
                                    '&:active': {
                                        transform: 'translateY(0)',
                                    },
                                    transition: 'all 0.2s ease'
                                }}
                            >
                                <Camera size={24} />
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
                                borderRadius: 2,
                                width: { xs: 56, sm: 48 },
                                height: { xs: 56, sm: 48 },
                                boxShadow: 2,
                                '&:hover': {
                                    bgcolor: 'secondary.dark',
                                    transform: 'translateY(-1px)',
                                    boxShadow: 3,
                                },
                                '&:disabled': {
                                    bgcolor: 'grey.300',
                                    color: 'grey.500',
                                },
                                '&:active': {
                                    transform: 'translateY(0)',
                                },
                                transition: 'all 0.2s ease'
                            }}
                        >
                            <ImagePlus size={24} />
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