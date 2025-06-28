import React, { createContext, useState, useContext, useEffect, useCallback } from 'react';

const SceneContext = createContext({
    scenes: [],
    currentSceneIndex: 0,
    isLoading: false,
    changeScene: () => {},
    onSceneChange: () => {}
});

export function SceneProvider({ children }) {
    const [scenes, setScenes] = useState([]);
    const [currentSceneIndex, setCurrentSceneIndex] = useState(0);
    const [isLoading, setIsLoading] = useState(false);
    const [sceneChangeTrigger, setSceneChangeTrigger] = useState(0);

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

    const onSceneChange = useCallback((callback) => {
        // Simple callback system using useEffect in the component that subscribes
        return () => {
            // Cleanup function - no need to track callbacks
        };
    }, []);

    const changeScene = useCallback((index) => {
        if (!scenes[index]) return;

        setIsLoading(true);
        setCurrentSceneIndex(index);

        try {
            window.module.load_scene(scenes[index].filename);

            setTimeout(() => {
                setIsLoading(false);
                // Trigger scene change notification
                setSceneChangeTrigger(prev => prev + 1);
            }, 300);
        } catch (error) {
            console.error("Error changing scene:", error);
            setIsLoading(false);
        }
    }, [scenes]);

    return (
        <SceneContext.Provider
            value={{
                scenes,
                currentSceneIndex,
                isLoading,
                changeScene,
                onSceneChange,
                sceneChangeTrigger
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