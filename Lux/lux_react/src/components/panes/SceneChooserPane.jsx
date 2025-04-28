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
import { usePane } from "./PaneContext.jsx";

const calculateFontSize = (name) => {
    if (!name) return '0.875rem';

    const length = name.length;
    if (length > 20) return '0.675rem';
    if (length > 15) return '0.75rem';
    if (length > 10) return '0.825rem';
    return '0.875rem';
};

export function SceneChooserPane() {
    const { scenes, currentSceneIndex, changeScene, isLoading } = useScene();
    const { setActivePane } = usePane(); // Get the setActivePane function from context
    const [selectedIndex, setSelectedIndex] = useState(currentSceneIndex);
    const [containerWidth, setContainerWidth] = useState(0);
    const containerRef = useRef(null);
    const theme = useTheme();
    const isWideLayout = useMediaQuery(theme.breakpoints.up('md'));

    const getBasePath = () => {
        return import.meta.env.DEV ? '/assets/images/' : './assets/images/';
    };

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

    useEffect(() => {
        setSelectedIndex(currentSceneIndex);
    }, [currentSceneIndex]);

    const handleSceneSelect = (index) => {
        console.log(`Scene selected: ${index}`);
        setSelectedIndex(index);
        changeScene(index);

        setTimeout(() => {
            setActivePane("home");
            console.log("Navigating to home pane after scene selection");
        }, 100);
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
                            const imagePath = `${getBasePath()}${getIconImagePath(scene)}`;
                            const fontSize = calculateFontSize(scene.name);

                            return (
                                <Grid item xs={6} sm={4} md={3} lg={6/getColumnCount()} key={index}>
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
                                                    paddingTop: '100%',
                                                    position: 'relative',
                                                    bgcolor: 'rgba(0,0,0,0.1)',
                                                    overflow: 'hidden'
                                                }}
                                            >
                                                <img
                                                    src={imagePath}
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

                                                        const parent = e.target.parentElement;
                                                        if (parent) {
                                                            const fallback = document.createElement('div');
                                                            fallback.style.position = 'absolute';
                                                            fallback.style.top = '0';
                                                            fallback.style.left = '0';
                                                            fallback.style.width = '100%';
                                                            fallback.style.height = '100%';
                                                            fallback.style.display = 'flex';
                                                            fallback.style.alignItems = 'center';
                                                            fallback.style.justifyContent = 'center';
                                                            fallback.style.backgroundColor = theme.palette.primary.light;
                                                            fallback.style.color = theme.palette.primary.contrastText;
                                                            fallback.style.fontSize = '2rem';
                                                            fallback.style.fontWeight = 'bold';
                                                            fallback.textContent = scene.name.charAt(0).toUpperCase();
                                                            parent.appendChild(fallback);
                                                        }
                                                    }}
                                                />
                                            </Box>
                                            <Box sx={{
                                                p: 1,
                                                textAlign: 'center',
                                                flexGrow: 1,
                                                display: 'flex',
                                                alignItems: 'center',
                                                justifyContent: 'center',
                                                height: '2.5rem'  // Fixed height for text container
                                            }}>
                                                <Typography
                                                    variant="body2"
                                                    fontWeight={isSelected ? 'medium' : 'normal'}
                                                    sx={{
                                                        width: '100%',
                                                        fontFamily: 'Roboto, Arial, sans-serif',
                                                        fontSize: fontSize,
                                                        lineHeight: 1.2,
                                                        display: '-webkit-box',
                                                        WebkitLineClamp: 2,
                                                        WebkitBoxOrient: 'vertical',
                                                        overflow: 'hidden',
                                                        wordBreak: 'break-word',
                                                        hyphens: 'auto'
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