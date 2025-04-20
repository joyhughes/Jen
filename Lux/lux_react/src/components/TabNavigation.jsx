import React from 'react';
import { Box, Tab, Tabs, Tooltip } from '@mui/material';
import { FaHome } from 'react-icons/fa';  // Home icon from Font Awesome
import { BiImageAlt } from 'react-icons/bi';  // Source image icon from Boxicons
import { FaBullseye } from 'react-icons/fa';  // Target/bullseye icon from Font Awesome
import { BsBrush } from 'react-icons/bs';  // Brush icon from Bootstrap Icons

function TabNavigation({ activePane, onPaneChange }) {
    const handleChange = (event, newValue) => {
        console.log("current active panel: " + activePane + ", new value: " + JSON.stringify(newValue));
        onPaneChange(newValue);
    };

    const iconStyle = {
        fontSize: '24px'  // Consistent size for all icons
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
                <Tab
                    icon={<FaHome style={iconStyle} />}
                    value="home"
                    sx={{
                        minHeight: '50px',
                        '&.Mui-selected': {
                            color: 'primary.main',
                            background: 'rgba(25, 118, 210, 0.08)'
                        }
                    }}
                    aria-label="Home"
                    title="Home"
                />

                <Tab
                    icon={<BiImageAlt style={iconStyle} />}
                    value="source"
                    sx={{
                        minHeight: '50px',
                        '&.Mui-selected': {
                            color: 'primary.main',
                            background: 'rgba(25, 118, 210, 0.08)'
                        }
                    }}
                    aria-label="Source Image"
                    title="Source Image"
                />

                <Tab
                    icon={<FaBullseye style={{...iconStyle, color: activePane === "target" ? "#e63946" : undefined}} />}
                    value="target"
                    sx={{
                        minHeight: '50px',
                        '&.Mui-selected': {
                            color: 'primary.main',
                            background: 'rgba(25, 118, 210, 0.08)'
                        }
                    }}
                    aria-label="Target Image"
                    title="Target Image"
                />

                <Tab
                    icon={<BsBrush style={iconStyle} />}
                    value="brush"
                    sx={{
                        minHeight: '50px',
                        '&.Mui-selected': {
                            color: 'primary.main',
                            background: 'rgba(25, 118, 210, 0.08)'
                        }
                    }}
                    aria-label="Brush"
                    title="Brush"
                />
            </Tabs>
        </Box>
    );
}

export default TabNavigation;