import React, { useState } from "react";
import Switch from '@mui/material/Switch';
import ToggleButton from "@mui/material/ToggleButton";
import Checkbox from "@mui/material/Checkbox";
import FormControlLabel from "@mui/material/FormControlLabel";

function JenSwitch({ json }) {
    const [switchValue, setSwitchValue] = useState(json.default_value || false);

    const handleSwitchChange = (event) => {
        setSwitchValue(event.target.checked);
        // Additional logic if needed to handle switch value change
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
                    <FormControlLabel
                        control={<Switch checked={switchValue} onChange={handleSwitchChange} />}
                        label={json.label}
                    />
                );
            case 'toggle_button':
                return (
                    <ToggleButton
                        value="check"
                        selected={switchValue}
                        onChange={handleSwitchChange}
                    >
                        {json.label}
                    </ToggleButton>
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