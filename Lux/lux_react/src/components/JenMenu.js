import React, { useState, useEffect } from "react";
import Paper from '@mui/material/Paper';

import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Tooltip from '@mui/material/Tooltip';
import { useTheme } from '@mui/material/styles';
import { styled } from '@mui/system';

function JenMenu( { menuName, panelSize } ) {  // menuName is name of menu in scene file
    const [menuChoices, setMenuChoices] = useState([]);
    const [selectedMenuChoice, setSelectedMenuChoice] = useState( '' );
    const [menuLabel, setMenuLabel] = useState( '' );
    const [description, setDescription] = useState( '' );

    const theme = useTheme();

    useEffect(() => {
        if (window.Module) {
            const choices = window.Module.get_menu_choices(menuName); 
            console.log( "Menu choices=" + choices );
            const choicesArray = choices.split(','); // Splitting the string into an array
            setMenuChoices(choicesArray);
            setSelectedMenuChoice(window.Module.get_initial_menu_choice(menuName));
            setMenuLabel(window.Module.get_menu_label(menuName));
            setDescription(window.Module.get_menu_description(menuName));
        } else {
            // Poll for the Module to be ready
            const intervalId = setInterval(() => {
            if (window.Module) {
                const choices = window.Module.get_menu_choices(menuName); 
                const choicesArray = choices.split(','); // Splitting the string into an array
                setMenuChoices(choicesArray);
                setSelectedMenuChoice(window.Module.get_initial_menu_choice(menuName));
                setMenuLabel(window.Module.get_menu_label(menuName));
                setDescription(window.Module.get_menu_description(menuName));
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
        <Paper elevation={3} sx={{ display: 'flex', justifyContent: 'center', alignItems: 'center', width: panelSize, height: 60 }}>
            <Stack sx={stackStyles}>
                {menuLabel && (
                    <Tooltip title={description}>
                        <Typography sx={typographyStyles}>
                            {menuLabel}
                        </Typography>
                    </Tooltip>
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
        </Paper>
    );
}

export default JenMenu;

