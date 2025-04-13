import React from 'react';
import { CssBaseline } from '@mui/material';

// Import theme provider
import { ThemeProvider } from './ThemeProvider.jsx';

// Import main container
import InterfaceContainer from './InterfaceContainer.jsx';

/**
 * Main App component that serves as the entry point for the Jen application.
 * Wraps the application in the theme provider and sets up the overall structure.
 */
function App() {
    return (
        <ThemeProvider>
            <CssBaseline />
            <InterfaceContainer />
        </ThemeProvider>
    );
}

export default App;