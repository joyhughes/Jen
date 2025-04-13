import React, { useState } from "react";
import WidgetContainer from './WidgetContainer';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Tooltip from '@mui/material/Tooltip';
import JenMenu from './JenMenu';
import JenSlider from './JenSlider';
import JenSwitch from './JenSwitch';
import JenDirection8 from './JenDirection8';
import JenDirection4 from './JenDirection4';
import JenDirection4Diagonal from './JenDirection4Diagonal';
import JenBlurPicker from './JenBlurPicker';
import JenMultiDirection8 from './JenMultiDirection8';
import JenFunkyPicker from './JenFunkyPicker';
import IconButton from '@mui/material/IconButton';
import CloseIcon from '@mui/icons-material/Close';
import AddIcon from '@mui/icons-material/Add';
import {ImagePicker} from "./ImagePicker.jsx";

function WidgetGroup({ panelSize, json, onChange }) {
  const [ renderCount, setRenderCount ] = useState(0);

  const renderWidget = ( name ) => {  // name of widget
    let widgetComponent;
    let widget;
    let height = 60;

    const widgetJSON = window.module.get_widget_JSON( name );

    try {
      widget = JSON.parse( widgetJSON );
    } catch (error) {
      console.error("renderWidget Error parsing JSON:", error);
      // Handle the error appropriately
    }

    if (!widget) {
      console.error(`No widget found for name: ${name}`);
    }
    let labelAndDescription =             
      <Tooltip title={widget.description ?? ''} placement="top" disableInteractive >
        <Typography variant="subtitle1" component="div">
          {widget.label}
        </Typography>
      </Tooltip>;

    switch ( widget.type ) {
      case 'menu_int':
      case 'menu_string':
        if (widget.tool === 'image') {
          widgetComponent = <ImagePicker json={widget} width={panelSize - 40} onChange={onChange} />;
          height = 220;
        } else {
          widgetComponent = <JenMenu json={widget} width={panelSize - 40} onChange={onChange} />;
        }
        break;
      case 'slider_int':
      case 'slider_float':
      case 'range_slider_int':
      case 'range_slider_float':
        widgetComponent =       
          <Stack spacing={-0.5} direction="column" alignItems="center">
            { labelAndDescription }
            <JenSlider key = { widget.name } json = { widget } width = { panelSize - 75 } />
          </Stack>;
      break;
      case 'switch_fn':
      case 'switch_condition':
        height = 30;
        widgetComponent =           
          <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
            <JenSwitch key={widget.name} json={widget} size = { "small" } onChange = { onChange } />
            { labelAndDescription }
          </Stack>;
      break;
      case 'direction_picker_8':
        widgetComponent =           
          <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
            <JenDirection8 key={widget.name} json={widget} />
            { labelAndDescription }
          </Stack>;
      break;
      case 'direction_picker_4':
        widgetComponent =           
          <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
            <JenDirection4 key={ widget.name } json={ widget } />
            { labelAndDescription }
          </Stack>;
      break;
      case 'direction_picker_4_diagonal':
        widgetComponent =           
          <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
            <JenDirection4Diagonal key={ widget.name } json={ widget } />
            { labelAndDescription }
          </Stack>;
      break;
      case 'box_blur_picker':
        height = 30;
        widgetComponent =           
          <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
            <JenBlurPicker key={ widget.name } json={ widget } />
            { labelAndDescription }
          </Stack>;
      break;
      case 'funk_factor_picker':
        height = 160;
        widgetComponent = <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
          <JenFunkyPicker key={ widget.name } json={ widget } />
          { labelAndDescription }
        </Stack>;
      break;
      case 'custom_blur_picker':
        console.log( "WidgetGroup custom_blur_picker widget=" + JSON.stringify( widget ) );
        height = 40 + widget.pickers.length * 70;
        const pickerElements = [];
        for (let i = 0; i < widget.pickers.length; i++) {

          const handleClose = () => {
            // Call the close handler with widget.name and row index
            window.module.remove_custom_blur_pickers(widget.name, i);
            setRenderCount( renderCount + 1 );
          };

          const picker = widget.pickers[i];
          const element = (
            <React.Fragment key={i}>
              <Stack spacing={1} direction="row" alignItems="center">
                <JenMultiDirection8
                  name={widget.name}
                  value={picker[0]}
                  code={i}
                  key={ i + renderCount * 256 }
                />
                <JenMultiDirection8
                  name={widget.name}
                  value={picker[1]}
                  code={i + 128}
                  key={ i + 128 + renderCount * 256 }

                />
                <IconButton size="small" onClick={handleClose}>
                  <CloseIcon fontSize="small" />
                </IconButton>
              </Stack>
            </React.Fragment>
          );
          pickerElements.push(element);
        }

        const handleAddPicker = () => {
          // Call the add handler with widget.name
          window.module.add_custom_blur_pickers(widget.name);
          setRenderCount( renderCount + 1 );
        };

        widgetComponent =       
          <Stack spacing={-0.5} direction="column" alignItems="center">
            { labelAndDescription }
            <Stack spacing={1} direction="column" alignItems="center">
              {pickerElements}
              <IconButton size="small" onClick={handleAddPicker}>
                <AddIcon fontSize="small" />
              </IconButton>
            </Stack>          
          </Stack>;
      break;
      case 'widget_switch_fn':
        let ws_widget;
        const ws_widgetJSON = window.module.get_widget_JSON( widget.widget );
        try {
          ws_widget = JSON.parse( ws_widgetJSON );
        } catch (error) {
          console.error("Error parsing JSON:", error);
        }
        const switcherJSON = window.module.get_widget_JSON( widget.switcher );
        let switcher;
        try {
          switcher = JSON.parse( switcherJSON );
        } catch (error) {
          console.error("Error parsing JSON:", error);
        }
        switch( ws_widget.type ) {
          case 'slider_int':
          case 'slider_float':
          case 'range_slider_int':
          case 'range_slider_float':   
            widgetComponent = 
              <Stack spacing={-0.5} direction="column" alignItems="center">
              <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
                <JenSwitch key={switcher.name} json={switcher} size={"small"} />
                { labelAndDescription }
              </Stack>
              <JenSlider key={ws_widget.name} json={ws_widget} width={panelSize - 75} />
            </Stack>;
          break;
          default:
            widgetComponent = <div key={ ws_widget.name }>Widget Switch - Unknown widget type: { ws_widget.type }</div>;
          break;
        }
      break;
      default:
        widgetComponent = <div key={ widget.name }>Unknown widget type: { widget.type }</div>;
        break;
    }

    return (
      <WidgetContainer key = { widget.name } panelSize = { panelSize } height = { height } >
        { widgetComponent }
      </WidgetContainer>
    );
  };

  return (
      json.widgets.map( renderWidget )
  );  
}
  
export default WidgetGroup;
