import React, {
  createContext,
  useState,
  useContext,
  useEffect,
  useCallback,
} from "react";

const SceneContext = createContext({
  scenes: [],
  currentSceneIndex: 0,
  isLoading: false,
  changeScene: () => {},
});

export function SceneProvider({ children }) {
  const [scenes, setScenes] = useState([]);
  const [currentSceneIndex, setCurrentSceneIndex] = useState(0);
  const [isLoading, setIsLoading] = useState(false);

  useEffect(() => {
    if (window.module) {
      loadSceneList();
    } else {
      const intervalId = setInterval(() => {
        if (window.module) {
          loadSceneList();
          clearInterval(intervalId);
        }
      }, 100);
      return () => clearInterval(intervalId);
    }
  }, []);

  const loadSceneList = useCallback(() => {
    try {
      const sceneListJSON = JSON.parse(window.module.get_scene_list_JSON());
      setScenes(sceneListJSON.scenes || []);
    } catch (error) {
      console.error("Error loading scene list:", error);
    }
  }, []);

  const sceneWithIcons = (sceneList) => {};

  const changeScene = useCallback(
    (index) => {
      if (!scenes[index]) return;

      setIsLoading(true);
      setCurrentSceneIndex(index);

      try {
        window.module.load_scene(scenes[index].filename);

        setTimeout(() => {
          setIsLoading(false);
        }, 300);
      } catch (error) {
        console.error("Error changing scene:", error);
        setIsLoading(false);
      }
    },
    [scenes]
  );

  return (
    <SceneContext.Provider
      value={{
        scenes,
        setScenes,
        currentSceneIndex,
        isLoading,
        changeScene,
      }}
    >
      {children}
    </SceneContext.Provider>
  );
}

export const useScene = () => {
  const context = useContext(SceneContext);
  if (context === undefined) {
    throw new Error("useScene must be used within a SceneProvider");
  }
  return context;
};
