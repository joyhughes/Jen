import React from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';
import WidgetGroup from '../WidgetGroup';
import MasonryImagePicker from '../MasonryImagePicker';

function SourceImagePane({ dimensions, panelSize, panelJSON, activeGroups, onWidgetGroupChange }) {
    // Find the source image group from active groups
    const sourceImageGroup = activeGroups.find(group => group.name === 'SOURCE_IMAGE_GROUP');

    // Find the image picker widget JSON if it exists
    const imagePickerJson = sourceImageGroup?.widgets
        .map(widgetName => {
            const widgetJSON = window.module?.get_widget_JSON(widgetName);
            try {
                return JSON.parse(widgetJSON);
            } catch (error) {
                return null;
            }
        })
        .find(widget => widget?.tool === 'image');

    // Filter out the image picker widget to avoid showing it twice
    const nonImagePickerWidgets = sourceImageGroup?.widgets.filter(widgetName => {
        const widgetJSON = window.module?.get_widget_JSON(widgetName);
        try {
            const widget = JSON.parse(widgetJSON);
            return widget?.tool !== 'image';
        } catch (error) {
            return true;
        }
    });

    const customSourceImageGroup = sourceImageGroup ? {
        ...sourceImageGroup,
        widgets: nonImagePickerWidgets || []
    } : null;

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
            {/* Use MasonryImagePicker instead of the regular ImagePicker */}
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
        </Box>
    );
}

export default SourceImagePane;