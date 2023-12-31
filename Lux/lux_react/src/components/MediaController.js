import React, { useState } from 'react';
import useEmscripten from './useEmscripten';
import { ButtonGroup, Button, Tooltip } from '@mui/material';
import RestartIcon from '@mui/icons-material/Replay'; // This is just an example icon for "restart"
import FrameIcon from '@mui/icons-material/SkipNext'; // This is an example icon for "advance one frame"
import PlayPauseIcon from '@mui/icons-material/PlayArrow'; // This is an example icon for "play"
import PauseIcon from '@mui/icons-material/Pause'; // This is an example icon for "pause"

const MediaController = () => {
  const [isPlaying, setIsPlaying] = useState(false);
//  module = useEmscripten();

  return (
    <ButtonGroup 
      variant="contained"
      sx={{
        maxHeight: '40px',
        '& .MuiButton-root': {
          width: '40px',
          height: '40px'
        }
      }}
    >
      <Tooltip title="Restart">
        <Button onClick={() => { /* Restart logic here */ }}>
          <RestartIcon />
        </Button>
      </Tooltip>

      <Tooltip title="Advance One Frame">
        <Button onClick={() => { /* Advance one frame logic here */ }}>
          <FrameIcon />
        </Button>
      </Tooltip>

      <Tooltip title={isPlaying ? "Pause" : "Play"}>
        <Button onClick={() => setIsPlaying(!isPlaying)}>
          {isPlaying ? <PauseIcon /> : <PlayPauseIcon />}
        </Button>
      </Tooltip>
    </ButtonGroup>
  );
};

export default MediaController;


