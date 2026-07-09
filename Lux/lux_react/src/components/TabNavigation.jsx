import React from 'react';
import { Box, Tab, Tabs, Tooltip } from '@mui/material';
import { FaHome, FaBullseye } from 'react-icons/fa';
import { BiImageAlt } from 'react-icons/bi';
import { BsBrush, BsCollectionPlay, BsCameraVideo } from 'react-icons/bs';
import { HiMicrophone } from 'react-icons/hi';
import { useAudioContext } from './AudioContext';

const ICON_SIZE = 20;

const PANES = [
    { value: 'home', label: 'Home', Icon: FaHome },
    { value: 'scenes', label: 'Scenes', Icon: BsCollectionPlay },
    { value: 'source', label: 'Source', Icon: BiImageAlt },
    { value: 'audio', label: 'Audio', Icon: HiMicrophone },
    { value: 'target', label: 'Target', Icon: FaBullseye },
    { value: 'brush', label: 'Brush', Icon: BsBrush },
    { value: 'camera', label: 'Camera', Icon: BsCameraVideo },
];

function TabNavigation({ activePane, onPaneChange }) {
    const { isEnabled: isAudioEnabled } = useAudioContext();

    const handleChange = (event, newValue) => {
        onPaneChange(newValue);
    };

    const renderIcon = (pane) => {
        const iconStyle = { fontSize: ICON_SIZE };

        if (pane.value === 'audio' && isAudioEnabled) {
            // Audio is live - show green glow plus an activity dot
            iconStyle.color = '#4CAF50';
            iconStyle.filter = 'drop-shadow(0 0 4px rgba(76, 175, 80, 0.7))';
            return (
                <Box sx={{ position: 'relative', display: 'flex', alignItems: 'center' }}>
                    <pane.Icon style={iconStyle} />
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
                            animation: 'pulse 2s infinite',
                        }}
                    />
                </Box>
            );
        }

        return <pane.Icon style={iconStyle} />;
    };

    return (
        <Box
            sx={{
                borderBottom: 1,
                borderColor: 'divider',
                bgcolor: 'background.paper',
                width: '100%',
                display: 'flex',
                alignItems: 'stretch',
            }}
        >
            <Tooltip title="Jen — generative art engine">
                <Box
                    sx={{
                        display: 'flex',
                        alignItems: 'center',
                        gap: 1,
                        px: 1,
                        borderRight: 1,
                        borderColor: 'divider',
                        flexShrink: 0,
                    }}
                >
                    <Box
                        component="img"
                        src={`${import.meta.env.BASE_URL}jen-logo.svg`}
                        alt="Jen"
                        sx={{ width: 28, height: 28 }}
                    />
                </Box>
            </Tooltip>

            <Tabs
                value={activePane}
                onChange={handleChange}
                variant="fullWidth"
                indicatorColor="primary"
                textColor="primary"
                aria-label="navigation tabs"
                sx={{ flex: 1, minWidth: 0 }}
            >
                {PANES.map((pane) => (
                    <Tab
                        key={pane.value}
                        value={pane.value}
                        icon={renderIcon(pane)}
                        label={pane.value === 'audio' && isAudioEnabled ? 'Audio ●' : pane.label}
                        aria-label={pane.label}
                        sx={{
                            '&.Mui-selected': {
                                background: 'rgba(167, 139, 250, 0.08)',
                            },
                        }}
                    />
                ))}
            </Tabs>
        </Box>
    );
}

export default TabNavigation;
