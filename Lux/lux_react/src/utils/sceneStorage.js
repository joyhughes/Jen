const SCENE_CONFIG_PREFIX = 'lux_scene_config_';
const SCENE_CONFIG_VERSION = '1.0';

export class SceneStorage {
    static saveSceneConfig(sceneName, config) {
        try {
            const storageKey = `${SCENE_CONFIG_PREFIX}${sceneName}`;
            const data = {
                version: SCENE_CONFIG_VERSION,
                timestamp: Date.now(),
                config: config
            };
            localStorage.setItem(storageKey, JSON.stringify(data));
            console.log(`Scene config saved for: ${sceneName}`);
            return true;
        } catch (error) {
            console.error('Failed to save scene config:', error);
            return false;
        }
    }

    static loadSceneConfig(sceneName) {
        try {
            const storageKey = `${SCENE_CONFIG_PREFIX}${sceneName}`;
            const data = localStorage.getItem(storageKey);
            if (!data) {
                console.log(`No saved config found for: ${sceneName}`);
                return null;
            }
            
            const parsed = JSON.parse(data);
            if (parsed.version !== SCENE_CONFIG_VERSION) {
                console.warn(`Scene config version mismatch for ${sceneName}, using default`);
                return null;
            }
            
            console.log(`Scene config loaded for: ${sceneName}`);
            return parsed.config;
        } catch (error) {
            console.error('Failed to load scene config:', error);
            return null;
        }
    }

    static listSavedConfigs() {
        const configs = [];
        try {
            for (let i = 0; i < localStorage.length; i++) {
                const key = localStorage.key(i);
                if (key && key.startsWith(SCENE_CONFIG_PREFIX)) {
                    const sceneName = key.replace(SCENE_CONFIG_PREFIX, '');
                    configs.push(sceneName);
                }
            }
        } catch (error) {
            console.error('Failed to list saved configs:', error);
        }
        return configs;
    }

    static deleteSceneConfig(sceneName) {
        try {
            const storageKey = `${SCENE_CONFIG_PREFIX}${sceneName}`;
            localStorage.removeItem(storageKey);
            console.log(`Scene config deleted for: ${sceneName}`);
            return true;
        } catch (error) {
            console.error('Failed to delete scene config:', error);
            return false;
        }
    }

    static clearAllConfigs() {
        try {
            const keysToRemove = [];
            for (let i = 0; i < localStorage.length; i++) {
                const key = localStorage.key(i);
                if (key && key.startsWith(SCENE_CONFIG_PREFIX)) {
                    keysToRemove.push(key);
                }
            }
            
            keysToRemove.forEach(key => localStorage.removeItem(key));
            console.log(`Cleared ${keysToRemove.length} scene configs`);
            return true;
        } catch (error) {
            console.error('Failed to clear all configs:', error);
            return false;
        }
    }

    static getConfigInfo(sceneName) {
        try {
            const storageKey = `${SCENE_CONFIG_PREFIX}${sceneName}`;
            const data = localStorage.getItem(storageKey);
            if (!data) return null;
            
            const parsed = JSON.parse(data);
            return {
                version: parsed.version,
                timestamp: parsed.timestamp,
                size: data.length
            };
        } catch (error) {
            console.error('Failed to get config info:', error);
            return null;
        }
    }

    static getConfigMetadata(sceneName) {
        const info = this.getConfigInfo(sceneName);
        if (!info) return null;
        
        return {
            size: info.size,
            version: info.version,
            lastModified: new Date(info.timestamp).toLocaleDateString(),
            name: sceneName,
            timestamp: info.timestamp
        };
    }

    static getStorageInfo() {
        let totalSize = 0;
        let configCount = 0;
        
        try {
            for (let i = 0; i < localStorage.length; i++) {
                const key = localStorage.key(i);
                if (key && key.startsWith(SCENE_CONFIG_PREFIX)) {
                    const data = localStorage.getItem(key);
                    if (data) {
                        configCount++;
                        totalSize += data.length;
                    }
                }
            }
        } catch (error) {
            console.error('Failed to get storage info:', error);
        }
        
        return {
            totalSize: totalSize,
            configCount: configCount,
            formattedSize: this.formatBytes(totalSize)
        };
    }

    static formatBytes(bytes) {
        if (bytes === 0) return '0 Bytes';
        const units = ['B', 'KB', 'MB', 'GB', 'TB'];
        const index = Math.floor(Math.log(bytes) / Math.log(1024));
        return (bytes / Math.pow(1024, index)).toFixed(2) + ' ' + units[index];
    }


    static hasConfigForScene(sceneName) {
        const storageKey = `${SCENE_CONFIG_PREFIX}${sceneName}`;
        return localStorage.getItem(storageKey) !== null;
    }
}