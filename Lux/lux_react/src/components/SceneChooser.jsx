import React, {useState} from "react";
import {
    Box,
    CircularProgress,
    Dialog,
    DialogContent,
    DialogTitle,
    Grid,
    MenuItem,
    Select,
    Typography
} from "@mui/material";
import IconButton from "@mui/material/IconButton";
import Paper from "@mui/material/Paper";
import {Close} from "@mui/icons-material";
import {useScene} from "./SceneContext.jsx";
import {BookOpen} from "lucide-react";
import ThumbnailCanvas from "./ThumbnailCanvas.jsx";

export function SceneChooser() {
    const {scenes, currentSceneIndex, changeScene, isLoading} = useScene();
    const [galleryOpen, setGalleryOpen] = useState(false);
    const [status, setStatus] = useState('loading')

    const currentScene = scenes[currentSceneIndex] || {name: "kaleidoswirl"};

    const getIconImagePath = (scene) => {
        console.log('scene now: ' + JSON.stringify(scene));
        if (scene) {
            return scene.name.toLowerCase().split(" ").join("_") + ".jpeg";
        } else {
            return '';
        }
    }


    return (
        <>
            <Paper sx={{p: 1, borderRadius: 1}}>
                <Box sx={{display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 0.5}}>
                    <Typography variant="body2" fontWeight="bold" sx={{display: 'flex', alignItems: 'center', gap: 1}}>
                        <BookOpen size={16}/>
                        Scene Selection
                    </Typography>
                    <Box>
                        {isLoading && <CircularProgress size={16} sx={{mr: 1}}/>}
                        <IconButton size="small" onClick={() => setGalleryOpen(true)}>
                            <Grid size={16}/>
                        </IconButton>
                    </Box>
                </Box>

                <Select
                    value={currentSceneIndex}
                    onChange={(e) => changeScene(e.target.value)}
                    fullWidth
                    size="small"
                    renderValue={() => (
                        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                            <Box
                                sx={{
                                    width: 24,
                                    height: 24,
                                    borderRadius: '4px',
                                    overflow: 'hidden',
                                    bgcolor: 'rgba(0,0,0,0.2)'
                                }}
                            >
                                <img
                                    src={`../../public/assets/images/${getIconImagePath(currentScene)}`}
                                    alt=""
                                    style={{ width: '100%', height: '100%', objectFit: 'cover' }}
                                    onError={(e) => { e.target.style.display = 'none' }}
                                />
                            </Box>
                            <Typography variant="body2" noWrap>
                                {currentScene.name}
                            </Typography>
                        </Box>
                    )}
                >
                    {scenes.map((scene, index) => (
                        <MenuItem key={index} value={index}>
                            <Box sx={{display: 'flex', alignItems: 'center', gap: 1}}>
                                <Box
                                    sx={{
                                        width: 32,
                                        height: 32,
                                        borderRadius: '4px',
                                        overflow: 'hidden',
                                        bgcolor: 'rgba(0,0,0,0.2)'
                                    }}
                                >
                                    <img
                                        src={`../../public/assets/images/${getIconImagePath(scene)}`}
                                        alt=""
                                        style={{width: '100%', height: '100%', objectFit: 'cover'}}
                                        onError={(e) => {
                                            e.target.style.display = 'none'
                                        }}
                                    />
                                </Box>
                                {scene.name}
                            </Box>
                        </MenuItem>
                    ))}
                </Select>
            </Paper>

            {/* Gallery Dialog */}
            <Dialog
                open={galleryOpen}
                onClose={() => setGalleryOpen(false)}
                maxWidth="md"
                fullWidth
            >
                <DialogTitle>
                    Scene Gallery
                    <IconButton
                        onClick={() => setGalleryOpen(false)}
                        sx={{position: 'absolute', right: 8, top: 8}}
                    >
                        <Close/>
                    </IconButton>
                </DialogTitle>
                <DialogContent>
                    <Grid container spacing={2}>
                        {scenes.map((scene, index) => (
                            <Grid item xs={6} sm={4} md={3} key={index}>
                                <Paper
                                    elevation={1}
                                    sx={{
                                        p: 1,
                                        cursor: 'pointer',
                                        border: index === currentSceneIndex ? '2px solid #4f8cff' : '1px solid rgba(0,0,0,0.1)',
                                        bgcolor: index === currentSceneIndex ? 'rgba(79,140,255,0.1)' : 'background.paper',
                                        '&:hover': {transform: 'scale(1.02)', boxShadow: 2},
                                        transition: 'all 0.2s'
                                    }}
                                    onClick={() => {
                                        changeScene(index);
                                        setGalleryOpen(false);
                                    }}
                                >
                                    <Box
                                        sx={{
                                            width: '100%',
                                            paddingTop: '100%',
                                            position: 'relative',
                                            mb: 1,
                                            borderRadius: 1,
                                            overflow: 'hidden',
                                            bgcolor: 'rgba(0,0,0,0.2)'
                                        }}
                                    >
                                        <img
                                            src={`../../public/assets/images/${getIconImagePath(scene)}`}
                                            alt=""
                                            style={{width: '100%', height: '100%', objectFit: 'cover'}}
                                            onError={(e) => {
                                                e.target.style.display = 'none'
                                            }}
                                        />
                                    </Box>
                                    <Typography variant="body2" align="center">
                                        {scene.name}
                                    </Typography>
                                </Paper>
                            </Grid>
                        ))}
                    </Grid>
                </DialogContent>
            </Dialog>
        </>
    );
}