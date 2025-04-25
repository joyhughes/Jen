import {createContext, useContext} from "react";

export const PaneContext = createContext({
    activePane: 'home',
    setActivePane: (pane) => {},
})


export function usePane() {
    return useContext(PaneContext);
}