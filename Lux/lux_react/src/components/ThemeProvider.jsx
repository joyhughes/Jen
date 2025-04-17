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
                common: {
                    black: '#000',
                    white: '#fff'
                },
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
                error: {
                    main: '#F44336',
                    light: '#E57373',
                    dark: '#D32F2F',
                    contrastText: '#fff',
                },
                warning: {
                    main: '#FF9800',
                    light: '#FFB74D',
                    dark: '#F57C00',
                    contrastText: 'rgba(0, 0, 0, 0.87)',
                },
                info: {
                    main: '#2196F3',
                    light: '#64B5F6',
                    dark: '#1976D2',
                    contrastText: '#fff',
                },
                success: {
                    main: '#4CAF50',
                    light: '#81C784',
                    dark: '#388E3C',
                    contrastText: 'rgba(0, 0, 0, 0.87)',
                },
                grey: {
                    50: '#fafafa',
                    100: '#f5f5f5',
                    200: '#eeeeee',
                    300: '#e0e0e0',
                    400: '#bdbdbd',
                    500: '#9e9e9e',
                    600: '#757575',
                    700: '#616161',
                    800: '#424242',
                    900: '#212121',
                    A100: '#d5d5d5',
                    A200: '#aaaaaa',
                    A400: '#303030',
                    A700: '#616161',
                },
                text: {
                    primary: mode === 'dark' ? 'rgba(255, 255, 255, 0.90)' : 'rgba(0, 0, 0, 0.87)',
                    secondary: mode === 'dark' ? 'rgba(255, 255, 255, 0.60)' : 'rgba(0, 0, 0, 0.60)',
                    disabled: mode === 'dark' ? 'rgba(255, 255, 255, 0.38)' : 'rgba(0, 0, 0, 0.38)',
                },
                divider: mode === 'dark' ? 'rgba(255, 255, 255, 0.12)' : 'rgba(0, 0, 0, 0.12)',
                background: {
                    paper: mode === 'dark' ? '#202124' : '#ffffff',
                    default: mode === 'dark' ? '#121212' : '#f5f5f5',
                    subtle: mode === 'dark' ? '#1a1a1a' : '#f0f0f0',
                },
                action: {
                    active: mode === 'dark' ? 'rgba(255, 255, 255, 0.54)' : 'rgba(0, 0, 0, 0.54)',
                    hover: mode === 'dark' ? 'rgba(255, 255, 255, 0.08)' : 'rgba(0, 0, 0, 0.04)',
                    hoverOpacity: 0.08,
                    selected: mode === 'dark' ? 'rgba(255, 255, 255, 0.16)' : 'rgba(0, 0, 0, 0.08)',
                    selectedOpacity: 0.16,
                    disabled: mode === 'dark' ? 'rgba(255, 255, 255, 0.26)' : 'rgba(0, 0, 0, 0.26)',
                    disabledBackground: mode === 'dark' ? 'rgba(255, 255, 255, 0.12)' : 'rgba(0, 0, 0, 0.12)',
                    disabledOpacity: 0.38,
                    focus: mode === 'dark' ? 'rgba(255, 255, 255, 0.12)' : 'rgba(0, 0, 0, 0.12)',
                    focusOpacity: 0.12,
                    activatedOpacity: 0.24,
                },
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
                    fontSize: '2.375rem',
                    lineHeight: 1.2,
                },
                h2: {
                    fontWeight: 600,
                    fontSize: '1.875rem',
                    lineHeight: 1.2,
                },
                h3: {
                    fontWeight: 600,
                    fontSize: '1.5rem',
                    lineHeight: 1.2,
                },
                h4: {
                    fontWeight: 600,
                    fontSize: '1.25rem',
                    lineHeight: 1.2,
                },
                h5: {
                    fontWeight: 600,
                    fontSize: '1.125rem',
                    lineHeight: 1.2,
                },
                h6: {
                    fontWeight: 600,
                    fontSize: '1rem',
                    lineHeight: 1.2,
                },
                subtitle1: {
                    fontWeight: 500,
                    fontSize: '0.875rem',
                    lineHeight: 1.5,
                },
                subtitle2: {
                    fontWeight: 500,
                    fontSize: '0.75rem',
                    lineHeight: 1.57,
                },
                body1: {
                    fontWeight: 400,
                    fontSize: '0.875rem',
                    lineHeight: 1.5,
                },
                body2: {
                    fontWeight: 400,
                    fontSize: '0.75rem',
                    lineHeight: 1.57,
                },
                button: {
                    fontWeight: 500,
                    fontSize: '0.875rem',
                    textTransform: 'none',
                },
                caption: {
                    fontWeight: 400,
                    fontSize: '0.75rem',
                    lineHeight: 1.66,
                },
                overline: {
                    fontWeight: 600,
                    fontSize: '0.75rem',
                    textTransform: 'uppercase',
                    letterSpacing: '0.08em',
                }
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
                            borderRadius: 8,
                        },
                        contained: {
                            boxShadow: 'none',
                            '&:hover': {
                                boxShadow: '0 2px 8px 0 ' + alpha(primaryColor, 0.3),
                            },
                        },
                        outlined: {
                            borderWidth: '1px',
                            '&:hover': {
                                borderWidth: '1px',
                            },
                        },
                        sizeSmall: {
                            padding: '4px 10px',
                            fontSize: '0.8125rem',
                        },
                    },
                },
                MuiButtonGroup: {
                    styleOverrides: {
                        root: {
                            boxShadow: 'none',
                        },
                    },
                },
                MuiSlider: {
                    styleOverrides: {
                        root: {
                            height: 6,
                            '& .MuiSlider-track': {
                                border: 'none',
                                transition: 'background-color 250ms cubic-bezier(0.4, 0, 0.2, 1) 0ms,box-shadow 250ms cubic-bezier(0.4, 0, 0.2, 1) 0ms',
                            },
                            '& .MuiSlider-thumb': {
                                height: 16,
                                width: 16,
                                backgroundColor: '#fff',
                                border: '2px solid currentColor',
                                '&:focus, &:hover, &.Mui-active, &.Mui-focusVisible': {
                                    boxShadow: `0 0 0 8px ${alpha(primaryColor, 0.16)}`,
                                },
                            },
                            '& .MuiSlider-valueLabel': {
                                backgroundColor: primaryColor,
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
                            borderRadius: 6,
                        },
                        sizeSmall: {
                            padding: 4,
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
                MuiChip: {
                    styleOverrides: {
                        root: {
                            borderRadius: 4,
                        },
                    },
                },
                MuiMenuItem: {
                    styleOverrides: {
                        root: {
                            borderRadius: 4,
                            margin: '0 4px',
                            width: 'calc(100% - 8px)',
                        },
                    },
                },
                MuiSwitch: {
                    styleOverrides: {
                        root: {
                            padding: 4,
                            '& .MuiSwitch-track': {
                                borderRadius: 22 / 2,
                            },
                        },
                    },
                },
                MuiDialog: {
                    styleOverrides: {
                        paper: {
                            borderRadius: 12,
                        },
                    },
                },
                MuiPopover: {
                    styleOverrides: {
                        paper: {
                            borderRadius: 12,
                        },
                    },
                },
                MuiDivider: {
                    styleOverrides: {
                        root: {
                            borderColor: mode === 'dark' ? 'rgba(255, 255, 255, 0.12)' : 'rgba(0, 0, 0, 0.08)',
                        },
                    },
                },
            },
            breakpoints: {
                values: {
                    xs: 0,    // Extra small devices
                    sm: 600,  // Small devices
                    md: 960,  // Medium devices
                    lg: 1280, // Large devices
                    xl: 1920, // Extra large devices
                },
            },
            shadows: [
                'none',
                '0 1px 3px 0 rgba(0, 0, 0, 0.1), 0 1px 2px 0 rgba(0, 0, 0, 0.06)',
                '0 3px 6px 0 rgba(0, 0, 0, 0.1), 0 2px 4px 0 rgba(0, 0, 0, 0.06)',
                '0 4px 8px 0 rgba(0, 0, 0, 0.1), 0 2px 6px 0 rgba(0, 0, 0, 0.06)',
                '0 6px 12px 0 rgba(0, 0, 0, 0.1), 0 3px 8px 0 rgba(0, 0, 0, 0.06)',
                '0 8px 16px 0 rgba(0, 0, 0, 0.1), 0 3px 10px 0 rgba(0, 0, 0, 0.06)',
                '0 12px 24px 0 rgba(0, 0, 0, 0.1), 0 4px 12px 0 rgba(0, 0, 0, 0.06)',
                '0 16px 32px 0 rgba(0, 0, 0, 0.1), 0 6px 16px 0 rgba(0, 0, 0, 0.06)',
                '0 20px 40px 0 rgba(0, 0, 0, 0.1), 0 8px 20px 0 rgba(0, 0, 0, 0.06)',
                '0 24px 48px 0 rgba(0, 0, 0, 0.1), 0 10px 24px 0 rgba(0, 0, 0, 0.06)',
                '0 28px 56px 0 rgba(0, 0, 0, 0.1), 0 12px 28px 0 rgba(0, 0, 0, 0.06)',
                '0 32px 64px 0 rgba(0, 0, 0, 0.1), 0 14px 32px 0 rgba(0, 0, 0, 0.06)',
                '0 36px 72px 0 rgba(0, 0, 0, 0.1), 0 16px 36px 0 rgba(0, 0, 0, 0.06)',
                '0 40px 80px 0 rgba(0, 0, 0, 0.1), 0 20px 40px 0 rgba(0, 0, 0, 0.06)',
                '0 44px 88px 0 rgba(0, 0, 0, 0.1), 0 24px 44px 0 rgba(0, 0, 0, 0.06)',
                '0 48px 96px 0 rgba(0, 0, 0, 0.1), 0 28px 48px 0 rgba(0, 0, 0, 0.06)',
                '0 52px 104px 0 rgba(0, 0, 0, 0.1), 0 32px 52px 0 rgba(0, 0, 0, 0.06)',
                '0 56px 112px 0 rgba(0, 0, 0, 0.1), 0 36px 56px 0 rgba(0, 0, 0, 0.06)',
                '0 60px 120px 0 rgba(0, 0, 0, 0.1), 0 40px 60px 0 rgba(0, 0, 0, 0.06)',
                '0 64px 128px 0 rgba(0, 0, 0, 0.1), 0 44px 64px 0 rgba(0, 0, 0, 0.06)',
                '0 68px 136px 0 rgba(0, 0, 0, 0.1), 0 48px 68px 0 rgba(0, 0, 0, 0.06)',
                '0 72px 144px 0 rgba(0, 0, 0, 0.1), 0 52px 72px 0 rgba(0, 0, 0, 0.06)',
                '0 76px 152px 0 rgba(0, 0, 0, 0.1), 0 56px 76px 0 rgba(0, 0, 0, 0.06)',
                '0 80px 160px 0 rgba(0, 0, 0, 0.1), 0 60px 80px 0 rgba(0, 0, 0, 0.06)',
                '0 84px 168px 0 rgba(0, 0, 0, 0.1), 0 64px 84px 0 rgba(0, 0, 0, 0.06)',
            ],
            transitions: {
                easing: {
                    easeInOut: 'cubic-bezier(0.4, 0, 0.2, 1)',
                    easeOut: 'cubic-bezier(0.0, 0, 0.2, 1)',
                    easeIn: 'cubic-bezier(0.4, 0, 1, 1)',
                    sharp: 'cubic-bezier(0.4, 0, 0.6, 1)',
                },
                duration: {
                    shortest: 150,
                    shorter: 200,
                    short: 250,
                    standard: 300,
                    complex: 375,
                    enteringScreen: 225,
                    leavingScreen: 195,
                },
            },
            zIndex: {
                mobileStepper: 1000,
                speedDial: 1050,
                appBar: 1100,
                drawer: 1200,
                modal: 1300,
                snackbar: 1400,
                tooltip: 1500,
            },
        });
    }, [mode, primaryColor, secondaryColor]);

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