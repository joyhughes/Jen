import React, { useState, useRef, useEffect, useCallback, useMemo } from 'react';
import {
    Box,
    Typography,
    Divider,
    Stack,
    Button,
    FormControl,
    InputLabel,
    Select,
    MenuItem,
    Alert,
    Card,
    CardContent,
    Switch,
    FormControlLabel,
    ButtonGroup,
    Chip
} from '@mui/material';
import { BiImage, BiCamera, BiRefresh } from 'react-icons/bi';
import MasonryImagePicker from '../MasonryImagePicker';
import WidgetGroup from '../WidgetGroup';
import CameraCapture from '../CameraCapture';
import { usePane } from './PaneContext';
import { FaHome } from 'react-icons/fa';

function TargetPane({activeGroups, onWidgetGroupChange }) {
    const { setActivePane } = usePane();
    
    const containerRef = useRef(null);
    const controlsRef = useRef(null);
    const [targetType, setTargetType] = useState('image'); 
    const [selectedImagePickerId, setSelectedImagePickerId] = useState('');
    const [showCameraCapture, setShowCameraCapture] = useState(false);
    const [cameraTargetActive, setCameraTargetActive] = useState(false);

    const targetGroups = activeGroups.filter(group => {
        return group && group.name.includes("image")
    });

    const targetImagePickers = targetGroups.flatMap(group => 
        group.widgets?.filter(widgetName => {
            try {
                const widgetJSON = window.module?.get_widget_JSON(widgetName);
                const widget = JSON.parse(widgetJSON);
                return widget?.tool === 'image' || 
                       (widget?.tool === 'pull_down' && widgetName.toLowerCase().includes('target'));
            } catch (error) {
                return false;
            }
        }).map(widgetName => ({
            widgetName,
            groupName: group.name,
            displayName: widgetName.replace(/_/g, ' ').replace(/\b\w/g, l => l.toUpperCase())
        })) || []
    );

    // Find target switch widget
    const targetSwitchWidget = targetGroups.flatMap(group => group.widgets || [])
        .find(widgetName => {
            try {
                const widgetJSON = window.module?.get_widget_JSON(widgetName);
                const widget = JSON.parse(widgetJSON);
                return widget?.tool === 'switch' && widgetName.toLowerCase().includes('target');
            } catch (error) {
                return false;
            }
        });

    // Update selected picker when available pickers change
    useEffect(() => {
        if (targetImagePickers.length > 0 && !targetImagePickers.find(p => p.widgetName === selectedImagePickerId)) {
            setSelectedImagePickerId(targetImagePickers[0].widgetName);
        }
    }, [targetImagePickers, selectedImagePickerId]);

    // Find the currently selected image group and picker
    const selectedPicker = targetImagePickers.find(p => p.widgetName === selectedImagePickerId);
    const imageGroup = selectedPicker ? targetGroups.find(group => group.name === selectedPicker.groupName) : targetGroups[0];

    // Get the widget JSON for the selected image picker
    const imagePickerJson = useMemo(() => {
        if (!selectedPicker) return null;
        
        try {
            const widgetJSON = window.module?.get_widget_JSON(selectedPicker.widgetName);
            const widget = JSON.parse(widgetJSON);
            return widget;
        } catch (error) {
            console.error(`Error parsing widget JSON for ${selectedPicker.widgetName}:`, error);
            return null;
        }
    }, [selectedPicker]);

    // Create a custom group for effect controls (non-image widgets in target groups)
    const customTargetGroup = useMemo(() => {
        if (!imageGroup) return null;
        
        const nonImageWidgets = imageGroup.widgets?.filter(widgetName => {
            try {
                const widgetJSON = window.module?.get_widget_JSON(widgetName);
                const widget = JSON.parse(widgetJSON);
                return widget?.tool !== 'image' && widget?.tool !== 'pull_down';
            } catch (error) {
                return false;
            }
        }) || [];

        if (nonImageWidgets.length === 0) return null;

        return {
            ...imageGroup,
            widgets: nonImageWidgets
        };
    }, [imageGroup]);

    // Responsive layout calculation
    useEffect(() => {
        if (!containerRef.current) return;

        const updateLayout = () => {
            const containerElement = containerRef.current;
            if (!containerElement) return;

            const width = containerElement.clientWidth;
            setContainerWidth(width);
            
            // Use side-by-side layout for wider screens when we have controls
            const hasSeparateControls = customTargetGroup && customTargetGroup.widgets.length > 0;
            const useSideBySide = width > 900 && hasSeparateControls;
            setShouldUseSideBySide(useSideBySide);
            
            if (useSideBySide && controlsRef.current) {
                setControlsWidth(controlsRef.current.clientWidth);
            } else {
                setControlsWidth(null);
            }
        };

        updateLayout();
        window.addEventListener('resize', updateLayout);
        return () => window.removeEventListener('resize', updateLayout);
    }, [customTargetGroup]);

    // Handle target type switching
    const handleTargetTypeChange = useCallback((newType) => {
        setTargetType(newType);
        
        // Update target switch widget if available
        if (targetSwitchWidget && window.module?.set_widget_value) {
            try {
                // Enable targeting for image and camera, disable for self
                const enableTargeting = newType !== 'self';
                window.module.set_widget_value(targetSwitchWidget, enableTargeting ? 1 : 0);
                
                // Trigger widget change callback
                if (onWidgetGroupChange) {
                    onWidgetGroupChange(targetSwitchWidget, enableTargeting);
                }
            } catch (error) {
                console.error('Error updating target switch:', error);
            }
        }

        // Handle camera-specific logic
        if (newType === 'camera') {
            setCameraTargetActive(true);
        } else {
            setCameraTargetActive(false);
            setShowCameraCapture(false);
        }
    }, [targetSwitchWidget, onWidgetGroupChange]);

    // Handle camera capture
    const handleCameraCapture = useCallback((imageData) => {
        if (window.module?.update_camera_target) {
            try {
                // Update the camera target in the backend
                window.module.update_camera_target(imageData);
                console.log('Camera target updated');
            } catch (error) {
                console.error('Error updating camera target:', error);
            }
        }
        setShowCameraCapture(false);
    }, []);

    // Get current target info for display
    const getCurrentTargetInfo = () => {
        switch (targetType) {
            case 'image':
                if (selectedPicker && imagePickerJson) {
                    const activeIndex = imagePickerJson.active_index || 0;
                    const activeItem = imagePickerJson.items?.[activeIndex];
                    return {
                        type: 'Image Target',
                        name: activeItem || 'No image selected',
                        icon: <BiImage />
                    };
                }
                return { type: 'Image Target', name: 'No picker available', icon: <BiImage /> };
            
            case 'self':
                return { 
                    type: 'Self Target', 
                    name: 'CA evolves toward its own state', 
                    icon: <BiRefresh /> 
                };
            
            case 'camera':
                return { 
                    type: 'Camera Target', 
                    name: cameraTargetActive ? 'Live camera feed' : 'Camera not active', 
                    icon: <BiCamera /> 
                };
            
            default:
                return { type: 'Unknown', name: 'Invalid target type', icon: <BiTarget /> };
        }
    };

    const currentTarget = getCurrentTargetInfo();

    return (
        <Box
            ref={containerRef}
            sx={{
                display: 'flex',
                flexDirection: shouldUseSideBySide ? 'row' : 'column',
                height: '100%',
                overflowY: 'auto',
                gap: 2,
                width: '100%'
            }}
        >
            {/* Target Type Selector */}
            <Box sx={{ mb: 2, width: '100%' }}>
                <Typography variant="h6" sx={{ mb: 2, display: 'flex', alignItems: 'center', gap: 1 }}>
                    <FaHome /> Target Selection for Cellular Automata
                </Typography>
                
                {/* Current Target Status */}
                <Card variant="outlined" sx={{ mb: 2 }}>
                    <CardContent sx={{ py: 1.5 }}>
                        <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
                            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                                {currentTarget.icon}
                                <Typography variant="body2" sx={{ fontWeight: 500 }}>
                                    {currentTarget.type}
                                </Typography>
                                <Chip 
                                    label={currentTarget.name} 
                                    size="small" 
                                    variant="outlined"
                                    sx={{ maxWidth: 200 }}
                                />
                            </Box>
                            
                            {/* Target Switch */}
                            {targetSwitchWidget && (
                                <FormControlLabel
                                    control={
                                        <Switch
                                            checked={targetType !== 'self'}
                                            onChange={(e) => {
                                                // This will be handled by the target type buttons
                                            }}
                                            size="small"
                                        />
                                    }
                                    label="Enable Targeting"
                                    sx={{ ml: 2 }}
                                />
                            )}
                        </Box>
                    </CardContent>
                </Card>

                {/* Target Type Buttons */}
                <ButtonGroup variant="outlined" sx={{ mb: 2, width: '100%' }}>
                    <Button
                        startIcon={<BiImage />}
                        onClick={() => handleTargetTypeChange('image')}
                        variant={targetType === 'image' ? 'contained' : 'outlined'}
                        sx={{ flex: 1 }}
                    >
                        Image Target
                    </Button>
                    <Button
                        startIcon={<BiRefresh />}
                        onClick={() => handleTargetTypeChange('self')}
                        variant={targetType === 'self' ? 'contained' : 'outlined'}
                        sx={{ flex: 1 }}
                    >
                        Self Target
                    </Button>
                    <Button
                        startIcon={<BiCamera />}
                        onClick={() => handleTargetTypeChange('camera')}
                        variant={targetType === 'camera' ? 'contained' : 'outlined'}
                        sx={{ flex: 1 }}
                    >
                        Camera Target
                    </Button>
                </ButtonGroup>
            </Box>

            {/* Target Content Based on Type */}
            {targetType === 'image' && (
                <>
                    {/* Image Target Picker Selector */}
                    {targetImagePickers.length > 1 && (
                        <Box sx={{ mb: 2 }}>
                            <FormControl size="small" sx={{ minWidth: 200, maxWidth: 250 }}>
                                <InputLabel id="target-image-picker-select-label">Target Image Picker</InputLabel>
                                <Select
                                    labelId="target-image-picker-select-label"
                                    value={selectedImagePickerId}
                                    label="Target Image Picker"
                                    onChange={(e) => setSelectedImagePickerId(e.target.value)}
                                >
                                    {targetImagePickers.map((picker) => (
                                        <MenuItem key={picker.widgetName} value={picker.widgetName}>
                                            <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-start' }}>
                                                <Typography variant="body2" sx={{ fontWeight: 500 }}>
                                                    {picker.displayName}
                                                </Typography>
                                                <Typography variant="caption" sx={{ color: 'text.secondary' }}>
                                                    {picker.groupName}
                                                </Typography>
                                            </Box>
                                        </MenuItem>
                                    ))}
                                </Select>
                            </FormControl>
                        </Box>
                    )}

                    {/* Image Picker Section */}
                    {imagePickerJson && (
                        <Box
                            sx={{
                                flex: shouldUseSideBySide ? `0 0 ${imageRatio * 100}%` : '1 0 auto',
                                width: shouldUseSideBySide ? `${imageRatio * 100}%` : '100%',
                            }}
                        >
                            <MasonryImagePicker
                                updateFuncName={"target_image_menu"}
                                setActivePane={setActivePane}
                                json={imagePickerJson}
                                width="100%"
                                onChange={onWidgetGroupChange}
                            />
                        </Box>
                    )}
                </>
            )}

            {targetType === 'self' && (
                <Box sx={{ p: 3, textAlign: 'center' }}>
                    <BiRefresh size={48} style={{ color: '#666', marginBottom: 16 }} />
                    <Typography variant="h6" sx={{ mb: 2 }}>
                        Self-Targeting Enabled
                    </Typography>
                    <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
                        The cellular automata will use its own current state as the target,
                        creating self-referential evolution patterns and autonomous development.
                    </Typography>
                    <Alert severity="info" sx={{ mt: 2 }}>
                        In self-targeting mode, the CA creates recursive feedback loops 
                        that can generate complex emergent patterns without external guidance.
                    </Alert>
                </Box>
            )}

            {targetType === 'camera' && (
                <Box>
                    {!showCameraCapture ? (
                        <Box sx={{ p: 3, textAlign: 'center' }}>
                            <BiCamera size={48} style={{ color: '#666', marginBottom: 16 }} />
                            <Typography variant="h6" sx={{ mb: 2 }}>
                                Camera Target
                            </Typography>
                            <Typography variant="body2" color="text.secondary" sx={{ mb: 3 }}>
                                Use live camera feed as the target for cellular automata evolution.
                                The CA will evolve toward matching the real-time camera input.
                            </Typography>
                            <Button
                                variant="contained"
                                startIcon={<BiCamera />}
                                onClick={() => setShowCameraCapture(true)}
                                size="large"
                            >
                                Activate Camera Target
                            </Button>
                            {cameraTargetActive && (
                                <Alert severity="success" sx={{ mt: 2 }}>
                                    Camera target is active and feeding live data to the CA system.
                                </Alert>
                            )}
                        </Box>
                    ) : (
                        <CameraCapture
                            onCapture={handleCameraCapture}
                            onClose={() => setShowCameraCapture(false)}
                            enableLivePreview={true}
                            autoSaveToGrid={false}
                        />
                    )}
                </Box>
            )}

            {/* Effect Controls Section */}
            {customTargetGroup && customTargetGroup.widgets.length > 0 && (
                <Box
                    ref={controlsRef}
                    sx={{
                        flex: shouldUseSideBySide ? `0 0 ${controlsRatio * 100}%` : '0 0 auto',
                        width: shouldUseSideBySide ? `${controlsRatio * 100}%` : '100%',
                        backgroundColor: 'background.paper',
                        borderRadius: 1,
                        p: 2,
                        border: '1px solid',
                        borderColor: 'divider'
                    }}
                >
                    <Typography variant="subtitle1" sx={{ mb: 1 }}>
                        Target Controls
                    </Typography>

                    <WidgetGroup
                        key={customTargetGroup.name}
                        panelSize={controlsWidth || (shouldUseSideBySide ? containerWidth * controlsRatio : containerWidth)}
                        json={customTargetGroup}
                        onChange={onWidgetGroupChange}
                    />
                </Box>
            )}

            {/* Show errors and empty states */}
            {targetType === 'image' && imageGroup && !imagePickerJson && (
                <Alert severity="warning" sx={{ mb: 2 }}>
                    Target group found, but no target image picker widget detected.
                </Alert>
            )}

            {targetGroups.length === 0 && (
                <Box sx={{ textAlign: 'center', py: 4, color: 'text.secondary' }}>
                    <Typography>
                        No target controls available. Please check your scene configuration for cellular automata.
                    </Typography>
                </Box>
            )}
        </Box>
    );
}

export default TargetPane;
