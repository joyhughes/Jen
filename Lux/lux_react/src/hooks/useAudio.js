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
  const manualSliderValuesRef = useRef({});

  // Pre-allocated state objects for zero-allocation hot paths
  const audioStateRef = useRef({
    lastVolume: 0,
    volumeHistory: new Array(4).fill(0), // REDUCED: 4 frames instead of 8 for better performance
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

  // Optimized SHO oscillators
  const oscillatorsRef = useRef({
    start_slider: new SimpleSHO(1.0, 0.02, 3.0),     // Ultra-low damping for continuous rotation
    spin_slider: new SimpleSHO(0.8, 0.01, 2.0),      // Frictionless spin
    expand_slider: new SimpleSHO(1.2, 0.05, 4.0),    // Smooth breathing
    phase_slider: new SimpleSHO(1.0, 0.03, 2.5)      // Wave motion
  });

  // Performance constants - REDUCED for better performance
  const AUDIO_UPDATE_INTERVAL = 100;    // 10fps for audio processing 
  const BACKEND_UPDATE_INTERVAL = 200;  // 5fps for backend calls  
  const PERFORMANCE_LOG_INTERVAL = 5000; // 5 seconds 

  const manualValuesRestoredRef = useRef(false);
  const lastAudioDetectedRef = useRef(0);

  // Capture current manual slider values before audio takes over
  const captureManualSliderValues = useCallback(() => {
    try {
      const defaultValues = {
        start_slider: 0,
        spin_slider: 0,
        expand_slider: 0,
        phase_slider: 0
      };

      const sliderNames = ['start_slider', 'spin_slider', 'expand_slider', 'phase_slider'];
      const manualValues = {};

      sliderNames.forEach(name => {
        const slider = document.querySelector(`.jen-slider[data-name="${name}"]`);
        if (slider) {
          const value = parseFloat(slider.getAttribute('data-value'));
          manualValues[name] = !isNaN(value) ? value : defaultValues[name];
        } else {
          try {
            if (window.module && typeof window.module.get_slider_value === 'function') {
              const backendValue = window.module.get_slider_value(name);
              manualValues[name] = !isNaN(backendValue) ? backendValue : defaultValues[name];
            } else {
              manualValues[name] = defaultValues[name];
            }
          } catch (error) {
            manualValues[name] = defaultValues[name];
          }
        }
      });

      console.log('📊 CAPTURED MANUAL VALUES:', manualValues);
      manualSliderValuesRef.current = manualValues;

      // Initialize oscillators to current manual positions
      Object.keys(manualValues).forEach(name => {
        if (oscillatorsRef.current[name]) {
          oscillatorsRef.current[name].reset(manualValues[name]);
        }
      });
    } catch (error) {
      console.warn('🎵 Error capturing manual values:', error);
      const fallbackValues = {
        start_slider: 0,
        spin_slider: 0,
        expand_slider: 0,
        phase_slider: 0
      };
      manualSliderValuesRef.current = fallbackValues;

      Object.keys(fallbackValues).forEach(name => {
        if (oscillatorsRef.current[name]) {
          oscillatorsRef.current[name].reset(fallbackValues[name]);
        }
      });
    }
  }, []);

  // CONTINUOUSLY update manual baseline values while audio is enabled
  const updateManualBaselines = useCallback(() => {
    if (!isEnabledRef.current) return;

    const sliderNames = ['start_slider', 'spin_slider', 'expand_slider', 'phase_slider'];
    const currentManualValues = { ...manualSliderValuesRef.current };
    let hasChanges = false;

    sliderNames.forEach(name => {
      const slider = document.querySelector(`.jen-slider[data-name="${name}"]`);
      if (slider) {
        const currentValue = parseFloat(slider.getAttribute('data-value'));
        if (!isNaN(currentValue) && currentValue !== currentManualValues[name]) {
          currentManualValues[name] = currentValue;
          hasChanges = true;
        }
      }
    });

    if (hasChanges) {
      manualSliderValuesRef.current = currentManualValues;
    }
  }, []);

  // Return sliders to their manual values smoothly using SHO
  const returnToManualValues = useCallback(() => {
    const manualValues = manualSliderValuesRef.current;
    const currentTime = getTime();

    Object.keys(manualValues).forEach(name => {
      if (oscillatorsRef.current[name]) {
        const oscillator = oscillatorsRef.current[name];

        switch (name) {
          case 'start_slider':
            const startTarget = Math.max(manualValues[name],
              manualValues[name] + Math.abs(Math.sin(currentTime * 0.0006)) * 1.5);
            oscillator.setTarget(startTarget);
            break;

          case 'spin_slider':
            const spinTarget = Math.max(manualValues[name],
              manualValues[name] + Math.abs(Math.sin(currentTime * 0.0008)) * 2.5);
            oscillator.setTarget(spinTarget);
            break;

          case 'expand_slider':
            const expandTarget = Math.max(manualValues[name],
              manualValues[name] + Math.abs(Math.sin(currentTime * 0.001)) * 1.8 +
              Math.abs(Math.cos(currentTime * 0.0007)) * 0.8);
            oscillator.setTarget(expandTarget);
            break;

          case 'phase_slider':
            const phaseTarget = Math.max(manualValues[name],
              manualValues[name] + Math.abs(Math.sin(currentTime * 0.0004)) * 3);
            oscillator.setTarget(phaseTarget);
            break;

          default:
            oscillator.setTarget(manualValues[name]);
        }
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
      captureManualSliderValues();
      manualValuesRestoredRef.current = false;
    } else {
      returnToManualValues();
      manualValuesRestoredRef.current = true;
    }
  }, [isEnabled, captureManualSliderValues, returnToManualValues]);

  // MAIN AUDIO UPDATE FUNCTION - Integrated with main animation loop
  const updateAudioParameters = useCallback((deltaTime) => {
    if (!isEnabledRef.current || !analyzerRef.current || !dataArrayRef.current) {
      // Log why we're not processing
      if (!isEnabledRef.current) {
        console.log("🎵 ⚠️ Audio not enabled, skipping update");
      } else if (!analyzerRef.current) {
        console.log("🎵 ⚠️ Analyzer not available, skipping update");
      } else if (!dataArrayRef.current) {
        console.log("🎵 ⚠️ Data array not available, skipping update");
      }
      return;
    }

    const now = getTime();
    const startTime = now;
    const audioState = audioStateRef.current;
    const currentSensitivity = sensitivityRef.current;

    // FPS TRACKING - Count every frame
    audioState.fpsFrameCount++;
    if (audioState.fpsStartTime === 0) {
      audioState.fpsStartTime = now;
      audioState.lastFpsLog = now;
    }

    // Log FPS every 5 seconds
    if (now - audioState.lastFpsLog >= 5000) {
      const timeElapsed = (now - audioState.fpsStartTime) / 1000; // Convert to seconds
      const currentFPS = audioState.fpsFrameCount / timeElapsed;

      console.log("📊 CURRENT FPS REPORT:");
      console.log(`   🎯 Actual FPS: ${currentFPS.toFixed(1)} fps`);
      console.log(`   ⏱️ Time Period: ${timeElapsed.toFixed(1)}s`);
      console.log(`   📈 Frame Count: ${audioState.fpsFrameCount} frames`);
      console.log(`   🎵 Audio Enabled: ${isEnabledRef.current}`);
      console.log(`   🔄 Delta Time: ${deltaTime.toFixed(4)}s (${(1 / deltaTime).toFixed(1)} fps target)`);
      console.log("   ════════════════════════════════════════");

      // Reset counters for next measurement period
      audioState.fpsFrameCount = 0;
      audioState.fpsStartTime = now;
      audioState.lastFpsLog = now;
    }

    // Debug log every 2 seconds for audio monitoring
    if (!audioState.lastDebugLog) audioState.lastDebugLog = 0;
    if (now - audioState.lastDebugLog > 2000) {
      console.log("🎵 ✅ Audio processing active - deltaTime:", deltaTime.toFixed(4), "sensitivity:", currentSensitivity);
      audioState.lastDebugLog = now;
    }

    // Layer 1: Audio processing throttling (30fps)
    const shouldProcessAudio = now - audioState.lastAudioUpdate >= AUDIO_UPDATE_INTERVAL;

    // Layer 2: Backend update throttling (20fps)
    const shouldUpdateBackend = now - audioState.lastBackendUpdate >= BACKEND_UPDATE_INTERVAL;

    // Always update SHO oscillators for smooth motion (60fps) - CONSTRAIN TO MANUAL MINIMUM
    const oscillatorValues = {};
    const manualValues = manualSliderValuesRef.current;
    Object.keys(oscillatorsRef.current).forEach(name => {
      const oscillator = oscillatorsRef.current[name];
      const rawValue = oscillator.update(deltaTime);

      // CRITICAL: Never go below manual baseline values
      oscillatorValues[name] = Math.max(rawValue, manualValues[name] || 0);
    });

    // Process audio data if throttle allows
    if (shouldProcessAudio) {
      try {
        // OPTIMIZED: Direct frequency analysis on main thread
        const dataArray = dataArrayRef.current;
        analyzerRef.current.getByteFrequencyData(dataArray);

        const dataLen = dataArray.length;

        // Bit-shift operations for performance (powers of 2)
        const bassEnd = dataLen >> 3;     // Divide by 8 (12.5% for bass)
        const midEnd = dataLen >> 1;      // Divide by 2 (50% for mid)

        // Stack variables for zero allocation
        let bassSum = 0, midSum = 0, highSum = 0, totalSum = 0;

        // PERFORMANCE: Single-pass frequency analysis with 4x sampling optimization (skip more samples)
        for (let i = 1; i < dataLen; i += 4) { // Skip 3 out of 4 samples for better performance
          const val = dataArray[i];
          totalSum += val;

          if (i < bassEnd) bassSum += val;
          else if (i < midEnd) midSum += val;
          else highSum += val;
        }

        // Fast normalization with bit operations (adjusted for 4x sampling)
        const quarterLen = dataLen >> 2; // Divide by 4 since we sample every 4th element
        const bassCount = bassEnd >> 2;
        const midCount = (midEnd - bassEnd) >> 2;
        const highCount = quarterLen - bassCount - midCount;

        // Optimized normalization (power-of-2 constant for CPU optimization)
        const norm = 64; // Tuned for better sensitivity
        const volume = Math.min(totalSum / (quarterLen * norm), 1);
        const bassLevel = bassCount > 0 ? Math.min(bassSum / (bassCount * norm), 1) : 0;
        const midLevel = midCount > 0 ? Math.min(midSum / (midCount * norm), 1) : 0;
        const highLevel = highCount > 0 ? Math.min(highSum / (highCount * norm), 1) : 0;

        // Advanced beat detection with temporal analysis
        audioState.volumeHistory.shift();
        audioState.volumeHistory.push(volume);

        const avgVolume = audioState.volumeHistory.reduce((a, b) => a + b) / audioState.volumeHistory.length;
        const volumeIncrease = volume - avgVolume;
        const beatThreshold = 0.08 + (currentSensitivity * 0.12);

        let beatDetected = false;
        if (audioState.beatCooldown <= 0 && volumeIncrease > beatThreshold && volume > 0.05) {
          beatDetected = true;
          audioState.beatCooldown = 8; // Prevent rapid fire
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

        // Audio processing logic
        if (currentSensitivity > 0) {
          const volumeThreshold = 0.02;

          if (volume > volumeThreshold) {
            manualValuesRestoredRef.current = false;
            lastAudioDetectedRef.current = now;

            updateManualBaselines();
            const manualValues = manualSliderValuesRef.current;

            // Perceptually-tuned power curves for better audio responsiveness (INCREASED STRENGTH)
            const volumeInfluence = Math.pow(volume, 0.7) * currentSensitivity * 2.0; // 2x stronger
            const bassInfluence = Math.pow(bassLevel, 0.8) * currentSensitivity * 2.5; // 2.5x stronger
            const midInfluence = Math.pow(midLevel, 0.6) * currentSensitivity * 2.2; // 2.2x stronger
            const highInfluence = Math.pow(highLevel, 0.9) * currentSensitivity * 2.8; // 2.8x stronger

            // Temporal modulation for organic motion
            const timePhase = now * 0.001;

            // Calculate audio targets with UNIDIRECTIONAL motion - ALWAYS ABOVE MANUAL VALUES
            const audioTargets = {
              start_slider: Math.max(manualValues.start_slider,
                manualValues.start_slider +
                volumeInfluence * 6 +
                Math.abs(Math.sin(timePhase * 0.8)) * volumeInfluence * 3),

              spin_slider: Math.max(manualValues.spin_slider,
                manualValues.spin_slider +
                highInfluence * 8 +
                (beatDetected ? 5 * currentSensitivity : 0) +
                Math.abs(Math.sin(timePhase * 0.6)) * highInfluence * 2.5),

              expand_slider: Math.max(manualValues.expand_slider,
                manualValues.expand_slider +
                bassInfluence * 6 +
                midInfluence * 4 +
                Math.abs(Math.sin(timePhase * 1.2)) * bassInfluence * 2),

              phase_slider: Math.max(manualValues.phase_slider,
                manualValues.phase_slider +
                midInfluence * 12 +
                Math.abs(Math.sin(timePhase * 0.4)) * midInfluence * 3)
            };

            // 🎵 DETAILED AUDIO LOGGING
            console.log("🎵 AUDIO → SLIDER TARGETS:");
            console.log(`   📊 Raw Audio: vol=${volume.toFixed(3)}, bass=${bassLevel.toFixed(3)}, mid=${midLevel.toFixed(3)}, high=${highLevel.toFixed(3)}, beat=${beatDetected}`);
            console.log(`   🎛️ Influences: vol=${volumeInfluence.toFixed(3)}, bass=${bassInfluence.toFixed(3)}, mid=${midInfluence.toFixed(3)}, high=${highInfluence.toFixed(3)}`);
            console.log(`   📍 Manual Base: start=${manualValues.start_slider.toFixed(2)}, spin=${manualValues.spin_slider.toFixed(2)}, expand=${manualValues.expand_slider.toFixed(2)}, phase=${manualValues.phase_slider.toFixed(2)}`);
            console.log(`   🎯 Audio Targets: start=${audioTargets.start_slider.toFixed(2)}, spin=${audioTargets.spin_slider.toFixed(2)}, expand=${audioTargets.expand_slider.toFixed(2)}, phase=${audioTargets.phase_slider.toFixed(2)}`);
            console.log(`   📈 Deltas: start=+${(audioTargets.start_slider - manualValues.start_slider).toFixed(2)}, spin=+${(audioTargets.spin_slider - manualValues.spin_slider).toFixed(2)}, expand=+${(audioTargets.expand_slider - manualValues.expand_slider).toFixed(2)}, phase=+${(audioTargets.phase_slider - manualValues.phase_slider).toFixed(2)}`);
            console.log("   ────────────────────────────────────────");

            // Set oscillator targets
            Object.keys(audioTargets).forEach(name => {
              if (oscillatorsRef.current[name]) {
                oscillatorsRef.current[name].setTarget(audioTargets[name]);
              }
            });

          } else {
            // No significant audio - continuous motion
            const timeSinceLastAudio = now - lastAudioDetectedRef.current;

            if (!manualValuesRestoredRef.current && timeSinceLastAudio > 200) {
              returnToManualValues();
              manualValuesRestoredRef.current = true;
            } else if (manualValuesRestoredRef.current) {
              updateManualBaselines();
              returnToManualValues(); // Continuous motion targets
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
      const backendValues = {};
      Object.keys(oscillatorValues).forEach(name => {
        const boundedValue = getBoundedValue(name, oscillatorValues[name]);
        backendValues[name] = boundedValue;

        if (window.module && window.module.set_slider_value) {
          window.module.set_slider_value(name, boundedValue);
        }
      });

      // Log final backend values every few updates
      if (!audioState.backendLogCount) audioState.backendLogCount = 0;
      audioState.backendLogCount++;
      if (audioState.backendLogCount % 10 === 0) { // Every 10th backend update
        const manualVals = manualSliderValuesRef.current;
        console.log("🎵 BACKEND VALUES SENT:");
        console.log(`   📍 Manual Baseline: start=${manualVals.start_slider?.toFixed(2)}, spin=${manualVals.spin_slider?.toFixed(2)}, expand=${manualVals.expand_slider?.toFixed(2)}, phase=${manualVals.phase_slider?.toFixed(2)}`);
        console.log(`   🎛️ Final Slider Values: start=${backendValues.start_slider?.toFixed(2)}, spin=${backendValues.spin_slider?.toFixed(2)}, expand=${backendValues.expand_slider?.toFixed(2)}, phase=${backendValues.phase_slider?.toFixed(2)}`);
        console.log(`   ✅ Above Baseline: start=${backendValues.start_slider >= manualVals.start_slider}, spin=${backendValues.spin_slider >= manualVals.spin_slider}, expand=${backendValues.expand_slider >= manualVals.expand_slider}, phase=${backendValues.phase_slider >= manualVals.phase_slider}`);
        console.log("   ════════════════════════════════════════");
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

      console.log(`🎵 OPTIMIZED AUDIO PERFORMANCE:`);
      console.log(`   Processing FPS: ${fps}/60 (${fps < 55 ? '⚠️ DROPPED' : '✅ SMOOTH'})`);
      console.log(`   Avg Time: ${avgTime.toFixed(2)}ms (${(avgTime / 16.67 * 100).toFixed(1)}% of 60fps budget)`);
      console.log(`   CPU Load: ${avgTime > 3 ? '🔥 HIGH' : avgTime > 1.5 ? '⚠️ MEDIUM' : '✅ LOW'}`);

      setPerformance({
        fps,
        avgProcessingTime: avgTime,
        droppedFrames: fps < 55 ? 1 : 0
      });

      audioState.frameCount = 0;
      audioState.lastPerformanceLog = now;
    }
  }, [updateManualBaselines, returnToManualValues]);

  // Helper function for parameter bounds
  const getBoundedValue = useCallback((name, value) => {
    switch (name) {
      case 'start_slider':
      case 'spin_slider':
      case 'expand_slider':
        return Math.round(Math.min(10, Math.max(-10, value)) * 10) / 10;
      case 'phase_slider':
        return Math.round(Math.min(20, Math.max(-20, value)) * 2) / 2;
      default:
        return value;
    }
  }, []);

  const startAudio = useCallback(async () => {
    console.log('🎵 Starting optimized audio system...');
    try {
      const stream = await navigator.mediaDevices.getUserMedia({
        audio: {
          channelCount: 1,
          sampleRate: 44100, // Standard rate for better frequency resolution
          echoCancellation: false,
          noiseSuppression: false,
          autoGainControl: false
        }
      });

      console.log('🎵 Got microphone stream');

      const audioContext = new (window.AudioContext || window.webkitAudioContext)();
      const microphone = audioContext.createMediaStreamSource(stream);
      const analyzer = audioContext.createAnalyser();

      // Optimized analyzer settings
      analyzer.fftSize = 1024;
      analyzer.smoothingTimeConstant = 0.3;
      analyzer.minDecibels = -90;
      analyzer.maxDecibels = -10;

      microphone.connect(analyzer);

      const dataArray = new Uint8Array(analyzer.frequencyBinCount);

      console.log('🎵 Audio context created, FFT size:', analyzer.fftSize, 'Buffer size:', dataArray.length);

      audioContextRef.current = audioContext;
      microphoneRef.current = microphone;
      analyzerRef.current = analyzer;
      dataArrayRef.current = dataArray;

      setHasPermission(true);
      setIsEnabled(true);

      console.log('🎵 ==========================================');
      console.log('🎵 OPTIMIZED AUDIO ENABLED');
      console.log('🎵 Integration: Function export (no RAF)');
      console.log('🎵 Processing: Direct main thread (optimized)');
      console.log('🎵 SHO: Analytical solutions');
      console.log('🎵 ==========================================');

    } catch (error) {
      console.error('🎵 Error starting audio:', error);
      setHasPermission(false);
    }
  }, []);

  const stopAudio = useCallback(() => {
    console.log('🎵 ==========================================');
    console.log('🎵 STOPPING OPTIMIZED AUDIO SYSTEM');
    console.log('🎵 ==========================================');

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
      volumeHistory: new Array(8).fill(0),
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