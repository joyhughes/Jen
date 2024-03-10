import React, { useState } from "react";
import Switch from '@mui/material/Switch';
import ToggleButton from "@mui/material/ToggleButton";
import Checkbox from "@mui/material/Checkbox";
import FormControlLabel from "@mui/material/FormControlLabel";

function JenSwitch({ json, onChange }) {
    const [switchValue, setSwitchValue] = useState(json.value || false);

    const handleSwitchChange = (event) => {
        setSwitchValue(event.target.checked);
        window.Module.handle_switch_value(json.name, event.target.checked);
        if ( json.affects_widget_groups ) {
            onChange();
        }
    };

    const renderSwitch = () => {
        switch (json.tool) {
            case 'checkbox':
                return (
                    <FormControlLabel
                        control={<Checkbox checked={switchValue} onChange={handleSwitchChange} />}
                        label={json.label}
                    />
                );
            case 'switch':
                return (
                    <Switch checked={switchValue} onChange={handleSwitchChange} size={"small"}/>
                );
            default:
                return <div>Unknown switch type: {json.tool}</div>;
        }
    };

    return (
        <div>
            {renderSwitch()}
        </div>
    );
}

export default JenSwitch;

/*
import React, { useState, useEffect } from "react";
import Switch from '@mui/material/Switch';
import ToggleButton from "@mui/material/ToggleButton";
import Checkbox from "@mui/material/Checkbox";


function JenSwitch( { switchName, switchType } ) {  // switchName is name of switch in scene file
    const [switchValue, setSwitchValue] = useState( false );
    const [switchLabel, setSwitchLabel] = useState( '' );

    function setupSwitch() {
        setSwitchValue( window.Module.get_switch_value(switchName) );
        setSwitchLabel( window.Module.get_switch_label(switchName) );
    };

    useEffect(() => {
        if (window.Module) {
            setupSwitch();
        } else {
            // Poll for the Module to be ready
            const intervalId = setInterval(() => {
            if (window.Module) {
                setupSwitch();
                clearInterval(intervalId);
            }
          }, 100); // Check every 100ms
    
          return () => clearInterval(intervalId);
        }
    }, [] );

    const handleSwitchChange = (event) => {
        setSwitchValue( event.target.checked );
        if (window.Module) {
            window.Module.handle_switch_value( switchName, event.target.checked ); 
        }
    };

    return (
        <div>
            <label>{switchLabel}</label>
            <input type="checkbox" checked={switchValue} onChange={handleSwitchChange} />
        </div>
    );
}

export default JenSwitch;
*/