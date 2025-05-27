import React, { useState, useRef } from 'react';
import { ButtonGroup, Button, Tooltip } from '@mui/material';
import RestartIcon from '@mui/icons-material/Replay'; // This is just an example icon for "restart"
import FrameIcon from '@mui/icons-material/SkipNext'; // This is an example icon for "advance one frame"
import PlayPauseIcon from '@mui/icons-material/PlayArrow'; // This is an example icon for "play"
import PauseIcon from '@mui/icons-material/Pause'; // This is an example icon for "pause"
import VideocamIcon from '@mui/icons-material/Videocam';
import VideocamOffIcon from '@mui/icons-material/VideocamOff';

const MediaController = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isRecording, setIsRecording] = useState(false);
  const workerRef = useRef(null);

  const startRecording = async () => {
    if (!workerRef.current) {
      workerRef.current = new Worker('/workers/videoEncodingWorker.js');
    }

    const options = {
      width: 946,  // Match your canvas size
      height: 532,
      fps: 30,
      bitrate: 2000000,
      codec: 'libx264',  // Use H.264 codec
      format: 'mp4',   // Use MP4 format
      preset: 'ultrafast'
    };

    workerRef.current.postMessage({
      type: 'startRecording',
      options
    });

    setIsRecording(true);
  };

  const stopRecording = () => {
    if (workerRef.current) {
      workerRef.current.postMessage({ type: 'stopRecording' });
      setIsRecording(false);
    }
  };

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

      <Tooltip title={isRecording ? "Stop Recording" : "Start Recording"}>
        <Button onClick={isRecording ? stopRecording : startRecording}>
          {isRecording ? <VideocamOffIcon /> : <VideocamIcon />}
        </Button>
      </Tooltip>
    </ButtonGroup>
  );
};

export default MediaController;


