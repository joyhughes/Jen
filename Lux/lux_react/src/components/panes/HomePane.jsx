import React from 'react';
import Box from '@mui/material/Box';
import WidgetGroup from '../WidgetGroup';
import SceneChooser from '../SceneChooser';

function HomePane({ dimensions, panelSize, panelJSON, activeGroups, onWidgetGroupChange }) {
    // Only render widget groups that should appear in the home pane
    const homeGroups = activeGroups.filter(group =>
        group.name !== 'SOURCE_IMAGE_GROUP' &&
        group.name !== 'TARGET_IMAGE_GROUP' &&
        group.name !== 'BRUSH_GROUP'
    );

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
            <SceneChooser width={panelSize} onChange={onWidgetGroupChange} />

            {homeGroups.map((group) => (
                <WidgetGroup
                    key={group.name}
                    panelSize={panelSize}
                    json={group}
                    onChange={onWidgetGroupChange}
                />
            ))}
        </Box>
    );
}

export default HomePane;
