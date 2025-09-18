import React, { useState, useCallback, useEffect } from 'react';
import { useAudioContext } from '../AudioContext';
import AudioMixer from '../AudioMixer';
import './AudioPane.css';
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

  const [partyMode, setPartyMode] = useState(true); // Default ON for birthday celebration
  const [mixerExpanded, setMixerExpanded] = useState(true); // Default expanded since mixer is primary control
  
  const [showFrequencyBands, setShowFrequencyBands] = useState(false);
  const [showMixerPresets, setShowMixerPresets] = useState(true);
  
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
        </div>
      </div>

      {/* Main Controls */}
      <div className="main-controls">
        {/* Audio Control Center - Flat Layout */}
        {isEnabled && (
          <div className="audio-control-center">
            {/* Live Audio Frequency Bands */}
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
              </div>
            )}


            {/* Mixer Presets */}
            {mixerExpanded && (
              <>
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
                  </div>
                )}
              </>
            )}
          </div>
        )}
      </div>
    </div>
  );
};

export default AudioPane; 