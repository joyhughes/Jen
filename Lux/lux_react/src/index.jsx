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


await import('./lux.js').then((module) => {
  const factory = module.default ? module.default : module;
  
  factory().then((instance) => {
    window.module = instance; // Assign to global variable
    window.module.canvas = document.getElementById('imagePortCanvas');
  });
});

const root = ReactDOM.createRoot(document.getElementById('root'));
root.render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);


