import React, { useState, useEffect, useRef } from "react";
import {
    Box,
    CircularProgress,
    Typography,
    Paper,
    useTheme,
    useMediaQuery,
    Alert,
    Fade,
    Grid,
    Tooltip
} from "@mui/material";
import { BookOpen, Info } from "lucide-react";
import { useScene } from "../SceneContext.jsx";

export function SceneChooserPane() {
    const { scenes, currentSceneIndex, changeScene, isLoading } = useScene();
    const [selectedIndex, setSelectedIndex] = useState(currentSceneIndex);
    const [containerWidth, setContainerWidth] = useState(0);
    const containerRef = useRef(null);
    const theme = useTheme();
    const isWideLayout = useMediaQuery(theme.breakpoints.up('md'));

    // Monitor container width for responsive layout
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

    // Keep local state in sync with context
    useEffect(() => {
        setSelectedIndex(currentSceneIndex);
    }, [currentSceneIndex]);

    const handleSceneSelect = (index) => {
        setSelectedIndex(index);
        changeScene(index);
    };

    const getIconImagePath = (scene) => {
        if (scene) {
            return scene.name.toLowerCase().split(" ").join("_") + ".jpeg";
        } else {
            return '';
        }
    };

    // Calculate columns based on container width
    const getColumnCount = () => {
        if (containerWidth < 350) return 2;
        if (containerWidth < 550) return 3;
        if (containerWidth < 850) return 4;
        return 5;
    };

    return (
        <Box
            ref={containerRef}
            sx={{
                padding: 2,
                height: '100%',
                overflowY: 'auto',
                width: '100%'
            }}
        >
            <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 2 }}>
                <Typography variant="h6" fontWeight="medium" sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                    <BookOpen size={20} />
                    Scene Selection
                </Typography>
                {isLoading && (
                    <Box sx={{ display: 'flex', alignItems: 'center' }}>
                        <CircularProgress size={20} sx={{ mr: 1 }} />
                        <Typography variant="body2" color="text.secondary">Loading scenes...</Typography>
                    </Box>
                )}
            </Box>

            {scenes.length === 0 ? (
                <Alert
                    severity="info"
                    icon={<Info />}
                    sx={{ mb: 2 }}
                >
                    No scenes available. Please check your configuration.
                </Alert>
            ) : (
                <Fade in={true} timeout={300}>
                    <Grid container spacing={1.5}>
                        {scenes.map((scene, index) => {
                            const isSelected = index === selectedIndex;

                            return (
                                <Grid item xs={3} sm={4} md={3} lg={6/getColumnCount()} key={index}>
                                    <Tooltip
                                        title={scene.description || scene.name}
                                        placement="top"
                                        arrow
                                    >
                                        <Paper
                                            elevation={isSelected ? 3 : 1}
                                            sx={{
                                                cursor: 'pointer',
                                                border: isSelected ? `2px solid ${theme.palette.primary.main}` : '1px solid rgba(0,0,0,0.1)',
                                                bgcolor: isSelected ? `${theme.palette.primary.main}10` : 'background.paper',
                                                borderRadius: 1,
                                                overflow: 'hidden',
                                                transition: 'all 0.2s ease',
                                                '&:hover': {
                                                    transform: 'translateY(-2px)',
                                                    boxShadow: 2
                                                },
                                                height: '100%',
                                                display: 'flex',
                                                flexDirection: 'column'
                                            }}
                                            onClick={() => handleSceneSelect(index)}
                                        >
                                            <Box
                                                sx={{
                                                    width: '100%',
                                                    paddingTop: '100%', // Square aspect ratio
                                                    position: 'relative',
                                                    bgcolor: 'rgba(0,0,0,0.1)',
                                                    overflow: 'hidden'
                                                }}
                                            >
                                                <img
                                                    src={`../../public/assets/images/${getIconImagePath(scene)}`}
                                                    alt={scene.name}
                                                    style={{
                                                        position: 'absolute',
                                                        top: 0,
                                                        left: 0,
                                                        width: '100%',
                                                        height: '100%',
                                                        objectFit: 'cover'
                                                    }}
                                                    onError={(e) => {
                                                        e.target.style.display = 'none';
                                                    }}
                                                />
                                            </Box>
                                            <Box sx={{
                                                p: 1,
                                                textAlign: 'center',
                                                flexGrow: 1,
                                                display: 'flex',
                                                alignItems: 'center',
                                                justifyContent: 'center'
                                            }}>
                                                <Typography
                                                    variant="body2"
                                                    fontWeight={isSelected ? 'medium' : 'normal'}
                                                    noWrap
                                                    sx={{
                                                        width: '100%',
                                                        overflow: 'hidden',
                                                        textOverflow: 'ellipsis'
                                                    }}
                                                >
                                                    {scene.name}
                                                </Typography>
                                            </Box>
                                        </Paper>
                                    </Tooltip>
                                </Grid>
                            );
                        })}
                    </Grid>
                </Fade>
            )}
        </Box>
    );
}