import React, { useState } from "react";
import {
    Stack,
    Typography,
    Tooltip,
    IconButton,
    Paper,
    Box,
    Divider,
    Collapse,
    useTheme,
    alpha
} from '@mui/material';
import {
    ChevronDown,
    ChevronUp,
    X as CloseIcon,
    Plus as AddIcon,
    Info
} from 'lucide-react';

// Import our enhanced components
import { JenImagePicker } from "./JenImagePicker.jsx";
import JenSlider from './JenSlider.jsx';
import JenSwitch from './JenSwitch.jsx';
import JenDirection8 from './JenDirection8'; // Assuming you'll enhance this later
import JenDirection4 from './JenDirection4'; // Assuming you'll enhance this later
import JenBlurPicker from './JenBlurPicker'; // Assuming you'll enhance this later
import JenMultiDirection8 from './JenMultiDirection8'; // Assuming you'll enhance this later

// A styled container for widget groups that can be collapsed
const WidgetGroupContainer = ({ title, description, children, defaultExpanded = true }) => {
    const theme = useTheme();
    const [expanded, setExpanded] = useState(defaultExpanded);

    return (
        <Paper
            elevation={0}
            variant="outlined"
            sx={{
                borderRadius: 1.5,
                mb: 2,
                overflow: 'hidden',
                borderColor: theme.palette.divider,
                transition: 'all 0.2s ease-in-out',
                '&:hover': {
                    borderColor: expanded ? theme.palette.primary.main : theme.palette.action.active,
                },
            }}
        >
            {/* Header */}
            <Box
                sx={{
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'space-between',
                    px: 2,
                    py: 1,
                    backgroundColor: expanded
                        ? alpha(theme.palette.primary.main, 0.08)
                        : theme.palette.background.paper,
                    cursor: 'pointer',
                    transition: 'background-color 0.2s ease-in-out',
                    '&:hover': {
                        backgroundColor: expanded
                            ? alpha(theme.palette.primary.main, 0.12)
                            : theme.palette.action.hover,
                    },
                }}
                onClick={() => setExpanded(!expanded)}
            >
                <Box sx={{ display: 'flex', alignItems: 'center' }}>
                    <Typography
                        variant="subtitle2"
                        fontWeight={600}
                        color={expanded ? 'primary.main' : 'text.primary'}
                    >
                        {title}
                    </Typography>
                    {description && (
                        <Tooltip title={description} arrow placement="top">
                            <Box component="span" sx={{ ml: 0.75, display: 'flex', alignItems: 'center' }}>
                                <Info size={16} color={theme.palette.text.secondary} />
                            </Box>
                        </Tooltip>
                    )}
                </Box>
                <IconButton
                    size="small"
                    sx={{ color: expanded ? 'primary.main' : 'text.secondary' }}
                >
                    {expanded ? <ChevronUp size={18} /> : <ChevronDown size={18} />}
                </IconButton>
            </Box>

            {/* Content */}
            <Collapse in={expanded}>
                <Divider />
                <Box sx={{ p: 2 }}>
                    {children}
                </Box>
            </Collapse>
        </Paper>
    );
};

// Individual widget container
const WidgetItemContainer = ({ children, title, description, type }) => {
    const theme = useTheme();

    // Return the children directly for components that handle their own container
    if (type === 'slider_int' || type === 'slider_float' ||
        type === 'range_slider_int' || type === 'range_slider_float' ||
        type === 'switch_fn' || type === 'switch_condition' ||
        type === 'menu_int' || type === 'menu_string') {
        return children;
    }

    // For other components, provide a container
    return (
        <Box
            sx={{
                mb: 2,
                '&:last-child': {
                    mb: 0,
                }
            }}
        >
            {title && (
                <Box sx={{ mb: 1, display: 'flex', alignItems: 'center' }}>
                    <Typography variant="body2" fontWeight={500}>
                        {title}
                    </Typography>
                    {description && (
                        <Tooltip title={description} arrow placement="top">
                            <Box component="span" sx={{ ml: 0.75, display: 'flex', alignItems: 'center' }}>
                                <Info size={14} color={theme.palette.text.secondary} />
                            </Box>
                        </Tooltip>
                    )}
                </Box>
            )}
            {children}
        </Box>
    );
};

function JenWidgetGroup({ panelSize, json, onChange }) {
    const theme = useTheme();
    const [renderCount, setRenderCount] = useState(0);

    const renderWidget = (name) => {
        let widgetComponent;
        let widget;

        // Fetch widget JSON from WebAssembly
        const widgetJSON = window.module.get_widget_JSON(name);

        try {
            widget = JSON.parse(widgetJSON);
        } catch (error) {
            console.error("renderWidget Error parsing JSON:", error);
            return null;
        }

        if (!widget) {
            console.error(`No widget found for name: ${name}`);
            return null;
        }

        // Switch based on widget type to render the appropriate component
        switch (widget.type) {
            case 'menu_int':
            case 'menu_string':
                if (widget.name === 'source_image_menu') {
                    widgetComponent = (
                        <JenImagePicker
                            json={widget}
                            width={panelSize ? panelSize - 16 : 280}
                            onChange={onChange}
                        />
                    );
                }
                break;

            case 'slider_int':
            case 'slider_float':
            case 'range_slider_int':
            case 'range_slider_float':
                widgetComponent = (
                    <JenSlider
                        json={widget}
                        width={panelSize - 32}
                    />
                );
                break;

            case 'switch_fn':
            case 'switch_condition':
                widgetComponent = (
                    <JenSwitch
                        json={widget}
                        onChange={onChange}
                    />
                );
                break;

            case 'direction_picker_8':
                widgetComponent = (
                    <WidgetItemContainer
                        title={widget.label}
                        description={widget.description}
                        type={widget.type}
                    >
                        <JenDirection8 key={widget.name} json={widget} />
                    </WidgetItemContainer>
                );
                break;

            case 'direction_picker_4':
                widgetComponent = (
                    <WidgetItemContainer
                        title={widget.label}
                        description={widget.description}
                        type={widget.type}
                    >
                        <JenDirection4 key={widget.name} json={widget} />
                    </WidgetItemContainer>
                );
                break;

            case 'box_blur_picker':
                widgetComponent = (
                    <WidgetItemContainer
                        title={widget.label}
                        description={widget.description}
                        type={widget.type}
                    >
                        <JenBlurPicker key={widget.name} json={widget} />
                    </WidgetItemContainer>
                );
                break;

            case 'custom_blur_picker':
                const pickerElements = [];

                for (let i = 0; i < widget.pickers.length; i++) {
                    const handleClose = () => {
                        window.module.remove_custom_blur_pickers(widget.name, i);
                        setRenderCount(renderCount + 1);
                    };

                    const picker = widget.pickers[i];
                    const element = (
                        <Box
                            key={i}
                            sx={{
                                display: 'flex',
                                alignItems: 'center',
                                mb: 1,
                                p: 1,
                                borderRadius: 1,
                                border: '1px solid',
                                borderColor: 'divider',
                                '&:hover': {
                                    borderColor: 'action.active',
                                }
                            }}
                        >
                            <Stack spacing={1} direction="row" alignItems="center" sx={{ flexGrow: 1 }}>
                                <JenMultiDirection8
                                    name={widget.name}
                                    value={picker[0]}
                                    code={i}
                                    key={i + renderCount * 256}
                                />
                                <JenMultiDirection8
                                    name={widget.name}
                                    value={picker[1]}
                                    code={i + 128}
                                    key={i + 128 + renderCount * 256}
                                />
                            </Stack>
                            <IconButton
                                size="small"
                                onClick={handleClose}
                                sx={{
                                    ml: 1,
                                    color: theme.palette.error.main,
                                    '&:hover': {
                                        backgroundColor: alpha(theme.palette.error.main, 0.08),
                                    }
                                }}
                            >
                                <CloseIcon size={16} />
                            </IconButton>
                        </Box>
                    );
                    pickerElements.push(element);
                }

                const handleAddPicker = () => {
                    window.module.add_custom_blur_pickers(widget.name);
                    setRenderCount(renderCount + 1);
                };

                widgetComponent = (
                    <WidgetItemContainer
                        title={widget.label}
                        description={widget.description}
                        type={widget.type}
                    >
                        <Paper
                            variant="outlined"
                            sx={{
                                p: 2,
                                borderRadius: 1,
                                backgroundColor: theme.palette.background.paper,
                            }}
                        >
                            {pickerElements.length > 0 ? (
                                pickerElements
                            ) : (
                                <Typography
                                    variant="body2"
                                    color="text.secondary"
                                    sx={{ textAlign: 'center', fontStyle: 'italic', mb: 2 }}
                                >
                                    No pickers added yet
                                </Typography>
                            )}

                            <Box sx={{ display: 'flex', justifyContent: 'center', mt: 1 }}>
                                <IconButton
                                    onClick={handleAddPicker}
                                    size="small"
                                    sx={{
                                        backgroundColor: theme.palette.primary.main,
                                        color: theme.palette.primary.contrastText,
                                        borderRadius: '50%',
                                        '&:hover': {
                                            backgroundColor: theme.palette.primary.dark,
                                        },
                                    }}
                                >
                                    <AddIcon size={18} />
                                </IconButton>
                            </Box>
                        </Paper>
                    </WidgetItemContainer>
                );
                break;

            case 'widget_switch_fn':
                // Handle the widget switch function type
                let ws_widget;
                let switcher;

                try {
                    const ws_widgetJSON = window.module.get_widget_JSON(widget.widget);
                    ws_widget = JSON.parse(ws_widgetJSON);

                    const switcherJSON = window.module.get_widget_JSON(widget.switcher);
                    switcher = JSON.parse(switcherJSON);
                } catch (error) {
                    console.error("Error parsing JSON:", error);
                    return null;
                }

                if (ws_widget && switcher) {
                    switch (ws_widget.type) {
                        case 'slider_int':
                        case 'slider_float':
                        case 'range_slider_int':
                        case 'range_slider_float':
                            widgetComponent = (
                                <Box sx={{ mb: 2 }}>
                                    <Box
                                        sx={{
                                            display: 'flex',
                                            alignItems: 'center',
                                            mb: 1.5,
                                            justifyContent: 'space-between',
                                        }}
                                    >
                                        <Box sx={{ display: 'flex', alignItems: 'center' }}>
                                            <Typography variant="body2" fontWeight={500}>
                                                {widget.label}
                                            </Typography>
                                            {widget.description && (
                                                <Tooltip title={widget.description} arrow placement="top">
                                                    <Box component="span" sx={{ ml: 0.75, display: 'flex', alignItems: 'center' }}>
                                                        <Info size={14} color={theme.palette.text.secondary} />
                                                    </Box>
                                                </Tooltip>
                                            )}
                                        </Box>
                                        <JenSwitch
                                            json={switcher}
                                            onChange={onChange}
                                        />
                                    </Box>
                                    <JenSlider
                                        json={ws_widget}
                                        width={panelSize - 32}
                                    />
                                </Box>
                            );
                            break;

                        default:
                            widgetComponent = (
                                <Typography color="error">
                                    Widget Switch - Unknown widget type: {ws_widget.type}
                                </Typography>
                            );
                            break;
                    }
                }
                break;

            default:
                widgetComponent = (
                    <Typography color="error">
                        Unknown widget type: {widget.type}
                    </Typography>
                );
                break;
        }

        return widgetComponent ? (
            <Box key={widget.name} sx={{ mb: 2 }}>
                {widgetComponent}
            </Box>
        ) : null;
    };

    // Group widgets by category if specified in json
    const groupedWidgets = {};
    const defaultGroup = 'Controls';

    // Process all widgets and organize them by group
    json.widgets.forEach(name => {
        try {
            const widgetJSON = window.module.get_widget_JSON(name);
            const widget = JSON.parse(widgetJSON);

            // Use the group property if it exists, otherwise use the default group
            const group = widget.group || defaultGroup;

            if (!groupedWidgets[group]) {
                groupedWidgets[group] = {
                    widgets: [],
                    description: widget.groupDescription
                };
            }

            groupedWidgets[group].widgets.push(name);

            // Store group description if it exists and hasn't been set yet
            if (widget.groupDescription && !groupedWidgets[group].description) {
                groupedWidgets[group].description = widget.groupDescription;
            }
        } catch (error) {
            console.error(`Error processing widget ${name}:`, error);
        }
    });

    return (
        <Box sx={{ p: 1 }}>
            {Object.keys(groupedWidgets).map((groupName, index) => {
                const group = groupedWidgets[groupName];

                return (
                    <WidgetGroupContainer
                        key={`group-${index}`}
                        title={groupName}
                        description={group.description}
                        defaultExpanded={true}
                    >
                        {group.widgets.map(renderWidget)}
                    </WidgetGroupContainer>
                );
            })}
        </Box>
    );
}

export default JenWidgetGroup;