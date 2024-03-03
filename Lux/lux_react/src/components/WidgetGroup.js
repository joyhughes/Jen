import React from 'react';
import WidgetContainer from './WidgetContainer';
import Stack from '@mui/material/Stack';
import Typography from '@mui/material/Typography';
import JenMenu from './JenMenu';
import JenSlider from './JenSlider';
import JenSwitch from './JenSwitch';
import JenDirection8 from './JenDirection8';
import JenDirection4 from './JenDirection4';

function WidgetGroup({ panelSize, json }) {
    const renderWidget = ( widget ) => {
      let widgetComponent;
      let height = 60;
  
      switch ( widget.type ) {
        case 'menu_int':
        case 'menu_string':
          widgetComponent = <JenMenu key = { widget.name } json = { widget } />;
        break;
        case 'slider_int':
        case 'slider_float':
        case 'range_slider_int':
        case 'range_slider_float':
          widgetComponent =       
            <Stack spacing={-0.5} direction="column" alignItems="center">
              <Typography style={{ textAlign: 'center' }}>
                  {widget.label}
              </Typography>
              <JenSlider key = { widget.name } json = { widget } width = { panelSize - 75 } />
            </Stack>;
        break;
        case 'switch_fn':
        case 'switch_condition':
          height = 30;
          widgetComponent =           
            <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
              <JenSwitch key={widget.name} json={widget} size = { "small" } />
              <Typography variant="subtitle1" component="div">
                {widget.label}
              </Typography>
            </Stack>;
        break;
        case 'direction_picker_8':
          widgetComponent =           
            <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
              <JenDirection8 key={widget.name} json={widget} />
              <Typography variant="subtitle1" component="div">
                {widget.label}
              </Typography>
            </Stack>;
        break;
        case 'direction_picker_4':
          widgetComponent =           
            <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
              <JenDirection4 key={widget.name} json={widget} />
              <Typography variant="subtitle1" component="div">
                {widget.label}
              </Typography>
            </Stack>;
        break;
        case 'widget_switch':
          switch( widget.widget.type ) {
            case 'slider_int':
            case 'slider_float':
            case 'range_slider_int':
            case 'range_slider_float':   
              widgetComponent =       
                <Stack spacing={-0.5} direction="column" alignItems="center">
                  <Stack spacing={1} direction="row" alignItems="center" sx={{ width: '100%', paddingLeft: '0px' }}>
                    <JenSwitch key={widget.switcher.name} json={widget.switcher} size = { "small" } />
                    <Typography variant="subtitle1" component="div">
                      {widget.label}
                    </Typography>
                  </Stack>
                  <JenSlider key = { widget.widget.name } json = { widget.widget } width = { panelSize - 75 } />
                </Stack>;
            break;
            default:
              widgetComponent = <div key={ widget.widget.name }>Unknown widget type: { widget.widget.type }</div>;
            break;
          }
        break;
        default:
          widgetComponent = <div key={ widget.name }>Unknown widget type: { widget.type }</div>;
          break;
      }
  
      console.log( window.Module.get_widget_JSON( "edge_block_switch" ) );

      return (
        <WidgetContainer key={ widget.name } panelSize = { panelSize } height = { height } >
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
