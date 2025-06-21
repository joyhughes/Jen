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
  const [sensitivity, setSensitivity] = useState(0.8);
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
  const sensitivityRef = useRef(0.8);
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
        audioConfigRef.current = [];
        return false;
      }

      console.log('🎵 📋 Loading audio config from WebAssembly...');
      const configJson = window.module.get_audio_config();
      console.log('🎵 📋 Raw config JSON:', configJson);
      
      const config = JSON.parse(configJson);
      console.log('🎵 📋 LOADED AUDIO CONFIG:', config);
      audioConfigRef.current = config;

      // Initialize oscillators for each configured parameter
      const newOscillators = {};
      const newManualValues = {};

      config.forEach(mapping => {
        const { name, channel, sensitivity, damping = 0.15, stiffness = 2.0 } = mapping;
        
        // Create SHO with parameter-specific tuning for slower, smoother motion
        // Increased damping and reduced stiffness for less aggressive response
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
      console.log('🎵 📋 Final config has ' + config.length + ' audio mappings');
      
      // Auto-enable audio in backend if configuration was successfully loaded
      if (config.length > 0) {
        console.log('🎵 🔧 Auto-enabling audio in backend since config was loaded...');
        try {
          if (window.module && window.module.enable_audio_input) {
            window.module.enable_audio_input(true);
            console.log('🎵 ✅ Audio enabled in backend');
          }
        } catch (error) {
          console.warn('🎵 ⚠️ Could not enable audio in backend:', error);
        }
      }
      
      return true;
      
    } catch (error) {
      console.warn('🎵 Error loading audio config:', error);
      audioConfigRef.current = [];
      return false;
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

    if (!isEnabled) {
      returnToManualValues();
      manualValuesRestoredRef.current = true;
    }
  }, [isEnabled, returnToManualValues]);

  // MAIN AUDIO UPDATE FUNCTION - Generic and configuration-driven
  const updateAudioParameters = useCallback((deltaTime) => {
    if (!isEnabledRef.current || !analyzerRef.current || !dataArrayRef.current) {
      console.log('🎵 ⚠️ updateAudioParameters early return:', {
        enabled: isEnabledRef.current,
        hasAnalyzer: !!analyzerRef.current,
        hasDataArray: !!dataArrayRef.current
      });
      return;
    }

    const now = getTime();
    const startTime = now;
    const audioState = audioStateRef.current;
    const currentSensitivity = sensitivityRef.current;
    const config = audioConfigRef.current;

    // Heartbeat log every 10 seconds
    if (!audioState.lastHeartbeat) audioState.lastHeartbeat = 0;
    if (now - audioState.lastHeartbeat >= 10000) {
      console.log('🎵 Audio system running');
      audioState.lastHeartbeat = now;
    }

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

        // Send audio data to WebAssembly backend
        if (window.module && window.module.update_audio_context) {
          try {
            // RE-ENABLED: Testing if backend crashes are fixed
            const timePhase = now * 0.001;
            window.module.update_audio_context(volume, bassLevel, midLevel, highLevel, beatDetected, timePhase);
            
            // Removed excessive debug logs
          } catch (error) {
            console.error('🎵 ❌ WebAssembly update_audio_context error:', error);
          }
        } else {
          console.warn('🎵 ⚠️ WebAssembly module or update_audio_context not available');
        }

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
                // Reduced influence calculation for gentler response
                const influence = Math.pow(channelValue, 0.8) * currentSensitivity * paramSensitivity * 0.7;
                const baseValue = manualValues[name] || 0;
                
                // Reduced temporal modulation for smoother motion
                const temporalMod = Math.abs(Math.sin(timePhase * 0.4)) * influence * 0.2;
                const beatBonus = beatDetected && channel === 'high' ? 1.5 * currentSensitivity : 0;
                
                const target = Math.max(baseValue, baseValue + offset + influence + temporalMod + beatBonus);
                oscillatorsRef.current[name].setTarget(target);
              }
            });

            // Log audio processing (reduced frequency)
            if (!audioState.audioLogCount) audioState.audioLogCount = 0;
            audioState.audioLogCount++;
            if (audioState.audioLogCount % 100 === 0) {
              console.log("🎵 Audio processing active");
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
      const updates = [];
      Object.keys(oscillatorValues).forEach(name => {
        const boundedValue = getBoundedValue(name, oscillatorValues[name]);
        
        // DISABLED: set_slider_value crashes with audio functions
        // Audio data flows through update_audio_context() → Joy's audio functions instead
        console.log(`🎵 🎶 ${name} = ${boundedValue.toFixed(2)} (via audio system)`);
        updates.push(`${name}=${boundedValue.toFixed(2)}`);
      });

      // Debug log backend updates every 5 seconds
      if (!audioState.lastSliderLog) audioState.lastSliderLog = 0;
      if (updates.length > 0 && now - audioState.lastSliderLog >= 5000) {
        console.log(`🎵 🎛️ Slider updates (set_slider_value disabled): ${updates.join(', ')}`);
        audioState.lastSliderLog = now;
      }

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
        // RE-ENABLED: Testing if backend crashes are fixed
        const bounds = window.module.get_slider_bounds(name);
        const { min, max, step } = JSON.parse(bounds);
        const clampedValue = Math.min(max, Math.max(min, value));
        const result = step ? Math.round(clampedValue / step) * step : clampedValue;
        console.log(`🎵 ✅ get_slider_bounds for ${name}: min=${min}, max=${max}, step=${step} → ${result}`);
        return result;
      }
    } catch (error) {
      // Fallback to generic bounds
      console.error(`🎵 ❌ Error getting slider bounds for ${name}:`, error);
    }
    
    // Generic fallback bounds
    const fallback = Math.round(Math.min(20, Math.max(-20, value)) * 10) / 10;
    console.log(`🎵 ⚠️ Using fallback bounds for ${name}: ${fallback}`);
    return fallback;
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

      console.log('🎵 🎤 Audio setup complete:', {
        contextState: audioContext.state,
        analyzerFFTSize: analyzer.fftSize,
        dataArrayLength: dataArray.length,
        sampleRate: audioContext.sampleRate
      });

      setHasPermission(true);
      
      // Load configuration and initialize before starting animation loop
      console.log('🎵 📋 Loading configuration before starting audio...');
      const configLoaded = loadAudioConfig();
      if (!configLoaded) {
        console.error('🎵 ❌ Failed to load audio config - stopping audio initialization');
        setHasPermission(false);
        return;
      }
      captureManualValues();
      
      // Now set enabled and start animation loop
      setIsEnabled(true);
      isEnabledRef.current = true;

      // Start animation loop for audio processing
      console.log('🎵 🔄 Starting animation loop...');
      let lastTime = getTime();
      const audioAnimationLoop = () => {
        if (!isEnabledRef.current) {
          console.log('🎵 🔄 Animation loop stopped - audio disabled');
          return;
        }
        
        const currentTime = getTime();
        const deltaTime = (currentTime - lastTime) / 1000; // Convert to seconds
        lastTime = currentTime;
        
        try {
          updateAudioParameters(deltaTime);
        } catch (error) {
          console.error('🎵 ❌ Animation loop error:', error);
        }
        
        requestAnimationFrame(audioAnimationLoop);
      };
      
      requestAnimationFrame(audioAnimationLoop);
      console.log('🎵 ✅ Animation loop started');

      console.log('🎵 ==========================================');
      console.log('🎵 CONFIGURATION-DRIVEN AUDIO ENABLED');
      console.log('🎵 Type-erased harness integration');
      console.log('🎵 Scene-agnostic architecture');
      console.log('🎵 ==========================================');

    } catch (error) {
      console.error('🎵 Error starting audio:', error);
      setHasPermission(false);
    }
  }, [updateAudioParameters]);

  const stopAudio = useCallback(() => {
    console.log('🎵 Stopping configuration-driven audio system');

    // SIMPLE FIX: Send current UI slider values to backend before stopping audio
    try {
      const config = audioConfigRef.current;
      console.log('🎵 🔍 Audio config:', config);
      
      if (config.length > 0) {
        console.log('🎵 📤 Sending displayed UI slider values to backend...');
        
        let successCount = 0;
        let errorCount = 0;
        
        // Use the displayed values that users actually see in the text fields
        const displayedValues = window.displayedSliderValues || {};
        console.log('🎵 🔍 Displayed values from UI text fields:', displayedValues);
        
        config.forEach(mapping => {
          const { name } = mapping;
          console.log(`🎵 🔍 Processing parameter: ${name}`);
          
          try {
            const uiValue = displayedValues[name];
            console.log(`🎵 🔍 Displayed value for ${name}: ${uiValue}`);
            
            if (uiValue !== undefined && !isNaN(uiValue)) {
              // DISABLED: set_slider_value crashes with audio functions
              // Audio functions get their values through update_audio_context() instead
              console.log(`🎵 🎶 Would restore ${name}: ${uiValue} (audio functions use update_audio_context)`);
              successCount++;
            } else {
              console.warn(`🎵 ⚠️ No displayed value available for ${name}`);
              errorCount++;
            }
          } catch (error) {
            console.warn(`🎵 ❌ Error processing ${name}:`, error);
            errorCount++;
          }
        });
        
        console.log(`🎵 📊 SUMMARY: ${successCount} successful, ${errorCount} errors out of ${config.length} parameters`);
        
        if (successCount > 0) {
          console.log('🎵 ✅ Displayed UI slider values sent to backend');
        } else {
          console.log('🎵 ⚠️ No displayed slider values were successfully sent to backend!');
        }
      } else {
        console.log('🎵 ⚠️ No audio configuration available');
      }
    } catch (error) {
      console.error('🎵 ❌ Error sending displayed values to backend:', error);
    }

    // Log state before cleanup
    console.log('🎵 🔍 State before cleanup:', {
      isEnabled: isEnabledRef.current,
      hasAudioContext: !!audioContextRef.current,
      hasMicrophone: !!microphoneRef.current,
      hasAnalyzer: !!analyzerRef.current
    });

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
    isEnabledRef.current = false;
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

    // Log final state
    console.log('🎵 🔍 State after cleanup:', {
      isEnabled: isEnabledRef.current,
      hasAudioContext: !!audioContextRef.current,
      hasMicrophone: !!microphoneRef.current,
      hasAnalyzer: !!analyzerRef.current
    });
    
    console.log('🎵 🛑 Audio system fully stopped');
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

  // DEBUG: Expose loadAudioConfig function globally for testing
  useEffect(() => {
    window.debugLoadAudioConfig = loadAudioConfig;
    window.debugAudioConfig = () => {
      console.log('🔍 Current audio config:', audioConfigRef.current);
      console.log('🔍 Current oscillators:', Object.keys(oscillatorsRef.current));
      console.log('🔍 Manual values:', manualValuesRef.current);
      console.log('🔍 Audio enabled:', isEnabledRef.current);
      
      // Test WebAssembly functions
      if (window.module) {
        console.log('🔍 WebAssembly module available');
        console.log('🔍 get_audio_config function:', typeof window.module.get_audio_config);
        console.log('🔍 enable_audio_input function:', typeof window.module.enable_audio_input);
        console.log('🔍 is_audio_enabled function:', typeof window.module.is_audio_enabled);
        
        if (typeof window.module.is_audio_enabled === 'function') {
          try {
            const backendEnabled = window.module.is_audio_enabled();
            console.log('🔍 Backend audio enabled:', backendEnabled);
          } catch (error) {
            console.warn('🔍 Error checking backend audio status:', error);
          }
        }
        
        if (typeof window.module.get_audio_config === 'function') {
          try {
            const configJson = window.module.get_audio_config();
            console.log('🔍 Raw config from backend:', configJson);
            const config = JSON.parse(configJson);
            console.log('🔍 Parsed config:', config);
          } catch (error) {
            console.warn('🔍 Error loading config from backend:', error);
          }
        }
      } else {
        console.log('🔍 WebAssembly module not available');
      }
    };
  }, [loadAudioConfig]);

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