import { useEffect } from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Alert from '@mui/material/Alert';
import { useTheme } from '@mui/material/styles';
import useMediaQuery from '@mui/material/useMediaQuery';
import MasonryImagePicker from '../MasonryImagePicker';

function TargetImagePane({ activeGroups, onWidgetGroupChange }) {
    const theme = useTheme();
    const isWideLayout = useMediaQuery(theme.breakpoints.up('md'));

    const targetImageGroup = activeGroups.find(group =>
        group.name.includes("image")
    );

    useEffect(() => {
        if (window.module) {
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
        .find(widget => widget?.tool === 'image' && widget?.name.includes("target"));

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
                        updateFuncName={imagePickerJson.name}
                        json={imagePickerJson}
                        width="100%"
                        onChange={onWidgetGroupChange}
                    />
                )}
            </Box>
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