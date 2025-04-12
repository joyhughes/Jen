import React, { useState, useEffect } from 'react';
import Button from '@mui/material/Button';
import Box from '@mui/material/Box';
import { styled } from '@mui/material/styles';

const Circle = styled('div')(({ filled }) => ({
    width: '16px',
    height: '16px',
    borderRadius: '50%',
    backgroundColor: filled ? 'white' : 'transparent',
    border: '2px solid white',
    margin: '2px auto', // Center the circle within the button
}));

function JenFunkyPicker({ json }) {
    const [funky, setFunky] = useState(BigInt(json.value) ?? BigInt("0xffffffffaa00aa00"));
//    const [funky, setFunky] = useState(json.value ?? 18446744072266754560n );

    useEffect(() => {
        // pass value as hexidecimal string
        window.module.pick_funk_factor(json.name, '0x' + funky.toString(16));
    }, [funky, json.name]);

    const funkItUp = (index) => {
        setFunky((prevFunky) => prevFunky ^ (BigInt(1) << BigInt(index)));
    };

    const renderButton = (index) => {
        const isBitSet = (funky & (BigInt(1) << BigInt(index))) !== BigInt(0);
        //    const isBitSet = true;
        return (
            <Box key={index} sx={{ display: 'flex', justifyContent: 'center', alignItems: 'center' }}>
                <Button
                    style={{ width: '20px', height: '20px', padding: 0, minWidth: 0 }}
                    onClick={() => funkItUp(index)}
                >
                    <Circle filled={isBitSet} />
                </Button>
            </Box>
        );
    };

    return (
        <Box
            sx={{
                display: 'grid',
                gridTemplateColumns: 'repeat(8, 20px)',
                gridTemplateRows: 'repeat(8, 20px)',
                gap: 0,
                width: '160px',
                height: '160px',
                border: '1px solid white',
            }}
        >
            {Array.from({ length: 64 }, (_, index) => renderButton(index))}
        </Box>
    );
}

export default JenFunkyPicker;