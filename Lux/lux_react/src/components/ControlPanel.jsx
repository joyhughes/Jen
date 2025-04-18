import React, { useState, useEffect, useRef } from "react";
import {
    Box,
    Typography,
    Collapse,
    IconButton,
    useTheme
} from '@mui/material';
import { ChevronDown, ChevronUp } from 'lucide-react';
import SceneChooser from "./SceneChooser";
import WidgetGroup from "./WidgetGroup";
import { THUMB_SIZE } from "./ThumbnailItem";

function ControlPanel({ dimensions, panelSize, isSmallScreen, isNarrowHeight }) {
    const theme = useTheme();
    const [panelJSON, setPanelJSON] = useState([]);
    const [activeGroups, setActiveGroups] = useState([]);
    const [expandedGroups, setExpandedGroups] = useState({});
    const [loading, setLoading] = useState(true);
    const scrollContainerRef = useRef(null);

    // Toggle group expansion
    const toggleGroupExpansion = (groupName) => {
        setExpandedGroups(prev => ({
            ...prev,
            [groupName]: !prev[groupName]
        }));
    };

    // Handle widget group change
    const handleWidgetGroupChange = () => {
        if (!window.module) return;

        const active = panelJSON.filter(group =>
            window.module.is_widget_group_active(group.name)
        );

        // Initialize expanded state for new groups
        const initialExpandedState = {};
        active.forEach(group => {
            if (expandedGroups[group.name] === undefined) {
                initialExpandedState[group.name] = true;
            } else {
                initialExpandedState[group.name] = expandedGroups[group.name];
            }
        });

        setActiveGroups(active);
        setExpandedGroups(prev => ({ ...prev, ...initialExpandedState }));
    };

    // Load panel JSON
    const setupPanel = () => {
        setLoading(true);
        if (!window.module) {
            setLoading(false);
            return;
        }

        try {
            const panelJSONString = window.module.get_panel_JSON();
            const parsedJSON = JSON.parse(panelJSONString);
            setPanelJSON(parsedJSON);
        } catch (error) {
            console.error("Error parsing panel JSON:", error);
        }
        setLoading(false);
    };

    // Clear panel when changing scenes
    const clearPanel = () => {
        setPanelJSON([]);
        setActiveGroups([]);
        setExpandedGroups({});
    };

    useEffect(() => {
        if (window.module) {
            setupPanel();
            window.module.set_scene_callback(setupPanel);
        } else {
            const intervalId = setInterval(() => {
                if (window.module) {
                    setupPanel();
                    window.module.set_scene_callback(setupPanel);
                    clearInterval(intervalId);
                }
            }, 100);
            return () => clearInterval(intervalId);
        }
    }, []);

    useEffect(() => {
        handleWidgetGroupChange();
    }, [panelJSON]);

    // Render widget groups in a single container
    return (
        <Box
            sx={{
                width: dimensions.width,
                height: dimensions.height,
                overflow: 'auto',
                bgcolor: '#222',
                color: 'white',
                '&::-webkit-scrollbar': {
                    width: '4px',
                    height: '4px',
                },
                '&::-webkit-scrollbar-thumb': {
                    backgroundColor: 'rgba(255, 255, 255, 0.2)',
                }
            }}
            ref={scrollContainerRef}
        >
            <Box sx={{ p: 1 }}>
                <SceneChooser width={THUMB_SIZE * 3} onChange={clearPanel} />
            </Box>

            {loading ? (
                <Typography variant="body2" sx={{ p: 2 }}>Loading controls...</Typography>
            ) : (
                <Box sx={{ p: 1 }}>
                    {activeGroups.map((group) => (
                        <Box
                            key={group.name}
                            id={`group-${group.name}`}
                            sx={{
                                bgcolor: 'rgba(0,0,0,0.2)',
                                borderRadius: 1,
                                overflow: 'hidden',
                                display: 'flex',
                                flexDirection: 'column',
                                position: 'relative',
                                width: THUMB_SIZE * 3,
                                mb: 1,
                                minHeight: expandedGroups[group.name] ? 'auto' : 'unset',
                            }}
                        >
                            <Box
                                sx={{
                                    display: 'flex',
                                    justifyContent: 'space-between',
                                    alignItems: 'center',
                                    px: 1,
                                    py: 0.5,
                                    bgcolor: 'rgba(0,0,0,0.3)',
                                    cursor: 'pointer',
                                }}
                                onClick={() => toggleGroupExpansion(group.name)}
                            >
                                <Typography
                                    variant="subtitle2"
                                    sx={{
                                        fontWeight: 600,
                                        fontSize: '0.8rem',
                                        textTransform: 'uppercase'
                                    }}
                                >
                                    {group.label || group.name}
                                </Typography>
                                <IconButton size="small" sx={{ color: 'white', p: 0.25 }}>
                                    {expandedGroups[group.name] ? (
                                        <ChevronUp size={14} />
                                    ) : (
                                        <ChevronDown size={14} />
                                    )}
                                </IconButton>
                            </Box>

                            <Collapse
                                in={expandedGroups[group.name] ?? true}
                                timeout="auto"
                            >
                                <WidgetGroup
                                    json={group}
                                    panelSize={panelSize}
                                    onChange={handleWidgetGroupChange}
                                />
                            </Collapse>
                        </Box>
                    ))}
                </Box>
            )}
        </Box>
    );
}

export default ControlPanel;