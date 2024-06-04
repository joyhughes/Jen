import React, { useState } from 'react';
import Button from '@mui/material/Button';
import Grid from '@mui/material/Grid';

function JenMultiDirection8({ name, value, code }) {
  const [directionsByte, setDirectionsByte] = useState( value ?? 0 );

  const handleDirectionChange = (dirIndex) => {
    // Toggles the selected direction's bit
    const updatedDirectionsByte = directionsByte ^ (1 << dirIndex);
    setDirectionsByte(updatedDirectionsByte);

    // Call the external function with the updated byte value
    window.module.pick_multi_direction8( name, updatedDirectionsByte, code ?? 0 );
  };

  const isDirectionActive = (dirIndex) => {
    return (directionsByte & (1 << dirIndex)) !== 0;
  };

  const renderDirectionButton = (dirIndex, label) => {
    return (
      <Button
        variant={isDirectionActive(dirIndex) ? "contained" : "outlined"}
        onClick={() => handleDirectionChange(dirIndex)}
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

export default JenMultiDirection8;