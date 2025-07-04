import React, { createContext, useContext } from 'react';
import useAudio from '../hooks/useAudio';

// Create the Audio Context
const AudioContext = createContext();

// Audio Provider Component
export const AudioProvider = ({ children }) => {
  const audioHook = useAudio();

  return (
    <AudioContext.Provider value={audioHook}>
      {children}
    </AudioContext.Provider>
  );
};

// Custom hook to use the Audio Context
export const useAudioContext = () => {
  const context = useContext(AudioContext);
  
  if (!context) {
    throw new Error('useAudioContext must be used within an AudioProvider');
  }
  
  return context;
};

export default AudioContext; 