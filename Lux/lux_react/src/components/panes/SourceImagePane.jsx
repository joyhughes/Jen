import React, { useEffect, useState, useRef } from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';
import Alert from '@mui/material/Alert';
import { useTheme } from '@mui/material/styles';
import useMediaQuery from '@mui/material/useMediaQuery';
import WidgetGroup from '../WidgetGroup';
import MasonryImagePicker from '../MasonryImagePicker';

function SourceImagePane({ dimensions, panelSize, panelJSON, activeGroups, onWidgetGroupChange }) {
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
        console.log(`SourceImagePane: Panel size ${panelSize}px, Container width ${containerWidth}px, Controls width ${controlsWidth}px, Wide layout: ${isWideLayout}`);
    }, [panelSize, containerWidth, controlsWidth, isWideLayout]);

    // Look for any source-related group name
    const sourceImageGroup = activeGroups.find(group =>
        group.name === 'SOURCE_IMAGE_GROUP' ||
        group.name === 'source' ||
        group.name === 'target' ||
        group.name.toLowerCase().includes('source') ||
        group.name.toLowerCase().includes('image')
    );

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
                selectedGroup: sourceImageGroup ? sourceImageGroup.name : null
            });
        }
    }, [activeGroups, sourceImageGroup]);

    // Find the image picker widget JSON if it exists
    const imagePickerJson = sourceImageGroup?.widgets
        ?.map(widgetName => {
            const widgetJSON = window.module?.get_widget_JSON(widgetName);
            try {
                const widget = JSON.parse(widgetJSON);
                return widget;
            } catch (error) {
                console.error(`Error parsing widget JSON for ${widgetName}:`, error);
                return null;
            }
        })
        .find(widget =>
            widget?.tool === 'image' ||
            (widget?.type === 'menu_string' && widget?.items && Array.isArray(widget.items))
        );

    // Filter out the image picker widget to avoid showing it twice
    const nonImagePickerWidgets = sourceImageGroup?.widgets?.filter(widgetName => {
        const widgetJSON = window.module?.get_widget_JSON(widgetName);
        try {
            const widget = JSON.parse(widgetJSON);
            return !(widget?.tool === 'image' ||
                (widget?.type === 'menu_string' && widget?.items && Array.isArray(widget.items)));
        } catch (error) {
            return true;
        }
    }) || [];

    const customSourceImageGroup = sourceImageGroup ? {
        ...sourceImageGroup,
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
        if (!customSourceImageGroup || !imagePickerJson) return 0.5;

        // Count widgets to determine space distribution
        const widgetCount = customSourceImageGroup.widgets.length;
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
                    <MasonryImagePicker
                        json={imagePickerJson}
                        width="100%"
                        onChange={onWidgetGroupChange}
                    />
                </Box>
            )}

            {/* Effect Controls Section */}
            {customSourceImageGroup && customSourceImageGroup.widgets.length > 0 && (
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
                        key={customSourceImageGroup.name}
                        panelSize={controlsWidth || (shouldUseSideBySide ? containerWidth * controlsRatio : containerWidth)}
                        json={customSourceImageGroup}
                        onChange={onWidgetGroupChange}
                    />
                </Box>
            )}

            {/* Show errors and empty states */}
            {sourceImageGroup && !imagePickerJson && (
                <Alert severity="warning" sx={{ mb: 2 }}>
                    Source image group found, but no image picker widget detected.
                </Alert>
            )}

            {!sourceImageGroup && (
                <Box sx={{ textAlign: 'center', py: 4, color: 'text.secondary' }}>
                    <Typography>
                        No source image controls available. Please check your scene configuration.
                    </Typography>
                </Box>
            )}
        </Box>
    );
}

export default SourceImagePane;