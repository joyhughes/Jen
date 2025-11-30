import { useEffect, useRef, useState, useMemo } from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Stack from '@mui/material/Stack';
import WidgetGroup from '../WidgetGroup';
import Button from "@mui/material/Button"
import { BiImageAlt } from 'react-icons/bi';
import { usePane } from './PaneContext'
import ThumbnailCanvas from '../ThumbnailCanvas';
import Divider from '@mui/material/Divider';



// here the groups will contain a list of widgets 
function HomePane({ panelSize, activeGroups, onWidgetGroupChange }) {
    const { setActivePane } = usePane();
    const containerRef = useRef(null);
    const [containerWidth, setContainerWidth] = useState(null);
    const [imagePreviewWidgets, setImagePreviewWidgets] = useState([])

    useEffect(() => {
        const resizeObserver = new ResizeObserver(entries => {
            for (let entry of entries) {
                setContainerWidth(entry.contentRect.width);
            }
        });
        if (containerRef.current) {
            resizeObserver.observe(containerRef.current);
        }

        if (activeGroups) {
            const activeWidgetNames = activeGroups.map(group => {
                return group.widgets
            })
            const activeImageWidgets = activeWidgetNames
                .flatMap(widgets => widgets)
                .filter(widget => isImageWidget(widget))
                .map(imageWidget => {
                    const widget = {
                        ...getActivePreviewImage(imageWidget),
                    }
                    return widget

                })
            setImagePreviewWidgets(activeImageWidgets)
        }

        return () => {
            if (containerRef.current) {
                resizeObserver.unobserve(containerRef.current);
            }
        };
    }, [activeGroups]);


    const breakpointColumns = useMemo(() => {
        const MIN_GROUP_WIDTH = 300;

        const availableWidth = containerWidth || panelSize;

        const maxColumns = Math.max(1, Math.floor(availableWidth / MIN_GROUP_WIDTH));

        const breakpoints = {
            default: maxColumns,
            900: Math.min(maxColumns, 2),
            600: 1
        };

        console.log("Home group breakpoints:", breakpoints, "Available width:", availableWidth);
        return breakpoints;
    }, [containerWidth, panelSize]);

    const isShortMenu = (widgetName) => {
        try {
            const widgetJSON = window.module?.get_widget_JSON(widgetName);
            const widget = JSON.parse(widgetJSON);
            if (widget?.tool === 'pull_down' || widget?.tool === 'radio') {
                const maxLength = Math.max(...(widget.items || []).map(item => item.length));
                return maxLength < 20;
            }
        } catch (error) {
            return false;
        }
        return false;
    };

    const isMenu = (widgetName) => {
        try {
            const widgetJSON = window.module?.get_widget_JSON(widgetName);
            const widget = JSON.parse(widgetJSON);
            return widget?.tool === 'pull_down' || widget?.tool === 'radio';
        } catch (error) {
            return false;
        }
    };

    const groupWidgets = (groups) => {
        const allWidgets = groups.filter(group => !group.name.includes("image")).flatMap(group => group.widgets);
        const groupedWidgets = [];
        let i = 0;

        while (i < allWidgets.length) {
            const currentWidget = allWidgets[i];

            if (i < allWidgets.length - 1 &&
                isShortMenu(currentWidget) &&
                isShortMenu(allWidgets[i + 1])) {
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



    const isImageWidget = (widgetName) => {
        if (!widgetName) return false;
        const widgetJSON = window.module?.get_widget_JSON(widgetName)
        const widget = JSON.parse(widgetJSON)
        return widget?.tool === 'image'
    }

    const getActivePreviewImage = (imgWidgetName) => {
        const rawWidget = window.module?.get_widget_JSON(imgWidgetName)
        const imageWidget = JSON.parse(rawWidget)
        if (imageWidget?.items && Array.isArray(imageWidget.items)) {
            let selectedIdx = -1
            if (imageWidget.choice && Number.isInteger(imageWidget.choice)) {
                selectedIdx = parseInt(imageWidget.choice)
            } else if (imageWidget.selected && Number.isInteger(imageWidget.selected)) {
                selectedIdx = parseInt(imageWidget.selected)
            } else if (imageWidget.value && Number.isInteger(imageWidget.value)) {
                selectedIdx = parseInt(imageWidget.value);
            }
            if (selectedIdx != -1) {
                const activeItem = selectedIdx >= 0 && selectedIdx < imageWidget.items.length ? imageWidget.items[selectedIdx] : (widget.items.length > 0 ? widget.items[0] : null)
                if (activeItem) {
                    const obj = {
                        imageName: activeItem,
                        name: imageWidget.name,
                        label: imageWidget.label
                    }
                    return obj
                }
            }
            return {}
        }

        return {}
    }

    const renderWidgetGroup = (groupedWidget, index) => {
        const breakpoints = breakpointColumns;
        const panelWidth = (containerWidth || panelSize) / breakpoints.default;

        if (groupedWidget.type === 'row') {
            return (
                <Box
                    key={`row-${index}`}
                    sx={{
                        width: '100%',
                        px: 1.5,
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
                                />
                            </Box>
                        ))}
                    </Stack>
                </Box>
            );
        } else {
            const isMenuWidget = isMenu(groupedWidget.widget);
            return (
                <Box
                    key={groupedWidget.widget}
                    sx={{
                        width: '100%',
                        px: isMenuWidget ? 1.5 : 1.5,
                    }}
                >
                    <WidgetGroup
                        json={{ widgets: [groupedWidget.widget] }}
                        panelSize={isMenuWidget ? panelWidth - 24 : panelWidth}
                        onChange={onWidgetGroupChange}
                    />
                </Box>
            );
        }
    };

    const displayImagePreview = (imageWidget) => {
        return (
            <Button
                key={imageWidget.name + ":" + imageWidget.imageName}
                variant="outlined"
                size="small"
                startIcon={<BiImageAlt />}
                onClick={() => setActivePane(imageWidget.name.includes("source") ? "source" : "target")}
                sx={{
                    py: 0.75,
                    px: 1.5,
                    borderColor: 'primary.main',
                    color: 'primary.main',
                    backgroundColor: 'rgba(25, 118, 210, 0.04)',
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
                        {imageWidget.label}
                    </Typography>
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
                            imageName={imageWidget.imageName}
                            width={32}
                            height={32}
                        />
                    </Box>
                </Box>
            </Button>
        )

    }


    const displayImagePreviews = (imageWidgets) => {
        return (
            <Box sx={{ mb: 2, display: 'flex', justifyContent: 'flex-start' }}>
                {imageWidgets.map(displayImagePreview)}
            </Box>
        )
    }



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
            {imagePreviewWidgets.length != 0 && displayImagePreviews(imagePreviewWidgets)}
            <Divider sx={{ mb: 2 }} />

            {activeGroups.length > 0 ? (
                <Box sx={{ width: '100%' }}>
                    {groupWidgets(activeGroups).map((groupedWidget, index) =>
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