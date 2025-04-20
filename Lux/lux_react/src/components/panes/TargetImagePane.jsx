import React from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import WidgetGroup from "../WidgetGroup.jsx";

function TargetImagePane({ dimensions, panelSize, panelJSON, activeGroups, onWidgetGroupChange }) {
    // Find the target image group from active groups
    const targetImageGroup = activeGroups.find(group => group.name === 'TARGET_IMAGE_GROUP');

    const handleUseCurrentImage = () => {
        // Functionality to use current canvas as target
        if (window.module) {
            window.module.use_current_as_target();
        }
    };

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
            <Typography variant="h6" sx={{ mb: 1 }}>Target Image</Typography>

            <Button
                variant="contained"
                color="primary"
                onClick={handleUseCurrentImage}
                sx={{ mb: 2 }}
            >
                Use Current Image
            </Button>

            {targetImageGroup && (
                <Box sx={{ mt: 1 }}>
                    <Typography variant="subtitle1">Target Controls</Typography>
                    <WidgetGroup
                        key={targetImageGroup.name}
                        panelSize={panelSize}
                        json={targetImageGroup}
                        onChange={onWidgetGroupChange}
                    />
                </Box>
            )}
        </Box>
    );
}
export default TargetImagePane;