import React, { useState } from "react";
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import Radio from '@mui/material/Radio';
import RadioGroup from '@mui/material/RadioGroup';
import InputLabel from '@mui/material/InputLabel';
import FormControl from '@mui/material/FormControl';
import FormControlLabel from '@mui/material/FormControlLabel';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import { useTheme } from '@mui/material/styles';

function JenMenu({ json, width, onChange }) {
    const [selectedMenuChoice, setSelectedMenuChoice] = useState(json.default_choice || '');

    const theme = useTheme();

    //console.log( "JenMenu json=" + JSON.stringify(json) );

    const handleMenuChange = (event) => {
        console.log( "JenMenu handleMenuChange event.target.value = " + event.target.value + " json.name = " + json.name + " json.affects_widget_groups = " + json.affects_widget_groups );
        window.Module.handle_menu_choice(json.name, event.target.value);
        setSelectedMenuChoice(event.target.value);
        if ( json.affects_widget_groups ) {
            onChange();
        }
    };

    const renderMenu = () => {
        if (json.tool === 'pull_down') {
            return (
                <FormControl sx={{ m: 1, minWidth: `${width}px` }} size="small">
                    <InputLabel size="normal" >{json.label}</InputLabel>
                    <Select
                        style={{ width: `${width}px` }}
                        value = { selectedMenuChoice }
                        label = { json.label }
                        onChange = { handleMenuChange }
                        displayEmpty
                        inputProps = { { 'aria-label': 'Without label' } }
                        MenuProps={{
                            PaperProps: {
                                style: {
                                    width: `${width}px`, // This ensures the dropdown has a constant width
                                },
                            },
                        }}
                    >
                        { json.items.map((choice, index) => (
                            <MenuItem key={index} value={index}>
                                {choice}
                            </MenuItem>
                        ))}
                    </Select>
                </FormControl>
            );
        } else if (json.tool === 'radio') {
            return (
                <RadioGroup
                    value={selectedMenuChoice}
                    onChange={handleMenuChange}
                >
                    {json.items.map((choice, index) => (
                        <FormControlLabel 
                            key={index} 
                            value={index} 
                            control={<Radio />} 
                            label={choice} 
                        />
                    ))}
                </RadioGroup>
            );
        } else {
            return <div>Unknown menu type: {json.tool}</div>;
        }
    };

    return (
            renderMenu()
    );
}

export default JenMenu;

