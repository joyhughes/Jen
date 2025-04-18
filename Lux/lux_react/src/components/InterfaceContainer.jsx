import React, { useEffect, useState, useCallback } from "react";
import { Box, useMediaQuery } from "@mui/material";
import ControlPanel from "./ControlPanel";
import ImagePort from "./ImagePort";

function InterfaceContainer({ panelSize }) {
    const isSmallScreen = useMediaQuery('(max-width:600px)');
    const isNarrowHeight = useMediaQuery('(max-height:500px)');
    const [moduleReady, setModuleReady] = useState(false);
    const [imagePortDimensions, setImagePortDimensions] = useState({ width: 0, height: 0 });
    const [controlPanelDimensions, setControlPanelDimensions] = useState({ width: 0, height: 0 });

    const resizeBox = useCallback(() => {
        let bufWidth = 512;
        let bufHeight = 512;

        if (window.module) {
            bufWidth = window.module.get_buf_width();
            bufHeight = window.module.get_buf_height();
        }

        const ratio = bufWidth / bufHeight;
        const windowWidth = window.innerWidth;
        const windowHeight = window.innerHeight;

        // Narrow height layout - image left, controls use remaining space
        if (isNarrowHeight && !isSmallScreen) {
            const imageWidth = Math.min(windowHeight * ratio, windowWidth * 0.5);
            const imageHeight = imageWidth / ratio;

            setImagePortDimensions({
                width: imageWidth,
                height: imageHeight
            });

            setControlPanelDimensions({
                width: windowWidth - imageWidth,
                height: windowHeight
            });
        }
        // Small screen (mobile) - image on top, controls below
        else if (isSmallScreen) {
            const imageWidth = windowWidth;
            const imageHeight = Math.min(imageWidth / ratio, windowHeight * 0.5);

            setImagePortDimensions({
                width: imageWidth,
                height: imageHeight
            });

            setControlPanelDimensions({
                width: windowWidth,
                height: windowHeight - imageHeight
            });
        }
        // Normal desktop layout - image left, controls right
        else {
            const controlWidth = Math.min(430, windowWidth * 0.4);
            const availableWidth = windowWidth - controlWidth;
            const imageHeight = Math.min(availableWidth / ratio, windowHeight);
            const imageWidth = imageHeight * ratio;

            setImagePortDimensions({
                width: imageWidth,
                height: imageHeight
            });

            setControlPanelDimensions({
                width: windowWidth - imageWidth,
                height: windowHeight
            });
        }
    }, [isSmallScreen, isNarrowHeight]);

    // Set up resize callback
    useEffect(() => {
        if (window.module) {
            window.module.set_resize_callback(resizeBox);
            setModuleReady(true);
        } else {
            const intervalId = setInterval(() => {
                if (window.module) {
                    clearInterval(intervalId);
                    window.module.set_resize_callback(resizeBox);
                    setModuleReady(true);
                }
            }, 100);
            return () => clearInterval(intervalId);
        }
    }, [resizeBox]);

    // Handle window resize
    useEffect(() => {
        window.addEventListener("resize", resizeBox);
        resizeBox();
        return () => window.removeEventListener("resize", resizeBox);
    }, [resizeBox]);

    return (
        <Box
            sx={{
                display: "flex",
                flexDirection: isSmallScreen ? "column" : "row",
                width: "100vw",
                height: "100vh",
                overflow: "hidden",
                justifyContent: "flex-start"
            }}
        >
            <Box sx={{
                display: 'flex',
                justifyContent: 'center',
                alignItems: 'center',
                width: isSmallScreen ? '100%' : imagePortDimensions.width,
                height: isSmallScreen ? 'auto' : '100%',
                backgroundColor: '#222',
            }}>
                <ImagePort
                    dimensions={imagePortDimensions}
                    moduleReady={moduleReady}
                />
            </Box>

            <ControlPanel
                dimensions={controlPanelDimensions}
                panelSize={panelSize}
                isSmallScreen={isSmallScreen}
                isNarrowHeight={isNarrowHeight}
            />
        </Box>
    );
}

export default InterfaceContainer;