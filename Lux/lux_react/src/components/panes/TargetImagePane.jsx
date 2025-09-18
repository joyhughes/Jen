import React, { useEffect, useState } from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';
import Alert from '@mui/material/Alert';
import { useTheme } from '@mui/material/styles';
import useMediaQuery from '@mui/material/useMediaQuery';
import WidgetGroup from '../WidgetGroup';
import MasonryImagePicker from '../MasonryImagePicker';

function TargetImagePane({ dimensions, panelSize, panelJSON, activeGroups, onWidgetGroupChange }) {
    const [debugInfo, setDebugInfo] = useState({
        groups: [],
        imageWidgets: [],
        selectedGroup: null
    });

    const theme = useTheme();
    const isWideLayout = useMediaQuery(theme.breakpoints.up('md'));

    // Look for any target-related group name
    const targetImageGroup = activeGroups.find(group =>
        group.name === 'TARGET_IMAGE_GROUP' ||
        group.name === 'target' ||
        group.name.toLowerCase().includes('target')
    );

    // Collect debug information on initial render
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
                selectedGroup: targetImageGroup ? targetImageGroup.name : null
            });
        }
    }, [activeGroups, targetImageGroup]);

    // Find the image picker widget JSON if it exists
    const imagePickerJson = targetImageGroup?.widgets
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
        .find(widget => widget?.tool === 'image');

    // Filter out the image picker widget to avoid showing it twice
    const nonImagePickerWidgets = targetImageGroup?.widgets?.filter(widgetName => {
        const widgetJSON = window.module?.get_widget_JSON(widgetName);
        try {
            const widget = JSON.parse(widgetJSON);
            return !(widget?.tool === 'image' ||
                (widget?.type === 'menu_string' && widget?.items && Array.isArray(widget.items)));
        } catch (error) {
            return true;
        }
    }) || [];

    const customTargetImageGroup = targetImageGroup ? {
        ...targetImageGroup,
        widgets: nonImagePickerWidgets
    } : null;

    return (
        <Box
            sx={{
                display: 'flex',
                flexDirection: isWideLayout ? 'row' : 'column',
                padding: 1,
                height: '100%',
                overflowY: 'auto',
                gap: 2
            }}
        >
            {/* Image Picker Section */}
            <Box
                sx={{
                    flex: isWideLayout ? '0 0 50%' : '1 0 auto',
                    maxWidth: isWideLayout ? '50%' : '100%'
                }}
            >
                {/* Show error if no image picker is found but target image group exists */}
                {targetImageGroup && !imagePickerJson && (
                    <Alert severity="warning" sx={{ mb: 2 }}>
                        Target image group found, but no image picker widget detected.
                    </Alert>
                )}

                {/* MasonryImagePicker component */}
                {imagePickerJson && (
                    <MasonryImagePicker
                        json={imagePickerJson}
                        width="100%"
                        onChange={onWidgetGroupChange}
                        imageType="target"
                    />
                )}
            </Box>

            {/* Effect Controls Section */}
            {customTargetImageGroup && customTargetImageGroup.widgets.length > 0 && (
                <Box
                    sx={{
                        flex: isWideLayout ? '0 0 50%' : '1 0 auto',
                        maxWidth: isWideLayout ? '50%' : '100%'
                    }}
                >
                    <Typography variant="subtitle1" sx={{ mb: 1 }}>
                        Effect Controls
                    </Typography>

                    <WidgetGroup
                        key={customTargetImageGroup.name}
                        panelSize={isWideLayout ? panelSize / 2 : panelSize}
                        json={customTargetImageGroup}
                        onChange={onWidgetGroupChange}
                    />
                </Box>
            )}

            {/* Show when no target image group is found */}
            {!targetImageGroup && (
                <Box sx={{ textAlign: 'center', py: 4, color: 'text.secondary' }}>
                    <Typography>
                        No target image controls available. Please check your scene configuration.
                    </Typography>
                </Box>
            )}
        </Box>
    );
}

export default TargetImagePane;