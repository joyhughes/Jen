import Box from '@mui/material/Box';
import InterfaceContainer from '../components/InterfaceContainer';


function Home() {
  const imgWidth = 1600; // Replace with your image width
  const imgHeight = 900; // Replace with your image height
  const ratio = imgWidth / imgHeight;

  let panelSize = 180;

  return (
    <div className="App">

      <InterfaceContainer ratio={ratio} panelSize={panelSize} />        

    </div>
  );
}

export default Home;

/*
                    <MediaController />
                    <ControlPanel />
                    <SceneView />
*/
/*
        <Box 
          sx={{ 
            maxWidth: '100%',          // Ensure the canvas never exceeds screen width
            padding: 1
          }}
        >
          <ImagePort ratio={ratio} panelSize={panelWidth} />        
        </Box>
*/