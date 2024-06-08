import InterfaceContainer from "../components/InterfaceContainer";
import Setting from "../components/SettingBtn/Setting";
function Home() {
  const imgWidth = 512; // Replace with your image width
  const imgHeight = 512; // Replace with your image height
  const ratio = imgWidth / imgHeight;

  let panelSize = 180;

  return (
    <div
      style={{ height: "100vh", width: "100%",overflow: "hidden" }}
    >
      <Setting panelSize={panelSize} />
      <InterfaceContainer ratio={ratio} panelSize={panelSize} />
    </div>
  );
}

export default Home;


