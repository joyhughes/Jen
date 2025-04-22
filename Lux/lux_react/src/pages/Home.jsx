import React, { useState, useEffect } from 'react';
import InterfaceContainer from "../components/InterfaceContainer";

function Home() {
  // Calculate initial panel size based on screen dimensions
  const [panelSize, setPanelSize] = useState(
      Math.max(360, Math.min(window.innerWidth * 0.3, 500))
  );

  // Update panel size if window is resized
  useEffect(() => {
    const handleResize = () => {
      setPanelSize(Math.max(360, Math.min(window.innerWidth * 0.3, 500)));
    };

    window.addEventListener('resize', handleResize);
    return () => window.removeEventListener('resize', handleResize);
  }, []);

  return (
      <div
          className="App"
          style={{ height: "100vh", width: "100vw", overflow: "hidden" }}
      >
        <InterfaceContainer panelSize={panelSize} />
      </div>
  );
}

export default Home;