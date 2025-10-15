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


  // Advanced mixer frequency bands
  const frequencyBands = [
    { id: 'volume', name: 'Volume', range: 'Overall', color: '#666', value: audioFeatures?.volume || 0 },
    { id: 'bass', name: 'Bass', range: '60-250Hz', color: '#ff3366', value: audioFeatures?.bassLevel || 0 },
    { id: 'mid', name: 'Mid', range: '500-2kHz', color: '#ffaa33', value: audioFeatures?.midLevel || 0 },
    { id: 'high', name: 'High', range: '2k+Hz', color: '#66ff33', value: audioFeatures?.highLevel || 0 }
  ];

  const linearToDb = (linear) => {
    if (linear <= 0) return -60;
    return 20 * Math.log10(linear);
  };

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
          {/* Vertical Mixer Strips - Primary Control */}
          <div className="mixer-strips-container">
            <div className="mixer-header">
              <h3>🎛️ Audio Mixer</h3>
              <div className="beat-indicator">
                <div className={`beat-pulse ${audioFeatures?.beatDetected ? 'active' : ''}`} />
                <span className="beat-text">{audioFeatures?.beatDetected ? '🎵 Beat!' : '⏸️ Waiting'}</span>
              </div>
            </div>
            
            <div className="frequency-strips">
              {frequencyBands.map(band => (
                <div key={band.id} className="band-strip">
                  <div className="band-header">
                    <span className="band-name" style={{ color: band.color }}>
                      {band.name}
                    </span>
                    <span className="band-range">{band.range}</span>
                  </div>
                  
                  <div className="band-meter">
                    <div 
                      className="level-bar"
                      style={{ 
                        height: `${Math.max(0, Math.min(100, ((linearToDb(band.value) + 60) / 60 * 100)))}%`,
                        backgroundColor: band.color 
                      }}
                    />
                    <div className="meter-scale">
                      <span className="db-marker" style={{ bottom: '100%' }}>0</span>
                      <span className="db-marker" style={{ bottom: '75%' }}>-12</span>
                      <span className="db-marker" style={{ bottom: '50%' }}>-24</span>
                      <span className="db-marker" style={{ bottom: '25%' }}>-36</span>
                      <span className="db-marker" style={{ bottom: '0%' }}>-60</span>
                    </div>
                  </div>
                  
                  <div className="level-display">
                    <div className="level-db" style={{ color: band.color }}>
                      {linearToDb(band.value).toFixed(1)}dB
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </div>

          <div className="sensitivity-section-enhanced">
            <div className="sensitivity-header">
              <span className="sensitivity-title">🎛️ Master Sensitivity</span>
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