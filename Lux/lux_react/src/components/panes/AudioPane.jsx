import React, { useState, useCallback, useEffect } from 'react';
import { useAudioContext } from '../AudioContext';
import AudioMixer from '../AudioMixer';
import './AudioPane.css';
import { Accordion, AccordionSummary, AccordionDetails } from '@mui/material';
import { ExpandMore } from '@mui/icons-material';
import { Typography, Box, Button } from '@mui/material';
import { HiMicrophone } from 'react-icons/hi';
import { FaMicrophoneSlash } from "react-icons/fa6";


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
  const [showIntegrationStatus, setShowIntegrationStatus] = useState(false);
  
  // Integration status state
  const [integrationStatus, setIntegrationStatus] = useState(null);

  // Log when component mounts/unmounts to debug navigation issues
  React.useEffect(() => {
    console.log('🎵 📱 AudioPane mounted, audio enabled:', isEnabled);
    return () => {
      console.log('🎵 📱 AudioPane unmounting, keeping audio running...');
    };
  }, [isEnabled]);

  // Check integration status when audio is enabled
  useEffect(() => {
    if (isEnabled) {
      const checkIntegration = () => {
        try {
          if (window.module && window.module.get_autoplay_audio_status) {
            const statusJson = window.module.get_autoplay_audio_status();
            const status = JSON.parse(statusJson);
            setIntegrationStatus(status);
            console.log('🎵 🎲 Autoplay+Audio integration status:', status);
          }
        } catch (error) {
          console.warn('🎵 ⚠️ Could not check integration status:', error);
        }
      };
      
      // Check immediately and then every 5 seconds
      checkIntegration();
      const interval = setInterval(checkIntegration, 5000);
      
      return () => clearInterval(interval);
    } else {
      setIntegrationStatus(null);
    }
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

  // Enhanced integration status check
  const checkIntegrationStatus = useCallback(() => {
    if (!window.module) return;
    
    try {
      // Get scene-agnostic autoplay info
      const autoplayInfo = window.module.get_scene_autoplay_info ? 
        JSON.parse(window.module.get_scene_autoplay_info()) : null;
      
      // Get audio status  
      const audioStatus = window.module.get_autoplay_audio_status ? 
        JSON.parse(window.module.get_autoplay_audio_status()) : null;
      
      setIntegrationStatus({
        ...audioStatus,
        ...autoplayInfo,
        timestamp: Date.now()
      });
      
    } catch (error) {
      console.error('🎵 ❌ Error checking integration status:', error);
    }
  }, []);

  // Autoplay control handlers
  const [autoplayIntensity, setAutoplayIntensity] = useState(0.002);
  
  const handleToggleAutoplay = useCallback(() => {
    if (!window.module || !window.module.enable_scene_autoplay) return;
    
    try {
      const newState = !integrationStatus?.autoplay_active;
      const success = window.module.enable_scene_autoplay(newState);
      
      if (success) {
        console.log(`🎲 Autoplay ${newState ? 'enabled' : 'disabled'} for current scene`);
        // Refresh status
        setTimeout(checkIntegrationStatus, 100);
      }
    } catch (error) {
      console.error('🎲 ❌ Error toggling autoplay:', error);
    }
  }, [integrationStatus?.autoplay_active, checkIntegrationStatus]);
  
  const handleIntensityChange = useCallback((e) => {
    const intensity = parseFloat(e.target.value);
    setAutoplayIntensity(intensity);
    
    if (window.module && window.module.set_autoplay_intensity) {
      try {
        const success = window.module.set_autoplay_intensity(intensity);
        if (success) {
          console.log(`🎲 Autoplay intensity set to: ${intensity}`);
        }
      } catch (error) {
        console.error('🎲 ❌ Error setting autoplay intensity:', error);
      }
    }
  }, []);

  return (
    <div className={`audio-pane ${partyMode ? 'party-mode' : ''}`}>
      {/* Mobile-First Header */}
      <div className="audio-header">
        <div className="header-main">
          <button 
            className={`audio-toggle ${isEnabled ? 'enabled' : 'disabled'}`}
            onClick={handleToggleAudio}
            title={isEnabled ? 'Click to mute microphone' : 'Click to unmute microphone'}
          >
            <span className="toggle-icon">
              {isEnabled ? <HiMicrophone size={24} /> : <FaMicrophoneSlash size={24} />}
            </span>
          </button>
          
          <button 
            className={`party-toggle ${partyMode ? 'active' : ''}`}
            onClick={handlePartyModeToggle}
            title="Birthday Party Mode - Enhanced sensitivity for celebration!"
          >
            🎂
          </button>
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
                      <div className="party-mode-info">
                        <div className="party-header">
                          <h4>🎂 Birthday Party Mode Active!</h4>
                          <p>Enhanced sensitivity and celebration effects enabled</p>
                        </div>
                      </div>
                    )}
                  </div>
                )}

                {/* Scene-Agnostic Autoplay Controls */}
                <div className="collapsible-section">
                  <div className="collapsible-header" onClick={() => setShowIntegrationStatus(!showIntegrationStatus)}>
                    <h4>🎲 Scene Autoplay</h4>
                    <span className="collapse-icon">{showIntegrationStatus ? '▲' : '▼'}</span>
                  </div>
                  {showIntegrationStatus && (
                    <div className="autoplay-controls">
                      <div className="autoplay-status">
                        <p>Universal autoplay system that works with any scene</p>
                        {integrationStatus && (
                          <div className="status-info">
                            <span className={`status-badge ${integrationStatus.has_autoplay ? 'active' : 'inactive'}`}>
                              {integrationStatus.has_autoplay ? '✅ Scene supports autoplay' : '❌ No autoplay in scene'}
                            </span>
                            {integrationStatus.has_autoplay && (
                              <div className="autoplay-scene-controls">
                                <button 
                                  className={`autoplay-toggle ${integrationStatus.autoplay_active ? 'active' : ''}`}
                                  onClick={handleToggleAutoplay}
                                >
                                  {integrationStatus.autoplay_active ? '⏸️ Disable Autoplay' : '▶️ Enable Autoplay'}
                                </button>
                                
                                <div className="intensity-control">
                                  <label>Autoplay Intensity:</label>
                                  <input 
                                    type="range"
                                    min="0.0001"
                                    max="0.01"
                                    step="0.0001"
                                    value={autoplayIntensity}
                                    onChange={handleIntensityChange}
                                    className="intensity-slider"
                                  />
                                  <span className="intensity-value">{(autoplayIntensity * 100).toFixed(2)}%</span>
                                </div>
                                
                                {integrationStatus.autoplay_parameters && integrationStatus.autoplay_parameters.length > 0 && (
                                  <div className="autoplay-parameters">
                                    <h5>Autoplay-Influenced Parameters:</h5>
                                    <ul>
                                      {integrationStatus.autoplay_parameters.map((param, index) => (
                                        <li key={index} className={`param-${param.type}`}>
                                          <span className="param-name">{param.name}</span>
                                          <span className="param-type">{param.type}</span>
                                        </li>
                                      ))}
                                    </ul>
                                  </div>
                                )}
                              </div>
                            )}
                          </div>
                        )}
                      </div>
                    </div>
                  )}
                </div>
              </div>
            )}
          </div>
        )}
      </div>
    </div>
  );
};

export default AudioPane; 