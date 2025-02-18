import React, { useState } from 'react';
import Button from '@mui/material/Button';
import Grid from '@mui/material/Grid';

const directionToNumber4Diagonal = (directionString) => {
    const mapping = {
        "up_right": 0,
        "down_right": 1,
        "down_left": 2,
        "up_left": 3
    };

    return mapping[directionString] ?? -1; // Return -1 if directionString is not in mapping
};

function JenDirection4Diagonal( { json } ) {
    const [ direction, setDirection ] = useState( directionToNumber4Diagonal( json.value ?? "down" ) );

    const handleDirectionChange = ( newDirection ) => {
        setDirection( newDirection );
        window.module.pick_direction4_diagonal( json.name, newDirection );
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
            <Grid item xs={4}>{renderDirectionButton(3, '↖')}</Grid>
            <Grid item xs={4}></Grid>
            <Grid item xs={4}>{renderDirectionButton(0, '↗')}</Grid>

            <Grid item xs={4}></Grid>
            <Grid item xs={4}><div style={{ height: '20px' }}></div></Grid>
            <Grid item xs={4}></Grid>

            <Grid item xs={4}>{renderDirectionButton(2, '↙')}</Grid>
            <Grid item xs={4}></Grid>
            <Grid item xs={4}>{renderDirectionButton(1, '↘')}</Grid>
        </Grid>
    );
}

export default JenDirection4Diagonal;