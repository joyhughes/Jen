import React from 'react';
import { Box, Tab, Tabs } from '@mui/material';
import HomeIcon from '@mui/icons-material/Home';
import ImageIcon from '@mui/icons-material/Image';
import FilterIcon from '@mui/icons-material/Filter';
import BrushIcon from '@mui/icons-material/Brush';

function TabNavigation({ activePane, onPaneChange }) {
    const handleChange = (event, newValue) => {
        console.log("current active panel: " + activePane + ", new value: " + JSON.stringify(newValue));
        onPaneChange(newValue);
    };

    return (
        <Box sx={{
            borderBottom: 1,
            borderColor: 'divider',
            bgcolor: 'background.paper',
            width: '100%'
        }}>
            <Tabs
                value={activePane}
                onChange={handleChange}
                variant="fullWidth"
                indicatorColor="primary"
                textColor="primary"
                aria-label="navigation tabs"
            >
                <Tab icon={<HomeIcon />} label="Home" value="home" sx={{ minHeight: '50px' }} />
                <Tab icon={<ImageIcon />} label="Source" value="source" sx={{ minHeight: '50px' }} />
                <Tab icon={<FilterIcon />} label="Target" value="target" sx={{ minHeight: '50px' }} />
                <Tab icon={<BrushIcon />} label="Brush" value="brush" sx={{ minHeight: '50px' }} />
            </Tabs>
        </Box>
    );
}

export default TabNavigation;







