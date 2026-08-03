import React, { useState } from 'react';
import { Box, Button, useTheme, Tooltip } from '@mui/material';
import {
    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,
    ArrowUpLeft,
    ArrowUpRight,
    ArrowDownLeft,
    ArrowDownRight
} from 'lucide-react';

const DIRECTIONS = {
    up: { icon: ArrowUp, label: 'Up' },
    down: { icon: ArrowDown, label: 'Down' },
    left: { icon: ArrowLeft, label: 'Left' },
    right: { icon: ArrowRight, label: 'Right' },
    up_left: { icon: ArrowUpLeft, label: 'Up-Left' },
    up_right: { icon: ArrowUpRight, label: 'Up-Right' },
    down_left: { icon: ArrowDownLeft, label: 'Down-Left' },
    down_right: { icon: ArrowDownRight, label: 'Down-Right' },
};

// Each variant: the module function to call, the widget's direction->number
// encoding, its default direction, and which cells of a 3x3 grid are active.
const VARIANTS = {
    direction_picker_4: {
        moduleFn: 'pick_direction4',
        numbers: { up: 0, right: 1, down: 2, left: 3 },
        defaultDirection: 'down',
        grid: [
            [null, 'up', null],
            ['left', null, 'right'],
            [null, 'down', null],
        ],
    },
    direction_picker_4_diagonal: {
        moduleFn: 'pick_direction4_diagonal',
        numbers: { up_right: 0, down_right: 1, down_left: 2, up_left: 3 },
        defaultDirection: 'up_right',
        grid: [
            ['up_left', null, 'up_right'],
            [null, null, null],
            ['down_left', null, 'down_right'],
        ],
    },
    direction_picker_8: {
        moduleFn: 'pick_direction8',
        numbers: {
            up: 0, up_right: 1, right: 2, down_right: 3,
            down: 4, down_left: 5, left: 6, up_left: 7,
        },
        defaultDirection: 'down',
        grid: [
            ['up_left', 'up', 'up_right'],
            ['left', null, 'right'],
            ['down_left', 'down', 'down_right'],
        ],
    },
};

// Single-select direction picker for all direction_picker_* widget types.
function JenDirectionPicker({ json }) {
    const theme = useTheme();
    const variant = VARIANTS[json.type] ?? VARIANTS.direction_picker_4;
    const [direction, setDirection] = useState(() =>
        variant.numbers[json?.value] ?? variant.numbers[variant.defaultDirection]
    );

    const handleDirectionChange = (newDirection) => {
        setDirection(newDirection);
        if (window.module) {
            window.module[variant.moduleFn](json.name, newDirection);
        }
    };

    return (
        <Box
            sx={{
                display: 'inline-grid',
                gridTemplateColumns: 'repeat(3, 36px)',
                gap: 0.5,
            }}
        >
            {variant.grid.flat().map((dir, i) => {
                if (!dir) {
                    return <Box key={i} sx={{ width: 36, height: 36 }} />;
                }
                const { icon: Icon, label } = DIRECTIONS[dir];
                const value = variant.numbers[dir];
                const isSelected = direction === value;
                return (
                    <Tooltip key={i} title={label} arrow placement="top">
                        <Button
                            variant={isSelected ? 'contained' : 'outlined'}
                            onClick={() => handleDirectionChange(value)}
                            sx={{
                                minWidth: 36,
                                width: 36,
                                height: 36,
                                padding: 0,
                                borderColor: isSelected ? 'transparent' : theme.palette.divider,
                                color: isSelected ? 'white' : theme.palette.text.secondary,
                                boxShadow: isSelected ? 2 : 0,
                                transition: 'all 0.2s',
                                '&:hover': {
                                    backgroundColor: isSelected
                                        ? theme.palette.primary.main
                                        : theme.palette.action.hover,
                                    borderColor: isSelected
                                        ? 'transparent'
                                        : theme.palette.primary.light,
                                },
                            }}
                        >
                            <Icon size={16} />
                        </Button>
                    </Tooltip>
                );
            })}
        </Box>
    );
}

export default JenDirectionPicker;
