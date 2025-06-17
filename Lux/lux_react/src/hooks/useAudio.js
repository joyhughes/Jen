import { useState, useRef, useCallback, useEffect } from 'react';

// Performance timing helper
const getTime = () => (typeof performance !== 'undefined' && performance.now) ? performance.now() : Date.now();

// Simple Harmonic Oscillator for parameter modulation
class SimpleHarmonicOscillator {
  constructor(mass = 1.0, damping = 0.1, stiffness = 10.0) {
    this.mass = mass;
    this.damping = damping;
    this.stiffness = stiffness;
    
    this.position = 0;
    this.velocity = 0;
    this.target = 0;
    
    this.dt = 1/60; // 60 FPS
  }
  
  setTarget(target) {
    this.target = target;
  }
  
  update() {
    // F = -kx - cv + driving_force
    const springForce = -this.stiffness * (this.position - this.target);
    const dampingForce = -this.damping * this.velocity;
    const totalForce = springForce + dampingForce;
    
    // a = F/m
    const acceleration = totalForce / this.mass;
    
    // Update velocity and position using Euler integration
    this.velocity += acceleration * this.dt;
    this.position += this.velocity * this.dt;
    
    return this.position;
  }
  
  setParameters(mass, damping, stiffness) {
    this.mass = mass;
    this.damping = damping;
    this.stiffness = stiffness;
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
  const animationFrameRef = useRef(null);
  const sensitivityRef = useRef(0.5);
  const isEnabledRef = useRef(false);
  const manualSliderValuesRef = useRef({});
  
  // SHO Physics oscillators for smooth parameter transitions
  const oscillatorsRef = useRef({
    start_slider: new SimpleHarmonicOscillator(1.0, 0.02, 3.0),     // VERY low damping for continuous rotation
    spin_slider: new SimpleHarmonicOscillator(0.8, 0.01, 2.0),      // Ultra-low damping for frictionless spin
    expand_slider: new SimpleHarmonicOscillator(1.2, 0.05, 4.0),    // Low damping for smooth breathing
    phase_slider: new SimpleHarmonicOscillator(1.0, 0.03, 2.5)      // Low damping for wave motion
  });
  
  const lastFrameTimeRef = useRef(0);
  const frameCountRef = useRef(0);
  const processingTimesRef = useRef([]);
  
  const lastSliderUpdateRef = useRef(0);
  const SLIDER_UPDATE_THROTTLE = 16; // 60fps for smooth animation (16ms)
  
  const manualValuesRestoredRef = useRef(false);
  const lastAudioDetectedRef = useRef(0);

  const getTime = () => {
    try {
      return performance.now();
    } catch {
      return Date.now();
    }
  };

  // Capture current manual slider values before audio takes over
  const captureManualSliderValues = useCallback(() => {
    try {
      // Target the SOURCE SLIDERS that feed the integrators for smooth continuous motion
      const defaultValues = {
        start_slider: 0,      // Source slider that feeds start_integrator
        spin_slider: 0,       // Source slider that feeds spin_integrator  
        expand_slider: 0,     // Source slider that feeds expand_integrator
        phase_slider: 0       // Source slider that feeds phase_integrator
      };
      
      const sliderNames = ['start_slider', 'spin_slider', 'expand_slider', 'phase_slider'];
      const manualValues = {};
      
      sliderNames.forEach(name => {
        // Try to get current value from DOM first
        const slider = document.querySelector(`.jen-slider[data-name="${name}"]`);
        if (slider) {
          const value = parseFloat(slider.getAttribute('data-value'));
          manualValues[name] = !isNaN(value) ? value : defaultValues[name];
        } else {
          // Try to get from WebAssembly backend
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
      // Fallback to defaults
      const fallbackValues = {
        start_slider: 0,
        spin_slider: 0,
        expand_slider: 0,
        phase_slider: 0
      };
      manualSliderValuesRef.current = fallbackValues;
      
      // Initialize oscillators to defaults
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
      // Get current value from DOM
      const slider = document.querySelector(`.jen-slider[data-name="${name}"]`);
      if (slider) {
        const currentValue = parseFloat(slider.getAttribute('data-value'));
        if (!isNaN(currentValue) && currentValue !== currentManualValues[name]) {
          console.log(`🔄 MANUAL BASELINE UPDATE: ${name} changed from ${currentManualValues[name]} to ${currentValue}`);
          currentManualValues[name] = currentValue;
          hasChanges = true;
        }
      }
    });
    
    if (hasChanges) {
      manualSliderValuesRef.current = currentManualValues;
      console.log('📊 UPDATED MANUAL BASELINES:', currentManualValues);
    }
  }, []);
  
  // Return sliders to their manual values smoothly using SHO
  const returnToManualValues = useCallback(() => {
    const manualValues = manualSliderValuesRef.current;
    
    // Instead of static targets, set CONTINUOUS MOTION targets for smooth animation
    Object.keys(manualValues).forEach(name => {
      if (oscillatorsRef.current[name]) {
        const oscillator = oscillatorsRef.current[name];
        const currentTime = getTime();
        
                  switch (name) {
            case 'start_slider':
              // Pre-kaleidoscope rotation - gentle continuous motion (ADDITIVE)
              const startTarget = manualValues[name] + Math.abs(Math.sin(currentTime * 0.0006)) * 1.5; // Always positive addition
              oscillator.setTarget(startTarget);
              break;
              
            case 'spin_slider':
              // Post-kaleidoscope rotation - main smooth spinning (ADDITIVE)
              const spinTarget = manualValues[name] + Math.abs(Math.sin(currentTime * 0.0008)) * 2.5; // Always positive addition
              oscillator.setTarget(spinTarget);
              break;
              
            case 'expand_slider':
              // Breathing expansion motion (ADDITIVE)
              const expandTarget = manualValues[name] + Math.abs(Math.sin(currentTime * 0.001)) * 1.8 + Math.abs(Math.cos(currentTime * 0.0007)) * 0.8; // Always positive breathing
              oscillator.setTarget(expandTarget);
              break;
              
            case 'phase_slider':
              // Wave phase motion for ripple effects (ADDITIVE)
              const phaseTarget = manualValues[name] + Math.abs(Math.sin(currentTime * 0.0004)) * 3; // Always positive wave motion
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

  // Ensure sensitivityRef is initialized on mount
  useEffect(() => {
    sensitivityRef.current = sensitivity;
  }, []); // Run once on mount

  useEffect(() => {
    isEnabledRef.current = isEnabled;
    
    // When audio is enabled, capture current manual slider values
    if (isEnabled) {
      captureManualSliderValues();
      manualValuesRestoredRef.current = false; // Reset flag when enabling
    } else {
      // When audio is disabled, return to manual values smoothly
      returnToManualValues();
      manualValuesRestoredRef.current = true; // Mark as restored
    }
  }, [isEnabled, captureManualSliderValues, returnToManualValues]);

  // SHO-based audio processing with smooth parameter modulation
  const processAudio = useCallback(() => {
    // Check current enabled state from ref (not stale closure)
    const currentlyEnabled = isEnabledRef.current && audioContextRef.current && analyzerRef.current && dataArrayRef.current;
    
    // CRITICAL: Stop immediately if audio is disabled
    if (!currentlyEnabled) {
      console.log('🎵 STOPPING audio processing - enabled:', isEnabledRef.current);
      // Clear the animation frame to completely stop the loop
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current);
        animationFrameRef.current = null;
      }
      return; // Don't continue the loop if disabled
    }

    const now = getTime();
    
    // Run at 60fps for smooth SHO physics
    if (now - lastFrameTimeRef.current < 16) { // 60fps = ~16ms
      animationFrameRef.current = requestAnimationFrame(processAudio);
      return;
    }

    const startTime = getTime();
    
    try {
      // Simple frequency analysis
      analyzerRef.current.getByteFrequencyData(dataArrayRef.current);
      const data = dataArrayRef.current;
      
      // Basic audio features - minimal computation
      const dataLen = data.length;
      const bassEnd = Math.floor(dataLen * 0.1);    // Bass: 0-10% of frequency range (low frequencies ~0-2kHz)
      const midEnd = Math.floor(dataLen * 0.5);     // Mid: 10-50% of frequency range (mid frequencies ~2-11kHz) 
                                                     // High: 50-100% of frequency range (high frequencies ~11-22kHz)
      
      let bassSum = 0, midSum = 0, highSum = 0, totalSum = 0;
      
      for (let i = 0; i < dataLen; i++) {
        const value = data[i];
        if (i < bassEnd) bassSum += value;
        else if (i < midEnd) midSum += value;
        else highSum += value;
        totalSum += value;
      }
      
      const volume = Math.min(totalSum / (dataLen * 255), 1);
      const bassLevel = Math.min(bassSum / (bassEnd * 255), 1);
      const midLevel = Math.min(midSum / ((midEnd - bassEnd) * 255), 1);
      const highLevel = Math.min(highSum / ((dataLen - midEnd) * 255), 1);
      
      // Simple beat detection
      const currentSensitivity = sensitivityRef.current;
      const beatThreshold = 0.3 + (currentSensitivity * 0.4);
      const beatDetected = volume > beatThreshold;
      
      // CRITICAL: Only process if sensitivity > 0
      if (currentSensitivity <= 0) {
        console.log('🎵 Sensitivity is 0, returning to manual values');
        returnToManualValues();
        setAudioFeatures({
          volume,
          bassLevel,
          midLevel, 
          highLevel,
          beatDetected: false, // No beat detection when sensitivity is 0
          dominantFrequency: 0
        });
        
        // Continue running oscillators even when sensitivity is 0 for smooth return
        // But only update backend every SLIDER_UPDATE_THROTTLE ms to prevent visual glitches
        const currentTime = getTime();
        if (currentTime - lastSliderUpdateRef.current >= SLIDER_UPDATE_THROTTLE) {
          lastSliderUpdateRef.current = currentTime;
          
          Object.keys(oscillatorsRef.current).forEach(name => {
            const oscillator = oscillatorsRef.current[name];
            const smoothValue = oscillator.update();
            
            // Ensure values are within proper bounds
            const boundedValue = getBoundedValue(name, smoothValue);
            
            if (window.module && window.module.set_slider_value) {
              window.module.set_slider_value(name, boundedValue);
            }
          });
        }
        
        animationFrameRef.current = requestAnimationFrame(processAudio);
        return;
      }
      
      // Audio influence calculation
      const volumeThreshold = 0.05; // Lower threshold for more responsive audio
      
      if (volume > volumeThreshold) {
        // Reset the manual values restored flag since we have audio
        manualValuesRestoredRef.current = false;
        lastAudioDetectedRef.current = getTime();
        
        // CRITICAL: Update manual baselines continuously to reflect user changes
        updateManualBaselines();
        
        // Get manual values as base
        const manualValues = manualSliderValuesRef.current;
        
        // LOG: Show current manual baseline values
        console.log('🎛️ MANUAL BASELINE VALUES:', {
          start_manual: manualValues.start_slider,
          spin_manual: manualValues.spin_slider,
          expand_manual: manualValues.expand_slider,
          phase_manual: manualValues.phase_slider
        });
        
        // Calculate individual audio contributions
        const audioContributions = {
          start_volume: volume * 8 * currentSensitivity,
          start_breathing: Math.sin(now * 0.001) * Math.abs(volume) * 2 * currentSensitivity,
          spin_high: highLevel * 12 * currentSensitivity,
          spin_beat: beatDetected ? 6 * currentSensitivity : 0,
          spin_base: Math.abs(Math.sin(now * 0.0008)) * 3 * currentSensitivity,
          expand_bass: bassLevel * 6 * currentSensitivity,
          expand_mid: midLevel * 4 * currentSensitivity,
          expand_breathing: Math.abs(Math.sin(now * 0.002)) * 2 * currentSensitivity * volume,
          phase_mid: midLevel * 15 * currentSensitivity,
          phase_wave: Math.abs(Math.sin(now * 0.0005)) * 5 * currentSensitivity
        };
        
        // LOG: Show audio contributions
        console.log('🎵 AUDIO CONTRIBUTIONS:', {
          volume: volume.toFixed(3),
          bassLevel: bassLevel.toFixed(3), 
          midLevel: midLevel.toFixed(3),
          highLevel: highLevel.toFixed(3),
          beatDetected,
          sensitivity: currentSensitivity,
          contributions: {
            start_total: (audioContributions.start_volume + audioContributions.start_breathing).toFixed(2),
            spin_total: (audioContributions.spin_high + audioContributions.spin_beat + audioContributions.spin_base).toFixed(2),
            expand_total: (audioContributions.expand_bass + audioContributions.expand_mid + audioContributions.expand_breathing).toFixed(2),
            phase_total: (audioContributions.phase_mid + audioContributions.phase_wave).toFixed(2)
          }
        });
        
        // Calculate audio targets optimized for SOURCE SLIDERS - PURELY ADDITIVE
        const audioTargets = {
          // start_slider: Source slider that feeds start_integrator - driven by overall volume (ADDITIVE ONLY)
          start_slider: Math.max(manualValues.start_slider, 
            manualValues.start_slider + audioContributions.start_volume + audioContributions.start_breathing
          ),
          
          // spin_slider: Source slider that feeds spin_integrator - driven by high frequencies + beat (ADDITIVE ONLY)
          spin_slider: Math.max(manualValues.spin_slider, 
            manualValues.spin_slider + audioContributions.spin_high + audioContributions.spin_beat + audioContributions.spin_base
          ),
          
          // expand_slider: Source slider that feeds expand_integrator - driven by bass + mid (ADDITIVE ONLY)
          expand_slider: Math.max(manualValues.expand_slider, 
            manualValues.expand_slider + audioContributions.expand_bass + audioContributions.expand_mid + audioContributions.expand_breathing
          ),
          
          // phase_slider: Source slider that feeds phase_integrator - driven by mid frequencies (ADDITIVE ONLY)
          phase_slider: Math.max(manualValues.phase_slider, 
            manualValues.phase_slider + audioContributions.phase_mid + audioContributions.phase_wave
          )
        };
        
        // LOG: Show final calculated targets
        console.log('🎯 FINAL AUDIO TARGETS:', {
          start: `${manualValues.start_slider} + ${(audioContributions.start_volume + audioContributions.start_breathing).toFixed(2)} = ${audioTargets.start_slider.toFixed(2)}`,
          spin: `${manualValues.spin_slider} + ${(audioContributions.spin_high + audioContributions.spin_beat + audioContributions.spin_base).toFixed(2)} = ${audioTargets.spin_slider.toFixed(2)}`,
          expand: `${manualValues.expand_slider} + ${(audioContributions.expand_bass + audioContributions.expand_mid + audioContributions.expand_breathing).toFixed(2)} = ${audioTargets.expand_slider.toFixed(2)}`,
          phase: `${manualValues.phase_slider} + ${(audioContributions.phase_mid + audioContributions.phase_wave).toFixed(2)} = ${audioTargets.phase_slider.toFixed(2)}`
        });

        // Set oscillator targets (SHO will smooth the transitions)
        Object.keys(audioTargets).forEach(name => {
          if (oscillatorsRef.current[name]) {
            oscillatorsRef.current[name].setTarget(audioTargets[name]);
          }
        });
        
        // Remove the old minimal logging
      } else {
        // No significant audio - maintain CONTINUOUS SMOOTH MOTION instead of static return
        const timeSinceLastAudio = getTime() - lastAudioDetectedRef.current;
        
        if (!manualValuesRestoredRef.current && timeSinceLastAudio > 200) { // Faster transition to continuous motion
          console.log('🎵 Audio stopped, switching to continuous motion mode');
          
          // LOG: Show manual values being restored
          const manualValues = manualSliderValuesRef.current;
          console.log('🏠 RESTORING TO MANUAL BASELINE:', manualValues);
          
          returnToManualValues(); // This now sets continuous motion targets
          manualValuesRestoredRef.current = true; // Prevent repeated calls
        } else if (manualValuesRestoredRef.current) {
          // CONTINUOUSLY update motion targets for frictionless animation
          // Also update baselines in case user changed sliders during quiet periods
          updateManualBaselines();
          
          const manualValues = manualSliderValuesRef.current;
          const currentTime = getTime();
          
          // Calculate continuous motion targets
          const continuousTargets = {};
          
          // Update continuous motion targets every frame for smooth motion
          Object.keys(oscillatorsRef.current).forEach(name => {
            const oscillator = oscillatorsRef.current[name];
            
                          switch (name) {
                case 'start_slider':
                  // Pre-kaleidoscope rotation - gentle continuous motion (ADDITIVE)
                  const startTarget = manualValues[name] + Math.abs(Math.sin(currentTime * 0.0006)) * 1.5; // Always positive addition
                  oscillator.setTarget(startTarget);
                  continuousTargets[name] = startTarget;
                  break;
                  
                case 'spin_slider':
                  // Post-kaleidoscope rotation - main smooth spinning (ADDITIVE)
                  const spinTarget = manualValues[name] + Math.abs(Math.sin(currentTime * 0.0008)) * 2.5; // Always positive addition
                  oscillator.setTarget(spinTarget);
                  continuousTargets[name] = spinTarget;
                  break;
                  
                case 'expand_slider':
                  // Breathing expansion motion (ADDITIVE)
                  const expandTarget = manualValues[name] + Math.abs(Math.sin(currentTime * 0.001)) * 1.8 + Math.abs(Math.cos(currentTime * 0.0007)) * 0.8; // Always positive breathing
                  oscillator.setTarget(expandTarget);
                  continuousTargets[name] = expandTarget;
                  break;
                  
                case 'phase_slider':
                  // Wave phase motion for ripple effects (ADDITIVE)
                  const phaseTarget = manualValues[name] + Math.abs(Math.sin(currentTime * 0.0004)) * 3; // Always positive wave motion
                  oscillator.setTarget(phaseTarget);
                  continuousTargets[name] = phaseTarget;
                  break;
            }
          });
          
          // LOG: Show continuous motion targets occasionally
          if (Math.random() < 0.02) { // 2% of frames to avoid spam
            console.log('🌊 CONTINUOUS MOTION TARGETS:', {
              manual_baseline: manualValues,
              continuous_targets: continuousTargets,
                              enhancements: {
                  start: `+${(continuousTargets.start_slider - manualValues.start_slider).toFixed(2)}`,
                  spin: `+${(continuousTargets.spin_slider - manualValues.spin_slider).toFixed(2)}`,
                  expand: `+${(continuousTargets.expand_slider - manualValues.expand_slider).toFixed(2)}`,
                  phase: `+${(continuousTargets.phase_slider - manualValues.phase_slider).toFixed(2)}`
                }
            });
          }
        }
      }
      
      // Update oscillators but THROTTLE backend updates for stability
      const currentTime = getTime();
      if (currentTime - lastSliderUpdateRef.current >= SLIDER_UPDATE_THROTTLE) {
        lastSliderUpdateRef.current = currentTime;
        
        // LOG: Show what oscillators are doing and what gets sent to backend
        const oscillatorStates = {};
        const finalBackendValues = {};
        
        // Update all oscillators and apply smooth values with proper bounds
        Object.keys(oscillatorsRef.current).forEach(name => {
          const oscillator = oscillatorsRef.current[name];
          const smoothValue = oscillator.update();
          
          // Store oscillator state for logging
          oscillatorStates[name] = {
            position: oscillator.position.toFixed(3),
            velocity: oscillator.velocity.toFixed(3),
            target: oscillator.target.toFixed(3)
          };
          
          // Ensure values are within proper bounds and precision
          const boundedValue = getBoundedValue(name, smoothValue);
          finalBackendValues[name] = boundedValue;
          
          // Apply smooth values to backend
          if (window.module && window.module.set_slider_value) {
            window.module.set_slider_value(name, boundedValue);
          }
        });
        
        // LOG: Show complete state every few frames
        if (Math.random() < 0.05) { // 5% of frames to avoid spam
          console.log('🔄 SHO OSCILLATOR STATES:', oscillatorStates);
          console.log('📤 SENT TO BACKEND:', finalBackendValues);
        }
      } else {
        // Still update oscillators for smooth internal state, just don't send to backend
        Object.keys(oscillatorsRef.current).forEach(name => {
          oscillatorsRef.current[name].update();
        });
      }
      
      setAudioFeatures({
        volume,
        bassLevel,
        midLevel, 
        highLevel,
        beatDetected,
        dominantFrequency: 0, // Skip expensive dominant frequency calc
        // Add slider activity indicators
        activeSliders: volume > volumeThreshold ? {
          start: bassLevel > 0.1,
          spin: highLevel > 0.1,
          expand: midLevel > 0.1,
          phase: midLevel > 0.1
        } : {
          start: false,
          spin: false,
          expand: false,
          phase: false
        }
      });
      
    } catch (error) {
      console.warn('Audio processing error:', error);
    }
    
    // Performance monitoring
    const processingTime = getTime() - startTime;
    processingTimesRef.current.push(processingTime);
    if (processingTimesRef.current.length > 30) {
      processingTimesRef.current.shift();
    }
    
    frameCountRef.current++;
    if (now - lastFrameTimeRef.current >= 1000) {
      const fps = frameCountRef.current;
      const avgTime = processingTimesRef.current.reduce((a, b) => a + b, 0) / processingTimesRef.current.length;
      
      setPerformance(prev => ({
        fps,
        avgProcessingTime: avgTime,
        droppedFrames: prev.droppedFrames + (fps < 55 ? 1 : 0) // Higher expectation with 60fps
      }));
      
      frameCountRef.current = 0;
      lastFrameTimeRef.current = now;
    }
    
    // Continue loop for smooth SHO updates even when no audio
    if (currentlyEnabled && isEnabledRef.current) {
      animationFrameRef.current = requestAnimationFrame(processAudio);
    } else {
      console.log('🎵 NOT continuing audio loop - disabled');
    }
  }, [returnToManualValues, updateManualBaselines]);

  // Helper function to ensure parameter values are within proper bounds and precision
  const getBoundedValue = useCallback((name, value) => {
    // Apply proper bounds and precision for each SOURCE SLIDER parameter type
    switch (name) {
      case 'start_slider':
        // Float slider: min=-10, max=10, 1 decimal precision
        return Math.round(Math.min(10, Math.max(-10, value)) * 10) / 10;
      
      case 'spin_slider':
        // Float slider: min=-10, max=10, 1 decimal precision
        return Math.round(Math.min(10, Math.max(-10, value)) * 10) / 10;
      
      case 'expand_slider':
        // Float slider: min=-10, max=10, 1 decimal precision
        return Math.round(Math.min(10, Math.max(-10, value)) * 10) / 10;
      
      case 'phase_slider':
        // Float slider: min=-20, max=20, 0.5 step precision
        return Math.round(Math.min(20, Math.max(-20, value)) * 2) / 2;
      
      default:
        return value;
    }
  }, []);

  const startAudio = useCallback(async () => {
    console.log('🎵 Starting audio...');
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ 
        audio: {
          channelCount: 1,
          sampleRate: 22050, // Lower sample rate for performance
          echoCancellation: false,
          noiseSuppression: false,
          autoGainControl: false
        } 
      });
      
      console.log('🎵 Got microphone stream');
      
      const audioContext = new (window.AudioContext || window.webkitAudioContext)();
      const microphone = audioContext.createMediaStreamSource(stream);
      const analyzer = audioContext.createAnalyser();
      
      // Minimal FFT size for performance
      analyzer.fftSize = 512;
      analyzer.smoothingTimeConstant = 0.8;
      
      microphone.connect(analyzer);
      
      const dataArray = new Uint8Array(analyzer.frequencyBinCount);
      
      console.log('🎵 Audio context created, FFT size:', analyzer.fftSize, 'Buffer size:', dataArray.length);
      
      audioContextRef.current = audioContext;
      microphoneRef.current = microphone;
      analyzerRef.current = analyzer;
      dataArrayRef.current = dataArray;
      
      // Set enabled state BEFORE starting the loop
      setHasPermission(true);
      setIsEnabled(true);
      
      console.log('🎵 Starting SHO-based processing loop...');
      
      // Start processing loop after state is set
      setTimeout(() => {
        console.log('🎵 Starting delayed SHO processing loop...');
        lastFrameTimeRef.current = getTime();
        
        // Create a new processing function that doesn't depend on stale closure
        const startProcessing = () => {
          if (analyzerRef.current && dataArrayRef.current) {
            console.log('🎵 Starting SHO processing with fresh state');
            processAudio();
          } else {
            console.log('🎵 Refs not ready, retrying...');
            setTimeout(startProcessing, 10);
          }
        };
        
        requestAnimationFrame(startProcessing);
      }, 50);
      
    } catch (error) {
      console.error('🎵 Error starting audio:', error);
      setHasPermission(false);
    }
  }, []); // Remove processAudio dependency to avoid stale closure

  const stopAudio = useCallback(() => {
    // Stop animation frame FIRST
    if (animationFrameRef.current) {
      cancelAnimationFrame(animationFrameRef.current);
      animationFrameRef.current = null;
    }
    
    // Stop microphone stream
    if (microphoneRef.current && microphoneRef.current.mediaStream) {
      microphoneRef.current.mediaStream.getTracks().forEach(track => track.stop());
    }
    
    // Close audio context
    if (audioContextRef.current && audioContextRef.current.state !== 'closed') {
      audioContextRef.current.close();
    }
    
    // Clear all refs
    audioContextRef.current = null;
    microphoneRef.current = null;
    analyzerRef.current = null;
    dataArrayRef.current = null;
    
    // Reset state
    setIsEnabled(false);
    setAudioFeatures({
      volume: 0,
      bassLevel: 0, 
      midLevel: 0,
      highLevel: 0,
      beatDetected: false,
      dominantFrequency: 0
    });
    
    // Reset performance counters
    frameCountRef.current = 0;
    processingTimesRef.current = [];
    lastFrameTimeRef.current = 0;
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
    stopAudio
  };
};

export default useAudio; 