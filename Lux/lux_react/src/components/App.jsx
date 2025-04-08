import React from 'react';
import { CssBaseline } from '@mui/material';

// Import theme provider
import { JenThemeProvider } from './JenThemeProvider';

// Import main container
import JenInterfaceContainer from './JenInterfaceContainer.jsx';

/**
 * Main App component that serves as the entry point for the Jen application.
 * Wraps the application in the theme provider and sets up the overall structure.
 */
function App() {
    return (
        <JenThemeProvider>
            <CssBaseline />
            <JenInterfaceContainer />
        </JenThemeProvider>
    );
}

export default App;