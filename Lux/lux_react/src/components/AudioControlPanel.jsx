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
    <div className="audio-control-panel enhanced">
      <div className="audio-toggle-section">
        <button 
          className={`audio-toggle-btn ${isEnabled ? 'active' : ''}`}
          onClick={() => {
            console.log('🎵 Audio toggle button clicked');
            toggleAudio();
          }}
          disabled={hasPermission === false}
        >
          <span className="button-icon">🎤</span>
          <span className="button-text">
            {isEnabled ? 'Audio Active' : 'Enable Audio'}
          </span>
        </button>
        
        <div className="audio-status-card">
          <div className="status-indicator">
            <div className={`status-dot ${isEnabled ? 'active' : 'inactive'}`}></div>
            <span className="status-text">
              {isEnabled ? 'Active' : 'Inactive'}
            </span>
          </div>
          <div className={`permission-status ${getPermissionStatus()}`}>
            {getPermissionStatus() === 'granted' && '✓ Microphone Access'}
            {getPermissionStatus() === 'denied' && '✗ Microphone Denied'}
            {getPermissionStatus() === 'prompt' && '⏳ Requesting Access'}
          </div>
        </div>
      </div>

      {isEnabled && (
        <>
          <div className="audio-meters-grid">
            <div className="frequency-meter">
              <div className="meter-header">
                <span className="meter-label">Bass</span>
                <span className="meter-value">{formatValue(audioFeatures?.bassLevel)}%</span>
              </div>
              <div className="meter-bar">
                <div 
                  className="meter-fill bass"
                  style={{ width: `${formatValue(audioFeatures?.bassLevel)}%` }}
                />
              </div>
            </div>
            
            <div className="frequency-meter">
              <div className="meter-header">
                <span className="meter-label">Mid</span>
                <span className="meter-value">{formatValue(audioFeatures?.midLevel)}%</span>
              </div>
              <div className="meter-bar">
                <div 
                  className="meter-fill mid"
                  style={{ width: `${formatValue(audioFeatures?.midLevel)}%` }}
                />
              </div>
            </div>
            
            <div className="frequency-meter">
              <div className="meter-header">
                <span className="meter-label">High</span>
                <span className="meter-value">{formatValue(audioFeatures?.highLevel)}%</span>
              </div>
              <div className="meter-bar">
                <div 
                  className="meter-fill high"
                  style={{ width: `${formatValue(audioFeatures?.highLevel)}%` }}
                />
              </div>
            </div>

            <div className="frequency-meter">
              <div className="meter-header">
                <span className="meter-label">Volume</span>
                <span className="meter-value">{formatValue(audioFeatures?.volume)}%</span>
              </div>
              <div className="meter-bar">
                <div 
                  className="meter-fill volume"
                  style={{ width: `${formatValue(audioFeatures?.volume)}%` }}
                />
              </div>
            </div>
          </div>

          <div className="audio-features-section">
            <div className="beat-indicator-card">
              <div className="beat-indicator">
                <div className={`beat-pulse ${audioFeatures?.beatDetected ? 'active' : ''}`} />
                <span className="beat-text">Beat Detection</span>
              </div>
              <div className="beat-status">
                {audioFeatures?.beatDetected ? '🎵 Beat!' : '⏸️ Waiting'}
              </div>
            </div>

            <div className="activity-indicator-card">
              <div className="audio-activity-indicator">
                <div className={`activity-dot ${(audioFeatures?.volume || 0) > 0.01 ? 'active' : ''}`} />
                <span className="activity-text">Audio Input</span>
              </div>
              <div className="activity-status">
                {(audioFeatures?.volume || 0) > 0.01 ? '🔊 Detected' : '🔇 Silent'}
              </div>
            </div>
          </div>

          <div className="sensitivity-section-enhanced">
            <div className="sensitivity-header">
              <span className="sensitivity-title">🎛️ Sensitivity</span>
              <span className="sensitivity-value">{Math.round((sensitivity ?? 1.0) * 100)}%</span>
            </div>
            <div className="sensitivity-slider-container">
              <input
                type="range"
                min="0"
                max="200"
                step="5"
                value={Math.round((sensitivity ?? 1.0) * 100)}
                onChange={(e) => setSensitivity && setSensitivity(parseInt(e.target.value) / 100)}
                className="sensitivity-slider enhanced"
              />
              <div className="slider-marks">  
                <span>0%</span>
                <span>100%</span>
                <span>200%</span>
              </div>
            </div>
          </div>
        </>
      )}

      {!isEnabled && hasPermission === false && (
        <div className="no-permission-message">
          <div className="message-icon">🚫</div>
          <div className="message-text">
            Microphone access is required for audio reactive features.
            Please allow microphone access and try again.
          </div>
        </div>
      )}
    </div>
  );
};

export default AudioControlPanel; 