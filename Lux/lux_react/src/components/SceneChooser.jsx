import React, { useState, useEffect } from "react";
import WidgetContainer from './WidgetContainer';
import Select from '@mui/material/Select';
import MenuItem from '@mui/material/MenuItem';
import InputLabel from '@mui/material/InputLabel';
import FormControl from '@mui/material/FormControl';
import Tooltip from '@mui/material/Tooltip';

function SceneChooser({ width, onChange }) {
    const [ sceneListJSON, setSceneListJSON ]   = useState( [] );
    const [ selectedMenuChoice, setSelectedMenuChoice ] = useState( 0 );

    const handleMenuChange = (event) => {
        //console.log( "JenMenu handleMenuChange event.target.value = " + event.target.value + " json.name = " + json.name + " json.affects_widget_groups = " + json.affects_widget_groups );
        onChange();
        window.module.load_scene( sceneListJSON.scenes[ event.target.value ].filename );
        setSelectedMenuChoice(event.target.value);
    };

    const setupSceneList = () => {
        const sceneListJSONString = window.module.get_scene_list_JSON();
      
        try {
          const parsedJSON = JSON.parse(sceneListJSONString);
          setSceneListJSON(parsedJSON);
        } catch (error) {
          console.error("Error parsing scene list JSON:", error);
        }
      };

    useEffect(() => {
        if (window.module) {
            setupSceneList();
        } else {
            // Poll for the Module to be ready
            const intervalId = setInterval(() => {
            if (window.module) {
                setupSceneList();
                clearInterval(intervalId);
            }
          }, 100); // Check every 100ms
    
          return () => clearInterval(intervalId);
        }
      }, [] );

    return (
        <WidgetContainer>
            <Tooltip title={'Choose a scene'} placement="top" disableInteractive >
                <FormControl sx={{ m: 1, minWidth: `${width}px` }} size="small">
                    <InputLabel>{'Scene'}</InputLabel>
                    <Select
                        style={{ width: `${width}px` }}
                        value={selectedMenuChoice}
                        label={'Scene'}
                        onChange={handleMenuChange}
                        displayEmpty
                        inputProps={{ 'aria-label': 'Without label' }}
                        MenuProps={{
                            PaperProps: {
                                style: {
                                    width: `${width}px`,
                                },
                            },
                        }}
                    >
                        {sceneListJSON && sceneListJSON.scenes ? (
                            sceneListJSON.scenes.map((scene, index) => (
                                <MenuItem key={index} value={index}>{scene.name}</MenuItem>
                            ))
                        ) : (
                            <MenuItem value="">
                                <em>Loading...</em>
                            </MenuItem>
                        )}                    
                    </Select>
                </FormControl>
            </Tooltip>
        </WidgetContainer>
    );
}

export default SceneChooser;
