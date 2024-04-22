import React, { useState } from 'react';
import Button from '@mui/material/Button';
import Grid from '@mui/material/Grid';
import Tooltip from '@mui/material/Tooltip';

const blurMethodToNumber = (blurString) => {
    const mapping = {
        "orthogonal": 0,
        "diagonal": 1,
        "all": 2,
        "custom": 3
    };

    return mapping[blurString] ?? -1; // Return -1 if blurString is not in mapping
};

function JenBlurPicker({ json }) {
    const [blurMethod, setBlurMethod] = useState(blurMethodToNumber(json.value ?? "orthogonal"));

    const handleBlurMethodChange = (newMethod) => {
        setBlurMethod(newMethod);
        // Assuming window.Module.pick_blurMethod is the method to update the blur type
        window.Module.pick_blur_method(json.name, newMethod);
    };

    const renderBlurButton = (typeValue, label) => {
        return (
            <Button
                variant={blurMethod === typeValue ? "contained" : "outlined"}
                onClick={() => handleBlurMethodChange(typeValue)}
                style={{ minWidth: '20px', height: '20px', padding: '0px' }}
            >
                {label}
            </Button>
        );
    };

    return (
        <Grid container spacing={0} style={{ width: '80px' }}>
            <Tooltip title={ 'orthogonal' } placement="top" disableInteractive  >
                <Grid item>{renderBlurButton(0, '+')}</Grid>
            </Tooltip>
            <Tooltip title={ 'diagonal' } placement="top" disableInteractive  >
                <Grid item>{renderBlurButton(1, '×')}</Grid>
            </Tooltip>
            <Tooltip title={ 'all' } placement="top" disableInteractive  >
                <Grid item>{renderBlurButton(2, '*')}</Grid>
            </Tooltip>            
            <Tooltip title={ 'custom' } placement="top" disableInteractive  >
                <Grid item>{renderBlurButton(3, 'C')}</Grid>
            </Tooltip>
        </Grid>
    );
}

export default JenBlurPicker;