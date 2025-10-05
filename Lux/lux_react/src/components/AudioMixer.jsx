import React, { useState, useEffect, useCallback, useRef } from 'react';
import './AudioMixer.css';

const AudioMixer = ({ 
  audioFeatures, 
  isEnabled, 
  onChannelRouting, 
  onFrequencyBandConfig,
  onMixerStateChange,
  sensitivity,
  setSensitivity,
  partyMode = false
}) => {
  // Enhanced frequency band configuration
  const [frequencyBands] = useState([
    { id: 'sub_bass', name: 'Sub Bass', range: '20-60Hz', color: '#ff0066', min: 0, max: 0.125 },
    { id: 'bass', name: 'Bass', range: '60-250Hz', color: '#ff3366', min: 0.125, max: 0.25 },
    { id: 'low_mid', name: 'Low Mid', range: '250-500Hz', color: '#ff6633', min: 0.25, max: 0.375 },
    { id: 'mid', name: 'Mid', range: '500-2kHz', color: '#ffaa33', min: 0.375, max: 0.5 },
    { id: 'high_mid', name: 'High Mid', range: '2k-4kHz', color: '#aaff33', min: 0.5, max: 0.625 },
    { id: 'presence', name: 'Presence', range: '4k-8kHz', color: '#66ff33', min: 0.625, max: 0.75 },
    { id: 'brilliance', name: 'Brilliance', range: '8k-16kHz', color: '#33ff66', min: 0.75, max: 0.875 },
    { id: 'air', name: 'Air', range: '16k+Hz', color: '#33ffaa', min: 0.875, max: 1.0 }
  ]);

  // Mixer state
  const [bandLevels, setBandLevels] = useState({});
  const [bandGains, setBandGains] = useState({});
  const [bandGainsDb, setBandGainsDb] = useState({});
  const [routingMatrix, setRoutingMatrix] = useState({});
  const [mixerPresets] = useState([
    { 
      id: 'party_balanced', 
      name: '🎉 Party Balanced',
      description: 'Equal response across all frequencies',
      gains: { sub_bass: 0.8, bass: 1.0, low_mid: 0.9, mid: 1.0, high_mid: 0.9, presence: 0.8, brilliance: 0.7, air: 0.6 },
      gainsDb: { sub_bass: -1.9, bass: 0.0, low_mid: -0.9, mid: 0.0, high_mid: -0.9, presence: -1.9, brilliance: -3.1, air: -4.4 }
    },
    { 
      id: 'bass_heavy', 
      name: '🔊 Bass Heavy',
      description: 'Emphasized low-end for electronic music',
      gains: { sub_bass: 1.2, bass: 1.5, low_mid: 1.0, mid: 0.8, high_mid: 0.7, presence: 0.6, brilliance: 0.5, air: 0.4 },
      gainsDb: { sub_bass: 1.6, bass: 3.5, low_mid: 0.0, mid: -1.9, high_mid: -3.1, presence: -4.4, brilliance: -6.0, air: -7.9 }
    },
    { 
      id: 'vocal_focus', 
      name: '🎤 Vocal Focus',
      description: 'Enhanced mid-range for vocal clarity',
      gains: { sub_bass: 0.6, bass: 0.7, low_mid: 1.2, mid: 1.5, high_mid: 1.3, presence: 1.0, brilliance: 0.8, air: 0.6 },
      gainsDb: { sub_bass: -4.4, bass: -3.1, low_mid: 1.6, mid: 3.5, high_mid: 2.3, presence: 0.0, brilliance: -1.9, air: -4.4 }
    },
    { 
      id: 'sparkle', 
      name: '✨ Sparkle',
      description: 'Bright and airy for ambient music',
      gains: { sub_bass: 0.5, bass: 0.6, low_mid: 0.8, mid: 1.0, high_mid: 1.2, presence: 1.4, brilliance: 1.3, air: 1.1 },
      gainsDb: { sub_bass: -6.0, bass: -4.4, low_mid: -1.9, mid: 0.0, high_mid: 1.6, presence: 2.9, brilliance: 2.3, air: 0.8 }
    },
    { 
      id: 'birthday_special', 
      name: '🎂 Birthday Special',
      description: 'Dynamic response optimized for celebration',
      gains: { sub_bass: 1.0, bass: 1.3, low_mid: 1.1, mid: 1.2, high_mid: 1.0, presence: 1.1, brilliance: 1.2, air: 0.9 },
      gainsDb: { sub_bass: 0.0, bass: 2.3, low_mid: 0.8, mid: 1.6, high_mid: 0.0, presence: 0.8, brilliance: 1.6, air: -0.9 }
    }
  ]);

  const [activePreset, setActivePreset] = useState(partyMode ? 'birthday_special' : 'party_balanced');
  const [showRoutingMatrix, setShowRoutingMatrix] = useState(false);
  const [beatSensitivity, setBeatSensitivity] = useState(1.0);
  const [rhythmMode, setRhythmMode] = useState('adaptive'); // adaptive, strict, loose
  
  // Visual parameters available for routing (these would come from the scene config)
  const [availableParameters, setAvailableParameters] = useState([]);
  
  // Real-time frequency analysis
  const analyzerRef = useRef(null);
  const frequencyDataRef = useRef(null);

  // Audio utility functions
  const linearToDb = useCallback((linear) => {
    if (linear <= 0) return -60; // Floor at -60dB for silence
    return Math.max(-60, Math.min(20, 20 * Math.log10(linear)));
  }, []);

  const dbToLinear = useCallback((db) => {
    return Math.pow(10, db / 20);
  }, []);

  const formatDb = useCallback((db) => {
    if (db <= -60) return '-∞';
    return `${db >= 0 ? '+' : ''}${db.toFixed(1)}`;
  }, []);

  // Initialize band gains from preset
  useEffect(() => {
    const preset = mixerPresets.find(p => p.id === activePreset);
    if (preset) {
      setBandGains(preset.gains);
      setBandGainsDb(preset.gainsDb);
      onFrequencyBandConfig?.({
        bands: frequencyBands,
        gains: preset.gains,
        beatSensitivity,
        rhythmMode
      });
    }
  }, [activePreset, beatSensitivity, rhythmMode, onFrequencyBandConfig]);

  // Load available parameters from audio config
  useEffect(() => {
    if (window.module && typeof window.module.get_audio_config === 'function') {
      try {
        const configJson = window.module.get_audio_config();
        const config = JSON.parse(configJson);
        setAvailableParameters(config.map(item => ({
          name: item.name,
          channel: item.channel,
          sensitivity: item.sensitivity || 1.0,
          type: item.type || 'continuous'
        })));
      } catch (error) {
        console.warn('🎛️ Could not load audio parameters:', error);
      }
    }
  }, [isEnabled]);

  // Enhanced frequency analysis function
  const analyzeFrequencyBands = useCallback((analyzer, dataArray) => {
    if (!analyzer || !dataArray) return {};

    analyzer.getByteFrequencyData(dataArray);
    const sampleRate = analyzer.context.sampleRate;
    const nyquist = sampleRate / 2;
    const binCount = analyzer.frequencyBinCount;
    const binWidth = nyquist / binCount;

    const results = {};

    frequencyBands.forEach(band => {
      const startBin = Math.floor((band.min * binCount));
      const endBin = Math.floor((band.max * binCount));
      
      let sum = 0;
      let count = 0;
      
      for (let i = startBin; i <= endBin && i < dataArray.length; i++) {
        sum += dataArray[i];
        count++;
      }
      
      const average = count > 0 ? sum / count : 0;
      const normalized = Math.min(average / 255, 1.0);
      const gained = normalized * (bandGains[band.id] || 1.0);
      
      results[band.id] = {
        raw: normalized,
        gained: gained,
        peak: gained > 0.8,
        frequency: (startBin + endBin) / 2 * binWidth
      };
    });

    return results;
  }, [frequencyBands, bandGains]);

  // Handle preset change
  const handlePresetChange = useCallback((presetId) => {
    setActivePreset(presetId);
    const preset = mixerPresets.find(p => p.id === presetId);
    if (preset) {
      console.log(`🎛️ Applied preset: ${preset.name}`);
    }
  }, [mixerPresets]);

  // Handle individual band gain adjustment
  const handleBandGainChange = useCallback((bandId, gain) => {
    setBandGains(prev => {
      const newGains = { ...prev, [bandId]: gain };
      onFrequencyBandConfig?.({
        bands: frequencyBands,
        gains: newGains,
        beatSensitivity,
        rhythmMode
      });
      return newGains;
    });
  }, [frequencyBands, beatSensitivity, rhythmMode, onFrequencyBandConfig]);

  // Handle individual band gain adjustment in dB
  const handleBandGainDbChange = useCallback((bandId, gainDb) => {
    const linearGain = dbToLinear(gainDb);
    setBandGainsDb(prev => ({ ...prev, [bandId]: gainDb }));
    setBandGains(prev => {
      const newGains = { ...prev, [bandId]: linearGain };
      onFrequencyBandConfig?.({
        bands: frequencyBands,
        gains: newGains,
        beatSensitivity,
        rhythmMode
      });
      return newGains;
    });
  }, [frequencyBands, beatSensitivity, rhythmMode, onFrequencyBandConfig, dbToLinear]);

  // Handle routing matrix changes
  const handleRoutingChange = useCallback((parameterName, bandId, enabled, gain = 1.0) => {
    setRoutingMatrix(prev => {
      const newMatrix = { ...prev };
      if (!newMatrix[parameterName]) {
        newMatrix[parameterName] = {};
      }
      
      if (enabled) {
        newMatrix[parameterName][bandId] = { gain, enabled: true };
      } else {
        delete newMatrix[parameterName][bandId];
      }
      
      onChannelRouting?.(newMatrix);
      return newMatrix;
    });
  }, [onChannelRouting]);

  // Real-time level updates
  useEffect(() => {
    if (!isEnabled || !audioFeatures) return;

    // Update band levels based on current audio features
    const newLevels = {};
    const newLevelsDb = {};

    if (audioFeatures.enhancedFeatures) {
      // Use enhanced 8-band data when available
      frequencyBands.forEach(band => {
        const linearLevel = audioFeatures.enhancedFeatures[band.id] || 0;
        newLevels[band.id] = linearLevel;
        newLevelsDb[band.id] = linearToDb(linearLevel);
      });
    } else {
      // Fallback to basic 3-band data
      newLevels.bass = audioFeatures.bassLevel || 0;
      newLevels.mid = audioFeatures.midLevel || 0;
      newLevels.high = audioFeatures.highLevel || 0;
      newLevelsDb.bass = linearToDb(audioFeatures.bassLevel || 0);
      newLevelsDb.mid = linearToDb(audioFeatures.midLevel || 0);
      newLevelsDb.high = linearToDb(audioFeatures.highLevel || 0);
    }

    setBandLevels(newLevels);
  }, [audioFeatures, isEnabled, frequencyBands, linearToDb]);

  // Export mixer state
  useEffect(() => {
    onMixerStateChange?.({
      bandGains,
      routingMatrix,
      activePreset,
      beatSensitivity,
      rhythmMode,
      frequencyBands
    });
  }, [bandGains, routingMatrix, activePreset, beatSensitivity, rhythmMode, onMixerStateChange]);

  return (
    <div className="audio-mixer">
      <div className="frequency-bands" role="region" aria-label="Audio frequency mixer controls">
          {frequencyBands.map(band => (
            <div key={band.id} className="band-strip">
              <div className="band-header">
                <span className="band-name" style={{ color: band.color }}>
                  {band.name}
                </span>
              </div>
              
              <div className="band-meter">
                <div 
                  className="level-bar"
                  style={{ 
                    height: `${Math.max(0, Math.min(100, ((linearToDb(bandLevels[band.id] || 0) + 60) / 60 * 100)))}%`,
                    backgroundColor: band.color 
                  }}
                />
              </div>
              
              <div className="band-controls">
                <input
                  type="range"
                  min="-20"
                  max="20"
                  step="0.5"
                  value={bandGainsDb[band.id] || 0}
                  onChange={(e) => handleBandGainDbChange(band.id, parseFloat(e.target.value))}
                  className="gain-slider vertical-slider"
                  style={{ accentColor: band.color }}
                  orient="vertical"
                />
                <div className="gain-value">
                  {formatDb(bandGainsDb[band.id] || 0)}
                </div>
              </div>
              
              <div className="band-footer">
                <span className="band-range">{band.range}</span>
              </div>
            </div>
          ))}
      </div>

      {/* Preset Selection */}
      <div className="preset-section">
        <label>Mixer Presets:</label>
        <div className="preset-grid">
          {mixerPresets.map(preset => (
            <button
              key={preset.id}
              className={`preset-button ${activePreset === preset.id ? 'active' : ''}`}
              onClick={() => handlePresetChange(preset.id)}
              title={preset.description}
            >
              {preset.name}
            </button>
          ))}
        </div>
      </div>

      {/* Global Controls */}
      <div className="global-controls">
        <div className="control-group">
          <label>Master Sensitivity: {sensitivity.toFixed(1)}</label>
          <input
            type="range"
            min="0"
            max="3"
            step="0.1"
            value={sensitivity}
            onChange={(e) => setSensitivity(parseFloat(e.target.value))}
            className="sensitivity-slider"
          />
        </div>
        
        <div className="control-group">
          <label>Beat Sensitivity: {beatSensitivity.toFixed(1)}</label>
          <input
            type="range"
            min="0.1"
            max="2.0"
            step="0.1"
            value={beatSensitivity}
            onChange={(e) => setBeatSensitivity(parseFloat(e.target.value))}
            className="beat-sensitivity-slider"
          />
        </div>

        <div className="control-group">
          <label>Rhythm Mode:</label>
          <select 
            value={rhythmMode} 
            onChange={(e) => setRhythmMode(e.target.value)}
            className="rhythm-mode-select"
          >
            <option value="adaptive">🎵 Adaptive</option>
            <option value="strict">🎯 Strict</option>
            <option value="loose">🌊 Loose</option>
          </select>
        </div>
      </div>
    </div>
  );
};

export default AudioMixer; 