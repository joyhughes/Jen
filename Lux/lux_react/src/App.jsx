
import './App.css';
//import './emscripten/lux.js';
import { BrowserRouter, Routes, Route } from 'react-router-dom';
//import Home from "./pages/Home";
//import Tour from "./pages/Tour";
//import SearchAppBar from './components/AppBar';
import Home from "./pages/Home";
import { ThemeProvider, createTheme } from '@mui/material';
//import { ThemeProvider, createTheme } from '@mui/material/styles';
import CssBaseline from '@mui/material/CssBaseline';

//default dark mode

const darkTheme = createTheme({
  palette: {
    mode: 'dark',
  },
});

// Jen prototype

function App() {
  return (<ThemeProvider theme={darkTheme}>
    <BrowserRouter>
    <Routes>
      <Route path="/" element={<Home/>} />
    </Routes>
  </BrowserRouter>
    </ThemeProvider>);
}

export default App;
