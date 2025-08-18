import React, {useEffect, useRef, useState, useMemo} from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';
import Stack from '@mui/material/Stack';
import Button from '@mui/material/Button';
import SaveIcon from '@mui/icons-material/Save';
import Masonry from 'react-masonry-css';
import WidgetGroup from '../WidgetGroup';

function HomePane({ dimensions, panelSize, panelJSON, activeGroups, onWidgetGroupChange, onSaveScene }) {
    const containerRef = useRef(null);
    const [containerWidth, setContainerWidth] = useState(null);

    // Set up ResizeObserver to track container width
    useEffect(() => {
        const resizeObserver = new ResizeObserver(entries => {
            for (let entry of entries) {
                setContainerWidth(entry.contentRect.width);
            }
        });

        if (containerRef.current) {
            resizeObserver.observe(containerRef.current);
        }

        return () => {
            if (containerRef.current) {
                resizeObserver.unobserve(containerRef.current);
            }
        };
    }, []);

    // Calculate responsive breakpoints for the group masonry layout - MEMOIZED
    const breakpointColumns = useMemo(() => {
        // Minimum width for a group column
        const MIN_GROUP_WIDTH = 300;

        // If we have the actual container width, use it, otherwise use panelSize
        const availableWidth = containerWidth || panelSize;

        // Calculate how many columns can fit
        const maxColumns = Math.max(1, Math.floor(availableWidth / MIN_GROUP_WIDTH));

        // Create breakpoint object for Masonry
        const breakpoints = {
            default: maxColumns,
            900: Math.min(maxColumns, 2),  // Max 2 columns on medium screens
            600: 1                         // Single column on small screens
        };

        // Only log when values actually change
        console.log("Home group breakpoints:", breakpoints, "Available width:", availableWidth);
        return breakpoints;
    }, [containerWidth, panelSize]); // Only recalculate when width changes

    // Filter out any group that has image picker widgets for the Home pane
    const nonImageGroups = activeGroups.filter(group => {
        // Skip source/target image groups entirely
        if (group.name.toLowerCase().includes('source') ||
            group.name.toLowerCase().includes('target') ||
            group.name === 'SOURCE_IMAGE_GROUP' ||
            group.name === 'TARGET_IMAGE_GROUP'
            ) 
            {
            return false;
        }

        // For other groups, check if they have image pickers
        if (group.widgets) {
            const hasImageWidget = group.widgets.some(widgetName => {
                try {
                    const widgetJSON = window.module?.get_widget_JSON(widgetName);
                    const widget = JSON.parse(widgetJSON);
                    return widget?.tool === 'image';
                } catch (error) {
                    return false;
                }
            });

            return !hasImageWidget;
        }

        return true;
    });

    // Helper function to check if a widget is a menu with short text
    const isShortMenu = (widgetName) => {
        try {
            const widgetJSON = window.module?.get_widget_JSON(widgetName);
            const widget = JSON.parse(widgetJSON);
            if (widget?.tool === 'pull_down' || widget?.tool === 'radio') {
                // Check if all menu items are short (less than 20 characters)
                const maxLength = Math.max(...(widget.items || []).map(item => item.length));
                return maxLength < 20;
            }
        } catch (error) {
            return false;
        }
        return false;
    };

    // Helper function to check if widget is a menu
    const isMenu = (widgetName) => {
        try {
            const widgetJSON = window.module?.get_widget_JSON(widgetName);
            const widget = JSON.parse(widgetJSON);
            return widget?.tool === 'pull_down' || widget?.tool === 'radio';
        } catch (error) {
            return false;
        }
    };

    // Group widgets intelligently
    const groupWidgets = () => {
        const allWidgets = nonImageGroups.flatMap(group => group.widgets);
        const groupedWidgets = [];
        let i = 0;

        while (i < allWidgets.length) {
            const currentWidget = allWidgets[i];
            
            // Check if current widget is a short menu and next widget is also a short menu
            if (i < allWidgets.length - 1 && 
                isShortMenu(currentWidget) && 
                isShortMenu(allWidgets[i + 1])) {
                // Group two short menus together
                groupedWidgets.push({
                    type: 'row',
                    widgets: [currentWidget, allWidgets[i + 1]]
                });
                i += 2;
            } else {
                // Single widget
                groupedWidgets.push({
                    type: 'single',
                    widget: currentWidget
                });
                i++;
            }
        }

        return groupedWidgets;
    };

    const renderWidgetGroup = (groupedWidget, index) => {
        const breakpoints = breakpointColumns;
        const panelWidth = (containerWidth || panelSize) / breakpoints.default;

        if (groupedWidget.type === 'row') {
            // Render two menus in a row
            return (
                <Box 
                    key={`row-${index}`}
                    sx={{ 
                        width: '100%',
                        px: 1.5, // Consistent padding that shifts content right
                        mb: 1
                    }}
                >
                    <Stack direction="row" spacing={1.5}>
                        {groupedWidget.widgets.map((widgetName, widgetIndex) => (
                            <Box key={widgetName} sx={{ flex: 1, minWidth: 0 }}>
                                <WidgetGroup
                                    json={{ widgets: [widgetName] }}
                                    panelSize={panelWidth / 2} 
                                    onChange={onWidgetGroupChange}
                                    disableImageWidgets={true}
                                />
                            </Box>
                        ))}
                    </Stack>
                </Box>
            );
        } else {
            // Single widget with full width
            const isMenuWidget = isMenu(groupedWidget.widget);
            return (
                <Box 
                    key={groupedWidget.widget}
                    sx={{ 
                        width: '100%',
                        px: isMenuWidget ? 1.5 : 1.5, // Add padding for menus to match sliders
                    }}
                >
                    <WidgetGroup
                        json={{ widgets: [groupedWidget.widget] }}
                        panelSize={isMenuWidget ? panelWidth - 24 : panelWidth} // Adjust width for menu padding
                        onChange={onWidgetGroupChange}
                        disableImageWidgets={true}
                    />
                </Box>
            );
        }
    };

    return (
        <Box
            ref={containerRef}
            sx={{
                display: 'flex',
                flexDirection: 'column',
                px: 1.25, // Increased padding to shift content right
                py: 1,
                height: '100%',
                overflowY: 'auto',
                width: '100%'
            }}
        >
            {/* Save Scene Button */}
            <Box sx={{ mb: 2 }}>
                <Button
                    variant="contained"
                    startIcon={<SaveIcon />}
                    onClick={onSaveScene}
                    sx={{
                        width: '100%',
                        mb: 1,
                        backgroundColor: 'primary.main',
                        '&:hover': {
                            backgroundColor: 'primary.dark',
                        }
                    }}
                    disabled={!onSaveScene}
                >
                    Save Scene State
                </Button>
            </Box>
            
            <Divider sx={{ mb: 2 }} />

            {nonImageGroups.length > 0 ? (
                <Box sx={{ width: '100%' }}>
                    {groupWidgets().map((groupedWidget, index) => 
                        renderWidgetGroup(groupedWidget, index)
                    )}
                </Box>
            ) : (
                <Box sx={{ textAlign: 'center', py: 4, color: 'text.secondary' }}>
                    <Typography>
                        No active widget groups available. Select a scene to begin.
                    </Typography>
                </Box>
            )}
        </Box>
    );
}

export default HomePane;