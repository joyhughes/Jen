import React, { useState, useEffect } from "react";
import Box from '@mui/material/Box';
import ImagePort from './ImagePort';  
import ControlPanel from './ControlPanel';

function InterfaceContainer( { ratio, panelSize } ) {

    const [ isRowDirection, setIsRowDirection ] = useState( true );

    const resizeBox = () => {
        let windowRatio, isRowDirection;
    
        windowRatio = window.innerWidth / window.innerHeight;
        if( windowRatio > ratio ) {
          isRowDirection = true;
        }
        else {
          isRowDirection = false;
        }
    
        setIsRowDirection( isRowDirection );
      };
    
      useEffect(() => {
        window.addEventListener( "resize", resizeBox );
        resizeBox();
        return () => window.removeEventListener( "resize", resizeBox );
      }, [ ratio, panelSize ] );

    return(
        <Box
            sx={{
            display: 'flex',
            flexDirection: isRowDirection ? 'row' : 'column',
            flexWrap: 'wrap',
            width: '100vw',
            height: '100vh',
            position: 'absolute',
            alignContent: 'flex-start',
            top: 0,
            left: 0,
            }}
        >
          < ImagePort ratio={ ratio } panelSize={ panelSize } />        

          < ControlPanel ratio={ ratio } panelSize={ panelSize } />

        </Box>
    );
}

export default InterfaceContainer;