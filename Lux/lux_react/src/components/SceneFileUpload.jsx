import React, { useState, useCallback, useEffect } from 'react';
import { 
    Box, 
    Button, 
    Typography, 
    Alert, 
    CircularProgress,
    Paper,
    useTheme
} from '@mui/material';
import { Upload, FileJson, CheckCircle, AlertCircle } from 'lucide-react';

export function SceneFileUpload({ onSceneLoaded }) {
    const [isLoading, setIsLoading] = useState(false);
    const [error, setError] = useState(null);
    const [success, setSuccess] = useState(false);
    const [fileName, setFileName] = useState('');
    const [isModuleReady, setIsModuleReady] = useState(false);
    const theme = useTheme();

    // Check if WASM module is ready
    useEffect(() => {
        const checkModuleReady = () => {
            // Debug: log what's available
            if (window.module) {
                const functions = Object.keys(window.module);
                console.log('SceneFileUpload: window.module available, functions:', functions);
                console.log('SceneFileUpload: Has load_scene_from_json?', typeof window.module.load_scene_from_json);
                console.log('SceneFileUpload: Has load_scene?', typeof window.module.load_scene);
                console.log('SceneFileUpload: Has FS?', !!window.module.FS);
            }
            if (window.Module) {
                const functions = Object.keys(window.Module);
                console.log('SceneFileUpload: window.Module available, functions:', functions);
                console.log('SceneFileUpload: Has load_scene_from_json?', typeof window.Module.load_scene_from_json);
            }
            
            // Check for different possible module configurations
            const ready = (
                (window.module && typeof window.module.load_scene_from_json === 'function') ||
                (window.Module && typeof window.Module.load_scene_from_json === 'function') ||
                (window.module && typeof window.module.load_scene === 'function' && window.module.FS)
            );
            
            if (ready && !isModuleReady) {
                console.log('SceneFileUpload: Module is now ready!');
            }
            
            setIsModuleReady(ready);
        };

        // Check immediately
        checkModuleReady();

        // Check periodically until ready
        const interval = setInterval(checkModuleReady, 100);
        
        return () => clearInterval(interval);
    }, [isModuleReady]);

    const handleSaveScene = useCallback(async () => {
        if (!isModuleReady) {
            setError('WASM module not ready. Please wait for the application to fully load.');
            return;
        }

        try {
            setError(null);
            setSuccess(false);
            setIsLoading(true);

            // Get current scene state from WASM module
            let sceneState = null;
            
            if (window.module && typeof window.module.save_scene_state === 'function') {
                const sceneJson = window.module.save_scene_state();
                sceneState = JSON.parse(sceneJson);
            } else if (window.Module && typeof window.Module.save_scene_state === 'function') {
                const sceneJson = window.Module.save_scene_state();
                sceneState = JSON.parse(sceneJson);
            } else {
                throw new Error('Scene saving function not available');
            }

            if (!sceneState || !sceneState.name) {
                throw new Error('Failed to get scene state');
            }

            // Note: Backend now detects saved scenes by harness structure, not timestamp
            // The harness values with both functions and value fields indicate runtime state

            // Create downloadable file
            const blob = new Blob([JSON.stringify(sceneState, null, 2)], {
                type: 'application/json'
            });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `${sceneState.name}_${Date.now()}.json`;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);

            setSuccess(true);
            setFileName(`${sceneState.name}_saved.json`);
            
            // Auto-reset success state after 3 seconds
            setTimeout(() => setSuccess(false), 3000);
            
        } catch (error) {
            setError(`Failed to save scene: ${error.message}`);
        } finally {
            setIsLoading(false);
        }
    }, [isModuleReady]);

    const handleLoadDefaultScene = useCallback(async () => {
        if (!isModuleReady) {
            setError('WASM module not ready. Please wait for the application to fully load.');
            return;
        }

        try {
            setError(null);
            setSuccess(false);
            setIsLoading(true);

            // Load the default kaleido scene
            if (window.module && typeof window.module.load_scene === 'function') {
                const success = window.module.load_scene('lux_files/kaleido.json');
                if (success) {
                    setSuccess(true);
                    setFileName('kaleido.json (default)');
                    
                    // Notify parent component
                    if (onSceneLoaded) {
                        onSceneLoaded({ name: 'Kaleidoswirl', type: 'default' });
                    }
                    
                    // Auto-reset success state after 3 seconds
                    setTimeout(() => setSuccess(false), 3000);
                } else {
                    throw new Error('Failed to load default scene');
                }
            } else if (window.Module && typeof window.Module.load_scene === 'function') {
                const success = window.Module.load_scene('lux_files/kaleido.json');
                if (success) {
                    setSuccess(true);
                    setFileName('kaleido.json (default)');
                    
                    // Notify parent component
                    if (onSceneLoaded) {
                        onSceneLoaded({ name: 'Kaleidoswirl', type: 'default' });
                    }
                    
                    // Auto-reset success state after 3 seconds
                    setTimeout(() => setSuccess(false), 3000);
                } else {
                    throw new Error('Failed to load default scene');
                }
            } else {
                throw new Error('Scene loading function not available');
            }
            
        } catch (error) {
            setError(`Failed to load default scene: ${error.message}`);
        } finally {
            setIsLoading(false);
        }
    }, [isModuleReady, onSceneLoaded]);

    const handleFileUpload = useCallback(async (event) => {
        const file = event.target.files[0];
        if (!file) return;

        // Check if module is ready
        if (!isModuleReady) {
            setError('WASM module not ready. Please wait for the application to fully load.');
            return;
        }

        // Reset states
        setError(null);
        setSuccess(false);
        setIsLoading(true);
        setFileName(file.name);

        try {
            // Validate file type
            if (!file.name.endsWith('.json')) {
                throw new Error('Please select a valid JSON file');
            }

            // Read file content
            const reader = new FileReader();
            reader.onload = async (e) => {
                try {
                    const jsonContent = e.target.result;
                    
                    // Validate JSON
                    const sceneData = JSON.parse(jsonContent);
                    
                    // Check if it's a valid scene
                    if (!sceneData.name || !sceneData.effects || !sceneData.functions) {
                        throw new Error('Invalid scene file format');
                    }

                    // Try to load scene via WASM module
                    let success = false;
                    
                    if (window.module && typeof window.module.load_scene_from_json === 'function') {
                        // Direct JSON loading
                        success = window.module.load_scene_from_json(jsonContent);
                    } else if (window.Module && typeof window.Module.load_scene_from_json === 'function') {
                        // Alternative module reference
                        success = window.Module.load_scene_from_json(jsonContent);
                    } else if (window.module && typeof window.module.load_scene === 'function') {
                        // Fallback: try to save as temporary file and load
                        try {
                            // Create a temporary filename
                            const tempFilename = `temp_scene_${Date.now()}.json`;
                            
                            // Try to write the file to the virtual filesystem
                            if (window.module.FS) {
                                window.module.FS.writeFile(tempFilename, jsonContent);
                                success = window.module.load_scene(tempFilename);
                                
                                // Clean up temp file
                                try {
                                    window.module.FS.unlink(tempFilename);
                                } catch (e) {
                                    console.warn('Could not clean up temp file:', e);
                                }
                            } else {
                                throw new Error('File system not available');
                            }
                        } catch (fileError) {
                            throw new Error(`Could not load scene: ${fileError.message}`);
                        }
                    } else {
                        throw new Error('Scene loading function not available. Please wait for the application to fully load.');
                    }
                    
                    if (success) {
                        setSuccess(true);
                        setError(null);
                        
                        // Force UI refresh by dispatching a custom event
                        setTimeout(() => {
                            window.dispatchEvent(new CustomEvent('sceneLoaded', { 
                                detail: { sceneName: sceneData.name, isSavedScene: true } 
                            }));
                        }, 500);
                        
                        // Notify parent component
                        if (onSceneLoaded) {
                            onSceneLoaded(sceneData);
                        }
                        
                        // Auto-reset success state after 3 seconds
                        setTimeout(() => setSuccess(false), 3000);
                    } else {
                        throw new Error('Failed to load scene');
                    }
                } catch (parseError) {
                    setError(`Invalid JSON: ${parseError.message}`);
                }
            };

            reader.onerror = () => {
                setError('Failed to read file');
            };

            reader.readAsText(file);

        } catch (error) {
            setError(error.message);
        } finally {
            setIsLoading(false);
            // Reset file input
            event.target.value = '';
        }
    }, [onSceneLoaded]);

    return (
        <Paper elevation={2} sx={{ p: 2, mb: 2 }}>
            <Typography variant="h6" gutterBottom>
                Load Scene File
            </Typography>
            
            <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
                Upload a saved scene file to restore its exact state, save the current scene, or load the default kaleido scene
            </Typography>
            
            {!isModuleReady && (
                <Alert severity="info" sx={{ mb: 2 }}>
                    Waiting for application to load...
                </Alert>
            )}

            <Box sx={{ display: 'flex', alignItems: 'center', gap: 2, mb: 2 }}>
                <Button
                    variant="outlined"
                    component="label"
                    startIcon={<Upload />}
                    disabled={isLoading || !isModuleReady}
                    sx={{ minWidth: 120 }}
                >
                    {isModuleReady ? 'Choose File' : 'Loading...'}
                    <input
                        type="file"
                        accept=".json"
                        onChange={handleFileUpload}
                        style={{ display: 'none' }}
                        disabled={!isModuleReady}
                    />
                </Button>

                <Button
                    variant="contained"
                    startIcon={<FileJson />}
                    disabled={!isModuleReady}
                    onClick={handleSaveScene}
                    sx={{ minWidth: 120 }}
                >
                    Save Scene
                </Button>

                <Button
                    variant="outlined"
                    startIcon={<FileJson />}
                    disabled={!isModuleReady}
                    onClick={handleLoadDefaultScene}
                    sx={{ minWidth: 140 }}
                >
                    Load Default
                </Button>

                {isLoading && (
                    <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                        <CircularProgress size={20} />
                        <Typography variant="body2">Loading...</Typography>
                    </Box>
                )}

                {success && (
                    <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, color: 'success.main' }}>
                        <CheckCircle size={20} />
                        <Typography variant="body2">Scene loaded!</Typography>
                    </Box>
                )}
            </Box>

            {fileName && (
                <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 2 }}>
                    <FileJson size={16} />
                    <Typography variant="body2" color="text.secondary">
                        {fileName}
                    </Typography>
                </Box>
            )}

            {error && (
                <Alert severity="error" icon={<AlertCircle />} sx={{ mt: 1 }}>
                    {error}
                </Alert>
            )}

            {success && (
                <Alert severity="success" icon={<CheckCircle />} sx={{ mt: 1 }}>
                    {fileName.includes('saved') 
                        ? `Scene "${fileName.replace(/_[0-9]+\.json$/, '')}" saved successfully!`
                        : `Scene "${fileName}" loaded successfully with all settings preserved!`
                    }
                </Alert>
            )}
        </Paper>
    );
}
