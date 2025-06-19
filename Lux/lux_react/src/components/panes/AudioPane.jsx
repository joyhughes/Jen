import React from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import AudioControlPanel from '../AudioControlPanel.jsx';

const AudioPane = ({ 
  dimensions, 
  panelSize,
  isEnabled,
  hasPermission,
  audioFeatures,
  sensitivity,
  setSensitivity,
  performance,
  toggleAudio
}) => {
  return (
    <Box sx={{ 
      p: 2, 
      height: '100%', 
      display: 'flex', 
      flexDirection: 'column',
      gap: 2
    }}>
      <Typography 
        variant="h6" 
        sx={{ 
          color: 'white', 
          fontWeight: 'bold',
          mb: 1,
          textAlign: 'center'
        }}
      >
        🎵 Audio Reactive Controls
      </Typography>
      
      <Typography 
        variant="body2" 
        sx={{ 
          color: 'rgba(255, 255, 255, 0.7)', 
          mb: 2,
          textAlign: 'center',
          fontSize: '0.85rem'
        }}
      >
        Control how the visuals respond to your microphone input
      </Typography>

      <Box sx={{ 
        flex: 1,
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'center'
      }}>
        <AudioControlPanel
          isEnabled={isEnabled}
          hasPermission={hasPermission}
          audioFeatures={audioFeatures}
          sensitivity={sensitivity}
          setSensitivity={setSensitivity}
          performance={performance}
          toggleAudio={toggleAudio}
        />
      </Box>

      {/* Future expansion area */}
      <Box sx={{ 
        mt: 'auto',
        pt: 2,
        borderTop: '1px solid rgba(255, 255, 255, 0.1)'
      }}>
        <Typography 
          variant="caption" 
          sx={{ 
            color: 'rgba(255, 255, 255, 0.5)',
            textAlign: 'center',
            display: 'block'
          }}
        >
          More audio features coming soon...
        </Typography>
      </Box>
    </Box>
  );
};

export default AudioPane; 