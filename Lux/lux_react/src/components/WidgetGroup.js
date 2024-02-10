import React from 'react';
import WidgetContainer from './WidgetContainer';
import JenMenu from './JenMenu';
import JenSlider from './JenSlider';
import JenSwitch from './JenSwitch';

function WidgetGroup({ panelSize, json }) {
    const renderWidget = ( widget ) => {
      let widgetComponent;
  
      switch ( widget. type ) {
        case 'menu_int':
        case 'menu_string':
          widgetComponent = <JenMenu key = { widget.name } json = { widget } />;
          break;
        case 'slider_int':
        case 'slider_float':
          widgetComponent = <JenSlider key = { widget.name } json = { widget } width = { panelSize - 80 } />;
          break;
        case 'switch_fn':
        case 'switch_condition':
          widgetComponent = <JenSwitch key = { widget.name } json = { widget } />;
          break;
        default:
          widgetComponent = <div key={ widget.name }>Unknown widget type: { widget.type }</div>;
          break;
      }
  
      return (
        <WidgetContainer key={ widget.name } panelSize = { panelSize }>
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
