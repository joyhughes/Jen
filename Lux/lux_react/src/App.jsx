import './App.css';
import { BrowserRouter, Routes, Route } from 'react-router-dom';
import Home from "./pages/Home";
import { ThemeProvider } from '@mui/material';
import CssBaseline from '@mui/material/CssBaseline';
import theme from './theme';

function App() {
  return (<ThemeProvider theme={theme}>
    <CssBaseline />
    {/* basename keeps routes matching when the app is served from a
        subpath such as GitHub Pages' /Jen/ */}
    <BrowserRouter basename={import.meta.env.BASE_URL}>
    <Routes>
      <Route path="/" element={<Home/>} />
    </Routes>
  </BrowserRouter>
    </ThemeProvider>);
}

export default App;
