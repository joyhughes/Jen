import React, {useEffect, useRef, useState, useCallback} from "react";
import Paper from '@mui/material/Paper';
import Box from '@mui/material/Box';
import CircularProgress from '@mui/material/CircularProgress';
import Typography from '@mui/material/Typography';
import Alert from '@mui/material/Alert';
import Button from '@mui/material/Button';
import Snackbar from '@mui/material/Snackbar';
import JenSlider from './JenSlider';
import { ControlPanelContext } from './InterfaceContainer';
import { useJenModule } from '../hooks/useJenModule.js';
import { SceneStorage } from '../utils/sceneStorage.js'; 

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
    const { sliderValues, onSliderChange, triggerSceneChange } = React.useContext(ControlPanelContext);
    
    // Use our simple, reliable module hook
    const { isReady: moduleReady, isLoading: moduleLoading, error: moduleError, callModuleFunction } = useJenModule();
    
    // Component-specific state
    const [panelJSON, setPanelJSON] = useState([]);
    const [activeGroups, setActiveGroups] = useState([]);
    const [isInitialized, setIsInitialized] = useState(false);
    const [setupError, setSetupError] = useState(null);
    const [currentSceneName, setCurrentSceneName] = useState('');
    const [hasUnsavedChanges, setHasUnsavedChanges] = useState(false);
    const [notification, setNotification] = useState({ open: false, message: '', severity: 'info' });
    
    const previousPaneRef = useRef(activePane);
    const hasSetupRun = useRef(false);
    const lastSavedValues = useRef({});
    const autoSaveTimeout = useRef(null);

    // Show notification
    const showNotification = useCallback((message, severity = 'info') => {
        setNotification({ open: true, message, severity });
    }, []);

    // Get current scene name from the module
    const getCurrentSceneName = useCallback(async () => {
        if (!moduleReady) return '';
        
        try {
            return await callModuleFunction('get_current_scene_name') || '';
        } catch (error) {
            console.error('Error getting current scene name:', error);
            return '';
        }
    }, [moduleReady, callModuleFunction]);

    // Collect current slider values from DOM and state
    const collectCurrentSliderValues = useCallback(() => {
        const values = { ...sliderValues };
        
        // Also collect from DOM to catch any recent changes
        document.querySelectorAll('.jen-slider').forEach(slider => {
            const name = slider.getAttribute('data-name');
            const value = parseFloat(slider.getAttribute('data-value'));
            if (name && !isNaN(value)) {
                values[name] = value;
            }
        });
        
        return values;
    }, [sliderValues]);

    // Check if slider values have changed
    const hasSliderValuesChanged = useCallback(() => {
        const currentValues = collectCurrentSliderValues();
        const lastValues = lastSavedValues.current;
        
        // Compare values
        const currentKeys = Object.keys(currentValues);
        const lastKeys = Object.keys(lastValues);
        
        if (currentKeys.length !== lastKeys.length) return true;
        
        return currentKeys.some(key => {
            const current = currentValues[key];
            const last = lastValues[key];
            return Math.abs(current - last) > 0.0001; // Small threshold for float comparison
        });
    }, [collectCurrentSliderValues]);

    // Save complete scene state using the new C++ backend
    const saveSceneConfig = useCallback(async () => {
        if (!currentSceneName) {
            console.warn('Cannot save config: no current scene name');
            return false;
        }

        try {
            // Save complete scene state using the new C++ backend
            const success = await SceneStorage.saveSceneConfig(currentSceneName);
            
            if (success) {
                // Also update our local slider values for UI consistency
                const sliderValues = collectCurrentSliderValues();
                lastSavedValues.current = { ...sliderValues };
                setHasUnsavedChanges(false);
                
                showNotification(`Complete scene state saved for "${currentSceneName}"`, 'success');
                return true;
            } else {
                showNotification(`Failed to save scene state for "${currentSceneName}"`, 'error');
                return false;
            }
        } catch (error) {
            console.error('Error saving complete scene state:', error);
            showNotification(`Failed to save scene: ${error.message}`, 'error');
            return false;
        }
    }, [currentSceneName, collectCurrentSliderValues, showNotification]);

    // Load complete scene state using the new C++ backend
    const loadSceneConfig = useCallback(async (sceneName) => {
        if (!sceneName) return null;

        try {
            // Load complete scene state using the new C++ backend
            const savedConfig = await SceneStorage.loadSceneConfig(sceneName);
            
            if (savedConfig) {
                console.log(`📋 Complete scene state loaded for: ${sceneName}`);
                // The backend has already restored the complete state
                // We just need to refresh the UI to reflect the changes
                setResetTrigger(prev => prev + 1);
                return savedConfig; // Return for backward compatibility
            } else {
                console.log(`📂 No saved scene state found for ${sceneName}, using defaults`);
                return null;
            }
        } catch (error) {
            console.error('Error loading complete scene state:', error);
            showNotification(`Failed to load scene state for ${sceneName}`, 'warning');
            return null;
        }
    }, [showNotification]);

    // Apply slider values to the scene
    const applySliderValues = useCallback(async (values) => {
        if (!moduleReady || !values) return;
        
        console.log('Applying slider values:', Object.keys(values));
        
        // Apply each slider value to backend
        for (const [name, value] of Object.entries(values)) {
            try {
                if (Array.isArray(value) && value.length === 2) {
                    // Handle range sliders
                    await callModuleFunction('set_range_slider_value', name, value[0], value[1]);
                } else {
                    // Handle single sliders
                    await callModuleFunction('set_slider_value', name, value);
                }
            } catch (error) {
                console.error(`Error applying slider value for ${name}:`, error);
            }
        }
        
        // Update global slider values by calling onSliderChange for each value
        // This ensures the JenSlider components get updated
        for (const [name, value] of Object.entries(values)) {
            onSliderChange(name, value);
        }
        
        lastSavedValues.current = { ...values };
    }, [moduleReady, callModuleFunction, onSliderChange]);

    // Export complete scene state as JSON
    const exportSceneState = useCallback(async () => {
        try {
            const stateJson = await SceneStorage.getCompleteSceneState();
            if (stateJson) {
                // Create and download the JSON file
                const blob = new Blob([JSON.stringify(stateJson, null, 2)], { type: 'application/json' });
                const url = URL.createObjectURL(blob);
                const link = document.createElement('a');
                link.href = url;
                link.download = `${currentSceneName || 'scene'}_complete_state_${new Date().toISOString().split('T')[0]}.json`;
                document.body.appendChild(link);
                link.click();
                document.body.removeChild(link);
                URL.revokeObjectURL(url);
                
                showNotification('Complete scene state exported successfully', 'success');
                return true;
            } else {
                showNotification('Failed to export scene state', 'error');
                return false;
            }
        } catch (error) {
            console.error('Error exporting scene state:', error);
            showNotification(`Failed to export scene state: ${error.message}`, 'error');
            return false;
        }
    }, [currentSceneName, showNotification]);

    // Import complete scene state from JSON
    const importSceneState = useCallback(async (file) => {
        try {
            const text = await file.text();
            const stateJson = JSON.parse(text);
            
            const success = await SceneStorage.loadCompleteSceneState(stateJson);
            if (success) {
                // Refresh the UI to reflect the imported state
                setResetTrigger(prev => prev + 1);
                showNotification('Complete scene state imported successfully', 'success');
                return true;
            } else {
                showNotification('Failed to import scene state', 'error');
                return false;
            }
        } catch (error) {
            console.error('Error importing scene state:', error);
            showNotification(`Failed to import scene state: ${error.message}`, 'error');
            return false;
        }
    }, [showNotification]);

    // Auto-save functionality
    const scheduleAutoSave = useCallback(() => {
        if (autoSaveTimeout.current) {
            clearTimeout(autoSaveTimeout.current);
        }
        
        // Auto-save after 2 seconds of inactivity
        autoSaveTimeout.current = setTimeout(() => {
            if (hasSliderValuesChanged() && currentSceneName) {
                console.log('Auto-saving scene configuration...');
                saveSceneConfig();
            }
        }, 2000);
    }, [hasSliderValuesChanged, currentSceneName, saveSceneConfig]);

    // Store current slider values - enhanced with change detection
    const storeSliderValues = useCallback(() => {
        const values = collectCurrentSliderValues();
        onSliderChange(values);
        
        // Check if values have changed and schedule auto-save
        if (hasSliderValuesChanged()) {
            setHasUnsavedChanges(true);
            scheduleAutoSave();
        }
    }, [collectCurrentSliderValues, onSliderChange, hasSliderValuesChanged, scheduleAutoSave]);

    // Enhanced restore function that tries localStorage first
    const restoreSliderValues = useCallback(async (sceneName = null) => {
        if (!moduleReady) return;
        
        const targetScene = sceneName || currentSceneName;
        if (!targetScene) return;
        
        try {
            // Try to load saved configuration first
            const savedValues = await loadSceneConfig(targetScene);
            
            if (savedValues) {
                console.log(`Restoring saved values for scene: ${targetScene}`);
                await applySliderValues(savedValues);
                // Trigger scene change to force slider updates
                triggerSceneChange();
                showNotification(`Loaded saved settings for "${targetScene}"`, 'info');
            } else {
                // Fall back to stored values from context (default behavior)
                console.log(`Applying default values for scene: ${targetScene}`);
                
                for (const [name, value] of Object.entries(sliderValues)) {
                    const slider = document.querySelector(`.jen-slider[data-name="${name}"]`);
                    if (slider) {
                        try {
                            await callModuleFunction('set_slider_value', name, value);
                        } catch (error) {
                            console.error(`Error restoring default slider value for ${name}:`, error);
                        }
                    }
                }
            }
        } catch (error) {
            console.error('Error in restoreSliderValues:', error);
        }
    }, [moduleReady, currentSceneName, loadSceneConfig, applySliderValues, showNotification, sliderValues, callModuleFunction]);

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

    // Enhanced setup panel with scene name detection
    const setupPanel = useCallback(async () => {
        if (!moduleReady) {
            console.warn("Attempted to setup panel before module is ready");
            return false;
        }

        try {
            console.log("Setting up control panel...");
            setSetupError(null);

            // Get current scene name
            const sceneName = await getCurrentSceneName();
            setCurrentSceneName(sceneName);
            console.log(`Current scene: ${sceneName}`);

            storeSliderValues();

            const panelJSONString = await callModuleFunction('get_panel_JSON');

            if (!panelJSONString) {
                throw new Error("No panel configuration received from module");
            }

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

            setIsInitialized(true);
            
            // Give the DOM time to render the sliders before applying values
            setTimeout(async () => {
                console.log('Restoring slider values for scene:', sceneName);
                await restoreSliderValues(sceneName);
            }, 200);

            return true;

        } catch (error) {
            console.error("Error setting up panel:", error);
            setSetupError(`Panel setup failed: ${error.message}`);
            return false;
        }
    }, [moduleReady, getCurrentSceneName, storeSliderValues, callModuleFunction, restoreSliderValues]);

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
                        await callModuleFunction('set_scene_callback', async () => {
                            console.log("Scene change detected, refreshing panel...");
                            
                            // Save current config before changing scenes
                            if (currentSceneName && hasUnsavedChanges) {
                                await saveSceneConfig();
                            }
                            
                            await setupPanel();
                            
                            // Update widget groups after scene change
                            setTimeout(handleWidgetGroupChange, 100);
                        });
                    } catch (error) {
                        console.error("Error setting scene callback:", error);
                        setSetupError(`Scene callback setup failed: ${error.message}`);
                    }
                }
            });
        }
    }, [moduleReady, setupPanel, callModuleFunction, handleWidgetGroupChange, currentSceneName, hasUnsavedChanges, saveSceneConfig]);

    // Update widget groups when panel configuration changes
    useEffect(() => {
        if (panelJSON && panelJSON.length > 0 && isInitialized) {
            console.log("Panel configuration changed, updating widget groups");
            handleWidgetGroupChange();
        }
    }, [panelJSON, isInitialized, handleWidgetGroupChange]);

    // Cleanup auto-save timeout
    useEffect(() => {
        return () => {
            if (autoSaveTimeout.current) {
                clearTimeout(autoSaveTimeout.current);
            }
        };
    }, []);

    // Pane context for child components
    const paneContextValue = {
        activePane,
        setActivePane: onPaneChange,
        saveSceneConfig,
        hasUnsavedChanges,
        currentSceneName
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
            saveSceneConfig,
            hasUnsavedChanges,
            currentSceneName
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

            <MediaController 
                panelSize={panelSize}
                saveSceneConfig={saveSceneConfig}
                hasUnsavedChanges={hasUnsavedChanges}
                currentSceneName={currentSceneName}
                exportSceneState={exportSceneState}
                importSceneState={importSceneState}
            />

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

            {/* Save notification */}
            <Snackbar
                open={notification.open}
                autoHideDuration={4000}
                onClose={() => setNotification(prev => ({ ...prev, open: false }))}
                anchorOrigin={{ vertical: 'bottom', horizontal: 'left' }}
            >
                <Alert 
                    onClose={() => setNotification(prev => ({ ...prev, open: false }))} 
                    severity={notification.severity}
                    sx={{ width: '100%' }}
                >
                    {notification.message}
                </Alert>
            </Snackbar>
        </Paper>
    );
}

export default ControlPanel;