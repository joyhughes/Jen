import React, { useEffect, useState, useCallback, useRef } from "react";
import {
    Box,
    useTheme,
    CircularProgress,
    Typography,
    Fade,
    IconButton,
    Tooltip
} from "@mui/material";
import { useMediaQuery } from "@mui/material";
import {
    ChevronLeft,
    ChevronRight,
    ChevronDown,
    ChevronUp
} from 'lucide-react';

// Import components
import ControlPanel from "./ControlPanel.jsx";
import EnhancedImagePort from "./ImagePort.jsx";

function InterfaceContainer() {
    const theme = useTheme();
    const [moduleReady, setModuleReady] = useState(false);
    const [loading, setLoading] = useState(true);
    const [layoutMode, setLayoutMode] = useState("side"); // "side" or "bottom"
    const [imagePortDimensions, setImagePortDimensions] = useState({ width: 0, height: 0 });
    const [controlPanelDimensions, setControlPanelDimensions] = useState({ width: 0, height: 0 });
    const [isPanelCollapsed, setIsPanelCollapsed] = useState(false);

    // Reference to track the last resize time for debouncing
    const lastResizeTime = useRef(0);

    // Default panel sizes based on layout mode
    const SIDE_PANEL_WIDTH = 340;
    const SIDE_PANEL_COLLAPSED_WIDTH = 36; // Width when collapsed
    const BOTTOM_PANEL_HEIGHT = 300;
    const BOTTOM_PANEL_COLLAPSED_HEIGHT = 36; // Height when collapsed

    // Use media query to determine default layout based on screen size
    const isSmallScreen = useMediaQuery(theme.breakpoints.down('md'));

    // Toggle control panel collapsed state
    const togglePanelCollapse = () => {
        setIsPanelCollapsed(!isPanelCollapsed);
        // Need to recalculate layout after state changes
        setTimeout(recalculateLayout, 50);
    };

    // Calculate layout based on buffer dimensions and window size
    const recalculateLayout = useCallback(() => {
        // Don't calculate too frequently (debounce)
        const now = Date.now();
        if (now - lastResizeTime.current < 50) return;
        lastResizeTime.current = now;

        // Get buffer dimensions from WebAssembly module or use defaults
        let bufWidth = 512, bufHeight = 512;
        if (window.module) {
            try {
                bufWidth = window.module.get_buf_width();
                bufHeight = window.module.get_buf_height();
            } catch (error) {
                console.error("Error getting buffer dimensions:", error);
            }
        }

        // Calculate aspect ratios
        const bufferRatio = bufWidth / bufHeight;
        const windowRatio = window.innerWidth / window.innerHeight;

        // Determine layout mode based on window ratio and buffer ratio
        const newLayoutMode = isSmallScreen || windowRatio < 1.2 ? "bottom" : "side";
        setLayoutMode(newLayoutMode);

        // Get effective panel sizes based on collapse state
        const effectiveSidePanelWidth = isPanelCollapsed ? SIDE_PANEL_COLLAPSED_WIDTH : SIDE_PANEL_WIDTH;
        const effectiveBottomPanelHeight = isPanelCollapsed ? BOTTOM_PANEL_COLLAPSED_HEIGHT : BOTTOM_PANEL_HEIGHT;

        // Calculate available space based on layout mode
        let availableWidth, availableHeight;
        if (newLayoutMode === "side") {
            availableWidth = window.innerWidth - effectiveSidePanelWidth;
            availableHeight = window.innerHeight;

            // Calculate image port dimensions to fit in available space while maintaining aspect ratio
            let imageWidth, imageHeight;
            const availableRatio = availableWidth / availableHeight;

            if (availableRatio > bufferRatio) {
                // Available space is wider than needed, constrain by height
                imageHeight = availableHeight;
                imageWidth = imageHeight * bufferRatio;
            } else {
                // Available space is taller than needed, constrain by width
                imageWidth = availableWidth;
                imageHeight = imageWidth / bufferRatio;
            }

            // Set final dimensions
            setImagePortDimensions({
                width: imageWidth,
                height: imageHeight
            });

            setControlPanelDimensions({
                width: effectiveSidePanelWidth,
                height: window.innerHeight
            });
        } else { // Bottom panel layout
            availableWidth = window.innerWidth;
            availableHeight = window.innerHeight - effectiveBottomPanelHeight;

            // Calculate image port dimensions to fit in available space while maintaining aspect ratio
            let imageWidth, imageHeight;
            const availableRatio = availableWidth / availableHeight;

            if (availableRatio > bufferRatio) {
                // Available space is wider than needed, constrain by height
                imageHeight = availableHeight;
                imageWidth = imageHeight * bufferRatio;
            } else {
                // Available space is taller than needed, constrain by width
                imageWidth = availableWidth;
                imageHeight = imageWidth / bufferRatio;
            }

            // Set final dimensions
            setImagePortDimensions({
                width: imageWidth,
                height: imageHeight
            });

            setControlPanelDimensions({
                width: window.innerWidth,
                height: effectiveBottomPanelHeight
            });
        }

        // If module is ready, finish loading after layout calculation
        if (moduleReady && loading) {
            // Small delay to ensure smooth transition
            setTimeout(() => setLoading(false), 300);
        }
    }, [moduleReady, loading, isSmallScreen, isPanelCollapsed]);

    // Set up resize callback and check for module availability
    useEffect(() => {
        const checkAndSetupModule = () => {
            if (window.module) {
                try {
                    window.module.set_resize_callback(recalculateLayout);
                    setModuleReady(true);
                    return true;
                } catch (error) {
                    console.error("Error setting resize callback:", error);
                    return false;
                }
            }
            return false;
        };

        // Try to set up immediately, if not available, poll
        if (!checkAndSetupModule()) {
            const intervalId = setInterval(() => {
                if (checkAndSetupModule()) {
                    clearInterval(intervalId);
                }
            }, 100);

            return () => clearInterval(intervalId);
        }
    }, [recalculateLayout]);

    // Add window resize listener
    useEffect(() => {
        window.addEventListener("resize", recalculateLayout);
        recalculateLayout(); // Initial calculation

        return () => window.removeEventListener("resize", recalculateLayout);
    }, [recalculateLayout]);

    // Loading screen
    if (loading) {
        return (
            <Box
                sx={{
                    display: "flex",
                    flexDirection: "column",
                    alignItems: "center",
                    justifyContent: "center",
                    width: "100vw",
                    height: "100vh",
                    backgroundColor: theme.palette.background.default
                }}
            >
                <CircularProgress size={48} sx={{ mb: 2 }} />
                <Typography variant="h6" color="text.secondary">
                    Initializing Jen...
                </Typography>
                <Typography variant="body2" color="text.secondary" sx={{ mt: 1 }}>
                    Loading generative art environment
                </Typography>
            </Box>
        );
    }

    return (
        <Fade in={!loading} timeout={500}>
            <Box
                sx={{
                    display: "flex",
                    flexDirection: layoutMode === "side" ? "row" : "column",
                    width: "100vw",
                    height: "100vh",
                    overflow: "hidden",
                    backgroundColor: theme.palette.background.default
                }}
            >
                {/* Canvas Area */}
                <Box
                    sx={{
                        display: "flex",
                        justifyContent: "center",
                        alignItems: "center",
                        width: layoutMode === "side"
                            ? `calc(100% - ${isPanelCollapsed ? SIDE_PANEL_COLLAPSED_WIDTH : SIDE_PANEL_WIDTH}px)`
                            : "100%",
                        height: layoutMode === "side"
                            ? "100%"
                            : `calc(100% - ${isPanelCollapsed ? BOTTOM_PANEL_COLLAPSED_HEIGHT : BOTTOM_PANEL_HEIGHT}px)`,
                        overflow: "hidden",
                        backgroundColor: theme.palette.background.subtle,
                        transition: "all 0.3s ease-in-out"
                    }}
                >
                    <EnhancedImagePort
                        dimensions={imagePortDimensions}
                        moduleReady={moduleReady}
                    />
                </Box>

                {/* Control Panel */}
                <Box
                    sx={{
                        position: "relative",
                        width: layoutMode === "side"
                            ? (isPanelCollapsed ? SIDE_PANEL_COLLAPSED_WIDTH : SIDE_PANEL_WIDTH)
                            : "100%",
                        height: layoutMode === "side"
                            ? "100%"
                            : (isPanelCollapsed ? BOTTOM_PANEL_COLLAPSED_HEIGHT : BOTTOM_PANEL_HEIGHT),
                        borderLeft: layoutMode === "side" ? `1px solid ${theme.palette.divider}` : "none",
                        borderTop: layoutMode === "bottom" ? `1px solid ${theme.palette.divider}` : "none",
                        backgroundColor: theme.palette.background.paper,
                        overflow: "hidden",
                        transition: "all 0.3s ease-in-out",
                        display: "flex",
                        flexDirection: "column"
                    }}
                >
                    {/* Collapse/Expand Button */}
                    <Tooltip
                        title={isPanelCollapsed ? "Expand Panel" : "Collapse Panel"}
                        placement={layoutMode === "side" ? "left" : "top"}
                    >
                        <IconButton
                            onClick={togglePanelCollapse}
                            size="small"
                            sx={{
                                position: "absolute",
                                [layoutMode === "side" ? "left" : "top"]: 2,
                                [layoutMode === "side" ? "top" : "right"]: 2,
                                backgroundColor: theme.palette.background.paper,
                                border: `1px solid ${theme.palette.divider}`,
                                zIndex: 10,
                                '&:hover': {
                                    backgroundColor: theme.palette.action.hover
                                }
                            }}
                        >
                            {layoutMode === "side"
                                ? (isPanelCollapsed ? <ChevronRight size={18} /> : <ChevronLeft size={18} />)
                                : (isPanelCollapsed ? <ChevronDown size={18} /> : <ChevronUp size={18} />)
                            }
                        </IconButton>
                    </Tooltip>

                    {/* Control Panel Content - Only show if not collapsed */}
                    {!isPanelCollapsed ? (
                        <ControlPanel
                            dimensions={controlPanelDimensions}
                            panelSize={layoutMode === "side" ? SIDE_PANEL_WIDTH - 16 : window.innerWidth - 16}
                            moduleReady={moduleReady}
                        />
                    ) : (
                        // If collapsed, show a hint or title in vertical orientation for side mode
                        layoutMode === "side" && (
                            <Box
                                sx={{
                                    height: '100%',
                                    width: '100%',
                                    display: 'flex',
                                    alignItems: 'center',
                                    justifyContent: 'center'
                                }}
                            >
                                <Typography
                                    variant="subtitle2"
                                    sx={{
                                        transform: 'rotate(-90deg)',
                                        whiteSpace: 'nowrap',
                                        color: theme.palette.text.secondary
                                    }}
                                >
                                    Controls
                                </Typography>
                            </Box>
                        )
                    )}
                </Box>
            </Box>
        </Fade>
    );
}

export default InterfaceContainer;