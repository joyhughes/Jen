import InterfaceContainer from "../components/InterfaceContainer";

function Home() {
  let panelSize = 180; // how to change panelSize?

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


