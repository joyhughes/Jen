import { useState, useEffect, useCallback } from 'react';

// Singleton instance
let moduleManagerInstance = null;

class JenModuleManager {
    constructor() {
        this.modulePromise = null;
        this.isReady = false;

        // start initializing immediately when the class instantiated
        this.initializeModule();
    }

    initializeModule() {
        if (this.modulePromise) {
            return this.modulePromise;
        }

        this.modulePromise = new Promise((resolve, reject) => {

            const timeoutId = setTimeout(() => {
                reject(new Error('Module initialization timed out'));
            }, 20000); // 20 seconds


            // check if module is already available
            if (this.isModuleOperational()) {
                clearTimeout(timeoutId);
                this.isReady = true;
                resolve(window.module);
                return;
            }

            // set up polling with exponential backoff
            let checkInterval = 50;
            const maxInterval = 1000;

            const checkModule = () => {
                if (this.isModuleOperational()) {
                    clearTimeout(timeoutId);
                    this.isReady = true;
                    resolve(window.module);
                    return;
                }

                checkInterval = Math.min(checkInterval * 1.2, maxInterval);
                setTimeout(checkModule, checkInterval);
            }

            checkModule();
        })
        return this.modulePromise;
    }

    isModuleOperational() {
        try {
            if (!window.module) {
                return false;
            }

            const criticalFunctions = ['get_panel_JSON', 'is_widget_group_active', 'set_slider_value', 'set_scene_callback'];
            for(const funcName of criticalFunctions) {
                if (typeof window.module[funcName] !== 'function') {
                    return false;
                }
            }

            // check if the function is there but module is not ready yet
            try {
                window.module.get_panel_JSON();
                return true;
            } catch(error) {
                return false;
            }
        } catch (error) {
            console.error('Module is not operational:', error);
            return false;
        }
    }

    async getModule() {
        if (this.modulePromise) {
            return this.initializeModule();
        }

        return this.initializeModule();
    }

    isModuleReady() {
        return this.isReady && this.isModuleOperational();
    }

    callModuleFunction(funcName, ...args) { 
        if (!this.isModuleReady()) {
            throw new Error('Module is not ready');
        }

        if (typeof window.module[funcName] !== 'function') {
            throw new Error(`Function ${funcName} is not available on the module`);
        }

        return window.module[funcName](...args);
    }

    // Static method to get singleton instance
    static getInstance() {
        if (!moduleManagerInstance) {
            moduleManagerInstance = new JenModuleManager();
        }
        return moduleManagerInstance;
    }
}

export const useJenModule = () => {
    const [isReady, setIsReady] = useState(false);
    const [isLoading, setIsLoading] = useState(true);
    const [error, setError] = useState(null);

    useEffect(() => {
        let isMounted = true;

        const initializeModule = async () => {
            try {
                setIsLoading(true);
                setError(null);

                const manager = JenModuleManager.getInstance();
                const module = await manager.getModule();
                
                if (isMounted) {
                    setIsReady(true);
                    setIsLoading(false);
                }
            } catch(error) {
                if (isMounted) {
                    setError(error.message || 'Module initialization failed');
                    setIsLoading(false);
                }
            }
        };

        initializeModule();

        return () => {
            isMounted = false;
        }
    }, [])

    const callModuleFunction = useCallback(async (funcName, ...args) => {
        if (!isReady) {
            throw new Error('Module is not ready');
        }
        
        const manager = JenModuleManager.getInstance();
        return manager.callModuleFunction(funcName, ...args);
    }, [isReady])

    return {
        isReady,
        isLoading,
        error,
        callModuleFunction
    }
}


