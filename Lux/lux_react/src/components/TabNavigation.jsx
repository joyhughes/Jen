import React from 'react';
import { Box, Tab, Tabs, Tooltip } from '@mui/material';
import { FaHome } from 'react-icons/fa';
import { BiImageAlt } from 'react-icons/bi';
import { FaBullseye } from 'react-icons/fa';
import { BsBrush } from 'react-icons/bs';
import { BsCollectionPlay } from 'react-icons/bs';
import { BsCameraVideo } from 'react-icons/bs';
import { HiMicrophone } from 'react-icons/hi';
import { useAudioContext } from './AudioContext';

function TabNavigation({ activePane, onPaneChange }) {
    const { isEnabled: isAudioEnabled } = useAudioContext();
    const handleChange = (event, newValue) => {
        console.log("current active panel: " + activePane + ", new value: " + JSON.stringify(newValue));
        onPaneChange(newValue);
    };

    const iconStyle = {
        fontSize: '24px'  // Consistent size for all icons
    };

    const getAudioIconStyle = () => {
        const baseStyle = { ...iconStyle };
        
        if (isAudioEnabled) {
            // Audio is active - show green glow
            baseStyle.color = "#4CAF50";
            baseStyle.filter = "drop-shadow(0 0 4px rgba(76, 175, 80, 0.7))";
        } else if (activePane === "audio") {
            // Audio pane is selected but not active - show orange
            baseStyle.color = "#ff6b35";
        }
        
        return baseStyle;
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
                variant="scrollable"
                scrollButtons="auto"
                allowScrollButtonsMobile
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
                    icon={<BsCollectionPlay style={iconStyle} />}
                    value="scenes"
                    sx={{
                        minHeight: '50px',
                        '&.Mui-selected': {
                            color: 'primary.main',
                            background: 'rgba(25, 118, 210, 0.08)'
                        }
                    }}
                    aria-label="Scenes"
                    title="Scenes"
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

                <Tab
                    icon={
                        <Box sx={{ position: 'relative', display: 'flex', alignItems: 'center' }}>
                            <HiMicrophone style={getAudioIconStyle()} />
                            {isAudioEnabled && (
                                <Box 
                                    sx={{
                                        position: 'absolute',
                                        top: -2,
                                        right: -2,
                                        width: 8,
                                        height: 8,
                                        borderRadius: '50%',
                                        backgroundColor: '#4CAF50',
                                        border: '1px solid white',
                                        boxShadow: '0 0 4px rgba(76, 175, 80, 0.7)',
                                        animation: 'pulse 2s infinite'
                                    }}
                                />
                            )}
                        </Box>
                    }
                    value="audio"
                    sx={{
                        minHeight: '50px',
                        '&.Mui-selected': {
                            color: 'primary.main',
                            background: 'rgba(25, 118, 210, 0.08)'
                        },
                        '& .MuiTab-iconWrapper': {
                            position: 'relative'
                        }
                    }}
                    aria-label={isAudioEnabled ? "Audio (Active)" : "Audio"}
                    title={isAudioEnabled ? "Audio (Active)" : "Audio"}
                />

                <Tab
                    icon={<BsCameraVideo style={iconStyle} />}
                    value="camera"
                    sx={{
                        minHeight: '50px',
                        '&.Mui-selected': {
                            color: 'primary.main',
                            background: 'rgba(25, 118, 210, 0.08)'
                        }
                    }}
                    aria-label="Camera"
                    title="Camera"
                />
            </Tabs>
        </Box>
    );
}

export default TabNavigation;