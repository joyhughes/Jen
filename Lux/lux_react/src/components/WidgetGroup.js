import React from 'react';
import WidgetContainer from './WidgetContainer';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import Tooltip from '@mui/material/Tooltip';
import JenMenu from './JenMenu';
import JenSlider from './JenSlider';
import JenSwitch from './JenSwitch';
import JenDirection8 from './JenDirection8';
import JenDirection4 from './JenDirection4';

function WidgetGroup({ panelSize, json, onChange }) {

  const renderWidget = ( name ) => {  // name of widget
    let widgetComponent;
    let widget;
    let height = 60;

    const widgetJSON = window.Module.get_widget_JSON( name );
    // console.log( "WidgetGroup renderWidget widgetJSON=" + widgetJSON );
    try {
      widget = JSON.parse( widgetJSON );
      // console.log( "WidgetGroup renderWidget widget=" + JSON.stringify( widget ) );
    } catch (error) {
      console.error("renderWidget Error parsing JSON:", error);
      // Handle the error appropriately
    }

    if (!widget) {
      console.error(`No widget found for name: ${name}`);
    }
    // console.log( "renderWidget: type = ", widget.type )
    switch ( widget.type ) {
      case 'menu_int':
      case 'menu_string':
        widgetComponent = < JenMenu key = { widget.name } json = { widget } width = { panelSize - 40 } onChange = { onChange } />;
      break;
      case 'slider_int':
      case 'slider_float':
      case 'range_slider_int':
      case 'range_slider_float':
        widgetComponent =       
          <Stack spacing={-0.5} direction="column" alignItems="center">
            <Tooltip title={widget.description ?? ''} placement="top" disableInteractive >
              <Typography variant="subtitle1" component="div">
                {widget.label}
              </Typography>
            </Tooltip>
            <JenSlider key = { widget.name } json = { widget } width = { panelSize - 75 } />
          </Stack>;
      break;
      case 'switch_fn':
      case 'switch_condition':
        height = 30;
        widgetComponent =           
          <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
            <JenSwitch key={widget.name} json={widget} size = { "small" } onChange = { onChange } />
            <Tooltip title={widget.description ?? ''} placement="top" disableInteractive >
              <Typography variant="subtitle1" component="div">
                {widget.label}
              </Typography>
            </Tooltip>
          </Stack>;
      break;
      case 'direction_picker_8':
        widgetComponent =           
          <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
            <JenDirection8 key={widget.name} json={widget} />
            <Tooltip title={widget.description ?? ''} placement="top" disableInteractive >
              <Typography variant="subtitle1" component="div">
                {widget.label}
              </Typography>
            </Tooltip>
          </Stack>;
      break;
      case 'direction_picker_4':
        widgetComponent =           
          <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
            <JenDirection4 key={widget.name} json={widget} />
            <Tooltip title={widget.description ?? ''} placement="top"disableInteractive  >
              <Typography variant="subtitle1" component="div">
                {widget.label}
              </Typography>
            </Tooltip>
          </Stack>;
      break;
      case 'widget_switch_fn':
        let ws_widget;
        const ws_widgetJSON = window.Module.get_widget_JSON( widget.widget );
        // console.log( "WidgetGroup renderWidget widgetJSON=" + ws_widgetJSON );
        try {
          ws_widget = JSON.parse( ws_widgetJSON );
          // console.log( "WidgetGroup renderWidget widget=" + JSON.stringify( widget ) );
        } catch (error) {
          console.error("Error parsing JSON:", error);
          // Handle the error appropriately
        }
        const switcherJSON = window.Module.get_widget_JSON( widget.switcher );
        //console.log( "WidgetGroup renderWidget switcherJSON=" + switcherJSON );
        let switcher;
        try {
          switcher = JSON.parse( switcherJSON );
          // console.log( "WidgetGroup renderWidget switcher=" + JSON.stringify( switcher ) );
        } catch (error) {
          console.error("Error parsing JSON:", error);
          // Handle the error appropriately
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
                <Tooltip title={widget.description ?? ''} placement="top" disableInteractive >
                  <Typography variant="subtitle1" component="div">
                    {widget.label}
                  </Typography>
                </Tooltip>
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

//      console.log( window.Module.get_widget_JSON( "edge_block_switch" ) );

    return (
      <WidgetContainer key = { widget.name } panelSize = { panelSize } height = { height } >
        { widgetComponent }
      </WidgetContainer>
    );
  };

  return (
    <div>
      { json.widgets.map( renderWidget ) }
    </div>
  );
}
  
export default WidgetGroup;
