import InterfaceContainer from "../components/InterfaceContainer";

function Home() {
  const imgWidth = 512; // Replace with your image width
  const imgHeight = 512; // Replace with your image height
  const ratio = imgWidth / imgHeight;

  let panelSize = 180;

  return (
    <div
      className="App"
      style={{ height: "100vh", width: "100vw", overflow: "hidden" }}
    >
      <InterfaceContainer ratio={ratio} panelSize={panelSize} />
    </div>
  );
}

export default Home;


