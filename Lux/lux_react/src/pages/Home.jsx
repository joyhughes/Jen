import InterfaceContainer from "../components/InterfaceContainer";
import {THUMB_SIZE} from "../components/ThumbnailItem.jsx";

function Home() {
  let panelSize = THUMB_SIZE * 3; // how to change panelSize?

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


