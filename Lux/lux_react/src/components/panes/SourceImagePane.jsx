import React, { useEffect, useState } from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';
import Alert from '@mui/material/Alert';
import WidgetGroup from '../WidgetGroup';
import MasonryImagePicker from '../MasonryImagePicker';

function SourceImagePane({ dimensions, panelSize, panelJSON, activeGroups, onWidgetGroupChange }) {
    const [debugInfo, setDebugInfo] = useState({
        groups: [],
        imageWidgets: [],
        selectedGroup: null
    });

    // Look for any source-related group name
    const sourceImageGroup = activeGroups.find(group =>
        group.name === 'SOURCE_IMAGE_GROUP' ||
        group.name === 'source' ||
        group.name === 'target' ||
        group.name.toLowerCase().includes('source') ||
        group.name.toLowerCase().includes('image')
    );

    // Collect debug information on initial render
    useEffect(() => {
        if (window.module) {
            // Log all groups for debugging
            console.log('All active groups:', activeGroups);

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

    // Log extra debugging information
    useEffect(() => {
        if (imagePickerJson) {
            console.log('Image Picker JSON found:', imagePickerJson);
        } else {
            console.log('No Image Picker JSON found');
            console.log('Debug Info:', debugInfo);
        }
    }, [imagePickerJson, debugInfo]);

    return (
        <Box
            sx={{
                display: 'flex',
                flexDirection: 'column',
                padding: 1,
                height: '100%',
                overflowY: 'auto'
            }}
        >
            {/* Debug Information */}
            {(!sourceImageGroup || !imagePickerJson) && (
                <Alert severity="info" sx={{ mb: 2 }}>
                    <Typography variant="body2">Debug Info:</Typography>
                    <Typography variant="caption" component="div">
                        Groups found: {debugInfo.groups.length}<br />
                        Image widgets found: {debugInfo.imageWidgets.length}<br />
                        Selected group: {debugInfo.selectedGroup || 'None'}
                    </Typography>
                </Alert>
            )}

            {/* Show error if no image picker is found but source image group exists */}
            {sourceImageGroup && !imagePickerJson && (
                <Alert severity="warning" sx={{ mb: 2 }}>
                    Source image group found, but no image picker widget detected.
                </Alert>
            )}

            {/* MasonryImagePicker component */}
            {imagePickerJson && (
                <MasonryImagePicker
                    json={imagePickerJson}
                    width={panelSize - 16} // Account for padding
                    onChange={onWidgetGroupChange}
                />
            )}

            {/* Effect list section */}
            {customSourceImageGroup && customSourceImageGroup.widgets.length > 0 && (
                <>
                    <Divider sx={{ my: 2 }} />

                    <Typography variant="subtitle1" sx={{ mb: 1 }}>
                        Effect Controls
                    </Typography>

                    <WidgetGroup
                        key={customSourceImageGroup.name}
                        panelSize={panelSize}
                        json={customSourceImageGroup}
                        onChange={onWidgetGroupChange}
                    />
                </>
            )}

            {/* Show when no source image group is found */}
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