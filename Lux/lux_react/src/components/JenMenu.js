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

function JenMenu({ json, width }) {
    const [selectedMenuChoice, setSelectedMenuChoice] = useState(json.default_choice || '');

    const theme = useTheme();

    //console.log( "JenMenu json=" + JSON.stringify(json) );

    const handleMenuChange = (event) => {
        //console.log( "JenMenu handleMenuChange event.target.value=" + event.target.value + " json.name=" + json.name);
        window.Module.handle_menu_choice(json.name, event.target.value);
        setSelectedMenuChoice(event.target.value);
        // Additional logic if needed to handle menu choice change
    };

    const renderMenu = () => {
        if (json.tool === 'pull_down') {
            return (
                <FormControl sx={{ m: 1, minWidth: width }} size="small">
                    <InputLabel size="normal" >{json.label}</InputLabel>
                    <Select
                        style={{ width: width }}
                        value = { selectedMenuChoice }
                        label = { json.label }
                        onChange = { handleMenuChange }
                        displayEmpty
                        inputProps = { { 'aria-label': 'Without label' } }
                    >
                        { json.choices.map((choice, index) => (
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
                    {json.choices.map((choice, index) => (
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


/*
import React, { useState, useEffect } from "react";
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import { useTheme } from '@mui/material/styles';

function JenMenu( { menuName } ) {  // menuName is name of menu in scene file
    const [menuChoices, setMenuChoices] = useState([]);
    const [selectedMenuChoice, setSelectedMenuChoice] = useState( '' );
    const [menuLabel, setMenuLabel] = useState( '' );
    const [description, setDescription] = useState( '' );

    const theme = useTheme();

    function setupMenu() {
        const choices = window.Module.get_menu_choices(menuName); 
        const choicesArray = choices.split(','); // Splitting the string into an array
        setMenuChoices(choicesArray);
        setSelectedMenuChoice(window.Module.get_default_menu_choice(menuName));
        setMenuLabel(window.Module.get_menu_label(menuName));
        setDescription(window.Module.get_menu_description(menuName));
    };

    useEffect(() => {
        if (window.Module) {
            setupMenu();
        } else {
            // Poll for the Module to be ready
            const intervalId = setInterval(() => {
            if (window.Module) {
                setupMenu();
                clearInterval(intervalId);
            }
          }, 100); // Check every 100ms
    
          return () => clearInterval(intervalId);
        }
    }, [] );
    
    const handleMenuChange = (event) => {
        setSelectedMenuChoice( event.target.value );
        //console.log( "ControlPanel handleMenuChange event.target.value=" + event.target.value );
        if (window.Module) {
            window.Module.handle_menu_choice( menuName, event.target.value ); 
        }
    };

     // Adjusted styles for the Stack component
    const stackStyles = {
        direction: "column",
        justifyContent: "center",
        alignItems: "center",
        spacing: 1, // adjust the spacing as needed
        height: 60, // the height of the Paper component
        paddingY: 0.5, // Optional: Add padding on top and bottom of the stack for better alignment
    };

    // Adjusted styles for the Typography component
    const typographyStyles = {
        fontSize: "1rem", // smaller font size to fit the space
        color: theme.palette.primary.main,
        lineHeight: "1.25rem", // adjust line height to be smaller than font size
    //    marginBottom: "-0.25rem", // adjust bottom margin to tighten up space
    };

    const selectStyles = {
        fontSize: "1rem", // Match font size with label
        "& .MuiSelect-select": {
            paddingTop: "2px", // Reduce padding to lower the Select component
            paddingBottom: "2px", // Reduce padding to align with the bottom
        },
        "& .MuiOutlinedInput-notchedOutline": {
            borderColor: "transparent", // Hide the outline to save space
        },
        // Adjust margin if needed to align with the bottom of the Paper container
        marginY: "auto",
    };

    // Adjusted styles for the MenuItem components
    const menuItemStyles = {
        fontSize: "1rem",
        paddingTop: "4px", // Reduced padding for tighter vertical spacing
        paddingBottom: "4px", // Reduced padding for tighter vertical spacing
        lineHeight: "1rem", // Reduced line-height for tighter vertical spacing
    };
  
    return (
        <Stack sx={stackStyles}>
            {menuLabel && (
                <Typography sx={typographyStyles}>
                    {menuLabel}
                </Typography>
            )}
            <Select
                value={selectedMenuChoice}
                onChange={handleMenuChange}
                displayEmpty
                inputProps={{ 'aria-label': 'Without label' }}
                sx={selectStyles}
            >
                {menuChoices.map((choice, index) => (
                    <MenuItem key={index} value={index} sx={menuItemStyles}>
                    {choice}
                    </MenuItem>
                ))}
            </Select>
        </Stack>
    );
}

export default JenMenu;
*/

