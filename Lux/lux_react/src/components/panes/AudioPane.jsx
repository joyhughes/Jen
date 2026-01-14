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

  const [partyMode, setPartyMode] = useState(true);
  const [mixerExpanded, setMixerExpanded] = useState(true);
  const [showFrequencyBands, setShowFrequencyBands] = useState(false);
  const [showMixerPresets, setShowMixerPresets] = useState(true);
  const [integrationStatus, setIntegrationStatus] = useState(null);

  useEffect(() => {
    if (isEnabled) {
      const checkIntegration = () => {
        try {
          if (window.module && window.module.get_autoplay_audio_status) {
            const statusJson = window.module.get_autoplay_audio_status();
            const status = JSON.parse(statusJson);
            setIntegrationStatus(status);
          }
        } catch (error) {
          // Ignore integration check errors
        }
      };

      checkIntegration();
      const interval = setInterval(checkIntegration, 5000);
      return () => clearInterval(interval);
    } else {
      setIntegrationStatus(null);
    }
  }, [isEnabled]);

  useEffect(() => {
    if (partyMode && sensitivity < 0.5) {
      setSensitivity(0.5);
    } else if (!partyMode && sensitivity > 0.5) {
      setSensitivity(0.5);
    }
  }, [partyMode]);

  const handleToggleAudio = () => {
    if (isEnabled) {
      disableAudio();
    } else {
      enableAudio();
    }
  };

  const handlePartyModeToggle = () => {
    setPartyMode(!partyMode);
  };

  const handleFrequencyBandConfig = useCallback((config) => {
    if (config.gains && handleMixerGainChange) {
      handleMixerGainChange(config.gains);
    }
  }, [handleMixerGainChange]);

  const handleChannelRouting = useCallback((routingMatrix) => {
  }, []);

  const handleMixerStateChange = useCallback((mixerState) => {
  }, []);

  const checkIntegrationStatus = useCallback(() => {
    if (!window.module) return;

    try {
      const autoplayInfo = window.module.get_scene_autoplay_info ?
        JSON.parse(window.module.get_scene_autoplay_info()) : null;

      const audioStatus = window.module.get_autoplay_audio_status ?
        JSON.parse(window.module.get_autoplay_audio_status()) : null;

      setIntegrationStatus({
        ...audioStatus,
        ...autoplayInfo,
        timestamp: Date.now()
      });
    } catch (error) {
      // Ignore errors
    }
  }, []);

  const handleToggleAutoplay = useCallback(() => {
    if (!window.module || !window.module.enable_scene_autoplay) return;

    try {
      const newState = !integrationStatus?.autoplay_active;
      const success = window.module.enable_scene_autoplay(newState);

      if (success) {
        setTimeout(checkIntegrationStatus, 100);
      }
    } catch (error) {
      // Ignore errors
    }
  }, [integrationStatus?.autoplay_active, checkIntegrationStatus]);
  
  return (
    <div className={`audio-pane ${partyMode ? 'party-mode' : ''}`}>
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

      <div className="main-controls">
        {isEnabled && (
          <div className="audio-control-center">
            {mixerExpanded && (
              <>
                <div className="collapsible-header" onClick={() => setShowMixerPresets(!showMixerPresets)}>
                  <h4>Mixer Presets</h4>
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

            {mixerExpanded ? (
              <div className="collapsible-header" onClick={() => setShowFrequencyBands(!showFrequencyBands)}>
                <h4>Live Audio Frequency Bands</h4>
                <span className="collapse-icon">{showFrequencyBands ? '▲' : '▼'}</span>
              </div>
            ) : (
              <h4>Live Audio Frequency Bands</h4>
            )}
            {(mixerExpanded ? showFrequencyBands : true) && (
              <div className="simple-meters">
                <div className="meter-row">
                  <div className="meter-group">
                    <label>Volume</label>
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
                    <label>Bass</label>
                    <div className="meter">
                      <div
                        className="meter-fill bass"
                        style={{ width: `${Math.min(mixerState.bass || 0, 100)}%` }}
                      />
                    </div>
                    <span>{Math.round(mixerState.bass || 0)}%</span>
                  </div>

                  <div className="meter-group">
                    <label>Mid</label>
                    <div className="meter">
                      <div
                        className="meter-fill mid"
                        style={{ width: `${Math.min(mixerState.mid || 0, 100)}%` }}
                      />
                    </div>
                    <span>{Math.round(mixerState.mid || 0)}%</span>
                  </div>

                  <div className="meter-group">
                    <label>High</label>
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
          </div>
        )}
      </div>
    </div>
  );
};

export default AudioPane; 