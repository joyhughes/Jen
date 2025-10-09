import React, {useEffect, useRef, useState, useMemo} from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';
import Stack from '@mui/material/Stack';
import Button from '@mui/material/Button';
import Masonry from 'react-masonry-css';
import WidgetGroup from '../WidgetGroup';
import { BiImageAlt } from 'react-icons/bi';
import { usePane } from './PaneContext';
import ThumbnailCanvas from '../ThumbnailCanvas';

function HomePane({ dimensions, panelSize, panelJSON, activeGroups, onWidgetGroupChange }) {
    const { setActivePane } = usePane();
    const containerRef = useRef(null);
    const [containerWidth, setContainerWidth] = useState(null);
    const [thumbnailStatus, setThumbnailStatus] = useState('loading');

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

    // Find groups that have image picker widgets
    const imageGroups = activeGroups.filter(group => {
        if (group.widgets) {
            return group.widgets.some(widgetName => {
                try {
                    const widgetJSON = window.module?.get_widget_JSON(widgetName);
                    const widget = JSON.parse(widgetJSON);
                    return widget?.tool === 'image';
                } catch (error) {
                    return false;
                }
            });
        }
        return false;
    });

    // Get the active/selected image from any image group
    const getActiveImage = () => {
        for (const group of imageGroups) {
            if (group.widgets) {
                for (const widgetName of group.widgets) {
                    try {
                        const widgetJSON = window.module?.get_widget_JSON(widgetName);
                        const widget = JSON.parse(widgetJSON);
                        if (widget?.tool === 'image' && widget?.items && Array.isArray(widget.items)) {
                            // Check for selected index in the same way as ImagePane/MasonryImagePicker
                            let selectedIdx = -1;
                            if (widget.choice !== undefined && Number.isInteger(widget.choice)) {
                                selectedIdx = widget.choice;
                            } else if (widget.selected !== undefined && Number.isInteger(widget.selected)) {
                                selectedIdx = widget.selected;
                            } else if (widget.value !== undefined && Number.isInteger(widget.value)) {
                                selectedIdx = widget.value;
                            }
                            
                            const activeItem = selectedIdx >= 0 && selectedIdx < widget.items.length 
                                ? widget.items[selectedIdx] 
                                : (widget.items.length > 0 ? widget.items[0] : null);
                            
                            if (activeItem) {
                                return { path: activeItem, name: widget.name || widgetName };
                            }
                        }
                    } catch (error) {
                        console.error(`Error parsing widget JSON for ${widgetName}:`, error);
                    }
                }
            }
        }
        return null;
    };

    const activeImage = getActiveImage();

    // Determine if the active image is source or target based on widget name
    const getImageLabel = () => {
        if (!activeImage) return "Images";
        const widgetName = activeImage.name.toLowerCase();
        if (widgetName.includes('source')) return "Source Image";
        if (widgetName.includes('target')) return "Target Image";
        return "Images";
    };

    // Filter out any group that has image picker widgets for the Home pane
    const nonImageGroups = activeGroups.filter(group => {
        // For groups, check if they have image pickers
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
            {imageGroups.length > 0 && (
                <Box sx={{ mb: 2, display: 'flex', justifyContent: 'flex-start' }}>
                    <Button
                        variant="outlined"
                        size="small"
                        startIcon={<BiImageAlt />}
                        onClick={() => setActivePane('image')}
                        sx={{
                            py: 0.75,
                            px: 1.5,
                            borderColor: 'primary.main',
                            color: 'primary.main',
                            backgroundColor: activeImage ? 'rgba(25, 118, 210, 0.04)' : 'transparent',
                            '&:hover': {
                                backgroundColor: 'rgba(25, 118, 210, 0.08)',
                                borderColor: 'primary.dark',
                            },
                            borderRadius: 2,
                            minWidth: 'auto'
                        }}
                    >
                        <Box sx={{ 
                            display: 'flex', 
                            alignItems: 'center', 
                            gap: 1
                        }}>
                            <Typography variant="body2" sx={{ fontWeight: 500 }}>
                                {getImageLabel()}
                            </Typography>
                            {activeImage && (
                                <Box
                                    sx={{
                                        width: 32,
                                        height: 32,
                                        borderRadius: 0.5,
                                        border: '1px solid',
                                        borderColor: 'primary.main',
                                        boxShadow: 1,
                                        overflow: 'hidden',
                                        flexShrink: 0
                                    }}
                                >
                                    <ThumbnailCanvas
                                        imageName={activeImage.path}
                                        width={32}
                                        height={32}
                                        setStatus={setThumbnailStatus}
                                    />
                                </Box>
                            )}
                        </Box>
                    </Button>
                </Box>
            )}

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