import React, { useState, useEffect, useRef } from "react";
import { Box, Typography, IconButton, useTheme } from '@mui/material';
import Masonry from 'react-masonry-css';
import JenMenu from './JenMenu';
import JenSlider from './JenSlider';
import JenSwitch from './JenSwitch';
import JenDirection8 from './JenDirection8';
import JenDirection4 from './JenDirection4';
import JenDirection4Diagonal from './JenDirection4Diagonal';
import JenBlurPicker from './JenBlurPicker';
import JenMultiDirection8 from './JenMultiDirection8';
import JenFunkyPicker from './JenFunkyPicker';
import JenColorPicker from './JenColorPicker';
import MasonryImagePicker from './MasonryImagePicker';
import { Plus, X } from 'lucide-react';
import { ControlPanelContext } from './InterfaceContainer';

function WidgetGroup({ json, panelSize, onChange}) {
    const [widgetElements, setWidgetElements] = useState([]);
    const containerRef = useRef(null);
    const [containerWidth, setContainerWidth] = useState(0);
    
    const { resetTrigger } = false;

    // Use ResizeObserver to detect actual width changes
    useEffect(() => {
        if (!containerRef.current) return;

        const resizeObserver = new ResizeObserver(entries => {
            for (let entry of entries) {
                const width = entry.contentRect.width;
                if (width > 0) {
                    setContainerWidth(width);
                }
            }
        });

        resizeObserver.observe(containerRef.current);

        return () => {
            if (containerRef.current) {
                resizeObserver.unobserve(containerRef.current);
            }
        };
    }, []);

    const createWidgetElement = (name) => {
        let widget;
        try {
            const widgetJSON = window.module.get_widget_JSON(name);
            widget = JSON.parse(widgetJSON);
            if (!widget) return null;
        } catch (error) {
            console.error("Error parsing widget JSON:", error);
            return null;
        }

        const labelElement = (
            <Typography
                sx={{
                    fontWeight: 500,
                    color: 'white',
                    fontSize: '1rem',
                    mb: 0.5,
                    fontFamily: 'Roboto, Arial, sans-serif' // Ensure consistent font
                }}
            >
                {widget.label}
            </Typography>
        );

        switch (widget.type) {
            case 'menu_int':
                return (
                    <Box key={widget.name} sx={{ mb: 1, width: '100%' }}>
                        <JenMenu json={widget} onChange={onChange} width="100px"  />
                    </Box>
                );

            case 'menu_string':
                if (widget.tool === 'image') {
                    return (
                        <Box key={widget.name} sx={{ mb: 1, width: '100%' }}>
                            <MasonryImagePicker
                                updateFuncName={widget.name}
                                json={widget}
                                width="100%"
                                onChange={onChange}
                            />
                        </Box>
                    );
                }
                return (
                    <Box key={widget.name} sx={{ mb: 1, width: '100%' }}>
                        {labelElement}
                        <JenMenu json={widget} onChange={onChange} width="100%" />
                    </Box>
                );

            case 'slider_int':
            case 'slider_float':
            case 'range_slider_int':
            case 'range_slider_float':
                return (
                    <Box key={widget.name} sx={{ mb: 1, width: '100%' }}>
                        {labelElement}
                        <JenSlider json={widget} width="100%"/>
                    </Box>
                );

            case 'switch_fn':
            case 'switch_condition':
                return (
                    <Box key={widget.name} sx={{ display: 'flex', alignItems: 'center', mb: 1, width: '100%' }}>
                        <JenSwitch json={widget} onChange={onChange} />
                        <Box sx={{ ml: 0.5 }}>{labelElement}</Box>
                    </Box>
                );

            case 'direction_picker_8':
                return (
                    <Box key={widget.name} sx={{ display: 'flex', alignItems: 'center', mb: 1, width: '100%' }}>
                        <JenDirection8 json={widget} />
                        <Box sx={{ ml: 0.5 }}>{labelElement}</Box>
                    </Box>
                );

            case 'direction_picker_4':
                return (
                    <Box key={widget.name} sx={{ display: 'flex', alignItems: 'center', mb: 1, width: '100%' }}>
                        <JenDirection4 json={widget} />
                        <Box sx={{ ml: 0.5 }}>{labelElement}</Box>
                    </Box>
                );

            case 'direction_picker_4_diagonal':
                return (
                    <Box key={widget.name} sx={{ display: 'flex', alignItems: 'center', mb: 1, width: '100%' }}>
                        <JenDirection4Diagonal json={widget} />
                        <Box sx={{ ml: 0.5 }}>{labelElement}</Box>
                    </Box>
                );

            case 'box_blur_picker':
                return (
                    <Box key={widget.name} sx={{ display: 'flex', alignItems: 'center', mb: 1, width: '100%' }}>
                        <JenBlurPicker json={widget} />
                        <Box sx={{ ml: 0.5 }}>{labelElement}</Box>
                    </Box>
                );

            case 'funk_factor_picker':
                return (
                    <Box key={widget.name} sx={{width: '100%', mb: 1}}>
                        <Box sx={{mt: 0.5}}>
                            <JenFunkyPicker key={widget.name} json={widget}/>
                        </Box>
                    </Box>
                );

            case 'ucolor_picker':
                return (
                    <Box key={widget.name} sx={{ width: '100%', mb: 1 }}>
                        <Box sx={{ mt: 0.5 }}>
                            <JenColorPicker key={widget.name} json={widget} /> 
                        </Box>
                    </Box>
                );

            case 'custom_blur_picker':
                // Handle multi-picker component with add/remove functionality
                const pickerElements = [];

                for (let i = 0; i < widget.pickers.length; i++) {
                    const handleClose = () => {
                        window.module.remove_custom_blur_pickers(widget.name, i);
                        // Force re-render by updating the widget elements
                        setTimeout(() => {
                            if (json && json.widgets) {
                                const elements = json.widgets
                                    .map(createWidgetElement)
                                    .filter(el => el !== null);
                                setWidgetElements(elements);
                            }
                        }, 50);
                    };

                    const picker = widget.pickers[i];

                    pickerElements.push(
                        <Box
                            key={`picker-${i}-${resetTrigger}`}
                            sx={{
                                display: 'flex',
                                alignItems: 'center',
                                gap: 1,
                                mb: 0.5,
                                p: 0.5,
                                border: `1px solid rgba(255, 255, 255, 0.1)`,
                                borderRadius: 1,
                                bgcolor: 'rgba(0, 0, 0, 0.2)'
                            }}
                        >
                            <JenMultiDirection8
                                name={widget.name}
                                value={picker[0]}
                                code={i}
                                key={`dir1-${i}-${resetTrigger}`}
                            />
                            <JenMultiDirection8
                                name={widget.name}
                                value={picker[1]}
                                code={i + 128}
                                key={`dir2-${i}-${resetTrigger}`}
                            />
                            <IconButton size="small" onClick={handleClose} sx={{ p: 0.2, color: 'white' }}>
                                <X size={12} />
                            </IconButton>
                        </Box>
                    );
                }

                const handleAddPicker = () => {
                    window.module.add_custom_blur_pickers(widget.name);
                    // Force re-render by updating the widget elements
                    setTimeout(() => {
                        if (json && json.widgets) {
                            const elements = json.widgets
                                .map(createWidgetElement)
                                .filter(el => el !== null);
                            setWidgetElements(elements);
                        }
                    }, 50);
                };

                return (
                    <Box key={widget.name} sx={{ width: '100%', mb: 1 }}>
                        <Box sx={{ mb: 0.5 }}>
                            {labelElement}
                        </Box>

                        {pickerElements}

                        <Box sx={{ display: 'flex', justifyContent: 'center', mt: 0.5 }}>
                            <IconButton
                                size="small"
                                onClick={handleAddPicker}
                                sx={{
                                    border: `1px dashed rgba(255, 255, 255, 0.3)`,
                                    borderRadius: 1,
                                    p: 0.3,
                                    color: 'white'
                                }}
                            >
                                <Plus size={12} />
                            </IconButton>
                        </Box>
                    </Box>
                );

            case 'widget_switch_fn':
                // Handle widget with a switch
                let wsWidget, switcher;

                try {
                    const wsWidgetJSON = window.module.get_widget_JSON(widget.widget);
                    wsWidget = JSON.parse(wsWidgetJSON);

                    const switcherJSON = window.module.get_widget_JSON(widget.switcher);
                    switcher = JSON.parse(switcherJSON);
                } catch (error) {
                    console.error("Error parsing widget_switch_fn JSON:", error);
                    return null;
                }

                // Only handle slider types for now
                if (wsWidget.type.includes('slider_')) {
                    return (
                        <Box key={widget.name} sx={{ width: '100%', mb: 1 }}>
                            <Box sx={{ display: 'flex', alignItems: 'center', mb: 0.5 }}>
                                <JenSwitch key={switcher.name} json={switcher} onChange={onChange} />
                                <Box sx={{ ml: 0.5 }}>{labelElement}</Box>
                            </Box>
                            <JenSlider key={wsWidget.name} json={wsWidget} width="100%" />
                        </Box>
                    );
                }

                return (
                    <Box key={widget.name} sx={{ mb: 1, width: '100%' }}>
                        <Typography variant="caption" color="text.secondary">
                            Unsupported widget_switch_fn type: {wsWidget.type}
                        </Typography>
                    </Box>
                );

            default:
                return (
                    <Box key={widget.name} sx={{ mb: 1, width: '100%' }}>
                        <Typography variant="caption" color="text.secondary">
                            Unknown widget type: {widget.type}
                        </Typography>
                    </Box>
                );
        }
    };

    // Create widget elements on initial render and when dependencies change
    useEffect(() => {
        if (json && json.widgets) {
            const elements = json.widgets
                .map(createWidgetElement)
                .filter(el => el !== null);
            setWidgetElements(elements);
        }
    }, [json, panelSize, resetTrigger]);

    // If no widgets, don't render anything
    if (widgetElements.length === 0) {
        return null;
    }

    return (
        widgetElements.map((element, index) => (
                    <div key={`widget-item-${index}`} className="widget-item">
                        {element}
                    </div>
                ))
    );
}

export default WidgetGroup;