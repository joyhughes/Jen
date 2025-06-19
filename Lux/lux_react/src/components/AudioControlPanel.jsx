import React from 'react';
import '../styles/audioStyles.css';

const AudioControlPanel = ({ 
  isEnabled, 
  hasPermission, 
  audioFeatures, 
  sensitivity, 
  setSensitivity, 
  performance, 
  toggleAudio 
}) => {
  
  const getPermissionStatus = () => {
    if (hasPermission === true) return 'granted';
    if (hasPermission === false) return 'denied';
    return 'prompt';
  };

  const formatValue = (value) => Math.round(value * 100);

  return (
    <div className="audio-control-panel">
      <div className="audio-toggle-section">
        <button 
          className="audio-toggle-btn"
          onClick={toggleAudio}
          disabled={hasPermission === false}
        >
          {isEnabled ? 'Disable Audio' : 'Enable Audio'}
        </button>
        
        <div className="audio-status">
          {isEnabled ? 'Active' : 'Inactive'}
          <div className={`permission-status ${getPermissionStatus()}`}>
            {getPermissionStatus() === 'granted' && 'Microphone granted'}
            {getPermissionStatus() === 'denied' && 'Microphone denied'}
            {getPermissionStatus() === 'prompt' && 'Microphone pending'}
          </div>
        </div>
      </div>

      {isEnabled && (
        <>
          <div className="audio-meters">
            <div className="frequency-meter">
              <div className="meter-label">Bass</div>
              <div className="meter-bar">
                <div 
                  className="meter-fill bass"
                  style={{ width: `${formatValue(audioFeatures.bassLevel)}%` }}
                />
              </div>
            </div>
            
            <div className="frequency-meter">
              <div className="meter-label">Mid</div>
              <div className="meter-bar">
                <div 
                  className="meter-fill mid"
                  style={{ width: `${formatValue(audioFeatures.midLevel)}%` }}
                />
              </div>
            </div>
            
            <div className="frequency-meter">
              <div className="meter-label">High</div>
              <div className="meter-bar">
                <div 
                  className="meter-fill high"
                  style={{ width: `${formatValue(audioFeatures.highLevel)}%` }}
                />
              </div>
            </div>

            <div className="frequency-meter">
              <div className="meter-label">Volume</div>
              <div className="meter-bar">
                <div 
                  className="meter-fill volume"
                  style={{ width: `${formatValue(audioFeatures.volume)}%` }}
                />
              </div>
            </div>
          </div>

          <div className="beat-indicator">
            <div className={`beat-pulse ${audioFeatures.beatDetected ? 'active' : ''}`} />
            <span>Beat: {audioFeatures.beatDetected ? 'Yes' : 'No'}</span>
          </div>

          <div className="sensitivity-section">
            <div className="sensitivity-label">
              <span>Sensitivity</span>
              <span>{Math.round(sensitivity * 100)}%</span>
            </div>
            <input
              type="range"
              min="0"
              max="1"
              step="0.1"
              value={sensitivity}
              onChange={(e) => setSensitivity(parseFloat(e.target.value))}
              className="sensitivity-slider"
            />
          </div>

          <div className="performance-stats">
            <span>FPS: {performance.fps}</span>
            <span>Avg: {performance.avgProcessingTime.toFixed(1)}ms</span>
            <span>Drops: {performance.droppedFrames}</span>
          </div>
        </>
      )}
    </div>
  );
};

export default AudioControlPanel; 