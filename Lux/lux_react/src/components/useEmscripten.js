/*
import { useEffect, useState } from 'react';

function useEmscripten() {
  const [moduleReady, setModuleReady] = useState(false);
  const Module = require('../lux.js');

  useEffect(() => {
    Module.onRuntimeInitialized = () => {
      setModuleReady(true);
    };
  }, []);

  return { moduleReady, moduleInstance: Module };
}

export default useEmscripten;

*/
/*
// emscriptenModule.js

// Load the Emscripten module
const Module = require('../lux.js');

// Initialize the module (this is often asynchronous)
Module.onRuntimeInitialized = function() {
  console.log('Emscripten module initialized');
};

// Export the initialized Module for use in other parts of your application
export default Module;

*/