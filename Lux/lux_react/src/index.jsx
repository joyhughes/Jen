import React from 'react';
import ReactDOM from 'react-dom/client';
import './index.css';
import ThemedApp from "./components/App.jsx";

/*
var factory = require('./lux.js');

factory().then((instance) => {
  window.module = instance; // Assign to global variable
  window.module.canvas = document.getElementById('imagePortCanvas');
});
*/

(async () => {
  try {
    await import('./lux.js').then((module) => {
      const factory = module.default ? module.default : module;
      
      factory().then((instance) => {
        window.module = instance; // Assign to global variable
        window.module.canvas = document.getElementById('imagePortCanvas');
      });
    });
  } catch (error) {
    console.error('Error initializing Emscripten module:', error);
  }
})();

const root = ReactDOM.createRoot(document.getElementById('root'));
root.render(
  <React.StrictMode>
    <ThemedApp />
  </React.StrictMode>
);


