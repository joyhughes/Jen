import React, { useState, useEffect } from "react";
import { Box, Paper, Typography, CircularProgress, Divider, Alert, Fade, useMediaQuery, useTheme } from '@mui/material';
import WidgetGroup from './WidgetGroup';
import MediaController from "./MediaController";
import SceneChooser from "./SceneChooser";

function ControlPanel({ dimensions, panelSize, moduleReady }) {
    const theme = useTheme();
    const isMobile = useMediaQuery(theme.breakpoints.down('sm'));
    const [panelJSON, setPanelJSON] = useState([]);
    const [activeGroups, setActiveGroups] = useState([]);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState(null);

    // Determine if panel is horizontal (side) or vertical (bottom)
    const isHorizontalLayout = dimensions.width < dimensions.height;

    // Determine column width based on device type and panel orientation
    const columnWidth = isMobile ? dimensions.width - 32 : 280; // More compact columns

    // Callback function to handle state changes in widget groups
    const handleWidgetGroupChange = () => {
        if (!window.module) return;

        try {
            // Filter to only active widget groups
            const active = panelJSON.filter(group =>
                window.module.is_widget_group_active(group.name)
            );
            setActiveGroups(active);
        } catch (error) {
            console.error("Error updating active widget groups:", error);
            setError("Failed to update widget groups");
        }
    };

    const setupPanel = () => {
        if (!window.module) return;

        setLoading(true);
        setError(null);

        try {
            const panelJSONString = window.module.get_panel_JSON();
            const parsedJSON = JSON.parse(panelJSONString);
            setPanelJSON(parsedJSON);
            setLoading(false);
        } catch (error) {
            console.error("Error parsing panel JSON:", error);
            setError("Failed to load control panel data");
            setLoading(false);
        }
    };

    const clearPanel = () => {
        setPanelJSON([]);
        setActiveGroups([]);
    };

    // Initially check for module and set up callback
    useEffect(() => {
        const checkAndSetupModule = () => {
            if (window.module) {
                setupPanel();

                // Register callback for scene changes
                try {
                    window.module.set_scene_callback(setupPanel);
                } catch (error) {
                    console.error("Error setting scene callback:", error);
                }

                return true;
            }
            return false;
        };

        // If module is not immediately available, poll for it
        if (!checkAndSetupModule()) {
            const intervalId = setInterval(() => {
                if (checkAndSetupModule()) {
                    clearInterval(intervalId);
                }
            }, 100);

            // Cleanup interval on component unmount
            return () => clearInterval(intervalId);
        }
    }, []);

    // Update active groups when panel JSON changes
    useEffect(() => {
        handleWidgetGroupChange();
    }, [panelJSON]);

    // Loading state
    if (loading) {
        return (
            <Paper
                sx={{
                    width: dimensions.width,
                    height: dimensions.height,
                    display: 'flex',
                    justifyContent: 'center',
                    alignItems: 'center',
                    flexDirection: 'column',
                    p: 2
                }}
            >
                <CircularProgress size={32} sx={{ mb: 1 }} />
                <Typography variant="body2" color="text.secondary">
                    Loading controls...
                </Typography>
            </Paper>
        );
    }

    // Error state
    if (error) {
        return (
            <Paper
                sx={{
                    width: dimensions.width,
                    height: dimensions.height,
                    p: 2
                }}
            >
                <Alert severity="error" sx={{ mb: 1 }}>{error}</Alert>
                <Typography variant="body2" color="text.secondary">
                    Try refreshing the page.
                </Typography>
            </Paper>
        );
    }

    const contentWidth = Math.floor(dimensions.width - 32); // Subtract padding


    return (
        <Paper
            elevation={0}
            sx={{
                width: columnWidth,
                height: dimensions.height,
                display: 'flex',
                flexDirection: 'column',
                borderRadius: 0,
                backgroundColor: theme.palette.background.default,
                overflow: 'hidden', // Hide overflow for the container
            }}
        >
            {/* Fixed header sections */}
            <Box

                 sx={{
                     p: 1.5,
                     backgroundColor: theme.palette.background.paper,
                     borderBottom: '1px solid',
                     borderColor: theme.palette.divider,
                     flexShrink: 0
                 }}


            > {/* This prevents the header from shrinking */}

                {/* Media Controls */}
                <Box
                    sx={{
                        width: columnWidth,
                        flex: '0 0 auto',
                    }}
                >
                    <MediaController panelSize={columnWidth} />
                </Box>

                {/* Scene Chooser */}
                <Box
                    sx={{
                        width: columnWidth,
                        flex: '0 0 auto',
                    }}
                >
                    <SceneChooser width={columnWidth} onChange={clearPanel} />
                </Box>
            </Box>

            <Box
                sx={{
                    flexGrow: 1,
                    overflowY: 'auto', // Always allow vertical scrolling
                    overflowX: isHorizontalLayout ? 'auto' : 'hidden',
                    backgroundColor: theme.palette.background.paper,
                    p: 1,
                }}
            >
                {activeGroups.length > 0 ? (
                    <Fade in={true} timeout={200}>
                        <Box
                            sx={{
                                display: 'flex',
                                flexDirection: isHorizontalLayout ? 'column' : 'row',
                                flexWrap: isHorizontalLayout ? 'wrap' : 'nowrap',
                                width: '100%',
                            }}
                        >
                            {activeGroups.map((group) => (
                                <Box
                                    key={group.name}
                                    sx={{
                                        width: columnWidth,
                                        flex: '0 0 auto',
                                    }}
                                >
                                    <WidgetGroup
                                        panelSize={columnWidth}
                                        json={group}
                                        onChange={handleWidgetGroupChange}
                                        isMobile={isMobile}
                                    />
                                </Box>
                            ))}
                        </Box>
                    </Fade>
                ) : (
                    <Box
                        sx={{
                            display: 'flex',
                            justifyContent: 'center',
                            alignItems: 'center',
                            height: '100%',
                            p: 2,
                            textAlign: 'center'
                        }}
                    >
                        <Typography variant="body2" color="text.secondary">
                            No active widgets available. Select a scene to begin.
                        </Typography>
                    </Box>
                )}
            </Box>
        </Paper>
    );
}

export default ControlPanel;