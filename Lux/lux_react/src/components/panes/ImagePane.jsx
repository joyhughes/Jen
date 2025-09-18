import React, { useEffect, useState, useRef } from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';
import Alert from '@mui/material/Alert';
import FormControl from '@mui/material/FormControl';
import InputLabel from '@mui/material/InputLabel';
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import { useTheme } from '@mui/material/styles';
import useMediaQuery from '@mui/material/useMediaQuery';
import WidgetGroup from '../WidgetGroup';
import MasonryImagePicker from '../MasonryImagePicker';
import {usePane} from "./PaneContext.jsx";

function ImagePane({ dimensions, panelSize, panelJSON, activeGroups, onWidgetGroupChange }) {
    const { setActivePane } = usePane();
    const [debugInfo, setDebugInfo] = useState({
        groups: [],
        imageWidgets: [],
        selectedGroup: null
    });

    const containerRef = useRef(null);
    const [containerWidth, setContainerWidth] = useState(0);
    const theme = useTheme();
    const isWideLayout = useMediaQuery(theme.breakpoints.up('md'));
    const controlsRef = useRef(null);
    const [controlsWidth, setControlsWidth] = useState(0);

    // Monitor the overall container width
    useEffect(() => {
        if (!containerRef.current) return;

        const resizeObserver = new ResizeObserver(entries => {
            for (let entry of entries) {
                setContainerWidth(entry.contentRect.width);
            }
        });

        resizeObserver.observe(containerRef.current);

        return () => {
            if (containerRef.current) {
                resizeObserver.unobserve(containerRef.current);
            }
        };
    }, []);

    // Monitor the controls area width separately
    useEffect(() => {
        if (!controlsRef.current) return;

        const resizeObserver = new ResizeObserver(entries => {
            for (let entry of entries) {
                setControlsWidth(entry.contentRect.width);
            }
        });

        resizeObserver.observe(controlsRef.current);

        return () => {
            if (controlsRef.current) {
                resizeObserver.unobserve(controlsRef.current);
            }
        };
    }, []);

    // Log panel size and measured width for debugging
    useEffect(() => {
        console.log(`ImagePane: Panel size ${panelSize}px, Container width ${containerWidth}px, Controls width ${controlsWidth}px, Wide layout: ${isWideLayout}`);
    }, [panelSize, containerWidth, controlsWidth, isWideLayout]);

    // Find all groups that contain widgets with "image" tool
    const imageGroups = activeGroups.filter(group => {
        if (!group.widgets) return false;
        
        return group.widgets.some(widgetName => {
            try {
                const widgetJSON = window.module?.get_widget_JSON(widgetName);
                const widget = JSON.parse(widgetJSON);
                return widget?.tool === 'image';
            } catch (error) {
                return false;
            }
        });
    });

    // Get all image picker widgets from all image groups
    const allImagePickers = imageGroups.flatMap(group => 
        group.widgets?.filter(widgetName => {
            try {
                const widgetJSON = window.module?.get_widget_JSON(widgetName);
                const widget = JSON.parse(widgetJSON);
                return widget?.tool === 'image';
            } catch (error) {
                return false;
            }
        }).map(widgetName => ({
            widgetName,
            groupName: group.name,
            displayName: widgetName.replace(/_/g, ' ').replace(/\b\w/g, l => l.toUpperCase())
        })) || []
    );

    // State for selected image picker
    const [selectedPickerId, setSelectedPickerId] = useState(allImagePickers[0]?.widgetName || '');

    // Update selected picker when image pickers change
    useEffect(() => {
        if (allImagePickers.length > 0 && !allImagePickers.find(p => p.widgetName === selectedPickerId)) {
            setSelectedPickerId(allImagePickers[0].widgetName);
        }
    }, [allImagePickers, selectedPickerId]);

    // Find the currently selected image group and picker
    const selectedPicker = allImagePickers.find(p => p.widgetName === selectedPickerId);
    const imageGroup = selectedPicker ? imageGroups.find(group => group.name === selectedPicker.groupName) : imageGroups[0];



    // Collect debug information
    useEffect(() => {
        if (window.module) {
            // Extract useful debug info
            const groupInfo = activeGroups.map(group => ({
                name: group.name,
                widgetCount: group.widgets ? group.widgets.length : 0
            }));

            // Find widgets that might be image pickers
            const imageWidgetsInfo = [];
            activeGroups.forEach(group => {
                if (group.widgets) {
                    group.widgets.forEach(widgetName => {
                        try {
                            const widgetJSON = window.module.get_widget_JSON(widgetName);
                            const widget = JSON.parse(widgetJSON);
                            if (widget && (widget.tool === 'image' || widget.type === 'menu_string')) {
                                imageWidgetsInfo.push({
                                    name: widgetName,
                                    group: group.name,
                                    type: widget.type,
                                    tool: widget.tool,
                                    items: widget.items ? widget.items.length : 0
                                });
                            }
                        } catch (error) {
                            console.error(`Error parsing widget JSON for ${widgetName}:`, error);
                        }
                    });
                }
            });

            setDebugInfo({
                groups: groupInfo,
                imageWidgets: imageWidgetsInfo,
                selectedGroup: imageGroup ? imageGroup.name : null
            });
        }
    }, [activeGroups, imageGroup]);

    // Find the selected image picker widget JSON
    const imagePickerJson = selectedPickerId ? (() => {
        try {
            const widgetJSON = window.module?.get_widget_JSON(selectedPickerId);
            const widget = JSON.parse(widgetJSON);
            return widget?.tool === 'image' ? widget : null;
        } catch (error) {
            console.error(`Error parsing widget JSON for ${selectedPickerId}:`, error);
            return null;
        }
    })() : null;

    // Filter out the image picker widget to avoid showing it twice
    const nonImagePickerWidgets = imageGroup?.widgets?.filter(widgetName => {
        const widgetJSON = window.module?.get_widget_JSON(widgetName);
        try {
            const widget = JSON.parse(widgetJSON);
            return !(widget?.tool === 'image' ||
                (widget?.type === 'menu_string' && widget?.items && Array.isArray(widget.items)));
        } catch (error) {
            return true;
        }
    }) || [];

    const customImageGroup = imageGroup ? {
        ...imageGroup,
        widgets: nonImagePickerWidgets
    } : null;

    // Dynamically determine the best layout based on container width
    // For wider screens, prefer side-by-side layout
    // Calculate the split ratio based on content
    const shouldUseSideBySide = isWideLayout && containerWidth > 600;

    // Calculate the optimal split ratio based on content
    // Give more space to whichever side has more widgets
    const getLayoutRatio = () => {
        // Default to 50/50 split
        if (!customImageGroup || !imagePickerJson) return 0.5;

        // Count widgets to determine space distribution
        const widgetCount = customImageGroup.widgets.length;
        const imageCount = imagePickerJson.items?.length || 0;

        // Adjust ratio based on content (min 0.35, max 0.65)
        const ratio = (widgetCount > imageCount * 2) ? 0.65 :
            (imageCount > widgetCount * 2) ? 0.35 : 0.5;

        return ratio;
    };

    const controlsRatio = getLayoutRatio();
    const imageRatio = 1 - controlsRatio;

    return (
        <Box
            ref={containerRef}
            sx={{
                display: 'flex',
                flexDirection: shouldUseSideBySide ? 'row' : 'column',
                padding: 1,
                height: '100%',
                overflowY: 'auto',
                gap: 2,
                width: '100%'
            }}
        >
            {/* Image Picker Section */}
            {imagePickerJson && (
                <Box
                    sx={{
                        flex: shouldUseSideBySide ? `0 0 ${imageRatio * 100}%` : '1 0 auto',
                        width: shouldUseSideBySide ? `${imageRatio * 100}%` : '100%',
                    }}
                >
                    {allImagePickers && allImagePickers.length >= 1 && (
                        <Box sx={{ 
                            mb: 2,
                            display: 'flex',
                            justifyContent: 'flex-start'
                        }}>
                            <FormControl size="small" sx={{ minWidth: 200, maxWidth: 250 }}>
                                <InputLabel id="image-picker-select-label">Image Picker</InputLabel>
                                <Select
                                    labelId="image-picker-select-label"
                                    value={selectedPickerId}
                                    label="Image Picker"
                                    onChange={(e) => setSelectedPickerId(e.target.value)}
                                >
                                    {allImagePickers.map((picker) => (
                                        <MenuItem key={picker.widgetName} value={picker.widgetName}>
                                            <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-start' }}>
                                                <Typography variant="body2" sx={{ fontWeight: 500 }}>
                                                    {picker.displayName}
                                                </Typography>
                                            </Box>
                                        </MenuItem>
                                    ))}
                                </Select>
                            </FormControl>
                        </Box>
                    )}

                    <MasonryImagePicker
                        setActivePane={setActivePane}
                        json={imagePickerJson}
                        width="100%"
                        onChange={onWidgetGroupChange}
                    />
                </Box>
            )}

            {/* Effect Controls Section */}
            {customImageGroup && customImageGroup.widgets.length > 0 && (
                <Box
                    ref={controlsRef}
                    sx={{
                        flex: shouldUseSideBySide ? `0 0 ${controlsRatio * 100}%` : '1 0 auto',
                        width: shouldUseSideBySide ? `${controlsRatio * 100}%` : '100%',
                    }}
                >
                    <Typography variant="subtitle1" sx={{ mb: 1 }}>
                        Effect Controls
                    </Typography>

                    <WidgetGroup
                        key={customImageGroup.name}
                        panelSize={controlsWidth || (shouldUseSideBySide ? containerWidth * controlsRatio : containerWidth)}
                        json={customImageGroup}
                        onChange={onWidgetGroupChange}
                    />
                </Box>
            )}

            {/* Show errors and empty states */}
            {imageGroup && !imagePickerJson && (
                <Alert severity="warning" sx={{ mb: 2 }}>
                    Image group found, but no image picker widget detected.
                </Alert>
            )}

            {imageGroups.length === 0 && (
                <Box sx={{ textAlign: 'center', py: 4, color: 'text.secondary' }}>
                    <Typography>
                        No image controls available. Please check your scene configuration.
                    </Typography>
                </Box>
            )}
        </Box>
    );
}

export default ImagePane;
