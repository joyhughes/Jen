import React, { useState, useEffect } from "react";
import {
    Box,
    Paper,
    Typography,
    CircularProgress,
    Divider,
    Alert,
    Fade,
    useTheme
} from '@mui/material';

// Import enhanced components
import JenWidgetGroup from './JenWidgetGroup.jsx';
import JenMediaController from "./JenMediaController.jsx"; // Assuming you'll enhance this
import JenSceneChooser from "./JenSceneChooser.jsx"; // Assuming you'll enhance this

function JenControlPanel({ dimensions, panelSize, moduleReady }) {
    const theme = useTheme();
    const [panelJSON, setPanelJSON] = useState([]);
    const [activeGroups, setActiveGroups] = useState([]);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState(null);

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

    // Setup the panel by loading JSON from the WebAssembly module
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

    // Clear the panel state
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

    // Loading state when module or panel data is not ready
    if (loading) {
        return (
            <Paper
                sx={{
                    minWidth: dimensions.width,
                    minHeight: dimensions.height,
                    display: 'flex',
                    justifyContent: 'center',
                    alignItems: 'center',
                    flexDirection: 'column',
                    p: 3
                }}
            >
                <CircularProgress size={40} sx={{ mb: 2 }} />
                <Typography variant="body1" color="text.secondary">
                    Loading control panel...
                </Typography>
            </Paper>
        );
    }

    // Error state
    if (error) {
        return (
            <Paper
                sx={{
                    minWidth: dimensions.width,
                    minHeight: dimensions.height,
                    p: 3
                }}
            >
                <Alert
                    severity="error"
                    sx={{ mb: 2 }}
                >
                    {error}
                </Alert>
                <Typography variant="body2" color="text.secondary">
                    Try refreshing the page or checking the console for more details.
                </Typography>
            </Paper>
        );
    }

    return (
        <Paper
            elevation={0}
            sx={{
                minWidth: dimensions.width,
                height: dimensions.height,
                display: 'flex',
                flexDirection: 'column',
                borderRadius: 0,
                backgroundColor: theme.palette.background.subtle,
                overflow: 'hidden', // Hide overflow
            }}
        >
            {/* Header section with media controls */}
            <Box
                sx={{
                    p: 2,
                    backgroundColor: theme.palette.background.paper,
                    borderBottom: '1px solid',
                    borderColor: theme.palette.divider,
                }}
            >
                <JenMediaController panelSize={panelSize} />
            </Box>

            {/* Scene chooser section */}
            <Box
                sx={{
                    p: 2,
                    backgroundColor: theme.palette.background.default,
                    borderBottom: '1px solid',
                    borderColor: theme.palette.divider,
                }}
            >
                <JenSceneChooser width={panelSize - 32} onChange={clearPanel} />
            </Box>

            {/* Widget groups section with scrolling */}
            <Box
                sx={{
                    flexGrow: 1,
                    overflow: 'auto', // Enable scrolling for this section only
                    p: 1,
                }}
            >
                {activeGroups.length > 0 ? (
                    <Fade in={true} timeout={300}>
                        <Box>
                            {activeGroups.map((group) => (
                                <JenWidgetGroup
                                    key={group.name}
                                    panelSize={panelSize}
                                    json={group}
                                    onChange={handleWidgetGroupChange}
                                />
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
                            p: 3,
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

export default JenControlPanel;