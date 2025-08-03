import React, { useState, useEffect, useCallback } from 'react';
import {
  Box,
  Button,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  TextField,
  List,
  ListItem,
  ListItemText,
  ListItemSecondaryAction,
  IconButton,
  Typography,
  Chip,
  Alert,
  Divider,
  Paper,
  Grid,
  Tooltip,
  Snackbar
} from '@mui/material';
import {
  Save,
  FolderOpen,
  Download,
  Upload,
  Delete,
  Refresh,
  Info,
  ContentCopy,
  CompareArrows
} from '@mui/icons-material';
import EnhancedSceneStateManager from '../utils/sceneStateManager';

const EnhancedSceneStateManagerComponent = ({ open, onClose, showNotification }) => {
  const [savedStates, setSavedStates] = useState([]);
  const [currentMetadata, setCurrentMetadata] = useState(null);
  const [saveDialogOpen, setSaveDialogOpen] = useState(false);
  const [saveName, setSaveName] = useState('');
  const [saveDescription, setSaveDescription] = useState('');
  const [isSaving, setIsSaving] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [fileInputRef] = useState(React.createRef());

  // Load saved states and current metadata
  const refreshData = useCallback(() => {
    try {
      const states = EnhancedSceneStateManager.listSavedStates();
      setSavedStates(states);
      
      const metadata = EnhancedSceneStateManager.getCurrentStateMetadata();
      setCurrentMetadata(metadata);
    } catch (error) {
      console.error('Error refreshing data:', error);
      showNotification?.('Failed to refresh data', 'error');
    }
  }, [showNotification]);

  useEffect(() => {
    if (open) {
      refreshData();
    }
  }, [open, refreshData]);

  // Save current state
  const handleSaveState = useCallback(async () => {
    if (!saveName.trim()) {
      showNotification?.('Please enter a save name', 'warning');
      return;
    }

    setIsSaving(true);
    try {
      const result = EnhancedSceneStateManager.saveStateToStorage(saveName.trim(), saveDescription.trim());
      
      if (result.success) {
        showNotification?.(`Enhanced state "${saveName}" saved successfully`, 'success');
        setSaveDialogOpen(false);
        setSaveName('');
        setSaveDescription('');
        refreshData();
      } else {
        showNotification?.(`Failed to save state: ${result.error}`, 'error');
      }
    } catch (error) {
      console.error('Error saving state:', error);
      showNotification?.(`Error saving state: ${error.message}`, 'error');
    } finally {
      setIsSaving(false);
    }
  }, [saveName, saveDescription, showNotification, refreshData]);

  // Load state
  const handleLoadState = useCallback(async (stateName) => {
    setIsLoading(true);
    try {
      const result = EnhancedSceneStateManager.loadStateFromStorage(stateName);
      
      if (result.success) {
        showNotification?.(`Enhanced state "${stateName}" loaded successfully`, 'success');
        refreshData();
        onClose?.();
      } else {
        showNotification?.(`Failed to load state: ${result.error}`, 'error');
      }
    } catch (error) {
      console.error('Error loading state:', error);
      showNotification?.(`Error loading state: ${error.message}`, 'error');
    } finally {
      setIsLoading(false);
    }
  }, [showNotification, refreshData, onClose]);

  // Delete state
  const handleDeleteState = useCallback((stateName) => {
    if (window.confirm(`Are you sure you want to delete the enhanced state "${stateName}"?`)) {
      try {
        const success = EnhancedSceneStateManager.deleteSavedState(stateName);
        if (success) {
          showNotification?.(`Enhanced state "${stateName}" deleted`, 'success');
          refreshData();
        } else {
          showNotification?.(`Failed to delete state "${stateName}"`, 'error');
        }
      } catch (error) {
        console.error('Error deleting state:', error);
        showNotification?.(`Error deleting state: ${error.message}`, 'error');
      }
    }
  }, [showNotification, refreshData]);

  // Export to file
  const handleExportToFile = useCallback(() => {
    try {
      EnhancedSceneStateManager.exportToFile();
      showNotification?.('Scene state exported to file', 'success');
    } catch (error) {
      console.error('Error exporting to file:', error);
      showNotification?.(`Error exporting to file: ${error.message}`, 'error');
    }
  }, [showNotification]);

  // Import from file
  const handleImportFromFile = useCallback(async (event) => {
    const file = event.target.files[0];
    if (!file) return;

    try {
      const result = await EnhancedSceneStateManager.importFromFile(file);
      
      if (result.success) {
        showNotification?.(`Scene state imported from ${file.name}`, 'success');
        refreshData();
        onClose?.();
      } else {
        showNotification?.(`Failed to import from ${file.name}`, 'error');
      }
    } catch (error) {
      console.error('Error importing from file:', error);
      showNotification?.(`Error importing from file: ${error.message}`, 'error');
    }
    
    // Reset file input
    event.target.value = '';
  }, [showNotification, refreshData, onClose]);

  // Copy state JSON to clipboard
  const handleCopyStateJson = useCallback(() => {
    try {
      const stateJson = EnhancedSceneStateManager.exportCurrentState();
      navigator.clipboard.writeText(stateJson);
      showNotification?.('Scene state JSON copied to clipboard', 'success');
    } catch (error) {
      console.error('Error copying state JSON:', error);
      showNotification?.(`Error copying state JSON: ${error.message}`, 'error');
    }
  }, [showNotification]);

  const formatTimestamp = (timestamp) => {
    return new Date(timestamp).toLocaleString();
  };

  const formatFileSize = (bytes) => {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  };

  return (
    <>
      <Dialog open={open} onClose={onClose} maxWidth="md" fullWidth>
        <DialogTitle>
          <Box display="flex" alignItems="center" justifyContent="space-between">
            <Typography variant="h6">Enhanced Scene State Manager</Typography>
            <IconButton onClick={refreshData} disabled={isLoading}>
              <Refresh />
            </IconButton>
          </Box>
        </DialogTitle>
        
        <DialogContent>
          {/* Current Scene Info */}
          {currentMetadata && (
            <Paper sx={{ p: 2, mb: 2 }}>
              <Typography variant="h6" gutterBottom>Current Scene</Typography>
              <Grid container spacing={2}>
                <Grid item xs={6}>
                  <Typography variant="body2" color="textSecondary">Scene Name</Typography>
                  <Typography variant="body1">{currentMetadata.scene_name}</Typography>
                </Grid>
                <Grid item xs={6}>
                  <Typography variant="body2" color="textSecondary">Functions</Typography>
                  <Typography variant="body1">{currentMetadata.total_functions}</Typography>
                </Grid>
                <Grid item xs={6}>
                  <Typography variant="body2" color="textSecondary">State Size</Typography>
                  <Typography variant="body1">{formatFileSize(currentMetadata.state_size_bytes)}</Typography>
                </Grid>
                <Grid item xs={6}>
                  <Typography variant="body2" color="textSecondary">Capture Time</Typography>
                  <Typography variant="body1">{formatTimestamp(currentMetadata.capture_time)}</Typography>
                </Grid>
              </Grid>
            </Paper>
          )}

          {/* Action Buttons */}
          <Box sx={{ mb: 2 }}>
            <Grid container spacing={1}>
              <Grid item>
                <Button
                  variant="contained"
                  startIcon={<Save />}
                  onClick={() => setSaveDialogOpen(true)}
                  disabled={isSaving}
                >
                  Save State
                </Button>
              </Grid>
              <Grid item>
                <Button
                  variant="outlined"
                  startIcon={<Download />}
                  onClick={handleExportToFile}
                >
                  Export to File
                </Button>
              </Grid>
              <Grid item>
                <Button
                  variant="outlined"
                  startIcon={<Upload />}
                  onClick={() => fileInputRef.current?.click()}
                >
                  Import from File
                </Button>
              </Grid>
              <Grid item>
                <Button
                  variant="outlined"
                  startIcon={<ContentCopy />}
                  onClick={handleCopyStateJson}
                >
                  Copy JSON
                </Button>
              </Grid>
            </Grid>
          </Box>

          <input
            type="file"
            ref={fileInputRef}
            style={{ display: 'none' }}
            accept=".json"
            onChange={handleImportFromFile}
          />

          <Divider sx={{ my: 2 }} />

          {/* Saved States List */}
          <Typography variant="h6" gutterBottom>Saved States</Typography>
          
          {savedStates.length === 0 ? (
            <Alert severity="info">
              No enhanced states saved yet. Save your first state to get started!
            </Alert>
          ) : (
            <List>
              {savedStates.map((state) => (
                <ListItem key={state.saveName} divider>
                  <ListItemText
                    primary={
                      <Box display="flex" alignItems="center" gap={1}>
                        <Typography variant="subtitle1">{state.saveName}</Typography>
                        <Chip 
                          label={state.sceneName} 
                          size="small" 
                          variant="outlined" 
                        />
                        {state.version !== '2.0' && (
                          <Chip 
                            label={`v${state.version}`} 
                            size="small" 
                            color="warning" 
                          />
                        )}
                      </Box>
                    }
                    secondary={
                      <Box>
                        {state.description && (
                          <Typography variant="body2" color="textSecondary">
                            {state.description}
                          </Typography>
                        )}
                        <Typography variant="caption" color="textSecondary">
                          Saved: {formatTimestamp(state.timestamp)}
                          {state.metadata?.state_size_bytes && (
                            ` • Size: ${formatFileSize(state.metadata.state_size_bytes)}`
                          )}
                        </Typography>
                      </Box>
                    }
                  />
                  <ListItemSecondaryAction>
                    <Box display="flex" gap={1}>
                      <Tooltip title="Load State">
                        <IconButton
                          onClick={() => handleLoadState(state.saveName)}
                          disabled={isLoading}
                        >
                          <FolderOpen />
                        </IconButton>
                      </Tooltip>
                      <Tooltip title="Delete State">
                        <IconButton
                          onClick={() => handleDeleteState(state.saveName)}
                          color="error"
                        >
                          <Delete />
                        </IconButton>
                      </Tooltip>
                    </Box>
                  </ListItemSecondaryAction>
                </ListItem>
              ))}
            </List>
          )}
        </DialogContent>
        
        <DialogActions>
          <Button onClick={onClose}>Close</Button>
        </DialogActions>
      </Dialog>

      {/* Save Dialog */}
      <Dialog open={saveDialogOpen} onClose={() => setSaveDialogOpen(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Save Enhanced Scene State</DialogTitle>
        <DialogContent>
          <TextField
            autoFocus
            margin="dense"
            label="Save Name"
            fullWidth
            value={saveName}
            onChange={(e) => setSaveName(e.target.value)}
            placeholder="Enter a name for this state"
          />
          <TextField
            margin="dense"
            label="Description (Optional)"
            fullWidth
            multiline
            rows={3}
            value={saveDescription}
            onChange={(e) => setSaveDescription(e.target.value)}
            placeholder="Describe this state..."
          />
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setSaveDialogOpen(false)}>Cancel</Button>
          <Button 
            onClick={handleSaveState} 
            variant="contained"
            disabled={!saveName.trim() || isSaving}
          >
            {isSaving ? 'Saving...' : 'Save'}
          </Button>
        </DialogActions>
      </Dialog>
    </>
  );
};

export default EnhancedSceneStateManagerComponent; 