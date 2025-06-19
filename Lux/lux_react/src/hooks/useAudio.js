import { useState, useRef, useCallback, useEffect } from 'react';

// Performance timing helper
const getTime = () => (typeof performance !== 'undefined' && performance.now) ? performance.now() : Date.now();

// Optimized Simple Harmonic Oscillator with analytical solutions
class SimpleSHO {
  constructor(mass = 1.0, damping = 0.1, stiffness = 10.0) {
    this.mass = mass;
    this.damping = damping;
    this.stiffness = stiffness;

    this.position = 0;
    this.velocity = 0;
    this.target = 0;

    // Pre-calculate mathematical constants for performance
    this.dampingRatio = this.damping / (2 * Math.sqrt(this.mass * this.stiffness));
    this.naturalFreq = Math.sqrt(this.stiffness / this.mass);
    this.dampedFreq = this.naturalFreq * Math.sqrt(Math.max(0, 1 - this.dampingRatio * this.dampingRatio));
    this.isCriticallyDamped = this.dampingRatio >= 1.0;
  }

  setTarget(target) {
    this.target = target;
  }

  update(deltaTime = 0.016667) {
    if (deltaTime <= 0) return this.position;

    const displacement = this.position - this.target;

    if (this.isCriticallyDamped) {
      // Analytical solution for critically damped case - no oscillation
      const exp_term = Math.exp(-this.naturalFreq * deltaTime);
      const vel_term = (this.velocity + this.naturalFreq * displacement) * deltaTime;

      this.position = this.target + (displacement + vel_term) * exp_term;
      this.velocity = (this.velocity - this.naturalFreq * vel_term) * exp_term;
    } else {
      // Analytical solution for under-damped case - smooth oscillation
      const exp_term = Math.exp(-this.dampingRatio * this.naturalFreq * deltaTime);
      const sin_term = Math.sin(this.dampedFreq * deltaTime);
      const cos_term = Math.cos(this.dampedFreq * deltaTime);

      const A = displacement;
      const B = (this.velocity + this.dampingRatio * this.naturalFreq * displacement) / this.dampedFreq;

      this.position = this.target + exp_term * (A * cos_term + B * sin_term);
      this.velocity = exp_term * (-this.dampingRatio * this.naturalFreq * (A * cos_term + B * sin_term) +
        this.dampedFreq * (-A * sin_term + B * cos_term));
    }

    return this.position;
  }

  setParameters(mass, damping, stiffness) {
    this.mass = mass;
    this.damping = damping;
    this.stiffness = stiffness;

    // Recalculate constants
    this.dampingRatio = this.damping / (2 * Math.sqrt(this.mass * this.stiffness));
    this.naturalFreq = Math.sqrt(this.stiffness / this.mass);
    this.dampedFreq = this.naturalFreq * Math.sqrt(Math.max(0, 1 - this.dampingRatio * this.dampingRatio));
    this.isCriticallyDamped = this.dampingRatio >= 1.0;
  }

  reset(position = 0) {
    this.position = position;
    this.velocity = 0;
    this.target = position;
  }
}

const useAudio = () => {
  const [isEnabled, setIsEnabled] = useState(false);
  const [hasPermission, setHasPermission] = useState(null);
  const [audioFeatures, setAudioFeatures] = useState({
    volume: 0,
    bassLevel: 0,
    midLevel: 0,
    highLevel: 0,
    beatDetected: false,
    dominantFrequency: 0
  });
  const [sensitivity, setSensitivity] = useState(0.5);
  const [performance, setPerformance] = useState({
    fps: 0,
    avgProcessingTime: 0,
    droppedFrames: 0
  });

  // Audio processing refs
  const audioContextRef = useRef(null);
  const microphoneRef = useRef(null);
  const analyzerRef = useRef(null);
  const dataArrayRef = useRef(null);
  const sensitivityRef = useRef(0.5);
  const isEnabledRef = useRef(false);
  
  // Configuration-driven audio mappings (loaded from scene JSON)
  const audioConfigRef = useRef([]);
  const oscillatorsRef = useRef({});
  const manualValuesRef = useRef({});

  // Pre-allocated state objects for zero-allocation hot paths
  const audioStateRef = useRef({
    lastVolume: 0,
    volumeHistory: new Array(4).fill(0),
    beatCooldown: 0,
    continuousMotionPhase: 0,
    lastAudioUpdate: 0,
    lastBackendUpdate: 0,
    frameCount: 0,
    processingTimes: [],
    // FPS tracking
    fpsFrameCount: 0,
    lastFpsLog: 0,
    fpsStartTime: 0
  });

  // Performance constants
  const AUDIO_UPDATE_INTERVAL = 100;    // 10fps for audio processing 
  const BACKEND_UPDATE_INTERVAL = 200;  // 5fps for backend calls  
  const PERFORMANCE_LOG_INTERVAL = 5000; // 5 seconds 

  const manualValuesRestoredRef = useRef(false);
  const lastAudioDetectedRef = useRef(0);

  // Load audio configuration from scene (via WebAssembly)
  const loadAudioConfig = useCallback(() => {
    try {
      if (!window.module || typeof window.module.get_audio_config !== 'function') {
        console.warn('🎵 No audio config available from WebAssembly');
        return;
      }

      const configJson = window.module.get_audio_config();
      const config = JSON.parse(configJson);
      
      console.log('🎵 📋 LOADED AUDIO CONFIG:', config);
      audioConfigRef.current = config;

      // Initialize oscillators for each configured parameter
      const newOscillators = {};
      const newManualValues = {};

      config.forEach(mapping => {
        const { name, channel, sensitivity, damping = 0.05, stiffness = 3.0 } = mapping;
        
        // Create SHO with parameter-specific tuning
        newOscillators[name] = new SimpleSHO(1.0, damping, stiffness);
        
        // Get current manual value
        try {
          const currentValue = window.module.get_slider_value 
            ? window.module.get_slider_value(name) 
            : 0;
          newManualValues[name] = currentValue;
          newOscillators[name].reset(currentValue);
        } catch (error) {
          newManualValues[name] = 0;
          newOscillators[name].reset(0);
        }
      });

      oscillatorsRef.current = newOscillators;
      manualValuesRef.current = newManualValues;

      console.log('🎵 ✅ Initialized oscillators for:', Object.keys(newOscillators));
      
    } catch (error) {
      console.warn('🎵 Error loading audio config:', error);
      audioConfigRef.current = [];
    }
  }, []);

  // Capture current manual values for all configured parameters
  const captureManualValues = useCallback(() => {
    try {
      const config = audioConfigRef.current;
      if (config.length === 0) return;

      const manualValues = {};

      config.forEach(mapping => {
        const { name } = mapping;
        try {
          const currentValue = window.module && window.module.get_slider_value
            ? window.module.get_slider_value(name)
            : 0;
          manualValues[name] = !isNaN(currentValue) ? currentValue : 0;
        } catch (error) {
          manualValues[name] = 0;
        }
      });

      console.log('📊 CAPTURED MANUAL VALUES:', manualValues);
      manualValuesRef.current = manualValues;

      // Reset oscillators to current manual positions
      Object.keys(manualValues).forEach(name => {
        if (oscillatorsRef.current[name]) {
          oscillatorsRef.current[name].reset(manualValues[name]);
        }
      });
    } catch (error) {
      console.warn('🎵 Error capturing manual values:', error);
    }
  }, []);

  // Update manual baseline values while audio is enabled
  const updateManualBaselines = useCallback(() => {
    if (!isEnabledRef.current) return;

    const config = audioConfigRef.current;
    if (config.length === 0) return;

    const currentManualValues = { ...manualValuesRef.current };
    let hasChanges = false;

    config.forEach(mapping => {
      const { name } = mapping;
      try {
        const currentValue = window.module && window.module.get_slider_value
          ? window.module.get_slider_value(name)
          : currentManualValues[name] || 0;
        
        if (!isNaN(currentValue) && currentValue !== currentManualValues[name]) {
          currentManualValues[name] = currentValue;
          hasChanges = true;
        }
      } catch (error) {
        // Keep existing value
      }
    });

    if (hasChanges) {
      manualValuesRef.current = currentManualValues;
    }
  }, []);

  // Return parameters to their manual values with gentle motion
  const returnToManualValues = useCallback(() => {
    const manualValues = manualValuesRef.current;
    const currentTime = getTime();

    Object.keys(manualValues).forEach(name => {
      if (oscillatorsRef.current[name]) {
        const oscillator = oscillatorsRef.current[name];
        const baseValue = manualValues[name];
        
        // Add subtle continuous motion above baseline
        const gentleMotion = Math.abs(Math.sin(currentTime * 0.0005)) * 0.5;
        const target = Math.max(baseValue, baseValue + gentleMotion);
        
        oscillator.setTarget(target);
      }
    });
  }, []);

  // Update refs when state changes
  useEffect(() => {
    sensitivityRef.current = sensitivity;
  }, [sensitivity]);

  useEffect(() => {
    isEnabledRef.current = isEnabled;

    if (isEnabled) {
      loadAudioConfig();
      captureManualValues();
      manualValuesRestoredRef.current = false;
    } else {
      returnToManualValues();
      manualValuesRestoredRef.current = true;
    }
  }, [isEnabled, loadAudioConfig, captureManualValues, returnToManualValues]);

  // MAIN AUDIO UPDATE FUNCTION - Generic and configuration-driven
  const updateAudioParameters = useCallback((deltaTime) => {
    if (!isEnabledRef.current || !analyzerRef.current || !dataArrayRef.current) {
      return;
    }

    const now = getTime();
    const startTime = now;
    const audioState = audioStateRef.current;
    const currentSensitivity = sensitivityRef.current;
    const config = audioConfigRef.current;

    if (config.length === 0) {
      console.log("🎵 ⚠️ No audio configuration loaded");
      return;
    }

    // FPS TRACKING - Count every frame
    audioState.fpsFrameCount++;
    if (audioState.fpsStartTime === 0) {
      audioState.fpsStartTime = now;
      audioState.lastFpsLog = now;
    }

    // Log FPS every 5 seconds
    if (now - audioState.lastFpsLog >= 5000) {
      const timeElapsed = (now - audioState.fpsStartTime) / 1000;
      const currentFPS = audioState.fpsFrameCount / timeElapsed;

      console.log("📊 AUDIO FPS REPORT:");
      console.log(`   🎯 FPS: ${currentFPS.toFixed(1)} | Config: ${config.length} params`);
      console.log(`   🎵 Mappings: ${config.map(c => `${c.name}→${c.channel}`).join(', ')}`);

      // Reset counters
      audioState.fpsFrameCount = 0;
      audioState.fpsStartTime = now;
      audioState.lastFpsLog = now;
    }

    // Audio processing throttling
    const shouldProcessAudio = now - audioState.lastAudioUpdate >= AUDIO_UPDATE_INTERVAL;
    const shouldUpdateBackend = now - audioState.lastBackendUpdate >= BACKEND_UPDATE_INTERVAL;

    // Always update SHO oscillators for smooth motion
    const oscillatorValues = {};
    const manualValues = manualValuesRef.current;
    
    Object.keys(oscillatorsRef.current).forEach(name => {
      const oscillator = oscillatorsRef.current[name];
      const rawValue = oscillator.update(deltaTime);
      // Never go below manual baseline
      oscillatorValues[name] = Math.max(rawValue, manualValues[name] || 0);
    });

    // Process audio data if throttle allows
    if (shouldProcessAudio) {
      try {
        const dataArray = dataArrayRef.current;
        analyzerRef.current.getByteFrequencyData(dataArray);

        const dataLen = dataArray.length;
        const bassEnd = dataLen >> 3;     // 12.5% for bass
        const midEnd = dataLen >> 1;      // 50% for mid

        let bassSum = 0, midSum = 0, highSum = 0, totalSum = 0;

        // Single-pass frequency analysis with 4x sampling optimization
        for (let i = 1; i < dataLen; i += 4) {
          const val = dataArray[i];
          totalSum += val;

          if (i < bassEnd) bassSum += val;
          else if (i < midEnd) midSum += val;
          else highSum += val;
        }

        // Fast normalization
        const quarterLen = dataLen >> 2;
        const bassCount = bassEnd >> 2;
        const midCount = (midEnd - bassEnd) >> 2;
        const highCount = quarterLen - bassCount - midCount;

        const norm = 64;
        const volume = Math.min(totalSum / (quarterLen * norm), 1);
        const bassLevel = bassCount > 0 ? Math.min(bassSum / (bassCount * norm), 1) : 0;
        const midLevel = midCount > 0 ? Math.min(midSum / (midCount * norm), 1) : 0;
        const highLevel = highCount > 0 ? Math.min(highSum / (highCount * norm), 1) : 0;

        // Beat detection
        audioState.volumeHistory.shift();
        audioState.volumeHistory.push(volume);

        const avgVolume = audioState.volumeHistory.reduce((a, b) => a + b) / audioState.volumeHistory.length;
        const volumeIncrease = volume - avgVolume;
        const beatThreshold = 0.08 + (currentSensitivity * 0.12);

        let beatDetected = false;
        if (audioState.beatCooldown <= 0 && volumeIncrease > beatThreshold && volume > 0.05) {
          beatDetected = true;
          audioState.beatCooldown = 8;
        } else {
          audioState.beatCooldown = Math.max(0, audioState.beatCooldown - 1);
        }

        // Update audio features
        setAudioFeatures(prev => ({
          ...prev,
          volume,
          bassLevel,
          midLevel,
          highLevel,
          beatDetected,
          dominantFrequency: 0
        }));

        // Audio processing logic - CONFIGURATION DRIVEN
        if (currentSensitivity > 0) {
          const volumeThreshold = 0.02;

          if (volume > volumeThreshold) {
            manualValuesRestoredRef.current = false;
            lastAudioDetectedRef.current = now;
            updateManualBaselines();

            const manualValues = manualValuesRef.current;
            const audioChannels = { volume, bass: bassLevel, mid: midLevel, high: highLevel };
            const timePhase = now * 0.001;

            // Process each configured parameter
            config.forEach(mapping => {
              const { name, channel, sensitivity: paramSensitivity, offset = 0 } = mapping;
              const channelValue = audioChannels[channel] || 0;
              
              if (channelValue > 0 && oscillatorsRef.current[name]) {
                const influence = Math.pow(channelValue, 0.7) * currentSensitivity * paramSensitivity;
                const baseValue = manualValues[name] || 0;
                
                // Add temporal modulation for organic motion
                const temporalMod = Math.abs(Math.sin(timePhase * 0.6)) * influence * 0.3;
                const beatBonus = beatDetected && channel === 'high' ? 2 * currentSensitivity : 0;
                
                const target = Math.max(baseValue, baseValue + offset + influence + temporalMod + beatBonus);
                oscillatorsRef.current[name].setTarget(target);
              }
            });

            // Log audio processing (reduced frequency)
            if (!audioState.audioLogCount) audioState.audioLogCount = 0;
            audioState.audioLogCount++;
            if (audioState.audioLogCount % 20 === 0) {
              console.log("🎵 AUDIO → TARGETS:");
              console.log(`   📊 Channels: vol=${volume.toFixed(3)}, bass=${bassLevel.toFixed(3)}, mid=${midLevel.toFixed(3)}, high=${highLevel.toFixed(3)}`);
              config.forEach(mapping => {
                const { name, channel } = mapping;
                const currentTarget = oscillatorsRef.current[name]?.target || 0;
                const baseValue = manualValues[name] || 0;
                console.log(`   🎯 ${name}: ${baseValue.toFixed(2)} → ${currentTarget.toFixed(2)} (${channel})`);
              });
            }

          } else {
            // No significant audio - return to manual values
            const timeSinceLastAudio = now - lastAudioDetectedRef.current;
            if (!manualValuesRestoredRef.current && timeSinceLastAudio > 200) {
              returnToManualValues();
              manualValuesRestoredRef.current = true;
            } else if (manualValuesRestoredRef.current) {
              updateManualBaselines();
              returnToManualValues();
            }
          }
        } else {
          returnToManualValues();
        }

        audioState.lastAudioUpdate = now;

      } catch (error) {
        console.warn('Audio processing error:', error);
      }
    }

    // Update backend with throttling
    if (shouldUpdateBackend) {
      Object.keys(oscillatorValues).forEach(name => {
        const boundedValue = getBoundedValue(name, oscillatorValues[name]);
        
        if (window.module && window.module.set_slider_value) {
          window.module.set_slider_value(name, boundedValue);
        }
      });

      audioState.lastBackendUpdate = now;
    }

    // Performance monitoring
    const processingTime = getTime() - startTime;
    audioState.processingTimes.push(processingTime);
    if (audioState.processingTimes.length > 30) {
      audioState.processingTimes.shift();
    }

    audioState.frameCount++;
    if (now - audioState.lastPerformanceLog >= PERFORMANCE_LOG_INTERVAL) {
      const avgTime = audioState.processingTimes.reduce((a, b) => a + b, 0) / audioState.processingTimes.length;
      const fps = audioState.frameCount;

      console.log(`🎵 PERFORMANCE: ${fps}/60 fps, ${avgTime.toFixed(2)}ms avg`);

      setPerformance({
        fps,
        avgProcessingTime: avgTime,
        droppedFrames: fps < 55 ? 1 : 0
      });

      audioState.frameCount = 0;
      audioState.lastPerformanceLog = now;
    }
  }, [updateManualBaselines, returnToManualValues]);

  // Helper function for parameter bounds (generic)
  const getBoundedValue = useCallback((name, value) => {
    // Use WebAssembly to get parameter bounds if available
    try {
      if (window.module && window.module.get_slider_bounds) {
        const bounds = window.module.get_slider_bounds(name);
        const { min, max, step } = JSON.parse(bounds);
        const clampedValue = Math.min(max, Math.max(min, value));
        return step ? Math.round(clampedValue / step) * step : clampedValue;
      }
    } catch (error) {
      // Fallback to generic bounds
    }
    
    // Generic fallback bounds
    return Math.round(Math.min(20, Math.max(-20, value)) * 10) / 10;
  }, []);

  const startAudio = useCallback(async () => {
    console.log('🎵 Starting configuration-driven audio system...');
    try {
      const stream = await navigator.mediaDevices.getUserMedia({
        audio: {
          channelCount: 1,
          sampleRate: 44100,
          echoCancellation: false,
          noiseSuppression: false,
          autoGainControl: false
        }
      });

      const audioContext = new (window.AudioContext || window.webkitAudioContext)();
      const microphone = audioContext.createMediaStreamSource(stream);
      const analyzer = audioContext.createAnalyser();

      analyzer.fftSize = 1024;
      analyzer.smoothingTimeConstant = 0.3;
      analyzer.minDecibels = -90;
      analyzer.maxDecibels = -10;

      microphone.connect(analyzer);
      const dataArray = new Uint8Array(analyzer.frequencyBinCount);

      audioContextRef.current = audioContext;
      microphoneRef.current = microphone;
      analyzerRef.current = analyzer;
      dataArrayRef.current = dataArray;

      setHasPermission(true);
      setIsEnabled(true);

      console.log('🎵 ==========================================');
      console.log('🎵 CONFIGURATION-DRIVEN AUDIO ENABLED');
      console.log('🎵 Type-erased harness integration');
      console.log('🎵 Scene-agnostic architecture');
      console.log('🎵 ==========================================');

    } catch (error) {
      console.error('🎵 Error starting audio:', error);
      setHasPermission(false);
    }
  }, []);

  const stopAudio = useCallback(() => {
    console.log('🎵 Stopping configuration-driven audio system');

    if (microphoneRef.current && microphoneRef.current.mediaStream) {
      microphoneRef.current.mediaStream.getTracks().forEach(track => track.stop());
    }

    if (audioContextRef.current && audioContextRef.current.state !== 'closed') {
      audioContextRef.current.close();
    }

    audioContextRef.current = null;
    microphoneRef.current = null;
    analyzerRef.current = null;
    dataArrayRef.current = null;

    setIsEnabled(false);
    setAudioFeatures({
      volume: 0,
      bassLevel: 0,
      midLevel: 0,
      highLevel: 0,
      beatDetected: false,
      dominantFrequency: 0
    });

    // Reset audio state
    audioStateRef.current = {
      lastVolume: 0,
      volumeHistory: new Array(4).fill(0),
      beatCooldown: 0,
      continuousMotionPhase: 0,
      lastAudioUpdate: 0,
      lastBackendUpdate: 0,
      frameCount: 0,
      processingTimes: []
    };
  }, []);

  const toggleAudio = useCallback(() => {
    if (isEnabled) {
      stopAudio();
    } else {
      startAudio();
    }
  }, [isEnabled, startAudio, stopAudio]);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      stopAudio();
    };
  }, [stopAudio]);

  return {
    isEnabled,
    hasPermission,
    audioFeatures,
    sensitivity,
    setSensitivity,
    performance,
    toggleAudio,
    startAudio,
    stopAudio,
    updateAudioParameters // Export for main animation loop integration
  };
};

export default useAudio;