import React, { useState } from 'react';
import Button from '@mui/material/Button';
import Grid from '@mui/material/Grid';

const directionToNumber8 = (directionString) => {
    const mapping = {
        "up": 0,
        "up_right": 1,
        "right": 2,
        "down_right": 3,
        "down": 4,
        "down_left": 5,
        "left": 6,
        "up_left": 7
    };

    return mapping[directionString] ?? -1; // Return -1 if directionString is not in mapping
};

function JenDirection8({ json }) {
    const [direction, setDirection] = useState(directionToNumber8(json.value));

    const handleDirectionChange = (newDirection) => {
        setDirection(newDirection);
        window.Module.pick_direction8(json.name, newDirection);
    };

    //console.log("JenDirection8 direction=" + direction);
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
            <Grid item xs={4}>{renderDirectionButton(7, '↖')}</Grid>
            <Grid item xs={4}>{renderDirectionButton(0, '↑')}</Grid>
            <Grid item xs={4}>{renderDirectionButton(1, '↗')}</Grid>

            <Grid item xs={4}>{renderDirectionButton(6, '←')}</Grid>
            <Grid item xs={4}>{/* Center Button */}</Grid>
            <Grid item xs={4}>{renderDirectionButton(2, '→')}</Grid>

            <Grid item xs={4}>{renderDirectionButton(5, '↙')}</Grid>
            <Grid item xs={4}>{renderDirectionButton(4, '↓')}</Grid>
            <Grid item xs={4}>{renderDirectionButton(3, '↘')}</Grid>
        </Grid>
    );
}

export default JenDirection8;