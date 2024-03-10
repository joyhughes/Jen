import React, { useState } from "react";
import Switch from '@mui/material/Switch';
import ToggleButton from "@mui/material/ToggleButton";
import Checkbox from "@mui/material/Checkbox";
import FormControlLabel from "@mui/material/FormControlLabel";

function JenSwitch({ json, onChange }) {
    const [switchValue, setSwitchValue] = useState( json.value ?? false );

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
