import React, { useState, useCallback, useEffect } from 'react';
import { useAudioContext } from '../AudioContext';
import AudioMixer from '../AudioMixer';
import './AudioPane.css';
import { Accordion, AccordionSummary, AccordionDetails } from '@mui/material';
import { ExpandMore } from '@mui/icons-material';
import { Typography, Box, Button } from '@mui/material';

const AudioPane = () => {
  const {
    isEnabled,
    mixerState,
    audioFeatures,
    sensitivity,
    setSensitivity,
    enableAudio,
    disableAudio,
    handleMixerGainChange
  } = useAudioContext();

  // Party mode state
  const [partyMode, setPartyMode] = useState(true); // Default ON for birthday celebration
  const [mixerExpanded, setMixerExpanded] = useState(false);
  const [showAdvanced, setShowAdvanced] = useState(false);
  
  // Collapsible sections for advanced view
  const [showFrequencyBands, setShowFrequencyBands] = useState(false);
  const [showMasterSensitivity, setShowMasterSensitivity] = useState(false);
  const [showMixerPresets, setShowMixerPresets] = useState(true);

  // Log when component mounts/unmounts to debug navigation issues
  React.useEffect(() => {
    console.log('🎵 📱 AudioPane mounted, audio enabled:', isEnabled);
    return () => {
      console.log('🎵 📱 AudioPane unmounting, keeping audio running...');
    };
  }, [isEnabled]);

  // Set sensitivity based on party mode (only when party mode changes)
  useEffect(() => {
    if (partyMode && sensitivity < 0.5) {
      setSensitivity(0.5); // Default to 50% for party mode
    } else if (!partyMode && sensitivity > 0.5) {
      setSensitivity(0.5); // Reset to 50% for normal mode
    }
  }, [partyMode]); // Only depend on partyMode, not sensitivity!

  const handleToggleAudio = () => {
    if (isEnabled) {
      disableAudio();
    } else {
      enableAudio();
    }
  };

  const handlePartyModeToggle = () => {
    setPartyMode(!partyMode);
    // The useEffect will handle sensitivity changes based on the new party mode state
  };

  const handleSensitivityChange = useCallback((value) => {
    console.log(`🎛️ Sensitivity changed to: ${value}%`);
    setSensitivity(value / 100); // Convert percentage to decimal
  }, [setSensitivity]);

  const handleFrequencyBandConfig = useCallback((config) => {
    console.log('🎛️ Frequency band config updated:', config);
    if (config.gains && handleMixerGainChange) {
      handleMixerGainChange(config.gains);
    }
  }, [handleMixerGainChange]);

  const handleChannelRouting = useCallback((routingMatrix) => {
    console.log('🎛️ Channel routing updated:', routingMatrix);
    // Could implement parameter routing here if needed
  }, []);

  const handleMixerStateChange = useCallback((mixerState) => {
    console.log('🎛️ Mixer state changed:', mixerState);
  }, []);

  // Convert sensitivity from decimal to percentage for display
  const sensitivityPercentage = Math.round(sensitivity * 100);
  const maxSensitivity = 200; // 0-200% range for all modes

  return (
    <div className={`audio-pane ${partyMode ? 'party-mode' : ''}`}>
      {/* Mobile-First Header */}
      <div className="audio-header">
        <div className="header-main">
          <button 
            className={`audio-toggle ${isEnabled ? 'enabled' : 'disabled'}`}
            onClick={handleToggleAudio}
          >
            <span className="toggle-icon">{isEnabled ? '🎵' : '🔇'}</span>
            <span className="toggle-text">{isEnabled ? 'Audio ON' : 'Audio OFF'}</span>
          </button>
          
          <button 
            className={`party-toggle ${partyMode ? 'active' : ''}`}
            onClick={handlePartyModeToggle}
            title="Birthday Party Mode - Enhanced sensitivity for celebration!"
          >
            🎂 Party Mode
          </button>
        </div>

        {/* Status Indicators */}
        <div className="status-bar">
          <div className="status-chip">
            <span className={isEnabled ? 'status-active' : 'status-inactive'}>
              {isEnabled ? '🟢 Active' : '🔴 Inactive'}
            </span>
          </div>
          {partyMode && (
            <div className="status-chip party">
              🎉 Birthday Mode
            </div>
          )}
          <div className="status-chip">
            Scene-Agnostic
          </div>
        </div>
      </div>

      {/* Main Controls */}
      <div className="main-controls">
        {/* Mixer Presets Container - Reorganized */}
        {isEnabled && (
          <div className="mixer-presets">
            <div className="viz-header">
              <h3>🎛️ Audio Control Center</h3>
              <button 
                className="mixer-toggle"
                onClick={() => setMixerExpanded(!mixerExpanded)}
              >
                {mixerExpanded ? '📊 Simple' : '🎛️ Advanced'}
              </button>
            </div>

            {/* Live Audio Frequency Bands - Top of mixer */}
            <div className="frequency-bands">
              {mixerExpanded ? (
                <div className="collapsible-header" onClick={() => setShowFrequencyBands(!showFrequencyBands)}>
                  <h4>🎵 Live Audio Frequency Bands</h4>
                  <span className="collapse-icon">{showFrequencyBands ? '▲' : '▼'}</span>
                </div>
              ) : (
                <h4>🎵 Live Audio Frequency Bands</h4>
              )}
              {(mixerExpanded ? showFrequencyBands : true) && (
                <div className="simple-meters">
                  <div className="meter-row">
                    <div className="meter-group">
                      <label>🔊 Volume</label>
                      <div className="meter">
                        <div 
                          className="meter-fill volume" 
                          style={{ width: `${Math.min(mixerState.volume || 0, 100)}%` }}
                        />
                      </div>
                      <span>{Math.round(mixerState.volume || 0)}%</span>
                    </div>
                  </div>

                  <div className="meter-row">
                    <div className="meter-group">
                      <label>🎸 Bass</label>
                      <div className="meter">
                        <div 
                          className="meter-fill bass" 
                          style={{ width: `${Math.min(mixerState.bass || 0, 100)}%` }}
                        />
                      </div>
                      <span>{Math.round(mixerState.bass || 0)}%</span>
                    </div>
                    
                    <div className="meter-group">
                      <label>🎤 Mid</label>
                      <div className="meter">
                        <div 
                          className="meter-fill mid" 
                          style={{ width: `${Math.min(mixerState.mid || 0, 100)}%` }}
                        />
                      </div>
                      <span>{Math.round(mixerState.mid || 0)}%</span>
                    </div>
                    
                    <div className="meter-group">
                      <label>✨ High</label>
                      <div className="meter">
                        <div 
                          className="meter-fill high" 
                          style={{ width: `${Math.min(mixerState.high || 0, 100)}%` }}
                        />
                      </div>
                      <span>{Math.round(mixerState.high || 0)}%</span>
                    </div>
                  </div>

                  {/* Beat Indicator */}
                  <div className="beat-section">
                    <div className={`beat-indicator ${mixerState.beat ? 'active' : ''}`}>
                      <span className="beat-icon">🥁</span>
                      <span className="beat-text">BEAT</span>
                    </div>
                    
                    <div className="processing-status">
                      <span className={mixerState.isProcessing ? 'status-active' : 'status-inactive'}>
                        {mixerState.isProcessing ? '🎵 Processing' : '⏸️ Quiet'}
                      </span>
                    </div>
                  </div>
                </div>
              )}
            </div>

            {/* Master Sensitivity - Middle of mixer */}
            <div className="master-sensitivity">
              {mixerExpanded ? (
                <div className="collapsible-header" onClick={() => setShowMasterSensitivity(!showMasterSensitivity)}>
                  <h4>🎚️ Master Sensitivity</h4>
                  <span className="collapse-icon">{showMasterSensitivity ? '▲' : '▼'}</span>
                </div>
              ) : (
                <h4>🎚️ Master Sensitivity</h4>
              )}
              {(mixerExpanded ? showMasterSensitivity : true) && (
                <div className="sensitivity-content">
                  <div className="sensitivity-header">
                    <label>
                      {partyMode ? '🎉 Party Intensity' : 'Global Sensitivity'}
                      <span className="sensitivity-value">{sensitivityPercentage}%</span>
                    </label>
                    {partyMode && (
                      <div className="party-hint">
                        🎂 Perfect for Jen's Birthday! 🎈
                      </div>
                    )}
                  </div>
                  
                  <div className="sensitivity-control">
                    <input
                      type="range"
                      min="0"
                      max={maxSensitivity}
                      step="5"
                      value={sensitivityPercentage}
                      onChange={(e) => handleSensitivityChange(parseInt(e.target.value))}
                      className={`sensitivity-slider ${partyMode ? 'party-slider' : ''}`}
                    />
                    <div className="sensitivity-markers">
                      <span>0%</span>
                      <span>50%</span>
                      <span>100%</span>
                      <span>{maxSensitivity}%</span>
                    </div>
                  </div>
                </div>
              )}
            </div>

            {/* Test Animation Speed Control */}
            <Accordion sx={{ mb: 2 }}>
              <AccordionSummary expandIcon={<ExpandMore />}>
                <Typography variant="h6" sx={{ fontWeight: 600 }}>
                  🏃 Animation Speed Test
                </Typography>
              </AccordionSummary>
              <AccordionDetails>
                <Typography variant="body2" sx={{ mb: 2, color: 'text.secondary' }}>
                  Test audio-controlled animation speed. Your voice will automatically control speed when audio is enabled.
                </Typography>
                
                <Box sx={{ mb: 2 }}>
                  <Typography variant="body2" sx={{ mb: 1, fontWeight: 600 }}>
                    Manual Speed Control (for testing):
                  </Typography>
                  <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
                    <Button 
                      variant="outlined" 
                      size="small"
                      onClick={() => {
                        if (window.module?.set_animation_speed_multiplier) {
                          window.module.set_animation_speed_multiplier(0.5);
                        }
                      }}
                    >
                      0.5x Slow
                    </Button>
                    <Button 
                      variant="outlined" 
                      size="small"
                      onClick={() => {
                        if (window.module?.set_animation_speed_multiplier) {
                          window.module.set_animation_speed_multiplier(1.0);
                        }
                      }}
                    >
                      1.0x Normal
                    </Button>
                    <Button 
                      variant="outlined" 
                      size="small"
                      onClick={() => {
                        if (window.module?.set_animation_speed_multiplier) {
                          window.module.set_animation_speed_multiplier(2.0);
                        }
                      }}
                    >
                      2.0x Fast
                    </Button>
                    <Button 
                      variant="outlined" 
                      size="small"
                      onClick={() => {
                        if (window.module?.set_animation_speed_multiplier) {
                          window.module.set_animation_speed_multiplier(3.0);
                        }
                      }}
                    >
                      3.0x Turbo
                    </Button>
                  </Box>
                </Box>
                
                <Typography variant="body2" sx={{ color: 'text.secondary', fontStyle: 'italic' }}>
                  💡 When audio is enabled, your voice will automatically control animation speed: 
                  louder = faster animation!
                </Typography>
              </AccordionDetails>
            </Accordion>

            {/* Advanced Mixer Presets - Bottom of mixer */}
            {mixerExpanded && (
              <div className="advanced-mixer-section">
                <div className="collapsible-header" onClick={() => setShowMixerPresets(!showMixerPresets)}>
                  <h4>🎛️ Mixer Presets</h4>
                  <span className="collapse-icon">{showMixerPresets ? '▲' : '▼'}</span>
                </div>
                {showMixerPresets && (
                  <div className="mixer-content">
                    <AudioMixer
                      audioFeatures={audioFeatures}
                      isEnabled={isEnabled}
                      onChannelRouting={handleChannelRouting}
                      onFrequencyBandConfig={handleFrequencyBandConfig}
                      onMixerStateChange={handleMixerStateChange}
                      sensitivity={sensitivity}
                      setSensitivity={setSensitivity}
                      partyMode={partyMode}
                    />

                    {/* Birthday Party Mode Info - Only in advanced view */}
                    {partyMode && (
                      <div className="party-info">
                        <div className="party-header">
                          <span>🎂</span>
                          <h3>Birthday Party Mode</h3>
                          <span>🎈</span>
                        </div>
                        <div className="party-features">
                          <div className="feature-item">
                            <span>🎵</span>
                            <span>Enhanced sensitivity for singing & music</span>
                          </div>
                          <div className="feature-item">
                            <span>🎸</span>
                            <span>Boosted bass response for party vibes</span>
                          </div>
                          <div className="feature-item">
                            <span>✨</span>
                            <span>Dynamic high-frequency sparkle</span>
                          </div>
                          <div className="feature-item">
                            <span>🥁</span>
                            <span>Improved beat detection for dancing</span>
                          </div>
                        </div>
                      </div>
                    )}
                  </div>
                )}
              </div>
            )}
          </div>
        )}
      </div>
    </div>
  );
};

export default AudioPane; 