import React, { useState, useEffect } from "react";
import Paper from '@mui/material/Paper';
import Box from '@mui/material/Box';

import MediaController from "./MediaController";
import TabNavigation from "./TabNavigation";
import HomePane from "./panes/HomePane";
import SourceImagePane from "./panes/SourceImagePane";
import TargetImagePane from "./panes/TargetImagePane";
import BrushPane from "./panes/BrushPane";
import { SceneChooserPane } from "./panes/SceneChooserPane";

function ControlPanel({ dimensions, panelSize, activePane, onPaneChange }) {
    const [panelJSON, setPanelJSON] = useState([]);
    const [activeGroups, setActiveGroups] = useState([]);

    const handleWidgetGroupChange = () => {
        // Fetch active widget groups from WebAssembly
        if (window.module && panelJSON) {
            const active = panelJSON.filter(group =>
                window.module.is_widget_group_active(group.name)
            );
            setActiveGroups(active);
        }
    };

    const setupPanel = () => {
        if (window.module) {
            const panelJSONString = window.module.get_panel_JSON();

            try {
                const parsedJSON = JSON.parse(panelJSONString);
                setPanelJSON(parsedJSON);
            } catch (error) {
                console.error("Error parsing panel JSON:", error);
            }
        }
    };

    useEffect(() => {
        if (window.module) {
            setupPanel();
            window.module.set_scene_callback(setupPanel);
        } else {
            // Poll for the Module to be ready
            const intervalId = setInterval(() => {
                if (window.module) {
                    setupPanel();
                    window.module.set_scene_callback(setupPanel);
                    clearInterval(intervalId);
                }
            }, 100);

            return () => clearInterval(intervalId);
        }
    }, []);

    useEffect(() => {
        handleWidgetGroupChange();
    }, [panelJSON]);

    const renderActivePane = () => {
        const commonProps = {
            dimensions,
            panelSize,
            panelJSON,
            activeGroups,
            onWidgetGroupChange: handleWidgetGroupChange
        };

        switch (activePane) {
            case "home":
                return <HomePane {...commonProps} />;
            case "scenes":
                return <SceneChooserPane />;
            case "source":
                return <SourceImagePane {...commonProps} />;
            case "target":
                return <TargetImagePane {...commonProps} />;
            case "brush":
                return <BrushPane {...commonProps} />;
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

            <MediaController panelSize={panelSize} />

            <Box
                sx={{
                    flex: 1,
                    overflow: 'auto',
                }}
            >
                {renderActivePane()}
            </Box>
        </Paper>
    );
}

export default ControlPanel;