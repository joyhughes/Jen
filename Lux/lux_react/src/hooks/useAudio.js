import { useState, useRef, useCallback, useEffect } from 'react';

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
  const [sensitivity, setSensitivity] = useState(0.5);

  const audioContextRef = useRef(null);
  const microphoneRef = useRef(null);
  const analyzerRef = useRef(null);
  const dataArrayRef = useRef(null);
  const sensitivityRef = useRef(0.5);
  const isEnabledRef = useRef(false);

  const audioStateRef = useRef({
    energyHistory: {
      values: [],
      average: 0
    },
    lastHeartbeat: 0,
    soundTypeHistory: {
      music: 0,
      singing: 0,
      drums: 0,
      instrument: 0
    },
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

  const initializeAudioSystem = useCallback(() => {
    try {
      if (window.module && window.module.enable_audio_input) {
        window.module.enable_audio_input(false);
      }

      if (window.module && window.module.update_audio_context) {
        window.module.update_audio_context(0.0, 0.0, 0.0, 0.0, false, 0.0);
      }

      return true;
    } catch (error) {
      return false;
    }
  }, []);

  const loadAudioConfig = useCallback(() => {
    try {
      if (window.module && window.module.enable_audio_input) {
        window.module.enable_audio_input(true);
      }
      return true;
    } catch (error) {
      return false;
    }
  }, []);

  useEffect(() => {
    sensitivityRef.current = sensitivity;

    try {
      if (window.module && window.module.set_audio_sensitivity) {
        window.module.set_audio_sensitivity(sensitivity);
      }
    } catch (error) {
      // Ignore
    }
  }, [sensitivity]);

  useEffect(() => {
    isEnabledRef.current = isEnabled;
  }, [isEnabled]);

  const updateAudioParameters = useCallback((deltaTime) => {
    if (!isEnabledRef.current || !analyzerRef.current || !dataArrayRef.current) {
      return;
    }

    const now = getTime();
    const audioState = audioStateRef.current;
    const currentSensitivity = sensitivityRef.current;

    if (!audioState.lastHeartbeat) audioState.lastHeartbeat = 0;

    analyzerRef.current.getByteFrequencyData(dataArrayRef.current);

    const bufferLength = dataArrayRef.current.length;
    const sampleRate = audioContextRef.current.sampleRate;
    const nyquist = sampleRate / 2;
    const binSize = nyquist / bufferLength;

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
      const gained = raw * (audioState.mixerGains[band.name] || 1.0);
      audioState.enhancedBands[band.name] = gained;
    });

    const bass = audioState.enhancedBands.bass;
    const mid = (audioState.enhancedBands.low_mid + audioState.enhancedBands.mid + audioState.enhancedBands.high_mid) / 3;
    const high = (audioState.enhancedBands.presence + audioState.enhancedBands.brilliance + audioState.enhancedBands.air) / 3;

    const volume = Math.max(
      audioState.enhancedBands.mid * 2.0,
      (bass + mid + high) / 3.0
    );

    const total_energy = volume + 0.001;
    const bass_ratio = (audioState.enhancedBands.sub_bass + audioState.enhancedBands.bass) / total_energy;
    const mid_ratio = (audioState.enhancedBands.low_mid + audioState.enhancedBands.mid) / total_energy;
    const high_ratio = (audioState.enhancedBands.presence + audioState.enhancedBands.brilliance + audioState.enhancedBands.air) / total_energy;

    const currentEnergy = volume;
    const beat = currentEnergy > (audioState.energyHistory.average * 1.4);

    const is_music = (bass_ratio > 0.3 && high_ratio > 0.25 && mid_ratio > 0.2); // Full spectrum
    const is_singing = (mid_ratio > 0.4 && volume > 0.12);                        // Mid-heavy
    const is_drums = (bass_ratio > 0.5 && beat);                                  // Bass + beat
    const is_instrument = (high_ratio > 0.35 && !beat);                          // High freq

    audioState.soundTypeHistory.music = is_music ? Math.min(1.0, audioState.soundTypeHistory.music + 0.1) : Math.max(0.0, audioState.soundTypeHistory.music - 0.05);
    audioState.soundTypeHistory.singing = is_singing ? Math.min(1.0, audioState.soundTypeHistory.singing + 0.1) : Math.max(0.0, audioState.soundTypeHistory.singing - 0.05);
    audioState.soundTypeHistory.drums = is_drums ? Math.min(1.0, audioState.soundTypeHistory.drums + 0.2) : Math.max(0.0, audioState.soundTypeHistory.drums - 0.1);
    audioState.soundTypeHistory.instrument = is_instrument ? Math.min(1.0, audioState.soundTypeHistory.instrument + 0.1) : Math.max(0.0, audioState.soundTypeHistory.instrument - 0.05);

    audioState.energyHistory.values.push(currentEnergy);
    if (audioState.energyHistory.values.length > 10) {
      audioState.energyHistory.values.shift();
    }
    audioState.energyHistory.average = audioState.energyHistory.values.reduce((a, b) => a + b, 0) / audioState.energyHistory.values.length;

    const sensitizedVolume = volume * currentSensitivity;
    const sensitizedBass = bass * currentSensitivity;
    const sensitizedMid = mid * currentSensitivity;
    const sensitizedHigh = high * currentSensitivity;


    if (isEnabledRef.current) {
      try {
        if (window.module && typeof window.module.update_audio_context === 'function') {
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
        console.error('Error updating audio context:', error);
      }
    }

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

  const initializeMicrophone = useCallback(async () => {
    try {
      const stream = await navigator.mediaDevices.getUserMedia({
        audio: {
          echoCancellation: false,
          noiseSuppression: false,
          autoGainControl: false,
          sampleRate: 44100
        }
      });

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

      return true;
    } catch (error) {
      console.error('Microphone initialization failed:', error);
      return false;
    }
  }, []);

  const enableAudio = useCallback(async () => {
    try {
      if (!analyzerRef.current) {
        const micSuccess = await initializeMicrophone();
        if (!micSuccess) {
          return;
        }
      }

      const configSuccess = loadAudioConfig();
      if (!configSuccess) {
        return;
      }

      setIsEnabled(true);
    } catch (error) {
      console.error('Error enabling audio:', error);
    }
  }, [initializeMicrophone, loadAudioConfig]);

  const disableAudio = useCallback(() => {
    try {
      if (window.module && window.module.enable_audio_input) {
        window.module.enable_audio_input(false);
      }
    } catch (error) {
      // Ignore
    }

    try {
      if (window.module && window.module.update_audio_context) {
        window.module.update_audio_context(0.0, 0.0, 0.0, 0.0, false, 0.0);
      }
    } catch (error) {
      // Ignore
    }

    setIsEnabled(false);

    setMixerState({
      volume: 0,
      bass: 0,
      mid: 0,
      high: 0,
      beat: false,
      isProcessing: false
    });

    setAudioFeatures({
      volume: 0,
      bassLevel: 0,
      midLevel: 0,
      highLevel: 0,
      beatDetected: false,
      dominantFrequency: 0
    });
  }, []);

  useEffect(() => {
    if (isEnabled) {
      const setupAudio = () => {
        if (!window.module) {
          setTimeout(setupAudio, 100);
          return;
        }

        window.audioUpdateFunction = updateAudioParameters;

        try {
          if (window.module.enable_audio_input) {
            window.module.enable_audio_input(true);
          }
        } catch (error) {
          console.error('Error enabling audio in backend:', error);
        }
      };

      setupAudio();
    } else {
      window.audioUpdateFunction = null;
    }

    return () => {
      window.audioUpdateFunction = null;
    };
  }, [isEnabled, updateAudioParameters]);

  useEffect(() => {
    initializeAudioSystem();
  }, [initializeAudioSystem]);

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

  const handleMixerGainChange = useCallback((gains) => {
    if (gains && audioStateRef.current) {
      audioStateRef.current.mixerGains = { ...gains };
    }
  }, []);

  const testIntegration = useCallback(() => {
    if (!isEnabled) {
      return false;
    }

    try {
      if (window.module && window.module.get_autoplay_audio_status) {
        const statusJson = window.module.get_autoplay_audio_status();
        JSON.parse(statusJson);
      }

      return mixerState.volume > 0 || mixerState.bass > 0 || mixerState.mid > 0 || mixerState.high > 0;
    } catch (error) {
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