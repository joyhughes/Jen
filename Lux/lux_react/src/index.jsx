import React from 'react';
import ReactDOM from 'react-dom/client';
import './index.css';
import App from './App.jsx';

/*
var factory = require('./lux.js');

factory().then((instance) => {
  window.module = instance; // Assign to global variable
  window.module.canvas = document.getElementById('imagePortCanvas');
});
*/

// Initialize WASM and worker
(async () => {
  try {
    // Initialize WASM module
    await import('./lux.js').then((module) => {
      const factory = module.default ? module.default : module;
      
      factory().then((instance) => {
        window.module = instance; // Assign to global variable
        window.module.canvas = document.getElementById('imagePortCanvas');
        console.log('[WASM] Module initialized successfully');

        // Initialize video encoding worker after WASM is ready
        const worker = new Worker(new URL('/workers/videoEncodingWorker.js', import.meta.url));
        
        worker.onmessage = (event) => {
          const { type, success, error } = event.data;
          if (type === 'initialized') {
            console.log('[Worker] Video encoding worker initialized');
          } else if (type === 'error') {
            console.error('[Worker] Error:', error);
          }
        };

        worker.onerror = (error) => {
          console.error('[Worker] Error:', error);
        };

        // Initialize the worker with WASM module
        worker.postMessage({
          type: 'init',
          wasmUrl: '/src/lux.js'
        });

        window.videoWorker = worker; // Store worker reference globally
      });
    });
  } catch (error) {
    console.error('[Init] Error initializing:', error);
  }
})();

const root = ReactDOM.createRoot(document.getElementById('root'));
root.render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);


