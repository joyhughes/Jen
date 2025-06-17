import {Box} from "@mui/material";
import React, {useCallback, useEffect, useRef, useState} from "react";
import ControlPanel from "./ControlPanel";
import ImagePort from "./ImagePort";
import {SceneProvider} from './SceneContext';
import {PaneContext} from "./panes/PaneContext.jsx";
import {AudioProvider} from './AudioContext';

// Create a context for control panel state
export const ControlPanelContext = React.createContext();

function InterfaceContainer({panelSize}) {
    const [moduleReady, setModuleReady] = useState(false);
    const [isRowDirection, setIsRowDirection] = useState(true);
    const [imagePortDimensions, setImagePortDimensions] = useState({width: 0, height: 0});
    const [controlPanelDimensions, setControlPanelDimensions] = useState({width: 0, height: 0});
    const [activePane, setActivePane] = useState("home");
    const [sliderValues, setSliderValues] = useState({});
    const [resetTrigger, setResetTrigger] = useState(0);
    const containerRef = useRef(null);

    // Store slider values when they change from user interaction
    const handleSliderChange = (name, value) => {
        setSliderValues(prev => {
            const newValues = { ...prev, [name]: value };
            // Only update backend when values actually change from user interaction
            if (window.module && prev[name] !== value) {
                try {
                    // Handle range sliders vs single sliders differently
                    if (Array.isArray(value) && value.length === 2) {
                        // For range sliders, call set_range_slider_value with two separate parameters
                        if (typeof window.module.set_range_slider_value === 'function') {
                            // Ensure values are proper numbers
                            const val1 = parseFloat(value[0]);
                            const val2 = parseFloat(value[1]);
                            
                            if (!isNaN(val1) && !isNaN(val2)) {
                                window.module.set_range_slider_value(name, val1, val2);
                            } else {
                                console.warn(`Invalid range slider values for ${name}:`, value);
                            }
                        } else {
                            console.warn(`set_range_slider_value function not available for ${name}`);
                        }
                    } else if (typeof value === 'number' && !isNaN(value)) {
                        // Single slider with valid number
                        window.module.set_slider_value(name, value);
                    } else {
                        console.warn(`Invalid slider value for ${name}:`, value);
                    }
                } catch (error) {
                    console.error(`Error setting slider value for ${name}:`, error, 'Value:', value);
                }
            }
            return newValues;
        });
        
        // Notify audio system about manual slider changes
        if (window.audioHook && window.audioHook.handleManualSliderChange) {
            window.audioHook.handleManualSliderChange(name, value);
        }
    };

    // Expose slider values globally for audio system access
    useEffect(() => {
        window.reactSliderValues = sliderValues;
        console.log('🎵 📊 Updated global slider values:', sliderValues);
    }, [sliderValues]);

    // Trigger UI reset - this will cause all widgets to refresh their values
    const triggerReset = () => {
        /*setResetTrigger(prev => prev + 1);
        // Clear stored slider values since they're now reset to defaults
        setSliderValues({});*/
    };

    // Store initial slider values when panel is first loaded
    const storeInitialSliderValues = () => {
        const values = {};
        document.querySelectorAll('.jen-slider').forEach(slider => {
            const name = slider.getAttribute('data-name');
            const value = parseFloat(slider.getAttribute('data-value'));
            if (name && !isNaN(value)) {
                values[name] = value;
            }
        });
        setSliderValues(values);
    };

    // Calculate optimal panel width based on available space
    const getOptimalPanelWidth = (totalWidth, imageWidth) => {
        // Calculate available space after accounting for the image
        const availableSpace = totalWidth - imageWidth;

        // Use as much space as available, with a minimum width of panelSize
        // But ensure we don't go below panelSize or a reasonable minimum (300px)
        const minPanelWidth = Math.max(panelSize, 300);

        // If we have plenty of space, use it (up to 50% of screen width)
        if (availableSpace > minPanelWidth) {
            // Use available space, but cap at 50% of total width to prevent control panel domination
            return Math.min(availableSpace, totalWidth * 0.5);
        }

        // Otherwise use the minimum panel width, but don't exceed 30% of screen in cramped situations
        return Math.min(minPanelWidth, totalWidth * 0.3);
    };

    const resizeBox = useCallback(() => {
        let windowRatio, isRowDirection;
        let availableWidth, availableHeight;
        let imagePortWidth, imagePortHeight;
        let controlPanelWidth, controlPanelHeight;
        let bufWidth, bufHeight;

        if (window.module) {
            bufWidth = window.module.get_buf_width();
            bufHeight = window.module.get_buf_height();
        } else {
            bufWidth = 512;
            bufHeight = 512;
        }

        let ratio = bufWidth / bufHeight;
        windowRatio = window.innerWidth / window.innerHeight;

        // Determine layout direction based on screen aspect ratio
        if (windowRatio > 1.2) { // Wider screens - panel on right
            isRowDirection = false;

            // Step 1: First calculate image dimensions based on available height
            availableHeight = window.innerHeight;

            // Calculate image dimensions to maintain aspect ratio, prioritizing height first
            imagePortHeight = availableHeight;
            imagePortWidth = imagePortHeight * ratio;

            // If image width would be too large, recalculate based on a reasonable width
            const maxImageWidth = window.innerWidth * 0.7; // Image shouldn't take more than 70% of width
            if (imagePortWidth > maxImageWidth) {
                imagePortWidth = maxImageWidth;
                imagePortHeight = imagePortWidth / ratio;
            }

            // Step 2: Now determine control panel width based on remaining space
            controlPanelWidth = getOptimalPanelWidth(window.innerWidth, imagePortWidth);
            controlPanelHeight = window.innerHeight;

            // Step 3: Make final adjustment to image width if needed
            if (imagePortWidth + controlPanelWidth > window.innerWidth) {
                // Reduce image width to fit available space
                imagePortWidth = window.innerWidth - controlPanelWidth;
                imagePortHeight = imagePortWidth / ratio;
            }
        } else { // Taller/square screens - panel on bottom
            isRowDirection = true;

            // Limit control panel height
            const maxPanelHeight = Math.min(panelSize, window.innerHeight * 0.4);

            // Calculate available space for image port
            availableWidth = window.innerWidth;
            availableHeight = window.innerHeight - maxPanelHeight;

            // Calculate image port dimensions to maintain aspect ratio
            if ((availableWidth / availableHeight) > ratio) {
                // Height constrained
                imagePortHeight = availableHeight;
                imagePortWidth = imagePortHeight * ratio;
            } else {
                // Width constrained
                imagePortWidth = availableWidth;
                imagePortHeight = imagePortWidth / ratio;
            }

            // Set control panel dimensions - use full width in row direction
            controlPanelWidth = window.innerWidth;
            controlPanelHeight = maxPanelHeight;
        }

        // Update state with calculated dimensions
        setImagePortDimensions({width: imagePortWidth, height: imagePortHeight});
        setControlPanelDimensions({width: controlPanelWidth, height: controlPanelHeight});
        setIsRowDirection(isRowDirection);

        console.log(`Layout: ${isRowDirection ? 'Row' : 'Column'}, Image: ${Math.round(imagePortWidth)}x${Math.round(imagePortHeight)}, Panel: ${Math.round(controlPanelWidth)}x${Math.round(controlPanelHeight)}`);
    }, [panelSize]);

    // Set up resize callback when module is ready
    useEffect(() => {
        if (window.module) {
            window.module.set_resize_callback(resizeBox);
            setModuleReady(true);
            storeInitialSliderValues();
        } else {
            // Poll for the Module to be ready
            const intervalId = setInterval(() => {
                if (window.module) {
                    clearInterval(intervalId);
                    window.module.set_resize_callback(resizeBox);
                    setModuleReady(true);
                    storeInitialSliderValues();
                }
            }, 100);

            return () => clearInterval(intervalId);
        }
    }, [resizeBox]);

    // Handle window resize events
    useEffect(() => {
        window.addEventListener("resize", resizeBox);
        resizeBox();
        return () => window.removeEventListener("resize", resizeBox);
    }, [panelSize, resizeBox]);

    // Handle pane changes from the ControlPanel
    const handlePaneChange = (pane) => {
        setActivePane(pane);
        setTimeout(resizeBox, 0);
    };

    const paneContextValue = {
        activePane,
        setActivePane: handlePaneChange
    }

    const controlPanelContextValue = {
        sliderValues,
        onSliderChange: handleSliderChange,
        resetTrigger,
        triggerReset
    };

    return (
        <SceneProvider>
            <AudioProvider>
                <PaneContext.Provider value={paneContextValue}>
                    <ControlPanelContext.Provider value={controlPanelContextValue}>
                        <Box
                            ref={containerRef}
                            sx={{
                                display: "flex",
                                flexDirection: isRowDirection ? "column" : "row",
                                width: "100vw",
                                height: "100vh",
                                top: 0,
                                left: 0,
                                justifyContent: "center",
                                alignItems: "center",
                                overflow: "hidden",
                                background: "#121212"
                            }}
                        >
                            <Box
                                sx={{
                                    display: "flex",
                                    justifyContent: "center",
                                    alignItems: "center",
                                    width: isRowDirection ? "100%" : "auto",
                                    height: isRowDirection ? "auto" : "100%"
                                }}
                            >
                                <ImagePort dimensions={imagePortDimensions}/>
                            </Box>

                            <ControlPanel
                                dimensions={controlPanelDimensions}
                                panelSize={panelSize}
                                activePane={activePane}
                                onPaneChange={handlePaneChange}
                            />
                        </Box>
                    </ControlPanelContext.Provider>
                </PaneContext.Provider>
            </AudioProvider>
        </SceneProvider>
    );
}

export default InterfaceContainer;