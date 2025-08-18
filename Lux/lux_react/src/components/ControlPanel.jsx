import React, {useEffect, useRef, useState, useCallback} from "react";
import Paper from '@mui/material/Paper';
import Box from '@mui/material/Box';
import CircularProgress from '@mui/material/CircularProgress';
import Typography from '@mui/material/Typography';
import Alert from '@mui/material/Alert';
import JenSlider from './JenSlider';
import { ControlPanelContext } from './InterfaceContainer';
import { useJenModule } from '../hooks/useJenModule.js'; // Our simple, reliable module manager

import MediaController from "./MediaController";
import TabNavigation from "./TabNavigation";
import HomePane from "./panes/HomePane";
import SourceImagePane from "./panes/SourceImagePane";
import TargetImagePane from "./panes/TargetImagePane";
import BrushPane from "./panes/BrushPane";
import AudioPane from "./panes/AudioPane";
import {SceneChooserPane} from "./panes/SceneChooserPane";
import {PaneContext} from "./panes/PaneContext.jsx";
import RealtimeCamera from "./RealtimeCamera.jsx";

function ControlPanel({ dimensions, panelSize, activePane, onPaneChange }) {
    const { sliderValues, onSliderChange } = React.useContext(ControlPanelContext);
    
    // Use our simple, reliable module hook
    const { isReady: moduleReady, isLoading: moduleLoading, error: moduleError, callModuleFunction, saveSceneState } = useJenModule();
    
    // Component-specific state
    const [panelJSON, setPanelJSON] = useState([]);
    const [activeGroups, setActiveGroups] = useState([]);
    const [isInitialized, setIsInitialized] = useState(false);
    const [setupError, setSetupError] = useState(null);
    
    const previousPaneRef = useRef(activePane);
    const hasSetupRun = useRef(false);

    // Store current slider values - this preserves user settings across scene changes
    const storeSliderValues = useCallback(() => {
        const values = {};
        document.querySelectorAll('.jen-slider').forEach(slider => {
            const name = slider.getAttribute('data-name');
            const value = parseFloat(slider.getAttribute('data-value'));
            if (name && !isNaN(value)) {
                values[name] = value;
            }
        });
        onSliderChange(values);
    }, [onSliderChange]);

    // Restore slider values after scene changes
    const restoreSliderValues = useCallback(async () => {
        if (!moduleReady) return;
        
        // Restore each slider value that we have stored
        for (const [name, value] of Object.entries(sliderValues)) {
            const slider = document.querySelector(`.jen-slider[data-name="${name}"]`);
            if (slider) {
                try {
                    await callModuleFunction('set_slider_value', name, value);
                } catch (error) {
                    console.error(`Error restoring slider value for ${name}:`, error);
                }
            }
        }
    }, [sliderValues, moduleReady, callModuleFunction]);

    // Handle navigation between panes
    useEffect(() => {
        console.log(`Pane changed from ${previousPaneRef.current} to ${activePane}`);
        
        // When returning to home pane, refresh the widget groups
        if (activePane === "home" && previousPaneRef.current !== "home" && isInitialized) {
            console.log("Navigated to home pane, refreshing widgets");
            handleWidgetGroupChange();
        }
        
        previousPaneRef.current = activePane;
    }, [activePane, isInitialized]);

    // Update which widget groups are currently active
    const handleWidgetGroupChange = useCallback(async () => {
        if (!moduleReady || !panelJSON || panelJSON.length === 0) {
            return;
        }

        console.log("Updating active widget groups...");

        try {
            // Check each widget group to see if it should be displayed
            const activeGroupPromises = panelJSON.map(async (group) => {
                try {
                    const isActive = await callModuleFunction('is_widget_group_active', group.name);
                    return isActive ? group : null;
                } catch (error) {
                    console.error(`Error checking if group '${group.name}' is active:`, error);
                    return null;
                }
            });

            const activeGroupResults = await Promise.all(activeGroupPromises);
            const activeGroups = activeGroupResults.filter(group => group !== null);

            console.log("Active groups:", activeGroups.map(g => g.name).join(", "));
            setActiveGroups(activeGroups);

        } catch (error) {
            console.error("Error updating widget groups:", error);
        }
    }, [panelJSON, moduleReady, callModuleFunction]);

    // Save scene state handler
    const handleSaveScene = useCallback(async () => {
        try {
            const sceneState = await saveSceneState();
            if (sceneState) {
                // Create downloadable file
                const blob = new Blob([JSON.stringify(sceneState, null, 2)], {
                    type: 'application/json'
                });
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `${sceneState.name || 'scene'}_${Date.now()}.json`;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);
                URL.revokeObjectURL(url);
                
                console.log('Scene saved successfully:', sceneState.name);
            } else {
                console.error('Failed to get scene state');
            }
        } catch (error) {
            console.error('Error saving scene:', error);
        }
    }, [saveSceneState]);

    // Set up the control panel once the module is ready
    const setupPanel = useCallback(async () => {
        if (!moduleReady) {
            console.warn("Attempted to setup panel before module is ready");
            return false;
        }

        try {
            console.log("Setting up control panel...");
            setSetupError(null);

            // Save current slider values before we potentially change scenes
            storeSliderValues();

            // Get the panel configuration from the C++ backend
            const panelJSONString = await callModuleFunction('get_panel_JSON');

            if (!panelJSONString) {
                throw new Error("No panel configuration received from module");
            }

            // Parse the JSON configuration
            const parsedJSON = JSON.parse(panelJSONString);

            if (!parsedJSON || parsedJSON.length === 0) {
                console.warn("Panel configuration is empty - this might be normal for some scenes");
                setPanelJSON([]);
                setActiveGroups([]);
                setIsInitialized(true);
                return true;
            }

            console.log(`Successfully loaded panel configuration with ${parsedJSON.length} widget groups`);
            setPanelJSON(parsedJSON);

            // Mark as initialized and restore slider values
            setIsInitialized(true);
            
            // Restore slider values after a short delay to ensure DOM is updated
            setTimeout(() => {
                restoreSliderValues();
            }, 100);

            return true;

        } catch (error) {
            console.error("Error setting up panel:", error);
            setSetupError(`Panel setup failed: ${error.message}`);
            return false;
        }
    }, [moduleReady, storeSliderValues, callModuleFunction, restoreSliderValues]);

    // Initialize the panel when the module becomes ready
    useEffect(() => {
        if (moduleReady && !hasSetupRun.current) {
            hasSetupRun.current = true;
            
            console.log("Module is ready, initializing control panel...");
            
            setupPanel().then(async (success) => {
                if (success) {
                    console.log("Setting up scene change callback...");
                    
                    try {
                        // Register callback for when scenes change
                        await callModuleFunction('set_scene_callback', () => {
                            console.log("Scene change detected, refreshing panel...");
                            setupPanel().then(() => {
                                // Update widget groups after scene change
                                setTimeout(handleWidgetGroupChange, 100);
                            });
                        });
                    } catch (error) {
                        console.error("Error setting scene callback:", error);
                        setSetupError(`Scene callback setup failed: ${error.message}`);
                    }
                }
            });
        }
    }, [moduleReady, setupPanel, callModuleFunction, handleWidgetGroupChange]);

    // Update widget groups when panel configuration changes
    useEffect(() => {
        if (panelJSON && panelJSON.length > 0 && isInitialized) {
            console.log("Panel configuration changed, updating widget groups");
            handleWidgetGroupChange();
        }
    }, [panelJSON, isInitialized, handleWidgetGroupChange]);

    // Pane context for child components
    const paneContextValue = {
        activePane,
        setActivePane: onPaneChange
    };

    const isLoading = moduleLoading || (moduleReady && !isInitialized);
    const hasError = moduleError || setupError;

    const renderActivePane = () => {
        if (hasError) {
            const errorMessage = moduleError || setupError || 'An unknown error occurred';
            
            return (
                <Box sx={{ p: 2 }}>
                    <Alert severity="error" sx={{ mb: 2 }}>
                        <Typography variant="h6">System Error</Typography>
                        <Typography variant="body2">
                            {String(errorMessage)}
                        </Typography>
                    </Alert>
                    <Typography variant="body2" color="text.secondary">
                        Please refresh the page to try again. If the problem persists, 
                        check the browser console for more details.
                    </Typography>
                </Box>
            );
        }

        const commonProps = {
            dimensions,
            panelSize,
            panelJSON,
            activeGroups,
            onWidgetGroupChange: handleWidgetGroupChange,
            isLoading,
            onSaveScene: handleSaveScene
        };

        switch (activePane) {
            case "home":
                return <HomePane {...commonProps} />;
            case "scenes":
                return (
                    <PaneContext.Provider value={paneContextValue}>
                        <SceneChooserPane />
                    </PaneContext.Provider>
                );
            case "source":
                return (
                    <PaneContext.Provider value={paneContextValue}>
                        <SourceImagePane {...commonProps}/>
                    </PaneContext.Provider>
                );
            case "target":
                return <TargetImagePane {...commonProps} />;
            case "brush":
                return <BrushPane {...commonProps} />;
            case "audio":
                return (
                    <AudioPane 
                        dimensions={dimensions} 
                        panelSize={panelSize}
                    />
                );
            case "camera":
                return (
                    <Box sx={{ p: 1, height: '100%' }}>
                        <RealtimeCamera 
                            width={Math.min(dimensions.width - 20, 512)}
                            height={Math.min(dimensions.height - 100, 512)}
                            onClose={() => onPaneChange("home")}
                        />
                    </Box>
                );
            default:
                return <HomePane {...commonProps} />;
        }
    };

    return (
        <Paper
            elevation={3}
            sx={{
                minWidth: dimensions.width,
                minHeight: dimensions.height,
                display: 'flex',
                flexDirection: 'column',
                flexGrow: 1,
                alignSelf: 'stretch',
                overflow: 'hidden',
                maxWidth: dimensions.width
            }}
        >
            <TabNavigation
                activePane={activePane}
                onPaneChange={onPaneChange}
            />

            <MediaController panelSize={panelSize} />

            <Box
                sx={{
                    flex: 1,
                    overflow: 'auto',
                    position: 'relative'
                }}
            >
                {isLoading && (
                    <Box
                        sx={{
                            position: 'absolute',
                            top: 0,
                            left: 0,
                            right: 0,
                            bottom: 0,
                            display: 'flex',
                            flexDirection: 'column',
                            alignItems: 'center',
                            justifyContent: 'center',
                            padding: 2,
                            zIndex: 10,
                            bgcolor: 'rgba(0,0,0,0.8)',
                            color: 'white'
                        }}
                    >
                        <CircularProgress size={48} sx={{ mb: 2 }} />
                        <Typography variant="h6" sx={{ mb: 1 }}>
                            {moduleLoading ? 'Loading Jen...' : 'Initializing Controls...'}
                        </Typography>
                        <Typography variant="body2" color="rgba(255,255,255,0.7)">
                            Please wait while the system starts up
                        </Typography>
                    </Box>
                )}
                
                {renderActivePane()}
            </Box>
        </Paper>
    );
}

export default ControlPanel;