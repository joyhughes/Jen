import React, { useState, useEffect } from "react";
import {
    Select,
    MenuItem,
    FormControl,
    InputLabel,
    Typography,
    Box,
    Tooltip,
    Chip,
    Paper,
    CircularProgress,
    useTheme,
    alpha
} from '@mui/material';
import {
    Info,
    FileBox,
    Sparkles,
    Tag
} from 'lucide-react';

function JenSceneChooser({ width, onChange }) {
    const theme = useTheme();
    const [sceneListJSON, setSceneListJSON] = useState(null);
    const [selectedMenuChoice, setSelectedMenuChoice] = useState(0);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState(null);

    // Handle scene selection change
    const handleMenuChange = (event) => {
        const sceneIndex = event.target.value;
        setSelectedMenuChoice(sceneIndex);

        if (onChange) {
            onChange();
        }

        if (window.module && sceneListJSON?.scenes) {
            try {
                window.module.load_scene(sceneListJSON.scenes[sceneIndex].filename);
            } catch (error) {
                console.error(`Error loading scene: ${sceneListJSON.scenes[sceneIndex].filename}`, error);
            }
        }
    };

    // Load scene list from WebAssembly module
    const setupSceneList = () => {
        if (!window.module) return;

        setLoading(true);
        setError(null);

        try {
            const sceneListJSONString = window.module.get_scene_list_JSON();
            const parsedJSON = JSON.parse(sceneListJSONString);
            setSceneListJSON(parsedJSON);
            setLoading(false);
        } catch (error) {
            console.error("Error parsing scene list JSON:", error);
            setError("Failed to load scene list");
            setLoading(false);
        }
    };

    // Initialize and poll for module availability
    useEffect(() => {
        if (window.module) {
            setupSceneList();
        } else {
            const intervalId = setInterval(() => {
                if (window.module) {
                    setupSceneList();
                    clearInterval(intervalId);
                }
            }, 100);

            return () => clearInterval(intervalId);
        }
    }, []);

    // Get current scene details
    const currentScene = sceneListJSON?.scenes?.[selectedMenuChoice];

    // Custom renderer for select items to show additional scene details
    const renderMenuItem = (scene, index) => {
        return (
            <MenuItem
                key={index}
                value={index}
                sx={{
                    display: 'flex',
                    flexDirection: 'column',
                    alignItems: 'flex-start',
                    py: 1,
                    borderBottom: index < sceneListJSON.scenes.length - 1 ? `1px solid ${theme.palette.divider}` : 'none'
                }}
            >
                <Box sx={{ display: 'flex', alignItems: 'center', width: '100%' }}>
                    <FileBox size={16} style={{ marginRight: 8 }} />
                    <Typography variant="body2" fontWeight={500}>
                        {scene.name}
                    </Typography>

                    {/* Display tags if they exist */}
                    {scene.tags && scene.tags.length > 0 && (
                        <Box sx={{ ml: 'auto', display: 'flex', gap: 0.5 }}>
                            {scene.tags.slice(0, 2).map((tag, tagIndex) => (
                                <Chip
                                    key={tagIndex}
                                    label={tag}
                                    size="small"
                                    variant="outlined"
                                    sx={{
                                        height: 20,
                                        '& .MuiChip-label': {
                                            px: 1,
                                            fontSize: '0.625rem'
                                        }
                                    }}
                                />
                            ))}
                            {scene.tags.length > 2 && (
                                <Chip
                                    label={`+${scene.tags.length - 2}`}
                                    size="small"
                                    variant="outlined"
                                    sx={{
                                        height: 20,
                                        '& .MuiChip-label': {
                                            px: 1,
                                            fontSize: '0.625rem'
                                        }
                                    }}
                                />
                            )}
                        </Box>
                    )}
                </Box>

                {/* Display description if it exists */}
                {scene.description && (
                    <Typography
                        variant="caption"
                        color="text.secondary"
                        sx={{
                            mt: 0.5,
                            alignSelf: 'flex-start',
                            maxWidth: '100%',
                            overflow: 'hidden',
                            textOverflow: 'ellipsis',
                            display: '-webkit-box',
                            WebkitLineClamp: 2,
                            WebkitBoxOrient: 'vertical'
                        }}
                    >
                        {scene.description}
                    </Typography>
                )}
            </MenuItem>
        );
    };

    return (
        <Box>
            <Typography
                variant="subtitle2"
                fontWeight={600}
                sx={{ mb: 1.5 }}
            >
                Scene Selection
            </Typography>

            {loading ? (
                <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, py: 1 }}>
                    <CircularProgress size={20} />
                    <Typography variant="body2" color="text.secondary">
                        Loading scenes...
                    </Typography>
                </Box>
            ) : error ? (
                <Typography variant="body2" color="error" sx={{ py: 1 }}>
                    {error}
                </Typography>
            ) : (
                <>
                    <FormControl
                        fullWidth
                        size="small"
                        sx={{ mb: 1.5 }}
                    >
                        <Select
                            value={selectedMenuChoice}
                            onChange={handleMenuChange}
                            displayEmpty
                            renderValue={(selected) => (
                                <Box sx={{ display: 'flex', alignItems: 'center' }}>
                                    <FileBox size={16} style={{ marginRight: 8 }} />
                                    <Typography variant="body2">
                                        {sceneListJSON?.scenes?.[selected]?.name || 'Select a scene'}
                                    </Typography>
                                </Box>
                            )}
                            MenuProps={{
                                PaperProps: {
                                    style: {
                                        maxHeight: 300,
                                        width: width,
                                    },
                                },
                            }}
                        >
                            {sceneListJSON && sceneListJSON.scenes ? (
                                sceneListJSON.scenes.map(renderMenuItem)
                            ) : (
                                <MenuItem value="">
                                    <em>No scenes available</em>
                                </MenuItem>
                            )}
                        </Select>
                    </FormControl>

                    {/* Display current scene details */}
                    {currentScene && (
                        <Paper
                            variant="outlined"
                            sx={{
                                p: 1.5,
                                borderRadius: 1,
                                backgroundColor: alpha(theme.palette.primary.main, 0.05),
                                borderColor: alpha(theme.palette.primary.main, 0.2)
                            }}
                        >
                            {currentScene.description && (
                                <Typography
                                    variant="body2"
                                    color="text.secondary"
                                    sx={{ mb: 1 }}
                                >
                                    {currentScene.description}
                                </Typography>
                            )}

                            {/* Tags display */}
                            {currentScene.tags && currentScene.tags.length > 0 && (
                                <Box sx={{ display: 'flex', alignItems: 'center', flexWrap: 'wrap', gap: 0.5, mt: 1 }}>
                                    <Tag size={14} color={theme.palette.text.secondary} />
                                    {currentScene.tags.map((tag, index) => (
                                        <Chip
                                            key={index}
                                            label={tag}
                                            size="small"
                                            sx={{
                                                height: 24,
                                                backgroundColor: alpha(theme.palette.primary.main, 0.1),
                                                '& .MuiChip-label': {
                                                    px: 1
                                                }
                                            }}
                                        />
                                    ))}
                                </Box>
                            )}

                            {/* Author and creation date if available */}
                            {(currentScene.author || currentScene.date) && (
                                <Box sx={{ display: 'flex', justifyContent: 'space-between', mt: 1 }}>
                                    {currentScene.author && (
                                        <Typography variant="caption" color="text.secondary">
                                            Created by: {currentScene.author}
                                        </Typography>
                                    )}
                                    {currentScene.date && (
                                        <Typography variant="caption" color="text.secondary">
                                            {currentScene.date}
                                        </Typography>
                                    )}
                                </Box>
                            )}
                        </Paper>
                    )}
                </>
            )}
        </Box>
    );
}

export default JenSceneChooser;