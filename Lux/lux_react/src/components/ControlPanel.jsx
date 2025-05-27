import React, {useEffect, useRef, useState} from "react";
import Paper from '@mui/material/Paper';
import Box from '@mui/material/Box';
import CircularProgress from '@mui/material/CircularProgress';
import Typography from '@mui/material/Typography';
import JenSlider from './JenSlider';
import { ControlPanelContext } from './InterfaceContainer';

import MediaController from "./MediaController";
import TabNavigation from "./TabNavigation";
import HomePane from "./panes/HomePane";
import SourceImagePane from "./panes/SourceImagePane";
import TargetImagePane from "./panes/TargetImagePane";
import BrushPane from "./panes/BrushPane";
import {SceneChooserPane} from "./panes/SceneChooserPane";
import {PaneContext} from "./panes/PaneContext.jsx";

function ControlPanel({ dimensions, panelSize, activePane, onPaneChange }) {
    const { sliderValues, onSliderChange } = React.useContext(ControlPanelContext);
    const [panelJSON, setPanelJSON] = useState([]);
    const [activeGroups, setActiveGroups] = useState([]);
    const [isInitialized, setIsInitialized] = useState(false);
    const [isLoading, setIsLoading] = useState(true);
    const setupAttempts = useRef(0);
    const maxSetupAttempts = 50;
    const loadingTimeoutRef = useRef(null);
    const previousPaneRef = useRef(activePane);

    // Store slider values before scene changes
    const storeSliderValues = () => {
        const values = {};
        document.querySelectorAll('.jen-slider').forEach(slider => {
            const name = slider.getAttribute('data-name');
            const value = parseFloat(slider.getAttribute('data-value'));
            if (name && !isNaN(value)) {
                values[name] = value;
            }
        });
        onSliderChange(values);
    };

    // Restore slider values after scene changes
    const restoreSliderValues = () => {
        if (!window.module) return;
        
        Object.entries(sliderValues).forEach(([name, value]) => {
            const slider = document.querySelector(`.jen-slider[data-name="${name}"]`);
            if (slider) {
                window.module.set_slider_value(name, value);
            }
        });
    };

    useEffect(() => {
        console.log(`Pane changed from ${previousPaneRef.current} to ${activePane}`);
        if (activePane === "home" && previousPaneRef.current !== "home"){
            console.log("Navigated to home pane, triggering widget reload");
            handleWidgetGroupChange();
        }
    }, [activePane])

    const handleWidgetGroupChange = () => {
        if (!window.module) {
            console.warn("Module not available when updating widget groups");
            return;
        }

        if (!panelJSON || panelJSON.length === 0) {
            console.warn("Panel JSON empty during widget group update");
            return;
        }

        console.log("Updating active widget groups...");

        try {
            // Get active groups from the module
            const active = panelJSON.filter(group => {
                try {
                    return window.module.is_widget_group_active(group.name);
                } catch (error) {
                    console.error(`Error checking if group '${group.name}' is active:`, error);
                    return false;
                }
            });

            console.log("Active groups:", active.map(g => g.name).join(", "));

            if (active.length === 0 && activeGroups.length === 0) {
                console.warn("No active widget groups found");
            }

            setActiveGroups(active);
        } catch (error) {
            console.error("Error in handleWidgetGroupChange:", error);
        }
    };

    const setupPanel = (force = false) => {
        if (!window.module) {
            console.warn("Module not available when setting up panel");
            return false;
        }

        try {
            console.log("Setting up panel...");
            setIsLoading(true);

            // Store current slider values before setup
            storeSliderValues();

            // Clear any existing loading timeout
            if (loadingTimeoutRef.current) {
                clearTimeout(loadingTimeoutRef.current);
            }

            const panelJSONString = window.module.get_panel_JSON();

            if (!panelJSONString) {
                console.warn("Empty panel JSON received from module");
                return false;
            }

            try {
                const parsedJSON = JSON.parse(panelJSONString);

                if (!parsedJSON || parsedJSON.length === 0) {
                    console.warn("Parsed panel JSON is empty");
                    return false;
                }

                console.log("Successfully parsed panel JSON with", parsedJSON.length, "items");
                setPanelJSON(parsedJSON);

                // Use requestAnimationFrame to ensure state updates have been processed
                requestAnimationFrame(() => {
                    handleWidgetGroupChange();
                    setIsInitialized(true);
                    setIsLoading(false);
                    
                    // Restore slider values after setup
                    setTimeout(restoreSliderValues, 100);
                });

                return true;
            } catch (error) {
                console.error("Error parsing panel JSON:", error);
                setIsLoading(false);
                return false;
            }
        } catch (error) {
            console.error("Error in setupPanel:", error);
            setIsLoading(false);
            return false;
        }
    };

    // Initial setup effect - runs once when component mounts
    useEffect(() => {
        const initializeModule = () => {
            if (window.module) {
                console.log("Module detected, initializing panel...");
                const success = setupPanel();

                if (success) {
                    console.log("Setting scene callback...");
                    window.module.set_scene_callback(() => {
                        console.log("Scene callback triggered");
                        setupPanel(true);

                        setTimeout(() => {
                            handleWidgetGroupChange()
                        }, 100)
                    });
                } else {
                    // If setup failed but we haven't exceeded max attempts, try again
                    if (setupAttempts.current < maxSetupAttempts) {
                        setupAttempts.current += 1;
                        console.log(`Setup attempt ${setupAttempts.current}/${maxSetupAttempts} failed, retrying in 200ms...`);

                        setTimeout(initializeModule, 200);
                    } else {
                        console.error("Max setup attempts reached, giving up");
                        setIsLoading(false);
                    }
                }
            } else {
                // Module not ready yet, poll for it
                setupAttempts.current += 1;

                if (setupAttempts.current < maxSetupAttempts) { // Allow more attempts for initial module loading
                    console.log(`Module not ready (attempt ${setupAttempts.current}/${maxSetupAttempts}), checking again in 100ms...`);
                    setTimeout(initializeModule, 100);
                } else {
                    console.error("Module not available after maximum attempts");
                    setIsLoading(false);
                }
            }
        };

        // Start the initialization process
        initializeModule();

        loadingTimeoutRef.current = setTimeout(() => {
            console.warn("Loading timeout reached, forcing loading state to complete");
            setIsLoading(false);
        }, 10000); // 10 second safety timeout

        return () => {
            if (loadingTimeoutRef.current) {
                clearTimeout(loadingTimeoutRef.current);
            }
        };
    }, []);

    // Run when panelJSON changes
    useEffect(() => {
        if (panelJSON && panelJSON.length > 0) {
            console.log("Panel JSON changed, updating widget groups");
            handleWidgetGroupChange();
        }
    }, [panelJSON]);

    // Re-attempt handling widgets if active groups are empty but should have content
    useEffect(() => {
        if (isInitialized && activeGroups.length === 0 && !isLoading) {
            console.log("No active groups found after initialization, retrying...");

            const retryTimeout = setTimeout(() => {
                handleWidgetGroupChange();
            }, 300);

            return () => clearTimeout(retryTimeout);
        }
    }, [isInitialized, activeGroups.length, isLoading]);


    const paneContextValue = {
        activePane,
        setActivePane: onPaneChange
    }

    const renderActivePane = () => {
        const commonProps = {
            dimensions,
            panelSize,
            panelJSON,
            activeGroups,
            onWidgetGroupChange: handleWidgetGroupChange,
            isLoading
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
                )
            case "target":
                return <TargetImagePane {...commonProps} />;
            case "brush":
                return <BrushPane {...commonProps} />;
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
                            display: 'flex',
                            justifyContent: 'center',
                            padding: 2,
                            zIndex: 5,
                            borderRadius: 1
                        }}
                    >
                        <CircularProgress size={24} sx={{ mr: 2 }} />
                        <Typography variant="body2" color="white">
                            Loading widgets...
                        </Typography>
                    </Box>
                )}
                {renderActivePane()}
            </Box>
        </Paper>
    );
}

export default ControlPanel;