import React, { useState } from 'react';
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

  const formatValue = (value) => Math.round((value || 0) * 100);

  return (
    <div className="audio-control-panel">
      <div className="audio-toggle-section">
        <button 
          className="audio-toggle-btn"
          onClick={() => {
            console.log('🎵 Audio toggle button clicked');
            toggleAudio();
          }}
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
                  style={{ width: `${formatValue(audioFeatures?.bassLevel)}%` }}
                />
              </div>
            </div>
            
            <div className="frequency-meter">
              <div className="meter-label">Mid</div>
              <div className="meter-bar">
                <div 
                  className="meter-fill mid"
                  style={{ width: `${formatValue(audioFeatures?.midLevel)}%` }}
                />
              </div>
            </div>
            
            <div className="frequency-meter">
              <div className="meter-label">High</div>
              <div className="meter-bar">
                <div 
                  className="meter-fill high"
                  style={{ width: `${formatValue(audioFeatures?.highLevel)}%` }}
                />
              </div>
            </div>

            <div className="frequency-meter">
              <div className="meter-label">Volume</div>
              <div className="meter-bar">
                <div 
                  className="meter-fill volume"
                  style={{ width: `${formatValue(audioFeatures?.volume)}%` }}
                />
              </div>
            </div>
          </div>

          <div className="beat-indicator">
            <div className={`beat-pulse ${audioFeatures?.beatDetected ? 'active' : ''}`} />
            <span>Beat: {audioFeatures?.beatDetected ? 'Yes' : 'No'}</span>
          </div>

          <div className="sensitivity-section">
            <div className="sensitivity-label">
              <span>Sensitivity</span>
              <span>{Math.round((sensitivity || 0) * 100)}%</span>
            </div>
            <input
              type="range"
              min="0"
              max="100"
              step="5"
              value={Math.round((sensitivity || 0.8) * 100)}
              onChange={(e) => setSensitivity && setSensitivity(parseInt(e.target.value) / 100)}
              className="sensitivity-slider"
            />
          </div>

          {audioFeatures?.dominantFreq && (
            <div className="frequency-info">
              <span>Dominant: {Math.round(audioFeatures.dominantFreq)}Hz</span>
            </div>
          )}

          {performance && (
            <div className="performance-stats">
              <span>FPS: {performance.fps || 0}</span>
              <span>Quality: {performance.avgProcessingTime < 2 ? 'high' : 'medium'}</span>
            </div>
          )}

          <div className="audio-activity-indicator">
            <div className={`activity-dot ${(audioFeatures?.volume || 0) > 0.01 ? 'active' : ''}`} />
            <span>Audio: {(audioFeatures?.volume || 0) > 0.01 ? 'Detected' : 'Silent'}</span>
          </div>
        </>
      )}
    </div>
  );
};

export default AudioControlPanel; 