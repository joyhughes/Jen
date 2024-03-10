import React, { useState } from 'react';
import Button from '@mui/material/Button';
import Grid from '@mui/material/Grid';

const directionToNumber4 = (directionString) => {
    const mapping = {
        "up": 0,
        "right": 1,
        "down": 2,
        "left": 3
    };

    return mapping[directionString] ?? -1; // Return -1 if directionString is not in mapping
};

function JenDirection4( { json } ) {
    const [ direction, setDirection ] = useState( directionToNumber4( json.value ) );

    const handleDirectionChange = ( newDirection ) => {
        setDirection( newDirection );
        window.Module.pick_direction4( json.name, newDirection );
    };

    const renderDirectionButton = ( dirValue, label ) => {
        return (
            <Button
                variant={ direction === dirValue ? "contained" : "outlined" }
                onClick={ () => handleDirectionChange( dirValue ) }
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