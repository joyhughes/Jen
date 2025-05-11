import React, {useEffect, useRef, useState} from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Divider from '@mui/material/Divider';
import Masonry from 'react-masonry-css';
import WidgetGroup from '../WidgetGroup';

function HomePane({ dimensions, panelSize, panelJSON, activeGroups, onWidgetGroupChange }) {
    const containerRef = useRef(null);
    const [containerWidth, setContainerWidth] = useState(0);

    // Use ResizeObserver to detect actual width changes
    useEffect(() => {
        if (!containerRef.current) return;

        const resizeObserver = new ResizeObserver(entries => {
            for (let entry of entries) {
                setContainerWidth(entry.contentRect.width);
            }
        });

        resizeObserver.observe(containerRef.current);

        return () => {
            if (containerRef.current) {
                resizeObserver.unobserve(containerRef.current);
            }
        };
    }, []);

    // Calculate responsive breakpoints for the group masonry layout
    const getBreakpointColumns = () => {
        // Minimum width for a group column
        const MIN_GROUP_WIDTH = 300;

        // If we have the actual container width, use it, otherwise use panelSize
        const availableWidth = containerWidth || panelSize;

        // Calculate how many columns can fit
        const maxColumns = Math.max(1, Math.floor(availableWidth / MIN_GROUP_WIDTH));

        // Create breakpoint object for Masonry
        const breakpoints = {
            default: maxColumns,
            900: Math.min(maxColumns, 2),  // Max 2 columns on medium screens
            600: 1                         // Single column on small screens
        };

        console.log("Home group breakpoints:", breakpoints, "Available width:", availableWidth);
        return breakpoints;
    };

    // Filter out any group that has image picker widgets for the Home pane
    const nonImageGroups = activeGroups.filter(group => {
        // Skip source/target image groups entirely
        if (group.name.toLowerCase().includes('source') ||
            group.name.toLowerCase().includes('target') ||
            group.name === 'SOURCE_IMAGE_GROUP' ||
            group.name === 'TARGET_IMAGE_GROUP') {
            return false;
        }

        // For other groups, check if they have image pickers
        if (group.widgets) {
            const hasImageWidget = group.widgets.some(widgetName => {
                try {
                    const widgetJSON = window.module?.get_widget_JSON(widgetName);
                    const widget = JSON.parse(widgetJSON);
                    return widget?.tool === 'image';
                } catch (error) {
                    return false;
                }
            });

            return !hasImageWidget;
        }

        return true;
    });
         return (
           <Box
             ref={containerRef}
             sx={{
               display: 'flex',
               flexDirection: 'column',
               padding: 1,
               height: '100%',
               overflowY: 'auto',
               width: '100%'
             }}
           >
             <Divider sx={{ mb: 2 }} />
    

           {nonImageGroups.length > 0 ? (
              // First compute the breakpoints & panelWidth once
              (() => {
                const breakpoints = getBreakpointColumns();
                const panelWidth = (containerWidth || panelSize) / breakpoints.default;
                // Flatten all of the widgets in all of your groups:
                const singletonWidgets = nonImageGroups.flatMap(group =>
                  group.widgets.map(widgetName => (
                    <WidgetGroup
                      key={widgetName}
                      json={{ widgets: [widgetName] }}
                      panelSize={panelWidth}
                      onChange={onWidgetGroupChange}
                      disableImageWidgets={true}
                    />
                  ))
                );
    
                return (
                  <Masonry
                    breakpointCols={breakpoints}
                    className="widget-masonry-grid"
                    columnClassName="widget-masonry-column"
                  >
                    {singletonWidgets}
                  </Masonry>
                );
              })()
            ) : (
               <Box sx={{ textAlign: 'center', py: 4, color: 'text.secondary' }}>
                 <Typography>
                   No active widget groups available. Select a scene to begin.
                 </Typography>
               </Box>
             )}
           </Box>
         );
    
}

export default HomePane;