import React from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import WidgetGroup from "../WidgetGroup.jsx";

function BrushPane({ dimensions, panelSize, panelJSON, activeGroups, onWidgetGroupChange }) {
    const brushGroup = activeGroups.find(group => group.name === 'BRUSH_GROUP');

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
            <Typography variant="h6" sx={{ mb: 1 }}>Brush Editor</Typography>

            {brushGroup ? (
                <WidgetGroup
                    key={brushGroup.name}
                    panelSize={panelSize}
                    json={brushGroup}
                    onChange={onWidgetGroupChange}
                />
            ) : (
                <Typography variant="body2" color="text.secondary">
                    No brush settings available for this scene.
                </Typography>
            )}
        </Box>
    );
}
export default BrushPane;