import React from "react";
import { Box, Typography, IconButton } from '@mui/material';
import JenMenu from './JenMenu';
import JenSlider from './JenSlider';
import JenSwitch from './JenSwitch';
import JenDirection8 from './JenDirection8';
import JenDirection4 from './JenDirection4';
import JenDirection4Diagonal from './JenDirection4Diagonal';
import JenBlurPicker from './JenBlurPicker';
import JenMultiDirection8 from './JenMultiDirection8';
import JenFunkyPicker from './JenFunkyPicker';
import { ImagePicker } from "./ImagePicker";
import { THUMB_SIZE } from "./ThumbnailItem";
import { Plus, X } from 'lucide-react';

function WidgetGroup({ json, panelSize, onChange }) {
    const [renderCount, setRenderCount] = React.useState(0);

    const renderWidget = (name) => {
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
                variant="body2"
                sx={{
                    fontWeight: 500,
                    color: 'white',
                    fontSize: '0.8rem',
                    mb: 0.5
                }}
            >
                {widget.label}
            </Typography>
        );

        switch (widget.type) {
            case 'menu_int':
                return (
                    <Box key={widget.name} sx={{ mb: 1 }}>
                        <JenMenu json={widget} onChange={onChange} width={panelSize - 8}/>
                    </Box>
                );

            case 'menu_string':
                return (
                    <Box key={widget.name} sx={{ mb: 1 }}>
                        {labelElement}
                        <ImagePicker
                            json={widget}
                            panelSize={THUMB_SIZE * 3}
                            onChange={onChange}
                        />
                    </Box>
                );

            case 'slider_int':
            case 'slider_float':
            case 'range_slider_int':
            case 'range_slider_float':
                return (
                    <Box key={widget.name} sx={{ mb: 1 }}>
                        {labelElement}
                        <JenSlider json={widget} width={panelSize - 8} />
                    </Box>
                );

            case 'switch_fn':
            case 'switch_condition':
                return (
                    <Box key={widget.name} sx={{ display: 'flex', alignItems: 'center', mb: 1 }}>
                        <JenSwitch json={widget} onChange={onChange} />
                        <Box sx={{ ml: 0.5 }}>{labelElement}</Box>
                    </Box>
                );

            case 'direction_picker_8':
                return (
                    <Box key={widget.name} sx={{ display: 'flex', alignItems: 'center', mb: 1 }}>
                        <JenDirection8 json={widget} />
                        <Box sx={{ ml: 0.5 }}>{labelElement}</Box>
                    </Box>
                );

            case 'direction_picker_4':
                return (
                    <Box key={widget.name} sx={{ display: 'flex', alignItems: 'center', mb: 1 }}>
                        <JenDirection4 json={widget} />
                        <Box sx={{ ml: 0.5 }}>{labelElement}</Box>
                    </Box>
                );

            case 'direction_picker_4_diagonal':
                return (
                    <Box key={widget.name} sx={{ display: 'flex', alignItems: 'center', mb: 1 }}>
                        <JenDirection4Diagonal json={widget} />
                        <Box sx={{ ml: 0.5 }}>{labelElement}</Box>
                    </Box>
                );

            case 'box_blur_picker':
                return (
                    <Box key={widget.name} sx={{ display: 'flex', alignItems: 'center', mb: 1 }}>
                        <JenBlurPicker json={widget} />
                        <Box sx={{ ml: 0.5 }}>{labelElement}</Box>
                    </Box>
                );

            case 'funk_factor_picker':
                return (
                    <Box key={widget.name} sx={{ width: '100%', mb: 1 }}>
                        <Box sx={{ mt: 0.5 }}>
                            <JenFunkyPicker key={widget.name} json={widget} />
                        </Box>
                    </Box>
                );

            case 'custom_blur_picker':
                // Handle multi-picker component with add/remove functionality
                const pickerElements = [];

                for (let i = 0; i < widget.pickers.length; i++) {
                    const handleClose = () => {
                        window.module.remove_custom_blur_pickers(widget.name, i);
                        setRenderCount(renderCount + 1);
                    };

                    const picker = widget.pickers[i];

                    pickerElements.push(
                        <Box
                            key={`picker-${i}-${renderCount}`}
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
                                key={`dir1-${i}-${renderCount}`}
                            />
                            <JenMultiDirection8
                                name={widget.name}
                                value={picker[1]}
                                code={i + 128}
                                key={`dir2-${i}-${renderCount}`}
                            />
                            <IconButton size="small" onClick={handleClose} sx={{ p: 0.2, color: 'white' }}>
                                <X size={12} />
                            </IconButton>
                        </Box>
                    );
                }

                const handleAddPicker = () => {
                    window.module.add_custom_blur_pickers(widget.name);
                    setRenderCount(renderCount + 1);
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
                            <JenSlider key={wsWidget.name} json={wsWidget} width={panelSize - 8} />
                        </Box>
                    );
                }

                return (
                    <Box key={widget.name} sx={{ mb: 1 }}>
                        <Typography variant="caption" color="text.secondary">
                            Unsupported widget_switch_fn type: {wsWidget.type}
                        </Typography>
                    </Box>
                );

            default:
                return (
                    <Box key={widget.name} sx={{ mb: 1 }}>
                        <Typography variant="caption" color="text.secondary">
                            Unknown widget type: {widget.type}
                        </Typography>
                    </Box>
                );
        }
    };

    return (
        <Box sx={{ px: 0.5, pt: 0.5, pb: 1}}>
            {json.widgets.map(renderWidget)}
        </Box>
    );
}

export default WidgetGroup;