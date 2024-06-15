import React, { useState } from 'react';
import { ButtonGroup, Button, Tooltip } from '@mui/material';
import RestartIcon from '@mui/icons-material/Replay';      // This is just an example icon for "restart"
import FrameIcon from '@mui/icons-material/SkipNext';      // This is an example icon for "advance one frame"
import PlayPauseIcon from '@mui/icons-material/PlayArrow'; // This is an example icon for "play"
import PauseIcon from '@mui/icons-material/Pause';         // This is an example icon for "pause"
import WidgetContainer from './WidgetContainer';

function MediaController( { panelSize } ) { 
  const [isRunning, setIsRunning] = useState(true);

  const handleRestart = () => {
    if ( window.module ) {
      window.module.restart();
    }
  }

  const handleAdvance = () => {
    if ( window.module ) {
      setIsRunning(false);
      window.module.advance_frame();
    }
  }

  const handleRunPause = () => {
    setIsRunning(!isRunning);
    if ( window.module ) {
      window.module.run_pause();
    }
  }

  return (
    <WidgetContainer panelSize={panelSize}>
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
        < Tooltip title="Restart" disableInteractive >
          <Button onClick={handleRestart}>
            <RestartIcon />
          </Button>
        </Tooltip>

        < Tooltip title="Advance One Frame" disableInteractive >
          <Button onClick={handleAdvance}>
            <FrameIcon />
          </Button>
        </Tooltip>

        <Tooltip title={isRunning ? "Pause" : "Run"} disableInteractive >
        <Button onClick={handleRunPause}>
            {isRunning ? <PauseIcon /> : <PlayPauseIcon />}
          </Button>
        </Tooltip>
      </ButtonGroup>
    </WidgetContainer>
  );
};

export default MediaController;