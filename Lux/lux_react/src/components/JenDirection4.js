import React, { useState } from 'react';
import Button from '@mui/material/Button';
import Grid from '@mui/material/Grid';

function JenDirection4({ json }) {
    const [direction, setDirection] = useState(json.default_value || -1);

    const handleDirectionChange = (newDirection) => {
        setDirection(newDirection);
        window.Module.pick_direction4(json.name, newDirection);
    };

    const renderDirectionButton = (dirValue, label) => {
        return (
            <Button
                variant={direction === dirValue ? "contained" : "outlined"}
                onClick={() => handleDirectionChange(dirValue)}
                style={{ minWidth: '20px', height: '20px', padding: '0px' }}
            >
                {label}
            </Button>
        );
    };

    return (
        <Grid container spacing={0} style={{ width: '60px' }}>
            <Grid item xs={4}></Grid>
            <Grid item xs={4}>{renderDirectionButton(0, '↑')}</Grid>
            <Grid item xs={4}></Grid>

            <Grid item xs={4}>{renderDirectionButton(3, '←')}</Grid>
            <Grid item xs={4}>{/* Center Button */}</Grid>
            <Grid item xs={4}>{renderDirectionButton(1, '→')}</Grid>

            <Grid item xs={4}></Grid>
            <Grid item xs={4}>{renderDirectionButton(2, '↓')}</Grid>
            <Grid item xs={4}></Grid>
        </Grid>
    );
}

export default JenDirection4;