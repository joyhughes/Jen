import React from 'react';
import Paper from '@mui/material/Paper';
import Tooltip from '@mui/material/Tooltip';
import IconButton from '@mui/material/IconButton';
import InfoIcon from '@mui/icons-material/Info';
import { useTheme } from '@mui/material/styles';

const WidgetContainer = ({ description = '', panelSize, height = 60, children, sx = {} }) => {
    const theme = useTheme();

    return (
        <Paper
            elevation={3}
            sx={{
                display: 'flex',
                justifyContent: 'center',
                alignItems: 'center',
                width: panelSize,
                height: height,
                position: 'relative',
                marginBottom: 2,
                ...sx
            }}
        >
            {description && (
                <Tooltip title={description} placement="top" disableInteractive>
                    <IconButton sx={{ position: 'absolute', top: 0, right: 0 }}>
                        <InfoIcon sx={{ fontSize: '1rem', color: theme.palette.primary.main }} />
                    </IconButton>
                </Tooltip>
            )}
            {children}
        </Paper>
    );
};

export default WidgetContainer;