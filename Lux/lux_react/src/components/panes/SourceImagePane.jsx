import { useEffect, useState, useRef } from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Alert from '@mui/material/Alert';
import { useTheme } from '@mui/material/styles';
import useMediaQuery from '@mui/material/useMediaQuery';
import WidgetGroup from '../WidgetGroup';
import MasonryImagePicker from '../MasonryImagePicker';
import { usePane } from "./PaneContext.jsx";

function SourceImagePane({ panelSize, activeGroups, onWidgetGroupChange }) {
    const { setActivePane } = usePane();

    const containerRef = useRef(null);
    const [containerWidth, setContainerWidth] = useState(0);
    const theme = useTheme();
    const isWideLayout = useMediaQuery(theme.breakpoints.up('md'));
    const controlsRef = useRef(null);
    const [controlsWidth, setControlsWidth] = useState(0);

    useEffect(() => {
        if (!containerRef.current) return;

        const resizeObserver = new ResizeObserver(entries => {
            for (let entry of entries) {
                setContainerWidth(entry.contentRect.width);
            }
        });

        resizeObserver.observe(containerRef.current);

        return () => {
            if (containerRef.current) {
                resizeObserver.unobserve(containerRef.current);
            }
        };
    }, []);

    // Monitor the controls area width separately
    useEffect(() => {
        if (!controlsRef.current) return;

        const resizeObserver = new ResizeObserver(entries => {
            for (let entry of entries) {
                setControlsWidth(entry.contentRect.width);
            }
        });

        resizeObserver.observe(controlsRef.current);

        return () => {
            if (controlsRef.current) {
                resizeObserver.unobserve(controlsRef.current);
            }
        };
    }, []);

    // Log panel size and measured width for debugging
    useEffect(() => {
        console.log(`SourceImagePane: Panel size ${panelSize}px, Container width ${containerWidth}px, Controls width ${controlsWidth}px, Wide layout: ${isWideLayout}`);
    }, [panelSize, containerWidth, controlsWidth, isWideLayout]);

    // Look for any source-related group name
    const sourceImageGroup = activeGroups.find(group =>
        group.name.toLowerCase().includes('image')
    );



    // Collect debug information
    useEffect(() => {
        if (window.module) {
            // Extract useful debug info
            const groupInfo = activeGroups.map(group => ({
                name: group.name,
                widgetCount: group.widgets ? group.widgets.length : 0
            }));

            // Find widgets that might be image pickers
            const imageWidgetsInfo = [];
            activeGroups.forEach(group => {
                if (group.widgets) {
                    group.widgets.forEach(widgetName => {
                        try {
                            const widgetJSON = window.module.get_widget_JSON(widgetName);
                            const widget = JSON.parse(widgetJSON);
                            if (widget && (widget.tool === 'image' || widget.type === 'menu_string')) {
                                imageWidgetsInfo.push({
                                    name: widgetName,
                                    group: group.name,
                                    type: widget.type,
                                    tool: widget.tool,
                                    items: widget.items ? widget.items.length : 0
                                });
                            }
                        } catch (error) {
                            console.error(`Error parsing widget JSON for ${widgetName}:`, error);
                        }
                    });
                }
            });
        }
    }, [activeGroups, sourceImageGroup]);

    // Find the image picker widget JSON if it exists
    const imagePickerJson = sourceImageGroup?.widgets
        ?.map(widgetName => {
            const widgetJSON = window.module?.get_widget_JSON(widgetName);
            try {
                const widget = JSON.parse(widgetJSON);
                return widget;
            } catch (error) {
                console.error(`Error parsing widget JSON for ${widgetName}:`, error);
                return null;
            }
        })
        .find(widget => widget?.tool === 'image');

    const shouldUseSideBySide = isWideLayout && containerWidth > 600;



    return (
        <Box
            ref={containerRef}
            sx={{
                display: 'flex',
                flexDirection: shouldUseSideBySide ? 'row' : 'column',
                padding: 1,
                height: '100%',
                overflowY: 'auto',
                gap: 2,
                width: '100%'
            }}
        >
            {/* Image Picker Section */}
            {imagePickerJson && (
                <Box
                    sx={{
                        flex: '1 0 auto',
                        width: '100%',
                    }}
                >
                    <MasonryImagePicker
                        updateFuncName={imagePickerJson.name}
                        setActivePane={setActivePane}
                        json={imagePickerJson}
                        width="100%"
                        onChange={onWidgetGroupChange}
                    />
                </Box>
            )}


            {/* Show errors and empty states */}
            {sourceImageGroup && !imagePickerJson && (
                <Alert severity="warning" sx={{ mb: 2 }}>
                    Source image group found, but no image picker widget detected.
                </Alert>
            )}

            {!sourceImageGroup && (
                <Box sx={{ textAlign: 'center', py: 4, color: 'text.secondary' }}>
                    <Typography>
                        No source image controls available. Please check your scene configuration.
                    </Typography>
                </Box>
            )}
        </Box>
    );
}

export default SourceImagePane;