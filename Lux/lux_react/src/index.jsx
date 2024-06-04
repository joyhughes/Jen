import React from 'react';
import ReactDOM from 'react-dom/client';
import './index.css';
import App from './App.jsx';

/*
var factory = require('./lux.js');

factory().then((instance) => {
  window.Module = instance; // Assign to global variable
  window.Module.canvas = document.getElementById('imagePortCanvas');
});
*/

import('./lux.js').then((instance) => {
  window.module = instance; // Assign to global variable
//  window.module.canvas = document.getElementById('imagePortCanvas');
});

const root = ReactDOM.createRoot(document.getElementById('root'));
root.render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);


