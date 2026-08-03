import { createTheme } from '@mui/material';

// Central design system for the Jen UI. All component styling that isn't
// layout-specific belongs here so the app stays visually consistent.

const theme = createTheme({
    palette: {
        mode: 'dark',
        background: {
            default: '#0b0d10',
            paper: '#14171c',
        },
        primary: {
            main: '#a78bfa',
            light: '#c4b5fd',
            dark: '#7c5df0',
            contrastText: '#0b0d10',
        },
        secondary: {
            main: '#22d3ee',
            contrastText: '#0b0d10',
        },
        divider: 'rgba(148, 163, 184, 0.12)',
        text: {
            primary: '#e6e8ee',
            secondary: '#9aa3b2',
        },
    },
    shape: {
        borderRadius: 10,
    },
    typography: {
        fontFamily: [
            'Inter',
            '-apple-system',
            'BlinkMacSystemFont',
            'Segoe UI',
            'Roboto',
            'Helvetica Neue',
            'Arial',
            'sans-serif',
        ].join(','),
        button: {
            textTransform: 'none',
            fontWeight: 600,
        },
        subtitle2: {
            fontWeight: 600,
            letterSpacing: 0.2,
        },
    },
    components: {
        MuiCssBaseline: {
            styleOverrides: {
                body: {
                    scrollbarColor: 'rgba(148,163,184,0.35) transparent',
                },
                '*::-webkit-scrollbar': {
                    width: 8,
                    height: 8,
                },
                '*::-webkit-scrollbar-thumb': {
                    backgroundColor: 'rgba(148,163,184,0.25)',
                    borderRadius: 8,
                },
                '*::-webkit-scrollbar-thumb:hover': {
                    backgroundColor: 'rgba(148,163,184,0.45)',
                },
                '*::-webkit-scrollbar-track': {
                    background: 'transparent',
                },
            },
        },
        MuiPaper: {
            styleOverrides: {
                root: {
                    backgroundImage: 'none',
                },
            },
        },
        MuiTooltip: {
            defaultProps: {
                arrow: true,
            },
        },
        MuiTab: {
            styleOverrides: {
                root: {
                    minHeight: 56,
                    minWidth: 50,
                    padding: '6px 6px',
                    fontSize: 10.5,
                    fontWeight: 600,
                    letterSpacing: 0.3,
                    color: '#9aa3b2',
                    '&.Mui-selected': {
                        color: '#c4b5fd',
                    },
                },
            },
        },
        MuiTabs: {
            styleOverrides: {
                indicator: {
                    height: 3,
                    borderRadius: '3px 3px 0 0',
                },
            },
        },
        MuiSlider: {
            styleOverrides: {
                root: {
                    height: 4,
                },
                thumb: {
                    width: 14,
                    height: 14,
                    '&:hover, &.Mui-focusVisible': {
                        boxShadow: '0 0 0 6px rgba(167, 139, 250, 0.16)',
                    },
                },
                rail: {
                    opacity: 0.25,
                },
            },
        },
        MuiIconButton: {
            styleOverrides: {
                root: {
                    borderRadius: 8,
                },
            },
        },
    },
});

export default theme;
