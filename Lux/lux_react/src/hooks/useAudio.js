import { useState, useRef, useCallback, useEffect } from 'react';

// Performance timing helper
const getTime = () => (typeof performance !== 'undefined' && performance.now) ? performance.now() : Date.now();

const useAudio = () => {
  const [isEnabled, setIsEnabled] = useState(false);
  const [mixerState, setMixerState] = useState({
    volume: 0,
    bass: 0,
    mid: 0,
    high: 0,
    beat: false,
    isProcessing: false
  });
  const [audioFeatures, setAudioFeatures] = useState({
    volume: 0,
    bassLevel: 0,
    midLevel: 0,
    highLevel: 0,
    beatDetected: false,
    dominantFrequency: 0
  });
  const [sensitivity, setSensitivity] = useState(0.5); // Default 50% for gentle audio reactivity

  // Audio processing refs
  const audioContextRef = useRef(null);
  const microphoneRef = useRef(null);
  const analyzerRef = useRef(null);
  const dataArrayRef = useRef(null);
  const sensitivityRef = useRef(0.5);
  const isEnabledRef = useRef(false);
  
  // Enhanced audio state for diverse effects
  const audioStateRef = useRef({
    energyHistory: {
      values: [],
      average: 0
    },
    lastHeartbeat: 0,
    // Sound type detection
    soundTypeHistory: {
      music: 0,
      singing: 0,
      drums: 0,
      instrument: 0
    },
    // Enhanced frequency analysis
    enhancedBands: {
      sub_bass: 0,
      bass: 0,
      low_mid: 0,
      mid: 0,
      high_mid: 0,
      presence: 0,
      brilliance: 0,
      air: 0
    },
    // Mixer gains (applied from AudioMixer)
    mixerGains: {
      sub_bass: 1.0,
      bass: 1.0,
      low_mid: 1.0,
      mid: 1.0,
      high_mid: 1.0,
      presence: 1.0,
      brilliance: 1.0,
      air: 1.0
    }
  });

  // Initialize audio system with clean state
  const initializeAudioSystem = useCallback(() => {
    try {
      console.log('🎵 📋 Initializing scene-agnostic audio system...');
      
      // First ensure audio is disabled and clean
      if (window.module && window.module.enable_audio_input) {
        window.module.enable_audio_input(false);
        console.log('🎵 🧹 Audio system initialized in disabled state');
      }
      
      // Clear any residual audio values
      if (window.module && window.module.update_audio_context) {
        window.module.update_audio_context(0.0, 0.0, 0.0, 0.0, false, 0.0);
        console.log('🎵 🧹 Audio values cleared on initialization');
      }
      
      return true;
      
    } catch (error) {
      console.warn('🎵 Error initializing audio system:', error);
      return false;
    }
  }, []);

  // Load audio configuration when enabling
  const loadAudioConfig = useCallback(() => {
    try {
      console.log('🎵 🔧 Enabling audio in backend...');
        try {
          if (window.module && window.module.enable_audio_input) {
            window.module.enable_audio_input(true);
          console.log('🎵 ✅ Audio enabled in backend (scene-agnostic mode)');
          }
        } catch (error) {
          console.warn('🎵 ⚠️ Could not enable audio in backend:', error);
      }
      
      // Check if autoplay is active and log the state for debugging
      try {
        if (window.module && window.module.get_slider_value) {
          const autoplayState = window.module.get_slider_value('autoplay_switch');
          console.log('🎵 🎲 Autoplay state detected:', autoplayState ? 'ACTIVE' : 'INACTIVE');
          if (autoplayState) {
            console.log('🎵 ✨ Audio + Autoplay: Both systems active - expect dynamic behavior!');
          } else {
            console.log('🎵 🎛️ Audio only: Manual control mode');
          }
        }
      } catch (error) {
        console.log('🎵 ℹ️ Could not check autoplay state (scene may not have autoplay)');
      }
      
      return true;
      
    } catch (error) {
      console.warn('🎵 Error enabling audio:', error);
      return false;
    }
  }, []);

  // Update refs when state changes
  useEffect(() => {
    sensitivityRef.current = sensitivity;
    
    // Update backend sensitivity
    try {
      if (window.module && window.module.set_audio_sensitivity) {
        window.module.set_audio_sensitivity(sensitivity);
        console.log(`🎵 🎛️ Backend sensitivity updated to: ${sensitivity}`);
      }
    } catch (error) {
      console.warn('🎵 ⚠️ Could not update backend sensitivity:', error);
    }
  }, [sensitivity]);

  useEffect(() => {
    isEnabledRef.current = isEnabled;
  }, [isEnabled]);

  // ENHANCED: Main audio update function with diverse sound analysis
  const updateAudioParameters = useCallback((deltaTime) => {
    if (!isEnabledRef.current || !analyzerRef.current || !dataArrayRef.current) {
      return; // Silent return, no logging needed
    }

    const now = getTime();
    const audioState = audioStateRef.current;
    const currentSensitivity = sensitivityRef.current;

    // Heartbeat log every 5 seconds
    if (!audioState.lastHeartbeat) audioState.lastHeartbeat = 0;
    if (now - audioState.lastHeartbeat >= 5000) {
      console.log('🎵 Enhanced audio system running (diverse sound analysis)');
      audioState.lastHeartbeat = now;
    }

    // Get audio data
    analyzerRef.current.getByteFrequencyData(dataArrayRef.current);
    
    // ENHANCED: 8-band frequency analysis for diverse effects
    const bufferLength = dataArrayRef.current.length;
    const sampleRate = audioContextRef.current.sampleRate;
    const nyquist = sampleRate / 2;
    const binSize = nyquist / bufferLength;
    
    // Enhanced frequency bands for diverse sound detection
    const bands = [
      { name: 'sub_bass', min: 20, max: 60 },      // Sub bass
      { name: 'bass', min: 60, max: 250 },         // Bass
      { name: 'low_mid', min: 250, max: 500 },     // Low mid
      { name: 'mid', min: 500, max: 2000 },        // Mid (main voice)
      { name: 'high_mid', min: 2000, max: 4000 },  // High mid (voice harmonics)
      { name: 'presence', min: 4000, max: 8000 },  // Presence
      { name: 'brilliance', min: 8000, max: 16000 }, // Brilliance
      { name: 'air', min: 16000, max: 22000 }      // Air
    ];
    
    // Calculate enhanced frequency bands
    bands.forEach(band => {
      const startBin = Math.floor(band.min / binSize);
      const endBin = Math.floor(band.max / binSize);
      
      let sum = 0;
      let count = 0;
      
      for (let i = startBin; i <= endBin && i < bufferLength; i++) {
        sum += dataArrayRef.current[i];
        count++;
      }
      
      const raw = count > 0 ? (sum / count) / 255.0 : 0;
      // Apply mixer gains
      const gained = raw * (audioState.mixerGains[band.name] || 1.0);
      audioState.enhancedBands[band.name] = gained;
    });
    
    // Legacy 3-band calculation for compatibility
    const bass = audioState.enhancedBands.bass;
    const mid = (audioState.enhancedBands.low_mid + audioState.enhancedBands.mid + audioState.enhancedBands.high_mid) / 3;
    const high = (audioState.enhancedBands.presence + audioState.enhancedBands.brilliance + audioState.enhancedBands.air) / 3;
    
    // Enhanced volume calculation
    const volume = Math.max(
      audioState.enhancedBands.mid * 2.0,  // Voice priority
      (bass + mid + high) / 3.0            // General average
    );
    
    // === ENHANCED SOUND TYPE DETECTION ===
    const total_energy = volume + 0.001;
    const bass_ratio = (audioState.enhancedBands.sub_bass + audioState.enhancedBands.bass) / total_energy;
    const mid_ratio = (audioState.enhancedBands.low_mid + audioState.enhancedBands.mid) / total_energy;
    const high_ratio = (audioState.enhancedBands.presence + audioState.enhancedBands.brilliance + audioState.enhancedBands.air) / total_energy;
    
    // Enhanced beat detection
    const currentEnergy = volume;
    const beat = currentEnergy > (audioState.energyHistory.average * 1.4);
    
    // Sound type classification with history
    const is_music = (bass_ratio > 0.3 && high_ratio > 0.25 && mid_ratio > 0.2); // Full spectrum
    const is_singing = (mid_ratio > 0.4 && volume > 0.12);                        // Mid-heavy
    const is_drums = (bass_ratio > 0.5 && beat);                                  // Bass + beat
    const is_instrument = (high_ratio > 0.35 && !beat);                          // High freq
    
    // Update sound type history for stability
    audioState.soundTypeHistory.music = is_music ? Math.min(1.0, audioState.soundTypeHistory.music + 0.1) : Math.max(0.0, audioState.soundTypeHistory.music - 0.05);
    audioState.soundTypeHistory.singing = is_singing ? Math.min(1.0, audioState.soundTypeHistory.singing + 0.1) : Math.max(0.0, audioState.soundTypeHistory.singing - 0.05);
    audioState.soundTypeHistory.drums = is_drums ? Math.min(1.0, audioState.soundTypeHistory.drums + 0.2) : Math.max(0.0, audioState.soundTypeHistory.drums - 0.1);
    audioState.soundTypeHistory.instrument = is_instrument ? Math.min(1.0, audioState.soundTypeHistory.instrument + 0.1) : Math.max(0.0, audioState.soundTypeHistory.instrument - 0.05);
    
    // Update energy history
    audioState.energyHistory.values.push(currentEnergy);
    if (audioState.energyHistory.values.length > 10) {
      audioState.energyHistory.values.shift();
    }
    audioState.energyHistory.average = audioState.energyHistory.values.reduce((a, b) => a + b, 0) / audioState.energyHistory.values.length;
    
    // Apply global sensitivity for backend
    const sensitizedVolume = volume * currentSensitivity;
    const sensitizedBass = bass * currentSensitivity;
    const sensitizedMid = mid * currentSensitivity;
    const sensitizedHigh = high * currentSensitivity;
    
    // Enhanced logging for sound types
    if (volume > 0.1) {
      const dominant_type = Object.keys(audioState.soundTypeHistory).reduce((a, b) => 
        audioState.soundTypeHistory[a] > audioState.soundTypeHistory[b] ? a : b
      );
      
      if (audioState.soundTypeHistory[dominant_type] > 0.3) {
        const type_icons = { music: '🎵', singing: '🎤', drums: '🥁', instrument: '🎸' };
        console.log(`${type_icons[dominant_type]} ${dominant_type.toUpperCase()} detected! Vol: ${volume.toFixed(3)}, Bass ratio: ${bass_ratio.toFixed(2)}, Mid ratio: ${mid_ratio.toFixed(2)}, High ratio: ${high_ratio.toFixed(2)}`);
      }
    }
    
    // Send enhanced audio data to backend
    if (isEnabledRef.current) {
      try {
        if (window.module && window.module.update_audio_context) {
          window.module.update_audio_context(
            sensitizedVolume,
            sensitizedBass, 
            sensitizedMid,
            sensitizedHigh,
            beat,
            now
          );
        }
      } catch (error) {
        console.warn('🎵 Error updating audio context:', error);
      }
    }

    // Update mixer state with enhanced bands
    setMixerState(prev => ({
      ...prev,
      volume: Math.min(volume * 100, 100),
      bass: Math.min(bass * 100, 100),
      mid: Math.min(mid * 100, 100),
      high: Math.min(high * 100, 100),
      beat: beat,
      isProcessing: true,
      // Enhanced features for mixer
      enhancedFeatures: {
        sub_bass: audioState.enhancedBands.sub_bass,
        bass: audioState.enhancedBands.bass,
        low_mid: audioState.enhancedBands.low_mid,
        mid: audioState.enhancedBands.mid,
        high_mid: audioState.enhancedBands.high_mid,
        presence: audioState.enhancedBands.presence,
        brilliance: audioState.enhancedBands.brilliance,
        air: audioState.enhancedBands.air
      },
      soundTypes: { ...audioState.soundTypeHistory }
    }));

    // Update audio features with enhanced data
    setAudioFeatures(prev => ({
      ...prev,
      volume: Math.min(volume * 100, 100),
      bassLevel: Math.min(bass * 100, 100),
      midLevel: Math.min(mid * 100, 100),
      highLevel: Math.min(high * 100, 100),
      beatDetected: beat,
      dominantFrequency: 0,
      enhancedFeatures: { ...audioState.enhancedBands },
      soundTypes: { ...audioState.soundTypeHistory }
    }));
  }, []);

  // Initialize microphone access
  const initializeMicrophone = useCallback(async () => {
    try {
      console.log('🎵 🎤 Requesting microphone access...');
      
      const stream = await navigator.mediaDevices.getUserMedia({
        audio: {
          echoCancellation: false,
          noiseSuppression: false,
          autoGainControl: false,
          sampleRate: 44100
        }
      });

      console.log('🎵 ✅ Microphone access granted');

      const audioContext = new (window.AudioContext || window.webkitAudioContext)();
      const microphone = audioContext.createMediaStreamSource(stream);
      const analyzer = audioContext.createAnalyser();

      analyzer.fftSize = 2048;
      analyzer.smoothingTimeConstant = 0.8;

      microphone.connect(analyzer);
      
      const dataArray = new Uint8Array(analyzer.frequencyBinCount);

      audioContextRef.current = audioContext;
      microphoneRef.current = microphone;
      analyzerRef.current = analyzer;
      dataArrayRef.current = dataArray;

      console.log('🎵 🔧 Audio analysis setup complete');
      
      return true;
    } catch (error) {
      console.error('🎵 ❌ Microphone initialization failed:', error);
      return false;
    }
  }, []);

  // Enable audio processing
  const enableAudio = useCallback(async () => {
    try {
      console.log('🎵 🚀 Enabling audio...');
      
      if (!analyzerRef.current) {
        const micSuccess = await initializeMicrophone();
        if (!micSuccess) {
          console.error('🎵 ❌ Failed to initialize microphone');
          return;
        }
      }

      const configSuccess = loadAudioConfig();
      if (!configSuccess) {
        console.error('🎵 ❌ Failed to load audio config');
        return;
      }
      
      // RESPECT USER'S ANIMATION STATE - Don't force animation to start
      // Audio will work with whatever animation state the user has set
      console.log('🎵 ✅ Audio enabled - respecting current animation state');
      
      // Now set enabled and start animation loop
      setIsEnabled(true);
      console.log('🎵 ✅ Audio enabled successfully');

    } catch (error) {
      console.error('🎵 ❌ Error enabling audio:', error);
    }
  }, [initializeMicrophone, loadAudioConfig]);

  // Disable audio processing
  const disableAudio = useCallback(() => {
    console.log('🎵 🛑 Disabling audio...');
    
    try {
      if (window.module && window.module.enable_audio_input) {
        window.module.enable_audio_input(false);
      }
    } catch (error) {
      console.warn('🎵 ⚠️ Error disabling audio in backend:', error);
    }
    
    // Send zero values to backend to clear any residual audio data
    try {
      if (window.module && window.module.update_audio_context) {
        window.module.update_audio_context(0.0, 0.0, 0.0, 0.0, false, 0.0);
        console.log('🎵 🧹 Cleared audio values in backend');
      }
    } catch (error) {
      console.warn('🎵 ⚠️ Error clearing audio values:', error);
    }
    
    setIsEnabled(false);
    
    // Reset mixer state
    setMixerState({
      volume: 0,
      bass: 0,
      mid: 0,
      high: 0,
      beat: false,
      isProcessing: false
    });
    
    // Reset audio features
    setAudioFeatures({
      volume: 0,
      bassLevel: 0,
      midLevel: 0,
      highLevel: 0,
      beatDetected: false,
      dominantFrequency: 0
    });

    console.log('🎵 ✅ Audio disabled and cleared');
  }, []);

  // Animation loop for audio processing
  useEffect(() => {
    let animationId;
    
    if (isEnabled) {
      const audioAnimationLoop = () => {
        updateAudioParameters(0.016667); // ~60fps
        animationId = requestAnimationFrame(audioAnimationLoop);
      };
      
      console.log('🎵 🔄 Starting audio animation loop...');
      audioAnimationLoop();
    }
    
    return () => {
      if (animationId) {
        cancelAnimationFrame(animationId);
        console.log('🎵 ⏹️ Audio animation loop stopped');
      }
    };
  }, [isEnabled, updateAudioParameters]);

  // Initialize audio system on mount
  useEffect(() => {
    initializeAudioSystem();
  }, [initializeAudioSystem]);

  // Cleanup on unmount
  useEffect(() => {
    const handleBeforeUnload = () => {
      if (isEnabled) {
        disableAudio();
      }
    };

    window.addEventListener('beforeunload', handleBeforeUnload);

    return () => {
      window.removeEventListener('beforeunload', handleBeforeUnload);
      handleBeforeUnload();
    };
  }, [isEnabled, disableAudio]);

  // Callback to receive mixer gain changes
  const handleMixerGainChange = useCallback((gains) => {
    if (gains && audioStateRef.current) {
      audioStateRef.current.mixerGains = { ...gains };
      console.log('🎛️ Mixer gains updated:', gains);
    }
  }, []);

  // Test function to verify audio+autoplay integration
  const testIntegration = useCallback(() => {
    if (!isEnabled) {
      console.log('🎵 ❌ Cannot test integration: Audio is disabled');
      return false;
    }
    
    try {
      // Test 1: Check if audio values are being sent to backend
      if (window.module && window.module.update_audio_context) {
        window.module.update_audio_context(0.5, 0.3, 0.4, 0.2, true, Date.now());
        console.log('🎵 ✅ Test 1 passed: Audio context update successful');
      } else {
        console.log('🎵 ❌ Test 1 failed: Audio context update not available');
        return false;
      }
      
      // Test 2: Check autoplay status
      if (window.module && window.module.get_autoplay_audio_status) {
        const statusJson = window.module.get_autoplay_audio_status();
        const status = JSON.parse(statusJson);
        console.log('🎵 ✅ Test 2 passed: Integration status check successful', status);
        
        if (status.has_autoplay) {
          console.log('🎵 🎲 Autoplay detected - integration ready!');
        } else {
          console.log('🎵 🎛️ Audio-only mode - no autoplay in this scene');
        }
      } else {
        console.log('🎵 ⚠️ Test 2 warning: Integration status check not available');
      }
      
      // Test 3: Check if audio functions are working
      const hasAudioFunctions = mixerState.volume > 0 || mixerState.bass > 0 || mixerState.mid > 0 || mixerState.high > 0;
      if (hasAudioFunctions) {
        console.log('🎵 ✅ Test 3 passed: Audio functions are processing data');
      } else {
        console.log('🎵 ⚠️ Test 3 warning: No audio data detected (may be silent)');
      }
      
      console.log('🎵 🎉 Integration test completed successfully!');
      return true;
      
    } catch (error) {
      console.log('🎵 ❌ Integration test failed:', error);
      return false;
    }
  }, [isEnabled, mixerState]);

  return {
    isEnabled,
    mixerState,
    audioFeatures,
    sensitivity,
    setSensitivity,
    enableAudio,
    disableAudio,
    handleMixerGainChange,
    testIntegration
  };
};

export default useAudio;