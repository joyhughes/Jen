import React, { useState, useEffect } from "react";
import {
    Select,
    MenuItem,
    InputLabel,
    FormControl,
    Box,
    Typography,
    Paper,
    useTheme
} from '@mui/material';
import { BookOpen } from 'lucide-react';
import {THUMB_SIZE} from "./ThumbnailItem.jsx";

function SceneChooser({ width, onChange }) {
    const theme = useTheme();
    const [sceneListJSON, setSceneListJSON] = useState({ scenes: [] });
    const [selectedMenuChoice, setSelectedMenuChoice] = useState(0);

    // Handle scene selection
    const handleMenuChange = (event) => {
        onChange();
        window.module.load_scene(sceneListJSON.scenes[event.target.value].filename);
        setSelectedMenuChoice(event.target.value);
    };

    // Load scene list
    const setupSceneList = () => {
        const sceneListJSONString = window.module.get_scene_list_JSON();
        try {
            const parsedJSON = JSON.parse(sceneListJSONString);
            setSceneListJSON(parsedJSON);
        } catch (error) {
            console.error("Error parsing scene list JSON:", error);
        }
    };

    // Initialize on mount
    useEffect(() => {
        if (window.module) {
            setupSceneList();
        } else {
            // Poll for the Module to be ready
            const intervalId = setInterval(() => {
                if (window.module) {
                    setupSceneList();
                    clearInterval(intervalId);
                }
            }, 100);

            return () => clearInterval(intervalId);
        }
    }, []);

    return (
        <Paper
            elevation={0}
            sx={{
                p: 0.75,
                borderRadius: 1,
                border: `1px solid ${theme.palette.divider}`,
                bgcolor: theme.palette.background.paper,
                width: width,
                mb: 0.5
            }}
        >
            <Box sx={{ mb: 0.5 }}>
                <Typography
                    variant="caption"
                    component="div"
                    sx={{
                        display: 'flex',
                        alignItems: 'center',
                        gap: 0.5,
                        fontWeight: 600
                    }}
                >
                    <BookOpen size={12} />
                    Scene Selection
                </Typography>
            </Box>

            <FormControl
                fullWidth
                size="small"
                variant="outlined"
                sx={{ '& .MuiOutlinedInput-root': { fontSize: '0.8rem' } }}
            >
                <InputLabel id="scene-select-label" sx={{ fontSize: '0.8rem' }}>Scene</InputLabel>
                <Select
                    labelId="scene-select-label"
                    id="scene-select"
                    value={selectedMenuChoice}
                    label="Scene"
                    onChange={handleMenuChange}
                    MenuProps={{
                        PaperProps: {
                            style: {
                                maxHeight: 300,
                                width: Math.min(width, THUMB_SIZE * 3) - 20,
                            },
                            elevation: 3,
                        },
                    }}
                >
                    {sceneListJSON && sceneListJSON.scenes ? (
                        sceneListJSON.scenes.map((scene, index) => (
                            <MenuItem key={index} value={index}>
                                {scene.name}
                            </MenuItem>
                        ))
                    ) : (
                        <MenuItem value="">
                            <em>Loading...</em>
                        </MenuItem>
                    )}
                </Select>
            </FormControl>
        </Paper>
    );
}

export default SceneChooser;