import React, { createContext, useContext, useState, useMemo } from 'react';
import {
    ThemeProvider as MuiThemeProvider,
    createTheme,
    alpha
} from '@mui/material/styles';
import CssBaseline from '@mui/material/CssBaseline';

// Create a context for theme-related operations
const ThemeContext = createContext({
    toggleColorMode: () => {},
    theme: null,
});

// Hook to use the theme context
export const useThemeContext = () => useContext(ThemeContext);

// The main theme provider component
export function ThemeProvider({ children }) {
    const [mode, setMode] = useState('dark'); // Default to dark mode

    // Theme toggle function
    const toggleColorMode = () => {
        setMode((prevMode) => (prevMode === 'light' ? 'dark' : 'light'));
    };

    // Create the theme with colors based on the current mode
    const theme = useMemo(() => {
        // Define custom color palette for Jen
        const primaryColor = mode === 'dark' ? '#64B5F6' : '#1976D2'; // Blue
        const secondaryColor = mode === 'dark' ? '#4DB6AC' : '#00897B'; // Teal

        return createTheme({
            palette: {
                mode,
                primary: {
                    main: primaryColor,
                    light: mode === 'dark' ? '#90CAF9' : '#42A5F5',
                    dark: mode === 'dark' ? '#1E88E5' : '#1565C0',
                    contrastText: '#fff',
                },
                secondary: {
                    main: secondaryColor,
                    light: mode === 'dark' ? '#80CBC4' : '#4DB6AC',
                    dark: mode === 'dark' ? '#00897B' : '#00695C',
                    contrastText: '#fff',
                },
                background: {
                    default: mode === 'dark' ? 'rgba(255, 255, 255, 0.12)' : '#f5f5f5',
                    paper: mode === 'dark' ? '#1e1e1e' : '#ffffff',
                    subtle: mode === 'dark' ? '#252525' : '#f0f0f0',
                },
                text: {
                    primary: mode === 'dark' ? '#e0e0e0' : '#263238',
                    secondary: mode === 'dark' ? '#b0b0b0' : '#546E7A',
                },
                action: {
                    active: mode === 'dark' ? '#90CAF9' : '#1976D2',
                    hover: mode === 'dark' ? 'rgba(144, 202, 249, 0.08)' : 'rgba(25, 118, 210, 0.04)',
                    selected: mode === 'dark' ? 'rgba(144, 202, 249, 0.16)' : 'rgba(25, 118, 210, 0.08)',
                },
                divider: mode === 'dark' ? 'rgba(255, 255, 255, 0.12)' : 'rgba(0, 0, 0, 0.12)',
            },
            shape: {
                borderRadius: 8,
            },
            typography: {
                fontFamily: [
                    '-apple-system',
                    'BlinkMacSystemFont',
                    '"Segoe UI"',
                    'Roboto',
                    '"Helvetica Neue"',
                    'Arial',
                    'sans-serif',
                ].join(','),
                h1: {
                    fontWeight: 600,
                },
                h2: {
                    fontWeight: 600,
                },
                h3: {
                    fontWeight: 600,
                },
                h4: {
                    fontWeight: 600,
                },
                h5: {
                    fontWeight: 600,
                },
                h6: {
                    fontWeight: 600,
                },
                subtitle1: {
                    fontWeight: 500,
                },
                subtitle2: {
                    fontWeight: 500,
                },
            },
            components: {
                MuiPaper: {
                    styleOverrides: {
                        root: {
                            backgroundImage: 'none',
                        },
                        outlined: {
                            borderColor: mode === 'dark' ? 'rgba(255, 255, 255, 0.12)' : 'rgba(0, 0, 0, 0.12)',
                        },
                    },
                },
                MuiButton: {
                    styleOverrides: {
                        root: {
                            textTransform: 'none',
                            fontWeight: 500,
                        },
                        containedPrimary: {
                            boxShadow: 'none',
                            '&:hover': {
                                boxShadow: '0 2px 8px 0 ' + alpha(primaryColor, 0.3),
                            },
                        },
                        containedSecondary: {
                            boxShadow: 'none',
                            '&:hover': {
                                boxShadow: '0 2px 8px 0 ' + alpha(secondaryColor, 0.3),
                            },
                        },
                        outlined: {
                            borderWidth: '1px',
                            '&:hover': {
                                borderWidth: '1px',
                            },
                        },
                    },
                },
                MuiSlider: {
                    styleOverrides: {
                        root: {
                            height: 6,
                            '& .MuiSlider-track': {
                                border: 'none',
                            },
                            '& .MuiSlider-thumb': {
                                height: 18,
                                width: 18,
                                backgroundColor: '#fff',
                                border: '2px solid currentColor',
                                '&:focus, &:hover, &.Mui-active, &.Mui-focusVisible': {
                                    boxShadow: 'inherit',
                                },
                                '&:before': {
                                    display: 'none',
                                },
                            },
                        },
                    },
                },
                MuiInputBase: {
                    styleOverrides: {
                        input: {
                            '&::placeholder': {
                                opacity: 0.7,
                            },
                        },
                    },
                },
                MuiIconButton: {
                    styleOverrides: {
                        root: {
                            color: mode === 'dark' ? '#b0b0b0' : '#546E7A',
                        },
                    },
                },
                MuiSelect: {
                    styleOverrides: {
                        root: {
                            borderRadius: 8,
                        },
                    },
                },
                MuiTooltip: {
                    styleOverrides: {
                        tooltip: {
                            backgroundColor: mode === 'dark' ? '#424242' : '#616161',
                            fontSize: '0.75rem',
                            padding: '8px 12px',
                            borderRadius: 6,
                        },
                        arrow: {
                            color: mode === 'dark' ? '#424242' : '#616161',
                        },
                    },
                },
                MuiCircularProgress: {
                    styleOverrides: {
                        circle: {
                            strokeLinecap: 'round',
                        },
                    },
                },
            },
        });
    }, [mode]);

    // Context value to provide
    const contextValue = {
        toggleColorMode,
        theme,
    };

    return (
        <ThemeContext.Provider value={contextValue}>
            <MuiThemeProvider theme={theme}>
                <CssBaseline />
                {children}
            </MuiThemeProvider>
        </ThemeContext.Provider>
    );
}