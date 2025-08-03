const SCENE_CONFIG_PREFIX = 'lux_scene_config_';
const SCENE_CONFIG_VERSION = '2.0'; // Updated version for complete scene state

export class SceneStorage {
    // Save complete scene state using the new C++ backend
    static async saveSceneConfig(sceneName, config = null) {
        try {
            if (!window.module || typeof window.module.save_scene_to_file !== 'function') {
                console.error('Scene save function not available');
                return false;
            }

            // Generate a filename for the scene state
            const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
            const filename = `${sceneName}_${timestamp}.json`;
            
            // Call the C++ save function
            window.module.save_scene_to_file(filename);
            
            // Also save to localStorage for backup and metadata
            const storageKey = `${SCENE_CONFIG_PREFIX}${sceneName}`;
            const data = {
                version: SCENE_CONFIG_VERSION,
                timestamp: Date.now(),
                filename: filename,
                sceneName: sceneName,
                config: config || {} // Keep backward compatibility
            };
            localStorage.setItem(storageKey, JSON.stringify(data));
            
            console.log(`Complete scene state saved for: ${sceneName} as ${filename}`);
            return true;
        } catch (error) {
            console.error('Failed to save complete scene state:', error);
            return false;
        }
    }

    // Load complete scene state using the new C++ backend
    static async loadSceneConfig(sceneName) {
        try {
            if (!window.module || typeof window.module.load_scene_from_file !== 'function') {
                console.error('Scene load function not available');
                return null;
            }

            // Try to find the most recent saved file for this scene
            const storageKey = `${SCENE_CONFIG_PREFIX}${sceneName}`;
            const data = localStorage.getItem(storageKey);
            
            if (data) {
                const parsed = JSON.parse(data);
                if (parsed.version === SCENE_CONFIG_VERSION && parsed.filename) {
                    // Load the complete scene state
                    window.module.load_scene_from_file(parsed.filename);
                    console.log(`Complete scene state loaded for: ${sceneName} from ${parsed.filename}`);
                    return parsed.config; // Return config for backward compatibility
                }
            }
            
            console.log(`No complete scene state found for: ${sceneName}`);
            return null;
        } catch (error) {
            console.error('Failed to load complete scene state:', error);
            return null;
        }
    }

    // Get the complete scene state as JSON (for export/import)
    static async getCompleteSceneState() {
        try {
            if (!window.module || typeof window.module.save_complete_state !== 'function') {
                console.error('Scene state function not available');
                return null;
            }

            // Get the complete state as JSON string from C++
            const stateJsonString = window.module.save_complete_state();
            
            // Parse it to get the actual JSON object
            const stateJson = JSON.parse(stateJsonString);
            return stateJson;
        } catch (error) {
            console.error('Failed to get complete scene state:', error);
            return null;
        }
    }

    // Load complete scene state from JSON (for import)
    static async loadCompleteSceneState(stateJson) {
        try {
            if (!window.module || typeof window.module.load_complete_state !== 'function') {
                console.error('Scene state load function not available');
                return false;
            }

            // Convert JSON object to string for C++ backend
            const stateJsonString = JSON.stringify(stateJson);
            
            // Load the complete state
            window.module.load_complete_state(stateJsonString);
            console.log('Complete scene state loaded from JSON');
            return true;
        } catch (error) {
            console.error('Failed to load complete scene state from JSON:', error);
            return false;
        }
    }

    static listSavedConfigs() {
        const configs = [];
        try {
            for (let i = 0; i < localStorage.length; i++) {
                const key = localStorage.key(i);
                if (key && key.startsWith(SCENE_CONFIG_PREFIX)) {
                    const sceneName = key.replace(SCENE_CONFIG_PREFIX, '');
                    const data = localStorage.getItem(key);
                    if (data) {
                        const parsed = JSON.parse(data);
                        configs.push({
                            sceneName: sceneName,
                            metadata: {
                                timestamp: parsed.timestamp,
                                version: parsed.version,
                                filename: parsed.filename
                            }
                        });
                    }
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
            const data = localStorage.getItem(storageKey);
            
            if (data) {
                const parsed = JSON.parse(data);
                if (parsed.filename) {
                    // Try to delete the file (if the backend supports it)
                    if (window.module && typeof window.module.delete_scene_file === 'function') {
                        window.module.delete_scene_file(parsed.filename);
                    }
                }
            }
            
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
                filename: parsed.filename,
                sceneName: parsed.sceneName
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
            sceneName: info.sceneName,
            savedAt: new Date(info.timestamp).toLocaleString(),
            version: info.version,
            filename: info.filename
        };
    }

    static getStorageInfo() {
        try {
            const configs = this.listSavedConfigs();
            const totalSize = JSON.stringify(configs).length;
            
            return {
                totalConfigs: configs.length,
                totalSize: this.formatBytes(totalSize),
                configs: configs
            };
        } catch (error) {
            console.error('Failed to get storage info:', error);
            return { totalConfigs: 0, totalSize: '0 B', configs: [] };
        }
    }

    static formatBytes(bytes) {
        if (bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    static hasConfigForScene(sceneName) {
        const storageKey = `${SCENE_CONFIG_PREFIX}${sceneName}`;
        return localStorage.getItem(storageKey) !== null;
    }
}